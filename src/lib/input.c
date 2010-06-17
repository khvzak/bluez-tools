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

#include "dbus-common.h"
#include "marshallers.h"
#include "input.h"

#define BLUEZ_DBUS_INPUT_INTERFACE "org.bluez.Input"

#define INPUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), INPUT_TYPE, InputPrivate))

struct _InputPrivate {
	DBusGProxy *dbus_g_proxy;
};

G_DEFINE_TYPE(Input, input, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_CONNECTED /* readonly */
};

enum {
	PROPERTY_CHANGED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void _input_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _input_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data);

static void input_class_init(InputClass *klass)
{
	g_type_class_add_private(klass, sizeof(InputPrivate));

	/* Properties registration */
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
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
			g_cclosure_bluez_marshal_VOID__STRING_BOXED,
			G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);
}

static void input_init(Input *self)
{
	self->priv = INPUT_GET_PRIVATE(self);

	g_assert(conn != NULL);
}

static void input_post_init(Input *self)
{
	g_assert(self->priv->dbus_g_proxy != NULL);

	/* DBUS signals connection */

	/* PropertyChanged(string name, variant value) */
	dbus_g_proxy_add_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, "PropertyChanged", G_CALLBACK(property_changed_handler), self, NULL);
}

static void _input_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Input *self = INPUT(object);

	GHashTable *properties = input_get_properties(self, NULL);
	if (properties == NULL) {
		return;
	}

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, g_strdup(dbus_g_proxy_get_path(self->priv->dbus_g_proxy)));
		break;

	case PROP_CONNECTED:
		g_value_set_boolean(value, g_value_get_boolean(g_hash_table_lookup(properties, "Connected")));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

	g_hash_table_unref(properties);
}

static void _input_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Input *self = INPUT(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
	{
		const gchar *dbus_object_path = g_value_get_string(value);
		g_assert(dbus_object_path != NULL);
		g_assert(self->priv->dbus_g_proxy == NULL);
		self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, dbus_object_path, BLUEZ_DBUS_INPUT_INTERFACE);
		input_post_init(self);
	}
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/* Methods */

/* void Connect() */
void input_connect(Input *self, GError **error)
{
	g_assert(self != NULL);

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Connect", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* void Disconnect() */
void input_disconnect(Input *self, GError **error)
{
	g_assert(self != NULL);

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Disconnect", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* dict GetProperties() */
GHashTable *input_get_properties(Input *self, GError **error)
{
	g_assert(self != NULL);

	GHashTable *ret;

	if (!dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID)) {
		return NULL;
	}

	return ret;
}

/* Signals handlers */
static void property_changed_handler(DBusGProxy *dbus_g_proxy, const gchar *name, const GValue *value, gpointer data)
{
	Input *self = INPUT(data);
	g_signal_emit(self, signals[PROPERTY_CHANGED], 0, name, value);
}

