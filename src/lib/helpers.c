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

#include <glib.h>
#include <string.h>

#include "manager.h"
#include "helpers.h"

/* UUID Name lookup table */
typedef struct {
	gchar *uuid;
	gchar *name;
} uuid_name_lookup_table_t;

static uuid_name_lookup_table_t uuid_name_lookup_table[] = {
	{"00001000-0000-1000-8000-00805F9B34FB", "ServiceDiscoveryServerService"},
	{"00001001-0000-1000-8000-00805F9B34FB", "BrowseGroupDescriptorService"},
	{"00001002-0000-1000-8000-00805F9B34FB", "PublicBrowseGroupService"},
	{"00001101-0000-1000-8000-00805F9B34FB", "SerialPortService"},
	{"00001102-0000-1000-8000-00805F9B34FB", "LANAccessUsingPPPService"},
	{"00001103-0000-1000-8000-00805F9B34FB", "DialupNetworkingService"},
	{"00001104-0000-1000-8000-00805F9B34FB", "IrMCSyncService"},
	{"00001105-0000-1000-8000-00805F9B34FB", "OBEXObjectPushService"},
	{"00001106-0000-1000-8000-00805F9B34FB", "OBEXFileTransferService"},
	{"00001107-0000-1000-8000-00805F9B34FB", "IrMCSyncCommandService"},
	{"00001108-0000-1000-8000-00805F9B34FB", "HeadsetService"},
	{"00001109-0000-1000-8000-00805F9B34FB", "CordlessTelephonyService"},
	{"0000110A-0000-1000-8000-00805F9B34FB", "AudioSourceService"},
	{"0000110B-0000-1000-8000-00805F9B34FB", "AudioSinkService"},
	{"0000110C-0000-1000-8000-00805F9B34FB", "AVRemoteControlTargetService"},
	{"0000110D-0000-1000-8000-00805F9B34FB", "AdvancedAudioDistributionService"},
	{"0000110E-0000-1000-8000-00805F9B34FB", "AVRemoteControlService"},
	{"0000110F-0000-1000-8000-00805F9B34FB", "VideoConferencingService"},
	{"00001110-0000-1000-8000-00805F9B34FB", "IntercomService"},
	{"00001111-0000-1000-8000-00805F9B34FB", "FaxService"},
	{"00001112-0000-1000-8000-00805F9B34FB", "HeadsetAudioGatewayService"},
	{"00001113-0000-1000-8000-00805F9B34FB", "WAPService"},
	{"00001114-0000-1000-8000-00805F9B34FB", "WAPClientService"},
	{"00001115-0000-1000-8000-00805F9B34FB", "PANUService"},
	{"00001116-0000-1000-8000-00805F9B34FB", "NAPService"},
	{"00001117-0000-1000-8000-00805F9B34FB", "GNService"},
	{"00001118-0000-1000-8000-00805F9B34FB", "DirectPrintingService"},
	{"00001119-0000-1000-8000-00805F9B34FB", "ReferencePrintingService"},
	{"0000111A-0000-1000-8000-00805F9B34FB", "ImagingService"},
	{"0000111B-0000-1000-8000-00805F9B34FB", "ImagingResponderService"},
	{"0000111C-0000-1000-8000-00805F9B34FB", "ImagingAutomaticArchiveService"},
	{"0000111D-0000-1000-8000-00805F9B34FB", "ImagingReferenceObjectsService"},
	{"0000111E-0000-1000-8000-00805F9B34FB", "HandsfreeService"},
	{"0000111F-0000-1000-8000-00805F9B34FB", "HandsfreeAudioGatewayService"},
	{"00001120-0000-1000-8000-00805F9B34FB", "DirectPrintingReferenceObjectsService"},
	{"00001121-0000-1000-8000-00805F9B34FB", "ReflectedUIService"},
	{"00001122-0000-1000-8000-00805F9B34FB", "BasicPringingService"},
	{"00001123-0000-1000-8000-00805F9B34FB", "PrintingStatusService"},
	{"00001124-0000-1000-8000-00805F9B34FB", "HumanInterfaceDeviceService"},
	{"00001125-0000-1000-8000-00805F9B34FB", "HardcopyCableReplacementService"},
	{"00001126-0000-1000-8000-00805F9B34FB", "HCRPrintService"},
	{"00001127-0000-1000-8000-00805F9B34FB", "HCRScanService"},
	{"00001128-0000-1000-8000-00805F9B34FB", "CommonISDNAccessService"},
	{"00001129-0000-1000-8000-00805F9B34FB", "VideoConferencingGWService"},
	{"0000112A-0000-1000-8000-00805F9B34FB", "UDIMTService"},
	{"0000112B-0000-1000-8000-00805F9B34FB", "UDITAService"},
	{"0000112C-0000-1000-8000-00805F9B34FB", "AudioVideoService"},
	{"00001200-0000-1000-8000-00805F9B34FB", "PnPInformationService"},
	{"00001201-0000-1000-8000-00805F9B34FB", "GenericNetworkingService"},
	{"00001202-0000-1000-8000-00805F9B34FB", "GenericFileTransferService"},
	{"00001203-0000-1000-8000-00805F9B34FB", "GenericAudioService"},
	{"00001203-0000-1000-8000-00805F9B34FB", "GenericAudioService"},
	{"00001204-0000-1000-8000-00805F9B34FB", "GenericTelephonyService"},
	{"00001205-0000-1000-8000-00805F9B34FB", "UPnPService"},
	{"00001206-0000-1000-8000-00805F9B34FB", "UPnPIpService"},
	{"00001300-0000-1000-8000-00805F9B34FB", "ESdpUPnPIpPanService"},
	{"00001301-0000-1000-8000-00805F9B34FB", "ESdpUPnPIpLapService"},
	{"00001302-0000-1000-8000-00805F9B34FB", "EdpUPnpIpL2CAPService"},

	// Custom:
	{"0000112F-0000-1000-8000-00805F9B34FB", "PhoneBookAccessService"},
	{"831C4071-7BC8-4A9C-A01C-15DF25A4ADBC", "ActiveSyncService"},
};

#define UUID_NAME_LOOKUP_TABLE_SIZE \
	(sizeof(uuid_name_lookup_table)/sizeof(uuid_name_lookup_table_t))

const gchar *get_uuid_name(const gchar *uuid)
{
	for (int i = 0; i < UUID_NAME_LOOKUP_TABLE_SIZE; i++) {
		if (g_ascii_strcasecmp(uuid_name_lookup_table[i].uuid, uuid) == 0)
			return uuid_name_lookup_table[i].name;
	}

	return uuid;
}

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

