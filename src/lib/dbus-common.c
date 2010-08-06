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

#include "bluez-api.h"
#include "obexd-api.h"
#include "ods-api.h"

#include "dbus-common.h"

DBusGConnection *session_conn = NULL;
DBusGConnection *system_conn = NULL;

void dbus_init()
{
	/* Marshallers registration
	 * Used for signals
	 */
	dbus_g_object_register_marshaller(g_cclosure_bt_marshal_VOID__STRING_BOXED, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_bt_marshal_VOID__INT_INT, G_TYPE_NONE, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_bt_marshal_VOID__STRING_BOOLEAN, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_bt_marshal_VOID__UINT64, G_TYPE_NONE, G_TYPE_UINT64, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_bt_marshal_VOID__STRING_STRING, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_bt_marshal_VOID__STRING_STRING_UINT64, G_TYPE_NONE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_bt_marshal_VOID__BOXED_STRING_STRING, G_TYPE_NONE, G_TYPE_VALUE, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

	/* Agents installation */
	dbus_g_object_type_install_info(AGENT_TYPE, &dbus_glib_agent_object_info);
	dbus_g_object_type_install_info(OBEXAGENT_TYPE, &dbus_glib_obexagent_object_info);
}

gboolean dbus_session_connect(GError **error)
{
	session_conn = dbus_g_bus_get(DBUS_BUS_SESSION, error);
	if (!session_conn) {
		return FALSE;
	}

	return TRUE;
}

void dbus_session_disconnect()
{
	dbus_g_connection_unref(session_conn);
}

gboolean dbus_system_connect(GError **error)
{
	system_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, error);
	if (!system_conn) {
		return FALSE;
	}

	return TRUE;
}

void dbus_system_disconnect()
{
	dbus_g_connection_unref(system_conn);
}

