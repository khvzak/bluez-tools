/*
 *
 *  bluez-tools - a set of tools to manage bluetooth devices for linux
 *
 *  Copyright (C) 2010-2011  Alexander Orlenko <zxteam@gmail.com>
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
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

#include "../dbus-common.h"
#include "../helpers.h"

#include "device.h"
#include "agent.h"

#define AGENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), AGENT_TYPE, AgentPrivate))

struct _AgentPrivate {
	/* Properties */
	GHashTable *pin_hash_table; /* (extern) name|mac -> pin hash table */
	gboolean interactive; /* Interactive mode */
};

G_DEFINE_TYPE(Agent, agent, G_TYPE_OBJECT);

enum {
	PROP_0,

	PROP_PIN_HASH_TABLE, /* write, construct only */
	PROP_INTERACTIVE /* write, construct only */
};

static void _agent_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void _agent_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	AGENT_RELEASED,

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static void agent_dispose(GObject *gobject)
{
	Agent *self = AGENT(gobject);

	dbus_g_connection_unregister_g_object(system_conn, gobject);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(agent_parent_class)->dispose(gobject);
}

static void agent_class_init(AgentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = agent_dispose;

	g_type_class_add_private(klass, sizeof(AgentPrivate));

	/* Properties registration */
	GParamSpec *pspec;

	gobject_class->get_property = _agent_get_property;
	gobject_class->set_property = _agent_set_property;

	/* object PinHashTable [write, construct only] */
	pspec = g_param_spec_boxed("PinHashTable", "pin_hash_table", "Name|MAC -> PIN hash table", G_TYPE_HASH_TABLE, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_PIN_HASH_TABLE, pspec);

	/* boolean Interactive [write, construct only] */
	pspec = g_param_spec_boolean("Interactive", "interactive", "Interactive mode", TRUE, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(gobject_class, PROP_INTERACTIVE, pspec);

	/* Signals registation */
	signals[AGENT_RELEASED] = g_signal_new("AgentReleased",
			G_TYPE_FROM_CLASS(gobject_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0, NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
}

static void agent_init(Agent *self)
{
	self->priv = AGENT_GET_PRIVATE(self);

	g_assert(system_conn != NULL);

	/* Properties init */
	self->priv->pin_hash_table = NULL;
	self->priv->interactive = TRUE;

	dbus_g_connection_register_g_object(system_conn, AGENT_DBUS_PATH, G_OBJECT(self));
}

static void _agent_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Agent *self = AGENT(object);

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void _agent_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Agent *self = AGENT(object);

	switch (property_id) {
	case PROP_PIN_HASH_TABLE:
		self->priv->pin_hash_table = g_value_get_boxed(value);
		break;

	case PROP_INTERACTIVE:
		self->priv->interactive = g_value_get_boolean(value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/* Methods */

static const gchar *find_device_pin(Agent *self, Device *device_obj)
{
	if (self->priv->pin_hash_table) {
		const gchar *pin_by_addr = g_hash_table_lookup(self->priv->pin_hash_table, device_get_address(device_obj));
		const gchar *pin_by_alias = g_hash_table_lookup(self->priv->pin_hash_table, device_get_alias(device_obj));
		const gchar *pin_all = g_hash_table_lookup(self->priv->pin_hash_table, "*");

		if (pin_by_addr)
			return pin_by_addr;
		else if (pin_by_alias)
			return pin_by_alias;
		else if (pin_all)
			return pin_all;
	}

	return NULL;
}

gboolean agent_release(Agent *self, GError **error)
{
	g_signal_emit(self, signals[AGENT_RELEASED], 0);

	return TRUE;
}

gboolean agent_request_pin_code(Agent *self, const gchar *device, gchar **ret, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	const gchar *pin = find_device_pin(self, device_obj);

	if (self->priv->interactive)
		g_print("Device: %s (%s)\n", device_get_alias(device_obj), device_get_address(device_obj));

	g_object_unref(device_obj);

	/* Try to use found PIN */
	if (pin != NULL) {
		if (self->priv->interactive)
			g_print("PIN found\n");

		*ret = g_new0(gchar, 17);
		sscanf(pin, "%16s", *ret);

		return TRUE;
	} else if (self->priv->interactive) {
		g_print("Enter PIN code: ");
		*ret = g_new0(gchar, 17);
		errno = 0;
		if (scanf("%16s", *ret) == EOF && errno) {
			g_warning("%s\n", strerror(errno));
		}

		return TRUE;
	}

	return FALSE; // Request rejected
}

gboolean agent_request_passkey(Agent *self, const gchar *device, guint *ret, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	const gchar *pin = find_device_pin(self, device_obj);

	if (self->priv->interactive)
		g_print("Device: %s (%s)\n", device_get_alias(device_obj), device_get_address(device_obj));

	g_object_unref(device_obj);

	/* Try to use found PIN */
	if (pin != NULL) {
		if (self->priv->interactive)
			g_print("Passkey found\n");

		sscanf(pin, "%u", ret);

		return TRUE;
	} else if (self->priv->interactive) {
		g_print("Enter passkey: ");
		errno = 0;
		if (scanf("%u", ret) == EOF && errno) {
			g_warning("%s\n", strerror(errno));
		}

		return TRUE;
	}

	return FALSE; // Request rejected
}

gboolean agent_display_passkey(Agent *self, const gchar *device, guint passkey, guint8 entered, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	const gchar *pin = find_device_pin(self, device_obj);

	if (self->priv->interactive)
		g_print("Device: %s (%s)\n", device_get_alias(device_obj), device_get_address(device_obj));

	g_object_unref(device_obj);

	if (self->priv->interactive) {
		g_print("Passkey: %u, entered: %u\n", passkey, entered);

		return TRUE;
	} else if (pin != NULL) {
		/* OK, device found */
		return TRUE;
	}

	return FALSE; // Request rejected
}

gboolean agent_request_confirmation(Agent *self, const gchar *device, guint passkey, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	const gchar *pin = find_device_pin(self, device_obj);

	if (self->priv->interactive)
		g_print("Device: %s (%s)\n", device_get_alias(device_obj), device_get_address(device_obj));

	g_object_unref(device_obj);

	/* Try to use found PIN */
	if (pin != NULL) {
		guint passkey_t;
		sscanf(pin, "%u", &passkey_t);

		if (g_strcmp0(pin, "*") == 0 || passkey_t == passkey) {
			if (self->priv->interactive)
				g_print("Passkey confirmed\n");

			return TRUE;
		} else {
			if (self->priv->interactive)
				g_print("Passkey rejected\n");

			// TODO: Fix error code
			if (error)
				*error = g_error_new(g_quark_from_static_string("org.bluez.Error.Rejected"), 0, "Passkey doesn't match");
			return FALSE;
		}
	} else if (self->priv->interactive) {
		g_print("Confirm passkey: %u (yes/no)? ", passkey);
		gchar yn[4] = {0,};
		errno = 0;
		if (scanf("%3s", yn) == EOF && errno) {
			g_warning("%s\n", strerror(errno));
		}
		if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
			return TRUE;
		} else {
			// TODO: Fix error code
			if (error)
				*error = g_error_new(g_quark_from_static_string("org.bluez.Error.Rejected"), 0, "Passkey doesn't match");
			return FALSE;
		}
	}

	return FALSE; // Request rejected
}

gboolean agent_authorize(Agent *self, const gchar *device, const gchar *uuid, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	const gchar *pin = find_device_pin(self, device_obj);

	if (self->priv->interactive)
		g_print("Device: %s (%s)\n", device_get_alias(device_obj), device_get_address(device_obj));

	g_object_unref(device_obj);

	if (pin != NULL) {
		if (self->priv->interactive)
			g_print("Connection to %s authorized\n", uuid2name(uuid));

		return TRUE;
	} else if (self->priv->interactive) {
		g_print("Authorize a connection to: %s (yes/no)? ", uuid2name(uuid));
		gchar yn[4] = {0,};
		errno = 0;
		if (scanf("%3s", yn) == EOF && errno) {
			g_warning("%s\n", strerror(errno));
		}
		if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
			return TRUE;
		} else {
			// TODO: Fix error code
			if (error)
				*error = g_error_new(g_quark_from_static_string("org.bluez.Error.Rejected"), 0, "Connection rejected by user");
			return FALSE;
		}
	}

	return FALSE; // Request rejected
}

gboolean agent_confirm_mode_change(Agent *self, const gchar *mode, GError **error)
{
	if (self->priv->interactive) {
		g_print("Confirm mode change: %s (yes/no)? ", mode);
		gchar yn[4] = {0,};
		errno = 0;
		if (scanf("%3s", yn) == EOF && errno) {
			g_warning("%s\n", strerror(errno));
		}
		if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
			return TRUE;
		} else {
			// TODO: Fix error code
			if (error)
				*error = g_error_new(g_quark_from_static_string("org.bluez.Error.Rejected"), 0, "Confirmation rejected by user");
			return FALSE;
		}
	}

	return TRUE; // Request accepted
}

gboolean agent_cancel(Agent *self, GError **error)
{
	if (self->priv->interactive)
		g_print("Request cancelled\n");

	return TRUE;
}

