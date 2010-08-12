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

#include "obexsession.h"

#define OBEXSESSION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXSESSION_TYPE, OBEXSessionPrivate))

struct _OBEXSessionPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;

	/* Properties */
	gchar *address;
};

G_DEFINE_TYPE(OBEXSession, obexsession, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_ADDRESS /* readonly */
};

static void _obexsession_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obexsession_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void obexsession_dispose(GObject *gobject)
{
	OBEXSession *self = OBEXSESSION(gobject);

	/* Properties free */
	g_free(self->priv->address);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexsession_parent_class)->dispose(gobject);
}

static void obexsession_class_init(OBEXSessionClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexsession_dispose;

	g_type_class_add_private(klass, sizeof(OBEXSessionPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _obexsession_get_property;
	gobject_class->set_property = _obexsession_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* string Address [readonly] */
	pspec = g_param_spec_string("Address", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_ADDRESS, pspec);
}

static void obexsession_init(OBEXSession *self)
{
	self->priv = OBEXSESSION_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(session_conn != NULL);
}

static void obexsession_post_init(OBEXSession *self, const gchar *dbus_object_path)
{
	g_assert(dbus_object_path != NULL);
	g_assert(strlen(dbus_object_path) > 0);
	g_assert(self->priv->dbus_g_proxy == NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex", dbus_object_path, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", OBEXSESSION_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", OBEXSESSION_DBUS_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex", dbus_object_path, OBEXSESSION_DBUS_INTERFACE);

	/* Properties init */
	GHashTable *properties = obexsession_get_properties(self, &error);
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

	g_hash_table_unref(properties);
}

static void _obexsession_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXSession *self = OBEXSESSION(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, obexsession_get_dbus_object_path(self));
		break;

	case PROP_ADDRESS:
		g_value_set_string(value, obexsession_get_address(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _obexsession_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	OBEXSession *self = OBEXSESSION(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		obexsession_post_init(self, g_value_get_string(value));
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
GHashTable *obexsession_get_properties(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* Properties access methods */
const gchar *obexsession_get_dbus_object_path(OBEXSession *self)
{
	g_assert(OBEXSESSION_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

const gchar *obexsession_get_address(OBEXSession *self)
{
	g_assert(OBEXSESSION_IS(self));

	return self->priv->address;
}

