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

#include <glib.h>

class GameInputHandler {
public:
    enum Keys {
        KeyUp,
        KeyDown,
        KeyLeft,
        KeyRight,
        KeyFire1,
        KeyFire2,
        KeySuicide,
        KeyFastForward,
        KeyStatusBar,
        KeyRestartLevel,
        KeysNum
    };
    struct KeyAssignment {
        Keys index;
        int &gfxlib_keycode;
        bool GameInputHandler::*ptr;
    };

    GameInputHandler();
    virtual ~GameInputHandler() {}
    void keyrelease(int gfxlib_keycode);
    void keypress(int gfxlib_keycode);

    char const *get_key_name(Keys keyindex);
    virtual char const *get_key_name_from_keycode(int gfxlib_keycode) = 0;
    int &get_key_variable(Keys keyindex);
    virtual KeyAssignment const &get_key(Keys keyindex) const = 0;
    void clear_all_keypresses();

    bool up();
    bool down();
    bool left();
    bool right();
    bool fire1();
    bool fire2();
    bool up_k, down_k, left_k, right_k, fire1_k, fire2_k, suicide, fast_forward, alternate_status, restart_level;

private:
};
