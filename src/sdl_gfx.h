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
#ifndef _GD_SDL_GFX_H
#define _GD_SDL_GFX_H
#include <SDL/SDL.h>
#include "cave.h"

/* these can't be larger than 127, or they mess up with utf8 coding */
#define GD_PLAYER_CHAR 28
#define GD_DIAMOND_CHAR 30

extern SDL_Surface *gd_screen;
extern const int statusbar_y1;
extern const int statusbar_y2;
extern const int statusbar_height;
extern const int statusbar_mid;
extern Uint8 *gd_keystate;
extern SDL_Joystick *gd_joy;

extern gboolean gd_quit;

int gd_drawcave(SDL_Surface *dest, const Cave *cave, int **gfx_buffer);
gboolean gd_sdl_init();
gboolean gd_scroll(const Cave *cave, gboolean exact_scroll);


void gd_select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5);
void gd_loadcells_default();
gboolean gd_loadcells_file(const char *filename);

void gd_loadfont_file(const char *filename);
void gd_loadfont_default();

SDL_Surface *gd_get_titleimage();

/* write text to gd_screen. return the next usable x coordinate */
/* pass x=-1 to center on screen */
int gd_blittext(SDL_Surface *screen, int x, int y, int color, const char *text);
int gd_blittext_n(SDL_Surface *screen, int x, int y, int color, const char *text);
int gd_blittext_printf(SDL_Surface *screen, int x, int y, int color, const char *format, ...);
int gd_blittext_printf_n(SDL_Surface *screen, int x, int y, int color, const char *format, ...);

void gd_clear_line(SDL_Surface *screen, int y);

gboolean gd_left();
gboolean gd_right();
gboolean gd_up();
gboolean gd_down();
gboolean gd_fire();

gboolean gd_space_or_fire();

char *gd_select_file(const char *title, const char *directory, const char *glob);

char *gd_wraptext(const char *orig, int width);


void gd_create_dark_background();
void gd_backup_and_dark_screen();
void gd_backup_and_black_screen();
void gd_restore_screen();

void gd_wait_for_key_releases();



#endif

