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

#ifndef __OBEXCLIENT_FILE_TRANSFER_H
#define __OBEXCLIENT_FILE_TRANSFER_H

#include <glib-object.h>

#define OBEXCLIENT_FILE_TRANSFER_DBUS_INTERFACE "org.openobex.FileTransfer"

/*
 * Type macros
 */
#define OBEXCLIENT_FILE_TRANSFER_TYPE				(obexclient_file_transfer_get_type())
#define OBEXCLIENT_FILE_TRANSFER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXCLIENT_FILE_TRANSFER_TYPE, OBEXClientFileTransfer))
#define OBEXCLIENT_FILE_TRANSFER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXCLIENT_FILE_TRANSFER_TYPE))
#define OBEXCLIENT_FILE_TRANSFER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXCLIENT_FILE_TRANSFER_TYPE, OBEXClientFileTransferClass))
#define OBEXCLIENT_FILE_TRANSFER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXCLIENT_FILE_TRANSFER_TYPE))
#define OBEXCLIENT_FILE_TRANSFER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXCLIENT_FILE_TRANSFER_TYPE, OBEXClientFileTransferClass))

typedef struct _OBEXClientFileTransfer OBEXClientFileTransfer;
typedef struct _OBEXClientFileTransferClass OBEXClientFileTransferClass;
typedef struct _OBEXClientFileTransferPrivate OBEXClientFileTransferPrivate;

struct _OBEXClientFileTransfer {
	GObject parent_instance;

	/*< private >*/
	OBEXClientFileTransferPrivate *priv;
};

struct _OBEXClientFileTransferClass {
	GObjectClass parent_class;
};

/* used by OBEXCLIENT_FILE_TRANSFER_TYPE */
GType obexclient_file_transfer_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void obexclient_file_transfer_change_folder(OBEXClientFileTransfer *self, const gchar *folder, GError **error);
void obexclient_file_transfer_copy_file(OBEXClientFileTransfer *self, const gchar *sourcefile, const gchar *targetfile, GError **error);
void obexclient_file_transfer_create_folder(OBEXClientFileTransfer *self, const gchar *folder, GError **error);
void obexclient_file_transfer_delete(OBEXClientFileTransfer *self, const gchar *file, GError **error);
void obexclient_file_transfer_get_file(OBEXClientFileTransfer *self, const gchar *targetfile, const gchar *sourcefile, GError **error);
GPtrArray *obexclient_file_transfer_list_folder(OBEXClientFileTransfer *self, GError **error);
void obexclient_file_transfer_move_file(OBEXClientFileTransfer *self, const gchar *sourcefile, const gchar *targetfile, GError **error);
void obexclient_file_transfer_put_file(OBEXClientFileTransfer *self, const gchar *sourcefile, const gchar *targetfile, GError **error);

const gchar *obexclient_file_transfer_get_dbus_object_path(OBEXClientFileTransfer *self);

#endif /* __OBEXCLIENT_FILE_TRANSFER_H */

