#!/bin/sh

# OBEXD API

API_VERSION=0.42

# client-api.txt
./gen-dbus-gobject.pl -header obexd-api-${API_VERSION}-fixed/client-api.txt > ../src/lib/obexd/obexclient.h
./gen-dbus-gobject.pl -source obexd-api-${API_VERSION}-fixed/client-api.txt > ../src/lib/obexd/obexclient.c

./gen-dbus-gobject.pl -header obexd-api-${API_VERSION}-fixed/client-api.txt 2 > ../src/lib/obexd/obexclient_session.h
./gen-dbus-gobject.pl -source obexd-api-${API_VERSION}-fixed/client-api.txt 2 > ../src/lib/obexd/obexclient_session.c

./gen-dbus-gobject.pl -header obexd-api-${API_VERSION}-fixed/client-api.txt 3 > ../src/lib/obexd/obexclient_file_transfer.h
./gen-dbus-gobject.pl -source obexd-api-${API_VERSION}-fixed/client-api.txt 3 > ../src/lib/obexd/obexclient_file_transfer.c

./gen-dbus-gobject.pl -header obexd-api-${API_VERSION}-fixed/client-api.txt 6 > ../src/lib/obexd/obexclient_transfer.h
./gen-dbus-gobject.pl -source obexd-api-${API_VERSION}-fixed/client-api.txt 6 > ../src/lib/obexd/obexclient_transfer.c

# obexd-api.txt
./gen-dbus-gobject.pl -header obexd-api-${API_VERSION}-fixed/obexd-api.txt > ../src/lib/obexd/obexmanager.h
./gen-dbus-gobject.pl -source obexd-api-${API_VERSION}-fixed/obexd-api.txt > ../src/lib/obexd/obexmanager.c

./gen-dbus-gobject.pl -header obexd-api-${API_VERSION}-fixed/obexd-api.txt 2 > ../src/lib/obexd/obextransfer.h
./gen-dbus-gobject.pl -source obexd-api-${API_VERSION}-fixed/obexd-api.txt 2 > ../src/lib/obexd/obextransfer.c

./gen-dbus-gobject.pl -header obexd-api-${API_VERSION}-fixed/obexd-api.txt 3 > ../src/lib/obexd/obexsession.h
./gen-dbus-gobject.pl -source obexd-api-${API_VERSION}-fixed/obexd-api.txt 3 > ../src/lib/obexd/obexsession.c
