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

static gchar *adapter_arg = NULL;

static GPtrArray *captured_adapters = NULL;
static void capture_adapter(Adapter *adapter);
static void release_adapter(Adapter *adapter);

static GPtrArray *captured_devices = NULL;
static void capture_device(Device *device);
static void release_device(Device *device);

/*
 *  Manager signals
 */
static void manager_adapter_added(Manager *manager, const gchar *adapter_path, gpointer data)
{
	if (adapter_arg == NULL) {
		Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
		g_print("[Manager] Adapter added: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter));
		capture_adapter(adapter);
	}
}

static void manager_adapter_removed(Manager *manager, const gchar *adapter_path, gpointer data)
{
	for (int i = 0; i < captured_adapters->len; i++) {
		Adapter *adapter = ADAPTER(g_ptr_array_index(captured_adapters, i));

		if (g_strcmp0(adapter_path, adapter_get_dbus_object_path(adapter)) == 0) {
			g_print("[Manager] Adapter removed: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter));
			release_adapter(adapter);
			break;
		}
	}

	// Exit if removed user-captured adapter
	if (captured_adapters->len == 0 && adapter_arg != NULL) {
		exit(EXIT_SUCCESS);
	}
}

static void manager_default_adapter_changed(Manager *manager, const gchar *adapter_path, gpointer data)
{
	if (adapter_arg == NULL) {
		Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
		g_print("[Manager] Default adapter changed: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter));
		g_object_unref(adapter);
	}
}

static void manager_property_changed(Manager *manager, const gchar *name, const GValue *value, gpointer data)
{
	if (adapter_arg == NULL) {
		// Only one property: Adapters
		g_print("[Manager] Property changed: %s\n", name);
	}
}

/*
 * Adapter signals
 */
static void adapter_device_created(Adapter *adapter, const gchar *device_path, gpointer data)
{
	Device *device = g_object_new(DEVICE_TYPE, "DBusObjectPath", device_path, NULL);
	g_print("[Adapter: %s (%s)] Device created: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter), device_get_alias(device), device_get_address(device));
	capture_device(device);
}

static void adapter_device_disappeared(Adapter *adapter, const gchar *address, gpointer data)
{
	g_print("[Adapter: %s (%s)] Device disappeared: %s\n", adapter_get_name(adapter), adapter_get_address(adapter), address);
}

static void adapter_device_found(Adapter *adapter, const gchar *address, GHashTable *values, gpointer data)
{
	g_print("[Adapter: %s (%s)] Device found:\n", adapter_get_name(adapter), adapter_get_address(adapter));
	g_print("[%s]\n", address);
	g_print("  Name: %s\n", g_value_get_string(g_hash_table_lookup(values, "Name")));
	g_print("  Alias: %s\n", g_value_get_string(g_hash_table_lookup(values, "Alias")));
	g_print("  Address: %s\n", g_value_get_string(g_hash_table_lookup(values, "Address")));
	g_print("  Class: 0x%x\n", g_value_get_uint(g_hash_table_lookup(values, "Class")));
	g_print("  LegacyPairing: %d\n", g_value_get_boolean(g_hash_table_lookup(values, "LegacyPairing")));
	g_print("  Paired: %d\n", g_value_get_boolean(g_hash_table_lookup(values, "Paired")));
	g_print("  RSSI: %d\n", g_value_get_int(g_hash_table_lookup(values, "RSSI")));
	g_print("\n");
}

static void adapter_device_removed(Adapter *adapter, const gchar *device_path, gpointer data)
{
	for (int i = 0; i < captured_devices->len; i++) {
		Device *device = DEVICE(g_ptr_array_index(captured_devices, i));

		if (g_strcmp0(device_path, device_get_dbus_object_path(device)) == 0) {
			g_print("[Adapter: %s (%s)] Device removed: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter), device_get_alias(device), device_get_address(device));
			release_device(device);
			break;
		}
	}
}

static void adapter_property_changed(Adapter *adapter, const gchar *name, const GValue *value, gpointer data)
{
	g_print("[Adapter: %s (%s)] Property changed: %s", adapter_get_name(adapter), adapter_get_address(adapter), name);
	if (G_VALUE_HOLDS_BOOLEAN(value)) {
		g_print(" -> %d\n", g_value_get_boolean(value));
	} else if (G_VALUE_HOLDS_INT(value)) {
		g_print(" -> %d\n", g_value_get_int(value));
	} else if (G_VALUE_HOLDS_UINT(value)) {
		g_print(" -> %d\n", g_value_get_uint(value));
	} else if (G_VALUE_HOLDS_STRING(value)) {
		g_print(" -> %s\n", g_value_get_string(value));
	} else {
		g_print("\n");
	}
}

/*
 * Device signals
 */
static void device_disconnect_requested(Device *device, gpointer data)
{
	g_print("[Device: %s (%s)] Disconnect requested\n", device_get_alias(device), device_get_address(device));
}

static void device_node_created(Device *device, const gchar *node, gpointer data)
{
	g_print("[Device: %s (%s)] Node created: %s\n", device_get_alias(device), device_get_address(device), node);
}

static void device_node_removed(Device *device, const gchar *node, gpointer data)
{
	g_print("[Device: %s (%s)] Node removed: %s\n", device_get_alias(device), device_get_address(device), node);
}

static void device_property_changed(Device *device, const gchar *name, const GValue *value, gpointer data)
{
	g_print("[Device: %s (%s)] Property changed: %s", device_get_alias(device), device_get_address(device), name);
	if (G_VALUE_HOLDS_BOOLEAN(value)) {
		g_print(" -> %d\n", g_value_get_boolean(value));
	} else if (G_VALUE_HOLDS_INT(value)) {
		g_print(" -> %d\n", g_value_get_int(value));
	} else if (G_VALUE_HOLDS_UINT(value)) {
		g_print(" -> %d\n", g_value_get_uint(value));
	} else if (G_VALUE_HOLDS_STRING(value)) {
		g_print(" -> %s\n", g_value_get_string(value));
	} else {
		g_print("\n");
	}
}

static void capture_adapter(Adapter *adapter)
{
	g_assert(ADAPTER_IS(adapter));

	g_signal_connect(adapter, "DeviceCreated", G_CALLBACK(adapter_device_created), NULL);
	g_signal_connect(adapter, "DeviceDisappeared", G_CALLBACK(adapter_device_disappeared), NULL);
	g_signal_connect(adapter, "DeviceFound", G_CALLBACK(adapter_device_found), NULL);
	g_signal_connect(adapter, "DeviceRemoved", G_CALLBACK(adapter_device_removed), NULL);
	g_signal_connect(adapter, "PropertyChanged", G_CALLBACK(adapter_property_changed), NULL);

	// Capturing signals from devices
	const GPtrArray *devices_list = adapter_get_devices(adapter);
	g_assert(devices_list != NULL);
	for (int i = 0; i < devices_list->len; i++) {
		const gchar *device_path = g_ptr_array_index(devices_list, i);
		Device *device = g_object_new(DEVICE_TYPE, "DBusObjectPath", device_path, NULL);
		capture_device(device);
	}

	g_ptr_array_add(captured_adapters, adapter);
}

static void release_adapter(Adapter *adapter)
{
	g_assert(ADAPTER_IS(adapter));

	for (int i = 0; i < captured_adapters->len; i++) {
		if (adapter == ADAPTER(g_ptr_array_index(captured_adapters, i))) {
			GPtrArray *devices_to_remove = g_ptr_array_new();
			for (int j = 0; j < captured_devices->len; j++) {
				Device *device = DEVICE(g_ptr_array_index(captured_devices, j));
				if (g_strcmp0(adapter_get_dbus_object_path(adapter), device_get_adapter(device)) == 0) {
					g_ptr_array_add(devices_to_remove, device);
				}
			}
			for (int j = 0; j < devices_to_remove->len; j++) {
				Device *device = DEVICE(g_ptr_array_index(devices_to_remove, j));
				release_device(device);
			}
			g_ptr_array_unref(devices_to_remove);
			g_object_unref(adapter);
			g_ptr_array_remove_index(captured_adapters, i);
			break;
		}
	}
}

static void capture_device(Device *device)
{
	g_assert(DEVICE_IS(device));

	g_signal_connect(device, "DisconnectRequested", G_CALLBACK(device_disconnect_requested), NULL);
	g_signal_connect(device, "NodeCreated", G_CALLBACK(device_node_created), NULL);
	g_signal_connect(device, "NodeRemoved", G_CALLBACK(device_node_removed), NULL);
	g_signal_connect(device, "PropertyChanged", G_CALLBACK(device_property_changed), NULL);

	// TODO: Add capturing services (eg. input, audio) signals

	g_ptr_array_add(captured_devices, device);
}

static void release_device(Device *device)
{
	g_assert(DEVICE_IS(device));

	for (int i = 0; i < captured_devices->len; i++) {
		if (device == DEVICE(g_ptr_array_index(captured_devices, i))) {
			g_object_unref(device);
			g_ptr_array_remove_index(captured_devices, i);
			break;
		}
	}
}

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "adapter#id"},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("- a bluetooth monitor");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "monitor summary");
	g_option_context_set_description(context, "monitor desc");

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

	captured_adapters = g_ptr_array_new();
	captured_devices = g_ptr_array_new();

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	if (adapter_arg != NULL) {
		Adapter *adapter = find_adapter(adapter_arg, &error);
		exit_if_error(error);
		capture_adapter(adapter);
	} else {
		const GPtrArray *adapters_list = manager_get_adapters(manager);
		g_assert(adapters_list != NULL);

		if (adapters_list->len == 0) {
			g_print("No adapters found\n");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < adapters_list->len; i++) {
			const gchar *adapter_path = g_ptr_array_index(adapters_list, i);
			Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
			capture_adapter(adapter);
		}
	}

	g_signal_connect(manager, "AdapterAdded", G_CALLBACK(manager_adapter_added), NULL);
	g_signal_connect(manager, "AdapterRemoved", G_CALLBACK(manager_adapter_removed), NULL);
	g_signal_connect(manager, "DefaultAdapterChanged", G_CALLBACK(manager_default_adapter_changed), NULL);
	g_signal_connect(manager, "PropertyChanged", G_CALLBACK(manager_property_changed), NULL);

	GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	// TODO: Add SIGINT handler (Ctrl+C)

	exit(EXIT_SUCCESS);
}

