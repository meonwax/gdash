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
#ifndef _GD_SETTINGS_H
#define _GD_SETTINGS_H
#include <glib.h>
#include "config.h"
#include "colors.h"

typedef enum _scaling_type {
    GD_SCALING_ORIGINAL,
    GD_SCALING_2X,
    GD_SCALING_2X_BILINEAR,    /* 2x with interpolation */
    GD_SCALING_2X_SCALE2X,    /* 2x with scale2x algorithm */
    GD_SCALING_3X,
    GD_SCALING_3X_BILINEAR,    /* 3x with interpolation */
    GD_SCALING_3X_SCALE3X,    /* 3x with scale3x */
    GD_SCALING_4X,
    GD_SCALING_4X_BILINEAR,    /* 4x with interpolation */
    GD_SCALING_4X_SCALE4X,    /* 4x with scale2x applied twice */
    GD_SCALING_MAX,
} GdScalingType;

extern const gchar *gd_scaling_name[];
extern const int gd_scaling_scale[];

extern const gchar *gd_languages_names[];

/* command line parameters */
extern int gd_param_cave, gd_param_level, gd_param_internal;
extern int gd_param_license;
extern char **gd_param_cavenames;




/* GTK settings */
#ifdef USE_GTK    /* only if having gtk */

/* editor settings */
extern gboolean gd_game_view;    /* show animated cells instead of arrows & ... */
extern gboolean gd_colored_objects;    /* show objects with different color */
extern gboolean gd_show_object_list;    /* show object list */
extern gboolean gd_show_test_label;    /* show a label with some variables, for testing */
extern int gd_editor_window_width;    /* window size */
extern int gd_editor_window_height;    /* window size */

/* preferences */
extern int gd_language;
extern gboolean gd_time_min_sec;
extern gboolean gd_mouse_play;
extern gboolean gd_show_preview;

/* graphics */
extern char *gd_theme;
extern GdScalingType gd_cell_scale_game;
extern gboolean gd_pal_emulation_game;
extern GdScalingType gd_cell_scale_editor;
extern gboolean gd_pal_emulation_editor;

/* keyboard */
extern guint gd_gtk_key_left;
extern guint gd_gtk_key_right;
extern guint gd_gtk_key_up;
extern guint gd_gtk_key_down;
extern guint gd_gtk_key_fire_1;
extern guint gd_gtk_key_fire_2;
extern guint gd_gtk_key_suicide;

/* html output option */
char *gd_html_stylesheet_filename;
char *gd_html_favicon_filename;


#endif    /* only if having gtk */



/* universal settings */
extern gboolean gd_no_invisible_outbox;
extern gboolean gd_all_caves_selectable;
extern gboolean gd_import_as_all_caves_selectable;
extern gboolean gd_random_colors;
extern gboolean gd_use_bdcff_highscore;
extern int gd_pal_emu_scanline_shade;
extern gboolean gd_fine_scroll;
extern gboolean gd_show_story;

/* palette settings */
extern int gd_c64_palette;
extern int gd_c64dtv_palette;
extern int gd_atari_palette;
extern GdColorType gd_preferred_palette;



#ifdef USE_SDL    /* only if having sdl */
extern guint gd_sdl_key_left;
extern guint gd_sdl_key_right;
extern guint gd_sdl_key_up;
extern guint gd_sdl_key_down;
extern guint gd_sdl_key_fire_1;
extern guint gd_sdl_key_fire_2;
extern guint gd_sdl_key_suicide;
extern gboolean gd_even_line_pal_emu_vertical_scroll;
extern gboolean gd_sdl_fullscreen;
extern GdScalingType gd_sdl_scale;
extern char *gd_sdl_theme;
extern gboolean gd_sdl_pal_emulation;
extern gboolean gd_show_name_of_game;
typedef enum _gd_status_bar_type {
    GD_STATUS_BAR_ORIGINAL,
    GD_STATUS_BAR_1STB,
    GD_STATUS_BAR_CRLI,
    GD_STATUS_BAR_FINAL,
    GD_STATUS_BAR_ATARI_ORIGINAL,
    GD_STATUS_BAR_MAX,
} GdStatusBarType;
extern GdStatusBarType gd_status_bar_type;
#endif    /* use_sdl */




/* sound settings */
#ifdef GD_SOUND
extern gboolean gd_sdl_sound;
extern gboolean gd_sdl_16bit_mixing;
extern gboolean gd_sdl_44khz_mixing;
extern gboolean gd_classic_sound;
extern int gd_sound_chunks_volume_percent;
extern int gd_sound_music_volume_percent;
#endif    /* if gd_sound */





/* gdash directories */
extern char *gd_user_config_dir;
extern char *gd_system_data_dir;
extern char *gd_system_caves_dir;
extern char *gd_system_sound_dir;
extern char *gd_system_music_dir;

/* init settings (directories), and load language files */
void gd_settings_init_dirs();
void gd_settings_set_locale();
void gd_settings_init_translation();

/* settings loading and saving */
void gd_save_settings();
void gd_load_settings();

/* command line arguments parsing */
GOptionContext *gd_option_context_new();

#ifdef USE_SDL
const char **gd_status_bar_type_get_names();
#endif    /* use_sdl */

#endif

