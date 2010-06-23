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
	PROP_CLASS, /* readonly */
	PROP_DEVICES, /* readonly */
	PROP_DISCOVERABLE, /* readwrite */
	PROP_DISCOVERABLE_TIMEOUT, /* readwrite */
	PROP_DISCOVERING, /* readonly */
	PROP_NAME, /* readwrite */
	PROP_PAIRABLE, /* readwrite */
	PROP_PAIREABLE_TIMEOUT, /* readwrite */
	PROP_POWERED, /* readwrite */
	PROP_UUIDS /* readonly */
};

enum {
	DEVICE_CREATED,
	DEVICE_DISAPPEARED,
	DEVICE_FOUND,
	DEVICE_REMOVED,
	PROPERTY_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void _adapter_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _adapter_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void device_created_handler(DBusGProxy *dbus_g_proxy, const gchar *device, gpointer data);
static void device_disappeared_handler(DBusGProxy *dbus_g_proxy, const gchar *address, gpointer data);
static void device_found_handler(DBusGProxy *dbus_g_proxy, const gchar *address, const GHashTable *values, gpointer data);
static void device_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *device, gpointer data);
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data);

static void adapter_class_init(AdapterClass *klass)
{
	g_type_class_add_private(klass, sizeof(AdapterPrivate));

	/* Properties registration */
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
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
	pspec = g_param_spec_uint("Class", NULL, NULL, 0, 65535, 0, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_CLASS, pspec);

	/* array{object} Devices [readonly] */
	pspec = g_param_spec_boxed("Devices", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_DEVICES, pspec);

	/* boolean Discoverable [readwrite] */
	pspec = g_param_spec_boolean("Discoverable", NULL, NULL, FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_DISCOVERABLE, pspec);

	/* uint32 DiscoverableTimeout [readwrite] */
	pspec = g_param_spec_uint("DiscoverableTimeout", NULL, NULL, 0, 65535, 0, G_PARAM_READWRITE);
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

	/* uint32 PaireableTimeout [readwrite] */
	pspec = g_param_spec_uint("PaireableTimeout", NULL, NULL, 0, 65535, 0, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_PAIREABLE_TIMEOUT, pspec);

	/* boolean Powered [readwrite] */
	pspec = g_param_spec_boolean("Powered", NULL, NULL, FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_POWERED, pspec);

	/* array{string} UUIDs [readonly] */
	pspec = g_param_spec_boxed("UUIDs", NULL, NULL, G_TYPE_PTR_ARRAY, G_PARAM_READABLE);
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
			g_cclosure_bluez_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);

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
			g_cclosure_bluez_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);
}

static void adapter_init(Adapter *self)
{
	self->priv = ADAPTER_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void adapter_post_init(Adapter *self)
{
	g_assert(self->priv->dbus_g_proxy != NULL);

	/* DBUS signals connection */

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
}

static void _adapter_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Adapter *self = ADAPTER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, g_strdup(adapter_get_dbus_object_path(self)));
		break;

	case PROP_ADDRESS:
	{
		GError *error = NULL;
		g_value_set_string(value, adapter_get_address(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_CLASS:
	{
		GError *error = NULL;
		g_value_set_uint(value, adapter_get_class(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_DEVICES:
	{
		GError *error = NULL;
		g_value_set_boxed(value, adapter_get_devices(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_DISCOVERABLE:
	{
		GError *error = NULL;
		g_value_set_boolean(value, adapter_get_discoverable(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_DISCOVERABLE_TIMEOUT:
	{
		GError *error = NULL;
		g_value_set_uint(value, adapter_get_discoverable_timeout(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_DISCOVERING:
	{
		GError *error = NULL;
		g_value_set_boolean(value, adapter_get_discovering(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_NAME:
	{
		GError *error = NULL;
		g_value_set_string(value, adapter_get_name(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_PAIRABLE:
	{
		GError *error = NULL;
		g_value_set_boolean(value, adapter_get_pairable(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_PAIREABLE_TIMEOUT:
	{
		GError *error = NULL;
		g_value_set_uint(value, adapter_get_paireable_timeout(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_POWERED:
	{
		GError *error = NULL;
		g_value_set_boolean(value, adapter_get_powered(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_UUIDS:
	{
		GError *error = NULL;
		g_value_set_boxed(value, adapter_get_uuids(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _adapter_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
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

	case PROP_DISCOVERABLE:
	{
		GError *error = NULL;
		adapter_set_property(self, "Discoverable", value, &error);
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_DISCOVERABLE_TIMEOUT:
	{
		GError *error = NULL;
		adapter_set_property(self, "DiscoverableTimeout", value, &error);
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_NAME:
	{
		GError *error = NULL;
		adapter_set_property(self, "Name", value, &error);
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_PAIRABLE:
	{
		GError *error = NULL;
		adapter_set_property(self, "Pairable", value, &error);
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_PAIREABLE_TIMEOUT:
	{
		GError *error = NULL;
		adapter_set_property(self, "PaireableTimeout", value, &error);
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_POWERED:
	{
		GError *error = NULL;
		adapter_set_property(self, "Powered", value, &error);
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
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

	gchar *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "CreateDevice", error, G_TYPE_STRING, address, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* object CreatePairedDevice(string address, object agent, string capability) */
gchar *adapter_create_paired_device(Adapter *self, const gchar *address, const gchar *agent, const gchar *capability, GError **error)
{
	g_assert(ADAPTER_IS(self));

	gchar *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "CreatePairedDevice", error, G_TYPE_STRING, address, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_STRING, capability, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* object FindDevice(string address) */
gchar *adapter_find_device(Adapter *self, const gchar *address, GError **error)
{
	g_assert(ADAPTER_IS(self));

	gchar *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "FindDevice", error, G_TYPE_STRING, address, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* dict GetProperties() */
GHashTable *adapter_get_properties(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* void RegisterAgent(object agent, string capability) */
void adapter_register_agent(Adapter *self, const gchar *agent, const gchar *capability, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RegisterAgent", error, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_STRING, capability, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void ReleaseSession() */
void adapter_release_session(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "ReleaseSession", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void RemoveDevice(object device) */
void adapter_remove_device(Adapter *self, const gchar *device, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RemoveDevice", error, DBUS_TYPE_G_OBJECT_PATH, device, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void RequestSession() */
void adapter_request_session(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RequestSession", error, G_TYPE_INVALID, G_TYPE_INVALID);
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

gchar *adapter_get_address(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, NULL);
	gchar *ret = g_value_dup_string(g_hash_table_lookup(properties, "Address"));
	g_hash_table_unref(properties);

	return ret;
}

guint32 adapter_get_class(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, 0);
	guint32 ret = g_value_get_uint(g_hash_table_lookup(properties, "Class"));
	g_hash_table_unref(properties);

	return ret;
}

GPtrArray *adapter_get_devices(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, NULL);
	GPtrArray *ret = g_value_dup_boxed(g_hash_table_lookup(properties, "Devices"));
	g_hash_table_unref(properties);

	return ret;
}

gboolean adapter_get_discoverable(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, 0);
	gboolean ret = g_value_get_boolean(g_hash_table_lookup(properties, "Discoverable"));
	g_hash_table_unref(properties);

	return ret;
}

void adapter_set_discoverable(Adapter *self, const gboolean value, GError **error)
{
	g_return_if_fail(ADAPTER_IS(self));

	GValue t = {0};
	g_value_init(&t, G_TYPE_BOOLEAN);
	g_value_set_boolean(&t, value);
	adapter_set_property(self, "Discoverable", &t, error);
	g_value_unset(&t);
}

guint32 adapter_get_discoverable_timeout(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, 0);
	guint32 ret = g_value_get_uint(g_hash_table_lookup(properties, "DiscoverableTimeout"));
	g_hash_table_unref(properties);

	return ret;
}

void adapter_set_discoverable_timeout(Adapter *self, const guint32 value, GError **error)
{
	g_return_if_fail(ADAPTER_IS(self));

	GValue t = {0};
	g_value_init(&t, G_TYPE_UINT);
	g_value_set_uint(&t, value);
	adapter_set_property(self, "DiscoverableTimeout", &t, error);
	g_value_unset(&t);
}

gboolean adapter_get_discovering(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, 0);
	gboolean ret = g_value_get_boolean(g_hash_table_lookup(properties, "Discovering"));
	g_hash_table_unref(properties);

	return ret;
}

gchar *adapter_get_name(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, NULL);
	gchar *ret = g_value_dup_string(g_hash_table_lookup(properties, "Name"));
	g_hash_table_unref(properties);

	return ret;
}

void adapter_set_name(Adapter *self, const gchar *value, GError **error)
{
	g_return_if_fail(ADAPTER_IS(self));

	GValue t = {0};
	g_value_init(&t, G_TYPE_STRING);
	g_value_set_string(&t, value);
	adapter_set_property(self, "Name", &t, error);
	g_value_unset(&t);
}

gboolean adapter_get_pairable(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, 0);
	gboolean ret = g_value_get_boolean(g_hash_table_lookup(properties, "Pairable"));
	g_hash_table_unref(properties);

	return ret;
}

void adapter_set_pairable(Adapter *self, const gboolean value, GError **error)
{
	g_return_if_fail(ADAPTER_IS(self));

	GValue t = {0};
	g_value_init(&t, G_TYPE_BOOLEAN);
	g_value_set_boolean(&t, value);
	adapter_set_property(self, "Pairable", &t, error);
	g_value_unset(&t);
}

guint32 adapter_get_paireable_timeout(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, 0);
	guint32 ret = g_value_get_uint(g_hash_table_lookup(properties, "PaireableTimeout"));
	g_hash_table_unref(properties);

	return ret;
}

void adapter_set_paireable_timeout(Adapter *self, const guint32 value, GError **error)
{
	g_return_if_fail(ADAPTER_IS(self));

	GValue t = {0};
	g_value_init(&t, G_TYPE_UINT);
	g_value_set_uint(&t, value);
	adapter_set_property(self, "PaireableTimeout", &t, error);
	g_value_unset(&t);
}

gboolean adapter_get_powered(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, 0);
	gboolean ret = g_value_get_boolean(g_hash_table_lookup(properties, "Powered"));
	g_hash_table_unref(properties);

	return ret;
}

void adapter_set_powered(Adapter *self, const gboolean value, GError **error)
{
	g_return_if_fail(ADAPTER_IS(self));

	GValue t = {0};
	g_value_init(&t, G_TYPE_BOOLEAN);
	g_value_set_boolean(&t, value);
	adapter_set_property(self, "Powered", &t, error);
	g_value_unset(&t);
}

GPtrArray *adapter_get_uuids(Adapter *self, GError **error)
{
	g_assert(ADAPTER_IS(self));

	GHashTable *properties = adapter_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, NULL);
	GPtrArray *ret = g_value_dup_boxed(g_hash_table_lookup(properties, "UUIDs"));
	g_hash_table_unref(properties);

	return ret;
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

static void device_found_handler(DBusGProxy *dbus_g_proxy, const gchar *address, const GHashTable *values, gpointer data)
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
	g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}

