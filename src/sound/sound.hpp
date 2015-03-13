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
#ifndef _HAVE_SDL
#define _HAVE_SDL

#include "config.h"

#include "cave/caverendered.hpp"

gboolean gd_sound_init(unsigned int bufsize=44100/25);
void gd_sound_close();

void gd_sound_off();
void gd_sound_play_sounds(SoundWithPos const &sound1, SoundWithPos const &sound2, SoundWithPos const &sound3);
void gd_sound_play_bonus_life();

void gd_music_play_random();
void gd_music_stop();

void gd_sound_set_music_volume(int percent);
void gd_sound_set_chunk_volumes(int percent);
void gd_sound_set_music_volume();
void gd_sound_set_chunk_volumes();

#endif
