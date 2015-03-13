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
#include <glib.h>
#include "config.h"
#include "cave.h"

#ifdef GD_SOUND
typedef enum _sound_flag {
    GD_SP_LOOPED = 1<<0,
    GD_SP_CLASSIC = 1<<1,
    GD_SP_FORCE = 1<<2,    /* force restart, regardless of precedence level */
    GD_SP_FAKE = 1<<3,    /* not real sounds. for example, diamond_random, which is changed to diamond_1, 2, 3, ... later */
} GdSoundFlag;

typedef struct _sound_property {
    GdSound sound;
    const char *filename;
    int flags;
    int channel;        /* channel this sound is played on. */
    int precedence;        /* greater numbers will have precedence. */
    GdSound replace;    /* replacement sound, if classic sounds are requested but this one is not classic */
} SoundProperty;

static SoundProperty sound_flags[] = {
    { GD_S_NONE, NULL, GD_SP_CLASSIC, 0, 0, 0},

    /* channel 1 sounds. */
    /* diamond collect sound has precedence over everything. */
    /* CHANNEL 1 SOUNDS ARE ALWAYS RESTARTED, so no need for GD_SP_FORCE flag. */
    { GD_S_STONE, "stone.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_NUT, "nut.ogg", 0, 1, 8},    /* nut falling is relatively silent, so low precedence. */
    { GD_S_NUT_CRACK, "nut_crack.ogg", 0, 1, 12},    /* higher precedence than a stone bouncing. */
    { GD_S_DIRT_BALL, "dirt_ball.ogg", 0, 1, 8},    /* sligthly lower precedence, as stones and diamonds should be "louder" */
    { GD_S_NITRO, "nitro.ogg", 0, 1, 10},
    { GD_S_FALLING_WALL, "falling_wall.ogg", 0, 1, 10, GD_S_STONE},
    { GD_S_EXPANDING_WALL, "expanding_wall.ogg", 0, 1, 10, GD_S_STONE},
    { GD_S_WALL_REAPPEAR, "wall_reappear.ogg", 0, 1, 9},
    { GD_S_DIAMOND_RANDOM, NULL, GD_SP_CLASSIC|GD_SP_FAKE, 1, 10},
    { GD_S_DIAMOND_1, "diamond_1.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_DIAMOND_2, "diamond_2.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_DIAMOND_3, "diamond_3.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_DIAMOND_4, "diamond_4.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_DIAMOND_5, "diamond_5.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_DIAMOND_6, "diamond_6.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_DIAMOND_7, "diamond_7.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_DIAMOND_8, "diamond_8.ogg", GD_SP_CLASSIC, 1, 10},
    { GD_S_DIAMOND_COLLECT, "diamond_collect.ogg", GD_SP_CLASSIC, 1, 100},            /* collect sounds have higher precedence than falling sounds and the like. */
    { GD_S_SKELETON_COLLECT, "skeleton_collect.ogg", 0, 1, 100, GD_S_DIAMOND_COLLECT},
    { GD_S_PNEUMATIC_COLLECT, "pneumatic_collect.ogg", 0, 1, 50, GD_S_DIAMOND_RANDOM},
    { GD_S_BOMB_COLLECT, "bomb_collect.ogg", 0, 1, 50, GD_S_DIAMOND_RANDOM},
    { GD_S_CLOCK_COLLECT, "clock_collect.ogg", GD_SP_CLASSIC, 1, 50},
    { GD_S_SWEET_COLLECT, "sweet_collect.ogg", 0, 1, 50, GD_S_NONE},
    { GD_S_KEY_COLLECT, "key_collect.ogg", 0, 1, 50, GD_S_DIAMOND_RANDOM},
    { GD_S_DIAMOND_KEY_COLLECT, "diamond_key_collect.ogg", 0, 1, 50, GD_S_DIAMOND_RANDOM},
    { GD_S_SLIME, "slime.ogg", 0, 1, 5, GD_S_NONE},        /* slime has lower precedence than diamond and stone falling sounds. */
    { GD_S_LAVA, "lava.ogg", 0, 1, 5, GD_S_NONE},        /* lava has low precedence, too. */
    { GD_S_REPLICATOR, "replicator.ogg", 0, 1, 5, GD_S_NONE},
    { GD_S_ACID_SPREAD, "acid_spread.ogg", 0,  1, 3, GD_S_NONE},    /* same for acid, even lower. */
    { GD_S_BLADDER_MOVE, "bladder_move.ogg", 0, 1, 5, GD_S_NONE},    /* same for bladder. */
    { GD_S_BLADDER_CONVERT, "bladder_convert.ogg", 0, 1, 8, GD_S_NONE},
    { GD_S_BLADDER_SPENDER, "bladder_spender.ogg", 0, 1, 8, GD_S_NONE},
    { GD_S_BITER_EAT, "biter_eat.ogg", 0, 1, 3, GD_S_NONE},        /* very low precedence. biters tend to produce too much sound. */

    /* channel2 sounds. */
    { GD_S_DOOR_OPEN, "door_open.ogg", GD_SP_CLASSIC, 2, 10},
    { GD_S_WALK_EARTH, "walk_earth.ogg", GD_SP_CLASSIC, 2, 10},
    { GD_S_WALK_EMPTY, "walk_empty.ogg", GD_SP_CLASSIC, 2, 10},
    { GD_S_STIRRING, "stirring.ogg", GD_SP_CLASSIC, 2, 10},
    { GD_S_BOX_PUSH, "box_push.ogg", 0, 2, 10, GD_S_STONE},
    { GD_S_TELEPORTER, "teleporter.ogg", 0, 2, 10, GD_S_NONE},
    { GD_S_TIMEOUT_1, "timeout_1.ogg", GD_SP_CLASSIC, 2, 20},    /* timeout sounds have increasing precedence so they are always started */
    { GD_S_TIMEOUT_2, "timeout_2.ogg", GD_SP_CLASSIC, 2, 21},    /* timeout sounds are examples which do not need "force restart" flag. */
    { GD_S_TIMEOUT_3, "timeout_3.ogg", GD_SP_CLASSIC, 2, 22},
    { GD_S_TIMEOUT_4, "timeout_4.ogg", GD_SP_CLASSIC, 2, 23},
    { GD_S_TIMEOUT_5, "timeout_5.ogg", GD_SP_CLASSIC, 2, 24},
    { GD_S_TIMEOUT_6, "timeout_6.ogg", GD_SP_CLASSIC, 2, 25},
    { GD_S_TIMEOUT_7, "timeout_7.ogg", GD_SP_CLASSIC, 2, 26},
    { GD_S_TIMEOUT_8, "timeout_8.ogg", GD_SP_CLASSIC, 2, 27},
    { GD_S_TIMEOUT_9, "timeout_9.ogg", GD_SP_CLASSIC, 2, 28},
    { GD_S_TIMEOUT, "timeout.ogg", GD_SP_FORCE, 2, 150, GD_S_NONE},
    { GD_S_EXPLOSION, "explosion.ogg", GD_SP_CLASSIC|GD_SP_FORCE, 2, 100},
    { GD_S_BOMB_EXPLOSION, "bomb_explosion.ogg", GD_SP_FORCE, 2, 100, GD_S_EXPLOSION},
    { GD_S_GHOST_EXPLOSION, "ghost_explosion.ogg", GD_SP_FORCE, 2, 100, GD_S_EXPLOSION},
    { GD_S_VOODOO_EXPLOSION, "voodoo_explosion.ogg", GD_SP_FORCE, 2, 100, GD_S_EXPLOSION},
    { GD_S_NITRO_EXPLOSION, "nitro_explosion.ogg", GD_SP_FORCE, 2, 100, GD_S_EXPLOSION},
    { GD_S_BOMB_PLACE, "bomb_place.ogg", 0, 2, 10, GD_S_NONE},
    { GD_S_FINISHED, "finished.ogg", GD_SP_CLASSIC|GD_SP_FORCE|GD_SP_LOOPED, 2, 15},    /* precedence larger than normal, but smaller than timeout sounds */
    { GD_S_SWITCH_BITER, "switch_biter.ogg", 0, 2, 10, GD_S_NONE},
    { GD_S_SWITCH_CREATURES, "switch_creatures.ogg", 0, 2, 10, GD_S_NONE},
    { GD_S_SWITCH_GRAVITY, "switch_gravity.ogg", 0, 2, 10, GD_S_NONE},
    { GD_S_SWITCH_EXPANDING, "switch_expanding.ogg", 0, 2, 10, GD_S_NONE},
    { GD_S_SWITCH_CONVEYOR, "switch_conveyor.ogg", 0, 2, 10, GD_S_NONE},
    { GD_S_SWITCH_REPLICATOR, "switch_replicator.ogg", 0, 2, 10, GD_S_NONE},

    /* channel 3 sounds. */
    { GD_S_AMOEBA, "amoeba.ogg", GD_SP_CLASSIC|GD_SP_LOOPED, 3, 30},
    { GD_S_AMOEBA_MAGIC, "amoeba_and_magic.ogg", GD_SP_CLASSIC|GD_SP_LOOPED, 3, 40},
    { GD_S_MAGIC_WALL, "magic_wall.ogg", GD_SP_CLASSIC|GD_SP_LOOPED, 3, 35},
    { GD_S_COVER, "cover.ogg", GD_SP_CLASSIC|GD_SP_LOOPED, 3, 100},
    { GD_S_PNEUMATIC_HAMMER, "pneumatic.ogg", GD_SP_CLASSIC|GD_SP_LOOPED, 3, 50},
    { GD_S_WATER, "water.ogg", GD_SP_LOOPED, 3, 20, GD_S_NONE},
    { GD_S_CRACK, "crack.ogg", GD_SP_CLASSIC, 3, 150},
    { GD_S_GRAVITY_CHANGE, "gravity_change.ogg", 0, 3, 60, GD_S_NONE},

    /* other sounds */
    /* the bonus life sound has nothing to do with the cave. */
    /* playing on channel 4. */
    { GD_S_BONUS_LIFE, "bonus_life.ogg", 0, 4, 0, GD_S_NONE},
};
#endif




/*
  some sound things
 */
#ifdef GD_SOUND
const char *
gd_sound_get_filename(GdSound sound)
{
    g_assert(sound>=GD_S_NONE && sound<GD_S_MAX);
    return sound_flags[sound].filename;
}

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
    
    /* check if the number of sounds in the array matches. */
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
            g_critical("channel must be 1<=c<=4 for sound %d", i);
            g_assert_not_reached();
        }
    }
#endif
}

/* plays sound in a cave */
void
gd_sound_play(GdCave *cave, GdSound sound)
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
    
    if (gd_sound_get_precedence(sound)>=gd_sound_get_precedence(*s)) {
//        g_print("sound: preferred %s over %s\n", sound_flags[sound].filename, sound_flags[*s].filename);
        *s=sound;
    }
#endif
}



