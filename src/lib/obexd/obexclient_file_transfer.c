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

#include "obexclient_file_transfer.h"

#define OBEXCLIENT_FILE_TRANSFER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXCLIENT_FILE_TRANSFER_TYPE, OBEXClientFileTransferPrivate))

struct _OBEXClientFileTransferPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;
};

G_DEFINE_TYPE(OBEXClientFileTransfer, obexclient_file_transfer, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH /* readwrite, construct only */
};

static void _obexclient_file_transfer_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obexclient_file_transfer_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void obexclient_file_transfer_dispose(GObject *gobject)
{
	OBEXClientFileTransfer *self = OBEXCLIENT_FILE_TRANSFER(gobject);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexclient_file_transfer_parent_class)->dispose(gobject);
}

static void obexclient_file_transfer_class_init(OBEXClientFileTransferClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexclient_file_transfer_dispose;

	g_type_class_add_private(klass, sizeof(OBEXClientFileTransferPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _obexclient_file_transfer_get_property;
	gobject_class->set_property = _obexclient_file_transfer_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);
}

static void obexclient_file_transfer_init(OBEXClientFileTransfer *self)
{
	self->priv = OBEXCLIENT_FILE_TRANSFER_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(session_conn != NULL);
}

static void obexclient_file_transfer_post_init(OBEXClientFileTransfer *self, const gchar *dbus_object_path)
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

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", OBEXCLIENT_FILE_TRANSFER_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", OBEXCLIENT_FILE_TRANSFER_DBUS_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex.client", dbus_object_path, OBEXCLIENT_FILE_TRANSFER_DBUS_INTERFACE);
}

static void _obexclient_file_transfer_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXClientFileTransfer *self = OBEXCLIENT_FILE_TRANSFER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, obexclient_file_transfer_get_dbus_object_path(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _obexclient_file_transfer_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	OBEXClientFileTransfer *self = OBEXCLIENT_FILE_TRANSFER(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		obexclient_file_transfer_post_init(self, g_value_get_string(value));
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

/* void ChangeFolder(string folder) */
void obexclient_file_transfer_change_folder(OBEXClientFileTransfer *self, const gchar *folder, GError **error)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "ChangeFolder", error, G_TYPE_STRING, folder, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void CopyFile(string sourcefile, string targetfile) */
void obexclient_file_transfer_copy_file(OBEXClientFileTransfer *self, const gchar *sourcefile, const gchar *targetfile, GError **error)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CopyFile", error, G_TYPE_STRING, sourcefile, G_TYPE_STRING, targetfile, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void CreateFolder(string folder) */
void obexclient_file_transfer_create_folder(OBEXClientFileTransfer *self, const gchar *folder, GError **error)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "CreateFolder", error, G_TYPE_STRING, folder, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Delete(string file) */
void obexclient_file_transfer_delete(OBEXClientFileTransfer *self, const gchar *file, GError **error)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Delete", error, G_TYPE_STRING, file, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void GetFile(string targetfile, string sourcefile) */
void obexclient_file_transfer_get_file(OBEXClientFileTransfer *self, const gchar *targetfile, const gchar *sourcefile, GError **error)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetFile", error, G_TYPE_STRING, targetfile, G_TYPE_STRING, sourcefile, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* array{dict} ListFolder() */
GPtrArray *obexclient_file_transfer_list_folder(OBEXClientFileTransfer *self, GError **error)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	GPtrArray *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "ListFolder", error, G_TYPE_INVALID, DBUS_TYPE_G_HASH_TABLE_ARRAY, &ret, G_TYPE_INVALID);

	return ret;
}

/* void MoveFile(string sourcefile, string targetfile) */
void obexclient_file_transfer_move_file(OBEXClientFileTransfer *self, const gchar *sourcefile, const gchar *targetfile, GError **error)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "MoveFile", error, G_TYPE_STRING, sourcefile, G_TYPE_STRING, targetfile, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void PutFile(string sourcefile, string targetfile) */
void obexclient_file_transfer_put_file(OBEXClientFileTransfer *self, const gchar *sourcefile, const gchar *targetfile, GError **error)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "PutFile", error, G_TYPE_STRING, sourcefile, G_TYPE_STRING, targetfile, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *obexclient_file_transfer_get_dbus_object_path(OBEXClientFileTransfer *self)
{
	g_assert(OBEXCLIENT_FILE_TRANSFER_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

