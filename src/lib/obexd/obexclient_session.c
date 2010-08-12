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

#include "obexclient_session.h"

#define OBEXCLIENT_SESSION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXCLIENT_SESSION_TYPE, OBEXClientSessionPrivate))

struct _OBEXClientSessionPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;

	/* Properties */
	guchar channel;
	gchar *destination;
	gchar *source;
};

G_DEFINE_TYPE(OBEXClientSession, obexclient_session, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_CHANNEL, /* readonly */
	PROP_DESTINATION, /* readonly */
	PROP_SOURCE /* readonly */
};

static void _obexclient_session_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obexclient_session_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void obexclient_session_dispose(GObject *gobject)
{
	OBEXClientSession *self = OBEXCLIENT_SESSION(gobject);

	/* Properties free */
	g_free(self->priv->destination);
	g_free(self->priv->source);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexclient_session_parent_class)->dispose(gobject);
}

static void obexclient_session_class_init(OBEXClientSessionClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexclient_session_dispose;

	g_type_class_add_private(klass, sizeof(OBEXClientSessionPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _obexclient_session_get_property;
	gobject_class->set_property = _obexclient_session_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* byte Channel [readonly] */
	pspec = g_param_spec_uchar("Channel", NULL, NULL, 0, 0xFF, 0, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_CHANNEL, pspec);

	/* string Destination [readonly] */
	pspec = g_param_spec_string("Destination", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_DESTINATION, pspec);

	/* string Source [readonly] */
	pspec = g_param_spec_string("Source", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_SOURCE, pspec);
}

static void obexclient_session_init(OBEXClientSession *self)
{
	self->priv = OBEXCLIENT_SESSION_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(session_conn != NULL);
}

static void obexclient_session_post_init(OBEXClientSession *self, const gchar *dbus_object_path)
{
	g_assert(dbus_object_path != NULL);
	g_assert(strlen(dbus_object_path) > 0);
	g_assert(self->priv->dbus_g_proxy == NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex.client", dbus_object_path, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", OBEXCLIENT_SESSION_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", OBEXCLIENT_SESSION_DBUS_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex.client", dbus_object_path, OBEXCLIENT_SESSION_DBUS_INTERFACE);

	/* Properties init */
	GHashTable *properties = obexclient_session_get_properties(self, &error);
	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
	g_assert(properties != NULL);

	/* byte Channel [readonly] */
	if (g_hash_table_lookup(properties, "Channel")) {
		self->priv->channel = g_value_get_uchar(g_hash_table_lookup(properties, "Channel"));
	} else {
		self->priv->channel = 0;
	}

	/* string Destination [readonly] */
	if (g_hash_table_lookup(properties, "Destination")) {
		self->priv->destination = g_value_dup_string(g_hash_table_lookup(properties, "Destination"));
	} else {
		self->priv->destination = g_strdup("undefined");
	}

	/* string Source [readonly] */
	if (g_hash_table_lookup(properties, "Source")) {
		self->priv->source = g_value_dup_string(g_hash_table_lookup(properties, "Source"));
	} else {
		self->priv->source = g_strdup("undefined");
	}

	g_hash_table_unref(properties);
}

static void _obexclient_session_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXClientSession *self = OBEXCLIENT_SESSION(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, obexclient_session_get_dbus_object_path(self));
		break;

	case PROP_CHANNEL:
		g_value_set_uchar(value, obexclient_session_get_channel(self));
		break;

	case PROP_DESTINATION:
		g_value_set_string(value, obexclient_session_get_destination(self));
		break;

	case PROP_SOURCE:
		g_value_set_string(value, obexclient_session_get_source(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _obexclient_session_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	OBEXClientSession *self = OBEXCLIENT_SESSION(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		obexclient_session_post_init(self, g_value_get_string(value));
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

/* void AssignAgent(object agent) */
void obexclient_session_assign_agent(OBEXClientSession *self, const gchar *agent, GError **error)
{
	g_assert(OBEXCLIENT_SESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "AssignAgent", error, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* dict GetProperties() */
GHashTable *obexclient_session_get_properties(OBEXClientSession *self, GError **error)
{
	g_assert(OBEXCLIENT_SESSION_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* void ReleaseAgent(object agent) */
void obexclient_session_release_agent(OBEXClientSession *self, const gchar *agent, GError **error)
{
	g_assert(OBEXCLIENT_SESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "ReleaseAgent", error, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *obexclient_session_get_dbus_object_path(OBEXClientSession *self)
{
	g_assert(OBEXCLIENT_SESSION_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

const guchar obexclient_session_get_channel(OBEXClientSession *self)
{
	g_assert(OBEXCLIENT_SESSION_IS(self));

	return self->priv->channel;
}

const gchar *obexclient_session_get_destination(OBEXClientSession *self)
{
	g_assert(OBEXCLIENT_SESSION_IS(self));

	return self->priv->destination;
}

const gchar *obexclient_session_get_source(OBEXClientSession *self)
{
	g_assert(OBEXCLIENT_SESSION_IS(self));

	return self->priv->source;
}

