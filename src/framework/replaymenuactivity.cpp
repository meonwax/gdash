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
#include <glib/gi18n.h>

#include "framework/commands.hpp"
#include "framework/gameactivity.hpp"
#include "framework/replaysaveractivity.hpp"
#include "framework/showtextactivity.hpp"
#include "gfx/screen.hpp"
#include "gfx/fontmanager.hpp"
#include "cave/gamecontrol.hpp"
#include "misc/printf.hpp"
#include "misc/util.hpp"
#include "cave/cavestored.hpp"
#include "cave/caveset.hpp"
#include "framework/replaymenuactivity.hpp"

ReplayMenuActivity::ReplayMenuActivity(App *app)
    : Activity(app) {
    lines_per_page = (app->screen->get_height() / app->font_manager->get_line_height()) - 5;

    /* for all caves */
    for (unsigned n=0; n<app->caveset->caves.size(); ++n) {
        CaveStored &cave=app->caveset->cave(n);

        /* if cave has replays... */
        if (!cave.replays.empty()) {
            /* add an empty item as separator */
            if (!items.empty()) {
                ReplayItem i = { NULL, NULL };
                items.push_back(i);
            }
            /* add cave data */
            ReplayItem i = { &cave, NULL };
            items.push_back(i);
            /* add replays, too */
            for (std::list<CaveReplay>::iterator rit=cave.replays.begin(); rit!=cave.replays.end(); ++rit) {
                // .cave unchanged, .replay is the address of the replay
                i.replay=&*rit;
                items.push_back(i);
            }
        }
    }
    current=1;
}


void ReplayMenuActivity::pushed_event() {
    if (items.empty()) {
        app->enqueue_command(new PopActivityCommand(app)); // pop myself
        // TRANSLATORS: 40 chars max
        app->enqueue_command(new PushActivityCommand(app, new ShowTextActivity(app, _("Replays"), _("No replays for caveset."), SmartPtr<Command>())));
    }
}

void ReplayMenuActivity::redraw_event() {
    app->clear_screen();

    int page=current/lines_per_page;
    // TRANSLATORS: 40 chars max
    app->title_line(CPrintf(_("Replays, Page %d/%d")) % (page+1) % (items.size()/lines_per_page+1));
    // TRANSLATORS: 40 chars max
    app->status_line(_("Space: play  F1: keys  Esc: exit"));

    for (unsigned n=0; n<lines_per_page && page*lines_per_page+n<items.size(); n++) {
        unsigned pos=page*lines_per_page+n;

        int y = (n+2) * app->font_manager->get_line_height();

        if (items[pos].cave && !items[pos].replay) {
            /* no replay pointer: this is a cave, so write its name. */
            app->set_color(GD_GDASH_WHITE);
            app->blittext_n(0, y, CPrintf(" %s") % items[pos].cave->name);
        }
        if (items[pos].replay) {
            /* level, successful/not, saved/not, player name  */
            app->blittext_n(0, y, CPrintf("  %c%c %c%c %cL%d, %s")
                            % GD_COLOR_INDEX_LIGHTBLUE % (items[pos].replay->saved?GD_CHECKED_BOX_CHAR:GD_UNCHECKED_BOX_CHAR)
                            % (items[pos].replay->success?GD_COLOR_INDEX_GREEN:GD_COLOR_INDEX_RED) % GD_BALL_CHAR
                            % (current==pos ? GD_COLOR_INDEX_YELLOW : GD_COLOR_INDEX_GRAY3)
                            % items[pos].replay->level % items[pos].replay->player_name);
        }
    }

    app->screen->flip();
}


/* the replay saver thing only works in the sdl version */
#ifdef HAVE_SDL

class SaveReplayCommand: public Command1Param<std::string> {
public:
    SaveReplayCommand(App *app, CaveStored *cave, CaveReplay *replay)
        :
        Command1Param<std::string>(app),
        filename_prefix(p1),
        cave(cave),
        replay(replay) {
    }
private:
    std::string &filename_prefix;
    CaveStored *cave;
    CaveReplay *replay;
    void execute() {
        app->enqueue_command(new PushActivityCommand(app, new ReplaySaverActivity(app, cave, replay, filename_prefix)));
    }
};

#endif /* IFDEF HAVE_SDL */


void ReplayMenuActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    /* these keys are active also when there are replays and when there aren't any. */
    switch (keycode) {
        case App::F1: {
            char const *help[]= {
                _("Cursor keys"), _("Move"),
                // TRANSLATORS: play here means playing the replay movie
                _("Space, Enter"), _("Play"),
                "I", _("Show replay info"),
                "", "",
                // TRANSLATORS: this is for a checkbox which selects if the replays is to be saved with the cave or not.
                "S", _("Toggle if saved with caves"),
                /* the replay saver thing only works in the sdl version */
#ifdef HAVE_SDL
                "W", _("Save movie"),
#endif /* IFDEF HAVE_SDL */
                "", "",
                "ESC", _("Main menu"),
                NULL
            };
            // TRANSLATORS: title of the cave replays window
            app->show_text_and_do_command(_("Replays Help"), help_strings_to_string(help));
        }
        break;
        case App::Escape:
            app->enqueue_command(new PopActivityCommand(app));
            break;
    }

    /* if no replays, the keys below should be inactive. so return. */
    if (items.empty())
        return;


    switch (keycode) {
        case App::Up:
            do {
                current = gd_clamp(current-1, 1, items.size()-1);
            } while (items[current].replay==NULL && current>=1);
            redraw_event();
            break;
        case App::Down:
            do {
                current = gd_clamp(current+1, 1, items.size()-1);
            } while (items[current].replay==NULL && current<items.size());
            redraw_event();
            break;
        case App::PageUp:
            current = gd_clamp(current-lines_per_page, 0, items.size()-1);
            redraw_event();
            break;
        case App::PageDown:
            current = gd_clamp(current+lines_per_page, 0, items.size()-1);
            redraw_event();
            break;
        case 'i':
        case 'I': {
            std::string s;
            CaveReplay *r = items[current].replay;
            s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Cave") % GD_COLOR_INDEX_YELLOW % items[current].cave->name;
            // TRANSLATORS: Cave level (1-5)
            s += SPrintf("%c%s: %c%d\n") % GD_COLOR_INDEX_WHITE % _("Level") % GD_COLOR_INDEX_YELLOW % r->level;
            // TRANSLATORS: "player" here means the name of the human player who played the replay
            s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Player") % GD_COLOR_INDEX_YELLOW % r->player_name;
            s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Successful") % GD_COLOR_INDEX_LIGHTBLUE % (r->success ? _("yes") : _("no"));
            s += SPrintf("%c%s: %c%d %s\n") % GD_COLOR_INDEX_WHITE % _("Score") % GD_COLOR_INDEX_LIGHTBLUE % r->score % _("points");
            s += "\n";
            if (r->comment != "")
                s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Comment") % GD_COLOR_INDEX_LIGHTBLUE % r->comment;
            if (r->duration > 0) {
                // TRANSLATORS: duration of replay in seconds
                s += SPrintf("%c%s: %c%ds\n") % GD_COLOR_INDEX_WHITE % _("Duration") % GD_COLOR_INDEX_LIGHTBLUE % r->duration;
            }
            if (r->date != "")
                s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Date") % GD_COLOR_INDEX_LIGHTBLUE % r->date;
            if (r->recorded_with != "")
                // TRANSLATORS: ie. software by which the replay was recorded.
                s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Recorded with") % GD_COLOR_INDEX_LIGHTBLUE % r->recorded_with;

            // TRANSLATORS: title of the window showing data about a particular replay
            app->show_message(_("Replay Info"), s);
        }
        break;
        case 's':
        case 'S':
            if (items[current].replay) {
                items[current].replay->saved=!items[current].replay->saved;
                app->caveset->edited = true;
                redraw_event();
            }
            break;
            /* the replay saver thing only works in the sdl version */
#ifdef HAVE_SDL
        case 'w':
        case 'W': {
            std::string prefix = SPrintf("%s%sout") % gd_last_folder % G_DIR_SEPARATOR;
            // TRANSLATE: the prefix of the name of the output files when saving the replay.
            app->input_text_and_do_command(_("Output filename prefix"), prefix.c_str(), new SaveReplayCommand(app, items[current].cave, items[current].replay));
        }
        break;
#endif /* IFDEF HAVE_SDL */
        case ' ':
        case App::Enter:
            if (items[current].replay) {
                GameControl *game = GameControl::new_replay(app->caveset, items[current].cave, items[current].replay);
                app->enqueue_command(new PushActivityCommand(app, new GameActivity(app, game)));
            }
            break;
    }
}
