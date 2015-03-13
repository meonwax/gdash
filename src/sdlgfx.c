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
#include <SDL.h>
#include <glib.h>
#include "colors.h"
#include "gameplay.h"
#include "cave.h"
#include "settings.h"
#include "sdlgfx.h"
#include "util.h"
#include "c64_gfx.h"	/* char c64_gfx[] with (almost) original graphics */
#include "title.h"
#include "c64_font.h"
#include "gfxutil.h"

#include "c64_png_colors.h"

#define GD_NUM_OF_CHARS 128

/* these can't be larger than 127, or they mess up with utf8 coding */
#define UNKNOWN_CHAR_INDEX 31
#define BACKSLASH_CHAR_INDEX 64
#define TILDE_CHAR_INDEX 95
#define UNDERSCORE_CHAR_INDEX 100
#define OPEN_BRACKET_CHAR_INDEX 27
#define CLOSE_BRACKET_CHAR_INDEX 29

int gd_scale=1;	/* a graphics scale things which sets EVERYTHING. it is set with gd_sdl_init, and cannot be modified later. */
int gd_scale_type=GD_SCALING_ORIGINAL;

static SDL_Surface *cells[2*NUM_OF_CELLS];
static const guchar *font;
static GHashTable *font_w, *font_n;
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


/* screen area */
SDL_Surface *gd_screen=NULL;
static SDL_Surface *dark_background=NULL;
static GList *backup_screens=NULL, *dark_screens=NULL;
int play_area_w=320;
int play_area_h=180;
int statusbar_height=20;
int statusbar_y1=1;
int statusbar_y2=10;
int statusbar_mid=(20-8)/2;
static int scroll_x, scroll_y;


/* quit, global variable which is set to true if the application should quit */
gboolean gd_quit=FALSE;

guint8 *gd_keystate;
SDL_Joystick *gd_joy;

static int cell_size=16;










/* read a gdk-pixbuf source, and return an sdl surface. */
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



/* This is taken from the SDL_gfx project. */
/*  
	SDL_rotozoom.c - rotozoomer for 32bit or 8bit surfaces
	LGPL (c) A. Schiffler
	32bit Zoomer with optional anti-aliasing by bilinear interpolation.
	Zoomes 32bit RGBA/ABGR 'src' surface to 'dst' surface.
*/

typedef struct tColorRGBA {
	guint8 r;
	guint8 g;
	guint8 b;
	guint8 a;
} tColorRGBA;

static void zoomSurfaceRGBA(SDL_Surface *src, SDL_Surface *dst, gboolean smooth)
{
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
	SDL_LockSurface(src);
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

	SDL_UnlockSurface(src);
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
	
	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	gd_scale2x(src->pixels, src->w, src->h, src->pitch, dst->pixels, dst->pitch);
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);
}

static void
scale3x(SDL_Surface *src, SDL_Surface *dst)
{
	g_assert(dst->w == src->w*3);
	g_assert(dst->h == src->h*3);
	g_assert(src->format->BytesPerPixel==4);
	g_assert(dst->format->BytesPerPixel==4);
	
	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	gd_scale3x(src->pixels, src->w, src->h, src->pitch, dst->pixels, dst->pitch);
	SDL_UnlockSurface(src);
	SDL_UnlockSurface(dst);
}




/* scales a pixbuf with the appropriate scaling type. */
SDL_Surface *
surface_scale(SDL_Surface *orig)
{
	SDL_Surface *dest, *dest2x;
	
	dest=SDL_CreateRGBSurface(orig->flags, orig->w*gd_scale, orig->h*gd_scale, orig->format->BitsPerPixel, orig->format->Rmask, orig->format->Gmask, orig->format->Bmask, orig->format->Amask);
	
	switch (gd_scale_type) {
		case GD_SCALING_ORIGINAL:
			SDL_BlitSurface(orig, NULL, dest, NULL);
			break;
		
		case GD_SCALING_2X:
			zoomSurfaceRGBA(orig, dest, FALSE);
			break;

		case GD_SCALING_2X_BILINEAR:
			zoomSurfaceRGBA(orig, dest, TRUE);
			break;

		case GD_SCALING_2X_SCALE2X:
			scale2x(orig, dest);
			break;

		case GD_SCALING_3X:
			zoomSurfaceRGBA(orig, dest, FALSE);
			break;

		case GD_SCALING_3X_BILINEAR:
			zoomSurfaceRGBA(orig, dest, TRUE);
			break;

		case GD_SCALING_3X_SCALE3X:
			scale3x(orig, dest);
			break;
			
		case GD_SCALING_4X:
			zoomSurfaceRGBA(orig, dest, FALSE);
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
	SDL_LockSurface(image);
	gd_pal_emu(image->pixels, image->w, image->h, image->pitch, image->format->Rshift, image->format->Gshift, image->format->Bshift, image->format->Ashift);
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
	
//	pal_emu(orig);
	scaled=surface_scale(orig);
	if (gd_sdl_pal_emulation)
		pal_emu_surface(scaled);
	displayformat=SDL_DisplayFormat(scaled);
	SDL_FreeSurface(scaled);

	return displayformat;
}

/* needed to alpha-copy an src image over dst.
   the resulting image of sdl_blitsurface is not exact?!
   can't explain why. */
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
	
	screen=surface_from_gdk_pixbuf_data((guint32 *) gdash_screen);
	tile=surface_from_gdk_pixbuf_data((guint32 *) gdash_tile);

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
		copy_alpha(screen, frame);

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
	statusbar_y1*=gd_scale;
	statusbar_y2*=gd_scale;
	statusbar_height*=gd_scale;
	statusbar_mid*=gd_scale;

	SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_JOYSTICK);
	gd_screen=SDL_SetVideoMode(play_area_w, play_area_h+statusbar_height, 32, SDL_ANYFORMAT | (gd_sdl_fullscreen?SDL_FULLSCREEN:0));
	SDL_ShowCursor(SDL_DISABLE);
	if (!gd_screen)
		return FALSE;
	
	/* keyboard, joystick */
	gd_keystate=SDL_GetKeyState(NULL);
	if (SDL_NumJoysticks()>0)
		gd_joy=SDL_JoystickOpen(0);

	/* icon */
	icon=surface_from_gdk_pixbuf_data((guint32 *) gdash_32);
	SDL_WM_SetIcon(icon, NULL);
	SDL_WM_SetCaption("GDash", NULL);
	SDL_FreeSurface(icon);
	
	font_n=g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, rendered_font_free);
	font_w=g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, rendered_font_free);
	
	return TRUE;
}




static inline gboolean
gd_joy_up()
{
	return gd_joy!=NULL && SDL_JoystickGetAxis(gd_joy, 1)<-3200;
}

static inline gboolean
gd_joy_down()
{
	return gd_joy!=NULL && SDL_JoystickGetAxis(gd_joy, 1)>3200;
}

static inline gboolean
gd_joy_left()
{
	return gd_joy!=NULL && SDL_JoystickGetAxis(gd_joy, 0)<-3200;
}

static inline gboolean
gd_joy_right()
{
	return gd_joy!=NULL && SDL_JoystickGetAxis(gd_joy, 0)>3200;
}

static inline gboolean
gd_joy_fire()
{
	return gd_joy!=NULL && (SDL_JoystickGetButton(gd_joy, 0) || SDL_JoystickGetButton(gd_joy, 1));
}




gboolean
gd_up()
{
	return gd_keystate[SDLK_UP] || gd_joy_up();
}

gboolean
gd_down()
{
	return gd_keystate[SDLK_DOWN] || gd_joy_down();
}

gboolean
gd_left()
{
	return gd_keystate[SDLK_LEFT] || gd_joy_left();
}

gboolean
gd_right()
{
	return gd_keystate[SDLK_RIGHT] || gd_joy_right();
}

gboolean
gd_fire()
{
	return gd_keystate[SDLK_LCTRL] || gd_keystate[SDLK_RCTRL] || gd_joy_fire();
}




/* like the above one, but used in the main menu */
gboolean
gd_space_or_enter_or_fire()
{
	/* set these to zero, so each keypress is reported only once */
	return gd_joy_fire() || gd_keystate[SDLK_SPACE] || gd_keystate[SDLK_RETURN];
}


/* SCROLLING
 *
 * scrolls to the player during game play.
 * called by drawcave
 * returns true, if player is not visible-ie it is out of the visible size in the drawing area.
 */
gboolean
gd_scroll(const Cave *cave, gboolean exact_scroll)
{
	static int scroll_desired_x=0, scroll_desired_y=0;
	static int scroll_speed_x=0, scroll_speed_y=0;
	gboolean out_of_window;
	int player_x, player_y, visible_x, visible_y;
	gboolean changed;
	int scroll_divisor;

	player_x=cave->player_x-cave->x1;	/* cell coordinates of player */
	player_y=cave->player_y-cave->y1;
	visible_x=(cave->x2-cave->x1+1)*cell_size;	/* pixel size of visible part of the cave (may be smaller in intermissions) */
	visible_y=(cave->y2-cave->y1+1)*cell_size;

	changed=FALSE;
	scroll_divisor=12;
	if (gd_fine_scroll)
		scroll_divisor*=2;	/* as fine scrolling is 50hz, whereas normal is 25hz only */
	if (gd_cave_scroll(visible_x, play_area_w, player_x*cell_size+cell_size/2-play_area_w/2, exact_scroll, play_area_w/4, play_area_w/8, &scroll_x, &scroll_desired_x, &scroll_speed_x, scroll_divisor))
		changed=TRUE;
	if (gd_cave_scroll(visible_y, play_area_h, player_y*cell_size+cell_size/2-play_area_h/2, exact_scroll, play_area_h/4, play_area_h/8, &scroll_y, &scroll_desired_y, &scroll_speed_y, scroll_divisor))
		changed=TRUE;
	
	/* if scrolling, we should update entire gd_screen. */
	if (changed) {
		int x, y;
		
		for (y=0; y<gd_gameplay.cave->h; y++)
			for (x=0; x<gd_gameplay.cave->w; x++)
				gd_gameplay.gfx_buffer[y][x]=-1;
	}

	/* check if active player is visible at the moment. */
	out_of_window=FALSE;
	/* check if active player is outside drawing area. if yes, we should wait for scrolling */
	if ((player_x*cell_size)<scroll_x || (player_x*cell_size+cell_size-1)>scroll_x+play_area_w)
		/* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
		if (cave->player_x>=cave->x1 && cave->player_x<=cave->x2)
			out_of_window=TRUE;
	if ((player_y*cell_size)<scroll_y || (player_y*cell_size+cell_size-1)>scroll_y+play_area_h)
		/* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
		if (cave->player_y>=cave->y1 && cave->player_y<=cave->y2)
			out_of_window=TRUE;
	
	return out_of_window;
}


/* just set current viewport to upper left. */
void
gd_scroll_to_origin()
{
	scroll_x=0;
	scroll_y=0;
}



int
gd_drawcave(SDL_Surface *dest, const Cave *cave, int **gfx_buffer, gboolean only_scroll)
{
	int x, y, xd, yd;
	SDL_Rect cliprect;
	int scroll_y_aligned;

	/* do the scrolling. scroll exactly, if player is not yet alive */
	gd_gameplay.out_of_window=gd_scroll(cave, cave->player_state==GD_PL_NOT_YET);

	/* on-screen clipping rectangle */
	cliprect.x=0;
	cliprect.y=statusbar_height;
	cliprect.w=play_area_w;
	cliprect.h=play_area_h;
	SDL_SetClipRect(dest, &cliprect);

	/* for the paused parameter, we set FALSE here, as the sdl version does not color the playfield when paused */
	gd_drawcave_game(cave, gfx_buffer, gd_gameplay.bonus_life_flash!=0, FALSE, !only_scroll);
	if (gd_sdl_pal_emulation && gd_even_line_pal_emu_vertical_scroll)
		/* make it even (dividable by two) */
		scroll_y_aligned=scroll_y/2*2;
	else
		scroll_y_aligned=scroll_y;
	/* here we draw all cells to be redrawn. we do not take scrolling area into consideration - sdl will do the clipping. */
	for (y=cave->y1, yd=0; y<=cave->y2; y++, yd++) {
		for (x=cave->x1, xd=0; x<=cave->x2; x++, xd++) {
			if (gd_gameplay.gfx_buffer[y][x] & GD_REDRAW) {	/* if it needs to be redrawn */
				SDL_Rect offset;

				offset.y=y*cell_size+statusbar_height-scroll_y_aligned;	/* sdl_blitsurface changes offset, so we have to set y here, too. */
				offset.x=x*cell_size-scroll_x;

				gd_gameplay.gfx_buffer[y][x]=gd_gameplay.gfx_buffer[y][x] & ~GD_REDRAW;	/* now we have drawn it */

				SDL_BlitSurface(cells[gfx_buffer[y][x]], NULL, dest, &offset);
			}
		}
	}

	/* restore clipping to whole screen */
	SDL_SetClipRect(dest, NULL);
	return 0;
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
loadcells(SDL_Surface *image)
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
setpalette(SDL_Surface *image, int index, GdColor col)
{
	SDL_Color c;
	c.r=gd_color_get_r(col);
	c.g=gd_color_get_g(col);
	c.b=gd_color_get_b(col);
	
	SDL_SetPalette(image, SDL_LOGPAL|SDL_PHYSPAL, &c, index, 1);
}

static void
loadcells_c64(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5)
{
	const guint8 *gfx;	/* currently used graphics, will point to c64_gfx or c64_custom_gfx */
	SDL_Surface *image;

	gfx=c64_custom_gfx?c64_custom_gfx:c64_gfx;

	/* create a 8-bit palette and set its colors */
	/* gfx[0] is the pixel width and height of one cell. */
	/* from gfx[1], we have the color data, one byte/pixel. so this is an indexed color image: 8bit/pixel. */
	image=SDL_CreateRGBSurfaceFrom((void *)(gfx+1), NUM_OF_CELLS_X*(int)gfx[0], NUM_OF_CELLS_Y*(int)gfx[0], 8, NUM_OF_CELLS_X*(int)gfx[0], 0, 0, 0, 0);
	/* sdl supports paletted images, so this is very easy: */
	setpalette(image, 0, 0);
	setpalette(image, 1, c0);
	setpalette(image, 2, c1);
	setpalette(image, 3, c2);
	setpalette(image, 4, c3);
	setpalette(image, 5, c4);
	setpalette(image, 6, c5);
	setpalette(image, 7, 0);
	setpalette(image, 8, 0);

	/* from here, same as any other image */
	loadcells(image);
	SDL_FreeSurface(image);
}

/* takes a c64_gfx.png-coded 32-bit pixbuf, and creates a paletted pixbuf in our internal format. */
guchar *
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
			int r, g, b, a;

			r=(p[x]&image->format->Rmask) >> image->format->Rshift << image->format->Rloss;
			g=(p[x]&image->format->Gmask) >> image->format->Gshift << image->format->Gloss;
			b=(p[x]&image->format->Bmask) >> image->format->Bshift << image->format->Bloss;
			a=(p[x]&image->format->Amask) >> image->format->Ashift << image->format->Aloss;
			
			data[out++]=c64_png_colors(r, g, b, a);
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
gd_is_surface_ok_for_theme (SDL_Surface *surface)
{
	if ((surface->w % NUM_OF_CELLS_X != 0) || (surface->h % NUM_OF_CELLS_Y != 0) || (surface->w / NUM_OF_CELLS_X != surface->h / NUM_OF_CELLS_Y)) {
		g_warning("image should contain %d cells in a row and %d in a column!", NUM_OF_CELLS_X, NUM_OF_CELLS_Y);
		return FALSE; /* Image should contain 16 cells in a row and 32 in a column! */
	}
	/* we do not check for an alpha channel, as in the gtk version. we do not need it here. */
	/* the gtk version needs it because of the editor. */

	return TRUE;	/* passed checks */
}


/* load theme from bmp file. */
/* return true if successful. */
gboolean
gd_loadcells_file(const char *filename)
{
	SDL_Surface *surface, *converted;

	/* load cell graphics */
	/* load from file */
	surface=SDL_LoadBMP(filename);

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
	converted=SDL_CreateRGBSurface(0, surface->w, surface->h, 32, rmask, bmask, gmask, amask);
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
		loadcells(converted);
	}
	SDL_FreeSurface(converted);
	
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
	guint32 col, black;
	SDL_Surface *image, *image_n;
	SDL_Surface **fn, **fw;
	
	/* if already rendered, return now */
	if (g_hash_table_lookup(font_w, GUINT_TO_POINTER(color)))
		return;
		
	fn=g_new0(SDL_Surface *, GD_NUM_OF_CHARS+1);
	fw=g_new0(SDL_Surface *, GD_NUM_OF_CHARS+1);
	
	/* colors data into memory */
	image=SDL_CreateRGBSurface(0, 16, 8, 32, rmask, gmask, bmask, amask);
	image_n=SDL_CreateRGBSurface(0, 8, 8, 32, rmask, gmask, bmask, amask);

	col=rgba_pixel_from_gdcolor(image->format, color, 0xff);
	black=rgba_pixel_from_gdcolor(image->format, 0, 0xff);
	
	/* for all characters */
	/* render character in "image", then do a displayformat(image) to generate the resulting one */
	for (j=0; j<GD_NUM_OF_CHARS; j++) {
		int x1, y1;
		
		y1=(j/16)*8;	/* 16 characters in a row */
		x1=(j%16)*8;
		
		SDL_LockSurface(image);
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
					p[0]=black;	/* wide */
					p[1]=black;
					p_n[0]=black;	/* normal */
				}
				p+=2;
				p_n++;
			}
		}
		SDL_UnlockSurface(image);
		SDL_UnlockSurface(image_n);
		fw[j]=displayformat(image);
		fn[j]=displayformat(image_n);
		/* small font does not draw black background */
		SDL_SetColorKey(fn[j], SDL_SRCCOLORKEY, SDL_MapRGB(fn[j]->format, 0, 0, 0));
	}
	SDL_FreeSurface(image);
	SDL_FreeSurface(image_n);
	
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
}


/* loads a font from a file */
void
gd_loadfont_file(const char *filename)
{
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
gd_blittext_font(SDL_Surface *screen, SDL_Surface **font, int x1, int y, const char *text)
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
			/* it is a normal character */
			if (c==GD_PLAYER_CHAR || c==GD_DIAMOND_CHAR)	/* special, by gdash */
				i=c;
			else
			if (c==GD_KEY_CHAR)
				i=92;
			else
			if (c=='@')
				i=0;
			else
			if (c>=' ' && c<='Z')	/* from space to Z, petscii=ascii */
				i=c;
			else
			if (c>='a' && c<='z')
				i=c-'a'+1;
			else
			if (c=='\\')
				i=BACKSLASH_CHAR_INDEX;
			else
			if (c=='_')
				i=UNDERSCORE_CHAR_INDEX;
			else
			if (c=='~')
				i=TILDE_CHAR_INDEX;
			else
			if (c=='[')
				i=OPEN_BRACKET_CHAR_INDEX;
			else
			if (c==']')
				i=CLOSE_BRACKET_CHAR_INDEX;
			else
				i=UNKNOWN_CHAR_INDEX;
			
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
	renderfont_color(color);
	return gd_blittext_font(screen, g_hash_table_lookup(font_w, GUINT_TO_POINTER(color)), x, y, text);
}

/* write something to the screen, with normal characters. */
/* x=-1 -> center horizontally */
int
gd_blittext_n(SDL_Surface *screen, int x, int y, GdColor color, const char *text)
{
	renderfont_color(color);
	return gd_blittext_font(screen, g_hash_table_lookup(font_n, GUINT_TO_POINTER(color)), x, y, text);
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

		loadcells_c64(c0, c1, c2, c3, c4, c5);
	}
}

/* selects built-in graphics */
void
gd_loadcells_default()
{
	g_free(c64_custom_gfx);
	c64_custom_gfx=NULL;
	using_png_gfx=FALSE;
	/* just to set some default */
	color0=0xffffffff;	/* this is an invalid gdash color; so redraw is forced */
}

/* the dark gray background */
void
gd_create_dark_background()
{
	SDL_Surface *tile, *dark_tile, *dark_screen_tiled;
	int x, y;
	
	g_assert(gd_screen!=NULL);
	
	if (dark_background)
		SDL_FreeSurface(dark_background);

	tile=surface_from_gdk_pixbuf_data((guint32 *) gdash_tile);
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
	while (gd_keystate[SDLK_RETURN]!=0 || gd_keystate[SDLK_ESCAPE]!=0 || gd_keystate[SDLK_SPACE]!=0 || gd_fire()) {
		SDL_Event event;

		/* process pending events, so presses and releases are applied */
		while (SDL_PollEvent(&event)) {
			/* meanwhile, if we receive a quit event, we set the global variable. */
			if (event.type==SDL_QUIT)
				gd_quit=TRUE;
		}
	}
}





