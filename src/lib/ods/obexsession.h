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

#ifndef __OBEXSESSION_H
#define __OBEXSESSION_H

#include <glib-object.h>

#define OBEXSESSION_DBUS_INTERFACE "org.openobex.Session"

/*
 * Type macros
 */
#define OBEXSESSION_TYPE				(obexsession_get_type())
#define OBEXSESSION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXSESSION_TYPE, OBEXSession))
#define OBEXSESSION_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXSESSION_TYPE))
#define OBEXSESSION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXSESSION_TYPE, OBEXSessionClass))
#define OBEXSESSION_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXSESSION_TYPE))
#define OBEXSESSION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXSESSION_TYPE, OBEXSessionClass))

typedef struct _OBEXSession OBEXSession;
typedef struct _OBEXSessionClass OBEXSessionClass;
typedef struct _OBEXSessionPrivate OBEXSessionPrivate;

struct _OBEXSession {
	GObject parent_instance;

	/*< private >*/
	OBEXSessionPrivate *priv;
};

struct _OBEXSessionClass {
	GObjectClass parent_class;
};

/* used by OBEXSESSION_TYPE */
GType obexsession_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void obexsession_cancel(OBEXSession *self, GError **error);
void obexsession_change_current_folder(OBEXSession *self, const gchar *path, GError **error);
void obexsession_change_current_folder_backward(OBEXSession *self, GError **error);
void obexsession_change_current_folder_to_root(OBEXSession *self, GError **error);
void obexsession_close(OBEXSession *self, GError **error);
void obexsession_copy_remote_file(OBEXSession *self, const gchar *remote_filename, const gchar *local_path, GError **error);
void obexsession_copy_remote_file_by_type(OBEXSession *self, const gchar *type, const gchar *local_path, GError **error);
void obexsession_create_folder(OBEXSession *self, const gchar *folder_name, GError **error);
void obexsession_delete_remote_file(OBEXSession *self, const gchar *remote_filename, GError **error);
void obexsession_disconnect(OBEXSession *self, GError **error);
gchar *obexsession_get_capability(OBEXSession *self, GError **error);
gchar *obexsession_get_current_path(OBEXSession *self, GError **error);
GHashTable *obexsession_get_transfer_info(OBEXSession *self, GError **error);
gboolean obexsession_is_busy(OBEXSession *self, GError **error);
void obexsession_remote_copy(OBEXSession *self, const gchar *remote_source, const gchar *remote_destination, GError **error);
void obexsession_remote_move(OBEXSession *self, const gchar *remote_source, const gchar *remote_destination, GError **error);
gchar *obexsession_retrieve_folder_listing(OBEXSession *self, GError **error);
void obexsession_send_file(OBEXSession *self, const gchar *local_path, GError **error);
void obexsession_send_file_ext(OBEXSession *self, const gchar *local_path, const gchar *remote_filename, const gchar *type, GError **error);

const gchar *obexsession_get_dbus_object_path(OBEXSession *self);

#endif /* __OBEXSESSION_H */

