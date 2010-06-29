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

#include "lib/dbus-common.h"
#include "lib/helpers.h"
#include "lib/adapter.h"
#include "lib/device.h"
#include "lib/manager.h"
#include "lib/agent.h"

static gboolean stop_discovery(gpointer data) {
	g_main_loop_quit(data);
	return FALSE;
}

static void adapter_device_found(Adapter *adapter, const gchar *address, const GHashTable *values, gpointer data)
{
	g_print("device found: %s\n", address);
}

static void adapter_device_disappeared(Adapter *adapter, const gchar *address, gpointer data)
{
	g_print("device disappeared: %s\n", address);
}

static gboolean list_arg = FALSE;
static gchar *adapter_arg = NULL;
static gboolean info_arg = FALSE;
static gboolean discover_arg = FALSE;
static gboolean set_arg = FALSE;

static GOptionEntry entries[] = {
	{ "list", 'l', 0, G_OPTION_ARG_NONE, &list_arg, "List all available adapters", NULL},
	{ "adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "adapter#id"},
	{ "info", 'i', 0, G_OPTION_ARG_NONE, &info_arg, "Show adapter info", NULL},
	{ "discover", 'd', 0, G_OPTION_ARG_NONE, &discover_arg, "Discover remote devices", NULL},
	{ "set", 0, 0, G_OPTION_ARG_NONE, &set_arg, "Set property", NULL},
	{ NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("[--set Name Value] - a bluetooth adapter manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "summary");
	g_option_context_set_description(context, "desc");
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}

	if (!list_arg && !info_arg && !discover_arg && !set_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	}

	g_option_context_free(context);

	if (!dbus_connect(&error)) {
		g_printerr("Couldn't connect to dbus: %s", error->message);
		exit(EXIT_FAILURE);
	}

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	if (list_arg) {
		const GPtrArray *adapters_list = manager_get_adapters(manager);
		g_return_val_if_fail(adapters_list != NULL, EXIT_FAILURE);

		g_print("Available adapters:\n");

		if (adapters_list->len == 0) {
			g_print("no adapters found\n");
		}

		for (int i = 0; i < adapters_list->len; i++) {
			Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", g_ptr_array_index(adapters_list, i), NULL);
			g_print("%s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter));
			g_object_unref(adapter);
		}
	} else if (info_arg) {
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);

		g_print("[%s]\n", g_basename(adapter_get_dbus_object_path(adapter)));
		g_print("  Name: %s [rw]\n", adapter_get_name(adapter));
		g_print("  Address: %s\n", adapter_get_address(adapter));
		g_print("  Class: 0x%x\n", adapter_get_class(adapter));
		g_print("  Discoverable: %d [rw]\n", adapter_get_discoverable(adapter));
		g_print("  DiscoverableTimeout: %d [rw]\n", adapter_get_discoverable_timeout(adapter));
		g_print("  Discovering: %d\n", adapter_get_discovering(adapter));
		g_print("  Pairable: %d [rw]\n", adapter_get_pairable(adapter));
		g_print("  PairableTimeout: %d [rw]\n", adapter_get_pairable_timeout(adapter));
		g_print("  Powered: %d [rw]\n", adapter_get_powered(adapter));
		g_print("  Service(s): [");
		const gchar **uuids = adapter_get_uuids(adapter);
		for (int j = 0; uuids[j] != NULL; j++) {
			if (j > 0) g_print(", ");
			g_print("%s", uuid2service(uuids[j]));
		}
		g_print("]\n");
		g_object_unref(adapter);
	} else if (discover_arg) {
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);

		g_signal_connect(adapter, "DeviceFound", G_CALLBACK(adapter_device_found), NULL);
		g_signal_connect(adapter, "DeviceDisappeared", G_CALLBACK(adapter_device_disappeared), NULL);

		adapter_start_discovery(adapter, error);
		exit_if_error(error);

		g_print("Searching...\n");

		GSource *timeout_src = g_timeout_source_new_seconds(60);
		g_source_attach(timeout_src, NULL);
		GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);
		g_source_set_callback(timeout_src, stop_discovery, mainloop, NULL);
		g_main_loop_run(mainloop);
		g_main_loop_unref(mainloop);
		g_source_unref(timeout_src);

		adapter_stop_discovery(adapter, &error);
		exit_if_error(error);

		g_print("Done\n");
	} else if (set_arg) {
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);

	}

	g_object_unref(manager);

	exit(EXIT_SUCCESS);
}

