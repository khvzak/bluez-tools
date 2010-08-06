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

#ifndef __OBEXCLIENT_TRANSFER_H
#define __OBEXCLIENT_TRANSFER_H

#include <glib-object.h>

#define OBEXCLIENT_TRANSFER_DBUS_INTERFACE "org.openobex.Transfer"

/*
 * Type macros
 */
#define OBEXCLIENT_TRANSFER_TYPE				(obexclient_transfer_get_type())
#define OBEXCLIENT_TRANSFER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXCLIENT_TRANSFER_TYPE, OBEXClientTransfer))
#define OBEXCLIENT_TRANSFER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXCLIENT_TRANSFER_TYPE))
#define OBEXCLIENT_TRANSFER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXCLIENT_TRANSFER_TYPE, OBEXClientTransferClass))
#define OBEXCLIENT_TRANSFER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXCLIENT_TRANSFER_TYPE))
#define OBEXCLIENT_TRANSFER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXCLIENT_TRANSFER_TYPE, OBEXClientTransferClass))

typedef struct _OBEXClientTransfer OBEXClientTransfer;
typedef struct _OBEXClientTransferClass OBEXClientTransferClass;
typedef struct _OBEXClientTransferPrivate OBEXClientTransferPrivate;

struct _OBEXClientTransfer {
	GObject parent_instance;

	/*< private >*/
	OBEXClientTransferPrivate *priv;
};

struct _OBEXClientTransferClass {
	GObjectClass parent_class;
};

/* used by OBEXCLIENT_TRANSFER_TYPE */
GType obexclient_transfer_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void obexclient_transfer_cancel(OBEXClientTransfer *self, GError **error);
GHashTable *obexclient_transfer_get_properties(OBEXClientTransfer *self, GError **error);

const gchar *obexclient_transfer_get_dbus_object_path(OBEXClientTransfer *self);
const gchar *obexclient_transfer_get_filename(OBEXClientTransfer *self);
const gchar *obexclient_transfer_get_name(OBEXClientTransfer *self);
const guint64 obexclient_transfer_get_size(OBEXClientTransfer *self);

#endif /* __OBEXCLIENT_TRANSFER_H */

