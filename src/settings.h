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
#ifndef _GD_SETTINGS_H
#define _GD_SETTINGS_H
#include <glib.h>

/* command line parameters */
extern int gd_param_cave, gd_param_level, gd_param_internal;
extern int gd_param_license;
extern char **gd_param_cavenames;

/* editor settings */
extern gboolean gd_game_view;	/**< show animated cells instead of arrows & ... */
extern gboolean gd_colored_objects;	/**< show objects with different color */
extern gboolean gd_show_object_list;	/**< show object list in editor */
extern gboolean gd_show_test_label;	/**< label with cave variables in tests */

/* settings */
extern gboolean gd_easy_play;
extern gboolean gd_time_min_sec;
extern gboolean gd_all_caves_selectable;
extern gboolean gd_mouse_play;
extern gboolean gd_random_colors;
extern gboolean gd_tv_emulation;
extern gboolean gd_gfx_interpolation;
extern gboolean gd_show_preview;
extern gboolean gd_allow_dirt_mod;
extern gboolean gd_use_bdcff_highscore;
extern gboolean gd_sdl_fullscreen;
extern gboolean gd_sdl_sound;
extern gboolean gd_sdl_16bit_mixing;
extern int gd_cell_scale;
extern char *gd_theme;

/* gdash directories */
extern char *gd_user_config_dir;
extern char *gd_system_data_dir;
extern char *gd_system_caves_dir;

/* init settings (directories), and load language files */
void gd_settings_init_with_language();

/* settings loading and saving */
void gd_save_settings();
void gd_load_settings();

/* command line arguments parsing */
GOptionContext *gd_option_context_new();

#endif

