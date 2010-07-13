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

#ifndef __PEER_H
#define __PEER_H

#include <glib-object.h>

/*
 * Type macros
 */
#define PEER_TYPE				(peer_get_type())
#define PEER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), PEER_TYPE, Peer))
#define PEER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), PEER_TYPE))
#define PEER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), PEER_TYPE, PeerClass))
#define PEER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), PEER_TYPE))
#define PEER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), PEER_TYPE, PeerClass))

typedef struct _Peer Peer;
typedef struct _PeerClass PeerClass;
typedef struct _PeerPrivate PeerPrivate;

struct _Peer {
	GObject parent_instance;

	/*< private >*/
	PeerPrivate *priv;
};

struct _PeerClass {
	GObjectClass parent_class;
};

/* used by PEER_TYPE */
GType peer_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
GHashTable *peer_get_properties(Peer *self, GError **error);
void peer_set_property(Peer *self, const gchar *name, const GValue *value, GError **error);

const gchar *peer_get_dbus_object_path(Peer *self);
const gboolean peer_get_enable(Peer *self);
void peer_set_enable(Peer *self, const gboolean value);
const gchar *peer_get_name(Peer *self);
void peer_set_name(Peer *self, const gchar *value);
const gchar *peer_get_uuid(Peer *self);

#endif /* __PEER_H */

