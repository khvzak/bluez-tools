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

#ifndef __ROUTER_H
#define __ROUTER_H

#include <glib-object.h>

/*
 * Type macros
 */
#define ROUTER_TYPE				(router_get_type())
#define ROUTER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ROUTER_TYPE, Router))
#define ROUTER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ROUTER_TYPE))
#define ROUTER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ROUTER_TYPE, RouterClass))
#define ROUTER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ROUTER_TYPE))
#define ROUTER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ROUTER_TYPE, RouterClass))

typedef struct _Router Router;
typedef struct _RouterClass RouterClass;
typedef struct _RouterPrivate RouterPrivate;

struct _Router {
	GObject parent_instance;

	/*< private >*/
	RouterPrivate *priv;
};

struct _RouterClass {
	GObjectClass parent_class;
};

/* used by ROUTER_TYPE */
GType router_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
GHashTable *router_get_properties(Router *self, GError **error);
void router_set_property(Router *self, const gchar *name, const GValue *value, GError **error);

const gchar *router_get_dbus_object_path(Router *self);
const gboolean router_get_enable(Router *self);
void router_set_enable(Router *self, const gboolean value);
const gchar *router_get_name(Router *self);
void router_set_name(Router *self, const gchar *value);
const gchar *router_get_uuid(Router *self);

#endif /* __ROUTER_H */

