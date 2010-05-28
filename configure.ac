AC_INIT([bluez-tools], [0.1], [bluez-tools-bug-report@notavailable.ru], [bluez-tools], [http://notavailable.ru/projects/bluez-tools/])
AC_PREREQ([2.65])
AM_INIT_AUTOMAKE([1.11 no-define foreign -Wall])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_INSTALL

PKG_CHECK_MODULES([DBUS], [dbus-1 >= 1.2.24])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.1])
PKG_CHECK_MODULES([DBUS_GLIB], [dbus-glib-1 >= 0.86])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
