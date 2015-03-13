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


static int cavenum;
static int levelnum;

static char *username;



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
        "I", "CAVE INFO",
        "", "",
        "F1", "MAIN MENU",
        "CTRL-Q", "QUIT PROGRAM",
        NULL
    };
    
    gd_help(strings_menu);
}





static void
showheader_pause(GdStatusBarColors *cols)
{
    gd_clear_header(cols->background);
    gd_blittext(gd_screen, -1, gd_statusbar_mid, cols->default_color, "SPACEBAR TO RESUME");
}

static void
showheader_gameover(GdStatusBarColors *cols)
{
    gd_clear_header(cols->background);
    gd_blittext(gd_screen, -1, gd_statusbar_mid, cols->default_color, "G A M E   O V E R");
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
play_game_func(GdGame *game)
{
    gboolean toggle=FALSE;    /* this is used to divide the rate of the user interrupt by 2, if no fine scrolling requested */
    gboolean exit_game;
    gboolean show_highscore;
    int statusbar_since=0;    /* count number of frames from when the outoftime or paused event happened. */
    gboolean paused;
    SDL_TimerID tim;
    SDL_Event event;
    gboolean restart, suicide;    /* for sdl key_downs */
    GdStatusBarColors cols_struct;
    GdStatusBarColors *cols=&cols_struct;
    char *wrapped;
    char **story_splitted=NULL;
    int story_pos=0, story_lines=0, story_max_show=0;   /* =0 -> avoid compiler warning */
    gboolean changed;

    /* install the sdl timer which will generate events to control the speed of the game and drawing, at an 50hz rate; 1/50hz=20ms */
    tim=SDL_AddTimer(20, timer_callback, NULL);

    suicide=FALSE;    /* detected suicide and restart level keys */
    restart=FALSE;        
    exit_game=FALSE;
    show_highscore=FALSE;
    paused=FALSE;
    while (!exit_game && SDL_WaitEvent(&event)) {
        GdGameState state;
        GdDirection player_move;
        
        switch(event.type) {
            case SDL_QUIT:    /* application closed by window manager */
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
                    case SDLK_i:
                        gd_show_cave_info(game->original_cave);
                        break;
                    case SDLK_SPACE:
                        paused=!paused;
                        if (paused) {
                            statusbar_since=0;        /* count frames "paused" */
                            gd_sound_off();        /* if paused, no sound. */
                        }
                        break;
                    case SDLK_F2:
                        suicide=TRUE;
                        break;
                    case SDLK_h:
                        gd_sound_off();    /* switch off sounds when showing help. */
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
                state=gd_game_main_int(game, 20, player_move, gd_fire(), suicide, restart, !paused && !game->out_of_window, FALSE, gd_keystate[SDLK_f]);

                /* state of game, returned by gd_game_main_int */
                switch (state) {
                    case GD_GAME_INVALID_STATE:
                        g_assert_not_reached();
                        break;
                        
                    case GD_GAME_SHOW_STORY:
                        gd_dark_screen();
                        gd_title_line(game->cave->name);
                        gd_status_line("FIRE: CONTINUE    UP, DOWN: SCROLL");
                        wrapped=gd_wrap_text(game->cave->story->str, gd_screen->w/gd_font_width()-2);
                        if (story_splitted)
                            g_strfreev(story_splitted);
                        story_splitted=g_strsplit_set(wrapped, "\n", -1);
                        g_free(wrapped);
                        story_lines=g_strv_length(story_splitted);
                        story_pos=0;
                        story_max_show=(gd_screen->h-gd_line_height()*4)/gd_line_height();
                        gd_blitlines_n(gd_screen, gd_font_width(), gd_line_height()*2, GD_GDASH_LIGHTBLUE, story_splitted, story_max_show, story_pos);
                        break;

                    case GD_GAME_SHOW_STORY_WAIT:
                        changed=FALSE;  /* if we changed the position, so the story must be redrawn */
                        if (gd_up() && story_pos>0) {
                            story_pos--;
                            changed=TRUE;
                        }
                        if (gd_down() && story_pos<story_lines-story_max_show-1) {
                            story_pos++;
                            changed=TRUE;
                        }
                        if (changed) {
                            gd_dark_screen();   /* these lines are copypaste from above */
                            gd_title_line(game->cave->name);
                            gd_status_line("FIRE: CONTINUE    UP, DOWN: SCROLL");
                            gd_blitlines_n(gd_screen, gd_font_width(), gd_line_height()*2, GD_GDASH_LIGHTBLUE, story_splitted, story_max_show, story_pos);
                        }
                        break;
                        
                    case GD_GAME_CAVE_LOADED:
                        g_strfreev(story_splitted);
                        story_splitted=NULL;
                        /* select colors, prepare drawing etc. */
                        gd_select_pixbuf_colors(game->cave->color0, game->cave->color1, game->cave->color2, game->cave->color3, game->cave->color4, game->cave->color5);
                        gd_scroll_to_origin();
                        SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));    /* fill whole gd_screen with black - cave might be smaller than previous! */
                        /* select status bar colors here, as some depend on actual cave colors */
                        gd_play_game_select_status_bar_colors(cols, game->cave);
                        gd_showheader_uncover(game, cols, TRUE);    /* true = show "playing replay" if necessary */
                        suicide=FALSE;    /* clear detected keypresses, so we do not "remember" them from previous cave runs */
                        restart=FALSE;        
                        break;

                    case GD_GAME_NOTHING:
                        /* normally continue. */
                        break;

                    case GD_GAME_LABELS_CHANGED:
                        gd_showheader_game(game, statusbar_since, cols, TRUE);    /* true = show "playing replay" if necessary */
                        suicide=FALSE;    /* clear detected keypresses, as cave was iterated and they were processed */
                        break;

                    case GD_GAME_TIMEOUT_NOW:
                        statusbar_since=0;
                        gd_showheader_game(game, statusbar_since, cols, TRUE);    /* also update the status bar here. */    /* true = show "playing replay" if necessary */
                        suicide=FALSE;    /* clear detected keypresses, as cave was iterated and they were processed */
                        break;
                    
                    case GD_GAME_NO_MORE_LIVES:
                        showheader_gameover(cols);
                        break;

                    case GD_GAME_STOP:
                        exit_game=TRUE;    /* game stopped, this could be a replay or a snapshot */ 
                        break;

                    case GD_GAME_GAME_OVER:
                        exit_game=TRUE;
                        show_highscore=TRUE;    /* normal game stopped, may jump to highscore later. */
                        break;
                }

                statusbar_since++;

                /* for the sdl version, it seems nicer if we first scroll, and then draw. */
                /* scrolling for the sdl version will merely invalidate the whole gfx buffer. */
                /* if drawcave was before scrolling, it would draw, scroll would invalidate, and then it should be drawn again */
                /* only do the drawing if the cave already exists. */
                toggle=!toggle;
                if (game->gfx_buffer) {
                    /* if fine scrolling, scroll at 50hz. if not, only scroll at every second call, so 25hz. */
                    if (game->cave && (toggle || gd_fine_scroll))
                        game->out_of_window=gd_scroll(game, game->cave->player_state==GD_PL_NOT_YET);    /* do the scrolling. scroll exactly, if player is not yet alive */

                    gd_drawcave(gd_screen, game);    /* draw the cave. */

                    /* may show pause header. but only if the cave already exists in a gfx buffer - or else we are seeing a story at the moment */
                    if (paused) {
                        if (statusbar_since/50%4==0)
                            showheader_pause(cols);
                        else
                            gd_showheader_game(game, statusbar_since, cols, TRUE);    /* true = show "playing replay" if necessary */
                    }
                }

                SDL_Flip(gd_screen);    /* can always be called, as it keeps track of dirty regions of the screen */
            }
    }
    SDL_RemoveTimer(tim);
    gd_sound_off();    /* we stop sounds. gd_game_free would do it, but we need the game struct for highscore */

    /* (if stopped because of a quit event, do not bother highscore at all) */
    if (!gd_quit && show_highscore && gd_is_highscore(gd_caveset_data->highscore, game->player_score)) {
        int rank;

        /* enter to highscore table */
        rank=gd_add_highscore(gd_caveset_data->highscore, game->player_name, game->player_score);
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
    GdGame *game;

    /* ask name of player. */
    name=gd_input_string("ENTER YOUR NAME", username);
    if (!name)
        return;
    g_free(username);
    username=name;
    
    gd_music_stop();
    
    game=gd_game_new(username, start_cave, start_level);
    play_game_func(game);
    gd_game_free(game);
}

static void
play_replay(GdCave *cave, GdReplay *replay)
{
    GdGame *game;
    
    gd_music_stop();
    
    game=gd_game_new_replay(cave, replay);
    play_game_func(game);
    gd_game_free(game);
    /* wait for keys, as for example escape may be pressed at this time */
    gd_wait_for_key_releases();
    
    gd_music_play_random();
}

static int
previous_selectable_cave(int cavenum)
{
    int cn=cavenum;
    
    while (cn>0) {
        GdCave *cave;

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
        GdCave *cave;
        
        cn++;
        cave=gd_return_nth_cave(cn);
        if (gd_all_caves_selectable || cave->selectable)
            return cn;
    }

    /* if not found any suitable, return current */
    return cavenum;
}









static GdMainMenuSelected
main_menu()
{
    const int image_centered_threshold=164*gd_scale;
    SDL_Surface **animation;
    int animcycle;
    int count;
    GdMainMenuSelected s;
    int x;
    int waitcycle=0;
    int y_gameline, y_caveline;
    int image_h;
    gboolean show_status;
    gboolean title_image_shown;

    animation=gd_get_title_animation(FALSE);
    animcycle=0;
    /* count number of frames */
    count=0;
    while(animation[count]!=NULL)
        count++;

    /* start playing after creating title animation above */
    gd_music_play_random();

    /* height of title screen, then decide which lines to show and where */
    image_h=animation[0]->h;
    if (gd_screen->h-image_h < 2*gd_font_height()) {
        /* less than 2 lines - place for only one line of text. */
        y_gameline=-1;
        y_caveline=image_h + (gd_screen->h-image_h-gd_font_height())/2;    /* centered in the small place */
        show_status=FALSE;
    } else
    if (gd_screen->h-image_h < 3*gd_font_height()) {
        /* more than 2, less than 3 - place for status bar. game name is not shown, as this will */
        /* only be true for a game with its own title screen, and i decided that in that case it */
        /* would make more sense. */
        y_gameline=-1;
        y_caveline=image_h + (gd_screen->h-image_h-gd_font_height()*2)/2;    /* centered there */
        show_status=TRUE;
    } else {
        image_h=image_centered_threshold;    /* "minimum" height for the image, and it will be centered */ 
        /* more than 3, less than 4 - place for everything. */
        y_gameline=image_h + (gd_screen->h-image_h-gd_font_height()-gd_font_height()*2)/2;    /* centered with cave name */
        y_caveline=y_gameline+gd_font_height();
        /* if there is some place, move the cave line one pixel lower. */
        if (y_caveline+2*gd_font_height()<gd_screen->h)
            y_caveline+=1*gd_scale;
        show_status=TRUE;
    }

    /* fill whole gd_screen with black */
    SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));
    /* then fill with the tile, so if the title image is very small, there is no black border */
    /* only do that if the image is significantly smaller */
    if (animation[0]->w < gd_screen->w*9/10 || animation[0]->h < image_centered_threshold*9/10) {
        SDL_Rect rect;
        
        rect.x=0;
        rect.y=0;
        rect.w=gd_screen->w;
        rect.h=image_centered_threshold;
        SDL_SetClipRect(gd_screen, &rect);
        gd_dark_screen();
        SDL_SetClipRect(gd_screen, NULL);
    }
    
    if (y_gameline!=-1) {
        x=gd_blittext_n(gd_screen, 0, y_gameline, GD_GDASH_WHITE, "GAME: ");
        x=gd_blittext_n(gd_screen, x, y_gameline, GD_GDASH_YELLOW, gd_caveset_data->name);
        if (gd_caveset_edited)    /* if edited (new replays), draw a sign XXX */
            x=gd_blittext_n(gd_screen, x, y_gameline, GD_GDASH_RED, " *");
    }
    if (show_status) {
        if (gd_has_new_error())
            /* show error flag */
               gd_status_line_red("E: SHOW ERROR MESSAGES    F1: HELP");
        else {
            /* otherwise normal status bar */
            if (gd_caveset_has_replays())
                gd_status_line("CRSR: SELECT   SPC: PLAY   R: REPLAYS");
            else
                gd_status_line("CRSR: SELECT   SPC: PLAY   F1: HELP");
        }
    } else {
        /* if no status bar allowed, at least show the error flag. */
        if (gd_has_new_error())
            /* show error flag */
            gd_blittext_n(gd_screen, gd_screen->w-gd_font_width(), gd_screen->h-gd_font_height(), GD_GDASH_RED, "E");
    }

    s=M_NONE;
    cavenum=gd_caveset_last_selected;
    levelnum=gd_caveset_last_selected_level;

    /* here we process the keys and joystick in an ugly way, so wait for releases first. */
    gd_wait_for_key_releases();

    title_image_shown=FALSE;
    while(!gd_quit && s==M_NONE) {
        SDL_Event event;
        SDL_Rect dest_pos;
        static int i=0;    /* used to slow down cave selection, as this function must run at 25hz for animation */

        /* play animation */
        if (!title_image_shown || count>1) {        
            animcycle=(animcycle+1)%count;
            dest_pos.x=(gd_screen->w-animation[animcycle]->w)/2;    /* centered horizontally */
            if (animation[animcycle]->h<image_centered_threshold)
                dest_pos.y=(image_centered_threshold-animation[animcycle]->h)/2;    /* centered vertically */
            else
                dest_pos.y=0;    /* top of screen, as not too much space left for info lines */
            SDL_BlitSurface(animation[animcycle], 0, gd_screen, &dest_pos);
            title_image_shown=TRUE;    /* shown at least once, so if not animated, we do not waste cpu */
        }

        /* selected cave */
        gd_clear_line(gd_screen, y_caveline);
        x=gd_blittext_n(gd_screen, 0, y_caveline, GD_GDASH_WHITE, "CAVE: ");
        if (gd_caveset_count()!=0) {    /* do not crash if no cave */
            x=gd_blittext_n(gd_screen, x, y_caveline, GD_GDASH_YELLOW, gd_return_nth_cave(cavenum)->name);
            x=gd_blittext_n(gd_screen, x, y_caveline, GD_GDASH_WHITE, "/");
            x=gd_blittext_printf_n(gd_screen, x, y_caveline, GD_GDASH_YELLOW, "%d", levelnum+1);
        }

        SDL_Flip(gd_screen);

        i=(i+1)%3;

        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:    /* application closed by window manager */
                    gd_quit=TRUE;
                    s=M_QUIT;
                    break;
                
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE:    /* escape: quit app */
                        case SDLK_q:
                            s=M_EXIT;
                            break;
                            
                        case SDLK_RETURN:    /* enter: start playing */
                            s=M_PLAY;
                            break;
                        
                        case SDLK_l:    /* load file */
                            s=M_LOAD;
                            break;

                        case SDLK_s:    /* save file */
                            s=M_SAVE;
                            break;

                        case SDLK_i:    /* caveset info */
                            s=M_INFO;
                            break;

                        case SDLK_n:    /* save file with new filename */
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
                            
                        case SDLK_b:
                            s=M_LICENSE;
                            break;
                            
                        case SDLK_o:    /* s: settings */
                            s=M_OPTIONS;
                            break;

                        case SDLK_t:    /* t: install theme */
                            s=M_INSTALL_THEME;
                            break;
                        
                        case SDLK_e:    /* show error console */
                            s=M_ERRORS;
                            break;
                            
                        case SDLK_h:    /* h: highscore */
                            s=M_HIGHSCORE;
                            break;
                        
                        case SDLK_F1:    /* f1: help */
                            s=M_HELP;
                            break;
                            
                        default:    /* other keys do nothing */
                            break;
                    }
                default:    /* other events we don't care */
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

        SDL_Delay(40);    /* 25 fps - we need exactly this for the animation */
    }

    gd_wait_for_key_releases();
    
    /* forget animation */
    for (x=0; x<count; x++)
        SDL_FreeSurface(animation[x]);
    g_free(animation);
    
    /* if play is selected, but no cave, rather jump to file load */
    if (s==M_PLAY && gd_caveset_count()==0)
        s=M_LOAD;
        
    return s;
}

static void
main_help()
{
    const char* strings_menu[]={
        "CURSOR", "SELECT CAVE&LEVEL",
        "SPACE, RETURN", "PLAY GAME",
        "I", "SHOW CAVESET INFO",
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
        "B", "LICENSE",
        "", "",
        "ESCAPE", "QUIT",
        NULL
    };
    
    gd_help(strings_menu);
}



/* save caveset to specified directory, and pop up error message if failed */
/* if not, call function to remember file name. */
static void
caveset_save(const gchar *filename)
{
    gboolean saved;

    saved=gd_caveset_save(filename);
    if (!saved)
        gd_error_console();
    else
        gd_caveset_file_operation_successful(filename);
}

/* ask for new filename to save file to. then do the save. */
static void
save_file_as(const char *directory)
{
    char *filename;
    char *filter;
    
    if (!gd_last_folder)
        gd_last_folder=g_strdup(g_get_home_dir());
    
    filter=g_strjoinv(";", gd_caveset_extensions);
    filename=gd_select_file("SAVE CAVESET AS", directory?directory:gd_last_folder, filter, TRUE);
    g_free(filter);

    /* if file selected */    
    if (filename) {
        /* remember last directory */
        g_free(gd_last_folder);
        gd_last_folder=g_path_get_dirname(filename);
        
        caveset_save(filename);

    } 
    g_free(filename);
}

/* if the caveset has a valid bdcff file name, save caves into that. if not, call the "save file as" function */
static void
save_file()
{
    if (!gd_caveset_filename)
        /* if no filename remembered, rather start the save_as function, which asks for one. */
        save_file_as(NULL);
    else
        /* if given, save. */
        gd_caveset_save(gd_caveset_filename);
}


int
main(int argc, char *argv[])
{
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
            gd_caveset_file_operation_successful(gd_param_cavenames[0]);
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
    gd_create_dark_background();
    gd_sound_init(0);
#ifdef GD_SOUND
    gd_sound_set_music_volume(gd_sound_music_volume_percent);
    gd_sound_set_chunk_volumes(gd_sound_chunks_volume_percent);
#endif

    gd_loadfont_default();
    gd_load_theme();

    username=g_strdup(g_get_real_name());
    
    while (!gd_quit) {
        GdMainMenuSelected s;
        
        /* if a cavenum was given on the command line */
        if (gd_param_cave) {
            /* do as if it was selected from the menu */
            cavenum=gd_param_cave-1;
            levelnum=gd_param_level-1;
            s=M_PLAY;
            
            gd_param_cave=0;    /* and forget it */
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
                gd_replays_menu(play_replay, TRUE);
                break;
            case M_HIGHSCORE:
                gd_show_highscore(NULL, 0);
                break;
            case M_INFO:
                gd_show_cave_info(NULL);
                break;

            /* FILES */                
            case M_LOAD:
                gd_open_caveset(NULL);
                break;
            case M_LOAD_FROM_INSTALLED:
                gd_open_caveset(gd_system_caves_dir);
                break;
            case M_SAVE:
                save_file(NULL);
                break;
            case M_SAVE_AS_NEW:
                save_file_as(NULL);
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
            case M_QUIT:
                gd_quit=TRUE;
                break;
        };
        
        /* if quit requested, check if the caveset is edited. */
        /* and the user wants to save, ignore the quit request */
        if (gd_quit) {
            /* ugly hack. gd_ask_yes_no would not work if quit is true. so set to false */
            gd_quit=FALSE;
            /* and if discards, set to true */
            if (gd_discard_changes())
                gd_quit=TRUE;
        }
    }

    SDL_Quit();

    gd_save_highscore(gd_user_config_dir);
    gd_save_settings();
    
    return 0;
}

