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
#ifndef _GD_GAMEPLAY
#define _GD_GAMEPLAY

#include "config.h"

#include "cave/helper/cavemap.hpp"
#include "cave/cavetypes.hpp"

// forward declarations
class CaveSet;
class CaveRendered;
class CaveReplay;
class CaveStored;

/// @ingroup Cave
/**
 * The GameControl class controls the flow of a game: iterating the cave,
 * loading the caves, adding score, managing number of lives etc.
 * It loads a cave from the caveset, passes movement keypresses
 * to the cave iterate routine, controls uncover and cover animations,
 * saves replays, manages highscore.
 *
 * While it is not the responsibility of this class to draw the cave to
 * the screen, it manages two maps; one map (gfx_buffer) contains the cell
 * indexes, which are essentially the cave, and the other one (covered)
 * is a map of booleans which show which cell is covered and which is not
 * (after loading and before a new cave). These maps are in the GameControl
 * class so it can easily manage them when loading a cave.
 *
 * The GameControl class has members which store the cave number, the pointer
 * to the currently played cave and so on. When playing a game, a new
 * GameControl object is created, and it is given a caveset with a start cave
 * number, or a rendered cave with or without a replay, or a stored cave
 * to test from the editor.
 *
 * The object controls rendereding the caves if needed, and also has a gfx_buffer
 * structure, which is a map with the same size that of the cave, and stores
 * the pixbuf indexes of the elements. It also controls recording replays.
 * Manages counting lives, manages bonus scores.
 *
 * To run the whole process, the main_int() function has to be called regurarly.
 * Usually it is called at 25hz or 50hz, depending on user interface settings.
 * Its first parameter is a milliseconds value, in which the caller tells the
 * GameControl, how many milliseconds have passed since the last call (40ms or
 * 20ms, respectively). The object then decides what to do - it may load the
 * cave, it may iterate the cave, it may count bonus points. The return value
 * of this function has information valuable to the caller; for example, loading
 * the cave is signaled (new cave size, new cave colors), also it is signaled
 * when the status bars have to be updated.
 *
 * Internally the GameControl has a state counter, which starts from its lowest value
 * (LOAD_CAVE), and is or is not incremented by the main_int() call. The
 * life cycle of a singe cave is as follows:
 *  - LOAD_CAVE: to load the cave from the caveset (for replays, tests, nothing)
 *  - SHOW_STORY: showing the story of the cave (only for games)
 *  - (SHOW_STORY_WAIT: waiting for the user to press fire)
 *  - (UNCOVER_START: the first frame of the uncover animation)
 *  - UNCOVER: uncovering the cave (70 frames total)
 *  - (UNCOVER_ALL: last frame of uncover animation)
 *  - CAVE_RUNNING: play.
 *  - CHECK_BONUS_TIME: when the cave is finished, bonus time is added to the points,
 *      depending on remaining cave time.
 *  - WAIT_BEFORE_COVER: a 4s wait before covering the cave, after counting bonus
 *      points.
 *  - COVER_START: first frame of covering animation (8 frames total)
 *  - COVER_ALL: last frame of covering animation
 */
class GameControl {
public:
    /// Type of a GameControl.
    enum Type {
        TYPE_NORMAL,            ///< Normal game from a caveset, with cave number and level number.
        TYPE_SNAPSHOT,          ///< Playing from a snapshot (continuing).
        TYPE_TEST,              ///< Testing a cave from the editor.
        TYPE_REPLAY,            ///< Playing a replay.
        TYPE_CONTINUE_REPLAY,   ///< During the replay, the user took control of the cave.
    };

    /// Returned by the main_int() function, this state variable stores important information for
    /// the caller, who controls drawing the cave on the screen.
    enum State {
        STATE_CAVE_LOADED,      ///< The new cave is loaded.
        STATE_SHOW_STORY,       ///< When this is received, the story should be shown to the user. First frame.
        STATE_SHOW_STORY_WAIT,  ///< When the story is shown, and has to be redrawn.
        STATE_PREPARE_FIRST_FRAME,       ///< Show thing in ui on which the cave can be drawn
        STATE_FIRST_FRAME,      ///< First frame - screen should be ready for drawing.
        STATE_NOTHING,          ///< Nothing special. Just draw the cave.
        STATE_LABELS_CHANGED,   ///< Draw the cave, and redraw header (with points etc.)
        STATE_TIMEOUT_NOW,      ///< This may be given once, at the exact moment of the cave timeout.
        STATE_NO_MORE_LIVES,    ///< Will be a game over; show "game over" sign to the user. A cover animation is still on the way!
        STATE_STOP,             ///< Finished; the GameControl object can be destroyed.
        STATE_GAME_OVER,        ///< Finished, but this is a game over, so highscore can be recorded by the caller for the game.
    };

    static GameControl *new_normal(CaveSet *caveset, std::string _player_name, int _cave, int _level);
    static GameControl *new_snapshot(const CaveRendered *snapshot);
    static GameControl *new_test(CaveStored *cave, int level);
    static GameControl *new_replay(CaveSet *caveset, CaveStored *cave, CaveReplay *replay);
    ~GameControl();

    /* functions to work on */
    CaveRendered *create_snapshot() const;
    State main_int(int millisecs_elapsed, GdDirectionEnum player_move, bool fire, bool suicide, bool restart, bool allow_iterate, bool fast_forward);

    /// Returns true if the game is running or finished - ie. game header should be shown.
    bool game_header() const {
        return state_counter>=0;
    }

    /* public variables */
    Type type;

    std::string player_name;    ///< Name of player
    int player_score;           ///< Score of player
    int player_lives;           ///< Remaining lives of player

    CaveSet *caveset;           ///< Caveset used to load next cave in normal games.
    CaveRendered *played_cave;  ///< Rendered version of the cave. This is the iterated one
    CaveStored *original_cave;  ///< original cave from caveset. Used to record highscore, as it is associated with the original cave in the caveset.

    int bonus_life_flash;       ///< flashing for bonus life
    int animcycle;              ///< animation frames, from 0 to 7, and then again 0

    CaveMap<int> gfx_buffer;      ///< contains the indexes to the cells; created by *start_level, deleted by *stop_game
    CaveMap<bool> covered;        ///< a map which stores which cells of the cave are still covered

    int replay_no_more_movements;
    bool story_shown;           ///< variable to remember if the story for a particular cave is to be shown.

private:
    CaveReplay *replay_record;
    CaveReplay *replay_from;
    unsigned int cave_num;      ///< actual playing cave number
    unsigned int level_num;     ///< actual playing level
    int cave_score;             ///< score collected in this cave
    int milliseconds_game;      ///< here we remember, how many milliseconds have passed since we last iterated the cave
    int milliseconds_anim;      ///< to remember how many milliseconds passed since last drawing the cave
    int state_counter;          ///< counter used to control the game flow, rendering of caves

    void add_bonus_life(bool inform_user);
    void increment_score(int increment);
    void select_next_level_indexes();

    /* internal functions for the different states */
    void load_cave();
    State show_story();
    void start_uncover();
    void uncover_animation();
    void uncover_all();
    State iterate_cave(int millisecs_elapsed, bool fast_forward, GdDirectionEnum player_move, bool fire, bool suicide, bool restart);
    State wait_before_cover();
    State check_bonus_score();
    void cover_animation();
    State finished_covering();

    // default constructor - only used internally
    GameControl();

    // do not allow copying and assigning - these functions are unimplemented
    GameControl(const GameControl &);
    GameControl &operator=(const GameControl &);
};

#endif
