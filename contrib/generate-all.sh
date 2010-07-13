#!/bin/sh

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

# network-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/network-api.txt 2 > ../src/lib/hub.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/network-api.txt 2 > ../src/lib/hub.c

# network-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/network-api.txt 3 > ../src/lib/peer.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/network-api.txt 3 > ../src/lib/peer.c

# network-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/network-api.txt 4 > ../src/lib/router.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/network-api.txt 4 > ../src/lib/router.c

# serial-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66-fixed/serial-api.txt > ../src/lib/serial.h
./gen-dbus-gobject.pl -source bluez-api-4.66-fixed/serial-api.txt > ../src/lib/serial.c
