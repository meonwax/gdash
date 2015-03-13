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
#ifndef _GD_COLORS
#define _GD_COLORS
 
#include <glib.h>

typedef guint32 GdColor;

/* color internal:
   XXRRGGBB;
   XX is 0 for RGB,
         1 for c64 colors (bb=index)
         3 for c64dtv (bb=index)
         2 for atari colors (bb=index)
*/

typedef enum _color_type {
    GD_COLOR_TYPE_RGB=0,
    GD_COLOR_TYPE_C64=1,
    GD_COLOR_TYPE_C64DTV=2,
    GD_COLOR_TYPE_ATARI=3,
    GD_COLOR_TYPE_UNKNOWN    /* should be the last one */
} GdColorType;

const char ** gd_color_get_c64_palette_names();
const char ** gd_color_get_atari_palette_names();
const char ** gd_color_get_c64dtv_palette_names();

const char ** gd_color_get_palette_types_names();

/* i/o */
const char* gd_color_get_string(GdColor color);
const char* gd_color_get_visible_name(GdColor color);

unsigned int gd_color_get_r(GdColor color);
unsigned int gd_color_get_g(GdColor color);
unsigned int gd_color_get_b(GdColor color);
GdColor gd_color_get_rgb(GdColor color);

GdColor gd_color_get_from_string(const char *color);
GdColor gd_color_get_from_rgb(int r, int g, int b);
GdColor gd_color_get_from_hsv(double h, double s, double v);

GdColor gd_c64_color(int index);
GdColor gd_atari_color(int index);
GdColor gd_atari_color_huesat(int hue, int sat);
GdColor gd_c64dtv_color(int index);
GdColor gd_c64dtv_color_huesat(int hue, int sat);

gboolean gd_color_is_c64(GdColor color);
gboolean gd_color_is_atari(GdColor color);
gboolean gd_color_is_dtv(GdColor color);
gboolean gd_color_is_rgb(GdColor color);
gboolean gd_color_is_unknown(GdColor color);

int gd_color_get_c64_index(GdColor color);
int gd_color_get_c64_index_try(GdColor color);

GdColor gd_gdash_color(int c);

/* traditional c64 color indexes. */
#define GD_COLOR_INDEX_BLACK (0)
#define GD_COLOR_INDEX_WHITE (1)
#define GD_COLOR_INDEX_RED (2)
#define GD_COLOR_INDEX_PURPLE (4)
#define GD_COLOR_INDEX_CYAN (3)
#define GD_COLOR_INDEX_GREEN (5)
#define GD_COLOR_INDEX_BLUE (6)
#define GD_COLOR_INDEX_YELLOW (7)
#define GD_COLOR_INDEX_ORANGE (8)
#define GD_COLOR_INDEX_BROWN (9)
#define GD_COLOR_INDEX_LIGHTRED (10)
#define GD_COLOR_INDEX_GRAY1 (11)
#define GD_COLOR_INDEX_GRAY2 (12)
#define GD_COLOR_INDEX_LIGHTGREEN (13)
#define GD_COLOR_INDEX_LIGHTBLUE (14)
#define GD_COLOR_INDEX_GRAY3 (15)

#define GD_GDASH_BLACK (gd_gdash_color(GD_COLOR_INDEX_BLACK))
#define GD_GDASH_WHITE (gd_gdash_color(GD_COLOR_INDEX_WHITE))
#define GD_GDASH_RED (gd_gdash_color(GD_COLOR_INDEX_RED))
#define GD_GDASH_PURPLE (gd_gdash_color(GD_COLOR_INDEX_PURPLE))
#define GD_GDASH_CYAN (gd_gdash_color(GD_COLOR_INDEX_CYAN))
#define GD_GDASH_GREEN (gd_gdash_color(GD_COLOR_INDEX_GREEN))
#define GD_GDASH_BLUE (gd_gdash_color(GD_COLOR_INDEX_BLUE))
#define GD_GDASH_YELLOW (gd_gdash_color(GD_COLOR_INDEX_YELLOW))
#define GD_GDASH_ORANGE (gd_gdash_color(GD_COLOR_INDEX_ORANGE))
#define GD_GDASH_BROWN (gd_gdash_color(GD_COLOR_INDEX_BROWN))
#define GD_GDASH_LIGHTRED (gd_gdash_color(GD_COLOR_INDEX_LIGHTRED))
#define GD_GDASH_GRAY1 (gd_gdash_color(GD_COLOR_INDEX_GRAY1))
#define GD_GDASH_GRAY2 (gd_gdash_color(GD_COLOR_INDEX_GRAY2))
#define GD_GDASH_LIGHTGREEN (gd_gdash_color(GD_COLOR_INDEX_LIGHTGREEN))
#define GD_GDASH_LIGHTBLUE (gd_gdash_color(GD_COLOR_INDEX_LIGHTBLUE))
#define GD_GDASH_GRAY3 (gd_gdash_color(GD_COLOR_INDEX_GRAY3))

#define GD_GDASH_MIDDLEBLUE (gd_gdash_color(16))

#define GD_COLOR_INVALID (0xFFFFFFFF)

#endif

