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

#include "serial.h"

#define SERIAL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), SERIAL_TYPE, SerialPrivate))

struct _SerialPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;
};

G_DEFINE_TYPE(Serial, serial, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH /* readwrite, construct only */
};

static void _serial_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _serial_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void serial_dispose(GObject *gobject)
{
	Serial *self = SERIAL(gobject);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(serial_parent_class)->dispose(gobject);
}

static void serial_class_init(SerialClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = serial_dispose;

	g_type_class_add_private(klass, sizeof(SerialPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _serial_get_property;
	gobject_class->set_property = _serial_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);
}

static void serial_init(Serial *self)
{
	self->priv = SERIAL_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(system_conn != NULL);
}

static void serial_post_init(Serial *self, const gchar *dbus_object_path)
{
	g_assert(dbus_object_path != NULL);
	g_assert(strlen(dbus_object_path) > 0);
	g_assert(self->priv->dbus_g_proxy == NULL);

	GError *error = NULL;

	/* Getting introspection XML */
	self->priv->introspection_g_proxy = dbus_g_proxy_new_for_name(system_conn, "org.bluez", dbus_object_path, "org.freedesktop.DBus.Introspectable");
	self->priv->introspection_xml = NULL;
	if (!dbus_g_proxy_call(self->priv->introspection_g_proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING, &self->priv->introspection_xml, G_TYPE_INVALID)) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", SERIAL_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", SERIAL_DBUS_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(system_conn, "org.bluez", dbus_object_path, SERIAL_DBUS_INTERFACE);
}

static void _serial_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Serial *self = SERIAL(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, serial_get_dbus_object_path(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _serial_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Serial *self = SERIAL(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		serial_post_init(self, g_value_get_string(value));
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

/* string Connect(string pattern) */
gchar *serial_connect(Serial *self, const gchar *pattern, GError **error)
{
	g_assert(SERIAL_IS(self));

	gchar *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Connect", error, G_TYPE_STRING, pattern, G_TYPE_INVALID, G_TYPE_STRING, &ret, G_TYPE_INVALID);

	return ret;
}

/* void Disconnect(string device) */
void serial_disconnect(Serial *self, const gchar *device, GError **error)
{
	g_assert(SERIAL_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Disconnect", error, G_TYPE_STRING, device, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* Properties access methods */
const gchar *serial_get_dbus_object_path(Serial *self)
{
	g_assert(SERIAL_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

