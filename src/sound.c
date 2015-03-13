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
#include "config.h"
#ifdef USE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif
#include <glib.h>
#include "settings.h"
#include "cave.h"
#include "util.h"

/*
	The C64 sound chip (the SID) had 3 channels. Boulder Dash used all 3 of them.
	
	Different channels were used for different sounds.
	Channel 3: amoeba sound, magic wall sound,
		cave cover & uncover sound, and the crack sound (gate open)
	
	Channel 2: Walking, diamond collecting and explosion; also time running out sound.
	
	Channel 1: other small sounds, ie. diamonds falling, boulders rolling.
	
	Sounds have precedence over each other. Ie. the crack sound is given precedence
	over other sounds (amoeba, for example.)
	Different channels also behave differently. Channel 2 sounds are not stopped, ie.
	walking can not be heard, until the explosion sound is finished playing completely.
	Channel 1 sounds are always stopped, if a new sound is requested.
	
	Here we implement this a bit differently. We use samples, instead of synthesizing
	the sounds. By stopping samples, sometimes small clicks generate. Therefore we do
	not stop them, rather fade them out quickly. (The SID had filters, which stopped
	these small clicks.)
	Also, channel 1 should be stopped very often. So I decided to use two SDL_Mixer
	channels to emulate one C64 channel; and they are used alternating. SDL channel 4
	is the "backup" channel 1. Other channels have the same indexes.

 */



#ifdef GD_SOUND
static Mix_Chunk *sounds[GD_S_MAX];
static gboolean mixer_started=FALSE;
static GdSound sound_playing[4];
#endif

#ifdef GD_SOUND
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
#endif




#ifdef GD_SOUND
static gboolean
is_sound_looped(GdSound sound)
{
	if (sound==GD_S_COVER || sound==GD_S_AMOEBA || sound==GD_S_MAGIC_WALL || sound==GD_S_COVER || sound==GD_S_PNEUMATIC_HAMMER)
		return TRUE;
	else
		return FALSE;
}
#endif

#ifdef GD_SOUND
static void
channel_done(int channel)
{
	sound_playing[channel]=GD_S_NONE;
}
#endif

#ifdef GD_SOUND
static void
play_sound(int channel, GdSound sound)
{
	/* channel 1 and channel 4 are used alternating */
	static gboolean channel1_alter=FALSE;
	static const GdSound diamond_sounds[]={
		GD_S_DIAMOND_1,
		GD_S_DIAMOND_2,
		GD_S_DIAMOND_3,
		GD_S_DIAMOND_4,
		GD_S_DIAMOND_5,
		GD_S_DIAMOND_6,
		GD_S_DIAMOND_7,
		GD_S_DIAMOND_8,
	};

	/* change diamond falling random to a selected diamond falling sound. */
	/* others are hack! */
	switch (sound) {
		case GD_S_DIAMOND_RANDOM:
		case GD_S_BOMB_COLLECT:
		case GD_S_KEY_COLLECT:
		case GD_S_SWITCH_CHANGE:
		case GD_S_BLADDER_SPENDER:
			sound=diamond_sounds[g_random_int_range(0, G_N_ELEMENTS(diamond_sounds))];
			break;

		default:
			break;
	}

	if (channel==1) {
		int other_channel;
		/* channel 1 and channel 4 emulate the sid channel 1 */
		channel1_alter=!channel1_alter;
		if (channel1_alter)
			channel=4, other_channel=1;
		else
			channel=1, other_channel=4;
			
		if (sound_playing[other_channel]!=GD_S_NONE)
			Mix_FadeOutChannel(other_channel, 50);
	}

	/* channel 2 and 3 sounds are started immediately; channel 1 may have been changed to channel 4 above. */
	Mix_PlayChannel(channel, sounds[sound], is_sound_looped(sound)?-1:0);
	Mix_Volume(channel, MIX_MAX_VOLUME);
	sound_playing[channel]=sound;
}
#endif

#ifdef GD_SOUND
static void
halt_channel(int channel)
{
	Mix_FadeOutChannel(channel, 50);
}
#endif





gboolean
gd_sound_init()
{
#ifdef GD_SOUND
	int i;
	
	for (i=0; i<G_N_ELEMENTS(sound_playing); i++)
		sound_playing[i]=GD_S_NONE;
	
	if(Mix_OpenAudio(gd_sdl_44khz_mixing?44100:22050, gd_sdl_16bit_mixing?AUDIO_S16:AUDIO_U8, 1, 1024)==-1) {
		g_warning("%s", Mix_GetError());
		return FALSE;
	}
	mixer_started=TRUE;
	Mix_ChannelFinished(channel_done);
	
	loadsound(GD_S_AMOEBA, "amoeba.ogg");
	loadsound(GD_S_MAGIC_WALL, "magic_wall.ogg");
	loadsound(GD_S_CRACK, "crack.ogg");
	loadsound(GD_S_COVER, "cover.ogg");

	loadsound(GD_S_TIMEOUT_1, "timeout_1.ogg");
	loadsound(GD_S_TIMEOUT_2, "timeout_2.ogg");
	loadsound(GD_S_TIMEOUT_3, "timeout_3.ogg");
	loadsound(GD_S_TIMEOUT_4, "timeout_4.ogg");
	loadsound(GD_S_TIMEOUT_5, "timeout_5.ogg");
	loadsound(GD_S_TIMEOUT_6, "timeout_6.ogg");
	loadsound(GD_S_TIMEOUT_7, "timeout_7.ogg");
	loadsound(GD_S_TIMEOUT_8, "timeout_8.ogg");
	loadsound(GD_S_TIMEOUT_9, "timeout_9.ogg");
	loadsound(GD_S_FINISHED, "finished.ogg");

	loadsound(GD_S_EXPLOSION, "explosion.ogg");
	loadsound(GD_S_WALK_EARTH, "walk_earth.ogg");
	loadsound(GD_S_WALK_EMPTY, "walk_empty.ogg");
	loadsound(GD_S_PNEUMATIC_HAMMER, "pneumatic.ogg");
	loadsound(GD_S_DOOR_OPEN, "door_open.ogg");
	loadsound(GD_S_STIRRING, "stirring.ogg");
	loadsound(GD_S_DIAMOND_COLLECT, "diamond_collect.ogg");
	loadsound(GD_S_SKELETON_COLLECT, "skeleton_collect.ogg");
	loadsound(GD_S_TELEPORTER, "teleporter.ogg");

	loadsound(GD_S_STONE, "stone.ogg");
	loadsound(GD_S_FALLING_WALL, "falling_wall.ogg");
	loadsound(GD_S_GROWING_WALL, "growing_wall.ogg");
	loadsound(GD_S_DIAMOND_1, "diamond_1.ogg");
	loadsound(GD_S_DIAMOND_2, "diamond_2.ogg");
	loadsound(GD_S_DIAMOND_3, "diamond_3.ogg");
	loadsound(GD_S_DIAMOND_4, "diamond_4.ogg");
	loadsound(GD_S_DIAMOND_5, "diamond_5.ogg");
	loadsound(GD_S_DIAMOND_6, "diamond_6.ogg");
	loadsound(GD_S_DIAMOND_7, "diamond_7.ogg");
	loadsound(GD_S_DIAMOND_8, "diamond_8.ogg");

	return TRUE;
#else
	/* if compiled without sound support, return TRUE, "sound init successful" */
	return TRUE;
#endif
}

void
gd_no_sound()
{
#ifdef GD_SOUND
	int i;
	
	if (!mixer_started)
		return;

	/* if any of the channels are playing looped sounds, that should be stopped. */
	/* non-looping sounds are not stopped; they will go away automatically with time. */
	for (i=0; i<G_N_ELEMENTS(sound_playing); i++) {
		if (is_sound_looped(sound_playing[i]))
			halt_channel(i);
	}
#endif
}

void
gd_play_sounds(GdSound sound1, GdSound sound2, GdSound sound3)
{
#ifdef GD_SOUND
	if (!mixer_started || !gd_sdl_sound)
		return;
	
	/* CHANNEL 3 is for crack sound, amoeba and magic wall. */
	if (sound3!=GD_S_NONE) {
		if (sound3==GD_S_CRACK) /* crack sound */
			play_sound(3, sound3);
		else {
			/* only play, if another sound is requested, ie. != the previous one. otherwise it would not loop */
			/* also, do not interrupt a crack sound */
			if (sound_playing[3]!=GD_S_CRACK && sound3!=sound_playing[3])
				play_sound(3, sound3);
		}
	} else {
		/* sound3=none, so interrupt sound requested */
		if (sound_playing[3]!=GD_S_CRACK)	/* do not interrupt crack sound */
			halt_channel(3);
	}

	
	/* CHANNEL 2 is for walking, explosions */
	/* if no sound requested, do nothing. */
	if (sound2!=GD_S_NONE) {
		/* 1) always restart explosion sound. */
		/* 2) play other sound only if not currently playing an explosion */
		if (sound2==GD_S_EXPLOSION || sound_playing[2]!=GD_S_EXPLOSION)
			play_sound(2, sound2);
	} else {
		/* pneumatic hammer is looped. stop it, if requested. */
		if (sound_playing[2]==GD_S_PNEUMATIC_HAMMER)
			halt_channel(2);
	}

	
	/* CHANNEL 1 is for small sounds */
	if (sound1!=GD_S_NONE)
		play_sound(1, sound1);
#endif
}

