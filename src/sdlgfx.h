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
#ifndef _GD_SDL_GFX_H
#define _GD_SDL_GFX_H
#include <SDL/SDL.h>
#include "cave.h"
#include "settings.h"
#include "gameplay.h"

/* these can't be larger than 31, or they mess up utf8 coding or are the same as some ascii letter */
#define GD_DOWN_CHAR 1
#define GD_LEFT_CHAR 2
#define GD_UP_CHAR 3
#define GD_RIGHT_CHAR 4

#define GD_BALL_CHAR 5
#define GD_UNCHECKED_BOX_CHAR 6
#define GD_CHECKED_BOX_CHAR 7

#define GD_PLAYER_CHAR 8
#define GD_DIAMOND_CHAR 9
#define GD_SKELETON_CHAR 11
#define GD_KEY_CHAR 12
#define GD_COMMENT_CHAR 13


extern SDL_Surface *gd_screen;
extern int gd_scale;
extern int gd_statusbar_y1;
extern int gd_statusbar_y2;
extern int gd_statusbar_height;
extern int gd_statusbar_mid;
extern Uint8 *gd_keystate;

extern gboolean gd_quit;



/* color sets for different status bar types. */
typedef struct _status_bar_colors {
    GdColor background;
    GdColor diamond_needed;
    GdColor diamond_value;
    GdColor diamond_collected;
    GdColor score;
    GdColor default_color;
} GdStatusBarColors;

int gd_drawcave(SDL_Surface *dest, GdGame *gameplay);
gboolean gd_sdl_init(GdScalingType scaling_type);
gboolean gd_scroll(GdGame *gameplay, gboolean exact_scroll);
void gd_scroll_to_origin();

void gd_clear_header(GdColor c);
void gd_showheader_uncover(const GdGame *game, const GdStatusBarColors *cols, gboolean show_replay_sign);
void gd_showheader_game(const GdGame *game, int timeout_since, const GdStatusBarColors *cols, gboolean show_replay_sign);
void gd_play_game_select_status_bar_colors(GdStatusBarColors *cols, const GdCave *cave);




void gd_select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5);
gboolean gd_is_surface_ok_for_theme(SDL_Surface *surface);
void gd_loadcells_default();
gboolean gd_loadcells_file(const char *filename);

void gd_loadfont_file(const char *filename);
void gd_loadfont_default();

SDL_Surface **gd_get_title_animation(gboolean one_frame_only);

/* write text to gd_screen. return the next usable x coordinate */
/* pass x=-1 to center on screen */
int gd_blittext(SDL_Surface *screen, int x, int y, GdColor color, const char *text);
int gd_blittext_n(SDL_Surface *screen, int x, int y, GdColor color, const char *text);
int gd_blittext_printf(SDL_Surface *screen, int x, int y, GdColor color, const char *format, ...);
int gd_blittext_printf_n(SDL_Surface *screen, int x, int y, GdColor color, const char *format, ...);
void gd_blitlines_n(SDL_Surface *screen, int x, int y, GdColor color, char **text, int maxlines, int first);

void gd_clear_line(SDL_Surface *screen, int y);

gboolean gd_left();
gboolean gd_right();
gboolean gd_up();
gboolean gd_down();
gboolean gd_fire();

gboolean gd_space_or_enter_or_fire();

void gd_dark_screen();
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

const char *gd_key_name(guint keysym);


#endif

