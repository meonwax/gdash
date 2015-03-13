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

#include <glib/gi18n.h>

#include "framework/askyesnoactivity.hpp"
#include "framework/commands.hpp"
#include "gfx/fontmanager.hpp"
#include "gfx/screen.hpp"
#include "cave/helper/colors.hpp"
#include "misc/printf.hpp"


// TRANSLATORS: This string should be translated to the first letter of "yes" in your
// language, uppercase. For example, the English "yes" -> the strings is "Y".
// It should rather not contain an accented character, as the user might not be able
// to type it if he does not have the correct keyboard layout.
static char const *yesletter = N_("Y");
gunichar yeschar = 0;
// TRANSLATORS: This string should be translated to the first letter of "no" in your
// language, uppercase. For example, the English "no" -> the strings is "N".
// It might contain an accented character, but make sure the user can easily type it!
// It should rather not contain an accented character, as the user might not be able
// to type it if he does not have the correct keyboard layout.
static char const *noletter = N_("N");
gunichar nochar = 0;

AskYesNoActivity::AskYesNoActivity(App *app, char const *question, const char *yes_answer, char const *no_answer, SmartPtr<Command> command_when_yes, SmartPtr<Command> command_when_no)
    :
    Activity(app),
    question(question),
    yes_answer(yes_answer), no_answer(no_answer),
    command_when_yes(command_when_yes), command_when_no(command_when_no) {
    // These are global, but no problem if updated every time
    yeschar = g_utf8_get_char(_(yesletter));
    nochar = g_utf8_get_char(_(noletter));
}


void AskYesNoActivity::redraw_event() {
    int height=6*app->font_manager->get_line_height();
    int y1=(app->screen->get_height()-height)/2;    /* middle of the screen */
    int cx=2*app->font_manager->get_font_width_narrow(), cy=y1, cw=app->screen->get_width()-2*cx, ch=height;

    app->draw_window(cx, cy, cw, ch);
    app->screen->set_clip_rect(cx, cy, cw, ch);
    app->set_color(GD_GDASH_WHITE);
    app->blittext_n(-1, y1+app->font_manager->get_line_height(), question.c_str());
    app->blittext_n(-1, y1+3*app->font_manager->get_line_height(), CPrintf("%s: %s, %s: %s") % _(noletter) % no_answer.c_str() % _(yesletter) % yes_answer.c_str());
    app->screen->remove_clip_rect();

    app->screen->flip();
}


void AskYesNoActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    if (g_unichar_toupper(keycode) == g_unichar_toupper(yeschar)) {
        app->enqueue_command(new PopActivityCommand(app));
        app->enqueue_command(command_when_yes);
    }
    if (g_unichar_toupper(keycode) == g_unichar_toupper(nochar)) {
        app->enqueue_command(new PopActivityCommand(app));
        app->enqueue_command(command_when_no);
    }
}
