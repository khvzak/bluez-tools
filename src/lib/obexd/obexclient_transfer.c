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

#include "obexclient_transfer.h"

#define OBEXCLIENT_TRANSFER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OBEXCLIENT_TRANSFER_TYPE, OBEXClientTransferPrivate))

struct _OBEXClientTransferPrivate {
	DBusGProxy *dbus_g_proxy;

	/* Introspection data */
	DBusGProxy *introspection_g_proxy;
	gchar *introspection_xml;

	/* Properties */
	gchar *filename;
	gchar *name;
	guint64 size;
};

G_DEFINE_TYPE(OBEXClientTransfer, obexclient_transfer, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_DBUS_OBJECT_PATH, /* readwrite, construct only */
	PROP_FILENAME, /* readonly */
	PROP_NAME, /* readonly */
	PROP_SIZE /* readonly */
};

static void _obexclient_transfer_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _obexclient_transfer_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void obexclient_transfer_dispose(GObject *gobject)
{
	OBEXClientTransfer *self = OBEXCLIENT_TRANSFER(gobject);

	/* Properties free */
	g_free(self->priv->filename);
	g_free(self->priv->name);

	/* Proxy free */
	g_object_unref(self->priv->dbus_g_proxy);

	/* Introspection data free */
	g_free(self->priv->introspection_xml);
	g_object_unref(self->priv->introspection_g_proxy);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(obexclient_transfer_parent_class)->dispose(gobject);
}

static void obexclient_transfer_class_init(OBEXClientTransferClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = obexclient_transfer_dispose;

	g_type_class_add_private(klass, sizeof(OBEXClientTransferPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _obexclient_transfer_get_property;
	gobject_class->set_property = _obexclient_transfer_set_property;

	/* object DBusObjectPath [readwrite, construct only] */
	pspec = g_param_spec_string("DBusObjectPath", "dbus_object_path", "Adapter D-Bus object path", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);

	/* string Filename [readonly] */
	pspec = g_param_spec_string("Filename", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_FILENAME, pspec);

	/* string Name [readonly] */
	pspec = g_param_spec_string("Name", NULL, NULL, NULL, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	/* uint64 Size [readonly] */
	pspec = g_param_spec_uint64("Size", NULL, NULL, 0, 0xFFFFFFFFFFFFFFFF, 0, G_PARAM_READABLE);
	g_object_class_install_property(gobject_class, PROP_SIZE, pspec);
}

static void obexclient_transfer_init(OBEXClientTransfer *self)
{
	self->priv = OBEXCLIENT_TRANSFER_GET_PRIVATE(self);

	/* DBusGProxy init */
	self->priv->dbus_g_proxy = NULL;

	g_assert(session_conn != NULL);
}

static void obexclient_transfer_post_init(OBEXClientTransfer *self, const gchar *dbus_object_path)
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

	gchar *check_intf_regex_str = g_strconcat("<interface name=\"", OBEXCLIENT_TRANSFER_DBUS_INTERFACE, "\">", NULL);
	if (!g_regex_match_simple(check_intf_regex_str, self->priv->introspection_xml, 0, 0)) {
		g_critical("Interface \"%s\" does not exist in \"%s\"", OBEXCLIENT_TRANSFER_DBUS_INTERFACE, dbus_object_path);
		g_assert(FALSE);
	}
	g_free(check_intf_regex_str);
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(session_conn, "org.openobex.client", dbus_object_path, OBEXCLIENT_TRANSFER_DBUS_INTERFACE);

	/* Properties init */
	GHashTable *properties = obexclient_transfer_get_properties(self, &error);
	if (error != NULL) {
		g_critical("%s", error->message);
	}
	g_assert(error == NULL);
	g_assert(properties != NULL);

	/* string Filename [readonly] */
	if (g_hash_table_lookup(properties, "Filename")) {
		self->priv->filename = g_value_dup_string(g_hash_table_lookup(properties, "Filename"));
	} else {
		self->priv->filename = g_strdup("undefined");
	}

	/* string Name [readonly] */
	if (g_hash_table_lookup(properties, "Name")) {
		self->priv->name = g_value_dup_string(g_hash_table_lookup(properties, "Name"));
	} else {
		self->priv->name = g_strdup("undefined");
	}

	/* uint64 Size [readonly] */
	if (g_hash_table_lookup(properties, "Size")) {
		self->priv->size = g_value_get_uint64(g_hash_table_lookup(properties, "Size"));
	} else {
		self->priv->size = 0;
	}

	g_hash_table_unref(properties);
}

static void _obexclient_transfer_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	OBEXClientTransfer *self = OBEXCLIENT_TRANSFER(object);

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		g_value_set_string(value, obexclient_transfer_get_dbus_object_path(self));
		break;

	case PROP_FILENAME:
		g_value_set_string(value, obexclient_transfer_get_filename(self));
		break;

	case PROP_NAME:
		g_value_set_string(value, obexclient_transfer_get_name(self));
		break;

	case PROP_SIZE:
		g_value_set_uint64(value, obexclient_transfer_get_size(self));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _obexclient_transfer_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	OBEXClientTransfer *self = OBEXCLIENT_TRANSFER(object);
	GError *error = NULL;

	switch (property_id) {
	case PROP_DBUS_OBJECT_PATH:
		obexclient_transfer_post_init(self, g_value_get_string(value));
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
void obexclient_transfer_cancel(OBEXClientTransfer *self, GError **error)
{
	g_assert(OBEXCLIENT_TRANSFER_IS(self));

	dbus_g_proxy_call(self->priv->dbus_g_proxy, "Cancel", error, G_TYPE_INVALID, G_TYPE_INVALID);
}

/* dict GetProperties() */
GHashTable *obexclient_transfer_get_properties(OBEXClientTransfer *self, GError **error)
{
	g_assert(OBEXCLIENT_TRANSFER_IS(self));

	GHashTable *ret = NULL;
	dbus_g_proxy_call(self->priv->dbus_g_proxy, "GetProperties", error, G_TYPE_INVALID, DBUS_TYPE_G_STRING_VARIANT_HASHTABLE, &ret, G_TYPE_INVALID);

	return ret;
}

/* Properties access methods */
const gchar *obexclient_transfer_get_dbus_object_path(OBEXClientTransfer *self)
{
	g_assert(OBEXCLIENT_TRANSFER_IS(self));

	return dbus_g_proxy_get_path(self->priv->dbus_g_proxy);
}

const gchar *obexclient_transfer_get_filename(OBEXClientTransfer *self)
{
	g_assert(OBEXCLIENT_TRANSFER_IS(self));

	return self->priv->filename;
}

const gchar *obexclient_transfer_get_name(OBEXClientTransfer *self)
{
	g_assert(OBEXCLIENT_TRANSFER_IS(self));

	return self->priv->name;
}

const guint64 obexclient_transfer_get_size(OBEXClientTransfer *self)
{
	g_assert(OBEXCLIENT_TRANSFER_IS(self));

	return self->priv->size;
}

