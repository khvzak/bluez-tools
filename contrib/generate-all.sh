#!/bin/sh

# BlueZ API

# adapter-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/adapter-api.txt > ../src/lib/adapter.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/adapter-api.txt > ../src/lib/adapter.c

# audio-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/audio-api.txt > ../src/lib/audio.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/audio-api.txt > ../src/lib/audio.c

# device-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/device-api.txt > ../src/lib/device.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/device-api.txt > ../src/lib/device.c

# input-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/input-api.txt > ../src/lib/input.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/input-api.txt > ../src/lib/input.c

# manager-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/manager-api.txt > ../src/lib/manager.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/manager-api.txt > ../src/lib/manager.c

# network-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/network-api.txt > ../src/lib/network.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/network-api.txt > ../src/lib/network.c

./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/network-api.txt 2 > ../src/lib/network_hub.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/network-api.txt 2 > ../src/lib/network_hub.c

./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/network-api.txt 3 > ../src/lib/network_peer.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/network-api.txt 3 > ../src/lib/network_peer.c

./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/network-api.txt 4 > ../src/lib/network_router.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/network-api.txt 4 > ../src/lib/network_router.c

# serial-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/serial-api.txt > ../src/lib/serial.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/serial-api.txt > ../src/lib/serial.c

# service-api.txt
#./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/service-api.txt > ../src/lib/service.h
#./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/service-api.txt > ../src/lib/service.c

# OBEX API

# client-api.txt
./gen-dbus-gobject.pl -header obexd-api-0.29-fixed/client-api.txt > ../src/lib/obexclient.h
./gen-dbus-gobject.pl -source obexd-api-0.29-fixed/client-api.txt > ../src/lib/obexclient.c

./gen-dbus-gobject.pl -header obexd-api-0.29-fixed/client-api.txt 2 > ../src/lib/obexclient_session.h
./gen-dbus-gobject.pl -source obexd-api-0.29-fixed/client-api.txt 2 > ../src/lib/obexclient_session.c

./gen-dbus-gobject.pl -header obexd-api-0.29-fixed/client-api.txt 3 > ../src/lib/obexclient_file_transfer.h
./gen-dbus-gobject.pl -source obexd-api-0.29-fixed/client-api.txt 3 > ../src/lib/obexclient_file_transfer.c

./gen-dbus-gobject.pl -header obexd-api-0.29-fixed/client-api.txt 6 > ../src/lib/obexclient_transfer.h
./gen-dbus-gobject.pl -source obexd-api-0.29-fixed/client-api.txt 6 > ../src/lib/obexclient_transfer.c

# obexd-api.txt
./gen-dbus-gobject.pl -header obexd-api-0.29-fixed/obexd-api.txt > ../src/lib/obexmanager.h
./gen-dbus-gobject.pl -source obexd-api-0.29-fixed/obexd-api.txt > ../src/lib/obexmanager.c

./gen-dbus-gobject.pl -header obexd-api-0.29-fixed/obexd-api.txt 2 > ../src/lib/obextransfer.h
./gen-dbus-gobject.pl -source obexd-api-0.29-fixed/obexd-api.txt 2 > ../src/lib/obextransfer.c

./gen-dbus-gobject.pl -header obexd-api-0.29-fixed/obexd-api.txt 3 > ../src/lib/obexsession.h
./gen-dbus-gobject.pl -source obexd-api-0.29-fixed/obexd-api.txt 3 > ../src/lib/obexsession.c
