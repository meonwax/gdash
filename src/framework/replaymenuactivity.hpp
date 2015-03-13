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

#include "framework/app.hpp"

class CaveStored;
class CaveReplay;

/** Show the replays of the current caveset to the user,
 * and also allow him to play them.
 * In the SDL version, this Activity may also launch a ReplaySaverActivity
 * to save the replay to a WAV/PNG combo. */
class ReplayMenuActivity: public Activity {
public:
    /** Ctor.
     * @param app The parent app. */
    ReplayMenuActivity(App *app);
    virtual void pushed_event();
    virtual void redraw_event();
    virtual void keypress_event(KeyCode keycode, int gfxlib_keycode);

private:
    /** This is a line of the list shown to the user.
     * For replays, both pointers are non-null. For cave names, replay is null.
     * For an empty (separator) line, both are null. */
    struct ReplayItem {
        CaveStored *cave;       ///< The cave to which the replay belongs.
        CaveReplay *replay;     ///< The replay object.
    };
    /** List of replays - or, the list of lines to be shown to the user. */
    std::vector<ReplayItem> items;
    /** Lines to show on one page. */
    unsigned lines_per_page;
    /** The current line selected. */
    unsigned current;
};
