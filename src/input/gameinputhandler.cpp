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

#include "input/gameinputhandler.hpp"
#include "input/joystick.hpp"

GameInputHandler::GameInputHandler() {
    clear_all_keypresses();
}


void GameInputHandler::clear_all_keypresses() {
    up_k = false;
    down_k = false;
    left_k = false;
    right_k = false;
    fire1_k = false;
    fire2_k = false;
    suicide = false;
    fast_forward = false;
    alternate_status = false;
    restart_level = false;
}


void GameInputHandler::keyrelease(int gfxlib_keycode) {
    for (unsigned i = 0; i != KeysNum; ++i) {
        KeyAssignment const &key = get_key(Keys(i));
        if (gfxlib_keycode == key.gfxlib_keycode) {
            this->*key.ptr = false;
        }
    }
}


void GameInputHandler::keypress(int gfxlib_keycode) {
    for (unsigned i = 0; i != KeysNum; ++i) {
        KeyAssignment const &key = get_key(Keys(i));
        if (gfxlib_keycode == key.gfxlib_keycode) {
            this->*key.ptr = true;
        }
    }
}


char const *GameInputHandler::get_key_name(Keys keyindex) {
    return get_key_name_from_keycode(get_key(keyindex).gfxlib_keycode);
}


int &GameInputHandler::get_key_variable(Keys keyindex) {
    return get_key(keyindex).gfxlib_keycode;
}


bool GameInputHandler::up() {
    return up_k || Joystick::up();
}


bool GameInputHandler::down() {
    return down_k || Joystick::down();
}


bool GameInputHandler::left() {
    return left_k || Joystick::left();
}


bool GameInputHandler::right() {
    return right_k || Joystick::right();
}


bool GameInputHandler::fire1() {
    return fire1_k || Joystick::fire1();
}


bool GameInputHandler::fire2() {
    return fire2_k || Joystick::fire2();
}
