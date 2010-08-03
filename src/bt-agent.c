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

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "<name|mac>"},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new(" - a bluetooth agent");
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
	}

	g_option_context_free(context);

	if (!dbus_connect(&error)) {
		g_printerr("Couldn't connect to dbus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	Agent *agent = g_object_new(AGENT_TYPE, NULL);

	adapter_register_agent(adapter, DBUS_AGENT_PATH, "DisplayYesNo", &error);
	exit_if_error(error);

	GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	// TODO: Add SIGINT handler (Ctrl+C)

	adapter_unregister_agent(adapter, DBUS_AGENT_PATH, &error);
	exit_if_error(error);

	g_main_loop_unref(mainloop);
	g_object_unref(agent);
	g_object_unref(adapter);
	g_object_unref(manager);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

