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
#include <stdlib.h>
#include "config.h"
#include "settings.h"

#define SETTINGS_INI_FILE "gdash.ini"
#define SETTINGS_GDASH_GROUP "GDash"

/* names of scaling types supported. */
/* scale2x and scale3x are not translated: the license says that we should call it in its original name. */
const gchar *gd_scaling_name[]={N_("Original"), N_("2x nearest"), N_("2x bilinear"), "Scale2x", N_("3x nearest"), N_("3x bilinear"), "Scale3x", N_("4x nearest"), N_("4x bilinear"), "Scale4x", NULL};
/* scaling factors of scaling types supported. */
const int gd_scaling_scale[]={1, 2, 2, 2, 3, 3, 3, 4, 4, 4};

/* possible languages. */
const gchar *gd_languages_names[]={N_("System default"), "English", "Deutsch", "Magyar", NULL};
/* this should correspond to the above one. */
#ifdef G_OS_WIN32
/* locale names used in windows. */
static const gchar *language_locale[][7]={
	{ "", NULL, },
	{ "English", NULL, },
	{ "German", NULL, },
	{ "Hungarian", NULL, },
};
/* these will be used on windows for a putenv to trick gtk. */
/* on linux, the setlocale call works correctly, and this is not needed. */
static const gchar *languages_for_env[]={
	NULL, "en", "de", "hu"
};
#else
/* locale names used in unix. */
/* anyone, a better solution for this? */
static const gchar *language_locale_default[]=
	{ "", NULL, };
static const gchar *language_locale_en[]=
	{ "en_US.UTF-8", "en_US.UTF8", "en_US.ISO8859-15", "en_US.ISO8859-1", "en_US.US-ASCII", "en_US", "en", NULL, };
static const gchar *language_locale_de[]=
	{ "de_DE.UTF-8", "de_DE.UTF8", "de_DE.ISO8859-15", "de_DE.ISO8859-1", "de_DE",
	  "de_AT.UTF-8", "de_AT.UTF8", "de_AT.ISO8859-15", "de_AT.ISO8859-1", "de_AT",
	  "de_CH.UTF-8", "de_CH.UTF8", "de_CH.ISO8859-15", "de_CH.ISO8859-1", "de_CH",
	  "de", NULL, };
static const gchar *language_locale_hu[]=
	{ "hu_HU.UTF-8", "hu_HU.ISO8859-2", "hu_HU", "hu", NULL, };
static const gchar **language_locale[]={
	language_locale_default,
	language_locale_en,
	language_locale_de,
	language_locale_hu,
};
#endif

/* editor settings */
#define SETTING_GAME_VIEW "game_view"
gboolean gd_game_view=TRUE;	/* show animated cells instead of arrows & ... */
#define SETTING_COLORED_OBJECTS "colored_objects"
gboolean gd_colored_objects=TRUE;	/* show objects with different color */
#define SETTING_SHOW_OBJECT_LIST "show_object_list"
gboolean gd_show_object_list=TRUE;	/* show object list */
#define SETTING_SHOW_TEST_LABEL "show_test_label"
gboolean gd_show_test_label=TRUE;	/* show a label with some variables, for testing */

#define SETTING_EDITOR_WINDOW_WIDTH "editor_window_width"
int gd_editor_window_width=800;	/* window size */
#define SETTING_EDITOR_WINDOW_HEIGHT "editor_window_height"
int gd_editor_window_height=520;	/* window size */

#define SETTING_LANGUAGE "language"
int gd_language=0;

/* settings */
#define SETTING_EASY_PLAY "easy_play"
gboolean gd_easy_play=FALSE;
#define SETTING_TIME_MIN_SEC "time_min_sec"
gboolean gd_time_min_sec=TRUE;
#define SETTING_ALL_CAVES_SELECTABLE "all_caves_selectable"
gboolean gd_all_caves_selectable=FALSE;
#define SETTING_IMPORT_AS_ALL_CAVES_SELECTABLE "import_as_all_caves_selectable"
gboolean gd_import_as_all_caves_selectable=TRUE;
#define SETTING_MOUSE_PLAY "mouse_play"
gboolean gd_mouse_play=FALSE;
#define SETTING_RANDOM_COLORS "random_colors"
gboolean gd_random_colors=FALSE;
#define SETTING_ALLOW_DIRT_MOD "allow_dirt_mod"
gboolean gd_allow_dirt_mod=TRUE;
#define SETTING_SHOW_PREVIEW "show_preview"
gboolean gd_show_preview=TRUE;
#define SETTING_USE_BDCFF_HIGHSCORE "use_bdcff_highscore"
gboolean gd_use_bdcff_highscore=FALSE;
#define SETTING_SHOW_NAME_OF_GAME "show_name_of_game"
gboolean gd_show_name_of_game=TRUE;
#define SETTING_THEME "theme"
char *gd_theme;

#define SETTING_CELL_SCALE_GAME "cell_scale_game"
GdScalingType gd_cell_scale_game=GD_SCALING_ORIGINAL;
#define SETTING_PAL_EMULATION_GAME "pal_emulation_game"
gboolean gd_pal_emulation_game=FALSE;
#define SETTING_CELL_SCALE_EDITOR "cell_scale_editor"
GdScalingType gd_cell_scale_editor=GD_SCALING_ORIGINAL;
#define SETTING_PAL_EMULATION_EDITOR "pal_emulation_editor"
gboolean gd_pal_emulation_editor=FALSE;

#define SETTING_PAL_EMU_SCANLINE_SHADE "pal_emu_scanline_shade"
int gd_pal_emu_scanline_shade=85;
#define SETTING_C64_PALETTE "c64_palette"
int gd_c64_palette=0;
#define SETTING_ATARI_PALETTE "atari_palette"
int gd_atari_palette=0;

/* sound settings */
#define SETTING_SDL_SOUND "sdl_sound"
gboolean gd_sdl_sound=TRUE;
#define SETTING_SDL_16BIT_MIXING "sdl_16bit_mixing"
gboolean gd_sdl_16bit_mixing=FALSE;
#define SETTING_SDL_44KHZ_MIXING "sdl_44khz_mixing"
gboolean gd_sdl_44khz_mixing=TRUE;
#define SETTING_CLASSIC_SOUND "classic_sound"
gboolean gd_classic_sound=FALSE;

/* sdl game settings */
#define SETTING_SDL_FULLSCREEN "sdl_fullscreen"
gboolean gd_sdl_fullscreen=FALSE;
#define SETTING_SDL_SCALE "sdl_scale"
GdScalingType gd_sdl_scale=GD_SCALING_ORIGINAL;
#define SETTING_SDL_THEME "sdl_theme"
char *gd_sdl_theme;
#define SETTING_SDL_PAL_EMULATION "sdl_pal_emulation"
gboolean gd_sdl_pal_emulation=FALSE;

char *gd_user_config_dir;
char *gd_system_data_dir;
char *gd_system_caves_dir;
char *gd_system_sound_dir;
char *gd_system_music_dir;

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
void gd_settings_init_dirs()
{
	g_assert(G_N_ELEMENTS(gd_scaling_name)==GD_SCALING_MAX+1);	/* +1 is the terminating NULL */
	g_assert(G_N_ELEMENTS(gd_scaling_scale)==GD_SCALING_MAX);
#ifdef G_OS_WIN32
	/* on win32, use the glib function. */
	gd_system_data_dir=g_win32_get_package_installation_directory (NULL, NULL);
#else
	/* on linux, this is a defined, built-in string, $perfix/share/locale */
	gd_system_data_dir=PKGDATADIR;
#endif
	gd_system_caves_dir=g_build_path(G_DIR_SEPARATOR_S, gd_system_data_dir, "caves", NULL);
	gd_system_sound_dir=g_build_path(G_DIR_SEPARATOR_S, gd_system_data_dir, "sound", NULL);
	gd_system_music_dir=g_build_path(G_DIR_SEPARATOR_S, gd_system_data_dir, "music", NULL);
	gd_user_config_dir=g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), PACKAGE, NULL);
}

/* set locale from the gdash setting variable. */
void
gd_settings_set_locale()
{
	int i;
	char *result;

	if (gd_language<0 || gd_language>=G_N_ELEMENTS(language_locale))
		gd_language=0;	/* switch to default, if out of bounds. */

	/* on windows, we put the LANGUAGE variable into the environment. that seems to be the only
	thing gtk+ reacts upon. we also set the locale below. */
#ifdef G_OS_WIN32
	g_assert(G_N_ELEMENTS(language_locale)==G_N_ELEMENTS(languages_for_env));
	if (languages_for_env[gd_language]) {
		char *env;
		
		env=g_strdup_printf("LANGUAGE=%s", languages_for_env[gd_language]);
		putenv(env);
		g_free(env);
	}
#endif
	
	/* try to set the locale. */
	i=0;
	result=NULL;
	while(result==NULL && language_locale[gd_language][i]!=NULL) {
		result=setlocale(LC_ALL, language_locale[gd_language][i]);
		i++;
	}
	if (result==NULL) {
		/* failed to set */
		g_warning("Failed to set language to '%s'. Switching to system default locale.", gd_languages_names[gd_language]);
		setlocale(LC_ALL, "");
	}
}

/* sets up directiories and loads translations */
void gd_settings_init_translation()
{
#ifdef G_OS_WIN32
	/* these would not be needed for the sdl version, but they do not hurt */
	bindtextdomain("gtk20-properties", gd_system_data_dir);
	bindtextdomain("gtk20", gd_system_data_dir);
	bindtextdomain("glib20", gd_system_data_dir);
	bind_textdomain_codeset("gtk20-properties", "UTF-8");
	bind_textdomain_codeset("gtk20", "UTF-8");
	bind_textdomain_codeset("glib20", "UTF-8");
	/* gdash strings */
	bindtextdomain (PACKAGE, gd_system_data_dir);
	/* gtk always uses utf8, so convert translated strings to utf8 if needed */
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#else
	/* and translated strings here. */
	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif
	textdomain(PACKAGE);
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
    gd_editor_window_width=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_EDITOR_WINDOW_WIDTH, gd_editor_window_width);
    gd_editor_window_height=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_EDITOR_WINDOW_HEIGHT, gd_editor_window_height);
    gd_language=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_LANGUAGE, gd_language);
    gd_easy_play=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_EASY_PLAY, gd_easy_play);
    gd_time_min_sec=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_TIME_MIN_SEC, gd_time_min_sec);
    gd_all_caves_selectable=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_ALL_CAVES_SELECTABLE, gd_all_caves_selectable);
    gd_import_as_all_caves_selectable=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_IMPORT_AS_ALL_CAVES_SELECTABLE, gd_import_as_all_caves_selectable);
    gd_mouse_play=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_MOUSE_PLAY, gd_mouse_play);
    gd_random_colors=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_RANDOM_COLORS, gd_random_colors);
    gd_allow_dirt_mod=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_ALLOW_DIRT_MOD, gd_allow_dirt_mod);
    gd_show_preview=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_PREVIEW, gd_show_preview);
    gd_show_name_of_game=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_NAME_OF_GAME, gd_show_name_of_game);
    gd_use_bdcff_highscore=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_USE_BDCFF_HIGHSCORE, gd_use_bdcff_highscore);
    gd_sdl_fullscreen=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_FULLSCREEN, gd_sdl_fullscreen);
    gd_sdl_sound=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_SOUND, gd_sdl_sound);
    gd_sdl_16bit_mixing=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_16BIT_MIXING, gd_sdl_16bit_mixing);
    gd_sdl_44khz_mixing=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_44KHZ_MIXING, gd_sdl_44khz_mixing);
    gd_classic_sound=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_CLASSIC_SOUND, gd_classic_sound);
    gd_cell_scale_game=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_CELL_SCALE_GAME, gd_cell_scale_game);
    if (gd_cell_scale_game<0 || gd_cell_scale_game>=GD_SCALING_MAX)
    	gd_cell_scale_game=0;
    gd_pal_emu_scanline_shade=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_PAL_EMU_SCANLINE_SHADE, gd_pal_emu_scanline_shade);
    gd_pal_emulation_game=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_PAL_EMULATION_GAME, gd_pal_emulation_game);
    gd_cell_scale_editor=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_CELL_SCALE_EDITOR, gd_cell_scale_editor);
    if (gd_cell_scale_editor<0 || gd_cell_scale_editor>=GD_SCALING_MAX)
    	gd_cell_scale_editor=0;
    gd_pal_emulation_editor=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_PAL_EMULATION_EDITOR, gd_pal_emulation_editor);
    gd_c64_palette=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_C64_PALETTE, gd_c64_palette);
    gd_atari_palette=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_ATARI_PALETTE, gd_atari_palette);
	gd_theme=g_key_file_get_string(ini, SETTINGS_GDASH_GROUP, SETTING_THEME, NULL);
	gd_sdl_theme=g_key_file_get_string(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_THEME, NULL);
    gd_sdl_scale=keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_SCALE, gd_sdl_scale);
    if (gd_sdl_scale<0 || gd_sdl_scale>=GD_SCALING_MAX)
    	gd_sdl_scale=0;
    gd_sdl_pal_emulation=keyfile_get_boolean_with_default(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_PAL_EMULATION, gd_sdl_pal_emulation);

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
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_EDITOR_WINDOW_WIDTH, gd_editor_window_width);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_EDITOR_WINDOW_HEIGHT, gd_editor_window_height);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_LANGUAGE, gd_language);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_EASY_PLAY, gd_easy_play);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_TIME_MIN_SEC, gd_time_min_sec);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_ALL_CAVES_SELECTABLE, gd_all_caves_selectable);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_IMPORT_AS_ALL_CAVES_SELECTABLE, gd_import_as_all_caves_selectable);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_MOUSE_PLAY, gd_mouse_play);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_ALLOW_DIRT_MOD, gd_allow_dirt_mod);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_RANDOM_COLORS, gd_random_colors);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_PAL_EMULATION_GAME, gd_pal_emulation_game);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_PAL_EMU_SCANLINE_SHADE, gd_pal_emu_scanline_shade);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_C64_PALETTE, gd_c64_palette);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_ATARI_PALETTE, gd_atari_palette);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_PAL_EMULATION_EDITOR, gd_pal_emulation_editor);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_PREVIEW, gd_show_preview);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SHOW_NAME_OF_GAME, gd_show_name_of_game);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_CELL_SCALE_GAME, gd_cell_scale_game);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_CELL_SCALE_EDITOR, gd_cell_scale_editor);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_USE_BDCFF_HIGHSCORE, gd_use_bdcff_highscore);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_FULLSCREEN, gd_sdl_fullscreen);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_SOUND, gd_sdl_sound);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_16BIT_MIXING, gd_sdl_16bit_mixing);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_44KHZ_MIXING, gd_sdl_44khz_mixing);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_CLASSIC_SOUND, gd_classic_sound);
    g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_SCALE, gd_sdl_scale);
    g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_PAL_EMULATION, gd_sdl_pal_emulation);

    if (gd_theme)
	    g_key_file_set_string(ini, SETTINGS_GDASH_GROUP, SETTING_THEME, gd_theme);
    if (gd_sdl_theme)
	    g_key_file_set_string(ini, SETTINGS_GDASH_GROUP, SETTING_SDL_THEME, gd_sdl_theme);
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



