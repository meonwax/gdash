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

#include <glib.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include "colors.h"
#include "settings.h"

#include "ataripal.h"

static char *c64_color_names[]={
	"Black", "White", "Red", "Cyan", "Purple", "Green", "Blue", "Yellow",
	"Orange", "Brown", "LightRed", "Gray1", "Gray2", "LightGreen", "LightBlue", "Gray3",
};

static char *c64_color_visible_names[]={
	N_("Black"), N_("White"), N_("Red"), N_("Cyan"), N_("Purple"), N_("Green"), N_("Blue"), N_("Yellow"),
	N_("Orange"), N_("Brown"), N_("Light red"), N_("Dark gray"), N_("Gray"), N_("Light green"), N_("Light blue"), N_("Light gray"),
};

static GdColor c64_colors_vice_old[]={
	0x000000, 0xFFFFFF, 0x68372b, 0x70a4b2, 0x6f3d86, 0x588d43, 0x352879, 0xb8c76f,
	0x6f4f25, 0x433900, 0x9a6759, 0x444444, 0x6c6c6c, 0x9ad284, 0x6c5eb5, 0x959595,
};

static GdColor c64_colors_vice_new[]={
	0x000000, 0xffffff, 0x894036, 0x7abfc7, 0x8a46ae, 0x68a941, 0x3e31a2, 0xd0dc71,
	0x905f25, 0x5c4700, 0xbb776d, 0x555555, 0x808080, 0xacea88, 0x7c70da, 0xababab,
};

static GdColor c64_colors_c64_hq[]={
	0x0A0A0A, 0xFFF8FF, 0x851F02, 0x65CDA8, 0xA73B9F, 0x4DAB19, 0x1A0C92, 0xEBE353,
	0xA94B02, 0x441E00, 0xD28074, 0x464646, 0x8B8B8B, 0x8EF68E, 0x4D91D1, 0xBABABA,
};

static GdColor c64_colors_c64s[]={
	0x000000, 0xFCFCFC, 0xA80000, 0x54FCFC, 0xA800A8, 0x00A800, 0x0000A8, 0xFCFC00,
	0xA85400, 0x802C00, 0xFC5454, 0x545454, 0x808080, 0x54FC54, 0x5454FC, 0xA8A8A8,
};

static GdColor c64_colors_ccs64[]={
	0x101010, 0xFFFFFF, 0xE04040, 0x60FFFF, 0xE060E0, 0x40E040, 0x4040E0, 0xFFFF40,
	0xE0A040, 0x9C7448, 0xFFA0A0, 0x545454, 0x888888, 0xA0FFA0, 0xA0A0FF, 0xC0C0C0,
};

static GdColor c64_colors_vice_default[]={
	0x000000, 0xFDFEFC, 0xBE1A24, 0x30E6C6, 0xB41AE2, 0x1FD21E, 0x211BAE, 0xDFF60A,
	0xB84104, 0x6A3304, 0xFE4A57, 0x424540, 0x70746F, 0x59FE59, 0x5F53FE, 0xA4A7A2,
};

static GdColor c64_colors_frodo[]={
	0x000000, 0xFFFFFF, 0xCC0000, 0x00FFCC, 0xFF00FF, 0x00CC00, 0x0000CC, 0xFFFF00,
	0xFF8800, 0x884400, 0xFF8888, 0x444444, 0x888888, 0x88FF88, 0x8888FF, 0xCCCCCC,
};

static GdColor c64_colors_godot[]={
	0x000000, 0xFFFFFF, 0x880000, 0xAAFFEE, 0xCC44CC, 0x00CC55, 0x0000AA, 0xEEEE77,
	0xDD8855, 0x664400, 0xFE7777, 0x333333, 0x777777, 0xAAFF66, 0x0088FF, 0xBBBBBB,
};

static GdColor c64_colors_pc64[]={
	0x212121, 0xFFFFFF, 0xB52121, 0x73FFFF, 0xB521B5, 0x21B521, 0x2121B5, 0xFFFF21,
	0xB57321, 0x944221, 0xFF7373, 0x737373, 0x949494, 0x73FF73, 0x7373FF, 0xB5B5B5,
};


/* make sure that pointeres and names match! */
static GdColor *c64_palette_pointers[]={
	c64_colors_vice_new,
	c64_colors_vice_old, c64_colors_vice_default, c64_colors_c64_hq, c64_colors_c64s,
	c64_colors_ccs64, c64_colors_frodo, c64_colors_godot, c64_colors_pc64,
	NULL
};

static const char *c64_palettes_names[]={
	"Vice new",
	"Vice old", "Vice default", "C64HQ", "C64S",
	"CCS64", "Frodo", "GoDot", "PC64",
	NULL
};



const char ** gd_color_get_c64_palette_names()
{
	return c64_palettes_names;
}

const char ** gd_color_get_atari_palette_names()
{
	return atari_palettes_names;
}


/* return c64 color with index. */
GdColor
gd_c64_color(int index)
{
	g_assert(index>=0 && index<=15);

	return (1<<24)+index;
}

/* return atari color with index. */
GdColor
gd_atari_color(int index)
{
	g_assert(index>=0 && index<=255);

	return (2<<24)+index;
}

/* convert color to rgbcolor; using the current palette. */
GdColor
gd_color_get_rgb(GdColor color)
{
	int index;
	const guint8 *atari_pal;
	const GdColor *c64_pal;
	
	/* silently switch to default, if invalid value */
	if (gd_c64_palette<0 || gd_c64_palette>=G_N_ELEMENTS(c64_palette_pointers)-1)
		gd_c64_palette=0;
	c64_pal=c64_palette_pointers[gd_c64_palette];
	if (gd_atari_palette<0 || gd_atari_palette>=G_N_ELEMENTS(atari_palettes_pointers)-1)
		gd_atari_palette=0;
	atari_pal=atari_palettes_pointers[gd_atari_palette];

	switch (color>>24) {
		case 0:
			/* is already rgb */
			return color;
		case 1:
			index=color&0x0f;
			return c64_pal[index];
		case 2:
			index=color&0xff;
			return gd_color_get_from_rgb(atari_pal[index*3], atari_pal[index*3+1], atari_pal[index*3+2]);
		default:
			g_assert_not_reached();
	}
}

unsigned int
gd_color_get_r(GdColor color)
{
	return ((gd_color_get_rgb(color)>>16)&0xff);
}

unsigned int
gd_color_get_g(GdColor color)
{
	return ((gd_color_get_rgb(color)>>8)&0xff);
}

unsigned int
gd_color_get_b(GdColor color)
{
	return ((gd_color_get_rgb(color)>>0)&0xff);
}

/* make up GdColor from r,g,b values. */
GdColor
gd_color_get_from_rgb(int r, int g, int b)
{
	return (r<<16)+(g<<8)+b;
}


GdColor
gd_color_get_from_string(const char *color)
{
	int i, r, g, b;

	for (i=0; i<G_N_ELEMENTS(c64_color_names); i++)
		if (g_ascii_strcasecmp(color, c64_color_names[i])==0)
			return gd_c64_color(i);

	/* may or may not have a # */
	if (color[0]=='#')
		color++;
	if (sscanf(color, "%02x%02x%02x", &r, &g, &b)!=3) {
		i=g_random_int_range(0, 16);
		g_warning("Unkonwn color %s, using randomly chosen %s\n", color, c64_color_names[i]);
		return gd_c64_color(i);
	}

	return gd_color_get_from_rgb(r, g, b);
}

const char*
gd_color_get_string(GdColor color)
{
	static char text[16];

	if (gd_color_is_c64(color)) {
		g_assert((color & 0xff)<G_N_ELEMENTS(c64_color_names));
		return c64_color_names[color&0xff];
	}

	sprintf(text, "#%02x%02x%02x", (color>>16)&255, (color>>8)&255, color&255);
	return text;
}

const char*
gd_color_get_visible_name(GdColor color)
{
	static char text[16];

	if (gd_color_is_c64(color)) {
		g_assert((color & 0xff)<G_N_ELEMENTS(c64_color_names));
		return c64_color_visible_names[color&0xff];
	}

	sprintf(text, "#%02x%02x%02x", (color>>16)&255, (color>>8)&255, color&255);
	return text;
}

gboolean
gd_color_is_c64(GdColor color)
{
	return (color>>24)==1;
}

/* get c64 color index from color; terminate app if not a c64 color. */
int gd_color_get_c64_index(GdColor color)
{
	g_assert(gd_color_is_c64(color));
	
	return (color & 0xf);
}

/* get c64 color index from color; but do not terminate if not found. */
int
gd_color_get_c64_index_try(GdColor color)
{
	if (gd_color_is_c64(color))
		return gd_color_get_c64_index(color);
	g_warning("Non-C64 color: %s", gd_color_get_string(color));

	return g_random_int_range(0, 16);
}

