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

#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "framework/titlescreenactivity.hpp"
#include "framework/gameactivity.hpp"
#include "framework/inputtextactivity.hpp"
#include "framework/settingsactivity.hpp"
#include "framework/replaymenuactivity.hpp"
#include "framework/commands.hpp"
#include "input/gameinputhandler.hpp"
#include "input/joystick.hpp"
#include "cave/caveset.hpp"
#include "cave/titleanimation.hpp"
#include "settings.hpp"
#include "gfx/screen.hpp"
#include "gfx/pixmap.hpp"
#include "gfx/pixbuffactory.hpp"
#include "gfx/fontmanager.hpp"
#include "sound/sound.hpp"
#include "misc/about.hpp"
#include "cave/gamecontrol.hpp"
#include "settings.hpp"


TitleScreenActivity::TitleScreenActivity(App *app)
    :
    Activity(app),
    scale(app->pixbuf_factory->get_pixmap_scale()),
    image_centered_threshold(164*scale),
    frames(0), time_ms(0), animcycle(0),
    alternate_status(false) {
    cavenum = app->caveset->last_selected_cave;
    levelnum = app->caveset->last_selected_level;
}


TitleScreenActivity::~TitleScreenActivity() {
    clear_animation();
}


void TitleScreenActivity::shown_event() {
    int scale = app->pixbuf_factory->get_pixmap_scale();
    app->screen->set_size(scale * 320, scale * 200);

    /* render title screen animation in memory pixmap */
    animation = get_title_animation_pixmap(app->caveset->title_screen, app->caveset->title_screen_scroll, false, *app->pixbuf_factory);

    /* height of title screen, then decide which lines to show and where */
    image_h=animation[0]->get_height();
    int font_h=app->font_manager->get_font_height();
    /* less than 2 lines left - place for only one line of text. */
    if (app->screen->get_height()-image_h < 2*font_h) {
        y_gameline=-1;
        y_caveline=image_h + (app->screen->get_height()-image_h-font_h)/2;    /* centered in the small place */
        show_status=false;
    } else if (app->screen->get_height()-image_h < 3*font_h) {
        /* more than 2, less than 3 - place for status bar. game name is not shown, as this will */
        /* only be true for a game with its own title screen, and i decided that in that case it */
        /* would make more sense. */
        y_gameline=-1;
        y_caveline=image_h + (app->screen->get_height()-image_h-font_h*2)/2;  /* centered there */
        show_status=true;
    } else {
        /* more than 3, less than 4 - place for everything. */
        y_gameline=image_h + (app->screen->get_height()-image_h-font_h-font_h*2)/2;   /* centered with cave name */
        y_caveline=y_gameline+font_h;
        /* if there is some place, move the cave line one pixel lower. */
        if (y_caveline+2*font_h<app->screen->get_height())
            y_caveline+=1*scale;
        show_status=true;
    }

    /* this is required because the caveset might have changed since the last redraw, and
     * thus the title screen might have changed, and the new title screen might have fewer
     * frames than the original. */
    animcycle = 0;
    
    app->screen->set_title(CPrintf("GDash - %s") % app->caveset->name);
    gd_music_play_random();
}


void TitleScreenActivity::clear_animation() {
    for (unsigned x=0; x<animation.size(); x++)
        delete animation[x];
    animation.clear();
}


void TitleScreenActivity::hidden_event() {
    clear_animation();
}


void TitleScreenActivity::redraw_event() {
    app->clear_screen();

    if (y_gameline!=-1) {
        // TRANSLATORS: Game here is like caveset, the loaded game from which the user will select the cave to play
        app->blittext_n(0, y_gameline, CPrintf("%c%s: %c%s %c%s") % GD_COLOR_INDEX_WHITE % _("Game") % GD_COLOR_INDEX_YELLOW % app->caveset->name % GD_COLOR_INDEX_RED % (app->caveset->edited ? "*" : ""));
    }

    int dx=(app->screen->get_width()-animation[animcycle]->get_width())/2;    /* centered horizontally */
    int dy;
    if (animation[animcycle]->get_height()<image_centered_threshold)
        dy=(image_centered_threshold - animation[animcycle]->get_height())/2; /* centered vertically */
    else
        dy=0;   /* top of screen, as not too much space was left for info lines */
    app->screen->blit(*animation[animcycle], dx, dy);

    if (show_status) {
        if (Joystick::have_joystick() && alternate_status) {
            // TRANSLATORS: 40 chars max. Joy here is the joystick (that one can also select the cave)
            app->status_line(_("Joy: select   Fire: play"));
        } else {
            // TRANSLATORS: 40 chars max. Select the game to play.
            if (get_active_logger().empty()) {
                app->status_line(_("Crsr: select   Space: play   H: help"));
            } else {
                app->status_line(_("Crsr: select   Space: play   X: errors"));
            }
        }
    }
    /* selected cave */
    if (app->caveset->caves.size() == 0) {
        app->blittext_n(0, y_caveline, CPrintf(_("%cNo caves.")) % GD_COLOR_INDEX_WHITE);
    } else {
        // TRANSLATORS: Cave is the name of the cave to play
        app->blittext_n(0, y_caveline, CPrintf(_("%cCave: %c%s%c/%c%d")) % GD_COLOR_INDEX_WHITE % GD_COLOR_INDEX_YELLOW % app->caveset->cave(cavenum).name % GD_COLOR_INDEX_WHITE % GD_COLOR_INDEX_YELLOW % (levelnum+1));
    }

    app->screen->flip();
}


static int previous_selectable_cave(CaveSet &caveset, unsigned cavenum) {
    unsigned cn=cavenum;
    while (cn>0) {
        cn--;
        if (gd_all_caves_selectable || caveset.cave(cn).selectable)
            return cn;
    }

    /* if not found any suitable, return current */
    return cavenum;
}


static int next_selectable_cave(CaveSet &caveset, unsigned cavenum) {
    unsigned cn=cavenum;
    while (cn+1<caveset.caves.size()) {
        cn++;
        if (gd_all_caves_selectable || caveset.cave(cn).selectable)
            return cn;
    }

    /* if not found any suitable, return current */
    return cavenum;
}


void TitleScreenActivity::timer_event(int ms_elapsed) {
    time_ms += ms_elapsed;
    if (time_ms >= 40) {
        time_ms -= 40;
        animcycle=(animcycle+1)%animation.size();
        frames++;
        if (frames>100) {
            frames=0;
            alternate_status=!alternate_status;
        }

        /* on every 5th timer event... */
        if (frames % 5 == 0) {
            /* joystick or keyboard up */
            if (app->gameinput->up()) {
                levelnum++;
                if (levelnum>4)
                    levelnum=4;
            }
            /* joystick or keyboard down */
            if (app->gameinput->down()) {
                levelnum--;
                if (levelnum<0)
                    levelnum=0;
            }
            /* joystick or keyboard left */
            if (app->gameinput->left())
                cavenum = previous_selectable_cave(*app->caveset, cavenum);
            /* joystick or keyboard right */
            if (app->gameinput->right())
                cavenum = next_selectable_cave(*app->caveset, cavenum);

            /* for a fire event, maybe from the joystick, start the game immediately.
             * when from the keyboard, we would ask the user name,
             * but how would the user press the enter key? :) */
            if (app->gameinput->fire1()) {
                NewGameCommand *command = new NewGameCommand(app, cavenum, levelnum);
                command->set_param1(gd_username);
                app->enqueue_command(command);
            }
        }

        redraw_event();
    }
}


void show_errors(App *app, Logger &l) {
    std::string text;

    Logger::Container const &errors = l.get_messages();
    for (Logger::Container::const_iterator it = errors.begin(); it != errors.end(); ++it) {
        text += it->message;
        text += "\n\n";
    }
    // TRANSLATORS: 40 chars max
    app->show_text_and_do_command(_("GDASH ERROR CONSOLE"), text);
    l.clear();
}


void TitleScreenActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    switch (keycode) {
        case 'h':
        case 'H': {
            static char const *strings[]= {
                // TRANSLATORS: cursor keys selected for playing, or joystick movement
                _("Cursor, joy"), _("Select cave & level"),
                // TRANSLATORS: users press space of game/joystick fire button to play
                _("Space, Fire1"), _("Play the game"),
                "", "",
                // TRANSLATORS: string should be short (~20 chars)
                "L, Tab (C)", _("Load (installed caves)"),
                // TRANSLATORS: string should be short (~20 chars)
                "S (N)", _("Save (Save as)"),
                "I", _("Caveset info"),
                "F", _("Hall of fame"),
                "R", _("Replays"),
                "", "",
                "E", _("Editor"),
                "", "",
                "F9", _("Sound volume"),
                // TRANSLATORS: string should be short (~20 chars)
                "F11", _("Fullscreen on/off"),
                "O", _("Options"),
                "K", _("Keyboard options"),
                "X", _("Error console"),
                "A", _("About GDash"),
                "", "",
                "Escape, Q", _("Quit game"),
                NULL
            };
            app->show_text_and_do_command(_("GDash Help"), help_strings_to_string(strings));
        }
        break;
        case 'a':
        case 'A':
            app->show_about_info();
            break;
        case 'i':
        case 'I':
            app->enqueue_command(new ShowCaveInfoCommand(app));
            break;
        case 's':
        case 'S':
            app->enqueue_command(new SaveFileCommand(app));
            break;
        case 'n':
        case 'N':
            app->enqueue_command(new SaveFileAsCommand(app));
            break;
        case 'l':
        case 'L':
        case App::Tab:
            app->enqueue_command(new SelectFileToLoadIfDiscardableCommand(app, gd_last_folder));
            break;
        case 'c':
        case 'C':
            app->enqueue_command(new SelectFileToLoadIfDiscardableCommand(app, gd_system_caves_dir));
            break;
        case 'e':
        case 'E':
            app->start_editor();
            break;
        case 'o':
        case 'O':
            app->show_settings(gd_get_game_settings_array());
            break;
        case 'k':
        case 'K':
            app->show_settings(gd_get_keyboard_settings_array(app->gameinput));
            break;
        case 'f':
        case 'F':
            app->enqueue_command(new ShowHighScoreCommand(app, NULL, -1));
            break;
        case 'x':
        case 'X':
            app->enqueue_command(new ShowErrorsCommand(app, get_active_logger()));
            break;
        case 'r':
        case 'R':
            app->enqueue_command(new PushActivityCommand(app, new ReplayMenuActivity(app)));
            break;
        case App::Enter:
        case ' ':
            app->caveset->last_selected_cave = cavenum;
            app->caveset->last_selected_level = levelnum;
            app->input_text_and_do_command(_("Enter your name"), gd_username.c_str(), new NewGameCommand(app, cavenum, levelnum));
            break;
        case 'q':
        case 'Q':
            app->quit_event();
            break;
        case App::Escape:
            /* if edited, do as if a quit is requested. then the user will be asked if discards edit. */
            /* otherwise, simply ask if he wants to quit. */
            if (app->caveset->edited)
                app->quit_event();
            else
                app->ask_yesorno_and_do_command(_("Quit game?"), _("yes"), _("no"), new PopAllActivitiesCommand(app),
                    SmartPtr<Command>());
            break;
    }
}
