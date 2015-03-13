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
#ifndef _GD_EDITOR_WIDGETS
#define _GD_EDITOR_WIDGETS

#include "config.h"

#include <gtk/gtk.h>

#include "cave/cavetypes.hpp"

class GdColor;

GtkWidget *gd_color_combo_new(const GdColor& color);
void gd_color_combo_set(GtkComboBox *combo, const GdColor& color);
const GdColor& gd_color_combo_get_color(GtkWidget *widget);

void gd_element_button_set_dialog_title(GtkWidget *button, const char *title);
void gd_element_button_set_dialog_sensitive(GtkWidget *button, gboolean sens);
GdElementEnum gd_element_button_get(GtkWidget *button);
GtkWidget *gd_element_button_new(GdElementEnum initial_element, gboolean stays_open, const char *special_title);
void gd_element_button_set(GtkWidget *button, GdElementEnum element);
void gd_element_button_update_pixbuf(GtkWidget *button);

GtkWidget *gd_direction_combo_new(GdDirectionEnum initial);
GdDirectionEnum gd_direction_combo_get_direction(GtkWidget *combo);

GtkWidget *gd_scheduling_combo_new(GdSchedulingEnum initial);
GdSchedulingEnum gd_scheduling_combo_get_scheduling(GtkWidget *combo);


#endif
