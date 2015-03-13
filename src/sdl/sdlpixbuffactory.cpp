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

#include <stdexcept>
#include <SDL.h>
#include <SDL_image.h>
#include <glib.h>

#include "sdl/sdlpixbuffactory.hpp"
#include "sdl/sdlpixmap.hpp"
#include "sdl/sdlscreen.hpp"
#include "cave/helper/colors.hpp"

SDLPixbufFactory::SDLPixbufFactory(GdScalingType scaling_type_, bool pal_emulation_)
:
    PixbufFactory(scaling_type_, pal_emulation_)
{
}

SDLPixbuf *SDLPixbufFactory::create_from_file(const char *filename) const
{
    return new SDLPixbuf(filename);
}

SDLPixbuf *SDLPixbufFactory::create_from_inline(int length, unsigned char const *data) const
{
    return new SDLPixbuf(length, data);
}

SDLPixbuf *SDLPixbufFactory::create(int w, int h) const
{
    return new SDLPixbuf(w, h);
}

Pixmap *SDLPixbufFactory::create_pixmap_from_pixbuf(Pixbuf const& pb, bool format_alpha) const
{
    SDLPixbuf *scaled = static_cast<SDLPixbuf*> (create_scaled(pb));
    SDL_Surface *surface;
    if (format_alpha)
        surface=SDL_DisplayFormatAlpha(scaled->surface);
    else
        surface=SDL_DisplayFormat(scaled->surface);
    return new SDLPixmap(surface);
}

SDLPixbuf *SDLPixbufFactory::create_composite_color(Pixbuf const &src, const GdColor& c, unsigned char a) const
{
    SDLPixbuf const &srcsdl = static_cast<SDLPixbuf const &>(src);
    SDL_Surface *rect=SDL_CreateRGBSurface(0, srcsdl.surface->w, srcsdl.surface->h, 32, srcsdl.rmask, srcsdl.gmask, srcsdl.bmask, 0); /* no amask, as we set overall alpha! */
    if (!rect)
        throw std::runtime_error(std::string("could not create surface: ")+SDL_GetError());
    SDL_FillRect(rect, NULL, SDL_MapRGB(rect->format, c.get_r(), c.get_g(), c.get_b()));
    SDL_SetAlpha(rect, SDL_SRCALPHA, a);    /* 50% alpha; nice choice. also sdl is rendering faster for the special value alpha=128 */    

    SDL_Surface *ret=SDL_CreateRGBSurface(0, srcsdl.surface->w, srcsdl.surface->h, 32, srcsdl.rmask, srcsdl.gmask, srcsdl.bmask, srcsdl.amask);
    if (!ret)
        throw std::runtime_error(std::string("could not create surface: ")+SDL_GetError());
    SDL_SetAlpha(srcsdl.surface, 0, 255);
    SDL_BlitSurface(srcsdl.surface, NULL, ret, NULL);
    SDL_BlitSurface(rect, NULL, ret, NULL);
    SDL_FreeSurface(rect);
    return new SDLPixbuf(ret);
}

SDLPixbuf *SDLPixbufFactory::create_subpixbuf(Pixbuf &src, int x, int y, int w, int h) const
{
    g_assert(x>=0 && y>=0 && x+w<=src.get_width() && y+h<=src.get_height());
    SDL_Surface *sub=SDL_CreateRGBSurfaceFrom(&src(x, y), w, h, 32, src.get_pitch(), src.rmask, src.gmask, src.bmask, src.amask);
    return new SDLPixbuf(sub);
}
