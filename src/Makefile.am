AM_CPPFLAGS = $(DBUS_CFLAGS) $(GLIB_CFLAGS) $(GOBJECT_CFLAGS) $(GIO_CFLAGS) $(DBUS_GLIB_CFLAGS)
AM_LDFLAGS = $(DBUS_LIBS) $(GLIB_LIBS) $(GOBJECT_LIBS) $(GIO_LIBS) $(DBUS_GLIB_LIBS)

BUILT_SOURCES = lib/marshallers.c lib/marshallers.h

GENMARSHAL_FLAGS = --prefix="g_cclosure_bluez_marshal" --g-fatal-warnings

lib/marshallers.h: lib/marshallers.list
			glib-genmarshal $(GENMARSHAL_FLAGS) --header lib/marshallers.list > lib/marshallers.h

lib/marshallers.c: lib/marshallers.list
			glib-genmarshal $(GENMARSHAL_FLAGS) --body lib/marshallers.list > lib/marshallers.c

lib_sources = 	lib/marshallers.c lib/marshallers.h \
		lib/dbus-common.c lib/dbus-common.h \
		lib/helpers.c lib/helpers.h \
		lib/adapter.c lib/adapter.h \
		lib/audio.c lib/audio.h \
		lib/device.c lib/device.h \
		lib/input.c lib/input.h \
		lib/manager.c lib/manager.h \
		lib/network.c lib/network.h \
		lib/serial.c lib/serial.h

bin_PROGRAMS = bt-monitor
bt_monitor_SOURCES = $(lib_sources) bt-monitor.c

CLEANFILES = Makefile.in lib/marshallers.c lib/marshallers.h

