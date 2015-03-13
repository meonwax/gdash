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

#include "sound.h"

/* start uncovering */
#define GAME_INT_UNCOVER_START -70
/* ...70 frames until full uncover... */
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




static void
cave_finished_highscore()
{
	/* enter to highscore table */
	gd_add_highscore(gd_gameplay.original_cave->highscore, gd_gameplay.player_name, gd_gameplay.cave_score);
}




void
gd_stop_game()
{
	if (gd_gameplay.gfx_buffer)
		gd_cave_map_free(gd_gameplay.gfx_buffer);
	gd_gameplay.gfx_buffer=NULL;
	
	gd_gameplay.player_lives=0;
	if (gd_gameplay.cave)
		gd_cave_free(gd_gameplay.cave);
	gd_gameplay.cave=NULL;
	gd_gameplay.original_cave=NULL;

	/* stop sounds */
	gd_no_sound();
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

/* returns TRUE on success */
gboolean
gd_game_start_level(const Cave *snapshot_cave)
{
	int x, y;
	
	/* unload cave, if loaded by some reason. */
	gd_cave_free(gd_gameplay.cave);
	gd_gameplay.cave=NULL;
	gd_gameplay.original_cave=NULL;
	
	gd_gameplay.cave_score=0;
	
	if (gd_gameplay.gfx_buffer)
		gd_cave_map_free(gd_gameplay.gfx_buffer);
	gd_gameplay.gfx_buffer=NULL;

	/* if no more lives, game over */
	/* but if snapshot, also go on (that also might be test from editor) */
	if (gd_gameplay.player_lives<1 && !snapshot_cave)
		return FALSE;
	
	gd_gameplay.is_snapshot= snapshot_cave!=NULL;
	
	/* load the cave */
	if (!snapshot_cave) {
		/* specified cave from memory */
		gd_gameplay.original_cave=gd_return_nth_cave(gd_gameplay.cave_num);
		gd_gameplay.cave=gd_cave_new_rendered(gd_gameplay.original_cave, gd_gameplay.level_num, g_random_int());	/* for playing: seed=random */
		gd_cave_setup_for_game(gd_gameplay.cave);
		if (gd_random_colors)	/* new cave-recolor if requested. gd_gameplay.cave is a copy only! */
			gd_cave_set_random_colors(gd_gameplay.cave);
		if (gd_easy_play)		/* if handicap on */
			gd_cave_easy(gd_gameplay.cave);
		if (gd_gameplay.cave->intermission && gd_gameplay.cave->intermission_instantlife)
			add_bonus_life(FALSE);
	}
	else {
		/* if a snapshot is requested, that one... just create a copy */
		gd_gameplay.cave=gd_cave_new_from_cave(snapshot_cave);
		/* for snapshot, lives and score are zero! */
		gd_gameplay.player_lives=0;
		gd_gameplay.player_score=0;
	}

	gd_gameplay.gfx_buffer=gd_cave_map_new(gd_gameplay.cave, int);
	for (y=0; y<gd_gameplay.cave->h; y++)
		for (x=0; x<gd_gameplay.cave->w; x++)
			gd_gameplay.gfx_buffer[y][x]=-1;	/* fill with "invalid" */

	/* cover all cells of cave */
	for (y=0; y < gd_gameplay.cave->h; y++)
		for (x=0; x < gd_gameplay.cave->w; x++)
			gd_gameplay.cave->map[y][x] |= COVERED;

	gd_gameplay.cover_counter=GAME_INT_UNCOVER_START;

	gd_cave_clear_sounds(gd_gameplay.cave);
	gd_sound_play(gd_gameplay.cave, GD_S_COVER);
	/* to play cover sound */
	gd_cave_play_sounds(gd_gameplay.cave);

	return TRUE;
}


void
gd_create_snapshot()
{
	g_return_if_fail (gd_gameplay.cave != NULL);

	if (gd_gameplay.snapshot_cave)
		gd_cave_free (gd_gameplay.snapshot_cave);
	/* make an exact copy */
	gd_gameplay.snapshot_cave=gd_cave_new_from_cave (gd_gameplay.cave);
	/* if it was a normal game previously, add "snapshot of ..." to cavename. otherwise it was a snapshot, so do not do this,
	   as it would result in names like snapshot of snapshot of ...  we don't care the editor test mode here. */
	if (!gd_gameplay.is_snapshot)
		/* change name */
		g_snprintf(gd_gameplay.snapshot_cave->name, sizeof(gd_gameplay.snapshot_cave->name), _("Snapshot of %s"), gd_gameplay.cave->name);
}

/* this starts a new game */
void
gd_new_game (const char *player_name, const int cave, const int level)
{
	gd_strcpy(gd_gameplay.player_name, player_name);
	gd_gameplay.cave_num=cave;
	gd_gameplay.level_num=level;

	gd_gameplay.player_lives=gd_caveset_data->initial_lives;
	gd_gameplay.player_score=0;
}

static void
next_level ()
{
	gd_gameplay.cave_num++;			/* next cave */
	if (gd_gameplay.cave_num>=gd_caveset_count()) {
		/* if no more caves at this level, back to first one */
		gd_gameplay.cave_num=0;
		/* but now more difficult */
		gd_gameplay.level_num++;
		if (gd_gameplay.level_num>4)
			/* if level 5 finished, back to first cave, same difficulty-original game behaviour */
			gd_gameplay.level_num=4;
	}
}


/* increment score of player.
	flash screen if bonus life
*/
static void
increment_score(int increment)
{
	int i=gd_gameplay.player_score/gd_caveset_data->bonus_life_score;
	
	gd_gameplay.player_score+=increment;
	gd_gameplay.cave_score+=increment;
	if (gd_gameplay.player_score/gd_caveset_data->bonus_life_score>i)
		add_bonus_life(TRUE);	/* if score crossed bonus_life_score point boundary, player won a bonus life */
}



gboolean
gd_game_iterate_cave(GdDirection player_move, gboolean fire, gboolean key_suicide, gboolean key_restart)
{
	/* if already time=0, skip iterating */
	if (gd_gameplay.cave->player_state!=GD_PL_TIMEOUT) {
		gd_cave_iterate (gd_gameplay.cave, player_move, fire, key_suicide);
		if (gd_gameplay.cave->score)
			increment_score (gd_gameplay.cave->score);

		gd_cave_play_sounds(gd_gameplay.cave);
	}
		
	if (gd_gameplay.cave->player_state==GD_PL_EXITED) {
		if (gd_gameplay.cave->intermission && gd_gameplay.cave->intermission_rewardlife && gd_gameplay.player_lives!=0)
			/* one life extra for completing intermission */
			add_bonus_life(FALSE);

		/* start adding points for remaining time */
		gd_gameplay.cover_counter=GAME_INT_CHECK_BONUS_TIME;
		gd_cave_clear_sounds(gd_gameplay.cave);
		gd_sound_play(gd_gameplay.cave, GD_S_FINISHED); /* play cave finished sound */
		gd_cave_play_sounds(gd_gameplay.cave);
		next_level();
		return TRUE;	/* nothing to do with this cave anymore */
	}

	if (((gd_gameplay.cave->player_state==GD_PL_DIED || gd_gameplay.cave->player_state==GD_PL_TIMEOUT) && fire) || key_restart) {
		/* player died, and user presses fire -> try again */
		/* time out -> try again */
		if (!gd_gameplay.cave->intermission && gd_gameplay.player_lives > 0)
			/* no minus life for intermissions */
			gd_gameplay.player_lives--;
		if (gd_gameplay.cave->intermission)
			/* only one chance for intermissions */
			next_level();

		if (gd_gameplay.player_lives==0 && !gd_gameplay.is_snapshot) {
			/* wait some time - this is a game over */
			if (gd_gameplay.wait_before_game_over)
				gd_gameplay.cover_counter=GAME_INT_WAIT_BEFORE_COVER;
			else
				gd_gameplay.cover_counter=GAME_INT_COVER_START;
		}
		else {
			/* start cover animation immediately */
			gd_gameplay.cover_counter=GAME_INT_COVER_START;
		}
		return TRUE;			/* player died, and user presses fire: he tries again. nothing to do with this cave anymore */
	}

	return FALSE;	/* not finished */
}

GdGameState
gd_game_main_int()
{
	int x, y;

	/* bonus life flashing is part of the game */
	if (gd_gameplay.bonus_life_flash)
		gd_gameplay.bonus_life_flash--;

	/* cave is running. nothing to do! */
	if (gd_gameplay.cover_counter==GAME_INT_CAVE_RUNNING)
		return GD_GAME_NOTHING;
	
	if (gd_gameplay.cover_counter==GAME_INT_UNCOVER_START) {
		gd_gameplay.cover_counter++;
		return GD_GAME_NOTHING;
	}

	/* if counter is negative, cave is to be uncovered */
	if (gd_gameplay.cover_counter<GAME_INT_CAVE_RUNNING) {
		int j;
		/* original game uncovered one cell per line each frame.
		 * we have different cave sizes, so uncover w*h/40 random cells each frame. (original was w=40).
		 * this way the uncovering is the same speed also for intermissions. */
		for (j=0; j < gd_gameplay.cave->w * gd_gameplay.cave->h / 40; j++)
			gd_gameplay.cave->map[g_random_int_range (0, gd_gameplay.cave->h)][g_random_int_range (0, gd_gameplay.cave->w)] &= ~COVERED;
		gd_gameplay.cover_counter++;
		if (gd_gameplay.cover_counter==GAME_INT_CAVE_RUNNING) {
			/* if reached running state, uncover all */
			for (y=0; y < gd_gameplay.cave->h; y++)
				for (x=0; x < gd_gameplay.cave->w; x++)
					gd_gameplay.cave->map[y][x] &= ~COVERED;

			/* and signal to install game interrupt. */
			/* first iteration will remove cover sound. */
			return GD_GAME_START_ITERATE;
		}
		return GD_GAME_NOTHING;
	}

	/* if cave finished, start covering */
	if (gd_gameplay.cover_counter==GAME_INT_CHECK_BONUS_TIME) {
		/* if time remaining, bonus points are added. do not start animation yet. */
		if (gd_gameplay.cave->time>0) {
			gd_gameplay.cave->time-=gd_gameplay.cave->timing_factor;		/* subtract number of "milliseconds" */
			increment_score(gd_gameplay.cave->timevalue);			/* higher levels get more bonus points per second remained */
			if (gd_gameplay.cave->time>60*gd_gameplay.cave->timing_factor) {	/* if much time (>60s) remained, fast counter :) */
				gd_gameplay.cave->time-=8*gd_gameplay.cave->timing_factor;	/* decrement by nine each frame, so it also looks like a fast counter. 9 is 8+1! */
				increment_score(gd_gameplay.cave->timevalue*8);
			}
			
			/* just to be neat */
			if (gd_gameplay.cave->time<0)
				gd_gameplay.cave->time=0;

		}
		else
			gd_gameplay.cover_counter=GAME_INT_WAIT_BEFORE_COVER; 	/* if no more points, start waiting a bit, and later start covering. */

		/* play bonus sound */
		gd_cave_set_seconds_sound(gd_gameplay.cave);
		gd_cave_play_sounds(gd_gameplay.cave);
		
		return GD_GAME_BONUS_SCORE;
	}
	
	if (gd_gameplay.cover_counter<GAME_INT_COVER_START) {
		/* after adding bonus points, we wait some time */
		gd_gameplay.cover_counter++;
		return GD_GAME_NOTHING;
	}

	if (gd_gameplay.cover_counter==GAME_INT_COVER_START) {
		gd_gameplay.cover_counter++;
		
		gd_cave_clear_sounds(gd_gameplay.cave);
		gd_sound_play(gd_gameplay.cave, GD_S_COVER);
		/* to play cover sound */
		gd_cave_play_sounds(gd_gameplay.cave);

		return GD_GAME_COVER_START;
	}
	
	if (gd_gameplay.cover_counter<GAME_INT_COVER_ALL) {
		/* cover animation. */
		int j;
		
		/* covering eight times faster than uncovering. */
		for (j=0; j < gd_gameplay.cave->w*gd_gameplay.cave->h*8/40; j++)
			gd_gameplay.cave->map[g_random_int_range (0, gd_gameplay.cave->h)][g_random_int_range (0, gd_gameplay.cave->w)] |= COVERED;
		gd_gameplay.cover_counter++;
		
		return GD_GAME_NOTHING;
	}

	if (gd_gameplay.cover_counter==GAME_INT_COVER_ALL) {
		/* cover all */
		for (y=0; y < gd_gameplay.cave->h; y++)
			for (x=0; x < gd_gameplay.cave->w; x++)
				gd_gameplay.cave->map[y][x] |= COVERED;
		gd_gameplay.cover_counter++;
		return GD_GAME_NOTHING;
	}
	
	/* and AFTER cover all, we are finished with the thing. */
	if (gd_gameplay.is_snapshot)
		return GD_GAME_STOP;	/* if it was a snapshot, we are finished. */
		
	if (gd_gameplay.cave->player_state==GD_PL_EXITED)
		cave_finished_highscore();	/* we also have added points for remaining time -> now check for highscore */
	if (gd_gameplay.player_lives!=0)
		return GD_GAME_NEXT_LEVEL;	/* and go to next level */
	return GD_GAME_GAME_OVER;
}



