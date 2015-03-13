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
#ifndef _GD_SDL_GFX
#define _GD_SDL_GFX

#include "config.h"

#include <SDL.h>

#include "gfx/screen.hpp"
#include "gfx/pixbuffactory.hpp"
#include "sdl/sdlpixbuf.hpp"

class SDLPixbufFactory: public PixbufFactory {
public:
    SDLPixbufFactory(GdScalingType scaling_type_=GD_SCALING_ORIGINAL, bool pal_emulation_=false);
    virtual SDLPixbuf *create(int w, int h) const;
    virtual SDLPixbuf *create_from_inline(int length, unsigned char const *data) const;
    virtual SDLPixbuf *create_from_file(const char *filename) const;
    virtual SDLPixbuf *create_composite_color(const Pixbuf &src, const GdColor &c, unsigned char alpha) const;
    virtual SDLPixbuf *create_subpixbuf(Pixbuf &src, int x, int y, int w, int h) const;
    virtual SDLPixbuf *create_rotated(const Pixbuf &src, Rotation r) const {
        throw "not implemented";
    }
    virtual Pixmap *create_pixmap_from_pixbuf(Pixbuf const &pb, bool format_alpha) const;
};


#endif
