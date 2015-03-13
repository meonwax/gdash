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
#ifndef GTKSCREEN_HPP_INCLUDED
#define GTKSCREEN_HPP_INCLUDED

#include "config.h"

#include <gtk/gtk.h>

#include "gfx/screen.hpp"

class PixbufFactory;
class ParticleSet;

/** Implementation of the Pixmap interface, using GTK+ cairo functions. */
class GTKPixmap: public Pixmap {
private:
    GdkPixbuf *pixbuf;
    cairo_surface_t *surface;

public:
    friend class GTKScreen;
    friend class GTKPixbufFactory;

    GTKPixmap(GdkPixbuf *pixbuf, cairo_surface_t *surface): pixbuf(pixbuf), surface(surface) {
        g_object_ref(pixbuf);
    }

    virtual int get_width() const;
    virtual int get_height() const;

    /** Return the GdkPixbuf* associated with the object. */
    cairo_surface_t *get_cairo_surface() {
        return surface;
    }
    /** Return the GdkPixbuf* associated with the object. */
    cairo_surface_t const *get_cairo_surface() const {
        return surface;
    }

    virtual ~GTKPixmap();
};


/**
 * This is a Screen, which uses Cairo for drawing.
 * It can render to a window (if given a GtkDrawingArea), or it can
 * render to a software back buffer (if the given GtkDrawingArea is NULL).
 */
class GTKScreen: public Screen {
private:
    GtkWidget *drawing_area;
    cairo_t *cr;
    cairo_surface_t *back;

    GTKScreen(const GTKScreen &);           // not impl
    GTKScreen &operator=(const GTKScreen &); // not impl
    void free_buffer();
    virtual void configure_size();
    virtual void flip();

public:
    /** Creates a new GTKScreen, using the drawing area given as canvas.
     * @param drawing_area A GtkDrawingArea. If NULL pointer, the Screen renders to
     * a software buffer.
     */
    GTKScreen(PixbufFactory &pixbuf_factory, GtkWidget *drawing_area);
    ~GTKScreen();
    virtual void set_title(char const *title);

    void set_drawing_area(GtkWidget *drawing_area);
    cairo_t *get_cairo_t() { return cr; }

    virtual Pixmap *create_pixmap_from_pixbuf(const Pixbuf &pb, bool keep_alpha) const;

    virtual void fill_rect(int x, int y, int w, int h, const GdColor &c);
    virtual void blit(Pixmap const &src, int dx, int dy) const;
    virtual void draw_particle_set(int dx, int dy, ParticleSet const &ps);

    virtual void set_clip_rect(int x1, int y1, int w, int h);
    virtual void remove_clip_rect();
};
#endif

