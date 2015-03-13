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


strip: $(bin_PROGRAMS)
	strip $(bin_PROGRAMS)

# base sources and headers:
#   the gdash engine.
base_headers = \
	colors.h \
	cave.h \
	cavedb.h \
	caveengine.h \
	caveobject.h \
	cavesound.h \
	bdcff.h \
	caveset.h \
	c64import.h \
	gameplay.h \
	settings.h \
	util.h \
	about.h \
	gfxutil.h

base_sources = \
	colors.c \
	cave.c \
	cavedb.c \
	caveengine.c \
	caveobject.c \
	cavesound.c \
	bdcff.c \
	caveset.c \
	c64import.c \
	gameplay.c \
	settings.c \
	util.c \
	about.c \
	gfxutil.c

# sdl-mixer generated sounds.
#   may be linked to the game or not.
sound_headers = \
	sound.h

sound_sources = \
	sound.c

# gtk sources and headers:
#   the gdash gtk ui, with editor.
gtksources = \
	gtkgfx.c \
	gtkui.c \
	gtkmain.c \
	editorwidgets.c \
	editorexport.c \
	editor.c \
	help.c

gtkheaders = \
	gtkgfx.h\
	gtkui.h\
	gtkmain.h \
	editor.h \
	editorwidgets.h\
	editorexport.h\
	help.h

# sdl sources and headers:
#   the gdash sdl ui with sounds.
sdlsources = \
	sdlgfx.c \
	sdlmain.c \
	sdlui.c

sdlheaders = \
	sdlgfx.h \
	sdlui.h



noinst_HEADERS = \
	$(base_headers) \
	$(gtkheaders) \
	$(sdlheaders) \
	$(sound_headers)





gdash_CFLAGS = -g -Wall @GTK_CFLAGS@ @GLIB_CFLAGS@ @SDL_CFLAGS@
gdash_LDFLAGS = -g -Wall
gdash_LDADD = @GTK_LIBS@ @GLIB_LIBS@ @LIBINTL@ @SDL_LIBS@ -lSDL_mixer
gdash_SOURCES = \
	$(base_sources) \
	$(gtksources) \
	$(sound_sources)






sdash_CFLAGS = -g -Wall @GLIB_CFLAGS@ @SDL_CFLAGS@
sdash_LDFLAGS = -g -Wall
sdash_LDADD = @GLIB_LIBS@ @SDL_LIBS@ -lSDL_mixer
sdash_SOURCES = \
	$(base_sources) \
	$(sdlsources) \
	$(sound_sources)
