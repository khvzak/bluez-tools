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

#ifndef __OBEXCLIENT_SESSION_H
#define __OBEXCLIENT_SESSION_H

#include <glib-object.h>

#define OBEXCLIENT_SESSION_DBUS_INTERFACE "org.openobex.Session"

/*
 * Type macros
 */
#define OBEXCLIENT_SESSION_TYPE				(obexclient_session_get_type())
#define OBEXCLIENT_SESSION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXCLIENT_SESSION_TYPE, OBEXClientSession))
#define OBEXCLIENT_SESSION_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXCLIENT_SESSION_TYPE))
#define OBEXCLIENT_SESSION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXCLIENT_SESSION_TYPE, OBEXClientSessionClass))
#define OBEXCLIENT_SESSION_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXCLIENT_SESSION_TYPE))
#define OBEXCLIENT_SESSION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXCLIENT_SESSION_TYPE, OBEXClientSessionClass))

typedef struct _OBEXClientSession OBEXClientSession;
typedef struct _OBEXClientSessionClass OBEXClientSessionClass;
typedef struct _OBEXClientSessionPrivate OBEXClientSessionPrivate;

struct _OBEXClientSession {
	GObject parent_instance;

	/*< private >*/
	OBEXClientSessionPrivate *priv;
};

struct _OBEXClientSessionClass {
	GObjectClass parent_class;
};

/* used by OBEXCLIENT_SESSION_TYPE */
GType obexclient_session_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void obexclient_session_assign_agent(OBEXClientSession *self, const gchar *agent, GError **error);
GHashTable *obexclient_session_get_properties(OBEXClientSession *self, GError **error);
void obexclient_session_release_agent(OBEXClientSession *self, const gchar *agent, GError **error);

const gchar *obexclient_session_get_dbus_object_path(OBEXClientSession *self);
const guchar obexclient_session_get_channel(OBEXClientSession *self);
const gchar *obexclient_session_get_destination(OBEXClientSession *self);
const gchar *obexclient_session_get_source(OBEXClientSession *self);

#endif /* __OBEXCLIENT_SESSION_H */

