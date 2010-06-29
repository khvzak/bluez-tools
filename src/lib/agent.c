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
#include "agent.h"

#define BLUEZ_DBUS_AGENT_INTERFACE "org.bluez.Agent"

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

	dbus_g_connection_register_g_object(conn, "/Agent", G_OBJECT(self));
}

/* Methods */

gboolean agent_release(Agent *self, GError **error)
{
	g_print("agent_release\n");
	return TRUE;
}

gboolean agent_request_pin_code(Agent *self, const gchar *device, gchar **ret, GError **error)
{
	g_print("agent_request_pin_code\n");
	return TRUE;
}

gboolean agent_request_passkey(Agent *self, const gchar *device, guint *ret, GError **error)
{
	g_print("agent_request_passkey\n");
	return TRUE;
}

gboolean agent_display_passkey(Agent *self, const gchar *device, guint passkey, guint8 entered, GError **error)
{
	g_print("agent_display_passkey\n");
	return TRUE;
}

gboolean agent_request_confirmation(Agent *self, const gchar *device, guint passkey, GError **error)
{
	g_print("agent_request_confirmation\n");
	return TRUE;
}

gboolean agent_authorize(Agent *self, const gchar *device, const gchar *uuid, GError **error)
{
	g_print("agent_authorize\n");
	return TRUE;
}

gboolean agent_confirm_mode_change(Agent *self, const gchar *mode, GError **error)
{
	g_print("agent_confirm_mode_change\n");
	return TRUE;
}

gboolean agent_cancel(Agent *self, GError **error)
{
	g_print("agent_cancel\n");
	return TRUE;
}

