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

#include "gtk/gtkpixbuffactory.hpp"
#include "cave/helper/colors.hpp"
#include "gtk/gtkpixmap.hpp"

GTKPixbuf *GTKPixbufFactory::create_composite_color(const Pixbuf &src, const GdColor& c, unsigned char alpha) const
{
    GTKPixbuf const &srcgtk = static_cast<GTKPixbuf const &>(src);
    guint32 color = (c.get_r()<<16) | (c.get_g()<<8) | (c.get_b()<<0);
    GdkPixbuf *pb = gdk_pixbuf_composite_color_simple(srcgtk.get_gdk_pixbuf(), src.get_width(), src.get_height(), GDK_INTERP_NEAREST, 255-alpha, 1, color, color);
    return new GTKPixbuf(pb);
}

GTKPixbuf *GTKPixbufFactory::create_subpixbuf(Pixbuf &src, int x, int y, int w, int h) const
{
    GTKPixbuf &srcgtk = static_cast<GTKPixbuf &>(src);
    GdkPixbuf *sub = gdk_pixbuf_new_subpixbuf(srcgtk.get_gdk_pixbuf(), x, y, w, h);
    g_assert(sub!=NULL);
    // gdk references pixbuf automatically, so it won't be cleared until the new GTKPixbuf is deleted, too
    return new GTKPixbuf(sub);
}

Pixmap *GTKPixbufFactory::create_pixmap_from_pixbuf(const Pixbuf& pb, bool format_alpha) const
{
    GTKPixbuf *scaled = static_cast<GTKPixbuf *>(create_scaled(pb));
    GdkDrawable *pixmap;
    GdkBitmap *mask;
    
    if (format_alpha)
        gdk_pixbuf_render_pixmap_and_mask(scaled->get_gdk_pixbuf(), &pixmap, &mask, 128);     // 128 = alpha threshold
    else {
        gdk_pixbuf_render_pixmap_and_mask(scaled->get_gdk_pixbuf(), &pixmap, NULL, 128);      // 128 = alpha threshold
        mask=NULL;
    }
    delete scaled;
    
    return new GTKPixmap(pixmap, mask);
}

GTKPixbuf *GTKPixbufFactory::create_rotated(const Pixbuf &src, Rotation r) const
{
    GdkPixbuf const *srcgtk = static_cast<GTKPixbuf const&>(src).get_gdk_pixbuf();
    GdkPixbuf *pb=NULL;
    switch (r) {
        case None:
            pb=gdk_pixbuf_rotate_simple(srcgtk, GDK_PIXBUF_ROTATE_NONE);
            break;
        case CounterClockWise:
            pb=gdk_pixbuf_rotate_simple(srcgtk, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
            break;
        case UpsideDown:
            pb=gdk_pixbuf_rotate_simple(srcgtk, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
            break;
        case ClockWise:
            pb=gdk_pixbuf_rotate_simple(srcgtk, GDK_PIXBUF_ROTATE_CLOCKWISE);
            break;
    }
    g_assert(pb!=NULL);
    return new GTKPixbuf(pb);
}


GTKPixbufFactory::GTKPixbufFactory(GdScalingType scaling_type_, bool pal_emulation_)
:   PixbufFactory(scaling_type_, pal_emulation_)
{
}


GTKPixbuf *GTKPixbufFactory::create(int w, int h) const
{
    return new GTKPixbuf(w, h);
}


GTKPixbuf *GTKPixbufFactory::create_from_inline(int length, unsigned char const *data) const
{
    return new GTKPixbuf(length, data);
}


GTKPixbuf *GTKPixbufFactory::create_from_file(const char *filename) const
{
    return new GTKPixbuf(filename);
}
