## Process this file with automake to produce Makefile.in

localedir = $(datadir)/locale

DEFS = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" @DEFS@

INCLUDES = \
	-I$(top_srcdir)/include

# if gtk is enabled, build gdash.
if GTK
  GDASH=gdash
else
  GDASH=
endif

# if sdl is enabled, build sdash.
if SDL
  SDASH=sdash
else
  SDASH=
endif


bin_PROGRAMS = $(GDASH) $(SDASH)




# base sources and headers:
#   the gdash engine.
base_headers = \
	cave.h \
	caveengine.h \
	caveobject.h \
	caveset.h \
	c64import.h \
	game.h \
	settings.h \
	util.h \
	about.h

base_sources = \
	cave.c \
	caveengine.c \
	caveobject.c \
	caveset.c \
	c64import.c \
	game.c \
	settings.c \
	util.c \
	about.c

# sdl-mixer generated sounds.
#   may be linked to the game or not.
sound_headers = \
	sound.h

sound_sources = \
	sound.c

# gtk sources and headers:
#   the gdash gtk ui, with editor.
gtk_sources = \
	gtk_gfx.c \
	gtk_ui.c \
	gtk_main.c \
	editor-widgets.c \
	editor-export.c \
	editor.c \
	help.c

gtk_headers = \
	gtk_gfx.h\
	gtk_ui.h\
	gtk_main.h \
	editor.h \
	editor-widgets.h\
	editor-export.h\
	help.h

# sdl sources and headers:
#   the gdash sdl ui with sounds.
sdl_sources = \
	sdl_gfx.c \
	sdl_main.c \
	sdl_ui.c

sdl_headers = \
	sdl_gfx.h \
	sdl_ui.h



noinst_HEADERS = \
	$(base_headers) \
	$(gtk_headers) \
	$(sdl_headers) \
	$(sound_headers)





gdash_CFLAGS = -g -Wall @GTK_CFLAGS@ @SDL_CFLAGS@
gdash_LDFLAGS = -g -Wall -lSDL_mixer
gdash_LDADD = @GTK_LIBS@ @LIBINTL@ @SDL_LIBS@
gdash_SOURCES = \
	$(base_sources) \
	$(gtk_sources) \
	$(sound_sources)






sdash_CFLAGS = -g -Wall @GLIB_CFLAGS@ @SDL_CFLAGS@
sdash_LDFLAGS = -g -Wall -lSDL_mixer
sdash_LDADD = @GLIB_LIBS@ @SDL_LIBS@
sdash_SOURCES = \
	$(base_sources) \
	$(sdl_sources) \
	$(sound_sources)
