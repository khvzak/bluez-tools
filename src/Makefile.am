AM_CPPFLAGS = $(DBUS_CFLAGS) $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
AM_LDFLAGS = $(DBUS_LIBS) $(GLIB_LIBS) $(DBUS_GLIB_LIBS)

# Marshallers generation
BUILT_SOURCES = lib/marshallers.c lib/marshallers.h
GENMARSHAL_FLAGS = --prefix="g_cclosure_bt_marshal" --g-fatal-warnings

lib/marshallers.h: lib/marshallers.list
			glib-genmarshal $(GENMARSHAL_FLAGS) --header lib/marshallers.list > lib/marshallers.h

lib/marshallers.c: lib/marshallers.list
			glib-genmarshal $(GENMARSHAL_FLAGS) --body lib/marshallers.list > lib/marshallers.c

bluez_sources = lib/bluez/adapter.c lib/bluez/adapter.h \
		lib/bluez/agent.c lib/bluez/agent.h \
		lib/bluez/audio.c lib/bluez/audio.h \
		lib/bluez/device.c lib/bluez/device.h \
		lib/bluez/input.c lib/bluez/input.h \
		lib/bluez/manager.c lib/bluez/manager.h \
		lib/bluez/network.c lib/bluez/network.h \
		lib/bluez/network_server.c lib/bluez/network_server.h \
		lib/bluez/serial.c lib/bluez/serial.h

obexd_sources = lib/obexd/obexagent.c lib/obexd/obexagent.h \
		lib/obexd/obexclient.c lib/obexd/obexclient.h \
		lib/obexd/obexclient_file_transfer.c lib/obexd/obexclient_file_transfer.h \
		lib/obexd/obexclient_session.c lib/obexd/obexclient_session.h \
		lib/obexd/obexclient_transfer.c lib/obexd/obexclient_transfer.h \
		lib/obexd/obexmanager.c lib/obexd/obexmanager.h \
		lib/obexd/obexsession.c lib/obexd/obexsession.h \
		lib/obexd/obextransfer.c lib/obexd/obextransfer.h

lib_sources = 	lib/marshallers.c lib/marshallers.h \
		lib/dbus-common.c lib/dbus-common.h \
		lib/helpers.c lib/helpers.h \
		lib/sdp.c lib/sdp.h \
		lib/bluez-api.h lib/obexd-api.h

bin_PROGRAMS = bt-monitor bt-adapter bt-agent bt-device bt-input bt-audio bt-network bt-serial bt-obex
bt_monitor_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-monitor.c
bt_adapter_SOURCES = ${lib_sources} $(bluez_sources) $(obexd_sources) bt-adapter.c
bt_agent_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-agent.c
bt_device_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-device.c
bt_input_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-input.c
bt_audio_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-audio.c
bt_network_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-network.c
bt_serial_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-serial.c
bt_obex_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-obex.c

CLEANFILES =	Makefile.in \
		lib/marshallers.c lib/marshallers.h
