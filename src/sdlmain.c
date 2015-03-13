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
#include <SDL/SDL.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "config.h"
#include "cave.h"
#include "caveobject.h"
#include "caveengine.h"
#include "cavesound.h"
#include "cavedb.h"
#include "caveset.h"
#include "c64import.h"
#include "settings.h"
#include "util.h"
#include "gameplay.h"
#include "sdlgfx.h"
#include "sdlui.h"
#include "sound.h"
#include "about.h"

/* for main menu */
typedef enum _state {
	M_NONE,
	M_QUIT,	/* quit immediately */
	M_EXIT, /* normal quit */
	M_ABOUT,
	M_LICENSE,
	M_PLAY,
	M_SAVE,
	M_SAVE_AS_NEW,
	M_REPLAYS,
	M_OPTIONS,
	M_INSTALL_THEME,
	M_HIGHSCORE,
	M_LOAD,
	M_LOAD_FROM_INSTALLED,
	M_ERRORS,
	M_HELP,
} State;

static int cavenum;
static int levelnum;

static char *username;

static char *caveset_filename=NULL;
static char *last_folder=NULL;





static void
clear_header()
{
	SDL_Rect r;
	
	r.x=0;
	r.y=0;
	r.w=gd_screen->w;
	r.h=statusbar_height;
	SDL_FillRect(gd_screen, &r, SDL_MapRGB(gd_screen->format, 0, 0, 0));
}

static void
showheader_uncover()
{
	int cavename_y;
	int len;
	char *str;
	gboolean first_line;

	clear_header();
	first_line=FALSE;	/* will be set to TRUE, if we draw in the next few code lines. so the y coordinate of the second status line can be decided. */

	/* if playing an intermission, tell the user! */
	if (gd_gameplay.type==GD_GAMETYPE_REPLAY) {
		gd_blittext(gd_screen, -1, statusbar_y1, GD_GDASH_YELLOW, "PLAYING REPLAY");
		first_line=TRUE;
	}
	else {
		if (gd_gameplay.cave->intermission) {
			gd_blittext(gd_screen, -1, statusbar_y1, GD_GDASH_YELLOW, "ONE LIFE EXTRA");
			first_line=TRUE;
		}
		else
			/* if not an intermission, we may show the name of the game (caveset) */
			if (gd_show_name_of_game) {		/* if showing the name of the cave... */
				len=g_utf8_strlen(gd_caveset_data->name, -1);
				if (gd_screen->w/gd_font_width()/2>=len)	/* if have place for double-width font */
					gd_blittext(gd_screen, -1, statusbar_y1, GD_GDASH_YELLOW, gd_caveset_data->name);
				else
					gd_blittext_n(gd_screen, -1, statusbar_y1, GD_GDASH_YELLOW, gd_caveset_data->name);
				first_line=TRUE;
			}
	}

	cavename_y=first_line?statusbar_y2:statusbar_mid;
	/* "xy players, cave ab/3" */
	if (gd_gameplay.type==GD_GAMETYPE_NORMAL)
		str=g_strdup_printf("%d%c, %s/%d", gd_gameplay.player_lives, GD_PLAYER_CHAR, gd_gameplay.cave->name, gd_gameplay.cave->rendered);
	else
		/* if not a normal game, do not show number of remaining lives */
		str=g_strdup_printf("%s/%d", gd_gameplay.cave->name, gd_gameplay.cave->rendered);
	len=g_utf8_strlen(str, -1);
	if (gd_screen->w/gd_font_width()/2>=len)	/* if have place for double-width font */
		gd_blittext(gd_screen, -1, cavename_y, GD_GDASH_WHITE, str);
	else
		gd_blittext_n(gd_screen, -1, cavename_y, GD_GDASH_WHITE, str);
	g_free(str);
}

static void
showheader_pause()
{
	clear_header();
	gd_blittext(gd_screen, -1, statusbar_mid, GD_GDASH_WHITE, "SPACEBAR TO RESUME");
}

static void
showheader_gameover()
{
	clear_header();
	gd_blittext(gd_screen, -1, statusbar_mid, GD_GDASH_WHITE, "G A M E   O V E R");
}

static void
showheader_game(int timeout_since)
{
	int x, y;
	gboolean first_line;
	first_line=FALSE;	/* will be set to TRUE, if we draw in the next few code lines. so the y coordinate of the second status line can be decided. */

	clear_header();

	/* if playing an intermission, tell the user! */
	if (gd_gameplay.type==GD_GAMETYPE_REPLAY) {
		gd_blittext(gd_screen, -1, statusbar_y1, GD_GDASH_YELLOW, "PLAYING REPLAY");
		first_line=TRUE;
	}

	/* y position of status bar */
	y=first_line?statusbar_y2:statusbar_mid;

	if (gd_gameplay.cave->player_state==GD_PL_TIMEOUT && timeout_since/50%4==0) {
		clear_header();
		gd_blittext(gd_screen, -1, y, GD_GDASH_WHITE, "OUT OF TIME");
		return;
	}
	if (gd_gameplay.cave->player_state==GD_PL_NOT_YET) {
		showheader_uncover();
		return;
	}

	if (gd_keystate[SDLK_LSHIFT] || gd_keystate[SDLK_RSHIFT]) {
		/* ALTERNATIVE STATUS BAR BY PRESSING SHIFT */
		x=14*gd_scale;
		
		x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_WHITE, "%c%02d", GD_PLAYER_CHAR, gd_gameplay.player_lives);
		x+=24*gd_scale;
		/* color numbers are not the same as key numbers! c3->k1, c2->k2, c1->k3 */
		/* this is how it was implemented in crdr7. */
		x=gd_blittext_printf(gd_screen, x, y, gd_gameplay.cave->color3, "%c%02d", GD_KEY_CHAR, gd_gameplay.cave->key1);
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, y, gd_gameplay.cave->color2, "%c%02d", GD_KEY_CHAR, gd_gameplay.cave->key2);
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, y, gd_gameplay.cave->color1, "%c%02d", GD_KEY_CHAR, gd_gameplay.cave->key3);
		x+=24*gd_scale;
		x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_WHITE, "%02d", gd_cave_time_show(gd_gameplay.cave, gd_gameplay.cave->gravity_will_change));
	} else {
		/* NORMAL STATUS BAR */
		x=0;
		int time_secs;

		/* cave time is rounded _UP_ to seconds. so at the exact moment when it changes from
		   2sec remaining to 1sec remaining, the player has exactly one second. when it changes
		   to zero, it is the exact moment of timeout. */
		time_secs=gd_cave_time_show(gd_gameplay.cave, gd_gameplay.cave->time);

		if (gd_keystate[SDLK_f]) {
			/* fast forward mode - show "FAST" */
			x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_YELLOW, "%cFAST%c", GD_DIAMOND_CHAR, GD_DIAMOND_CHAR);
		} else {
			/* normal speed mode - show diamonds needed and value */
			if (gd_gameplay.cave->diamonds_needed>gd_gameplay.cave->diamonds_collected) {
				if (gd_gameplay.cave->diamonds_needed>0)
					x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_RED, "%03d", gd_gameplay.cave->diamonds_needed);
				else
					/* did not already count diamonds needed */
					x=gd_blittext(gd_screen, x, y, GD_GDASH_RED, "???");
			}
			else
				x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_WHITE, " %c%c", GD_DIAMOND_CHAR, GD_DIAMOND_CHAR);
			x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_WHITE, "%c%02d", GD_DIAMOND_CHAR, gd_gameplay.cave->diamond_value);
		}
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_GREEN, "%03d", gd_gameplay.cave->diamonds_collected);
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_WHITE, "%03d", time_secs);
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, y, GD_GDASH_WHITE, "%06d", gd_gameplay.player_score);
	}
}

static void
game_help()
{
	const char* strings_menu[]={
		gd_key_name(gd_sdl_key_left), "MOVE LEFT",
		gd_key_name(gd_sdl_key_right), "MOVE RIGHT",
		gd_key_name(gd_sdl_key_up), "MOVE UP",
		gd_key_name(gd_sdl_key_down), "MOVE DOWN",
		gd_key_name(gd_sdl_key_fire_1), "FIRE (SNAP)",
		gd_key_name(gd_sdl_key_suicide), "SUICIDE",
		"", "", 
		"F", "FAST FORWARD (HOLD)",
		"SHIFT", "STATUS BAR (HOLD)",
		"", "",
		"SPACE", "PAUSE",
		"ESC", "RESTART LEVEL",
		"", "",
		"F1", "MAIN MENU",
		"CTRL-Q", "QUIT PROGRAM",
		NULL
	};
	
	gd_help(strings_menu);
}






/* generate an user event */
static Uint32
timer_callback(Uint32 interval, void *param)
{
	SDL_Event ev;
	
	ev.type=SDL_USEREVENT;
	SDL_PushEvent(&ev);
	
	return interval;
}



/* the game itself */
static void
play_game_func()
{
	static gboolean toggle=FALSE;	/* this is used to divide the rate of the user interrupt by 2, if no fine scrolling requested */
	gboolean exit_game;
	gboolean show_highscore;
	int statusbar_since=0;	/* count number of frames from when the outoftime or paused event happened. */
	gboolean paused;
	SDL_TimerID tim;
	SDL_Event event;
	gboolean restart, suicide;	/* for sdl key_downs */

	exit_game=FALSE;
	show_highscore=FALSE;
	paused=FALSE;
	
	/* install the sdl timer which will generate events to control the speed of the game and drawing, at an 50hz rate; 1/50hz=20ms */
	tim=SDL_AddTimer(20, timer_callback, NULL);

	suicide=FALSE;	/* detected suicide and restart level keys */
	restart=FALSE;		
	while (!exit_game && SDL_WaitEvent(&event)) {
		GdGameState state;
		GdDirection player_move;
		
		switch(event.type) {
			case SDL_QUIT:	/* application closed by window manager */
				gd_quit=TRUE;
				exit_game=TRUE;
				break;
			
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE:
						restart=TRUE;
						break;
					case SDLK_F1:
						exit_game=TRUE;
						break;
					case SDLK_q:
						if (gd_keystate[SDLK_LCTRL]||gd_keystate[SDLK_RCTRL]) {
							gd_quit=TRUE;
							exit_game=TRUE;
						}
						break;
					case SDLK_SPACE:
						paused=!paused;
						if (paused) {
							statusbar_since=0;		/* count frames "paused" */
							gd_no_sound();		/* if paused, no sound. */
						}
						break;
					case SDLK_F2:
						suicide=TRUE;
						break;
					case SDLK_h:
						gd_no_sound();	/* switch off sounds when showing help. */
						game_help();
						/* no need to turn on sounds; next cave iteration will restore them. */
						/* also, do not worry about 25hz user events, as the help function will simply drop them. */
						break;
					default:
						break;
				}
				break;
			
			case SDL_USEREVENT:
				/* get movement */
				player_move=gd_direction_from_keypress(gd_up(), gd_down(), gd_left(), gd_right());
				/* tell the interrupt "20ms has passed" */
				state=gd_game_main_int(20, player_move, gd_fire(), suicide, restart, !paused && !gd_gameplay.out_of_window, FALSE, gd_keystate[SDLK_f]);
				if (paused) {
					if (statusbar_since/50%4==0)
						showheader_pause();
					else
						showheader_game(statusbar_since);
				}

				/* state of game, returned by gd_game_main_int */
				switch (state) {
					case GD_GAME_INVALID_STATE:
						g_assert_not_reached();
						break;
						
					case GD_GAME_CAVE_LOADED:
						gd_select_pixbuf_colors(gd_gameplay.cave->color0, gd_gameplay.cave->color1, gd_gameplay.cave->color2, gd_gameplay.cave->color3, gd_gameplay.cave->color4, gd_gameplay.cave->color5);
						gd_scroll_to_origin();
						SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));	/* fill whole gd_screen with black */
						showheader_uncover();
						suicide=FALSE;	/* clear detected keypresses, so we do not "remember" them from previous cave runs */
						restart=FALSE;		
						break;

					case GD_GAME_NOTHING:
						/* normally continue. */
						break;

					case GD_GAME_LABELS_CHANGED:
						showheader_game(statusbar_since);
						suicide=FALSE;	/* clear detected keypresses, as cave was iterated and they were processed */
						break;

					case GD_GAME_TIMEOUT_NOW:
						statusbar_since=0;
						showheader_game(statusbar_since);	/* also update the status bar here. */
						suicide=FALSE;	/* clear detected keypresses, as cave was iterated and they were processed */
						break;
					
					case GD_GAME_NO_MORE_LIVES:
						showheader_gameover();
						break;

					case GD_GAME_STOP:
						exit_game=TRUE;	/* game stopped, this could be a replay or a snapshot */ 
						break;

					case GD_GAME_GAME_OVER:
						exit_game=TRUE;
						show_highscore=TRUE;	/* normal game stopped, may jump to highscore later. */
						break;
				}

				statusbar_since++;

				/* for the sdl version, it seems nicer if we first scroll, and then draw. */
				/* scrolling for the sdl version will merely invalidate the whole gfx buffer. */
				/* if drawcave was before scrolling, it would draw, scroll would invalidate, and then it should be drawn again */
				/* only do the drawing if the cave already exists. */
				toggle=!toggle;
				/* if fine scrolling, scroll at 50hz. if not, only scroll at every second call, so 25hz. */
				if (gd_gameplay.cave && (toggle || gd_fine_scroll))
					gd_gameplay.out_of_window=gd_scroll(gd_gameplay.cave, gd_gameplay.cave->player_state==GD_PL_NOT_YET);	/* do the scrolling. scroll exactly, if player is not yet alive */

				gd_drawcave(gd_screen, gd_gameplay.cave, gd_gameplay.gfx_buffer);	/* draw the cave. */
				SDL_Flip(gd_screen);	/* can always be called, as it keeps track of dirty regions of the screen */
				break;
			}
	}
	SDL_RemoveTimer(tim);
	gd_no_sound();	/* we stop sounds. gd_stop_game would do it, but we need the game struct for highscore */

	/* (if stopped because of a quit event, do not bother highscore at all) */
	if (!gd_quit && show_highscore && gd_is_highscore(gd_caveset_data->highscore, gd_gameplay.player_score)) {
		int rank;

		/* enter to highscore table */
		rank=gd_add_highscore(gd_caveset_data->highscore, gd_gameplay.player_name, gd_gameplay.player_score);
		gd_show_highscore(NULL, rank);
	}
	else {
		/* no high score */
	}
}


static void
play_game(int start_cave, int start_level)
{
	char *name;

	/* ask name of player. */
	name=gd_input_string("ENTER YOUR NAME", username);
	if (!name)
		return;
	g_free(username);
	username=name;
	
	gd_music_stop();
	
	gd_new_game(username, start_cave, start_level);
	play_game_func();
	gd_stop_game();
}

static void
play_replay(Cave *cave, GdReplay *replay)
{
	gd_music_stop();
	
	gd_new_game_replay(cave, replay);
	play_game_func();
	gd_stop_game();
	/* wait for keys, as for example escape may be pressed at this time */
	gd_wait_for_key_releases();
	
	gd_music_play_random();
}

static int
previous_selectable_cave(int cavenum)
{
	int cn=cavenum;
	
	while (cn>0) {
		Cave *cave;

		cn--;
		cave=gd_return_nth_cave(cn);
		if (gd_all_caves_selectable || cave->selectable)
			return cn;
	}

	/* if not found any suitable, return current */
	return cavenum;
}

static int
next_selectable_cave(int cavenum)
{
	int cn=cavenum;
	
	while (cn<gd_caveset_count()-1) {
		Cave *cave;
		
		cn++;
		cave=gd_return_nth_cave(cn);
		if (gd_all_caves_selectable || cave->selectable)
			return cn;
	}

	/* if not found any suitable, return current */
	return cavenum;
}






/*
 *   SDASH REPLAYS MENU
 */

static void
replays_menu()
{
	gboolean finished;
	/* an item stores a cave (to see its name) or a cave+replay */
	typedef struct _item {
		Cave *cave;
		GdReplay *replay;
	} Item;
	int n, page;
	int current;
	const int lines_per_page=gd_screen->h/gd_line_height()-5;
	GList *citer;
	GPtrArray *items=NULL;
	Cave *cave;
	Item i;
	
	items=g_ptr_array_new();
	/* for all caves */
	for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
		GList *riter;
		
		cave=citer->data;
		/* if cave has replays... */
		if (cave->replays!=NULL) {
			/* add cave data */
			i.cave=cave;
			i.replay=NULL;
			g_ptr_array_add(items, g_memdup(&i, sizeof(i)));
			
			/* add replays, too */
			for (riter=cave->replays; riter!=NULL; riter=riter->next) {
				i.replay=(GdReplay *)riter->data;
				g_ptr_array_add(items, g_memdup(&i, sizeof(i)));
			}
		}
	}
	
	gd_backup_and_dark_screen();
	gd_status_line("CRSR:MOVE  SPACE:PLAY  S:SAVED  ESC:EXIT");
	
	current=1;
	finished=FALSE;
	while (!finished && !gd_quit) {
		page=current/lines_per_page;	/* show 18 lines per page */
		SDL_Event event;
		
		/* show lines */
		gd_clear_line(gd_screen, 0);	/* for empty top row */
		for (n=0; n<lines_per_page; n++) {	/* for empty caves&replays rows */
			int y=(n+2)*gd_line_height();
			
			gd_clear_line(gd_screen, y);
		}

		gd_title_line("GDASH REPLAYS, PAGE %d/%d", page+1, items->len/lines_per_page+1);
		for (n=0; n<lines_per_page && page*lines_per_page+n<items->len; n++) {
			int pos=page*lines_per_page+n;
			GdColor col_cave=current==pos?GD_GDASH_YELLOW:GD_GDASH_LIGHTBLUE;	/* selected=yellow, otherwise blue */
			GdColor col=current==pos?GD_GDASH_YELLOW:GD_GDASH_GRAY3;	/* selected=yellow, otherwise blue */
			Item *i;
			int x, y;
			
			i=(Item *) g_ptr_array_index(items, pos);

			x=0;
			y=(n+2)*gd_line_height();
			
			if (!i->replay) {
				/* no replay pointer: this is a cave, so write its name. */
				x=gd_blittext_n(gd_screen, x, y, col_cave, i->cave->name);
			} else {
				const char *comm;
				int c;
				char buffer[100];
				
				/* successful or not */
				x=gd_blittext_printf_n(gd_screen, x, y, i->replay->success?GD_GDASH_GREEN:GD_GDASH_RED, " %c ", GD_BALL_CHAR);

				/* player name */
				g_utf8_strncpy(buffer, i->replay->player_name, 15);	/* name: maximum 15 characters */
				x=gd_blittext_n(gd_screen, x, y, col, buffer);
				/* put 16-length spaces */
				for (c=g_utf8_strlen(buffer, -1); c<16; c++)
					x=gd_blittext_n(gd_screen, x, y, col, " ");

				/* write comment */
				if (!g_str_equal(i->replay->comment, ""))
					comm=i->replay->comment;
				else
				/* or date */
				if (!g_str_equal(i->replay->date, ""))
					comm=i->replay->date;
				else
				/* or nothing */
					comm="-";
				g_utf8_strncpy(buffer, comm, 19);	/* comment or data: maximum 20 characters */
				x=gd_blittext_n(gd_screen, x, y, col, comm);
				/* put 20-length spaces */
				for (c=g_utf8_strlen(buffer, -1); c<19; c++)
					x=gd_blittext_n(gd_screen, x, y, col, " ");

				/* saved - check box */				
				x=gd_blittext_printf_n(gd_screen, x, y, col, " %c", i->replay->saved?GD_CHECKED_BOX_CHAR:GD_UNCHECKED_BOX_CHAR);
			}
		}
		SDL_Flip(gd_screen);	/* draw to usere's screen */
		
		SDL_WaitEvent(&event);
		switch (event.type) {
			case SDL_QUIT:
				gd_quit=TRUE;
				break;
				
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_UP:
						do {
							current=gd_clamp(current-1, 1, items->len-1);
						} while (((Item *)g_ptr_array_index(items, current))->replay==NULL && current>=1);
						break;
					case SDLK_DOWN:
						do {
							current=gd_clamp(current+1, 1, items->len-1);
						} while (((Item *)g_ptr_array_index(items, current))->replay==NULL && current<items->len);
						break;
					case SDLK_s:
						{
							Item *i=(Item *)g_ptr_array_index(items, current);
							
							if (i->replay) {
								i->replay->saved=!i->replay->saved;
								gd_caveset_edited=TRUE;
							}
						}
						break;
					case SDLK_SPACE:
					case SDLK_RETURN:
						{
							Item *i=(Item *)g_ptr_array_index(items, current);
							
							if (i->replay) {
								gd_backup_and_black_screen();
								play_replay(i->cave, i->replay);
								gd_restore_screen();
							}
						}
						break;
					case SDLK_ESCAPE:
						finished=TRUE;
						break;
					
					case SDLK_PAGEUP:
						current=gd_clamp(current-lines_per_page, 0, items->len-1);
						break;
						
					case SDLK_PAGEDOWN:
						current=gd_clamp(current+lines_per_page, 0, items->len-1);
						break;
						
					default:
						/* other keys do nothing */
						break;
				}
				break;
			
			default:
				/* other events do nothing */
				break;
		}
	}
	
	/* set the theme. other variables are already set by the above code. */
	/* forget list of themes */
	g_ptr_array_foreach(items, (GFunc) g_free, NULL);
	g_ptr_array_free(items, TRUE);
	
	gd_restore_screen();
}




static State
main_menu()
{
	SDL_Surface **animation;
	int animcycle;
	int count;
	State s;
	int x, y;
	int waitcycle=0;

	SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));	/* fill whole gd_screen with black */
	animation=gd_get_title_animation();
	animcycle=0;
	/* count number of frames */
	count=0;
	while(animation[count]!=NULL)
		count++;

	/* start playing after creating title animation above */
	gd_music_play_random();

	y=gd_screen->h-3*gd_line_height()-gd_line_height()/4;	/* three lines and a bit more */
	x=gd_blittext_n(gd_screen, 0, y, GD_GDASH_WHITE, "GAME: ");
	x=gd_blittext_n(gd_screen, x, y, GD_GDASH_YELLOW, gd_caveset_data->name);
	if (gd_caveset_edited)	/* if edited (new replays), draw a sign */
		x=gd_blittext_n(gd_screen, x, y, GD_GDASH_RED, " *");

	gd_status_line("CRSR: SELECT   SPACE: PLAY   F1: HELP");
	
	if (gd_has_new_error())
		/* show error flag */
		gd_blittext_n(gd_screen, gd_screen->w-gd_font_width(), gd_screen->h-gd_font_height(), GD_GDASH_RED, "E");

	s=M_NONE;
	cavenum=gd_caveset_last_selected;
	levelnum=gd_caveset_last_selected_level;

	while(!gd_quit && s==M_NONE) {
		SDL_Event event;
		static int i=0;	/* used to slow down cave selection */
		int y=gd_screen->h-2*gd_line_height()-gd_line_height()/4;	/* two lines and a bit more */

		gd_clear_line(gd_screen, y);
		x=gd_blittext_n(gd_screen, 0, y, GD_GDASH_WHITE, "CAVE: ");
		x=gd_blittext_n(gd_screen, x, y, GD_GDASH_YELLOW, gd_return_nth_cave(cavenum)->name);
		x=gd_blittext_n(gd_screen, x, y, GD_GDASH_WHITE, "/");
		x=gd_blittext_printf_n(gd_screen, x, y, GD_GDASH_YELLOW, "%d", levelnum+1);

		/* play animation */		
		animcycle=(animcycle+1)%count;
		SDL_BlitSurface(animation[animcycle], 0, gd_screen, 0);
		SDL_Flip(gd_screen);
		
		i=(i+1)%3;

		while (SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:	/* application closed by window manager */
					gd_quit=TRUE;
					s=M_QUIT;
					break;
				
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_ESCAPE:	/* escape: quit app */
							s=M_EXIT;
							break;
							
						case SDLK_RETURN:	/* enter: start playing */
							s=M_PLAY;
							break;
						
						case SDLK_l:	/* load file */
							s=M_LOAD;
							break;

						case SDLK_s:	/* save file */
							s=M_SAVE;
							break;

						case SDLK_n:	/* save file with new filename */
							s=M_SAVE_AS_NEW;
							break;

						case SDLK_c:
							s=M_LOAD_FROM_INSTALLED;
							break;
						
						case SDLK_r:
							s=M_REPLAYS;
							break;

						case SDLK_a:
							s=M_ABOUT;
							break;
							
						case SDLK_i:
							s=M_LICENSE;
							break;
							
						case SDLK_o:	/* s: settings */
							s=M_OPTIONS;
							break;

						case SDLK_t:	/* t: install theme */
							s=M_INSTALL_THEME;
							break;
						
						case SDLK_e:	/* show error console */
							s=M_ERRORS;
							break;
							
						case SDLK_h:	/* h: highscore */
							s=M_HIGHSCORE;
							break;
						
						case SDLK_F1:	/* f1: help */
							s=M_HELP;
							break;
							
						default:	/* other keys do nothing */
							break;
					}
				default:	/* other events we don't care */
					break;
			}
		}
		
		/* not all frames process the joystick - otherwise it would be too fast. */
		waitcycle=(waitcycle+1)%2;
		
		/* we use gd_down() and functions like that, so the joystick also works */
		if (waitcycle==0) {
			/* joystick or keyboard up */		
			if (gd_up() && i==0) {
				levelnum++;
				if (levelnum>4)
					levelnum=4;
			}
			
			/* joystick or keyboard down */
			if (gd_down() && i==0) {
				levelnum--;
				if (levelnum<0)
					levelnum=0;
			}
			
			/* joystick or keyboard left */
			if (gd_left() && i==0)
				cavenum=previous_selectable_cave(cavenum);
			
			/* joystick or keyboard right */
			if (gd_right() && i==0)
				cavenum=next_selectable_cave(cavenum);
		}
		
		if (gd_space_or_enter_or_fire())
			s=M_PLAY;

		SDL_Delay(40);	/* 25 fps - we need exactly this for the animation */
	}

	gd_wait_for_key_releases();
	
	/* forget animation */
	for (x=0; x<count; x++)
		SDL_FreeSurface(animation[x]);
	g_free(animation);
	
	return s;
}

static void
main_help()
{
	const char* strings_menu[]={
		"CURSOR", "SELECT CAVE&LEVEL",
		"SPACE, RETURN", "PLAY GAME",
		"H", "SHOW HALL OF FAME",
		"R", "SHOW REPLAYS",
		"", "",
		"L", "LOAD CAVESET",
		"C", "LOAD FROM INSTALLED CAVES",
		"S", "SAVE CAVESET (REPLAYS)",
		"N", "SAVE AS NEW FILE",
		"", "",
		"O", "OPTIONS",
		"T", "INSTALL THEME",
		"E", "ERROR CONSOLE", 
		"A", "ABOUT GDASH",
		"I", "LICENSE",
		"", "",
		"ESCAPE", "QUIT",
		NULL
	};
	
	gd_help(strings_menu);
}


static void
caveset_file_operation_successful(const char *filename)
{
	/* if it is a bd file, remember new filename */
	if (g_str_has_suffix(filename, ".bd")) {
		char *stored;

		/* first make copy, then free and set pointer. we might be called with filename=caveset_filename */
		stored=g_strdup(filename);
		g_free(caveset_filename);
		caveset_filename=stored;
	} else {
		g_free(caveset_filename);
		caveset_filename=NULL;
	}
}

static void
load_file(const char *directory)
{
	char *filename;
	char *filter;
	
	/* if the caveset is edited, ask the user if to save. */
	/* if does not want to discard, get out here */
	if (!gd_discard_changes())
		return;
	
	if (!last_folder)
		last_folder=g_strdup(g_get_home_dir());
		
	filter=g_strjoinv(";", gd_caveset_extensions);
	filename=gd_select_file("SELECT CAVESET TO LOAD", directory?directory:last_folder, filter, FALSE);
	g_free(filter);

	/* if file selected */	
	if (filename) {
		/* remember last directory */
		g_free(last_folder);
		last_folder=g_path_get_dirname(filename);

		gd_save_highscore(gd_user_config_dir);
		
		gd_caveset_load_from_file(filename, gd_user_config_dir);

		/* if successful loading and this is a bd file, and we load highscores from our own config dir */
		if (!gd_has_new_error() && g_str_has_suffix(filename, ".bd") && !gd_use_bdcff_highscore)
			gd_load_highscore(gd_user_config_dir);
		
		g_free(filename);
	} 
}


/* save caveset to specified directory, and pop up error message if failed */
/* if not, call function to remember file name. */
static void
caveset_save(const gchar *filename)
{
	gboolean saved;

	saved=gd_caveset_save(filename, FALSE);
	if (!saved)
		gd_error_console();
	else
		caveset_file_operation_successful(filename);
}

/* ask for new filename to save file to. then do the save. */
static void
save_file_as(const char *directory)
{
	char *filename;
	char *filter;
	
	if (!last_folder)
		last_folder=g_strdup(g_get_home_dir());
	
	filter=g_strjoinv(";", gd_caveset_extensions);
	filename=gd_select_file("SAVE CAVESET AS", directory?directory:last_folder, filter, TRUE);
	g_free(filter);

	/* if file selected */	
	if (filename) {
		/* remember last directory */
		g_free(last_folder);
		last_folder=g_path_get_dirname(filename);
		
		caveset_save(filename);

	} 
	g_free(filename);
}

/* if the caveset has a valid bdcff file name, save caves into that. if not, call the "save file as" function */
static void
save_file()
{
	if (!caveset_filename)
		/* if no filename remembered, rather start the save_as function, which asks for one. */
		save_file_as(NULL);
	else
		/* if given, save. */
		caveset_save(caveset_filename);
}


int
main(int argc, char *argv[])
{
	State s;
	GOptionContext *context;
	GError *error=NULL;
	
	/* command line parsing */
	context=gd_option_context_new();
	g_option_context_parse (context, &argc, &argv, &error);
	g_option_context_free (context);
	if (error) {
		g_warning("%s", error->message);
		g_error_free(error);
	}

	/* show license? */
	if (gd_param_license) {
		char *wrapped=gd_wrap_text(gd_about_license, 72);
		
		/* print license and quit. */
		g_print("%s", wrapped);
		g_free(wrapped);
		return 0;
	}

	gd_settings_init_dirs();

	gd_install_log_handler();

	gd_cave_init();
	gd_cave_db_init();
	gd_cave_sound_db_init();
	gd_c64_import_init_tables();
	
	gd_load_settings();

	gd_caveset_clear();

	gd_clear_error_flag();

	gd_wait_before_game_over=TRUE;
	
	/* LOAD A CAVESET FROM A FILE, OR AN INTERNAL ONE */
	/* if remaining arguments, they are filenames */
	if (gd_param_cavenames && gd_param_cavenames[0]) {
		/* load caveset, "ignore" errors. */
		if (!gd_caveset_load_from_file (gd_param_cavenames[0], gd_user_config_dir)) {
			g_critical (_("Errors during loading caveset from file '%s'"), gd_param_cavenames[0]);
		} else
			caveset_file_operation_successful(gd_param_cavenames[0]);
	}
	else if (gd_param_internal) {
		/* if specified an internal caveset */
		if (!gd_caveset_load_from_internal (gd_param_internal-1, gd_user_config_dir))
			g_critical (_("%d: no such internal caveset"), gd_param_internal);
	}
	
	/* if failed or nothing requested, load default */
	if (gd_caveset==NULL)
		gd_caveset_load_from_internal (0, gd_user_config_dir);

	/* if cave or level values given, check range */
	if (gd_param_cave)
		if (gd_param_cave<1 || gd_param_cave>=gd_caveset_count() || gd_param_level<1 || gd_param_level>5) {
			g_critical (_("Invalid cave or level number!\n"));
				gd_param_cave=0;
				gd_param_level=0;
		}

	gd_sdl_init(gd_sdl_scale);
	gd_sound_init();
	
	gd_loadfont_default();
	gd_load_theme();

	gd_create_dark_background();
	
	username=g_strdup(g_get_real_name());
	
	while (!gd_quit) {
		/* if a cavenum was given on the command line */
		if (gd_param_cave) {
			/* do as if it was selected from the menu */
			cavenum=gd_param_cave-1;
			levelnum=gd_param_level-1;
			s=M_PLAY;
			
			gd_param_cave=0;	/* and forget it */
		}
		else
			s=main_menu();

		switch(s) {
			case M_NONE:
				break;
			
			/* PLAYING */
			case M_PLAY:
				/* get selected cave&level, then play */
				gd_caveset_last_selected=cavenum;
				gd_caveset_last_selected_level=levelnum;
				play_game(cavenum, levelnum);	
				break;

			case M_REPLAYS:
				replays_menu();
				break;
				
			case M_HIGHSCORE:
				gd_show_highscore(NULL, 0);
				break;			

			/* FILES */				
			case M_LOAD:
				load_file(NULL);
				break;
			
			case M_SAVE:
				save_file(NULL);
				break;
			
			case M_SAVE_AS_NEW:
				save_file_as(NULL);
				break;
			
			case M_LOAD_FROM_INSTALLED:
				load_file(gd_system_caves_dir);
				break;

			/* INFO */
			case M_ABOUT:
				gd_about();
				break;

			case M_LICENSE:
				gd_show_license();
				break;

			case M_HELP:
				main_help();
				break;
				
			/* SETUP */
			case M_INSTALL_THEME:
				gd_install_theme();
				break;
				
			case M_OPTIONS:
				gd_settings_menu();
				break;

			case M_ERRORS:
				gd_error_console();
				break;
				
			/* EXIT */				
			case M_EXIT:
				/* normal quit; ask about saving if necessary. */
				if (gd_discard_changes())
					gd_quit=TRUE;
				break;
				
			case M_QUIT:
				/* quit immediately */
				gd_quit=TRUE;
				break;
		};
	}

	SDL_Quit();

	gd_save_highscore(gd_user_config_dir);
	gd_save_settings();
	
	return 0;
}

