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

static void adapter_device_found(Adapter *adapter, const gchar *address, GHashTable *values, gpointer data)
{
	g_assert(data != NULL);
	GHashTable *found_devices = data;

	if (g_hash_table_lookup(found_devices, address) != NULL) {
		return;
	}

	if (g_hash_table_size(found_devices) == 0) g_print("\n");

	g_print("[%s]\n", address);
	g_print("  Name: %s\n", g_hash_table_lookup(values, "Name") != NULL ? g_value_get_string(g_hash_table_lookup(values, "Name")) : NULL);
	g_print("  Alias: %s\n", g_hash_table_lookup(values, "Alias") != NULL ? g_value_get_string(g_hash_table_lookup(values, "Alias")) : NULL);
	g_print("  Address: %s\n", g_value_get_string(g_hash_table_lookup(values, "Address")));
	g_print("  Icon: %s\n", g_value_get_string(g_hash_table_lookup(values, "Icon")));
	g_print("  Class: 0x%x\n", g_value_get_uint(g_hash_table_lookup(values, "Class")));
	g_print("  LegacyPairing: %d\n", g_value_get_boolean(g_hash_table_lookup(values, "LegacyPairing")));
	g_print("  Paired: %d\n", g_value_get_boolean(g_hash_table_lookup(values, "Paired")));
	g_print("  RSSI: %d\n", g_value_get_int(g_hash_table_lookup(values, "RSSI")));
	g_print("\n");

	g_hash_table_insert(found_devices, g_strdup(address), g_value_dup_string(g_hash_table_lookup(values, "Alias")));
}

/*
static void adapter_device_disappeared(Adapter *adapter, const gchar *address, gpointer data)
{
	g_assert(data != NULL);
	GHashTable *found_devices = data;

	g_print("Device disappeared: %s (%s)\n", g_value_get_string(g_hash_table_lookup(found_devices, address)), address);
}
 */

static void adapter_property_changed(Adapter *adapter, const gchar *name, const GValue *value, gpointer data)
{
	g_assert(data != NULL);
	GMainLoop *mainloop = data;

	if (g_strcmp0(name, "Discovering") == 0 && g_value_get_boolean(value) == FALSE) {
		g_main_loop_quit(mainloop);
	}
}

static gboolean list_arg = FALSE;
static gchar *adapter_arg = NULL;
static gboolean info_arg = FALSE;
static gboolean discover_arg = FALSE;
static gboolean set_arg = FALSE;
static gchar *set_property_arg = NULL;
static gchar *set_value_arg = NULL;

static GOptionEntry entries[] = {
	{"list", 'l', 0, G_OPTION_ARG_NONE, &list_arg, "List all available adapters", NULL},
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter Name or MAC", "<name|mac>"},
	{"info", 'i', 0, G_OPTION_ARG_NONE, &info_arg, "Show adapter info", NULL},
	{"discover", 'd', 0, G_OPTION_ARG_NONE, &discover_arg, "Discover remote devices", NULL},
	{"set", 0, 0, G_OPTION_ARG_NONE, &set_arg, "Set adapter property", NULL},
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

	context = g_option_context_new("- a bluetooth adapter manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"Set Options:\n"
			"  --set <property> <value>\n"
			"  Where `property` is one of:\n"
			"     Name\n"
			"     Discoverable\n"
			"     DiscoverableTimeout\n"
			"     Pairable\n"
			"     PairableTimeout\n"
			"     Powered\n\n"
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home page <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!list_arg && !info_arg && !discover_arg && !set_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (set_arg && (argc != 3 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0)) {
		g_print("%s: Invalid arguments for --set\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
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

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	if (list_arg) {
		const GPtrArray *adapters_list = manager_get_adapters(manager);
		g_assert(adapters_list != NULL);

		if (adapters_list->len == 0) {
			g_print("No adapters found\n");
			exit(EXIT_FAILURE);
		}

		g_print("Available adapters:\n");
		for (int i = 0; i < adapters_list->len; i++) {
			const gchar *adapter_path = g_ptr_array_index(adapters_list, i);
			Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
			g_print("%s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter));
			g_object_unref(adapter);
		}
	} else if (info_arg) {
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);

		gchar *adapter_intf = g_path_get_basename(adapter_get_dbus_object_path(adapter));
		g_print("[%s]\n", adapter_intf);
		g_print("  Name: %s [rw]\n", adapter_get_name(adapter));
		g_print("  Address: %s\n", adapter_get_address(adapter));
		g_print("  Class: 0x%x\n", adapter_get_class(adapter));
		g_print("  Discoverable: %d [rw]\n", adapter_get_discoverable(adapter));
		g_print("  DiscoverableTimeout: %d [rw]\n", adapter_get_discoverable_timeout(adapter));
		g_print("  Discovering: %d\n", adapter_get_discovering(adapter));
		g_print("  Pairable: %d [rw]\n", adapter_get_pairable(adapter));
		g_print("  PairableTimeout: %d [rw]\n", adapter_get_pairable_timeout(adapter));
		g_print("  Powered: %d [rw]\n", adapter_get_powered(adapter));
		g_print("  UUIDs: [");
		const gchar **uuids = adapter_get_uuids(adapter);
		for (int j = 0; uuids[j] != NULL; j++) {
			if (j > 0) g_print(", ");
			g_print("%s", uuid2name(uuids[j]));
		}
		g_print("]\n");

		g_free(adapter_intf);
		g_object_unref(adapter);
	} else if (discover_arg) {
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);

		// To store pairs MAC => Name
		GHashTable *found_devices = g_hash_table_new(g_str_hash, g_str_equal);

		// Mainloop
		GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);

		g_signal_connect(adapter, "DeviceFound", G_CALLBACK(adapter_device_found), found_devices);
		//g_signal_connect(adapter, "DeviceDisappeared", G_CALLBACK(adapter_device_disappeared), found_devices);
		g_signal_connect(adapter, "PropertyChanged", G_CALLBACK(adapter_property_changed), mainloop);

		g_print("Searching...\n");
		adapter_start_discovery(adapter, &error);
		exit_if_error(error);

		g_main_loop_run(mainloop);
		/* Discovering process here... */
		g_main_loop_unref(mainloop);

		g_print("Done\n");
		g_hash_table_unref(found_devices);
		g_object_unref(adapter);
	} else if (set_arg) {
		set_property_arg = argv[1];
		set_value_arg = argv[2];

		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);

		GValue v = {0,};

		if (g_strcmp0(set_property_arg, "Name") == 0) {
			g_value_init(&v, G_TYPE_STRING);
			g_value_set_string(&v, set_value_arg);
		} else if (
				g_strcmp0(set_property_arg, "Discoverable") == 0 ||
				g_strcmp0(set_property_arg, "Pairable") == 0 ||
				g_strcmp0(set_property_arg, "Powered") == 0
				) {
			g_value_init(&v, G_TYPE_BOOLEAN);

			if (g_strcmp0(set_value_arg, "0") == 0 || g_ascii_strcasecmp(set_value_arg, "FALSE") == 0 || g_ascii_strcasecmp(set_value_arg, "OFF") == 0) {
				g_value_set_boolean(&v, FALSE);
			} else if (g_strcmp0(set_value_arg, "1") == 0 || g_ascii_strcasecmp(set_value_arg, "TRUE") == 0 || g_ascii_strcasecmp(set_value_arg, "ON") == 0) {
				g_value_set_boolean(&v, TRUE);
			} else {
				g_print("%s: Invalid boolean value: %s\n", g_get_prgname(), set_value_arg);
				g_print("Try `%s --help` for more information.\n", g_get_prgname());
				exit(EXIT_FAILURE);
			}
		} else if (
				g_strcmp0(set_property_arg, "DiscoverableTimeout") == 0 ||
				g_strcmp0(set_property_arg, "PairableTimeout") == 0
				) {
			g_value_init(&v, G_TYPE_UINT);
			g_value_set_uint(&v, (guint) atoi(set_value_arg));
		} else {
			g_print("%s: Invalid property: %s\n", g_get_prgname(), set_property_arg);
			g_print("Try `%s --help` for more information.\n", g_get_prgname());
			exit(EXIT_FAILURE);
		}

		GHashTable *props = adapter_get_properties(adapter, &error);
		exit_if_error(error);
		GValue *old_value = g_hash_table_lookup(props, set_property_arg);
		g_assert(old_value != NULL);
		if (G_VALUE_HOLDS_STRING(old_value)) {
			g_print("%s: %s -> %s\n", set_property_arg, g_value_get_string(old_value), g_value_get_string(&v));
		} else if (G_VALUE_HOLDS_BOOLEAN(old_value)) {
			g_print("%s: %d -> %d\n", set_property_arg, g_value_get_boolean(old_value), g_value_get_boolean(&v));
		} else if (G_VALUE_HOLDS_UINT(old_value)) {
			g_print("%s: %d -> %d\n", set_property_arg, g_value_get_uint(old_value), g_value_get_uint(&v));
		}
		g_hash_table_unref(props);

		adapter_set_property(adapter, set_property_arg, &v, &error);
		exit_if_error(error);

		g_value_unset(&v);
		g_object_unref(adapter);
	}

	g_object_unref(manager);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

