## Process this file with automake to produce Makefile.in
AM_CFLAGS = -g -Wall @GLIB_CFLAGS@
LDADD = @GLIB_LIBS@

noinst_PROGRAMS = \
	any2gdash oldbin2gdash palette2c

any2gdash_SOURCES = \
	any2gdash.c

oldbin2gdash_SOURCES = \
	oldbin2gdash.c

palette2c_SOURCES = \
	palette2c.c

EXTRA_DIST = palettetoact.c
