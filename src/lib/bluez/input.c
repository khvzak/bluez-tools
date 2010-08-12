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

#include "input.h"

#define INPUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), INPUT_TYPE, InputPrivate))

struct _InputPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;

	/* Properties */
	gboolean connected;
};

G_DEFINE_TYPE(Input, input, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_CONNECTED /* readonly */
};

static void _input_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _input_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	PROPERTY_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data);

static void input_dispose(GObject *gobject)
{
	Input *self = INPUT(gobject);

	/* DBus signals disconnection */
	dbus_g_proxy_disconnect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self);

	/* Properties free */
	/* none */

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(input_parent_class)->dispose(gobject);
}

static void input_class_init(InputClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = input_dispose;

	g_type_class_add_private(klass, sizeof(InputPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _input_get_property;
	gobject_class->set_property = _input_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* boolean Connected [readonly] */
	pspec = g_param_spec_boolean("Connected", NULL, NULL, FALSE, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_CONNECTED, pspec);

	/* Signals registation */
	signals[PROPERTY_CHANGED] = g_signal_new("PropertyChanged",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_bt_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);
}

static void input_init(Input *self)
{
	self->priv = INPUT_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(system_conn != NULL);
}

static void input_post_init(Input *self, const gchar *dbus_object_path)
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

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", INPUT_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", INPUT_DBUS_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(system_conn, "org.bluez", dbus_object_path, INPUT_DBUS_INTERFACE);

	/* DBus signals connection */

	/* PropertyChanged(string name, variant value) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self, NULL);

	/* Properties init */
	GHashTable *properties = input_get_properties(self, &error);
	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
	g_assert(properties != NULL);

	/* boolean Connected [readonly] */
	if (g_hash_table_lookup(properties, "Connected")) {
		self->priv->connected = g_value_get_boolean(g_hash_table_lookup(properties, "Connected"));
	} else {
		self->priv->connected = FALSE;
	}

	g_hash_table_unref(properties);
}

static void _input_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Input *self = INPUT(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, input_get_dbus_object_path(self));
		break;

	case PROP_CONNECTED:
		g_value_set_boolean(value, input_get_connected(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _input_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Input *self = INPUT(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		input_post_init(self, g_value_get_string(value));
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

/* void Connect() */
void input_connect(Input *self, GError **error)
{
	g_assert(INPUT_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Connect", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Disconnect() */
void input_disconnect(Input *self, GError **error)
{
	g_assert(INPUT_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Disconnect", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* dict GetProperties() */
GHashTable *input_get_properties(Input *self, GError **error)
{
	g_assert(INPUT_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* Properties access methods */
const gchar *input_get_dbus_object_path(Input *self)
{
	g_assert(INPUT_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

const gboolean input_get_connected(Input *self)
{
	g_assert(INPUT_IS(self));

	return self->priv->connected;
}

/* Signals handlers */
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data)
{
	Input *self = INPUT(data);

	if (g_strcmp0(name, "Connected") == 0) {
		self->priv->connected = g_value_get_boolean(value);
	}

	g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}

