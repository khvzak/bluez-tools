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

#ifndef __OBEXAGENT_H
#define __OBEXAGENT_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "../marshallers.h"

#define OBEXAGENT_DBUS_PATH "/ObexAgent"

/*
 * Type macros
 */
#define OBEXAGENT_TYPE					(obexagent_get_type())
#define OBEXAGENT(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), OBEXAGENT_TYPE, OBEXAgent))
#define OBEXAGENT_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), OBEXAGENT_TYPE))
#define OBEXAGENT_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), OBEXAGENT_TYPE, OBEXAgentClass))
#define OBEXAGENT_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), OBEXAGENT_TYPE))
#define OBEXAGENT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), OBEXAGENT_TYPE, OBEXAgentClass))

typedef struct _OBEXAgent OBEXAgent;
typedef struct _OBEXAgentClass OBEXAgentClass;
typedef struct _OBEXAgentPrivate OBEXAgentPrivate;

struct _OBEXAgent {
	GObject parent_instance;

	/*< private >*/
	OBEXAgentPrivate *priv;
};

struct _OBEXAgentClass {
	GObjectClass parent_class;
};

/* used by OBEXAGENT_TYPE */
GType obexagent_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */

/* Server API */
gboolean obexagent_authorize(OBEXAgent *self, const gchar *transfer, const gchar *bt_address, const gchar *name, const gchar *type, gint length, gint time, gchar **ret, GError **error);
gboolean obexagent_cancel(OBEXAgent *self, GError **error);

/* Client API */
gboolean obexagent_release(OBEXAgent *self, GError **error);
gboolean obexagent_request(OBEXAgent *self, const gchar *transfer, gchar **ret, GError **error);
gboolean obexagent_progress(OBEXAgent *self, const gchar *transfer, guint64 transferred, GError **error);
gboolean obexagent_complete(OBEXAgent *self, const gchar *transfer, GError **error);
gboolean obexagent_error(OBEXAgent *self, const gchar *transfer, const gchar *message, GError **error);

/* Glue code */
static const DBusGMethodInfo dbus_glib_obexagent_methods[] = {
	{ (GCallback) obexagent_authorize, g_cclosure_bt_marshal_BOOLEAN__BOXED_STRING_STRING_STRING_INT_INT_POINTER_POINTER, 0},
	{ (GCallback) obexagent_cancel, g_cclosure_bt_marshal_BOOLEAN__POINTER, 111},
	{ (GCallback) obexagent_release, g_cclosure_bt_marshal_BOOLEAN__POINTER, 140},
	{ (GCallback) obexagent_request, g_cclosure_bt_marshal_BOOLEAN__BOXED_POINTER_POINTER, 170},
	{ (GCallback) obexagent_progress, g_cclosure_bt_marshal_BOOLEAN__BOXED_UINT64_POINTER, 226},
	{ (GCallback) obexagent_complete, g_cclosure_bt_marshal_BOOLEAN__BOXED_POINTER, 286},
	{ (GCallback) obexagent_error, g_cclosure_bt_marshal_BOOLEAN__BOXED_STRING_POINTER, 330},
};

static const DBusGObjectInfo dbus_glib_obexagent_object_info = {
	0,
	dbus_glib_obexagent_methods,
	7,
	"org.openobex.Agent\0Authorize\0S\0transfer\0I\0o\0bt_address\0I\0s\0name\0I\0s\0type\0I\0s\0length\0I\0i\0time\0I\0i\0arg6\0O\0F\0N\0s\0\0org.openobex.Agent\0Cancel\0S\0\0org.openobex.Agent\0Release\0S\0\0org.openobex.Agent\0Request\0S\0transfer\0I\0o\0arg1\0O\0F\0N\0s\0\0org.openobex.Agent\0Progress\0S\0transfer\0I\0o\0transferred\0I\0t\0\0org.openobex.Agent\0Complete\0S\0transfer\0I\0o\0\0org.openobex.Agent\0Error\0S\0transfer\0I\0o\0message\0I\0s\0\0\0",
	"\0",
	"\0"
};

#endif /* __OBEXAGENT_H */

