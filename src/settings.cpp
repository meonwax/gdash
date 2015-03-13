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

#include "config.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <map>

#include "settings.hpp"
#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "misc/util.hpp"
#include "cave/gamerender.hpp"
#include "gfx/pixbuffactory.hpp"
#include "input/gameinputhandler.hpp"

#ifdef HAVE_SDL
#include <SDL.h>
#endif

#ifdef HAVE_GTK
#include <gdk/gdkkeysyms.h>
#endif


#define SETTINGS_INI_FILE "gdash.ini"
#define SETTINGS_GDASH_GROUP "GDash"

/* possible languages. */
/* they are not translated, so every language name is show in that language itself. */
const char *gd_languages_names[]={N_("System default"), "English", "Deutsch", "Magyar", NULL};

/* this should correspond to the above one. */
#ifdef G_OS_WIN32
    /* locale names used in windows. */
    static const char *language_locale_default[]= { "", NULL, };
    static const char *language_locale_en[]= { "English", NULL, };
    static const char *language_locale_de[]= { "German", NULL, };
    static const char *language_locale_hu[]= { "Hungarian", NULL, };
    /* these will be used on windows for a putenv to trick gtk. */
    /* on linux, the setlocale call works correctly, and this is not needed. */
    static const char *languages_for_env[]={ NULL, "en", "de", "hu" };
#else
    /* locale names used in unix. */
    /* anyone, a better solution for this? */
    static const char *language_locale_default[]=
        { "", NULL, };
    static const char *language_locale_en[]=
        { "en_US.UTF-8", "en_US.UTF8", "en_US.ISO8859-15", "en_US.ISO8859-1", "en_US.US-ASCII", "en_US", "en", NULL, };
    static const char *language_locale_de[]=
        { "de_DE.UTF-8", "de_DE.UTF8", "de_DE.ISO8859-15", "de_DE.ISO8859-1", "de_DE",
          "de_AT.UTF-8", "de_AT.UTF8", "de_AT.ISO8859-15", "de_AT.ISO8859-1", "de_AT",
          "de_CH.UTF-8", "de_CH.UTF8", "de_CH.ISO8859-15", "de_CH.ISO8859-1", "de_CH",
          "de", NULL, };
    static const char *language_locale_hu[]=
        { "hu_HU.UTF-8", "hu_HU.ISO8859-2", "hu_HU", "hu", NULL, };
#endif    /* ifdef g_os_win32 else */

/* put the locales to be tried in an array - same for windows and unix */
static const char **language_locale[]={
    language_locale_default,
    language_locale_en,
    language_locale_de,
    language_locale_hu,
};


static std::map<char const *, int *> settings_integers;
static std::map<char const *, bool *> settings_bools;
static std::map<char const *, std::string *> settings_strings;


/* universal settings */
std::string gd_username;
std::string gd_theme;
bool gd_no_invisible_outbox=false;
bool gd_all_caves_selectable=false;
bool gd_import_as_all_caves_selectable=false;
bool gd_use_bdcff_highscore=false;
int gd_pal_emu_scanline_shade=80;
bool gd_fine_scroll=false;
bool gd_particle_effects=true;
bool gd_show_story=true;
bool gd_show_name_of_game=true;
int gd_status_bar_colors=GD_STATUS_BAR_ORIGINAL;

/* palette settings */
int gd_c64_palette=0;
int gd_c64dtv_palette=0;
int gd_atari_palette=0;
int gd_preferred_palette=GdColor::TypeRGB;
    /* editor settings */
    bool gd_game_view=true;    /* show animated cells instead of arrows & ... */
    bool gd_colored_objects=true;    /* show objects with different color */
    bool gd_show_object_list=true;    /* show object list */
    bool gd_show_test_label=true;    /* show a label with some variables, for testing */
    int gd_editor_window_width=800;    /* window size */
    int gd_editor_window_height=520;    /* window size */
    bool gd_fast_uncover_in_test = true;

    /* preferences */
    int gd_language=0;
    bool gd_show_preview=true;

    /* graphics */
    bool gd_fullscreen = false;
    int gd_cell_scale_game=GD_SCALING_2X;
    bool gd_pal_emulation_game=false;
    int gd_cell_scale_editor=GD_SCALING_ORIGINAL;
    bool gd_pal_emulation_editor=false;

    /* html output option */
    /* CURRENTLY ONLY FROM THE COMMAND LINE */
    char *gd_html_stylesheet_filename=NULL;
    char *gd_html_favicon_filename=NULL;

/* GTK keyboard settings */
#ifdef HAVE_GTK    /* only if having gtk */
    int gd_gtk_key_left=GDK_Left;
    int gd_gtk_key_right=GDK_Right;
    int gd_gtk_key_up=GDK_Up;
    int gd_gtk_key_down=GDK_Down;
    int gd_gtk_key_fire_1=GDK_Control_L;
    int gd_gtk_key_fire_2=GDK_Control_R;
    int gd_gtk_key_suicide=GDK_s;
    int gd_gtk_key_fast_forward=GDK_f;
    int gd_gtk_key_status_bar=GDK_Shift_L;
    int gd_gtk_key_restart_level=GDK_Escape;
#endif    /* only if having gtk */

/* SDL settings */
#ifdef HAVE_SDL
    int gd_sdl_key_left=SDLK_LEFT;
    int gd_sdl_key_right=SDLK_RIGHT;
    int gd_sdl_key_up=SDLK_UP;
    int gd_sdl_key_down=SDLK_DOWN;
    int gd_sdl_key_fire_1=SDLK_LCTRL;
    int gd_sdl_key_fire_2=SDLK_RCTRL;
    int gd_sdl_key_suicide=SDLK_s;
    int gd_sdl_key_fast_forward=SDLK_f;
    int gd_sdl_key_status_bar=SDLK_LSHIFT;
    int gd_sdl_key_restart_level=SDLK_ESCAPE;
#endif    /* use_sdl */

/* sound settings */
#ifdef HAVE_SDL
    bool gd_sound_enabled=true;
    bool gd_sound_16bit_mixing=true;
    bool gd_sound_44khz_mixing=true;
    bool gd_sound_stereo=true;
    bool gd_classic_sound=false;
    int gd_sound_chunks_volume_percent=50;
    int gd_sound_music_volume_percent=50;
#endif    /* if gd_sound */


/* some directories the game uses */
const char *gd_user_config_dir;
const char *gd_system_data_dir;
const char *gd_system_caves_dir;
const char *gd_system_music_dir;

std::vector<std::string> gd_sound_dirs, gd_themes_dirs, gd_fonts_dirs;

/* command line parameters */
int gd_param_license=0;
char **gd_param_cavenames=NULL;


static void set_page_numbers(Setting *settings) {
    int page = -1;
    for (size_t i = 0; settings[i].name != NULL; ++i) {
        if (settings[i].type == TypePage)
            page++;
        settings[i].page = page;
    }
}


Setting *gd_get_game_settings_array() {
    static Setting settings_static[] = {
        { TypePage, _("Game") },
        { TypeStringv, _("Language"), &gd_language, false, gd_languages_names, _("The language of the application. Changing this setting requires a restart!") },
        { TypeBoolean, _("All caves selectable"), &gd_all_caves_selectable, false, NULL, _("All caves and intermissions can be selected at game start.") },
        { TypeBoolean, _("Import as all selectable"), &gd_import_as_all_caves_selectable, false, NULL, _("Original, C64 games are imported not with A, E, I, M caves selectable, but all caves (ABCD, EFGH... excluding intermissions). This does not affect BDCFF caves.") },
        { TypeBoolean, _("Use BDCFF highscore"), &gd_use_bdcff_highscore, false, NULL, _("Use BDCFF highscores. GDash saves highscores in its own configuration directory and also in the *.bd files. However, it prefers loading them from the configuration directory; as the *.bd files might be read-only. You can enable this setting to let GDash load them from the *.bd files. This can be selected for a specific file in the file open dialog, too.") },
        { TypeBoolean, _("Show story"), &gd_show_story, false, NULL, _("If the cave has a story, it will be shown when the cave is first started.") },
        { TypeBoolean, _("Game name at uncover"), &gd_show_name_of_game, false, NULL, _("Show the name of the game when uncovering a cave.") },
        { TypeBoolean, _("No invisible outbox"), &gd_no_invisible_outbox, false, NULL, _("Show invisible outboxes as visible (blinking) ones.") },

        { TypePage, _("Game graphics") },
        { TypeTheme,   _("Theme"), NULL, false },
        { TypeStringv, _("Scale"), &gd_cell_scale_game, true, gd_scaling_name, _("Scaling factor and method.") },
        { TypeBoolean, _("PAL emulation"), &gd_pal_emulation_game, true, NULL, _("Use PAL emulated graphics, i.e. lines are striped, and colors are distorted like on a TV.") },
        { TypePercent, _("  PAL scanline shade"), &gd_pal_emu_scanline_shade, true, NULL, _("Darker rows for PAL emulation.") },
        { TypeBoolean, _("Particle effects"), &gd_particle_effects, false, NULL, _("Particle effects during play.") },
        { TypeBoolean, _("Fine scrolling"), &gd_fine_scroll, false, NULL, _("50 frames per second scrolling.") },
        { TypeStringv, _("Status bar colors"), &gd_status_bar_colors, false, gd_status_bar_colors_get_names(), _("Preferred status bar color scheme. Only affects the colors, not the status bar layout.") },
        { TypeStringv, _("  C64 palette"), &gd_c64_palette, false, GdColor::get_c64_palette_names(), _("The color palette for games imported from C64 files.") },
        { TypeStringv, _("  C64DTV palette"), &gd_c64dtv_palette, false, GdColor::get_c64dtv_palette_names(), _("The color palette for imported C64 DTV games.") },
        { TypeStringv, _("  Atari palette"), &gd_atari_palette, false, GdColor::get_atari_palette_names(), _("The color palette for imported Atari games.") },
        { TypeStringv, _("  Preferred palette"), &gd_preferred_palette, false, GdColor::get_palette_types_names(), _("New caves and random colored caves use this palette.") },

        { TypePage, _("Editor graphics") },
        { TypeStringv, _("Scale"), &gd_cell_scale_editor, true, gd_scaling_name, _("Scaling factor and method.") },
        { TypeBoolean, _("PAL emulation"), &gd_pal_emulation_editor, true, NULL, _("Use PAL emulated graphics, i.e. lines are striped, and colors are distorted like on a TV.") },
        { TypeBoolean, _("Animated view"), &gd_game_view, false, NULL, _("Show simplified view of cave in the editor.") },
        { TypeBoolean, _("Colored objects"), &gd_colored_objects, false, NULL, _("Cave objects are colored, to make them different from random cave elements.") },
        { TypeBoolean, _("Object list"), &gd_show_object_list, false, NULL, _("Show objects list sidebar in the editor.") },
        { TypeBoolean, _("Fast uncover"), &gd_fast_uncover_in_test, false, NULL, _("Fast uncovering and covering of cave in editor cave test.") },

    #ifdef HAVE_SDL
        { TypePage, _("Sound") },
        { TypeBoolean, _("Sound"), &gd_sound_enabled, true, NULL, _("Play sounds and music in the program.") },
        { TypeBoolean, _("Classic sounds only"), &gd_classic_sound, true, NULL, _("Play only classic sounds taken from the original game.") },
        { TypeBoolean, _("Stereo sounds"), &gd_sound_stereo, true, NULL, _("If you enable stereo sounds, you will hear the direction of sounds in the caves.") },
        { TypeBoolean, _("16-bit mixing"), &gd_sound_16bit_mixing, true, NULL, _("Use 16-bit mixing of sounds. Try changing this setting if sound is clicky.") },
        { TypeBoolean, _("44kHz mixing"), &gd_sound_44khz_mixing, true, NULL, _("Use 44kHz mixing of sounds. Try changing this setting if sound is clicky.") },
    #endif
    
        /* end */
        { TypeBoolean, NULL },
    };
    
    set_page_numbers(settings_static);
    
    return settings_static;
}



Setting *gd_get_keyboard_settings_array(GameInputHandler *gih) {
    static Setting settings_static[] = {
        { TypePage, _("Keyboard") },
        { TypeKey,     _("Key left"), &gih->get_key_variable(GameInputHandler::KeyLeft), false },
        { TypeKey,     _("Key right"), &gih->get_key_variable(GameInputHandler::KeyRight), false },
        { TypeKey,     _("Key up"), &gih->get_key_variable(GameInputHandler::KeyUp), false },
        { TypeKey,     _("Key down"), &gih->get_key_variable(GameInputHandler::KeyDown), false },
        { TypeKey,     _("Key snap"), &gih->get_key_variable(GameInputHandler::KeyFire1), false },
        { TypeKey,     _("Key snap (alt.)"), &gih->get_key_variable(GameInputHandler::KeyFire2), false },
        { TypeKey,     _("Key suicide"), &gih->get_key_variable(GameInputHandler::KeySuicide), false },
        { TypeKey,     _("Key fast forward"), &gih->get_key_variable(GameInputHandler::KeyFastForward), false },
        { TypeKey,     _("Key status bar"), &gih->get_key_variable(GameInputHandler::KeyStatusBar), false },
        { TypeKey,     _("Key restart level"), &gih->get_key_variable(GameInputHandler::KeyRestartLevel), false },
        /* end */
        { TypeBoolean, NULL },
    };
    
    set_page_numbers(settings_static);
    
    return settings_static;
}




/* gets boolean value from key file; returns def if not found or unreadable */
static bool keyfile_get_bool_with_default(GKeyFile *keyfile, const char *group, const char *key, bool def) {
    GError *error=NULL;
    gboolean result;
    
    result=g_key_file_get_boolean(keyfile, group, key, &error);
    if (!error)
        return result!=FALSE;
    gd_message(error->message);
    g_error_free(error);
    return def;
}

/* gets integer value from key file; returns def if not found or unreadable */
static int keyfile_get_integer_with_default(GKeyFile *keyfile, const char *group, const char *key, int def) {
    GError *error=NULL;
    int result=g_key_file_get_integer(keyfile, group, key, &error);
    if (!error)
        return result;
    gd_message(error->message);
    g_error_free(error);
    return def;
}

static std::string keyfile_get_string(GKeyFile *keyfile, const char *group, const char *key) {    
    if (!g_key_file_has_key(keyfile, group, key, NULL))
        return "";
    
    GError *error=NULL;
    char *result=g_key_file_get_string(keyfile, group, key, &error);
    if (result) {
        std::string ret(result);
        g_free(result);
        return ret;
    }
    gd_message(error->message);
    g_error_free(error);
    return "";
}


void gd_settings_init() {
    gd_username = g_get_real_name();
    settings_bools["no_invisible_outbox"] = &gd_no_invisible_outbox;
    settings_bools["all_caves_selectable"] = &gd_all_caves_selectable;
    settings_bools["import_as_all_caves_selectable"] = &gd_import_as_all_caves_selectable;
    settings_bools["use_bdcff_highscore"] = &gd_use_bdcff_highscore;
    settings_bools["fine_scroll"] = &gd_fine_scroll;
    settings_bools["particle_effects"] = &gd_particle_effects;
    settings_bools["show_story"] = &gd_show_story;
    settings_bools["show_name_of_game"] = &gd_show_name_of_game;
    settings_integers["pal_emu_scanline_shade"] = &gd_pal_emu_scanline_shade;
    settings_integers["status_bar_colors"] = &gd_status_bar_colors;
    settings_integers["c64_palette"] = &gd_c64_palette;
    settings_integers["c64dtv_palette"] = &gd_c64dtv_palette;
    settings_integers["atari_palette"] = &gd_atari_palette;
    settings_integers["preferred_palette"] = &gd_preferred_palette;
    settings_strings["username"] = &gd_username;
    settings_strings["theme"] = &gd_theme;

    settings_bools["game_view"] = &gd_game_view;
    settings_bools["colored_objects"] = &gd_colored_objects;
    settings_bools["show_object_list"] = &gd_show_object_list;
    settings_bools["show_test_label"] = &gd_show_test_label;
    settings_bools["show_preview"] = &gd_show_preview;
    settings_bools["pal_emulation_game"] = &gd_pal_emulation_game;
    settings_bools["pal_emulation_editor"] = &gd_pal_emulation_editor;
    settings_bools["fast_uncover_in_test"] = &gd_fast_uncover_in_test;
    settings_integers["editor_window_width"] = &gd_editor_window_width;
    settings_integers["editor_window_height"] = &gd_editor_window_height;
    settings_integers["language"] = &gd_language;
    settings_integers["cell_scale_game"] = &gd_cell_scale_game;
    settings_integers["cell_scale_editor"] = &gd_cell_scale_editor;
    settings_bools["fullscreen"] = &gd_fullscreen;

    #ifdef HAVE_GTK
    settings_integers["gtk_key_left"] = &gd_gtk_key_left;
    settings_integers["gtk_key_right"] = &gd_gtk_key_right;
    settings_integers["gtk_key_up"] = &gd_gtk_key_up;
    settings_integers["gtk_key_down"] = &gd_gtk_key_down;
    settings_integers["gtk_key_fire_1"] = &gd_gtk_key_fire_1;
    settings_integers["gtk_key_fire_2"] = &gd_gtk_key_fire_2;
    settings_integers["gtk_key_suicide"] = &gd_gtk_key_suicide;
    settings_integers["gtk_key_fast_forward"] = &gd_gtk_key_fast_forward;
    settings_integers["gtk_key_status_bar"] = &gd_gtk_key_status_bar;
    settings_integers["gtk_key_restart_level"] = &gd_gtk_key_restart_level;
    #endif

    #ifdef HAVE_SDL    /* only if having sdl */
    settings_integers["sdl_key_left"] = &gd_sdl_key_left;
    settings_integers["sdl_key_right"] = &gd_sdl_key_right;
    settings_integers["sdl_key_up"] = &gd_sdl_key_up;
    settings_integers["sdl_key_down"] = &gd_sdl_key_down;
    settings_integers["sdl_key_fire_1"] = &gd_sdl_key_fire_1;
    settings_integers["sdl_key_fire_2"] = &gd_sdl_key_fire_2;
    settings_integers["sdl_key_suicide"] = &gd_sdl_key_suicide;
    settings_integers["sdl_key_fast_forward"] = &gd_sdl_key_fast_forward;
    settings_integers["sdl_key_status_bar"] = &gd_sdl_key_status_bar;
    settings_integers["sdl_key_restart_level"] = &gd_sdl_key_restart_level;
    #endif    /* use_sdl */

    #ifdef HAVE_SDL
    settings_bools["sound"] = &gd_sound_enabled;
    settings_bools["sound_16bit_mixing"] = &gd_sound_16bit_mixing;
    settings_bools["sound_44khz_mixing"] = &gd_sound_44khz_mixing;
    settings_bools["sound_stereo"] = &gd_sound_stereo;
    settings_bools["classic_sound"] = &gd_classic_sound;
    settings_integers["sound_chunks_volume_percent"] = &gd_sound_chunks_volume_percent;
    settings_integers["sound_music_volume_percent"] = &gd_sound_music_volume_percent;
    #endif
}


static void add_dirs(std::vector<std::string>& dirs, const char *specific) {
    // user's own config
    dirs.push_back(gd_tostring_free(g_build_path(G_DIR_SEPARATOR_S, gd_user_config_dir, specific, NULL)));
    dirs.push_back(gd_tostring_free(g_build_path(G_DIR_SEPARATOR_S, gd_user_config_dir, NULL)));
    // system-wide gdash config
    dirs.push_back(gd_tostring_free(g_build_path(G_DIR_SEPARATOR_S, gd_system_data_dir, specific, NULL)));
    dirs.push_back(gd_tostring_free(g_build_path(G_DIR_SEPARATOR_S, gd_system_data_dir, NULL)));
    // for testing: actual directory
    dirs.push_back(gd_tostring_free(g_build_path(G_DIR_SEPARATOR_S, ".", specific, NULL)));
    dirs.push_back(gd_tostring_free(g_build_path(G_DIR_SEPARATOR_S, ".", NULL)));
}

/* sets up directiories and loads translations */
void gd_settings_init_dirs() {
#ifdef G_OS_WIN32
    /* on win32, use the glib function. */
    gd_system_data_dir=g_win32_get_package_installation_directory (NULL, NULL);
#else
    /* on linux, this is a defined, built-in string, $perfix/share/locale */
    gd_system_data_dir=PKGDATADIR;
#endif
    gd_system_caves_dir=g_build_path(G_DIR_SEPARATOR_S, gd_system_data_dir, "caves", NULL);
    gd_system_music_dir=g_build_path(G_DIR_SEPARATOR_S, gd_system_data_dir, "music", NULL);
    gd_user_config_dir=g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), PACKAGE, NULL);
    
    add_dirs(gd_sound_dirs, "sound");
    add_dirs(gd_themes_dirs, "themes");
    add_dirs(gd_fonts_dirs, "fonts");
}

/* set locale from the gdash setting variable. */
/* only bother setting the locale cleverly when we are in the gtk version. */
/* for sdl version, not really matters. */
/* if no gtk, just set the system default locale. */
void gd_settings_set_locale() {
    if (gd_language<0 || gd_language>=int(G_N_ELEMENTS(language_locale)))
        gd_language=0;    /* switch to default, if out of bounds. */

    /* on windows, we put the LANGUAGE variable into the environment. that seems to be the only
    thing gtk+ acts upon. we also set the locale below. */
#ifdef G_OS_WIN32
    g_assert(G_N_ELEMENTS(language_locale)==G_N_ELEMENTS(languages_for_env));
    if (languages_for_env[gd_language]) {
        g_setenv("LANGUAGE", languages_for_env[gd_language], TRUE);
    }
#endif
    
    /* try to set the locale. */
    int i=0;
    char *result=NULL;
    while (result==NULL && language_locale[gd_language][i]!=NULL) {
        result = setlocale(LC_ALL, language_locale[gd_language][i]);
        i++;
    }
    if (result == NULL) {
        /* failed to set */
        gd_message(CPrintf("Failed to set language to '%s'. Switching to system default locale.") % gd_languages_names[gd_language]);
        setlocale(LC_ALL, "");
    }
}

/* sets up directiories and loads translations */
/* also instructs gettext to give all strings as utf8, as gtk uses that */
void gd_settings_init_translation() {
    /* different directories storing the translation files on unix and win32. */
    /* gdash (and gtk) always uses utf8, so convert translated strings to utf8 if needed. */
#ifdef G_OS_WIN32
    bindtextdomain("gtk20-properties", gd_system_data_dir);
    bind_textdomain_codeset("gtk20-properties", "UTF-8");
    bindtextdomain("gtk20", gd_system_data_dir);
    bind_textdomain_codeset("gtk20", "UTF-8");
    bindtextdomain("glib20", gd_system_data_dir);
    bind_textdomain_codeset("glib20", "UTF-8");
    bindtextdomain(PACKAGE, gd_system_data_dir);        /* gdash */
    bind_textdomain_codeset(PACKAGE, "UTF-8");
#else
    bindtextdomain(PACKAGE, LOCALEDIR);                 /* gdash */
    bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif
    textdomain(PACKAGE);    /* set default textdomain to gdash */
}


/* load settings from .config/gdash/gdash.ini */
void gd_load_settings() {
    GError *error=NULL;
    gchar *data;
    gsize length;
    
    char *filename=g_build_path(G_DIR_SEPARATOR_S, gd_user_config_dir, SETTINGS_INI_FILE, NULL);    
    if (!g_file_get_contents(filename, &data, &length, &error)) {
        /* no ini file found */
        gd_message(CPrintf("%s: %s") % filename % error->message);
        g_error_free(error);
        return;
    }
    g_free(filename);

    /* if zero length file, also return */
    if (length==0)
        return;

    GKeyFile *ini=g_key_file_new();
    gboolean success=g_key_file_load_from_data(ini, data, length, G_KEY_FILE_NONE, &error);
    g_free(data);
    if (!success) {
        gd_message(CPrintf("INI file contents error: %s") % error->message);
        g_error_free(error);
        return;
    }

    /* load the settings */
    for (std::map<char const *, int *>::const_iterator it = settings_integers.begin(); it != settings_integers.end(); ++it) {
        char const *key = it->first;
        int &var = *it->second;
        var = keyfile_get_integer_with_default(ini, SETTINGS_GDASH_GROUP, key, var);
    }
    for (std::map<char const *, bool *>::const_iterator it = settings_bools.begin(); it != settings_bools.end(); ++it) {
        char const *key = it->first;
        bool &var = *it->second;
        var = keyfile_get_bool_with_default(ini, SETTINGS_GDASH_GROUP, key, var);
    }
    for (std::map<char const *, std::string *>::const_iterator it = settings_strings.begin(); it != settings_strings.end(); ++it) {
        char const *key = it->first;
        std::string &var = *it->second;
        var = keyfile_get_string(ini, SETTINGS_GDASH_GROUP, key);
    }

    /* check settings */
    if (gd_cell_scale_game<0 || gd_cell_scale_game>=GD_SCALING_MAX)
        gd_cell_scale_game=GD_SCALING_ORIGINAL;
    if (gd_cell_scale_editor<0 || gd_cell_scale_editor>=GD_SCALING_MAX)
        gd_cell_scale_editor=GD_SCALING_ORIGINAL;
    if (gd_preferred_palette<0 || gd_preferred_palette>=int(GdColor::Invalid))
        gd_preferred_palette=GdColor::TypeRGB;
    if (gd_status_bar_colors<0 || gd_status_bar_colors > int(GD_STATUS_BAR_MAX))
        gd_status_bar_colors=GD_STATUS_BAR_ORIGINAL;

    g_key_file_free(ini);
}


/* save settings to .config/gdash.ini */
void gd_save_settings() {    
    GKeyFile *ini=g_key_file_new();

    /* save them */
    for (std::map<char const *, int *>::const_iterator it = settings_integers.begin(); it != settings_integers.end(); ++it) {
        char const *key = it->first;
        int &var = *it->second;
        g_key_file_set_integer(ini, SETTINGS_GDASH_GROUP, key, var);
    }
    for (std::map<char const *, bool *>::const_iterator it = settings_bools.begin(); it != settings_bools.end(); ++it) {
        char const *key = it->first;
        bool &var = *it->second;
        g_key_file_set_boolean(ini, SETTINGS_GDASH_GROUP, key, (gboolean) var);
    }
    for (std::map<char const *, std::string *>::const_iterator it = settings_strings.begin(); it != settings_strings.end(); ++it) {
        char const *key = it->first;
        std::string &var = *it->second;
        g_key_file_set_string(ini, SETTINGS_GDASH_GROUP, key, var.c_str());
    }
    
    GError *error=NULL;
    /* convert to string and free */
    gchar *data=g_key_file_to_data(ini, NULL, &error);
    g_key_file_free(ini);
    if (error) {
        /* this is highly unlikely - why would g_key_file_to_data report error? docs do not mention. */
        gd_warning(CPrintf("Unable to save settings: %s") % error->message);
        g_error_free(error);
        g_free(data);
        return;
    }
    
    char *filename=g_build_path(G_DIR_SEPARATOR_S, gd_user_config_dir, SETTINGS_INI_FILE, NULL);
    g_mkdir_with_parents(gd_user_config_dir, 0700);
    g_file_set_contents(filename, data, -1, &error);
    g_free(filename);
    if (error) {
        /* error saving the file */
        gd_warning(CPrintf("Unable to save settings: %s") % error->message);
        g_error_free(error);
        g_free(data);
        return;
    }
    g_free(data);
}

GOptionContext *gd_option_context_new() {
    GOptionEntry const entries[]={
        {"license", 'L', 0, G_OPTION_ARG_NONE, &gd_param_license, N_("Show license and quit")},
        {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &gd_param_cavenames, N_("Cave names")},
        {NULL}
    };
    GOptionContext *context=g_option_context_new(_("[FILE NAME]"));
    g_option_context_set_help_enabled(context, TRUE);
    g_option_context_add_main_entries(context, entries, PACKAGE);    /* gdash parameters */
    
    return context;
}
