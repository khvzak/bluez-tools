AM_CPPFLAGS = $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
AM_LDFLAGS = $(GLIB_LIBS) $(DBUS_GLIB_LIBS)

# Marshallers generation
#BUILT_SOURCES = lib/marshallers.c lib/marshallers.h
#GENMARSHAL_FLAGS = --prefix="g_cclosure_bt_marshal" --g-fatal-warnings
#
#lib/marshallers.h: lib/marshallers.list
#			glib-genmarshal $(GENMARSHAL_FLAGS) --header lib/marshallers.list > lib/marshallers.h
#
#lib/marshallers.c: lib/marshallers.list
#			glib-genmarshal $(GENMARSHAL_FLAGS) --body lib/marshallers.list > lib/marshallers.c

bluez_sources = lib/bluez/adapter.c lib/bluez/adapter.h \
		lib/bluez/agent.c lib/bluez/agent.h \
		lib/bluez/audio.c lib/bluez/audio.h \
		lib/bluez/device.c lib/bluez/device.h \
		lib/bluez/input.c lib/bluez/input.h \
		lib/bluez/manager.c lib/bluez/manager.h \
		lib/bluez/network.c lib/bluez/network.h \
		lib/bluez/network_server.c lib/bluez/network_server.h \
		lib/bluez/serial.c lib/bluez/serial.h

if OBEX
obexd_sources = lib/obexd/obexagent.c lib/obexd/obexagent.h \
		lib/obexd/obexclient.c lib/obexd/obexclient.h \
		lib/obexd/obexclient_file_transfer.c lib/obexd/obexclient_file_transfer.h \
		lib/obexd/obexclient_session.c lib/obexd/obexclient_session.h \
		lib/obexd/obexclient_transfer.c lib/obexd/obexclient_transfer.h \
		lib/obexd/obexmanager.c lib/obexd/obexmanager.h \
		lib/obexd/obexsession.c lib/obexd/obexsession.h \
		lib/obexd/obextransfer.c lib/obexd/obextransfer.h
else
obexd_sources =
endif

lib_sources = 	lib/marshallers.c lib/marshallers.h \
		lib/dbus-common.c lib/dbus-common.h \
		lib/helpers.c lib/helpers.h \
		lib/sdp.c lib/sdp.h \
		lib/bluez-api.h

if OBEX
lib_sources += lib/obexd-api.h
endif

bin_PROGRAMS = bt-monitor bt-adapter bt-agent bt-device bt-input bt-audio bt-network bt-serial
bt_monitor_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-monitor.c
bt_adapter_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-adapter.c
bt_agent_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-agent.c
bt_device_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-device.c
bt_input_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-input.c
bt_audio_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-audio.c
bt_network_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-network.c
bt_serial_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-serial.c

if OBEX
bin_PROGRAMS += bt-obex
bt_obex_SOURCES = $(lib_sources) $(bluez_sources) $(obexd_sources) bt-obex.c
bt_obex_LDADD = $(LIBREADLINE)
endif

dist_man_MANS = bt-monitor.1 bt-adapter.1 bt-agent.1 bt-device.1 bt-input.1 bt-audio.1 bt-network.1 bt-serial.1
if OBEX
dist_man_MANS += bt-obex.1
endif

#CLEANFILES =	lib/marshallers.c lib/marshallers.h
