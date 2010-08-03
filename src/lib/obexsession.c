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
#include "obexsession.h"

#define OBEXSESSION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXSESSION_TYPE, OBEXSessionPrivate))

struct _OBEXSessionPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;
};

G_DEFINE_TYPE(OBEXSession, obexsession, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH /* readwrite, construct only */
};

static void _obexsession_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obexsession_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	CANCELLED,
	CLOSED,
	DISCONNECTED,
	ERROR_OCCURRED,
	TRANSFER_COMPLETED,
	TRANSFER_PROGRESS,
	TRANSFER_STARTED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void cancelled_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void closed_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void disconnected_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void error_occurred_handler(DBusGProxy *dbus_g_proxy, const gchar *error_name, const gchar *error_message, gpointer data);
static void transfer_completed_handler(DBusGProxy *dbus_g_proxy, gpointer data);
static void transfer_progress_handler(DBusGProxy *dbus_g_proxy, const guint64 bytes_transferred, gpointer data);
static void transfer_started_handler(DBusGProxy *dbus_g_proxy, const gchar *filename, const gchar *local_path, const guint64 total_bytes, gpointer data);

static void obexsession_dispose(GObject *gobject)
{
	OBEXSession *self = OBEXSESSION(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "Cancelled", G_CALLBACK(cancelled_handler), self);
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "Closed", G_CALLBACK(closed_handler), self);
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

	/* Signals registation */
	signals[CANCELLED] = g_signal_new("Cancelled",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	signals[CLOSED] = g_signal_new("Closed",
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

static void obexsession_init(OBEXSession *self)
{
	self->priv = OBEXSESSION_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void obexsession_post_init(OBEXSession *self, const gchar *dbus_object_path)
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

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", BLUEZ_DBUS_OBEXSESSION_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", BLUEZ_DBUS_OBEXSESSION_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, "org.openobex", dbus_object_path, BLUEZ_DBUS_OBEXSESSION_INTERFACE);

	/* DBus signals connection */

	/* Cancelled() */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "Cancelled", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "Cancelled", G_CALLBACK(cancelled_handler), self, NULL);

	/* Closed() */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "Closed", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "Closed", G_CALLBACK(closed_handler), self, NULL);

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

static void _obexsession_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXSession *self = OBEXSESSION(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, obexsession_get_dbus_object_path(self));
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

/* void Cancel() */
void obexsession_cancel(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Cancel", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void ChangeCurrentFolder(string path) */
void obexsession_change_current_folder(OBEXSession *self, const gchar *path, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "ChangeCurrentFolder", error, G_TYPE_STRING, path, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void ChangeCurrentFolderBackward() */
void obexsession_change_current_folder_backward(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "ChangeCurrentFolderBackward", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void ChangeCurrentFolderToRoot() */
void obexsession_change_current_folder_to_root(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "ChangeCurrentFolderToRoot", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Close() */
void obexsession_close(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Close", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void CopyRemoteFile(string remote_filename, string local_path) */
void obexsession_copy_remote_file(OBEXSession *self, const gchar *remote_filename, const gchar *local_path, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CopyRemoteFile", error, G_TYPE_STRING, remote_filename, G_TYPE_STRING, local_path, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void CopyRemoteFileByType(string type, string local_path) */
void obexsession_copy_remote_file_by_type(OBEXSession *self, const gchar *type, const gchar *local_path, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CopyRemoteFileByType", error, G_TYPE_STRING, type, G_TYPE_STRING, local_path, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void CreateFolder(string folder_name) */
void obexsession_create_folder(OBEXSession *self, const gchar *folder_name, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CreateFolder", error, G_TYPE_STRING, folder_name, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void DeleteRemoteFile(string remote_filename) */
void obexsession_delete_remote_file(OBEXSession *self, const gchar *remote_filename, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "DeleteRemoteFile", error, G_TYPE_STRING, remote_filename, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Disconnect() */
void obexsession_disconnect(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Disconnect", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* string GetCapability() */
gchar *obexsession_get_capability(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetCapability", error, G_TYPE_INVALID, G_TYPE_STRING, &ret, G_TYPE_INVALID);

	return ret;
}

/* string GetCurrentPath() */
gchar *obexsession_get_current_path(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetCurrentPath", error, G_TYPE_INVALID, G_TYPE_STRING, &ret, G_TYPE_INVALID);

	return ret;
}

/* dict GetTransferInfo() */
GHashTable *obexsession_get_transfer_info(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetTransferInfo", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* boolean IsBusy() */
gboolean obexsession_is_busy(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	gboolean ret = FALSE;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "IsBusy", error, G_TYPE_INVALID, G_TYPE_BOOLEAN, &ret, G_TYPE_INVALID);

	return ret;
}

/* void RemoteCopy(string remote_source, string remote_destination) */
void obexsession_remote_copy(OBEXSession *self, const gchar *remote_source, const gchar *remote_destination, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RemoteCopy", error, G_TYPE_STRING, remote_source, G_TYPE_STRING, remote_destination, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void RemoteMove(string remote_source, string remote_destination) */
void obexsession_remote_move(OBEXSession *self, const gchar *remote_source, const gchar *remote_destination, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RemoteMove", error, G_TYPE_STRING, remote_source, G_TYPE_STRING, remote_destination, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* string RetrieveFolderListing() */
gchar *obexsession_retrieve_folder_listing(OBEXSession *self, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "RetrieveFolderListing", error, G_TYPE_INVALID, G_TYPE_STRING, &ret, G_TYPE_INVALID);

	return ret;
}

/* void SendFile(string local_path) */
void obexsession_send_file(OBEXSession *self, const gchar *local_path, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "SendFile", error, G_TYPE_STRING, local_path, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void SendFileExt(string local_path, string remote_filename, string type) */
void obexsession_send_file_ext(OBEXSession *self, const gchar *local_path, const gchar *remote_filename, const gchar *type, GError **error)
{
	g_assert(OBEXSESSION_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "SendFileExt", error, G_TYPE_STRING, local_path, G_TYPE_STRING, remote_filename, G_TYPE_STRING, type, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *obexsession_get_dbus_object_path(OBEXSession *self)
{
	g_assert(OBEXSESSION_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

/* Signals handlers */
static void cancelled_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXSession *self = OBEXSESSION(data);

	g_signal_emit(self, signals[CANCELLED], 0);
}

static void closed_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXSession *self = OBEXSESSION(data);

	g_signal_emit(self, signals[CLOSED], 0);
}

static void disconnected_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXSession *self = OBEXSESSION(data);

	g_signal_emit(self, signals[DISCONNECTED], 0);
}

static void error_occurred_handler(DBusGProxy *dbus_g_proxy, const gchar *error_name, const gchar *error_message, gpointer data)
{
	OBEXSession *self = OBEXSESSION(data);

	g_signal_emit(self, signals[ERROR_OCCURRED], 0, error_name, error_message);
}

static void transfer_completed_handler(DBusGProxy *dbus_g_proxy, gpointer data)
{
	OBEXSession *self = OBEXSESSION(data);

	g_signal_emit(self, signals[TRANSFER_COMPLETED], 0);
}

static void transfer_progress_handler(DBusGProxy *dbus_g_proxy, const guint64 bytes_transferred, gpointer data)
{
	OBEXSession *self = OBEXSESSION(data);

	g_signal_emit(self, signals[TRANSFER_PROGRESS], 0, bytes_transferred);
}

static void transfer_started_handler(DBusGProxy *dbus_g_proxy, const gchar *filename, const gchar *local_path, const guint64 total_bytes, gpointer data)
{
	OBEXSession *self = OBEXSESSION(data);

	g_signal_emit(self, signals[TRANSFER_STARTED], 0, filename, local_path, total_bytes);
}

