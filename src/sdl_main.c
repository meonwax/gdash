#include <SDL/SDL.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "config.h"
#include "cave.h"
#include "caveobject.h"
#include "caveengine.h"
#include "caveset.h"
#include "settings.h"
#include "util.h"
#include "game.h"
#include "sdl_gfx.h"
#include "sdl_sound.h"
#include "sdl_ui.h"
#include "about.h"

/* for main menu */
typedef enum _state {
	M_NONE,
	M_QUIT,
	M_PLAY,
	M_OPTIONS,
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
	showheader_new();
	gd_blittext_n(gd_screen, -1, statusbar_y1, GD_C64_WHITE, gd_default_cave->name);
	gd_blittext_printf_n(gd_screen, -1, statusbar_y2, GD_C64_WHITE, "%d%c, %s/%d", game.player_lives, GD_PLAYER_CHAR, game.cave->name, game.cave->rendered);
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
	int x=0;

	SDL_Rect r;
	
	r.x=0;
	r.y=0;
	r.w=gd_screen->w;
	r.h=statusbar_height;
	SDL_FillRect(gd_screen, &r, SDL_MapRGB(gd_screen->format, 0, 0, 0));

	if (game.cave->diamonds_needed>game.cave->diamonds_collected)	
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_RED, "%03d", game.cave->diamonds_needed);
	else
		x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, " %c%c", GD_DIAMOND_CHAR, GD_DIAMOND_CHAR);
	x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, "%c%02d", GD_DIAMOND_CHAR, game.cave->diamond_value);
	x+=10;
	x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_GREEN, "%03d", game.cave->diamonds_collected);
	x+=10;
	x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, "%03d", game.cave->time/game.cave->timing_factor);
	x+=10;
	x=gd_blittext_printf(gd_screen, x, statusbar_mid, GD_C64_WHITE, "%06d", game.player_score);
}

static void
game_help()
{
	const char* strings_menu[]={
		"CURSOR", "MOVE",
		"CTRL", "FIRE",
		"F2", "SUICIDE",
		"ESC", "RESTART LEVEL",
		"", "",
		"SPACE", "PAUSE",
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

		gd_select_pixbuf_colors(game.cave->color0, game.cave->color1, game.cave->color2, game.cave->color3, game.cave->color4, game.cave->color5);
		SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));	/* fill whole gd_screen with black */

		last_draw=SDL_GetTicks()-40;	/* -40 ensures that drawing starts immediately */
		last_iterate=SDL_GetTicks();	/* to avoid compiler warning */
		caveloop=TRUE;
		iterate=FALSE;
		suicide=FALSE;
		paused=FALSE;
		gd_play_sounds(GD_S_NONE, GD_S_NONE, GD_S_COVER);
		showheader_uncover();
		while (caveloop) {
			SDL_Event event;
			int wait;

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
			while (iterate && SDL_GetTicks()>=last_iterate+game.cave->speed) {
				if (!paused && !game.out_of_window) {
					PLAYER_STATE pl;

					/* returns true if cave is finished. if false, we will continue */
					pl=game.cave->player_state;
					iterate=!gd_game_iterate_cave(gd_up(), gd_down(), gd_left(), gd_right(), gd_fire(), suicide, gd_keystate[SDLK_ESCAPE]);
					if (pl!=PL_TIMEOUT && game.cave->player_state==PL_TIMEOUT)
						/* cave did timeout at this moment */
						outoftime_since=SDL_GetTicks();
					gd_play_sounds(game.cave->sound1, game.cave->sound2, game.cave->sound3);
				}
				
				if (paused)
					gd_no_sound();
				
				/* set this to false here. */
				/* this is to prevent holding the suicide key for longer than cave->speed killing many players. */
				suicide=FALSE;
				
				/* we do not do last_iterate=getticks, as do not want to miss iterate cycles */
				last_iterate+=game.cave->speed;
			}
			
			/* CAVE DRAWING, BONUS POINTS, ... ********************************************************************* */
			/* drawing at 25fps, 1000ms/25fps=40ms */
			if (SDL_GetTicks()>=last_draw+40) {
				GdGameState state;
				
				state=gd_game_main_int();
				switch(state) {
					case GD_GAME_NOTHING:
						/* nothing to do */
						break;
					case GD_GAME_BONUS_SCORE:
						if (gd_cave_set_seconds_sound(game.cave))
							gd_play_sounds(game.cave->sound1, game.cave->sound2, game.cave->sound3);
						break;
					case GD_GAME_COVER_START:
						/* to play cover sound */
						gd_play_sounds(game.cave->sound1, game.cave->sound2, game.cave->sound3);
						break;
					case GD_GAME_STOP:
						/* game finished, add highscore */
						exit_game=TRUE;
						caveloop=FALSE;
						break;
					case GD_GAME_START_ITERATE:
						gd_no_sound();	/* when the uncover animation is over, we must stop sounds. */
						iterate=TRUE;	/* start cave movements */
							/* XXX delete conver header */
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
				gd_drawcave(gd_screen, game.cave, game.gfx_buffer);

				/* draw status bar */
				if (game.player_lives==0)
					showheader_gameover();
				else
				switch(game.cave->player_state) {
					case PL_TIMEOUT:
						if ((SDL_GetTicks()-outoftime_since)/1000%4==0)
							showheader_timeout();
						else
							showheader_game();
						break;
					case PL_LIVING:
					case PL_DIED:
						if (paused && (SDL_GetTicks()-paused_since)/1000%4==0)
							showheader_pause();
						else
							showheader_game();
						break;
						
					case PL_NOT_YET:
						if (paused && (SDL_GetTicks()-paused_since)/1000%4==0)
							showheader_pause();
						else
							showheader_uncover();
						break;
						
					case PL_EXITED:
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
	gd_no_sound();

	/* (if stopped because of a quit event, do not bother highscore at all) */
	if (!gd_quit && show_highscore && gd_score_is_highscore(gd_default_cave->highscore, game.player_score)) {
		GdHighScore *hs=g_new0(GdHighScore, 1);

		/* enter to highscore table */
		g_strlcpy(hs->name, game.player_name, sizeof(hs->name));
		hs->score=game.player_score;

		gd_default_cave->highscore=g_list_insert_sorted(gd_default_cave->highscore, hs, gd_highscore_compare);
		gd_show_highscore(hs);
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
	SDL_Surface *image;
	State s;
	int x;
	
	SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));	/* fill whole gd_screen with black */
	image=gd_get_titleimage();
	SDL_BlitSurface(image, NULL, gd_screen, NULL);
	SDL_FreeSurface(image);

	x=gd_blittext_n(gd_screen, 0, 172, GD_C64_WHITE, "GAME: ");
	x=gd_blittext_n(gd_screen, x, 172, GD_C64_YELLOW, gd_default_cave->name);

	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "CRSR: SELECT   SPACE: PLAY   H: KEYS");	/* FIXME 8 */

	s=M_NONE;
	cavenum=gd_caveset_first_selectable();
	if (gd_all_caves_selectable)
		cavenum=0;
	levelnum=0;
	while(!gd_quit && s==M_NONE) {
		SDL_Event event;
		static int i=0;	/* used to slow down cave selection */

		gd_clear_line(gd_screen, 180);
		x=gd_blittext_n(gd_screen, 0, 180, GD_C64_WHITE, "CAVE: ");
		x=gd_blittext_n(gd_screen, x, 180, GD_C64_YELLOW, gd_return_nth_cave(cavenum)->name);
		x=gd_blittext_n(gd_screen, x, 180, GD_C64_WHITE, "/");
		x=gd_blittext_printf_n(gd_screen, x, 180, GD_C64_YELLOW, "%d", levelnum+1);
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
							
						case SDLK_o:	/* s: settings */
							s=M_OPTIONS;
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

		if (gd_space_or_fire())
			s=M_PLAY;
					
		SDL_Delay(75);
	}
	
	return s;
}

static void
main_help()
{
	const char* strings_menu[]={
		"CURSOR", "SELECT CAVE&LEVEL",
		"SPACE", "PLAY GAME",
		"S", "SHOW HALL OF FAME",
		"", "",
		"L", "LOAD CAVESET",
		"C", "LOAD FROM INSTALLED CAVES",
		"", "",
		"O", "OPTIONS",
		"E", "ERROR CONSOLE", 
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
		g_print("%s", gd_about_license);
		return 0;
	}

	gd_cave_init();
	gd_settings_init_with_language();

	gd_install_log_handler();
	
	gd_load_settings();

	gd_caveset_clear();

	gd_clear_error_flag();

	game.wait_before_game_over=TRUE;
	
	/* LOAD A CAVESET FROM A FILE, OR AN INTERNAL ONE */
	/* if remaining arguments, they are filenames */
	if (gd_param_cavenames && gd_param_cavenames[0]) {
		/* load caveset, "ignore" errors. */
		if (!gd_caveset_load_from_file (gd_param_cavenames[0], gd_user_config_dir))
			g_critical (_("Errors during loading caveset from file %s"), gd_param_cavenames[0]);
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

	gd_sdl_init();
	gd_sound_init();

	gd_loadfont_default();

	gd_loadcells_default();
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

			case M_LOAD:
				load_file(NULL);
				break;
			
			case M_LOAD_FROM_INSTALLED:
				load_file(gd_system_caves_dir);
				break;
				
			case M_OPTIONS:
				gd_settings_menu();
				break;
				
			case M_HIGHSCORE:
				gd_show_highscore(NULL);
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

