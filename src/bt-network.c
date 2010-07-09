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
#include <glib.h>

#include "lib/bluez-dbus.h"

static void network_property_changed(Network *network, const gchar *name, const GValue *value, gpointer data)
{
	GMainLoop *mainloop = data;
	if (g_strcmp0(name, "Connected") == 0) {
		if (g_value_get_boolean(value) == TRUE) {
			g_print("Network service is connected\n");
		} else {
			g_print("Network service is disconnected\n");
		}
		g_main_loop_quit(mainloop);
	}
}

static gchar *adapter_arg = NULL;
static gboolean connect_arg = FALSE;
static gchar *connect_device_arg = NULL;
static gchar *connect_service_arg = NULL;
static gchar *disconnect_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "adapter#id"},
	{"connect", 'c', 0, G_OPTION_ARG_NONE, &connect_arg, "Connect to a network device", NULL},
	{"disconnect", 'd', 0, G_OPTION_ARG_STRING, &disconnect_arg, "Disconnect from a network device", "device#id"},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("- a bluetooth network manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "network summary");
	g_option_context_set_description(context,
			"Connect Options:\n"
			"  -c, --connect <device#id> <pattern>\n"
			"  Where `pattern` is:\n"
			"     UUID 128 bit string\n"
			"     Profile short name: gn, panu or nap\n"
			"     UUID hexadecimal number\n\n"
			"network desc"
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!connect_arg && !disconnect_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (connect_arg && argc != 3) {
		g_print("%s: Invalid arguments for --connect\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (connect_arg) {
		connect_device_arg = argv[1];
		connect_service_arg = argv[2];
	}

	if (!dbus_connect(&error)) {
		g_printerr("Couldn't connect to dbus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	Device *device = find_device(adapter, connect_device_arg != NULL ? connect_device_arg : disconnect_arg, &error);
	exit_if_error(error);

	// TODO: Test to NETWORK service

	GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);

	Network *network = g_object_new(NETWORK_TYPE, "DBusObjectPath", device_get_dbus_object_path(device), NULL);
	g_signal_connect(network, "PropertyChanged", G_CALLBACK(network_property_changed), mainloop);

	if (connect_arg) {
		if (network_get_connected(network) == TRUE) {
			g_print("Network service is already connected\n");
		} else {
			gchar *intf = network_connect(network, connect_service_arg, &error);
			exit_if_error(error);
			g_main_loop_run(mainloop);
			g_free(intf);
		}
		g_print("Interface: %s\n", network_get_interface(network));
		g_print("UUID: %s\n", network_get_uuid(network));
	} else if (disconnect_arg) {
		if (network_get_connected(network) == FALSE) {
			g_print("Network service is already disconnected\n");
		} else {
			network_disconnect(network, &error);
			exit_if_error(error);
			g_main_loop_run(mainloop);
		}
	}

	g_main_loop_unref(mainloop);
	g_object_unref(network);
	g_object_unref(device);
	g_object_unref(adapter);

	exit(EXIT_SUCCESS);
}

