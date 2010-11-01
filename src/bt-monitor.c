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

// adapter_path => GPtrArray{adapter_object, device1_object, device2_object, ...}
static GHashTable *captured_adapters_devices_t = NULL;
// device_path => GPtrArray{device_object, service1_object, service2_object, ...}
static GHashTable *captured_devices_services_t = NULL;

static void capture_adapter(Adapter *adapter);
static void release_adapter(Adapter *adapter);
static void capture_device(Device *device);
static void release_device(Device *device);
static void reload_device_services(Device *device);

/*
 *  Manager signals
 */
static void manager_adapter_added(Manager *manager, const gchar *adapter_path, gpointer data)
{
	//g_print("manager_adapter_added()\n");

	if (adapter_arg == NULL) {
		Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
		g_print("[Manager] Adapter added: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter));
		capture_adapter(adapter);
	}
}

static void manager_adapter_removed(Manager *manager, const gchar *adapter_path, gpointer data)
{
	//g_print("manager_adapter_removed()\n");

	GSList *t = g_hash_table_lookup(captured_adapters_devices_t, adapter_path);
	if (t != NULL) {
		Adapter *adapter = ADAPTER(g_slist_nth_data(t, 0));
		g_print("[Manager] Adapter removed: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter));
		release_adapter(adapter);
	}

	// Exit if removed user-captured adapter
	if (adapter_arg != NULL && g_hash_table_size(captured_adapters_devices_t) == 0) {
		exit(EXIT_SUCCESS);
	}
}

static void manager_default_adapter_changed(Manager *manager, const gchar *adapter_path, gpointer data)
{
	//g_print("manager_default_adapter_changed()\n");

	if (adapter_arg == NULL) {
		Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
		g_print("[Manager] Default adapter changed: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter));
		g_object_unref(adapter);
	}
}

static void manager_property_changed(Manager *manager, const gchar *name, const GValue *value, gpointer data)
{
	//g_print("manager_property_changed()\n");

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
	//g_print("adapter_device_created()\n");

	if (intf_supported(BLUEZ_DBUS_NAME, device_path, DEVICE_DBUS_INTERFACE)) {
		Device *device = g_object_new(DEVICE_TYPE, "DBusObjectPath", device_path, NULL);
		g_print("[Adapter: %s (%s)] Device created: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter), device_get_alias(device), device_get_address(device));
		capture_device(device);
	} else {
		g_print("[Adapter: %s (%s)]* Device created: %s\n", adapter_get_name(adapter), adapter_get_address(adapter), device_path);
	}
}

static void adapter_device_disappeared(Adapter *adapter, const gchar *address, gpointer data)
{
	//g_print("adapter_device_disappeared()\n");

	g_print("[Adapter: %s (%s)] Device disappeared: %s\n", adapter_get_name(adapter), adapter_get_address(adapter), address);
}

static void adapter_device_found(Adapter *adapter, const gchar *address, GHashTable *values, gpointer data)
{
	//g_print("adapter_device_found()\n");

	g_print("[Adapter: %s (%s)] Device found:\n", adapter_get_name(adapter), adapter_get_address(adapter));
	g_print("[%s]\n", address);
	g_print("  Name: %s\n", g_value_get_string(g_hash_table_lookup(values, "Name")));
	g_print("  Alias: %s\n", g_value_get_string(g_hash_table_lookup(values, "Alias")));
	g_print("  Address: %s\n", g_value_get_string(g_hash_table_lookup(values, "Address")));
	g_print("  Icon: %s\n", g_value_get_string(g_hash_table_lookup(values, "Icon")));
	g_print("  Class: 0x%x\n", g_value_get_uint(g_hash_table_lookup(values, "Class")));
	g_print("  LegacyPairing: %d\n", g_value_get_boolean(g_hash_table_lookup(values, "LegacyPairing")));
	g_print("  Paired: %d\n", g_value_get_boolean(g_hash_table_lookup(values, "Paired")));
	g_print("  RSSI: %d\n", g_value_get_int(g_hash_table_lookup(values, "RSSI")));
	g_print("\n");
}

static void adapter_device_removed(Adapter *adapter, const gchar *device_path, gpointer data)
{
	//g_print("adapter_device_removed()\n");

	GSList *t2 = g_hash_table_lookup(captured_devices_services_t, device_path);
	if (t2 != NULL) {
		Device *device = DEVICE(g_slist_nth_data(t2, 0));
		g_print("[Adapter: %s (%s)] Device removed: %s (%s)\n", adapter_get_name(adapter), adapter_get_address(adapter), device_get_alias(device), device_get_address(device));
		release_device(device);
	} else {
		g_print("[Adapter: %s (%s)]* Device removed: %s\n", adapter_get_name(adapter), adapter_get_address(adapter), device_path);
	}
}

static void adapter_property_changed(Adapter *adapter, const gchar *name, const GValue *value, gpointer data)
{
	//g_print("adapter_property_changed()\n");

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
	//g_print("device_disconnect_requested()\n");

	g_print("[Device: %s (%s)] Disconnect requested\n", device_get_alias(device), device_get_address(device));
}

static void device_property_changed(Device *device, const gchar *name, const GValue *value, gpointer data)
{
	//g_print("device_property_changed()\n");

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

	if (g_strcmp0(name, "UUIDs") == 0) {
		reload_device_services(device);
	}
}

/*
 * Services signals
 */
static void audio_property_changed(Audio *audio, const gchar *name, const GValue *value, gpointer data)
{
	//g_print("audio_property_changed()\n");

	Device *device = DEVICE(data);
	g_print("[Device: %s (%s)] Audio property changed: %s", device_get_alias(device), device_get_address(device), name);
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

static void input_property_changed(Input *input, const gchar *name, const GValue *value, gpointer data)
{
	//g_print("input_property_changed()\n");

	Device *device = DEVICE(data);
	g_print("[Device: %s (%s)] Input property changed: %s", device_get_alias(device), device_get_address(device), name);
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

static void network_property_changed(Network *network, const gchar *name, const GValue *value, gpointer data)
{
	//g_print("network_property_changed()\n");

	Device *device = DEVICE(data);
	g_print("[Device: %s (%s)] Network property changed: %s", device_get_alias(device), device_get_address(device), name);
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
 * Capturing methods
 */
static void capture_adapter(Adapter *adapter)
{
	//g_print("capture_adapter()\n");

	g_assert(ADAPTER_IS(adapter));

	GSList *t = g_slist_append(NULL, adapter);
	g_hash_table_insert(captured_adapters_devices_t, g_strdup(adapter_get_dbus_object_path(adapter)), t);

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
}

static void release_adapter(Adapter *adapter)
{
	//g_print("release_adapter()\n");

	g_assert(ADAPTER_IS(adapter));

	GSList *t = g_hash_table_lookup(captured_adapters_devices_t, adapter_get_dbus_object_path(adapter));
	while (g_slist_length(t) > 1) {
		Device *device = DEVICE(g_slist_nth_data(t, 1));
		release_device(device);
		t = g_hash_table_lookup(captured_adapters_devices_t, adapter_get_dbus_object_path(adapter));
	}
	g_slist_free(t);
	g_hash_table_remove(captured_adapters_devices_t, adapter_get_dbus_object_path(adapter));
	g_object_unref(adapter);
}

static void capture_device(Device *device)
{
	//g_print("capture_device()\n");

	g_assert(DEVICE_IS(device));

	GSList *t = g_hash_table_lookup(captured_adapters_devices_t, device_get_adapter(device));
	if (t == NULL) {
		Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", device_get_adapter(device), NULL);
		g_printerr("[Device: %s (%s)] Uncaptured adapter: %s (%s)\n", device_get_alias(device), device_get_address(device), adapter_get_name(adapter), adapter_get_address(adapter));
		g_object_unref(adapter);
		return;
	}
	t = g_slist_append(t, device);

	GSList *t2 = g_hash_table_lookup(captured_devices_services_t, device_get_dbus_object_path(device));
	if (t2 != NULL) {
		g_printerr("[Device: %s (%s)] Already captured device\n", device_get_alias(device), device_get_address(device));
		return;
	}
	t2 = g_slist_append(t2, device);

	g_signal_connect(device, "DisconnectRequested", G_CALLBACK(device_disconnect_requested), NULL);
	g_signal_connect(device, "PropertyChanged", G_CALLBACK(device_property_changed), NULL);

	g_hash_table_insert(captured_adapters_devices_t, g_strdup(device_get_adapter(device)), t);
	g_hash_table_insert(captured_devices_services_t, g_strdup(device_get_dbus_object_path(device)), t2);

	reload_device_services(device);
}

static void release_device(Device *device)
{
	//g_print("release_device()\n");

	g_assert(DEVICE_IS(device));

	GSList *t = g_hash_table_lookup(captured_adapters_devices_t, device_get_adapter(device));
	if (t != NULL) {
		t = g_slist_remove(t, device);
		g_hash_table_insert(captured_adapters_devices_t, g_strdup(device_get_adapter(device)), t);
	}

	GSList *t2 = g_hash_table_lookup(captured_devices_services_t, device_get_dbus_object_path(device));
	while (g_slist_length(t2) > 1) {
		GObject *service = g_slist_nth_data(t2, 1);
		t2 = g_slist_remove(t2, service);
		g_object_unref(service);
	}
	g_slist_free(t2);
	g_hash_table_remove(captured_devices_services_t, device_get_dbus_object_path(device));

	g_object_unref(device);
}

static void reload_device_services(Device *device)
{
	//g_print("reload_device_services()\n");

	GSList *t2 = g_hash_table_lookup(captured_devices_services_t, device_get_dbus_object_path(device));
	if (t2 == NULL) {
		g_printerr("[Device: %s (%s)] Uncaptured device\n", device_get_alias(device), device_get_address(device));
		return;
	}

	while (g_slist_length(t2) > 1) {
		GObject *service = g_slist_nth_data(t2, 1);
		t2 = g_slist_remove(t2, service);
		g_object_unref(service);
	}

	// Capturing signals from available services
	if (intf_supported(BLUEZ_DBUS_NAME, device_get_dbus_object_path(device), AUDIO_DBUS_INTERFACE)) {
		Audio *audio = g_object_new(AUDIO_TYPE, "DBusObjectPath", device_get_dbus_object_path(device), NULL);
		g_signal_connect(audio, "PropertyChanged", G_CALLBACK(audio_property_changed), device);
		t2 = g_slist_append(t2, audio);
	}
	if (intf_supported(BLUEZ_DBUS_NAME, device_get_dbus_object_path(device), INPUT_DBUS_INTERFACE)) {
		Input *input = g_object_new(INPUT_TYPE, "DBusObjectPath", device_get_dbus_object_path(device), NULL);
		g_signal_connect(input, "PropertyChanged", G_CALLBACK(input_property_changed), device);
		t2 = g_slist_append(t2, input);
	}
	if (intf_supported(BLUEZ_DBUS_NAME, device_get_dbus_object_path(device), NETWORK_DBUS_INTERFACE)) {
		Network *network = g_object_new(NETWORK_TYPE, "DBusObjectPath", device_get_dbus_object_path(device), NULL);
		g_signal_connect(network, "PropertyChanged", G_CALLBACK(network_property_changed), device);
		t2 = g_slist_append(t2, network);
	}

	g_hash_table_insert(captured_devices_services_t, g_strdup(device_get_dbus_object_path(device)), t2);
}

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter Name or MAC", "<name|mac>"},
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

	context = g_option_context_new("- a bluetooth monitor");
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

	captured_adapters_devices_t = g_hash_table_new(g_str_hash, g_str_equal);
	captured_devices_services_t = g_hash_table_new(g_str_hash, g_str_equal);

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

	g_print("Monitor registered\n");

	GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	// TODO: Add SIGINT handler (Ctrl+C)

	g_main_loop_unref(mainloop);
	g_object_unref(manager);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

