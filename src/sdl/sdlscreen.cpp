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

#include <stdexcept>
#include <memory>
#include <SDL_image.h>

#include "sdl/sdlscreen.hpp"
#include "sdl/sdlpixbuf.hpp"
#include "gfx/pixbuffactory.hpp"

#include "misc/logger.hpp"
#include "settings.hpp"


SDLScreen::SDLScreen(PixbufFactory &pixbuf_factory)
    : SDLAbstractScreen(pixbuf_factory) {
    surface = NULL;
}


SDLScreen::~SDLScreen() {
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


void SDLScreen::configure_size() {
    /* close window, if already exists, to create a new one */
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    /* init screen */
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    /* for some reason, keyboard settings must be done here */
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    SDL_EnableUNICODE(1);
    /* icon */
    SDL_RWops *rwop = SDL_RWFromConstMem(Screen::gdash_icon_32_png, Screen::gdash_icon_32_size);
    SDL_Surface *icon = IMG_Load_RW(rwop, 1);  // 1 = automatically closes rwop
    SDL_WM_SetIcon(icon, NULL);
    SDL_FreeSurface(icon);
    set_title("GDash");

    /* create screen */
    Uint32 flags = SDL_ANYFORMAT;
    surface = SDL_SetVideoMode(w, h, 0, flags | (gd_fullscreen ? SDL_FULLSCREEN : 0));
    if (gd_fullscreen && !surface)
        surface = SDL_SetVideoMode(w, h, 0, flags);        // try the same, without fullscreen
    if (!surface)
        throw ScreenConfigureException("cannot initialize sdl video");
    /* do not show mouse cursor */
    SDL_ShowCursor(SDL_DISABLE);
    /* warp mouse pointer so cursor cannot be seen, if the above call did nothing for some reason */
    SDL_WarpMouse(w - 1, h - 1);
}


void SDLScreen::set_title(char const *title) {
    SDL_WM_SetCaption(title, NULL);
}


bool SDLScreen::must_redraw_all_before_flip() const {
    if (surface == NULL)
        return false;
    /* if we have double buffering, all stuff must be redrawn before flips. */
    /* unused currently, but could be used for directx */
    return (surface->flags & SDL_DOUBLEBUF) != 0;
}


void SDLScreen::flip() {
    SDL_Flip(surface);
}


Pixmap *SDLScreen::create_pixmap_from_pixbuf(Pixbuf const &pb, bool keep_alpha) const {
    SDLPixbuf const &sdlpb = static_cast<SDLPixbuf const &>(pb);
    return new SDLPixmap(keep_alpha ? SDL_DisplayFormatAlpha(sdlpb.get_surface()) : SDL_DisplayFormat(sdlpb.get_surface()));
}
