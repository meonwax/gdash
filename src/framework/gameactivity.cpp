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

#include "framework/gameactivity.hpp"
#include "framework/titlescreenactivity.hpp"
#include "framework/commands.hpp"
#include "gfx/screen.hpp"
#include "cave/caveset.hpp"
#include "cave/gamecontrol.hpp"
#include "settings.hpp"
#include "sound/sound.hpp"
#include "input/gameinputhandler.hpp"


GameActivity::GameActivity(App *app, GameControl *game)
    :   Activity(app),
        game(game),
        cellrenderer(*app->pixbuf_factory, gd_theme),
        gamerenderer(*app->screen, cellrenderer, *app->font_manager, *game),
        exit_game(false),
        show_highscore(false),
        paused(false),
        time_ms(0) {
}


GameActivity::~GameActivity() {
    gd_sound_off(); /* we stop sounds. */
}


void GameActivity::shown_event() {
    app->game_active(true);
    int cell_size = cellrenderer.get_cell_size();
    app->screen->set_size(cell_size*GAME_RENDERER_SCREEN_SIZE_X, cell_size*GAME_RENDERER_SCREEN_SIZE_Y);
    gd_music_stop();
}


void GameActivity::hidden_event() {
    app->game_active(false);
}


void GameActivity::redraw_event() {
    gamerenderer.redraw();
}


void GameActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    switch (keycode) {
        case EndGameKey:
            exit_game = true;
            break;
        case RestartLevelKey:
            app->gameinput->restart_level = true;     /* hack */
            break;
        case PauseKey:
            paused = !paused;
            if (paused) {
                gamerenderer.statusbar_since = 0;     /* count frames "paused" */
                gd_sound_off();                       /* if paused, no sound. */
            }
            break;
        case RandomColorKey:
            gamerenderer.set_random_colors();
            break;
        case 'h':
        case 'H': {
            /* switch off sounds when showing help. */
            /* no need to turn on sounds later; next cave iteration will restore them. */
            gd_sound_off();
            const char *strings_menu[]= {
                app->gameinput->get_key_name(GameInputHandler::KeyLeft), _("Move left"),
                app->gameinput->get_key_name(GameInputHandler::KeyRight), _("Move right"),
                app->gameinput->get_key_name(GameInputHandler::KeyUp), _("Move up"),
                app->gameinput->get_key_name(GameInputHandler::KeyDown), _("Move down"),
                app->gameinput->get_key_name(GameInputHandler::KeyFire1), _("Snap"),
                app->gameinput->get_key_name(GameInputHandler::KeySuicide), _("Suicide"),
                // TRANSLATORS: "hold" here means that the key must be kept depressed by the user.
                app->gameinput->get_key_name(GameInputHandler::KeyFastForward), _("Fast forward (hold)"),
                // TRANSLATORS: "hold" here means that the key must be kept depressed by the user.
                app->gameinput->get_key_name(GameInputHandler::KeyStatusBar), _("Status bar (hold)"),
                app->gameinput->get_key_name(GameInputHandler::KeyRestartLevel), _("Restart level"),
                /* if you change these, change the enum in the class definition as well */
                "", "",
                _("Space"), _("Pause"),
                //~ "F3", _("Take snapshot"),
                //~ "F4", _("Revert to snapshot"),
                "", "",
                "F2", _("Random colors"),
                "F9", _("Sound volume"),
                "F1", _("End game"),
                NULL
            };
            // TRANSLATORS: title of the help window during the game
            app->show_text_and_do_command(_("Game Help"), help_strings_to_string(strings_menu));
        }
        break;
    }
}


void GameActivity::timer_event(int ms_elapsed) {
    time_ms += ms_elapsed;
    int step = gd_fine_scroll ? 20 : 40;
    /* check if enough time has elapsed to signal the game object. */
    if (time_ms < step)
        return;
    time_ms -= step;

    GdDirectionEnum player_move = gd_direction_from_keypress(app->gameinput->up(), app->gameinput->down(), app->gameinput->left(), app->gameinput->right());
    GameRenderer::State state = gamerenderer.main_int(step, player_move, app->gameinput->fire1() || app->gameinput->fire2(), app->gameinput->suicide, app->gameinput->restart_level, paused, app->gameinput->alternate_status, app->gameinput->fast_forward);

    /* state of game, returned by gd_game_main_int */
    switch (state) {
        case GameRenderer::CaveLoaded:
        case GameRenderer::Iterated:
        case GameRenderer::Nothing:
            break;

        case GameRenderer::Stop:        /* game stopped, this could be a replay or a snapshot */
            exit_game=true;
            break;

        case GameRenderer::GameOver:    /* normal game stopped, may jump to highscore later. */
            exit_game=true;
            show_highscore=true;
            break;
    }

    if (exit_game) {
        /* pop the game activity. */
        app->enqueue_command(new PopActivityCommand(app));
        /* if showing highscore, enqueue a command to show it. */
        /* there might be a command to be run after the game. if there is no highscore,
         * the game object will execute it, when popping. if there is a highscoresactivity
         * object, the command to be run is passed to it. */
        if (show_highscore && game->caveset->highscore.is_highscore(game->player_score)) {
            /* enter to highscore table */
            int rank = game->caveset->highscore.add(game->player_name, game->player_score);
            app->enqueue_command(new ShowHighScoreCommand(app, NULL, rank));
        } else {
            /* no high score */
        }
    }
}

