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
  SDASH=sdash sreplay
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
	editorbits.c \
	help.c

gtkheaders = \
	gtkgfx.h\
	gtkui.h\
	gtkmain.h \
	editor.h \
	editorwidgets.h \
	editorexport.h \
	editorbits.h \
	help.h




# sdl sources and headers:
#   the gdash sdl ui with sounds.
sdlbasesources = \
	sdlgfx.c \
	sdlui.c

sdlbaseheaders = \
	sdlgfx.h \
	sdlui.h

# sdl sources and headers:
#   the gdash sdl ui with sounds.
sdashsources = \
	sdlmain.c

sdashheaders = 


# sdl sources and headers:
#   the gdash sdl ui with sounds.
sreplaysources = \
	sdlreplay.c \
	IMG_savepng.c

sreplayheaders = \
	IMG_savepng.h



noinst_HEADERS = \
	$(base_headers) \
	$(gtkheaders) \
	$(sdlbaseheaders) \
	$(sound_headers) \
	$(sdashheaders) \
	$(sreplayheaders)






# gtk cflags also for the sdl version - for settings.c
# sdl is needed for sound.
gdash_CFLAGS = -g -Wall @GTK_CFLAGS@ @GLIB_CFLAGS@ @SDL_CFLAGS@
gdash_LDFLAGS = -g -Wall
if SDL
    GDASH_LIBADD=-lSDL_mixer
endif
gdash_LDADD = @GTK_LIBS@ @GLIB_LIBS@ @LIBINTL@ @SDL_LIBS@ $(GDASH_LIBADD)
gdash_SOURCES = \
	$(base_sources) \
	$(gtksources) \
	$(sound_sources)





# gtk cflags also for the sdl version - for settings.c
# sdl is needed for gfx and sound.
sdash_CFLAGS = -g -Wall @GTK_CFLAGS@ @GLIB_CFLAGS@ @SDL_CFLAGS@
sdash_LDFLAGS = -g -Wall
if SDL
    SDASH_LIBADD=-lSDL_mixer -lSDL_image
endif
sdash_LDADD = @GLIB_LIBS@ @SDL_LIBS@ $(SDASH_LIBADD)
sdash_SOURCES = \
	$(base_sources) \
	$(sdlbasesources) \
	$(sdashsources) \
	$(sound_sources)
	
# gtk cflags also for the sdl version - for settings.c
# sdl is needed for gfx and sound.
sreplay_CFLAGS = -g -Wall @GTK_CFLAGS@ @GLIB_CFLAGS@ @SDL_CFLAGS@ @LIBPNG_CFLAGS@
sreplay_LDFLAGS = -g -Wall
if SDL
    SREPLAY_LIBADD=-lSDL_mixer -lSDL_image
endif
sreplay_LDADD = @GLIB_LIBS@ @SDL_LIBS@ @LIBPNG_LIBS@ $(SREPLAY_LIBADD)
sreplay_SOURCES = \
	$(base_sources) \
	$(sdlbasesources) \
	$(sreplaysources) \
	$(sound_sources)

