/*
 * Copyright (c) 2007, 2008, 2009, Czirkos Zoltan <cirix@fw.hu>
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
#include "cave.h"
#include "caveobject.h"
#include "cavesound.h"
#include "caveset.h"
#include "caveengine.h"
#include "settings.h"
#include "gameplay.h"
#include "util.h"
#include "sound.h"
#include "config.h"

#define GAME_INT_INVALID -100
/* prepare cave, gfx buffer */
#define GAME_INT_LOAD_CAVE -73
/* show description/note of cave. */
#define GAME_INT_SHOW_STORY -72
/* waiting fire button after showing the story. */
#define GAME_INT_SHOW_STORY_WAIT -71
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

gboolean gd_wait_before_game_over;        /* wait some time before covering the cave, if there is a game over. main() should set it true for sdl, false for gtk+ */




static void
cave_finished_highscore(GdGame *game)
{
    /* enter to highscore table */
    gd_add_highscore(game->original_cave->highscore, game->player_name, game->cave_score);
}




void
gd_game_free(GdGame *game)
{
    /* stop sounds */
    gd_sound_off();

    if (game->gfx_buffer)
        gd_cave_map_free(game->gfx_buffer);

    game->player_lives=0;
    if (game->cave)
        gd_cave_free(game->cave);

    /* if we recorded some replays during this run, we check them. we remove those which are too short */
    if (game->replays_recorded) {
        GList *citer;

        /* check all caves */
        for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
            GdCave *cave=(GdCave *)citer->data;
            GList *riter;

            /* check replays of all caves */
            for (riter=cave->replays; riter!=NULL; ) {
                GdReplay *replay=(GdReplay *)riter->data;
                GList *nextrep=riter->next;    /* remember next iter, as we may delete the current */

                /* if we recorded this replay now, and it is too short, we delete it */
                /* but do not delete successful ones! */
                if (g_list_find(game->replays_recorded, replay) && (replay->movements->len<16) && (!replay->success)) {
                    cave->replays=g_list_delete_link(cave->replays, riter);    /* delete from list */
                    gd_replay_free(replay);    /* also free replay */
                }
                riter=nextrep;
            }
        }

        /* free the list of newly recorded replays, as we checked them */
        g_list_free(game->replays_recorded);
        game->replays_recorded=NULL;
    }

    g_free(game);
}

/* add bonus life. if sound enabled, play sound, too. */
static void
add_bonus_life(GdGame *game, gboolean inform_user)
{
    /* only inform about bonus life when playing a game */
    /* or when testing the cave (so the user can see that a bonus life can be earned in that cave */
    if (game->type==GD_GAMETYPE_NORMAL || game->type==GD_GAMETYPE_TEST)
        if (inform_user) {
            gd_sound_play_bonus_life();
            game->bonus_life_flash=100;
        }

    /* really increment number of lifes? only in a real game, nowhere else. */
    if (game->player_lives && game->player_lives<gd_caveset_data->maximum_lives)
        /* only add a life, if lives is >0.  lives==0 is a test run or a snapshot, no bonus life then. */
        /* also, obey max number of bonus lives. */
        game->player_lives++;
}

/* increment score of player.
    flash screen if bonus life
*/
static void
increment_score(GdGame *game, int increment)
{
    int i;

    i=game->player_score/gd_caveset_data->bonus_life_score;
    game->player_score+=increment;
    game->cave_score+=increment;
    if (game->replay_record)    /* also record to replay */
        game->replay_record->score+=increment;
    if (game->player_score/gd_caveset_data->bonus_life_score>i)
        add_bonus_life(game, TRUE);    /* if score crossed bonus_life_score point boundary, player won a bonus life */
}

/* do the things associated with loading a new cave. function creates gfx buffer and the like. */
static void
load_cave(GdGame *game)
{
    guint32 seed;

    /* delete gfx buffer */
    if (game->gfx_buffer)
        gd_cave_map_free(game->gfx_buffer);
    game->gfx_buffer=NULL;

    /* load the cave */
    game->cave_score=0;
    switch (game->type) {
        case GD_GAMETYPE_NORMAL:
            /* delete previous cave */
            gd_cave_free(game->cave);
            game->cave=NULL;

            /* specified cave from memory; render the cave with a randomly selected seed */
            game->original_cave=gd_return_nth_cave(game->cave_num);
            seed=g_random_int_range(0, GD_CAVE_SEED_MAX);
            game->cave=gd_cave_new_rendered(game->original_cave, game->level_num, seed);    /* for playing: seed=random */
            gd_cave_setup_for_game(game->cave);
            if (gd_random_colors)    /* new cave-recolor if requested. gameplay->cave is a copy only, so no worries! */
                gd_cave_set_random_colors(game->cave, gd_preferred_palette);
            if (game->cave->intermission && game->cave->intermission_instantlife)
                add_bonus_life(game, FALSE);

            /* create replay */
            game->replay_record=gd_replay_new();
            game->replay_record->level=game->cave->rendered-1;    /* rendered=0 means not rendered here. 1=level 1 */
            game->replay_record->seed=game->cave->render_seed;
            game->replay_record->checksum=gd_cave_adler_checksum(game->cave);    /* calculate a checksum for this cave */
            gd_strcpy(game->replay_record->recorded_with, PACKAGE_STRING);
            gd_strcpy(game->replay_record->player_name, game->player_name);
            gd_strcpy(game->replay_record->date, gd_get_current_date());
            game->original_cave->replays=g_list_append(game->original_cave->replays, game->replay_record);
            /* also store the pointer in a list of freshly recorded replays. so we can later check them if they are too short and delete */
            game->replays_recorded=g_list_append(game->replays_recorded, game->replay_record);
            break;

        case GD_GAMETYPE_TEST:
            g_assert(game->cave==NULL);
            g_assert(game->original_cave!=NULL);

            seed=g_random_int_range(0, GD_CAVE_SEED_MAX);
            game->cave=gd_cave_new_rendered(game->original_cave, game->level_num, seed);    /* for playing: seed=random */
            gd_cave_setup_for_game(game->cave);
            break;

        case GD_GAMETYPE_SNAPSHOT:
            /* if a snapshot or test is requested, that one... just create a copy */
            /* copy already created in new_game, so nothing to do here. */
            g_assert(game->cave!=NULL);
            g_assert(game->original_cave==NULL);
            break;

        case GD_GAMETYPE_REPLAY:
            g_assert(game->replay_from!=NULL);
            g_assert(game->cave==NULL);

            game->replay_record=NULL;
            gd_replay_rewind(game->replay_from);
            game->replay_no_more_movements=0;

            game->cave=gd_cave_new_rendered(game->original_cave, game->replay_from->level, game->replay_from->seed);
            gd_cave_setup_for_game(game->cave);
            break;

        case GD_GAMETYPE_CONTINUE_REPLAY:
            g_assert_not_reached();
            break;
    }

    game->milliseconds_anim=0;
    game->milliseconds_game=0;        /* set game timer to zero, too */
}


GdCave *
gd_create_snapshot(GdGame *game)
{
    GdCave *snapshot;
    g_return_val_if_fail (game->cave != NULL, NULL);

    /* make an exact copy */
    snapshot=gd_cave_new_from_cave(game->cave);
    return snapshot;
}

/* this starts a new game */
GdGame *
gd_game_new(const char *player_name, const int cave, const int level)
{
    GdGame *game;

    game=g_new0(GdGame, 1);

    gd_strcpy(game->player_name, player_name);
    game->cave_num=cave;
    game->level_num=level;

    game->player_lives=gd_caveset_data->initial_lives;
    game->player_score=0;

    game->type=GD_GAMETYPE_NORMAL;
    game->state_counter=GAME_INT_LOAD_CAVE;

    game->show_story=TRUE;

    return game;
}

/* starts a new snapshot playing */
GdGame *
gd_game_new_snapshot(GdCave *snapshot)
{
    GdGame *game;

    game=g_new0(GdGame, 1);

    gd_strcpy(game->player_name, "");
    game->player_lives=0;
    game->player_score=0;

    g_assert(snapshot->rendered!=0);    /* we accept only rendered caves, trivially */
    game->cave=gd_cave_new_from_cave(snapshot);

    game->type=GD_GAMETYPE_SNAPSHOT;
    game->state_counter=GAME_INT_LOAD_CAVE;

    return game;
}

/* starts a new snapshot playing */
GdGame *
gd_game_new_test(GdCave *cave, int level)
{
    GdGame *game;

    game=g_new0(GdGame, 1);

    gd_strcpy(game->player_name, "");
    game->player_lives=0;
    game->player_score=0;

    game->original_cave=cave;
    game->level_num=level;

    game->type=GD_GAMETYPE_TEST;
    game->state_counter=GAME_INT_LOAD_CAVE;

    return game;
}

/* starts a new snapshot playing */
GdGame *
gd_game_new_replay(GdCave *cave, GdReplay *replay)
{
    GdGame *game;

    game=g_new0(GdGame, 1);
    gd_strcpy(game->player_name, "");
    game->player_lives=0;
    game->player_score=0;

    game->original_cave=cave;
    game->replay_from=replay;

    game->type=GD_GAMETYPE_REPLAY;
    game->state_counter=GAME_INT_LOAD_CAVE;

    return game;
}



/* called from iterate_cave, to increment cave and level number.
   does not load the cave, just sets the new numbers!
 */
static void
next_level(GdGame *game)
{
    game->cave_num++;            /* next cave */

    /* if no more caves at this level, back to first one */
    if (game->cave_num>=gd_caveset_count()) {
        game->cave_num=0;
        game->level_num++;        /* but now more difficult */
        if (game->level_num>4)
            /* if level 5 finished, back to first cave, same difficulty (original game behaviour) */
            game->level_num=4;
    }

    /* show story. */
    /* if the user fails to solve the cave, the story will not be shown again */
    game->show_story=TRUE;
}





static void
iterate_cave(GdGame *game, GdDirection player_move, gboolean fire, gboolean suicide, gboolean restart)
{
    /* if we are playing a replay, but the user intervents, continue as a snapshot. */
    /* do not trigger this for fire, as it would not be too intuitive. */
    if (game->type==GD_GAMETYPE_REPLAY)
        if (player_move!=MV_STILL) {
            game->type=GD_GAMETYPE_CONTINUE_REPLAY;
            game->replay_from=NULL;
        }

    /* ANYTHING EXCEPT A TIMEOUT, WE ITERATE THE CAVE */
    if (game->cave->player_state!=GD_PL_TIMEOUT) {
        /* IF PLAYING FROM REPLAY, OVERWRITE KEYPRESS VARIABLES FROM REPLAY */
        if (game->type==GD_GAMETYPE_REPLAY) {
            gboolean result;

            /* if the user does touch the keyboard, we immediately exit replay, and he can continue playing */
            result=gd_replay_get_next_movement(game->replay_from, &player_move, &fire, &suicide);
            /* if could not get move from snapshot, continue from keyboard input. */
            if (!result)
                game->replay_no_more_movements++;
            /* if no more available movements, and the user does not do anything, we cover cave and stop game. */
            if (game->replay_no_more_movements>15)
                game->state_counter=GAME_INT_COVER_START;
        }

        /* iterate cave */
        gd_cave_iterate(game->cave, player_move, fire, suicide);
        if (game->replay_record)
            gd_replay_store_movement(game->replay_record, player_move, fire, suicide);
        if (game->cave->score)
            increment_score(game, game->cave->score);

        gd_sound_play_cave(game->cave);
    }

    if (game->cave->player_state==GD_PL_EXITED) {
        if (game->cave->intermission && game->cave->intermission_rewardlife && game->player_lives!=0)
            /* one life extra for completing intermission */
            add_bonus_life(game, FALSE);
        if (game->replay_record)
            game->replay_record->success=TRUE;

        /* start adding points for remaining time */
        game->state_counter=GAME_INT_CHECK_BONUS_TIME;
        gd_cave_clear_sounds(game->cave);
        gd_sound_play(game->cave, GD_S_FINISHED); /* play cave finished sound */
        gd_sound_play_cave(game->cave);
        next_level(game);
    }

    if (((game->cave->player_state==GD_PL_DIED || game->cave->player_state==GD_PL_TIMEOUT) && fire) || restart) {
        /* player died, and user presses fire -> try again */
        /* time out -> try again */
        if (!game->cave->intermission && game->player_lives>0)
            /* no minus life for intermissions */
            game->player_lives--;
        if (game->cave->intermission)
            /* only one chance for intermissions */
            next_level(game);

        if (game->player_lives==0 && game->type==GD_GAMETYPE_NORMAL) {
            /* wait some time - this is a game over */
            if (gd_wait_before_game_over)
                game->state_counter=GAME_INT_WAIT_BEFORE_COVER;
            else
                game->state_counter=GAME_INT_COVER_START;    /* jump immediately to covering */
        }
        else {
            /* start cover animation immediately */
            game->state_counter=GAME_INT_COVER_START;
        }
    }
}

GdGameState
gd_game_main_int(GdGame *game, int millisecs_elapsed, GdDirection player_move, gboolean fire, gboolean suicide, gboolean restart, gboolean allow_iterate, gboolean yellowish_draw, gboolean fast_forward)
{
    gboolean frame;    /* set to true, if this will be an animation frame */
    int x, y;
    GdGameState return_state;
    int counter_next;

    counter_next=GAME_INT_INVALID;
    return_state=GD_GAME_INVALID_STATE;
    game->milliseconds_anim+=millisecs_elapsed;    /* keep track of time */
    frame=FALSE;    /* set to true, if this will be an animation frame */
    if (game->milliseconds_anim>=40) {
        frame=TRUE;
        game->milliseconds_anim-=40;
    }

    /* cannot be less than uncover start. */
    if (game->state_counter<GAME_INT_LOAD_CAVE) {
        g_assert_not_reached();
    }
    else
    if (game->state_counter==GAME_INT_LOAD_CAVE) {
        /* do the things associated with loading a new cave. function creates gfx buffer and the like. */
        load_cave(game);
        return_state=GD_GAME_NOTHING;
        counter_next=GAME_INT_SHOW_STORY;
    }
    else
    /* for normal game, every cave can have a long string of description/story. show that. */
    if (game->state_counter==GAME_INT_SHOW_STORY) {
        /* if we have a story... */
        /* and user settings permit showing that... etc */
        if (gd_show_story && game->show_story && game->original_cave && game->original_cave->story->len!=0) {
            gd_cave_clear_sounds(game->cave);            /* to stop cover sound from previous cave; there should be no cover sound when the user reads the story */
            gd_sound_play_cave(game->cave);
            counter_next=GAME_INT_SHOW_STORY_WAIT;
            return_state=GD_GAME_SHOW_STORY;
            game->show_story=FALSE;    /* so the story will not be shown again, for example when the cave is not solved and played again. */
        } else {
            /* if no story */
            counter_next=GAME_INT_START_UNCOVER;
            return_state=GD_GAME_NOTHING;
        }
    }
    else
    if (game->state_counter==GAME_INT_SHOW_STORY_WAIT) {
        /* if user presses fire, proceed with loading the cave. */
        if (fire) {
            counter_next=GAME_INT_START_UNCOVER;
            return_state=GD_GAME_NOTHING;
        } else {
            /* do nothing. */
            counter_next=game->state_counter;
            return_state=GD_GAME_SHOW_STORY_WAIT;
        }
    }
    else
    /* the very beginning. */
    if (game->state_counter==GAME_INT_START_UNCOVER) {
        /* create gfx buffer. */
        game->gfx_buffer=gd_cave_map_new(game->cave, int);
        for (y=0; y<game->cave->h; y++)
            for (x=0; x<game->cave->w; x++)
                game->gfx_buffer[y][x]=-1;    /* fill with "invalid" */

        /* cover all cells of cave */
        for (y=0; y<game->cave->h; y++)
            for (x=0; x<game->cave->w; x++)
                game->cave->map[y][x] |= COVERED;

        /* to play cover sound */
        gd_cave_clear_sounds(game->cave);
        gd_sound_play(game->cave, GD_S_COVER);
        gd_sound_play_cave(game->cave);

        counter_next=game->state_counter+1;
        /* very important: tell the caller that we loaded a new cave. */
        /* size of the cave might be new, colors might be new, and so on. */
        return_state=GD_GAME_CAVE_LOADED;
    }
    else
    /* uncover animation */
    if (game->state_counter<GAME_INT_UNCOVER_ALL) {
        counter_next=game->state_counter;
        if (frame) {
            int j;

            /* original game uncovered one cell per line each frame.
             * we have different cave sizes, so uncover width*height/40 random cells each frame. (original was width=40).
             * this way the uncovering is the same speed also for intermissions. */
            for (j=0; j<game->cave->w*game->cave->h/40; j++)
                game->cave->map[g_random_int_range(0, game->cave->h)][g_random_int_range(0, game->cave->w)] &= ~COVERED;

            counter_next++;    /* as we did something, advance the counter. */
        }
        return_state=GD_GAME_NOTHING;
    }
    else
    /* time to uncover the whole cave. */
    if (game->state_counter==GAME_INT_UNCOVER_ALL) {
        for (y=0; y<game->cave->h; y++)
            for (x=0; x<game->cave->w; x++)
                game->cave->map[y][x] &= ~COVERED;

        /* to stop uncover sound. */
        gd_cave_clear_sounds(game->cave);
        gd_sound_play_cave(game->cave);

        counter_next=GAME_INT_CAVE_RUNNING;
        return_state=GD_GAME_NOTHING;
    }
    else
    /* normal. */
    if (game->state_counter==GAME_INT_CAVE_RUNNING) {
        int cavespeed;

        if (!fast_forward)
            cavespeed=game->cave->speed;    /* cave speed in ms, like 175ms/frame */
        else
            cavespeed=40;    /* if fast forward, ignore cave speed, and go as 25 iterations/sec */

        /* ITERATION - cave is running. */
        return_state=GD_GAME_NOTHING;    /* normally nothing happes. but if we iterate, this might change. */
        if (allow_iterate)    /* if allowing cave movements, add elapsed time to timer. and then we can check what to do. */
            game->milliseconds_game+=millisecs_elapsed;
        if (game->milliseconds_game>=cavespeed) {
            GdPlayerState pl;

            game->milliseconds_game-=cavespeed;
            pl=game->cave->player_state;
            iterate_cave(game, player_move, fire, suicide, restart);
            return_state=GD_GAME_LABELS_CHANGED;    /* as we iterated, the score and the like could have been changed. */
            if (pl!=GD_PL_TIMEOUT && game->cave->player_state==GD_PL_TIMEOUT)
                return_state=GD_GAME_TIMEOUT_NOW;    /* and if the cave timeouted at this moment, that is a special case. */
        }

        counter_next=game->state_counter;    /* do not change counter state, as it is set by iterate_cave() */
    }
    /* before covering, we check for time bonus score */
    else
    if (game->state_counter==GAME_INT_CHECK_BONUS_TIME) {
        if (frame) {
            /* if time remaining, bonus points are added. do not start animation yet. */
            if (game->cave->time>0) {
                game->cave->time-=game->cave->timing_factor;        /* subtract number of "milliseconds" - nothing to do with gameplay->millisecs! */
                increment_score(game, game->cave->timevalue);            /* higher levels get more bonus points per second remained */
                if (game->cave->time>60*game->cave->timing_factor) {    /* if much time (>60s) remained, fast counter :) */
                    game->cave->time-=8*game->cave->timing_factor;    /* decrement by nine each frame, so it also looks like a fast counter. 9 is 8+1! */
                    increment_score(game, game->cave->timevalue*8);
                }

                /* just to be neat */
                if (game->cave->time<0)
                    game->cave->time=0;
                counter_next=game->state_counter;    /* do not change yet */
            }
            else
            /* if no more points, start waiting a bit, and later start covering. */
                counter_next=GAME_INT_WAIT_BEFORE_COVER;

            /* play bonus sound */
            gd_cave_set_seconds_sound(game->cave);
            gd_sound_play_cave(game->cave);
            return_state=GD_GAME_LABELS_CHANGED;
        }
        else {
            return_state=GD_GAME_NOTHING;
            counter_next=game->state_counter;    /* do not change counter state, as it is set by iterate_cave() */
        }
    }
    else
    /* after adding bonus points, we wait some time before starting to cover. this is the FIRST frame... so we check for game over and maybe jump there */
    /* if no more lives, game is over. */
    if (game->state_counter==GAME_INT_WAIT_BEFORE_COVER) {
        counter_next=game->state_counter;
        if (frame)
            counter_next++;    /* 40ms elapsed, advance counter */
        if (game->type==GD_GAMETYPE_NORMAL && game->player_lives==0)
            return_state=GD_GAME_NO_MORE_LIVES;
        else
            return_state=GD_GAME_NOTHING;
    }
    else
    /* after adding bonus points, we wait some time before starting to cover. ... and the other frames. */
    if (game->state_counter>GAME_INT_WAIT_BEFORE_COVER && game->state_counter<GAME_INT_COVER_START) {
        counter_next=game->state_counter;
        if (frame)
            counter_next++;    /* 40ms elapsed, advance counter */
        return_state=GD_GAME_NOTHING;
    }
    else
    /* starting to cover. start cover sound. */
    if (game->state_counter==GAME_INT_COVER_START) {

        gd_cave_clear_sounds(game->cave);
        gd_sound_play(game->cave, GD_S_COVER);
        /* to play cover sound */
        gd_sound_play_cave(game->cave);

        counter_next=game->state_counter+1;
        return_state=GD_GAME_NOTHING;
    }
    else
    /* covering. */
    if (game->state_counter>GAME_INT_COVER_START && game->state_counter<GAME_INT_COVER_ALL) {
        counter_next=game->state_counter;
        if (frame) {
            int j;

            counter_next++;    /* 40ms elapsed, doing cover: advance counter */
            /* covering eight times faster than uncovering. */
            for (j=0; j < game->cave->w*game->cave->h*8/40; j++)
                game->cave->map[g_random_int_range (0, game->cave->h)][g_random_int_range (0, game->cave->w)] |= COVERED;
        }

        return_state=GD_GAME_NOTHING;
    }
    else
    if (game->state_counter==GAME_INT_COVER_ALL) {
        /* cover all */
        for (y=0; y<game->cave->h; y++)
            for (x=0; x<game->cave->w; x++)
                game->cave->map[y][x] |= COVERED;

        counter_next=game->state_counter+1;
        return_state=GD_GAME_NOTHING;
    }
    else {
        /* cover all + 1 */

        /* if this is a normal game: */
        if (game->type==GD_GAMETYPE_NORMAL) {
            if (game->cave->player_state==GD_PL_EXITED)
                cave_finished_highscore(game);    /* we also have added points for remaining time -> now check for highscore */
            if (game->player_lives!=0)
                return_state=GD_GAME_NOTHING;    /* and go to next level */
            else
                return_state=GD_GAME_GAME_OVER;
        }
        else
            /* for snapshots and replays and the like, this is the end. */
            return_state=GD_GAME_STOP;

        /* load next cave on next call. */
        counter_next=GAME_INT_LOAD_CAVE;
    }
    g_assert(counter_next!=GAME_INT_INVALID);
    g_assert(return_state!=GD_GAME_INVALID_STATE);

    /* draw the cave */
    if (frame) {
        if (game->bonus_life_flash)    /* bonus life - frames */
            game->bonus_life_flash--;
        game->animcycle=(game->animcycle+1)%8;
    }

    /* always render the cave to the gfx buffer; however it may do nothing if animcycle was not changed. */
    if (game->gfx_buffer)
        gd_drawcave_game(game->cave, game->gfx_buffer, game->bonus_life_flash!=0, yellowish_draw, game->animcycle, gd_no_invisible_outbox);

    game->state_counter=counter_next;
    return return_state;
}

