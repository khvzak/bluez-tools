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

	/* Properties */
	GPtrArray *adapters;
};

G_DEFINE_TYPE(Manager, manager, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_ADAPTERS /* readonly */
};

enum {
	ADAPTER_ADDED,
	ADAPTER_REMOVED,
	DEFAULT_ADAPTER_CHANGED,
	PROPERTY_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void _manager_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _manager_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void adapter_added_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter, gpointer data);
static void adapter_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter, gpointer data);
static void default_adapter_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter, gpointer data);
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data);

static void manager_dispose(GObject *gobject)
{
	Manager *self = MANAGER(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "AdapterAdded", G_CALLBACK(adapter_added_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "AdapterRemoved", G_CALLBACK(adapter_removed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "DefaultAdapterChanged", G_CALLBACK(default_adapter_changed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self);

	/* Properties free */
	g_ptr_array_unref(self->priv->adapters);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(manager_parent_class)->dispose(gobject);
}

static void manager_class_init(ManagerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = manager_dispose;

	g_type_class_add_private(klass, sizeof(ManagerPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _manager_get_property;
	gobject_class->set_property = _manager_set_property;

	/* array{object} Adapters [readonly] */
	pspec = g_param_spec_boxed("Adapters", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_ADAPTERS, pspec);

	/* Signals registation */
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

	signals[PROPERTY_CHANGED] = g_signal_new("PropertyChanged",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bluez_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);
}

static void manager_init(Manager *self)
{
	self->priv = MANAGER_GET_PRIVATE(self);

	g_assert(conn != NULL);

	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, BLUEZ_DBUS_MANAGER_PATH, BLUEZ_DBUS_MANAGER_INTERFACE);

	g_assert(self->priv->dbus_g_proxy != NULL);

	/* DBus signals connection */

	/* AdapterAdded(object adapter) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "AdapterAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "AdapterAdded", G_CALLBACK(adapter_added_handler), self, NULL);

	/* AdapterRemoved(object adapter) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "AdapterRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "AdapterRemoved", G_CALLBACK(adapter_removed_handler), self, NULL);

	/* DefaultAdapterChanged(object adapter) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DefaultAdapterChanged", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DefaultAdapterChanged", G_CALLBACK(default_adapter_changed_handler), self, NULL);

	/* PropertyChanged(string name, variant value) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self, NULL);

	/* Properties init */
	GError *error = NULL;
	GHashTable *properties = manager_get_properties(self, &error);
	g_assert(error == NULL);
	g_assert(properties != NULL);

	/* array{object} Adapters [readonly] */
	self->priv->adapters = g_value_dup_boxed(g_hash_table_lookup(properties, "Adapters"));

	g_hash_table_unref(properties);
}

static void _manager_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Manager *self = MANAGER(object);

	switch (property_id) {
	case PROP_ADAPTERS:
		g_value_set_boxed(value, manager_get_adapters(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _manager_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Manager *self = MANAGER(object);

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/* Methods */

/* object DefaultAdapter() */
gchar *manager_default_adapter(Manager *self, GError **error)
{
	g_assert(MANAGER_IS(self));

	gchar *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "DefaultAdapter", error, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* object FindAdapter(string pattern) */
gchar *manager_find_adapter(Manager *self, const gchar *pattern, GError **error)
{
	g_assert(MANAGER_IS(self));

	gchar *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "FindAdapter", error, G_TYPE_STRING, pattern, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* dict GetProperties() */
GHashTable *manager_get_properties(Manager *self, GError **error)
{
	g_assert(MANAGER_IS(self));

	GHashTable *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* Properties access methods */
const GPtrArray *manager_get_adapters(Manager *self)
{
	g_assert(MANAGER_IS(self));

	return self->priv->adapters;
}

/* Signals handlers */
static void adapter_added_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter, gpointer data)
{
	Manager *self = MANAGER(data);

	g_signal_emit(self, signals[ADAPTER_ADDED], 0, adapter);
}

static void adapter_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter, gpointer data)
{
	Manager *self = MANAGER(data);

	g_signal_emit(self, signals[ADAPTER_REMOVED], 0, adapter);
}

static void default_adapter_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *adapter, gpointer data)
{
	Manager *self = MANAGER(data);

	g_signal_emit(self, signals[DEFAULT_ADAPTER_CHANGED], 0, adapter);
}

static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data)
{
	Manager *self = MANAGER(data);

	if (g_strcmp0(name, "Adapters") == 0) {
		g_ptr_array_unref(self->priv->adapters);
		self->priv->adapters = g_value_dup_boxed(value);
	}

	g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}

