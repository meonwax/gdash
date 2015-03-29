## Process this file with automake to produce Makefile.in

SUBDIRS = po include src caves docs sound music shaders

ACLOCAL_AMFLAGS = -Im4

desktopdir = $(datadir)/applications
desktop_DATA = gdash.desktop

icondir = $(datadir)/pixmaps
icon_DATA = gdash.png

gdash.nsi: gdash.nsi.in
	sed s/GDASH-VERSION/$(PACKAGE_VERSION)/ <$^ >$@
	echo do not forget to strip gdash.exe
	echo do not forget to create documentation

EXTRA_DIST = \
	$(icon_DATA) $(desktop_DATA) \
	config.rpath mkinstalldirs m4/ChangeLog \
	gdash.ico gdash.nsi.in gtkrc_win32 \
	engine_tests.bd replays_example.bd \
	COPYING.SDL COPYING.GTK COPYING.HQX README-SDL.txt \
	doxygen.conf doxygen.css

doxy:
	doxygen doxygen.conf
