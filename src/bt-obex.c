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
#include <glib.h>

#include "lib/bluez-dbus.h"

static gchar *adapter_arg = NULL;
static gchar *server_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "<name|mac>"},
	{"server", 's', 0, G_OPTION_ARG_STRING, &server_arg, "Start OPP/FTP server", "<opp|ftp>"},
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
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!server_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (server_arg && g_strcmp0(server_arg, "opp") != 0 && g_strcmp0(server_arg, "ftp") != 0) {
		g_print("%s: Invalid server type\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (!dbus_connect(&error)) {
		g_printerr("Couldn't connect to dbus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	OBEXManager *obexmanager = g_object_new(OBEXMANAGER_TYPE, NULL);

	if (server_arg) {
		if (g_strcmp0(server_arg, "opp") == 0) {
			gchar *opp_serv_path = obexmanager_create_bluetooth_server(obexmanager, adapter_get_address(adapter), server_arg, FALSE, error);
			exit_if_error(error);

			OBEXServer *serv = g_object_new(OBEXSERVER_TYPE, "DBusObjectPath", opp_serv_path, NULL);
			obexserver_start(serv, "/home/zak/obex_t", TRUE, TRUE, error);
			exit_if_error(error);

			g_free(opp_serv_path);
		} else if (g_strcmp0(server_arg, "ftp") == 0) {

		}

		GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(mainloop);

		g_main_loop_unref(mainloop);
	}

	g_object_unref(obexmanager);
	g_object_unref(adapter);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

