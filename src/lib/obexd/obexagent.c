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
#include <dbus/dbus-glib.h>

#include "../dbus-common.h"
#include "../helpers.h"

#include "obexclient_transfer.h"
#include "obexagent.h"

#define OBEXAGENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXAGENT_TYPE, OBEXAgentPrivate))

struct _OBEXAgentPrivate {
	/* Unused */
	DBusGProxy *proxy;

	/* Properties */
	gchar *root_folder;
};

G_DEFINE_TYPE(OBEXAgent, obexagent, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_ROOT_FOLDER, /* readwrite, construct only */
};

static void _obexagent_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obexagent_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	OBEXAGENT_RELEASED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void obexagent_dispose(GObject *gobject)
{
	OBEXAgent *self = OBEXAGENT(gobject);

	dbus_g_connection_unregister_g_object(session_conn, gobject);

	/* Proxy free */
	//g_object_unref(self->priv->proxy);

	/* Properties free */
	g_free(self->priv->root_folder);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexagent_parent_class)->dispose(gobject);
}

static void obexagent_class_init(OBEXAgentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexagent_dispose;

	g_type_class_add_private(klass, sizeof(OBEXAgentPrivate));

	/* Signals registation */
	signals[OBEXAGENT_RELEASED] = g_signal_new("AgentReleased",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _obexagent_get_property;
	gobject_class->set_property = _obexagent_set_property;

	/* string RootFolder [readwrite, construct only] */
	pspec = g_param_spec_string("RootFolder", "root_folder", "Root folder location", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_ROOT_FOLDER, pspec);
}

static void obexagent_init(OBEXAgent *self)
{
	self->priv = OBEXAGENT_GET_PRIVATE(self);

	g_assert(session_conn != NULL);

	/* Properties init */
	self->priv->root_folder = NULL;

	dbus_g_connection_register_g_object(session_conn, OBEXAGENT_DBUS_PATH, G_OBJECT(self));
	g_print("OBEXAgent registered\n");
}

static void _obexagent_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXAgent *self = OBEXAGENT(object);

	switch (property_id) {
	case PROP_ROOT_FOLDER:
		g_value_set_string(value, self->priv->root_folder);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _obexagent_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	OBEXAgent *self = OBEXAGENT(object);

	switch (property_id) {
	case PROP_ROOT_FOLDER:
		self->priv->root_folder = g_value_dup_string(value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/* Methods */

/* Server API */
gboolean obexagent_authorize(OBEXAgent *self, const gchar *transfer, const gchar *bt_address, const gchar *name, const gchar *type, gint length, gint time, gchar **ret, GError **error)
{
	g_assert(self->priv->root_folder != NULL && strlen(self->priv->root_folder) > 0);

	*ret = NULL;
	g_print("[ObjectPush Request]\n");
	g_print("  Address: %s\n", bt_address);
	g_print("  Name: %s\n", name);
	if (type && strlen(type)) {
		g_print("  Type: %s\n", type);
	}
	g_print("  Length: %d bytes\n", length);
	/*if (time) {
		g_print("  Time: %d\n", time);
	}*/

	gchar yn[4] = {0,};
	g_print("Accept (yes/no)? ");
	errno = 0;
	if (scanf("%3s", yn) == EOF && errno) {
		g_warning("%s\n", strerror(errno));
	}
	if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
		if (!g_path_is_absolute(self->priv->root_folder)) {
			gchar *abs_path = get_absolute_path(self->priv->root_folder);
			*ret = g_build_filename(abs_path, name, NULL);
			g_free(abs_path);
		} else {
			*ret = g_build_filename(self->priv->root_folder, name, NULL);
		}

		return TRUE;
	} else {
		// TODO: Fix error code
		if (error)
			*error = g_error_new(g_quark_from_static_string("org.openobex.Error.Rejected"), 0, "File transfer rejected");
		return FALSE;
	}
}

gboolean obexagent_cancel(OBEXAgent *self, GError **error)
{
	g_print("Request cancelled\n");
	return TRUE;
}

/* Client API */
static gboolean update_progress = FALSE;

gboolean obexagent_release(OBEXAgent *self, GError **error)
{
	if (update_progress) {
		g_print("\n");
		update_progress = FALSE;
	}

	g_print("OBEXAgent released\n");

	g_signal_emit(self, signals[OBEXAGENT_RELEASED], 0);

	return TRUE;
}

gboolean obexagent_request(OBEXAgent *self, const gchar *transfer, gchar **ret, GError **error)
{
	*ret = NULL;

	if (intf_supported(OBEXC_DBUS_NAME, transfer, OBEXCLIENT_TRANSFER_DBUS_INTERFACE)) {
		OBEXClientTransfer *transfer_t = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
		g_print("[Transfer Request]\n");
		g_print("  Name: %s\n", obexclient_transfer_get_name(transfer_t));
		g_print("  Size: %llu bytes\n", obexclient_transfer_get_size(transfer_t));
		g_print("  Filename: %s\n", obexclient_transfer_get_filename(transfer_t));
		g_object_unref(transfer_t);

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
	} else {
		g_print("Error: Unknown transfer request\n");
		// TODO: Fix error code
		if (error)
			*error = g_error_new(g_quark_from_static_string("org.openobex.Error.Rejected"), 0, "File transfer rejected");
		return FALSE;
	}

	return TRUE;
}

gboolean obexagent_progress(OBEXAgent *self, const gchar *transfer, guint64 transferred, GError **error)
{
	if (intf_supported(OBEXC_DBUS_NAME, transfer, OBEXCLIENT_TRANSFER_DBUS_INTERFACE)) {
		OBEXClientTransfer *transfer_t = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
		guint64 total = obexclient_transfer_get_size(transfer_t);

		guint pp = (transferred / (gfloat) total)*100;

		if (!update_progress) {
			g_print("[Transfer#%s] Progress: %3u%%", obexclient_transfer_get_name(transfer_t), pp);
			update_progress = TRUE;
		} else {
			g_print("\b\b\b\b%3u%%", pp);
		}

		if (pp == 100) {
			g_print("\n");
			update_progress = FALSE;
		}

		g_object_unref(transfer_t);
	}

	return TRUE;
}

gboolean obexagent_complete(OBEXAgent *self, const gchar *transfer, GError **error)
{
	if (update_progress) {
		g_print("\n");
		update_progress = FALSE;
	}

	if (intf_supported(OBEXC_DBUS_NAME, transfer, OBEXCLIENT_TRANSFER_DBUS_INTERFACE)) {
		OBEXClientTransfer *transfer_t = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
		g_print("[Transfer#%s] Completed\n", obexclient_transfer_get_name(transfer_t));
		g_object_unref(transfer_t);
	}

	return TRUE;
}

gboolean obexagent_error(OBEXAgent *self, const gchar *transfer, const gchar *message, GError **error)
{
	if (update_progress) {
		g_print("\n");
		update_progress = FALSE;
	}

	g_print("[Transfer] Error: %s\n", message);

	/*
	 * Transfer interface does not exists
	 * Code commented
	 *
	if (intf_supported(OBEXC_DBUS_NAME, transfer, OBEXCLIENT_TRANSFER_DBUS_INTERFACE)) {
		OBEXClientTransfer *transfer_t = g_object_new(OBEXCLIENT_TRANSFER_TYPE, "DBusObjectPath", transfer, NULL);
		g_print("[Transfer#%s] Error: %s\n", obexclient_transfer_get_name(transfer_t), message);
		g_object_unref(transfer_t);
	}
	 */

	return TRUE;
}

