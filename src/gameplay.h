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
#ifndef _GD_GAMEPLAY_H
#define _GD_GAMEPLAY_H
#include <glib.h>
#include "gameplay.h"
#include "cave.h"

typedef enum _gd_gametype {
	GD_GAMETYPE_NORMAL,
	GD_GAMETYPE_SNAPSHOT,
	GD_GAMETYPE_TEST,
	GD_GAMETYPE_REPLAY,
	GD_GAMETYPE_CONTINUE_REPLAY,
} GdGameType;

typedef struct _gd_gameplay {
	GdString player_name;	/* Name of player */
	int player_score;		/* Score of player */
	int player_lives;		/* Remaining lives of player */
	
	GdGameType type;

	Cave *cave;				/* Copy of the cave. This is the iterated, changed (ruined...) one */
	Cave *original_cave;	/* original cave from caveset. used to record highscore */
	
	GdReplay *replay_record;
	GdReplay *replay_from;
	
	GList *replays_recorded;
	

	gboolean out_of_window;	/* will be set to true, if player is not visible in the window, and we have to wait for scrolling */

	int cave_num;	  /* actual playing cave number */
	int cave_score;		/* score collected in this cave */
	int level_num;	   /* actual playing level */
	int bonus_life_flash;	 /* different kind of flashing, for bonus life */
	
	int state_counter;	   /* counter used to control the game flow, rendering of caves */
	int **gfx_buffer;		/* contains the indexes to the cells; created by *start_level, deleted by *stop_game */
	int animcycle;
	int milliseconds;
	
	int replay_no_more_movements;
} GdGameplay;

typedef enum _gd_game_state {
	GD_GAME_INVALID_STATE,
	GD_GAME_CAVE_LOADED,
	GD_GAME_NOTHING,
	GD_GAME_LABELS_CHANGED,
	GD_GAME_TIMEOUT_NOW,	/* this signals the moment of time out */
	GD_GAME_NO_MORE_LIVES,
	GD_GAME_STOP,
	GD_GAME_GAME_OVER,
} GdGameState;

extern GdGameplay gd_gameplay;

extern gboolean gd_wait_before_game_over;		/* wait some time before covering the cave, if there is a game over. main() should set it true for sdl, false for gtk+ */

Cave *gd_create_snapshot();

void gd_stop_game();
void gd_new_game(const char *player_name, const int cave, const int level);
void gd_new_game_snapshot(Cave *snapshot);
void gd_new_game_test(Cave *cave, int level);
void gd_new_game_replay(Cave *cave, GdReplay *replay);

GdGameState gd_game_main_int(GdDirection player_move, gboolean fire, gboolean key_suicide, gboolean key_restart, gboolean allow_iterate, gboolean yellowish_draw, gboolean fast_forward);


#endif
