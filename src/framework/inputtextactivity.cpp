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

#include "framework/inputtextactivity.hpp"
#include "framework/inputtextactivity.hpp"
#include "gfx/fontmanager.hpp"
#include "gfx/screen.hpp"
#include "cave/helper/colors.hpp"
#include "misc/printf.hpp"

InputTextActivity::InputTextActivity(App *app, char const *title_line, const char *default_text, SmartPtr<Command1Param<std::string> > command_when_successful)
:
    Activity(app),
    title(title_line),
    command_when_successful(command_when_successful),
    ms(0), blink(false)
{
    text = g_string_new(default_text);
}


InputTextActivity::~InputTextActivity() {
    g_string_free(text, TRUE);
}


void InputTextActivity::redraw_event() {
    int height=6*app->font_manager->get_line_height();
    int y1=(app->screen->get_height()-height)/2;    /* middle of the screen */
    int cx=2*app->font_manager->get_font_width_narrow(), cy=y1, cw=app->screen->get_width()-2*cx, ch=height;

    app->draw_window(cx, cy, cw, ch);
    app->screen->set_clip_rect(cx, cy, cw, ch);
    int width=cw/app->font_manager->get_font_width_narrow();

    app->set_color(GD_GDASH_WHITE);
    app->blittext_n(-1, y1+app->font_manager->get_line_height(), title.c_str());
    int len = g_utf8_strlen(text->str, -1);
    int x;
    if (len<width-1)
        x=-1; /* if fits on screen (+1 for cursor), centered */
    else
        x=cx+cw-(len+1)*app->font_manager->get_font_width_narrow();    /* otherwise show end, +1 for cursor */
    app->blittext_n(x, y1+3*app->font_manager->get_line_height(), CPrintf("%s%c") % text->str % (blink?'_':' '));
    app->screen->remove_clip_rect();
    app->screen->flip();
}


void InputTextActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    switch (keycode) {
        case App::Enter:
            command_when_successful->set_param1(text->str);
            app->enqueue_command(new PopActivityCommand(app));
            app->enqueue_command(command_when_successful);
            break;
        case App::BackSpace:
            if (text->len!=0) {
                char *ptr=text->str+text->len;    /* string pointer + length: points to the terminating zero */
                ptr=g_utf8_prev_char(ptr);    /* step back one utf8 character */
                g_string_truncate(text, ptr-text->str);
                redraw_event();
            }
            break;
        case App::Escape:
            app->enqueue_command(new PopActivityCommand(app));
            break;
        default:
            if (keycode >= ' ') {
                g_string_append_unichar(text, keycode);
                redraw_event();
            }
            break;
    }
}


void InputTextActivity::timer_event(int ms_elapsed) {
    ms += ms_elapsed;
    if (ms > 400) {
        blink = !blink;
        ms -= 400;
        redraw_event();
    }
}
