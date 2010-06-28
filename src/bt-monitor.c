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

static gchar *capture_adapter_name = NULL;
static GPtrArray *captured_adapters = NULL;
static GPtrArray *captured_devices = NULL;

/*
 *  Manager signals
 */
static void manager_adapter_added(Manager *manager, const gchar *adapter_path, gpointer data)
{
	Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
	gchar *adapter_name = adapter_get_name(adapter, NULL);

	if (capture_adapter_name == NULL || (g_strcmp0(capture_adapter_name, adapter_name) == 0 && captured_adapters->len == 0)) {
		g_ptr_array_add(captured_adapters, adapter);
		g_print("[MANAGER] adapter added: %s (%s)\n", adapter_name, adapter_path);
	} else {
		g_object_unref(adapter);
	}

	g_free(adapter_name);
}

static void manager_adapter_removed(Manager *manager, const gchar *adapter_path, gpointer data)
{
	for (int i = 0; i < captured_adapters->len; i++) {
		Adapter *adapter = ADAPTER(g_ptr_array_index(captured_adapters, i));
		gchar *adapter_name = adapter_get_name(adapter, NULL);

		if (g_strcmp0(adapter_path, adapter_get_dbus_object_path(adapter)) == 0) {
			g_ptr_array_remove_index(captured_adapters, i);
			g_print("[MANAGER] adapter removed: %s (%s)\n", adapter_name, adapter_path);
			g_object_unref(adapter);
			g_free(adapter_name);
			break;
		}

		g_free(adapter_name);
	}
}

static void manager_default_adapter_changed(Manager *manager, const gchar *adapter_path, gpointer data)
{
	g_print("[MANAGER] default adapter changed: %s\n", adapter_path);
}

static void manager_property_changed(Manager *manager, const gchar *name, const GValue *value, gpointer data)
{
	g_print("[MANAGER] property changed: %s -> %s\n", name, g_strdup_value_contents(value));
}

/*
 * Adapter signals
 */
static void adapter_device_created(Adapter *adapter, const gchar *device_path, gpointer data)
{
	g_print("[ADAPTER] device created: %s\n", device_path);
}

static void adapter_device_disappeared(Adapter *adapter, const gchar *address, gpointer data)
{
	g_print("[ADAPTER] device disappeared: %s\n", address);
}

static void adapter_device_found(Adapter *adapter, const gchar *address, gpointer data)
{
	g_print("[ADAPTER] device found: %s\n", address);
}

static void adapter_device_removed(Adapter *adapter, const gchar *device_path, gpointer data)
{
	g_print("[ADAPTER] device removed: %s\n", device_path);
}

static void adapter_property_changed(Adapter *adapter, const gchar *name, const GValue *value, gpointer data)
{
	g_print("[ADAPTER] property changed: %s -> %s\n", name, g_strdup_value_contents(value));
}

/*
 * Device signals
 */
static void device_disconnect_requested(Device *device, gpointer data)
{
	g_print("[DEVICE] disconnect requested\n");
}

static void device_node_created(Device *device, const gchar *node, gpointer data)
{
	g_print("[DEVICE] node created: %s\n", node);
}

static void device_node_removed(Device *device, const gchar *node, gpointer data)
{
	g_print("[DEVICE] node removed: %s\n", node);
}

static void device_property_changed(Device *device, const gchar *name, const GValue *value, gpointer data)
{
	g_print("[DEVICE] property changed: %s -> %s\n", name, g_strdup_value_contents(value));
}

/*
 * Service signals
 */
//static void audio_property_changed()
//{
//
//}

//static void input_property_changed()
//{
//
//}

//static void network_property_changed()
//{
//
//}

static GOptionEntry entries[] = {
	{ "adapter", 'a', 0, G_OPTION_ARG_STRING, &capture_adapter_name, "Adapter name or MAC", NULL},
	{ NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("- a bluetooth monitor");
	g_option_context_add_main_entries(context, entries, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}
	g_option_context_free(context);

	if (!dbus_connect(&error)) {
		g_printerr("Couldn't connect to dbus: %s", error->message);
		exit(EXIT_FAILURE);
	}

	captured_adapters = g_ptr_array_new();
	captured_devices = g_ptr_array_new();

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	if (capture_adapter_name != NULL) {
		gchar *adapter_path = find_adapter_by_name(capture_adapter_name, &error);
		exit_if_error(error);

		g_print("capturing adapter: %s (%s)\n", capture_adapter_name, adapter_path);

		Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
		g_ptr_array_add(captured_adapters, adapter);
		g_free(adapter_path);
	} else {
		GPtrArray *adapters_list = manager_get_adapters(manager, &error);
		g_return_val_if_fail(adapters_list != NULL, NULL);
		exit_if_error(error);

		if (adapters_list->len == 0) {
			g_print("no adapters found\n");
		}

		for (int i = 0; i < adapters_list->len; i++) {
			gchar *adapter_path = g_ptr_array_index(adapters_list, i);
			Adapter *adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
			gchar *adapter_name = adapter_get_name(adapter, &error);
			exit_if_error(error);

			g_print("capturing adapter: %s (%s)\n", adapter_name, adapter_path);

			g_ptr_array_add(captured_adapters, adapter);
			g_free(adapter_name);
		}

		g_ptr_array_unref(adapters_list);
	}

	g_signal_connect(manager, "AdapterAdded", G_CALLBACK(manager_adapter_added), NULL);
	g_signal_connect(manager, "AdapterRemoved", G_CALLBACK(manager_adapter_removed), NULL);
	g_signal_connect(manager, "DefaultAdapterChanged", G_CALLBACK(manager_default_adapter_changed), NULL);
	g_signal_connect(manager, "PropertyChanged", G_CALLBACK(manager_property_changed), NULL);

	// Capturing signals from adapters
	for (int i = 0; i < captured_adapters->len; i++) {
		Adapter *adapter = ADAPTER(g_ptr_array_index(captured_adapters, i));

		g_signal_connect(adapter, "DeviceCreated", G_CALLBACK(adapter_device_created), NULL);
		g_signal_connect(adapter, "DeviceDisappeared", G_CALLBACK(adapter_device_disappeared), NULL);
		g_signal_connect(adapter, "DeviceFound", G_CALLBACK(adapter_device_found), NULL);
		g_signal_connect(adapter, "DeviceRemoved", G_CALLBACK(adapter_device_removed), NULL);
		g_signal_connect(adapter, "PropertyChanged", G_CALLBACK(adapter_property_changed), NULL);

		// Capturing signals from devices
		GPtrArray *devices_list = adapter_get_devices(adapter, NULL);
		for (int i = 0; i < devices_list->len; i++) {
			gchar *device_path = g_ptr_array_index(devices_list, i);
			Device *device = g_object_new(DEVICE_TYPE, "DBusObjectPath", device_path, NULL);
			g_ptr_array_add(captured_devices, device);

			g_signal_connect(device, "DisconnectRequested", G_CALLBACK(device_disconnect_requested), NULL);
			g_signal_connect(device, "NodeCreated", G_CALLBACK(device_node_created), NULL);
			g_signal_connect(device, "NodeRemoved", G_CALLBACK(device_node_removed), NULL);
			g_signal_connect(device, "PropertyChanged", G_CALLBACK(device_property_changed), NULL);
		}

		g_ptr_array_unref(devices_list);
	}

	GMainLoop *mainloop;
	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	exit(EXIT_SUCCESS);
}

