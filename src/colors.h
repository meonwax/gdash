/*
 * Copyright (c) 2007, 2008 Czirkos Zoltan <cirix@fw.hu>
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
#ifndef _GD_COLORS
#define _GD_COLORS
 
#include <glib.h>

/* color:
   XXRRGGBB;
   XX is 0 for RGB,
         1 for c64 colors (bb=index)
         2 for atari colors (bb=index)
*/

typedef guint32 GdColor;

const char ** gd_color_get_c64_palette_names();
const char ** gd_color_get_atari_palette_names();

/* i/o */
const char* gd_color_get_string(GdColor color);
const char* gd_color_get_visible_name(GdColor color);

unsigned int gd_color_get_r(GdColor color);
unsigned int gd_color_get_g(GdColor color);
unsigned int gd_color_get_b(GdColor color);
GdColor gd_color_get_rgb(GdColor color);

GdColor gd_color_get_from_string(const char *color);
GdColor gd_color_get_from_rgb(int r, int g, int b);

GdColor gd_c64_color(int index);
GdColor gd_atari_color(int index);
gboolean gd_color_is_c64(GdColor color);
int gd_color_get_c64_index(GdColor color);
int gd_color_get_c64_index_try(GdColor color);

#define GD_C64_BLACK (gd_c64_color(0))
#define GD_C64_WHITE (gd_c64_color(1))
#define GD_C64_RED (gd_c64_color(2))
#define GD_C64_PURPLE (gd_c64_color(4))
#define GD_C64_CYAN (gd_c64_color(3))
#define GD_C64_GREEN (gd_c64_color(5))
#define GD_C64_BLUE (gd_c64_color(6))
#define GD_C64_YELLOW (gd_c64_color(7))
#define GD_C64_ORANGE (gd_c64_color(8))
#define GD_C64_BROWN (gd_c64_color(9))
#define GD_C64_LIGHTRED (gd_c64_color(10))
#define GD_C64_GRAY1 (gd_c64_color(11))
#define GD_C64_GRAY2 (gd_c64_color(12))
#define GD_C64_LIGHTGREEN (gd_c64_color(13))
#define GD_C64_LIGHTBLUE (gd_c64_color(14))
#define GD_C64_GRAY3 (gd_c64_color(15))

#endif

