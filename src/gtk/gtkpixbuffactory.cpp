/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <cstdlib>
#include <stdexcept>

#include "gtk/gtkpixbuffactory.hpp"
#include "gtk/gtkpixbuf.hpp"
#include "cave/colors.hpp"

Pixbuf *GTKPixbufFactory::create_composite_color(const Pixbuf &src, const GdColor &c, unsigned char alpha) const {
    GTKPixbuf const &srcgtk = static_cast<GTKPixbuf const &>(src);
    guint32 color = (c.get_r() << 16) | (c.get_g() << 8) | (c.get_b() << 0);
    GdkPixbuf *pb = gdk_pixbuf_composite_color_simple(srcgtk.get_gdk_pixbuf(), src.get_width(), src.get_height(), GDK_INTERP_NEAREST, 255 - alpha, 1, color, color);
    return new GTKPixbuf(pb);
}

Pixbuf *GTKPixbufFactory::create_subpixbuf(Pixbuf &src, int x, int y, int w, int h) const {
    GTKPixbuf &srcgtk = static_cast<GTKPixbuf &>(src);
    GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf(srcgtk.get_gdk_pixbuf(), x, y, w, h);
    g_assert(sub != NULL);
    // gdk references pixbuf automatically, so it won't be cleared until the new GTKPixbuf is deleted, too
    return new GTKPixbuf(sub);
}

Pixbuf *GTKPixbufFactory::create_rotated(const Pixbuf &src, Rotation r) const {
    GdkPixbuf const *srcgtk = static_cast<GTKPixbuf const &>(src).get_gdk_pixbuf();
    GdkPixbuf *pb = NULL;
    switch (r) {
        case None:
            pb = gdk_pixbuf_rotate_simple(srcgtk, GDK_PIXBUF_ROTATE_NONE);
            break;
        case CounterClockWise:
            pb = gdk_pixbuf_rotate_simple(srcgtk, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
            break;
        case UpsideDown:
            pb = gdk_pixbuf_rotate_simple(srcgtk, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
            break;
        case ClockWise:
            pb = gdk_pixbuf_rotate_simple(srcgtk, GDK_PIXBUF_ROTATE_CLOCKWISE);
            break;
    }
    g_assert(pb != NULL);
    return new GTKPixbuf(pb);
}


Pixbuf *GTKPixbufFactory::create(int w, int h) const {
    return new GTKPixbuf(w, h);
}


Pixbuf *GTKPixbufFactory::create_from_inline(int length, unsigned char const *data) const {
    GInputStream *is = g_memory_input_stream_new_from_data(data, length, NULL);
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_stream(is, NULL, &error);
    g_object_unref(is);
    if (!pixbuf) {
        std::runtime_error exc(std::string("cannot load image ") + error->message);
        g_error_free(error);
        throw exc;
    }
    return new GTKPixbuf(pixbuf);
}


Pixbuf *GTKPixbufFactory::create_from_file(const char *filename) const {
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    if (!pixbuf) {
        std::runtime_error exc(std::string("cannot load image ") + error->message);
        g_error_free(error);
        throw exc;
    }
    return new GTKPixbuf(pixbuf);
}


