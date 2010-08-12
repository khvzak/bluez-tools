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
	SESSION_CREATED,
	SESSION_REMOVED,
	TRANSFER_COMPLETED,
	TRANSFER_STARTED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void session_created_handler(DBusGProxy *dbus_g_proxy, const gchar *session, gpointer data);
static void session_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *session, gpointer data);
static void transfer_completed_handler(DBusGProxy *dbus_g_proxy, const gchar *transfer, const gboolean success, gpointer data);
static void transfer_started_handler(DBusGProxy *dbus_g_proxy, const gchar *transfer, gpointer data);

static void obexmanager_dispose(GObject *gobject)
{
	OBEXManager *self = OBEXMANAGER(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "SessionCreated", G_CALLBACK(session_created_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "SessionRemoved", G_CALLBACK(session_removed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "TransferCompleted", G_CALLBACK(transfer_completed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "TransferStarted", G_CALLBACK(transfer_started_handler), self);

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

	signals[TRANSFER_COMPLETED] = g_signal_new("TransferCompleted",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bt_marshal_VOID__STRING_BOOLEAN,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_BOOLEAN);

	signals[TRANSFER_STARTED] = g_signal_new("TransferStarted",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void obexmanager_init(OBEXManager *self)
{
	self->priv = OBEXMANAGER_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(session_conn != NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex", OBEXMANAGER_DBUS_PATH, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", OBEXMANAGER_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", OBEXMANAGER_DBUS_INTERFACE, OBEXMANAGER_DBUS_PATH);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);

	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex", OBEXMANAGER_DBUS_PATH, OBEXMANAGER_DBUS_INTERFACE);

	/* DBus signals connection */

	/* SessionCreated(object session) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "SessionCreated", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "SessionCreated", G_CALLBACK(session_created_handler), self, NULL);

	/* SessionRemoved(object session) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "SessionRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "SessionRemoved", G_CALLBACK(session_removed_handler), self, NULL);

	/* TransferCompleted(object transfer, boolean success) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "TransferCompleted", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_BOOLEAN, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "TransferCompleted", G_CALLBACK(transfer_completed_handler), self, NULL);

	/* TransferStarted(object transfer) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "TransferStarted", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "TransferStarted", G_CALLBACK(transfer_started_handler), self, NULL);
}

/* Methods */

/* void RegisterAgent(object agent) */
void obexmanager_register_agent(OBEXManager *self, const gchar *agent, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RegisterAgent", error, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void UnregisterAgent(object agent) */
void obexmanager_unregister_agent(OBEXManager *self, const gchar *agent, GError **error)
{
	g_assert(OBEXMANAGER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "UnregisterAgent", error, DBUS_TYPE_G_OBJECT_PATH, agent, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Signals handlers */
static void session_created_handler(DBusGProxy *dbus_g_proxy, const gchar *session, gpointer data)
{
	OBEXManager *self = OBEXMANAGER(data);

	g_signal_emit(self, signals[SESSION_CREATED], 0, session);
}

static void session_removed_handler(DBusGProxy *dbus_g_proxy, const gchar *session, gpointer data)
{
	OBEXManager *self = OBEXMANAGER(data);

	g_signal_emit(self, signals[SESSION_REMOVED], 0, session);
}

static void transfer_completed_handler(DBusGProxy *dbus_g_proxy, const gchar *transfer, const gboolean success, gpointer data)
{
	OBEXManager *self = OBEXMANAGER(data);

	g_signal_emit(self, signals[TRANSFER_COMPLETED], 0, transfer, success);
}

static void transfer_started_handler(DBusGProxy *dbus_g_proxy, const gchar *transfer, gpointer data)
{
	OBEXManager *self = OBEXMANAGER(data);

	g_signal_emit(self, signals[TRANSFER_STARTED], 0, transfer);
}

