/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
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

#include "config.h"

/* the replay saver thing only works in the sdl version */
#ifdef HAVE_SDL

#include <SDL/SDL_mixer.h>
#include <glib/gi18n.h>

#include "framework/replaysaveractivity.hpp"
#include "framework/commands.hpp"
#include "sdl/IMG_savepng.h"
#include "cave/gamecontrol.hpp"
#include "cave/titleanimation.hpp"
#include "cave/caveset.hpp"
#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "settings.hpp"
#include "sound/sound.hpp"


void ReplaySaverActivity::SDLInmemoryScreen::set_title(char const *) {
}


void ReplaySaverActivity::SDLInmemoryScreen::configure_size() {
    if (surface)
        SDL_FreeSurface(surface);
    surface = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
}


ReplaySaverActivity::SDLInmemoryScreen::~SDLInmemoryScreen() {
    SDL_FreeSurface(surface);
}


void ReplaySaverActivity::SDLInmemoryScreen::save(char const *filename) {
    IMG_SavePNG(filename, surface, 2);        // 2 = not too much compression, but a bit faster than the default
}


ReplaySaverActivity::ReplaySaverActivity(App *app, CaveStored *cave, CaveReplay *replay, std::string const &filename_prefix)
:
    Activity(app),
    filename_prefix(filename_prefix),
    game(GameControl::new_replay(app->caveset, cave, replay)),
    pf(),
    fm(pf, ""),
    pm(),
    cellrenderer(pf, gd_theme),
    gamerenderer(pm, cellrenderer, fm, *game)
{
    int cell_size = cellrenderer.get_cell_size();
    pm.set_size(cell_size*GAME_RENDERER_SCREEN_SIZE_X, cell_size*GAME_RENDERER_SCREEN_SIZE_Y);
    gamerenderer.set_show_replay_sign(false);
    std::string wav_filename = filename_prefix + ".wav";
    wavfile = fopen(wav_filename.c_str(), "wb");
    if (!wavfile) {
        gd_critical(CPrintf("Cannot open %s for sound output") % wav_filename);
        app->enqueue_command(new PopActivityCommand(app));
        return;
    }
    fseek(wavfile, 44, SEEK_SET);    /* 44bytes offset: start of data in a wav file */
    wavlen = 0;
    frame = 0;
}


void ReplaySaverActivity::shown_event() {
    std::vector<Pixmap *> animation = get_title_animation_pixmap(app->caveset->title_screen, app->caveset->title_screen_scroll, true, pf);
    pm.blit(*animation[0], 0, 0);
    delete animation[0];
    gd_music_stop();

    /* enable own timer and sound saver */
    install_own_mixer();
}


ReplaySaverActivity::~ReplaySaverActivity() {
    Mix_SetPostMix(NULL, NULL);    /* remove wav file saver */
    uninstall_own_mixer();
    gd_sound_off();

    /* write wav header, as now we now its final size. */
    fseek(wavfile, 0, SEEK_SET);
    Uint32 out32;
    Uint16 out16;

    int i=0;
    i+=fwrite("RIFF", 1, 4, wavfile);    /* "RIFF" */
    out32=GUINT32_TO_LE(wavlen+36); i+=fwrite(&out32, 1, 4, wavfile);    /* 4 + 8+subchunk1size + 8+subchunk2size */
    i+=fwrite("WAVE", 1, 4, wavfile);    /* "WAVE" */

    i+=fwrite("fmt ", 1, 4, wavfile); /* "fmt " */
    out32=GUINT32_TO_LE(16); i+=fwrite(&out32, 1, 4, wavfile); /* fmt chunk size=16 bytes */
    out16=GUINT16_TO_LE(1); i+=fwrite(&out16, 1, 2, wavfile); /* 1=pcm */
    out16=GUINT16_TO_LE(channels); i+=fwrite(&out16, 1, 2, wavfile);
    out32=GUINT32_TO_LE(frequency); i+=fwrite(&out32, 1, 4, wavfile);
    out32=GUINT32_TO_LE(frequency*bits/8*channels); i+=fwrite(&out32, 1, 4, wavfile); /* byterate */
    out16=GUINT16_TO_LE(bits/8*channels); i+=fwrite(&out16, 1, 2, wavfile); /* blockalign */
    out16=GUINT16_TO_LE(bits); i+=fwrite(&out16, 1, 2, wavfile); /* bitspersample */

    i+=fwrite("data", 1, 4, wavfile); /* "data" */
    out32=GUINT32_TO_LE(wavlen); i+=fwrite(&out32, 1, 4, wavfile);    /* actual data length */
    fclose(wavfile);

    if (i!=44)
        gd_critical("Could not write wav header to file!");

    std::string message = SPrintf(_("Saved %d video frames and %dMiB of audio data to %s_*.png and %s.wav.")) % (frame+1) %  (wavlen/1048576) % filename_prefix % filename_prefix;
    app->show_message(_("Replay Saved"), message);

    // restore settings
    gd_show_name_of_game=saved_gd_show_name_of_game;
    gd_sound_set_music_volume();
    gd_sound_set_chunk_volumes();
    gd_music_play_random();
    delete game;
}


void ReplaySaverActivity::install_own_mixer() {
    gd_sound_close();
    
    /* we setup mixing and other parameters for our own needs. */
    saved_gd_sdl_sound=gd_sound_enabled;
    saved_gd_sdl_44khz_mixing=gd_sound_44khz_mixing;
    saved_gd_sdl_16bit_mixing=gd_sound_16bit_mixing;
    saved_gd_sound_stereo=gd_sound_stereo;
    const char *driver=g_getenv("SDL_AUDIODRIVER");
    if (driver)
        saved_driver=driver;

    /* and after tweaking settings to our own need, we can init sdl. */
    gd_sound_enabled=true;
    gd_sound_44khz_mixing=true;
    gd_sound_16bit_mixing=true;
    gd_sound_stereo=true;
    // select the dummy driver, as it accepts any format, and we have control
    g_setenv("SDL_AUDIODRIVER", "dummy", TRUE);
    gd_sound_init(44100/25);    /* so the buffer will be 1/25th of a second. and we get our favourite 25hz interrupt from the mixer. */

    /* query audio format from sdl */
    Uint16 format;
    Mix_QuerySpec(&frequency, &format, &channels);
    if (frequency!=44100)        /* something must be really going wrong. */
        gd_critical("Cannot initialize mixer to 44100Hz mixing. The replay saver will not work correctly!");

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
    Mix_SetPostMix(mixfunc, this);
}


void ReplaySaverActivity::uninstall_own_mixer()
{
    gd_sound_close();

    // restore settings
    gd_sound_enabled=saved_gd_sdl_sound;
    gd_sound_44khz_mixing=saved_gd_sdl_44khz_mixing;
    gd_sound_16bit_mixing=saved_gd_sdl_16bit_mixing;
    gd_sound_stereo=saved_gd_sound_stereo;
    
    if (saved_driver!="")
        g_setenv("SDL_AUDIODRIVER", saved_driver.c_str(), TRUE);
    else
        g_unsetenv("SDL_AUDIODRIVER");
    
    gd_sound_init();
}


/* this function saves the wav file, and also does the timing! */
void ReplaySaverActivity::mixfunc(void *udata, Uint8 *stream, int len) {
    ReplaySaverActivity *rs=static_cast<ReplaySaverActivity *>(udata);
    
    if (!rs->wavfile)
        return;    
    if (fwrite(stream, 1, len, rs->wavfile)!=size_t(len))
        gd_critical("Cannot write to wav file!");

    // push a user event -> to do the timing
    SDL_Event ev;
    ev.type=SDL_USEREVENT+1;
    SDL_PushEvent(&ev);

    rs->wavlen+=len;
}


void ReplaySaverActivity::redraw_event() {
    app->clear_screen();
    
    app->title_line("SAVING REPLAY");
    app->status_line("PLEASE WAIT");

    // show it to the user.
    // it is not in displayformat, but a small image - not that slow to draw.
    // center coordinates
    int x=(app->screen->get_width()-pm.get_width())/2;
    int y=(app->screen->get_height()-pm.get_height())/2;
    // HACK
    SDL_Rect destr;
    destr.x = x;
    destr.y = y;
    destr.w = 0;
    destr.h = 0;
    SDL_BlitSurface(pm.get_surface(), 0, static_cast<SDLScreen*>(app->screen)->get_surface(), &destr);

    app->screen->flip();
}


void ReplaySaverActivity::timer2_event() {
    /* iterate and see what happened */
    /* no movement, no fire, no suicide, no restart, no pause, no fast movement.
     * the gamerenderer takes the moves from the replay. if we would add a move or a fire
     * press, the gamerenderer would switch to "continue replay" mode. */
    bool suicide=false;
    bool restart=false;
    GameRenderer::State state = gamerenderer.main_int(40, MV_STILL, false, suicide, restart, false, false, false);
    switch (state) {
        case GameRenderer::CaveLoaded:
        case GameRenderer::Iterated:
        case GameRenderer::Nothing:
            break;

        case GameRenderer::Stop:        /* game stopped, this could be a replay or a snapshot */
        case GameRenderer::GameOver:    /* game over should not happen for a replay, but no problem */
            app->enqueue_command(new PopActivityCommand(app));
            break;
    }

    /* before incrementing frame number, check if to save the frame to disk. */
    char *filename=g_strdup_printf("%s_%08d.png", filename_prefix.c_str(), frame);
    pm.save(filename);
    g_free(filename);
    
    redraw_event();
    frame++;
}

#endif /* IFDEF HAVE_SDL */
