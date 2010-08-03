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

#ifndef __OBEXSERVER_H
#define __OBEXSERVER_H

#include <glib-object.h>

#define BLUEZ_DBUS_OBEXSERVER_INTERFACE "org.openobex.Server"

/*
 * Type macros
 */
#define OBEXSERVER_TYPE				(obexserver_get_type())
#define OBEXSERVER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXSERVER_TYPE, OBEXServer))
#define OBEXSERVER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXSERVER_TYPE))
#define OBEXSERVER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXSERVER_TYPE, OBEXServerClass))
#define OBEXSERVER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXSERVER_TYPE))
#define OBEXSERVER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXSERVER_TYPE, OBEXServerClass))

typedef struct _OBEXServer OBEXServer;
typedef struct _OBEXServerClass OBEXServerClass;
typedef struct _OBEXServerPrivate OBEXServerPrivate;

struct _OBEXServer {
	GObject parent_instance;

	/*< private >*/
	OBEXServerPrivate *priv;
};

struct _OBEXServerClass {
	GObjectClass parent_class;
};

/* used by OBEXSERVER_TYPE */
GType obexserver_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void obexserver_close(OBEXServer *self, GError **error);
GHashTable *obexserver_get_server_session_info(OBEXServer *self, const gchar *session_object, GError **error);
gchar **obexserver_get_server_session_list(OBEXServer *self, GError **error);
gboolean obexserver_is_started(OBEXServer *self, GError **error);
void obexserver_set_option(OBEXServer *self, const gchar *name, const GValue *value, GError **error);
void obexserver_start(OBEXServer *self, const gchar *path, const gboolean allow_write, const gboolean auto_accept, GError **error);
void obexserver_stop(OBEXServer *self, GError **error);

const gchar *obexserver_get_dbus_object_path(OBEXServer *self);

#endif /* __OBEXSERVER_H */

