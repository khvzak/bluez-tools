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

#include <glib.h>
#include <string.h>

#include "dbus-common.h"
#include "marshallers.h"
#include "network_hub.h"

#define NETWORK_HUB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), NETWORK_HUB_TYPE, NetworkHubPrivate))

struct _NetworkHubPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;

	/* Properties */
	gboolean enabled;
	gchar *name;
	gchar *uuid;
};

G_DEFINE_TYPE(NetworkHub, network_hub, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_ENABLED, /* readwrite */
	PROP_NAME, /* readwrite */
	PROP_UUID /* readonly */
};

static void _network_hub_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _network_hub_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void network_hub_dispose(GObject *gobject)
{
	NetworkHub *self = NETWORK_HUB(gobject);

	/* Properties free */
	g_free(self->priv->name);
	g_free(self->priv->uuid);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(network_hub_parent_class)->dispose(gobject);
}

static void network_hub_class_init(NetworkHubClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = network_hub_dispose;

	g_type_class_add_private(klass, sizeof(NetworkHubPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _network_hub_get_property;
	gobject_class->set_property = _network_hub_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* boolean Enabled [readwrite] */
	pspec = g_param_spec_boolean("Enabled", NULL, NULL, FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_ENABLED, pspec);

	/* string Name [readwrite] */
	pspec = g_param_spec_string("Name", NULL, NULL, NULL, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	/* string Uuid [readonly] */
	pspec = g_param_spec_string("Uuid", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_UUID, pspec);
}

static void network_hub_init(NetworkHub *self)
{
	self->priv = NETWORK_HUB_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void network_hub_post_init(NetworkHub *self, const gchar *dbus_object_path)
{
	g_assert(dbus_object_path != NULL);
	g_assert(strlen(dbus_object_path) > 0);
	g_assert(self->priv->dbus_g_proxy == NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, dbus_object_path, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", BLUEZ_DBUS_NETWORK_HUB_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", BLUEZ_DBUS_NETWORK_HUB_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, dbus_object_path, BLUEZ_DBUS_NETWORK_HUB_INTERFACE);

	/* Properties init */
	GHashTable *properties = network_hub_get_properties(self, &error);
	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
	g_assert(properties != NULL);

	/* boolean Enabled [readwrite] */
	if (g_hash_table_lookup(properties, "Enabled")) {
		self->priv->enabled = g_value_get_boolean(g_hash_table_lookup(properties, "Enabled"));
	} else {
		self->priv->enabled = FALSE;
	}

	/* string Name [readwrite] */
	if (g_hash_table_lookup(properties, "Name")) {
		self->priv->name = g_value_dup_string(g_hash_table_lookup(properties, "Name"));
	} else {
		self->priv->name = g_strdup("undefined");
	}

	/* string Uuid [readonly] */
	if (g_hash_table_lookup(properties, "Uuid")) {
		self->priv->uuid = g_value_dup_string(g_hash_table_lookup(properties, "Uuid"));
	} else {
		self->priv->uuid = g_strdup("undefined");
	}

	g_hash_table_unref(properties);
}

static void _network_hub_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	NetworkHub *self = NETWORK_HUB(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, network_hub_get_dbus_object_path(self));
		break;

	case PROP_ENABLED:
		g_value_set_boolean(value, network_hub_get_enabled(self));
		break;

	case PROP_NAME:
		g_value_set_string(value, network_hub_get_name(self));
		break;

	case PROP_UUID:
		g_value_set_string(value, network_hub_get_uuid(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _network_hub_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	NetworkHub *self = NETWORK_HUB(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		network_hub_post_init(self, g_value_get_string(value));
		break;

	case PROP_ENABLED:
		network_hub_set_property(self, "Enabled", value, &error);
		break;

	case PROP_NAME:
		network_hub_set_property(self, "Name", value, &error);
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

/* Methods */

/* dict GetProperties() */
GHashTable *network_hub_get_properties(NetworkHub *self, GError **error)
{
	g_assert(NETWORK_HUB_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* void SetProperty(string name, variant value) */
void network_hub_set_property(NetworkHub *self, const gchar *name, const GValue *value, GError **error)
{
	g_assert(NETWORK_HUB_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "SetProperty", error, G_TYPE_STRING, name, G_TYPE_VALUE, value, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *network_hub_get_dbus_object_path(NetworkHub *self)
{
	g_assert(NETWORK_HUB_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

const gboolean network_hub_get_enabled(NetworkHub *self)
{
	g_assert(NETWORK_HUB_IS(self));

	return self->priv->enabled;
}

void network_hub_set_enabled(NetworkHub *self, const gboolean value)
{
	g_assert(NETWORK_HUB_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_BOOLEAN);
	g_value_set_boolean(&t, value);
	network_hub_set_property(self, "Enabled", &t, &error);
	g_value_unset(&t);

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

const gchar *network_hub_get_name(NetworkHub *self)
{
	g_assert(NETWORK_HUB_IS(self));

	return self->priv->name;
}

void network_hub_set_name(NetworkHub *self, const gchar *value)
{
	g_assert(NETWORK_HUB_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_STRING);
	g_value_set_string(&t, value);
	network_hub_set_property(self, "Name", &t, &error);
	g_value_unset(&t);

	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
}

const gchar *network_hub_get_uuid(NetworkHub *self)
{
	g_assert(NETWORK_HUB_IS(self));

	return self->priv->uuid;
}

