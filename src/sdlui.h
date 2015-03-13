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
#ifndef _GD_SDL_UI_H
#define _GD_SDL_UI_H

#include <glib.h>

/* for main menu */
typedef enum _gd_main_menu_selected {
    M_NONE,
    M_QUIT,    /* quit immediately */
    M_EXIT, /* normal quit */
    M_ABOUT,
    M_LICENSE,
    M_PLAY,
    M_SAVE,
    M_INFO,
    M_SAVE_AS_NEW,
    M_REPLAYS,
    M_OPTIONS,
    M_INSTALL_THEME,
    M_HIGHSCORE,
    M_LOAD,
    M_LOAD_FROM_INSTALLED,
    M_ERRORS,
    M_HELP,
} GdMainMenuSelected;


char *gd_last_folder;
char *gd_caveset_filename;
void gd_open_caveset(const char *directory);
void gd_caveset_file_operation_successful(const char *filename);


char *gd_select_file(const char *title, const char *start_dir, const char *glob, gboolean for_save);
void gd_settings_menu();
void gd_show_highscore(GdCave *highlight_cave, int highlight_line);
void gd_show_cave_info(GdCave *show_cave);
void gd_help(const char **strings);
void gd_message(const char *message);

void gd_title_line(const char *format, ...);
void gd_status_line(const char *text);
void gd_status_line_red(const char *text);

void gd_error_console();
void gd_show_error(GdErrorMessage *error);

gboolean gd_ask_yes_no(const char *question, const char *answer1, const char *answer2, gboolean *result);
char *gd_input_string(const char *title, const char *current);
int gd_select_key(const char *title);
gboolean gd_discard_changes();

void gd_about();
void gd_show_license();

void gd_replays_menu(void (*play_func) (GdCave *cave, GdReplay *replay), gboolean for_game);

void gd_install_theme();

#endif

