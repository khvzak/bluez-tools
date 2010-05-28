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
#include "manager.h"

static void manager_init(Manager *self)
{
	
}

/*gboolean find_adapter(const gchar *adapter, GError **error, Adapter **adapter_obj)
{
	//g_assert(!conn);

	if (adapter == NULL) {
		return default_adapter(error, adapter_obj);
	}

	DBusGProxy *manager_obj;
	manager_obj = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, BLUEZ_DBUS_MANAGER_PATH, BLUEZ_DBUS_MANAGER_INTERFACE);

	if (!dbus_g_proxy_call(manager_obj, "FindAdapter", error, G_TYPE_STRING, adapter, G_TYPE_INVALID, DBUS_TYPE_G_PROXY, adapter_obj, G_TYPE_INVALID)) {
		return FALSE;
	}

	return TRUE;
}

gboolean default_adapter(GError **error, Adapter **adapter_obj)
{
	//g_assert(!conn);

	DBusGProxy *manager_obj;
	manager_obj = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, BLUEZ_DBUS_MANAGER_PATH, BLUEZ_DBUS_MANAGER_INTERFACE);

	if (!dbus_g_proxy_call(manager_obj, "DefaultAdapter", error, G_TYPE_INVALID, DBUS_TYPE_G_PROXY, adapter_obj, G_TYPE_INVALID)) {
		return FALSE;
	} else {
		dbus_g_proxy_set_interface(*adapter_obj, BLUEZ_DBUS_ADAPTER_INTERFACE);
	}

	return TRUE;
}*/
