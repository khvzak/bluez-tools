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

static void create_paired_device_done(gpointer data)
{
	GMainLoop *mainloop = data;
	g_main_loop_quit(mainloop);
}

static int rec_elem_num = 0;
static int attr_elem_num = 0;
static int seq_elem_num = 0;
static int int_elem_num = 0;

static int current_attr_id = -1;
static int current_uuid_id = -1;

void xml_start_element(GMarkupParseContext *context,
		const gchar *element_name,
		const gchar **attribute_names,
		const gchar **attribute_values,
		gpointer user_data,
		GError **error)
{
	GPatternSpec *int_pattern = g_pattern_spec_new("*int*");

	if (g_strcmp0(element_name, "record") == 0) {
		rec_elem_num++;

		attr_elem_num = 0;
		seq_elem_num = 0;
		int_elem_num = 0;

		current_attr_id = -1;
		current_uuid_id = -1;

		if (rec_elem_num == 1) g_print("\n");
	} else if (g_strcmp0(element_name, "attribute") == 0 && g_strcmp0(attribute_names[0], "id") == 0) {
		int attr_id = xtoi(attribute_values[0]);
		const gchar *attr_name = sdp_get_attr_id_name(attr_id);
		current_attr_id = attr_id;
		attr_elem_num++;

		if (attr_elem_num > 1) g_print("\n");
		if (attr_name == NULL) {
			g_print("AttrID-%s: ", attribute_values[0]);
		} else {
			g_print("%s: ", attr_name);
		}
	} else if (g_strcmp0(element_name, "sequence") == 0) {
		seq_elem_num++;
		int_elem_num = 0;
		if (seq_elem_num > 1) {
			g_print("\n");
			for (int i = 0; i < seq_elem_num; i++) g_print(" ");
		}
	} else if ((g_pattern_match(int_pattern, strlen(element_name), element_name, NULL)) && g_strcmp0(attribute_names[0], "value") == 0) {
		int_elem_num++;

		if (int_elem_num > 1) g_print(", ");
		if (current_uuid_id == SDP_UUID_RFCOMM) {
			g_print("Channel: %d", xtoi(attribute_values[0]));
		} else {
			g_print("0x%x", xtoi(attribute_values[0]));
		}
	} else if (g_strcmp0(element_name, "uuid") == 0 && g_strcmp0(attribute_names[0], "value") == 0) {
		int uuid_id = -1;
		const gchar *uuid_name;
		int_elem_num++;

		if (attribute_values[0][0] == '0' && attribute_values[0][1] == 'x') {
			uuid_id = xtoi(attribute_values[0]);
			uuid_name = sdp_get_uuid_name(uuid_id);
			current_uuid_id = uuid_id;
		} else {
			uuid_name = get_uuid_name(attribute_values[0]);
		}

		if (int_elem_num > 1) g_print(", ");
		if (uuid_name == NULL) {
			g_print("\"UUID-%s\"", attribute_values[0]);
		} else {
			g_print("\"%s\"", uuid_name);
		}
	} else if (g_strcmp0(element_name, "text") == 0 && g_strcmp0(attribute_names[0], "value") == 0) {
		int_elem_num++;

		if (int_elem_num > 1) g_print(", ");
		g_print("\"%s\"", attribute_values[0]);
	} else if (g_strcmp0(element_name, "boolean") == 0 && g_strcmp0(attribute_names[0], "value") == 0) {
		int_elem_num++;

		if (int_elem_num > 1) g_print(", ");
		g_print("%s", attribute_values[0]);
	} else {
		if (error)
			*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, "Invalid XML element: %s", element_name);
	}

	g_pattern_spec_free(int_pattern);
}

void xml_end_element(GMarkupParseContext *context,
		const gchar *element_name,
		gpointer user_data,
		GError **error)
{
	if (g_strcmp0(element_name, "record") == 0) {
		attr_elem_num = 0;
		seq_elem_num = 0;
		int_elem_num = 0;

		current_attr_id = -1;
		current_uuid_id = -1;

		g_print("\n\n");
	} else if (g_strcmp0(element_name, "attribute") == 0) {
		seq_elem_num = 0;
		int_elem_num = 0;

		current_attr_id = -1;
		current_uuid_id = -1;
	} else if (g_strcmp0(element_name, "sequence") == 0) {
		seq_elem_num--;
		int_elem_num = 0;

		current_uuid_id = -1;
	}
}

static gchar *adapter_arg = NULL;
static gboolean list_arg = FALSE;
static gchar *connect_arg = NULL;
static gchar *remove_arg = NULL;
static gchar *info_arg = NULL;
static gchar *services_arg = NULL;
static gboolean set_arg = FALSE;
static gchar *set_device_arg = NULL;
static gchar *set_property_arg = NULL;
static gchar *set_value_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "<name|mac>"},
	{"list", 'l', 0, G_OPTION_ARG_NONE, &list_arg, "List added devices", NULL},
	{"connect", 'c', 0, G_OPTION_ARG_STRING, &connect_arg, "Connect to a device", "<mac>"},
	{"remove", 'r', 0, G_OPTION_ARG_STRING, &remove_arg, "Remove device", "<name|mac>"},
	{"info", 'i', 0, G_OPTION_ARG_STRING, &info_arg, "Get info about device", "<name|mac>"},
	{"services", 's', 0, G_OPTION_ARG_STRING, &services_arg, "Discover device services", "<name|mac>"},
	{"set", 0, 0, G_OPTION_ARG_NONE, &set_arg, "Set device property", NULL},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("- a bluetooth device manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"Set Options:\n"
			"  --set <name|mac> <property> <value>\n"
			"  Where `property` is one of:\n"
			"     Alias\n"
			"     Trusted\n"
			"     Blocked\n\n"
			"Report bugs to <"PACKAGE_BUGREPORT">."
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
		gchar *created_device = adapter_create_paired_device_end(adapter, &error);
		exit_if_error(error);

		g_print("Done\n");
		g_main_loop_unref(mainloop);
		g_free(created_device);
		g_object_unref(agent);
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

		// TODO: Translate class of device ?

		g_print("[%s]\n", device_get_address(device));
		g_print("  Name: %s\n", device_get_name(device));
		g_print("  Alias: %s [rw]\n", device_get_alias(device));
		g_print("  Address: %s\n", device_get_address(device));
		g_print("  Class: %x\n", device_get_class(device));
		g_print("  Paired: %d\n", device_get_paired(device));
		g_print("  Trusted: %d [rw]\n", device_get_trusted(device));
		g_print("  Blocked: %d [rw]\n", device_get_blocked(device));
		g_print("  Connected: %d\n", device_get_connected(device));
		g_print("  UUIDs: [");
		const gchar **uuids = device_get_uuids(device);
		for (int j = 0; uuids[j] != NULL; j++) {
			if (j > 0) g_print(", ");
			g_print("%s", get_uuid_name(uuids[j]));
		}
		g_print("]\n");

		g_object_unref(device);
	} else if (services_arg) {
		Device *device = find_device(adapter, services_arg, &error);
		exit_if_error(error);

		g_print("Discovering services...\n");
		GHashTable *device_services = device_discover_services(device, NULL, &error);
		exit_if_error(error);

		GHashTableIter iter;
		gpointer key, value;

		// TOOD: Add verbose option

		g_hash_table_iter_init(&iter, device_services);
		while (g_hash_table_iter_next(&iter, &key, &value)) {
			GMarkupParser xml_parser = {xml_start_element, xml_end_element, NULL, NULL, NULL};
			GMarkupParseContext *xml_parse_context = g_markup_parse_context_new(&xml_parser, 0, NULL, NULL);
			g_markup_parse_context_parse(xml_parse_context, value, strlen(value), &error);
			exit_if_error(error);
			g_markup_parse_context_free(xml_parse_context);
			//g_print("%s", value);
		}

		g_print("Done\n");
		g_object_unref(device);
	} else if (set_arg) {
		set_device_arg = argv[1];
		set_property_arg = argv[2];
		set_value_arg = argv[3];

		Device *device = find_device(adapter, set_device_arg, &error);
		exit_if_error(error);

		GValue v = {0,};

		if (g_strcmp0(set_property_arg, "Alias") == 0) {
			g_value_init(&v, G_TYPE_STRING);
			g_value_set_string(&v, set_value_arg);
		} else if (
				g_strcmp0(set_property_arg, "Trusted") == 0 ||
				g_strcmp0(set_property_arg, "Blocked") == 0
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
		} else {
			g_print("%s: Invalid property: %s\n", g_get_prgname(), set_property_arg);
			g_print("Try `%s --help` for more information.\n", g_get_prgname());
			exit(EXIT_FAILURE);
		}

		GHashTable *props = device_get_properties(device, &error);
		exit_if_error(error);
		GValue *old_value = g_hash_table_lookup(props, set_property_arg);
		g_assert(old_value != NULL);
		if (G_VALUE_HOLDS_STRING(old_value)) {
			g_print("%s: %s -> %s\n", set_property_arg, g_value_get_string(old_value), g_value_get_string(&v));
		} else if (G_VALUE_HOLDS_BOOLEAN(old_value)) {
			g_print("%s: %d -> %d\n", set_property_arg, g_value_get_boolean(old_value), g_value_get_boolean(&v));
		}
		g_hash_table_unref(props);

		device_set_property(device, set_property_arg, &v, &error);
		exit_if_error(error);

		g_value_unset(&v);
		g_object_unref(device);
	}

	g_object_unref(adapter);

	exit(EXIT_SUCCESS);
}

