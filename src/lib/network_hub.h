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

#ifndef __NETWORK_HUB_H
#define __NETWORK_HUB_H

#include <glib-object.h>

#define BLUEZ_DBUS_NETWORK_HUB_INTERFACE "org.bluez.NetworkHub"

/*
 * Type macros
 */
#define NETWORK_HUB_TYPE				(network_hub_get_type())
#define NETWORK_HUB(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), NETWORK_HUB_TYPE, NetworkHub))
#define NETWORK_HUB_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), NETWORK_HUB_TYPE))
#define NETWORK_HUB_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), NETWORK_HUB_TYPE, NetworkHubClass))
#define NETWORK_HUB_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), NETWORK_HUB_TYPE))
#define NETWORK_HUB_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), NETWORK_HUB_TYPE, NetworkHubClass))

typedef struct _NetworkHub NetworkHub;
typedef struct _NetworkHubClass NetworkHubClass;
typedef struct _NetworkHubPrivate NetworkHubPrivate;

struct _NetworkHub {
	GObject parent_instance;

	/*< private >*/
	NetworkHubPrivate *priv;
};

struct _NetworkHubClass {
	GObjectClass parent_class;
};

/* used by NETWORK_HUB_TYPE */
GType network_hub_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
GHashTable *network_hub_get_properties(NetworkHub *self, GError **error);
void network_hub_set_property(NetworkHub *self, const gchar *name, const GValue *value, GError **error);

const gchar *network_hub_get_dbus_object_path(NetworkHub *self);
const gboolean network_hub_get_enabled(NetworkHub *self);
void network_hub_set_enabled(NetworkHub *self, const gboolean value);
const gchar *network_hub_get_name(NetworkHub *self);
void network_hub_set_name(NetworkHub *self, const gchar *value);
const gchar *network_hub_get_uuid(NetworkHub *self);

#endif /* __NETWORK_HUB_H */

