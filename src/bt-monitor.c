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
#include "lib/adapter.h"
#include "lib/manager.h"

static gchar *adapter_name = NULL;

static void property_changed_handler(Manager *manager, const gchar *name, const GValue *value, gpointer data)
{
	g_print("property changed: %s\n", name);
}

static void adapter_added_handler(Manager *manager, const gchar *adapter_path, gpointer data)
{
	g_print("adapter added: %s\n", adapter_path);
}

static void adapter_removed_handler(Manager *manager, const gchar *adapter_path, gpointer data)
{
	g_print("adapter removed: %s\n", adapter_path);
}

static void default_adapter_changed_handler(Manager *manager, const gchar *adapter_path, gpointer data)
{
	g_print("default adapter changed: %s\n", adapter_path);
}

static GOptionEntry entries[] = {
	{ "adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_name, "Adapter name or MAC", NULL},
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
		// ME: should I call this functions?
		g_error_free(error);
		g_option_context_free(context);
		exit(EXIT_FAILURE);
	}
	g_option_context_free(context);

	if (!dbus_connect(&error)) {
		g_printerr("Couldn't connect to dbus: %s", error->message);
		g_error_free(error);
		exit(EXIT_FAILURE);
	}

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	if (!adapter_name) {
		// Listen for all events
	} else {
		// Listen for an adapter events
		//Adapter *adapter_obj;
		//if (!find_adapter(adapter, &error, &adapter_obj)) {
		//	g_printerr("Couldn't find adapter '%s': %s\n", adapter, error->message);
		//	g_error_free(error);
		//	exit(EXIT_FAILURE);
		//}
	}

	g_signal_connect(manager, "PropertyChanged", G_CALLBACK(property_changed_handler), NULL);
	g_signal_connect(manager, "AdapterAdded", G_CALLBACK(adapter_added_handler), NULL);
	g_signal_connect(manager, "AdapterRemoved", G_CALLBACK(adapter_removed_handler), NULL);
	g_signal_connect(manager, "DefaultAdapterChanged", G_CALLBACK(default_adapter_changed_handler), NULL);

	GMainLoop *mainloop;
	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	exit(EXIT_SUCCESS);
}
