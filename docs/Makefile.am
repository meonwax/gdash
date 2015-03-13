## Process this file with automake to produce Makefile.in
EXTRA_DIST = \
	c64_internals.txt ckdelay.xls cd7_speeds.xls ckdelay.txt brc_format.txt \
	cave_style.css


# To create the docs, GDash must be installed on the system,
# so it can use the translations.

.PHONY: Docs Doc-English.html Doc-Deutsch.html Doc-Magyar.html

Docs: Doc-English.html Doc-Deutsch.html Doc-Magyar.html

Doc-English.html: ../src/gdash
	../src/gdash -q --save-docs 1

Doc-Deutsch.html: ../src/gdash
	../src/gdash -q --save-docs 2

Doc-Magyar.html: ../src/gdash
	../src/gdash -q --save-docs 3
