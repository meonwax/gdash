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
#ifndef _GD_GAME_H
#define _GD_GAME_H
#include <glib.h>
#include "cave.h"

typedef struct _gd_game {
	GdString player_name;	/* Name of player */
	int player_score;		/* Score of player */
	int player_lives;		/* Remaining lives of player */
	Cave *cave;				/* Copy of the cave. This is the iterated one */
	Cave *original_cave;	/* original cave from caveset. used to record highscore */
	Cave *snapshot_cave;	/* Snapshot cave */
	gboolean is_snapshot;
	gboolean out_of_window;	/* will be set to true, if player is not visible in the window, and we have to wait for scrolling */

	int cave_num;	  /* actual playing cave number */
	int cave_score;		/* score collected in this cave */
	int level_num;	   /* actual playing level */
	int cover_counter;	   /* counter used to control the game flow, rendering of caves */
	int bonus_life_flash;	 /* different kind of flashing, for bonus life */
	
	gboolean wait_before_game_over;		/* wait some time before covering the cave, if there is a game over. main() should set it true for sdl, false for gtk+ */
	
	int **gfx_buffer;		/* contains the indexes to the cells; created by *start_level, deleted by *stop_game */
} GdGame;

typedef enum _gd_game_state {
	GD_GAME_NOTHING,
	GD_GAME_START_ITERATE,
	GD_GAME_BONUS_SCORE,
	GD_GAME_COVER_START,
	GD_GAME_NEXT_LEVEL,
	GD_GAME_STOP,
	GD_GAME_GAME_OVER,
} GdGameState;

extern GdGame game;

void gd_stop_game();
void gd_create_snapshot();
gboolean gd_game_start_level(const Cave *snapshot_cave);
void gd_new_game (const char *player_name, const int cave, const int level);

gboolean gd_game_iterate_cave(gboolean up, gboolean down, gboolean left, gboolean right, gboolean fire, gboolean key_suicide, gboolean key_restart);
GdGameState gd_game_main_int();
gboolean gd_game_scroll(int width, int visible, int center, gboolean exact, int start, int to, int *current, int *desired, int *speed);


#endif

