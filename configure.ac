AC_INIT([unscroll], [0.0.1])
AC_CONFIG_SRCDIR([src/unscroll.c])

AM_INIT_AUTOMAKE

AC_PROG_CC

PKG_CHECK_MODULES([cairo], [cairo >= 1.9.2])
PKG_CHECK_MODULES([poppler], [poppler-glib >= 0.12.2])
PKG_CHECK_MODULES([glib], [glib-2.0 >= 2.22])

AC_CHECK_LIB(popt, poptGetContext,,AC_ERROR([Libpopt required.]))
AC_CHECK_LIB(paper, paperinit,,AC_ERROR([Libpaper required.]))
AC_CHECK_LIB(jpeg, jpeg_CreateCompress,,AC_ERROR([Libjpeg required.]))

AC_CHECK_HEADER([paper.h],,AC_ERROR([Libpaper required.]))
AC_CHECK_HEADER([jpeglib.h],,AC_ERROR([Libjpeg required.]))

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
