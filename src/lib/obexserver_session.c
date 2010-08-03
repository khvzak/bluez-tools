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
#include "obexserver_session.h"

#define OBEXSERVER_SESSION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXSERVER_SESSION_TYPE, OBEXServerSessionPrivate))

struct _OBEXServerSessionPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;
};

G_DEFINE_TYPE(OBEXServerSession, obexserver_session, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH /* readwrite, construct only */
};

static void _obexserver_session_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obexserver_session_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	CANCELLED,
	DISCONNECTED,
	ERROR_OCCURRED,
	TRANSFER_COMPLETED,
	TRANSFER_PROGRESS,
	TRANSFER_STARTED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void cancelled_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void disconnected_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void error_occurred_handler(DBusGProxy *dbus_g_proxy, const gchar *error_name, const gchar *error_message, gpointer data);
static void transfer_completed_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void transfer_progress_handler(DBusGProxy *dbus_g_proxy, const guint64 bytes_transferred, gpointer data);
static void transfer_started_handler(DBusGProxy *dbus_g_proxy, const gchar *filename, const gchar *local_path, const guint64 total_bytes, gpointer data);

static void obexserver_session_dispose(GObject *gobject)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "Cancelled", G_CALLBACK(cancelled_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "Disconnected", G_CALLBACK(disconnected_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "ErrorOccurred", G_CALLBACK(error_occurred_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "TransferCompleted", G_CALLBACK(transfer_completed_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "TransferProgress", G_CALLBACK(transfer_progress_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "TransferStarted", G_CALLBACK(transfer_started_handler), self);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexserver_session_parent_class)->dispose(gobject);
}

static void obexserver_session_class_init(OBEXServerSessionClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexserver_session_dispose;

	g_type_class_add_private(klass, sizeof(OBEXServerSessionPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _obexserver_session_get_property;
	gobject_class->set_property = _obexserver_session_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* Signals registation */
	signals[CANCELLED] = g_signal_new("Cancelled",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[DISCONNECTED] = g_signal_new("Disconnected",
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

	signals[TRANSFER_COMPLETED] = g_signal_new("TransferCompleted",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[TRANSFER_PROGRESS] = g_signal_new("TransferProgress",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bluez_marshal_VOID__UINT64,
			G_TYPE_NONE, 1, G_TYPE_UINT64);

	signals[TRANSFER_STARTED] = g_signal_new("TransferStarted",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bluez_marshal_VOID__STRING_STRING_UINT64,
			G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64);
}

static void obexserver_session_init(OBEXServerSession *self)
{
	self->priv = OBEXSERVER_SESSION_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void obexserver_session_post_init(OBEXServerSession *self, const gchar *dbus_object_path)
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

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", BLUEZ_DBUS_OBEXSERVER_SESSION_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", BLUEZ_DBUS_OBEXSERVER_SESSION_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, dbus_object_path, BLUEZ_DBUS_OBEXSERVER_SESSION_INTERFACE);

	/* DBus signals connection */

	/* Cancelled() */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "Cancelled", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "Cancelled", G_CALLBACK(cancelled_handler), self, NULL);

	/* Disconnected() */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "Disconnected", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "Disconnected", G_CALLBACK(disconnected_handler), self, NULL);

	/* ErrorOccurred(string error_name, string error_message) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "ErrorOccurred", G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "ErrorOccurred", G_CALLBACK(error_occurred_handler), self, NULL);

	/* TransferCompleted() */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "TransferCompleted", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "TransferCompleted", G_CALLBACK(transfer_completed_handler), self, NULL);

	/* TransferProgress(uint64 bytes_transferred) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "TransferProgress", G_TYPE_UINT64, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "TransferProgress", G_CALLBACK(transfer_progress_handler), self, NULL);

	/* TransferStarted(string filename, string local_path, uint64 total_bytes) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "TransferStarted", G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "TransferStarted", G_CALLBACK(transfer_started_handler), self, NULL);
}

static void _obexserver_session_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, obexserver_session_get_dbus_object_path(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _obexserver_session_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		obexserver_session_post_init(self, g_value_get_string(value));
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

/* void Accept() */
void obexserver_session_accept(OBEXServerSession *self, GError **error)
{
	g_assert(OBEXSERVER_SESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Accept", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Cancel() */
void obexserver_session_cancel(OBEXServerSession *self, GError **error)
{
	g_assert(OBEXSERVER_SESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Cancel", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Disconnect() */
void obexserver_session_disconnect(OBEXServerSession *self, GError **error)
{
	g_assert(OBEXSERVER_SESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Disconnect", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* dict GetTransferInfo() */
GHashTable *obexserver_session_get_transfer_info(OBEXServerSession *self, GError **error)
{
	g_assert(OBEXSERVER_SESSION_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetTransferInfo", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* void Reject() */
void obexserver_session_reject(OBEXServerSession *self, GError **error)
{
	g_assert(OBEXSERVER_SESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Reject", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *obexserver_session_get_dbus_object_path(OBEXServerSession *self)
{
	g_assert(OBEXSERVER_SESSION_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

/* Signals handlers */
static void cancelled_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(data);

	g_signal_emit(self, signals[CANCELLED], 0);
}

static void disconnected_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(data);

	g_signal_emit(self, signals[DISCONNECTED], 0);
}

static void error_occurred_handler(DBusGProxy *dbus_g_proxy, const gchar *error_name, const gchar *error_message, gpointer data)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(data);

	g_signal_emit(self, signals[ERROR_OCCURRED], 0, error_name, error_message);
}

static void transfer_completed_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(data);

	g_signal_emit(self, signals[TRANSFER_COMPLETED], 0);
}

static void transfer_progress_handler(DBusGProxy *dbus_g_proxy, const guint64 bytes_transferred, gpointer data)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(data);

	g_signal_emit(self, signals[TRANSFER_PROGRESS], 0, bytes_transferred);
}

static void transfer_started_handler(DBusGProxy *dbus_g_proxy, const gchar *filename, const gchar *local_path, const guint64 total_bytes, gpointer data)
{
	OBEXServerSession *self = OBEXSERVER_SESSION(data);

	g_signal_emit(self, signals[TRANSFER_STARTED], 0, filename, local_path, total_bytes);
}

