## Process this file with automake to produce Makefile.in

shadersdata = \
	Curvature.shader \
	Dot_N_Bloom.shader \
	GDash-TV.shader \
	Phosphor-simple.shader \
	templateshader

if SDL
  shadersdir = $(pkgdatadir)/shaders
  shaders_DATA = $(shadersdata)
endif

EXTRA_DIST = $(shadersdata)
