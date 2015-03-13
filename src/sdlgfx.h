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
#include "settings.h"

/* these can't be larger than 127, or they mess up with utf8 coding */
#define GD_PLAYER_CHAR 28
#define GD_KEY_CHAR 29
#define GD_DIAMOND_CHAR 30

extern SDL_Surface *gd_screen;
extern int gd_scale;
extern int statusbar_y1;
extern int statusbar_y2;
extern int statusbar_height;
extern int statusbar_mid;
extern Uint8 *gd_keystate;
extern SDL_Joystick *gd_joy;

extern gboolean gd_quit;

int gd_drawcave(SDL_Surface *dest, const Cave *cave, int **gfx_buffer);
gboolean gd_sdl_init(GdScalingType scaling_type);
gboolean gd_scroll(const Cave *cave, gboolean exact_scroll);
void gd_scroll_to_origin();

void gd_select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5);
gboolean gd_is_surface_ok_for_theme(SDL_Surface *surface);
void gd_loadcells_default();
gboolean gd_loadcells_file(const char *filename);

void gd_loadfont_file(const char *filename);
void gd_loadfont_default();

SDL_Surface **gd_get_title_animation();

/* write text to gd_screen. return the next usable x coordinate */
/* pass x=-1 to center on screen */
int gd_blittext(SDL_Surface *screen, int x, int y, GdColor color, const char *text);
int gd_blittext_n(SDL_Surface *screen, int x, int y, GdColor color, const char *text);
int gd_blittext_printf(SDL_Surface *screen, int x, int y, GdColor color, const char *format, ...);
int gd_blittext_printf_n(SDL_Surface *screen, int x, int y, GdColor color, const char *format, ...);

void gd_clear_line(SDL_Surface *screen, int y);

gboolean gd_left();
gboolean gd_right();
gboolean gd_up();
gboolean gd_down();
gboolean gd_fire();

gboolean gd_space_or_enter_or_fire();

void gd_create_dark_background();
void gd_backup_and_dark_screen();
void gd_backup_and_black_screen();
void gd_restore_screen();

void gd_process_pending_events();

void gd_wait_for_key_releases();

int gd_font_height();
int gd_font_width();
int gd_line_height();

void gd_load_theme();


#endif

