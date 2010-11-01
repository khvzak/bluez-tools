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

static gchar *adapter_arg = NULL;
static gboolean connect_arg = FALSE;
static gchar *connect_device_arg = NULL;
static gchar *connect_service_arg = NULL;
static gboolean disconnect_arg = FALSE;
static gchar *disconnect_device_arg = NULL;
static gchar *disconnect_tty_device_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter Name or MAC", "<name|mac>"},
	{"connect", 'c', 0, G_OPTION_ARG_NONE, &connect_arg, "Connect to the serial device", NULL},
	{"disconnect", 'd', 0, G_OPTION_ARG_NONE, &disconnect_arg, "Disconnect from the serial device", NULL},
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

	context = g_option_context_new(" - a bluetooth serial manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"Connect Options:\n"
			"  -c, --connect <name|mac> <pattern>\n"
			"  Where\n"
			"    `name|mac` is a device Name or MAC\n"
			"    `pattern` is:\n"
			"       UUID 128 bit string\n"
			"       Profile short name, e.g: spp, dun\n"
			"       RFCOMM channel, 1-30\n\n"
			"Disconnect Options:\n"
			"  -d, --disconnect <name|mac> <tty_device>\n"
			"  Where\n"
			"    `name|mac` is a device Name or MAC\n"
			"    `tty_device` is a RFCOMM TTY device that has been connected\n\n"
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home page <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!connect_arg && !disconnect_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (connect_arg && (argc != 3 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0)) {
		g_print("%s: Invalid arguments for --connect\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (disconnect_arg && (argc != 3 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0)) {
		g_print("%s: Invalid arguments for --disconnect\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (connect_arg) {
		connect_device_arg = argv[1];
		connect_service_arg = argv[2];
	} else {
		disconnect_device_arg = argv[1];
		disconnect_tty_device_arg = argv[2];
	}

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

	Device *device = find_device(adapter, connect_device_arg != NULL ? connect_device_arg : disconnect_device_arg, &error);
	exit_if_error(error);

	if (!intf_supported(BLUEZ_DBUS_NAME, device_get_dbus_object_path(device), SERIAL_DBUS_INTERFACE)) {
		g_printerr("Serial service is not supported by this device\n");
		exit(EXIT_FAILURE);
	}

	Serial *serial = g_object_new(SERIAL_TYPE, "DBusObjectPath", device_get_dbus_object_path(device), NULL);

	if (connect_arg) {
		gchar *tty_device = serial_connect(serial, connect_service_arg, &error);
		exit_if_error(error);
		g_print("Created RFCOMM TTY device: %s\n", tty_device);
		g_free(tty_device);
	} else if (disconnect_arg) {
		serial_disconnect(serial, disconnect_tty_device_arg, &error);
		exit_if_error(error);
		g_print("Done\n");
	}

	g_object_unref(serial);
	g_object_unref(device);
	g_object_unref(adapter);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

