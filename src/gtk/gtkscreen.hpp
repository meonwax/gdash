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
#ifndef _GD_GTK_GFX
#define _GD_GTK_GFX

#include "config.h"

#include <gtk/gtk.h>

#include "gfx/screen.hpp"

/**
 * This is a Screen, which uses a GtkDrawingArea for its canvas.
 */
class GTKScreen: public Screen {
private:
    GtkWidget *drawing_area;
    GdkPixmap *pixmap;
    GdkGC *gc;

    GTKScreen(const GTKScreen &);           // not impl
    GTKScreen &operator=(const GTKScreen &); // not impl

public:
    /** Creates a new GTKScreen, using the drawing area given as canvas.
     * @param drawing_area A GtkDrawingArea.
     */
    GTKScreen(GtkWidget *drawing_area_);
    virtual void configure_size();
    virtual void set_title(char const *title);

    virtual void fill_rect(int x, int y, int w, int h, const GdColor &c);

    virtual void blit_full(Pixmap const &src, int dx, int dy, int x, int y, int w, int h) const;

    virtual void set_clip_rect(int x1, int y1, int w, int h);
    virtual void remove_clip_rect();

    virtual void flip();
};
#endif

