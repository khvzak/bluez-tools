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
	g_assert(data != NULL);
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
static gboolean service_arg = FALSE;
static gchar *service_name_arg = NULL;
static gchar *service_property_arg = NULL;
static gchar *service_value_arg = NULL;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "<name|mac>"},
	{"connect", 'c', 0, G_OPTION_ARG_NONE, &connect_arg, "Connect to a network device", NULL},
	{"disconnect", 'd', 0, G_OPTION_ARG_STRING, &disconnect_arg, "Disconnect from a network device", "<name|mac>"},
	{"service", 's', 0, G_OPTION_ARG_NONE, &service_arg, "Manage GN/PANU/NAP services", NULL},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("- a bluetooth network manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"Connect Options:\n"
			"  -c, --connect <name|mac> <pattern>\n"
			"  Where `pattern` is:\n"
			"     UUID 128 bit string\n"
			"     Profile short name: gn, panu or nap\n"
			"     UUID hexadecimal number\n\n"
			"Service Options:\n"
			" -s, --service <gn|panu|nap> [<property> <value>]\n"
			"  Where `property` is one of:\n"
			"     Name\n"
			"     Enabled\n"
			"  By default - show status\n\n"
			"Report bugs to <"PACKAGE_BUGREPORT">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (!connect_arg && !disconnect_arg && !service_arg) {
		g_print("%s", g_option_context_get_help(context, FALSE, NULL));
		exit(EXIT_FAILURE);
	} else if (connect_arg && argc != 3) {
		g_print("%s: Invalid arguments for --connect\n", g_get_prgname());
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	} else if (service_arg && argc != 2 && argc != 4) {
		g_print("%s: Invalid arguments for --service\n", g_get_prgname());
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

	if (connect_arg || disconnect_arg) {
		if (connect_arg) {
			connect_device_arg = argv[1];
			connect_service_arg = argv[2];
		}

		Device *device = find_device(adapter, connect_device_arg != NULL ? connect_device_arg : disconnect_arg, &error);
		exit_if_error(error);

		if (!intf_is_supported(device_get_dbus_object_path(device), NETWORK_INTF)) {
			g_printerr("Network service is not supported by this device\n");
			exit(EXIT_FAILURE);
		}

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
			g_print("UUID: %s (%s)\n", uuid2name(network_get_uuid(network)), network_get_uuid(network));
		} else if (disconnect_arg) {
			if (network_get_connected(network) == FALSE) {
				g_print("Network service is already disconnected\n");
			} else {
				network_disconnect(network, &error);
				exit_if_error(error);
				g_main_loop_run(mainloop);
			}
		}

		g_object_unref(network);
		g_object_unref(device);
		g_main_loop_unref(mainloop);
	} else if (service_arg) {
		GValue v = {0,};

		service_name_arg = argv[1];
		if (argc == 4) {
			service_property_arg = argv[2];
			service_value_arg = argv[3];

			if (g_strcmp0(service_property_arg, "Name") == 0) {
				g_value_init(&v, G_TYPE_STRING);
				g_value_set_string(&v, service_value_arg);
			} else if (g_strcmp0(service_property_arg, "Enabled") == 0) {
				g_value_init(&v, G_TYPE_BOOLEAN);

				if (g_strcmp0(service_value_arg, "0") == 0 || g_ascii_strcasecmp(service_value_arg, "FALSE") == 0 || g_ascii_strcasecmp(service_value_arg, "OFF") == 0) {
					g_value_set_boolean(&v, FALSE);
				} else if (g_strcmp0(service_value_arg, "1") == 0 || g_ascii_strcasecmp(service_value_arg, "TRUE") == 0 || g_ascii_strcasecmp(service_value_arg, "ON") == 0) {
					g_value_set_boolean(&v, TRUE);
				} else {
					g_print("%s: Invalid boolean value: %s\n", g_get_prgname(), service_value_arg);
					g_print("Try `%s --help` for more information.\n", g_get_prgname());
					exit(EXIT_FAILURE);
				}
			} else {
				g_print("%s: Invalid property: %s\n", g_get_prgname(), service_property_arg);
				g_print("Try `%s --help` for more information.\n", g_get_prgname());
				exit(EXIT_FAILURE);
			}
		}

		if (g_ascii_strcasecmp(service_name_arg, "GN") == 0) {
			if (!intf_is_supported(adapter_get_dbus_object_path(adapter), NETWORK_HUB_INTF)) {
				g_printerr("GN service is not supported by this adapter\n");
				exit(EXIT_FAILURE);
			}

			NetworkHub *hub = g_object_new(NETWORK_HUB_TYPE, "DBusObjectPath", adapter_get_dbus_object_path(adapter), NULL);

			if (service_property_arg == NULL) {
				g_print("[Service: GN]\n");
				g_print("  Name: %s\n", network_hub_get_name(hub));
				g_print("  Enabled: %d\n", network_hub_get_enabled(hub));
				g_print("  UUID: %s (%s)\n", uuid2name(network_hub_get_uuid(hub)), network_hub_get_uuid(hub));
			} else {
				GHashTable *props = network_hub_get_properties(hub, &error);
				exit_if_error(error);
				GValue *old_value = g_hash_table_lookup(props, service_property_arg);
				g_assert(old_value != NULL);
				if (G_VALUE_HOLDS_STRING(old_value)) {
					g_print("%s: %s -> %s\n", service_property_arg, g_value_get_string(old_value), g_value_get_string(&v));
				} else if (G_VALUE_HOLDS_BOOLEAN(old_value)) {
					g_print("%s: %d -> %d\n", service_property_arg, g_value_get_boolean(old_value), g_value_get_boolean(&v));
				}
				g_hash_table_unref(props);

				network_hub_set_property(hub, service_property_arg, &v, &error);
				exit_if_error(error);
			}

			g_object_unref(hub);
		} else if (g_ascii_strcasecmp(service_name_arg, "PANU") == 0) {
			if (!intf_is_supported(adapter_get_dbus_object_path(adapter), NETWORK_PEER_INTF)) {
				g_printerr("PANU service is not supported by this adapter\n");
				exit(EXIT_FAILURE);
			}

			NetworkPeer *peer = g_object_new(NETWORK_PEER_TYPE, "DBusObjectPath", adapter_get_dbus_object_path(adapter), NULL);

			if (service_property_arg == NULL) {
				g_print("[Service: PANU]\n");
				g_print("  Name: %s\n", network_peer_get_name(peer));
				g_print("  Enabled: %d\n", network_peer_get_enabled(peer));
				g_print("  UUID: %s (%s)\n", uuid2name(network_peer_get_uuid(peer)), network_peer_get_uuid(peer));
			} else {
				GHashTable *props = network_peer_get_properties(peer, &error);
				exit_if_error(error);
				GValue *old_value = g_hash_table_lookup(props, service_property_arg);
				g_assert(old_value != NULL);
				if (G_VALUE_HOLDS_STRING(old_value)) {
					g_print("%s: %s -> %s\n", service_property_arg, g_value_get_string(old_value), g_value_get_string(&v));
				} else if (G_VALUE_HOLDS_BOOLEAN(old_value)) {
					g_print("%s: %d -> %d\n", service_property_arg, g_value_get_boolean(old_value), g_value_get_boolean(&v));
				}
				g_hash_table_unref(props);

				network_peer_set_property(peer, service_property_arg, &v, &error);
				exit_if_error(error);
			}

			g_object_unref(peer);
		} else if (g_ascii_strcasecmp(service_name_arg, "NAP") == 0) {
			if (!intf_is_supported(adapter_get_dbus_object_path(adapter), NETWORK_ROUTER_INTF)) {
				g_printerr("NAP service is not supported by this adapter\n");
				exit(EXIT_FAILURE);
			}

			NetworkRouter *router = g_object_new(NETWORK_ROUTER_TYPE, "DBusObjectPath", adapter_get_dbus_object_path(adapter), NULL);

			if (service_property_arg == NULL) {
				g_print("[Service: NAP]\n");
				g_print("  Name: %s\n", network_router_get_name(router));
				g_print("  Enabled: %d\n", network_router_get_enabled(router));
				g_print("  UUID: %s (%s)\n", uuid2name(network_router_get_uuid(router)), network_router_get_uuid(router));
			} else {
				GHashTable *props = network_router_get_properties(router, &error);
				exit_if_error(error);
				GValue *old_value = g_hash_table_lookup(props, service_property_arg);
				g_assert(old_value != NULL);
				if (G_VALUE_HOLDS_STRING(old_value)) {
					g_print("%s: %s -> %s\n", service_property_arg, g_value_get_string(old_value), g_value_get_string(&v));
				} else if (G_VALUE_HOLDS_BOOLEAN(old_value)) {
					g_print("%s: %d -> %d\n", service_property_arg, g_value_get_boolean(old_value), g_value_get_boolean(&v));
				}
				g_hash_table_unref(props);

				network_router_set_property(router, service_property_arg, &v, &error);
				exit_if_error(error);
			}

			g_object_unref(router);
		} else {
			g_print("%s: Invalid service name: %s\n", g_get_prgname(), service_name_arg);
			g_print("Try `%s --help` for more information.\n", g_get_prgname());
			exit(EXIT_FAILURE);
		}

		if (argc == 4) {
			g_value_unset(&v);
		}
	}

	g_object_unref(adapter);
	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

