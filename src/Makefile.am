AM_CPPFLAGS = $(DBUS_CFLAGS) $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
AM_LDFLAGS = $(DBUS_LIBS) $(GLIB_LIBS) $(DBUS_GLIB_LIBS)

lib_sources = lib/dbus-common.c lib/dbus-common.h \
              lib/adapter.c lib/adapter.h \
              lib/manager.c lib/manager.h

bin_PROGRAMS = bt-monitor
bt_monitor_SOURCES = $(lib_sources) bt-monitor.c monitor.c monitor.h
