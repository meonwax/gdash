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
#include "cave.h"
#include "caveengine.h"




/* for gravity */
static const GdDirection left_eighth[]={ MV_STILL, MV_UP_LEFT, MV_UP, MV_UP_RIGHT, MV_RIGHT, MV_DOWN_RIGHT, MV_DOWN, MV_DOWN_LEFT };
static const GdDirection left_fourth[]={ MV_STILL, MV_LEFT, MV_UP_LEFT, MV_UP, MV_UP_RIGHT, MV_RIGHT, MV_DOWN_RIGHT, MV_DOWN, MV_DOWN_LEFT, MV_LEFT };
static const GdDirection right_eighth[]={ MV_STILL, MV_UP_RIGHT, MV_RIGHT, MV_DOWN_RIGHT, MV_DOWN, MV_DOWN_LEFT, MV_LEFT, MV_UP_LEFT, MV_UP };
static const GdDirection right_fourth[]={ MV_STILL, MV_RIGHT, MV_DOWN_RIGHT, MV_DOWN, MV_DOWN_LEFT, MV_LEFT, MV_UP_LEFT, MV_UP, MV_UP_RIGHT };


gboolean
gd_cave_set_seconds_sound(Cave *cave)
{
	switch(cave->time/cave->timing_factor) {
		case 9: cave->sound2=GD_S_TIMEOUT_1; break;
		case 8: cave->sound2=GD_S_TIMEOUT_2; break;
		case 7: cave->sound2=GD_S_TIMEOUT_3; break;
		case 6: cave->sound2=GD_S_TIMEOUT_4; break;
		case 5: cave->sound2=GD_S_TIMEOUT_5; break;
		case 4: cave->sound2=GD_S_TIMEOUT_6; break;
		case 3: cave->sound2=GD_S_TIMEOUT_7; break;
		case 2: cave->sound2=GD_S_TIMEOUT_8; break;
		case 1: cave->sound2=GD_S_TIMEOUT_9; break;
		default:
			/* no sound */
			return FALSE;	/* did not set. */
	}
	return TRUE;	/* did set. */
}





static inline GdElement*
getp (const Cave *cave, const int x, const int y)
{
	return cave->getp(cave, x, y);
}

/*
   perfect (non-lineshifting) GET function. returns a pointer to a selected cave element by its coordinates.
 */
static inline GdElement*
getp_perfect (const Cave *cave, const int x, const int y)
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
getp_shift (const Cave *cave, int x, int y)
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
get(const Cave *cave, const int x, const int y)
{
	return *getp(cave, x, y);
}

/* returns an element which is somewhere near x,y */
static inline GdElement
get_dir(const Cave *cave, const int x, const int y, const GdDirection dir)
{
	return get(cave, x+gd_dx[dir], y+gd_dy[dir]);
}





/* returns true if the element is explodable and explodes to space, for example the player */
static inline gboolean
explodes_to_space(const Cave *cave, const int x, const int y)
{
	return gd_elements[get(cave, x,y)&O_MASK].properties&P_EXPLODES_TO_SPACE;
}

/* returns true if the element is explodable and explodes to diamond, for example a butterfly */
static inline gboolean
explodes_to_diamonds (const Cave *cave, const int x, const int y)
{
	return gd_elements[get(cave, x,y)&O_MASK].properties&P_EXPLODES_TO_DIAMONDS;
}

/* returns true if the element is explodable and explodes to stones, for example the stonefly */
static inline gboolean
explodes_to_stones(const Cave *cave, const int x, const int y)
{
	return (gd_elements[get(cave, x,y)&O_MASK].properties&P_EXPLODES_TO_STONES)!=0;
}

static inline gboolean
explodes_by_hit_dir(const Cave *cave, const int x, const int y, GdDirection dir)
{
	return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_EXPLODES)!=0;
}

/* returns true if the element is not explodable, for example the steel wall */
static inline gboolean
non_explodable (const Cave *cave, const int x, const int y)
{
	return (gd_elements[get(cave, x,y)&O_MASK].properties&P_NON_EXPLODABLE)!=0;
}

/* returns true if neighbouring element is space */
static inline gboolean
is_space_dir(const Cave *cave, const int x, const int y, const GdDirection dir)
{
	return (get_dir(cave, x, y, dir)&O_MASK)==O_SPACE;
}

/* returns true if neighbouring element is space */
static inline gboolean
is_element_dir(const Cave *cave, const int x, const int y, const GdDirection dir, GdElement e)
{
	GdElement examined=get_dir(cave, x, y, dir);
	if (e==O_DIRT2)
		e=O_DIRT;
	if (examined==O_DIRT2)
		examined=O_DIRT;
	return e==examined;
}

/* returns true if the element can be eaten by the amoeba, eg. space and dirt. */
static inline gboolean
amoeba_eats_dir(const Cave *cave, const int x, const int y, const GdDirection dir)
{
	return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_AMOEBA_CONSUMES)!=0;
}

/* returns true if the element is rounded, so stones and diamonds roll down on it. for example a stone or brick wall */
static inline gboolean
rounded_dir (const Cave *cave, const int x, const int y, const GdDirection dir)
{
	return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_ROUNDED)!=0;
}

static inline gboolean
blows_up_flies_dir(const Cave *cave, const int x, const int y, const GdDirection dir)
{
	return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_BLOWS_UP_FLIES)!=0;
}

/* returns true if the element is a counter-clockwise creature */
static inline gboolean
rotates_ccw (const Cave *cave, const int x, const int y)
{
	return (gd_elements[get(cave, x,y)&O_MASK].properties&P_CCW)!=0;
}

static inline gboolean
can_be_hammered_dir(const Cave *cave, const int x, const int y, const GdDirection dir)
{
	return (gd_elements[get_dir(cave, x, y, dir)&O_MASK].properties&P_CAN_BE_HAMMERED)!=0;
}






/* store an element at the given position */
static inline void
store(Cave *cave, const int x, const int y, const GdElement element)
{
	(*getp(cave, x, y))=element;
}

/* store an element with SCANNED flag turned on */
static inline void
store_sc (Cave *cave, const int x, const int y, const GdElement element)
{
	(*getp(cave, x, y))=element|SCANNED;
}

/* store an element to a neighbouring cell */
static inline void
store_dir(Cave *cave, const int x, const int y, const GdDirection dir, const GdElement element)
{
	(*getp(cave, x+gd_dx[dir], y+gd_dy[dir]))=element|SCANNED;
}

/* store an element to a neighbouring cell */
static inline void
store_dir_no_scanned(Cave *cave, const int x, const int y, const GdDirection dir, const GdElement element)
{
	(*getp(cave, x+gd_dx[dir], y+gd_dy[dir]))=element;
}

/* move element to direction; then place space at x, y */
static inline void
move(Cave *cave, const int x, const int y, const GdDirection dir, const GdElement e)
{
	store_dir(cave, x, y, dir, e);
	store(cave, x, y, O_SPACE);
}

/* increment a cave element; can be used for elements which are one after the other, for example bladder1, bladder2, bladder3... */
static inline void
next (Cave *cave, const int x, const int y)
{
	(*getp(cave, x, y))++;
}






/*
 * a creature explodes to a 3x3 something. */
static void
creature_explode (Cave *cave, const int x, const int y, const GdElement explode_to)
{
	int xx, yy;

	/* the processing of an explosion took pretty much time: processing 3x3=9 elements */	
	cave->ckdelay+=1200;

	for (yy=y-1; yy<=y+1; yy++)
		for (xx=x-1; xx<=x+1; xx++) {
			if (non_explodable (cave, xx, yy))
				continue;
			if (get(cave, xx, yy)==O_VOODOO && !cave->voodoo_can_be_destroyed)
				store_sc(cave, xx, yy, O_TIME_PENALTY);
			else
				store_sc(cave, xx, yy, explode_to);
		}
}

static void
voodoo_explode(Cave *cave, const int x, const int y)
{
	int xx, yy;

	/* the processing of an explosion took pretty much time: processing 3x3=9 elements */	
	cave->ckdelay+=1000;

	/* voodoo explodes to 3x3 steel */
	for (yy=y-1; yy<=y+1; yy++)
		for (xx=x-1; xx<=x+1; xx++)
			store_sc(cave, xx, yy, O_PRE_STEEL_1);
	/* middle is a time penalty (which will be turned into a gravestone) */
	store_sc(cave, x, y, O_TIME_PENALTY);
}

static void
explode_try_skip_voodoo(Cave *cave, const int x, const int y, const GdElement expl)
{
	if (non_explodable (cave, x, y))
		return;
	/* bomb does not explode voodoo */
	if (!cave->voodoo_can_be_destroyed && get(cave, x, y)==O_VOODOO)
		return;
	store_sc (cave, x, y, expl);
}

/* X shaped ghost explosion; does not touch voodoo! */
static void
ghost_explode(Cave *cave, const int x, const int y)
{
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
bomb_explode(Cave *cave, const int x, const int y)
{
	/* the processing of an explosion took pretty much time: processing 5 elements */	
	cave->ckdelay+=650;

	explode_try_skip_voodoo(cave, x, y, O_BOMB_EXPL_1);
	explode_try_skip_voodoo(cave, x-1, y, O_BOMB_EXPL_1);
	explode_try_skip_voodoo(cave, x+1, y, O_BOMB_EXPL_1);
	explode_try_skip_voodoo(cave, x, y+1, O_BOMB_EXPL_1);
	explode_try_skip_voodoo(cave, x, y-1, O_BOMB_EXPL_1);
}

/**
	explode an element with the appropriate type of exlposion.
 */
static void
explode(Cave *cave, const int x, const int y)
{
	cave->sound2=GD_S_EXPLOSION;
	if (get(cave, x, y)==O_GHOST)
		ghost_explode(cave, x, y);
	else if (get(cave, x, y)==O_BOMB_TICK_7)
		bomb_explode(cave, x, y);
	else if (get(cave, x, y)==O_VOODOO)
		voodoo_explode(cave, x, y);
	else if (explodes_to_space (cave, x, y))
		creature_explode(cave, x, y, O_EXPLODE_1);
	else if (explodes_to_diamonds (cave, x, y))
		creature_explode(cave, x, y, O_PRE_DIA_1);
	else if (explodes_to_stones(cave, x, y))
		creature_explode(cave, x, y, O_PRE_STONE_1);
	else if (get(cave, x, y)==O_FALLING_WALL_F)
		creature_explode(cave, x, y, O_EXPLODE_1);
	else
		/* assert, as caller must have called this for some reason */
		g_assert_not_reached ();
}

static void
explode_dir(Cave *cave, const int x, const int y, GdDirection dir)
{
	explode(cave, x+gd_dx[dir], y+gd_dy[dir]);
}

/**
	player eats specified object.
	returns O_SPACE if he eats it (diamond, dirt, space, outbox)
	returns other element if something other appears there and he can't move.
	cave pointer is needed to know the diamond values.
 */
static GdElement
player_get_element (Cave* cave, const GdElement object)
{
	int i;
	GdSound prevsound=cave->sound2;

	/* usually play this sound */	
	if (cave->sound2!=GD_S_EXPLOSION)
		cave->sound2=GD_S_WALK_EARTH;
	
	switch (object) {
	/* KEYS AND DOORS */
	case O_KEY_1:
		cave->key1++;
		return O_SPACE;
	case O_KEY_2:
		cave->key2++;
		return O_SPACE;
	case O_KEY_3:
		cave->key3++;
		return O_SPACE;
	case O_DOOR_1:
		if (cave->key1==0)
			return object;
		cave->key1--;
		return O_SPACE;
	case O_DOOR_2:
		if (cave->key2==0)
			return object;
		cave->key2--;
		return O_SPACE;
	case O_DOOR_3:
		if (cave->key3==0)
			return object;
		cave->key3--;
		return O_SPACE;

	/* SWITCHES */
	case O_CREATURE_SWITCH:
		/* creatures change direction. */
		cave->creatures_backwards=!cave->creatures_backwards;
		return object;
	case O_GROWING_WALL_SWITCH:
		/* expanding wall change direction. */
		cave->expanding_wall_changed=!cave->expanding_wall_changed;
		return object;
	case O_BITER_SWITCH:
		cave->biter_delay_frame++;
		if (cave->biter_delay_frame==4)
			cave->biter_delay_frame=0;
		return object;

	/* USUAL STUFF */
	case O_DIRT:
	case O_DIRT2:
	case O_STEEL_EATABLE:
	case O_BRICK_EATABLE:
		return O_SPACE;
	
	case O_DIAMOND_KEY:
		cave->diamond_key_collected=TRUE;
		return O_SPACE;
	case O_SWEET:
		cave->sweet_eaten=TRUE;
		cave->pushing_stone_prob=cave->pushing_stone_prob_sweet;
		return O_SPACE;
	case O_PNEUMATIC_HAMMER:
		cave->got_pneumatic_hammer=TRUE;
		return O_SPACE;

	case O_CLOCK:
		/* bonus time */
		cave->time+=cave->bonus_time*cave->timing_factor;
		/* no space, rather a dirt remains there... */
		return O_DIRT;
	case O_DIAMOND:
		if (cave->sound2!=GD_S_EXPLOSION)
			cave->sound2=GD_S_DIAMOND_COLLECT;
		cave->score+=cave->diamond_value;
		cave->diamonds_collected++;
		if (cave->diamonds_needed==cave->diamonds_collected) {
			cave->diamond_value=cave->extra_diamond_value;	/* extra is worth more points. */
			cave->gate_open_flash=1;
			cave->sound3=GD_S_CRACK;
		}
		return O_SPACE;
	case O_SKELETON:
		cave->skeletons_collected++;
		for (i=0; i<cave->skeletons_worth_diamonds; i++)
			player_get_element(cave, O_DIAMOND);	/* as if player got a diamond */
		return O_SPACE;
	case O_OUTBOX:
	case O_INVIS_OUTBOX:
		cave->player_state=PL_EXITED;	/* player now exits the cave! */
		return O_SPACE;
	case O_SPACE:
		if (cave->sound2!=GD_S_EXPLOSION)
			cave->sound2=GD_S_WALK_EMPTY;
		return O_SPACE;

	default:
		/* the object will remain there. */
		cave->sound2=prevsound;		/* sound not changed */
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
do_teleporter(Cave *cave, int px, int py, GdDirection player_move)
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
			store_dir(cave, tx, ty, player_move, get(cave, px, py));	/* new player appears near teleporter found */
			store(cave, px, py, O_SPACE);	/* current player disappears */
			return TRUE;	/* return true as teleporter worked */
		}
	} while (tx!=px || ty!=py);	/* loop until we get back to original coordinates */
	return FALSE;	/* return false as we did not find any usable teleporter */
}

/* play diamond or stone sound of given element. */
static void
diamond_stone_sound(Cave *cave, GdElement element)
{
	switch(element) {
		case O_STONE:
		case O_STONE_F:
		case O_WAITING_STONE:
		case O_CHASING_STONE:
		case O_FALLING_WALL:
		case O_FALLING_WALL_F:
			cave->sound1=GD_S_STONE;
			break;
		case O_DIAMOND:
		case O_DIAMOND_F:
			cave->sound1=GD_S_DIAMOND_RANDOM;
			break;
		default:
			/* do nothing. may be called for anything by magic wall... */
			break;
	}
}

void
gd_cave_iterate(Cave *cave, const gboolean up, const gboolean down, const gboolean left, const gboolean right, const gboolean player_fire, const gboolean suicide)
{
	int x, y;
	gboolean amoeba_found_enclosed=TRUE;	/* amoeba found to be enclosed. if not, this is cleared */
	gboolean voodoo_touched=FALSE;	/* voodoo was touched this frame */
	int amoeba_count=0;		/* counting the number of amoebas. after scan, check if too much */
	gboolean inbox_toggle;
	int i;
	GList *iter;
	gboolean start_signal;
	GdDirection player_move;
	/* directions for o_something_1, 2, 3 and 4 (creatures) */
	static const GdDirection creature_dir[]={ MV_LEFT, MV_UP, MV_RIGHT, MV_DOWN };
	static const GdDirection creature_chdir[]={ MV_RIGHT, MV_DOWN, MV_LEFT, MV_UP };
	
	/* set these to no sound; and they will be set during iteration. */
	cave->sound1=GD_S_NONE;
	cave->sound2=GD_S_NONE;
	cave->sound3=GD_S_NONE;

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

	if (cave->player_seen_ago<100)
		cave->player_seen_ago++;	/* increment this. if the scan routine comes across player, clears it. */

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
	if (suicide && cave->player_state==PL_LIVING)
		store(cave, cave->player_x, cave->player_y, O_EXPLODE_1);

	iter=cave->hammered_walls;
	while (iter!=NULL) {
		GList *next=iter->next;	/* we have to remember this, as we might delete the current link */
		HammeredWall *w=(HammeredWall *)iter->data;

		w->reappear--;
		if (w->reappear<0) {
			store(cave, w->x, w->y, O_BRICK);
			cave->hammered_walls=g_list_delete_link(cave->hammered_walls, iter);
		}
		iter=next;
	}

	cave->ckdelay=0;
	/* the cave scan routine */
	for (y=0; y<cave->h; y++)
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
			case O_STONE:	/* standing stone */
			case O_DIAMOND:	/* standing diamond */
				if (cave->gravity_disabled)
					break;
				if (is_space_dir(cave, x, y, cave->gravity)) {	/* beginning to fall */
					diamond_stone_sound(cave, get(cave, x, y));
					move(cave, x, y, cave->gravity, get(cave, x, y)==O_STONE?cave->falling_stone_to:cave->falling_diamond_to);
				}
				else if (rounded_dir(cave, x, y, cave->gravity)) {	/* rolling down, if sitting on a rounded object  */
					if (is_space_dir(cave, x, y, left_fourth[cave->gravity]) && is_space_dir(cave, x, y, left_eighth[cave->gravity])) {
						/* rolling left? */
						diamond_stone_sound(cave, get(cave, x, y));
						move(cave, x, y, left_fourth[cave->gravity], get(cave, x, y)==O_STONE?cave->falling_stone_to:cave->falling_diamond_to);
					}
					else if (is_space_dir(cave, x, y, right_fourth[cave->gravity]) && is_space_dir(cave, x, y, right_eighth[cave->gravity])) {
						/* rolling right? */
						diamond_stone_sound(cave, get(cave, x, y));
						move(cave, x, y, right_fourth[cave->gravity], get(cave, x, y)==O_STONE?cave->falling_stone_to:cave->falling_diamond_to);
					}
				}
				break;
			case O_STONE_F:	/* falling stone */
			case O_DIAMOND_F:	/* falling diamond */
				if (cave->gravity_disabled)
					break;
				if (is_space_dir(cave, x, y, cave->gravity))	/* falling further */
					move(cave, x, y, cave->gravity, get(cave, x, y));
				else if (get(cave, x, y)==O_DIAMOND_F && get_dir(cave, x, y, cave->gravity)==O_VOODOO && cave->voodoo_collects_diamonds) {
					/* this is a 1stB-style voodoo. explodes by stone, collects diamonds */
					player_get_element (cave, O_DIAMOND);	/* as if player got diamond */
					store(cave, x, y, O_SPACE);	/* diamond disappears */
				}
				else if (get(cave, x, y)==O_STONE_F && get_dir(cave, x, y, cave->gravity)==O_VOODOO && cave->voodoo_dies_by_stone) {
					/* this is a 1stB-style vodo. explodes by stone, collects diamonds */
					explode_dir (cave, x, y, cave->gravity);
				}
				else if (get_dir(cave, x, y, cave->gravity)==O_MAGIC_WALL) {
					diamond_stone_sound(cave, O_DIAMOND);	/* always play diamond sound */
					if (cave->magic_wall_state==MW_DORMANT)
						cave->magic_wall_state=MW_ACTIVE;
					if (cave->magic_wall_state==MW_ACTIVE && is_space_dir(cave, x, y, MV_TWICE+cave->gravity)) {
						/* if magic wall active and place underneath, */
						/* it turns boulder into diamond and vice versa. or anything the effect says to do. */
						store_dir(cave, x, y, MV_TWICE+cave->gravity, get(cave, x, y)==O_STONE_F?cave->magic_stone_to:cave->magic_diamond_to);
					}
					store(cave, x, y, O_SPACE);	/* active or non-active or anything, element falling in will always disappear */
				}
				else if (explodes_by_hit_dir(cave, x, y, cave->gravity))
					explode_dir(cave, x, y, cave->gravity);
				else if (rounded_dir(cave, x, y, cave->gravity)) {	/* rounded element, falling to left or right */
					if (is_space_dir(cave, x, y, left_eighth[cave->gravity]) && is_space_dir(cave, x, y, left_fourth[cave->gravity])) {
						diamond_stone_sound(cave, get(cave, x, y));
						move(cave, x, y, left_fourth[cave->gravity], get(cave, x, y));	/* try to roll left first */
					}
					else if (is_space_dir(cave, x, y, right_eighth[cave->gravity]) && is_space_dir(cave, x, y, right_fourth[cave->gravity])) {
						diamond_stone_sound(cave, get(cave, x, y));
						move(cave, x, y, right_fourth[cave->gravity], get(cave, x, y));	/* if not, try to roll right */
					}
					else {
						/* cannot roll in any direction, so it stops */
						diamond_stone_sound(cave, get(cave, x, y));
						store(cave, x, y, get(cave, x, y)==O_STONE_F?cave->bouncing_stone_to:cave->bouncing_diamond_to);
					}
				}
				else {
					/* any other element, stops */
					diamond_stone_sound(cave, get(cave, x, y));
					store(cave, x, y, get(cave, x, y)==O_STONE_F?cave->bouncing_stone_to:cave->bouncing_diamond_to);
				}
				break;

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
					gboolean ccw=rotates_ccw(cave, x, y);	/* check if default is counterclockwise */
					GdElement base;	/* base element number (which is like O_***_1) */
					int dir, dirn, dirp;	/* direction */

					base=O_COW_1;

					dir=get(cave, x, y)-base;	/* facing where */
					creature_move=cave->creatures_backwards? creature_chdir:creature_dir;

					/* now change direction if backwards */
					if (cave->creatures_backwards)
						ccw=!ccw;

					if (ccw) {
						dirn=(dir+3)&3;	/* fast turn */
						dirp=(dir+1)&3;	/* slow turn */
					} else {
						dirn=(dir+1)&3; /* fast turn */
						dirp=(dir+3)&3;	/* slow turn */
					}

					if (is_space_dir(cave, x, y, creature_move[dirn]))
						move(cave, x, y, creature_move[dirn], base+dirn);	/* turn and move to preferred dir */
					else if (is_space_dir(cave, x, y, creature_move[dir]))
						move(cave, x, y, creature_move[dir], base+dir);	/* go on */
					else
						store(cave, x, y, base+dirp);	/* turn in place if nothing else possible */
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

			case O_GUARD_1:
			case O_GUARD_2:
			case O_GUARD_3:
			case O_GUARD_4:
			case O_ALT_GUARD_1:
			case O_ALT_GUARD_2:
			case O_ALT_GUARD_3:
			case O_ALT_GUARD_4:
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
					voodoo_touched=TRUE;
				/* check if touches something bad and should explode (includes voodoo by the flags) */
				if (blows_up_flies_dir(cave, x, y, MV_DOWN) || blows_up_flies_dir(cave, x, y, MV_UP)
					|| blows_up_flies_dir(cave, x, y, MV_LEFT) || blows_up_flies_dir(cave, x, y, MV_RIGHT))
					explode (cave, x, y);
				/* otherwise move */
				else {
					const GdDirection *creature_move;
					gboolean ccw=rotates_ccw(cave, x, y);	/* check if default is counterclockwise */
					GdElement base;	/* base element number (which is like O_***_1) */
					int dir, dirn, dirp;	/* direction */

					if (get(cave, x, y)>=O_GUARD_1 && get(cave, x, y)<=O_GUARD_4)
						base=O_GUARD_1;
					else if (get(cave, x, y)>=O_BUTTER_1 && get(cave, x, y)<=O_BUTTER_4)
						base=O_BUTTER_1;
					else if (get(cave, x, y)>=O_STONEFLY_1 && get(cave, x, y)<=O_STONEFLY_4)
						base=O_STONEFLY_1;
					else if (get(cave, x, y)>=O_ALT_GUARD_1 && get(cave, x, y)<=O_ALT_GUARD_4)
						base=O_ALT_GUARD_1;
					else if (get(cave, x, y)>=O_ALT_BUTTER_1 && get(cave, x, y)<=O_ALT_BUTTER_4)
						base=O_ALT_BUTTER_1;
					else if (get(cave, x, y)>=O_COW_1 && get(cave, x, y)<=O_COW_4)
						base=O_COW_1;
					else
						g_assert_not_reached();

					dir=get(cave, x, y)-base;	/* facing where */
					creature_move=cave->creatures_backwards? creature_chdir:creature_dir;

					/* now change direction if backwards */
					if (cave->creatures_backwards)
						ccw=!ccw;

					if (ccw) {
						dirn=(dir+3)&3;	/* fast turn */
						dirp=(dir+1)&3;	/* slow turn */
					} else {
						dirn=(dir+1)&3; /* fast turn */
						dirp=(dir+3)&3;	/* slow turn */
					}

					if (is_space_dir(cave, x, y, creature_move[dirn]))
						move(cave, x, y, creature_move[dirn], base+dirn);	/* turn and move to preferred dir */
					else if (is_space_dir(cave, x, y, creature_move[dir]))
						move(cave, x, y, creature_move[dir], base+dir);	/* go on */
					else
						store(cave, x, y, base+dirp);	/* turn in place if nothing else possible */
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
				if (cave->player_state!=PL_EXITED)
					cave->player_state=PL_LIVING;
				if (cave->pneumatic_hammer_active_delay==0)	/* if hammering time is up, becomes a normal player again. */
					store(cave, x, y, O_PLAYER);
				break;

			/* the active pneumatic hammer itself */
			case O_PNEUMATIC_ACTIVE_RIGHT:
			case O_PNEUMATIC_ACTIVE_LEFT:
				if (cave->pneumatic_hammer_active_delay==0) {
					gboolean broken=FALSE;
					store(cave, x, y, O_SPACE);	/* pneumatic hammer element disappears */
					switch(get_dir(cave, x, y, MV_DOWN)) {	/* what is under the pneumatic hammer? */
						case O_WALLED_KEY_1:
							store_dir(cave, x, y, MV_DOWN, O_KEY_1);
							broken=TRUE;
							break;
						case O_WALLED_KEY_2:
							store_dir(cave, x, y, MV_DOWN, O_KEY_2);
							broken=TRUE;
							break;
						case O_WALLED_KEY_3:
							store_dir(cave, x, y, MV_DOWN, O_KEY_3);
							broken=TRUE;
							break;
						case O_WALLED_DIAMOND:
							store_dir(cave, x, y, MV_DOWN, O_DIAMOND);
							broken=TRUE;
							break;
						case O_BRICK:
							store_dir(cave, x, y, MV_DOWN, O_SPACE);
							broken=TRUE;
							break;
						default:
							/* this could happen, for example if the element is exploded by a firefly during hammering... but we simply ignore this. */
							break;
					}
					if (broken) {
						if (cave->hammered_walls_reappear) {
							/* if it will reappear, append to list */
							HammeredWall w;

							w.x=x;
							w.y=y+1;
							w.reappear=cave->hammered_wall_reappear_frame;
							cave->hammered_walls=g_list_prepend(cave->hammered_walls, g_memdup(&w, sizeof(w)));
						}
					}
				}
				break;

			case O_PLAYER:
				if (cave->kill_player) {
					explode (cave, x, y);
					break;
				}
				cave->player_seen_ago=0;
				/* bd4 intermission caves have many players. so if one of them has exited,
				 * do not change the flag anymore. so this if () is needed */
				if (cave->player_state!=PL_EXITED)
					cave->player_state=PL_LIVING;
				/* check for pneumatic hammer things */
				/* 1) press fire, 2) have pneumatic hammer 4) space on left or right for hammer 5) stand on something */
				if (player_fire && cave->got_pneumatic_hammer && is_space_dir(cave, x, y, player_move)
					&& !is_space_dir(cave, x, y, MV_DOWN)) {
					if (player_move==MV_LEFT && can_be_hammered_dir(cave, x, y, MV_DOWN_LEFT)) {
						cave->pneumatic_hammer_active_delay=cave->pneumatic_hammer_frame;
						store_dir(cave, x, y, MV_LEFT, O_PNEUMATIC_ACTIVE_LEFT);
						store(cave, x, y, O_PLAYER_PNEUMATIC_LEFT);
						break;	/* finished. */
					}
					if (player_move==MV_RIGHT && can_be_hammered_dir(cave, x, y, MV_DOWN_RIGHT)) {
						cave->pneumatic_hammer_active_delay=cave->pneumatic_hammer_frame;
						store_dir(cave, x, y, MV_RIGHT, O_PNEUMATIC_ACTIVE_RIGHT);
						store(cave, x, y, O_PLAYER_PNEUMATIC_RIGHT);
						break;	/* finished. */
					}
				}

				if (player_move!=MV_STILL) {
					/* only do every check if he is not moving */
					GdElement what=get_dir(cave, x, y, player_move);
					GdElement remains=what;

					/* if we are 'eating' a teleporter, and the function returns true (teleporting worked), break here */
					if (what==O_TELEPORTER && do_teleporter(cave, x, y, player_move))
						break;

					switch (what) {
					case O_WAITING_STONE:
					case O_STONE:
					case O_CHASING_STONE:
						/* pushing some kind of stone */
						if (player_move==left_fourth[cave->gravity] || player_move==right_fourth[cave->gravity]) {
							/* only push if player dir is orthogonal to gravity, ie. gravity down, pushing left&right possible */
							if ((what==O_WAITING_STONE)	/* waiting stones are light, can always push */
								||(what==O_CHASING_STONE && cave->sweet_eaten)	/* chasing can be pushed if player is turbo */
								||(what==O_STONE && g_rand_int_range(cave->random, 0, 1000000)<cave->pushing_stone_prob*1000000)) {	/* stones are heavy, maybe push */
								/* if decided that he will be able to push, */
									if (is_space_dir(cave, x, y, MV_TWICE + player_move)) {
										store_dir(cave, x, y, MV_TWICE + player_move, what);
										remains=O_SPACE;
										cave->sound1=GD_S_STONE;
									}
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
					case O_BLADDER_9:
						/* pushing a bladder. keep in mind that after pushing, we always get an O_BLADDER,
						 * not an O_BLADDER_x. */
						/* there is no "delayed" state of a bladder, so we use store_dir_no_scanned! */
						switch (player_move) {
						case MV_DOWN:
							if (is_space_dir(cave, x, y, MV_DOWN_2))	/* pushing bladder down */
								store_dir_no_scanned(cave, x, y, MV_DOWN_2, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_DOWN_LEFT))	/* if no space to push down, maybe left (down-left to player) */
								store_dir_no_scanned(cave, x, y, MV_DOWN_LEFT, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_DOWN_RIGHT))	/* if not, maybe right (down-right to player) */
								store_dir_no_scanned(cave, x, y, MV_DOWN_RIGHT, O_BLADDER), remains=O_SPACE;
							break;
						case MV_LEFT:
							if (is_space_dir(cave, x, y, MV_LEFT_2))	/* pushing it left */
								store_dir_no_scanned(cave, x, y, MV_LEFT_2, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_DOWN_LEFT))	/* maybe down, and player will move left */
								store_dir_no_scanned(cave, x, y, MV_DOWN_LEFT, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_UP_LEFT))	/* maybe up, and player will move left */
								store_dir_no_scanned(cave, x, y, MV_UP_LEFT, O_BLADDER), remains=O_SPACE;
							break;
						case MV_RIGHT:
							if (is_space_dir(cave, x, y, MV_RIGHT_2))	/* pushing it right */
								store_dir_no_scanned(cave, x, y, MV_RIGHT_2, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_DOWN_RIGHT))	/* maybe down, and player will move right */
								store_dir_no_scanned(cave, x, y, MV_DOWN_RIGHT, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_UP_RIGHT))	/* maybe up, and player will move right */
								store_dir_no_scanned(cave, x, y, MV_UP_RIGHT, O_BLADDER), remains=O_SPACE;
							break;
						default:
							/* pushing bladder in other directions not possible */
							break;
						}
						break;
					case O_BOMB:
						/* if its a bomb, remember he now has one. */
						/* we do not change the "remains" and "what" variables, so that part of the code will be ineffective */
						store_dir(cave, x, y, player_move, O_SPACE);
						if (player_fire)
							store(cave, x, y, O_PLAYER_BOMB);
						else
							move(cave, x, y, player_move, O_PLAYER_BOMB);
						break;
					case O_BOX:
						/* a box is only pushed with the fire pressed */
						/* we do not change the "remains" and "what" variables, so that part of the code will be ineffective */
						if (player_fire) {
							switch (player_move) {
							case MV_LEFT:
								if (is_space_dir(cave, x, y, MV_LEFT_2))
									store_dir(cave, x, y, MV_LEFT_2, O_BOX), remains=O_SPACE;
								break;
							case MV_RIGHT:
								if (is_space_dir(cave, x, y, MV_RIGHT_2))
									store_dir(cave, x, y, MV_RIGHT_2, O_BOX), remains=O_SPACE;
								break;
							case MV_UP:
								if (is_space_dir(cave, x, y, MV_UP_2))
									store_dir(cave, x, y, MV_UP_2, O_BOX), remains=O_SPACE;
								break;
							case MV_DOWN:
								if (is_space_dir(cave, x, y, MV_DOWN_2))
									store_dir(cave, x, y, MV_DOWN_2, O_BOX), remains=O_SPACE;
								break;
							default:
								/* push in no other directions possible */
								break;
							}
						}
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

						/* if snapping anything and we have snapping explosions set */
						if (remains==O_SPACE && player_fire && cave->snap_explosions)
							remains=O_EXPLODE_1;
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
				/* much simpler; cannot push stones */
				if (cave->kill_player) {
					explode (cave, x, y);
					break;
				}
				cave->player_seen_ago=0;
				/* bd4 intermission caves have many players. so if one of them has exited,
				 * do not change the flag anymore. so this if () is needed */
				if (cave->player_state!=PL_EXITED)
					cave->player_state=PL_LIVING;
				if (player_move==MV_STILL)
					break;

				if (player_fire) {
					/* placing a bomb into empty space or dirt */
					if (is_space_dir(cave, x, y, player_move) || is_element_dir(cave, x, y, player_move, O_DIRT)) {
						store_dir(cave, x, y, player_move, O_BOMB_TICK_1);
						/* placed bomb, he is normal player again */
						store(cave, x, y, O_PLAYER);
					}
					break;
				}

				/* pushing and collecting */
				if (player_move!=MV_STILL) {
					/* only do every check if he is not moving */
					GdElement what=get_dir(cave, x, y, player_move);
					GdElement remains=what;

					/* if we are 'eating' a teleporter, and the function returns true (teleporting worked), break here */
					if (what==O_TELEPORTER && do_teleporter(cave, x, y, player_move))
						break;

					switch (what) {
					case O_WAITING_STONE:
					case O_STONE:
					case O_CHASING_STONE:
						/* pushing some kind of stone */
						if (player_move==left_fourth[cave->gravity] || player_move==right_fourth[cave->gravity]) {
							/* only push if player dir is orthogonal to gravity, ie. gravity down, pushing left&right possible */
							if ((what==O_WAITING_STONE)	/* waiting stones are light, can always push */
								||(what==O_CHASING_STONE && cave->sweet_eaten)	/* chasing can be pushed if player is turbo */
								||(what==O_STONE && g_rand_int_range(cave->random, 0, 1000000)<cave->pushing_stone_prob*1000000)) {	/* stones are heavy, maybe push */
								/* if decided that he will be able to push, */
									if (is_space_dir(cave, x, y, MV_TWICE + player_move)) {
										store_dir(cave, x, y, MV_TWICE + player_move, what);
										remains=O_SPACE;
										cave->sound1=GD_S_STONE;
									}
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
					case O_BLADDER_9:
						/* pushing a bladder. keep in mind that after pushing, we always get an O_BLADDER,
						 * not an O_BLADDER_x. */
						/* there is no "delayed" state of a bladder, so we use store_dir_no_scanned! */
						switch (player_move) {
						case MV_DOWN:
							if (is_space_dir(cave, x, y, MV_DOWN_2))	/* pushing bladder down */
								store_dir_no_scanned(cave, x, y, MV_DOWN_2, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_DOWN_LEFT))	/* if no space to push down, maybe left (down-left to player) */
								store_dir_no_scanned(cave, x, y, MV_DOWN_LEFT, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_DOWN_RIGHT))	/* if not, maybe right (down-right to player) */
								store_dir_no_scanned(cave, x, y, MV_DOWN_RIGHT, O_BLADDER), remains=O_SPACE;
							break;
						case MV_LEFT:
							if (is_space_dir(cave, x, y, MV_LEFT_2))	/* pushing it left */
								store_dir_no_scanned(cave, x, y, MV_LEFT_2, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_DOWN_LEFT))	/* maybe down, and player will move left */
								store_dir_no_scanned(cave, x, y, MV_DOWN_LEFT, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_UP_LEFT))	/* maybe up, and player will move left */
								store_dir_no_scanned(cave, x, y, MV_UP_LEFT, O_BLADDER), remains=O_SPACE;
							break;
						case MV_RIGHT:
							if (is_space_dir(cave, x, y, MV_RIGHT_2))	/* pushing it right */
								store_dir_no_scanned(cave, x, y, MV_RIGHT_2, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_DOWN_RIGHT))	/* maybe down, and player will move right */
								store_dir_no_scanned(cave, x, y, MV_DOWN_RIGHT, O_BLADDER), remains=O_SPACE;
							else if (is_space_dir(cave, x, y, MV_UP_RIGHT))	/* maybe up, and player will move right */
								store_dir_no_scanned(cave, x, y, MV_UP_RIGHT, O_BLADDER), remains=O_SPACE;
							break;
						default:
							/* pushing bladder in other directions not possible */
							break;
						}
						break;
					default:
						/* get element. if cannot get, player_get_element will return the same */
						remains=player_get_element (cave, what);
						break;
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
				cave->player_seen_ago=0;
				/* bd4 intermission caves have many players. so if one of them has exited,
				 * do not change the flag anymore. so this if () is needed */
				if (cave->player_state!=PL_EXITED)
					cave->player_state=PL_LIVING;
				if (player_fire) {
					/* player "exits" stirring the pot by pressing fire */
					cave->gravity_disabled=FALSE;
					store(cave, x, y, O_PLAYER);
					cave->gravity_switch_active=TRUE;
				}
				break;

			case O_AMOEBA:
				amoeba_count++;
				if (cave->amoeba_too_big)
					store(cave, x, y, cave->too_big_amoeba_to);
				else if (cave->amoeba_enclosed)
					store(cave, x, y, cave->enclosed_amoeba_to);
				else {
					if (amoeba_found_enclosed)
						/* if still found enclosed, check all four directions, if this one is able to grow. */
						if (amoeba_eats_dir(cave, x, y, MV_UP) || amoeba_eats_dir(cave, x, y, MV_DOWN)
							|| amoeba_eats_dir(cave, x, y, MV_LEFT) || amoeba_eats_dir(cave, x, y, MV_RIGHT)) {
							amoeba_found_enclosed=FALSE;	/* not enclosed. this is a local (per scan) flag! */
							cave->amoeba_started=TRUE;
						}

					if (cave->amoeba_started)	/* if it is alive, decide if it attempts to grow */
						if (g_rand_int_range(cave->random, 0, 1000000)<cave->amoeba_growth_prob*1000000) {
							switch (g_rand_int_range(cave->random, 0, 4)) {	/* decided to grow, choose a random direction. */
							case 0:	/* let this be up. numbers indifferent. */
								if (amoeba_eats_dir(cave, x, y, MV_UP))
									store_dir(cave, x, y, MV_UP, O_AMOEBA);
								break;
							case 1:	/* down */
								if (amoeba_eats_dir(cave, x, y, MV_DOWN))
									store_dir(cave, x, y, MV_DOWN, O_AMOEBA);
								break;
							case 2:	/* left */
								if (amoeba_eats_dir(cave, x, y, MV_LEFT))
									store_dir(cave, x, y, MV_LEFT, O_AMOEBA);
								break;
							case 3:	/* right */
								if (amoeba_eats_dir(cave, x, y, MV_RIGHT))
									store_dir(cave, x, y, MV_RIGHT, O_AMOEBA);
								break;
							}
						}
				}
				break;

			case O_H_GROWING_WALL:
			case O_V_GROWING_WALL:
				/* checks first if direction is changed. */
				if ((get(cave, x, y)==O_H_GROWING_WALL && !cave->expanding_wall_changed)
					|| (get(cave, x, y)==O_V_GROWING_WALL && cave->expanding_wall_changed)) {
					if (is_space_dir(cave, x, y, MV_LEFT))
						store_dir(cave, x, y, MV_LEFT, get(cave, x, y));
					if (is_space_dir(cave, x, y, MV_RIGHT))
						store_dir(cave, x, y, MV_RIGHT, get(cave, x, y));
				}
				else {
					if (is_space_dir(cave, x, y, MV_UP))
						store_dir(cave, x, y, MV_UP, get(cave, x, y));
					if (is_space_dir(cave, x, y, MV_DOWN))
						store_dir(cave, x, y, MV_DOWN, get(cave, x, y));
				}
				break;

			case O_GROWING_WALL:
				/* the wall which grows in all four directions. */
				if (is_space_dir(cave, x, y, MV_LEFT))
					store_dir(cave, x, y, MV_LEFT, O_GROWING_WALL);
				if (is_space_dir(cave, x, y, MV_RIGHT))
					store_dir(cave, x, y, MV_RIGHT, O_GROWING_WALL);
				if (is_space_dir(cave, x, y, MV_UP))
					store_dir(cave, x, y, MV_UP, O_GROWING_WALL);
				if (is_space_dir(cave, x, y, MV_DOWN))
					store_dir(cave, x, y, MV_DOWN, O_GROWING_WALL);
				break;

			case O_SLIME:
				/*
				 * unpredictable: g_rand_int
				 * predictable: c64 predictable random generator.
				 *    for predictable, a random number is generated, whether or not it is even possible that the stone
				 *    will be able to pass. 
				 */
				if (cave->slime_predictable? ((gd_c64_predictable_random (cave)&cave->slime_permeability_c64)==0) : g_rand_int_range(cave->random, 0, 1000000)<cave->slime_permeability*1000000) {
					if (is_space_dir(cave, x, y, MV_DOWN)) {
						if (get_dir(cave, x, y, MV_UP)==cave->slime_eats_1) {
							store_dir(cave, x, y, MV_DOWN, cave->slime_converts_1);	/* output a falling xy under */
							store_dir(cave, x, y, MV_UP, O_SPACE);
						}
						else if (get_dir(cave, x, y, MV_UP)==cave->slime_eats_2) {
							store_dir(cave, x, y, MV_DOWN, cave->slime_converts_2);
							store_dir(cave, x, y, MV_UP, O_SPACE);
						}
						else if (get_dir(cave, x, y, MV_UP)==O_WAITING_STONE) {	/* waiting stones pass without awakening */
							store_dir(cave, x, y, MV_DOWN, O_WAITING_STONE);
							store_dir(cave, x, y, MV_UP, O_SPACE);
						}
						else if (get_dir(cave, x, y, MV_UP)==O_CHASING_STONE) {	/* chasing stones pass */
							store_dir(cave, x, y, MV_DOWN, O_CHASING_STONE);
							store_dir(cave, x, y, MV_UP, O_SPACE);
						}
					} else
					/* bladders move UP the slime */
					if (is_space_dir(cave, x, y, MV_UP) && get_dir(cave, x, y, MV_DOWN)==O_BLADDER) {
						store_dir(cave, x, y, MV_DOWN, O_SPACE);
						store_dir(cave, x, y, MV_UP, O_BLADDER_1);
					}
				}
				break;

			case O_ACID:
				/* choose randomly, if it spreads */
				if (g_rand_int_range(cave->random, 0, 1000000)<=cave->acid_spread_ratio*1000000) {
					/* the current one explodes */
					store(cave, x, y, cave->acid_turns_to);
					/* and if neighbours are eaten, put acid there. */
					if (is_element_dir(cave, x, y, MV_UP, cave->acid_eats_this))
						store_dir(cave, x, y, MV_UP, O_ACID);
					if (is_element_dir(cave, x, y, MV_DOWN, cave->acid_eats_this))
						store_dir(cave, x, y, MV_DOWN, O_ACID);
					if (is_element_dir(cave, x, y, MV_LEFT, cave->acid_eats_this))
						store_dir(cave, x, y, MV_LEFT, O_ACID);
					if (is_element_dir(cave, x, y, MV_RIGHT, cave->acid_eats_this))
						store_dir(cave, x, y, MV_RIGHT, O_ACID);
				}
				break;

			case O_TRAPPED_DIAMOND:
				if (cave->diamond_key_collected)
					store(cave, x, y, O_DIAMOND);
				break;

			case O_WAITING_STONE:
				if (is_space_dir(cave, x, y, MV_DOWN)) {	/* beginning to fall */
					/* he wakes up. */
					move(cave, x, y, MV_DOWN, O_CHASING_STONE);
				}
				else if (rounded_dir(cave, x, y, MV_DOWN)) {	/* rolling down a brick wall or a stone */
					if (is_space_dir(cave, x, y, MV_LEFT) && is_space_dir(cave, x, y, MV_DOWN_LEFT)) {
						/* maybe rolling left */
						move(cave, x, y, MV_LEFT, O_WAITING_STONE);
					}
					else if (is_space_dir(cave, x, y, MV_RIGHT) && is_space_dir(cave, x, y, MV_DOWN_RIGHT)) {
						/* or maybe right */
						move(cave, x, y, MV_RIGHT, O_WAITING_STONE);
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
						if (horizontal) {	/*********************************/
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
						} else {	/********************************/
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
						if (horizontal) {	/* check for horizontal */
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
						} else {	/* check for vertical */
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

			case O_BITER_1:
			case O_BITER_2:
			case O_BITER_3:
			case O_BITER_4:
				if (cave->biters_wait_frame==0) {
					static GdDirection biter_move[]={ MV_UP, MV_RIGHT, MV_DOWN, MV_LEFT };
					GdElement biter_try[]={ O_DIRT, cave->biter_eat, O_SPACE, O_STONE };
					int dir=get(cave, x, y)-O_BITER_1;	/* direction, last two bits 0..3 */
					int dirn=(dir+3)&3;
					int dirp=(dir+1)&3;
					int i;

					for (i=0; i<G_N_ELEMENTS (biter_try); i++) {
						if (is_element_dir(cave, x, y, biter_move[dir], biter_try[i])) {
							move(cave, x, y, biter_move[dir], O_BITER_1+dir);
							break;
						}
						else if (is_element_dir(cave, x, y, biter_move[dirn], biter_try[i])) {
							move(cave, x, y, biter_move[dirn], O_BITER_1+dirn);
							break;
						}
						else if (is_element_dir(cave, x, y, biter_move[dirp], biter_try[i])) {
							move(cave, x, y, biter_move[dirp], O_BITER_1+dirp);
							break;
						}
					}
					if (i==G_N_ELEMENTS (biter_try))
						/* i=number of elements in array: could not move, so just turn */
						store(cave, x, y, O_BITER_1+dirp);
					else if (biter_try[i]==O_STONE)
						/* if there was a stone there, where we moved... do not eat stones, just throw them back */
						store(cave, x, y, O_STONE);
				}
				break;

			case O_FALLING_WALL:
				if (is_space_dir(cave, x, y, MV_DOWN)) {
					/* try falling if space under. */
					int yy;
					for (yy=y+1; yy<y+cave->h; yy++)
						/* yy<y+cave->h is to check everything OVER the wall - since caves wrap around !! */
						if (get(cave, x, yy)!=O_SPACE)
							/* stop cycle when other than space */
							break;
					/* if scanning stopped by a player... start falling! */
					if (get(cave, x, yy)==O_PLAYER || get(cave, x, yy)==O_PLAYER_GLUED || get(cave, x, yy)==O_PLAYER_BOMB) {
						diamond_stone_sound(cave, get(cave, x, y));
						move(cave, x, y, MV_DOWN, O_FALLING_WALL_F);
					}
				}
				break;

			case O_FALLING_WALL_F:
				switch (get_dir(cave, x, y, MV_DOWN)) {
					case O_PLAYER:
					case O_PLAYER_GLUED:
					case O_PLAYER_BOMB:
						/* if player under, it explodes - the falling wall, not the player! */
						explode (cave, x, y);
						break;
					case O_SPACE:
						/* continue falling */
						move(cave, x, y, MV_DOWN, O_FALLING_WALL_F);
						break;
					default:
						/* stop */
						diamond_stone_sound(cave, get(cave, x, y));
						store(cave, x, y, O_FALLING_WALL);
						break;
					}
				break;

			case O_WATER:
				if (!cave->water_does_not_flow_down && is_space_dir(cave, x, y, MV_DOWN))	/* emulating the odd behaviour in crdr */
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

				/* EXPLOSIONS ETC */
			case O_EXPLODE_5:
				store(cave, x, y, cave->explosion_to);
				break;
			case O_PRE_DIA_5:
				store(cave, x, y, cave->diamond_birth_to);
				break;
			case O_PRE_STONE_4:
				store(cave, x, y, O_STONE);
				break;
			case O_GHOST_EXPL_4:
				{
					static GdElement ghost_explode[]={
						O_SPACE, O_SPACE, O_DIRT, O_DIRT, O_CLOCK, O_CLOCK, O_PRE_OUTBOX,
						O_BOMB, O_BOMB, O_PLAYER, O_GHOST, O_BLADDER, O_DIAMOND, O_SWEET,
						O_WAITING_STONE, O_BITER_1
					};

					store(cave, x, y, ghost_explode[g_rand_int_range(cave->random, 0, G_N_ELEMENTS (ghost_explode))]);
				}
				break;
			case O_BOMB_EXPL_4:
				store(cave, x, y, cave->bomb_explode_to);
				break;
			case O_PRE_STEEL_4:
				store(cave, x, y, O_STEEL);
				break;
			case O_PRE_CLOCK_4:
				store(cave, x, y, O_CLOCK);
				break;
			case O_BOMB_TICK_7:
				explode (cave, x, y);
				break;

			case O_PRE_OUTBOX:
				if (cave->diamonds_collected>=cave->diamonds_needed) /* if no more diamonds needed */
					store(cave, x, y, O_OUTBOX);	/* open outbox */
				break;
			case O_PRE_INVIS_OUTBOX:
				if (cave->diamonds_collected>=cave->diamonds_needed)	/* if no more diamonds needed */
					store(cave, x, y, O_INVIS_OUTBOX);	/* open outbox. invisible one :P */
				break;
			case O_INBOX:
				if (cave->hatching_delay==0 && !inbox_toggle)	/* if it is time of birth */
					store(cave, x, y, O_PRE_PL_1);
				inbox_toggle=!inbox_toggle;
				break;
			case O_PRE_PL_3:
				store(cave, x, y, O_PLAYER);
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
							break;	/* ghost did move -> exit loop */
						}
					}
				}
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
				/* simply the next identifier */
				next (cave, x, y);
				break;

			case O_BLADDER_SPENDER:
				if (is_space_dir(cave, x, y, MV_UP)) {
					store_dir(cave, x, y, MV_UP, O_BLADDER);
					store(cave, x, y, O_PRE_STEEL_1);
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
				next (cave, x, y);
				break;
			case O_BLADDER_9:
				/* bladders roll 'up', like stones roll down on brick. but: bladders do roll only brick, they
				 * float in place holding a diamond or a stone! or any other element. */
				if (is_element_dir(cave, x, y, MV_UP, cave->bladder_converts_by) || is_element_dir(cave, x, y, MV_DOWN, cave->bladder_converts_by) || is_element_dir(cave, x, y, MV_LEFT, cave->bladder_converts_by) || is_element_dir(cave, x, y, MV_RIGHT, cave->bladder_converts_by))
					/* if touches the specified element, let it be a clock */
					store(cave, x, y, O_PRE_CLOCK_1);
				else if (is_space_dir(cave, x, y, MV_UP))
					/* if able to go up */
					move(cave, x, y, MV_UP, O_BLADDER);
				else if (get_dir(cave, x, y, MV_UP)==O_BRICK && is_space_dir(cave, x, y, MV_LEFT) && is_space_dir(cave, x, y, MV_UP_LEFT))
					/* rolling up, to left */
					move(cave, x, y, MV_LEFT, O_BLADDER_9);
				else if (get_dir(cave, x, y, MV_UP)==O_BRICK && is_space_dir(cave, x, y, MV_RIGHT) && is_space_dir(cave, x, y, MV_UP_RIGHT))
					/* rolling up, to right */
					move(cave, x, y, MV_RIGHT, O_BLADDER_9);
				break;
			default:
				/* other inanimate elements that do nothing */
				break;
			}
		}

	/* finally: forget "scanned" flags for objects. */
	for (y=0; y<cave->h; y++)
		for (x=0; x<cave->w; x++) {
			if (get(cave, x, y)&SCANNED)
				store(cave, x, y, get(cave, x, y)&~SCANNED);
			if (get(cave, x, y)==O_TIME_PENALTY) {
				store(cave, x, y, O_GRAVESTONE);
				cave->time_decrement+=cave->penalty_time;	/* there is time penalty for destroying the voodoo */
			}
		}

	/* update timing calculated by iterating and counting elements which
	   were slow to process on c64 */
	if (cave->c64_scheduling) {
		if (cave->bd1_scheduling)
			cave->speed=(88+3.66*cave->c64_timing+(cave->ckdelay+cave->ckdelay_extra_for_animation)/1000);
		else
			cave->speed=MAX(65+cave->ckdelay/1000, cave->c64_timing*20);
	}
	
	if (cave->short_explosions) {
		for (y=0; y<cave->h; y++)
			for (x=0; x<cave->w; x++) 
				if (get(cave, x, y)==O_EXPLODE_1)
					store(cave, x, y, O_EXPLODE_2);
				else
				if (get(cave, x, y)==O_PRE_DIA_1)
					store(cave, x, y, O_PRE_DIA_2);
	}


	/* this loop finds the coordinates of the player. needed for scrolling and chasing stone.*/
	/* but we only do this, if a living player was found. if not yet, the setup routine coordinates are used */
	if (cave->player_state==PL_LIVING) {
		if (cave->active_is_first_found) {
			/* to be 1stb compatible, we do everything backwards. */
			for (y=cave->h-1; y>=0; y--)
				for (x=cave->w-1; x>=0; x--)
					if (get(cave, x, y)==O_PLAYER || get(cave, x, y)==O_PLAYER_BOMB || get(cave, x, y)==O_PLAYER_STIRRING) {
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
					if (get(cave, x, y)==O_PLAYER || get(cave, x, y)==O_PLAYER_BOMB || get(cave, x, y)==O_PLAYER_STIRRING) {
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

	/* amoeba and magic wall have sound. */
	if (cave->sound3==GD_S_NONE) {
		/* we so not set it, if a gate open triggered the crack sound already */
		if (cave->magic_wall_state==MW_ACTIVE && cave->magic_wall_sound)
			cave->sound3=GD_S_MAGIC_WALL;
		if (amoeba_count>0)
			cave->sound3=GD_S_AMOEBA;	/* amoeba sound takes precedence over magic wall sound */
	}
	
	if (cave->amoeba_started) {
		/* check flags after evaluating. */
		/* amoeba turns into diamond in the next frame, if enclosed */
		cave->amoeba_enclosed=amoeba_found_enclosed;
		/* too many amoeba found, it turns into stones in the next frame. */
		cave->amoeba_too_big=amoeba_count >= cave->amoeba_threshold;
	}
	/* amoeba can also be turned into diamond by magic wall */
	if (cave->magic_wall_stops_amoeba && cave->magic_wall_state==MW_ACTIVE)
		cave->amoeba_enclosed=TRUE;

	if ((cave->player_state==PL_LIVING && cave->player_seen_ago>15) || cave->kill_player)
	/* check if player is alive. */
		cave->player_state=PL_DIED;

	/* check if any voodoo exploded, and kill players the next scan if that happended. */
	if (voodoo_touched)
		cave->kill_player=TRUE;

	/* now check times. --------------------------- */
	/* decrement time if a voodoo was killed. */
	cave->time-=cave->time_decrement*cave->timing_factor;
	if (cave->time<0)
		cave->time=0;
	cave->time_decrement=0;

	/* only decrement time when player is already born. */
	if (cave->hatching_delay==0) {
		int secondsbefore, secondsafter;
		
		secondsbefore=cave->time/cave->timing_factor;
		cave->time-=cave->speed;
		if (cave->time<0)
			cave->time=0;
		secondsafter=cave->time/cave->timing_factor;
		if (secondsbefore && secondsafter)
			gd_cave_set_seconds_sound(cave);
	}
	/* a gravity switch was activated; seconds counting down */
	if (cave->gravity_will_change>0) {
		cave->gravity_will_change-=cave->speed;
		if (cave->gravity_will_change<0)
			cave->gravity_will_change=0;

		if (cave->gravity_will_change==0)
			cave->gravity=cave->gravity_next_direction;
	}

	/* check if cave failed by timeout */		
	if (cave->player_state==PL_LIVING && cave->time==0)
		cave->player_state=PL_TIMEOUT;

	/* creatures direction automatically change */
	if (cave->creatures_direction_will_change>0) {
		cave->creatures_direction_will_change-=cave->speed;
		if (cave->creatures_direction_will_change<0)
			cave->creatures_direction_will_change=0;

		if (cave->creatures_direction_will_change==0) {
			cave->creatures_backwards=!cave->creatures_backwards;
			cave->creatures_direction_will_change=cave->creatures_direction_auto_change_time*cave->timing_factor;
		}
	}

	/* magic wall; if active&wait or not wait for hatching */
	if (cave->magic_wall_state==MW_ACTIVE && (cave->hatching_delay==0 || !cave->magic_timer_wait_for_hatching)) {
		cave->magic_wall_milling_time-=cave->speed;
		if (cave->magic_wall_milling_time<0)
			cave->magic_wall_milling_time=0;
		if (cave->magic_wall_milling_time==0)
			cave->magic_wall_state=MW_EXPIRED;
	}
	/* we may wait for hatching, when starting amoeba */
	if (cave->amoeba_timer_started_immediately || (cave->amoeba_started && (cave->hatching_delay==0 || !cave->amoeba_timer_wait_for_hatching))) {
		cave->amoeba_slow_growth_time-=cave->speed;
		if (cave->amoeba_slow_growth_time<0)
			cave->amoeba_slow_growth_time=0;
		if (cave->amoeba_slow_growth_time==0)
			cave->amoeba_growth_prob=cave->amoeba_fast_growth_prob;
	}

	/* check for player hatching. */
	start_signal=FALSE;
	/* if not the c64 scheduling, but the correct frametime is used, hatching delay should always be decremented. */
	/* otherwise, the if (millisecs...) condition below will set this. */
	if (!cave->c64_scheduling) {		/* NON-C64 scheduling */
		if (cave->hatching_delay>0) {
			cave->hatching_delay--;
			if (cave->hatching_delay==0)
				start_signal=TRUE;
		}
	}
	else {								/* C64 scheduling */
		if (cave->hatching_delay>0) {
			cave->hatching_delay-=cave->speed;
			if (cave->hatching_delay<=0) {
				cave->hatching_delay=0;
				start_signal=TRUE;
			}
		}
	}

	/* if decremented hatching, and it became zero: */
	if (start_signal) {		/* THIS IS THE CAVE START SIGNAL */
		/* setup direction auto change */
		if (cave->creatures_direction_auto_change_time) {
			cave->creatures_direction_will_change=cave->creatures_direction_auto_change_time*cave->timing_factor;

			if (cave->creatures_direction_auto_change_on_start)
				cave->creatures_backwards=!cave->creatures_backwards;
		}
		
		cave->sound3=GD_S_CRACK;	/* takes precedence over amoeba and magic wall sound */
	}

	/* for biters */
	if (cave->biters_wait_frame==0)
		cave->biters_wait_frame=cave->biter_delay_frame;
	else
		cave->biters_wait_frame--;

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
}

