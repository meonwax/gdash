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

#ifndef _GD_ASKYESNO
#define _GD_ASKYESNO

#include <string>
#include <glib.h>

#include "framework/app.hpp"

class Command;


/** An activity which shows a question to the user, and asks yes or no.
 * Two commands are given to such an activity; one of which will be executed
 * if the answer was yes, the other if the answer was no.
 * See App::ask_yesorno_and_do_command(). */
class AskYesNoActivity: public Activity {
public:
    /** Ctor.
     * @param app The parent app.
     * @param question The text of the question to ask.
     *      Should not be too long, so it fits on the screen in the SDL version as well.
     * @param yes_answer The text of the answer treated as "yes".
     * @param no_answer The text of the answer treated as "no".
     * @param command_when_yes The command to be executed if the user said yes. May be NULL.
     * @param command_when_no The command to be executed if the user said no. May be NULL. */
    AskYesNoActivity(App *app, char const *question, const char *yes_answer, char const *no_answer, SmartPtr<Command> command_when_yes, SmartPtr<Command> command_when_no);
    virtual void keypress_event(KeyCode keycode, int gfxlib_keycode);
    virtual void redraw_event();

private:
    std::string question, yes_answer, no_answer;
    SmartPtr<Command> command_when_yes, command_when_no;
};

#endif
