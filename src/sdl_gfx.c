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
#include "game.h"
#include "cave.h"
#include "settings.h"
#include "sdl_gfx.h"
#include "util.h"
#include "c64_gfx.h"	/* char c64_gfx[] with (almost) original graphics */
#include "title.h"
#include "c64_font.h"

#define GD_NUM_OF_CHARS 128

/* these can't be larger than 127, or they mess up with utf8 coding */
#define GD_UNKNOWN_CHAR 31
#define GD_BACKSLASH_CHAR 64

static SDL_Surface *cells[2*NUM_OF_CELLS];
static const guchar *font;
static GHashTable *font_w, *font_n;
static GdColor color0, color1, color2, color3, color4, color5;	/* currently used cell colors */
static guint8 *c64_custom_gfx=NULL;
static gboolean using_png_gfx;

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
const int play_area_w=320;
const int play_area_h=180;
const int statusbar_y1=1;
const int statusbar_y2=10;
const int statusbar_height=20;
const int statusbar_mid=(20-8)/2;
static int scroll_x, scroll_y;


/* quit, global variable which is set to true if the application should quit */
gboolean gd_quit=FALSE;

Uint8 *gd_keystate;
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
		surface=SDL_CreateRGBSurfaceFrom(data+6, GUINT32_FROM_BE(data[4]), GUINT32_FROM_BE(data[5]), 24, GUINT32_FROM_BE(data[3]), rmask, gmask, bmask, 0);
	else
		/* unknown pixel format */
		g_assert_not_reached();
	g_assert(surface!=NULL);
	
	return surface;
}

/* returns the gdash title screen as an sdl surface. */
SDL_Surface *
gd_get_titleimage()
{
	return surface_from_gdk_pixbuf_data((guint32 *) gdash_screen);
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
gd_sdl_init()
{
	SDL_Surface *icon;
	
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

	player_x=cave->player_x-cave->x1;	/* cell coordinates of player */
	player_y=cave->player_y-cave->y1;
	visible_x=(cave->x2-cave->x1+1)*cell_size;	/* pixel size of visible part of the cave (may be smaller in intermissions) */
	visible_y=(cave->y2-cave->y1+1)*cell_size;

	changed=FALSE;
	if (gd_cave_scroll(visible_x, play_area_w, player_x*cell_size+cell_size/2-play_area_w/2, exact_scroll, play_area_w/4, play_area_w/8, &scroll_x, &scroll_desired_x, &scroll_speed_x))
		changed=TRUE;
	if (gd_cave_scroll(visible_y, play_area_h, player_y*cell_size+cell_size/2-play_area_h/2, exact_scroll, play_area_h/4, play_area_h/8, &scroll_y, &scroll_desired_y, &scroll_speed_y))
		changed=TRUE;
	
	/* if scrolling, we should update entire gd_screen. */
	if (changed) {
		int x, y;
		
		for (y=0; y<game.cave->h; y++)
			for (x=0; x<game.cave->w; x++)
				game.gfx_buffer[y][x]=-1;
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
gd_drawcave(SDL_Surface *dest, const Cave *cave, int **gfx_buffer)
{
	int x, y, xd, yd;
	SDL_Rect cliprect;

	/* do the scrolling. scroll exactly, if player is not yet alive */
	game.out_of_window=gd_scroll(cave, cave->player_state==PL_NOT_YET);

	/* on-screen clipping rectangle */
	cliprect.x=0;
	cliprect.y=statusbar_height;
	cliprect.w=play_area_w;
	cliprect.h=play_area_h;
	SDL_SetClipRect(dest, &cliprect);

	gd_drawcave_game(cave, gfx_buffer, game.bonus_life_flash!=0, FALSE);

	/* here we draw all cells to be redrawn. we do not take scrolling area into consideration - sdl will do the clipping. */
	for (y=cave->y1, yd=0; y<=cave->y2; y++, yd++) {
		for (x=cave->x1, xd=0; x<=cave->x2; x++, xd++) {
			if (game.gfx_buffer[y][x] & GD_REDRAW) {	/* if it needs to be redrawn */
				SDL_Rect offset;

				offset.y=y*cell_size+statusbar_height-scroll_y;	/* sdl_blitsurface changes offset, so we have to set y here, too. */
				offset.x=x*cell_size-scroll_x;

				game.gfx_buffer[y][x]=game.gfx_buffer[y][x] & ~GD_REDRAW;	/* now we have drawn it */

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

	/* if we have scaled pixmaps, remove them */
	free_cells();

	/* 8 (NUM_OF_CELLS_X) cells in a row, so divide by it and we get the size of a cell in pixels */
	pixbuf_cell_size=image->w/NUM_OF_CELLS_X;
	g_assert(pixbuf_cell_size==16);	/* FIXME currently only this is supported */
	cell_size=pixbuf_cell_size;	/* FIXME no scaling supported yet */

	rect=SDL_CreateRGBSurface(0, pixbuf_cell_size, pixbuf_cell_size, 32, 0, 0, 0, 0);
	SDL_FillRect(rect, NULL, SDL_MapRGB(rect->format, (gd_flash_color>>16)&0xff, (gd_flash_color>>8)&0xff, gd_flash_color&0xff));
	SDL_SetAlpha(rect, SDL_SRCALPHA, 128);	/* 50% alpha; nice choice. also sdl is rendering faster for the special value alpha=128 */

	cut=SDL_CreateRGBSurface(0, pixbuf_cell_size, pixbuf_cell_size, 32, 0, 0, 0, 0);

	/* make individual cells */
	for (i=0; i<NUM_OF_CELLS_X*NUM_OF_CELLS_Y; i++) {
		SDL_Rect from;

		from.x=(i%8)*pixbuf_cell_size;
		from.y=(i/8)*pixbuf_cell_size;
		from.w=pixbuf_cell_size;
		from.h=pixbuf_cell_size;

		SDL_BlitSurface(image, &from, cut, NULL);
		cells[i]=SDL_DisplayFormat(cut);
		SDL_BlitSurface(rect, NULL, cut, NULL);		/* create yellowish image */
		cells[NUM_OF_CELLS+i]=SDL_DisplayFormat(cut);
	}
	SDL_FreeSurface(cut);
	SDL_FreeSurface(rect);
}

/* creates a guint32 value, which can be raw-written to a sdl_surface memory area. */
/* takes the pixel format of the surface into consideration. */
static guint32
rgba_pixel_from_color(SDL_PixelFormat *format, GdColor col, guint8 a)
{
	guint8 r=(col>>16)&255;
	guint8 g=(col>>8)&255;
	guint8 b=col&255;

	return (r<<format->Rshift)+(g<<format->Gshift)+(b<<format->Bshift)+(a<<format->Ashift);
}

/* sets one of the colors in the indexed palette of an sdl surface to a GdColor. */
static void
setpalette(SDL_Surface *image, int index, GdColor col)
{
	SDL_Color c;
	c.r=(col>>16)&255;
	c.g=(col>>8)&255;
	c.b=col&255;
	
	SDL_SetPalette(image, SDL_LOGPAL|SDL_PHYSPAL, &c, index, 1);
}

static void
loadcells_c64(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5)
{
	const guchar *gfx;	/* currently used graphics, will point to c64_gfx or c64_custom_gfx */
	SDL_Surface *image;

	gfx=c64_custom_gfx?c64_custom_gfx:c64_gfx;

	/* create a 8-bit palette and set its colors */
	/* gfx[0] is the pixel width and height of one cell. */
	/* from gfx[1], we have the color data, one byte/pixel. so this is an indexed color image: 8bit/pixel. */
	image=SDL_CreateRGBSurfaceFrom((void *)(gfx+1), NUM_OF_CELLS_X*gfx[0], NUM_OF_CELLS_Y*gfx[0], 8, NUM_OF_CELLS_X*gfx[0], 0, 0, 0, 0);
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
	int cols[]={
	/* abgr */
	/* 0000 */ 0,	/* transparent */
	/* 0001 */ 0,
	/* 0010 */ 0,
	/* 0011 */ 0,
	/* 0100 */ 0,
	/* 0101 */ 0,
	/* 0110 */ 0,
	/* 0111 */ 0,
	/* 1000 */ 1, /* black - background */
	/* 1001 */ 2, /* red - foreg1 */
	/* 1010 */ 5, /* green - amoeba */
	/* 1011 */ 4, /* yellow - foreg3 */
	/* 1100 */ 6, /* blue - slime */
	/* 1101 */ 3, /* purple - foreg2 */
	/* 1110 */ 7, /* black around arrows (used in editor) is coded as cyan */
	/* 1111 */ 8, /* white is the arrow */
	};
	int x, y;
	guchar *data;
	int out;
	
	g_assert(image->format->BytesPerPixel==4);

	data=g_new(guchar, image->w*image->h+1);
	out=0;
	data[out++]=image->w/NUM_OF_CELLS_X;
	
	SDL_LockSurface(image);
	for (y=0; y<image->h; y++) {
		guint32 *p=(guint32 *)((char *)image->pixels+y*image->pitch);

		for (x=0; x<image->w; x++) {
			int r, g, b, a, c;

			r=(p[x]&image->format->Rmask) >> image->format->Rshift;
			g=(p[x]&image->format->Gmask) >> image->format->Gshift;
			b=(p[x]&image->format->Bmask) >> image->format->Bshift;
			a=(p[x]&image->format->Amask) >> image->format->Ashift;
			c=(a>>7)*8 + (b>>7)*4 + (g>>7)*2 + (r>>7)*1; /* lower 4 bits will be rgba */

			data[out++]=cols[c];
		}
	}
	SDL_UnlockSurface(image);
	return data;
}


/* returns true, if the given pixbuf seems to be a c64 imported image. */
static gboolean
check_if_pixbuf_c64_png(SDL_Surface *image)
{
	guchar *p;
	int x, y;

	g_assert(image->format->Amask!=0);	
	g_assert(image->format->BytesPerPixel==4);	
	SDL_LockSurface(image);
	for (y=0; y<image->h; y++) {
		p=(guchar *)image->pixels + y * image->pitch;
		for (x=0; x<image->w*image->format->BytesPerPixel; x++)
			if (p[x]!=0 && p[x]!=255)
				return FALSE;
	}
	SDL_UnlockSurface(image);
	return TRUE;
}

gboolean
gd_loadcells_file(const char *filename)
{
	SDL_Surface *surface, *converted;

	/* load cell graphics */
	/* load from file */
	surface=SDL_LoadBMP(filename);

	if (!surface) {
		g_warning("unable to load image");
		return FALSE;
	}

	if ((surface->w % NUM_OF_CELLS_X != 0) || (surface->h % NUM_OF_CELLS_Y != 0) || (surface->w / NUM_OF_CELLS_X != surface->h / NUM_OF_CELLS_Y)) {
		g_warning("Image should contain %d cells in a row and %d in a column!", NUM_OF_CELLS_X, NUM_OF_CELLS_Y);
		SDL_FreeSurface(surface);
		return FALSE;
	}

	/* convert to 32bit rgba */
	converted=SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, surface->h, 32, rmask, gmask, bmask, amask);
	SDL_BlitSurface(surface, NULL, converted, NULL);
	SDL_FreeSurface(surface);
	surface=converted;

	if (check_if_pixbuf_c64_png(surface)) {
		/* c64 pixbuf with a small number of colors which can be changed */
		g_free(c64_custom_gfx);
		c64_custom_gfx=c64_gfx_data_from_pixbuf(surface);
		using_png_gfx=FALSE;
	} else {
		/* normal, "truecolor" pixbuf */
		g_free(c64_custom_gfx);
		c64_custom_gfx=NULL;
		using_png_gfx=TRUE;
		loadcells(surface);
	}
	SDL_FreeSurface(surface);
	
	return TRUE;
}



/* C64 FONT HANDLING */

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

	col=rgba_pixel_from_color(image->format, color, 0xff);
	black=rgba_pixel_from_color(image->format, 0, 0xff);
	
	/* for all characters */
	/* render character in "image", then do a displayformat(image) to generate the resulting one */
	for (j=0; j<GD_NUM_OF_CHARS; j++) {
		int x1, y1;
		
		y1=(j/16)*8;	/* 16 characters in a row */
		x1=(j%16)*8;
		
		for (y=0; y<8; y++) {
			guint32 *p=(guint32*) ((char *)image->pixels + y*image->pitch);
			guint32 *p_n=(guint32*) ((char *)image_n->pixels + y*image_n->pitch);

			for (x=0; x<8; x++) {
				if (font[(y1+y)*128+x1+x]) {
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
		fw[j]=SDL_DisplayFormat(image);
		fn[j]=SDL_DisplayFormat(image_n);
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





/* clears one line on the screen. takes dark screen or totally black screen into consideration. */
void
gd_clear_line(SDL_Surface *screen, int y)
{
	SDL_Rect rect;
	
	rect.x=0;
	rect.y=y;
	rect.w=screen->w;
	rect.h=8;	/* FIXME HACK */
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
			if (c==GD_PLAYER_CHAR || c==GD_DIAMOND_CHAR || c==GD_KEY_CHAR)	/* special, by gdash */
				i=c;
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
				i=GD_BACKSLASH_CHAR;
			else
			if (c=='_')
				i=100;
			else
				i=GD_UNKNOWN_CHAR;
			
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
	if (using_png_gfx) {
		/* nothing to do */
	} else
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

/* after loading the cells, the dark gray background - which is
   a tiled steel wall - can be created.
   this function is to be called after initializing the application
   and having done gd_loadcells_default.
 */
void
gd_create_dark_background()
{
	int x, y;
	
	g_assert(gd_screen!=NULL);
	
	if (dark_background)
		SDL_FreeSurface(dark_background);

	gd_select_pixbuf_colors(0x000000, 0x101010, 0x202020, 0x303030, 0x404040, 0x505050);		/* dark gray colors */
	dark_background=SDL_CreateRGBSurface(0, gd_screen->w, gd_screen->h, 32, 0, 0, 0, 0);
	for (y=0; y<gd_screen->h; y+=cells[0]->h)
		for (x=0; x<gd_screen->w; x+=cells[0]->w) {
			SDL_Rect rect;
			
			rect.x=x;
			rect.y=y;
			
			SDL_BlitSurface(cells[gd_elements[O_STEEL].image], NULL, dark_background, &rect);
		}
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





