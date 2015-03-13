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

#include <glib/gi18n.h>

#include "framework/volumeactivity.hpp"
#include "framework/commands.hpp"
#include "gfx/fontmanager.hpp"
#include "gfx/screen.hpp"
#include "misc/util.hpp"
#include "sound/sound.hpp"
#include "settings.hpp"


VolumeActivity::VolumeActivity(App *app)
: Activity(app), wait_ms(WaitBeforeDisappearMs) {
}


void VolumeActivity::redraw_event() {
    int height=6*app->font_manager->get_line_height();
    int y1=(app->screen->get_height()-height)/2;    /* middle of the screen */
    int cx=2*app->font_manager->get_font_width_narrow(), cy=y1, cw=app->screen->get_width()-2*cx, ch=height;

    app->draw_window(cx, cy, cw, ch);
    app->screen->set_clip_rect(cx, cy, cw, ch);
    app->set_color(GD_GDASH_WHITE);
    // TRANSLATORS: sound volume
    app->blittext_n(-1, y1+app->font_manager->get_line_height(), _("Volume"));

    std::string name, value;
    
    // TRANSLATORS: sound volume during game. the space at the end makes the text of the same length with the other string.
    name = _("Cave volume ");
    value = "";
    for (int i = 0; i < 100; i += 10) /* each char is 10% */
        value += (i < gd_sound_chunks_volume_percent) ? GD_FULL_BOX_CHAR : GD_UNCHECKED_BOX_CHAR;
    app->blittext_n(-1, y1+3*app->font_manager->get_line_height(), CPrintf("%c%s  F5 %c%d %cF6") % GD_COLOR_INDEX_LIGHTBLUE % name % GD_COLOR_INDEX_GREEN % value % GD_COLOR_INDEX_LIGHTBLUE);

    // TRANSLATORS: title screen music volume. the space at the end makes the text of the same length with the other string.
    name = _("Music volume");
    value = "";
    for (int i = 0; i < 100; i += 10) /* each char is 10% */
        value += (i < gd_sound_music_volume_percent) ? GD_FULL_BOX_CHAR : GD_UNCHECKED_BOX_CHAR;
    app->blittext_n(-1, y1+4*app->font_manager->get_line_height(), CPrintf("%c%s  F7 %c%d %cF8") % GD_COLOR_INDEX_LIGHTBLUE % name % GD_COLOR_INDEX_GREEN % value % GD_COLOR_INDEX_LIGHTBLUE);

    app->screen->remove_clip_rect();

    app->screen->flip();
}


void VolumeActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    switch (keycode) {
        case App::F5:
            gd_sound_chunks_volume_percent = gd_clamp(gd_sound_chunks_volume_percent-10, 0, 100);
            gd_sound_set_chunk_volumes();
            redraw_event();
            wait_ms = WaitBeforeDisappearMs;
            break;
        case App::F6:
            gd_sound_chunks_volume_percent = gd_clamp(gd_sound_chunks_volume_percent+10, 0, 100);
            gd_sound_set_chunk_volumes();
            redraw_event();
            wait_ms = WaitBeforeDisappearMs;
            break;
        case App::F7:
            gd_sound_music_volume_percent = gd_clamp(gd_sound_music_volume_percent-10, 0, 100);
            gd_sound_set_music_volume();
            redraw_event();
            wait_ms = WaitBeforeDisappearMs;
            break;
        case App::F8:
            gd_sound_music_volume_percent = gd_clamp(gd_sound_music_volume_percent+10, 0, 100);
            gd_sound_set_music_volume();
            redraw_event();
            wait_ms = WaitBeforeDisappearMs;
            break;
        case App::Escape:
            /* do this so the next event will remove it */
            wait_ms = 0;
            break;
    }
}


void VolumeActivity::timer_event(int ms_elapsed) {
    wait_ms -= ms_elapsed;
    if (wait_ms <= 0) {
        app->enqueue_command(new PopActivityCommand(app));
    }
}
