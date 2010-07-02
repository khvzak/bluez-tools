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

static gchar *adapter_arg = NULL;
static gboolean list_arg = FALSE;
static gchar *connect_arg = NULL;
static gchar *remove_arg = NULL;
static gchar *info_arg = NULL;
static gchar *services_arg = NULL;
static gboolean set_arg = FALSE;
static gchar *set_name_arg = NULL;
static gchar *set_value_arg = NULL;

static GOptionEntry entries[] = {
	{ "adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter name or MAC", "adapter#id"},
	{ "list", 'l', 0, G_OPTION_ARG_NONE, &list_arg, "List added devices", NULL},
	{ "connect", 'c', 0, G_OPTION_ARG_STRING, &connect_arg, "Connect to a device", "device#id"},
	{ "remove", 'r', 0, G_OPTION_ARG_STRING, &remove_arg, "Remove device", "device#id"},
	{ "info", 'i', 0, G_OPTION_ARG_STRING, &info_arg, "Get info about device", "device#id"},
	{ "services", 's', 0, G_OPTION_ARG_STRING, &services_arg, "Discover device services", "device#id"},
	{ "set", 0, 0, G_OPTION_ARG_NONE, &set_arg, "Set property", NULL},
	{ NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	g_type_init();

	context = g_option_context_new("[--set device#id Name Value] - a bluetooth device manager");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "summary");
	g_option_context_set_description(context, "desc");

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
		g_printerr("Couldn't connect to dbus: %s", error->message);
		exit(EXIT_FAILURE);
	}

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	if (list_arg) {
		const GPtrArray *devices_list = adapter_get_devices(adapter);
		g_assert(devices_list != NULL);

		g_print("Added devices:\n");
		if (devices_list->len == 0) {
			g_print("no devices found\n");
		}

		for (int i = 0; i < devices_list->len; i++) {
			const gchar *device_path = g_ptr_array_index(devices_list, i);
			Device *device = g_object_new(DEVICE_TYPE, "DBusObjectPath", device_path, NULL);
			g_print("%s (%s)\n", device_get_name(device), device_get_address(device));
			g_object_unref(device);
		}
	} else if (connect_arg) {

	} else if (remove_arg) {
		Device *device = find_device(adapter, info_arg, &error);
		exit_if_error(error);

	} else if (info_arg) {
		Device *device = find_device(adapter, info_arg, &error);
		exit_if_error(error);

	} else if (services_arg) {
		Device *device = find_device(adapter, info_arg, &error);
		exit_if_error(error);

	} else if (set_arg) {
		Device *device = find_device(adapter, info_arg, &error);
		exit_if_error(error);

	}

	g_object_unref(manager);
	g_object_unref(adapter);

	exit(EXIT_SUCCESS);
}

