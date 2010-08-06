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

#ifndef __SERIAL_H
#define __SERIAL_H

#include <glib-object.h>

#define SERIAL_DBUS_INTERFACE "org.bluez.Serial"

/*
 * Type macros
 */
#define SERIAL_TYPE				(serial_get_type())
#define SERIAL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), SERIAL_TYPE, Serial))
#define SERIAL_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), SERIAL_TYPE))
#define SERIAL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), SERIAL_TYPE, SerialClass))
#define SERIAL_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), SERIAL_TYPE))
#define SERIAL_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), SERIAL_TYPE, SerialClass))

typedef struct _Serial Serial;
typedef struct _SerialClass SerialClass;
typedef struct _SerialPrivate SerialPrivate;

struct _Serial {
	GObject parent_instance;

	/*< private >*/
	SerialPrivate *priv;
};

struct _SerialClass {
	GObjectClass parent_class;
};

/* used by SERIAL_TYPE */
GType serial_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
gchar *serial_connect(Serial *self, const gchar *pattern, GError **error);
void serial_disconnect(Serial *self, const gchar *device, GError **error);

const gchar *serial_get_dbus_object_path(Serial *self);

#endif /* __SERIAL_H */

