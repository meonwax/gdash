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

#include <SDL_image.h>
#include <stdexcept>

#include "cave/colors.hpp"
#include "sdl/sdlpixbuf.hpp"


int SDLPixbuf::get_width() const {
    return surface->w;
}


int SDLPixbuf::get_height() const {
    return surface->h;
}


SDLPixbuf::SDLPixbuf(int w, int h) {
    surface = SDL_CreateRGBSurface(SDL_SRCALPHA, w, h, 32, rmask, gmask, bmask, amask);
    if (!surface)
        throw std::runtime_error(std::string("could not create surface: ") + SDL_GetError());
}


SDLPixbuf::SDLPixbuf(SDL_Surface *surface_)
    : surface(surface_) {
    /* if the r/g/b/masks do not match our preset values, the image is not stored in R,G,B,[A] byte order
     * in memory. so convert it. */
    if (surface->format->BytesPerPixel != 4 || surface->format->Rmask != rmask || surface->format->Gmask != gmask || surface->format->Bmask != bmask) {
        SDL_Surface *newsurface = SDL_CreateRGBSurface(SDL_SRCALPHA, surface->w, surface->h, 32, rmask, gmask, bmask, surface->format->Amask != 0 ? amask : 0);
        SDL_SetAlpha(surface, 0, SDL_ALPHA_OPAQUE);
        SDL_BlitSurface(surface, NULL, newsurface, NULL);
        SDL_FreeSurface(surface);
        surface = newsurface;
    }
}


SDLPixbuf::~SDLPixbuf() {
    SDL_FreeSurface(surface);
}


void SDLPixbuf::fill_rect(int x, int y, int w, int h, const GdColor &c) {
    SDL_Rect dst;
    dst.x = x;
    dst.y = y;
    dst.w = w;
    dst.h = h;
    SDL_FillRect(surface, &dst, SDL_MapRGB(surface->format, c.get_r(), c.get_g(), c.get_b()));
}


void SDLPixbuf::blit_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const {
    SDL_Rect src, dst;
    src.x = x;
    src.y = y;
    src.w = w;
    src.h = h;
    dst.x = dx;
    dst.y = dy;
    SDL_SetAlpha(surface, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
    SDL_BlitSurface(surface, &src, static_cast<SDLPixbuf &>(dest).surface, &dst);
}


void SDLPixbuf::copy_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const {
    SDL_Rect src, dst;
    src.x = x;
    src.y = y;
    src.w = w;
    src.h = h;
    dst.x = dx;
    dst.y = dy;
    SDL_SetAlpha(surface, 0, SDL_ALPHA_OPAQUE);
    SDL_BlitSurface(surface, &src, static_cast<SDLPixbuf &>(dest).surface, &dst);
}


unsigned char *SDLPixbuf::get_pixels() const {
    return static_cast<unsigned char *>(surface->pixels);
}


int SDLPixbuf::get_pitch() const {
    return surface->pitch;
}
