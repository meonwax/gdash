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
#include <gtk/gtk.h>
#include <cstring>
#include "misc/printf.hpp"
#include "settings.hpp"


class Setting;
class PixbufFactory;

struct SettingsWindow {
    static void bool_toggle(GtkWidget *widget, gpointer data);
    static void int_change(GtkWidget *widget, gpointer data);
    static void stringv_change(GtkWidget *widget, gpointer data);
    static GtkWidget *combo_box_new_from_stringv(const char **str);
    static GtkWidget *combo_box_new_from_themelist(std::vector<std::string> const &strings);
    static void do_settings_dialog(Setting *settings, PixbufFactory &pf);

    static gboolean keysim_button_keypress_event(GtkWidget* widget, GdkEventKey* event, gpointer data);
    static void keysim_button_clicked_cb(GtkWidget *button, gpointer data);
    static GtkWidget *gd_keysim_button(const char *what_for, int *keyval);
};
