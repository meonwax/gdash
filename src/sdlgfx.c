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
#include <SDL.h>
#include <SDL_image.h>
#include <glib.h>
#include "colors.h"
#include "cave.h"
#include "gameplay.h"
#include "settings.h"
#include "sdlgfx.h"
#include "util.h"
#include "c64_gfx.h"	/* char c64_gfx[] with (almost) original graphics */
#include "title.h"
#include "c64_font.h"
#include "gfxutil.h"
#include "caveset.h" 	/* UGLY */

#include "c64_png_colors.h"

#define NUM_OF_CHARS 128

int gd_scale=1;	/* a graphics scale things which sets EVERYTHING. it is set with gd_sdl_init, and cannot be modified later. */
int gd_scale_type=GD_SCALING_ORIGINAL;

static SDL_Surface *cells[2*NUM_OF_CELLS];
static const guchar *font;
static GHashTable *font_w, *font_n;
static GList *font_color_recently_used;
static GdColor color0, color1, color2, color3, color4, color5;	/* currently used cell colors */
static guint8 *c64_custom_gfx=NULL;
static gboolean using_png_gfx;

/* these masks are always for RGB and RGBA ordering in memory.
   that is what gdk-pixbuf uses, and also what sdl uses.
   no BGR, no ABGR.
 */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    static guint32 rmask = 0xff000000;
    static guint32 gmask = 0x00ff0000;
    static guint32 bmask = 0x0000ff00;
    static guint32 amask = 0x000000ff;
#else
    static guint32 rmask = 0x000000ff;
    static guint32 gmask = 0x0000ff00;
    static guint32 bmask = 0x00ff0000;
    static guint32 amask = 0xff000000;
#endif


/* screen area */
SDL_Surface *gd_screen=NULL;
static SDL_Surface *dark_background=NULL;
static GList *backup_screens=NULL, *dark_screens=NULL;
static int play_area_w=320;
static int play_area_h=180;
int gd_statusbar_height=20;
int gd_statusbar_y1=1;
int gd_statusbar_y2=10;
int gd_statusbar_mid=(20-8)/2;
static int scroll_x, scroll_y;


/* quit, global variable which is set to true if the application should quit */
gboolean gd_quit=FALSE;

guint8 *gd_keystate;
static SDL_Joystick *joystick_1;

static int cell_size=16;







/* return name of key. names taken from sdl documentation */
/*
	w3m -dump file:///usr/share/doc/libsdl1.2-dev/docs/html/sdlkey.html |grep -v "──" |
		cut -c 1-19,38- |sed "s/│$//;s/ *$//g;s/│/case /;s/ *│/: return \"/;s/$/\";/"|tee ~/keys.txt
*/
const char *
gd_key_name(guint keysym)
{
	static char keyname[30];
	switch(keysym) {
		case SDLK_BACKSPACE: return "BACKSPACE";
		case SDLK_TAB: return "TAB";
		case SDLK_CLEAR: return "CLEAR";
		case SDLK_RETURN: return "RETURN";
		case SDLK_PAUSE: return "PAUSE";
		case SDLK_ESCAPE: return "ESCAPE";
		case SDLK_SPACE: return "SPACE";
		case SDLK_EXCLAIM: return "EXCLAIM";
		case SDLK_QUOTEDBL: return "QUOTEDBL";
		case SDLK_HASH: return "HASH";
		case SDLK_DOLLAR: return "DOLLAR";
		case SDLK_AMPERSAND: return "AMPERSAND";
		case SDLK_QUOTE: return "QUOTE";
		case SDLK_LEFTPAREN: return "LEFT PARENTHESIS";
		case SDLK_RIGHTPAREN: return "RIGHT PARENTHESIS";
		case SDLK_ASTERISK: return "ASTERISK";
		case SDLK_PLUS: return "PLUS SIGN";
		case SDLK_COMMA: return "COMMA";
		case SDLK_MINUS: return "MINUS SIGN";
		case SDLK_PERIOD: return "PERIOD";
		case SDLK_SLASH: return "FORWARD SLASH";
		case SDLK_0: return "0";
		case SDLK_1: return "1";
		case SDLK_2: return "2";
		case SDLK_3: return "3";
		case SDLK_4: return "4";
		case SDLK_5: return "5";
		case SDLK_6: return "6";
		case SDLK_7: return "7";
		case SDLK_8: return "8";
		case SDLK_9: return "9";
		case SDLK_COLON: return "COLON";
		case SDLK_SEMICOLON: return "SEMICOLON";
		case SDLK_LESS: return "LESS-THAN SIGN";
		case SDLK_EQUALS: return "EQUALS SIGN";
		case SDLK_GREATER: return "GREATER-THAN SIGN";
		case SDLK_QUESTION: return "QUESTION MARK";
		case SDLK_AT: return "AT";
		case SDLK_LEFTBRACKET: return "LEFT BRACKET";
		case SDLK_BACKSLASH: return "BACKSLASH";
		case SDLK_RIGHTBRACKET: return "RIGHT BRACKET";
		case SDLK_CARET: return "CARET";
		case SDLK_UNDERSCORE: return "UNDERSCORE";
		case SDLK_BACKQUOTE: return "GRAVE";
		case SDLK_a: return "A";
		case SDLK_b: return "B";
		case SDLK_c: return "C";
		case SDLK_d: return "D";
		case SDLK_e: return "E";
		case SDLK_f: return "F";
		case SDLK_g: return "G";
		case SDLK_h: return "H";
		case SDLK_i: return "I";
		case SDLK_j: return "J";
		case SDLK_k: return "K";
		case SDLK_l: return "L";
		case SDLK_m: return "M";
		case SDLK_n: return "N";
		case SDLK_o: return "O";
		case SDLK_p: return "P";
		case SDLK_q: return "Q";
		case SDLK_r: return "R";
		case SDLK_s: return "S";
		case SDLK_t: return "T";
		case SDLK_u: return "U";
		case SDLK_v: return "V";
		case SDLK_w: return "W";
		case SDLK_x: return "X";
		case SDLK_y: return "Y";
		case SDLK_z: return "Z";
		case SDLK_DELETE: return "DELETE";
		case SDLK_KP0: return "KEYPAD 0";
		case SDLK_KP1: return "KEYPAD 1";
		case SDLK_KP2: return "KEYPAD 2";
		case SDLK_KP3: return "KEYPAD 3";
		case SDLK_KP4: return "KEYPAD 4";
		case SDLK_KP5: return "KEYPAD 5";
		case SDLK_KP6: return "KEYPAD 6";
		case SDLK_KP7: return "KEYPAD 7";
		case SDLK_KP8: return "KEYPAD 8";
		case SDLK_KP9: return "KEYPAD 9";
		case SDLK_KP_PERIOD: return "KEYPAD PERIOD";
		case SDLK_KP_DIVIDE: return "KEYPAD DIVIDE";
		case SDLK_KP_MULTIPLY: return "KEYPAD MULTIPLY";
		case SDLK_KP_MINUS: return "KEYPAD MINUS";
		case SDLK_KP_PLUS: return "KEYPAD PLUS";
		case SDLK_KP_ENTER: return "KEYPAD ENTER";
		case SDLK_KP_EQUALS: return "KEYPAD EQUALS";
		case SDLK_UP: return "UP ARROW";
		case SDLK_DOWN: return "DOWN ARROW";
		case SDLK_RIGHT: return "RIGHT ARROW";
		case SDLK_LEFT: return "LEFT ARROW";
		case SDLK_INSERT: return "INSERT";
		case SDLK_HOME: return "HOME";
		case SDLK_END: return "END";
		case SDLK_PAGEUP: return "PAGE UP";
		case SDLK_PAGEDOWN: return "PAGE DOWN";
		case SDLK_F1: return "F1";
		case SDLK_F2: return "F2";
		case SDLK_F3: return "F3";
		case SDLK_F4: return "F4";
		case SDLK_F5: return "F5";
		case SDLK_F6: return "F6";
		case SDLK_F7: return "F7";
		case SDLK_F8: return "F8";
		case SDLK_F9: return "F9";
		case SDLK_F10: return "F10";
		case SDLK_F11: return "F11";
		case SDLK_F12: return "F12";
		case SDLK_F13: return "F13";
		case SDLK_F14: return "F14";
		case SDLK_F15: return "F15";
		case SDLK_NUMLOCK: return "NUMLOCK";
		case SDLK_CAPSLOCK: return "CAPSLOCK";
		case SDLK_SCROLLOCK: return "SCROLLOCK";
		case SDLK_RSHIFT: return "RIGHT SHIFT";
		case SDLK_LSHIFT: return "LEFT SHIFT";
		case SDLK_RCTRL: return "RIGHT CTRL";
		case SDLK_LCTRL: return "LEFT CTRL";
		case SDLK_RALT: return "RIGHT ALT";
		case SDLK_LALT: return "LEFT ALT";
		case SDLK_RMETA: return "RIGHT META";
		case SDLK_LMETA: return "LEFT META";
		case SDLK_LSUPER: return "LEFT WINDOWS KEY";
		case SDLK_RSUPER: return "RIGHT WINDOWS KEY";
		case SDLK_MODE: return "MODE SHIFT";
		case SDLK_HELP: return "HELP";
		case SDLK_PRINT: return "PRINT-SCREEN";
		case SDLK_SYSREQ: return "SYSRQ";
		case SDLK_BREAK: return "BREAK";
		case SDLK_MENU: return "MENU";
		case SDLK_POWER: return "POWER";
		case SDLK_EURO: return "EURO";
		default:
			sprintf(keyname, "KEY %04X", keysym);
			return g_intern_string(keyname);	/* abuse? :) */
	}
}



#if 0
/* read a gdk-pixbuf source, and return an sdl surface. */
/* these masks are always for RGB and RGBA ordering in memory.
   that is what gdk-pixbuf uses, and also what sdl uses.
   no BGR, no ABGR.
 */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    static guint32 rmask = 0xff000000;
    static guint32 gmask = 0x00ff0000;
    static guint32 bmask = 0x0000ff00;
    static guint32 amask = 0x000000ff;

	/* for non-alpha channel gdk pixbuf includes */
    static guint32 rmask_24 = 0xff0000;
    static guint32 gmask_24 = 0x00ff00;
    static guint32 bmask_24 = 0x0000ff;
#else
    static guint32 rmask = 0x000000ff;
    static guint32 gmask = 0x0000ff00;
    static guint32 bmask = 0x00ff0000;
    static guint32 amask = 0xff000000;

	/* for non-alpha channel gdk pixbuf includes */
    static guint32 rmask_24 = 0x0000ff;
    static guint32 gmask_24 = 0x00ff00;
    static guint32 bmask_24 = 0xff0000;
#endif
static SDL_Surface *
surface_from_gdk_pixbuf_data(guint32 *data)
{
	SDL_Surface *surface;
	
	g_assert(GUINT32_FROM_BE(data[0])==0x47646b50);	/* gdk-pixbuf magic number */
	if (GUINT32_FROM_BE(data[2])==0x1010002)	/* 32-bit rgba */
		surface=SDL_CreateRGBSurfaceFrom(data+6, GUINT32_FROM_BE(data[4]), GUINT32_FROM_BE(data[5]), 32, GUINT32_FROM_BE(data[3]), rmask, gmask, bmask, amask);
	else
	if (GUINT32_FROM_BE(data[2])==0x1010001)	/* 24-bit rgb */
		surface=SDL_CreateRGBSurfaceFrom(data+6, GUINT32_FROM_BE(data[4]), GUINT32_FROM_BE(data[5]), 24, GUINT32_FROM_BE(data[3]), rmask_24, gmask_24, bmask_24, 0);
	else
		/* unknown pixel format */
		g_assert_not_reached();
	g_assert(surface!=NULL);
	
	return surface;
}
#endif

static SDL_Surface *
surface_from_raw_data(const guint8 *data, int length)
{
	SDL_RWops *rwop;
	SDL_Surface *surface;

	rwop=SDL_RWFromConstMem(data, length);
	surface=IMG_Load_RW(rwop, 1);	/* 1 = automatically closes rwop */
	return surface;
}

static SDL_Surface *
surface_from_base64(const char *base64)
{
	guchar *data;
	gsize length;
	SDL_Surface *surface;
	
	data=g_base64_decode(base64, &length);
	surface=surface_from_raw_data(data, length);
	g_free(data);
	
	return surface;
}

/* This is taken from the SDL_gfx project. */
/*  
	SDL_rotozoom.c - rotozoomer for 32bit or 8bit surfaces
	LGPL (c) A. Schiffler
	32bit Zoomer with optional anti-aliasing by bilinear interpolation.
	Zoomes 32bit RGBA/ABGR 'src' surface to 'dst' surface.
*/

static void
zoomSurfaceRGBA(SDL_Surface *src, SDL_Surface *dst, gboolean smooth)
{
	typedef struct tColorRGBA {
		guint8 r;
		guint8 g;
		guint8 b;
		guint8 a;
	} tColorRGBA;

	int x, y, sx, sy, *sax, *say, *csax, *csay, csx, csy, ex, ey, t1, t2, sstep;
	tColorRGBA *c00, *c01, *c10, *c11;
	tColorRGBA *sp, *csp, *dp;
	int dgap;

	g_assert(src->format->BytesPerPixel==4);
	g_assert(dst->format->BytesPerPixel==4);

	/* Variable setup */
	if (smooth) {
		/* For interpolation: assume source dimension is one pixel */
		/* smaller to avoid overflow on right and bottom edge. */
		sx = (int) (65536.0 * (float) (src->w - 1) / (float) dst->w);
		sy = (int) (65536.0 * (float) (src->h - 1) / (float) dst->h);
	} else {
		sx = (int) (65536.0 * (float) src->w / (float) dst->w);
		sy = (int) (65536.0 * (float) src->h / (float) dst->h);
	}

	/* Allocate memory for row increments */
	sax=g_new(gint32, dst->w+1);
	say=g_new(gint32, dst->h+1);

	/* Precalculate row increments */
	if (SDL_MUSTLOCK(src))
		SDL_LockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_LockSurface(dst);
	sp = csp = (tColorRGBA *) src->pixels;
	dp = (tColorRGBA *) dst->pixels;

	csx = 0;
	csax = sax;
	for (x = 0; x <= dst->w; x++) {
		*csax = csx;
		csax++;
		csx &= 0xffff;
		csx += sx;
	}
	csy = 0;
	csay = say;
	for (y = 0; y <= dst->h; y++) {
		*csay = csy;
		csay++;
		csy &= 0xffff;
		csy += sy;
	}

	dgap = dst->pitch - dst->w * 4;

	/*
     * Switch between interpolating and non-interpolating code 
     */
	if (smooth) {
		/* Interpolating Zoom */

		/* Scan destination */
		csay = say;
		for (y = 0; y < dst->h; y++) {
		    /* Setup color source pointers */
		    c00 = csp;
		    c01 = csp;
		    c01++;
		    c10 = (tColorRGBA *) ((guint8 *) csp + src->pitch);
		    c11 = c10;
		    c11++;
		    csax = sax;
		    for (x = 0; x < dst->w; x++) {
				/* Interpolate colors */
				ex = (*csax & 0xffff);
				ey = (*csay & 0xffff);
				t1 = ((((c01->r - c00->r) * ex) >> 16) + c00->r) & 0xff;
				t2 = ((((c11->r - c10->r) * ex) >> 16) + c10->r) & 0xff;
				dp->r = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->g - c00->g) * ex) >> 16) + c00->g) & 0xff;
				t2 = ((((c11->g - c10->g) * ex) >> 16) + c10->g) & 0xff;
				dp->g = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->b - c00->b) * ex) >> 16) + c00->b) & 0xff;
				t2 = ((((c11->b - c10->b) * ex) >> 16) + c10->b) & 0xff;
				dp->b = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->a - c00->a) * ex) >> 16) + c00->a) & 0xff;
				t2 = ((((c11->a - c10->a) * ex) >> 16) + c10->a) & 0xff;
				dp->a = (((t2 - t1) * ey) >> 16) + t1;

				/* Advance source pointers */
				csax++;
				sstep = (*csax >> 16);
				c00 += sstep;
				c01 += sstep;
				c10 += sstep;
				c11 += sstep;
				/* Advance destination pointer */
				dp++;
		    }
		    /* Advance source pointer */
		    csay++;
		    csp = (tColorRGBA *) ((guint8 *) csp + (*csay >> 16) * src->pitch);
		    /* Advance destination pointers */
		    dp = (tColorRGBA *) ((guint8 *) dp + dgap);
		}
	} else {
		/* Non-Interpolating Zoom */
		csay = say;
		for (y = 0; y < dst->h; y++) {
		    sp = csp;
		    csax = sax;
		    for (x = 0; x < dst->w; x++) {
				/* Draw */
				*dp = *sp;
				/* Advance source pointers */
				csax++;
				sstep = (*csax >> 16);
				sp += sstep;
				/* Advance destination pointer */
				dp++;
		    }
		    /* Advance source pointer */
		    csay++;
		    sstep = (*csay >> 16) * src->pitch;
		    csp = (tColorRGBA *) ((guint8 *) csp + sstep);

		    /* Advance destination pointers */
		    dp = (tColorRGBA *) ((guint8 *) dp + dgap);
		}

	}

	if (SDL_MUSTLOCK(src))
		SDL_UnlockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_UnlockSurface(dst);

	/* Remove temp arrays */
	g_free(sax);
	g_free(say);
}


/* wrapper */
static void
scale2x(SDL_Surface *src, SDL_Surface *dst)
{
	g_assert(dst->w == src->w*2);
	g_assert(dst->h == src->h*2);
	g_assert(src->format->BytesPerPixel==4);
	g_assert(dst->format->BytesPerPixel==4);
	
	if (SDL_MUSTLOCK(src))
		SDL_LockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_LockSurface(dst);
	gd_scale2x_raw(src->pixels, src->w, src->h, src->pitch, dst->pixels, dst->pitch);
	if (SDL_MUSTLOCK(src))
		SDL_UnlockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_UnlockSurface(dst);
}

static void
scale3x(SDL_Surface *src, SDL_Surface *dst)
{
	g_assert(dst->w == src->w*3);
	g_assert(dst->h == src->h*3);
	g_assert(src->format->BytesPerPixel==4);
	g_assert(dst->format->BytesPerPixel==4);
	
	if (SDL_MUSTLOCK(src))
		SDL_LockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_LockSurface(dst);
	gd_scale3x_raw(src->pixels, src->w, src->h, src->pitch, dst->pixels, dst->pitch);
	if (SDL_MUSTLOCK(src))
		SDL_UnlockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_UnlockSurface(dst);
}





/* nearest neighbor scaling for 2x and 3x. */
/* nearest pixel 2x scaling. */
static void
scale_2x_nearest(SDL_Surface *src, SDL_Surface *dst)
{
   	guint32 E;
	int y, x;
	guint8 *srcpix=src->pixels;
	guint8 *dstpix=dst->pixels;
	int width=src->w;
	int height=src->h;
	int srcpitch=src->pitch;
	int dstpitch=dst->pitch;

	g_assert(dst->w == src->w*2);
	g_assert(dst->h == src->h*2);
	g_assert(src->format->BytesPerPixel==4);
	g_assert(dst->format->BytesPerPixel==4);

	if (SDL_MUSTLOCK(src))
		SDL_LockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_LockSurface(dst);
	for (y=0; y<height; ++y) {
		for (x=0; x<width; ++x) {
			E = *(guint32*)(srcpix + (y*srcpitch) + (4*x));

			*(guint32*)(dstpix + y*2*dstpitch + x*2*4) = E;
			*(guint32*)(dstpix + y*2*dstpitch + (x*2+1)*4) = E;
			*(guint32*)(dstpix + (y*2+1)*dstpitch + x*2*4) = E;
			*(guint32*)(dstpix + (y*2+1)*dstpitch + (x*2+1)*4) = E;
		}
	}
	if (SDL_MUSTLOCK(src))
		SDL_UnlockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_UnlockSurface(dst);
}


/* nearest pixel 3x scaling. the rotozoomer is not correct at the bottom of the image. */
static void
scale_3x_nearest(SDL_Surface *src, SDL_Surface *dst)
{
   	guint32 E;
	int y, x;
	guint8 *srcpix=src->pixels;
	guint8 *dstpix=dst->pixels;
	int width=src->w;
	int height=src->h;
	int srcpitch=src->pitch;
	int dstpitch=dst->pitch;

	g_assert(dst->w == src->w*3);
	g_assert(dst->h == src->h*3);
	g_assert(src->format->BytesPerPixel==4);
	g_assert(dst->format->BytesPerPixel==4);

	if (SDL_MUSTLOCK(src))
		SDL_LockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_LockSurface(dst);
	for (y=0; y<height; ++y) {
		int ny=y*3;	/* new coordinate */

		for (x=0; x<width; ++ x) {
			int nx=x*3;	/* new coordinate */

			E = *(guint32*)(srcpix + (y*srcpitch + 4*x));

			*(guint32*)(dstpix + ny*dstpitch + nx*4) = E;
			*(guint32*)(dstpix + ny*dstpitch + (nx+1)*4) = E;
			*(guint32*)(dstpix + ny*dstpitch + (nx+2)*4) = E;
			*(guint32*)(dstpix + (ny+1)*dstpitch + nx*4) = E;
			*(guint32*)(dstpix + (ny+1)*dstpitch + (nx+1)*4) = E;
			*(guint32*)(dstpix + (ny+1)*dstpitch + (nx+2)*4) = E;
			*(guint32*)(dstpix + (ny+2)*dstpitch + nx*4) = E;
			*(guint32*)(dstpix + (ny+2)*dstpitch + (nx+1)*4) = E;
			*(guint32*)(dstpix + (ny+2)*dstpitch + (nx+2)*4) = E;
		}
	}
	if (SDL_MUSTLOCK(src))
		SDL_UnlockSurface(src);
	if (SDL_MUSTLOCK(dst))
		SDL_UnlockSurface(dst);
}



/* scales a pixbuf with the appropriate scaling type. */
static SDL_Surface *
surface_scale(SDL_Surface *orig)
{
	SDL_Surface *dest, *dest2x;
	
	/* special case: no scaling. */
	/* return a new pixbuf, but its pixels are at the same place as the original. (so the pixbuf struct can be freed on its own) */
	if (gd_scale_type==GD_SCALING_ORIGINAL) {
		g_assert(gd_scale==1);	/* just to be sure */
		
		return SDL_CreateRGBSurfaceFrom(orig->pixels, orig->w, orig->h, orig->format->BitsPerPixel, orig->pitch, orig->format->Rmask, orig->format->Gmask, orig->format->Bmask, orig->format->Amask);
	}

	dest=SDL_CreateRGBSurface(orig->flags, orig->w*gd_scale, orig->h*gd_scale, orig->format->BitsPerPixel, orig->format->Rmask, orig->format->Gmask, orig->format->Bmask, orig->format->Amask);
	
	switch (gd_scale_type) {
		case GD_SCALING_ORIGINAL:
			/* handled above */
			g_assert_not_reached();
			break;
		
		case GD_SCALING_2X:
			scale_2x_nearest(orig, dest);
			break;

		case GD_SCALING_2X_BILINEAR:
			zoomSurfaceRGBA(orig, dest, TRUE);
			break;

		case GD_SCALING_2X_SCALE2X:
			scale2x(orig, dest);
			break;

		case GD_SCALING_3X:
			scale_3x_nearest(orig, dest);
			break;

		case GD_SCALING_3X_BILINEAR:
			zoomSurfaceRGBA(orig, dest, TRUE);
			break;

		case GD_SCALING_3X_SCALE3X:
			scale3x(orig, dest);
			break;
			
		case GD_SCALING_4X:
			dest2x=SDL_CreateRGBSurface(orig->flags, orig->w*2, orig->h*2, orig->format->BitsPerPixel, orig->format->Rmask, orig->format->Gmask, orig->format->Bmask, orig->format->Amask);
			scale_2x_nearest(orig, dest2x);
			scale_2x_nearest(dest2x, dest);
			SDL_FreeSurface(dest2x);
			break;

		case GD_SCALING_4X_BILINEAR:
			zoomSurfaceRGBA(orig, dest, TRUE);
			break;

		case GD_SCALING_4X_SCALE4X:
			/* scale2x applied twice. */
			dest2x=SDL_CreateRGBSurface(orig->flags, orig->w*2, orig->h*2, orig->format->BitsPerPixel, orig->format->Rmask, orig->format->Gmask, orig->format->Bmask, orig->format->Amask);
			scale2x(orig, dest2x);
			scale2x(dest2x, dest);
			SDL_FreeSurface(dest2x);
			break;
			
		/* not a valid case, but to avoid compiler warning */
		case GD_SCALING_MAX:
			g_assert_not_reached();
			break;
	}
	
	return dest;
}


/* wrapper */
static void
pal_emu_surface(SDL_Surface *image)
{
	if (SDL_MUSTLOCK(image))
		SDL_LockSurface(image);
	gd_pal_emulate_raw(image->pixels, image->w, image->h, image->pitch, image->format->Rshift, image->format->Gshift, image->format->Bshift, image->format->Ashift);
	if (SDL_MUSTLOCK(image))
		SDL_UnlockSurface(image);
}


/* create new surface, which is SDL_DisplayFormatted. */
/* it is also scaled by gd_scale. */
static SDL_Surface *
displayformat(SDL_Surface *orig)
{
	SDL_Surface *scaled, *displayformat;

	/* at this point we must already be working with 32bit surfaces */	
	g_assert(orig->format->BytesPerPixel==4);
	
	scaled=surface_scale(orig);
	if (gd_sdl_pal_emulation)
		pal_emu_surface(scaled);
	displayformat=SDL_DisplayFormat(scaled);
	SDL_FreeSurface(scaled);

	return displayformat;
}

static SDL_Surface *
displayformatalpha(SDL_Surface *orig)
{
	SDL_Surface *scaled, *displayformat;

	/* at this point we must already be working with 32bit surfaces */	
	g_assert(orig->format->BytesPerPixel==4);
	
	scaled=surface_scale(orig);
	if (gd_sdl_pal_emulation)
		pal_emu_surface(scaled);
	displayformat=SDL_DisplayFormatAlpha(scaled);
	SDL_FreeSurface(scaled);

	return displayformat;
}

#if 0
/* needed to alpha-copy an src image over dst.
   the resulting image of sdl_blitsurface is not exact?!
   can't explain why. XXX. */
static void
copy_alpha(SDL_Surface *src, SDL_Surface *dst)
{
	int x, y;
	
	g_assert(src->format->BytesPerPixel==4);
	g_assert(dst->format->BytesPerPixel==4);
	g_assert(src->w == dst->w);
	g_assert(src->h == dst->h);
	
	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	for (y=0; y<src->h; y++) {
		guint32 *srcpix=(guint32 *)((guint8*)src->pixels + src->pitch*y);
		guint32 *dstpix=(guint32 *)((guint8*)dst->pixels + dst->pitch*y);
		for (x=0; x<src->w; x++) {
			if ((srcpix[x]&src->format->Amask)>>src->format->Ashift)	/* if alpha is nonzero */
				dstpix[x]=srcpix[x];	/* copy pixel as it is */
		}
	}
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);
}
#endif

/* create and return an array of surfaces, which contain the title animation.
   the array is one pointer larger than all frames; the last pointer is a null.
   up to the caller to free.
 */
SDL_Surface **
gd_get_title_animation()
{
	SDL_Surface *screen;
	SDL_Surface *tile;
	SDL_Surface *bigone;
	SDL_Surface *frame;
	SDL_Surface **animation;
	int x, y, i;
	
	screen=NULL;
	tile=NULL;
	if (gd_caveset_data->title_screen->len!=0) {
		/* user defined title screen */
		screen=surface_from_base64(gd_caveset_data->title_screen->str);
		if (!screen) {
			g_warning("Caveset is storing an invalid title screen image.");
			g_string_assign(gd_caveset_data->title_screen, "");
		} else {
			/* if we loaded the screen, now try to load the tile. */
			/* only if the screen has an alpha channel. otherwise it would not make any sense */
			if (screen->format->Amask!=0 && gd_caveset_data->title_screen_scroll->len!=0) {
				tile=surface_from_base64(gd_caveset_data->title_screen_scroll->str);
				
				if (!tile) {
					g_warning("Caveset is storing an invalid title screen background image.");
					g_string_assign(gd_caveset_data->title_screen_scroll, "");
				}
			}
		}
		
	}

	/* if no special title image or unable to load that one, load the built-in */
	if (!screen) {
		/* the screen */
		screen=surface_from_raw_data(gdash_screen, sizeof(gdash_screen));
		/* the tile to be put under the screen */
		tile=surface_from_raw_data(gdash_tile, sizeof(gdash_tile));
		g_assert(screen!=NULL);
		g_assert(tile!=NULL);
	}	

	/* if no tile, let it be black. */
	/* the sdl version does the same. */
	if (!tile) {
		/* one-row pixbuf, so no animation. */
		tile=SDL_CreateRGBSurface(0, screen->w, 1, 32, 0, 0, 0, 0);
		SDL_FillRect(tile, NULL, SDL_MapRGB(tile->format, 0, 0, 0));
	}

	/* do not allow more than 40 frames of animation */
	g_assert(tile->h<40);
	animation=g_new0(SDL_Surface *, tile->h+1);
	
	/* create a big image, which is one tile larger than the title image size */
	bigone=SDL_CreateRGBSurface(0, screen->w, screen->h+tile->h, 32, 0, 0, 0, 0);
	/* and fill it with the tile. */
	for (y=0; y<screen->h+tile->h; y+=tile->h)
		for (x=0; x<screen->w; x+=tile->h) {
			SDL_Rect dest;
			
			dest.x=x;
			dest.y=y;
			SDL_BlitSurface(tile, 0, bigone, &dest);
		}
	
	frame=SDL_CreateRGBSurface(0, screen->w, screen->h, 32, rmask, gmask, bmask, amask);	/* must be same *mask so copy_alpha works correctly */
	for (i=0; i<tile->h; i++) {
		SDL_Rect src;
		
		/* copy part of the big tiled image */
		src.x=0;
		src.y=i;
		src.w=screen->w;
		src.h=screen->h;
		SDL_BlitSurface(bigone, &src, frame, 0);
		/* and composite it with the title image */
//		copy_alpha(screen, frame);
		SDL_BlitSurface(screen, NULL, frame, NULL);
		animation[i]=displayformat(frame);
	}
	SDL_FreeSurface(frame);
	SDL_FreeSurface(bigone);
	SDL_FreeSurface(screen);
	SDL_FreeSurface(tile);

	return animation;
}

static void
rendered_font_free(gpointer font)
{
	SDL_Surface **p;

	/* null-terminated list of pointers to sdl_surfaces */	
	for(p=font; p!=NULL; p++)
		SDL_FreeSurface(*p);
	g_free(font);
}


gboolean
gd_sdl_init(GdScalingType scaling_type)
{
	SDL_Surface *icon;
	
	/* set the cell scale option. */
	gd_scale_type=scaling_type;
	gd_scale=gd_scaling_scale[gd_scale_type];
	play_area_w*=gd_scale;
	play_area_h*=gd_scale;
	gd_statusbar_y1*=gd_scale;
	gd_statusbar_y2*=gd_scale;
	gd_statusbar_height*=gd_scale;
	gd_statusbar_mid*=gd_scale;

	SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_JOYSTICK);
	SDL_EnableKeyRepeat(250, 100);
	gd_screen=SDL_SetVideoMode(play_area_w, play_area_h+gd_statusbar_height, 32, SDL_DOUBLEBUF | SDL_ANYFORMAT | (gd_sdl_fullscreen?SDL_FULLSCREEN:0));
	/* do not show mouse cursor */
	SDL_ShowCursor(SDL_DISABLE);
	/* warp mouse pointer so cursor cannot be seen, if the above call did nothing for some reason */
	SDL_WarpMouse(gd_screen->w-1, gd_screen->h-1);
	if (!gd_screen)
		return FALSE;
	
	/* keyboard, joystick */
	gd_keystate=SDL_GetKeyState(NULL);
	if (SDL_NumJoysticks()>0)
		joystick_1=SDL_JoystickOpen(0);

	/* icon */
	icon=surface_from_raw_data(gdash_32, sizeof(gdash_32));
	SDL_WM_SetIcon(icon, NULL);
	SDL_WM_SetCaption("GDash", NULL);
	SDL_FreeSurface(icon);
	
	font_n=g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, rendered_font_free);
	font_w=g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, rendered_font_free);
	font_color_recently_used=NULL;
	
	return TRUE;
}




static inline gboolean
get_joy_up()
{
	return joystick_1!=NULL && SDL_JoystickGetAxis(joystick_1, 1)<-3200;
}

static inline gboolean
get_joy_down()
{
	return joystick_1!=NULL && SDL_JoystickGetAxis(joystick_1, 1)>3200;
}

static inline gboolean
get_joy_left()
{
	return joystick_1!=NULL && SDL_JoystickGetAxis(joystick_1, 0)<-3200;
}

static inline gboolean
get_joy_right()
{
	return joystick_1!=NULL && SDL_JoystickGetAxis(joystick_1, 0)>3200;
}

static inline gboolean
get_joy_fire()
{
	return joystick_1!=NULL && (SDL_JoystickGetButton(joystick_1, 0) || SDL_JoystickGetButton(joystick_1, 1));
}




gboolean
gd_up()
{
	return gd_keystate[gd_sdl_key_up] || get_joy_up();
}

gboolean
gd_down()
{
	return gd_keystate[gd_sdl_key_down] || get_joy_down();
}

gboolean
gd_left()
{
	return gd_keystate[gd_sdl_key_left] || get_joy_left();
}

gboolean
gd_right()
{
	return gd_keystate[gd_sdl_key_right] || get_joy_right();
}

gboolean
gd_fire()
{
	return gd_keystate[gd_sdl_key_fire_1] || gd_keystate[gd_sdl_key_fire_2] || get_joy_fire();
}




/* like the above one, but used in the main menu */
gboolean
gd_space_or_enter_or_fire()
{
	/* set these to zero, so each keypress is reported only once */
	return get_joy_fire() || gd_keystate[SDLK_SPACE] || gd_keystate[SDLK_RETURN];
}


/* process pending events, so presses and releases are applied */
/* also check for quit event */
void
gd_process_pending_events()
{
	SDL_Event event;
	
	/* process pending events, so presses and releases are applied */
	while (SDL_PollEvent(&event)) {
		/* meanwhile, if we receive a quit event, we set the global variable. */
		if (event.type==SDL_QUIT)
			gd_quit=TRUE;
	}
}



/* this function waits until SPACE, ENTER and ESCAPE are released */
void
gd_wait_for_key_releases()
{
	/* wait until the user releases return and escape */
	while (gd_keystate[SDLK_RETURN]!=0 || gd_keystate[SDLK_ESCAPE]!=0 || gd_keystate[SDLK_SPACE]!=0 || gd_keystate[SDLK_n]!=0 || gd_fire()) {
		SDL_Event event;

		/* process pending events, so presses and releases are applied */
		while (SDL_PollEvent(&event)) {
			/* meanwhile, if we receive a quit event, we set the global variable. */
			if (event.type==SDL_QUIT)
				gd_quit=TRUE;
		}
		
		SDL_Delay(50);	/* do not eat cpu */
	}
}











/* remove pixmaps from x server */
static void
free_cells()
{
	int i;
	
	/* if cells already loaded, unref them */
	for (i=0; i<G_N_ELEMENTS(cells); i++)
		if (cells[i]) {
			SDL_FreeSurface(cells[i]);
			cells[i]=NULL;
		}
}


static void
loadcells_from_surface(SDL_Surface *image)
{
	int i;
	int pixbuf_cell_size;
	SDL_Surface *rect;
	SDL_Surface *cut;

	/* if we have display-formatted pixmaps, remove them */
	free_cells();

	/* 8 (NUM_OF_CELLS_X) cells in a row, so divide by it and we get the size of a cell in pixels */
	pixbuf_cell_size=image->w/NUM_OF_CELLS_X;
	cell_size=pixbuf_cell_size*gd_scale;

	rect=SDL_CreateRGBSurface(0, pixbuf_cell_size, pixbuf_cell_size, 32, 0, 0, 0, 0);	/* no amask, as we set overall alpha! */
	SDL_FillRect(rect, NULL, SDL_MapRGB(rect->format, (gd_flash_color>>16)&0xff, (gd_flash_color>>8)&0xff, gd_flash_color&0xff));
	SDL_SetAlpha(rect, SDL_SRCALPHA, 128);	/* 50% alpha; nice choice. also sdl is rendering faster for the special value alpha=128 */

	cut=SDL_CreateRGBSurface(0, pixbuf_cell_size, pixbuf_cell_size, 32, rmask, gmask, bmask, amask);

	/* make individual cells */
	for (i=0; i<NUM_OF_CELLS_X*NUM_OF_CELLS_Y; i++) {
		SDL_Rect from;

		from.x=(i%NUM_OF_CELLS_X)*pixbuf_cell_size;
		from.y=(i/NUM_OF_CELLS_X)*pixbuf_cell_size;
		from.w=pixbuf_cell_size;
		from.h=pixbuf_cell_size;

		SDL_BlitSurface(image, &from, cut, NULL);

		cells[i]=displayformat(cut);
		SDL_BlitSurface(rect, NULL, cut, NULL);		/* create yellowish image */
		cells[NUM_OF_CELLS+i]=displayformat(cut);
	}
	SDL_FreeSurface(cut);
	SDL_FreeSurface(rect);
}

/* sets one of the colors in the indexed palette of an sdl surface to a GdColor. */
static void
surface_setpalette(SDL_Surface *image, int index, GdColor col)
{
	SDL_Color c;
	c.r=gd_color_get_r(col);
	c.g=gd_color_get_g(col);
	c.b=gd_color_get_b(col);
	
	SDL_SetPalette(image, SDL_LOGPAL|SDL_PHYSPAL, &c, index, 1);
}

static void
loadcells_from_c64_data(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5)
{
	const guint8 *gfx;	/* currently used graphics, will point to c64_gfx or c64_custom_gfx */
	SDL_Surface *image;

	gfx=c64_custom_gfx?c64_custom_gfx:c64_gfx;

	/* create a 8-bit palette and set its colors */
	/* gfx[0] is the pixel width and height of one cell. */
	/* from gfx[1], we have the color data, one byte/pixel. so this is an indexed color image: 8bit/pixel. */
	image=SDL_CreateRGBSurfaceFrom((void *)(gfx+1), NUM_OF_CELLS_X*(int)gfx[0], NUM_OF_CELLS_Y*(int)gfx[0], 8, NUM_OF_CELLS_X*(int)gfx[0], 0, 0, 0, 0);
	/* sdl supports paletted images, so this is very easy: */
	surface_setpalette(image, 0, 0);
	surface_setpalette(image, 1, c0);
	surface_setpalette(image, 2, c1);
	surface_setpalette(image, 3, c2);
	surface_setpalette(image, 4, c3);
	surface_setpalette(image, 5, c4);
	surface_setpalette(image, 6, c5);
	surface_setpalette(image, 7, 0);
	surface_setpalette(image, 8, 0);

	/* from here, same as any other image */
	loadcells_from_surface(image);
	SDL_FreeSurface(image);
}

/* takes a c64_gfx.png-coded 32-bit pixbuf, and creates a paletted pixbuf in our internal format. */
static guchar *
c64_gfx_data_from_pixbuf(SDL_Surface *image)
{
	int x, y;
	guint8 *data;
	int out;
	
	g_assert(image->format->BytesPerPixel==4);

	data=g_new(guint8, image->w*image->h+1);
	out=0;
	data[out++]=image->w/NUM_OF_CELLS_X;

	SDL_LockSurface(image);
	for (y=0; y<image->h; y++) {
		guint32 *p=(guint32 *)((char *)image->pixels+y*image->pitch);

		for (x=0; x<image->w; x++) {
			int r, g, b;

			r=(p[x]&image->format->Rmask) >> image->format->Rshift << image->format->Rloss;
			g=(p[x]&image->format->Gmask) >> image->format->Gshift << image->format->Gloss;
			b=(p[x]&image->format->Bmask) >> image->format->Bshift << image->format->Bloss;
			/* should be:
				a=(p[x]&image->format->Amask) >> image->format->Ashift << image->format->Aloss;
				but we do not use the alpha channel in sdash, so we just use 255 (max alpha)
			*/
			data[out++]=c64_png_colors(r, g, b, 255);
		}
	}
	SDL_UnlockSurface(image);
	
	return data;
}


/* returns true, if the given pixbuf seems to be a c64 imported image. */
static gboolean
check_if_pixbuf_c64_png(SDL_Surface *image)
{
	const guchar *p;
	int x, y;
	gboolean c64_png;

	g_assert(image->format->BytesPerPixel==4);	

	SDL_LockSurface(image);
	c64_png=TRUE;
	for (y=0; y<image->h; y++) {
		p=((guchar *)image->pixels) + y * image->pitch;
		for (x=0; x<image->w*image->format->BytesPerPixel; x++)
			if (p[x]!=0 && p[x]!=255)
				c64_png=FALSE;
	}
	SDL_UnlockSurface(image);

	return c64_png;
}

/* check if given surface is ok to be a gdash theme. */
gboolean
gd_is_surface_ok_for_theme(SDL_Surface *surface)
{
	if ((surface->w % NUM_OF_CELLS_X != 0) || (surface->h % NUM_OF_CELLS_Y != 0) || (surface->w / NUM_OF_CELLS_X != surface->h / NUM_OF_CELLS_Y)) {
		g_warning("image should contain %d cells in a row and %d in a column!", NUM_OF_CELLS_X, NUM_OF_CELLS_Y);
		return FALSE; /* Image should contain 16 cells in a row and 32 in a column! */
	}
	/* we do not check for an alpha channel, as in the gtk version. we do not need it here. */
	/* the gtk version needs it because of the editor. */

	return TRUE;	/* passed checks */
}


/* load theme from image file. */
/* return true if successful. */
gboolean
gd_loadcells_file(const char *filename)
{
	SDL_Surface *surface, *converted;

	/* load cell graphics */
	/* load from file */
	surface=IMG_Load(filename);

	if (!surface) {
		g_warning("%s: unable to load image", filename);
		return FALSE;
	}

	/* do some checks. if those fail, the error is already reported by the function */	
	if (!gd_is_surface_ok_for_theme(surface)) {
		SDL_FreeSurface(surface);
		return FALSE;
	}

	/* convert to 32bit rgba */
	converted=SDL_CreateRGBSurface(0, surface->w, surface->h, 32, rmask, gmask, bmask, amask);	/* 32bits per pixel, but we dont need an alpha channel */
	SDL_SetAlpha(surface, 0, 0);	/* do not use the alpha blending for the copy */
	SDL_BlitSurface(surface, NULL, converted, NULL);
	SDL_FreeSurface(surface);

	if (check_if_pixbuf_c64_png(converted)) {
		/* c64 pixbuf with a small number of colors which can be changed */
		g_free(c64_custom_gfx);
		c64_custom_gfx=c64_gfx_data_from_pixbuf(converted);
		using_png_gfx=FALSE;
	} else {
		/* normal, "truecolor" pixbuf */
		g_free(c64_custom_gfx);
		c64_custom_gfx=NULL;
		using_png_gfx=TRUE;
		loadcells_from_surface(converted);
	}
	SDL_FreeSurface(converted);
	color0=GD_COLOR_INVALID; /* this is an invalid gdash color; so redraw is forced */
	
	return TRUE;
}



/* load the theme specified in gd_sdl_theme. */
/* if successful, ok. */
/* if fails, or no theme specified, load the builtin, and forget gd_sdl_theme */
void
gd_load_theme()
{
	if (gd_sdl_theme) {
		if (!gd_loadcells_file(gd_sdl_theme)) {
			/* if failing to load the bmp file specified in the config file: forget the setting now, and load the builtin. */
			g_free(gd_sdl_theme);
			gd_sdl_theme=NULL;
		}
	}
	if (!gd_sdl_theme)
		gd_loadcells_default();		/* if no theme specified, or loading the file failed, simply load the builtin. */
}


/* selects built-in graphics */
void
gd_loadcells_default()
{
	g_free(c64_custom_gfx);
	c64_custom_gfx=NULL;
	using_png_gfx=FALSE;
	/* just to set some default */
	color0=GD_COLOR_INVALID; /* this is an invalid gdash color; so redraw is forced */
}




/* C64 FONT HANDLING */

/* creates a guint32 value, which can be raw-written to a sdl_surface memory area. */
/* takes the pixel format of the surface into consideration. */
static guint32
rgba_pixel_from_gdcolor(SDL_PixelFormat *format, GdColor col, guint8 a)
{
	return SDL_MapRGBA(format, gd_color_get_r(col), gd_color_get_g(col), gd_color_get_b(col), a);
}


/* FIXME fixed font size: 8x8 */
/* renders the fonts for a specific color. */
/* the wide font draws black pixels, the narrow (normal) font does not! */
static void
renderfont_color(GdColor color)
{
	int j, x, y;
	guint32 col, transparent;
	SDL_Surface *image, *image_n;
	SDL_Surface **fn, **fw;

	/* check that we already got an rgb color */	
	g_assert(gd_color_is_rgb(color));

	/* remove from list and add to the front */		
	font_color_recently_used=g_list_remove(font_color_recently_used, GUINT_TO_POINTER(color));
	font_color_recently_used=g_list_prepend(font_color_recently_used, GUINT_TO_POINTER(color));

	/* if already rendered, return now */
	if (g_hash_table_lookup(font_w, GUINT_TO_POINTER(color)))
		return;

	/* -------- RENDERING A NEW FONT -------- */
	/* if storing too many fonts, free the oldest one. */
	if (g_list_length(font_color_recently_used)>32) {
		GList *last;
		
		/* if list too big, remove least recently used */
		last=g_list_last(font_color_recently_used);
		g_hash_table_remove(font_w, last->data);
		g_hash_table_remove(font_n, last->data);
		font_color_recently_used=g_list_delete_link(font_color_recently_used, last);
	}
		
	fn=g_new0(SDL_Surface *, NUM_OF_CHARS+1);
	fw=g_new0(SDL_Surface *, NUM_OF_CHARS+1);
	
	/* colors data into memory */
	image=SDL_CreateRGBSurface(SDL_SRCALPHA, 16, 8, 32, rmask, gmask, bmask, amask);
	image_n=SDL_CreateRGBSurface(SDL_SRCALPHA, 8, 8, 32, rmask, gmask, bmask, amask);

	col=rgba_pixel_from_gdcolor(image->format, color, SDL_ALPHA_OPAQUE);
	transparent=rgba_pixel_from_gdcolor(image->format, 0, SDL_ALPHA_TRANSPARENT);	/* color does not matter as totally transparent */
	
	/* for all characters */
	/* render character in "image", then do a displayformat(image) to generate the resulting one */
	for (j=0; j<NUM_OF_CHARS; j++) {
		int x1, y1;
		
		y1=(j/16)*8;	/* 16 characters in a row */
		x1=(j%16)*8;
		
		if (SDL_MUSTLOCK(image))
			SDL_LockSurface(image);
		if (SDL_MUSTLOCK(image_n))
			SDL_LockSurface(image_n);
		for (y=0; y<8; y++) {
			guint32 *p=(guint32*) ((char *)image->pixels + y*image->pitch);
			guint32 *p_n=(guint32*) ((char *)image_n->pixels + y*image_n->pitch);

			for (x=0; x<8; x++) {
				/* the font array is encoded the same way as a c64-colored pixbuf. see c64_gfx_data...() */
				if (font[(y1+y)*128+x1+x]!=1) {
					p[0]=col;	/* wide */
					p[1]=col;
					p_n[0]=col;	/* normal */
				} else {
					p[0]=transparent;	/* wide */
					p[1]=transparent;
					p_n[0]=transparent;	/* normal */
				}
				p+=2;
				p_n++;
			}
		}
		if (SDL_MUSTLOCK(image))
			SDL_UnlockSurface(image);
		if (SDL_MUSTLOCK(image_n))
			SDL_UnlockSurface(image_n);
		fw[j]=displayformatalpha(image);	/* alpha channel is used! */
		fn[j]=displayformatalpha(image_n);
	}
	SDL_FreeSurface(image);
	SDL_FreeSurface(image_n);

	/* add newly rendered font to hash table. to the recently used list, it is already added. */	
	g_hash_table_insert(font_w, GINT_TO_POINTER(color), fw);
	g_hash_table_insert(font_n, GINT_TO_POINTER(color), fn);
}

/* XXX these functions are not really implemented */
static void
loadfont_buffer()
{
	/* forget all previously rendered chars */
	g_hash_table_remove_all(font_w);
	g_hash_table_remove_all(font_n);
	g_list_free(font_color_recently_used);
	font_color_recently_used=NULL;
}


/* loads a font from a file */
void
gd_loadfont_file(const char *filename)
{
	/* not yet implemented */
	g_assert_not_reached();
}

/* loads inlined c64 font */
void
gd_loadfont_default()
{
	font=c64_font+1;	/* first byte in c64_gfx[] is the "cell size", we ignore that */
	loadfont_buffer();
}




int gd_font_height()
{
	return 8*gd_scale;
}

int gd_font_width()
{
	return 8*gd_scale;
}

int gd_line_height()
{
	return gd_font_height()+2;
}

/* clears one line on the screen. takes dark screen or totally black screen into consideration. */
void
gd_clear_line(SDL_Surface *screen, int y)
{
	SDL_Rect rect;
	
	rect.x=0;
	rect.y=y;
	rect.w=screen->w;
	rect.h=gd_font_height();
	if (dark_screens!=NULL && dark_screens->data!=NULL)
		SDL_BlitSurface(dark_screens->data, &rect, screen, &rect);
	else
		SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0, 0, 0));	/* fill rectangle with black */
}




/* function which draws characters on the screen. used internally. */
/* x=-1 -> center horizontally */
static int
blittext_font(SDL_Surface *screen, SDL_Surface **font, int x1, int y, const char *text)
{
	const char *p=text;
	SDL_Rect destrect;
	int i;
	int x;
	gunichar c;

	if (x1==-1)
		x1=screen->w/2 - (font[0]->w*g_utf8_strlen(text, -1))/2;
	
	x=x1;
	c=g_utf8_get_char(p);
	while (c!=0) {
		if (c=='\n') {
			/* if it is an enter */
			if (x==x1) 
				y+=font[0]->h/2;
			else
				y+=font[0]->h;
			x=x1;
		} else {
			if (c<NUM_OF_CHARS)
				i=c;
			else
				i=127;	/* unknown char */
			
			destrect.x=x;
			destrect.y=y;
			SDL_BlitSurface(font[i], NULL, screen, &destrect);
			
			x+=font[i]->w;
		}
				
		p=g_utf8_next_char(p);	/* next character */
		c=g_utf8_get_char(p);
	}
		
	return x;
}

/* write something to the screen, with wide characters. */
/* x=-1 -> center horizontally */
int
gd_blittext(SDL_Surface *screen, int x, int y, GdColor color, const char *text)
{
	color=gd_color_get_rgb(color);	/* convert to rgb, so they are stored that way in the hash table */
	renderfont_color(color);
	return blittext_font(screen, g_hash_table_lookup(font_w, GUINT_TO_POINTER(color)), x, y, text);
}

/* write something to the screen, with normal characters. */
/* x=-1 -> center horizontally */
int
gd_blittext_n(SDL_Surface *screen, int x, int y, GdColor color, const char *text)
{
	color=gd_color_get_rgb(color);	/* convert to rgb, so they are stored that way in the hash table */
	renderfont_color(color);
	return blittext_font(screen, g_hash_table_lookup(font_n, GUINT_TO_POINTER(color)), x, y, text);
}

/* write something to the screen, with wide characters. */
/* x=-1 -> center horizontally */
int
gd_blittext_printf(SDL_Surface *screen, int x, int y, GdColor color, const char *format, ...)
{
	va_list args;
	char *text;
	
	va_start(args, format);
	text=g_strdup_vprintf(format, args);
	x=gd_blittext(screen, x, y, color, text);
	g_free(text);
	va_end(args);

	return x;
}

/* write something to the screen, with normal characters. */
/* x=-1 -> center horizontally */
int
gd_blittext_printf_n(SDL_Surface *screen, int x, int y, GdColor color, const char *format, ...)
{
	va_list args;
	char *text;
	
	va_start(args, format);
	text=g_strdup_vprintf(format, args);
	x=gd_blittext_n(screen, x, y, color, text);
	g_free(text);
	va_end(args);

	return x;
}



void
gd_select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5)
{
	/* if non-c64 gfx, nothing to do */
	if (using_png_gfx)
		return;

	/* convert to rgb value */
	c0=gd_color_get_rgb(c0);
	c1=gd_color_get_rgb(c1);
	c2=gd_color_get_rgb(c2);
	c3=gd_color_get_rgb(c3);
	c4=gd_color_get_rgb(c4);
	c5=gd_color_get_rgb(c5);

	/* and compare rgb values! */
	if (c0!=color0 || c1!=color1 || c2!=color2 || c3!=color3 || c4!=color4 || c5!=color5) {
		/* if not the same colors as requested before */
		color0=c0;
		color1=c1;
		color2=c2;
		color3=c3;
		color4=c4;
		color5=c5;

		loadcells_from_c64_data(c0, c1, c2, c3, c4, c5);
	}
}






/*
	logical_size: logical pixel size of playfield, usually larger than the screen.
	physical_size: visible part. (remember: player_x-x1!)

	center: the coordinates to scroll to.
	exact: scroll exactly
	start: start scrolling if difference is larger than
	to: scroll to, if started, until difference is smaller than
	current

	desired: the function stores its data here
	speed: the function stores its data here
	
	cell_size: size of one cell. used to determine if the play field is only a slightly larger than the screen, in that case no scrolling is desirable
*/
static gboolean
cave_scroll(int logical_size, int physical_size, int center, gboolean exact, int start, int to, int *current, int *desired, int speed, int cell_size)
{
	int i;
	gboolean changed;
	int max;
	
	max=logical_size-physical_size;
	if (max<0)
		max=0;

	changed=FALSE;

	/* if cave size smaller than the screen, no scrolling req'd */
	if (logical_size<physical_size) {
		*desired=0;
		if (*current!=0) {
			*current=0;
			changed=TRUE;
		}

		return changed;
	}
	
	/* if cave size is only a slightly larger than the screen, also no scrolling */
	if (logical_size<=physical_size+cell_size) {
		*desired=max/2;	/* scroll to the middle of the cell */
	} else {
		/* hystheresis function.
		 * when scrolling left, always go a bit less left than player being at the middle.
		 * when scrolling right, always go a bit less to the right. */
		if (exact)
			*desired=center;
		else {
			if (*current+start<center)
				*desired=center-to;
			if (*current-start>center)
				*desired=center+to;
		}
	}
	*desired=gd_clamp(*desired, 0, max);

	if (*current<*desired) {
		for (i=0; i<speed; i++)
			if (*current<*desired)
				(*current)++;
		changed=TRUE;
	}
	if (*current > *desired) {
		for (i=0; i<speed; i++)
			if (*current>*desired)
				(*current)--;
		changed=TRUE;
	}
	
	return changed;
}





/* just set current viewport to upper left. */
void
gd_scroll_to_origin()
{
	scroll_x=0;
	scroll_y=0;
}


/* SCROLLING
 *
 * scrolls to the player during game play.
 * called by drawcave
 * returns true, if player is not visible-ie it is out of the visible size in the drawing area.
 */
gboolean
gd_scroll(GdGame *game, gboolean exact_scroll)
{
	static int scroll_desired_x=0, scroll_desired_y=0;
	gboolean out_of_window;
	int player_x, player_y, visible_x, visible_y;
	gboolean changed;
	int scroll_divisor;
	/* max scrolling speed depends on the speed of the cave. */
	/* game moves cell_size_game* 1s/cave time pixels in a second. */
	/* scrolling moves scroll speed * 1s/scroll_time in a second. */
	/* these should be almost equal; scrolling speed a little slower. */
	/* that way, the player might reach the border with a small probability, */
	/* but the scrolling will not "oscillate", ie. turn on for little intervals as it has */
	/* caught up with the desired position. smaller is better. */
	int scroll_speed=cell_size*(gd_fine_scroll?20:40)/game->cave->speed;
	
	player_x=game->cave->player_x-game->cave->x1;	/* cell coordinates of player */
	player_y=game->cave->player_y-game->cave->y1;
	visible_x=(game->cave->x2-game->cave->x1+1)*cell_size;	/* pixel size of visible part of the cave (may be smaller in intermissions) */
	visible_y=(game->cave->y2-game->cave->y1+1)*cell_size;

	/* cell_size contains the scaled size, but we need the original. */
	changed=FALSE;
	/* some sort of scrolling speed. with larger cells, the divisor must be smaller, so the scrolling faster. */
	scroll_divisor=256/cell_size;
	if (gd_fine_scroll)
		scroll_divisor*=2;	/* as fine scrolling is 50hz, whereas normal is 25hz only */

	if (cave_scroll(visible_x, play_area_w, player_x*cell_size+cell_size/2-play_area_w/2, exact_scroll, play_area_w/4, play_area_w/8, &scroll_x, &scroll_desired_x, scroll_speed, cell_size))
		changed=TRUE;
	if (cave_scroll(visible_y, play_area_h, player_y*cell_size+cell_size/2-play_area_h/2, exact_scroll, play_area_h/5, play_area_h/10, &scroll_y, &scroll_desired_y, scroll_speed, cell_size))
		changed=TRUE;
	
	/* if scrolling, we should update entire gd_screen. */
	if (changed) {
		int x, y;
		
		for (y=0; y<game->cave->h; y++)
			for (x=0; x<game->cave->w; x++)
				game->gfx_buffer[y][x] |= GD_REDRAW;
	}

	/* check if active player is visible at the moment. */
	out_of_window=FALSE;
	/* check if active player is outside drawing area. if yes, we should wait for scrolling */
	if ((player_x*cell_size)<scroll_x || (player_x*cell_size+cell_size-1)>scroll_x+play_area_w)
		/* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
		if (game->cave->player_x>=game->cave->x1 && game->cave->player_x<=game->cave->x2)
			out_of_window=TRUE;
	if ((player_y*cell_size)<scroll_y || (player_y*cell_size+cell_size-1)>scroll_y+play_area_h)
		/* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
		if (game->cave->player_y>=game->cave->y1 && game->cave->player_y<=game->cave->y2)
			out_of_window=TRUE;

	/* if not yet born, we treat as visible. so cave will run. the user is unable to control an unborn player, so this is the right behaviour. */
	if (game->cave->player_state==GD_PL_NOT_YET)
		return FALSE;
	return out_of_window;
}




int
gd_drawcave(SDL_Surface *dest, GdGame *game)
{
	int x, y, xd, yd;
	SDL_Rect cliprect;
	int scroll_y_aligned;

	/* on-screen clipping rectangle */
	cliprect.x=0;
	cliprect.y=gd_statusbar_height;
	cliprect.w=play_area_w;
	cliprect.h=play_area_h;
	SDL_SetClipRect(dest, &cliprect);

	/* for the paused parameter, we set FALSE here, as the sdl version does not color the playfield when paused */
	if (gd_sdl_pal_emulation && gd_even_line_pal_emu_vertical_scroll)
		/* make it even (dividable by two) */
		scroll_y_aligned=scroll_y/2*2;
	else
		scroll_y_aligned=scroll_y;
	/* here we draw all cells to be redrawn. we do not take scrolling area into consideration - sdl will do the clipping. */
	for (y=game->cave->y1, yd=0; y<=game->cave->y2; y++, yd++) {
		for (x=game->cave->x1, xd=0; x<=game->cave->x2; x++, xd++) {
			if (game->gfx_buffer[y][x] & GD_REDRAW) {	/* if it needs to be redrawn */
				SDL_Rect offset;

				offset.y=y*cell_size+gd_statusbar_height-scroll_y_aligned;	/* sdl_blitsurface destroys offset, so we have to set y here, too. (ie. in every iteration) */
				offset.x=x*cell_size-scroll_x;

				game->gfx_buffer[y][x]=game->gfx_buffer[y][x] & ~GD_REDRAW;	/* now we have drawn it */

				SDL_BlitSurface(cells[game->gfx_buffer[y][x]], NULL, dest, &offset);
			}
		}
	}

	/* restore clipping to whole screen */
	SDL_SetClipRect(dest, NULL);
	return 0;
}





/* the dark gray background */
/* to be called at application startup */
void
gd_create_dark_background()
{
	SDL_Surface *tile, *dark_tile, *dark_screen_tiled;
	int x, y;
	
	g_assert(gd_screen!=NULL);
	
	if (dark_background)
		SDL_FreeSurface(dark_background);

	tile=surface_from_raw_data(gdash_tile, sizeof(gdash_tile));
	/* 24bpp, as it has no alpha channel */
	dark_tile=SDL_CreateRGBSurface(0, tile->w, tile->h, 24, 0, 0, 0, 0);
	SDL_BlitSurface(tile, NULL, dark_tile, NULL);
	SDL_FreeSurface(tile);
	SDL_SetAlpha(dark_tile, SDL_SRCALPHA, 256/6);	/* 1/6 opacity */
	
	/* create the image, and fill it with the tile. */
	/* the image is screen size / gd_scale, so we prefer the original screen size here */
	/* and only do the scaling later! */
	dark_screen_tiled=SDL_CreateRGBSurface(0, gd_screen->w/gd_scale, gd_screen->h/gd_scale, 32, rmask, gmask, bmask, amask);
	for (y=0; y<dark_screen_tiled->h; y+=dark_tile->h)
		for (x=0; x<dark_screen_tiled->w; x+=dark_tile->w) {
			SDL_Rect rect;
			
			rect.x=x;
			rect.y=y;
			
			SDL_BlitSurface(dark_tile, NULL, dark_screen_tiled, &rect);
		}
	SDL_FreeSurface(dark_tile);
	dark_background=displayformat(dark_screen_tiled);
	SDL_FreeSurface(dark_screen_tiled);
}

void
gd_dark_screen()
{
	SDL_BlitSurface(dark_background, NULL, gd_screen, NULL);
}


/*
 * screen backup and restore functions. these are used by menus, help screens
 * and the like. backups and restores can be nested.
 *
 */
/* backups the current screen contents, and darkens it. */
/* later, gd_restore_screen can be called. */
void
gd_backup_and_dark_screen()
{
	SDL_Surface *backup_screen, *dark_screen;
	
	backup_screen=SDL_CreateRGBSurface(0, gd_screen->w, gd_screen->h, 32, 0, 0, 0, 0);
	SDL_BlitSurface(gd_screen, 0, backup_screen, 0);

	dark_screen=SDL_DisplayFormat(dark_background);

	SDL_BlitSurface(dark_screen, NULL, gd_screen, NULL);
	
	backup_screens=g_list_prepend(backup_screens, backup_screen);
	dark_screens=g_list_prepend(dark_screens, dark_screen);
}

/* backups the current screen contents, and clears it. */
/* later, gd_restore_screen can be called. */
void
gd_backup_and_black_screen()
{
	SDL_Surface *backup_screen;
	
	backup_screen=SDL_CreateRGBSurface(0, gd_screen->w, gd_screen->h, 32, 0, 0, 0, 0);
	SDL_BlitSurface(gd_screen, 0, backup_screen, 0);
	
	backup_screens=g_list_prepend(backup_screens, backup_screen);
	dark_screens=g_list_prepend(dark_screens, NULL);
}

/* restores a backed up screen. */
void
gd_restore_screen()
{
	SDL_Surface *backup_screen, *dark_screen;

	/* check if lists are non-empty */
	g_assert(backup_screens!=NULL);
	g_assert(dark_screens!=NULL);
	
	backup_screen=backup_screens->data;
	backup_screens=g_list_delete_link(backup_screens, backup_screens);	/* remove first */
	dark_screen=dark_screens->data;
	dark_screens=g_list_delete_link(dark_screens, dark_screens);	/* remove first */
	
	SDL_BlitSurface(backup_screen, 0, gd_screen, 0);
	SDL_Flip(gd_screen);
	SDL_FreeSurface(backup_screen);
	if (dark_screen)
		SDL_FreeSurface(dark_screen);
}



