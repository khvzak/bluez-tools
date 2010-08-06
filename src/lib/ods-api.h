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

#ifndef __ODS_API_H
#define __ODS_API_H

/* Global includes */
#include <glib.h>
#include <dbus/dbus-glib.h>

#define ODS_DBUS_NAME "org.openobex"

/* ODS DBus API */
#include "ods/obexmanager.h"
#include "ods/obexserver.h"
#include "ods/obexserver_session.h"
#include "ods/obexsession.h"

#endif /* __ODS_API_H */

