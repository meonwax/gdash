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

#ifndef _GD_SHOWTEXTAPPLET
#define _GD_SHOWTEXTAPPLET

#include "framework/app.hpp"

#include <vector>
#include <string>

class Command;


class ShowTextActivity: public Activity {
public:
    ShowTextActivity(App *app, char const *title_line, std::string const &text, SmartPtr<Command> command_after_exit = SmartPtr<Command>());
    ~ShowTextActivity();
    
    virtual void redraw_event();
    virtual void keypress_event(KeyCode keycode, int gfxlib_keycode);

private:
    SmartPtr<Command> command_after_exit;
    std::string title_line;
    /* for long text */
    std::vector<std::string> wrapped_text;
    int linesavailable, scroll_y, scroll_max_y;
};

#endif
