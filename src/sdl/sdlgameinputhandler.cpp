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

#include <SDL.h>

#include "sdl/sdlgameinputhandler.hpp"
#include "settings.hpp"

/* This should be in the same order as the enum, so that can be used
 * as an index */
GameInputHandler::KeyAssignment SDLGameInputHandler::keys_array[] = {
    { KeyUp, gd_sdl_key_up, &GameInputHandler::up_k },
    { KeyDown, gd_sdl_key_down, &GameInputHandler::down_k },
    { KeyLeft, gd_sdl_key_left, &GameInputHandler::left_k },
    { KeyRight, gd_sdl_key_right, &GameInputHandler::right_k },
    { KeyFire1, gd_sdl_key_fire_1, &GameInputHandler::fire1_k },
    { KeyFire2, gd_sdl_key_fire_2, &GameInputHandler::fire2_k },
    { KeySuicide, gd_sdl_key_suicide, &GameInputHandler::suicide },
    { KeyFastForward, gd_sdl_key_fast_forward, &GameInputHandler::fast_forward },
    { KeyStatusBar, gd_sdl_key_status_bar, &GameInputHandler::alternate_status },
    { KeyRestartLevel, gd_sdl_key_restart_level, &GameInputHandler::restart_level },
};


char const *SDLGameInputHandler::get_key_name_from_keycode(int gfxlib_keycode) {
    return SDL_GetKeyName(SDLKey(gfxlib_keycode));
}


GameInputHandler::KeyAssignment const &SDLGameInputHandler::get_key(Keys keyindex) const {
    return keys_array[keyindex];
}
