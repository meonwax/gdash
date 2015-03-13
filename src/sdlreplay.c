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
#include <SDL/SDL_mixer.h>
#include "IMG_savepng.h"
#include <glib.h>
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


/* for saving the wav file */
static unsigned int wavlen;
static unsigned int frame;
static Uint16 format;
static int frequency, channels, bits;

/* this function saves the wav file,
   and also does the timing! */
static void
mixfunc(void *udata, Uint8 *stream, int len)
{
    SDL_Event ev;

    if (fwrite(stream, 1, len, (FILE *)udata)!=len) {
        g_critical("Cannot write to wav file!");
    }

    ev.type=SDL_USEREVENT;
    SDL_PushEvent(&ev);

    wavlen+=len;
}

/* the game itself */
static void
play_game_func(GdGame *game, const char *filename_prefix)
{
    gboolean toggle=FALSE;    /* this is used to divide the rate of the user interrupt by 2, if no fine scrolling requested */
    gboolean exit_game;
    int statusbar_since=0;    /* count number of frames from when the outoftime or paused event happened. we need it for timeout header flash */
    SDL_Event event;
    GdStatusBarColors cols_struct;
    GdStatusBarColors *cols=&cols_struct;
    Uint32 out32;
    Uint16 out16;
    FILE *wavfile;
    char *filename, *text;
    int i;
    gboolean show;

    /* start the wave file */
    filename=g_strdup_printf("%s.wav", filename_prefix);
    wavfile=fopen(filename, "wb");
    if (!wavfile) {
        g_critical("Cannot open %s for sound output", filename);
        return;
    }
    g_free(filename);

    fseek(wavfile, 44, SEEK_SET);    /* 44bytes offset: start of data in a wav file */
    wavlen=0;
    frame=0;
    Mix_SetPostMix(mixfunc, wavfile);

    /* these are not important, but otherwise the "saved xxxx.png" line in the first frame could not be seen. */
    cols->background=GD_GDASH_BLACK;
    cols->default_color=GD_GDASH_WHITE;

    /* now do the replay */
    exit_game=FALSE;
    while (!exit_game && SDL_WaitEvent(&event)) {
        GdGameState state;

        switch(event.type) {
            case SDL_QUIT:    /* application closed by window manager */
                gd_quit=TRUE;
                exit_game=TRUE;
                break;

            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_F1:
                        exit_game=TRUE;
                        break;

                    case SDLK_q:
                        if (gd_keystate[SDLK_LCTRL]||gd_keystate[SDLK_RCTRL]) {
                            gd_quit=TRUE;
                            exit_game=TRUE;
                        }
                        break;
                    default:
                        break;
                }
                break;

            case SDL_USEREVENT:
                /* get movement */
                /* tell the interrupt "20ms has passed" */
                /* no movement, no fire, no suicide, no restart, no pause, no fast movement */
                state=gd_game_main_int(game, 20, MV_STILL, FALSE, FALSE, FALSE, !game->out_of_window, FALSE, FALSE);
                show=FALSE;

                /* state of game, returned by gd_game_main_int */
                switch (state) {
                    case GD_GAME_INVALID_STATE:
                        g_assert_not_reached();
                        break;

                    case GD_GAME_SHOW_STORY:
                    case GD_GAME_SHOW_STORY_WAIT:
                        /* should not happen */
                        break;

                    case GD_GAME_CAVE_LOADED:
                        /* select colors, prepare drawing etc. */
                        gd_select_pixbuf_colors(game->cave->color0, game->cave->color1, game->cave->color2, game->cave->color3, game->cave->color4, game->cave->color5);
                        gd_scroll_to_origin();
                        SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));    /* fill whole gd_screen with black - cave might be smaller than previous! */
                        /* select status bar colors here, as some depend on actual cave colors */
                        gd_play_game_select_status_bar_colors(cols, game->cave);
                        gd_showheader_uncover(game, cols, FALSE);    /* false=do not say "playing replay" in the status bar */
                        show=TRUE;
                        break;

                    case GD_GAME_NOTHING:
                        /* normally continue. */
                        break;

                    case GD_GAME_LABELS_CHANGED:
                        gd_showheader_game(game, statusbar_since, cols, FALSE);    /* false=not showing "playing replay" as it would be ugly in the saved video */
                        show=TRUE;
                        break;

                    case GD_GAME_TIMEOUT_NOW:
                        statusbar_since=0;
                        gd_showheader_game(game, statusbar_since, cols, FALSE);    /* also update the status bar here. */
                        show=TRUE;
                        break;

                    case GD_GAME_NO_MORE_LIVES:
                        /* should not reach */
                        break;

                    case GD_GAME_STOP:
                        exit_game=TRUE;    /* game stopped, this could be a replay or a snapshot */
                        break;

                    case GD_GAME_GAME_OVER:
                        exit_game=TRUE;
                        /* ... but should not reach. */
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
                }

                /* before incrementing frame number, check if to save the frame to disk. */
                if (frame%2==0) {
                    /* save every second frame, so 25hz */
                    filename=g_strdup_printf("%s_%08d.png", filename_prefix, frame/2);
                    IMG_SavePNG(filename, gd_screen, -1);
#if 0
                    /* now we can ruin the screen */
                    if (game->cave && game->replay_from) {
                        gd_clear_header(cols->background);
                        gd_blittext_printf_n(gd_screen, -1, gd_statusbar_y1, cols->default_color, "Movement %d of %d", game->replay_from->current_playing_pos, game->replay_from->movements->len);
                        gd_blittext_printf_n(gd_screen, -1, gd_statusbar_y2, cols->default_color, filename, game->replay_from->current_playing_pos, game->replay_from->movements->len);
                    }
#endif
                    g_free(filename);

                }
            
                /* once per iteration, show it to the user. */
                if (show || frame%10==0)
                    SDL_Flip(gd_screen);

                frame++;
                break;
            }
    }

    Mix_SetPostMix(NULL, NULL);    /* remove wav file saver */
    gd_sound_off();    /* we stop sounds. gd_game_free would do it, but we need the game struct for highscore */

    /* write wav header, as now we now its final size. */
    fseek(wavfile, 0, SEEK_SET);

    i=0;
    out32=GUINT32_TO_BE(0x52494646); i+=fwrite(&out32, 1, 4, wavfile);    /* "RIFF" */
    out32=GUINT32_TO_LE(wavlen+36); i+=fwrite(&out32, 1, 4, wavfile);    /* 4 + 8+subchunk1size + 8+subchunk2size */
    out32=GUINT32_TO_BE(0x57415645); i+=fwrite(&out32, 1, 4, wavfile);    /* "WAVE" */

    out32=GUINT32_TO_BE(0x666d7420); i+=fwrite(&out32, 1, 4, wavfile); /* "fmt " */
    out32=GUINT32_TO_LE(16); i+=fwrite(&out32, 1, 4, wavfile); /* fmt chunk size=16 bytes */
    out16=GUINT16_TO_LE(1); i+=fwrite(&out16, 1, 2, wavfile); /* 1=pcm */
    out16=GUINT16_TO_LE(channels); i+=fwrite(&out16, 1, 2, wavfile);
    out32=GUINT32_TO_LE(frequency); i+=fwrite(&out32, 1, 4, wavfile);
    out32=GUINT32_TO_LE(frequency*bits/8*channels); i+=fwrite(&out32, 1, 4, wavfile); /* byterate */
    out16=GUINT16_TO_LE(bits/8*channels); i+=fwrite(&out16, 1, 2, wavfile); /* blockalign */
    out16=GUINT16_TO_LE(bits); i+=fwrite(&out16, 1, 2, wavfile); /* bitspersample */

    out32=GUINT32_TO_BE(0x64617461); i+=fwrite(&out32, 1, 4, wavfile); /* "data" */
    out32=GUINT32_TO_LE(wavlen); i+=fwrite(&out32, 1, 4, wavfile);    /* actual data length */

    if (i!=44)
        g_critical("Could not write wav header to file!");

    text=g_strdup_printf("Saved %d video frames and %dMiB of audio data to %s_*.png and %s.wav.", frame/2, wavlen/1048576, filename_prefix, filename_prefix);
    gd_message(text);
    g_free(text);

    fclose(wavfile);
}

/* draws the title screen (maybe that of the game) */
static void
draw_title_screen()
{
    SDL_Surface **animation;
    SDL_Rect r;
    int x;

    animation=gd_get_title_animation(TRUE);    /* true=we need only the first frame */

    SDL_FillRect(gd_screen, NULL, SDL_MapRGB(gd_screen->format, 0, 0, 0));
    r.x=(gd_screen->w-animation[0]->w)/2;
    r.y=(gd_screen->h-animation[0]->h)/2;
    SDL_BlitSurface(animation[0], NULL, gd_screen, &r);
    SDL_Flip(gd_screen);

    /* forget animation */
    for (x=0; animation[x]!=NULL; x++)
        SDL_FreeSurface(animation[x]);
    g_free(animation);
}


static void
play_replay(GdCave *cave, GdReplay *replay)
{
    GdGame *game;
    char *prefix;
    char *prefix_rec;

    gd_wait_for_key_releases();
    prefix_rec=g_strdup_printf("%s%sout", gd_last_folder?gd_last_folder:g_get_home_dir(), G_DIR_SEPARATOR_S);    /* recommended */
    prefix=gd_input_string("OUTPUT FILENAME PREFIX", prefix_rec);
    g_free(prefix_rec);
    if (!prefix)
        return;

    /* draw the title screen, so it will be the first frame of the movie */
    draw_title_screen();

    game=gd_game_new_replay(cave, replay);
    play_game_func(game, prefix);
    gd_game_free(game);
    g_free(prefix);

    /* wait for keys, as for example escape may be pressed at this time */
    gd_wait_for_key_releases();
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
    int y_gameline;
    int image_h;
    gboolean show_status;
    gboolean title_image_shown;

    animation=gd_get_title_animation(FALSE);
    animcycle=0;
    /* count number of frames */
    count=0;
    while(animation[count]!=NULL)
        count++;

    /* height of title screen, then decide which lines to show and where */
    image_h=animation[0]->h;
    if (gd_screen->h-image_h < 2*gd_font_height()) {
        /* less than 2 lines - place for only one line of text. */
        y_gameline=image_h + (gd_screen->h-image_h-gd_font_height())/2;    /* centered in the small place */
        show_status=FALSE;
    } else
    if (gd_screen->h-image_h < 3*gd_font_height()) {
        /* more than 2, less than 3 - place for status bar. game name is not shown, as this will */
        /* only be true for a game with its own title screen, and i decided that in that case it */
        /* would make more sense. */
        y_gameline=image_h + (gd_screen->h-image_h-gd_font_height()*2)/2;    /* centered there */
        show_status=TRUE;
    } else {
        image_h=image_centered_threshold;    /* "minimum" height for the image, and it will be centered */
        /* more than 3, less than 4 - place for everything. */
        y_gameline=image_h + (gd_screen->h-image_h-gd_font_height()-gd_font_height())/2;
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
    }
    if (show_status)
        gd_status_line("L: LOAD   R: REPLAYS   ESC: EXIT");

    if (gd_has_new_error())
        /* show error flag */
        gd_blittext_n(gd_screen, gd_screen->w-gd_font_width(), gd_screen->h-gd_font_height(), GD_GDASH_RED, "E");

    s=M_NONE;

    title_image_shown=FALSE;
    while(!gd_quit && s==M_NONE) {
        SDL_Event event;

        /* play animation. if more frames, always draw. if only one frame, draw only once */
        if (!title_image_shown || count>1) {
            SDL_Rect dest_pos;
            animcycle=(animcycle+1)%count;
            dest_pos.x=(gd_screen->w-animation[animcycle]->w)/2;    /* centered horizontally */
            if (animation[animcycle]->h<image_centered_threshold)
                dest_pos.y=(image_centered_threshold-animation[animcycle]->h)/2;    /* centered vertically */
            else
                dest_pos.y=0;    /* top of screen, as not too much space left for info lines */
            SDL_BlitSurface(animation[animcycle], 0, gd_screen, &dest_pos);
            title_image_shown=TRUE;    /* shown at least once, so if not animated, we do not waste cpu */
        }
        SDL_Flip(gd_screen);

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

                        case SDLK_F1:    /* f1: help */
                            s=M_HELP;
                            break;

                        case SDLK_l:    /* load file */
                            s=M_LOAD;
                            break;
                        case SDLK_c:
                            s=M_LOAD_FROM_INSTALLED;
                            break;

                        case SDLK_r:
                            s=M_REPLAYS;
                            break;
                        case SDLK_i:    /* caveset info */
                            s=M_INFO;
                            break;
                        case SDLK_h:    /* h: highscore */
                            s=M_HIGHSCORE;
                            break;

                        case SDLK_a:
                            s=M_ABOUT;
                            break;

                        case SDLK_b:
                            s=M_LICENSE;
                            break;

                        case SDLK_e:    /* show error console */
                            s=M_ERRORS;
                            break;

                        default:    /* other keys do nothing */
                            break;
                    }
                default:    /* other events we don't care */
                    break;
            }
        }
        SDL_Delay(40);    /* 25 fps - we need exactly this for the animation */
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
        "R", "SHOW REPLAYS",
        "I", "SHOW CAVESET INFO",
        "H", "SHOW HALL OF FAME",
        "", "",
        "L", "LOAD CAVESET",
        "C", "LOAD FROM INSTALLED CAVES",
        "", "",
        "E", "ERROR CONSOLE",
        "A", "ABOUT GDASH",
        "B", "LICENSE",
        "", "",
        "ESCAPE", "QUIT",
        NULL
    };

    gd_help(strings_menu);
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

    /* we setup mixing and other parameters for our own needs. */
    /* this is why settings cannot be saved on exit! */
#ifdef GD_SOUND
    gd_sdl_sound=TRUE;
    gd_sdl_44khz_mixing=TRUE;
    gd_sdl_16bit_mixing=TRUE;
#endif
    gd_fine_scroll=FALSE;
    gd_sdl_fullscreen=FALSE;
    gd_sdl_scale=GD_SCALING_ORIGINAL;
    gd_sdl_pal_emulation=FALSE;
    gd_even_line_pal_emu_vertical_scroll=FALSE;
    gd_random_colors=FALSE;

    /* and after tweaking settings to our own need, we can init sdl. */
    putenv("SDL_AUDIODRIVER=dummy");    /* do not output audio; also this will make sure sdl accepts 44khz 16bit. */
    gd_sdl_init(gd_sdl_scale);
    gd_create_dark_background();

    gd_sound_init(44100/50);    /* so the buffer will be 1/50th of a second. and we get our favourite 50hz interrupt from the mixer. */
    /* query audio format from sdl, and start the wave file */
    Mix_QuerySpec(&frequency, &format, &channels);
    if (frequency!=44100) {
        /* something must be really going wrong. */
        g_critical("Cannot initialize mixer to 44100Hz mixing. The application will not work correctly!");
    }
    switch(format) {
        case AUDIO_U8: bits=8; break;
        case AUDIO_S8: bits=8; break;
        case AUDIO_U16LSB: bits=16; break;
        case AUDIO_S16LSB: bits=16; break;
        case AUDIO_U16MSB: bits=16; break;
        case AUDIO_S16MSB: bits=16; break;
        default:
            g_assert_not_reached();
    }
#ifdef GD_SOUND
    gd_sound_set_music_volume(gd_sound_music_volume_percent);
    gd_sound_set_chunk_volumes(gd_sound_chunks_volume_percent);
#endif

    gd_loadfont_default();
    gd_load_theme();

    /* LOAD A CAVESET FROM A FILE */
    if (gd_param_cavenames && gd_param_cavenames[0]) {
        /* load caveset, "ignore" errors. */
        if (!gd_caveset_load_from_file (gd_param_cavenames[0], gd_user_config_dir)) {
            g_critical ("Errors during loading caveset from file '%s'", gd_param_cavenames[0]);
        }
    }
    else
        /* set caveset name to this, otherwise it would look ugly */
        gd_strcpy(gd_caveset_data->name, ("No caveset loaded"));

    while (!gd_quit) {
        GdMainMenuSelected s;

        s=main_menu();

        switch(s) {
            case M_NONE:
                break;

            case M_INSTALL_THEME:
            case M_OPTIONS:
            case M_PLAY:
            case M_SAVE:
            case M_SAVE_AS_NEW:
                g_assert_not_reached();
                break;

            case M_REPLAYS:
                gd_replays_menu(play_replay, FALSE);
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
            case M_ERRORS:
                gd_error_console();
                break;

            /* EXIT */
            case M_EXIT:
            case M_QUIT:
                gd_quit=TRUE;
                break;
        };
    }

    SDL_Quit();

    /* MUST NOT SAVE SETTINGS */

    return 0;
}

