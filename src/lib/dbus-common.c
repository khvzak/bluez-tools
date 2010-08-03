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

#include "marshallers.h"
#include "agent.h"
#include "dbus-common.h"

DBusGConnection *conn = NULL;

gboolean dbus_connect(GError **error)
{
	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, error);
	if (!conn) {
		return FALSE;
	}

	/* Marshallers registration
	 * Used for signals
	 */
	dbus_g_object_register_marshaller(g_cclosure_bluez_marshal_VOID__STRING_BOXED, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_bluez_marshal_VOID__STRING_BOOLEAN, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_bluez_marshal_VOID__INT_INT, G_TYPE_NONE, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);

	/* Agent installation */
	dbus_g_object_type_install_info(AGENT_TYPE, &dbus_glib_agent_object_info);

	return TRUE;
}

void dbus_disconnect()
{
	dbus_g_connection_unref(conn);
}
