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

#include "sdl/sdlscreen.hpp"
#include "sdl/sdlpixbuf.hpp"

#include "misc/logger.hpp"
#include "settings.hpp"

// title image
#include "gdash_icon_32.cpp"



SDLScreen::SDLScreen() {
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
    SDLPixbuf icon(sizeof(gdash_icon_32), gdash_icon_32);
    SDL_WM_SetIcon(icon.get_surface(), NULL);
    set_title("GDash");

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
}


void SDLScreen::set_title(char const *title) {
    SDL_WM_SetCaption(title, NULL);
}


bool SDLScreen::must_redraw_all_before_flip() {
    return false;
}


void SDLScreen::flip() {
    SDL_Flip(surface);
}


