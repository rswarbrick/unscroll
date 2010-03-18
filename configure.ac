AC_INIT([unscroll], [0.0.1])
AC_CONFIG_SRCDIR([src/unscroll.c])

AM_INIT_AUTOMAKE

AC_PROG_CC

PKG_CHECK_MODULES([cairo], [cairo])
PKG_CHECK_MODULES([poppler], [poppler-glib >= 0.12.2])
PKG_CHECK_MODULES([glib], [glib-2.0 >= 2.22])

AC_CHECK_LIB(popt, poptGetContext)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
