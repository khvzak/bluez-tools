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

#ifndef __NETWORK_PEER_H
#define __NETWORK_PEER_H

#include <glib-object.h>

#define BLUEZ_DBUS_NETWORK_PEER_INTERFACE "org.bluez.NetworkPeer"

/*
 * Type macros
 */
#define NETWORK_PEER_TYPE				(network_peer_get_type())
#define NETWORK_PEER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), NETWORK_PEER_TYPE, NetworkPeer))
#define NETWORK_PEER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), NETWORK_PEER_TYPE))
#define NETWORK_PEER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), NETWORK_PEER_TYPE, NetworkPeerClass))
#define NETWORK_PEER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), NETWORK_PEER_TYPE))
#define NETWORK_PEER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), NETWORK_PEER_TYPE, NetworkPeerClass))

typedef struct _NetworkPeer NetworkPeer;
typedef struct _NetworkPeerClass NetworkPeerClass;
typedef struct _NetworkPeerPrivate NetworkPeerPrivate;

struct _NetworkPeer {
	GObject parent_instance;

	/*< private >*/
	NetworkPeerPrivate *priv;
};

struct _NetworkPeerClass {
	GObjectClass parent_class;
};

/* used by NETWORK_PEER_TYPE */
GType network_peer_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
GHashTable *network_peer_get_properties(NetworkPeer *self, GError **error);
void network_peer_set_property(NetworkPeer *self, const gchar *name, const GValue *value, GError **error);

const gchar *network_peer_get_dbus_object_path(NetworkPeer *self);
const gboolean network_peer_get_enabled(NetworkPeer *self);
void network_peer_set_enabled(NetworkPeer *self, const gboolean value);
const gchar *network_peer_get_name(NetworkPeer *self);
void network_peer_set_name(NetworkPeer *self, const gchar *value);
const gchar *network_peer_get_uuid(NetworkPeer *self);

#endif /* __NETWORK_PEER_H */

