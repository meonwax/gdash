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

#include "framework/showtextactivity.hpp"
#include "framework/commands.hpp"
#include "cave/helper/colors.hpp"
#include "gfx/screen.hpp"
#include "gfx/fontmanager.hpp"
#include "misc/util.hpp"
#include "misc/printf.hpp"


ShowTextActivity::ShowTextActivity(App *app, char const *title_line, std::string const &text, SmartPtr<Command> command_after_exit)
    :
    Activity(app),
    command_after_exit(command_after_exit),
    title_line(title_line) {
    wrapped_text = gd_wrap_text(text.c_str(), app->screen->get_width() / app->font_manager->get_font_width_narrow()-4);
    linesavailable = app->screen->get_height() / app->font_manager->get_line_height()-4;
    if ((int)wrapped_text.size() < linesavailable)
        scroll_max_y = 0;
    else
        scroll_max_y = wrapped_text.size()-linesavailable;
    scroll_y = 0;
}


void ShowTextActivity::redraw_event() {
    app->clear_screen();

    app->title_line(title_line.c_str());
    // TRANSLATORS: 40 chars max
    app->status_line(_("Crsr: move     Esc: exit"));

    // up & down arrow
    app->set_color(GD_GDASH_GRAY2);
    if (scroll_y<scroll_max_y)
        app->blittext_n(app->screen->get_width() - app->font_manager->get_font_width_narrow(),
                        app->screen->get_height()-3*app->font_manager->get_line_height(), CPrintf("%c") % GD_DOWN_CHAR);
    if (scroll_y>0)
        app->blittext_n(app->screen->get_width() - app->font_manager->get_font_width_narrow(),
                        app->font_manager->get_line_height()*2, CPrintf("%c") % GD_UP_CHAR);
    // text
    app->set_color(GD_GDASH_LIGHTBLUE);
    for (int l=0; l<linesavailable && scroll_y + l<(int)wrapped_text.size(); ++l)
        app->blittext_n(app->font_manager->get_font_width_narrow()*2,
                        l*app->font_manager->get_line_height()+app->font_manager->get_line_height()*2, wrapped_text[scroll_y+l].c_str());

    app->screen->flip();
}


ShowTextActivity::~ShowTextActivity() {
    app->enqueue_command(command_after_exit);
}


void ShowTextActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    switch (keycode) {
        case App::PageUp:
            scroll_y -= linesavailable-1;
            if (scroll_y < 0)
                scroll_y = 0;
            redraw_event();
            break;
        case App::Up:
            if (scroll_y > 0) {
                scroll_y--;
                redraw_event();
            }
            break;
        case App::PageDown:
            scroll_y += linesavailable-1;
            if (scroll_y > scroll_max_y)
                scroll_y = scroll_max_y;
            redraw_event();
            break;
        case App::Down:
            if (scroll_y < scroll_max_y) {
                scroll_y++;
                redraw_event();
            }
            break;
        default:
            app->enqueue_command(new PopActivityCommand(app));
            break;
        case 0:
            /* unknown key or modifier - do nothing */
            break;
    }
}
