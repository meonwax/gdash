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

#include <glib.h>
#include <glib/gi18n.h>
#include <cstring>

#include "framework/messageactivity.hpp"
#include "framework/commands.hpp"
#include "cave/helper/colors.hpp"
#include "gfx/screen.hpp"
#include "gfx/fontmanager.hpp"
#include "misc/util.hpp"
#include "misc/printf.hpp"


MessageActivity::MessageActivity(App *app, std::string const &primary, std::string const &secondary, SmartPtr<Command> command_after_exit)
:
    Activity(app),
    command_after_exit(command_after_exit)
{
    std::string text;
    if (secondary != "")
        text = SPrintf("%c%s\n\n%c%s") % GD_COLOR_INDEX_WHITE % primary % GD_COLOR_INDEX_GRAY3 % secondary;
    else 
        text = SPrintf("%c%s") % GD_COLOR_INDEX_WHITE % primary;
    wrapped_text = gd_wrap_text(text.c_str(), app->screen->get_width() / app->font_manager->get_font_width_narrow()-6);
}


void MessageActivity::redraw_event() {
    int height = (wrapped_text.size() + 2) * app->font_manager->get_line_height(); /* +2 empty lines */
    int y1=(app->screen->get_height()-height)/2;    /* middle of the screen */
    int cx=2*app->font_manager->get_font_width_narrow(), cy=y1, cw=app->screen->get_width()-2*cx, ch=height;

    app->draw_window(cx, cy, cw, ch);
    app->screen->set_clip_rect(cx, cy, cw, ch);
    
    app->set_color(GD_GDASH_WHITE);
    for (size_t i = 0; i < wrapped_text.size(); ++i)
        app->blittext_n(-1, y1+(i+1)*app->font_manager->get_line_height(), wrapped_text[i].c_str());
    
    app->screen->remove_clip_rect();

    app->screen->flip();
}


MessageActivity::~MessageActivity() {
    app->enqueue_command(command_after_exit);
}


void MessageActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    switch (keycode) {
        default:
            app->enqueue_command(new PopActivityCommand(app));
            break;
        case 0:
            /* unknown key or modifier - do nothing */
            break;
    }
}
