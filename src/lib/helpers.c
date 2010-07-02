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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "helpers.h"
#include "adapter.h"
#include "manager.h"

Adapter *find_adapter(const gchar *name, GError **error)
{
	gchar *adapter_path = NULL;
	Adapter *adapter = NULL;

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	// If name is null - return default adapter
	if (name == NULL || strlen(name) == 0) {
		adapter_path = manager_default_adapter(manager, error);
		if (adapter_path) {
			adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
		}
	} else {
		// Try to find by id
		adapter_path = manager_find_adapter(manager, name, error);

		// Found
		if (adapter_path) {
			adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
		} else {
			// Try to find by name
			const GPtrArray *adapters_list = manager_get_adapters(manager);
			g_assert(adapters_list != NULL);
			for (int i = 0; i < adapters_list->len; i++) {
				adapter_path = g_ptr_array_index(adapters_list, i);
				adapter = g_object_new(ADAPTER_TYPE, "DBusObjectPath", adapter_path, NULL);
				adapter_path = NULL;

				if (g_strcmp0(name, adapter_get_name(adapter)) == 0) {
					if (error) {
						g_error_free(*error);
						*error = NULL;
					}
					break;
				}

				g_object_unref(adapter);
				adapter = NULL;
			}
		}
	}

	g_object_unref(manager);
	if (adapter_path) g_free(adapter_path);

	return adapter;
}

const gchar *uuid2service(const gchar *uuid)
{
	static GHashTable *t = NULL;
	if (t == NULL) {
		t = g_hash_table_new(g_str_hash, g_str_equal);
		g_hash_table_insert(t, "00001000-0000-1000-8000-00805f9b34fb", "ServiceDiscoveryServer");
		g_hash_table_insert(t, "00001001-0000-1000-8000-00805f9b34fb", "BrowseGroupDescriptor");
		g_hash_table_insert(t, "00001002-0000-1000-8000-00805f9b34fb", "PublicBrowseGroup");
		g_hash_table_insert(t, "00001101-0000-1000-8000-00805f9b34fb", "SerialPort");
		g_hash_table_insert(t, "00001102-0000-1000-8000-00805f9b34fb", "LANAccessUsingPPP");
		g_hash_table_insert(t, "00001103-0000-1000-8000-00805f9b34fb", "DialupNetworking");
		g_hash_table_insert(t, "00001104-0000-1000-8000-00805f9b34fb", "IrMCSync");
		g_hash_table_insert(t, "00001105-0000-1000-8000-00805f9b34fb", "OBEXObjectPush");
		g_hash_table_insert(t, "00001106-0000-1000-8000-00805f9b34fb", "OBEXFileTransfer");
		g_hash_table_insert(t, "00001107-0000-1000-8000-00805f9b34fb", "IrMCSyncCommand");
		g_hash_table_insert(t, "00001108-0000-1000-8000-00805f9b34fb", "Headset");
		g_hash_table_insert(t, "00001109-0000-1000-8000-00805f9b34fb", "CordlessTelephony");
		g_hash_table_insert(t, "0000110a-0000-1000-8000-00805f9b34fb", "AudioSource");
		g_hash_table_insert(t, "0000110b-0000-1000-8000-00805f9b34fb", "AudioSink");
		g_hash_table_insert(t, "0000110c-0000-1000-8000-00805f9b34fb", "AVRemoteControlTarget");
		g_hash_table_insert(t, "0000110d-0000-1000-8000-00805f9b34fb", "AdvancedAudioDistribution");
		g_hash_table_insert(t, "0000110e-0000-1000-8000-00805f9b34fb", "AVRemoteControl");
		g_hash_table_insert(t, "0000110f-0000-1000-8000-00805f9b34fb", "VideoConferencing");
		g_hash_table_insert(t, "00001110-0000-1000-8000-00805f9b34fb", "Intercom");
		g_hash_table_insert(t, "00001111-0000-1000-8000-00805f9b34fb", "Fax");
		g_hash_table_insert(t, "00001112-0000-1000-8000-00805f9b34fb", "HeadsetAudioGateway");
		g_hash_table_insert(t, "00001113-0000-1000-8000-00805f9b34fb", "WAP");
		g_hash_table_insert(t, "00001114-0000-1000-8000-00805f9b34fb", "WAPClient");
		g_hash_table_insert(t, "00001115-0000-1000-8000-00805f9b34fb", "PANU");
		g_hash_table_insert(t, "00001116-0000-1000-8000-00805f9b34fb", "NAP");
		g_hash_table_insert(t, "00001117-0000-1000-8000-00805f9b34fb", "GN");
		g_hash_table_insert(t, "00001118-0000-1000-8000-00805f9b34fb", "DirectPrinting");
		g_hash_table_insert(t, "00001119-0000-1000-8000-00805f9b34fb", "ReferencePrinting");
		g_hash_table_insert(t, "0000111a-0000-1000-8000-00805f9b34fb", "Imaging");
		g_hash_table_insert(t, "0000111b-0000-1000-8000-00805f9b34fb", "ImagingResponder");
		g_hash_table_insert(t, "0000111c-0000-1000-8000-00805f9b34fb", "ImagingAutomaticArchive");
		g_hash_table_insert(t, "0000111d-0000-1000-8000-00805f9b34fb", "ImagingReferenceObjects");
		g_hash_table_insert(t, "0000111e-0000-1000-8000-00805f9b34fb", "Handsfree");
		g_hash_table_insert(t, "0000111f-0000-1000-8000-00805f9b34fb", "HandsfreeAudioGateway");
		g_hash_table_insert(t, "00001120-0000-1000-8000-00805f9b34fb", "DirectPrintingReferenceObjects");
		g_hash_table_insert(t, "00001121-0000-1000-8000-00805f9b34fb", "ReflectedUI");
		g_hash_table_insert(t, "00001122-0000-1000-8000-00805f9b34fb", "BasicPringing");
		g_hash_table_insert(t, "00001123-0000-1000-8000-00805f9b34fb", "PrintingStatus");
		g_hash_table_insert(t, "00001124-0000-1000-8000-00805f9b34fb", "HumanInterfaceDevice");
		g_hash_table_insert(t, "00001125-0000-1000-8000-00805f9b34fb", "HardcopyCableReplacement");
		g_hash_table_insert(t, "00001126-0000-1000-8000-00805f9b34fb", "HCRPrint");
		g_hash_table_insert(t, "00001127-0000-1000-8000-00805f9b34fb", "HCRScan");
		g_hash_table_insert(t, "00001128-0000-1000-8000-00805f9b34fb", "CommonISDNAccess");
		g_hash_table_insert(t, "00001129-0000-1000-8000-00805f9b34fb", "VideoConferencingGW");
		g_hash_table_insert(t, "0000112a-0000-1000-8000-00805f9b34fb", "UDIMT");
		g_hash_table_insert(t, "0000112b-0000-1000-8000-00805f9b34fb", "UDITA");
		g_hash_table_insert(t, "0000112c-0000-1000-8000-00805f9b34fb", "AudioVideo");
		g_hash_table_insert(t, "0000112d-0000-1000-8000-00805f9b34fb", "SIMAccess");
		g_hash_table_insert(t, "00001200-0000-1000-8000-00805f9b34fb", "PnPInformation");
		g_hash_table_insert(t, "00001201-0000-1000-8000-00805f9b34fb", "GenericNetworking");
		g_hash_table_insert(t, "00001202-0000-1000-8000-00805f9b34fb", "GenericFileTransfer");
		g_hash_table_insert(t, "00001203-0000-1000-8000-00805f9b34fb", "GenericAudio");
		g_hash_table_insert(t, "00001204-0000-1000-8000-00805f9b34fb", "GenericTelephony");
	}

	if (g_hash_table_lookup(t, uuid) != NULL) {
		return g_hash_table_lookup(t, uuid);
	} else {
		return uuid;
	}
}

Device *find_device(Adapter *adapter, const gchar *name, GError **error)
{
	g_assert(adapter != NULL);
	g_assert(ADAPTER_IS(adapter));

	g_assert(name);
	g_assert(strlen(name) > 0);

	gchar *device_path = NULL;
	Device *device = NULL;

	// Try to find by MAC
	device_path = adapter_find_device(adapter, name, error);

	// Found
	if (device_path) {
		device = g_object_new(DEVICE_TYPE, "DBusObjectPath", device_path, NULL);
	} else {
		// Try to find by name
		const GPtrArray *devices_list = adapter_get_devices(adapter);
		g_assert(devices_list != NULL);
		for (int i = 0; i < devices_list->len; i++) {
			device_path = g_ptr_array_index(devices_list, i);
			device = g_object_new(DEVICE_TYPE, "DBusObjectPath", device_path, NULL);
			device_path = NULL;

			if (g_strcmp0(name, device_get_name(device)) == 0 || g_strcmp0(name, device_get_alias(device)) == 0) {
				if (error) {
					g_error_free(*error);
					*error = NULL;
				}
				break;
			}

			g_object_unref(device);
			device = NULL;
		}
	}

	if (device_path) g_free(device_path);

	return device;
}

