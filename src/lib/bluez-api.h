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

#ifndef __BLUEZ_API_H
#define __BLUEZ_API_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Global includes */
#include <glib.h>
#include <dbus/dbus-glib.h>

#define BLUEZ_DBUS_NAME "org.bluez"

/* BlueZ DBus API */
#include "bluez/adapter.h"
#include "bluez/agent.h"
#include "bluez/audio.h"
#include "bluez/device.h"
#include "bluez/input.h"
#include "bluez/manager.h"
#include "bluez/network.h"
#include "bluez/network_server.h"
#include "bluez/serial.h"

#endif /* __BLUEZ_API_H */

