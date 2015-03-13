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
#ifndef _GD_GTKPIXBUF
#define _GD_GTKPIXBUF

#include "config.h"

#include <gtk/gtk.h>
#include "gfx/pixbuf.hpp"

/**
 * Implementation of the Pixbuf interface, using GDK-pixbuf
 */
class GTKPixbuf: public Pixbuf {
private:
    GdkPixbuf *pixbuf;

    GTKPixbuf(const GTKPixbuf&);                // copy ctor not implemented
    GTKPixbuf& operator=(const GTKPixbuf&);     // operator= not implemented
public:
    GTKPixbuf(int length, unsigned char const *data);
    GTKPixbuf(const char *filename);
    GTKPixbuf(int w, int h);
    GTKPixbuf(GdkPixbuf *pb);

    virtual int get_width() const;
    virtual int get_height() const;
    virtual void blit_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const;
    virtual void copy_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const;

    virtual bool has_alpha() const;
    virtual void fill_rect(int x, int y, int w, int h, const GdColor& c);

    virtual unsigned char *get_pixels() const;
    virtual int get_pitch() const;

    /**
     * Return the GdkPixbuf* associated with the object.
     */
    GdkPixbuf *get_gdk_pixbuf() { return pixbuf; }
    /**
     * Return the GdkPixbuf* associated with the object.
     */
    GdkPixbuf const *get_gdk_pixbuf() const { return pixbuf; }

    virtual ~GTKPixbuf();
};
#endif
