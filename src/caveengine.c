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


/* IMPORTANT NOTES */

/*
 * LAVA.
 *
 * Lava absorbs everything going into it. Everything.
 * But it does not "pull" elements; only the things disappear which
 * _do_ go directly into it. So if the player steps into the lava,
 * he will die. If a dragonfly flies over it, it will not.
 *
 * This behavior is implemented in the is_space_dir and the store
 * functions. is_space_dir returns true for the lava, too. The store
 * function ignores any store requests into the lava.
 * The player_get function will also behave for lava as it does for space.
 */



#include <glib.h>
#include "cave.h"
#include "cavedb.h"
#include "cavesound.h"
#include "caveengine.h"




/* for gravity */
static const GdDirection ccw_eighth[]={ MV_STILL, MV_UP_LEFT, MV_UP, MV_UP_RIGHT, MV_RIGHT, MV_DOWN_RIGHT, MV_DOWN, MV_DOWN_LEFT };
static const GdDirection ccw_fourth[]={ MV_STILL, MV_LEFT, MV_UP_LEFT, MV_UP, MV_UP_RIGHT, MV_RIGHT, MV_DOWN_RIGHT, MV_DOWN, MV_DOWN_LEFT, MV_LEFT };
static const GdDirection cw_eighth[]={ MV_STILL, MV_UP_RIGHT, MV_RIGHT, MV_DOWN_RIGHT, MV_DOWN, MV_DOWN_LEFT, MV_LEFT, MV_UP_LEFT, MV_UP };
static const GdDirection cw_fourth[]={ MV_STILL, MV_RIGHT, MV_DOWN_RIGHT, MV_DOWN, MV_DOWN_LEFT, MV_LEFT, MV_UP_LEFT, MV_UP, MV_UP_RIGHT };

static const GdDirection opposite[]={ MV_STILL, MV_DOWN, MV_DOWN_LEFT, MV_LEFT, MV_UP_LEFT, MV_UP, MV_UP_RIGHT, MV_RIGHT, MV_DOWN_RIGHT };


/* sets timeout sound. */
void
gd_cave_set_seconds_sound(GdCave *cave)
{
    /* this is an integer division, so 0 seconds can be 0.5 seconds... */
    /* also, when this reaches 8, the player still has 8.9999 seconds. so the sound is played at almost t=9s. */
    switch(cave->time/cave->timing_factor) {
        case 8: gd_sound_play(cave, GD_S_TIMEOUT_1); break;
        case 7: gd_sound_play(cave, GD_S_TIMEOUT_2); break;
        case 6: gd_sound_play(cave, GD_S_TIMEOUT_3); break;
        case 5: gd_sound_play(cave, GD_S_TIMEOUT_4); break;
        case 4: gd_sound_play(cave, GD_S_TIMEOUT_5); break;
        case 3: gd_sound_play(cave, GD_S_TIMEOUT_6); break;
        case 2: gd_sound_play(cave, GD_S_TIMEOUT_7); break;
        case 1: gd_sound_play(cave, GD_S_TIMEOUT_8); break;
        case 0: gd_sound_play(cave, GD_S_TIMEOUT_9); break;
    }
}

/* play diamond or stone sound of given element. */
static void
play_sound_of_element(GdCave *cave, GdElement element)
{
    /* stone and diamond fall sounds. */
    switch(element) {
        case O_NUT:
        case O_NUT_F:
            if (cave->nut_sound)
                gd_sound_play(cave, GD_S_NUT);
            break;
        case O_STONE:
        case O_STONE_F:
        case O_FLYING_STONE:
        case O_FLYING_STONE_F:
        case O_MEGA_STONE:
        case O_MEGA_STONE_F:
        case O_WAITING_STONE:
        case O_CHASING_STONE:
            if (cave->stone_sound)
                gd_sound_play(cave, GD_S_STONE);
            break;

        case O_NITRO_PACK:
        case O_NITRO_PACK_F:
            if (cave->nitro_sound)
                gd_sound_play(cave, GD_S_NITRO);
            break;

        case O_FALLING_WALL:
        case O_FALLING_WALL_F:
            if (cave->falling_wall_sound)
                gd_sound_play(cave, GD_S_FALLING_WALL);
            break;

        case O_H_EXPANDING_WALL:
        case O_V_EXPANDING_WALL:
        case O_EXPANDING_WALL:
        case O_H_EXPANDING_STEEL_WALL:
        case O_V_EXPANDING_STEEL_WALL:
        case O_EXPANDING_STEEL_WALL:
            if (cave->expanding_wall_sound)
                gd_sound_play(cave, GD_S_EXPANDING_WALL);
            break;

        case O_DIAMOND:
        case O_DIAMOND_F:
        case O_FLYING_DIAMOND:
        case O_FLYING_DIAMOND_F:
            if (cave->diamond_sound)
                gd_sound_play(cave, GD_S_DIAMOND_RANDOM);
            break;

        case O_BLADDER_SPENDER:
            if (cave->bladder_spender_sound)
                gd_sound_play(cave, GD_S_BLADDER_SPENDER);
            break;

        case O_PRE_CLOCK_1:
            if (cave->bladder_convert_sound)
                gd_sound_play(cave, GD_S_BLADDER_CONVERT);
            break;

        case O_SLIME:
            if (cave->slime_sound)
                gd_sound_play(cave, GD_S_SLIME);
            break;

        case O_LAVA:
            if (cave->lava_sound)
                gd_sound_play(cave, GD_S_LAVA);
            break;

        case O_ACID:
            if (cave->acid_spread_sound)
                gd_sound_play(cave, GD_S_ACID_SPREAD);
            break;

        case O_BLADDER:
            if (cave->bladder_sound)
                gd_sound_play(cave, GD_S_BLADDER_MOVE);
            break;

        case O_BITER_1:
        case O_BITER_2:
        case O_BITER_3:
        case O_BITER_4:
            if (cave->biter_sound)
                gd_sound_play(cave, GD_S_BITER_EAT);
            break;

        case O_DIRT_BALL:
        case O_DIRT_BALL_F:
        case O_DIRT_LOOSE:
        case O_DIRT_LOOSE_F:
            gd_sound_play(cave, GD_S_DIRT_BALL);
            break;

        default:
            /* do nothing. */
            break;
    }
}






static inline GdElement*
getp(const GdCave *cave, const int x, const int y)
{
    return cave->getp(cave, x, y);
}

/*
   perfect (non-lineshifting) GET function. returns a pointer to a selected cave element by its coordinates.
 */
static inline GdElement*
getp_perfect(const GdCave *cave, const int x, const int y)
{
    /* (x+n) mod n: this wors also for x>=n and -n+1<x<0 */
    return &(cave->map[(y+cave->h)%cave->h][(x+cave->w)%cave->w]);
}

/*
  line shifting GET function; returns a pointer to the selected cave element.
  this is used to emulate the line-shifting behaviour of original games, so that
  the player entering one side will appear one row above or below on the other.
*/
static inline GdElement*
getp_shift(const GdCave *cave, int x, int y)
{
    if (x>=cave->w) {
        y++;
        x-=cave->w;
    } else
    if (x<0) {
        y--;
        x+=cave->w;
    }
    y=(y+cave->h)%cave->h;
    return &(cave->map[y][x]);
}


static inline GdElement
get(const GdCave *cave, const int x, const int y)
{
    return *getp(cave, x, y);
}

/* returns an element which is somewhere near x,y */
static inline GdElement
get_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return get(cave, x+gd_dx[dir], y+gd_dy[dir]);
}





static inline gboolean
explodes_by_hit_dir(const GdCave *cave, const int x, const int y, GdDirection dir)
{
    return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_EXPLODES_BY_HIT)!=0;
}

/* returns true if the element is not explodable, for example the steel wall */
static inline gboolean
non_explodable(const GdCave *cave, const int x, const int y)
{
    return (gd_elements[get(cave, x,y)&O_MASK].properties&P_NON_EXPLODABLE)!=0;
}

/* returns true if the element can be eaten by the amoeba, eg. space and dirt. */
static inline gboolean
amoeba_eats_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_AMOEBA_CONSUMES)!=0;
}

/* returns true if the element is sloped, so stones and diamonds roll down on it. for example a stone or brick wall */
static inline gboolean
sloped_dir(const GdCave *cave, const int x, const int y, const GdDirection dir, const GdDirection slop)
{
    switch (slop) {
        case MV_LEFT:
            return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_SLOPED_LEFT)!=0;
        case MV_RIGHT:
            return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_SLOPED_RIGHT)!=0;
        case MV_UP:
            return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_SLOPED_UP)!=0;
        case MV_DOWN:
            return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_SLOPED_DOWN)!=0;
        default:
            break;
    }

    return FALSE;
}

/* returns true if the element is sloped for bladder movement (brick=yes, diamond=no, for example) */
static inline gboolean
sloped_for_bladder_dir (const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_BLADDER_SLOPED)!=0;
}

static inline gboolean
blows_up_flies_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_BLOWS_UP_FLIES)!=0;
}

/* returns true if the element is a counter-clockwise creature */
static inline gboolean
rotates_ccw (const GdCave *cave, const int x, const int y)
{
    return (gd_elements[get(cave, x, y)&O_MASK].properties&P_CCW)!=0;
}

/* returns true if the element is a player */
static inline gboolean
is_player(const GdCave *cave, const int x, const int y)
{
    return (gd_elements[get(cave, x, y)&O_MASK].properties&P_PLAYER)!=0;
}

/* returns true if the element is a player */
static inline gboolean
is_player_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_PLAYER)!=0;
}

static inline gboolean
can_be_hammered_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_CAN_BE_HAMMERED)!=0;
}


/* returns true if the element is explodable and explodes to space, for example the player */
static inline gboolean
is_first_stage_of_explosion(const GdCave *cave, const int x, const int y)
{
    return (gd_elements[get(cave, x, y)&O_MASK].properties&P_EXPLOSION_FIRST_STAGE)!=0;
}

/* returns true if the element is moved by the conveyor belt */
static inline gboolean
moved_by_conveyor_top_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_MOVED_BY_CONVEYOR_TOP)!=0;
}

/* returns true if the element is moved by the conveyor belt */
static inline gboolean
moved_by_conveyor_bottom_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_MOVED_BY_CONVEYOR_BOTTOM)!=0;
}

static inline gboolean
is_scanned_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    return (get_dir(cave, x, y, dir)&SCANNED) != 0;
}






/* returns true if neighbouring element is "e" */
/* treats dirt specially */
/* treats lava specially */
static inline gboolean
is_element_dir(const GdCave *cave, const int x, const int y, const GdDirection dir, GdElement e)
{
    GdElement examined=get_dir(cave, x, y, dir);

    /* if it is a dirt-like, change to dirt, so equality will evaluate to true */
    if (gd_elements[examined & O_MASK].properties & P_DIRT)
        examined=O_DIRT;
    if (gd_elements[e & O_MASK].properties & P_DIRT)
        e=O_DIRT;
    /* if the element on the map is a lava, it should be like space */
    if (examined==O_LAVA)
        examined=O_SPACE;
    return e==examined;
}

/* returns true if neighbouring element is space */
static inline gboolean
is_space_dir(const GdCave *cave, const int x, const int y, const GdDirection dir)
{
    GdElement e=get_dir(cave, x, y, dir)&O_MASK;

    return e==O_SPACE || e==O_LAVA;
}




/* store an element at the given position */
static inline void
store(GdCave *cave, const int x, const int y, const GdElement element)
{
    GdElement *e=getp(cave, x, y);

    if (*e==O_LAVA) {
        play_sound_of_element(cave, O_LAVA);
        return;
    }
    *e=element;
}

/* store an element with SCANNED flag turned on */
static inline void
store_sc(GdCave *cave, const int x, const int y, const GdElement element)
{
    store(cave, x, y, element|SCANNED);
}

/* store an element to a neighbouring cell */
static inline void
store_dir(GdCave *cave, const int x, const int y, const GdDirection dir, const GdElement element)
{
    store(cave, x+gd_dx[dir], y+gd_dy[dir], element|SCANNED);
}

/* store an element to a neighbouring cell */
static inline void
store_dir_no_scanned(GdCave *cave, const int x, const int y, const GdDirection dir, const GdElement element)
{
    store(cave, x+gd_dx[dir], y+gd_dy[dir], element);
}

/* move element to direction; then place space at x, y */
static inline void
move(GdCave *cave, const int x, const int y, const GdDirection dir, const GdElement e)
{
    store_dir(cave, x, y, dir, e);
    store(cave, x, y, O_SPACE);
}

/* increment a cave element; can be used for elements which are one after the other, for example bladder1, bladder2, bladder3... */
static inline void
next(GdCave *cave, const int x, const int y)
{
    (*getp(cave, x, y))++;
}
















static void
cell_explode(GdCave *cave, int x, int y, GdElement explode_to)
{
    if (non_explodable (cave, x, y))
        return;

    if (cave->voodoo_any_hurt_kills_player && get(cave, x, y)==O_VOODOO)
        cave->voodoo_touched=TRUE;

    if (get(cave, x, y)==O_VOODOO && !cave->voodoo_disappear_in_explosion)
        /* voodoo turns into a time penalty */
        store_sc(cave, x, y, O_TIME_PENALTY);
    else if (get(cave, x, y)==O_NITRO_PACK || get(cave, x, y)==O_NITRO_PACK_F)
        /* nitro pack inside an explosion - it is now triggered */
        store_sc(cave, x, y, O_NITRO_PACK_EXPLODE);
    else
        /* for everything else */
        store_sc(cave, x, y, explode_to);
}

/* a creature explodes to a 3x3 something. */
static void
creature_explode(GdCave *cave, int x, int y, GdElement explode_to)
{
    int xx, yy;

    /* the processing of an explosion took pretty much time: processing 3x3=9 elements */
    cave->ckdelay+=1200;
    gd_sound_play(cave, GD_S_EXPLOSION);

    for (yy=y-1; yy<=y+1; yy++)
        for (xx=x-1; xx<=x+1; xx++)
            cell_explode(cave, xx, yy, explode_to);
}

static void
nitro_explode(GdCave *cave, int x, int y)
{
    int xx, yy;

    /* the processing of an explosion took pretty much time: processing 3x3=9 elements */
    cave->ckdelay+=1200;
    gd_sound_play(cave, GD_S_NITRO_EXPLOSION);

    for (yy=y-1; yy<=y+1; yy++)
        for (xx=x-1; xx<=x+1; xx++)
            cell_explode(cave, xx, yy, O_NITRO_EXPL_1);
    /* the current cell is explicitly changed into a nitro expl, as cell_explode changes it to a triggered nitro pack */
    store_sc(cave, x, y, O_NITRO_EXPL_1);
}

/* a voodoo explodes, leaving a 3x3 steel and a time penalty behind. */
static void
voodoo_explode(GdCave *cave, int x, int y)
{
    int xx, yy;

    /* the processing of an explosion took pretty much time: processing 3x3=9 elements */
    cave->ckdelay+=1000;

    gd_sound_play(cave, GD_S_VOODOO_EXPLOSION);
    if (cave->voodoo_any_hurt_kills_player)
        cave->voodoo_touched=TRUE;

    /* voodoo explodes to 3x3 steel */
    for (yy=y-1; yy<=y+1; yy++)
        for (xx=x-1; xx<=x+1; xx++)
            store_sc(cave, xx, yy, O_PRE_STEEL_1);
    /* middle is a time penalty (which will be turned into a gravestone) */
    store_sc(cave, x, y, O_TIME_PENALTY);
}

/* a bomb does not explode the voodoo, neither does the ghost.
   this function check this, and stores the new element or not.
   destroying the voodoo is also controlled by the voodoo_disappear_in_explosion flag. */
static void
explode_try_skip_voodoo(GdCave *cave, const int x, const int y, const GdElement expl)
{
    if (non_explodable (cave, x, y))
        return;
    /* bomb does not explode voodoo */
    if (!cave->voodoo_disappear_in_explosion && get(cave, x, y)==O_VOODOO)
        return;
    if (cave->voodoo_any_hurt_kills_player && get(cave, x, y)==O_VOODOO)
        cave->voodoo_touched=TRUE;
    store_sc (cave, x, y, expl);
}

/* X shaped ghost explosion; does not touch voodoo! */
static void
ghost_explode(GdCave *cave, const int x, const int y)
{
    gd_sound_play(cave, GD_S_GHOST_EXPLOSION);

    /* the processing of an explosion took pretty much time: processing 5 elements */
    cave->ckdelay+=650;

    explode_try_skip_voodoo(cave, x, y, O_GHOST_EXPL_1);
    explode_try_skip_voodoo(cave, x-1, y-1, O_GHOST_EXPL_1);
    explode_try_skip_voodoo(cave, x+1, y+1, O_GHOST_EXPL_1);
    explode_try_skip_voodoo(cave, x-1, y+1, O_GHOST_EXPL_1);
    explode_try_skip_voodoo(cave, x+1, y-1, O_GHOST_EXPL_1);
}

/*+shaped bomb explosion; does not touch voodoo! */
static void
bomb_explode(GdCave *cave, const int x, const int y)
{
    gd_sound_play(cave, GD_S_BOMB_EXPLOSION);

    /* the processing of an explosion took pretty much time: processing 5 elements */
    cave->ckdelay+=650;

    explode_try_skip_voodoo(cave, x, y, O_BOMB_EXPL_1);
    explode_try_skip_voodoo(cave, x-1, y, O_BOMB_EXPL_1);
    explode_try_skip_voodoo(cave, x+1, y, O_BOMB_EXPL_1);
    explode_try_skip_voodoo(cave, x, y+1, O_BOMB_EXPL_1);
    explode_try_skip_voodoo(cave, x, y-1, O_BOMB_EXPL_1);
}

/*
    explode an element with the appropriate type of exlposion.
 */
static void
explode(GdCave *cave, int x, int y)
{
    GdElement e=get(cave, x, y)&O_MASK;

    switch (e) {
        case O_GHOST:
            ghost_explode(cave, x, y);
            break;

        case O_BOMB_TICK_7:
            bomb_explode(cave, x, y);
            break;

        case O_VOODOO:
            voodoo_explode(cave, x, y);
            break;

        case O_NITRO_PACK:
        case O_NITRO_PACK_F:
        case O_NITRO_PACK_EXPLODE:
            nitro_explode(cave, x, y);
            break;

        case O_AMOEBA_2:
            creature_explode(cave, x, y, O_AMOEBA_2_EXPL_1);
            break;

        case O_FALLING_WALL_F:
            creature_explode(cave, x, y, O_EXPLODE_1);
            break;

        case O_BUTTER_1:
        case O_BUTTER_2:
        case O_BUTTER_3:
        case O_BUTTER_4:
            creature_explode(cave, x, y, cave->butterfly_explode_to);
            break;

        case O_ALT_BUTTER_1:
        case O_ALT_BUTTER_2:
        case O_ALT_BUTTER_3:
        case O_ALT_BUTTER_4:
            creature_explode(cave, x, y, cave->alt_butterfly_explode_to);
            break;

        case O_FIREFLY_1:
        case O_FIREFLY_2:
        case O_FIREFLY_3:
        case O_FIREFLY_4:
            creature_explode(cave, x, y, cave->firefly_explode_to);
            break;

        case O_ALT_FIREFLY_1:
        case O_ALT_FIREFLY_2:
        case O_ALT_FIREFLY_3:
        case O_ALT_FIREFLY_4:
            creature_explode(cave, x, y, cave->alt_firefly_explode_to);
            break;

        case O_PLAYER:
        case O_PLAYER_BOMB:
        case O_PLAYER_GLUED:
        case O_PLAYER_STIRRING:
        case O_PLAYER_PNEUMATIC_LEFT:
        case O_PLAYER_PNEUMATIC_RIGHT:
            creature_explode(cave, x, y, O_EXPLODE_1);
            break;

        case O_STONEFLY_1:
        case O_STONEFLY_2:
        case O_STONEFLY_3:
        case O_STONEFLY_4:
            creature_explode(cave, x, y, cave->stonefly_explode_to);
            break;

        case O_DRAGONFLY_1:
        case O_DRAGONFLY_2:
        case O_DRAGONFLY_3:
        case O_DRAGONFLY_4:
            creature_explode(cave, x, y, cave->dragonfly_explode_to);
            break;

        default:
            g_assert_not_reached();
            break;
    }
}

static void inline
explode_dir(GdCave *cave, const int x, const int y, GdDirection dir)
{
    explode(cave, x+gd_dx[dir], y+gd_dy[dir]);
}


/*
    player eats specified object.
    returns O_SPACE if he eats it (diamond, dirt, space, outbox)
    returns other element if something other appears there and he can't move.
    cave pointer is needed to know the diamond values.
 */
static GdElement
player_get_element (GdCave* cave, const GdElement object)
{
    int i;

    switch (object) {
    case O_DIAMOND_KEY:
        cave->diamond_key_collected=TRUE;
        gd_sound_play(cave, GD_S_DIAMOND_KEY_COLLECT);
        return O_SPACE;

    /* KEYS AND DOORS */
    case O_KEY_1:
        gd_sound_play(cave, GD_S_KEY_COLLECT);
        cave->key1++;
        return O_SPACE;
    case O_KEY_2:
        gd_sound_play(cave, GD_S_KEY_COLLECT);
        cave->key2++;
        return O_SPACE;
    case O_KEY_3:
        gd_sound_play(cave, GD_S_KEY_COLLECT);
        cave->key3++;
        return O_SPACE;
    case O_DOOR_1:
        if (cave->key1==0)
            return object;
        gd_sound_play(cave, GD_S_DOOR_OPEN);
        cave->key1--;
        return O_SPACE;
    case O_DOOR_2:
        if (cave->key2==0)
            return object;
        gd_sound_play(cave, GD_S_DOOR_OPEN);
        cave->key2--;
        return O_SPACE;
    case O_DOOR_3:
        if (cave->key3==0)
            return object;
        gd_sound_play(cave, GD_S_DOOR_OPEN);
        cave->key3--;
        return O_SPACE;

    /* SWITCHES */
    case O_CREATURE_SWITCH:        /* creatures change direction. */
        gd_sound_play(cave, GD_S_SWITCH_CREATURES);
        cave->creatures_backwards=!cave->creatures_backwards;
        return object;
    case O_EXPANDING_WALL_SWITCH:        /* expanding wall change direction. */
        gd_sound_play(cave, GD_S_SWITCH_EXPANDING);
        cave->expanding_wall_changed=!cave->expanding_wall_changed;
        return object;
    case O_BITER_SWITCH:        /* biter change delay */
        gd_sound_play(cave, GD_S_SWITCH_BITER);
        cave->biter_delay_frame++;
        if (cave->biter_delay_frame==4)
            cave->biter_delay_frame=0;
        return object;
    case O_REPLICATOR_SWITCH:    /* replicator on/off switch */
        gd_sound_play(cave, GD_S_SWITCH_REPLICATOR);
        cave->replicators_active=!cave->replicators_active;
        return object;
    case O_CONVEYOR_SWITCH:    /* conveyor belts on/off */
        gd_sound_play(cave, GD_S_SWITCH_CONVEYOR);
        cave->conveyor_belts_active=!cave->conveyor_belts_active;
        return object;
    case O_CONVEYOR_DIR_SWITCH: /* conveyor belts switch direction */
        gd_sound_play(cave, GD_S_SWITCH_CONVEYOR);
        cave->conveyor_belts_direction_changed=!cave->conveyor_belts_direction_changed;
        return object;

    /* USUAL STUFF */
    case O_DIRT:
    case O_DIRT2:
    case O_STEEL_EATABLE:
    case O_BRICK_EATABLE:
    case O_DIRT_SLOPED_UP_RIGHT:
    case O_DIRT_SLOPED_UP_LEFT:
    case O_DIRT_SLOPED_DOWN_LEFT:
    case O_DIRT_SLOPED_DOWN_RIGHT:
    case O_DIRT_BALL:
    case O_DIRT_LOOSE:
        gd_sound_play(cave, GD_S_WALK_EARTH);
        return O_SPACE;

    case O_SWEET:
        gd_sound_play(cave, GD_S_SWEET_COLLECT);
        cave->sweet_eaten=TRUE;
        return O_SPACE;

    case O_PNEUMATIC_HAMMER:
        gd_sound_play(cave, GD_S_PNEUMATIC_COLLECT);
        cave->got_pneumatic_hammer=TRUE;
        return O_SPACE;

    case O_CLOCK:
        /* bonus time */
        gd_sound_play(cave, GD_S_CLOCK_COLLECT);
        cave->time+=cave->time_bonus*cave->timing_factor;
        if (cave->time>cave->max_time*cave->timing_factor)
            cave->time-=cave->max_time*cave->timing_factor;
        /* no space, rather a dirt remains there... */
        return O_DIRT;
    case O_DIAMOND:
    case O_FLYING_DIAMOND:
        gd_sound_play(cave, GD_S_DIAMOND_COLLECT);
        cave->score+=cave->diamond_value;
        cave->diamonds_collected++;
        if (cave->diamonds_needed==cave->diamonds_collected) {
            cave->gate_open=TRUE;
            cave->diamond_value=cave->extra_diamond_value;    /* extra is worth more points. */
            cave->gate_open_flash=1;
            cave->sound3=GD_S_CRACK;
        }
        return O_SPACE;
    case O_SKELETON:
        cave->skeletons_collected++;
        for (i=0; i<cave->skeletons_worth_diamonds; i++)
            player_get_element(cave, O_DIAMOND);    /* as if player got a diamond */
        gd_sound_play(cave, GD_S_SKELETON_COLLECT); /* _after_ calling get_element for the fake diamonds, so we overwrite its sounds */
        return O_SPACE;
    case O_OUTBOX:
    case O_INVIS_OUTBOX:
        cave->player_state=GD_PL_EXITED;    /* player now exits the cave! */
        return O_SPACE;
    case O_SPACE:
    case O_LAVA:    /* player goes into lava, as if it was space */
        gd_sound_play(cave, GD_S_WALK_EMPTY);
        return O_SPACE;

    default:
        /* the object will remain there. */
        return object;
    }
}

/*
   process a crazy dream-style teleporter.
   called from gd_cave_iterate, for a player or a player_bomb.
   player is standing at px, py, and trying to move in the direction player_move,
   where there is a teleporter.
   we check the whole cave, from px+1,py, till we get back to px,py (by wrapping
   around). the first teleporter we find, and which is suitable, will be the destination.
   return TRUE if teleporter worked, FALSE if cound not find any suitable teleporter.
 */
static gboolean
do_teleporter(GdCave *cave, int px, int py, GdDirection player_move)
{
    int tx, ty;

    tx=px;
    ty=py;

    do {
        /* jump to next element; wrap around columns and rows. */
        tx++;
        if (tx>=cave->w) {
            tx=0;
            ty++;
            if (ty>=cave->h)
                ty=0;
        }
        /* if we found a teleporter... */
        if (get(cave, tx, ty)==O_TELEPORTER && is_space_dir(cave, tx, ty, player_move)) {
            store_dir(cave, tx, ty, player_move, get(cave, px, py));    /* new player appears near teleporter found */
            store(cave, px, py, O_SPACE);    /* current player disappears */
            gd_sound_play(cave, GD_S_TELEPORTER);
            return TRUE;    /* return true as teleporter worked */
        }
    } while (tx!=px || ty!=py);    /* loop until we get back to original coordinates */
    return FALSE;    /* return false as we did not find any usable teleporter */
}

/*
    try to push an element.
    returns true if the push is possible; also does move the specified _element_.
    up to the caller to move the _player_itself_.
*/

static gboolean
do_push(GdCave *cave, int x, int y, GdDirection player_move, gboolean player_fire)
{
    gboolean result;
    GdElement what=get_dir(cave, x, y, player_move);
    GdDirection grav_compat=cave->gravity_affects_all?cave->gravity:MV_DOWN;    /* gravity for falling wall, bladder, ... */

    result=FALSE;

    switch(what) {
        case O_WAITING_STONE:
        case O_STONE:
        case O_NITRO_PACK:
        case O_CHASING_STONE:
        case O_MEGA_STONE:
        case O_FLYING_STONE:
        case O_NUT:
            /* pushing some kind of stone or nut */
            /* directions possible: 90degrees cw or ccw to current gravity. */
            /* only push if player dir is orthogonal to gravity, ie. gravity down, pushing left&right possible */
            if (player_move==ccw_fourth[cave->gravity] || player_move==cw_fourth[cave->gravity]) {
                int prob;

                prob=0;
                /* different probabilities for different elements. */
                switch(what) {
                    case O_WAITING_STONE:
                        prob=1000000; /* waiting stones are light, can always push */
                        break;
                    case O_CHASING_STONE:
                        if (cave->sweet_eaten) /* chasing can be pushed if player is turbo */
                            prob=1000000;
                        break;
                    case O_MEGA_STONE:
                        if (cave->mega_stones_pushable_with_sweet && cave->sweet_eaten) /* mega may(!) be pushed if player is turbo */
                            prob=1000000;
                        break;
                    case O_STONE:
                    case O_NUT:
                    case O_FLYING_STONE:
                    case O_NITRO_PACK:
                        if (cave->sweet_eaten)
                            prob=cave->pushing_stone_prob_sweet;    /* probability with sweet */
                        else
                            prob=cave->pushing_stone_prob;    /* probability without sweet. */
                        break;
                    default:
                        g_assert_not_reached();
                        break;
                }

                if (is_space_dir(cave, x, y, MV_TWICE+player_move) && g_rand_int_range(cave->random, 0, 1000000)<prob) {
                    /* if decided that he will be able to push, */
                    store_dir(cave, x, y, MV_TWICE+player_move, what);
                    play_sound_of_element(cave, what);
                    result=TRUE;
                }
            }
            break;

        case O_BLADDER:
        case O_BLADDER_1:
        case O_BLADDER_2:
        case O_BLADDER_3:
        case O_BLADDER_4:
        case O_BLADDER_5:
        case O_BLADDER_6:
        case O_BLADDER_7:
        case O_BLADDER_8:
            /* pushing a bladder. keep in mind that after pushing, we always get an O_BLADDER,
             * not an O_BLADDER_x. */
            /* there is no "delayed" state of a bladder, so we use store_dir_no_scanned! */

            /* first check: we cannot push a bladder "up" */
            if (player_move!=opposite[grav_compat]) {
                /* pushing a bladder "down". p=player, o=bladder, 1, 2, 3=directions to check. */
                /* player moving in the direction of gravity. */
                /*  p   p  g  */
                /* 2o3  |  |  */
                /*  1   v  v  */
                if (player_move==grav_compat) {
                    if (is_space_dir(cave, x, y, MV_TWICE+player_move))    /* pushing bladder down */
                        store_dir_no_scanned(cave, x, y, MV_TWICE+player_move, O_BLADDER), result=TRUE;
                    else if (is_space_dir(cave, x, y, cw_eighth[grav_compat]))    /* if no space to push down, maybe left (down-left to player) */
                                                                                    /* left is "down, turned right (cw)" */
                        store_dir_no_scanned(cave, x, y, cw_eighth[grav_compat], O_BLADDER), result=TRUE;
                    else if (is_space_dir(cave, x, y, ccw_eighth[grav_compat]))    /* if not, maybe right (down-right to player) */
                        store_dir_no_scanned(cave, x, y, ccw_eighth[grav_compat], O_BLADDER), result=TRUE;
                }
                /* pushing a bladder "left". p=player, o=bladder, 1, 2, 3=directions to check. */
                /*  3        g */
                /* 1op  <-p  | */
                /*  2        v */
                else if (player_move==cw_fourth[grav_compat]) {
                    if (is_space_dir(cave, x, y, MV_TWICE+cw_fourth[grav_compat]))    /* pushing it left */
                        store_dir_no_scanned(cave, x, y, MV_TWICE+cw_fourth[grav_compat], O_BLADDER), result=TRUE;
                    else if (is_space_dir(cave, x, y, cw_eighth[grav_compat]))    /* maybe down, and player will move left */
                        store_dir_no_scanned(cave, x, y, cw_eighth[grav_compat], O_BLADDER), result=TRUE;
                    else if (is_space_dir(cave, x, y, cw_eighth[player_move]))    /* maybe up, and player will move left */
                        store_dir_no_scanned(cave, x, y, cw_eighth[player_move], O_BLADDER), result=TRUE;
                }
                /* pushing a bladder "right". p=player, o=bladder, 1, 2, 3=directions to check. */
                /*  3        g */
                /* po1  p-<  | */
                /*  2        v */
                else if (player_move==ccw_fourth[grav_compat]) {
                    if (is_space_dir(cave, x, y, MV_TWICE+player_move))    /* pushing it right */
                        store_dir_no_scanned(cave, x, y, MV_TWICE+player_move, O_BLADDER), result=TRUE;
                    else if (is_space_dir(cave, x, y, ccw_eighth[grav_compat]))    /* maybe down, and player will move right */
                        store_dir_no_scanned(cave, x, y, ccw_eighth[grav_compat], O_BLADDER), result=TRUE;
                    else if (is_space_dir(cave, x, y, ccw_eighth[player_move]))    /* maybe up, and player will move right */
                        store_dir_no_scanned(cave, x, y, ccw_eighth[player_move], O_BLADDER), result=TRUE;
                }

                if (result)
                    play_sound_of_element(cave, O_BLADDER);
            }
            break;

        case O_BOX:
            /* a box is only pushed with the fire pressed */
            if (player_fire) {
                /* but always with 100% probability */
                switch (player_move) {
                    case MV_LEFT:
                    case MV_RIGHT:
                    case MV_UP:
                    case MV_DOWN:
                        /* pushing in some dir, two steps in that dir - is there space? */
                        if (is_space_dir(cave, x, y, player_move+MV_TWICE)) {
                            /* yes, so push. */
                            store_dir(cave, x, y, player_move+MV_TWICE, O_BOX);
                            result=TRUE;
                            gd_sound_play(cave, GD_S_BOX_PUSH);
                        }
                        break;
                    default:
                        /* push in no other directions possible */
                        break;
                }
            }
            break;

        /* pushing of other elements not possible */
        default:
            break;
    }

    return result;
}

/* from the key press booleans, create a direction */
GdDirection
gd_direction_from_keypress(gboolean up, gboolean down, gboolean left, gboolean right)
{
    GdDirection player_move;

    /* from the key press booleans, create a direction */
    if (up && right)
        player_move=MV_UP_RIGHT;
    else if (down && right)
        player_move=MV_DOWN_RIGHT;
    else if (down && left)
        player_move=MV_DOWN_LEFT;
    else if (up && left)
        player_move=MV_UP_LEFT;
    else if (up)
        player_move=MV_UP;
    else if (down)
        player_move=MV_DOWN;
    else if (left)
        player_move=MV_LEFT;
    else if (right)
        player_move=MV_RIGHT;
    else
        player_move=MV_STILL;

    return player_move;
}


/* clear these to no sound; and they will be set during iteration. */
void
gd_cave_clear_sounds(GdCave *cave)
{
    cave->sound1=GD_S_NONE;
    cave->sound2=GD_S_NONE;
    cave->sound3=GD_S_NONE;
}



static void
do_start_fall(GdCave *cave, int x, int y, GdDirection falling_direction, GdElement falling_element)
{
    if (cave->gravity_disabled)
        return;

    if (is_space_dir(cave, x, y, falling_direction)) {    /* beginning to fall */
        play_sound_of_element(cave, get(cave, x, y));
        move(cave, x, y, falling_direction, falling_element);
    }
    /* check if it is on a sloped element, and it can roll. */
    /* for example, sloped wall looks like: */
    /*  /| */
    /* /_| */
    /* this is tagged as sloped up&left. */
    /* first check if the stone or diamond is coming from "up" (ie. opposite of gravity) */
    /* then check the direction to roll (left or right) */
    /* this way, gravity can also be pointing right, and the above slope will work as one would expect */
    else if (sloped_dir(cave, x, y, falling_direction, opposite[falling_direction])) {    /* rolling down, if sitting on a sloped object  */
        if (sloped_dir(cave, x, y, falling_direction, cw_fourth[falling_direction]) && is_space_dir(cave, x, y, cw_fourth[falling_direction]) && is_space_dir(cave, x, y, cw_eighth[falling_direction])) {
            /* rolling left? - keep in mind that ccw_fourth rotates gravity ccw, so here we use cw_fourth */
            play_sound_of_element(cave, get(cave, x, y));
            move(cave, x, y, cw_fourth[falling_direction], falling_element);
        }
        else if (sloped_dir(cave, x, y, falling_direction, ccw_fourth[falling_direction]) && is_space_dir(cave, x, y, ccw_fourth[falling_direction]) && is_space_dir(cave, x, y, ccw_eighth[falling_direction])) {
            /* rolling right? */
            play_sound_of_element(cave, get(cave, x, y));
            move(cave, x, y, ccw_fourth[falling_direction], falling_element);
        }
    }
}

static gboolean
do_fall_try_crush_voodoo(GdCave *cave, int x, int y, GdDirection fall_dir)
{
    if (get_dir(cave, x, y, fall_dir)==O_VOODOO && cave->voodoo_dies_by_stone) {
        /* this is a 1stB-style vodo. explodes by stone, collects diamonds */
        explode_dir(cave, x, y, fall_dir);
        return TRUE;
    }
    else
        return FALSE;
}

static gboolean
do_fall_try_eat_voodoo(GdCave *cave, int x, int y, GdDirection fall_dir)
{
    if (get_dir(cave, x, y, fall_dir)==O_VOODOO && cave->voodoo_collects_diamonds) {
        /* this is a 1stB-style voodoo. explodes by stone, collects diamonds */
        player_get_element(cave, O_DIAMOND);    /* as if player got diamond */
        store(cave, x, y, O_SPACE);    /* diamond disappears */
        return TRUE;
    }
    else
        return FALSE;
}


static gboolean
do_fall_try_crack_nut(GdCave *cave, int x, int y, GdDirection fall_dir, GdElement bouncing)
{
    if (get_dir(cave, x, y, fall_dir)==O_NUT || get_dir(cave, x, y, fall_dir)==O_NUT_F) {
        /* stones */
        store(cave, x, y, bouncing);
        store_dir(cave, x, y, fall_dir, cave->nut_turns_to_when_crushed);
        if (cave->nut_sound)
            gd_sound_play(cave, GD_S_NUT_CRACK);
        return TRUE;
    }
    else
        return FALSE;
}

static gboolean
do_fall_try_magic(GdCave *cave, int x, int y, GdDirection fall_dir, GdElement magic)
{
    if (get_dir(cave, x, y, fall_dir)==O_MAGIC_WALL) {
        play_sound_of_element(cave, O_DIAMOND);    /* always play diamond sound */
        if (cave->magic_wall_state==GD_MW_DORMANT)
            cave->magic_wall_state=GD_MW_ACTIVE;
        if (cave->magic_wall_state==GD_MW_ACTIVE && is_space_dir(cave, x, y, MV_TWICE+fall_dir)) {
            /* if magic wall active and place underneath, it turns element into anything the effect says to do. */
            store_dir(cave, x, y, MV_TWICE+fall_dir, magic);
        }
        store(cave, x, y, O_SPACE);    /* active or non-active or anything, element falling in will always disappear */
        return TRUE;
    }
    else
        return FALSE;
}

static gboolean
do_fall_try_crush(GdCave *cave, int x, int y, GdDirection fall_dir)
{
    if (explodes_by_hit_dir(cave, x, y, fall_dir)) {
        explode_dir(cave, x, y, fall_dir);
        return TRUE;
    }
    else
        return FALSE;
}

static gboolean
do_fall_roll_or_stop(GdCave *cave, int x, int y, GdDirection fall_dir, GdElement bouncing)
{
    if (is_space_dir(cave, x, y, fall_dir))    { /* falling further */
        move(cave, x, y, fall_dir, get(cave, x, y));
        return TRUE;
    }
    /* check if it is on a sloped element, and it can roll. */
    /* for example, sloped wall looks like: */
    /*  /| */
    /* /_| */
    /* this is tagged as sloped up&left. */
    /* first check if the stone or diamond is coming from "up" (ie. opposite of gravity) */
    /* then check the direction to roll (left or right) */
    /* this way, gravity can also be pointing right, and the above slope will work as one would expect */
    if (sloped_dir(cave, x, y, fall_dir, opposite[fall_dir])) {    /* sloped element, falling to left or right */
        if (sloped_dir(cave, x, y, fall_dir, cw_fourth[fall_dir]) && is_space_dir(cave, x, y, cw_eighth[fall_dir]) && is_space_dir(cave, x, y, cw_fourth[fall_dir])) {
            play_sound_of_element(cave, get(cave, x, y));
            move(cave, x, y, cw_fourth[fall_dir], get(cave, x, y));    /* try to roll left first - see O_STONE to understand why cw_fourth */
        }
        else if (sloped_dir(cave, x, y, fall_dir, ccw_fourth[fall_dir]) && is_space_dir(cave, x, y, ccw_eighth[fall_dir]) && is_space_dir(cave, x, y, ccw_fourth[fall_dir])) {
            play_sound_of_element(cave, get(cave, x, y));
            move(cave, x, y, ccw_fourth[fall_dir], get(cave, x, y));    /* if not, try to roll right */
        }
        else {
            /* cannot roll in any direction, so it stops */
            play_sound_of_element(cave, get(cave, x, y));
            store(cave, x, y, bouncing);
        }
        return TRUE;
    }

    /* any other element, stops */
    play_sound_of_element(cave, get(cave, x, y));
    store(cave, x, y, bouncing);
    return TRUE;
}











/* process a cave. */
void
gd_cave_iterate(GdCave *cave, GdDirection player_move, gboolean player_fire, gboolean suicide)
{
    int x, y, i;
    int ymin, ymax;    /* for border scan */
    gboolean amoeba_found_enclosed, amoeba_2_found_enclosed;    /* amoeba found to be enclosed. if not, this is cleared */
    int amoeba_count, amoeba_2_count;        /* counting the number of amoebas. after scan, check if too much */
    gboolean found_water;        /* cave scan found water - for sound */
    gboolean inbox_toggle;
    gboolean start_signal;
    GdDirection grav_compat=cave->gravity_affects_all?cave->gravity:MV_DOWN;    /* gravity for falling wall, bladder, ... */
    /* directions for o_something_1, 2, 3 and 4 (creatures) */
    static const GdDirection creature_dir[]={ MV_LEFT, MV_UP, MV_RIGHT, MV_DOWN };
    static const GdDirection creature_chdir[]={ MV_RIGHT, MV_DOWN, MV_LEFT, MV_UP };
    int time_decrement_sec;
    GdElement biter_try[]={ O_DIRT, cave->biter_eat, O_SPACE, O_STONE };    /* biters eating elements preference, they try to go in this order */
    gboolean amoeba_sound, magic_sound;

    gd_cave_clear_sounds(cave);

    /* if diagonal movements not allowed, */
    /* horizontal movements have precedence. [BROADRIBB] */
    if (!cave->diagonal_movements) {
        switch (player_move) {
        case MV_UP_RIGHT:
        case MV_DOWN_RIGHT:
            player_move=MV_RIGHT;
            break;
        case MV_UP_LEFT:
        case MV_DOWN_LEFT:
            player_move=MV_LEFT;
            break;
        default:
            /* no correction needed */
            break;
        }
    }

    /* set cave get function; to implement perfect or lineshifting borders */
    if (cave->lineshift)
        cave->getp=getp_shift;
    else
        cave->getp=getp_perfect;

    /* increment this. if the scan routine comes across player, clears it (sets to zero). */
    if (cave->player_seen_ago<100)
        cave->player_seen_ago++;

    if (cave->pneumatic_hammer_active_delay>0)
        cave->pneumatic_hammer_active_delay--;

    /* inboxes and outboxes flash with the rhythm of the game, not the display.
     * also, a player can be born only from an open, not from a steel-wall-like inbox. */
    cave->inbox_flash_toggle=!cave->inbox_flash_toggle;
    inbox_toggle=cave->inbox_flash_toggle;

    if (cave->gate_open_flash>0)
        cave->gate_open_flash--;

    /* score collected this frame */
    cave->score=0;

    /* suicide only kills the active player */
    /* player_x, player_y was set by the previous iterate routine, or the cave setup. */
    /* we must check if there is a player or not - he may have exploded or something like that */
    if (suicide && cave->player_state==GD_PL_LIVING && is_player(cave, cave->player_x, cave->player_y))
        store(cave, cave->player_x, cave->player_y, O_EXPLODE_1);

    /* check for walls reappearing */
    if (cave->hammered_reappear)
        for (y=0; y<cave->h; y++)
            for (x=0; x<cave->w; x++) {
                /* timer for the cell > 0? */
                if (cave->hammered_reappear[y][x]>0) {
                    /* decrease timer */
                    cave->hammered_reappear[y][x]--;
                    /* check if it became zero */
                    if (cave->hammered_reappear[y][x]==0) {
                        store(cave, x, y, O_BRICK);
                        gd_sound_play(cave, GD_S_WALL_REAPPEAR);
                    }
                }
            }

    /* variables to check during the scan */
    amoeba_found_enclosed=TRUE;        /* will be set to false if any of the amoeba is found free. */
    amoeba_2_found_enclosed=TRUE;
    amoeba_count=0;
    amoeba_2_count=0;
    found_water=FALSE;
    cave->ckdelay=0;
    time_decrement_sec=0;

    /* check whether to scan the first and last line */
    if (cave->border_scan_first_and_last) {
        ymin=0;
        ymax=cave->h-1;
    } else {
        ymin=1;
        ymax=cave->h-2;
    }
    /* the cave scan routine */
    for (y=ymin; y<=ymax; y++)
        for (x=0; x<cave->w; x++) {
            /* if we find a scanned element, change it to the normal one, and that's all. */
            /* this is required, for example for chasing stones, which have moved, always passing slime! */
            if (get(cave, x, y)&SCANNED) {
                store(cave, x, y, get(cave, x, y)& ~SCANNED);
                continue;
            }

            /* add the ckdelay correction value for every element seen. */
            cave->ckdelay+=gd_elements[get(cave, x, y)].ckdelay;

            switch (get(cave, x, y)) {
            /*
             *     P L A Y E R S
             */
            case O_PLAYER:
                if (cave->kill_player) {
                    explode (cave, x, y);
                    break;
                }
                cave->player_seen_ago=0;
                /* bd4 intermission caves have many players. so if one of them has exited,
                 * do not change the flag anymore. so this if () is needed */
                if (cave->player_state!=GD_PL_EXITED)
                    cave->player_state=GD_PL_LIVING;

                /* check for pneumatic hammer things */
                /* 1) press fire, 2) have pneumatic hammer 4) space on left or right for hammer 5) stand on something */
                if (player_fire && cave->got_pneumatic_hammer && is_space_dir(cave, x, y, player_move)
                    && !is_space_dir(cave, x, y, MV_DOWN)) {
                    if (player_move==MV_LEFT && can_be_hammered_dir(cave, x, y, MV_DOWN_LEFT)) {
                        cave->pneumatic_hammer_active_delay=cave->pneumatic_hammer_frame;
                        store_dir(cave, x, y, MV_LEFT, O_PNEUMATIC_ACTIVE_LEFT);
                        store(cave, x, y, O_PLAYER_PNEUMATIC_LEFT);
                        break;    /* finished. */
                    }
                    if (player_move==MV_RIGHT && can_be_hammered_dir(cave, x, y, MV_DOWN_RIGHT)) {
                        cave->pneumatic_hammer_active_delay=cave->pneumatic_hammer_frame;
                        store_dir(cave, x, y, MV_RIGHT, O_PNEUMATIC_ACTIVE_RIGHT);
                        store(cave, x, y, O_PLAYER_PNEUMATIC_RIGHT);
                        break;    /* finished. */
                    }
                }

                if (player_move!=MV_STILL) {
                    /* only do every check if he is not moving */
                    GdElement what=get_dir(cave, x, y, player_move);
                    GdElement remains=what;
                    gboolean push;

                    /* if we are 'eating' a teleporter, and the function returns true (teleporting worked), break here */
                    if (what==O_TELEPORTER && do_teleporter(cave, x, y, player_move))
                        break;

                    /* try to push element; if successful, break  */
                    push=do_push(cave, x, y, player_move, player_fire);
                    if (push)
                        remains=O_SPACE;
                    else
                        switch (what) {
                        case O_BOMB:
                            /* if its a bomb, remember he now has one. */
                            /* we do not change the "remains" and "what" variables, so that part of the code will be ineffective */
                            gd_sound_play(cave, GD_S_BOMB_COLLECT);
                            store_dir(cave, x, y, player_move, O_SPACE);
                            if (player_fire)
                                store(cave, x, y, O_PLAYER_BOMB);
                            else
                                move(cave, x, y, player_move, O_PLAYER_BOMB);
                            break;

                        case O_POT:
                            /* we do not change the "remains" and "what" variables, so that part of the code will be ineffective */
                            if (!player_fire && !cave->gravity_switch_active && cave->skeletons_collected>=cave->skeletons_needed_for_pot) {
                                cave->skeletons_collected-=cave->skeletons_needed_for_pot;
                                move(cave, x, y, player_move, O_PLAYER_STIRRING);
                                cave->gravity_disabled=TRUE;
                            }
                            break;

                        case O_GRAVITY_SWITCH:
                            /* (we cannot use player_get for this as it does not have player_move parameter) */
                            /* only allow changing direction if the new dir is not diagonal */
                            if (cave->gravity_switch_active && (player_move==MV_LEFT || player_move==MV_RIGHT || player_move==MV_UP || player_move==MV_DOWN)) {
                                gd_sound_play(cave, GD_S_SWITCH_GRAVITY);
                                cave->gravity_will_change=cave->gravity_change_time*cave->timing_factor;
                                cave->gravity_next_direction=player_move;
                                cave->gravity_switch_active=FALSE;
                            }
                            break;

                        default:
                            /* get element - process others. if cannot get, player_get_element will return the same */
                            remains=player_get_element (cave, what);
                            break;
                        }

                    if (remains!=what || remains==O_SPACE) {
                        /* if anything changed, apply the change. */

                        /* if snapping anything and we have snapping explosions set. but these is not true for pushing. */
                        if (remains==O_SPACE && player_fire && !push)
                            remains=cave->snap_element;
                        if (remains!=O_SPACE || player_fire)
                            /* if any other element than space, player cannot move. also if pressing fire, will not move. */
                            store_dir(cave, x, y, player_move, remains);
                        else
                            /* if space remains there, the player moves. */
                            move(cave, x, y, player_move, O_PLAYER);
                    }

                }
                break;

            case O_PLAYER_BOMB:
                /* much simpler; cannot steal stones */
                if (cave->kill_player) {
                    explode (cave, x, y);
                    break;
                }
                cave->player_seen_ago=0;
                /* bd4 intermission caves have many players. so if one of them has exited,
                 * do not change the flag anymore. so this if () is needed */
                if (cave->player_state!=GD_PL_EXITED)
                    cave->player_state=GD_PL_LIVING;

                if (player_move!=MV_STILL) {    /* if the player does not move, nothing to do */
                    GdElement what=get_dir(cave, x, y, player_move);
                    GdElement remains=what;

                    if (player_fire) {
                        /* placing a bomb into empty space or dirt */
                        if (is_space_dir(cave, x, y, player_move) || is_element_dir(cave, x, y, player_move, O_DIRT)) {
                            store_dir(cave, x, y, player_move, O_BOMB_TICK_1);
                            /* placed bomb, he is normal player again */
                            store(cave, x, y, O_PLAYER);
                            gd_sound_play(cave, GD_S_BOMB_PLACE);
                        }
                        break;
                    }

                    /* pushing and collecting */
                    /* if we are 'eating' a teleporter, and the function returns true (teleporting worked), break here */
                    if (what==O_TELEPORTER && do_teleporter(cave, x, y, player_move))
                        break;

                    if (do_push(cave, x, y, player_move, FALSE))    /* player fire is false... */
                        remains=O_SPACE;
                    else {
                        switch (what) {
                            case O_GRAVITY_SWITCH:
                                /* (we cannot use player_get for this as it does not have player_move parameter) */
                                /* only allow changing direction if the new dir is not diagonal */
                                if (cave->gravity_switch_active && (player_move==MV_LEFT || player_move==MV_RIGHT || player_move==MV_UP || player_move==MV_DOWN)) {
                                    gd_sound_play(cave, GD_S_SWITCH_GRAVITY);
                                    cave->gravity_will_change=cave->gravity_change_time*cave->timing_factor;
                                    cave->gravity_next_direction=player_move;
                                    cave->gravity_switch_active=FALSE;
                                }
                                break;
                            default:
                                /* get element. if cannot get, player_get_element will return the same */
                                remains=player_get_element (cave, what);
                                break;
                        }
                    }

                    /* if element changed, OR there is space, move. */
                    if (remains!=what || remains==O_SPACE) {
                        /* if anything changed, apply the change. */
                        move(cave, x, y, player_move, O_PLAYER_BOMB);
                    }
                }
                break;

            case O_PLAYER_STIRRING:
                if (cave->kill_player) {
                    explode (cave, x, y);
                    break;
                }
                gd_sound_play(cave, GD_S_STIRRING); /* stirring sound, if no other walking sound or explosion */
                cave->player_seen_ago=0;
                /* bd4 intermission caves have many players. so if one of them has exited,
                 * do not change the flag anymore. so this if () is needed */
                if (cave->player_state!=GD_PL_EXITED)
                    cave->player_state=GD_PL_LIVING;
                if (player_fire) {
                    /* player "exits" stirring the pot by pressing fire */
                    cave->gravity_disabled=FALSE;
                    store(cave, x, y, O_PLAYER);
                    cave->gravity_switch_active=TRUE;
                }
                break;

            /* player holding pneumatic hammer */
            case O_PLAYER_PNEUMATIC_LEFT:
            case O_PLAYER_PNEUMATIC_RIGHT:
                /* usual player stuff */
                if (cave->kill_player) {
                    explode (cave, x, y);
                    break;
                }
                cave->player_seen_ago=0;
                if (cave->player_state!=GD_PL_EXITED)
                    cave->player_state=GD_PL_LIVING;
                if (cave->pneumatic_hammer_active_delay==0)    /* if hammering time is up, becomes a normal player again. */
                    store(cave, x, y, O_PLAYER);
                break;

            /* the active pneumatic hammer itself */
            case O_PNEUMATIC_ACTIVE_RIGHT:
            case O_PNEUMATIC_ACTIVE_LEFT:
                if (cave->pneumatic_hammer_active_delay==0) {
                    GdElement new_elem;

                    store(cave, x, y, O_SPACE);    /* pneumatic hammer element disappears */
                    /* which is the new element which appears after that one is hammered? */
                    new_elem=gd_element_get_hammered(get_dir(cave, x, y, MV_DOWN));
                    /* if there is a new element, display it */
                    /* O_NONE might be returned, for example if the element being hammered explodes during hammering (by a nearby explosion) */
                    if (new_elem!=O_NONE) {
                        store_dir(cave, x, y, MV_DOWN, new_elem);

                        /* and if walls reappear, remember it in array */
                        if (cave->hammered_walls_reappear) {
                            int wall_y;

                            wall_y=(y+1)%cave->h;
                            cave->hammered_reappear[wall_y][x]=cave->hammered_wall_reappear_frame;
                        }
                    }
                }
                break;


            /*
             *     S T O N E S,   D I A M O N D S
             */
            case O_STONE:    /* standing stone */
                do_start_fall(cave, x, y, cave->gravity, cave->stone_falling_effect);
                break;

            case O_MEGA_STONE:    /* standing mega_stone */
                do_start_fall(cave, x, y, cave->gravity, O_MEGA_STONE_F);
                break;

            case O_DIAMOND:    /* standing diamond */
                do_start_fall(cave, x, y, cave->gravity, cave->diamond_falling_effect);
                break;

            case O_NUT:    /* standing nut */
                do_start_fall(cave, x, y, cave->gravity, O_NUT_F);
                break;

            case O_DIRT_BALL:    /* standing dirt ball */
                do_start_fall(cave, x, y, cave->gravity, O_DIRT_BALL_F);
                break;

            case O_DIRT_LOOSE:    /* standing loose dirt */
                do_start_fall(cave, x, y, cave->gravity, O_DIRT_LOOSE_F);
                break;

            case O_FLYING_STONE:    /* standing stone */
                do_start_fall(cave, x, y, opposite[cave->gravity], O_FLYING_STONE_F);
                break;

            case O_FLYING_DIAMOND:    /* standing diamond */
                do_start_fall(cave, x, y, opposite[cave->gravity], O_FLYING_DIAMOND_F);
                break;

            /*
             *     F A L L I N G    E L E M E N T S,    F L Y I N G   S T O N E S,   D I A M O N D S
             */
            case O_DIRT_BALL_F:    /* falling dirt ball */
                if (!cave->gravity_disabled)
                    do_fall_roll_or_stop(cave, x, y, cave->gravity, O_DIRT_BALL);
                break;

            case O_DIRT_LOOSE_F:    /* falling loose dirt */
                if (!cave->gravity_disabled)
                    do_fall_roll_or_stop(cave, x, y, cave->gravity, O_DIRT_LOOSE);
                break;

            case O_STONE_F:    /* falling stone */
                if (!cave->gravity_disabled) {
                    if (do_fall_try_crush_voodoo(cave, x, y, cave->gravity)) break;
                    if (do_fall_try_crack_nut(cave, x, y, cave->gravity, cave->stone_bouncing_effect)) break;
                    if (do_fall_try_magic(cave, x, y, cave->gravity, cave->magic_stone_to)) break;
                    if (do_fall_try_crush(cave, x, y, cave->gravity)) break;
                    do_fall_roll_or_stop(cave, x, y, cave->gravity, cave->stone_bouncing_effect);
                }
                break;

            case O_MEGA_STONE_F:    /* falling mega */
                if (!cave->gravity_disabled) {
                    if (do_fall_try_crush_voodoo(cave, x, y, cave->gravity)) break;
                    if (do_fall_try_crack_nut(cave, x, y, cave->gravity, O_MEGA_STONE)) break;
                    if (do_fall_try_magic(cave, x, y, cave->gravity, cave->magic_mega_stone_to)) break;
                    if (do_fall_try_crush(cave, x, y, cave->gravity)) break;
                    do_fall_roll_or_stop(cave, x, y, cave->gravity, O_MEGA_STONE);
                }
                break;

            case O_DIAMOND_F:    /* falling diamond */
                if (!cave->gravity_disabled) {
                    if (do_fall_try_eat_voodoo(cave, x, y, cave->gravity)) break;
                    if (do_fall_try_magic(cave, x, y, cave->gravity, cave->magic_diamond_to)) break;
                    if (do_fall_try_crush(cave, x, y, cave->gravity)) break;
                    do_fall_roll_or_stop(cave, x, y, cave->gravity, cave->diamond_bouncing_effect);
                }
                break;

            case O_NUT_F:    /* falling nut */
                if (!cave->gravity_disabled) {
                    if (do_fall_try_magic(cave, x, y, cave->gravity, cave->magic_nut_to)) break;
                    if (do_fall_try_crush(cave, x, y, cave->gravity)) break;
                    do_fall_roll_or_stop(cave, x, y, cave->gravity, O_NUT);
                }
                break;

            case O_FLYING_STONE_F:    /* falling stone */
                if (!cave->gravity_disabled) {
                    GdDirection fall_dir=opposite[cave->gravity];

                    if (do_fall_try_crush_voodoo(cave, x, y, fall_dir)) break;
                    if (do_fall_try_crack_nut(cave, x, y, fall_dir, O_FLYING_STONE)) break;
                    if (do_fall_try_magic(cave, x, y, fall_dir, cave->magic_flying_stone_to)) break;
                    if (do_fall_try_crush(cave, x, y, fall_dir)) break;
                    do_fall_roll_or_stop(cave, x, y, fall_dir, O_FLYING_STONE);
                }
                break;

            case O_FLYING_DIAMOND_F:    /* falling diamond */
                if (!cave->gravity_disabled) {
                    GdDirection fall_dir=opposite[cave->gravity];

                    if (do_fall_try_eat_voodoo(cave, x, y, fall_dir)) break;
                    if (do_fall_try_magic(cave, x, y, fall_dir, cave->magic_flying_diamond_to)) break;
                    if (do_fall_try_crush(cave, x, y, fall_dir)) break;
                    do_fall_roll_or_stop(cave, x, y, fall_dir, O_FLYING_DIAMOND);
                }
                break;



            /*
             * N I T R O    P A C K
             */
            case O_NITRO_PACK:    /* standing nitro pack */
                do_start_fall(cave, x, y, cave->gravity, O_NITRO_PACK_F);
                break;

            case O_NITRO_PACK_F:    /* falling nitro pack */
                if (!cave->gravity_disabled) {
                    if (is_space_dir(cave, x, y, cave->gravity))    /* if space, falling further */
                        move(cave, x, y, cave->gravity, get(cave, x, y));
                    else if (do_fall_try_magic(cave, x, y, cave->gravity, cave->magic_nitro_pack_to)) {
                        /* try magic wall; if true, function did the work */
                    }
                    else if (is_element_dir(cave, x, y, cave->gravity, O_DIRT)) {
                        /* falling on a dirt, it does NOT explode - just stops at its place. */
                        play_sound_of_element(cave, O_NITRO_PACK);
                        store(cave, x, y, O_NITRO_PACK);
                    }
                    else
                        /* falling on any other element it explodes */
                        explode(cave, x, y);
                }
                break;
            case O_NITRO_PACK_EXPLODE:    /* a triggered nitro pack */
                explode(cave, x, y);
                break;



            /*
             *     C R E A T U R E S
             */

            case O_COW_1:
            case O_COW_2:
            case O_COW_3:
            case O_COW_4:
                /* if cannot move in any direction, becomes an enclosed cow */
                if (!is_space_dir(cave, x, y, MV_UP) && !is_space_dir(cave, x, y, MV_DOWN)
                    && !is_space_dir(cave, x, y, MV_LEFT) && !is_space_dir(cave, x, y, MV_RIGHT))
                    store(cave, x, y, O_COW_ENCLOSED_1);
                else {
                    /* THIS IS THE CREATURE MOVE thing copied. */
                    const GdDirection *creature_move;
                    gboolean ccw=rotates_ccw(cave, x, y);    /* check if default is counterclockwise */
                    GdElement base;    /* base element number (which is like O_***_1) */
                    int dir, dirn, dirp;    /* direction */

                    base=O_COW_1;

                    dir=get(cave, x, y)-base;    /* facing where */
                    creature_move=cave->creatures_backwards? creature_chdir:creature_dir;

                    /* now change direction if backwards */
                    if (cave->creatures_backwards)
                        ccw=!ccw;

                    if (ccw) {
                        dirn=(dir+3)&3;    /* fast turn */
                        dirp=(dir+1)&3;    /* slow turn */
                    } else {
                        dirn=(dir+1)&3; /* fast turn */
                        dirp=(dir+3)&3;    /* slow turn */
                    }

                    if (is_space_dir(cave, x, y, creature_move[dirn]))
                        move(cave, x, y, creature_move[dirn], base+dirn);    /* turn and move to preferred dir */
                    else if (is_space_dir(cave, x, y, creature_move[dir]))
                        move(cave, x, y, creature_move[dir], base+dir);    /* go on */
                    else
                        store(cave, x, y, base+dirp);    /* turn in place if nothing else possible */
                }
                break;
            /* enclosed cows wait some time before turning to a skeleton */
            case O_COW_ENCLOSED_1:
            case O_COW_ENCLOSED_2:
            case O_COW_ENCLOSED_3:
            case O_COW_ENCLOSED_4:
            case O_COW_ENCLOSED_5:
            case O_COW_ENCLOSED_6:
                if (is_space_dir(cave, x, y, MV_UP) || is_space_dir(cave, x, y, MV_LEFT) || is_space_dir(cave, x, y, MV_RIGHT) || is_space_dir(cave, x, y, MV_DOWN))
                    store(cave, x, y, O_COW_1);
                else
                    next(cave, x, y);
                break;
            case O_COW_ENCLOSED_7:
                if (is_space_dir(cave, x, y, MV_UP) || is_space_dir(cave, x, y, MV_LEFT) || is_space_dir(cave, x, y, MV_RIGHT) || is_space_dir(cave, x, y, MV_DOWN))
                    store(cave, x, y, O_COW_1);
                else
                    store(cave, x, y, O_SKELETON);
                break;

            case O_FIREFLY_1:
            case O_FIREFLY_2:
            case O_FIREFLY_3:
            case O_FIREFLY_4:
            case O_ALT_FIREFLY_1:
            case O_ALT_FIREFLY_2:
            case O_ALT_FIREFLY_3:
            case O_ALT_FIREFLY_4:
            case O_BUTTER_1:
            case O_BUTTER_2:
            case O_BUTTER_3:
            case O_BUTTER_4:
            case O_ALT_BUTTER_1:
            case O_ALT_BUTTER_2:
            case O_ALT_BUTTER_3:
            case O_ALT_BUTTER_4:
            case O_STONEFLY_1:
            case O_STONEFLY_2:
            case O_STONEFLY_3:
            case O_STONEFLY_4:
                /* check if touches a voodoo */
                if (get_dir(cave, x, y, MV_LEFT)==O_VOODOO || get_dir(cave, x, y, MV_RIGHT)==O_VOODOO || get_dir(cave, x, y, MV_UP)==O_VOODOO || get_dir(cave, x, y, MV_DOWN)==O_VOODOO)
                    cave->voodoo_touched=TRUE;
                /* check if touches something bad and should explode (includes voodoo by the flags) */
                if (blows_up_flies_dir(cave, x, y, MV_DOWN) || blows_up_flies_dir(cave, x, y, MV_UP)
                    || blows_up_flies_dir(cave, x, y, MV_LEFT) || blows_up_flies_dir(cave, x, y, MV_RIGHT))
                    explode (cave, x, y);
                /* otherwise move */
                else {
                    const GdDirection *creature_move;
                    gboolean ccw=rotates_ccw(cave, x, y);    /* check if default is counterclockwise */
                    GdElement base;    /* base element number (which is like O_***_1) */
                    int dir, dirn, dirp;    /* direction */

                    if (get(cave, x, y)>=O_FIREFLY_1 && get(cave, x, y)<=O_FIREFLY_4)
                        base=O_FIREFLY_1;
                    else if (get(cave, x, y)>=O_BUTTER_1 && get(cave, x, y)<=O_BUTTER_4)
                        base=O_BUTTER_1;
                    else if (get(cave, x, y)>=O_STONEFLY_1 && get(cave, x, y)<=O_STONEFLY_4)
                        base=O_STONEFLY_1;
                    else if (get(cave, x, y)>=O_ALT_FIREFLY_1 && get(cave, x, y)<=O_ALT_FIREFLY_4)
                        base=O_ALT_FIREFLY_1;
                    else if (get(cave, x, y)>=O_ALT_BUTTER_1 && get(cave, x, y)<=O_ALT_BUTTER_4)
                        base=O_ALT_BUTTER_1;
                    else
                        g_assert_not_reached();

                    dir=get(cave, x, y)-base;    /* facing where */
                    creature_move=cave->creatures_backwards? creature_chdir:creature_dir;

                    /* now change direction if backwards */
                    if (cave->creatures_backwards)
                        ccw=!ccw;

                    if (ccw) {
                        dirn=(dir+3)&3;    /* fast turn */
                        dirp=(dir+1)&3;    /* slow turn */
                    } else {
                        dirn=(dir+1)&3; /* fast turn */
                        dirp=(dir+3)&3;    /* slow turn */
                    }

                    if (is_space_dir(cave, x, y, creature_move[dirn]))
                        move(cave, x, y, creature_move[dirn], base+dirn);    /* turn and move to preferred dir */
                    else if (is_space_dir(cave, x, y, creature_move[dir]))
                        move(cave, x, y, creature_move[dir], base+dir);    /* go on */
                    else
                        store(cave, x, y, base+dirp);    /* turn in place if nothing else possible */
                }
                break;

            case O_WAITING_STONE:
                if (is_space_dir(cave, x, y, grav_compat)) {    /* beginning to fall */
                    /* it wakes up. */
                    move(cave, x, y, grav_compat, O_CHASING_STONE);
                }
                else if (sloped_dir(cave, x, y, grav_compat, opposite[grav_compat])) {    /* rolling down a brick wall or a stone */
                    if (sloped_dir(cave, x, y, grav_compat, cw_fourth[grav_compat]) && is_space_dir(cave, x, y, cw_fourth[grav_compat]) && is_space_dir(cave, x, y, cw_eighth[grav_compat])) {
                        /* maybe rolling left - see case O_STONE to understand why we use cw_fourth here */
                        move(cave, x, y, cw_fourth[grav_compat], O_WAITING_STONE);
                    }
                    else if (sloped_dir(cave, x, y, grav_compat, ccw_fourth[grav_compat]) && is_space_dir(cave, x, y, ccw_fourth[grav_compat]) && is_space_dir(cave, x, y, ccw_eighth[grav_compat])) {
                        /* or maybe right */
                        move(cave, x, y, ccw_fourth[grav_compat], O_WAITING_STONE);
                    }
                }
                break;

            case O_CHASING_STONE:
                {
                    int px=cave->px[0];
                    int py=cave->py[0];
                    gboolean horizontal=g_rand_boolean(cave->random);
                    gboolean dont_move=FALSE;
                    int i=3;

                    /* try to move... */
                    while (1) {
                        if (horizontal) {    /*********************************/
                            /* check for a horizontal movement */
                            if (px==x) {
                                /* if coordinates are the same */
                                i-=1;
                                horizontal=!horizontal;
                                if (i==2)
                                    continue;
                            } else {
                                if (px>x && is_space_dir(cave, x, y, MV_RIGHT)) {
                                    move(cave, x, y, MV_RIGHT, O_CHASING_STONE);
                                    dont_move=TRUE;
                                    break;
                                } else
                                if (px<x && is_space_dir(cave, x, y, MV_LEFT)) {
                                    move(cave, x, y, MV_LEFT, O_CHASING_STONE);
                                    dont_move=TRUE;
                                    break;
                                } else {
                                    i-=2;
                                    if (i==1) {
                                        horizontal=!horizontal;
                                        continue;
                                    }
                                }
                            }
                        } else {    /********************************/
                            /* check for a vertical movement */
                            if (py==y) {
                                /* if coordinates are the same */
                                i-=1;
                                horizontal=!horizontal;
                                if (i==2)
                                    continue;
                            } else {
                                if (py>y && is_space_dir(cave, x, y, MV_DOWN)) {
                                    move(cave, x, y, MV_DOWN, O_CHASING_STONE);
                                    dont_move=TRUE;
                                    break;
                                } else
                                if (py<y && is_space_dir(cave, x, y, MV_UP)) {
                                    move(cave, x, y, MV_UP, O_CHASING_STONE);
                                    dont_move=TRUE;
                                    break;
                                } else {
                                    i-=2;
                                    if (i==1) {
                                        horizontal=!horizontal;
                                        continue;
                                    }
                                }
                            }
                        }
                        if (i!=0)
                            dont_move=TRUE;
                        break;
                    }

                    /* if we should move in both directions, but can not move in any, stop. */
                    if (!dont_move) {
                        if (horizontal) {    /* check for horizontal */
                             if (x>=px) {
                                 if (is_space_dir(cave, x, y, MV_UP) && is_space_dir(cave, x, y, MV_UP_LEFT))
                                     move(cave, x, y, MV_UP, O_CHASING_STONE);
                                 else
                                 if (is_space_dir(cave, x, y, MV_DOWN) && is_space_dir(cave, x, y, MV_DOWN_LEFT))
                                     move(cave, x, y, MV_DOWN, O_CHASING_STONE);
                             } else {
                                 if (is_space_dir(cave, x, y, MV_UP) && is_space_dir(cave, x, y, MV_UP_RIGHT))
                                     move(cave, x, y, MV_UP, O_CHASING_STONE);
                                 else
                                 if (is_space_dir(cave, x, y, MV_DOWN) && is_space_dir(cave, x, y, MV_DOWN_RIGHT))
                                     move(cave, x, y, MV_DOWN, O_CHASING_STONE);
                             }
                        } else {    /* check for vertical */
                            if (y>=py) {
                                if (is_space_dir(cave, x, y, MV_LEFT) && is_space_dir(cave, x, y, MV_UP_LEFT))
                                    move(cave, x, y, MV_LEFT, O_CHASING_STONE);
                                else
                                if (is_space_dir(cave, x, y, MV_RIGHT) && is_space_dir(cave, x, y, MV_UP_RIGHT))
                                    move(cave, x, y, MV_RIGHT, O_CHASING_STONE);
                            } else {
                                if (is_space_dir(cave, x, y, MV_LEFT) && is_space_dir(cave, x, y, MV_DOWN_LEFT))
                                    move(cave, x, y, MV_LEFT, O_CHASING_STONE);
                                else
                                if (is_space_dir(cave, x, y, MV_RIGHT) && is_space_dir(cave, x, y, MV_DOWN_RIGHT))
                                    move(cave, x, y, MV_RIGHT, O_CHASING_STONE);
                            }
                        }
                    }
                }
                break;

            case O_REPLICATOR:
                if (cave->replicators_wait_frame==0 && cave->replicators_active && !cave->gravity_disabled) {
                    /* only replicate, if space is under it. */
                    /* do not replicate players! */
                    /* also obeys gravity settings. */
                    /* only replicate element if it is not a scanned one */
                    /* do not replicate space... that condition looks like it makes no sense,
                       but otherwise it generates SCANNED spaces, which cannot be "collected" by the player, so he cannot run under a replicator */
                    if (is_space_dir(cave, x, y, cave->gravity) && !is_player_dir(cave, x, y, opposite[cave->gravity])
                        && !is_space_dir(cave, x, y, opposite[cave->gravity])) {
                        store_dir(cave, x, y, cave->gravity, get_dir(cave, x, y, opposite[cave->gravity]));
                        gd_sound_play(cave, GD_S_REPLICATOR);
                    }
                }
                break;

            case O_BITER_1:
            case O_BITER_2:
            case O_BITER_3:
            case O_BITER_4:
                if (cave->biters_wait_frame==0) {
                    static GdDirection biter_move[]={ MV_UP, MV_RIGHT, MV_DOWN, MV_LEFT };
                    int dir=get(cave, x, y)-O_BITER_1;    /* direction, last two bits 0..3 */
                    int dirn=(dir+3)&3;
                    int dirp=(dir+1)&3;
                    int i;
                    GdElement made_sound_of=O_NONE;

                    for (i=0; i<G_N_ELEMENTS (biter_try); i++) {
                        if (is_element_dir(cave, x, y, biter_move[dir], biter_try[i])) {
                            move(cave, x, y, biter_move[dir], O_BITER_1+dir);
                            if (biter_try[i]!=O_SPACE)
                                made_sound_of=O_BITER_1;    /* sound of a biter eating */
                            break;
                        }
                        else if (is_element_dir(cave, x, y, biter_move[dirn], biter_try[i])) {
                            move(cave, x, y, biter_move[dirn], O_BITER_1+dirn);
                            if (biter_try[i]!=O_SPACE)
                                made_sound_of=O_BITER_1;    /* sound of a biter eating */
                            break;
                        }
                        else if (is_element_dir(cave, x, y, biter_move[dirp], biter_try[i])) {
                            move(cave, x, y, biter_move[dirp], O_BITER_1+dirp);
                            if (biter_try[i]!=O_SPACE)
                                made_sound_of=O_BITER_1;    /* sound of a biter eating */
                            break;
                        }
                    }
                    if (i==G_N_ELEMENTS (biter_try))
                        /* i=number of elements in array: could not move, so just turn */
                        store(cave, x, y, O_BITER_1+dirp);
                    else if (biter_try[i]==O_STONE) {
                        /* if there was a stone there, where we moved... do not eat stones, just throw them back */
                        store(cave, x, y, O_STONE);
                        made_sound_of=O_STONE;
                    }

                    /* if biter did move, we had sound. play it. */
                    if (made_sound_of!=O_NONE)
                        play_sound_of_element(cave, made_sound_of);
                }
                break;

            case O_DRAGONFLY_1:
            case O_DRAGONFLY_2:
            case O_DRAGONFLY_3:
            case O_DRAGONFLY_4:
                /* check if touches a voodoo */
                if (get_dir(cave, x, y, MV_LEFT)==O_VOODOO || get_dir(cave, x, y, MV_RIGHT)==O_VOODOO || get_dir(cave, x, y, MV_UP)==O_VOODOO || get_dir(cave, x, y, MV_DOWN)==O_VOODOO)
                    cave->voodoo_touched=TRUE;
                /* check if touches something bad and should explode (includes voodoo by the flags) */
                if (blows_up_flies_dir(cave, x, y, MV_DOWN) || blows_up_flies_dir(cave, x, y, MV_UP)
                    || blows_up_flies_dir(cave, x, y, MV_LEFT) || blows_up_flies_dir(cave, x, y, MV_RIGHT))
                    explode (cave, x, y);
                /* otherwise move */
                else {
                    const GdDirection *creature_move;
                    gboolean ccw=rotates_ccw(cave, x, y);    /* check if default is counterclockwise */
                    GdElement base=O_DRAGONFLY_1;    /* base element number (which is like O_***_1) */
                    int dir, dirn;    /* direction */

                    dir=get(cave, x, y)-base;    /* facing where */
                    creature_move=cave->creatures_backwards? creature_chdir:creature_dir;

                    /* now change direction if backwards */
                    if (cave->creatures_backwards)
                        ccw=!ccw;

                    if (ccw)
                        dirn=(dir+3)&3;    /* fast turn */
                    else
                        dirn=(dir+1)&3; /* fast turn */

                    /* if can move forward, does so. */
                    if (is_space_dir(cave, x, y, creature_move[dir]))
                        move(cave, x, y, creature_move[dir], base+dir);
                    else
                    /* otherwise turns 90 degrees in place. */
                        store(cave, x, y, base+dirn);
                }
                break;


            case O_BLADDER:
                store(cave, x, y, O_BLADDER_1);
                break;

            case O_BLADDER_1:
            case O_BLADDER_2:
            case O_BLADDER_3:
            case O_BLADDER_4:
            case O_BLADDER_5:
            case O_BLADDER_6:
            case O_BLADDER_7:
            case O_BLADDER_8:
                /* bladder with any delay state: try to convert to clock. */
                if (is_element_dir(cave, x, y, opposite[grav_compat], cave->bladder_converts_by)
                    || is_element_dir(cave, x, y, cw_fourth[grav_compat], cave->bladder_converts_by)
                    || is_element_dir(cave, x, y, ccw_fourth[grav_compat], cave->bladder_converts_by)) {
                    /* if touches the specified element, let it be a clock */
                    store(cave, x, y, O_PRE_CLOCK_1);
                    play_sound_of_element(cave, O_PRE_CLOCK_1);    /* plays the bladder convert sound */
                } else {
                    /* is space over the bladder? */
                    if (is_space_dir(cave, x, y, opposite[grav_compat])) {
                        if (get(cave, x, y)==O_BLADDER_8) {
                            /* if it is a bladder 8, really move up */
                            move(cave, x, y, opposite[grav_compat], O_BLADDER_1);
                            play_sound_of_element(cave, O_BLADDER);
                        }
                        else
                            /* if smaller delay, just increase delay. */
                            next(cave, x, y);
                    }
                    else
                    /* if not space, is something sloped over the bladder? */
                    if (sloped_for_bladder_dir(cave, x, y, opposite[grav_compat]) && sloped_dir(cave, x, y, opposite[grav_compat], opposite[grav_compat])) {
                        if (sloped_dir(cave, x, y, opposite[grav_compat], ccw_fourth[opposite[grav_compat]])
                            && is_space_dir(cave, x, y, ccw_fourth[opposite[grav_compat]])
                            && is_space_dir(cave, x, y, ccw_eighth[opposite[grav_compat]])) {
                            /* rolling up, to left */
                            if (get(cave, x, y)==O_BLADDER_8) {
                                /* if it is a bladder 8, really roll */
                                move(cave, x, y, ccw_fourth[opposite[grav_compat]], O_BLADDER_8);
                                play_sound_of_element(cave, O_BLADDER);
                            } else
                                /* if smaller delay, just increase delay. */
                                next(cave, x, y);
                        }
                        else
                        if (sloped_dir(cave, x, y, opposite[grav_compat], cw_fourth[opposite[grav_compat]])
                            && is_space_dir(cave, x, y, cw_fourth[opposite[grav_compat]])
                            && is_space_dir(cave, x, y, cw_eighth[opposite[grav_compat]])) {
                            /* rolling up, to left */
                            if (get(cave, x, y)==O_BLADDER_8) {
                                /* if it is a bladder 8, really roll */
                                move(cave, x, y, cw_fourth[opposite[grav_compat]], O_BLADDER_8);
                                play_sound_of_element(cave, O_BLADDER);
                            } else
                                /* if smaller delay, just increase delay. */
                                next(cave, x, y);
                        }
                    }
                    /* no space, no sloped thing over it - store bladder 1 and that is for now. */
                    else
                        store(cave, x, y, O_BLADDER_1);
                }
                break;

            case O_GHOST:
                if (blows_up_flies_dir(cave, x, y, MV_DOWN) || blows_up_flies_dir(cave, x, y, MV_UP)
                    || blows_up_flies_dir(cave, x, y, MV_LEFT) || blows_up_flies_dir(cave, x, y, MV_RIGHT))
                    explode (cave, x, y);
                else {
                    int i;

                    /* the ghost is given four possibilities to move. */
                    for (i=0; i<4; i++) {
                        static GdDirection dirs[]={MV_UP, MV_DOWN, MV_LEFT, MV_RIGHT};
                        GdDirection random_dir;

                        random_dir=dirs[g_rand_int_range(cave->random, 0, G_N_ELEMENTS(dirs))];
                        if (is_space_dir(cave, x, y, random_dir)) {
                            move(cave, x, y, random_dir, O_GHOST);
                            break;    /* ghost did move -> exit loop */
                        }
                    }
                }
                break;



            /*
             *     A C T I V E    E L E M E N T S
             */

            case O_AMOEBA:
                amoeba_count++;
                switch (cave->amoeba_state) {
                    case GD_AM_TOO_BIG:
                        store(cave, x, y, cave->amoeba_too_big_effect);
                        break;
                    case GD_AM_ENCLOSED:
                        store(cave, x, y, cave->amoeba_enclosed_effect);
                        break;
                    case GD_AM_SLEEPING:
                    case GD_AM_AWAKE:
                        /* if no amoeba found during THIS SCAN yet, which was able to grow, check this one. */
                        if (amoeba_found_enclosed)
                            /* if still found enclosed, check all four directions, if this one is able to grow. */
                            if (amoeba_eats_dir(cave, x, y, MV_UP) || amoeba_eats_dir(cave, x, y, MV_DOWN)
                                || amoeba_eats_dir(cave, x, y, MV_LEFT) || amoeba_eats_dir(cave, x, y, MV_RIGHT)) {
                                amoeba_found_enclosed=FALSE;    /* not enclosed. this is a local (per scan) flag! */
                                cave->amoeba_state=GD_AM_AWAKE;
                            }

                        /* if alive, check in which dir to grow (or not) */
                        if (cave->amoeba_state==GD_AM_AWAKE) {
                            if (g_rand_int_range(cave->random, 0, 1000000)<cave->amoeba_growth_prob) {
                                switch (g_rand_int_range(cave->random, 0, 4)) {    /* decided to grow, choose a random direction. */
                                case 0:    /* let this be up. numbers indifferent. */
                                    if (amoeba_eats_dir(cave, x, y, MV_UP))
                                        store_dir(cave, x, y, MV_UP, O_AMOEBA);
                                    break;
                                case 1:    /* down */
                                    if (amoeba_eats_dir(cave, x, y, MV_DOWN))
                                        store_dir(cave, x, y, MV_DOWN, O_AMOEBA);
                                    break;
                                case 2:    /* left */
                                    if (amoeba_eats_dir(cave, x, y, MV_LEFT))
                                        store_dir(cave, x, y, MV_LEFT, O_AMOEBA);
                                    break;
                                case 3:    /* right */
                                    if (amoeba_eats_dir(cave, x, y, MV_RIGHT))
                                        store_dir(cave, x, y, MV_RIGHT, O_AMOEBA);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                break;

            case O_AMOEBA_2:
                amoeba_2_count++;
                /* check if it is touching an amoeba, and explosion is enabled */
                if (cave->amoeba_2_explodes_by_amoeba
                    && (is_element_dir(cave, x, y, MV_DOWN, O_AMOEBA) || is_element_dir(cave, x, y, MV_UP, O_AMOEBA)
                        || is_element_dir(cave, x, y, MV_LEFT, O_AMOEBA) || is_element_dir(cave, x, y, MV_RIGHT, O_AMOEBA)))
                        explode (cave, x, y);
                else
                    switch (cave->amoeba_2_state) {
                        case GD_AM_TOO_BIG:
                            store(cave, x, y, cave->amoeba_2_too_big_effect);
                            break;
                        case GD_AM_ENCLOSED:
                            store(cave, x, y, cave->amoeba_2_enclosed_effect);
                            break;
                        case GD_AM_SLEEPING:
                        case GD_AM_AWAKE:
                            /* if no amoeba found during THIS SCAN yet, which was able to grow, check this one. */
                            if (amoeba_2_found_enclosed)
                                if (amoeba_eats_dir(cave, x, y, MV_UP) || amoeba_eats_dir(cave, x, y, MV_DOWN)
                                    || amoeba_eats_dir(cave, x, y, MV_LEFT) || amoeba_eats_dir(cave, x, y, MV_RIGHT)) {
                                    amoeba_2_found_enclosed=FALSE;    /* not enclosed. this is a local (per scan) flag! */
                                    cave->amoeba_2_state=GD_AM_AWAKE;
                                }

                            if (cave->amoeba_2_state==GD_AM_AWAKE)    /* if it is alive, decide if it attempts to grow */
                                if (g_rand_int_range(cave->random, 0, 1000000)<cave->amoeba_2_growth_prob) {
                                    switch (g_rand_int_range(cave->random, 0, 4)) {    /* decided to grow, choose a random direction. */
                                    case 0:    /* let this be up. numbers indifferent. */
                                        if (amoeba_eats_dir(cave, x, y, MV_UP))
                                            store_dir(cave, x, y, MV_UP, O_AMOEBA_2);
                                        break;
                                    case 1:    /* down */
                                        if (amoeba_eats_dir(cave, x, y, MV_DOWN))
                                            store_dir(cave, x, y, MV_DOWN, O_AMOEBA_2);
                                        break;
                                    case 2:    /* left */
                                        if (amoeba_eats_dir(cave, x, y, MV_LEFT))
                                            store_dir(cave, x, y, MV_LEFT, O_AMOEBA_2);
                                        break;
                                    case 3:    /* right */
                                        if (amoeba_eats_dir(cave, x, y, MV_RIGHT))
                                            store_dir(cave, x, y, MV_RIGHT, O_AMOEBA_2);
                                        break;
                                    }
                                }
                            break;
                    }
                break;

            case O_ACID:
                /* choose randomly, if it spreads */
                if (g_rand_int_range(cave->random, 0, 1000000)<=cave->acid_spread_ratio) {
                    /* the current one explodes */
                    store(cave, x, y, cave->acid_turns_to);
                    /* and if neighbours are eaten, put acid there. */
                    if (is_element_dir(cave, x, y, MV_UP, cave->acid_eats_this)) {
                        play_sound_of_element(cave, O_ACID);
                        store_dir(cave, x, y, MV_UP, O_ACID);
                    }
                    if (is_element_dir(cave, x, y, MV_DOWN, cave->acid_eats_this)) {
                        play_sound_of_element(cave, O_ACID);
                        store_dir(cave, x, y, MV_DOWN, O_ACID);
                    }
                    if (is_element_dir(cave, x, y, MV_LEFT, cave->acid_eats_this)) {
                        play_sound_of_element(cave, O_ACID);
                        store_dir(cave, x, y, MV_LEFT, O_ACID);
                    }
                    if (is_element_dir(cave, x, y, MV_RIGHT, cave->acid_eats_this)) {
                        play_sound_of_element(cave, O_ACID);
                        store_dir(cave, x, y, MV_RIGHT, O_ACID);
                    }
                }
                break;

            case O_WATER:
                found_water=TRUE;
                if (!cave->water_does_not_flow_down && is_space_dir(cave, x, y, MV_DOWN))    /* emulating the odd behaviour in crdr */
                    store_dir(cave, x, y, MV_DOWN, O_WATER_1);
                if (is_space_dir(cave, x, y, MV_UP))
                    store_dir(cave, x, y, MV_UP, O_WATER_1);
                if (is_space_dir(cave, x, y, MV_LEFT))
                    store_dir(cave, x, y, MV_LEFT, O_WATER_1);
                if (is_space_dir(cave, x, y, MV_RIGHT))
                    store_dir(cave, x, y, MV_RIGHT, O_WATER_1);
                break;

            case O_WATER_16:
                store(cave, x, y, O_WATER);
                break;

            case O_H_EXPANDING_WALL:
            case O_V_EXPANDING_WALL:
            case O_H_EXPANDING_STEEL_WALL:
            case O_V_EXPANDING_STEEL_WALL:
                /* checks first if direction is changed. */
                if (((get(cave, x, y)==O_H_EXPANDING_WALL || get(cave, x, y)==O_H_EXPANDING_STEEL_WALL) && !cave->expanding_wall_changed)
                    || ((get(cave, x, y)==O_V_EXPANDING_WALL || get(cave, x, y)==O_V_EXPANDING_STEEL_WALL) && cave->expanding_wall_changed)) {
                    if (is_space_dir(cave, x, y, MV_LEFT)) {
                        store_dir(cave, x, y, MV_LEFT, get(cave, x, y));
                        play_sound_of_element(cave, get(cave, x, y));
                    }
                    if (is_space_dir(cave, x, y, MV_RIGHT)) {
                        store_dir(cave, x, y, MV_RIGHT, get(cave, x, y));
                        play_sound_of_element(cave, get(cave, x, y));
                    }
                }
                else {
                    if (is_space_dir(cave, x, y, MV_UP)) {
                        store_dir(cave, x, y, MV_UP, get(cave, x, y));
                        play_sound_of_element(cave, get(cave, x, y));
                    }
                    if (is_space_dir(cave, x, y, MV_DOWN)) {
                        store_dir(cave, x, y, MV_DOWN, get(cave, x, y));
                        play_sound_of_element(cave, get(cave, x, y));
                    }
                }
                break;

            case O_EXPANDING_WALL:
            case O_EXPANDING_STEEL_WALL:
                /* the wall which grows in all four directions. */
                if (is_space_dir(cave, x, y, MV_LEFT)) {
                    store_dir(cave, x, y, MV_LEFT, get(cave, x, y));
                    play_sound_of_element(cave, get(cave, x, y));
                }
                if (is_space_dir(cave, x, y, MV_RIGHT)) {
                    store_dir(cave, x, y, MV_RIGHT, get(cave, x, y));
                    play_sound_of_element(cave, get(cave, x, y));
                }
                if (is_space_dir(cave, x, y, MV_UP)) {
                    store_dir(cave, x, y, MV_UP, get(cave, x, y));
                    play_sound_of_element(cave, get(cave, x, y));
                }
                if (is_space_dir(cave, x, y, MV_DOWN)) {
                    store_dir(cave, x, y, MV_DOWN, get(cave, x, y));
                    play_sound_of_element(cave, get(cave, x, y));
                }
                break;

            case O_SLIME:
                g_print("Step[%03d]", cave->frame); /* XXX */
                int rrr=gd_cave_c64_random(cave);
                g_print(".Rand[%03d].Perm[%03d].Result[%d]\n", rrr, cave->slime_permeability_c64, (rrr&cave->slime_permeability_c64)==0);
                /*
                 * unpredictable: g_rand_int
                 * predictable: c64 predictable random generator.
                 *    for predictable, a random number is generated, whether or not it is even possible that the stone
                 *    will be able to pass.
                 */
                if (cave->slime_predictable? ((rrr /* XXX */ &cave->slime_permeability_c64)==0) : g_rand_int_range(cave->random, 0, 1000000)<cave->slime_permeability) {
                    GdDirection grav=cave->gravity;
                    GdDirection oppos=opposite[cave->gravity];

                    /* space under the slime? elements may pass from top to bottom then. */
                    if (is_space_dir(cave, x, y, grav)) {
                        if (get_dir(cave, x, y, oppos)==cave->slime_eats_1) {
                            store_dir(cave, x, y, grav, cave->slime_converts_1);    /* output a falling xy under */
                            store_dir(cave, x, y, oppos, O_SPACE);
                            play_sound_of_element(cave, O_SLIME);
                        }
                        else if (get_dir(cave, x, y, oppos)==cave->slime_eats_2) {
                            store_dir(cave, x, y, grav, cave->slime_converts_2);
                            store_dir(cave, x, y, oppos, O_SPACE);
                            play_sound_of_element(cave, O_SLIME);
                        }
                        else if (get_dir(cave, x, y, oppos)==cave->slime_eats_3) {
                            store_dir(cave, x, y, grav, cave->slime_converts_3);
                            store_dir(cave, x, y, oppos, O_SPACE);
                            play_sound_of_element(cave, O_SLIME);
                        }
                        else if (get_dir(cave, x, y, oppos)==O_WAITING_STONE) {    /* waiting stones pass without awakening */
                            store_dir(cave, x, y, grav, O_WAITING_STONE);
                            store_dir(cave, x, y, oppos, O_SPACE);
                            play_sound_of_element(cave, O_SLIME);
                        }
                        else if (get_dir(cave, x, y, oppos)==O_CHASING_STONE) {    /* chasing stones pass */
                            store_dir(cave, x, y, grav, O_CHASING_STONE);
                            store_dir(cave, x, y, oppos, O_SPACE);
                            play_sound_of_element(cave, O_SLIME);
                        }
                    } else
                    /* or space over the slime? elements may pass from bottom to up then. */
                    if (is_space_dir(cave, x, y, oppos)) {
                        if (get_dir(cave, x, y, grav)==O_BLADDER) {                    /* bladders move UP the slime */
                            store_dir(cave, x, y, grav, O_SPACE);
                            store_dir(cave, x, y, oppos, O_BLADDER_1);
                            play_sound_of_element(cave, O_SLIME);
                        } else
                        if (get_dir(cave, x, y, grav)==O_FLYING_STONE) {
                            store_dir(cave, x, y, grav, O_SPACE);
                            store_dir(cave, x, y, oppos, O_FLYING_STONE_F);
                            play_sound_of_element(cave, O_SLIME);
                        } else
                        if (get_dir(cave, x, y, grav)==O_FLYING_DIAMOND) {
                            store_dir(cave, x, y, grav, O_SPACE);
                            store_dir(cave, x, y, oppos, O_FLYING_DIAMOND_F);
                            play_sound_of_element(cave, O_SLIME);
                        }
                    }
                }
                break;

            case O_FALLING_WALL:
                if (is_space_dir(cave, x, y, grav_compat)) {
                    /* try falling if space under. */
                    int yy;
                    for (yy=y+1; yy<y+cave->h; yy++)
                        /* yy<y+cave->h is to check everything OVER the wall - since caves wrap around !! */
                        if (get(cave, x, yy)!=O_SPACE)
                            /* stop cycle when other than space */
                            break;
                    /* if scanning stopped by a player... start falling! */
                    if (get(cave, x, yy)==O_PLAYER || get(cave, x, yy)==O_PLAYER_GLUED || get(cave, x, yy)==O_PLAYER_BOMB) {
                        move(cave, x, y, grav_compat, O_FALLING_WALL_F);
                        /* no sound when the falling wall starts falling! */
                    }
                }
                break;

            case O_FALLING_WALL_F:
                switch (get_dir(cave, x, y, grav_compat)) {
                    case O_PLAYER:
                    case O_PLAYER_GLUED:
                    case O_PLAYER_BOMB:
                        /* if player under, it explodes - the falling wall, not the player! */
                        explode (cave, x, y);
                        break;
                    case O_SPACE:
                        /* continue falling */
                        move(cave, x, y, grav_compat, O_FALLING_WALL_F);
                        break;
                    default:
                        /* stop */
                        play_sound_of_element(cave, get(cave, x, y));
                        store(cave, x, y, O_FALLING_WALL);
                        break;
                    }
                break;


            /*
             * C O N V E Y O R    B E L T S
             */
            case O_CONVEYOR_RIGHT:
            case O_CONVEYOR_LEFT:
                /* only works if gravity is up or down!!! */
                /* first, check for gravity and running belts. */
                if (!cave->gravity_disabled && cave->conveyor_belts_active) {
                    const GdDirection *dir;
                    gboolean left;

                    /* decide direction */
                    left=get(cave, x, y)!=O_CONVEYOR_RIGHT;
                    if (cave->conveyor_belts_direction_changed)
                        left=!left;
                    dir=left?ccw_eighth:cw_eighth;

                    /* CHECK IF IT CONVEYS THE ELEMENT ABOVE IT */
                    /* if gravity is normal, and the conveyor belt has something ABOVE which can be moved
                        OR
                       the gravity is up, so anything that should float now goes DOWN and touches the conveyor */
                    if ((cave->gravity==MV_DOWN && moved_by_conveyor_top_dir(cave, x, y, MV_UP))
                        || (cave->gravity==MV_UP && moved_by_conveyor_bottom_dir(cave, x, y, MV_UP))) {
                        if (!is_scanned_dir(cave, x, y, MV_UP) && is_space_dir(cave, x, y, dir[MV_UP]))
                        {
                            store_dir(cave, x, y, dir[MV_UP], get_dir(cave, x, y, MV_UP));    /* move */
                            store_dir(cave, x, y, MV_UP, O_SPACE);    /* and place a space. */
                        }
                    }
                    /* CHECK IF IT CONVEYS THE ELEMENT BELOW IT */
                    if ((cave->gravity==MV_UP && moved_by_conveyor_top_dir(cave, x, y, MV_DOWN))
                        || (cave->gravity==MV_DOWN && moved_by_conveyor_bottom_dir(cave, x, y, MV_DOWN))) {
                        if (!is_scanned_dir(cave, x, y, MV_DOWN) && is_space_dir(cave, x, y, dir[MV_DOWN]))
                        {
                            store_dir(cave, x, y, dir[MV_DOWN], get_dir(cave, x, y, MV_DOWN));    /* move */
                            store_dir(cave, x, y, MV_DOWN, O_SPACE);    /* and clear. */
                        }
                    }
                }
                break;

            /*
             * S I M P L E   C H A N G I N G;   E X P L O S I O N S
             */
             case O_EXPLODE_5:
                store(cave, x, y, cave->explosion_effect);
                break;
             case O_NUT_EXPL_4:
                store(cave, x, y, O_DIAMOND);
                break;
            case O_PRE_DIA_5:
                store(cave, x, y, cave->diamond_birth_effect);
                break;
            case O_PRE_STONE_4:
                store(cave, x, y, O_STONE);
                break;

            case O_NITRO_EXPL_4:
                store(cave, x, y, cave->nitro_explosion_effect);
                break;
            case O_BOMB_EXPL_4:
                store(cave, x, y, cave->bomb_explosion_effect);
                break;
            case O_AMOEBA_2_EXPL_4:
                store(cave, x, y, cave->amoeba_2_explosion_effect);
                break;

            case O_GHOST_EXPL_4:
                {
                    static GdElement ghost_explode[]={
                        O_SPACE, O_SPACE, O_DIRT, O_DIRT, O_CLOCK, O_CLOCK, O_PRE_OUTBOX,
                        O_BOMB, O_BOMB, O_PLAYER, O_GHOST, O_BLADDER, O_DIAMOND, O_SWEET,
                        O_WAITING_STONE, O_BITER_1
                    };

                    store(cave, x, y, ghost_explode[g_rand_int_range(cave->random, 0, G_N_ELEMENTS(ghost_explode))]);
                }
                break;
            case O_PRE_STEEL_4:
                store(cave, x, y, O_STEEL);
                break;
            case O_PRE_CLOCK_4:
                store(cave, x, y, O_CLOCK);
                break;
            case O_BOMB_TICK_7:
                explode(cave, x, y);
                break;

            case O_TRAPPED_DIAMOND:
                if (cave->diamond_key_collected)
                    store(cave, x, y, O_DIAMOND);
                break;

            case O_PRE_OUTBOX:
                if (cave->gate_open) /* if no more diamonds needed */
                    store(cave, x, y, O_OUTBOX);    /* open outbox */
                break;
            case O_PRE_INVIS_OUTBOX:
                if (cave->gate_open)    /* if no more diamonds needed */
                    store(cave, x, y, O_INVIS_OUTBOX);    /* open outbox. invisible one :P */
                break;
            case O_INBOX:
                if (cave->hatched && !inbox_toggle)    /* if it is time of birth */
                    store(cave, x, y, O_PRE_PL_1);
                inbox_toggle=!inbox_toggle;
                break;
            case O_PRE_PL_3:
                store(cave, x, y, O_PLAYER);
                break;

            case O_PRE_DIA_1:
            case O_PRE_DIA_2:
            case O_PRE_DIA_3:
            case O_PRE_DIA_4:
            case O_PRE_STONE_1:
            case O_PRE_STONE_2:
            case O_PRE_STONE_3:
            case O_BOMB_TICK_1:
            case O_BOMB_TICK_2:
            case O_BOMB_TICK_3:
            case O_BOMB_TICK_4:
            case O_BOMB_TICK_5:
            case O_BOMB_TICK_6:
            case O_PRE_STEEL_1:
            case O_PRE_STEEL_2:
            case O_PRE_STEEL_3:
            case O_BOMB_EXPL_1:
            case O_BOMB_EXPL_2:
            case O_BOMB_EXPL_3:
            case O_NUT_EXPL_1:
            case O_NUT_EXPL_2:
            case O_NUT_EXPL_3:
            case O_GHOST_EXPL_1:
            case O_GHOST_EXPL_2:
            case O_GHOST_EXPL_3:
            case O_EXPLODE_1:
            case O_EXPLODE_2:
            case O_EXPLODE_3:
            case O_EXPLODE_4:
            case O_PRE_PL_1:
            case O_PRE_PL_2:
            case O_PRE_CLOCK_1:
            case O_PRE_CLOCK_2:
            case O_PRE_CLOCK_3:
            case O_NITRO_EXPL_1:
            case O_NITRO_EXPL_2:
            case O_NITRO_EXPL_3:
            case O_AMOEBA_2_EXPL_1:
            case O_AMOEBA_2_EXPL_2:
            case O_AMOEBA_2_EXPL_3:
                /* simply the next identifier */
                next(cave, x, y);
                break;
            case O_WATER_1:
            case O_WATER_2:
            case O_WATER_3:
            case O_WATER_4:
            case O_WATER_5:
            case O_WATER_6:
            case O_WATER_7:
            case O_WATER_8:
            case O_WATER_9:
            case O_WATER_10:
            case O_WATER_11:
            case O_WATER_12:
            case O_WATER_13:
            case O_WATER_14:
            case O_WATER_15:
                found_water=TRUE;    /* for sound */
                /* simply the next identifier */
                next(cave, x, y);
                break;

            case O_BLADDER_SPENDER:
                if (is_space_dir(cave, x, y, opposite[grav_compat])) {
                    store_dir(cave, x, y, opposite[grav_compat], O_BLADDER);
                    store(cave, x, y, O_PRE_STEEL_1);
                    play_sound_of_element(cave, O_BLADDER_SPENDER);
                }
                break;



            default:
                /* other inanimate elements that do nothing */
                break;
            }
        }

    /* POSTPROCESSING */

    /* another scan-like routine: */
    /* short explosions (for example, in bd1) started with explode_2. */
    /* internally we use explode_1; and change it to explode_2 if needed. */
    if (cave->short_explosions)
        for (y=0; y<cave->h; y++)
            for (x=0; x<cave->w; x++)
                if (is_first_stage_of_explosion(cave, x, y)) {
                    next(cave, x, y);    /* select next frame of explosion */
                    store(cave, x, y, get(cave, x, y)&~SCANNED);    /* forget scanned flag immediately */
                }

    /* finally: forget "scanned" flags for objects. */
    /* also, check for time penalties. */
    /* these is something like an effect table, but we do not really use one. */
    for (y=0; y<cave->h; y++)
        for (x=0; x<cave->w; x++) {
            if (get(cave, x, y)&SCANNED)
                store(cave, x, y, get(cave, x, y)&~SCANNED);
            if (get(cave, x, y)==O_TIME_PENALTY) {
                store(cave, x, y, O_GRAVESTONE);
                time_decrement_sec+=cave->time_penalty;    /* there is time penalty for destroying the voodoo */
            }
        }


    /* this loop finds the coordinates of the player. needed for scrolling and chasing stone.*/
    /* but we only do this, if a living player was found. if not yet, the setup routine coordinates are used */
    if (cave->player_state==GD_PL_LIVING) {
        if (cave->active_is_first_found) {
            /* to be 1stb compatible, we do everything backwards. */
            for (y=cave->h-1; y>=0; y--)
                for (x=cave->w-1; x>=0; x--)
                    if (is_player(cave, x, y)) {
                        /* here we remember the coordinates. */
                        cave->player_x=x;
                        cave->player_y=y;
                    }
        }
        else
        {
            /* as in the original: look for the last one */
            for (y=0; y<cave->h; y++)
                for (x=0; x<cave->w; x++)
                    if (is_player(cave, x, y)) {
                        /* here we remember the coordinates. */
                        cave->player_x=x;
                        cave->player_y=y;
                    }
        }
    }

    /* record coordinates of player for chasing stone */
    for (i=0; i<G_N_ELEMENTS(cave->px)-1; i++) {
        cave->px[i]=cave->px[i+1];
        cave->py[i]=cave->py[i+1];
    }
    cave->px[G_N_ELEMENTS(cave->px)-1]=cave->player_x;
    cave->py[G_N_ELEMENTS(cave->py)-1]=cave->player_y;

    /* SCHEDULING */

    /* update timing calculated by iterating and counting elements which
       were slow to process on c64 */
    switch (cave->scheduling) {
        case GD_SCHEDULING_MILLISECONDS:
            /* cave->speed already contains the milliseconds value, do not touch it */
            break;

        case GD_SCHEDULING_BD1:
            if (!cave->intermission)
                /* non-intermissions */
                cave->speed=(88+3.66*cave->c64_timing+(cave->ckdelay+cave->ckdelay_extra_for_animation)/1000);
            else
                /* intermissions were quicker, as only lines 1-12 were processed by the engine. */
                cave->speed=(60+3.66*cave->c64_timing+(cave->ckdelay+cave->ckdelay_extra_for_animation)/1000);
            break;

        case GD_SCHEDULING_BD1_ATARI:
            /* about 20ms/frame faster than c64 version */
            if (!cave->intermission)
                cave->speed=(74+3.2*cave->c64_timing+(cave->ckdelay)/1000);            /* non-intermissions */
            else
                cave->speed=(65+2.88*cave->c64_timing+(cave->ckdelay)/1000);        /* for intermissions */
            break;

        case GD_SCHEDULING_BD2:
            /* 60 is a guess. */
            cave->speed=MAX(60+(cave->ckdelay+cave->ckdelay_extra_for_animation)/1000, cave->c64_timing*20);
            break;

        case GD_SCHEDULING_PLCK:
            /* 65 is totally empty cave in construction kit, with delay=0) */
            cave->speed=MAX(65+cave->ckdelay/1000, cave->c64_timing*20);
            break;

        case GD_SCHEDULING_BD2_PLCK_ATARI:
            /* a really fast engine; timing works like c64 plck. */
            /* 40 ms was measured in the construction kit, with delay=0 */
            cave->speed=MAX(40+cave->ckdelay/1000, cave->c64_timing*20);
            break;

        case GD_SCHEDULING_CRDR:
            if (cave->hammered_walls_reappear)    /* this made the engine very slow. */
                cave->ckdelay+=60000;
            cave->speed=MAX(130+cave->ckdelay/1000, cave->c64_timing*20);
            break;

        case GD_SCHEDULING_MAX:
            /* to avoid compiler warning */
            g_assert_not_reached();
            break;
    }

    /* cave 3 sounds. precedence is controlled by the sound_play function. */
    /* but we have to check amoeba&magic together as they had a different gritty sound when mixed */
    if (found_water && cave->water_sound)
        gd_sound_play(cave, GD_S_WATER);
    magic_sound=cave->magic_wall_state==GD_MW_ACTIVE && cave->magic_wall_sound;
    amoeba_sound=cave->hatched && cave->amoeba_sound && ((amoeba_count>0 && cave->amoeba_state==GD_AM_AWAKE) || (amoeba_2_count>0 && cave->amoeba_2_state==GD_AM_AWAKE));
    if (amoeba_sound && magic_sound)
        gd_sound_play(cave, GD_S_AMOEBA_MAGIC);
    else
        if (amoeba_sound)
            gd_sound_play(cave, GD_S_AMOEBA);
        else
            if (magic_sound)
                gd_sound_play(cave, GD_S_MAGIC_WALL);
    if (cave->hatched)
        if ((amoeba_count>0 && cave->amoeba_state==GD_AM_AWAKE)
            || (amoeba_2_count>0 && cave->amoeba_2_state==GD_AM_AWAKE))
            play_sound_of_element(cave, O_AMOEBA);
    /* pneumatic hammer sound - overrides everything. */
    if (cave->pneumatic_hammer_active_delay>0 && cave->pneumatic_hammer_sound)
        gd_sound_play(cave, GD_S_PNEUMATIC_HAMMER);

    /* CAVE VARIABLES */

    /* PLAYER */
    if ((cave->player_state==GD_PL_LIVING && cave->player_seen_ago>15) || cave->kill_player)    /* check if player is alive. */
        cave->player_state=GD_PL_DIED;
    if (cave->voodoo_touched)    /* check if any voodoo exploded, and kill players the next scan if that happended. */
        cave->kill_player=TRUE;

    /* AMOEBA */
    if (cave->amoeba_state==GD_AM_AWAKE) {
        /* check flags after evaluating. */
        if (amoeba_count>=cave->amoeba_max_count)
            cave->amoeba_state=GD_AM_TOO_BIG;
        if (amoeba_found_enclosed)
            cave->amoeba_state=GD_AM_ENCLOSED;
    }
    /* amoeba can also be turned into diamond by magic wall */
    if (cave->magic_wall_stops_amoeba && cave->magic_wall_state==GD_MW_ACTIVE)
        cave->amoeba_state=GD_AM_ENCLOSED;
    /* AMOEBA 2 */
    if (cave->amoeba_2_state==GD_AM_AWAKE) {
        /* check flags after evaluating. */
        if (amoeba_2_count>=cave->amoeba_2_max_count)
            cave->amoeba_2_state=GD_AM_TOO_BIG;
        if (amoeba_2_found_enclosed)
            cave->amoeba_2_state=GD_AM_ENCLOSED;
    }
    /* amoeba 2 can also be turned into diamond by magic wall */
    if (cave->magic_wall_stops_amoeba && cave->magic_wall_state==GD_MW_ACTIVE)
        cave->amoeba_2_state=GD_AM_ENCLOSED;


    /* now check times. --------------------------- */
    /* decrement time if a voodoo was killed. */
    cave->time-=time_decrement_sec*cave->timing_factor;
    if (cave->time<0)
        cave->time=0;

    /* only decrement time when player is already born. */
    if (cave->hatched) {
        int secondsbefore, secondsafter;

        secondsbefore=cave->time/cave->timing_factor;
        cave->time-=cave->speed;
        if (cave->time<=0)
            cave->time=0;
        secondsafter=cave->time/cave->timing_factor;
        if (cave->time/cave->timing_factor<10)
            /* if less than 10 seconds, no walking sound, but play explosion sound */
            gd_sound_play(cave, GD_S_NONE);
        if (secondsbefore!=secondsafter)
            gd_cave_set_seconds_sound(cave);
    }
    /* a gravity switch was activated; seconds counting down */
    if (cave->gravity_will_change>0) {
        cave->gravity_will_change-=cave->speed;
        if (cave->gravity_will_change<0)
            cave->gravity_will_change=0;

        if (cave->gravity_will_change==0) {
            cave->gravity=cave->gravity_next_direction;
            if (cave->gravity_change_sound)
                gd_sound_play(cave, GD_S_GRAVITY_CHANGE);    /* takes precedence over amoeba and magic wall sound */
        }
    }

    /* creatures direction automatically change */
    if (cave->creatures_direction_will_change>0) {
        cave->creatures_direction_will_change-=cave->speed;
        if (cave->creatures_direction_will_change<0)
            cave->creatures_direction_will_change=0;

        if (cave->creatures_direction_will_change==0) {
            if (cave->creature_direction_auto_change_sound)
                gd_sound_play(cave, GD_S_SWITCH_CREATURES);
            cave->creatures_backwards=!cave->creatures_backwards;
            cave->creatures_direction_will_change=cave->creatures_direction_auto_change_time*cave->timing_factor;
        }
    }

    /* magic wall; if active&wait or not wait for hatching */
    if (cave->magic_wall_state==GD_MW_ACTIVE && (cave->hatched || !cave->magic_timer_wait_for_hatching)) {
        cave->magic_wall_time-=cave->speed;
        if (cave->magic_wall_time<0)
            cave->magic_wall_time=0;
        if (cave->magic_wall_time==0)
            cave->magic_wall_state=GD_MW_EXPIRED;
    }
    /* we may wait for hatching, when starting amoeba */
    if (cave->amoeba_timer_started_immediately || (cave->amoeba_state==GD_AM_AWAKE && (cave->hatched || !cave->amoeba_timer_wait_for_hatching))) {
        cave->amoeba_time-=cave->speed;
        if (cave->amoeba_time<0)
            cave->amoeba_time=0;
        if (cave->amoeba_time==0)
            cave->amoeba_growth_prob=cave->amoeba_fast_growth_prob;
    }
    /* we may wait for hatching, when starting amoeba */
    if (cave->amoeba_timer_started_immediately || (cave->amoeba_2_state==GD_AM_AWAKE && (cave->hatched || !cave->amoeba_timer_wait_for_hatching))) {
        cave->amoeba_2_time-=cave->speed;
        if (cave->amoeba_2_time<0)
            cave->amoeba_2_time=0;
        if (cave->amoeba_2_time==0)
            cave->amoeba_2_growth_prob=cave->amoeba_2_fast_growth_prob;
    }

    /* check for player hatching. */
    start_signal=FALSE;
    /* if not the c64 scheduling, but the correct frametime is used, hatching delay should always be decremented. */
    /* otherwise, the if (millisecs...) condition below will set this. */
    if (cave->scheduling==GD_SCHEDULING_MILLISECONDS) {        /* NON-C64 scheduling */
        if (cave->hatching_delay_frame>0) {
            cave->hatching_delay_frame--;    /* for milliseconds-based, non-c64 schedulings, hatching delay means frames. */
            if (cave->hatching_delay_frame==0)
                start_signal=TRUE;
        }
    }
    else {                                /* C64 scheduling */
        if (cave->hatching_delay_time>0) {
            cave->hatching_delay_time-=cave->speed;    /* for c64 schedulings, hatching delay means milliseconds. */
            if (cave->hatching_delay_time<=0) {
                cave->hatching_delay_time=0;
                start_signal=TRUE;
            }
        }
    }

    /* if decremented hatching, and it became zero: */
    if (start_signal) {        /* THIS IS THE CAVE START SIGNAL */
        cave->hatched=TRUE;    /* record that now the cave is in its normal state */

        gd_cave_count_diamonds(cave);    /* if diamonds needed is below zero, we count the available diamonds now. */

        /* setup direction auto change */
        if (cave->creatures_direction_auto_change_time) {
            cave->creatures_direction_will_change=cave->creatures_direction_auto_change_time*cave->timing_factor;

            if (cave->creatures_direction_auto_change_on_start)
                cave->creatures_backwards=!cave->creatures_backwards;
        }

        gd_sound_play(cave, GD_S_CRACK);
    }

    /* for biters */
    if (cave->biters_wait_frame==0)
        cave->biters_wait_frame=cave->biter_delay_frame;
    else
        cave->biters_wait_frame--;
    /* replicators delay */
    if (cave->replicators_wait_frame==0)
        cave->replicators_wait_frame=cave->replicator_delay_frame;
    else
        cave->replicators_wait_frame--;


    /* LAST THOUGTS */

    /* check if cave failed by timeout */
    if (cave->player_state==GD_PL_LIVING && cave->time==0) {
        gd_cave_clear_sounds(cave);
        cave->player_state=GD_PL_TIMEOUT;
        gd_sound_play(cave, GD_S_TIMEOUT);
    }

    /* set these for drawing. */
    cave->last_direction=player_move;
    /* here we remember last movements for animation. this is needed here, as animation
       is in sync with the game, not the keyboard directly. (for example, after exiting
       the cave, the player was "running" in the original, till bonus points were counted
       for remaining time and so on. */
    if (player_move==MV_LEFT || player_move==MV_UP_LEFT || player_move==MV_DOWN_LEFT)
        cave->last_horizontal_direction=MV_LEFT;
    if (player_move==MV_RIGHT || player_move==MV_UP_RIGHT || player_move==MV_DOWN_RIGHT)
        cave->last_horizontal_direction=MV_RIGHT;
        
        
        
    cave->frame++;  /* XXX */
}

