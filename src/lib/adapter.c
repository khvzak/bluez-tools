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
#include "adapter.h"

#define BLUEZ_DBUS_ADAPTER_INTERFACE "org.bluez.Adapter"

#define ADAPTER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), ADAPTER_TYPE, AdapterPrivate))

struct _AdapterPrivate {
	DBusGProxy *dbus_g_proxy;
};

G_DEFINE_TYPE(Adapter, adapter, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_ADDRESS, /* readonly */
	PROP_NAME, /* readwrite */
	PROP_CLASS, /* readonly */
	PROP_POWERED, /* readwrite */
	PROP_DISCOVERABLE, /* readwrite */
	PROP_PAIRABLE, /* readwrite */
	PROP_PAIRABLE_TIMEOUT, /* readwrite */
	PROP_DISCOVERABLE_TIMEOUT, /* readwrite */
	PROP_DISCOVERING, /* readonly */
	PROP_DEVICES, /* readonly */
	PROP_UUIDS /* readonly */
};

enum {
	PROPERTY_CHANGED,
	DEVICE_FOUND,
	DEVICE_DISAPPEARED,
	DEVICE_CREATED,
	DEVICE_REMOVED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void adapter_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void adapter_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *property, const GValue *value, gpointer data);
static void device_found_handler(DBusGProxy *dbus_g_proxy, const gchar *address, const GHashTable *properties, gpointer data);
static void device_disappeared_handler(DBusGProxy *dbus_g_proxy, const gchar *address, gpointer data);
static void device_created_handler(DBusGProxy *dbus_g_proxy, const gchar *path, gpointer data);
static void device_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *path, gpointer data);

static void adapter_class_init(AdapterClass *klass)
{
	g_type_class_add_private(klass, sizeof(AdapterPrivate));

	/* Properties registration */
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->set_property = adapter_set_property;
	gobject_class->get_property = adapter_get_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* string Address [readonly] */
	pspec = g_param_spec_string("Address", "address", "Adapter address", NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_ADDRESS, pspec);

	/* string Name [readwrite] */
	pspec = g_param_spec_string("Name", "name", "Adapter name", NULL, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	/* uint32 Class [readonly] */
	pspec = g_param_spec_uint("Class", "class", "Adapter bluetooth class", 0, 4294967295, 0, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_CLASS, pspec);

	/* boolean Powered [readwrite] */
	pspec = g_param_spec_boolean("Powered", "powered", "Adapter powered state", FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_POWERED, pspec);

	/* boolean Discoverable [readwrite] */
	pspec = g_param_spec_boolean("Discoverable", "discoverable", "Adapter discoverable state", FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_DISCOVERABLE, pspec);

	/* boolean Pairable [readwrite] */
	pspec = g_param_spec_boolean("Pairable", "pairable", "Adapter pairable state", FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_PAIRABLE, pspec);

	/* uint32 PaireableTimeout [readwrite] */
	pspec = g_param_spec_uint("PaireableTimeout", "paireable_timeout", "Adapter paireable timeout", 0, 4294967295, 0, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_PAIRABLE_TIMEOUT, pspec);

	/* uint32 DiscoverableTimeout [readwrite] */
	pspec = g_param_spec_uint("DiscoverableTimeout", "discoverable_timeout", "Adapter discoverable timeout", 0, 4294967295, 0, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_DISCOVERABLE_TIMEOUT, pspec);

	/* boolean Discovering [readonly] */
	pspec = g_param_spec_boolean("Discovering", "discovering", "Adapter discover mode state", FALSE, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_DISCOVERING, pspec);

	/* array{object} Devices [readonly] */
	pspec = g_param_spec_boxed("Devices", "devices", "List of added devices", G_TYPE_PTR_ARRAY, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_DEVICES, pspec);

	/* array{string} UUIDs [readonly] */
	pspec = g_param_spec_boxed("UUIDs", "uuids", "List of available UUIDs", G_TYPE_PTR_ARRAY, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_UUIDS, pspec);
}

static void adapter_init(Adapter *self)
{
	self->priv = ADAPTER_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void adapter_post_init(Adapter *self)
{
	/* DBUS signals connection */

	/* PropertyChanged(string name, variant value)  */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self, NULL);

	/* DeviceFound(string address, dict values) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DeviceFound", G_TYPE_STRING, dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DeviceFound", G_CALLBACK(device_found_handler), self, NULL);

	/* DeviceDisappeared(string address) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DeviceDisappeared", G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DeviceDisappeared", G_CALLBACK(device_disappeared_handler), self, NULL);

	/* DeviceCreated(object device) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DeviceCreated", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DeviceCreated", G_CALLBACK(device_created_handler), self, NULL);

	/* DeviceRemoved(object device) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DeviceRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DeviceRemoved", G_CALLBACK(device_removed_handler), self, NULL);
}

static void adapter_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Adapter *self = ADAPTER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
	{
		const gchar *dbus_object_path = g_value_get_string(value);
		g_assert(dbus_object_path != NULL);
		g_assert(self->priv->dbus_g_proxy == NULL);
		self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, dbus_object_path, BLUEZ_DBUS_ADAPTER_INTERFACE);
		adapter_post_init(self);
	}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void adapter_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Adapter *self = ADAPTER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
	{
		// TODO: check pointers
		gchar *object_path = g_strdup(dbus_g_proxy_get_path(self->priv->dbus_g_proxy));
		g_value_set_string(value, object_path);
	}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/* Signals handlers */
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data)
{
	Adapter *self = ADAPTER(data);
	g_print("property changed\n");
	//g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}
