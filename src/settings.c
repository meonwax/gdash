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
#include "config.h"

#define SETTINGS_INI_FILE "gdash.ini"
#define SETTINGS_GDASH_GROUP "GDash"

/* editor settings */
#define SETTING_GAME_VIEW "game_view"
gboolean gd_game_view=TRUE;	/**< show animated cells instead of arrows & ... */
#define SETTING_COLORED_OBJECTS "colored_objects"
gboolean gd_colored_objects=TRUE;	/**< show objects with different color */
#define SETTING_SHOW_OBJECT_LIST "show_object_list"
gboolean gd_show_object_list=TRUE;	/**< show object list */
#define SETTING_SHOW_TEST_LABEL "show_test_label"
gboolean gd_show_test_label=TRUE;	/**< show a label with some variables, for testing */

/* settings */
#define SETTING_EASY_PLAY "easy_play"
gboolean gd_easy_play=FALSE;
#define SETTING_TIME_MIN_SEC "time_min_sec"
gboolean gd_time_min_sec=TRUE;
#define SETTING_ALL_CAVES_SELECTABLE "all_caves_selectable"
gboolean gd_all_caves_selectable=FALSE;
#define SETTING_MOUSE_PLAY "mouse_play"
gboolean gd_mouse_play=FALSE;
#define SETTING_RANDOM_COLORS "random_colors"
gboolean gd_random_colors=FALSE;
#define SETTING_TV_EMULATION "tv_emulation"
gboolean gd_tv_emulation=TRUE;
#define SETTING_GFX_INTERPOLATION "gfx_interpolation"
gboolean gd_gfx_interpolation=FALSE;
#define SETTING_ALLOW_DIRT_MOD "allow_dirt_mod"
gboolean gd_allow_dirt_mod=TRUE;
#define SETTING_SHOW_PREVIEW "show_preview"
gboolean gd_show_preview=TRUE;
#define SETTING_CELL_SCALE "cell_scale"
int gd_cell_scale=1;
#define SETTING_THEME "theme"
char *gd_theme;
#define SETTING_USE_BDCFF_HIGHSCORE "use_bdcff_highscore"
gboolean gd_use_bdcff_highscore=FALSE;

/* sdl settings */
#define SETTING_SDL_FULLSCREEN "sdl_fullscreen"
gboolean gd_sdl_fullscreen=FALSE;
#define SETTING_SDL_SOUND "sdl_sound"
gboolean gd_sdl_sound=TRUE;
#define SETTING_SDL_16BIT_MIXING "sdl_16bit_mixing"
gboolean gd_sdl_16bit_mixing=FALSE;

char *gd_user_config_dir;
char *gd_system_data_dir;
char *gd_system_caves_dir;

/* command line parameters */
int gd_param_cave=0, gd_param_level=1, gd_param_internal=0;
int gd_param_license=0;
char **gd_param_cavenames=NULL;



/* gets boolean value from key file; returns def if not found or unreadable */
static gboolean
keyfile_get_boolean_with_default(GKeyFile *keyfile, const char *group, const char *key, gboolean def)
{
	GError *error=NULL;
	gboolean result;
	
	result=g_key_file_get_boolean(keyfile, group, key, &error);
	if (!error)
		return result;
	g_warning(error->message);
	g_error_free(error);
	return def;
}

/* gets integer value from key file; returns def if not found or unreadable */
static int
keyfile_get_integer_with_default(GKeyFile *keyfile, const char *group, const char *key, int def)
{
	GError *error=NULL;
	int result;
	
	result=g_key_file_get_integer(keyfile, group, key, &error);
	if (!error)
		return result;
	g_warning(error->message);
	g_error_free(error);
	return def;
}

/* sets up directiories and loads translations */
void gd_settings_init_with_language()
{
#ifdef G_OS_WIN32
	/* on win32, use the glib function. */
	gd_system_data_dir=g_win32_get_package_installation_directory (NULL, NULL);
	gd_system_caves_dir=g_build_path (G_DIR_SEPARATOR_S, gd_system_data_dir, "caves", NULL);
	bindtextdomain (PACKAGE, gd_system_data_dir);
	/* these would not be needed for the sdl version, but they do not hurt */
	bindtextdomain ("gtk20-properties", gd_system_data_dir);
	bindtextdomain ("gtk20", gd_system_data_dir);
#else
	/* on linux, this is a defined, built-in string, $perfix/share/locale */
	gd_system_data_dir=PKGDATADIR;
	gd_system_caves_dir=PKGDATADIR;
	/* and translated strings here. */
	bindtextdomain (PACKAGE, LOCALEDIR);
#endif
	gd_user_config_dir=g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), PACKAGE, NULL);
	/* gtk always uses utf8, so convert translated strings to utf8 if needed */
	bind_textdomain_codeset (PACKAGE, "UTF-8");
}




/* load settings from .config/gdash/gdash.ini */
void
gd_load_settings()
{
    GKeyFile *ini;
    GError *error=NULL;
    gchar *data;
    gsize length;
    char *filename;
    gboolean success;
    
    filename=g_build_path(G_DIR_SEPARATOR_S, gd_user_config_dir, SETTINGS_INI_FILE, NULL);
    
    if (!g_file_get_contents(filename, &data, &length, &error)) {
        /* no ini file found */
        g_warning("%s: %s", filename, error->message);
        g_error_free(error);
        error=NULL;
        return;
    }
    g_free(filename);

	/* if zero length file, also return */
    if (length==0)
    	return;

    ini=g_key_file_new();
    success=g_key_file_load_from_data(ini, data, length, 0, &error);
    g_free(data);
    if (!success) {
        g_warning("Ini file contents error: %s", error->message);
        g_error_free(error);
        error=NULL;
        return;
    }

    gd_game_view=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_GAME_VIEW, gd_game_view);
    gd_colored_objects=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_COLORED_OBJECTS, gd_colored_objects);
    gd_show_object_list=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_OBJECT_LIST, gd_show_object_list);
    gd_show_test_label=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_TEST_LABEL, gd_show_test_label);
    gd_easy_play=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_EASY_PLAY, gd_easy_play);
    gd_time_min_sec=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_TIME_MIN_SEC, gd_time_min_sec);
    gd_all_caves_selectable=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_ALL_CAVES_SELECTABLE, gd_all_caves_selectable);
    gd_mouse_play=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_MOUSE_PLAY, gd_mouse_play);
    gd_random_colors=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_RANDOM_COLORS, gd_random_colors);
    gd_tv_emulation=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_TV_EMULATION, gd_tv_emulation);
    gd_gfx_interpolation=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_GFX_INTERPOLATION, gd_gfx_interpolation);
    gd_allow_dirt_mod=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_ALLOW_DIRT_MOD, gd_allow_dirt_mod);
    gd_show_preview=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_PREVIEW, gd_show_preview);
    gd_use_bdcff_highscore=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_USE_BDCFF_HIGHSCORE, gd_use_bdcff_highscore);
    gd_sdl_fullscreen=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_FULLSCREEN, gd_sdl_fullscreen);
    gd_sdl_sound=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_SOUND, gd_sdl_sound);
    gd_sdl_16bit_mixing=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_16BIT_MIXING, gd_sdl_16bit_mixing);
    gd_cell_scale=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_CELL_SCALE, gd_cell_scale);
	gd_theme=g_key_file_get_string(ini, SETTINGS_GDASH_GROUP, SETTING_THEME, NULL);

    g_key_file_free(ini);
}


/* save settings to .config/gdash.ini */
void
gd_save_settings()
{
    GKeyFile *ini;
    GError *error=NULL;
    gchar *data;
    char *filename;
    
    ini=g_key_file_new();
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_GAME_VIEW, gd_game_view);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_COLORED_OBJECTS, gd_colored_objects);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_OBJECT_LIST, gd_show_object_list);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_TEST_LABEL, gd_show_test_label);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_EASY_PLAY, gd_easy_play);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_TIME_MIN_SEC, gd_time_min_sec);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_ALL_CAVES_SELECTABLE, gd_all_caves_selectable);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_MOUSE_PLAY, gd_mouse_play);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_ALLOW_DIRT_MOD, gd_allow_dirt_mod);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_RANDOM_COLORS, gd_random_colors);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_TV_EMULATION, gd_tv_emulation);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_GFX_INTERPOLATION, gd_gfx_interpolation);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_PREVIEW, gd_show_preview);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_CELL_SCALE, gd_cell_scale);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_USE_BDCFF_HIGHSCORE, gd_use_bdcff_highscore);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_FULLSCREEN, gd_sdl_fullscreen);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_SOUND, gd_sdl_sound);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_16BIT_MIXING, gd_sdl_16bit_mixing);
    if (gd_theme)
	    g_key_file_set_string(ini, SETTINGS_GDASH_GROUP, SETTING_THEME, gd_theme);
    data=g_key_file_to_data(ini, NULL, &error);
    g_key_file_free(ini);
    if (error) {
    	/* this is highly unlikely - why would g_key_file_to_data report error? docs do not mention. */
        g_warning("Unable to save settings: %s", error->message);
        g_error_free(error);
        g_free(data);
        return;
    }
    
    filename=g_build_path (G_DIR_SEPARATOR_S, gd_user_config_dir, SETTINGS_INI_FILE, NULL);
	g_mkdir_with_parents(gd_user_config_dir, 0700);
    g_file_set_contents(filename, data, -1, &error);
    g_free(filename);
    if (error) {
    	/* error saving the file */
        g_warning("Unable to save settings: %s", error->message);
        g_error_free(error);
        g_free(data);
        return;
    }
    g_free(data);
}




GOptionContext *
gd_option_context_new()
{
	GOptionEntry entries[]={
		{"cave", 'c', 0, G_OPTION_ARG_INT, &gd_param_cave, N_("Select cave number C"), "C"},
		{"level", 'l', 0, G_OPTION_ARG_INT, &gd_param_level, N_("Select level number L"), "L"},
		{"internal", 'i', 0, G_OPTION_ARG_INT, &gd_param_internal, N_("Load internal caveset number I"), "I"},
		{"license", 'L', 0, G_OPTION_ARG_NONE, &gd_param_license, N_("Show license and quit")},
		{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &gd_param_cavenames, N_("Cave names")},
		{NULL}
	};
	GOptionContext *context;

	context=g_option_context_new (_("[FILE NAME]"));
	g_option_context_set_help_enabled (context, TRUE);
	g_option_context_add_main_entries (context, entries, PACKAGE);	/* gdash parameters */
	
	return context;
}



