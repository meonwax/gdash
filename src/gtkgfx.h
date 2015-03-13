/*
 * Copyright (c) 2007, 2008, 2009, Czirkos Zoltan <cirix@fw.hu>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _GD_GFX_H
#define _GD_GFX_H

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>
#include "settings.h"
#include "cave.h"

extern int gd_cell_size_game, gd_cell_size_editor;
extern GdkPixbuf *gd_pixbuf_for_builtin_theme;

/* tv stripes for a pixbuf. exported for the settings window */
void gd_pal_emulate_pixbuf(GdkPixbuf *pixbuf);
GdkPixbuf *gd_pixbuf_scale(GdkPixbuf *orig, GdScalingType type);

/* png graphics loading */
const char* gd_is_pixbuf_ok_for_theme(GdkPixbuf *pixbuf);
const char* gd_is_image_ok_for_theme(const char *filename);
void gd_loadcells_default();
gboolean gd_loadcells_file(const char *filename);

/* set scaling */
void gd_select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5);
GdColor gd_current_background_color();

GdkPixmap *gd_game_pixmap(int index);
GdkPixmap *gd_editor_pixmap(int index);

GdkPixbuf *gd_get_element_pixbuf_with_border (GdElement element);
GdkPixbuf *gd_get_element_pixbuf_simple_with_border (GdElement element);
GdkPixbuf *gd_drawcave_to_pixbuf(const GdCave *cave, const int width, const int height, const gboolean game_view, const gboolean border);

void gd_create_pixbuf_for_builtin_theme();
GdkPixbuf *gd_pixbuf_load_from_data(guchar *data, int length);
GdkPixbuf *gd_pixbuf_load_from_base64(gchar *base64);
GdkPixbuf *gd_create_title_image();
#endif

