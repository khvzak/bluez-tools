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

static void network_class_init(NetworkClass *klass)
{
	g_type_class_add_private(klass, sizeof(NetworkPrivate));

	/* Properties registration */
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
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

	/* DBUS signals connection */

	/* PropertyChanged(string name, variant value) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self, NULL);
}

static void _network_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Network *self = NETWORK(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, g_strdup(network_get_dbus_object_path(self)));
		break;

	case PROP_CONNECTED:
	{
		GError *error = NULL;
		g_value_set_boolean(value, network_get_connected(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_INTERFACE:
	{
		GError *error = NULL;
		g_value_set_string(value, network_get_interface(self, &error));
		if (error != NULL) {
			g_print("%s: %s\n", g_get_prgname(), error->message);
			g_error_free(error);
		}
	}
		break;

	case PROP_UUID:
	{
		GError *error = NULL;
		g_value_set_string(value, network_get_uuid(self, &error));
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

	gchar *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "Connect", error, G_TYPE_STRING, uuid, G_TYPE_INVALID, G_TYPE_STRING, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

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

	GHashTable *ret;
	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* Properties access methods */
const gchar *network_get_dbus_object_path(Network *self)
{
	g_assert(NETWORK_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

gboolean network_get_connected(Network *self, GError **error)
{
	g_assert(NETWORK_IS(self));

	GHashTable *properties = network_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, 0);
	gboolean ret = g_value_get_boolean(g_hash_table_lookup(properties, "Connected"));
	g_hash_table_unref(properties);

	return ret;
}

gchar *network_get_interface(Network *self, GError **error)
{
	g_assert(NETWORK_IS(self));

	GHashTable *properties = network_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, NULL);
	gchar *ret = g_value_dup_string(g_hash_table_lookup(properties, "Interface"));
	g_hash_table_unref(properties);

	return ret;
}

gchar *network_get_uuid(Network *self, GError **error)
{
	g_assert(NETWORK_IS(self));

	GHashTable *properties = network_get_properties(self, error);
	g_return_val_if_fail(properties != NULL, NULL);
	gchar *ret = g_value_dup_string(g_hash_table_lookup(properties, "UUID"));
	g_hash_table_unref(properties);

	return ret;
}

/* Signals handlers */
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data)
{
	Network *self = NETWORK(data);
	g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}

