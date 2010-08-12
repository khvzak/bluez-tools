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

#include "obexclient.h"

#define OBEXCLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXCLIENT_TYPE, OBEXClientPrivate))

struct _OBEXClientPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;
};

G_DEFINE_TYPE(OBEXClient, obexclient, G_TYPE_OBJECT);

static void obexclient_dispose(GObject *gobject)
{
	OBEXClient *self = OBEXCLIENT(gobject);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexclient_parent_class)->dispose(gobject);
}

static void obexclient_class_init(OBEXClientClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexclient_dispose;

	g_type_class_add_private(klass, sizeof(OBEXClientPrivate));
}

static void obexclient_init(OBEXClient *self)
{
	self->priv = OBEXCLIENT_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(session_conn != NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex.client", OBEXCLIENT_DBUS_PATH, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", OBEXCLIENT_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", OBEXCLIENT_DBUS_INTERFACE, OBEXCLIENT_DBUS_PATH);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);

	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex.client", OBEXCLIENT_DBUS_PATH, OBEXCLIENT_DBUS_INTERFACE);
}

/* Methods */

/* object CreateSession(dict device) */
gchar *obexclient_create_session(OBEXClient *self, const GHashTable *device, GError **error)
{
	g_assert(OBEXCLIENT_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CreateSession", error, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, device, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID);

	return ret;
}

/* void ExchangeBusinessCards(dict device, string clientfile, string file) */
void obexclient_exchange_business_cards(OBEXClient *self, const GHashTable *device, const gchar *clientfile, const gchar *file, GError **error)
{
	g_assert(OBEXCLIENT_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "ExchangeBusinessCards", error, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, device, G_TYPE_STRING, clientfile, G_TYPE_STRING, file, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* string GetCapabilities(dict device) */
gchar *obexclient_get_capabilities(OBEXClient *self, const GHashTable *device, GError **error)
{
	g_assert(OBEXCLIENT_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetCapabilities", error, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, device, G_TYPE_INVALID, G_TYPE_STRING, &ret, G_TYPE_INVALID);

	return ret;
}

/* void PullBusinessCard(dict device, string file) */
void obexclient_pull_business_card(OBEXClient *self, const GHashTable *device, const gchar *file, GError **error)
{
	g_assert(OBEXCLIENT_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "PullBusinessCard", error, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, device, G_TYPE_STRING, file, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void RemoveSession(object session) */
void obexclient_remove_session(OBEXClient *self, const gchar *session, GError **error)
{
	g_assert(OBEXCLIENT_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RemoveSession", error, DBUS_TYPE_G_OBJECT_PATH, session, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void SendFiles(dict device, array{string} files, object agent) */
void obexclient_send_files(OBEXClient *self, const GHashTable *device, const gchar **files, const gchar *agent, GError **error)
{
	g_assert(OBEXCLIENT_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "SendFiles", error, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, device, G_TYPE_STRV, files, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_INVALID, G_TYPE_INVALID);
}

