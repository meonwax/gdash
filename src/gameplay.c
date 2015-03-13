/*
 * Copyright (c) 2007, 2008 Czirkos Zoltan <cirix@fw.hu>
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

#define GAME_INT_INVALID -100
/* start uncovering */
#define GAME_INT_LOAD_CAVE -70
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

GdGameplay gd_gameplay;

gboolean gd_wait_before_game_over;		/* wait some time before covering the cave, if there is a game over. main() should set it true for sdl, false for gtk+ */




static void
cave_finished_highscore()
{
	/* enter to highscore table */
	gd_add_highscore(gd_gameplay.original_cave->highscore, gd_gameplay.player_name, gd_gameplay.cave_score);
}




void
gd_stop_game()
{
	/* stop sounds */
	gd_no_sound();

	if (gd_gameplay.gfx_buffer)
		gd_cave_map_free(gd_gameplay.gfx_buffer);
	gd_gameplay.gfx_buffer=NULL;

	gd_gameplay.player_lives=0;
	if (gd_gameplay.cave)
		gd_cave_free(gd_gameplay.cave);
	gd_gameplay.cave=NULL;
	gd_gameplay.original_cave=NULL;
	
	/* if we recorded some replays during this run, we check them. we remove those which are too short */
	if (gd_gameplay.replays_recorded) {
		GList *citer;
		
		/* check all caves */
		for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
			Cave *cave=(Cave *)citer->data;
			GList *riter;
			
			/* check replays of all caves */
			for (riter=cave->replays; riter!=NULL; ) {
				GdReplay *replay=(GdReplay *)riter->data;
				GList *nextrep=riter->next;	/* remember next iter, as we may delete the current */
				
				/* if we recorded this replay now, and it is too short, we delete it */
				/* but do not delete successful ones! */
				if (g_list_find(gd_gameplay.replays_recorded, replay) && (replay->movements->len<16) && (!replay->success)) {
					cave->replays=g_list_delete_link(cave->replays, riter);	/* delete from list */
					gd_replay_free(replay);	/* also free replay */
				}
				riter=nextrep;
			}
		}
		
		/* free the list of newly recorded replays, as we checked them */
		g_list_free(gd_gameplay.replays_recorded);
		gd_gameplay.replays_recorded=NULL;
	}
	
}

/* add bonus life. if sound enabled, play sound, too. */
static void
add_bonus_life(gboolean sound)
{
	if (sound)
		gd_play_bonus_life_sound();
	if (gd_gameplay.player_lives && gd_gameplay.player_lives<gd_caveset_data->maximum_lives)
		/* only add a life, if lives is >0.  lives==0 is a test run or a snapshot, no bonus life then. */
		/* also, obey max number of bonus lives. */
		gd_gameplay.player_lives++;
	gd_gameplay.bonus_life_flash=100;
}

/* increment score of player.
	flash screen if bonus life
*/
static void
increment_score(int increment)
{
	int i;

	i=gd_gameplay.player_score/gd_caveset_data->bonus_life_score;
	gd_gameplay.player_score+=increment;
	gd_gameplay.cave_score+=increment;
	if (gd_gameplay.replay_record)	/* also record to replay */
		gd_gameplay.replay_record->score+=increment;
	if (gd_gameplay.player_score/gd_caveset_data->bonus_life_score>i)
		add_bonus_life(TRUE);	/* if score crossed bonus_life_score point boundary, player won a bonus life */
}

/* do the things associated with loading a new cave. function creates gfx buffer and the like. */
static void
load_cave()
{
	int x, y;
	guint32 seed;

	/* delete gfx buffer */
	if (gd_gameplay.gfx_buffer)
		gd_cave_map_free(gd_gameplay.gfx_buffer);
	gd_gameplay.gfx_buffer=NULL;

	/* load the cave */
	gd_gameplay.cave_score=0;
	switch (gd_gameplay.type) {
		case GD_GAMETYPE_NORMAL:
			/* delete previous cave */
			gd_cave_free(gd_gameplay.cave);
			gd_gameplay.cave=NULL;

			/* specified cave from memory; render the cave with a randomly selected seed */
			gd_gameplay.original_cave=gd_return_nth_cave(gd_gameplay.cave_num);
			seed=g_random_int_range(0, GD_CAVE_SEED_MAX);
			gd_gameplay.cave=gd_cave_new_rendered(gd_gameplay.original_cave, gd_gameplay.level_num, seed);	/* for playing: seed=random */
			gd_cave_setup_for_game(gd_gameplay.cave);
			if (gd_random_colors)	/* new cave-recolor if requested. gd_gameplay.cave is a copy only, so no worries! */
				gd_cave_set_random_colors(gd_gameplay.cave, gd_preferred_palette);
			if (gd_gameplay.cave->intermission && gd_gameplay.cave->intermission_instantlife)
				add_bonus_life(FALSE);

			/* create replay */
			gd_gameplay.replay_record=gd_replay_new();
			gd_gameplay.replay_record->level=gd_gameplay.cave->rendered-1;	/* rendered=0 means not rendered here. 1=level 1 */
			gd_gameplay.replay_record->seed=gd_gameplay.cave->render_seed;
			gd_strcpy(gd_gameplay.replay_record->player_name, gd_gameplay.player_name);
			gd_strcpy(gd_gameplay.replay_record->date, gd_get_current_date());
			gd_gameplay.original_cave->replays=g_list_append(gd_gameplay.original_cave->replays, gd_gameplay.replay_record);
			/* also store the pointer in a list of freshly recorded replays. so we can later check them if they are too short and delete */
			gd_gameplay.replays_recorded=g_list_append(gd_gameplay.replays_recorded, gd_gameplay.replay_record);
			break;

		case GD_GAMETYPE_TEST:
			g_assert(gd_gameplay.cave==NULL);
			g_assert(gd_gameplay.original_cave!=NULL);

			seed=g_random_int_range(0, GD_CAVE_SEED_MAX);
			gd_gameplay.cave=gd_cave_new_rendered(gd_gameplay.original_cave, gd_gameplay.level_num, seed);	/* for playing: seed=random */
			gd_cave_setup_for_game(gd_gameplay.cave);
			break;

		case GD_GAMETYPE_SNAPSHOT:
			/* if a snapshot or test is requested, that one... just create a copy */
			/* copy already created in new_game, so nothing to do here. */
			g_assert(gd_gameplay.cave!=NULL);
			g_assert(gd_gameplay.original_cave==NULL);
			break;

		case GD_GAMETYPE_REPLAY:
			g_assert(gd_gameplay.replay_from!=NULL);
			g_assert(gd_gameplay.cave==NULL);
	
			gd_gameplay.replay_record=NULL;		
			gd_replay_rewind(gd_gameplay.replay_from);
			gd_gameplay.replay_no_more_movements=0;

			gd_gameplay.cave=gd_cave_new_rendered(gd_gameplay.original_cave, gd_gameplay.replay_from->level, gd_gameplay.replay_from->seed);
			gd_cave_setup_for_game(gd_gameplay.cave);
			break;
		
		case GD_GAMETYPE_CONTINUE_REPLAY:
			g_assert_not_reached();
			break;
	}

	gd_gameplay.gfx_buffer=gd_cave_map_new(gd_gameplay.cave, int);
	for (y=0; y<gd_gameplay.cave->h; y++)
		for (x=0; x<gd_gameplay.cave->w; x++)
			gd_gameplay.gfx_buffer[y][x]=-1;	/* fill with "invalid" */

	gd_gameplay.state_counter=GAME_INT_LOAD_CAVE;
	gd_gameplay.milliseconds_anim=0;
	gd_gameplay.milliseconds_game=0;		/* set game timer to zero, too */
}


Cave *
gd_create_snapshot()
{
	Cave *snapshot;
	g_return_val_if_fail (gd_gameplay.cave != NULL, NULL);

	/* make an exact copy */
	snapshot=gd_cave_new_from_cave(gd_gameplay.cave);
	return snapshot;
}

/* this starts a new game */
void
gd_new_game(const char *player_name, const int cave, const int level)
{
	gd_strcpy(gd_gameplay.player_name, player_name);
	gd_gameplay.cave_num=cave;
	gd_gameplay.level_num=level;

	gd_gameplay.player_lives=gd_caveset_data->initial_lives;
	gd_gameplay.player_score=0;
	gd_gameplay.replay_from=NULL;
	gd_gameplay.replay_record=NULL;

	gd_gameplay.type=GD_GAMETYPE_NORMAL;
	gd_gameplay.state_counter=GAME_INT_LOAD_CAVE;
}

/* starts a new snapshot playing */
void
gd_new_game_snapshot(Cave *snapshot)
{
	gd_strcpy(gd_gameplay.player_name, "");
	gd_gameplay.player_lives=0;
	gd_gameplay.player_score=0;

	g_assert(snapshot->rendered!=0);	/* we accept only rendered caves, trivially */
	gd_gameplay.original_cave=NULL;		/* we do not use this for snapshots */
	gd_gameplay.cave=gd_cave_new_from_cave(snapshot);
	gd_gameplay.replay_from=NULL;
	gd_gameplay.replay_record=NULL;

	gd_gameplay.type=GD_GAMETYPE_SNAPSHOT;
	gd_gameplay.state_counter=GAME_INT_LOAD_CAVE;
}

/* starts a new snapshot playing */
void
gd_new_game_test(Cave *cave, int level)
{
	gd_strcpy(gd_gameplay.player_name, "");
	gd_gameplay.player_lives=0;
	gd_gameplay.player_score=0;

	gd_gameplay.cave=NULL;
	gd_gameplay.original_cave=cave;
	gd_gameplay.level_num=level;
	gd_gameplay.replay_from=NULL;
	gd_gameplay.replay_record=NULL;

	gd_gameplay.type=GD_GAMETYPE_TEST;
	gd_gameplay.state_counter=GAME_INT_LOAD_CAVE;
}

/* starts a new snapshot playing */
void
gd_new_game_replay(Cave *cave, GdReplay *replay)
{
	gd_strcpy(gd_gameplay.player_name, "");
	gd_gameplay.player_lives=0;
	gd_gameplay.player_score=0;

	gd_gameplay.cave=NULL;		/* we do not use this for replay */
	gd_gameplay.original_cave=cave;
	gd_gameplay.replay_from=replay;
	gd_gameplay.replay_record=NULL;

	gd_gameplay.type=GD_GAMETYPE_REPLAY;
	gd_gameplay.state_counter=GAME_INT_LOAD_CAVE;
}



/* called from iterate_cave, to increment cave and level number.
   does not load the cave, just sets the new numbers!
 */
static void
next_level()
{
	gd_gameplay.cave_num++;			/* next cave */

	/* if no more caves at this level, back to first one */
	if (gd_gameplay.cave_num>=gd_caveset_count()) {
		gd_gameplay.cave_num=0;
		gd_gameplay.level_num++;		/* but now more difficult */
		if (gd_gameplay.level_num>4)
			/* if level 5 finished, back to first cave, same difficulty (original game behaviour) */
			gd_gameplay.level_num=4;
	}
}





void
iterate_cave(GdDirection player_move, gboolean fire, gboolean suicide, gboolean restart)
{
	/* if we are playing a replay, but the user intervents, continue as a snapshot. */
	/* do not trigger this for fire, as it would not be too intuitive. */
	if (gd_gameplay.type==GD_GAMETYPE_REPLAY)
		if (player_move!=MV_STILL) {
			gd_gameplay.type=GD_GAMETYPE_CONTINUE_REPLAY;
			gd_gameplay.replay_from=NULL;
		}

	/* ANYTHING EXCEPT A TIMEOUT, WE ITERATE THE CAVE */
	if (gd_gameplay.cave->player_state!=GD_PL_TIMEOUT) {
		/* IF PLAYING FROM REPLAY, OVERWRITE KEYPRESS VARIABLES FROM REPLAY */
		if (gd_gameplay.type==GD_GAMETYPE_REPLAY) {
			gboolean result;
			
			/* if the user does touch the keyboard, we immediately exit replay, and he can continue playing */
			result=gd_replay_get_next_movement(gd_gameplay.replay_from, &player_move, &fire, &suicide);
			/* if could not get move from snapshot, continue from keyboard input. */
			if (!result)
				gd_gameplay.replay_no_more_movements++;
			/* if no more available movements, and the user does not do anything, we cover cave and stop game. */			
			if (gd_gameplay.replay_no_more_movements>15)
				gd_gameplay.state_counter=GAME_INT_COVER_START;
		}

		/* iterate cave */
		gd_cave_iterate(gd_gameplay.cave, player_move, fire, suicide);
		if (gd_gameplay.replay_record)
			gd_replay_store_next(gd_gameplay.replay_record, player_move, fire, suicide);
		if (gd_gameplay.cave->score)
			increment_score(gd_gameplay.cave->score);

		gd_cave_play_sounds(gd_gameplay.cave);
	}

	if (gd_gameplay.cave->player_state==GD_PL_EXITED) {
		if (gd_gameplay.cave->intermission && gd_gameplay.cave->intermission_rewardlife && gd_gameplay.player_lives!=0)
			/* one life extra for completing intermission */
			add_bonus_life(FALSE);
		if (gd_gameplay.replay_record)
			gd_gameplay.replay_record->success=TRUE;

		/* start adding points for remaining time */
		gd_gameplay.state_counter=GAME_INT_CHECK_BONUS_TIME;
		gd_cave_clear_sounds(gd_gameplay.cave);
		gd_sound_play(gd_gameplay.cave, GD_S_FINISHED); /* play cave finished sound */
		gd_cave_play_sounds(gd_gameplay.cave);
		next_level();
	}

	if (((gd_gameplay.cave->player_state==GD_PL_DIED || gd_gameplay.cave->player_state==GD_PL_TIMEOUT) && fire) || restart) {
		/* player died, and user presses fire -> try again */
		/* time out -> try again */
		if (!gd_gameplay.cave->intermission && gd_gameplay.player_lives>0)
			/* no minus life for intermissions */
			gd_gameplay.player_lives--;
		if (gd_gameplay.cave->intermission)
			/* only one chance for intermissions */
			next_level();

		if (gd_gameplay.player_lives==0 && gd_gameplay.type==GD_GAMETYPE_NORMAL) {
			/* wait some time - this is a game over */
			if (gd_wait_before_game_over)
				gd_gameplay.state_counter=GAME_INT_WAIT_BEFORE_COVER;
			else
				gd_gameplay.state_counter=GAME_INT_COVER_START;	/* jump immediately to covering */
		}
		else {
			/* start cover animation immediately */
			gd_gameplay.state_counter=GAME_INT_COVER_START;
		}
	}
}

GdGameState
gd_game_main_int(int millisecs_elapsed, GdDirection player_move, gboolean fire, gboolean suicide, gboolean restart, gboolean allow_iterate, gboolean yellowish_draw, gboolean fast_forward)
{
	gboolean frame;	/* set to true, if this will be an animation frame */
	int x, y;
	GdGameState return_state;
	int counter_next;

	counter_next=GAME_INT_INVALID;
	return_state=GD_GAME_INVALID_STATE;
	gd_gameplay.milliseconds_anim+=millisecs_elapsed;	/* keep track of time */
	frame=FALSE;	/* set to true, if this will be an animation frame */
	if (gd_gameplay.milliseconds_anim>=40) {
		frame=TRUE;
		gd_gameplay.milliseconds_anim-=40;
	}

	/* cannot be less than uncover start. */
	if (gd_gameplay.state_counter<GAME_INT_LOAD_CAVE) {
		g_assert_not_reached();
	}
	else
	/* the very beginning. */
	if (gd_gameplay.state_counter==GAME_INT_LOAD_CAVE) {
		/* do the things associated with loading a new cave. function creates gfx buffer and the like. */
		load_cave();

		/* cover all cells of cave */
		for (y=0; y<gd_gameplay.cave->h; y++)
			for (x=0; x<gd_gameplay.cave->w; x++)
				gd_gameplay.cave->map[y][x] |= COVERED;

		/* to play cover sound */
		gd_cave_clear_sounds(gd_gameplay.cave);
		gd_sound_play(gd_gameplay.cave, GD_S_COVER);
		gd_cave_play_sounds(gd_gameplay.cave);

		counter_next=gd_gameplay.state_counter+1;
		/* very important: tell the caller that we loaded a new cave. */
		/* size of the cave might be new, colors might be new, and so on. */
		return_state=GD_GAME_CAVE_LOADED;
	}
	else
	/* uncover animation */
	if (gd_gameplay.state_counter<GAME_INT_UNCOVER_ALL) {
		counter_next=gd_gameplay.state_counter;
		if (frame) {
			int j;
			
			/* original game uncovered one cell per line each frame.
			 * we have different cave sizes, so uncover width*height/40 random cells each frame. (original was width=40).
			 * this way the uncovering is the same speed also for intermissions. */
			for (j=0; j<gd_gameplay.cave->w*gd_gameplay.cave->h/40; j++)
				gd_gameplay.cave->map[g_random_int_range(0, gd_gameplay.cave->h)][g_random_int_range(0, gd_gameplay.cave->w)] &= ~COVERED;

			counter_next++;	/* as we did something, advance the counter. */
		}
		return_state=GD_GAME_NOTHING;
	}
	else
	/* time to uncover the whole cave. */
	if (gd_gameplay.state_counter==GAME_INT_UNCOVER_ALL) {
		for (y=0; y<gd_gameplay.cave->h; y++)
			for (x=0; x<gd_gameplay.cave->w; x++)
				gd_gameplay.cave->map[y][x] &= ~COVERED;

		/* to stop uncover sound. */
		gd_cave_clear_sounds(gd_gameplay.cave);
		gd_cave_play_sounds(gd_gameplay.cave);

		counter_next=GAME_INT_CAVE_RUNNING;
		return_state=GD_GAME_NOTHING;
	}
	else
	/* normal. */
	if (gd_gameplay.state_counter==GAME_INT_CAVE_RUNNING) {
		int cavespeed;

		if (!fast_forward)
			cavespeed=gd_gameplay.cave->speed;	/* cave speed in ms, like 175ms/frame */
		else
			cavespeed=40;	/* if fast forward, ignore cave speed, and go as 25 iterations/sec */

		/* ITERATION - cave is running. */
		return_state=GD_GAME_NOTHING;	/* normally nothing happes. but if we iterate, this might change. */
		if (allow_iterate)	/* if allowing cave movements, add elapsed time to timer. and then we can check what to do. */
			gd_gameplay.milliseconds_game+=millisecs_elapsed;
		if (gd_gameplay.milliseconds_game>=cavespeed) {
			GdPlayerState pl;
			
			gd_gameplay.milliseconds_game-=cavespeed;
			pl=gd_gameplay.cave->player_state;
			iterate_cave(player_move, fire, suicide, restart);
			return_state=GD_GAME_LABELS_CHANGED;	/* as we iterated, the score and the like could have been changed. */
			if (pl!=GD_PL_TIMEOUT && gd_gameplay.cave->player_state==GD_PL_TIMEOUT)
				return_state=GD_GAME_TIMEOUT_NOW;	/* and if the cave timeouted at this moment, that is a special case. */
		}

		counter_next=gd_gameplay.state_counter;	/* do not change counter state, as it is set by iterate_cave() */
	}
	/* before covering, we check for time bonus score */
	else
	if (gd_gameplay.state_counter==GAME_INT_CHECK_BONUS_TIME) {
		if (frame) {
			/* if time remaining, bonus points are added. do not start animation yet. */
			if (gd_gameplay.cave->time>0) {
				gd_gameplay.cave->time-=gd_gameplay.cave->timing_factor;		/* subtract number of "milliseconds" - nothing to do with gd_gameplay.millisecs! */
				increment_score(gd_gameplay.cave->timevalue);			/* higher levels get more bonus points per second remained */
				if (gd_gameplay.cave->time>60*gd_gameplay.cave->timing_factor) {	/* if much time (>60s) remained, fast counter :) */
					gd_gameplay.cave->time-=8*gd_gameplay.cave->timing_factor;	/* decrement by nine each frame, so it also looks like a fast counter. 9 is 8+1! */
					increment_score(gd_gameplay.cave->timevalue*8);
				}

				/* just to be neat */
				if (gd_gameplay.cave->time<0)
					gd_gameplay.cave->time=0;
				counter_next=gd_gameplay.state_counter;	/* do not change yet */
			}
			else
			/* if no more points, start waiting a bit, and later start covering. */
				counter_next=GAME_INT_WAIT_BEFORE_COVER;

			/* play bonus sound */
			gd_cave_set_seconds_sound(gd_gameplay.cave);
			gd_cave_play_sounds(gd_gameplay.cave);
			return_state=GD_GAME_LABELS_CHANGED;
		}
		else {
			return_state=GD_GAME_NOTHING;
			counter_next=gd_gameplay.state_counter;	/* do not change counter state, as it is set by iterate_cave() */
		}
	}
	else
	/* after adding bonus points, we wait some time before starting to cover. this is the FIRST frame... so we check for game over and maybe jump there */
	/* if no more lives, game is over. */
	if (gd_gameplay.state_counter==GAME_INT_WAIT_BEFORE_COVER) {
		counter_next=gd_gameplay.state_counter;
		if (frame)
			counter_next++;	/* 40ms elapsed, advance counter */
		if (gd_gameplay.type==GD_GAMETYPE_NORMAL && gd_gameplay.player_lives==0)
			return_state=GD_GAME_NO_MORE_LIVES;
		else
			return_state=GD_GAME_NOTHING;
	}
	else
	/* after adding bonus points, we wait some time before starting to cover. ... and the other frames. */
	if (gd_gameplay.state_counter>GAME_INT_WAIT_BEFORE_COVER && gd_gameplay.state_counter<GAME_INT_COVER_START) {
		counter_next=gd_gameplay.state_counter;
		if (frame)
			counter_next++;	/* 40ms elapsed, advance counter */
		return_state=GD_GAME_NOTHING;
	}
	else
	/* starting to cover. start cover sound. */
	if (gd_gameplay.state_counter==GAME_INT_COVER_START) {

		gd_cave_clear_sounds(gd_gameplay.cave);
		gd_sound_play(gd_gameplay.cave, GD_S_COVER);
		/* to play cover sound */
		gd_cave_play_sounds(gd_gameplay.cave);

		counter_next=gd_gameplay.state_counter+1;
		return_state=GD_GAME_NOTHING;
	}
	else
	/* covering. */
	if (gd_gameplay.state_counter>GAME_INT_COVER_START && gd_gameplay.state_counter<GAME_INT_COVER_ALL) {
		counter_next=gd_gameplay.state_counter;
		if (frame) {
			int j;

			counter_next++;	/* 40ms elapsed, doing cover: advance counter */
			/* covering eight times faster than uncovering. */
			for (j=0; j < gd_gameplay.cave->w*gd_gameplay.cave->h*8/40; j++)
				gd_gameplay.cave->map[g_random_int_range (0, gd_gameplay.cave->h)][g_random_int_range (0, gd_gameplay.cave->w)] |= COVERED;
		}
		
		return_state=GD_GAME_NOTHING;
	}
	else
	if (gd_gameplay.state_counter==GAME_INT_COVER_ALL) {
		/* cover all */
		for (y=0; y<gd_gameplay.cave->h; y++)
			for (x=0; x<gd_gameplay.cave->w; x++)
				gd_gameplay.cave->map[y][x] |= COVERED;

		counter_next=gd_gameplay.state_counter+1;
		return_state=GD_GAME_NOTHING;
	}
	else {
		/* cover all + 1 */

		/* if this is a normal game: */
		if (gd_gameplay.type==GD_GAMETYPE_NORMAL) {
			if (gd_gameplay.cave->player_state==GD_PL_EXITED)
				cave_finished_highscore();	/* we also have added points for remaining time -> now check for highscore */
			if (gd_gameplay.player_lives!=0)
				return_state=GD_GAME_NOTHING;	/* and go to next level */
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
		if (gd_gameplay.bonus_life_flash)	/* bonus life - frames */
			gd_gameplay.bonus_life_flash--;
		gd_gameplay.animcycle=(gd_gameplay.animcycle+1)%8;
	}
	/* always render the cave to the gfx buffer; however it may do nothing if animcycle was not changed. */
	gd_drawcave_game(gd_gameplay.cave, gd_gameplay.gfx_buffer, gd_gameplay.bonus_life_flash!=0, yellowish_draw, gd_gameplay.animcycle, gd_no_invisible_outbox);

	gd_gameplay.state_counter=counter_next;
	return return_state;
}

