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

#ifndef GD_SDLPIXBUF
#define GD_SDLPIXBUF

#include "config.h"

#include <SDL.h>

#include "gfx/pixbuf.hpp"


/// A class which represents a 32-bit RGBA image in memory.
class SDLPixbuf: public Pixbuf {
private:
    SDL_Surface* surface;                       ///< SDL pixbuf data
    
    void check_and_convert_to_rgba();

    SDLPixbuf(const SDLPixbuf&);                // copy ctor not implemented
    SDLPixbuf& operator=(const SDLPixbuf&);     // operator= not implemented
    SDLPixbuf(SDL_Surface *surface_);
    
    friend class SDLPixbufFactory;
    friend class SDLScreen;

public:
    SDLPixbuf(const char *filename);
    SDLPixbuf(int w, int h);
    SDLPixbuf(int length, unsigned char const *data);   // should be a friend

    ~SDLPixbuf();

    virtual int get_width() const;
    virtual int get_height() const;

    virtual void blit_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const;
    virtual void copy_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const;

    virtual bool has_alpha() const;
    virtual void fill_rect(int x, int y, int w, int h, const GdColor& c);

    virtual unsigned char *get_pixels() const;
    virtual int get_pitch() const;

    virtual void lock() const;
    virtual void unlock() const;
};

#endif
