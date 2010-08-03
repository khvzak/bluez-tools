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

#ifndef __OBEXSERVER_SESSION_H
#define __OBEXSERVER_SESSION_H

#include <glib-object.h>

#define BLUEZ_DBUS_OBEXSERVER_SESSION_INTERFACE "org.openobex.ServerSession"

/*
 * Type macros
 */
#define OBEXSERVER_SESSION_TYPE				(obexserver_session_get_type())
#define OBEXSERVER_SESSION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXSERVER_SESSION_TYPE, OBEXServerSession))
#define OBEXSERVER_SESSION_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXSERVER_SESSION_TYPE))
#define OBEXSERVER_SESSION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXSERVER_SESSION_TYPE, OBEXServerSessionClass))
#define OBEXSERVER_SESSION_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXSERVER_SESSION_TYPE))
#define OBEXSERVER_SESSION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXSERVER_SESSION_TYPE, OBEXServerSessionClass))

typedef struct _OBEXServerSession OBEXServerSession;
typedef struct _OBEXServerSessionClass OBEXServerSessionClass;
typedef struct _OBEXServerSessionPrivate OBEXServerSessionPrivate;

struct _OBEXServerSession {
	GObject parent_instance;

	/*< private >*/
	OBEXServerSessionPrivate *priv;
};

struct _OBEXServerSessionClass {
	GObjectClass parent_class;
};

/* used by OBEXSERVER_SESSION_TYPE */
GType obexserver_session_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void obexserver_session_accept(OBEXServerSession *self, GError **error);
void obexserver_session_cancel(OBEXServerSession *self, GError **error);
void obexserver_session_disconnect(OBEXServerSession *self, GError **error);
GHashTable *obexserver_session_get_transfer_info(OBEXServerSession *self, GError **error);
void obexserver_session_reject(OBEXServerSession *self, GError **error);

const gchar *obexserver_session_get_dbus_object_path(OBEXServerSession *self);

#endif /* __OBEXSERVER_SESSION_H */

