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

#include "dbus-common.h"
#include "marshallers.h"
#include "manager.h"

#define BLUEZ_DBUS_MANAGER_PATH "/"
#define BLUEZ_DBUS_MANAGER_INTERFACE "org.bluez.Manager"

#define MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), MANAGER_TYPE, ManagerPrivate))

struct _ManagerPrivate {
	DBusGProxy *dbus_g_proxy;
};

G_DEFINE_TYPE(Manager, manager, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_ADAPTERS
};

enum {
	PROPERTY_CHANGED,
	ADAPTER_ADDED,
	ADAPTER_REMOVED,
	DEFAULT_ADAPTER_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void manager_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void manager_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *property, const GValue *value, gpointer data);
static void adapter_added_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter_path, gpointer data);
static void adapter_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter_path, gpointer data);
static void default_adapter_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter_path, gpointer data);

static void manager_class_init(ManagerClass *klass)
{
	g_type_class_add_private(klass, sizeof(ManagerPrivate));

	/* Properties registration */
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->set_property = manager_set_property;
	gobject_class->get_property = manager_get_property;

	pspec = g_param_spec_boxed("Adapters", "adapters", "List of adapters", G_TYPE_PTR_ARRAY, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_ADAPTERS, pspec);

	signals[PROPERTY_CHANGED] = g_signal_new("PropertyChanged",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bluez_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);

	signals[ADAPTER_ADDED] = g_signal_new("AdapterAdded",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[ADAPTER_REMOVED] = g_signal_new("AdapterRemoved",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[DEFAULT_ADAPTER_CHANGED] = g_signal_new("DefaultAdapterChanged",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void manager_init(Manager *self)
{
	self->priv = MANAGER_GET_PRIVATE(self);

	g_assert(conn != NULL);

	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, BLUEZ_DBUS_MANAGER_PATH, BLUEZ_DBUS_MANAGER_INTERFACE);

	/* DBUS signals connection */

	/* PropertyChanged(string name, variant value)  */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self, NULL);

	/* AdapterAdded(object adapter) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "AdapterAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "AdapterAdded", G_CALLBACK(adapter_added_handler), self, NULL);

	/* AdapterRemoved(object adapter) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "AdapterRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "AdapterRemoved", G_CALLBACK(adapter_removed_handler), self, NULL);

	/* DefaultAdapterChanged(object adapter) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DefaultAdapterChanged", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DefaultAdapterChanged", G_CALLBACK(default_adapter_changed_handler), self, NULL);
}

static void manager_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Manager *self = MANAGER(object);

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void manager_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Manager *self = MANAGER(object);

	GHashTable *properties;
	if (!manager_get_properties(self, NULL, &properties)) {
		return;
	}

	switch (property_id) {
	case PROP_ADAPTERS:
		g_value_set_boxed(value, g_value_dup_boxed(g_hash_table_lookup(properties, "Adapters")));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

	g_hash_table_unref(properties);
}

/* Methods */

/* dict GetProperties()
 *
 * Properties:
 * 	array{object} Adapters
 *
 */
gboolean manager_get_properties(Manager *self, GError **error, GHashTable **properties)
{
	g_assert(self != NULL);

	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), properties, G_TYPE_INVALID)) {
		return FALSE;
	}

	return TRUE;
}

/* object DefaultAdapter() */
gboolean manager_get_default_adapter(Manager *self, GError **error, gchar **adapter_path)
{
	g_assert(self != NULL);

	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "DefaultAdapter", error, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, adapter_path, G_TYPE_INVALID)) {
		return FALSE;
	}

	return TRUE;
}

/* object FindAdapter(string pattern) */
gboolean manager_find_adapter(Manager *self, const gchar *adapter_name, GError **error, gchar **adapter_path)
{
	g_assert(self != NULL);

	if (adapter_name == NULL) {
		return manager_get_default_adapter(self, error, adapter_path);
	}

	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "FindAdapter", error, G_TYPE_STRING, adapter_name, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, adapter_path, G_TYPE_INVALID)) {
		return FALSE;
	}

	return TRUE;
}

/* Signals handlers */
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data)
{
	Manager *self = MANAGER(data);
	g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}

static void adapter_added_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter_path, gpointer data)
{
	Manager *self = MANAGER(data);
	g_signal_emit(self, signals[ADAPTER_ADDED], 0, adapter_path);
}

static void adapter_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter_path, gpointer data)
{
	Manager *self = MANAGER(data);
	g_signal_emit(self, signals[ADAPTER_REMOVED], 0, adapter_path);
}

static void default_adapter_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter_path, gpointer data)
{
	Manager *self = MANAGER(data);
	g_signal_emit(self, signals[DEFAULT_ADAPTER_CHANGED], 0, adapter_path);
}
