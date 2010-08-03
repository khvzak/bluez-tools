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
#include "obexmanager.h"

#define OBEXMANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXMANAGER_TYPE, OBEXManagerPrivate))

struct _OBEXManagerPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;
};

G_DEFINE_TYPE(OBEXManager, obexmanager, G_TYPE_OBJECT);

enum {
	SESSION_CLOSED,
	SESSION_CONNECT_ERROR,
	SESSION_CONNECTED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void session_closed_handler(DBusGProxy *dbus_g_proxy, const gchar *path, gpointer data);
static void session_connect_error_handler(DBusGProxy *dbus_g_proxy, const gchar *path, const gchar *error_name, const gchar *error_message, gpointer data);
static void session_connected_handler(DBusGProxy *dbus_g_proxy, const gchar *path, gpointer data);

static void obexmanager_dispose(GObject *gobject)
{
	OBEXManager *self = OBEXMANAGER(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "SessionClosed", G_CALLBACK(session_closed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "SessionConnectError", G_CALLBACK(session_connect_error_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "SessionConnected", G_CALLBACK(session_connected_handler), self);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexmanager_parent_class)->dispose(gobject);
}

static void obexmanager_class_init(OBEXManagerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexmanager_dispose;

	g_type_class_add_private(klass, sizeof(OBEXManagerPrivate));

	/* Signals registation */
	signals[SESSION_CLOSED] = g_signal_new("SessionClosed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[SESSION_CONNECT_ERROR] = g_signal_new("SessionConnectError",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bluez_marshal_VOID__STRING_STRING_STRING,
			G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	signals[SESSION_CONNECTED] = g_signal_new("SessionConnected",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void obexmanager_init(OBEXManager *self)
{
	self->priv = OBEXMANAGER_GET_PRIVATE(self);

	g_assert(conn != NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(conn, "org.openobex", BLUEZ_DBUS_OBEXMANAGER_PATH, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", BLUEZ_DBUS_OBEXMANAGER_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", BLUEZ_DBUS_OBEXMANAGER_INTERFACE, BLUEZ_DBUS_OBEXMANAGER_PATH);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);

	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, "org.openobex", BLUEZ_DBUS_OBEXMANAGER_PATH, BLUEZ_DBUS_OBEXMANAGER_INTERFACE);

	/* DBus signals connection */

	/* SessionClosed(object path) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "SessionClosed", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "SessionClosed", G_CALLBACK(session_closed_handler), self, NULL);

	/* SessionConnectError(object path, string error_name, string error_message) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "SessionConnectError", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "SessionConnectError", G_CALLBACK(session_connect_error_handler), self, NULL);

	/* SessionConnected(object path) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "SessionConnected", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "SessionConnected", G_CALLBACK(session_connected_handler), self, NULL);
}

/* Methods */

/* boolean CancelSessionConnect(object session_object) */
gboolean obexmanager_cancel_session_connect(OBEXManager *self, const gchar *session_object, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	gboolean ret = FALSE;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CancelSessionConnect", error, DBUS_TYPE_G_OBJECT_PATH, session_object, G_TYPE_INVALID, G_TYPE_BOOLEAN, &ret, G_TYPE_INVALID);

	return ret;
}

/* object CreateBluetoothServer(string source_address, string pattern, boolean require_pairing) */
gchar *obexmanager_create_bluetooth_server(OBEXManager *self, const gchar *source_address, const gchar *pattern, const gboolean require_pairing, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CreateBluetoothServer", error, G_TYPE_STRING, source_address, G_TYPE_STRING, pattern, G_TYPE_BOOLEAN, require_pairing, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID);

	return ret;
}

/* object CreateBluetoothSession(string target_address, string source_address, string pattern) */
gchar *obexmanager_create_bluetooth_session(OBEXManager *self, const gchar *target_address, const gchar *source_address, const gchar *pattern, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CreateBluetoothSession", error, G_TYPE_STRING, target_address, G_TYPE_STRING, source_address, G_TYPE_STRING, pattern, G_TYPE_INVALID, DBUS_TYPE_G_OBJECT_PATH, &ret, G_TYPE_INVALID);

	return ret;
}

/* dict GetServerInfo(object server_object) */
GHashTable *obexmanager_get_server_info(OBEXManager *self, const gchar *server_object, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetServerInfo", error, DBUS_TYPE_G_OBJECT_PATH, server_object, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* array{string} GetServerList() */
gchar **obexmanager_get_server_list(OBEXManager *self, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	gchar **ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetServerList", error, G_TYPE_INVALID, G_TYPE_STRV, &ret, G_TYPE_INVALID);

	return ret;
}

/* dict GetSessionInfo(object session_object) */
GHashTable *obexmanager_get_session_info(OBEXManager *self, const gchar *session_object, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetSessionInfo", error, DBUS_TYPE_G_OBJECT_PATH, session_object, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* array{string} GetSessionList() */
gchar **obexmanager_get_session_list(OBEXManager *self, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	gchar **ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetSessionList", error, G_TYPE_INVALID, G_TYPE_STRV, &ret, G_TYPE_INVALID);

	return ret;
}

/* string GetVersion() */
gchar *obexmanager_get_version(OBEXManager *self, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetVersion", error, G_TYPE_INVALID, G_TYPE_STRING, &ret, G_TYPE_INVALID);

	return ret;
}

/* Signals handlers */
static void session_closed_handler(DBusGProxy *dbus_g_proxy, const gchar *path, gpointer data)
{
	OBEXManager *self = OBEXMANAGER(data);

	g_signal_emit(self, signals[SESSION_CLOSED], 0, path);
}

static void session_connect_error_handler(DBusGProxy *dbus_g_proxy, const gchar *path, const gchar *error_name, const gchar *error_message, gpointer data)
{
	OBEXManager *self = OBEXMANAGER(data);

	g_signal_emit(self, signals[SESSION_CONNECT_ERROR], 0, path, error_name, error_message);
}

static void session_connected_handler(DBusGProxy *dbus_g_proxy, const gchar *path, gpointer data)
{
	OBEXManager *self = OBEXMANAGER(data);

	g_signal_emit(self, signals[SESSION_CONNECTED], 0, path);
}

