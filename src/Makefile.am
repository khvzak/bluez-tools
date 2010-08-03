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
		lib/bluez-dbus.h \
		lib/adapter.c lib/adapter.h \
		lib/agent.c lib/agent.h \
		lib/audio.c lib/audio.h \
		lib/device.c lib/device.h \
		lib/input.c lib/input.h \
		lib/manager.c lib/manager.h \
		lib/network.c lib/network.h \
		lib/network_hub.c lib/network_hub.h \
		lib/network_peer.c lib/network_peer.h \
		lib/network_router.c lib/network_router.h \
		lib/serial.c lib/serial.h \
		lib/sdp.c lib/sdp.h

obex_sources = 	lib/obexmanager.c lib/obexmanager.h \
		lib/obexsession.c lib/obexsession.h \
		lib/obexserver.c lib/obexserver.h \
		lib/obexserver_session.c lib/obexserver_session.h

bin_PROGRAMS = bt-monitor bt-adapter bt-agent bt-device bt-input bt-audio bt-network bt-serial bt-obex
bt_monitor_SOURCES = $(lib_sources) bt-monitor.c
bt_adapter_SOURCES = ${lib_sources} bt-adapter.c
bt_agent_SOURCES = $(lib_sources) bt-agent.c
bt_device_SOURCES = $(lib_sources) bt-device.c
bt_input_SOURCES = $(lib_sources) bt-input.c
bt_audio_SOURCES = $(lib_sources) bt-audio.c
bt_network_SOURCES = $(lib_sources) bt-network.c
bt_serial_SOURCES = $(lib_sources) bt-serial.c
bt_obex_SOURCES = $(lib_sources) $(obex_sources) bt-obex.c

CLEANFILES = Makefile.in lib/marshallers.c lib/marshallers.h

