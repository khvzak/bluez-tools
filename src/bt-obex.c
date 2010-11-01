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

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <glib.h>

#include "lib/dbus-common.h"
#include "lib/helpers.h"
#include "lib/bluez-api.h"
#include "lib/obexd-api.h"

static GHashTable *server_transfers = NULL;
static GMainLoop *mainloop = NULL;

static void sigterm_handler(int sig)
{
	g_message("%s received", sig == SIGTERM ? "SIGTERM" : "SIGINT");

	if (g_main_loop_is_running(mainloop))
		g_main_loop_quit(mainloop);
}

static void agent_released(OBEXAgent *agent, gpointer data)
{
	g_assert(data != NULL);
	GMainLoop *mainloop = data;

	if (g_main_loop_is_running(mainloop))
		g_main_loop_quit(mainloop);
}

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
	g_hash_table_insert(server_transfers, g_strdup(transfer_path), t);
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

	/* Query current locale */
	setlocale(LC_CTYPE, "");

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
		g_printerr("Couldn't connect to DBus system bus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	if (!dbus_session_connect(&error)) {
		g_printerr("Couldn't connect to DBus session bus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	/* Check, that bluetooth daemon is running */
	if (!intf_supported(BLUEZ_DBUS_NAME, MANAGER_DBUS_PATH, MANAGER_DBUS_INTERFACE)) {
		g_printerr("%s: bluez service is not found\n", g_get_prgname());
		g_printerr("Did you forget to run bluetoothd?\n");
		exit(EXIT_FAILURE);
	}

	/* Check, that obexd daemon is running */
	if (!intf_supported(OBEXS_DBUS_NAME, OBEXMANAGER_DBUS_PATH, OBEXMANAGER_DBUS_INTERFACE)) {
		g_printerr("%s: obex service is not found\n", g_get_prgname());
		g_printerr("Did you forget to run obexd?\n");
		exit(EXIT_FAILURE);
	}

	if (server_arg) {
		if (argc == 2) {
			server_path_arg = argv[1];
		}

		/* Check that `path` is valid */
		gchar *root_folder = server_path_arg == NULL ? g_get_current_dir() : g_strdup(server_path_arg);
		if (!is_dir(root_folder, &error)) {
			exit_if_error(error);
		}

		server_transfers = g_hash_table_new(g_str_hash, g_str_equal);

		OBEXManager *manager = g_object_new(OBEXMANAGER_TYPE, NULL);
		g_signal_connect(manager, "SessionCreated", G_CALLBACK(obexmanager_session_created), NULL);
		g_signal_connect(manager, "SessionRemoved", G_CALLBACK(obexmanager_session_removed), NULL);
		g_signal_connect(manager, "TransferStarted", G_CALLBACK(obexmanager_transfer_started), NULL);
		g_signal_connect(manager, "TransferCompleted", G_CALLBACK(obexmanager_transfer_completed), NULL);

		OBEXAgent *agent = g_object_new(OBEXAGENT_TYPE, "RootFolder", root_folder, NULL);

		g_free(root_folder);

		obexmanager_register_agent(manager, OBEXAGENT_DBUS_PATH, &error);
		exit_if_error(error);

		mainloop = g_main_loop_new(NULL, FALSE);

		/* Add SIGTERM && SIGINT handlers */
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = sigterm_handler;
		sigaction(SIGTERM, &sa, NULL);
		sigaction(SIGINT, &sa, NULL);

		g_main_loop_run(mainloop);

		/* Waiting for connections... */

		g_main_loop_unref(mainloop);

		/* Stop active transfers */
		GHashTableIter iter;
		gpointer key, value;
		g_hash_table_iter_init(&iter, server_transfers);
		while (g_hash_table_iter_next(&iter, &key, &value)) {
			OBEXTransfer *t = OBEXTRANSFER(value);
			obextransfer_cancel(t, NULL); // skip errors
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

		/* Check that `file` is valid */
		if (!is_file(opp_file_arg, &error)) {
			exit_if_error(error);
		}

		gchar * files_to_send[] = {NULL, NULL};
		files_to_send[0] = g_path_is_absolute(opp_file_arg) ? g_strdup(opp_file_arg) : get_absolute_path(opp_file_arg);

		/* Get source address (address of adapter) */
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);
		gchar *src_address = g_strdup(adapter_get_address(adapter));

		/* Get destination address (address of remote device) */
		gchar *dst_address = NULL;
		if (g_regex_match_simple("^\\x{2}:\\x{2}:\\x{2}:\\x{2}:\\x{2}:\\x{2}$", opp_device_arg, 0, 0)) {
			dst_address = g_strdup(opp_device_arg);
		} else {
			Device *device = find_device(adapter, opp_device_arg, &error);
			exit_if_error(error);
			dst_address = g_strdup(device_get_address(device));
			g_object_unref(device);
		}

		g_object_unref(adapter);

		/* Build arguments */
		GHashTable *device_dict = g_hash_table_new(g_str_hash, g_str_equal);
		GValue src_v = {0};
		GValue dst_v = {0};
		g_value_init(&src_v, G_TYPE_STRING);
		g_value_init(&dst_v, G_TYPE_STRING);
		g_value_set_string(&src_v, src_address);
		g_value_set_string(&dst_v, dst_address);
		g_hash_table_insert(device_dict, "Source", &src_v);
		g_hash_table_insert(device_dict, "Destination", &dst_v);

		mainloop = g_main_loop_new(NULL, FALSE);

		OBEXClient *client = g_object_new(OBEXCLIENT_TYPE, NULL);

		OBEXAgent *agent = g_object_new(OBEXAGENT_TYPE, NULL);
		g_signal_connect(agent, "AgentReleased", G_CALLBACK(agent_released), mainloop);

		/* Sending file(s) */
		obexclient_send_files(client, device_dict, files_to_send, OBEXAGENT_DBUS_PATH, &error);
		exit_if_error(error);

		/* Add SIGTERM && SIGINT handlers */
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = sigterm_handler;
		sigaction(SIGTERM, &sa, NULL);
		sigaction(SIGINT, &sa, NULL);

		g_main_loop_run(mainloop);

		/* Sending files process here ?? */

		g_main_loop_unref(mainloop);

		g_object_unref(agent);
		g_object_unref(client);

		g_value_unset(&src_v);
		g_value_unset(&dst_v);
		g_hash_table_unref(device_dict);

		g_free(src_address);
		g_free(dst_address);
		g_free(files_to_send[0]);
		files_to_send[0] = NULL;
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
		GValue src_v = {0};
		GValue dst_v = {0};
		GValue target_v = {0};
		g_value_init(&src_v, G_TYPE_STRING);
		g_value_init(&dst_v, G_TYPE_STRING);
		g_value_init(&target_v, G_TYPE_STRING);
		g_value_set_string(&src_v, src_address);
		g_value_set_string(&dst_v, dst_address);
		g_value_set_string(&target_v, "FTP");
		g_hash_table_insert(device_dict, "Source", &src_v);
		g_hash_table_insert(device_dict, "Destination", &dst_v);
		g_hash_table_insert(device_dict, "Target", &target_v);

		OBEXClient *client = g_object_new(OBEXCLIENT_TYPE, NULL);
		OBEXAgent *agent = g_object_new(OBEXAGENT_TYPE, NULL);

		/* Create FTP session */
		gchar *session_path = obexclient_create_session(client, device_dict, &error);
		exit_if_error(error);

		OBEXClientFileTransfer *ftp_session = g_object_new(OBEXCLIENT_FILE_TRANSFER_TYPE, "DBusObjectPath", session_path, NULL);
		g_free(session_path);

		g_print("FTP session opened\n");

		while (TRUE) {
			gchar *cmd = readline("> ");
			if (cmd == NULL) {
				continue;
			} else {
				add_history(cmd);
			}

			gint f_argc;
			gchar **f_argv;
			/* Parsing command line */
			if (!g_shell_parse_argv(cmd, &f_argc, &f_argv, &error)) {
				g_print("%s\n", error->message);
				g_error_free(error);
				error = NULL;

				g_free(cmd);
				continue;
			}

			/* Execute commands */
			if (g_strcmp0(f_argv[0], "cd") == 0) {
				if (f_argc != 2 || strlen(f_argv[1]) == 0) {
					g_print("invalid arguments\n");
				} else {
					obexclient_file_transfer_change_folder(ftp_session, f_argv[1], &error);
					if (error) {
						g_print("%s\n", error->message);
						g_error_free(error);
						error = NULL;
					}
				}
			} else if (g_strcmp0(f_argv[0], "mkdir") == 0) {
				if (f_argc != 2 || strlen(f_argv[1]) == 0) {
					g_print("invalid arguments\n");
				} else {
					obexclient_file_transfer_create_folder(ftp_session, f_argv[1], &error);
					if (error) {
						g_print("%s\n", error->message);
						g_error_free(error);
						error = NULL;
					}
				}
			} else if (g_strcmp0(f_argv[0], "ls") == 0) {
				if (f_argc != 1) {
					g_print("invalid arguments\n");
				} else {
					GPtrArray *folders = obexclient_file_transfer_list_folder(ftp_session, &error);
					if (error) {
						g_print("%s\n", error->message);
						g_error_free(error);
						error = NULL;
					} else {
						for (int i = 0; i < folders->len; i++) {
							GHashTable *el = g_ptr_array_index(folders, i);
							g_print(
									"%s\t%llu\t%s\n",
									g_value_get_string(g_hash_table_lookup(el, "Type")),
									G_VALUE_HOLDS_UINT64(g_hash_table_lookup(el, "Size")) ?
									g_value_get_uint64(g_hash_table_lookup(el, "Size")) :
									0,
									g_value_get_string(g_hash_table_lookup(el, "Name"))
									);
						}
					}

					if (folders) g_ptr_array_unref(folders);

					/*obexclient_remove_session(client, obexclient_file_transfer_get_dbus_object_path(ftp_session), &error);
					exit_if_error(error);
					g_object_unref(ftp_session);
					session_path = obexclient_create_session(client, device_dict, &error);
					exit_if_error(error);
					ftp_session = g_object_new(OBEXCLIENT_FILE_TRANSFER_TYPE, "DBusObjectPath", session_path, NULL);
					g_free(session_path);*/

				}
			} else if (g_strcmp0(f_argv[0], "get") == 0) {
				if (f_argc != 3 || strlen(f_argv[1]) == 0 || strlen(f_argv[2]) == 0) {
					g_print("invalid arguments\n");
				} else {
					gchar *abs_dst_path = get_absolute_path(f_argv[2]);
					gchar *dir = g_path_get_dirname(abs_dst_path);
					if (!is_dir(dir, &error)) {
						g_print("%s\n", error->message);
						g_error_free(error);
						error = NULL;
					} else {
						obexclient_file_transfer_get_file(ftp_session, abs_dst_path, f_argv[1], &error);
						if (error) {
							g_print("%s\n", error->message);
							g_error_free(error);
							error = NULL;
						}
					}
					g_free(dir);
					g_free(abs_dst_path);
				}
			} else if (g_strcmp0(f_argv[0], "put") == 0) {
				if (f_argc != 3 || strlen(f_argv[1]) == 0 || strlen(f_argv[2]) == 0) {
					g_print("invalid arguments\n");
				} else {
					gchar *abs_src_path = get_absolute_path(f_argv[1]);
					if (!is_file(abs_src_path, &error)) {
						g_print("%s\n", error->message);
						g_error_free(error);
						error = NULL;
					} else {
						obexclient_file_transfer_put_file(ftp_session, abs_src_path, f_argv[2], &error);
						if (error) {
							g_print("%s\n", error->message);
							g_error_free(error);
							error = NULL;
						}
					}
					g_free(abs_src_path);
				}
			} else if (g_strcmp0(f_argv[0], "cp") == 0) {
				if (f_argc != 3 || strlen(f_argv[1]) == 0 || strlen(f_argv[2]) == 0) {
					g_print("invalid arguments\n");
				} else {
					obexclient_file_transfer_copy_file(ftp_session, f_argv[1], f_argv[2], &error);
					if (error) {
						g_print("%s\n", error->message);
						g_error_free(error);
						error = NULL;
					}
				}
			} else if (g_strcmp0(f_argv[0], "mv") == 0) {
				if (f_argc != 3 || strlen(f_argv[1]) == 0 || strlen(f_argv[2]) == 0) {
					g_print("invalid arguments\n");
				} else {
					obexclient_file_transfer_move_file(ftp_session, f_argv[1], f_argv[2], &error);
					if (error) {
						g_print("%s\n", error->message);
						g_error_free(error);
						error = NULL;
					}
				}
			} else if (g_strcmp0(f_argv[0], "rm") == 0) {
				if (f_argc != 2 || strlen(f_argv[1]) == 0) {
					g_print("invalid arguments\n");
				} else {
					obexclient_file_transfer_delete(ftp_session, f_argv[1], &error);
					if (error) {
						g_print("%s\n", error->message);
						g_error_free(error);
						error = NULL;
					}
				}
			} else if (g_strcmp0(f_argv[0], "help") == 0) {
				g_print(
						"help\t\t\tShow this message\n"
						"exit\t\t\tClose FTP session\n"
						"cd <folder>\t\tChange the current folder of the remote device\n"
						"mkdir <folder>\t\tCreate a new folder in the remote device\n"
						"ls\t\t\tList folder contents\n"
						"get <src> <dst>\t\tCopy the src file (from remote device) to the dst file (on local filesystem)\n"
						"put <src> <dst>\t\tCopy the src file (from local filesystem) to the dst file (on remote device)\n"
						"cp <src> <dst>\t\tCopy a file within the remote device from src file to dst file\n"
						"mv <src> <dst>\t\tMove a file within the remote device from src file to dst file\n"
						"rm <target>\t\tDeletes the specified file/folder\n"
						);
			} else if (g_strcmp0(f_argv[0], "exit") == 0 || g_strcmp0(f_argv[0], "quit") == 0) {
				obexclient_remove_session(client, obexclient_file_transfer_get_dbus_object_path(ftp_session), &error);
				exit_if_error(error);

				g_strfreev(f_argv);
				g_free(cmd);
				break;
			} else {
				g_print("invalid command\n");
			}

			g_strfreev(f_argv);
			g_free(cmd);
		}

		g_object_unref(agent);
		g_object_unref(client);
		g_object_unref(ftp_session);

		g_value_unset(&src_v);
		g_value_unset(&dst_v);
		g_value_unset(&target_v);
		g_hash_table_unref(device_dict);

		g_free(src_address);
		g_free(dst_address);
	}

	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

