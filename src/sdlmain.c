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
	M_QUIT,
	M_ABOUT,
	M_LICENSE,
	M_PLAY,
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





static void
showheader_new()
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
	showheader_new();
	if (gd_show_name_of_game) {
		/* if showing the name of the cave... */
		gd_blittext_n(gd_screen, -1, statusbar_y1, GD_C64_WHITE, gd_caveset_data->name);
		cavename_y=statusbar_y2;	/* show cave name in the second line */
	} else
		/* otherwise, cave name in the middle. */
		cavename_y=statusbar_mid;
	gd_blittext_printf_n(gd_screen, -1, cavename_y, GD_C64_WHITE, "%d%c, %s/%d", gd_gameplay.player_lives, GD_PLAYER_CHAR, gd_gameplay.cave->name, gd_gameplay.cave->rendered);
}

static void
showheader_pause()
{
	showheader_new();
	gd_blittext(gd_screen, -1, statusbar_mid, GD_C64_WHITE, "SPACEBAR TO RESUME");
}

static void
showheader_timeout()
{
	showheader_new();
	gd_blittext(gd_screen, -1, statusbar_mid, GD_C64_WHITE, "OUT OF TIME");
}

static void
showheader_gameover()
{
	showheader_new();
	gd_blittext(gd_screen, -1, statusbar_mid, GD_C64_WHITE, "G A M E   O V E R");
}

static void
showheader_game()
{
	int x;

	SDL_Rect r;
	
	r.x=0;
	r.y=0;
	r.w=gd_screen->w;
	r.h=statusbar_height;
	SDL_FillRect(gd_screen, &r, SDL_MapRGB(gd_screen->format, 0, 0, 0));

	if (gd_keystate[SDLK_LSHIFT] || gd_keystate[SDLK_RSHIFT]) {
		/* ALTERNATIVE STATUS BAR BY PRESSING SHIFT */
		x=14*gd_scale;
		
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, "%c%02d", GD_PLAYER_CHAR, gd_gameplay.player_lives);
		x+=24*gd_scale;
		/* color numbers are not the same as key numbers! c3->k1, c2->k2, c1->k3 */
		/* this is how it was implemented in crdr7. */
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, gd_gameplay.cave->color3, "%c%02d", GD_KEY_CHAR, gd_gameplay.cave->key1);
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, gd_gameplay.cave->color2, "%c%02d", GD_KEY_CHAR, gd_gameplay.cave->key2);
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, gd_gameplay.cave->color1, "%c%02d", GD_KEY_CHAR, gd_gameplay.cave->key3);
		x+=24*gd_scale;
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, "%02d", gd_cave_time_show(gd_gameplay.cave, gd_gameplay.cave->gravity_will_change));
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
			x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_YELLOW, "%cFAST%c", GD_DIAMOND_CHAR, GD_DIAMOND_CHAR);
		} else {
			/* normal speed mode - show diamonds needed and value */
			if (gd_gameplay.cave->diamonds_needed>gd_gameplay.cave->diamonds_collected) {
				if (gd_gameplay.cave->diamonds_needed>0)
					x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_RED, "%03d", gd_gameplay.cave->diamonds_needed);
				else
					/* did not already count diamonds needed */
					x=gd_blittext(gd_screen, x, statusbar_mid, GD_C64_RED, "???");
			}
			else
				x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, " %c%c", GD_DIAMOND_CHAR, GD_DIAMOND_CHAR);
			x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, "%c%02d", GD_DIAMOND_CHAR, gd_gameplay.cave->diamond_value);
		}
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_GREEN, "%03d", gd_gameplay.cave->diamonds_collected);
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, "%03d", time_secs);
		x+=10*gd_scale;
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, "%06d", gd_gameplay.player_score);
	}
}

static void
game_help()
{
	const char* strings_menu[]={
		"CURSOR", "MOVE",
		"CTRL", "FIRE",
		"F2", "SUICIDE",
		"F", "FASH FORWARD (HOLD)",
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


/* the game itself */
static void
play_game(int start_cave, int start_level)
{
	gboolean exit_game;
	gboolean iterate;
	gboolean show_highscore;
	guint32 last_iterate, last_draw;
	guint32 paused_since=0, outoftime_since=0;
	char *name;

	name=gd_input_string("ENTER YOUR NAME", username);
	if (!name)
		return;
	g_free(username);
	username=name;
	
	gd_new_game(username, start_cave, start_level);

	exit_game=FALSE;
	show_highscore=FALSE;
	while (!exit_game) {
		gboolean success;
		gboolean caveloop;
		gboolean suicide;
		gboolean paused;
		
		success=gd_game_start_level(NULL);
		/* if refused to start, exit this loop */
		if (!success)
			break;

		gd_select_pixbuf_colors(gd_gameplay.cave->color0, gd_gameplay.cave->color1, gd_gameplay.cave->color2, gd_gameplay.cave->color3, gd_gameplay.cave->color4, gd_gameplay.cave->color5);
		gd_scroll_to_origin();
		SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));	/* fill whole gd_screen with black */

		last_draw=SDL_GetTicks()-40;	/* -40 ensures that drawing starts immediately */
		last_iterate=SDL_GetTicks();	/* to avoid compiler warning */
		caveloop=TRUE;
		iterate=FALSE;
		suicide=FALSE;
		paused=FALSE;
		showheader_uncover();
		while (caveloop) {
			SDL_Event event;
			int wait;
			int speed_div;
			
			speed_div=1;
			if (gd_keystate[SDLK_f])
				speed_div=5;

			while (SDL_PollEvent(&event)) {
				switch(event.type) {
					case SDL_QUIT:	/* application closed by window manager */
						gd_quit=TRUE;
						exit_game=TRUE;
						caveloop=FALSE;
						break;
					
					case SDL_KEYDOWN:
						switch(event.key.keysym.sym) {
							case SDLK_F1:
								exit_game=TRUE;
								caveloop=FALSE;
								break;
							case SDLK_q:
								if (gd_keystate[SDLK_LCTRL]||gd_keystate[SDLK_RCTRL]) {
									gd_quit=TRUE;
									exit_game=TRUE;
									caveloop=FALSE;
								}
								break;
							case SDLK_SPACE:
								paused=!paused;
								paused_since=SDL_GetTicks();
								break;
							case SDLK_F2:
								suicide=TRUE;
								break;
							case SDLK_h:
								wait=SDL_GetTicks();	/* remember time */
								gd_no_sound();
								game_help();
								wait=SDL_GetTicks()-wait;
								last_iterate+=wait;
								last_draw+=wait;
								break;
							default:
								break;
						}
						break;
						
				}
			}
			
			/* CAVE ITERATE ********************************************************************* */
			while (iterate && SDL_GetTicks()>=last_iterate+gd_gameplay.cave->speed/speed_div) {
				if (!paused && !gd_gameplay.out_of_window) {
					GdPlayerState pl;
					GdDirection player_move;
					
					/* returns true if cave is finished. if false, we will continue */
					player_move=gd_direction_from_keypress(gd_up(), gd_down(), gd_left(), gd_right());
					pl=gd_gameplay.cave->player_state;

					iterate=!gd_game_iterate_cave(player_move, gd_fire(), suicide, gd_keystate[SDLK_ESCAPE]);

					if (pl!=GD_PL_TIMEOUT && gd_gameplay.cave->player_state==GD_PL_TIMEOUT)
						/* cave did timeout at this moment */
						outoftime_since=SDL_GetTicks();
				}
				
				if (paused)
					gd_no_sound();
				
				/* set this to false here. */
				/* this is to prevent holding the suicide key for longer than cave->speed killing many players. */
				suicide=FALSE;
				
				/* we do not do last_iterate=getticks, as do not want to miss iterate cycles */
				last_iterate+=gd_gameplay.cave->speed/speed_div;
			}
			
			/* CAVE DRAWING, BONUS POINTS, ... ********************************************************************* */
			/* drawing at 25fps, 1000ms/25fps=40ms */
			if (SDL_GetTicks()>=last_draw+40) {
				GdGameState state;
				
				state=gd_game_main_int();
				switch(state) {
					case GD_GAME_NOTHING:						/* nothing to do */
						break;
					case GD_GAME_BONUS_SCORE:
						break;
					case GD_GAME_COVER_START:
						break;
					case GD_GAME_STOP:
						/* game finished, add highscore */
						exit_game=TRUE;
						caveloop=FALSE;
						break;
					case GD_GAME_START_ITERATE:
						iterate=TRUE;	/* start cave movements */
						last_iterate=SDL_GetTicks();
						break;
					case GD_GAME_NEXT_LEVEL:
						caveloop=FALSE;
						break;
					case GD_GAME_GAME_OVER:
						show_highscore=TRUE;
						caveloop=FALSE;
						break;
				}

				/* draw cave to gd_screen */
				gd_drawcave(gd_screen, gd_gameplay.cave, gd_gameplay.gfx_buffer);

				/* draw status bar */
				if (gd_gameplay.player_lives==0)
					showheader_gameover();
				else
				switch(gd_gameplay.cave->player_state) {
					case GD_PL_TIMEOUT:
						if ((SDL_GetTicks()-outoftime_since)/1000%4==0)
							showheader_timeout();
						else
							showheader_game();
						break;
					case GD_PL_LIVING:
					case GD_PL_DIED:
						if (paused && (SDL_GetTicks()-paused_since)/1000%4==0)
							showheader_pause();
						else
							showheader_game();
						break;
						
					case GD_PL_NOT_YET:
						if (paused && (SDL_GetTicks()-paused_since)/1000%4==0)
							showheader_pause();
						else
							showheader_uncover();
						break;
						
					case GD_PL_EXITED:
						showheader_game();
						break;
				}

				last_draw+=40;
			}

			SDL_Flip(gd_screen);
			
			SDL_Delay(20);
		}
	}
	gd_stop_game();

	/* (if stopped because of a quit event, do not bother highscore at all) */
	if (!gd_quit && show_highscore && gd_is_highscore(gd_caveset_data->highscore, gd_gameplay.player_score)) {
		GdHighScore hs;
		int rank;

		/* enter to highscore table */
		gd_strcpy(hs.name, gd_gameplay.player_name);
		hs.score=gd_gameplay.player_score;
		
		rank=gd_add_highscore(gd_caveset_data->highscore, hs);
		gd_show_highscore(NULL, rank);
	}
	else {
		/* no high score */
	}
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

	y=gd_screen->h-3*gd_line_height();
	x=gd_blittext_n(gd_screen, 0, y, GD_C64_WHITE, "GAME: ");
	x=gd_blittext_n(gd_screen, x, y, GD_C64_YELLOW, gd_caveset_data->name);

	gd_status_line("CRSR: SELECT   RETURN: PLAY   H: HELP");
	
	if (gd_has_new_error())
		/* show error flag */
		gd_blittext_n(gd_screen, gd_screen->w-gd_font_width(), gd_screen->h-gd_font_height(), GD_C64_RED, "E");

	s=M_NONE;
	cavenum=gd_caveset_first_selectable();
	if (gd_all_caves_selectable)
		cavenum=0;
	levelnum=0;

	while(!gd_quit && s==M_NONE) {
		SDL_Event event;
		static int i=0;	/* used to slow down cave selection */
		int y=gd_screen->h-2*gd_line_height();

		gd_clear_line(gd_screen, y);
		x=gd_blittext_n(gd_screen, 0, y, GD_C64_WHITE, "CAVE: ");
		x=gd_blittext_n(gd_screen, x, y, GD_C64_YELLOW, gd_return_nth_cave(cavenum)->name);
		x=gd_blittext_n(gd_screen, x, y, GD_C64_WHITE, "/");
		x=gd_blittext_printf_n(gd_screen, x, y, GD_C64_YELLOW, "%d", levelnum+1);

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
							s=M_QUIT;
							break;
							
						case SDLK_RETURN:	/* enter: start playing */
							s=M_PLAY;
							break;
						
						case SDLK_l:	/* load file */
							s=M_LOAD;
							break;

						case SDLK_c:
							s=M_LOAD_FROM_INSTALLED;
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
							
						case SDLK_s:	/* h: highscore */
							s=M_HIGHSCORE;
							break;
						
						case SDLK_h:
						case SDLK_F1:
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
		"S", "SHOW HALL OF FAME",
		"", "",
		"L", "LOAD CAVESET",
		"C", "LOAD FROM INSTALLED CAVES",
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
load_file(const char *directory)
{
	static char *last_directory=NULL;
	char *filename;
	
	if (!last_directory)
		last_directory=g_strdup(g_get_home_dir());
	
	filename=gd_select_file("SELECT CAVESET TO LOAD", directory?directory:last_directory, "*.bd;*.gds");

	/* if file selected */	
	if (filename) {
		/* remember last directory */
		g_free(last_directory);
		last_directory=g_path_get_dirname(filename);

		gd_save_highscore(gd_user_config_dir);
		
		gd_caveset_load_from_file(filename, gd_user_config_dir);

		/* if successful loading and this is a bd file, and we load highscores from our own config dir */
		if (!gd_has_new_error() && g_str_has_suffix(filename, ".bd") && !gd_use_bdcff_highscore)
			gd_load_highscore(gd_user_config_dir);
		
		g_free(filename);
	} 
}

int main(int argc, char *argv[])
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

	gd_settings_init_with_language();

	gd_install_log_handler();

	gd_cave_init();
	gd_cave_db_init();
	gd_cave_sound_db_init();
	gd_c64_import_init_tables();
	
	gd_load_settings();

	gd_caveset_clear();

	gd_clear_error_flag();

	gd_gameplay.wait_before_game_over=TRUE;
	
	/* LOAD A CAVESET FROM A FILE, OR AN INTERNAL ONE */
	/* if remaining arguments, they are filenames */
	if (gd_param_cavenames && gd_param_cavenames[0]) {
		/* load caveset, "ignore" errors. */
		if (!gd_caveset_load_from_file (gd_param_cavenames[0], gd_user_config_dir))
			g_critical (_("Errors during loading caveset from file '%s'"), gd_param_cavenames[0]);
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
				
			case M_PLAY:
				play_game(cavenum, levelnum);	
				break;
				
			case M_ABOUT:
				gd_about();
				break;

			case M_LICENSE:
				gd_show_license();
				break;

			case M_LOAD:
				load_file(NULL);
				break;
			
			case M_LOAD_FROM_INSTALLED:
				load_file(gd_system_caves_dir);
				break;
				
			case M_INSTALL_THEME:
				gd_install_theme();
				break;
				
			case M_OPTIONS:
				gd_settings_menu();
				break;
				
			case M_HIGHSCORE:
				gd_show_highscore(NULL, 0);
				break;
			
			case M_HELP:
				main_help();
				break;
				
			case M_ERRORS:
				gd_error_console();
				break;
				
			case M_QUIT:
				gd_quit=TRUE;
				break;
		};
	}

	SDL_Quit();

	gd_save_highscore(gd_user_config_dir);
	gd_save_settings();
	
	return 0;
}

