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

#include <SDL_image.h>
#include <stdexcept>

#include "cave/helper/colors.hpp"
#include "sdl/sdlpixbuf.hpp"


int SDLPixbuf::get_width() const {
    return surface->w;
}


int SDLPixbuf::get_height() const {
    return surface->h;
}


void SDLPixbuf::check_and_convert_to_rgba() {
    /* if the r/g/b/masks do not match our preset values, the image is not stored in R,G,B,[A] byte order
     * in memory. so convert it. */
    if (surface->format->BytesPerPixel!=4 || surface->format->Rmask != rmask || surface->format->Gmask != gmask || surface->format->Bmask != bmask) {
        SDL_Surface *newsurface = SDL_CreateRGBSurface(SDL_SRCALPHA, surface->w, surface->h, 32, rmask, gmask, bmask, surface->format->Amask != 0 ? amask : 0);
        SDL_SetAlpha(surface, 0, SDL_ALPHA_OPAQUE);
        SDL_BlitSurface(surface, NULL, newsurface, NULL);
        SDL_FreeSurface(surface);
        surface = newsurface;
    }
}


SDLPixbuf::SDLPixbuf(int length, unsigned char const *data) {
    SDL_RWops *rwop=SDL_RWFromConstMem(data, length);
    surface=IMG_Load_RW(rwop, 1);    // 1 = automatically closes rwop
    if (!surface)       // if could not load
        throw std::runtime_error(std::string("cannot load image ")+IMG_GetError());
    check_and_convert_to_rgba();
}


SDLPixbuf::SDLPixbuf(const char *filename) {
    surface=IMG_Load(filename);
    if (!surface)       // if could not load
        throw std::runtime_error(std::string("cannot load image ")+IMG_GetError());
    check_and_convert_to_rgba();
}


SDLPixbuf::SDLPixbuf(int w, int h) {
    surface=SDL_CreateRGBSurface(SDL_SRCALPHA, w, h, 32, rmask, gmask, bmask, amask);
    if (!surface)
        throw std::runtime_error(std::string("could not create surface: ")+SDL_GetError());
}


SDLPixbuf::SDLPixbuf(SDL_Surface *surface_)
    :   surface(surface_) {
}


SDLPixbuf::~SDLPixbuf() {
    SDL_FreeSurface(surface);
}


void SDLPixbuf::fill_rect(int x, int y, int w, int h, const GdColor &c) {
    SDL_Rect dst;
    dst.x=x;
    dst.y=y;
    dst.w=w;
    dst.h=h;
    SDL_FillRect(surface, &dst, SDL_MapRGB(surface->format, c.get_r(), c.get_g(), c.get_b()));
}


bool SDLPixbuf::has_alpha() const {
    return surface->format->Amask!=0;
}


void SDLPixbuf::blit_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const {
    SDL_Rect src, dst;
    src.x=x;
    src.y=y;
    src.w=w;
    src.h=h;
    dst.x=dx;
    dst.y=dy;
    SDL_SetAlpha(surface, SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
    SDL_BlitSurface(surface, &src, static_cast<SDLPixbuf &>(dest).surface, &dst);
}


void SDLPixbuf::copy_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const {
    SDL_Rect src, dst;
    src.x=x;
    src.y=y;
    src.w=w;
    src.h=h;
    dst.x=dx;
    dst.y=dy;
    SDL_SetAlpha(surface, 0, SDL_ALPHA_OPAQUE);
    SDL_BlitSurface(surface, &src, static_cast<SDLPixbuf &>(dest).surface, &dst);
}


unsigned char *SDLPixbuf::get_pixels() const {
    return static_cast<unsigned char *>(surface->pixels);
}


int SDLPixbuf::get_pitch() const {
    return surface->pitch;
}


void SDLPixbuf::lock() const {
    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);
}


void SDLPixbuf::unlock() const {
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
}
