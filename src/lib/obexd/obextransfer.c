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

#include "obextransfer.h"

#define OBEXTRANSFER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXTRANSFER_TYPE, OBEXTransferPrivate))

struct _OBEXTransferPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;
};

G_DEFINE_TYPE(OBEXTransfer, obextransfer, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH /* readwrite, construct only */
};

static void _obextransfer_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obextransfer_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	PROGRESS,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void progress_handler(DBusGProxy *dbus_g_proxy, const gint32 total, const gint32 transfered, gpointer data);

static void obextransfer_dispose(GObject *gobject)
{
	OBEXTransfer *self = OBEXTRANSFER(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "Progress", G_CALLBACK(progress_handler), self);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obextransfer_parent_class)->dispose(gobject);
}

static void obextransfer_class_init(OBEXTransferClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obextransfer_dispose;

	g_type_class_add_private(klass, sizeof(OBEXTransferPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _obextransfer_get_property;
	gobject_class->set_property = _obextransfer_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* Signals registation */
	signals[PROGRESS] = g_signal_new("Progress",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bt_marshal_VOID__INT_INT,
			G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
}

static void obextransfer_init(OBEXTransfer *self)
{
	self->priv = OBEXTRANSFER_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(session_conn != NULL);
}

static void obextransfer_post_init(OBEXTransfer *self, const gchar *dbus_object_path)
{
	g_assert(dbus_object_path != NULL);
	g_assert(strlen(dbus_object_path) > 0);
	g_assert(self->priv->dbus_g_proxy == NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex", dbus_object_path, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", OBEXTRANSFER_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", OBEXTRANSFER_DBUS_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex", dbus_object_path, OBEXTRANSFER_DBUS_INTERFACE);

	/* DBus signals connection */

	/* Progress(int32 total, int32 transfered) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "Progress", G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "Progress", G_CALLBACK(progress_handler), self, NULL);
}

static void _obextransfer_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXTransfer *self = OBEXTRANSFER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, obextransfer_get_dbus_object_path(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _obextransfer_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	OBEXTransfer *self = OBEXTRANSFER(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		obextransfer_post_init(self, g_value_get_string(value));
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
void obextransfer_cancel(OBEXTransfer *self, GError **error)
{
	g_assert(OBEXTRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Cancel", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *obextransfer_get_dbus_object_path(OBEXTransfer *self)
{
	g_assert(OBEXTRANSFER_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

/* Signals handlers */
static void progress_handler(DBusGProxy *dbus_g_proxy, const gint32 total, const gint32 transfered, gpointer data)
{
	OBEXTransfer *self = OBEXTRANSFER(data);

	g_signal_emit(self, signals[PROGRESS], 0, total, transfered);
}

