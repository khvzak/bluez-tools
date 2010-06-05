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

#ifndef __ADAPTER_H
#define __ADAPTER_H

#include <glib-object.h>

/*
 * Type macros
 */
#define ADAPTER_TYPE				(adapter_get_type())
#define ADAPTER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ADAPTER_TYPE, Adapter))
#define ADAPTER_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ADAPTER_TYPE))
#define ADAPTER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ADAPTER_TYPE, AdapterClass))
#define ADAPTER_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ADAPTER_TYPE))
#define ADAPTER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ADAPTER_TYPE, AdapterClass))

typedef struct _Adapter Adapter;
typedef struct _AdapterClass AdapterClass;
typedef struct _AdapterPrivate AdapterPrivate;

struct _Adapter {
	GObject parent_instance;

	/*< private >*/
	AdapterPrivate *priv;
};

struct _AdapterClass {
	GObjectClass parent_class;
};

/*
 * Method definitions
 */
gboolean adapter_get_properties(Adapter *self, GError **error, GHashTable **properties);
gboolean adapter_request_session(Adapter *self, GError **error);
gboolean adapter_release_session(Adapter *self, GError **error);
gboolean adapter_start_discovery(Adapter *self, GError **error);
gboolean adapter_stop_discovery(Adapter *self, GError **error);
gboolean adapter_find_device(Adapter *self, GError **error, const gchar *device_address, gchar **device_path);
gboolean adapter_create_device(Adapter *self, GError **error, const gchar *device_address, gchar **device_path);
gboolean adapter_create_paired_device(Adapter *self, GError **error, const gchar *device_address, const gchar *agent_path, const gchar *agent_capability, gchar **device_path);
gboolean adapter_cancel_device_creation(Adapter *self, GError **error, const gchar *device_address);
gboolean adapter_remove_device(Adapter *self, GError **error, const gchar *device_path);
gboolean adapter_register_agent(Adapter *self, GError **error, const gchar *agent_path, const gchar *agent_capability);
gboolean adapter_unregister_agent(Adapter *self, GError **error, const gchar *agent_path);

#endif /* __ADAPTER_H */
