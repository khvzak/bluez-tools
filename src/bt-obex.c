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

#include "lib/dbus-common.h"
#include "lib/helpers.h"
#include "lib/bluez-api.h"
#include "lib/obexd-api.h"

GHashTable *server_transfers = NULL;
GMainLoop *mainloop = NULL;

/* OBEXTransfer signals */
static void obextransfer_progress(OBEXTransfer *transfer, gint total, gint transfered, gpointer data)
{
	guint pp = (transfered / (gfloat) total)*100;

	static gboolean update_progress = FALSE;
	if (!update_progress) {
		g_print("[OBEXTransfer] Progress: %3u%%", pp);
		update_progress = TRUE;
	} else {
		g_print("\b\b\b\b%3u%%", pp);
	}

	if (pp == 100) {
		g_print("\n");
		update_progress = FALSE;
	}
}

/* OBEXManager signals */
static void obexmanager_session_created(OBEXManager *manager, const gchar *session_path, gpointer data)
{
	g_print("[OBEXManager] FTP session opened\n");
}

static void obexmanager_session_removed(OBEXManager *manager, const gchar *session_path, gpointer data)
{
	g_print("[OBEXManager] FTP session closed\n", session_path);
}

static void obexmanager_transfer_started(OBEXManager *manager, const gchar *transfer_path, gpointer data)
{
	g_print("[OBEXManager] Transfer started\n", transfer_path);

	OBEXTransfer *t = g_object_new(OBEXTRANSFER_TYPE, "DBusObjectPath", transfer_path, NULL);
	g_signal_connect(t, "Progress", G_CALLBACK(obextransfer_progress), NULL);
	g_hash_table_insert(server_transfers, transfer_path, t);
}

static void obexmanager_transfer_completed(OBEXManager *manager, const gchar *transfer_path, gboolean success, gpointer data)
{
	OBEXTransfer *t = g_hash_table_lookup(server_transfers, transfer_path);
	if (t) {
		g_print("[OBEXManager] Transfer %s\n", success == TRUE ? "succeeded" : "failed");
		g_object_unref(t);
		g_hash_table_remove(server_transfers, transfer_path);
	} else {
		// Bug ?
	}
}

/* Async callback for SendFiles() */
/*static void send_files_done(gpointer data)
{
	g_assert(data != NULL);
	GMainLoop *mainloop = data;
	g_main_loop_quit(mainloop);
}*/

/* Main arguments */
static gchar *adapter_arg = NULL;
static gboolean server_arg = FALSE;
static gchar *server_path_arg = NULL;
static gboolean opp_arg = FALSE;
static gchar *opp_device_arg = NULL;
static gchar *opp_file_arg = NULL;
static gchar *ftp_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "<name|mac>"},
	{"server", 's', 0, G_OPTION_ARG_NONE, &server_arg, "Register self at OBEX server", NULL},
	{"opp", 'p', 0, G_OPTION_ARG_NONE, &opp_arg, "Send file to remote device", NULL},
	{"ftp", 'f', 0, G_OPTION_ARG_STRING, &ftp_arg, "Start FTP session with remote device", "<name|mac>"},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();
	dbus_init();

	context = g_option_context_new(" - a bluetooth OBEX client/server");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"Server Options:\n"
			"  -s, --server [<path>]\n"
			"  Register self at OBEX server and use given `path` as OPP save directory\n"
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
	} else if (server_arg && argc != 1 && (argc != 2 || strlen(argv[1]) == 0)) {
		g_print("%s: Invalid arguments for --server\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (opp_arg && (argc != 3 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0)) {
		g_print("%s: Invalid arguments for --opp\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (!dbus_system_connect(&error)) {
		g_printerr("Couldn't connect to dbus system bus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	if (!dbus_session_connect(&error)) {
		g_printerr("Couldn't connect to dbus session bus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	/* Check, that bluetooth daemon is running */
	if (!intf_supported(BLUEZ_DBUS_NAME, MANAGER_DBUS_PATH, MANAGER_DBUS_INTERFACE)) {
		g_printerr("%s: BLUEZ service does not found\n", g_get_prgname());
		g_printerr("Did you forget to run bluetoothd?\n");
		exit(EXIT_FAILURE);
	}

	/* Check, that obexd daemon is running */
	if (!intf_supported(OBEXD_DBUS_NAME, OBEXMANAGER_DBUS_PATH, OBEXMANAGER_DBUS_INTERFACE)) {
		g_printerr("%s: OBEXD service does not found\n", g_get_prgname());
		g_printerr("Did you forget to run obexd?\n");
		exit(EXIT_FAILURE);
	}

	if (server_arg) {
		if (argc == 2) {
			server_path_arg = argv[1];
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

		server_transfers = g_hash_table_new(g_str_hash, g_str_equal);

		OBEXManager *manager = g_object_new(OBEXMANAGER_TYPE, NULL);
		g_signal_connect(manager, "SessionCreated", G_CALLBACK(obexmanager_session_created), NULL);
		g_signal_connect(manager, "SessionRemoved", G_CALLBACK(obexmanager_session_removed), NULL);
		g_signal_connect(manager, "TransferStarted", G_CALLBACK(obexmanager_transfer_started), NULL);
		g_signal_connect(manager, "TransferCompleted", G_CALLBACK(obexmanager_transfer_completed), NULL);

		OBEXAgent *agent = g_object_new(OBEXAGENT_TYPE, "RootFolder", root_dir, NULL);

		g_free(root_dir);

		obexmanager_register_agent(manager, OBEXAGENT_DBUS_PATH, &error);
		exit_if_error(error);

		mainloop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(mainloop);

		/* Waiting for connections... */

		// TODO: Add SIGINT handler

		g_main_loop_unref(mainloop);

		/* Stop active transfers */
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init(&iter, server_transfers);
		while (g_hash_table_iter_next(&iter, &key, &value)) {
			OBEXTransfer *t = OBEXTRANSFER(value);
			obextransfer_cancel(t, NULL);
			g_object_unref(t);
			g_hash_table_iter_remove(&iter);
		}
		g_hash_table_unref(server_transfers);

		obexmanager_unregister_agent(manager, OBEXAGENT_DBUS_PATH, &error);
		g_object_unref(agent);
		g_object_unref(manager);
	} else if (opp_arg) {
		opp_device_arg = argv[1];
		opp_file_arg = argv[2];

		/* Check that `file` is valid and readable */
		{
			struct stat buf;
			if (stat(opp_file_arg, &buf) != 0) {
				g_printerr("%s: %s\n", g_get_prgname(), strerror(errno));
				exit(EXIT_FAILURE);
			}
			if (!S_ISREG(buf.st_mode)) {
				g_printerr("%s: Invalid file: %s\n", g_get_prgname(), opp_file_arg);
				exit(EXIT_FAILURE);
			}
		}
		gchar * files_to_send[] = {NULL, NULL};
		if (!g_path_is_absolute(opp_file_arg)) {
			gchar *current_dir = g_get_current_dir();
			files_to_send[0] = g_build_filename(current_dir, opp_file_arg, NULL);
			g_free(current_dir);
		} else {
			files_to_send[0] = g_strdup(opp_file_arg);
		}

		/* Get source address (address of adapter) */
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);
		gchar *src_address = g_strdup(adapter_get_address(adapter));

		/* Get destination address (address of remote device) */
		Device *device = find_device(adapter, opp_device_arg, &error);
		exit_if_error(error);
		gchar *dst_address = g_strdup(device == NULL ? opp_device_arg : device_get_address(device));

		g_object_unref(device);
		g_object_unref(adapter);

		/* Build arguments */
		GHashTable *device_dict = g_hash_table_new(g_str_hash, g_str_equal);
		GValue source = {0};
		GValue destination = {0};
		g_value_init(&source, G_TYPE_STRING);
		g_value_init(&destination, G_TYPE_STRING);
		g_value_set_string(&source, src_address);
		g_value_set_string(&destination, dst_address);
		g_hash_table_insert(device_dict, "Source", &source);
		g_hash_table_insert(device_dict, "Destination", &destination);

		OBEXClient *client = g_object_new(OBEXCLIENT_TYPE, NULL);
		OBEXAgent *agent = g_object_new(OBEXAGENT_TYPE, NULL);

		mainloop = g_main_loop_new(NULL, FALSE);

		/* Sending file(s) */
		obexclient_send_files_begin(client, send_files_done, mainloop, device_dict, files_to_send, OBEXAGENT_DBUS_PATH);

		g_main_loop_run(mainloop);

		/* Sending files process here ?? */

		// TODO: Add SIGINT handler ??

		g_main_loop_unref(mainloop);

		obexclient_send_files_end(client, &error);
		exit_if_error(error);

		g_object_unref(agent);
		g_object_unref(client);

		g_value_unset(&source);
		g_value_unset(&destination);
		g_hash_table_unref(device_dict);

		g_free(src_address);
		g_free(dst_address);
		g_strfreev(files_to_send);
	} else if (ftp_arg) {
		/* Get source address (address of adapter) */
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);
		gchar *src_address = g_strdup(adapter_get_address(adapter));

		/* Get destination address (address of remote device) */
		Device *device = find_device(adapter, ftp_arg, &error);
		exit_if_error(error);
		gchar *dst_address = g_strdup(device == NULL ? ftp_arg : device_get_address(device));

		g_object_unref(device);
		g_object_unref(adapter);

		/* Build arguments */
		GHashTable *device_dict = g_hash_table_new(g_str_hash, g_str_equal);
		GValue source = {0};
		GValue destination = {0};
		GValue target = {0};
		g_value_init(&source, G_TYPE_STRING);
		g_value_init(&destination, G_TYPE_STRING);
		g_value_init(&target, G_TYPE_STRING);
		g_value_set_string(&source, src_address);
		g_value_set_string(&destination, dst_address);
		g_value_set_string(&target, "FTP");
		g_hash_table_insert(device_dict, "Source", &source);
		g_hash_table_insert(device_dict, "Destination", &destination);
		g_hash_table_insert(device_dict, "Target", &target);

		OBEXClient *client = g_object_new(OBEXCLIENT_TYPE, NULL);
		OBEXAgent *agent = g_object_new(OBEXAGENT_TYPE, NULL);

		/* Create FTP session */
		gchar *session_path = obexclient_create_session(client, device_dict, &error);
		exit_if_error(error);

		OBEXClientFileTransfer *ftp_session = g_object_new(OBEXCLIENT_FILE_TRANSFER_TYPE, "DBusObjectPath", session_path, NULL);

		g_print("FTP session opened\n");

		while (TRUE) {
			g_print("> ");
			gchar cmd[128] = {0,};
			errno = 0;
			if (scanf("%128s", cmd) == EOF && errno) {
				g_warning("%s\n", strerror(errno));
			}

			g_print("cmd: %s\n", cmd);
		}
	}

	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

