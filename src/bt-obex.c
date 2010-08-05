/*
 *
 *  bluez-tools - a set of tools to manage bluetooth devices for linux
 *
 *  Copyright (C) 2010  Alexander Orlenko <zxteam@gmail.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <glib.h>

#include "lib/bluez-dbus.h"

typedef struct {
	OBEXServer *serv;
	OBEXServerSession *sess;
	gchar *sess_id;

	/* Client address */
	gchar *client_name;
	gchar *client_addr;

	/* Transfer info */
	gchar *filename;
	gchar *local_path;
	guint64 size;
	guint pp; // per sent
} sess_transf_s;

static gchar *server_type = NULL;
static GHashTable *server_sessions = NULL;

/* Main arguments */
static gchar *adapter_arg = NULL;
static gboolean server_arg = FALSE;
static gchar *server_type_arg = NULL;
static gchar *server_path_arg = NULL;
static gboolean opp_arg = FALSE;
static gchar *opp_device_arg = NULL;
static gchar *opp_file_arg = NULL;
static gchar *ftp_arg = NULL;

/* Sessions signals defs */
static void obexserver_session_cancelled(OBEXServerSession *session, gpointer data);
static void obexserver_session_disconnected(OBEXServerSession *session, gpointer data);
static void obexserver_session_transfer_started(OBEXServerSession *session, const gchar *filename, const gchar *local_path, guint64 total_bytes, gpointer data);
static void obexserver_session_transfer_progress(OBEXServerSession *session, guint64 bytes_transferred, gpointer data);
static void obexserver_session_transfer_completed(OBEXServerSession *session, gpointer data);
static void obexserver_session_error_occurred(OBEXServerSession *session, const gchar *error_name, const gchar *error_message, gpointer data);

static void obexsession_cancelled(OBEXSession *session, gpointer data);
static void obexsession_disconnected(OBEXSession *session, gpointer data);
static void obexsession_closed(OBEXSession *session, gpointer data);
static void obexsession_transfer_started(OBEXSession *session, const gchar *filename, const gchar *local_path, guint64 total_bytes, gpointer data);
static void obexsession_transfer_progress(OBEXSession *session, guint64 bytes_transferred, gpointer data);
static void obexsession_transfer_completed(OBEXSession *session, gpointer data);
static void obexsession_error_occurred(OBEXSession *session, const gchar *error_name, const gchar *error_message, gpointer data);

/*
 * OBEXManager signals
 */
static void obexmanager_session_connected(OBEXManager *manager, const gchar *path, gpointer data)
{
	g_print("[OBEXManager] Session connected: %s\n", path);
}

static void obexmanager_session_connect_error(OBEXManager *manager, const gchar *path, const gchar *error_name, const gchar *error_message, gpointer data)
{
	g_print("[OBEXManager] Session connect error: %s:%s\n", path, error_name, error_message);
}

static void obexmanager_session_closed(OBEXManager *manager, const gchar *path, gpointer data)
{
	g_print("[OBEXManager] Session closed: %s\n", path);
}

/*
 * OBEXServer signals
 */
static void obexserver_started(OBEXServer *server, gpointer data)
{
	g_print("[%sServer] Started\n", server_type);
}

static void obexserver_stopped(OBEXServer *server, gpointer data)
{
	g_print("[%sServer] Stopped\n", server_type);
}

static void obexserver_closed(OBEXServer *server, gpointer data)
{
	g_print("[%sServer] Closed\n", server_type);
}

static void obexserver_error_occurred(OBEXServer *server, const gchar *error_name, const gchar *error_message, gpointer data)
{
	g_print("[%sServer] %s:%s\n", server_type, error_name, error_message);
}

static void obexserver_session_created(OBEXServer *server, const gchar *path, gpointer data)
{
	OBEXServerSession *session = g_object_new(OBEXSERVER_SESSION_TYPE, "DBusObjectPath", path, NULL);
	g_signal_connect(session, "Cancelled", G_CALLBACK(obexserver_session_cancelled), NULL);
	g_signal_connect(session, "Disconnected", G_CALLBACK(obexserver_session_disconnected), NULL);
	g_signal_connect(session, "TransferStarted", G_CALLBACK(obexserver_session_transfer_started), NULL);
	g_signal_connect(session, "TransferProgress", G_CALLBACK(obexserver_session_transfer_progress), NULL);
	g_signal_connect(session, "TransferCompleted", G_CALLBACK(obexserver_session_transfer_completed), NULL);
	g_signal_connect(session, "ErrorOccurred", G_CALLBACK(obexserver_session_error_occurred), NULL);

	sess_transf_s *t = g_new0(sess_transf_s, 1);
	t->serv = server;
	t->sess = session;
	t->sess_id = g_path_get_basename(path);

	g_print("[%sServer] Session created: %s\n", server_type, t->sess_id);

	/* Get remote address & name (if possible) */
	GError *error = NULL;
	GHashTable *sess_info = obexserver_get_server_session_info(server, path, &error);
	exit_if_error(error);
	t->client_addr = g_strdup(g_hash_table_lookup(sess_info, "BluetoothAddress"));
	g_hash_table_unref(sess_info);

	Adapter *adapter_t = find_adapter(adapter_arg, &error);
	exit_if_error(error);
	Device *device_t = find_device(adapter_t, t->client_addr, &error);
	exit_if_error(error);
	if (device_t)
		t->client_name = g_strdup(device_get_name(device_t));
	g_object_unref(device_t);
	g_object_unref(adapter_t);

	g_hash_table_insert(server_sessions, g_strdup(path), t);

	if (t->client_name)
		g_print("[%sServer] Client: %s (%s)\n", server_type, t->client_name, t->client_addr);
	else
		g_print("[%sServer] Client: %s\n", server_type, t->client_addr);
}

static void obexserver_session_removed(OBEXServer *server, const gchar *path, gpointer data)
{
	sess_transf_s *t = g_hash_table_lookup(server_sessions, path);
	g_assert(t != NULL);

	g_print("[%sServer] Session removed: %s\n", server_type, t->sess_id);

	/* Transfer free (if exists) */
	g_free(t->filename);
	g_free(t->local_path);
	t->size = 0;
	t->pp = 0;

	/* Session data free */
	g_free(t->sess_id);
	g_free(t->client_addr);
	g_free(t->client_name);
	g_object_unref(t->sess);
	g_free(t);

	g_hash_table_remove(server_sessions, path);
}

/*
 * OBEXServerSession signals
 */
static void obexserver_session_cancelled(OBEXServerSession *session, gpointer data)
{
	sess_transf_s *t = g_hash_table_lookup(server_sessions, obexserver_session_get_dbus_object_path(session));
	g_assert(t != NULL);

	g_print("[%s] Cancelled\n", t->sess_id);
}

static void obexserver_session_disconnected(OBEXServerSession *session, gpointer data)
{
	sess_transf_s *t = g_hash_table_lookup(server_sessions, obexserver_session_get_dbus_object_path(session));
	g_assert(t != NULL);

	g_print("[%s] Disconnected\n", t->sess_id);
}

static void obexserver_session_transfer_started(OBEXServerSession *session, const gchar *filename, const gchar *local_path, guint64 total_bytes, gpointer data)
{
	sess_transf_s *t = g_hash_table_lookup(server_sessions, obexserver_session_get_dbus_object_path(session));
	g_assert(t != NULL);

	GError *error = NULL;

	g_print("[%s] Transfer started:\n", t->sess_id);
	g_print("  Filename: %s\n", filename);
	g_print("  Save path: %s\n", local_path);
	g_print("  Size: %llu bytes\n", total_bytes);

	gchar yn[4] = {0,};
	g_print("Accept (yes/no)? ");
	errno = 0;
	if (scanf("%3s", yn) == EOF && errno) {
		g_warning("%s\n", strerror(errno));
	}
	if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
		obexserver_session_accept(session, &error);
		exit_if_error(error);
	} else {
		obexserver_session_reject(session, &error);
		exit_if_error(error);
		return;
	}

	t->filename = g_strdup(filename);
	t->local_path = g_strdup(local_path);
	t->size = total_bytes;
	t->pp = 0;
}

static void obexserver_session_transfer_progress(OBEXServerSession *session, guint64 bytes_transferred, gpointer data)
{
	sess_transf_s *t = g_hash_table_lookup(server_sessions, obexserver_session_get_dbus_object_path(session));
	g_assert(t != NULL);

	t->pp = (bytes_transferred / (gfloat) t->size)*100;

	static gboolean update_progress = FALSE;
	if (!update_progress) {
		g_print("[%s] Transfer progress: %3u%%", t->sess_id, t->pp);
		update_progress = TRUE;
	} else {
		g_print("\b\b\b\b%3u%%", t->pp);
	}

	if (t->pp == 100) g_print("\n");
}

static void obexserver_session_transfer_completed(OBEXServerSession *session, gpointer data)
{
	sess_transf_s *t = g_hash_table_lookup(server_sessions, obexserver_session_get_dbus_object_path(session));
	g_assert(t != NULL);

	g_print("[%s] Transfer completed\n", t->sess_id);

	g_free(t->filename);
	t->filename = NULL;
	g_free(t->local_path);
	t->local_path = NULL;
	t->size = 0;
	t->pp = 0;
}

static void obexserver_session_error_occurred(OBEXServerSession *session, const gchar *error_name, const gchar *error_message, gpointer data)
{
	sess_transf_s *t = g_hash_table_lookup(server_sessions, obexserver_session_get_dbus_object_path(session));
	g_assert(t != NULL);

	/* If an error occurred during transfer */
	if (t->pp > 0 && t->pp < 100) g_print("\n");

	g_print("[%s] %s:%s\n", t->sess_id, error_name, error_message);
}

/*
 * OBEXSession signals
 */
static void obexsession_cancelled(OBEXSession *session, gpointer data)
{

}

static void obexsession_disconnected(OBEXSession *session, gpointer data)
{

}

static void obexsession_closed(OBEXSession *session, gpointer data)
{

}

static void obexsession_transfer_started(OBEXSession *session, const gchar *filename, const gchar *local_path, guint64 total_bytes, gpointer data)
{

}

static void obexsession_transfer_progress(OBEXSession *session, guint64 bytes_transferred, gpointer data)
{

}

static void obexsession_transfer_completed(OBEXSession *session, gpointer data)
{

}

static void obexsession_error_occurred(OBEXSession *session, const gchar *error_name, const gchar *error_message, gpointer data)
{

}

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "<name|mac>"},
	{"server", 's', 0, G_OPTION_ARG_NONE, &server_arg, "Start OPP/FTP server", NULL},
	{"opp", 'p', 0, G_OPTION_ARG_NONE, &opp_arg, "Send file to remote device", NULL},
	{"ftp", 'f', 0, G_OPTION_ARG_STRING, &ftp_arg, "Start FTP session with remote device", "<name|mac>"},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new(" - a bluetooth OBEX client/server");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"Server Options:\n"
			"  -s, --server <opp|ftp> [<path>]\n"
			"  Start OPP/FTP server and use given `path` as root directory\n"
			"  If `path` does not specified - use current directory\n\n"
			"OPP Options:\n"
			"  -p, --opp <name|mac> <file>\n"
			"  Send `file` to remote device using Object Push Profile\n\n"
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home page <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!server_arg && !opp_arg && (!ftp_arg || strlen(ftp_arg) == 0)) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (server_arg && (argc != 2 || strlen(argv[1]) == 0) && (argc != 3 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0)) {
		g_print("%s: Invalid arguments for --server\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (opp_arg && (argc != 3 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0)) {
		g_print("%s: Invalid arguments for --opp\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (!dbus_connect(&error)) {
		g_printerr("Couldn't connect to dbus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	if (!intf_is_supported(BLUEZ_DBUS_OBEXMANAGER_PATH, OBEXMANAGER_INTF)) {
		g_printerr("%s: OBEX service does not found\n", g_get_prgname());
		g_printerr("Did you forget to run obex-data-server?\n");
		exit(EXIT_FAILURE);
	}

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	OBEXManager *manager = g_object_new(OBEXMANAGER_TYPE, NULL);
	g_signal_connect(manager, "SessionConnected", G_CALLBACK(obexmanager_session_connected), NULL);
	g_signal_connect(manager, "SessionConnectError", G_CALLBACK(obexmanager_session_connect_error), NULL);
	g_signal_connect(manager, "SessionClosed", G_CALLBACK(obexmanager_session_closed), NULL);

	if (server_arg) {
		server_sessions = g_hash_table_new(g_str_hash, g_str_equal);
		server_type_arg = argv[1];
		if (argc == 3) {
			server_path_arg = argv[2];
		}

		/* Check that `path` is valid */
		gchar *root_dir = NULL;
		if (server_path_arg == NULL) {
			root_dir = g_get_current_dir();
		} else {
			struct stat buf;
			if (stat(server_path_arg, &buf) != 0) {
				g_printerr("%s: %s\n", g_get_prgname(), strerror(errno));
				exit(EXIT_FAILURE);
			}
			if (!S_ISDIR(buf.st_mode)) {
				g_printerr("%s: Invalid directory: %s\n", g_get_prgname(), server_path_arg);
				exit(EXIT_FAILURE);
			}
			root_dir = g_strdup(server_path_arg);
		}

		gboolean require_pairing = TRUE;
		gboolean auto_accept = FALSE;

		if (g_strcmp0(server_type_arg, "opp") == 0) {
			require_pairing = FALSE;
			auto_accept = FALSE;
			server_type = "OPP";
		} else if (g_strcmp0(server_type_arg, "ftp") == 0) {
			require_pairing = TRUE;
			auto_accept = TRUE;
			server_type = "FTP";
		} else {
			g_print("%s: Invalid server type: %s\n", g_get_prgname(), server_type_arg);
			g_print("Try `%s --help` for more information.\n", g_get_prgname());
			exit(EXIT_FAILURE);
		}

		gchar *serv_path = obexmanager_create_bluetooth_server(manager, adapter_get_address(adapter), server_type_arg, require_pairing, &error);
		exit_if_error(error);

		OBEXServer *serv = g_object_new(OBEXSERVER_TYPE, "DBusObjectPath", serv_path, NULL);
		g_signal_connect(serv, "Started", G_CALLBACK(obexserver_started), NULL);
		g_signal_connect(serv, "Stopped", G_CALLBACK(obexserver_stopped), NULL);
		g_signal_connect(serv, "Closed", G_CALLBACK(obexserver_closed), NULL);
		g_signal_connect(serv, "ErrorOccurred", G_CALLBACK(obexserver_error_occurred), NULL);
		g_signal_connect(serv, "SessionCreated", G_CALLBACK(obexserver_session_created), NULL);
		g_signal_connect(serv, "SessionRemoved", G_CALLBACK(obexserver_session_removed), NULL);

		gboolean is_started = obexserver_is_started(serv, &error);
		exit_if_error(error);
		if (is_started) {
			g_printerr("%s: Server is already started\n", g_get_prgname());
			exit(EXIT_FAILURE);
		}

		obexserver_start(serv, root_dir, TRUE, auto_accept, &error);
		exit_if_error(error);

		GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(mainloop);

		/* Waiting for connections... */

		g_main_loop_unref(mainloop);
		g_object_unref(serv);
		g_free(serv_path);
		g_free(root_dir);
		g_hash_table_unref(server_sessions);
	} else if (opp_arg) {
		
	}

	g_object_unref(manager);
	g_object_unref(adapter);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

