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
#ifndef _GD_GFX_SCREEN
#define _GD_GFX_SCREEN

#include "config.h"

class GdColor;
class ParticleSet;
class PixmapStorage;

#include <vector>
#include "gfx/pixmap.hpp"
#include "settings.hpp"

/// @ingroup Graphics
/// A class which represents a screen (or a drawing area in a window) to draw pixmaps to.
/// A Screen object can know about one or mor PixmapStorage objects, which are notified
/// before a video mode change.
class Screen {
private:
    std::vector<PixmapStorage *> pixmap_storages;

protected:
    int w, h;
    bool fullscreen;

public:
    Screen(): w(0), h(0) {}
    virtual ~Screen() {}
    
    void set_size(int w, int h, bool fullscreen = gd_fullscreen);
    virtual void configure_size() = 0;
    void register_pixmap_storage(PixmapStorage *ps);
    void unregister_pixmap_storage(PixmapStorage *ps);
    int get_width() const { return w; }
    int get_height() const { return h; }

    virtual void set_title(char const *title) = 0;

    virtual void fill_rect(int x, int y, int w, int h, const GdColor &c)=0;
    void fill(const GdColor &c) {
        fill_rect(0, 0, get_width(), get_height(), c);
    }
    virtual void blit_full(Pixmap const &src, int dx, int dy, int x, int y, int w, int h) const = 0;
    void blit(Pixmap const &src, int dx, int dy) const {
        blit_full(src, dx, dy, 0, 0, src.get_width(), src.get_height());
    }

    virtual void set_clip_rect(int x1, int y1, int w, int h) = 0;
    virtual void remove_clip_rect() = 0;

    virtual void draw_particle_set(int dx, int dy, ParticleSet const &ps);
    
    /**
     * Returns if the screen is double buffered, which means that everything must be redrawn
     * before flipping. */
    virtual bool must_redraw_all_before_flip();

    /**
     * Copies the contents of the drawing back buffer of the screen. To be called
     * when drawing is finished.
     */
    virtual void flip();
};

#endif
