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
 
#ifndef GD_SDLSCREEN
#define GD_SDLSCREEN

#include "config.h"

#include <SDL.h>

#include "gfx/screen.hpp"

class SDLAbstractScreen: public Screen {
protected:
    SDL_Surface *surface;

public:
    SDLAbstractScreen(): surface(NULL) {}
    virtual void fill_rect(int x, int y, int w, int h, const GdColor& c);
    virtual void blit_full(Pixmap const &src, int dx, int dy, int x, int y, int w, int h) const;
    virtual void set_clip_rect(int x1, int y1, int w, int h);
    virtual void remove_clip_rect();
    virtual void draw_particle_set(int dx, int dy, ParticleSet const &ps);
    
    SDL_Surface *get_surface() const { return surface; }
};


class SDLScreen: public SDLAbstractScreen {
private:
    SDLScreen(const SDLScreen &);       // not impl
    SDLScreen& operator=(const SDLScreen &);    // not impl
    int previous_configured_w, previous_configured_h;
    bool previous_configured_fullscreen;
    
public:
    SDLScreen();
    ~SDLScreen();
    virtual void configure_size();
    virtual void set_title(char const *title);
    virtual void flip();
    void reinit() { configure_size(); }

};

#endif
