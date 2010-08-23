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

#ifndef __OBEXD_API_H
#define __OBEXD_API_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Global includes */
#include <glib.h>
#include <dbus/dbus-glib.h>

#ifdef OBEX_SUPPORT
#define OBEXS_DBUS_NAME "org.openobex"
#define OBEXC_DBUS_NAME "org.openobex.client"

/* OBEXD DBus API */
#include "obexd/obexagent.h"
#include "obexd/obexclient.h"
#include "obexd/obexclient_file_transfer.h"
#include "obexd/obexclient_session.h"
#include "obexd/obexclient_transfer.h"
#include "obexd/obexmanager.h"
#include "obexd/obexsession.h"
#include "obexd/obextransfer.h"
#endif

#endif /* __OBEXD_API_H */

