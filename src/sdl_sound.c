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
#include <SDL/SDL_mixer.h>
#include <glib.h>
#include "settings.h"
#include "cave.h"
#include "util.h"



static Mix_Chunk *sounds[GD_S_MAX];
static GdSound sound2_last=GD_S_NONE, sound3_last=GD_S_NONE;
static gboolean mixer_started=FALSE;

static void
loadsound(GdSound which, const char *filename)
{
	const char *full_filename;
	
	g_assert(which!=GD_S_DIAMOND_RANDOM);	/* this should not be loaded, rather it will be selected randomly */
	g_assert(mixer_started);
	
	if (sounds[which]!=NULL) {
		Mix_FreeChunk(sounds[which]);
		sounds[which]=NULL;
	}
	
	full_filename=gd_find_file(filename);
	if (!full_filename)	/* if cannot find file, exit now */
		return;
		
	sounds[which]=Mix_LoadWAV(full_filename);
	if (sounds[which]==NULL)
		g_warning("%s: %s", filename, Mix_GetError());
}

gboolean
gd_sound_init()
{
	if(Mix_OpenAudio(22050, gd_sdl_16bit_mixing?AUDIO_S16:AUDIO_U8, 1, 4096)==-1) {
		g_warning("%s", Mix_GetError());
		return FALSE;
	}
	mixer_started=TRUE;
	
	loadsound(GD_S_AMOEBA, "amoeba.wav");
	loadsound(GD_S_MAGIC_WALL, "magic_wall.wav");
	loadsound(GD_S_CRACK, "crack.wav");
	loadsound(GD_S_COVER, "cover.wav");

	loadsound(GD_S_TIMEOUT_1, "timeout_1.wav");
	loadsound(GD_S_TIMEOUT_2, "timeout_2.wav");
	loadsound(GD_S_TIMEOUT_3, "timeout_3.wav");
	loadsound(GD_S_TIMEOUT_4, "timeout_4.wav");
	loadsound(GD_S_TIMEOUT_5, "timeout_5.wav");
	loadsound(GD_S_TIMEOUT_6, "timeout_6.wav");
	loadsound(GD_S_TIMEOUT_7, "timeout_7.wav");
	loadsound(GD_S_TIMEOUT_8, "timeout_8.wav");
	loadsound(GD_S_TIMEOUT_9, "timeout_9.wav");
	loadsound(GD_S_FINISHED, "finished.wav");

	loadsound(GD_S_EXPLOSION, "explosion.wav");
	loadsound(GD_S_WALK_EARTH, "walk_earth.wav");
	loadsound(GD_S_WALK_EMPTY, "walk_empty.wav");
	loadsound(GD_S_DIAMOND_COLLECT, "diamond_collect.wav");

	loadsound(GD_S_STONE, "stone.wav");
	loadsound(GD_S_DIAMOND_1, "diamond_1.wav");
	loadsound(GD_S_DIAMOND_2, "diamond_2.wav");
	loadsound(GD_S_DIAMOND_3, "diamond_3.wav");
	loadsound(GD_S_DIAMOND_4, "diamond_4.wav");
	loadsound(GD_S_DIAMOND_5, "diamond_5.wav");
	loadsound(GD_S_DIAMOND_6, "diamond_6.wav");
	loadsound(GD_S_DIAMOND_7, "diamond_7.wav");
	loadsound(GD_S_DIAMOND_8, "diamond_8.wav");

	return TRUE;
}

void gd_no_sound()
{
	if (!mixer_started)
		return;

	sound2_last=GD_S_NONE;
	sound3_last=GD_S_NONE;
	Mix_HaltChannel(-1);
}

void gd_play_sounds(GdSound sound1, GdSound sound2, GdSound sound3)
{
	static GdSound diamond_sounds[]={
		GD_S_DIAMOND_1,
		GD_S_DIAMOND_2,
		GD_S_DIAMOND_3,
		GD_S_DIAMOND_4,
		GD_S_DIAMOND_5,
		GD_S_DIAMOND_6,
		GD_S_DIAMOND_7,
		GD_S_DIAMOND_8,
	};
	
	if (!mixer_started || !gd_sdl_sound)
		return;
	
	if (sound1==GD_S_DIAMOND_RANDOM)
		sound1=diamond_sounds[g_random_int_range(0, G_N_ELEMENTS(diamond_sounds))];
	/* channel 3 is for crack sound, amoeba and magic wall. */
	if (sound3!=GD_S_NONE) {
		if (sound3==GD_S_CRACK) {
			/* crack sound */
			Mix_PlayChannel(3, sounds[sound3], 0);
			sound3_last=sound3;
		}
		else {
			/* if any other, play looped */
			if (!Mix_Playing(3) || (sound3!=sound3_last && sound3_last!=GD_S_CRACK)) {
				Mix_PlayChannel(3, sounds[sound3], 1000000);	/* "infinite" loop (for amoeba, magic and cover sound) */
				sound3_last=sound3;
			}
		}
	} else
		if (sound3_last!=GD_S_CRACK)	/* do not interrupt crack sound */
			Mix_HaltChannel(3);
	
	/* channel 2 is for walking, explosions */
	/* if no sound requested, do nothing. */
	if (sound2!=GD_S_NONE) {
		/* always (re)start the explosion sound if requested. */
		if (sound2==GD_S_EXPLOSION) {
			Mix_PlayChannel(2, sounds[sound2], 0);
			sound2_last=sound2;
		}
		else
		/* if channel 2 is playing the explosion sound, do nothing. */
		if (!(Mix_Playing(2) && sound2_last==GD_S_EXPLOSION)) {
			Mix_PlayChannel(2, sounds[sound2], 0);
			sound2_last=sound2;
		}
	}
	
	/* channel 1 is for small sounds */
	if (sound1!=GD_S_NONE)
		Mix_PlayChannel(1, sounds[sound1], 0);
}



