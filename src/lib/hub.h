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

#ifndef __HUB_H
#define __HUB_H

#include <glib-object.h>

/*
 * Type macros
 */
#define HUB_TYPE				(hub_get_type())
#define HUB(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), HUB_TYPE, Hub))
#define HUB_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), HUB_TYPE))
#define HUB_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), HUB_TYPE, HubClass))
#define HUB_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), HUB_TYPE))
#define HUB_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), HUB_TYPE, HubClass))

typedef struct _Hub Hub;
typedef struct _HubClass HubClass;
typedef struct _HubPrivate HubPrivate;

struct _Hub {
	GObject parent_instance;

	/*< private >*/
	HubPrivate *priv;
};

struct _HubClass {
	GObjectClass parent_class;
};

/* used by HUB_TYPE */
GType hub_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
GHashTable *hub_get_properties(Hub *self, GError **error);
void hub_set_property(Hub *self, const gchar *name, const GValue *value, GError **error);

const gchar *hub_get_dbus_object_path(Hub *self);
const gboolean hub_get_enable(Hub *self);
void hub_set_enable(Hub *self, const gboolean value);
const gchar *hub_get_name(Hub *self);
void hub_set_name(Hub *self, const gchar *value);
const gchar *hub_get_uuid(Hub *self);

#endif /* __HUB_H */

