AC_INIT([bluez-tools], m4_esyscmd([./git-version-gen .tarball-version]), [zxteam@gmail.com], [bluez-tools], [http://code.google.com/p/bluez-tools/])
AC_PREREQ([2.65])
AM_INIT_AUTOMAKE([1.11 no-define foreign -Wall subdir-objects])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_INSTALL
AM_PROG_CC_C_O

PKG_CHECK_MODULES([DBUS], [dbus-1 >= 1.2.16])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.0])
PKG_CHECK_MODULES([DBUS_GLIB], [dbus-glib-1 >= 0.84])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
