#!/bin/sh

# adapter-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66/adapter-api.txt > ../src/lib/adapter.h
./gen-dbus-gobject.pl -source bluez-api-4.66/adapter-api.txt > ../src/lib/adapter.c

# audio-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66/audio-api.txt > ../src/lib/audio.h
./gen-dbus-gobject.pl -source bluez-api-4.66/audio-api.txt > ../src/lib/audio.c

# device-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66/device-api.txt > ../src/lib/device.h
./gen-dbus-gobject.pl -source bluez-api-4.66/device-api.txt > ../src/lib/device.c

# input-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66/input-api.txt > ../src/lib/input.h
./gen-dbus-gobject.pl -source bluez-api-4.66/input-api.txt > ../src/lib/input.c

# manager-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66/manager-api.txt > ../src/lib/manager.h
./gen-dbus-gobject.pl -source bluez-api-4.66/manager-api.txt > ../src/lib/manager.c

# network-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66/network-api.txt > ../src/lib/network.h
./gen-dbus-gobject.pl -source bluez-api-4.66/network-api.txt > ../src/lib/network.c

# serial-api.txt
./gen-dbus-gobject.pl -header bluez-api-4.66/serial-api.txt > ../src/lib/serial.h
./gen-dbus-gobject.pl -source bluez-api-4.66/serial-api.txt > ../src/lib/serial.c
