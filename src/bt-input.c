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
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "lib/dbus-common.h"
#include "lib/helpers.h"
#include "lib/bluez-api.h"

static void input_property_changed(Input *input, const gchar *name, const GValue *value, gpointer data)
{
	g_assert(data != NULL);
	GMainLoop *mainloop = data;

	if (g_strcmp0(name, "Connected") == 0) {
		if (g_value_get_boolean(value) == TRUE) {
			g_print("Input service is connected\n");
		} else {
			g_print("Input service is disconnected\n");
		}
		g_main_loop_quit(mainloop);
	}
}

static gchar *adapter_arg = NULL;
static gchar *connect_arg = NULL;
static gchar *disconnect_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter Name or MAC", "<name|mac>"},
	{"connect", 'c', 0, G_OPTION_ARG_STRING, &connect_arg, "Connect to the input device", "<name|mac>"},
	{"disconnect", 'd', 0, G_OPTION_ARG_STRING, &disconnect_arg, "Disconnect from the input device", "<name|mac>"},
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

	context = g_option_context_new("- a bluetooth input manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home page <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if ((!connect_arg || strlen(connect_arg) == 0) && (!disconnect_arg || strlen(disconnect_arg) == 0)) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (!dbus_system_connect(&error)) {
		g_printerr("Couldn't connect to DBus system bus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	/* Check, that bluetooth daemon is running */
	if (!intf_supported(BLUEZ_DBUS_NAME, MANAGER_DBUS_PATH, MANAGER_DBUS_INTERFACE)) {
		g_printerr("%s: bluez service is not found\n", g_get_prgname());
		g_printerr("Did you forget to run bluetoothd?\n");
		exit(EXIT_FAILURE);
	}

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	Device *device = find_device(adapter, connect_arg != NULL ? connect_arg : disconnect_arg, &error);
	exit_if_error(error);

	if (!intf_supported(BLUEZ_DBUS_NAME, device_get_dbus_object_path(device), INPUT_DBUS_INTERFACE)) {
		g_printerr("Input service is not supported by this device\n");
		exit(EXIT_FAILURE);
	}

	GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);

	Input *input = g_object_new(INPUT_TYPE, "DBusObjectPath", device_get_dbus_object_path(device), NULL);
	g_signal_connect(input, "PropertyChanged", G_CALLBACK(input_property_changed), mainloop);

	if (connect_arg) {
		if (input_get_connected(input) == TRUE) {
			g_print("Input service is already connected\n");
		} else {
			input_connect(input, &error);
			exit_if_error(error);
			g_main_loop_run(mainloop);
		}
	} else if (disconnect_arg) {
		if (input_get_connected(input) == FALSE) {
			g_print("Input service is already disconnected\n");
		} else {
			input_disconnect(input, &error);
			exit_if_error(error);
			g_main_loop_run(mainloop);
		}
	}

	g_main_loop_unref(mainloop);
	g_object_unref(input);
	g_object_unref(device);
	g_object_unref(adapter);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

