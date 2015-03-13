/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
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
#ifndef _GD_SETTINGS
#define _GD_SETTINGS

#include "config.h"

#include <glib.h>
#include <string>
#include <vector>

class GameInputHandler;

/* universal settings */
extern int gd_language;
extern std::string gd_username;
extern std::string gd_theme;
extern bool gd_no_invisible_outbox;
extern bool gd_all_caves_selectable;
extern bool gd_import_as_all_caves_selectable;
extern bool gd_use_bdcff_highscore;
extern int gd_pal_emu_scanline_shade;
extern bool gd_fine_scroll;
extern bool gd_particle_effects;
extern bool gd_show_story;
extern bool gd_show_name_of_game;
extern int gd_status_bar_colors;

/* palette settings */
extern int gd_c64_palette;
extern int gd_c64dtv_palette;
extern int gd_atari_palette;
extern int gd_preferred_palette;


/* GTK settings */

/* editor settings */
extern bool gd_game_view;    /* show animated cells instead of arrows & ... */
extern bool gd_colored_objects;    /* show objects with different color */
extern bool gd_show_object_list;    /* show object list */
extern bool gd_show_test_label;    /* show a label with some variables, for testing */
extern int gd_editor_window_width;    /* window size */
extern int gd_editor_window_height;    /* window size */
extern bool gd_fast_uncover_in_test;

/* preferences */
extern bool gd_show_preview;

/* graphics */
extern bool gd_fullscreen;
extern int gd_cell_scale_game;
extern bool gd_pal_emulation_game;
extern int gd_cell_scale_editor;
extern bool gd_pal_emulation_editor;

/* keyboard */
#ifdef HAVE_GTK
extern int gd_gtk_key_left;
extern int gd_gtk_key_right;
extern int gd_gtk_key_up;
extern int gd_gtk_key_down;
extern int gd_gtk_key_fire_1;
extern int gd_gtk_key_fire_2;
extern int gd_gtk_key_suicide;
extern int gd_gtk_key_fast_forward;
extern int gd_gtk_key_status_bar;
extern int gd_gtk_key_restart_level;
#endif

/* html output option */
extern char *gd_html_stylesheet_filename;
extern char *gd_html_favicon_filename;



/* SDL settings */

#ifdef HAVE_SDL
extern int gd_sdl_key_left;
extern int gd_sdl_key_right;
extern int gd_sdl_key_up;
extern int gd_sdl_key_down;
extern int gd_sdl_key_fire_1;
extern int gd_sdl_key_fire_2;
extern int gd_sdl_key_suicide;
extern int gd_sdl_key_fast_forward;
extern int gd_sdl_key_status_bar;
extern int gd_sdl_key_restart_level;
#endif


/* SOUND settings */

#ifdef HAVE_SDL
extern bool gd_sound_enabled;
extern bool gd_sound_16bit_mixing;
extern bool gd_sound_44khz_mixing;
extern bool gd_sound_stereo;
extern bool gd_classic_sound;
extern int gd_sound_chunks_volume_percent;
extern int gd_sound_music_volume_percent;
#endif    /* if gd_sound */


/* command line parameters */
extern int gd_param_license;
extern char **gd_param_cavenames;

/* gdash directories */
extern const char *gd_user_config_dir;
extern const char *gd_system_data_dir;
extern const char *gd_system_caves_dir;
extern const char *gd_system_music_dir;

extern std::vector<std::string> gd_sound_dirs, gd_themes_dirs, gd_fonts_dirs;

extern const char *gd_languages_names[];

/* init settings (directories), and load language files */
void gd_settings_init();
void gd_settings_init_dirs();
void gd_settings_set_locale();
void gd_settings_init_translation();

/* settings loading and saving */
void gd_save_settings();
void gd_load_settings();

/* command line arguments parsing */
GOptionContext *gd_option_context_new();



/* for settings */
enum SettingType {
    TypePage,
    TypeBoolean,
    TypeTheme,
    TypePercent,
    TypeStringv,
    TypeKey,
};
class Setting {
public:
    SettingType type;
    const char *name;
    void *var;
    bool restart;     // a setting which requires a restart
    const char **stringv;
    char const *description;
    unsigned page;
};

Setting *gd_get_game_settings_array();
Setting *gd_get_keyboard_settings_array(GameInputHandler *gih);

#endif
