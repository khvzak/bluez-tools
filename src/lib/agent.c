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
#include <glib.h>

#include "dbus-common.h"
#include "helpers.h"
#include "device.h"
#include "agent.h"

#define AGENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), AGENT_TYPE, AgentPrivate))

struct _AgentPrivate {
	DBusGProxy *proxy;
};

G_DEFINE_TYPE(Agent, agent, G_TYPE_OBJECT);

static void agent_dispose(GObject *gobject)
{
	Agent *self = AGENT(gobject);

	dbus_g_connection_unregister_g_object(conn, gobject);

	/* Chain up to the parent class */
	G_OBJECT_CLASS(agent_parent_class)->dispose(gobject);
}

static void agent_class_init(AgentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose = agent_dispose;

	g_type_class_add_private(klass, sizeof(AgentPrivate));
}

static void agent_init(Agent *self)
{
	self->priv = AGENT_GET_PRIVATE(self);

	g_assert(conn != NULL);

	dbus_g_connection_register_g_object(conn, DBUS_AGENT_PATH, G_OBJECT(self));
}

/* Methods */

gboolean agent_release(Agent *self, GError **error)
{
	g_print("Agent released\n");
	return TRUE;
}

gboolean agent_request_pin_code(Agent *self, const gchar *device, gchar **ret, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	g_print("Device: %s (%s)\n", device_get_address(device_obj), device_get_alias(device_obj));
	g_object_unref(device_obj);

	*ret = g_new0(gchar, 17);
	g_print("Enter PIN code: ");
	scanf("%16s", *ret);
	return TRUE;
}

gboolean agent_request_passkey(Agent *self, const gchar *device, guint *ret, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	g_print("Device: %s (%s)\n", device_get_address(device_obj), device_get_alias(device_obj));
	g_object_unref(device_obj);

	g_print("Enter passkey: ");
	scanf("%u", ret);
	return TRUE;
}

gboolean agent_display_passkey(Agent *self, const gchar *device, guint passkey, guint8 entered, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	g_print("Device: %s (%s)\n", device_get_address(device_obj), device_get_alias(device_obj));
	g_object_unref(device_obj);

	g_print("Passkey: %u, entered: %u\n", passkey, entered);
	return TRUE;
}

gboolean agent_request_confirmation(Agent *self, const gchar *device, guint passkey, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	g_print("Device: %s (%s)\n", device_get_address(device_obj), device_get_alias(device_obj));
	g_object_unref(device_obj);

	gchar yn[4] = {0,};
	g_print("Confirm passkey: %u (yes/no)? ", passkey);
	scanf("%3s", yn);
	if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
		return TRUE;
	} else {
		// TODO: Fix error code
		if (error)
			*error = g_error_new(g_quark_from_static_string("org.bluez.Error.Rejected"), 0, "Passkey doesn't match");
		return FALSE;
	}

	return TRUE;
}

gboolean agent_authorize(Agent *self, const gchar *device, const gchar *uuid, GError **error)
{
	Device *device_obj = g_object_new(DEVICE_TYPE, "DBusObjectPath", device, NULL);
	g_print("Device: %s (%s)\n", device_get_address(device_obj), device_get_alias(device_obj));
	g_object_unref(device_obj);

	gchar yn[4] = {0,};
	g_print("Authorize a connection to: %s (yes/no)? ", uuid2service(uuid));
	scanf("%3s", yn);
	if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
		return TRUE;
	} else {
		// TODO: Fix error code
		if (error)
			*error = g_error_new(g_quark_from_static_string("org.bluez.Error.Rejected"), 0, "Connection rejected by user");
		return FALSE;
	}

	return TRUE;
}

gboolean agent_confirm_mode_change(Agent *self, const gchar *mode, GError **error)
{
	gchar yn[4] = {0,};
	g_print("Confirm mode change: %s (yes/no)? ", mode);
	scanf("%3s", yn);
	if (g_strcmp0(yn, "y") == 0 || g_strcmp0(yn, "yes") == 0) {
		return TRUE;
	} else {
		// TODO: Fix error code
		if (error)
			*error = g_error_new(g_quark_from_static_string("org.bluez.Error.Rejected"), 0, "Confirmation rejected by user");
		return FALSE;
	}

	return TRUE;
}

gboolean agent_cancel(Agent *self, GError **error)
{
	g_print("Cancelled\n");
	return TRUE;
}

