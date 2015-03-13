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
#ifndef _GD_GTKPIXBUFFACTORY
#define _GD_GTKPIXBUFFACTORY

#include "config.h"

#include "gfx/pixbuffactory.hpp"
#include "gtk/gtkpixbuf.hpp"

class GTKPixbufFactory: public PixbufFactory {
public:
    GTKPixbufFactory(GdScalingType scaling_type_=GD_SCALING_ORIGINAL, bool pal_emulation_=false);
    virtual GTKPixbuf *create(int w, int h) const;
    virtual GTKPixbuf *create_from_inline(int length, unsigned char const *data) const;
    virtual GTKPixbuf *create_from_file(const char *filename) const;
    virtual GTKPixbuf *create_rotated(const Pixbuf &src, Rotation r) const;
    virtual GTKPixbuf *create_composite_color(const Pixbuf &src, const GdColor& c, unsigned char alpha) const;
    virtual GTKPixbuf *create_subpixbuf(Pixbuf &src, int x, int y, int w, int h) const;
    virtual Pixmap *create_pixmap_from_pixbuf(const Pixbuf& pb, bool format_alpha) const;
};

#endif

