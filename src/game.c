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
#include "caveset.h"
#include "caveengine.h"
#include "settings.h"
#include "game.h"

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

GdGame game;




static void
cave_finished_highscore()
{
	GdHighScore *hs=g_new0(GdHighScore, 1);
	
	g_strlcpy(hs->name, game.player_name, sizeof(hs->name));
	hs->score=game.cave_score;

	/* enter to highscore table */
	game.original_cave->highscore=g_list_insert_sorted(game.original_cave->highscore, hs, gd_highscore_compare);
	/* we do not show the cave highscore table here... but it can be viewed from the menu. */
}




void
gd_stop_game()
{
	if (game.gfx_buffer)
		gd_cave_map_free(game.gfx_buffer);
	game.gfx_buffer=NULL;
	
	game.player_lives=0;
	if (game.cave)
		gd_cave_free(game.cave);
	game.cave=NULL;
	game.original_cave=NULL;
}

/* returns TRUE on success */
gboolean
gd_game_start_level (const Cave *snapshot_cave)
{
	int x, y;
	
	/* unload cave, if loaded by some reason. */
	gd_cave_free (game.cave);
	game.cave=NULL;
	game.original_cave=NULL;
	
	game.cave_score=0;
	
	if (game.gfx_buffer)
		gd_cave_map_free(game.gfx_buffer);
	game.gfx_buffer=NULL;

	/* if no more lives, game over */
	/* but if snapshot, also go on (that also might be test from editor) */
	if (game.player_lives<1 && !snapshot_cave)
		return FALSE;
	
	game.is_snapshot= snapshot_cave!=NULL;
	
	/* load the cave */
	if (!snapshot_cave) {
		/* specified cave from memory */
		game.original_cave=gd_return_nth_cave(game.cave_num);
		game.cave=gd_cave_new_rendered(game.original_cave, game.level_num);
		gd_cave_setup_for_game(game.cave);
		if (gd_random_colors)	/* new cave-recolor if requested. game.cave is a copy only! */
			gd_cave_set_random_colors(game.cave);
		if (gd_easy_play)		/* if handicap on */
			gd_cave_easy(game.cave);
		if (game.cave->intermission && game.cave->intermission_instantlife) {
			game.player_lives++;
			game.bonus_life_flash=100;
		}
	}
	else {
		/* if a snapshot is requested, that one... just create a copy */
		game.cave=gd_cave_new_from_cave(snapshot_cave);
		/* for snapshot, lives and score are zero! */
		game.player_lives=0;
		game.player_score=0;
	}

	game.gfx_buffer=gd_cave_map_new(game.cave, int);
	for (y=0; y<game.cave->h; y++)
		for (x=0; x<game.cave->w; x++)
			game.gfx_buffer[y][x]=-1;	/* fill with "invalid" */

	/* cover all cells of cave */
	for (y=0; y < game.cave->h; y++)
		for (x=0; x < game.cave->w; x++)
			game.cave->map[y][x] |= COVERED;

	game.cover_counter=GAME_INT_UNCOVER_START;
	return TRUE;
}


void
gd_create_snapshot()
{
	g_return_if_fail (game.cave != NULL);

	if (game.snapshot_cave)
		gd_cave_free (game.snapshot_cave);
	/* make an exact copy */
	game.snapshot_cave=gd_cave_new_from_cave (game.cave);
	/* if it was a normal game previously, add "snapshot of ..." to cavename. otherwise it was a snapshot, so do not do this,
	   as it would result in names like snapshot of snapshot of ...  we don't care the editor test mode here. */
	if (!game.is_snapshot)
		/* change name */
		g_snprintf(game.snapshot_cave->name, sizeof(game.snapshot_cave->name), _("Snapshot of %s"), game.cave->name);
}

/* this starts a new game */
void
gd_new_game (const char *player_name, const int cave, const int level)
{
	g_strlcpy(game.player_name, player_name, sizeof(GdString));
	game.cave_num=cave;
	game.level_num=level;

	game.player_lives=3;
	game.player_score=0;

	gd_game_start_level (NULL);
}

static void
next_level ()
{
	game.cave_num++;			/* next cave */
	if (game.cave_num >= gd_caveset_count ()) {
		/* if no more caves at this level, back to first one */
		game.cave_num=0;
		/* but now more difficult */
		game.level_num++;
		if (game.level_num > 4)
			/* if level 5 finished, back to first cave, same difficulty-original game behaviour */
			game.level_num=4;
	}
}


/* increment score of player.
	flash screen if bonus life
*/
static void
increment_score (const int increment)
{
	int i=game.player_score / 500;
	game.player_score+=increment;
	game.cave_score+=increment;
	if (game.player_score / 500 > i) {	/* if score crossed 500point boundary, */
		if (game.player_lives)
			game.player_lives++;	/* player gets an extra life, but only if no test or snapshot. */
		game.bonus_life_flash=100;	/* also flash the screen for 100 frames=4 secs */
	}
}



gboolean
gd_game_iterate_cave(gboolean up, gboolean down, gboolean left, gboolean right, gboolean fire, gboolean key_suicide, gboolean key_restart)
{
	/* if already time=0, skip iterating */
	if (game.cave->player_state!=PL_TIMEOUT)
		gd_cave_iterate (game.cave, up, down, left, right, fire, key_suicide);
		
	if (game.cave->score)
		increment_score (game.cave->score);
	if (game.cave->player_state==PL_EXITED) {
		if (game.cave->intermission && game.cave->intermission_rewardlife && game.player_lives!=0) {
			/* one life extra for completing intermission */
			game.player_lives++;
			game.bonus_life_flash=100;
		}

		/* start adding points for remaining time */
		game.cover_counter=GAME_INT_CHECK_BONUS_TIME;
		game.cave->sound1=GD_S_NONE;
		game.cave->sound2=GD_S_FINISHED;	/* cave finished sound */
		game.cave->sound3=GD_S_NONE;
		next_level();
		return TRUE;	/* nothing to do with this cave anymore */
	}

	if (((game.cave->player_state==PL_DIED || game.cave->player_state==PL_TIMEOUT) && fire) || key_restart) {
		/* player died, and user presses fire -> try again */
		/* time out -> try again */
		if (!game.cave->intermission && game.player_lives > 0)
			/* no minus life for intermissions */
			game.player_lives--;
		if (game.cave->intermission)
			/* only one chance for intermissions */
			next_level();

		if (game.player_lives==0 && !game.is_snapshot) {
			/* wait some time - this is a game over */
			if (game.wait_before_game_over)
				game.cover_counter=GAME_INT_WAIT_BEFORE_COVER;
			else
				game.cover_counter=GAME_INT_COVER_START;
		}
		else {
			/* start cover animation immediately */
			game.cover_counter=GAME_INT_COVER_START;
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
	if (game.bonus_life_flash)
		game.bonus_life_flash--;

	/* cave is running. nothing to do! */
	if (game.cover_counter==GAME_INT_CAVE_RUNNING)
		return GD_GAME_NOTHING;
	
	if (game.cover_counter==GAME_INT_UNCOVER_START) {
		game.cover_counter++;
		return GD_GAME_NOTHING;
	}

	/* if counter is negative, cave is to be uncovered */
	if (game.cover_counter<GAME_INT_CAVE_RUNNING) {
		int j;
		/* original game uncovered one cell per line each frame.
		 * we have different cave sizes, so uncover w*h/40 random cells each frame. (original was w=40).
		 * this way the uncovering is the same speed also for intermissions. */
		for (j=0; j < game.cave->w * game.cave->h / 40; j++)
			game.cave->map[g_random_int_range (0, game.cave->h)][g_random_int_range (0, game.cave->w)] &= ~COVERED;
		game.cover_counter++;
		if (game.cover_counter==GAME_INT_CAVE_RUNNING) {
			/* if reached running state, uncover all */
			for (y=0; y < game.cave->h; y++)
				for (x=0; x < game.cave->w; x++)
					game.cave->map[y][x] &= ~COVERED;
			/* and signal to install game interrupt. */
			return GD_GAME_START_ITERATE;
		}
		return GD_GAME_NOTHING;
	}

	/* if cave finished, start covering */
	if (game.cover_counter==GAME_INT_CHECK_BONUS_TIME) {
		/* if time remaining, bonus points are added. do not start animation yet. */
		if (game.cave->time>0) {
			game.cave->time-=game.cave->timing_factor;		/* subtract number of "milliseconds" */
			increment_score(game.cave->timevalue);			/* higher levels get more bonus points per second remained */
			if (game.cave->time>60*game.cave->timing_factor) {	/* if much time (>60s) remained, fast counter :) */
				game.cave->time-=8*game.cave->timing_factor;	/* decrement by nine each frame, so it also looks like a fast counter. 9 is 8+1! */
				increment_score(game.cave->timevalue*8);
			}
			/* ... does not increment counter when time bonus counts down */
		}
		else
			game.cover_counter++;	/* if no more points, start. */
		
		return GD_GAME_BONUS_SCORE;
	}
	
	if (game.cover_counter<GAME_INT_COVER_START) {
		/* after adding bonus points, we wait some time */
		game.cover_counter++;
		return GD_GAME_NOTHING;
	}

	if (game.cover_counter==GAME_INT_COVER_START) {
		game.cover_counter++;
		
		game.cave->sound1=GD_S_NONE;
		game.cave->sound2=GD_S_NONE;
		game.cave->sound3=GD_S_COVER;	/* cover sound */
		return GD_GAME_COVER_START;
	}
	
	if (game.cover_counter<GAME_INT_COVER_ALL) {
		/* cover animation. */
		int j;
		
		/* covering eight times faster than uncovering. */
		for (j=0; j < game.cave->w*game.cave->h*8/40; j++)
			game.cave->map[g_random_int_range (0, game.cave->h)][g_random_int_range (0, game.cave->w)] |= COVERED;
		game.cover_counter++;
		
		return GD_GAME_NOTHING;
	}

	if (game.cover_counter==GAME_INT_COVER_ALL) {
		/* cover all */
		for (y=0; y < game.cave->h; y++)
			for (x=0; x < game.cave->w; x++)
				game.cave->map[y][x] |= COVERED;
		game.cover_counter++;
		return GD_GAME_NOTHING;
	}
	
	/* and AFTER cover all, we are finished with the thing. */
	if (game.is_snapshot)
		return GD_GAME_STOP;	/* if it was a snapshot, we are finished. */
		
	if (game.cave->player_state==PL_EXITED)
		cave_finished_highscore();	/* we also have added points for remaining time -> now check for highscore */
	if (game.player_lives!=0)
		return GD_GAME_NEXT_LEVEL;	/* and go to next level */
	return GD_GAME_GAME_OVER;
}



