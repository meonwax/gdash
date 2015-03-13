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

#include "cave/cavestored.hpp"
#include "cave/caverendered.hpp"
#include "cave/caveset.hpp"
#include "settings.hpp"
#include "sound/sound.hpp"
#include "misc/util.hpp"

#include "cave/gamecontrol.hpp"


/* prepare cave, gfx buffer */
#define GAME_INT_LOAD_CAVE -74
/* show description/note of cave. */
#define GAME_INT_SHOW_STORY -73
/* waiting fire button after showing the story. */
#define GAME_INT_SHOW_STORY_WAIT -72
/* after clicking, when the story seen. */
#define GAME_INT_STORY_CLICKED -71
/* start uncovering */
#define GAME_INT_START_UNCOVER -70
/* ...70 frames until full uncover... */
#define GAME_INT_UNCOVER_ALL -1
/* normal running state. */
#define GAME_INT_CAVE_RUNNING 0
/* add points for remaining time */
#define GAME_INT_CHECK_BONUS_TIME 1
/* ...2..99 = wait and do nothing, after adding time */
#define GAME_INT_WAIT_BEFORE_COVER 2
/* start covering */
#define GAME_INT_COVER_START 100
/* ... 8 frames of cover animation */
#define GAME_INT_COVER_ALL 108

/// The default constructor which fills members with some initial values.
GameControl::GameControl() :
    player_score(0),
    player_lives(0),
    caveset(NULL),
    played_cave(NULL),
    original_cave(NULL),
    bonus_life_flash(0),
    animcycle(0),
    story_shown(false),
    replay_record(NULL),
    replay_from(NULL),
    cave_num(0),
    level_num(0),
    milliseconds_game(0),
    milliseconds_anim(0),
    state_counter(GAME_INT_LOAD_CAVE) {
}

GameControl::~GameControl() {
    /* stop sounds */
    gd_sound_off();

    if (played_cave)
        delete played_cave;
    if (replay_record)
        delete replay_record;
}


/// Create a full game from the caveset.
/// @returns A newly allocated GameControl. Free with delete.
GameControl *GameControl::new_normal(CaveSet *caveset, std::string player_name, int cave, int level) {
    GameControl *g=new GameControl;

    g->type=TYPE_NORMAL;

    g->caveset=caveset;
    g->player_name=player_name;
    g->cave_num=cave;
    g->level_num=level;
    g->player_lives=g->caveset->initial_lives;
    g->story_shown=false;

    return g;
}

/// Create a new game of a single snapshow.
/// @returns A newly allocated GameControl. Free with delete.
GameControl *GameControl::new_snapshot(const CaveRendered *snapshot) {
    GameControl *g=new GameControl;

    g->played_cave=new CaveRendered(*snapshot);
    g->type=TYPE_SNAPSHOT;

    return g;
}

/// Create a new game to test in the editor.
/// @returns A newly allocated GameControl. Free with delete.
GameControl *GameControl::new_test(CaveStored *cave, int level) {
    GameControl *g=new GameControl;

    g->original_cave=cave;
    g->level_num=level;
    g->type=TYPE_TEST;

    return g;
}

/// Create a cave replay.
/// @returns A newly allocated GameControl. Free with delete.
GameControl *GameControl::new_replay(CaveSet *caveset, CaveStored *cave, CaveReplay *replay) {
    GameControl *g=new GameControl;

    g->caveset=caveset;
    g->original_cave=cave;
    g->replay_from=replay;
    g->type=TYPE_REPLAY;

    return g;
}


/// Add a bonus life (if live number is not more than maximum number of lives.
/// Only feasible for normal games, not for replays or tests.
/// @param inform_user If set to true, will also show bonus life flash. Sometimes this
///      is not desired (after completing an intermission, for example).
void GameControl::add_bonus_life(bool inform_user) {
    /* only inform about bonus life when playing a game */
    /* or when testing the cave (so the user can see that a bonus life can be earned in that cave */
    if (type==TYPE_NORMAL || type==TYPE_TEST)
        if (inform_user) {
            gd_sound_play_bonus_life();
            bonus_life_flash=100;
        }

    /* really increment number of lifes? only in a real game, nowhere else. */
    if (type==TYPE_NORMAL && player_lives<caveset->maximum_lives)
        /* only add a life, if lives is >0.  lives==0 is a test run or a snapshot, no bonus life then. */
        /* also, obey max number of bonus lives. */
        player_lives++;
}

/// Increment score of the player.
/// Store in the game score, in player score, and also in the replay, if any.
/// If a bonus life is got, show bonus life flash.
void GameControl::increment_score(int increment) {
    int i=0;

    if (caveset)
        i=player_score/caveset->bonus_life_score;
    player_score+=increment;
    cave_score+=increment;
    if (replay_record)  /* also record to replay */
        replay_record->score+=increment;
    if (caveset && player_score/caveset->bonus_life_score>i)
        add_bonus_life(true);   /* if score crossed bonus_life_score point boundary, player won a bonus life */
}


/// Load the cave.
/// The role of this function greatly depends on the GameControl type.
/// The cave might be loaded from a caveset, it might be just a copy of a CaveRendered given.
void GameControl::load_cave() {
    guint32 seed;

    /* delete gfx buffer */
    gfx_buffer.remove();
    covered.remove();

    /* load the cave */
    cave_score=0;
    switch (type) {
        case TYPE_NORMAL:
            /* delete previous cave */
            delete played_cave;

            /* specified cave from memory; render the cave with a randomly selected seed */
            original_cave=&caveset->cave(cave_num);
            seed=g_random_int_range(0, GD_CAVE_SEED_MAX);
            played_cave=new CaveRendered(*original_cave, level_num, seed);  /* for playing: seed=random */
            played_cave->setup_for_game();
            if (played_cave->intermission && played_cave->intermission_instantlife)
                add_bonus_life(false);

            /* create replay */
            replay_record=new CaveReplay;
            replay_record->level=played_cave->rendered_on+1;    /* compatibility with bdcff - level=1 is written in file */
            replay_record->seed=played_cave->render_seed;
            replay_record->checksum=gd_cave_adler_checksum(*played_cave);   /* calculate a checksum for this cave */
            replay_record->recorded_with=PACKAGE_STRING;                    // name of gdash and version
            replay_record->player_name=player_name;
            replay_record->date=gd_get_current_date_time();
            break;

        case TYPE_TEST:
            g_assert(original_cave!=NULL);

            /* delete previous try */
            delete played_cave;

            seed=g_random_int_range(0, GD_CAVE_SEED_MAX);
            played_cave=new CaveRendered(*original_cave, level_num, seed);  /* for playing: seed=random */
            played_cave->setup_for_game();
            break;

        case TYPE_SNAPSHOT:
            /* if a snapshot or test is requested, that one... just create a copy */
            /* copy already created in new_game, so nothing to do here. */
            g_assert(played_cave!=NULL);
            g_assert(original_cave==NULL);
            break;

        case TYPE_REPLAY:
            g_assert(replay_from!=NULL);
            g_assert(played_cave==NULL);

            replay_record=NULL;
            replay_from->rewind();
            replay_no_more_movements=0;

            /* -1 is because level=1 is in bdcff for level 1, and internally we number levels from 0 */
            played_cave=new CaveRendered(*original_cave, replay_from->level-1, replay_from->seed);
            played_cave->setup_for_game();
            break;

        case TYPE_CONTINUE_REPLAY:
            g_assert_not_reached();
            break;
    }

    milliseconds_anim=0;
    milliseconds_game=0;        /* set game timer to zero, too */
    state_counter=GAME_INT_SHOW_STORY;
}


/// Gives a snapshot of the currently played cave.
/// @return The snapshot, free with delete.
CaveRendered *GameControl::create_snapshot() const {
    /* make an exact copy */
    return new CaveRendered(*played_cave);
}


/// For games, calculate the next cave number and level number.
/// Called from main_int(), in a normal game, when the cave
/// is successfully finished (or not successfully, but it is an
/// intermission).
/// This does NOT load the new cave, just calculates its number.
void GameControl::select_next_level_indexes() {
    cave_num++;         /* next cave */

    /* if no more caves at this level, back to first one */
    if (cave_num>=caveset->caves.size()) {
        cave_num=0;
        level_num++;        /* but now more difficult */
        if (level_num>4)
            /* if level 5 finished, back to first cave, same difficulty (original game behaviour) */
            level_num=4;
    }

    /* show story. */
    /* if the user fails to solve the cave, the story will not be shown again */
    story_shown=false;
}


/*
 * functions for different states.
 */

/// Show story for a cave, if it has not been already shown.
GameControl::State GameControl::show_story() {
    State return_state;

    /* if we have a story... */
    /* and user settings permit showing that... etc */
    if (gd_show_story && !story_shown && type==TYPE_NORMAL && original_cave->story!="") {
        played_cave->clear_sounds();            /* to stop cover sound from previous cave; there should be no cover sound when the user reads the story */
        gd_sound_play_sounds(played_cave->sound1, played_cave->sound2, played_cave->sound3);
        state_counter=GAME_INT_SHOW_STORY_WAIT;
        return_state=STATE_SHOW_STORY;
        story_shown=true;   /* so the story will not be shown again, for example when the cave is not solved and played again. */
    } else {
        /* if no story */
        state_counter=GAME_INT_STORY_CLICKED;
        return_state=STATE_NOTHING;
    }

    return return_state;
}

/// Start uncover animation. Create gfx buffers.
void GameControl::start_uncover() {
    /* create gfx buffer. fill with "invalid" */
    gfx_buffer.set_size(played_cave->w, played_cave->h, -1);
    /* cover all cells of cave */
    covered.set_size(played_cave->w, played_cave->h, true);

    /* to play cover sound */
    played_cave->clear_sounds();
    played_cave->sound_play(GD_S_COVER, played_cave->player_x, played_cave->player_y);
    gd_sound_play_sounds(played_cave->sound1, played_cave->sound2, played_cave->sound3);

    /* advance to next state */
    state_counter+=1;
}

/// One frame of the uncover animation.
void GameControl::uncover_animation() {
    /* original game uncovered one cell per line each frame.
     * we have different cave sizes, so uncover width*height/40 random cells each frame. (original was width=40).
     * this way the uncovering is the same speed also for intermissions. */
    for (int j=0; j<played_cave->w*played_cave->h/40; j++)
        covered(g_random_int_range(0, played_cave->w), g_random_int_range(0, played_cave->h))=false;
    state_counter+=1;   /* as we did something, advance the counter. */
}


/// Uncover all cells of cave, and switch in to the normal cave running state.
void GameControl::uncover_all() {
    covered.fill(false);

    /* to stop uncover sound. */
    played_cave->clear_sounds();
    gd_sound_play_sounds(played_cave->sound1, played_cave->sound2, played_cave->sound3);

    state_counter=GAME_INT_CAVE_RUNNING;
}


/// One frame of the cover animation.
void GameControl::cover_animation() {
    /* covering eight times faster than uncovering. */
    for (int j=0; j<played_cave->w*played_cave->h*8/40; j++)
        covered(g_random_int_range(0, played_cave->w), g_random_int_range(0, played_cave->h))=true;

    state_counter+=1;   /* as we did something, advance the counter. */
}


/// Iterate the cave.
/// @param millisecs_elapsed The number of milliseconds elapsed since the last call.
/// @param fast_forward If set to true, cave will be iterated at 25fps, regardless of cave speed calculated by the cave.
/// @param player_move The direction of move, by keypresses.
/// @param fire If the user pressed the fire.
/// @param suicide If the user pressed the suicide button.
/// @param restart If the user requested a cave restart.
GameControl::State GameControl::iterate_cave(int millisecs_elapsed, bool fast_forward, GdDirectionEnum player_move, bool fire, bool suicide, bool restart) {
    State return_state;

    int irl_cavespeed;      // caveset in real life. if fast_forward is enabled, we ignore the speed of the cave (but it thinks it runs at normal speed)
    if (!fast_forward)
        irl_cavespeed=played_cave->speed;   /* cave speed in ms, like 175ms/frame */
    else
        irl_cavespeed=40;                   /* if fast forward, ignore cave speed, and go as 25 iterations/sec */

    /* if we are playing a replay, but the user intervents, continue as a snapshot. */
    /* do not trigger this for fire, as it would not be too intuitive. */
    if (type==TYPE_REPLAY)
        if (player_move!=MV_STILL) {
            type=TYPE_CONTINUE_REPLAY;
            replay_from=NULL;
        }

    /* ANYTHING EXCEPT A TIMEOUT, WE ITERATE THE CAVE */
    /* iterate cave */
    return_state=STATE_NOTHING; /* normally nothing happes. but if we iterate, this might change. */
    milliseconds_game+=millisecs_elapsed;

    /* decide if cave will be iterated. */
    if (played_cave->player_state!=GD_PL_TIMEOUT && milliseconds_game>=irl_cavespeed) {
        // ok, cave has to be iterated.

        /* IF PLAYING FROM REPLAY, OVERWRITE KEYPRESS VARIABLES FROM REPLAY */
        if (type==TYPE_REPLAY) {
            /* if the user does touch the keyboard, we immediately exit replay, and he can continue playing */
            bool result=replay_from->get_next_movement(player_move, fire, suicide);
            /* if could not get move from snapshot, continue from keyboard input. */
            if (!result)
                replay_no_more_movements++;
            /* if no more available movements, and the user does not do anything, we cover cave and stop game. */
            if (replay_no_more_movements>15)
                state_counter=GAME_INT_COVER_START;
        }

        milliseconds_game -= irl_cavespeed;
        // if recording the replay, now store the movement
        if (replay_record)
            replay_record->store_movement(player_move, fire, suicide);

        PlayerState player_state_prev = played_cave->player_state;

        // cave iterate gives us a new player move, which might have diagonal movements removed
        player_move=played_cave->iterate(player_move, fire, suicide);

        if (played_cave->score)
            increment_score(played_cave->score);
        return_state = STATE_LABELS_CHANGED;  /* as we iterated, the score and the like could have been changed. */
        if (player_state_prev != GD_PL_TIMEOUT && played_cave->player_state==GD_PL_TIMEOUT)
            return_state = STATE_TIMEOUT_NOW; /* and if the cave timeouted at this exact moment, that is a special case. */

        gd_sound_play_sounds(played_cave->sound1, played_cave->sound2, played_cave->sound3);
    }

    if (played_cave->player_state==GD_PL_EXITED) {
        /* start adding points for remaining time */
        state_counter=GAME_INT_CHECK_BONUS_TIME;
        played_cave->clear_sounds();
        played_cave->sound_play(GD_S_FINISHED, played_cave->player_x, played_cave->player_y); /* play cave finished sound */
        gd_sound_play_sounds(played_cave->sound1, played_cave->sound2, played_cave->sound3);
    }

    /* player died and user presses fire -> try again */
    /* time out and user presses fire -> try again */
    /* user requests cave restart (when stuck under stones) -> try again */
    if (((played_cave->player_state==GD_PL_DIED || played_cave->player_state==GD_PL_TIMEOUT) && fire) || restart) {
        if (type==TYPE_NORMAL && player_lives==0) {
            /* wait some time - this is a game over */
            state_counter=GAME_INT_WAIT_BEFORE_COVER;
        } else {
            /* start cover animation immediately */
            state_counter=GAME_INT_COVER_START;
        }
    }

    return return_state;
}

/// After adding bonus points, we wait some time before starting to cover.
/// This is the FIRST frame... so we check for game over and maybe jump there.
/// If no more lives, game is over.
GameControl::State GameControl::wait_before_cover() {
    State return_state;

    state_counter+=1; /* 40ms elapsed, advance counter */
    if (type==TYPE_NORMAL && player_lives==0)
        return_state=STATE_NO_MORE_LIVES;
    else
        return_state=STATE_NOTHING;

    return return_state;
}

/// Add some bonus scores.
/// This may advance the state, or not - depending on if there is still cave time left.
GameControl::State GameControl::check_bonus_score() {
    State return_state;

    /* player exited, but some time remained. now count bonus points. */
    /* if time remaining, bonus points are added. do not start animation yet. */
    if (played_cave->time>0) {
        // cave time is mult'd by timing factor, which is 1000 or 1200, depending on cave pal setting.
        // we do not really care this here, but have to multiply.
        if (played_cave->time>60*played_cave->timing_factor) {  /* if much time (>60s) remained, fast counter :) */
            played_cave->time-=9*played_cave->timing_factor;    /* decrement by nine each frame, so it also looks like a fast counter. 9 is 8+1! */
            increment_score(played_cave->timevalue*9);
        } else {
            played_cave->time-=played_cave->timing_factor;      /* subtract number of "milliseconds" - nothing to do with gameplay->millisecs! */
            increment_score(played_cave->timevalue);            /* higher levels get more bonus points per second remained */
        }
        // maybe we we substracted too much (remaining time was fraction of a second)
        if (played_cave->time<0)
            played_cave->time=0;
    } else
        /* if no more points, start waiting a bit, and later start covering. */
        state_counter=GAME_INT_WAIT_BEFORE_COVER;

    /* play bonus sound - which is the same as the seconds sound, when only <10 seconds left */
    played_cave->set_seconds_sound();
    gd_sound_play_sounds(played_cave->sound1, played_cave->sound2, played_cave->sound3);
    return_state=STATE_LABELS_CHANGED;

    return return_state;
}

/// After finishing the cave cover animation, decide what to do next.
/// For a normal game, manage replays, and select next level.
/// For testing, do nothing, as the cave will be reloaded.
/// For other types, signal GameControl finish.
GameControl::State GameControl::finished_covering() {
    State return_state;
    gfx_buffer.remove();
    covered.remove();

    /* if this is a normal game: */
    if (type==TYPE_NORMAL) {
        // if the replay was successful, or it has some length which makes sense, add it to the cave.
        if (replay_record->success || replay_record->length()>=16)
            caveset->cave(cave_num).replays.push_back(*replay_record);
        delete replay_record;
        replay_record=0;

        switch (played_cave->player_state) {
            case GD_PL_EXITED:
                // one life extra for completing intermission
                if (played_cave->intermission && played_cave->intermission_rewardlife)
                    add_bonus_life(false);
                // we also have added points for remaining time -> now check for highscore
                caveset->cave(cave_num).highscore.add(player_name, cave_score);
                break;
            case GD_PL_DIED:
            case GD_PL_TIMEOUT:
                if (!played_cave->intermission && player_lives>0)  // normal cave, died -> lives decreased
                    player_lives--;
                break;

            case GD_PL_LIVING:
            case GD_PL_NOT_YET:
                break;
        }
        /* if successful or this was an intermission, advance to next level.
         * for intermissions, there is only one chance given, so always go to next. */
        if (played_cave->player_state == GD_PL_EXITED || played_cave->intermission)
            select_next_level_indexes();

        if (player_lives>0)
            return_state=STATE_NOTHING;
        else
            return_state=STATE_GAME_OVER;
    } else if (type == TYPE_TEST) {
        /* if testing, start again. do nothing, cave will be reloaded. */
        return_state = STATE_NOTHING;
    } else
        /* for snapshots and replays and the like, this is the end. */
        return_state=STATE_STOP;

    /* load next cave on next call. */
    state_counter = GAME_INT_LOAD_CAVE;
    return return_state;
}


/// The main_int function, which controls the whole working of a GameControl.
/// @param millisecs_elapsed The number of milliseconds elapsed since the last call.
/// @param player_move The direction of move, by keypresses.
/// @param fire If the user pressed the fire.
/// @param suicide If the user pressed the suicide button.
/// @param restart If the user requested a cave restart.
/// @param allow_iterate If the game is paused by the user, this prevents the cave from iterating. But the cells are still animated!
/// @param fast_forward If set to true, cave will be iterated at 25fps, regardless of cave speed calculated by the cave.
GameControl::State GameControl::main_int(int millisecs_elapsed, GdDirectionEnum player_move, bool fire, bool suicide, bool restart, bool allow_iterate, bool fast_forward) {
    State return_state;

    milliseconds_anim+=millisecs_elapsed;   /* keep track of time */
    bool is_animation_frame=false;          /* set to true, if this will be an animation frame */
    if (milliseconds_anim>=40) {            /* 40 ms -> 25 fps */
        is_animation_frame=true;
        milliseconds_anim-=40;
        if (bonus_life_flash > 0)   /* bonus life - frames */
            bonus_life_flash--;
        animcycle = (animcycle+1) % 8;
    }


    if (state_counter<GAME_INT_LOAD_CAVE) {
        /* cannot be less than uncover start. */
        g_assert_not_reached();
    } else if (state_counter==GAME_INT_LOAD_CAVE) {
        /* do our tasks associated with loading a new cave. */
        /* the caller will not know about this yet */
        load_cave();
        return_state=STATE_CAVE_LOADED;
    } else if (state_counter==GAME_INT_SHOW_STORY) {
        /* for normal game, every cave can have a long string of description/story. show that. */
        return_state=show_story();
    } else if (state_counter==GAME_INT_SHOW_STORY_WAIT) {
        /* if we are showing the story, we are waiting for the user to press fire. nothing else */
        /* if user presses fire (or maybe esc), proceed with loading the cave. */
        if (fire || restart)
            state_counter=GAME_INT_STORY_CLICKED;
        return_state=STATE_SHOW_STORY_WAIT;
    } else if (state_counter==GAME_INT_STORY_CLICKED) {
        state_counter=GAME_INT_START_UNCOVER;
        return_state=STATE_PREPARE_FIRST_FRAME;
    } else if (state_counter==GAME_INT_START_UNCOVER) {
        /* the very beginning. this will be the first cave frame drawn by the caller. */
        start_uncover();
        /* very important: tell the caller that we loaded a new cave. */
        /* size of the cave might be new, colors might be new, and so on. */
        return_state=STATE_FIRST_FRAME;
    } else if (state_counter<GAME_INT_UNCOVER_ALL) {
        /* uncover animation */
        if (is_animation_frame)
            uncover_animation();
        /* if fast uncover animation in test requested, do 3 more times */
        if (type == TYPE_TEST && gd_fast_uncover_in_test)
            for (unsigned i = 0; i < 3 && state_counter<GAME_INT_UNCOVER_ALL; ++i)
                uncover_animation();
        return_state=STATE_NOTHING;
    } else if (state_counter==GAME_INT_UNCOVER_ALL) {
        /* time to uncover the whole cave. */
        uncover_all();
        return_state=STATE_NOTHING;
    } else if (state_counter==GAME_INT_CAVE_RUNNING) {
        /* normal. */
        if (allow_iterate)
            return_state=iterate_cave(millisecs_elapsed, fast_forward, player_move, fire, suicide, restart);
        else
            return_state=STATE_NOTHING;
    } else if (state_counter==GAME_INT_CHECK_BONUS_TIME) {
        /* before covering, we check for time bonus score */
        if (is_animation_frame) {
            check_bonus_score();
            return_state=STATE_LABELS_CHANGED;
        } else
            return_state=STATE_NOTHING;
    } else if (state_counter==GAME_INT_WAIT_BEFORE_COVER) {
        /* after adding bonus points, we wait some time before starting to cover. this is the FIRST frame... so we check for game over and maybe jump there */
        /* if no more lives, game is over. */
        if (is_animation_frame)
            return_state=wait_before_cover();
        else
            return_state=STATE_NOTHING;
    } else if (state_counter>GAME_INT_WAIT_BEFORE_COVER && state_counter<GAME_INT_COVER_START) {
        /* after adding bonus points, we wait some time before starting to cover. ... and the other frames. */
        /* here we do nothing, but wait */
        if (is_animation_frame)
            state_counter+=1;       /* 40ms elapsed, advance counter */
        return_state=STATE_NOTHING;
    } else
        /* starting to cover. start cover sound. */
        if (state_counter==GAME_INT_COVER_START) {

            played_cave->clear_sounds();
            played_cave->sound_play(GD_S_COVER, played_cave->player_x, played_cave->player_y);
            /* to play cover sound */
            gd_sound_play_sounds(played_cave->sound1, played_cave->sound2, played_cave->sound3);

            state_counter+=1;
            return_state=STATE_NOTHING;
        } else
            /* covering. */
            if (state_counter>GAME_INT_COVER_START && state_counter<GAME_INT_COVER_ALL) {
                if (is_animation_frame)
                    cover_animation();
                return_state=STATE_NOTHING;
            } else if (state_counter==GAME_INT_COVER_ALL) {
                /* cover all */
                covered.fill(true);

                state_counter+=1;
                return_state=STATE_NOTHING;
            } else {
                return_state=finished_covering();
            }

    return return_state;
}
