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

#ifndef __HELPERS_H
#define __HELPERS_H

#include <stdio.h>
#include <glib.h>

/* Bluez DBus Interfaces */
#include "adapter.h"
#include "device.h"
#include "audio.h"
#include "input.h"
#include "network.h"
#include "network_hub.h"
#include "network_peer.h"
#include "network_router.h"
#include "serial.h"

/* OBEX DBus Interfaces */
#include "obexmanager.h"
#include "obexsession.h"
#include "obexserver.h"
#include "obexserver_session.h"

enum {
	DEVICE_INTF,

	/* BlueZ Interfaces */
	AUDIO_INTF,
	INPUT_INTF,
	NETWORK_INTF,
	NETWORK_HUB_INTF,
	NETWORK_PEER_INTF,
	NETWORK_ROUTER_INTF,
	SERIAL_INTF,

	/* OBEX Interfaces */
	OBEXMANAGER_INTF,
	OBEXSESSION_INRF,
	OBEXSERVER_INTF,
	OBEXSERVER_SESSION_INTF,
};

/* Adapter helpers */
Adapter *find_adapter(const gchar *name, GError **error);

/* Device helpers */
Device *find_device(Adapter *adapter, const gchar *name, GError **error);

/* Others helpers */
#define exit_if_error(error) G_STMT_START{ \
if (error) { \
	g_printerr("%s\n", error->message); \
	exit(EXIT_FAILURE); \
}; }G_STMT_END

inline int xtoi(const gchar *str)
{
	int i = 0;
	sscanf(str, "0x%x", &i);
	return i;
}

const gchar *uuid2name(const gchar *uuid);
const gchar *name2uuid(const gchar *name);

/* Interface helpers */
gboolean intf_is_supported(const gchar *dbus_object_path, int intf_id);

#endif /* __HELPERS_H */

