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
#ifndef _GD_CAVESOUND
#define _GD_CAVESOUND

#include "config.h"

enum GdSound {
    GD_S_NONE,

    GD_S_STONE,
    GD_S_DIRT_BALL,
    GD_S_NITRO,
    GD_S_FALLING_WALL,
    GD_S_EXPANDING_WALL,
    GD_S_WALL_REAPPEAR,
    GD_S_DIAMOND_RANDOM,    /* randomly select a diamond sound */
    GD_S_DIAMOND_1,
    GD_S_DIAMOND_2,
    GD_S_DIAMOND_3,
    GD_S_DIAMOND_4,
    GD_S_DIAMOND_5,
    GD_S_DIAMOND_6,
    GD_S_DIAMOND_7,
    GD_S_DIAMOND_8,
    GD_S_DIAMOND_COLLECT,
    GD_S_SKELETON_COLLECT,
    GD_S_PNEUMATIC_COLLECT,
    GD_S_BOMB_COLLECT,
    GD_S_CLOCK_COLLECT,
    GD_S_SWEET_COLLECT,
    GD_S_KEY_COLLECT,
    GD_S_DIAMOND_KEY_COLLECT,
    GD_S_SLIME,
    GD_S_LAVA,
    GD_S_REPLICATOR,
    GD_S_ACID_SPREAD,
    GD_S_BLADDER_MOVE,
    GD_S_BLADDER_CONVERT,
    GD_S_BLADDER_SPENDER,
    GD_S_BITER_EAT,
    GD_S_NUT,
    GD_S_NUT_CRACK,

    GD_S_DOOR_OPEN,
    GD_S_WALK_EARTH,
    GD_S_WALK_EMPTY,
    GD_S_STIRRING,
    GD_S_BOX_PUSH,
    GD_S_TELEPORTER,
    GD_S_TIMEOUT_1,
    GD_S_TIMEOUT_2,
    GD_S_TIMEOUT_3,
    GD_S_TIMEOUT_4,
    GD_S_TIMEOUT_5,
    GD_S_TIMEOUT_6,
    GD_S_TIMEOUT_7,
    GD_S_TIMEOUT_8,
    GD_S_TIMEOUT_9,
    GD_S_TIMEOUT,
    GD_S_EXPLOSION,
    GD_S_BOMB_EXPLOSION,
    GD_S_GHOST_EXPLOSION,
    GD_S_VOODOO_EXPLOSION,
    GD_S_NITRO_EXPLOSION,
    GD_S_BOMB_PLACE,
    GD_S_FINISHED,
    GD_S_SWITCH_BITER,
    GD_S_SWITCH_CREATURES,
    GD_S_SWITCH_GRAVITY,
    GD_S_SWITCH_EXPANDING,
    GD_S_SWITCH_CONVEYOR,
    GD_S_SWITCH_REPLICATOR,

    GD_S_AMOEBA,            /* loop */
    GD_S_MAGIC_WALL,        /* loop */
    GD_S_AMOEBA_MAGIC,      /* loop */
    GD_S_COVER,             /* loop */
    GD_S_PNEUMATIC_HAMMER,  /* loop */
    GD_S_WATER,             /* loop */
    GD_S_CRACK,
    GD_S_GRAVITY_CHANGE,
    GD_S_BONUS_LIFE,

    GD_S_MAX,
};

struct SoundWithPos {
    GdSound sound;                          ///< The sound
    int dx, dy;                             ///< Position relative to the active player
    SoundWithPos() {}
    SoundWithPos(GdSound sound, int dx, int dy): sound(sound), dx(dx), dy(dy) {}
};

const char *gd_sound_get_filename(GdSound sound);
bool gd_sound_is_looped(GdSound sound);
bool gd_sound_is_fake(GdSound sound);
bool gd_sound_is_classic(GdSound sound);
bool gd_sound_force_start(GdSound sound);
int gd_sound_get_channel(GdSound sound);
int gd_sound_get_precedence(GdSound sound);
GdSound gd_sound_classic_equivalent(GdSound sound);


#endif    /* CAVESOUND.H */

