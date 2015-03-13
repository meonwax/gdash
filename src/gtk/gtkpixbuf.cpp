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
#include <cstdlib>
#include <stdexcept>

#include "cave/helper/colors.hpp"
#include "gtk/gtkpixbuf.hpp"

GTKPixbuf::GTKPixbuf(int length, unsigned char const *data) {
    GInputStream *is=g_memory_input_stream_new_from_data(data, length, NULL);
    GError *error=NULL;
    pixbuf=gdk_pixbuf_new_from_stream(is, NULL, &error);
    g_object_unref(is);
    if (!pixbuf) {
        std::runtime_error exc(std::string("cannot load image ")+error->message);
        g_error_free(error);
        throw exc;
    }
}

GTKPixbuf::GTKPixbuf(const char *filename) {
    GError *error=NULL;
    pixbuf=gdk_pixbuf_new_from_file(filename, &error);
    if (!pixbuf) {
        std::runtime_error exc(std::string("cannot load image ")+error->message);
        g_error_free(error);
        throw exc;
    }
}

GTKPixbuf::GTKPixbuf(int w, int h) {
    pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
}

GTKPixbuf::GTKPixbuf(GdkPixbuf *pixbuf_)
    :   pixbuf(pixbuf_) {
}

int GTKPixbuf::get_width() const {
    return gdk_pixbuf_get_width(pixbuf);
}

int GTKPixbuf::get_height() const {
    return gdk_pixbuf_get_height(pixbuf);
}

void GTKPixbuf::blit_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const {
    GdkPixbuf *destpb=static_cast<GTKPixbuf &>(dest).pixbuf;

    gdk_pixbuf_composite(pixbuf, destpb, dx, dy, w, h, dx-x, dy-y, 1, 1, GDK_INTERP_NEAREST, 255);
}

void GTKPixbuf::copy_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const {
    // gdk_pixbuf_copy_area does not like clipping
    if (dy+h>dest.get_height()) {
        h=dest.get_height()-dy;
        if (h<0)
            return;
    }
    if (dx+w>dest.get_width()) {
        w=dest.get_width()-dx;
        if (w<0)
            return;
    }
    GdkPixbuf *destpb=static_cast<GTKPixbuf &>(dest).pixbuf;
    gdk_pixbuf_copy_area(pixbuf, x, y, w, h, destpb, dx, dy);
}

bool GTKPixbuf::has_alpha() const {
    return gdk_pixbuf_get_has_alpha(pixbuf);
}

void GTKPixbuf::fill_rect(int x, int y, int w, int h, const GdColor &c) {
    GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf(pixbuf, x, y, w, h);
    gdk_pixbuf_fill(sub, (c.get_r()<<24) | (c.get_g()<<16) | (c.get_b()<<8) | (0xff << 0));
    g_object_unref(sub);
}

unsigned char *GTKPixbuf::get_pixels() const {
    return gdk_pixbuf_get_pixels(pixbuf);
}

int GTKPixbuf::get_pitch() const {
    return gdk_pixbuf_get_rowstride(pixbuf);
}

GTKPixbuf::~GTKPixbuf() {
    g_object_unref(pixbuf);
}
