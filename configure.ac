AC_INIT([bluez-tools], [0.1], [bluez-tools-bug-report@notavailable.ru], [bluez-tools], [http://notavailable.ru/projects/bluez-tools/])
AC_PREREQ([2.65])
AM_INIT_AUTOMAKE([1.11 no-define foreign -Wall])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_INSTALL

dnl 1.2.24
PKG_CHECK_MODULES([DBUS], [dbus-1 >= 1.2.16])

dnl 2.24.1
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.22.4])
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0 >= 2.22.4])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.22.4])

dnl 0.86
PKG_CHECK_MODULES([DBUS_GLIB], [dbus-glib-1 >= 0.82])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
