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

#ifndef _GD_INPUTTEXTAPPLET
#define _GD_INPUTTEXTAPPLET

#include <string>
#include <glib.h>

#include "framework/app.hpp"
#include "framework/commands.hpp"

template <typename T> class Command1Param;

/** Ask the user to type one line of text, and when successful, call the Command parametrized with the text.
 *  See App::input_text_and_do_command() as well. */
class InputTextActivity: public Activity {
public:
    /** Ctor.
     * @param title_line The title of the window.
     * @param default_text The default value of the text box.
     * @param command_when_successful The command of one string parameter to be parametrized with the line
     *        typed and executed, if the user accepts the input. */
    InputTextActivity(App *app, char const *title_line, const char *default_text, SmartPtr<Command1Param<std::string> > command_when_successful);
    ~InputTextActivity();
    virtual void keypress_event(KeyCode keycode, int gfxlib_keycode);
    virtual void redraw_event();
    virtual void timer_event(int ms_elapsed);

private:
    std::string title;
    SmartPtr<Command1Param<std::string> > command_when_successful;
    GString *text;
    int ms;
    bool blink;
};

#endif
