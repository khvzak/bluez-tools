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
#include "lib/sdp.h"
#include "lib/bluez-api.h"

enum {
	REC,
	ATTR,
	SEQ,
	ELEM,

	ATTR_ID,
	UUID_ID,

	LAST_E
};

static int xml_t[LAST_E] = {0, 0, 0, 0, -1, -1};

/* Main arguments */
static gchar *adapter_arg = NULL;
static gboolean list_arg = FALSE;
static gchar *connect_arg = NULL;
static gchar *disconnect_arg = NULL;
static gchar *remove_arg = NULL;
static gchar *info_arg = NULL;
static gboolean services_arg = FALSE;
static gchar *services_device_arg = NULL;
static gchar *services_pattern_arg = NULL;
static gboolean set_arg = FALSE;
static gchar *set_device_arg = NULL;
static gchar *set_property_arg = NULL;
static gchar *set_value_arg = NULL;
static gboolean verbose_arg = FALSE;

static gboolean is_verbose_attr(int attr_id)
{
	if (
			attr_id == SDP_ATTR_ID_SERVICE_CLASS_ID_LIST ||
			attr_id == SDP_ATTR_ID_PROTOCOL_DESCRIPTOR_LIST ||
			attr_id == SDP_ATTR_ID_BLUETOOTH_PROFILE_DESCRIPTOR_LIST ||
			attr_id == SDP_ATTR_ID_DOCUMENTATION_URL ||
			attr_id == SDP_ATTR_ID_SERVICE_NAME ||
			attr_id == SDP_ATTR_ID_SERVICE_DESCRIPTION ||
			attr_id == SDP_ATTR_ID_PROVIDER_NAME ||
			attr_id == SDP_ATTR_ID_SECURITY_DESCRIPTION
			)
		return FALSE;

	return TRUE;
}

static const gchar *xml_get_attr_value(const gchar *attr_name, const gchar **attribute_names, const gchar **attribute_values)
{
	for (int i = 0; attribute_names[i] != NULL; i++) {
		if (g_strcmp0(attribute_names[i], attr_name) == 0) {
			return attribute_values[i];
		}
	}

	return NULL;
}

static void xml_start_element(GMarkupParseContext *context,
		const gchar *element_name,
		const gchar **attribute_names,
		const gchar **attribute_values,
		gpointer user_data,
		GError **error)
{
	const gchar *id_t = xml_get_attr_value("id", attribute_names, attribute_values);
	const gchar *value_t = xml_get_attr_value("value", attribute_names, attribute_values);

	if (g_strcmp0(element_name, "record") == 0) {
		xml_t[REC]++;
	} else if (g_strcmp0(element_name, "attribute") == 0 && id_t) {
		int attr_id = xtoi(id_t);
		const gchar *attr_name = sdp_get_attr_id_name(attr_id);

		xml_t[ATTR]++;
		xml_t[ATTR_ID] = attr_id;

		if (!verbose_arg && is_verbose_attr(xml_t[ATTR_ID])) return;

		if (attr_name == NULL) {
			g_print("AttrID-%s: ", id_t);
		} else {
			g_print("%s: ", attr_name);
		}
	} else if (g_strcmp0(element_name, "sequence") == 0) {
		xml_t[SEQ]++;
	} else if (g_pattern_match_simple("*int*", element_name) && value_t) {
		xml_t[ELEM]++;

		if (!verbose_arg && is_verbose_attr(xml_t[ATTR_ID])) return;

		if (xml_t[ELEM] == 1 && xml_t[SEQ] > 1) {
			g_print("\n");
			for (int i = 0; i < xml_t[SEQ]; i++) g_print("  ");
		} else if (xml_t[ELEM] > 1) {
			g_print(", ");
		}

		if (xml_t[UUID_ID] == SDP_UUID_RFCOMM) {
			g_print("Channel: %d", xtoi(value_t));
		} else {
			g_print("0x%x", xtoi(value_t));
		}
	} else if (g_strcmp0(element_name, "uuid") == 0 && value_t) {
		int uuid_id = -1;
		const gchar *uuid_name;

		if (value_t[0] == '0' && value_t[1] == 'x') {
			uuid_id = xtoi(value_t);
			uuid_name = sdp_get_uuid_name(uuid_id);
		} else {
			uuid_name = uuid2name(value_t);
		}

		xml_t[ELEM]++;
		xml_t[UUID_ID] = uuid_id;

		if (!verbose_arg && is_verbose_attr(xml_t[ATTR_ID])) return;

		if (xml_t[ELEM] == 1 && xml_t[SEQ] > 1) {
			g_print("\n");
			for (int i = 0; i < xml_t[SEQ]; i++) g_print("  ");
		} else if (xml_t[ELEM] > 1) {
			g_print(", ");
		}

		if (uuid_name == NULL) {
			g_print("\"UUID-%s\"", value_t);
		} else {
			g_print("\"%s\"", uuid_name);
		}
	} else if (g_strcmp0(element_name, "text") == 0 && value_t) {
		xml_t[ELEM]++;

		if (!verbose_arg && is_verbose_attr(xml_t[ATTR_ID])) return;

		if (xml_t[ELEM] == 1 && xml_t[SEQ] > 1) {
			g_print("\n");
			for (int i = 0; i < xml_t[SEQ]; i++) g_print("  ");
		} else if (xml_t[ELEM] > 1) {
			g_print(", ");
		}

		g_print("\"%s\"", value_t);
	} else if (g_strcmp0(element_name, "boolean") == 0 && value_t) {
		xml_t[ELEM]++;

		if (!verbose_arg && is_verbose_attr(xml_t[ATTR_ID])) return;

		if (xml_t[ELEM] == 1 && xml_t[SEQ] > 1) {
			g_print("\n");
			for (int i = 0; i < xml_t[SEQ]; i++) g_print("  ");
		} else if (xml_t[ELEM] > 1) {
			g_print(", ");
		}

		g_print("%s", value_t);
	} else {
		if (error)
			*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT, "Invalid XML element: %s", element_name);
	}
}

static void xml_end_element(GMarkupParseContext *context,
		const gchar *element_name,
		gpointer user_data,
		GError **error)
{
	if (g_strcmp0(element_name, "record") == 0) {
		xml_t[ATTR] = 0;
		xml_t[SEQ] = 0;
		xml_t[ELEM] = 0;

		xml_t[ATTR_ID] = -1;
		xml_t[UUID_ID] = -1;
	} else if (g_strcmp0(element_name, "attribute") == 0) {
		xml_t[SEQ] = 0;
		xml_t[ELEM] = 0;

		int old_attr_id = xml_t[ATTR_ID];
		xml_t[ATTR_ID] = -1;
		xml_t[UUID_ID] = -1;

		if (!verbose_arg && is_verbose_attr(old_attr_id)) return;

		g_print("\n");
	} else if (g_strcmp0(element_name, "sequence") == 0) {
		xml_t[SEQ]--;
		xml_t[ELEM] = 0;

		xml_t[UUID_ID] = -1;
	}
}

static void create_paired_device_done(gpointer data)
{
	g_assert(data != NULL);
	GMainLoop *mainloop = data;
	g_main_loop_quit(mainloop);
}

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter Name or MAC", "<name|mac>"},
	{"list", 'l', 0, G_OPTION_ARG_NONE, &list_arg, "List added devices", NULL},
	{"connect", 'c', 0, G_OPTION_ARG_STRING, &connect_arg, "Connect to the remote device", "<mac>"},
	{"disconnect", 'd', 0, G_OPTION_ARG_STRING, &disconnect_arg, "Disconnect the remote device", "<name|mac>"},
	{"remove", 'r', 0, G_OPTION_ARG_STRING, &remove_arg, "Remove device", "<name|mac>"},
	{"info", 'i', 0, G_OPTION_ARG_STRING, &info_arg, "Get info about device", "<name|mac>"},
	{"services", 's', 0, G_OPTION_ARG_NONE, &services_arg, "Discover device services", NULL},
	{"set", 0, 0, G_OPTION_ARG_NONE, &set_arg, "Set device property", NULL},
	{"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_arg, "Verbosely display remote service records", NULL},
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

	context = g_option_context_new("- a bluetooth device manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"Services Options:\n"
			"  -s, --services <name|mac> [<pattern>]\n"
			"  Where `pattern` is an optional specific UUID to search\n\n"
			"Set Options:\n"
			"  --set <name|mac> <property> <value>\n"
			"  Where\n"
			"    `name|mac` is a device Name or MAC\n"
			"    `property` is one of:\n"
			"       Alias\n"
			"       Trusted\n"
			"       Blocked\n\n"
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home page <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!list_arg && (!connect_arg || strlen(connect_arg) == 0) && (!disconnect_arg || strlen(disconnect_arg) == 0) && (!remove_arg || strlen(remove_arg) == 0) && (!info_arg || strlen(info_arg) == 0) && !services_arg && !set_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (services_arg && (argc != 2 || strlen(argv[1]) == 0) && (argc != 3 || strlen(argv[1]) == 0)) {
		g_print("%s: Invalid arguments for --services\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (set_arg && (argc != 4 || strlen(argv[1]) == 0 || strlen(argv[2]) == 0 || strlen(argv[3]) == 0)) {
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

		adapter_create_paired_device_begin(adapter, create_paired_device_done, mainloop, connect_arg, AGENT_DBUS_PATH, "DisplayYesNo");
		g_main_loop_run(mainloop);
		gchar *created_device = adapter_create_paired_device_end(adapter, &error);
		exit_if_error(error);

		g_print("Done\n");
		g_main_loop_unref(mainloop);
		g_free(created_device);
		g_object_unref(agent);
	} else if (disconnect_arg) {
		Device *device = find_device(adapter, disconnect_arg, &error);
		exit_if_error(error);

		g_print("Disconnecting: %s\n", disconnect_arg);
		device_disconnect(device, &error);
		exit_if_error(error);

		g_print("Done\n");
		g_object_unref(device);
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
		g_print("  Icon: %s\n", device_get_icon(device));
		g_print("  Class: 0x%x\n", device_get_class(device));
		g_print("  Paired: %d\n", device_get_paired(device));
		g_print("  Trusted: %d [rw]\n", device_get_trusted(device));
		g_print("  Blocked: %d [rw]\n", device_get_blocked(device));
		g_print("  Connected: %d\n", device_get_connected(device));
		g_print("  UUIDs: [");
		const gchar **uuids = device_get_uuids(device);
		for (int j = 0; uuids[j] != NULL; j++) {
			if (j > 0) g_print(", ");
			g_print("%s", uuid2name(uuids[j]));
		}
		g_print("]\n");

		g_object_unref(device);
	} else if (services_arg) {
		services_device_arg = argv[1];
		if (argc == 3) {
			services_pattern_arg = argv[2];
		}

		Device *device = find_device(adapter, services_device_arg, &error);
		exit_if_error(error);

		g_print("Discovering services...\n");
		GHashTable *device_services = device_discover_services(device, name2uuid(services_pattern_arg), &error);
		exit_if_error(error);

		GHashTableIter iter;
		gpointer key, value;

		g_hash_table_iter_init(&iter, device_services);
		int n = 0;
		while (g_hash_table_iter_next(&iter, &key, &value)) {
			n++;
			if (n == 1) g_print("\n");
			g_print("[RECORD:%d]\n", (gint) key);
			GMarkupParser xml_parser = {xml_start_element, xml_end_element, NULL, NULL, NULL};
			GMarkupParseContext *xml_parse_context = g_markup_parse_context_new(&xml_parser, 0, NULL, NULL);
			g_markup_parse_context_parse(xml_parse_context, value, strlen(value), &error);
			exit_if_error(error);
			g_markup_parse_context_free(xml_parse_context);
			g_print("\n");
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
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

