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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

#include "../dbus-common.h"

#include "obexclient_transfer.h"
#include "obexagent.h"

#define OBEXAGENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXAGENT_TYPE, OBEXAgentPrivate))

struct _OBEXAgentPrivate {
	DBusGProxy *proxy;
};

G_DEFINE_TYPE(OBEXAgent, obexagent, G_TYPE_OBJECT);

static void obexagent_dispose(GObject *gobject)
{
	OBEXAgent *self = OBEXAGENT(gobject);

	dbus_g_connection_unregister_g_object(session_conn, gobject);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexagent_parent_class)->dispose(gobject);
}

static void obexagent_class_init(OBEXAgentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexagent_dispose;

	g_type_class_add_private(klass, sizeof(OBEXAgentPrivate));
}

static void obexagent_init(OBEXAgent *self)
{
	self->priv = OBEXAGENT_GET_PRIVATE(self);

	g_assert(conn != NULL);

	dbus_g_connection_register_g_object(session_conn, OBEXAGENT_DBUS_PATH, G_OBJECT(self));

	g_print("OBEX Agent registered\n");
}

/* Methods */

/* Agent API */
gboolean obexagent_authorize(OBEXAgent *self, const gchar *transfer, const gchar *bt_address, const gchar *name, const gchar *type, gint length, gint time, gchar **ret, GError **error)
{
	*ret = NULL;
	g_print("[Bluetooth ObjectPush request]\n");
	OBEXClientTransfer *transfer_obj = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
	g_print("  Transfer: %s (%llu bytes), %s\n", obexclient_transfer_get_name(transfer_obj), obexclient_transfer_get_size(transfer_obj), obexclient_transfer_get_filename(transfer_obj));
	g_object_unref(transfer_obj);
	g_print("  Address: %s\n", bt_address);
	g_print("  Name: %s\n", name);
	g_print("  Type: %s\n", type);
	g_print("  Length: %d\n", length);
	g_print("  Time: %d\n", time);

	gchar yn[4] = {0,};
	g_print("Accept (yes/no)? ");
	errno = 0;
	if (scanf("%3s", yn) == EOF && errno) {
		g_warning("%s\n", strerror(errno));
	}
	if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
		*ret = g_strdup("/home/zak/obp.ext");
		return TRUE;
	} else {
		// TODO: Fix error code
		if (error)
			*error = g_error_new(g_quark_from_static_string("org.openobex.Error.Rejected"), 0, "File transfer rejected");
		return FALSE;
	}

	return TRUE;
}

gboolean obexagent_cancel(OBEXAgent *self, GError **error)
{
	g_print("Request cancelled\n");
	return TRUE;
}

/* Client API */
gboolean obexagent_release(OBEXAgent *self, GError **error)
{
	g_print("OBEX Agent released\n");
	return TRUE;
}

gboolean obexagent_request(OBEXAgent *self, const gchar *transfer, gchar **ret, GError **error)
{
	*ret = NULL;
	OBEXClientTransfer *transfer_obj = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
	g_print("Transfer: %s (%llu bytes), %s\n", obexclient_transfer_get_name(transfer_obj), obexclient_transfer_get_size(transfer_obj), obexclient_transfer_get_filename(transfer_obj));
	g_object_unref(transfer_obj);

	gchar yn[4] = {0,};
	g_print("Accept (yes/no)? ");
	errno = 0;
	if (scanf("%3s", yn) == EOF && errno) {
		g_warning("%s\n", strerror(errno));
	}
	if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
		return TRUE;
	} else {
		// TODO: Fix error code
		if (error)
			*error = g_error_new(g_quark_from_static_string("org.openobex.Error.Rejected"), 0, "File transfer rejected");
		return FALSE;
	}

	return TRUE;
}

gboolean obexagent_progress(OBEXAgent *self, const gchar *transfer, guint64 transferred, GError **error)
{
	OBEXClientTransfer *transfer_obj = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
	g_print("Transfer progress: %s (%llu/%llu bytes), %s\n", obexclient_transfer_get_name(transfer_obj), transferred, obexclient_transfer_get_size(transfer_obj), obexclient_transfer_get_filename(transfer_obj));
	g_object_unref(transfer_obj);

	return TRUE;
}

gboolean obexagent_complete(OBEXAgent *self, const gchar *transfer, GError **error)
{
	OBEXClientTransfer *transfer_obj = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
	g_print("Transfer complete: %s (%llu bytes), %s\n", obexclient_transfer_get_name(transfer_obj), obexclient_transfer_get_size(transfer_obj), obexclient_transfer_get_filename(transfer_obj));
	g_object_unref(transfer_obj);

	return TRUE;
}

gboolean obexagent_error(OBEXAgent *self, const gchar *transfer, const gchar *message, GError **error)
{
	OBEXClientTransfer *transfer_obj = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
	g_print("Transfer error: %s (%llu bytes), %s: %s\n", obexclient_transfer_get_name(transfer_obj), obexclient_transfer_get_size(transfer_obj), obexclient_transfer_get_filename(transfer_obj), message);
	g_object_unref(transfer_obj);

	return TRUE;
}

