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

#ifndef __AGENT_H
#define __AGENT_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "../marshallers.h"

#define AGENT_DBUS_PATH "/Agent"

/*
 * Type macros
 */
#define AGENT_TYPE					(agent_get_type())
#define AGENT(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), AGENT_TYPE, Agent))
#define AGENT_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), AGENT_TYPE))
#define AGENT_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), AGENT_TYPE, AgentClass))
#define AGENT_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), AGENT_TYPE))
#define AGENT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), AGENT_TYPE, AgentClass))

typedef struct _Agent Agent;
typedef struct _AgentClass AgentClass;
typedef struct _AgentPrivate AgentPrivate;

struct _Agent {
	GObject parent_instance;

	/*< private >*/
	AgentPrivate *priv;
};

struct _AgentClass {
	GObjectClass parent_class;
};

/* used by AGENT_TYPE */
GType agent_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
gboolean agent_release(Agent *self, GError **error);
gboolean agent_request_pin_code(Agent *self, const gchar *device, gchar **ret, GError **error);
gboolean agent_request_passkey(Agent *self, const gchar *device, guint *ret, GError **error);
gboolean agent_display_passkey(Agent *self, const gchar *device, guint passkey, guint8 entered, GError **error);
gboolean agent_request_confirmation(Agent *self, const gchar *device, guint passkey, GError **error);
gboolean agent_authorize(Agent *self, const gchar *device, const gchar *uuid, GError **error);
gboolean agent_confirm_mode_change(Agent *self, const gchar *mode, GError **error);
gboolean agent_cancel(Agent *self, GError **error);

/* Glue code */
static const DBusGMethodInfo dbus_glib_agent_methods[] = {
	{ (GCallback) agent_release, g_cclosure_bt_marshal_BOOLEAN__POINTER, 0},
	{ (GCallback) agent_request_pin_code, g_cclosure_bt_marshal_BOOLEAN__BOXED_POINTER_POINTER, 27},
	{ (GCallback) agent_request_passkey, g_cclosure_bt_marshal_BOOLEAN__BOXED_POINTER_POINTER, 85},
	{ (GCallback) agent_display_passkey, g_cclosure_bt_marshal_BOOLEAN__BOXED_UINT_UCHAR_POINTER, 143},
	{ (GCallback) agent_request_confirmation, g_cclosure_bt_marshal_BOOLEAN__BOXED_UINT_POINTER, 212},
	{ (GCallback) agent_authorize, g_cclosure_bt_marshal_BOOLEAN__BOXED_STRING_POINTER, 274},
	{ (GCallback) agent_confirm_mode_change, g_cclosure_bt_marshal_BOOLEAN__STRING_POINTER, 323},
	{ (GCallback) agent_cancel, g_cclosure_bt_marshal_BOOLEAN__POINTER, 369},
};

static const DBusGObjectInfo dbus_glib_agent_object_info = {
	0,
	dbus_glib_agent_methods,
	8,
	"org.bluez.Agent\0Release\0S\0\0org.bluez.Agent\0RequestPinCode\0S\0device\0I\0o\0arg1\0O\0F\0N\0s\0\0org.bluez.Agent\0RequestPasskey\0S\0device\0I\0o\0arg1\0O\0F\0N\0u\0\0org.bluez.Agent\0DisplayPasskey\0S\0device\0I\0o\0passkey\0I\0u\0entered\0I\0y\0\0org.bluez.Agent\0RequestConfirmation\0S\0device\0I\0o\0passkey\0I\0u\0\0org.bluez.Agent\0Authorize\0S\0device\0I\0o\0uuid\0I\0s\0\0org.bluez.Agent\0ConfirmModeChange\0S\0mode\0I\0s\0\0org.bluez.Agent\0Cancel\0S\0\0\0",
	"\0",
	"\0"
};

#endif /* __AGENT_H */

