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

#ifdef HAVE_SDL
#include <SDL.h>
#endif

#include "input/joystick.hpp"


#ifdef HAVE_SDL
static SDL_Joystick *joystick_1 = NULL;
#endif

void Joystick::init() {
#ifdef HAVE_SDL
    if (!SDL_WasInit(SDL_INIT_JOYSTICK))
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    if (SDL_NumJoysticks()>0)
        joystick_1=SDL_JoystickOpen(0);
#endif
}

bool Joystick::have_joystick() {
#ifdef HAVE_SDL
    return joystick_1 != NULL;
#else
    return false;
#endif
}

bool Joystick::up() {
#ifdef HAVE_SDL
    return joystick_1!=NULL && SDL_JoystickGetAxis(joystick_1, 1)<-3200;
#else
    return false;
#endif
}

bool Joystick::down() {
#ifdef HAVE_SDL
    return joystick_1!=NULL && SDL_JoystickGetAxis(joystick_1, 1)>3200;
#else
    return false;
#endif
}

bool Joystick::left() {
#ifdef HAVE_SDL
    return joystick_1!=NULL && SDL_JoystickGetAxis(joystick_1, 0)<-3200;
#else
    return false;
#endif
}

bool Joystick::right() {
#ifdef HAVE_SDL
    return joystick_1!=NULL && SDL_JoystickGetAxis(joystick_1, 0)>3200;
#else
    return false;
#endif
}

bool Joystick::fire1() {
#ifdef HAVE_SDL
    return joystick_1!=NULL && (SDL_JoystickGetButton(joystick_1, 0));
#else
    return false;
#endif
}

bool Joystick::fire2() {
#ifdef HAVE_SDL
    return joystick_1!=NULL && (SDL_JoystickGetButton(joystick_1, 1));
#else
    return false;
#endif
}
