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

#ifndef __OBEXCLIENT_H
#define __OBEXCLIENT_H

#include <glib-object.h>

#define OBEXCLIENT_DBUS_PATH "/"
#define OBEXCLIENT_DBUS_INTERFACE "org.openobex.Client"

/*
 * Type macros
 */
#define OBEXCLIENT_TYPE				(obexclient_get_type())
#define OBEXCLIENT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXCLIENT_TYPE, OBEXClient))
#define OBEXCLIENT_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXCLIENT_TYPE))
#define OBEXCLIENT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), OBEXCLIENT_TYPE, OBEXClientClass))
#define OBEXCLIENT_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXCLIENT_TYPE))
#define OBEXCLIENT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXCLIENT_TYPE, OBEXClientClass))

typedef struct _OBEXClient OBEXClient;
typedef struct _OBEXClientClass OBEXClientClass;
typedef struct _OBEXClientPrivate OBEXClientPrivate;

struct _OBEXClient {
	GObject parent_instance;

	/*< private >*/
	OBEXClientPrivate *priv;
};

struct _OBEXClientClass {
	GObjectClass parent_class;
};

/* used by OBEXCLIENT_TYPE */
GType obexclient_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
gchar *obexclient_create_session(OBEXClient *self, const GHashTable *device, GError **error);
void obexclient_exchange_business_cards(OBEXClient *self, const GHashTable *device, const gchar *clientfile, const gchar *file, GError **error);
gchar *obexclient_get_capabilities(OBEXClient *self, const GHashTable *device, GError **error);
void obexclient_pull_business_card(OBEXClient *self, const GHashTable *device, const gchar *file, GError **error);
void obexclient_remove_session(OBEXClient *self, const gchar *session, GError **error);
void obexclient_send_files(OBEXClient *self, const GHashTable *device, const gchar **files, const gchar *agent, GError **error);

#endif /* __OBEXCLIENT_H */

