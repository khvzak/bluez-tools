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

#ifndef __DBUS_COMMON_H
#define __DBUS_COMMON_H

#include <glib.h>
#include <dbus/dbus-glib.h>

#define DBUS_TYPE_G_STRING_VARIANT_HASHTABLE (dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#define DBUS_TYPE_G_UINT_STRING_HASHTABLE (dbus_g_type_get_map("GHashTable", G_TYPE_UINT, G_TYPE_STRING))
#define DBUS_TYPE_G_HASH_TABLE_ARRAY (dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_STRING_VARIANT_HASHTABLE))

extern DBusGConnection *session_conn;
extern DBusGConnection *system_conn;

void dbus_init();
gboolean dbus_session_connect(GError **error);
void dbus_session_disconnect();
gboolean dbus_system_connect(GError **error);
void dbus_system_disconnect();
void dbus_disconnect();

#endif /* __DBUS_COMMON_H */

