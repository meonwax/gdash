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
#ifndef _GD_SDL_UI_H
#define _GD_SDL_UI_H

char *gd_select_file(const char *title, const char *start_dir, const char *glob);
void gd_settings_menu();
void gd_show_highscore(Cave *highlight_cave, int highlight_line);
void gd_help(const char **strings);

void gd_error_console();
void gd_show_error(GdErrorMessage *error);

char *gd_input_string(const char *title, const char *current);

void gd_about();
void gd_show_license();

void gd_title_line(const char *format, ...);
void gd_status_line(const char *text);

void gd_install_theme();

#endif

