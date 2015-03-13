## Process this file with automake to produce Makefile.in

SUBDIRS = po include src caves util docs sound music

ACLOCAL_AMFLAGS = -I m4


desktopdir = $(datadir)/applications
desktop_DATA = gdash.desktop

gdash.nsi: gdash.nsi.in
	sed s/GDASH-VERSION/$(PACKAGE_VERSION)/ <$^ >$@

EXTRA_DIST = config.rpath mkinstalldirs m4/ChangeLog gdash.ico gdash.nsi.in $(desktop_DATA) gdk-pixbuf.loaders engine_tests.bd COPYING.SDL COPYING.GTK README-SDL.txt

