## Process this file with automake to produce Makefile.in
CAVES = \
	Afl_Posocopi.gds

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
	image.png \
	statistics.png

TITLE = \
	gdash_screen.png \
	gdash_tile.png

GAMEBACKGROUND = \
	gamebackground.png

GDASH_ICON_32 = \
	gdash_icon_32.png

GDASH_ICON_48 = \
	gdash_icon_48.png

FOR_HTML = \
	background.png \
	style.css

noinst_HEADERS = \
	icons.cpp \
	c64_gfx.cpp \
	c64_font.cpp \
	title.cpp \
	gamebackground.cpp \
	gdash_icon_32.cpp \
	gdash_icon_48.cpp \
	levels.cpp \
	ataripal.cpp \
	dtvpal.cpp \
	for_html.cpp

pkgdata_DATA = boulder_rush.png c64_gfx.png c64_gfx_bd2.png

EXTRA_DIST = $(ICONS) $(TITLE) $(GAMEBACKGROUND) $(FOR_HTML) \
	icons.list \
	files2c.sh \
	c64_font.png \
	palettetoact.c palette2c.c \
	$(GDASH_ICON_32) \
	$(GDASH_ICON_48) \
	$(pkgdata_DATA) \
	$(CAVES) \
	$(ATARIPALETTES) \
	$(DTVPALETTES)


# for included ICONS
icons.list: $(ICONS) Makefile.am
	( rm -f $@; \
		for image in $(ICONS); do \
			echo $$image | sed s/\.png$$// >> $@; \
			echo $$image >> $@; \
		done )

# the include files
icons.cpp: $(ICONS) icons.list
	gdk-pixbuf-csource --rle --build-list `cat icons.list` > $@

# own programs which generate include files
noinst_PROGRAMS = \
	file2c

# this utility creates an include file for any datafile
file2c_SOURCES = \
	file2c.cpp

levels.cpp: $(CAVES) file2c Makefile.am
	sh ./files2c.sh $@ level $(CAVES)

ataripal.cpp: $(ATARIPALETTES) file2c Makefile.am
	sh ./files2c.sh $@ atari_palettes $(ATARIPALETTES)

dtvpal.cpp: $(DTVPALETTES) file2c Makefile.am
	sh ./files2c.sh $@ c64dtv_palettes $(DTVPALETTES)

# the empty string causes files2c.sh not to write the pointer arrays
title.cpp: $(TITLE) file2c Makefile.am
	sh ./files2c.sh $@ "" $(TITLE)

# the empty string causes files2c.sh not to write the pointer arrays
gamebackground.cpp: $(GAMEBACKGROUND) file2c Makefile.am
	sh ./files2c.sh $@ "" $(GAMEBACKGROUND)

# the empty string causes files2c.sh not to write the pointer arrays
gdash_icon_32.cpp: $(GDASH_ICON_32) file2c Makefile.am
	sh ./files2c.sh $@ "" $(GDASH_ICON_32)

# the empty string causes files2c.sh not to write the pointer arrays
gdash_icon_48.cpp: $(GDASH_ICON_48) file2c Makefile.am
	sh ./files2c.sh $@ "" $(GDASH_ICON_48)

# the empty string causes files2c.sh not to write the pointer arrays
c64_gfx.cpp: c64_gfx.png file2c Makefile.am
	sh ./files2c.sh $@ "" c64_gfx.png

# the empty string causes files2c.sh not to write the pointer arrays
c64_font.cpp: c64_font.png file2c Makefile.am
	sh ./files2c.sh $@ "" c64_font.png

# the empty string causes files2c.sh not to write the pointer arrays
for_html.cpp: $(FOR_HTML) file2c Makefile.am
	sh ./files2c.sh $@ "" $(FOR_HTML)
