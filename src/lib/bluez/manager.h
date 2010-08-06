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

#ifndef __MANAGER_H
#define __MANAGER_H

#include <glib-object.h>

#define MANAGER_DBUS_PATH "/"
#define MANAGER_DBUS_INTERFACE "org.bluez.Manager"

/*
 * Type macros
 */
#define MANAGER_TYPE				(manager_get_type())
#define MANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), MANAGER_TYPE, Manager))
#define MANAGER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), MANAGER_TYPE))
#define MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MANAGER_TYPE, ManagerClass))
#define MANAGER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), MANAGER_TYPE))
#define MANAGER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), MANAGER_TYPE, ManagerClass))

typedef struct _Manager Manager;
typedef struct _ManagerClass ManagerClass;
typedef struct _ManagerPrivate ManagerPrivate;

struct _Manager {
	GObject parent_instance;

	/*< private >*/
	ManagerPrivate *priv;
};

struct _ManagerClass {
	GObjectClass parent_class;
};

/* used by MANAGER_TYPE */
GType manager_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
gchar *manager_default_adapter(Manager *self, GError **error);
gchar *manager_find_adapter(Manager *self, const gchar *pattern, GError **error);
GHashTable *manager_get_properties(Manager *self, GError **error);

const GPtrArray *manager_get_adapters(Manager *self);

#endif /* __MANAGER_H */

