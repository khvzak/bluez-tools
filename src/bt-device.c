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

static void create_paired_device_done(gpointer data)
{
	GMainLoop *mainloop = data;
	g_main_loop_quit(mainloop);
}

static gchar *adapter_arg = NULL;
static gboolean list_arg = FALSE;
static gchar *connect_arg = NULL;
static gchar *remove_arg = NULL;
static gchar *info_arg = NULL;
static gchar *services_arg = NULL;
static gboolean set_arg = FALSE;
static gchar *set_device_arg = NULL;
static gchar *set_name_arg = NULL;
static gchar *set_value_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "adapter#id"},
	{"list", 'l', 0, G_OPTION_ARG_NONE, &list_arg, "List added devices", NULL},
	{"connect", 'c', 0, G_OPTION_ARG_STRING, &connect_arg, "Connect to a device", "device#id"},
	{"remove", 'r', 0, G_OPTION_ARG_STRING, &remove_arg, "Remove device", "device#id"},
	{"info", 'i', 0, G_OPTION_ARG_STRING, &info_arg, "Get info about device", "device#id"},
	{"services", 's', 0, G_OPTION_ARG_STRING, &services_arg, "Discover device services", "device#id"},
	{"set", 0, 0, G_OPTION_ARG_NONE, &set_arg, "Set property", NULL},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("- a bluetooth device manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "device summary");
	g_option_context_set_description(context,
			"Set Options:\n"
			"  --set <device#id> <Name> <Value>\n"
			"  Where `Name` is device property name:\n"
			"     Alias\n"
			"     Trusted\n"
			"     Blocked\n"
			"  And `Value` is property value to set\n\n"
			"device desc"
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!list_arg && !connect_arg && !remove_arg && !info_arg && !services_arg && !set_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (set_arg && argc != 4) {
		g_print("%s: Invalid arguments for --set\n", g_get_prgname());
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

	if (list_arg) {
		const GPtrArray *devices_list = adapter_get_devices(adapter);
		g_assert(devices_list != NULL);

		if (devices_list->len == 0) {
			g_print("No devices found\n");
			exit(EXIT_FAILURE);
		}

		g_print("Added devices:\n");
		for (int i = 0; i < devices_list->len; i++) {
			const gchar *device_path = g_ptr_array_index(devices_list, i);
			Device *device = g_object_new(DEVICE_TYPE, "DBusObjectPath", device_path, NULL);
			g_print("%s (%s)\n", device_get_alias(device), device_get_address(device));
			g_object_unref(device);
		}
	} else if (connect_arg) {
		g_print("Connecting to: %s\n", connect_arg);
		Agent *agent = g_object_new(AGENT_TYPE, NULL);
		GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);

		adapter_create_paired_device_begin(adapter, create_paired_device_done, mainloop, connect_arg, DBUS_AGENT_PATH, "DisplayYesNo");
		g_main_loop_run(mainloop);
		g_char *created_device = adapter_create_paired_device_end(adapter, &error);
		exit_if_error(error);

		g_main_loop_unref(mainloop);
		g_object_unref(agent);
		g_free(created_device);
	} else if (remove_arg) {
		Device *device = find_device(adapter, remove_arg, &error);
		exit_if_error(error);

		adapter_remove_device(adapter, device_get_dbus_object_path(device), &error);
		exit_if_error(error);

		g_print("Done\n");
		g_object_unref(device);
	} else if (info_arg) {
		Device *device = find_device(adapter, info_arg, &error);
		exit_if_error(error);

		g_print("[%s]\n", device_get_address(device));
		g_print("  Name: %s\n", device_get_name(device));
		g_print("  Alias: %s [rw]\n", device_get_alias(device));
		g_print("  Address: %s\n", device_get_address(device));
		// TODO: Add class to type conv
		g_print("  Class: %x\n", device_get_class(device));
		g_print("  Paired: %d\n", device_get_paired(device));
		g_print("  Trusted: %d [rw]\n", device_get_trusted(device));
		g_print("  Blocked: %d [rw]\n", device_get_blocked(device));
		g_print("  Connected: %d\n", device_get_connected(device));
		g_print("  UUIDs: [");
		const gchar **uuids = device_get_uuids(device);
		for (int j = 0; uuids[j] != NULL; j++) {
			if (j > 0) g_print(", ");
			g_print("%s", uuid2service(uuids[j]));
		}
		g_print("]\n");

		g_object_unref(device);
	} else if (services_arg) {
		Device *device = find_device(adapter, services_arg, &error);
		exit_if_error(error);

		// TODO: Add services scan

		g_object_unref(device);
	} else if (set_arg) {
		set_device_arg = argv[1];
		set_name_arg = argv[2];
		set_value_arg = argv[3];

		Device *device = find_device(adapter, set_device_arg, &error);
		exit_if_error(error);

		GValue v = {0,};

		if (g_strcmp0(set_name_arg, "Alias") == 0) {
			g_value_init(&v, G_TYPE_STRING);
			g_value_set_string(&v, set_value_arg);
		} else if (
				g_strcmp0(set_name_arg, "Trusted") == 0 ||
				g_strcmp0(set_name_arg, "Blocked") == 0
				) {
			g_value_init(&v, G_TYPE_BOOLEAN);

			if (g_strcmp0(set_value_arg, "0") == 0 || g_ascii_strcasecmp(set_value_arg, "FALSE") == 0 || g_ascii_strcasecmp(set_value_arg, "OFF") == 0) {
				g_value_set_boolean(&v, FALSE);
			} else if (g_strcmp0(set_value_arg, "1") == 0 || g_ascii_strcasecmp(set_value_arg, "TRUE") == 0 || g_ascii_strcasecmp(set_value_arg, "ON") == 0) {
				g_value_set_boolean(&v, TRUE);
			} else {
				g_print("Invalid value: %s\n", set_value_arg);
			}
		} else {
			g_print("Invalid property: %s\n", set_name_arg);
			exit(EXIT_FAILURE);
		}

		GHashTable *props = device_get_properties(device, &error);
		exit_if_error(error);
		GValue *old_value = g_hash_table_lookup(props, set_name_arg);
		g_assert(old_value != NULL);
		if (G_VALUE_HOLDS_STRING(old_value)) {
			g_print("%s: %s -> %s\n", set_name_arg, g_value_get_string(old_value), g_value_get_string(&v));
		} else if (G_VALUE_HOLDS_BOOLEAN(old_value)) {
			g_print("%s: %d -> %d\n", set_name_arg, g_value_get_boolean(old_value), g_value_get_boolean(&v));
		}
		g_hash_table_unref(props);

		device_set_property(device, set_name_arg, &v, &error);
		exit_if_error(error);

		g_value_unset(&v);
		g_object_unref(device);
	}

	g_object_unref(adapter);

	exit(EXIT_SUCCESS);
}

