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

#include <gtk/gtk.h>

#include "gtk/gtkpixmap.hpp"
#include "gtk/gtkscreen.hpp"
#include "cave/helper/colors.hpp"

void GTKScreen::fill_rect(int x, int y, int w, int h, const GdColor &c) {
    GdkColor col;
    col.red=c.get_r()*257;   // 257, as 255*257=65535
    col.green=c.get_g()*257;   // 257, as 255*257=65535
    col.blue=c.get_b()*257;   // 257, as 255*257=65535
    gdk_gc_set_rgb_fg_color(gc, &col);
    gdk_draw_rectangle(pixmap, gc, TRUE, x, y, w, h);
}

void GTKScreen::blit_full(Pixmap const &src, int dx, int dy, int x, int y, int w, int h) const {
    GTKPixmap const &srcgtk=static_cast<GTKPixmap const &>(src);

    if (srcgtk.mask) {
        gdk_gc_set_clip_mask(gc, srcgtk.mask);
        gdk_gc_set_clip_origin(gc, dx, dy);
    }
    gdk_draw_drawable(pixmap, gc, srcgtk.pixmap, x, y, dx, dy, w, h);
    if (srcgtk.mask) {
        gdk_gc_set_clip_mask(gc, NULL);
        gdk_gc_set_clip_origin(gc, 0, 0);
    }
}

void GTKScreen::set_clip_rect(int x1, int y1, int w, int h) {
    GdkRectangle rect;
    rect.x=x1;
    rect.y=y1;
    rect.width=w;
    rect.height=h;
    gdk_gc_set_clip_rectangle(gc, &rect);
}

void GTKScreen::remove_clip_rect() {
    gdk_gc_set_clip_rectangle(gc, NULL);
}


GTKScreen::GTKScreen(GtkWidget *drawing_area_)
  : Screen()
  , drawing_area(drawing_area_)
  , pixmap(NULL)
  , gc(NULL)
{
    gtk_widget_set_double_buffered(drawing_area, FALSE);
}


void GTKScreen::configure_size() {
    gtk_widget_set_size_request(drawing_area, w, h);
    if (pixmap)
        g_object_unref(pixmap);
    if (gc)
        g_object_unref(gc);
    pixmap = gdk_pixmap_new(GDK_DRAWABLE(drawing_area->window), w, h, -1);
    gc = gdk_gc_new(GDK_DRAWABLE(pixmap));
}


void GTKScreen::set_title(char const *title) {
    GtkWidget *toplevel = gtk_widget_get_toplevel(drawing_area);
    gtk_window_set_title(GTK_WINDOW(toplevel), title);
}


void GTKScreen::flip() {
    gdk_draw_drawable(drawing_area->window, gc, pixmap, 0, 0, 0, 0, w, h);
}
