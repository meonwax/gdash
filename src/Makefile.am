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

bin_PROGRAMS = $(GDASH)


strip: $(bin_PROGRAMS)
	strip $(bin_PROGRAMS)

# base sources and headers:
#   the gdash engine.
base_headers = \
	misc/printf.hpp \
	misc/smartptr.hpp \
	cave/cavetypes.hpp \
	cave/elementproperties.hpp \
	cave/helper/namevaluepair.hpp \
	cave/helper/reflective.hpp \
	cave/helper/adoptingcontainer.hpp \
	cave/helper/cavemap.hpp \
	cave/helper/colors.hpp \
	cave/helper/cavehighscore.hpp \
	cave/cavebase.hpp \
	cave/cavestored.hpp \
	cave/helper/caverandom.hpp \
	cave/helper/cavesound.hpp \
	cave/object/caveobjectfill.hpp \
	cave/object/caveobjectboundaryfill.hpp \
	cave/object/caveobjectcopypaste.hpp \
	cave/object/caveobjectfillrect.hpp \
	cave/object/caveobjectfloodfill.hpp \
	cave/object/caveobject.hpp \
	cave/object/caveobjectrectangular.hpp \
	cave/object/caveobjectjoin.hpp \
	cave/object/caveobjectline.hpp \
	cave/object/caveobjectmaze.hpp \
	cave/object/caveobjectpoint.hpp \
	cave/object/caveobjectrandomfill.hpp \
	cave/object/caveobjectraster.hpp \
	cave/object/caveobjectrectangle.hpp \
	cave/caverendered.hpp \
	cave/particle.hpp \
	cave/helper/cavereplay.hpp \
	cave/caveset.hpp \
	fileops/bdcffhelper.hpp \
	fileops/bdcffload.hpp \
	fileops/bdcffsave.hpp \
	fileops/c64import.hpp \
	fileops/brcimport.hpp \
	fileops/binaryimport.hpp \
	fileops/loadfile.hpp \
	cave/gamecontrol.hpp \
	settings.hpp \
	misc/util.hpp \
	misc/logger.hpp \
	misc/about.hpp \
	gfx/pixbuf.hpp \
	gfx/pixmap.hpp \
	gfx/screen.hpp \
	gfx/pixbuffactory.hpp \
	gfx/pixbufmanip.hpp \
	gfx/pixbufmanip_hqx.hpp \
	gfx/cellrenderer.hpp \
	gfx/fontmanager.hpp \
	cave/gamerender.hpp \
	cave/titleanimation.hpp \
	framework/app.hpp \
	framework/activity.hpp \
	framework/volumeactivity.hpp \
	framework/titlescreenactivity.hpp \
	framework/showtextactivity.hpp \
	framework/messageactivity.hpp \
	framework/gameactivity.hpp \
	framework/selectfileactivity.hpp \
	framework/inputtextactivity.hpp \
	framework/askyesnoactivity.hpp \
	framework/settingsactivity.hpp \
	framework/thememanager.hpp \
	framework/replaymenuactivity.hpp \
	framework/replaysaveractivity.hpp \
	framework/commands.hpp \
	input/joystick.hpp \
	input/gameinputhandler.hpp \
	sound/sound.hpp

base_sources = \
	misc/printf.cpp \
	cave/helper/colors.cpp \
	cave/cavetypes.cpp \
	cave/elementproperties.cpp \
	cave/helper/cavemap.cpp \
	cave/helper/cavereplay.cpp \
	cave/caverendered.cpp \
	cave/particle.cpp \
	cave/caverenderedengine.cpp \
	cave/helper/caverandom.cpp \
	cave/helper/cavesound.cpp \
	cave/helper/cavehighscore.cpp \
	cave/cavebase.cpp \
	cave/cavestored.cpp \
	cave/object/caveobject.cpp \
	cave/object/caveobjectrectangular.cpp \
	cave/object/caveobjectfill.cpp \
	cave/object/caveobjectboundaryfill.cpp \
	cave/object/caveobjectcopypaste.cpp \
	cave/object/caveobjectfillrect.cpp \
	cave/object/caveobjectfloodfill.cpp \
	cave/object/caveobjectjoin.cpp \
	cave/object/caveobjectline.cpp \
	cave/object/caveobjectmaze.cpp \
	cave/object/caveobjectpoint.cpp \
	cave/object/caveobjectrandomfill.cpp \
	cave/object/caveobjectraster.cpp \
	cave/object/caveobjectrectangle.cpp \
	cave/caveset.cpp \
	fileops/bdcffhelper.cpp \
	fileops/bdcffload.cpp \
	fileops/bdcffsave.cpp \
	fileops/c64import.cpp \
	fileops/brcimport.cpp \
	fileops/binaryimport.cpp \
	fileops/loadfile.cpp \
	cave/gamecontrol.cpp \
	settings.cpp \
	misc/util.cpp \
	misc/logger.cpp \
	misc/about.cpp \
	gfx/pixbuf.cpp \
	gfx/pixmap.cpp \
	gfx/screen.cpp \
	gfx/pixbuffactory.cpp \
	gfx/pixbufmanip.cpp \
	gfx/pixbufmanip_hq2x.cpp \
	gfx/pixbufmanip_hq3x.cpp \
	gfx/pixbufmanip_hq4x.cpp \
	gfx/cellrenderer.cpp \
	gfx/fontmanager.cpp \
	cave/gamerender.cpp \
	cave/titleanimation.cpp \
	framework/app.cpp \
	framework/activity.cpp \
	framework/volumeactivity.cpp \
	framework/titlescreenactivity.cpp \
	framework/showtextactivity.cpp \
	framework/messageactivity.cpp \
	framework/gameactivity.cpp \
	framework/selectfileactivity.cpp \
	framework/inputtextactivity.cpp \
	framework/askyesnoactivity.cpp \
	framework/settingsactivity.cpp \
	framework/thememanager.cpp \
	framework/replaymenuactivity.cpp \
	framework/replaysaveractivity.cpp \
	framework/commands.cpp \
	input/joystick.cpp \
	input/gameinputhandler.cpp \
	sound/sound.cpp

# gtk sources and headers:
#   the gdash gtk ui, with editor.
gtksources = \
	gtk/gtkpixbuf.cpp \
	gtk/gtkpixmap.cpp \
	gtk/gtkpixbuffactory.cpp \
	gtk/gtkscreen.cpp \
	gtk/gtkui.cpp \
	gtk/gtkuisettings.cpp \
	gtk/gtkgameinputhandler.cpp \
	editor/editorwidgets.cpp \
	editor/editorautowidgets.cpp \
	editor/editorcellrenderer.cpp \
	editor/exporthtml.cpp \
	editor/exportcrli.cpp \
	editor/editor.cpp \
	gtk/help.cpp \
	gtk/gtkapp.cpp \
	gtk/gtkmainwindow.cpp \
	sdl/sdlpixbuf.cpp \
	sdl/sdlpixmap.cpp \
	sdl/sdlabstractscreen.cpp \
	sdl/sdlscreen.cpp \
	sdl/sdlpixbuffactory.cpp \
	sdl/ogl.cpp \
	sdl/sdlgameinputhandler.cpp \
	sdl/sdlmainwindow.cpp \
	sdl/IMG_savepng.c \
	sdl/sdlparticle.cpp \
	gtkmain.cpp

gtkheaders = \
	gtk/gtkpixbuf.hpp \
	gtk/gtkpixmap.hpp \
	gtk/gtkpixbuffactory.hpp \
	gtk/gtkscreen.hpp \
	gtk/gtkui.hpp \
	gtk/gtkuisettings.hpp \
	gtk/gtkgameinputhandler.hpp \
	editor/editorwidgets.hpp \
	editor/editorautowidgets.hpp \
	editor/editorcellrenderer.hpp \
	editor/exporthtml.hpp \
	editor/exportcrli.hpp \
	editor/editor.hpp \
	gtk/help.hpp \
	gtk/gtkapp.hpp \
	gtk/gtkmainwindow.hpp \
	sdl/sdlpixbuf.hpp \
	sdl/sdlpixmap.hpp \
	sdl/sdlabstractscreen.hpp \
	sdl/sdlscreen.hpp \
	sdl/sdlpixbuffactory.hpp \
	sdl/ogl.hpp \
	sdl/sdlgameinputhandler.hpp \
	sdl/sdlmainwindow.hpp \
	sdl/sdlparticle.hpp \
	sdl/IMG_savepng.h




noinst_HEADERS = \
	$(base_headers) \
	$(gtkheaders)






gdash_CPPFLAGS = -g -Wall -DGDASH_GTK @GTK_CFLAGS@ @GLIB_CFLAGS@ @SDL_CFLAGS@ @GL_CFLAGS@ @LIBPNG_CFLAGS@
gdash_LDFLAGS = -g -Wall
if SDL
    GDASH_LIBADD=-lSDL_mixer -lSDL_image
endif
gdash_LDADD = @GTK_LIBS@ @GLIB_LIBS@ @LIBINTL@ @SDL_LIBS@ @GL_LIBS@ @LIBPNG_LIBS@ $(GDASH_LIBADD)
gdash_SOURCES = \
	$(base_sources) \
	$(gtksources) \
	$(sound_sources)
