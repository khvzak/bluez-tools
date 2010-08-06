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

#ifndef __OBEXMANAGER_H
#define __OBEXMANAGER_H

#include <glib-object.h>

#define OBEXMANAGER_DBUS_PATH "/"
#define OBEXMANAGER_DBUS_INTERFACE "org.openobex.Manager"

/*
 * Type macros
 */
#define OBEXMANAGER_TYPE				(obexmanager_get_type())
#define OBEXMANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXMANAGER_TYPE, OBEXManager))
#define OBEXMANAGER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXMANAGER_TYPE))
#define OBEXMANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXMANAGER_TYPE, OBEXManagerClass))
#define OBEXMANAGER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXMANAGER_TYPE))
#define OBEXMANAGER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXMANAGER_TYPE, OBEXManagerClass))

typedef struct _OBEXManager OBEXManager;
typedef struct _OBEXManagerClass OBEXManagerClass;
typedef struct _OBEXManagerPrivate OBEXManagerPrivate;

struct _OBEXManager {
	GObject parent_instance;

	/*< private >*/
	OBEXManagerPrivate *priv;
};

struct _OBEXManagerClass {
	GObjectClass parent_class;
};

/* used by OBEXMANAGER_TYPE */
GType obexmanager_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void obexmanager_register_agent(OBEXManager *self, const gchar *agent, GError **error);
void obexmanager_unregister_agent(OBEXManager *self, const gchar *agent, GError **error);

#endif /* __OBEXMANAGER_H */

