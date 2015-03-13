## Process this file with automake to produce Makefile.in

localedir = $(datadir)/locale

DEFS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" @DEFS@

INCLUDES = \
	-I$(top_srcdir)/include

bin_PROGRAMS = gdash sdash

noinst_HEADERS = \
	cave.h \
	caveengine.h \
	caveobject.h\
	caveset.h\
	c64import.h\
	game.h \
	gtk_gfx.h\
	gtk_ui.h\
	gtk_main.h \
	editor.h \
	editor-widgets.h\
	editor-export.h\
	help.h\
	settings.h\
	util.h\
	sdl_gfx.h\
	sdl_sound.h\
	sdl_ui.h\
	about.h

gdash_SOURCES = \
	settings.c\
	util.c\
	cave.c\
	caveengine.c\
	caveobject.c\
	caveset.c\
	c64import.c\
	game.c\
	gtk_gfx.c\
	gtk_ui.c\
	gtk_main.c\
	editor-widgets.c\
	editor-export.c\
	editor.c\
	help.c\
	about.c

gdash_CFLAGS = -g -Wall @GTK_CFLAGS@
gdash_LDFLAGS = -g -Wall
gdash_LDADD = @GTK_LIBS@ @LIBINTL@





sdash_CFLAGS = -g -Wall @GLIB_CFLAGS@ @SDL_CFLAGS@
sdash_LDFLAGS = -g -Wall -lSDL_mixer
sdash_LDADD = @GLIB_LIBS@ @SDL_LIBS@

sdash_SOURCES=\
	sdl_gfx.c\
	sdl_main.c\
	sdl_sound.c\
	sdl_ui.c\
	settings.c\
	util.c\
	cave.c\
	caveengine.c\
	caveobject.c\
	caveset.c\
	c64import.c\
	game.c\
	about.c
