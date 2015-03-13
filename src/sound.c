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
#include "config.h"
#ifdef USE_SDL
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif
#include <glib.h>
#include "settings.h"
#include "cave.h"
#include "cavesound.h"
#include "util.h"

/*
	The C64 sound chip (the SID) had 3 channels. Boulder Dash used all 3 of them.
	
	Different channels were used for different sounds.
	Channel 1: other small sounds, ie. diamonds falling, boulders rolling.

	Channel 2: Walking, diamond collecting and explosion; also time running out sound.

	Channel 3: amoeba sound, magic wall sound,
		cave cover & uncover sound, and the crack sound (gate open)
	
	Sounds have precedence over each other. Ie. the crack sound is given precedence
	over other sounds (amoeba, for example.)
	Different channels also behave differently. Channel 2 sounds are not stopped, ie.
	walking can not be heard, until the explosion sound is finished playing completely.
	Explosions are always restarted, though. This is controlled by the array defined
	in cave.c.
	Channel 1 sounds are always stopped, if a new sound is requested.
	
	Here we implement this a bit differently. We use samples, instead of synthesizing
	the sounds. By stopping samples, sometimes small clicks generate. Therefore we do
	not stop them, rather fade them out quickly. (The SID had filters, which stopped
	these small clicks.)
	Also, channel 1 and 2 should be stopped very often. So I decided to use two
	SDL_Mixer channels to emulate one C64 channel; and they are used alternating.
	SDL channel 4 is the "backup" channel 1, channel 5 is the backup channel 2.
	Other channels have the same indexes.

 */



#ifdef GD_SOUND
static Mix_Chunk *sounds[GD_S_MAX];
static gboolean mixer_started=FALSE;
static GdSound snd_playing[5];
static Mix_Music *music=NULL;

static int chunk_volumes=MIX_MAX_VOLUME;
static int music_volume=MIX_MAX_VOLUME;
#endif

#ifdef GD_SOUND
static GdSound
sound_playing(int channel)
{
	return snd_playing[channel];
}
#endif

void
gd_sound_set_music_volume(int percent)
{
#ifdef GD_SOUND
	music_volume=MIX_MAX_VOLUME*percent/100;
	Mix_VolumeMusic(music_volume);
#endif
}

void
gd_sound_set_chunk_volumes(int percent)
{
#ifdef GD_SOUND
	int i;
	
	chunk_volumes=MIX_MAX_VOLUME*percent/100;
	
	for (i=0; i<G_N_ELEMENTS(sounds); i++) {
		if (sounds[i]!=NULL)
			Mix_VolumeChunk(sounds[i], chunk_volumes);
	}
#endif
}

#ifdef GD_SOUND
static void
loadsound(GdSound which, const char *filename)
{
	const char *full_filename;

	g_assert(!gd_sound_is_fake(which));	
	g_assert(mixer_started);
	/* make sure sound isn't already loaded */	
	g_assert(sounds[which]==NULL);
	
	full_filename=gd_find_file(filename);
	if (!full_filename)	{/* if cannot find file, exit now */
		g_warning("No such sound file: %s", filename);
		return;
	}
		
	sounds[which]=Mix_LoadWAV(full_filename);
	if (sounds[which]==NULL)
		g_warning("%s: %s", filename, Mix_GetError());
	else
		Mix_VolumeChunk(sounds[which], chunk_volumes);
}
#endif




#ifdef GD_SOUND
/* this is called by sdl for an sdl channel. so no magic with channel_x_alter. */
static void
channel_done(int channel)
{
	snd_playing[channel]=GD_S_NONE;
}
#endif

#ifdef GD_SOUND
static void
halt_channel(int channel)
{
	Mix_FadeOutChannel(channel, 40);
}
#endif

#ifdef GD_SOUND
static void
play_sound(int channel, GdSound sound)
{
	/* channel 1 and channel 4 are used alternating */
	/* channel 2 and channel 5 are used alternating */
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

	/* if the user likes classic sounds */
	if (gd_classic_sound)
		if (!gd_sound_is_classic(sound))
			sound=gd_sound_classic_equivalent(sound);
	/* no sound is possible, as not all sounds have classic equivalent */	
	if (sound==GD_S_NONE)
		return;

	/* change diamond falling random to a selected diamond falling sound. */
	if (sound==GD_S_DIAMOND_RANDOM)
		sound=diamond_sounds[g_random_int_range(0, G_N_ELEMENTS(diamond_sounds))];
	
	/* at this point, fake sounds should have been changed to normal sounds */
	g_assert(!gd_sound_is_fake(sound));
	
	/* channel 1 may have been changed to channel 4 above. */
	Mix_PlayChannel(channel, sounds[sound], gd_sound_is_looped(sound)?-1:0);
	Mix_Volume(channel, MIX_MAX_VOLUME);
	snd_playing[channel]=sound;
}
#endif





gboolean
gd_sound_init(unsigned int bufsize)
{
#ifdef GD_SOUND
	int i;
	
	for (i=0; i<G_N_ELEMENTS(snd_playing); i++)
		snd_playing[i]=GD_S_NONE;
	
	/* default buffer size. */
	if (bufsize==0)
		bufsize=1024;
	if(Mix_OpenAudio(gd_sdl_44khz_mixing?44100:22050, gd_sdl_16bit_mixing?AUDIO_S16:AUDIO_U8, 2, bufsize)==-1) {
		g_warning("%s", Mix_GetError());
		return FALSE;
	}
	mixer_started=TRUE;
	/* add callback when a sound stops playing */
	Mix_ChannelFinished(channel_done);
	
	for (i=0; i<GD_S_MAX; i++)
		if (gd_sound_get_filename(i)!=NULL)
			loadsound(i, gd_sound_get_filename(i));
	
	return TRUE;
#else
	/* if compiled without sound support, return TRUE, "sound init successful" */
	return TRUE;
#endif
}

void
gd_sound_off()
{
#ifdef GD_SOUND
	int i;
	
	if (!mixer_started)
		return;

	/* stop all sounds. */
	for (i=0; i<G_N_ELEMENTS(snd_playing); i++)
		halt_channel(i);
#endif
}

void
gd_sound_play_bonus_life()
{
#ifdef GD_SOUND
	if (!mixer_started || !gd_sdl_sound)
		return;

	play_sound(gd_sound_get_channel(GD_S_BONUS_LIFE), GD_S_BONUS_LIFE);
#endif
}

static void
play_sounds(GdSound sound1, GdSound sound2, GdSound sound3)
{
#ifdef GD_SOUND
	if (!mixer_started || !gd_sdl_sound)
		return;

	/* CHANNEL 1 is for small sounds */
	if (sound1!=GD_S_NONE) {
		/* start new sound if higher or same precedence than the one currently playing */
		if (gd_sound_get_precedence(sound1)>=gd_sound_get_precedence(sound_playing(1)))
			play_sound(1, sound1);
	} else {
		/* only interrupt looped sounds. non-looped sounds will go away automatically. */
		if (gd_sound_is_looped(sound_playing(1)))
			halt_channel(1);
	}
	
	/* CHANNEL 2 is for walking, explosions */
	/* if no sound requested, do nothing. */
	if (sound2!=GD_S_NONE) {
		gboolean play=FALSE;
		
		/* always start if not currently playing a sound. */
		if (sound_playing(2)==GD_S_NONE
			|| gd_sound_force_start(sound2)
			|| gd_sound_get_precedence(sound2)>gd_sound_get_precedence(sound_playing(2)))
			play=TRUE;

		/* if figured out to play: do it. */
		/* if the requested sound is looped, and already playing, forget the request. */
		if (play && !(gd_sound_is_looped(sound2) && sound2==sound_playing(2)))
			play_sound(2, sound2);
	} else {
		/* only interrupt looped sounds. non-looped sounds will go away automatically. */
		if (gd_sound_is_looped(sound_playing(2)))
			halt_channel(2);
	}
	
	/* CHANNEL 3 is for crack sound, amoeba and magic wall. */
	if (sound3!=GD_S_NONE) {
		/* if requests a non-looped sound, play that immediately. that can be a crack sound, gravity change, new life, ... */
		if (!gd_sound_is_looped(sound3))
			play_sound(3, sound3);
		else {
			/* if the sound is looped, play it, but only if != previous one. if they are equal,
			   the sound is looped, and already playing, no need to touch it. */
			/* also, do not interrupt the previous sound, if it is non-looped. later calls of this function will probably
			   contain the same sound3, and then it will be set. */
			if (sound_playing(3)==GD_S_NONE || (sound3!=sound_playing(3) && gd_sound_is_looped(sound_playing(3))))
				play_sound(3, sound3);
		}
	} else {
		/* sound3=none, so interrupt sound requested. */
		/* only interrupt looped sounds. non-looped sounds will go away automatically. */
		if (gd_sound_is_looped(sound_playing(3)))
			halt_channel(3);
	}
#endif
}

void
gd_sound_play_cave(GdCave *cave)
{
	play_sounds(cave->sound1, cave->sound2, cave->sound3);
}




void
gd_music_play_random()
{
#ifdef GD_SOUND
	static GPtrArray *music_filenames=NULL;

	if (!mixer_started || !gd_sdl_sound)
		return;

	/* if already playing, do nothing */	
	if (Mix_PlayingMusic())
		return;
		
	Mix_FreeMusic(music);
	music=NULL;

	if (!music_filenames) {
		const char *name;	/* name of file */
		const char *opened;		/* directory we use */
		GDir *musicdir;
		
		music_filenames=g_ptr_array_new();
		opened=gd_system_music_dir;
		musicdir=g_dir_open(opened, 0, NULL);
		if (!musicdir) {
			/* for testing: open "music" dir in current directory. */
			opened="music";
			musicdir=g_dir_open(opened, 0, NULL);
		}
		if (!musicdir)
			/* if still cannot open, return now */
			return;
		
		while ((name=g_dir_read_name(musicdir))) {
			if (g_str_has_suffix(name, ".ogg"))
				g_ptr_array_add(music_filenames, g_build_filename(opened, name, NULL));
		}
		g_dir_close(musicdir);
	}
	
	/* if loaded dir contents, but no file found */
	if (music_filenames->len==0)
		return;
	
	music=Mix_LoadMUS(g_ptr_array_index(music_filenames, g_random_int_range(0, music_filenames->len)));
	if (!music)
		g_warning("%s", SDL_GetError());
	if (music) {
		Mix_VolumeMusic(music_volume);
		Mix_PlayMusic(music, -1);
	}
#endif
}

void
gd_music_stop()
{
#ifdef GD_SOUND
	if (!mixer_started)
		return;
	
	if (Mix_PlayingMusic())
		Mix_HaltMusic();
#endif
}

