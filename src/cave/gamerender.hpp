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

#ifndef _GD_GAMERENDERER
#define _GD_GAMERENDERER

#include "config.h"

#include <vector>
#include <string>

#include "cave/cavetypes.hpp"
#include "cave/helper/colors.hpp"

class Screen;
class CellRenderer;
class FontManager;
class GameControl;
class Pixbuf;
class Pixmap;
class PixbufFactory;

#define GAME_RENDERER_SCREEN_SIZE_X (20)
#define GAME_RENDERER_SCREEN_SIZE_Y (12.5)

enum GdStatusBarType {
    GD_STATUS_BAR_ORIGINAL,
    GD_STATUS_BAR_1STB,
    GD_STATUS_BAR_CRLI,
    GD_STATUS_BAR_FINAL,
    GD_STATUS_BAR_ATARI_ORIGINAL,
};
enum {
    // should be the last from the status bar type
    GD_STATUS_BAR_MAX=int(GD_STATUS_BAR_ATARI_ORIGINAL)
};
const char **gd_status_bar_colors_get_names();


/// @ingroup Cave
///
/// @brief This class manages drawing the cave and scrolling on the screen; it also
/// receives keypresses (controls) from the user and passes it to the game
/// controlling class GameControl.
///
/// The GameRenderer class can be used the following way:
/// - Creating an object, and assigning it a CellRenderer, a FontManager, a Screen
///   for the graphics stuff, and a GameControl object to control the game.
/// - The GameRenderer::main_int() function is to be called regurarly, and passed
///   the number of milliseconds elapsed (that can be 20 or 40), and the user
///   controls.
/// The object will then control the game and also draw it. And that's all.
/// Showing the story, scrolling the cave, drawing the status bar, enabling and
/// disabling game pause and everything is handled inside.
class GameRenderer {
    Screen &screen;
    CellRenderer &cells;
    FontManager &font_manager;
    GameControl &game;

    int play_area_w;
    int play_area_h;
    int statusbar_height;
    int statusbar_y1;
    int statusbar_y2;
    int statusbar_mid;

    bool out_of_window;

    bool show_replay_sign;

    int scroll_x, scroll_y;
    int scroll_desired_x, scroll_desired_y;

    // for showing the story
    std::vector<std::string> wrapped_story;
    int story_y, story_max_y;
    unsigned linesavailable;
    Pixmap *story_background;
    bool toggle;

    void drawstory();
    void drawcave(bool force_draw);
    static bool cave_scroll(int logical_size, int physical_size, int center, bool exact, int start, int to, int &current, int &desired, int speed, int cell_size);
    bool scroll(int ms, bool exact_scroll);
    void scroll_to_origin();

    void clear_header();
    bool showheader_firstline(bool in_game);
    void showheader_uncover();
    void showheader_game(bool alternate_status_bar, bool fast_forward);
    void showheader_gameover();
    void showheader_pause();

    void set_colors_from_cave();
    void select_status_bar_colors();

    struct StatusBarColors {
        GdColor background;
        GdColor diamond_needed;
        GdColor diamond_value;
        GdColor diamond_collected;
        GdColor score;
        GdColor default_color;
    };
    StatusBarColors cols;

public:
    int statusbar_since;

    GameRenderer(Screen &screen_, CellRenderer &cells_, FontManager &font_manager_, GameControl &game_);
    ~GameRenderer();

    enum State {
        CaveLoaded,
        Nothing,
        Iterated,
        Stop,
        GameOver
    };

    State main_int(int ms, GdDirectionEnum player_move, bool fire, bool &suicide, bool &restart, bool paused, bool alternate_status, bool fast_forward);

    /**
     * Show replay sign means that for non-real-game game types,
     * the status bar can show the type ("PLAYING REPLAY" or "TESTING CAVE").
     * By default, it is turned on.
     */
    void set_show_replay_sign(bool srs);

    /**
     * The user can request some random colors if he does not like the
     * ones from the cave. */
    void set_random_colors();

    /**
     * Call this to redraw everything.
     */
    void redraw();
};


#endif
