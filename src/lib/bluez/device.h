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

#ifndef __DEVICE_H
#define __DEVICE_H

#include <glib-object.h>

#define DEVICE_DBUS_INTERFACE "org.bluez.Device"

/*
 * Type macros
 */
#define DEVICE_TYPE				(device_get_type())
#define DEVICE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), DEVICE_TYPE, Device))
#define DEVICE_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), DEVICE_TYPE))
#define DEVICE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), DEVICE_TYPE, DeviceClass))
#define DEVICE_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), DEVICE_TYPE))
#define DEVICE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), DEVICE_TYPE, DeviceClass))

typedef struct _Device Device;
typedef struct _DeviceClass DeviceClass;
typedef struct _DevicePrivate DevicePrivate;

struct _Device {
	GObject parent_instance;

	/*< private >*/
	DevicePrivate *priv;
};

struct _DeviceClass {
	GObjectClass parent_class;
};

/* used by DEVICE_TYPE */
GType device_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void device_cancel_discovery(Device *self, GError **error);
void device_disconnect(Device *self, GError **error);
GHashTable *device_discover_services(Device *self, const gchar *pattern, GError **error);
GHashTable *device_get_properties(Device *self, GError **error);
void device_set_property(Device *self, const gchar *name, const GValue *value, GError **error);

const gchar *device_get_dbus_object_path(Device *self);
const gchar *device_get_adapter(Device *self);
const gchar *device_get_address(Device *self);
const gchar *device_get_alias(Device *self);
void device_set_alias(Device *self, const gchar *value);
const gboolean device_get_blocked(Device *self);
void device_set_blocked(Device *self, const gboolean value);
const guint32 device_get_class(Device *self);
const gboolean device_get_connected(Device *self);
const gchar *device_get_icon(Device *self);
const gboolean device_get_legacy_pairing(Device *self);
const gchar *device_get_name(Device *self);
const gboolean device_get_paired(Device *self);
const GPtrArray *device_get_services(Device *self);
const gboolean device_get_trusted(Device *self);
void device_set_trusted(Device *self, const gboolean value);
const gchar **device_get_uuids(Device *self);

#endif /* __DEVICE_H */

