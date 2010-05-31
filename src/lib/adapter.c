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
#include "adapter.h"

#define ADAPTER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), ADAPTER_TYPE, AdapterPrivate))

struct _AdapterPrivate {
	DBusGProxy *dbus_g_proxy;
};

G_DEFINE_TYPE(Adapter, adapter, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_G_PROXY
};

static void adapter_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void adapter_get_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void adapter_class_init(AdapterClass *klass)
{
	g_type_class_add_private(klass, sizeof(AdapterPrivate));

	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->set_property = adapter_set_property;
	gobject_class->get_property = adapter_get_property;

	pspec = g_param_spec_string("adapter_dbus_g_proxy", "adapter_dbus_g_proxy", "Adapter DBUS GProxy", NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, PROP_DBUS_G_PROXY, pspec);
}

static void adapter_init(Adapter *self)
{
	AdapterPrivate *priv;

	self->priv = priv = ADAPTER_GET_PRIVATE(self);

	// TODO: Assert for conn
	// g_assert(!conn);
}

static void adapter_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Adapter *self = ADAPTER(object);

	switch (property_id) {
	case PROP_DBUS_G_PROXY:
		self->priv->dbus_g_proxy = g_value_get_object(value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void adapter_get_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Adapter *self = ADAPTER(object);

	switch (property_id) {
	case PROP_DBUS_G_PROXY:
		g_value_set_object(value, self->priv->dbus_g_proxy);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

