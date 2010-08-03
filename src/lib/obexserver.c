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
#include "obexserver.h"

#define OBEXSERVER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXSERVER_TYPE, OBEXServerPrivate))

struct _OBEXServerPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;
};

G_DEFINE_TYPE(OBEXServer, obexserver, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH /* readwrite, construct only */
};

static void _obexserver_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obexserver_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	CLOSED,
	ERROR_OCCURRED,
	SESSION_CREATED,
	SESSION_REMOVED,
	STARTED,
	STOPPED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void closed_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void error_occurred_handler(DBusGProxy *dbus_g_proxy, const gchar *error_name, const gchar *error_message, gpointer data);
static void session_created_handler(DBusGProxy *dbus_g_proxy, const gchar *session_object, gpointer data);
static void session_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *session_object, gpointer data);
static void started_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void stopped_handler(DBusGProxy *dbus_g_proxy, gpointer data);

static void obexserver_dispose(GObject *gobject)
{
	OBEXServer *self = OBEXSERVER(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "Closed", G_CALLBACK(closed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "ErrorOccurred", G_CALLBACK(error_occurred_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "SessionCreated", G_CALLBACK(session_created_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "SessionRemoved", G_CALLBACK(session_removed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "Started", G_CALLBACK(started_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "Stopped", G_CALLBACK(stopped_handler), self);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexserver_parent_class)->dispose(gobject);
}

static void obexserver_class_init(OBEXServerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexserver_dispose;

	g_type_class_add_private(klass, sizeof(OBEXServerPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _obexserver_get_property;
	gobject_class->set_property = _obexserver_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* Signals registation */
	signals[CLOSED] = g_signal_new("Closed",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[ERROR_OCCURRED] = g_signal_new("ErrorOccurred",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bluez_marshal_VOID__STRING_STRING,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

	signals[SESSION_CREATED] = g_signal_new("SessionCreated",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[SESSION_REMOVED] = g_signal_new("SessionRemoved",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[STARTED] = g_signal_new("Started",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[STOPPED] = g_signal_new("Stopped",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
}

static void obexserver_init(OBEXServer *self)
{
	self->priv = OBEXSERVER_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void obexserver_post_init(OBEXServer *self, const gchar *dbus_object_path)
{
	g_assert(dbus_object_path != NULL);
	g_assert(strlen(dbus_object_path) > 0);
	g_assert(self->priv->dbus_g_proxy == NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(conn, "org.openobex", dbus_object_path, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", BLUEZ_DBUS_OBEXSERVER_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", BLUEZ_DBUS_OBEXSERVER_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, "org.openobex", dbus_object_path, BLUEZ_DBUS_OBEXSERVER_INTERFACE);

	/* DBus signals connection */

	/* Closed() */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "Closed", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "Closed", G_CALLBACK(closed_handler), self, NULL);

	/* ErrorOccurred(string error_name, string error_message) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "ErrorOccurred", G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "ErrorOccurred", G_CALLBACK(error_occurred_handler), self, NULL);

	/* SessionCreated(object session_object) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "SessionCreated", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "SessionCreated", G_CALLBACK(session_created_handler), self, NULL);

	/* SessionRemoved(object session_object) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "SessionRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "SessionRemoved", G_CALLBACK(session_removed_handler), self, NULL);

	/* Started() */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "Started", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "Started", G_CALLBACK(started_handler), self, NULL);

	/* Stopped() */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "Stopped", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "Stopped", G_CALLBACK(stopped_handler), self, NULL);
}

static void _obexserver_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXServer *self = OBEXSERVER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, obexserver_get_dbus_object_path(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _obexserver_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	OBEXServer *self = OBEXSERVER(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		obexserver_post_init(self, g_value_get_string(value));
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

/* void Close() */
void obexserver_close(OBEXServer *self, GError **error)
{
	g_assert(OBEXSERVER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Close", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* dict GetServerSessionInfo(object session_object) */
GHashTable *obexserver_get_server_session_info(OBEXServer *self, const gchar *session_object, GError **error)
{
	g_assert(OBEXSERVER_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetServerSessionInfo", error, DBUS_TYPE_G_OBJECT_PATH, session_object, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* array{string} GetServerSessionList() */
gchar **obexserver_get_server_session_list(OBEXServer *self, GError **error)
{
	g_assert(OBEXSERVER_IS(self));

	gchar **ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetServerSessionList", error, G_TYPE_INVALID, G_TYPE_STRV, &ret, G_TYPE_INVALID);

	return ret;
}

/* boolean IsStarted() */
gboolean obexserver_is_started(OBEXServer *self, GError **error)
{
	g_assert(OBEXSERVER_IS(self));

	gboolean ret = FALSE;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "IsStarted", error, G_TYPE_INVALID, G_TYPE_BOOLEAN, &ret, G_TYPE_INVALID);

	return ret;
}

/* void SetOption(string name, variant value) */
void obexserver_set_option(OBEXServer *self, const gchar *name, const GValue *value, GError **error)
{
	g_assert(OBEXSERVER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "SetOption", error, G_TYPE_STRING, name, G_TYPE_VALUE, value, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Start(string path, boolean allow_write, boolean auto_accept) */
void obexserver_start(OBEXServer *self, const gchar *path, const gboolean allow_write, const gboolean auto_accept, GError **error)
{
	g_assert(OBEXSERVER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Start", error, G_TYPE_STRING, path, G_TYPE_BOOLEAN, allow_write, G_TYPE_BOOLEAN, auto_accept, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Stop() */
void obexserver_stop(OBEXServer *self, GError **error)
{
	g_assert(OBEXSERVER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Stop", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *obexserver_get_dbus_object_path(OBEXServer *self)
{
	g_assert(OBEXSERVER_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

/* Signals handlers */
static void closed_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXServer *self = OBEXSERVER(data);

	g_signal_emit(self, signals[CLOSED], 0);
}

static void error_occurred_handler(DBusGProxy *dbus_g_proxy, const gchar *error_name, const gchar *error_message, gpointer data)
{
	OBEXServer *self = OBEXSERVER(data);

	g_signal_emit(self, signals[ERROR_OCCURRED], 0, error_name, error_message);
}

static void session_created_handler(DBusGProxy *dbus_g_proxy, const gchar *session_object, gpointer data)
{
	OBEXServer *self = OBEXSERVER(data);

	g_signal_emit(self, signals[SESSION_CREATED], 0, session_object);
}

static void session_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *session_object, gpointer data)
{
	OBEXServer *self = OBEXSERVER(data);

	g_signal_emit(self, signals[SESSION_REMOVED], 0, session_object);
}

static void started_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXServer *self = OBEXSERVER(data);

	g_signal_emit(self, signals[STARTED], 0);
}

static void stopped_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXServer *self = OBEXSERVER(data);

	g_signal_emit(self, signals[STOPPED], 0);
}

