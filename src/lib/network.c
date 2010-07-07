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
#include "network.h"

#define BLUEZ_DBUS_NETWORK_INTERFACE "org.bluez.Network"

#define NETWORK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), NETWORK_TYPE, NetworkPrivate))

struct _NetworkPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Properties */
	gboolean connected;
	gchar *interface;
	gchar *uuid;
};

G_DEFINE_TYPE(Network, network, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_CONNECTED, /* readonly */
	PROP_INTERFACE, /* readonly */
	PROP_UUID /* readonly */
};

enum {
	PROPERTY_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void _network_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _network_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data);

static void network_dispose(GObject *gobject)
{
	Network *self = NETWORK(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self);

	/* Properties free */
	g_free(self->priv->interface);
	g_free(self->priv->uuid);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(network_parent_class)->dispose(gobject);
}

static void network_class_init(NetworkClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = network_dispose;

	g_type_class_add_private(klass, sizeof(NetworkPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _network_get_property;
	gobject_class->set_property = _network_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* boolean Connected [readonly] */
	pspec = g_param_spec_boolean("Connected", NULL, NULL, FALSE, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_CONNECTED, pspec);

	/* string Interface [readonly] */
	pspec = g_param_spec_string("Interface", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_INTERFACE, pspec);

	/* string UUID [readonly] */
	pspec = g_param_spec_string("UUID", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_UUID, pspec);

	/* Signals registation */
	signals[PROPERTY_CHANGED] = g_signal_new("PropertyChanged",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bluez_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);
}

static void network_init(Network *self)
{
	self->priv = NETWORK_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void network_post_init(Network *self)
{
	g_assert(self->priv->dbus_g_proxy != NULL);

	/* DBus signals connection */

	/* PropertyChanged(string name, variant value) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self, NULL);

	/* Properties init */
	GError *error = NULL;
	GHashTable *properties = network_get_properties(self, &error);
	g_assert(error == NULL);
	g_assert(properties != NULL);

	/* boolean Connected [readonly] */
	if (g_hash_table_lookup(properties, "Connected")) {
		self->priv->connected = g_value_get_boolean(g_hash_table_lookup(properties, "Connected"));
	} else {
		self->priv->connected = FALSE;
	}

	/* string Interface [readonly] */
	if (g_hash_table_lookup(properties, "Interface")) {
		self->priv->interface = g_value_dup_string(g_hash_table_lookup(properties, "Interface"));
	} else {
		self->priv->interface = g_strdup("undefined");
	}

	/* string UUID [readonly] */
	if (g_hash_table_lookup(properties, "UUID")) {
		self->priv->uuid = g_value_dup_string(g_hash_table_lookup(properties, "UUID"));
	} else {
		self->priv->uuid = g_strdup("undefined");
	}

	g_hash_table_unref(properties);
}

static void _network_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Network *self = NETWORK(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, network_get_dbus_object_path(self));
		break;

	case PROP_CONNECTED:
		g_value_set_boolean(value, network_get_connected(self));
		break;

	case PROP_INTERFACE:
		g_value_set_string(value, network_get_interface(self));
		break;

	case PROP_UUID:
		g_value_set_string(value, network_get_uuid(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _network_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Network *self = NETWORK(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
	{
		const gchar *dbus_object_path = g_value_get_string(value);
		g_assert(dbus_object_path != NULL);
		g_assert(self->priv->dbus_g_proxy == NULL);
		self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, dbus_object_path, BLUEZ_DBUS_NETWORK_INTERFACE);
		network_post_init(self);
	}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/* Methods */

/* string Connect(string uuid) */
gchar *network_connect(Network *self, const gchar *uuid, GError **error)
{
	g_assert(NETWORK_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Connect", error, G_TYPE_STRING, uuid, G_TYPE_INVALID, G_TYPE_STRING, &ret, G_TYPE_INVALID);

	return ret;
}

/* void Disconnect() */
void network_disconnect(Network *self, GError **error)
{
	g_assert(NETWORK_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Disconnect", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* dict GetProperties() */
GHashTable *network_get_properties(Network *self, GError **error)
{
	g_assert(NETWORK_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* Properties access methods */
const gchar *network_get_dbus_object_path(Network *self)
{
	g_assert(NETWORK_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

const gboolean network_get_connected(Network *self)
{
	g_assert(NETWORK_IS(self));

	return self->priv->connected;
}

const gchar *network_get_interface(Network *self)
{
	g_assert(NETWORK_IS(self));

	return self->priv->interface;
}

const gchar *network_get_uuid(Network *self)
{
	g_assert(NETWORK_IS(self));

	return self->priv->uuid;
}

/* Signals handlers */
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data)
{
	Network *self = NETWORK(data);

	if (g_strcmp0(name, "Connected") == 0) {
		self->priv->connected = g_value_get_boolean(value);
	} else if (g_strcmp0(name, "Interface") == 0) {
		g_free(self->priv->interface);
		self->priv->interface = g_value_dup_string(value);
	} else if (g_strcmp0(name, "UUID") == 0) {
		g_free(self->priv->uuid);
		self->priv->uuid = g_value_dup_string(value);
	}

	g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}

