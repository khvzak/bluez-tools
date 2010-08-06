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

#ifndef __INPUT_H
#define __INPUT_H

#include <glib-object.h>

#define INPUT_DBUS_INTERFACE "org.bluez.Input"

/*
 * Type macros
 */
#define INPUT_TYPE				(input_get_type())
#define INPUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), INPUT_TYPE, Input))
#define INPUT_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), INPUT_TYPE))
#define INPUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), INPUT_TYPE, InputClass))
#define INPUT_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), INPUT_TYPE))
#define INPUT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), INPUT_TYPE, InputClass))

typedef struct _Input Input;
typedef struct _InputClass InputClass;
typedef struct _InputPrivate InputPrivate;

struct _Input {
	GObject parent_instance;

	/*< private >*/
	InputPrivate *priv;
};

struct _InputClass {
	GObjectClass parent_class;
};

/* used by INPUT_TYPE */
GType input_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void input_connect(Input *self, GError **error);
void input_disconnect(Input *self, GError **error);
GHashTable *input_get_properties(Input *self, GError **error);

const gchar *input_get_dbus_object_path(Input *self);
const gboolean input_get_connected(Input *self);

#endif /* __INPUT_H */

