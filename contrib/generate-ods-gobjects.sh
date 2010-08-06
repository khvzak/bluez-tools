#!/bin/sh

# ODS (obex-data-server) API

API_VERSION=0.4.5

./gen-dbus-gobject.pl -header ods-api-${API_VERSION}-fixed/obex-data-server-api-${API_VERSION}-fixed.txt > ../src/lib/ods/obexmanager.h
./gen-dbus-gobject.pl -source ods-api-${API_VERSION}-fixed/obex-data-server-api-${API_VERSION}-fixed.txt > ../src/lib/ods/obexmanager.c

./gen-dbus-gobject.pl -header ods-api-${API_VERSION}-fixed/obex-data-server-api-${API_VERSION}-fixed.txt 2 > ../src/lib/ods/obexserver.h
./gen-dbus-gobject.pl -source ods-api-${API_VERSION}-fixed/obex-data-server-api-${API_VERSION}-fixed.txt 2 > ../src/lib/ods/obexserver.c

./gen-dbus-gobject.pl -header ods-api-${API_VERSION}-fixed/obex-data-server-api-${API_VERSION}-fixed.txt 3 > ../src/lib/ods/obexsession.h
./gen-dbus-gobject.pl -source ods-api-${API_VERSION}-fixed/obex-data-server-api-${API_VERSION}-fixed.txt 3 > ../src/lib/ods/obexsession.c

./gen-dbus-gobject.pl -header ods-api-${API_VERSION}-fixed/obex-data-server-api-${API_VERSION}-fixed.txt 4 > ../src/lib/ods/obexserver_session.h
./gen-dbus-gobject.pl -source ods-api-${API_VERSION}-fixed/obex-data-server-api-${API_VERSION}-fixed.txt 4 > ../src/lib/ods/obexserver_session.c

