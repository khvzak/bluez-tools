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
#include "peer.h"

#define BLUEZ_DBUS_PEER_INTERFACE "org.bluez.network.Peer"

#define PEER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PEER_TYPE, PeerPrivate))

struct _PeerPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Properties */
	gboolean enable;
	gchar *name;
	gchar *uuid;
};

G_DEFINE_TYPE(Peer, peer, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_ENABLE, /* readwrite */
	PROP_NAME, /* readwrite */
	PROP_UUID /* readonly */
};

static void _peer_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _peer_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void peer_dispose(GObject *gobject)
{
	Peer *self = PEER(gobject);

	/* Properties free */
	g_free(self->priv->name);
	g_free(self->priv->uuid);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(peer_parent_class)->dispose(gobject);
}

static void peer_class_init(PeerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = peer_dispose;

	g_type_class_add_private(klass, sizeof(PeerPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _peer_get_property;
	gobject_class->set_property = _peer_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* boolean Enable [readwrite] */
	pspec = g_param_spec_boolean("Enable", NULL, NULL, FALSE, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_ENABLE, pspec);

	/* string Name [readwrite] */
	pspec = g_param_spec_string("Name", NULL, NULL, NULL, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	/* string UUID [readonly] */
	pspec = g_param_spec_string("UUID", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_UUID, pspec);
}

static void peer_init(Peer *self)
{
	self->priv = PEER_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void peer_post_init(Peer *self)
{
	g_assert(self->priv->dbus_g_proxy != NULL);

	/* Properties init */
	GError *error = NULL;
	GHashTable *properties = peer_get_properties(self, &error);
	g_assert(error == NULL);
	g_assert(properties != NULL);

	/* boolean Enable [readwrite] */
	if (g_hash_table_lookup(properties, "Enable")) {
		self->priv->enable = g_value_get_boolean(g_hash_table_lookup(properties, "Enable"));
	} else {
		self->priv->enable = FALSE;
	}

	/* string Name [readwrite] */
	if (g_hash_table_lookup(properties, "Name")) {
		self->priv->name = g_value_dup_string(g_hash_table_lookup(properties, "Name"));
	} else {
		self->priv->name = g_strdup("undefined");
	}

	/* string UUID [readonly] */
	if (g_hash_table_lookup(properties, "UUID")) {
		self->priv->uuid = g_value_dup_string(g_hash_table_lookup(properties, "UUID"));
	} else {
		self->priv->uuid = g_strdup("undefined");
	}

	g_hash_table_unref(properties);
}

static void _peer_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Peer *self = PEER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, peer_get_dbus_object_path(self));
		break;

	case PROP_ENABLE:
		g_value_set_boolean(value, peer_get_enable(self));
		break;

	case PROP_NAME:
		g_value_set_string(value, peer_get_name(self));
		break;

	case PROP_UUID:
		g_value_set_string(value, peer_get_uuid(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _peer_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Peer *self = PEER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
	{
		const gchar *dbus_object_path = g_value_get_string(value);
		g_assert(dbus_object_path != NULL);
		g_assert(self->priv->dbus_g_proxy == NULL);
		self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, dbus_object_path, BLUEZ_DBUS_PEER_INTERFACE);
		peer_post_init(self);
	}
		break;

	case PROP_ENABLE:
	{
		GError *error = NULL;
		peer_set_property(self, "Enable", value, &error);
		g_assert(error == NULL);
	}
		break;

	case PROP_NAME:
	{
		GError *error = NULL;
		peer_set_property(self, "Name", value, &error);
		g_assert(error == NULL);
	}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/* Methods */

/* dict GetProperties() */
GHashTable *peer_get_properties(Peer *self, GError **error)
{
	g_assert(PEER_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* void SetProperty(string name, variant value) */
void peer_set_property(Peer *self, const gchar *name, const GValue *value, GError **error)
{
	g_assert(PEER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "SetProperty", error, G_TYPE_STRING, name, G_TYPE_VALUE, value, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *peer_get_dbus_object_path(Peer *self)
{
	g_assert(PEER_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

const gboolean peer_get_enable(Peer *self)
{
	g_assert(PEER_IS(self));

	return self->priv->enable;
}

void peer_set_enable(Peer *self, const gboolean value)
{
	g_assert(PEER_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_BOOLEAN);
	g_value_set_boolean(&t, value);
	peer_set_property(self, "Enable", &t, &error);
	g_value_unset(&t);

	g_assert(error == NULL);
}

const gchar *peer_get_name(Peer *self)
{
	g_assert(PEER_IS(self));

	return self->priv->name;
}

void peer_set_name(Peer *self, const gchar *value)
{
	g_assert(PEER_IS(self));

	GError *error = NULL;

	GValue t = {0};
	g_value_init(&t, G_TYPE_STRING);
	g_value_set_string(&t, value);
	peer_set_property(self, "Name", &t, &error);
	g_value_unset(&t);

	g_assert(error == NULL);
}

const gchar *peer_get_uuid(Peer *self)
{
	g_assert(PEER_IS(self));

	return self->priv->uuid;
}

