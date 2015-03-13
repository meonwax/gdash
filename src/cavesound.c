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
#include <glib.h>
#include "config.h"
#include "cave.h"

#ifdef GD_SOUND
typedef enum _sound_flag {
	GD_SP_LOOPED = 1<<0,
	GD_SP_CLASSIC = 1<<1,
	GD_SP_FORCE = 1<<2,	/* force restart, regardless of precedence level */
	GD_SP_FAKE = 1<<3,	/* not real sounds. for example, diamond_random, which is changed to diamond_1, 2, 3, ... later */
} GdSoundFlag;

typedef struct _sound_property {
	GdSound sound;
	int flags;
	int channel;		/* channel this sound is played on. */
	int precedence;		/* greater numbers will have precedence. */
	GdSound replace;	/* replacement sound, if classic sounds are requested but this one is not classic */
} SoundProperty;

static SoundProperty sound_flags[] = {
	{ GD_S_NONE, GD_SP_CLASSIC, 0, 0, 0},

	/* channel 1 sounds. */
	/* diamond collect sound has precedence over everything. */
	/* CHANNEL 1 SOUNDS ARE ALWAYS RESTARTED, so no need for GD_SP_FORCE flag. */
	{ GD_S_STONE, GD_SP_CLASSIC, 1, 10},
	{ GD_S_FALLING_WALL, 0, 1, 10, GD_S_STONE},
	{ GD_S_GROWING_WALL, 0, 1, 10, GD_S_STONE},
	{ GD_S_DIAMOND_RANDOM, GD_SP_CLASSIC|GD_SP_FAKE, 1, 10},
	{ GD_S_DIAMOND_1, GD_SP_CLASSIC, 1, 10},
	{ GD_S_DIAMOND_2, GD_SP_CLASSIC, 1, 10},
	{ GD_S_DIAMOND_3, GD_SP_CLASSIC, 1, 10},
	{ GD_S_DIAMOND_4, GD_SP_CLASSIC, 1, 10},
	{ GD_S_DIAMOND_5, GD_SP_CLASSIC, 1, 10},
	{ GD_S_DIAMOND_6, GD_SP_CLASSIC, 1, 10},
	{ GD_S_DIAMOND_7, GD_SP_CLASSIC, 1, 10},
	{ GD_S_DIAMOND_8, GD_SP_CLASSIC, 1, 10},
	{ GD_S_DIAMOND_COLLECT, GD_SP_CLASSIC, 1, 100},			/* collect sounds have higher precedence than falling sounds and the like. */
	{ GD_S_SKELETON_COLLECT, 0, 1, 100, GD_S_DIAMOND_COLLECT},
	{ GD_S_PNEUMATIC_COLLECT, 0, 1, 50, GD_S_DIAMOND_RANDOM},
	{ GD_S_BOMB_COLLECT, 0, 1, 50, GD_S_DIAMOND_RANDOM},
	{ GD_S_CLOCK_COLLECT, GD_SP_CLASSIC, 1, 50},
	{ GD_S_SWEET_COLLECT, 0, 1, 50, GD_S_NONE},
	{ GD_S_KEY_COLLECT, 0, 1, 50, GD_S_DIAMOND_RANDOM},
	{ GD_S_SLIME, 0,  1, 5, GD_S_NONE},		/* slime has lower precedence than diamond and stone falling sounds. */
	{ GD_S_ACID_SPREAD, 0,  1, 3, GD_S_NONE},	/* same for acid, even lower. */
	{ GD_S_BLADDER_MOVE, 0, 1, 5, GD_S_NONE},	/* same for bladder. */
	{ GD_S_BLADDER_CONVERT, 0, 1, 8, GD_S_NONE},
	{ GD_S_BLADDER_SPENDER, 0, 1, 8, GD_S_NONE},
	{ GD_S_BITER_EAT, 0, 1, 3, GD_S_NONE},		/* very low precedence. biters tend to produce too much sound. */

	/* channel2 sounds. */
	{ GD_S_DOOR_OPEN, GD_SP_CLASSIC, 2, 10},
	{ GD_S_WALK_EARTH, GD_SP_CLASSIC, 2, 10},
	{ GD_S_WALK_EMPTY, GD_SP_CLASSIC, 2, 10},
	{ GD_S_STIRRING, GD_SP_CLASSIC, 2, 10},
	{ GD_S_BOX_PUSH, 0, 2, 10, GD_S_STONE},
	{ GD_S_TELEPORTER, 0, 2, 10, GD_S_NONE},
	{ GD_S_TIMEOUT_1, GD_SP_CLASSIC, 2, 20},	/* timeout sounds have increasing precedence so they are always started */
	{ GD_S_TIMEOUT_2, GD_SP_CLASSIC, 2, 21},
	{ GD_S_TIMEOUT_3, GD_SP_CLASSIC, 2, 22},
	{ GD_S_TIMEOUT_4, GD_SP_CLASSIC, 2, 23},
	{ GD_S_TIMEOUT_5, GD_SP_CLASSIC, 2, 24},
	{ GD_S_TIMEOUT_6, GD_SP_CLASSIC, 2, 25},
	{ GD_S_TIMEOUT_7, GD_SP_CLASSIC, 2, 26},
	{ GD_S_TIMEOUT_8, GD_SP_CLASSIC, 2, 27},
	{ GD_S_TIMEOUT_9, GD_SP_CLASSIC, 2, 28},
	{ GD_S_TIMEOUT, GD_SP_FORCE, 2, 150, GD_S_NONE},
	{ GD_S_EXPLOSION, GD_SP_CLASSIC|GD_SP_FORCE, 2, 100},
	{ GD_S_BOMB_EXPLOSION, GD_SP_FORCE, 2, 100, GD_S_EXPLOSION},
	{ GD_S_GHOST_EXPLOSION, GD_SP_FORCE, 2, 100, GD_S_EXPLOSION},
	{ GD_S_VOODOO_EXPLOSION, GD_SP_FORCE, 2, 100, GD_S_EXPLOSION},
	{ GD_S_BOMB_PLACE, 0, 2, 10, GD_S_NONE},
	{ GD_S_FINISHED, GD_SP_CLASSIC|GD_SP_FORCE|GD_SP_LOOPED, 2, 15},
	{ GD_S_SWITCH_BITER, 0, 2, 10, GD_S_NONE},
	{ GD_S_SWITCH_CREATURES, 0, 2, 10, GD_S_NONE},
	{ GD_S_SWITCH_GRAVITY, 0, 2, 10, GD_S_NONE},
	{ GD_S_SWITCH_GROWING, 0, 2, 10, GD_S_NONE},

	/* channel 3 sounds. */
	{ GD_S_AMOEBA, GD_SP_CLASSIC|GD_SP_LOOPED, 3, 30},
	{ GD_S_MAGIC_WALL, GD_SP_CLASSIC|GD_SP_LOOPED, 3, 40},
	{ GD_S_COVER, GD_SP_CLASSIC|GD_SP_LOOPED, 3, 100},
	{ GD_S_PNEUMATIC_HAMMER, GD_SP_CLASSIC|GD_SP_LOOPED, 3, 50},
	{ GD_S_WATER, GD_SP_LOOPED, 3, 20, GD_S_NONE},
	{ GD_S_CRACK, GD_SP_CLASSIC, 3, 150},
	{ GD_S_GRAVITY_CHANGE, 0, 3, 60, GD_S_NONE},

	/* other sounds */
	/* the bonus life sound has nothing to do with the cave. */
	/* playing on channel 4. */
	{ GD_S_BONUS_LIFE, 0, 4, 0, GD_S_NONE},
};
#endif




/*
  some sound things
 */
#ifdef GD_SOUND
gboolean
gd_sound_is_looped(GdSound sound)
{
	g_assert(sound>=GD_S_NONE && sound<GD_S_MAX);
	return (sound_flags[sound].flags&GD_SP_LOOPED)!=0;
}

gboolean
gd_sound_is_fake(GdSound sound)
{
	g_assert(sound>=GD_S_NONE && sound<GD_S_MAX);
	return (sound_flags[sound].flags&GD_SP_FAKE)!=0;
}

gboolean
gd_sound_is_classic(GdSound sound)
{
	g_assert(sound>=GD_S_NONE && sound<GD_S_MAX);
	return (sound_flags[sound].flags&GD_SP_CLASSIC)!=0;
}

gboolean
gd_sound_force_start(GdSound sound)
{
	g_assert(sound>=GD_S_NONE && sound<GD_S_MAX);
	return (sound_flags[sound].flags&GD_SP_FORCE)!=0;
}


gboolean
gd_sound_classic_equivalent(GdSound sound)
{
	g_assert(sound>=GD_S_NONE && sound<GD_S_MAX);
	if (gd_sound_is_classic(sound))
		return sound;
	
	/* replacement */
	return sound_flags[sound].replace;
}

int
gd_sound_get_channel(GdSound sound)
{
	g_assert(sound>=GD_S_NONE && sound<GD_S_MAX);
	
	return sound_flags[sound].channel;
}

int
gd_sound_get_precedence(GdSound sound)
{
	g_assert(sound>=GD_S_NONE && sound<GD_S_MAX);
	
	return sound_flags[sound].precedence;
}
#endif






void
gd_cave_sound_db_init()
{
#ifdef GD_SOUND
	/* only if sound compiled in. otherwise does nothing. */
	int i;
	
	g_assert(G_N_ELEMENTS(sound_flags)==GD_S_MAX);
	
	for (i=0; i<GD_S_MAX; i++) {
		if (sound_flags[i].sound!=i) {
			g_critical("sound db index mismatch: %d", i);
			g_assert_not_reached();
		}
		
		if (!gd_sound_is_classic(i)) {
			/* if it is a non classic sound */
			if (!gd_sound_is_classic(gd_sound_classic_equivalent(i))) {
				g_critical("replacement for a non-classic sound must be classic: %d", i);
				g_assert_not_reached();
			}
		}
		
		if (i>GD_S_NONE && (gd_sound_get_channel(i)<1 || gd_sound_get_channel(i)>4)) {
			g_critical("channel must be 1<=c<=3 for sound %d", i);
			g_assert_not_reached();
		}
		
		
	}
	
#endif
}

/* plays sound in a cave. returns true, if sound will be played */
void
gd_sound_play(Cave *cave, GdSound sound)
{
#ifdef GD_SOUND
	GdSound *s;
	
	if (sound==GD_S_NONE)
		return;

	switch(gd_sound_get_channel(sound)) {
		case 1: s=&cave->sound1; break;
		case 2: s=&cave->sound2; break;
		case 3: s=&cave->sound3; break;
		default:
			g_assert_not_reached();
	}
	
	if (gd_sound_get_precedence(sound)>=gd_sound_get_precedence(*s))
		*s=sound;
#endif
}



