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

#ifndef __OBEXTRANSFER_H
#define __OBEXTRANSFER_H

#include <glib-object.h>

#define OBEXTRANSFER_DBUS_INTERFACE "org.openobex.Transfer"

/*
 * Type macros
 */
#define OBEXTRANSFER_TYPE				(obextransfer_get_type())
#define OBEXTRANSFER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXTRANSFER_TYPE, OBEXTransfer))
#define OBEXTRANSFER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXTRANSFER_TYPE))
#define OBEXTRANSFER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXTRANSFER_TYPE, OBEXTransferClass))
#define OBEXTRANSFER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXTRANSFER_TYPE))
#define OBEXTRANSFER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXTRANSFER_TYPE, OBEXTransferClass))

typedef struct _OBEXTransfer OBEXTransfer;
typedef struct _OBEXTransferClass OBEXTransferClass;
typedef struct _OBEXTransferPrivate OBEXTransferPrivate;

struct _OBEXTransfer {
	GObject parent_instance;

	/*< private >*/
	OBEXTransferPrivate *priv;
};

struct _OBEXTransferClass {
	GObjectClass parent_class;
};

/* used by OBEXTRANSFER_TYPE */
GType obextransfer_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void obextransfer_cancel(OBEXTransfer *self, GError **error);

const gchar *obextransfer_get_dbus_object_path(OBEXTransfer *self);

#endif /* __OBEXTRANSFER_H */

