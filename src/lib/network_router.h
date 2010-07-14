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

#ifndef __NETWORK_ROUTER_H
#define __NETWORK_ROUTER_H

#include <glib-object.h>

#define BLUEZ_DBUS_NETWORK_ROUTER_INTERFACE "org.bluez.NetworkRouter"

/*
 * Type macros
 */
#define NETWORK_ROUTER_TYPE				(network_router_get_type())
#define NETWORK_ROUTER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), NETWORK_ROUTER_TYPE, NetworkRouter))
#define NETWORK_ROUTER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), NETWORK_ROUTER_TYPE))
#define NETWORK_ROUTER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), NETWORK_ROUTER_TYPE, NetworkRouterClass))
#define NETWORK_ROUTER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), NETWORK_ROUTER_TYPE))
#define NETWORK_ROUTER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), NETWORK_ROUTER_TYPE, NetworkRouterClass))

typedef struct _NetworkRouter NetworkRouter;
typedef struct _NetworkRouterClass NetworkRouterClass;
typedef struct _NetworkRouterPrivate NetworkRouterPrivate;

struct _NetworkRouter {
	GObject parent_instance;

	/*< private >*/
	NetworkRouterPrivate *priv;
};

struct _NetworkRouterClass {
	GObjectClass parent_class;
};

/* used by NETWORK_ROUTER_TYPE */
GType network_router_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
GHashTable *network_router_get_properties(NetworkRouter *self, GError **error);
void network_router_set_property(NetworkRouter *self, const gchar *name, const GValue *value, GError **error);

const gchar *network_router_get_dbus_object_path(NetworkRouter *self);
const gboolean network_router_get_enabled(NetworkRouter *self);
void network_router_set_enabled(NetworkRouter *self, const gboolean value);
const gchar *network_router_get_name(NetworkRouter *self);
void network_router_set_name(NetworkRouter *self, const gchar *value);
const gchar *network_router_get_uuid(NetworkRouter *self);

#endif /* __NETWORK_ROUTER_H */

