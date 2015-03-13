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
#ifndef _GD_GTKUI
#define _GD_GTKUI

#include "config.h"

#include <gtk/gtk.h>
#include <exception>
#include <vector>
#include <string>

class CaveStored;
class CaveSet;
class Logger;
class Setting;
class PixbufFactory;

#define GD_ICON_CAVE_EDITOR "cave-editor"
#define GD_ICON_EDITOR_MOVE "editor-move"
#define GD_ICON_EDITOR_JOIN "editor-add-join"
#define GD_ICON_EDITOR_POINT "editor-add-point"
#define GD_ICON_EDITOR_FREEHAND "editor-add-freehand"
#define GD_ICON_EDITOR_LINE "editor-add-line"
#define GD_ICON_EDITOR_RECTANGLE "editor-add-rectangle"
#define GD_ICON_EDITOR_FILLRECT "editor-add-filled-rectangle"
#define GD_ICON_EDITOR_RASTER "editor-add-raster"
#define GD_ICON_EDITOR_FILL_BORDER "editor-add-fill-border"
#define GD_ICON_EDITOR_FILL_REPLACE "editor-add-fill-replace"
#define GD_ICON_EDITOR_MAZE "editor-add-maze"
#define GD_ICON_EDITOR_MAZE_UNI "editor-add-maze-uni"
#define GD_ICON_EDITOR_MAZE_BRAID "editor-add-maze-braid"
#define GD_ICON_SNAPSHOT "snapshot"
#define GD_ICON_RESTART_LEVEL "restart-level"
#define GD_ICON_RANDOM_FILL "random-fill"
#define GD_ICON_REMOVE_MAP "remove-map"
#define GD_ICON_FLATTEN_CAVE "flatten-cave"
#define GD_ICON_AWARD "icon-award"
#define GD_ICON_TO_TOP "icon-to-top"
#define GD_ICON_TO_BOTTOM "icon-to-botton"
#define GD_ICON_OBJECT_ON_ALL "icon-object-on-all"
#define GD_ICON_OBJECT_NOT_ON_ALL "icon-object-not-on-all"
#define GD_ICON_OBJECT_NOT_ON_CURRENT "icon-object-not-on-current"
#define GD_ICON_REPLAY "icon-replay"
#define GD_ICON_KEYBOARD "icon-keyboard"
#define GD_ICON_IMAGE "icon-image"

GdkPixbuf *gd_pixbuf_for_builtin_theme();
GdkPixbuf *gd_icon();
void gd_register_stock_icons();

GtkWindow *guess_active_toplevel();

void gd_show_errors(Logger &l, const char *title, bool always_show=false);

void gd_save_caveset_as(CaveSet &caveset);
void gd_save_caveset(CaveSet &caveset);
void gd_open_caveset(const char *directory, CaveSet &caveset);

bool gd_question_yesno(const char *primary, const char *secondary);
bool gd_discard_changes(CaveSet const &caveset);

void gd_warningmessage(const char *primary, const char *secondary);
void gd_errormessage(const char *primary, const char *secondary);
void gd_infomessage(const char *primary, const char *secondary);

GtkWidget *gd_label_new_leftaligned(const char *markup);
GtkWidget *gd_label_new_centered(const char *markup);
void gd_dialog_add_hint(GtkDialog *dialog, const char *hint);

char *gd_select_image_file(const char *title);

GtkWindow *guess_active_toplevel();

#endif

