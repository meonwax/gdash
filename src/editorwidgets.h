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
#ifndef _GD_EDITOR_WIDGETS_H
#define _GD_EDITOR_WIDGETS_H

#include <gtk/gtkwidget.h>
#include <gtk/gtkcombobox.h>
#include "cave.h"

GtkWidget *gd_color_combo_new(const GdColor color);
GdColor gd_color_combo_get_color(GtkWidget *widget);
void gd_color_combo_set(GtkComboBox *combo, GdColor color);

void gd_element_button_set_dialog_title(GtkWidget *button, const char *title);
void gd_element_button_set_dialog_sensitive(GtkWidget *button, gboolean sens);
GdElement gd_element_button_get(GtkWidget *button);
GtkWidget *gd_element_button_new(GdElement initial_element, gboolean stays_open, const char *special_title);
void gd_element_button_set(GtkWidget *button, const GdElement element);
void gd_element_button_update_pixbuf(GtkWidget *button);

GtkWidget *gd_direction_combo_new(const GdDirection initial);
GdDirection gd_direction_combo_get_direction(GtkWidget *combo);

GtkWidget *gd_scheduling_combo_new(const GdScheduling initial);
GdScheduling gd_scheduling_combo_get_scheduling(GtkWidget *combo);


#endif
