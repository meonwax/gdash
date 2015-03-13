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

#ifndef _GD_GAMEAPPLET
#define _GD_GAMEAPPLET

#include "framework/app.hpp"
#include "gfx/cellrenderer.hpp"
#include "cave/gamerender.hpp"

class GameControl;
class Command;

class GameActivity: public Activity {
public:
    GameActivity(App *app, GameControl *game);
    ~GameActivity();
    virtual void shown_event();
    virtual void hidden_event();
    virtual void redraw_event();
    virtual void keypress_event(KeyCode keycode, int gfxlib_keycode);
    virtual void timer_event(int ms_elapsed);
    
    /* if you change these, change the help strings in the cpp file as well */
    enum Keys {
        EndGameKey = App::F1,
        RandomColorKey = App::F2,
        TakeSnapshotKey = App::F3,
        RevertToSnapshotKey = App::F4,
        PauseKey = ' ',
        RestartLevelKey = App::F12,
    };

private:
    GameControl *game;
    CellRenderer cellrenderer;
    GameRenderer gamerenderer;
    bool exit_game, show_highscore, paused;
    int time_ms;
};

#endif

