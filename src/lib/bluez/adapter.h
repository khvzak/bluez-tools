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

#define ADAPTER_DBUS_INTERFACE "org.bluez.Adapter"

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

/* used by ADAPTER_TYPE */
GType adapter_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void adapter_cancel_device_creation(Adapter *self, const gchar *address, GError **error);
gchar *adapter_create_device(Adapter *self, const gchar *address, GError **error);
void adapter_create_paired_device_begin(Adapter *self, void (*AsyncNotifyFunc)(gpointer data), gpointer data, const gchar *address, const gchar *agent, const gchar *capability);
gchar *adapter_create_paired_device_end(Adapter *self, GError **error);
gchar *adapter_find_device(Adapter *self, const gchar *address, GError **error);
GHashTable *adapter_get_properties(Adapter *self, GError **error);
void adapter_register_agent(Adapter *self, const gchar *agent, const gchar *capability, GError **error);
void adapter_remove_device(Adapter *self, const gchar *device, GError **error);
void adapter_set_property(Adapter *self, const gchar *name, const GValue *value, GError **error);
void adapter_start_discovery(Adapter *self, GError **error);
void adapter_stop_discovery(Adapter *self, GError **error);
void adapter_unregister_agent(Adapter *self, const gchar *agent, GError **error);

const gchar *adapter_get_dbus_object_path(Adapter *self);
const gchar *adapter_get_address(Adapter *self);
const guint32 adapter_get_class(Adapter *self);
const GPtrArray *adapter_get_devices(Adapter *self);
const gboolean adapter_get_discoverable(Adapter *self);
void adapter_set_discoverable(Adapter *self, const gboolean value);
const guint32 adapter_get_discoverable_timeout(Adapter *self);
void adapter_set_discoverable_timeout(Adapter *self, const guint32 value);
const gboolean adapter_get_discovering(Adapter *self);
const gchar *adapter_get_name(Adapter *self);
void adapter_set_name(Adapter *self, const gchar *value);
const gboolean adapter_get_pairable(Adapter *self);
void adapter_set_pairable(Adapter *self, const gboolean value);
const guint32 adapter_get_pairable_timeout(Adapter *self);
void adapter_set_pairable_timeout(Adapter *self, const guint32 value);
const gboolean adapter_get_powered(Adapter *self);
void adapter_set_powered(Adapter *self, const gboolean value);
const gchar **adapter_get_uuids(Adapter *self);

#endif /* __ADAPTER_H */

