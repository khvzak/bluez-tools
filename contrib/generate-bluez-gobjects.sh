#!/bin/sh

# BlueZ API

API_VERSION=4.96

# adapter-api.txt
./gen-dbus-gobject.pl -header bluez-api-${API_VERSION}-fixed/adapter-api.txt > ../src/lib/bluez/adapter.h
./gen-dbus-gobject.pl -source bluez-api-${API_VERSION}-fixed/adapter-api.txt > ../src/lib/bluez/adapter.c

# audio-api.txt
./gen-dbus-gobject.pl -header bluez-api-${API_VERSION}-fixed/audio-api.txt > ../src/lib/bluez/audio.h
./gen-dbus-gobject.pl -source bluez-api-${API_VERSION}-fixed/audio-api.txt > ../src/lib/bluez/audio.c

# device-api.txt
./gen-dbus-gobject.pl -header bluez-api-${API_VERSION}-fixed/device-api.txt > ../src/lib/bluez/device.h
./gen-dbus-gobject.pl -source bluez-api-${API_VERSION}-fixed/device-api.txt > ../src/lib/bluez/device.c

# input-api.txt
./gen-dbus-gobject.pl -header bluez-api-${API_VERSION}-fixed/input-api.txt > ../src/lib/bluez/input.h
./gen-dbus-gobject.pl -source bluez-api-${API_VERSION}-fixed/input-api.txt > ../src/lib/bluez/input.c

# manager-api.txt
./gen-dbus-gobject.pl -header bluez-api-${API_VERSION}-fixed/manager-api.txt > ../src/lib/bluez/manager.h
./gen-dbus-gobject.pl -source bluez-api-${API_VERSION}-fixed/manager-api.txt > ../src/lib/bluez/manager.c

# network-api.txt
./gen-dbus-gobject.pl -header bluez-api-${API_VERSION}-fixed/network-api.txt > ../src/lib/bluez/network.h
./gen-dbus-gobject.pl -source bluez-api-${API_VERSION}-fixed/network-api.txt > ../src/lib/bluez/network.c

./gen-dbus-gobject.pl -header bluez-api-${API_VERSION}-fixed/network-api.txt 2 > ../src/lib/bluez/network_server.h
./gen-dbus-gobject.pl -source bluez-api-${API_VERSION}-fixed/network-api.txt 2 > ../src/lib/bluez/network_server.c

# serial-api.txt
./gen-dbus-gobject.pl -header bluez-api-${API_VERSION}-fixed/serial-api.txt > ../src/lib/bluez/serial.h
./gen-dbus-gobject.pl -source bluez-api-${API_VERSION}-fixed/serial-api.txt > ../src/lib/bluez/serial.c
