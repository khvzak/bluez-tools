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

#include <string.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

#include "../dbus-common.h"
#include "../marshallers.h"

#include "adapter.h"

#define ADAPTER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), ADAPTER_TYPE, AdapterPrivate))

struct _AdapterPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;

	/* Properties */
	gchar *address;
	guint32 class;
	GPtrArray *devices;
	gboolean discoverable;
	guint32 discoverable_timeout;
	gboolean discovering;
	gchar *name;
	gboolean pairable;
	guint32 pairable_timeout;
	gboolean powered;
	gchar **uuids;

	/* Async calls */
	DBusGProxyCall *create_paired_device_call;
};

G_DEFINE_TYPE(Adapter, adapter, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_ADDRESS, /* readonly */
	PROP_CLASS, /* readonly */
	PROP_DEVICES, /* readonly */
	PROP_DISCOVERABLE, /* readwrite */
	PROP_DISCOVERABLE_TIMEOUT, /* readwrite */
	PROP_DISCOVERING, /* readonly */
	PROP_NAME, /* readwrite */
	PROP_PAIRABLE, /* readwrite */
	PROP_PAIRABLE_TIMEOUT, /* readwrite */
	PROP_POWERED, /* readwrite */
	PROP_UUIDS /* readonly */
};

static void _adapter_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _adapter_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	DEVICE_CREATED,
	DEVICE_DISAPPEARED,
	DEVICE_FOUND,
	DEVICE_REMOVED,
	PROPERTY_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void device_created_handler(DBusGProxy *dbus_g_proxy, const gchar *device, gpointer data);
static void device_disappeared_handler(DBusGProxy *dbus_g_proxy, const gchar *address, gpointer data);
static void device_found_handler(DBusGProxy *dbus_g_proxy, const gchar *address, GHashTable *values, gpointer data);
static void device_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *device, gpointer data);
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data);

static void adapter_dispose(GObject *gobject)
{
	Adapter *self = ADAPTER(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "DeviceCreated", G_CALLBACK(device_created_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "DeviceDisappeared", G_CALLBACK(device_disappeared_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "DeviceFound", G_CALLBACK(device_found_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "DeviceRemoved", G_CALLBACK(device_removed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self);

	/* Properties free */
	g_free(self->priv->address);
	g_ptr_array_unref(self->priv->devices);
	g_free(self->priv->name);
	g_strfreev(self->priv->uuids);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(adapter_parent_class)->dispose(gobject);
}

static void adapter_class_init(AdapterClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = adapter_dispose;

	g_type_class_add_private(klass, sizeof(AdapterPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _adapter_get_property;
	gobject_class->set_property = _adapter_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* string Address [readonly] */
	pspec = g_param_spec_string("Address", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_ADDRESS, pspec);

	/* uint32 Class [readonly] */
	pspec = g_param_spec_uint("Class", NULL, NULL, 0, 0xFFFFFFFF, 0, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_CLASS, pspec);

	/* array{object} Devices [readonly] */
	pspec = g_param_spec_boxed("Devices", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_DEVICES, pspec);

	/* boolean Discoverable [readwrite] */
	pspec = g_param_spec_boolean("Discoverable", NULL, NULL, FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_DISCOVERABLE, pspec);

	/* uint32 DiscoverableTimeout [readwrite] */
	pspec = g_param_spec_uint("DiscoverableTimeout", NULL, NULL, 0, 0xFFFFFFFF, 0, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_DISCOVERABLE_TIMEOUT, pspec);

	/* boolean Discovering [readonly] */
	pspec = g_param_spec_boolean("Discovering", NULL, NULL, FALSE, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_DISCOVERING, pspec);

	/* string Name [readwrite] */
	pspec = g_param_spec_string("Name", NULL, NULL, NULL, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	/* boolean Pairable [readwrite] */
	pspec = g_param_spec_boolean("Pairable", NULL, NULL, FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_PAIRABLE, pspec);

	/* uint32 PairableTimeout [readwrite] */
	pspec = g_param_spec_uint("PairableTimeout", NULL, NULL, 0, 0xFFFFFFFF, 0, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_PAIRABLE_TIMEOUT, pspec);

	/* boolean Powered [readwrite] */
	pspec = g_param_spec_boolean("Powered", NULL, NULL, FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_POWERED, pspec);

	/* array{string} UUIDs [readonly] */
	pspec = g_param_spec_boxed("UUIDs", NULL, NULL, G_TYPE_STRV, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_UUIDS, pspec);

	/* Signals registation */
	signals[DEVICE_CREATED] = g_signal_new("DeviceCreated",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[DEVICE_DISAPPEARED] = g_signal_new("DeviceDisappeared",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[DEVICE_FOUND] = g_signal_new("DeviceFound",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bt_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_HASH_TABLE);

	signals[DEVICE_REMOVED] = g_signal_new("DeviceRemoved",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[PROPERTY_CHANGED] = g_signal_new("PropertyChanged",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bt_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);
}

static void adapter_init(Adapter *self)
{
	self->priv = ADAPTER_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	/* Async calls init */
	self->priv->create_paired_device_call = NULL;

	g_assert(system_conn != NULL);
}

static void adapter_post_init(Adapter *self, const gchar *dbus_object_path)
{
	g_assert(dbus_object_path != NULL);
	g_assert(strlen(dbus_object_path) > 0);
	g_assert(self->priv->dbus_g_proxy == NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(system_conn, "org.bluez", dbus_object_path, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", ADAPTER_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", ADAPTER_DBUS_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(system_conn, "org.bluez", dbus_object_path, ADAPTER_DBUS_INTERFACE);

	/* DBus signals connection */

	/* DeviceCreated(object device) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DeviceCreated", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DeviceCreated", G_CALLBACK(device_created_handler), self, NULL);

	/* DeviceDisappeared(string address) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DeviceDisappeared", G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DeviceDisappeared", G_CALLBACK(device_disappeared_handler), self, NULL);

	/* DeviceFound(string address, dict values) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DeviceFound", G_TYPE_STRING, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DeviceFound", G_CALLBACK(device_found_handler), self, NULL);

	/* DeviceRemoved(object device) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "DeviceRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "DeviceRemoved", G_CALLBACK(device_removed_handler), self, NULL);

	/* PropertyChanged(string name, variant value) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self, NULL);

	/* Properties init */
	GHashTable *properties = adapter_get_properties(self, &error);
	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
	g_assert(properties != NULL);

	/* string Address [readonly] */
	if (g_hash_table_lookup(properties, "Address")) {
		self->priv->address = g_value_dup_string(g_hash_table_lookup(properties, "Address"));
	} else {
		self->priv->address = g_strdup("undefined");
	}

	/* uint32 Class [readonly] */
	if (g_hash_table_lookup(properties, "Class")) {
		self->priv->class = g_value_get_uint(g_hash_table_lookup(properties, "Class"));
	} else {
		self->priv->class = 0;
	}

	/* array{object} Devices [readonly] */
	if (g_hash_table_lookup(properties, "Devices")) {
		self->priv->devices = g_value_dup_boxed(g_hash_table_lookup(properties, "Devices"));
	} else {
		self->priv->devices = g_ptr_array_new();
	}

	/* boolean Discoverable [readwrite] */
	if (g_hash_table_lookup(properties, "Discoverable")) {
		self->priv->discoverable = g_value_get_boolean(g_hash_table_lookup(properties, "Discoverable"));
	} else {
		self->priv->discoverable = FALSE;
	}

	/* uint32 DiscoverableTimeout [readwrite] */
	if (g_hash_table_lookup(properties, "DiscoverableTimeout")) {
		self->priv->discoverable_timeout = g_value_get_uint(g_hash_table_lookup(properties, "DiscoverableTimeout"));
	} else {
		self->priv->discoverable_timeout = 0;
	}

	/* boolean Discovering [readonly] */
	if (g_hash_table_lookup(properties, "Discovering")) {
		self->priv->discovering = g_value_get_boolean(g_hash_table_lookup(properties, "Discovering"));
	} else {
		self->priv->discovering = FALSE;
	}

	/* string Name [readwrite] */
	if (g_hash_table_lookup(properties, "Name")) {
		self->priv->name = g_value_dup_string(g_hash_table_lookup(properties, "Name"));
	} else {
		self->priv->name = g_strdup("undefined");
	}

	/* boolean Pairable [readwrite] */
	if (g_hash_table_lookup(properties, "Pairable")) {
		self->priv->pairable = g_value_get_boolean(g_hash_table_lookup(properties, "Pairable"));
	} else {
		self->priv->pairable = FALSE;
	}

	/* uint32 PairableTimeout [readwrite] */
	if (g_hash_table_lookup(properties, "PairableTimeout")) {
		self->priv->pairable_timeout = g_value_get_uint(g_hash_table_lookup(properties, "PairableTimeout"));
	} else {
		self->priv->pairable_timeout = 0;
	}

	/* boolean Powered [readwrite] */
	if (g_hash_table_lookup(properties, "Powered")) {
		self->priv->powered = g_value_get_boolean(g_hash_table_lookup(properties, "Powered"));
	} else {
		self->priv->powered = FALSE;
	}

	/* array{string} UUIDs [readonly] */
	if (g_hash_table_lookup(properties, "UUIDs")) {
		self->priv->uuids = (gchar **) g_value_dup_boxed(g_hash_table_lookup(properties, "UUIDs"));
	} else {
		self->priv->uuids = g_new0(char *, 1);
		self->priv->uuids[0] = NULL;
	}

	g_hash_table_unref(properties);
}

static void _adapter_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Adapter *self = ADAPTER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, adapter_get_dbus_object_path(self));
		break;

	case PROP_ADDRESS:
		g_value_set_string(value, adapter_get_address(self));
		break;

	case PROP_CLASS:
		g_value_set_uint(value, adapter_get_class(self));
		break;

	case PROP_DEVICES:
		g_value_set_boxed(value, adapter_get_devices(self));
		break;

	case PROP_DISCOVERABLE:
		g_value_set_boolean(value, adapter_get_discoverable(self));
		break;

	case PROP_DISCOVERABLE_TIMEOUT:
		g_value_set_uint(value, adapter_get_discoverable_timeout(self));
		break;

	case PROP_DISCOVERING:
		g_value_set_boolean(value, adapter_get_discovering(self));
		break;

	case PROP_NAME:
		g_value_set_string(value, adapter_get_name(self));
		break;

	case PROP_PAIRABLE:
		g_value_set_boolean(value, adapter_get_pairable(self));
		break;

	case PROP_PAIRABLE_TIMEOUT:
		g_value_set_uint(value, adapter_get_pairable_timeout(self));
		break;

	case PROP_POWERED:
		g_value_set_boolean(value, adapter_get_powered(self));
		break;

	case PROP_UUIDS:
		g_value_set_boxed(value, adapter_get_uuids(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _adapter_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Adapter *self = ADAPTER(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		adapter_post_init(self, g_value_get_string(value));
		break;

	case PROP_DISCOVERABLE:
		adapter_set_property(self, "Discoverable", value, &error);
		break;

	case PROP_DISCOVERABLE_TIMEOUT:
		adapter_set_property(self, "DiscoverableTimeout", value, &error);
		break;

	case PROP_NAME:
		adapter_set_property(self, "Name", value, &error);
		break;

	case PROP_PAIRABLE:
		adapter_set_property(self, "Pairable", value, &error);
		break;

	case PROP_PAIRABLE_TIMEOUT:
		adapter_set_property(self, "PairableTimeout", value, &error);
		break;

	case PROP_POWERED:
		adapter_set_property(self, "Powered", value, &error);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

static void adapter_async_notify_callback(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	gpointer *p = data;
	void (*AsyncNotifyFunc)(gpointer data) = p[0];
	(*AsyncNotifyFunc)(p[1]);
	g_free(p);
}

/* Methods */

/* void CancelDeviceCreation(string address) */
void adapter_cancel_device_creation(Adapter *self, const gchar *address, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CancelDeviceCreation", error, G_TYPE_STRING, address, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* object CreateDevice(string address) */
gchar *adapter_create_device(Adapter *self, const gchar *address, GError **error)
{
	g_assert(ADAPTER_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CreateDevice", error, G_TYPE_STRING, address, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID);

	return ret;
}

/* object CreatePairedDevice(string address, object agent, string capability) [async] */
void adapter_create_paired_device_begin(Adapter *self, void (*AsyncNotifyFunc)(gpointer data), gpointer data, const gchar *address, const gchar *agent, const gchar *capability)
{
	g_assert(ADAPTER_IS(self));
	g_assert(self->priv->create_paired_device_call == NULL);

	gpointer *p = g_new0(gpointer, 2);
	p[0] = AsyncNotifyFunc;
	p[1] = data;

	self->priv->create_paired_device_call = dbus_g_proxy_begin_call(self->priv->dbus_g_proxy, "CreatePairedDevice", (DBusGProxyCallNotify) adapter_async_notify_callback, p, NULL, G_TYPE_STRING, address, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_STRING, capability, G_TYPE_INVALID);
}

gchar *adapter_create_paired_device_end(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));
	g_assert(self->priv->create_paired_device_call != NULL);

	gchar *ret = NULL;
	dbus_g_proxy_end_call(self->priv->dbus_g_proxy, self->priv->create_paired_device_call, error, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID);
	self->priv->create_paired_device_call = NULL;

	return ret;
}

/* object FindDevice(string address) */
gchar *adapter_find_device(Adapter *self, const gchar *address, GError **error)
{
	g_assert(ADAPTER_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "FindDevice", error, G_TYPE_STRING, address, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID);

	return ret;
}

/* dict GetProperties() */
GHashTable *adapter_get_properties(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* void RegisterAgent(object agent, string capability) */
void adapter_register_agent(Adapter *self, const gchar *agent, const gchar *capability, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RegisterAgent", error, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_STRING, capability, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void RemoveDevice(object device) */
void adapter_remove_device(Adapter *self, const gchar *device, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RemoveDevice", error, DBUS_TYPE_G_OBJECT_PATH, device, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void SetProperty(string name, variant value) */
void adapter_set_property(Adapter *self, const gchar *name, const GValue *value, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "SetProperty", error, G_TYPE_STRING, name, G_TYPE_VALUE, value, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void StartDiscovery() */
void adapter_start_discovery(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "StartDiscovery", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void StopDiscovery() */
void adapter_stop_discovery(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "StopDiscovery", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void UnregisterAgent(object agent) */
void adapter_unregister_agent(Adapter *self, const gchar *agent, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "UnregisterAgent", error, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *adapter_get_dbus_object_path(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

const gchar *adapter_get_address(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->address;
}

const guint32 adapter_get_class(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->class;
}

const GPtrArray *adapter_get_devices(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->devices;
}

const gboolean adapter_get_discoverable(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->discoverable;
}

void adapter_set_discoverable(Adapter *self, const gboolean value)
{
	g_assert(ADAPTER_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_BOOLEAN);
	g_value_set_boolean(&t, value);
	adapter_set_property(self, "Discoverable", &t, &error);
	g_value_unset(&t);

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

const guint32 adapter_get_discoverable_timeout(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->discoverable_timeout;
}

void adapter_set_discoverable_timeout(Adapter *self, const guint32 value)
{
	g_assert(ADAPTER_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_UINT);
	g_value_set_uint(&t, value);
	adapter_set_property(self, "DiscoverableTimeout", &t, &error);
	g_value_unset(&t);

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

const gboolean adapter_get_discovering(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->discovering;
}

const gchar *adapter_get_name(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->name;
}

void adapter_set_name(Adapter *self, const gchar *value)
{
	g_assert(ADAPTER_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_STRING);
	g_value_set_string(&t, value);
	adapter_set_property(self, "Name", &t, &error);
	g_value_unset(&t);

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

const gboolean adapter_get_pairable(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->pairable;
}

void adapter_set_pairable(Adapter *self, const gboolean value)
{
	g_assert(ADAPTER_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_BOOLEAN);
	g_value_set_boolean(&t, value);
	adapter_set_property(self, "Pairable", &t, &error);
	g_value_unset(&t);

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

const guint32 adapter_get_pairable_timeout(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->pairable_timeout;
}

void adapter_set_pairable_timeout(Adapter *self, const guint32 value)
{
	g_assert(ADAPTER_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_UINT);
	g_value_set_uint(&t, value);
	adapter_set_property(self, "PairableTimeout", &t, &error);
	g_value_unset(&t);

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

const gboolean adapter_get_powered(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->powered;
}

void adapter_set_powered(Adapter *self, const gboolean value)
{
	g_assert(ADAPTER_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_BOOLEAN);
	g_value_set_boolean(&t, value);
	adapter_set_property(self, "Powered", &t, &error);
	g_value_unset(&t);

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

const gchar **adapter_get_uuids(Adapter *self)
{
	g_assert(ADAPTER_IS(self));

	return self->priv->uuids;
}

/* Signals handlers */
static void device_created_handler(DBusGProxy *dbus_g_proxy, const gchar *device, gpointer data)
{
	Adapter *self = ADAPTER(data);

	g_signal_emit(self, signals[DEVICE_CREATED], 0, device);
}

static void device_disappeared_handler(DBusGProxy *dbus_g_proxy, const gchar *address, gpointer data)
{
	Adapter *self = ADAPTER(data);

	g_signal_emit(self, signals[DEVICE_DISAPPEARED], 0, address);
}

static void device_found_handler(DBusGProxy *dbus_g_proxy, const gchar *address, GHashTable *values, gpointer data)
{
	Adapter *self = ADAPTER(data);

	g_signal_emit(self, signals[DEVICE_FOUND], 0, address, values);
}

static void device_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *device, gpointer data)
{
	Adapter *self = ADAPTER(data);

	g_signal_emit(self, signals[DEVICE_REMOVED], 0, device);
}

static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data)
{
	Adapter *self = ADAPTER(data);

	if (g_strcmp0(name, "Address") == 0) {
		g_free(self->priv->address);
		self->priv->address = g_value_dup_string(value);
	} else if (g_strcmp0(name, "Class") == 0) {
		self->priv->class = g_value_get_uint(value);
	} else if (g_strcmp0(name, "Devices") == 0) {
		g_ptr_array_unref(self->priv->devices);
		self->priv->devices = g_value_dup_boxed(value);
	} else if (g_strcmp0(name, "Discoverable") == 0) {
		self->priv->discoverable = g_value_get_boolean(value);
	} else if (g_strcmp0(name, "DiscoverableTimeout") == 0) {
		self->priv->discoverable_timeout = g_value_get_uint(value);
	} else if (g_strcmp0(name, "Discovering") == 0) {
		self->priv->discovering = g_value_get_boolean(value);
	} else if (g_strcmp0(name, "Name") == 0) {
		g_free(self->priv->name);
		self->priv->name = g_value_dup_string(value);
	} else if (g_strcmp0(name, "Pairable") == 0) {
		self->priv->pairable = g_value_get_boolean(value);
	} else if (g_strcmp0(name, "PairableTimeout") == 0) {
		self->priv->pairable_timeout = g_value_get_uint(value);
	} else if (g_strcmp0(name, "Powered") == 0) {
		self->priv->powered = g_value_get_boolean(value);
	} else if (g_strcmp0(name, "UUIDs") == 0) {
		g_strfreev(self->priv->uuids);
		self->priv->uuids = (gchar **) g_value_dup_boxed(value);
	}

	g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}

