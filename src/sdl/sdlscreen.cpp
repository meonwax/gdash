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
#include <cmath>

#include "cave/helper/colors.hpp"
#include "sdl/sdlpixbuf.hpp"
#include "sdl/sdlpixmap.hpp"
#include "sdl/sdlscreen.hpp"
#include "sdl/gfxprim.hpp"
#include "cave/particle.hpp"
#include "settings.hpp"


// title image
#include "gdash_icon_32.cpp"


void SDLAbstractScreen::fill_rect(int x, int y, int w, int h, const GdColor& c) {
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
    SDL_BlitSurface(static_cast<SDLPixmap const&>(src).surface, &srcr, surface, &dstr);
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
    ParticleSet::const_iterator it;
    
    Uint8 r = ps.color.get_r();
    Uint8 g = ps.color.get_g();
    Uint8 b = ps.color.get_b();
    Uint8 a = ps.life / 1000.0 * ps.opacity * 255;
    Uint32 color = r<<24 | g<<16 | b<<8 | a<<0;
    int size = ceil(ps.size);
    for (it = ps.begin(); it != ps.end(); ++it) {
        filledCircleColor(surface, dx + it->px, dy + it->py, size, color);
    }
}




SDLScreen::SDLScreen() {
    previous_configured_w = -1;
    previous_configured_h = -1;
    previous_configured_fullscreen = false;
    surface = NULL;
}


SDLScreen::~SDLScreen() {
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


void SDLScreen::configure_size() {
    /* check if the previous size and fullscreen state matches the currently requested one.
     * if this is so, then do nothing. if the new one is different, reconfigure the screen.
     * on the first call of configure_size, this part of the code will surely be executed,
     * as the default values of previous* are -1, which can't be a screen size. */
    if (w != previous_configured_w || h != previous_configured_h || gd_fullscreen != previous_configured_fullscreen) {
        /* close window, if already exists, to create a new one */
        if (SDL_WasInit(SDL_INIT_VIDEO))
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
        /* init screen */
        SDL_InitSubSystem(SDL_INIT_VIDEO);
        /* for some reason, keyboard settings must be done here */
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
        SDL_EnableUNICODE(1);
        /* icon */
        SDLPixbuf icon(sizeof(gdash_icon_32), gdash_icon_32);
        SDL_WM_SetIcon(icon.surface, NULL);
        SDL_WM_SetCaption("GDash", NULL);

        /* create screen */
        Uint32 flags = SDL_ANYFORMAT | SDL_ASYNCBLIT;
        surface = SDL_SetVideoMode(w, h, 32, flags | (gd_fullscreen?SDL_FULLSCREEN:0));
        if (gd_fullscreen && !surface)
            surface=SDL_SetVideoMode(w, h, 32, flags);        // try the same, without fullscreen
        if (!surface)
            throw std::runtime_error("cannot initialize sdl video");
        /* do not show mouse cursor */
        SDL_ShowCursor(SDL_DISABLE);
        /* warp mouse pointer so cursor cannot be seen, if the above call did nothing for some reason */
        SDL_WarpMouse(w-1, h-1);
        
        previous_configured_w = w;
        previous_configured_h = h;
        previous_configured_fullscreen = gd_fullscreen;
    }
}


void SDLScreen::set_title(char const *title) {
    SDL_WM_SetCaption(title, NULL);
}


void SDLScreen::flip() {
    SDL_Flip(surface);
}


