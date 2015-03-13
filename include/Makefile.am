## Process this file with automake to produce Makefile.in
CAVES = \
	No_One_01.gds \
	No_One_02.gds \
	No_One_03.gds \
	No_One_04.gds \
	No_One_05.gds \
	No_One_06.gds \
	No_One_07.gds \
	No_One_08.gds \
	No_One_09.gds \
	No_One_10.gds

ATARIPALETTES = \
	BuiltIn.act \
	BuiltIn_contrast.act \
	Default.act \
	Jakub.act \
	Jakub_contrast.act \
	Real.act \
	Real_contrast.act \
	XFormer.act

DTVPALETTES = \
	Spiff.act \
	Murray.act

ICONS = \
	add_filled_rectangle.png \
	add_freehand.png \
	add_line.png \
	add_join.png \
	add_point.png \
	add_raster.png \
	add_rectangle.png \
	add_fill_replace.png \
	add_fill_border.png \
	add_maze.png \
	add_maze_uni.png \
	add_maze_braid.png \
	cave_editor.png \
	move.png \
	snapshot.png \
	restart_level.png \
	random_fill.png \
	award.png \
	to_top.png \
	to_bottom.png \
	object_on_all.png \
	object_not_on_all.png \
	object_not_on_current.png \
	replay.png \
	keyboard.png \
	image.png

noinst_HEADERS = \
	c64_png_colors.h \
	icons.h \
	c64_gfx.h \
	c64_font.h \
	title.h \
	levels.h \
	ataripal.h \
	dtvpal.h

icondir = $(datadir)/pixmaps
icon_DATA = gdash.png

pkgdata_DATA = boulder_rush.png c64_gfx.png

EXTRA_DIST = $(ICONS) $(TITLE) $(icon_DATA) \
	icons.list title.list \
	files2c.sh \
	c64_font.png \
	c64_gfx_bd2.png \
	$(pkgdata_DATA) \
	$(CAVES) levels.h \
	$(ATARIPALETTES) ataripal.h \
	$(DTVPALETTES) dtvpal.h


# for included ICONS
icons.list: $(ICONS) Makefile.am
	( rm -f $@; \
		for image in $(ICONS); do \
			echo $$image | sed s/\.png$$// >> $@; \
			echo $$image >> $@; \
		done )

# the include files
icons.h: $(ICONS) icons.list
	gdk-pixbuf-csource --raw --build-list `cat icons.list` > $@

#title.h: $(TITLE) title.list
#	gdk-pixbuf-csource --raw --build-list `cat title.list` > $@

if GTK
  CELLS2C = cells2c
else
  CELLS2C = 
endif

# own programs which generate include files
noinst_PROGRAMS = \
	$(CELLS2C) \
	file2c

# this one creates inlined original c64 graphics
cells2c_CFLAGS = @GTK_CFLAGS@
cells2c_LDADD =  @GTK_LIBS@
cells2c_SOURCES = \
	cells2c.c



if GTK
c64_font.h: c64_font.png $(CELLS2C)
	./cells2c c64_font.png >$@
else
c64_font.h: c64_font.png $(CELLS2C)
	echo; echo; echo "*** GTK+ is needed to regenerate $@"
	false
endif


if GTK
c64_gfx.h: c64_gfx.png $(CELLS2C)
	./cells2c c64_gfx.png >$@
else
c64_gfx.h: c64_gfx.png $(CELLS2C)
	echo; echo; echo "*** GTK+ is needed to regenerate $@"
	false
endif




# this utility creates an include file for any datafile
file2c_SOURCES = \
	file2c.c

levels.h: $(CAVES) file2c Makefile.am
	sh files2c.sh $@ level $(CAVES)

ataripal.h: $(ATARIPALETTES) file2c Makefile.am
	sh files2c.sh $@ atari_palettes $(ATARIPALETTES)

dtvpal.h: $(DTVPALETTES) file2c Makefile.am
	sh files2c.sh $@ c64dtv_palettes $(DTVPALETTES)

TITLE = \
	gdash_screen.png \
	gdash_tile.png \
	gdash.png \
	gdash_32.png

# the empty string causes files2c.sh not to write the pointer arrays
title.h: $(TITLE) file2c Makefile.am
	sh files2c.sh $@ "" $(TITLE)
