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

#include <cmath>

#include "sdl/sdlabstractscreen.hpp"

#include "cave/helper/colors.hpp"
#include "sdl/sdlpixbuf.hpp"
#include "sdl/sdlpixmap.hpp"
#include "sdl/sdlscreen.hpp"
#include "sdl/sdlparticle.hpp"
#include "cave/particle.hpp"

void SDLAbstractScreen::fill_rect(int x, int y, int w, int h, const GdColor &c) {
    SDL_Rect dst;
    dst.x=x;
    dst.y=y;
    dst.w=w;
    dst.h=h;
    SDL_FillRect(surface, &dst, SDL_MapRGB(surface->format, c.get_r(), c.get_g(), c.get_b()));
}


void SDLAbstractScreen::blit_full(Pixmap const &src, int dx, int dy, int x, int y, int w, int h) const {
    SDL_Rect srcr, dstr;
    srcr.x=x;
    srcr.y=y;
    srcr.w=w;
    srcr.h=h;
    dstr.x=dx;
    dstr.y=dy;
    SDL_BlitSurface(static_cast<SDLPixmap const &>(src).surface, &srcr, surface, &dstr);
}


void SDLAbstractScreen::set_clip_rect(int x1, int y1, int w, int h) {
    /* on-screen clipping rectangle */
    SDL_Rect cliprect;
    cliprect.x=x1;
    cliprect.y=y1;
    cliprect.w=w;
    cliprect.h=h;
    SDL_SetClipRect(surface, &cliprect);
}


void SDLAbstractScreen::remove_clip_rect() {
    SDL_SetClipRect(surface, NULL);
}


void SDLAbstractScreen::draw_particle_set(int dx, int dy, ParticleSet const &ps) {
    Uint8 r = ps.color.get_r();
    Uint8 g = ps.color.get_g();
    Uint8 b = ps.color.get_b();
    Uint8 a = ps.life / 1000.0 * ps.opacity * 255;
    Uint32 color = r<<24 | g<<16 | b<<8 | a<<0;
    int size = ceil(ps.size);
    for (ParticleSet::const_iterator it = ps.begin(); it != ps.end(); ++it) {
        filledCircleColor(surface, dx + it->px, dy + it->py, size, color);
    }
}
