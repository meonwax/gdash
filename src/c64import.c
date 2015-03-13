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
#include <glib/gi18n.h>
#include <string.h>
#include "settings.h"
#include "cave.h"
#include "caveobject.h"
#include "c64import.h"
#include "util.h"

/* conversion table for imported plck caves. */
static const GdElement plck_nybble[]={
	/*  0 */ O_STONE, O_DIAMOND, O_MAGIC_WALL, O_BRICK,
	/*  4 */ O_STEEL, O_H_GROWING_WALL, O_VOODOO, O_DIRT,
	/*  8 */ O_GUARD_1, O_BUTTER_4, O_AMOEBA, O_SLIME,
	/* 12 */ O_PRE_INVIS_OUTBOX, O_PRE_OUTBOX, O_INBOX, O_SPACE
};

/* conversion table for imported crazy dream caves. */
static const GdElement crazydream_import_table[]={
	/*  0 */ O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL,
	/*  4 */ O_PRE_OUTBOX, O_OUTBOX, O_PRE_INVIS_OUTBOX, O_INVIS_OUTBOX,
	/*  8 */ O_GUARD_1, O_GUARD_2, O_GUARD_3, O_GUARD_4,
	/*  c */ O_GUARD_1, O_GUARD_2, O_GUARD_3, O_GUARD_4,
	/* 10 */ O_STONE, O_STONE, O_STONE_F, O_STONE_F,
	/* 14 */ O_DIAMOND, O_DIAMOND, O_DIAMOND_F, O_DIAMOND_F,
	/* 18 */ O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4,
	/* 1c */ O_BITER_SWITCH, O_BITER_SWITCH, O_BLADDER_SPENDER, O_PRE_DIA_1,	/* 6 different stages */
	/* 20 */ O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4,
	/* 24 */ O_PRE_DIA_5, O_INBOX, O_PRE_PL_1, O_PRE_PL_2,
	/* 28 */ O_PRE_PL_3, O_CLOCK, O_H_GROWING_WALL, O_H_GROWING_WALL,	/* CLOCK: not mentioned in marek's bd inside faq */
	/* 2c */ O_CREATURE_SWITCH, O_CREATURE_SWITCH, O_GROWING_WALL_SWITCH, O_GROWING_WALL_SWITCH,
	/* 30 */ O_BUTTER_3, O_BUTTER_4, O_BUTTER_1, O_BUTTER_2,
	/* 34 */ O_BUTTER_3, O_BUTTER_4, O_BUTTER_1, O_BUTTER_2,
	/* 38 */ O_STEEL, O_SLIME, O_BOMB, O_SWEET,
	/* 3c */ O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4,
	/* 40 */ O_BLADDER, O_BLADDER_1, O_BLADDER_2, O_BLADDER_3,
	/* 44 */ O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7,
	/* 48 */ O_BLADDER_8, O_BLADDER_9, O_EXPLODE_1, O_EXPLODE_1,
	/* 4c */ O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5,
	/* 50 */ O_PLAYER, O_PLAYER, O_PLAYER_BOMB, O_PLAYER_BOMB,
	/* 54 */ O_PLAYER_GLUED, O_PLAYER_GLUED, O_VOODOO, O_AMOEBA,
	/* 58 */ O_AMOEBA, O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3,
	/* 5c */ O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
	/* 60 */ O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4,
	/* 64 */ O_GHOST, O_GHOST, O_GHOST_EXPL_1, O_GHOST_EXPL_2,
	/* 68 */ O_GHOST_EXPL_3, O_GHOST_EXPL_4, O_GRAVESTONE, O_STONE_GLUED,
	/* 6c */ O_DIAMOND_GLUED, O_DIAMOND_KEY, O_TRAPPED_DIAMOND, O_GRAVESTONE,
	/* 70 */ O_WAITING_STONE, O_WAITING_STONE, O_CHASING_STONE, O_CHASING_STONE,
	/* 74 */ O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
	/* 78 */ O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,
	/* 7c */ O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,

	/* 80 */ O_POT, O_PLAYER_STIRRING, O_GRAVITY_SWITCH, O_GRAVITY_SWITCH,
	/* 84 */ O_PNEUMATIC_HAMMER, O_PNEUMATIC_HAMMER, O_BOX, O_BOX,
	/* 88 */ O_UNKNOWN, O_UNKNOWN, O_ACID, O_ACID,
	/* 8c */ O_KEY_1, O_KEY_2, O_KEY_3, O_UNKNOWN,
	/* 90 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
	/* 94 */ O_UNKNOWN, O_TELEPORTER, O_UNKNOWN, O_SKELETON,
	/* 98 */ O_WATER, O_WATER_16, O_WATER_15, O_WATER_14,
	/* 9c */ O_WATER_13, O_WATER_12, O_WATER_11, O_WATER_10,
	/* a0 */ O_WATER_9, O_WATER_8, O_WATER_7, O_WATER_6,
	/* a4 */ O_WATER_5, O_WATER_4, O_WATER_3, O_WATER_2,
	/* a8 */ O_WATER_1, O_COW_ENCLOSED_1, O_COW_ENCLOSED_2, O_COW_ENCLOSED_3,
	/* ac */ O_COW_ENCLOSED_4, O_COW_ENCLOSED_5, O_COW_ENCLOSED_6, O_COW_ENCLOSED_7,
	/* b0 */ O_COW_1, O_COW_2, O_COW_3, O_COW_4,
	/* b4 */ O_COW_1, O_COW_2, O_COW_3, O_COW_4,
	/* b8 */ O_DIRT_GLUED, O_STEEL_EXPLODABLE, O_DOOR_1, O_DOOR_2,
	/* bc */ O_DOOR_3, O_FALLING_WALL, O_FALLING_WALL_F, O_FALLING_WALL_F,
	/* c0 */ O_WALLED_DIAMOND, O_UNKNOWN, O_WALLED_KEY_1, O_WALLED_KEY_2,
/* c5=brick?! (vital key), c7=dirt?! (think twice) */
/* c7=dirt, as it has a code which will change it to dirt. */
	/* c4 */ O_WALLED_KEY_3, O_BRICK, O_UNKNOWN, O_DIRT,
	/* c8 */ O_DIRT2, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
	/* cc */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
	/* d0 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
	/* d4 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
	/* d8 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
	/* dc */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
	/* e0 */ O_ALT_GUARD_1, O_ALT_GUARD_2, O_ALT_GUARD_3, O_ALT_GUARD_4,
	/* e4 */ O_ALT_GUARD_1, O_ALT_GUARD_2, O_ALT_GUARD_3, O_ALT_GUARD_4,
	/* e8 */ O_ALT_BUTTER_3, O_ALT_BUTTER_4, O_ALT_BUTTER_1, O_ALT_BUTTER_2,
	/* ec */ O_ALT_BUTTER_3, O_ALT_BUTTER_4, O_ALT_BUTTER_1, O_ALT_BUTTER_2,
	/* f0 */ O_WATER, O_WATER, O_WATER, O_WATER,
	/* f4 */ O_WATER, O_WATER, O_WATER, O_WATER,
	/* f8 */ O_WATER, O_WATER, O_WATER, O_WATER,
	/* fc */ O_WATER, O_WATER, O_WATER, O_WATER,
};

/* conversion table for imported 1stb caves. */
const GdElement gd_crazylight_import_table[]={
	/*  0 */ O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL,
	/*  4 */ O_PRE_OUTBOX, O_OUTBOX, O_PRE_INVIS_OUTBOX, O_INVIS_OUTBOX,
	/*  8 */ O_GUARD_1, O_GUARD_2, O_GUARD_3, O_GUARD_4,
	/*  c */ O_GUARD_1|SCANNED, O_GUARD_2|SCANNED, O_GUARD_3|SCANNED, O_GUARD_4|SCANNED,
	/* 10 */ O_STONE, O_STONE|SCANNED, O_STONE_F, O_STONE_F|SCANNED,
	/* 14 */ O_DIAMOND, O_DIAMOND|SCANNED, O_DIAMOND_F, O_DIAMOND_F|SCANNED,
	/* 18 */ O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4,
	/* 1c */ O_BITER_SWITCH, O_BITER_SWITCH, O_BLADDER_SPENDER, O_PRE_DIA_1,	/* 6 different stages, the first is the pre_dia_0 */
	/* 20 */ O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4,
	/* 24 */ O_PRE_DIA_5, O_INBOX, O_PRE_PL_1, O_PRE_PL_2,
	/* 28 */ O_PRE_PL_3, O_CLOCK, O_H_GROWING_WALL, O_H_GROWING_WALL|SCANNED,	/* CLOCK: not mentioned in marek's bd inside faq */
	/* 2c */ O_CREATURE_SWITCH, O_CREATURE_SWITCH, O_GROWING_WALL_SWITCH, O_GROWING_WALL_SWITCH,
	/* 30 */ O_BUTTER_3, O_BUTTER_4, O_BUTTER_1, O_BUTTER_2,
	/* 34 */ O_BUTTER_3|SCANNED, O_BUTTER_4|SCANNED, O_BUTTER_1|SCANNED, O_BUTTER_2|SCANNED,
	/* 38 */ O_STEEL, O_SLIME, O_BOMB, O_SWEET,
	/* 3c */ O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4,
	/* 40 */ O_BLADDER, O_BLADDER_1, O_BLADDER_2, O_BLADDER_3,
	/* 44 */ O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7,
	/* 48 */ O_BLADDER_8, O_BLADDER_9, O_EXPLODE_1, O_EXPLODE_1,
	/* 4c */ O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5,
	/* 50 */ O_PLAYER, O_PLAYER|SCANNED, O_PLAYER_BOMB, O_PLAYER_BOMB|SCANNED,
	/* 54 */ O_PLAYER_GLUED, O_PLAYER_GLUED|SCANNED, O_VOODOO, O_AMOEBA,
	/* 58 */ O_AMOEBA|SCANNED, O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3,
	/* 5c */ O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
	/* 60 */ O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4,
	/* 64 */ O_ACID, O_ACID, O_FALLING_WALL, O_FALLING_WALL_F,
	/* 68 */ O_FALLING_WALL_F|SCANNED, O_BOX, O_GRAVESTONE, O_STONE_GLUED,
	/* 6c */ O_DIAMOND_GLUED, O_DIAMOND_KEY, O_TRAPPED_DIAMOND, O_GRAVESTONE,
	/* 70 */ O_WAITING_STONE, O_WAITING_STONE|SCANNED, O_CHASING_STONE, O_CHASING_STONE|SCANNED,
	/* 74 */ O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
	/* 78 */ O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,
	/* 7c */ O_BITER_1|SCANNED, O_BITER_2|SCANNED, O_BITER_3|SCANNED, O_BITER_4|SCANNED,
};


static guint8 no1_default_colors[]={
	4, 10, 1, 8, 9, 3, 12, 11, 1, 6, 14, 7,	14, 3, 7,
	5, 8, 7, 4, 9, 3, 10, 5, 1, 5, 4, 1, 9, 6, 1,
	12, 11, 5, 4, 2, 7, 14, 4, 7, 10, 8, 1,	8, 5, 7,
	14, 2, 3, 3, 11, 1, 7, 5, 1, 11, 10, 7, 9, 8, 1
};

/* internal character (letter) codes in c64 games.
   missing: "triple line" after >, diamond between ()s, player's head after )
   used for converting names of caves imported from crli and other types of binary data */
const char *gd_bd_internal_chars="            ,!./0123456789:*<=>  ABCDEFGHIJKLMNOPQRSTUVWXYZ( ) _";

typedef enum _dirt_mod {
	DIRT_MOD_NEVER,
	DIRT_MOD_SLIME,
	DIRT_MOD_AMOEBA,
	DIRT_MOD_BOTH,
} DirtModType;

static GdElement
bd1_import(guint8 c, int i)
{
	/* conversion table for imported bd1 caves. */
	static const GdElement bd1_import_table[]={
		/*  0 */ O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL,
		/*  4 */ O_PRE_OUTBOX, O_OUTBOX, O_STEEL_EXPLODABLE, O_STEEL,
		/*  8 */ O_GUARD_1, O_GUARD_2, O_GUARD_3, O_GUARD_4,
		/*  c */ O_GUARD_1, O_GUARD_2, O_GUARD_3, O_GUARD_4,
		/* 10 */ O_STONE, O_STONE, O_STONE_F, O_STONE_F,
		/* 14 */ O_DIAMOND, O_DIAMOND, O_DIAMOND_F, O_DIAMOND_F,
		/* 18 */ O_ACID, O_ACID, O_EXPLODE_1, O_EXPLODE_2,	/* ACID: marek roth extension in crazy dream 3 */
		/* 1c */ O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5, O_PRE_DIA_1,
		/* 20 */ O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4, O_PRE_DIA_5,
		/* 24 */ O_PRE_DIA_5, O_INBOX, O_PRE_PL_1, O_PRE_PL_2,
		/* 28 */ O_PRE_PL_3, O_PRE_PL_3, O_H_GROWING_WALL, O_H_GROWING_WALL,
		/* 2c */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
		/* 30 */ O_BUTTER_4, O_BUTTER_1, O_BUTTER_2, O_BUTTER_3,
		/* 34 */ O_BUTTER_4, O_BUTTER_1, O_BUTTER_2, O_BUTTER_3,
		/* 38 */ O_PLAYER, O_PLAYER, O_AMOEBA, O_AMOEBA,
		/* 3c */ O_VOODOO, O_INVIS_OUTBOX, O_SLIME, O_UNKNOWN
	};

	if (c<G_N_ELEMENTS(bd1_import_table))
		return bd1_import_table[c];
	g_warning("Invalid BD1 element in imported file at cave data %d: %d", i, c);
	return O_UNKNOWN;
}

static GdElement
firstboulder_import(guint8 c, int i)
{
	/* conversion table for imported 1stb caves. */
	static const GdElement firstboulder_import_table[]={
		/*  0 */ O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL,
		/*  4 */ O_PRE_OUTBOX, O_OUTBOX, O_PRE_INVIS_OUTBOX, O_INVIS_OUTBOX,
		/*  8 */ O_GUARD_1, O_GUARD_2, O_GUARD_3, O_GUARD_4,
		/*  c */ O_GUARD_1, O_GUARD_2, O_GUARD_3, O_GUARD_4,
		/* 10 */ O_STONE, O_STONE, O_STONE_F, O_STONE_F,
		/* 14 */ O_DIAMOND, O_DIAMOND, O_DIAMOND_F, O_DIAMOND_F,
		/* 18 */ O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4,
		/* 1c */ O_BITER_SWITCH, O_BITER_SWITCH, O_BLADDER_SPENDER, O_PRE_DIA_1,
		/* 20 */ O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4,
		/* 24 */ O_PRE_DIA_5, O_INBOX, O_PRE_PL_1, O_PRE_PL_2,
		/* 28 */ O_PRE_PL_3, O_CLOCK, O_H_GROWING_WALL, O_H_GROWING_WALL,	/* CLOCK: not mentioned in marek's bd inside faq */
		/* 2c */ O_CREATURE_SWITCH, O_CREATURE_SWITCH, O_GROWING_WALL_SWITCH, O_GROWING_WALL_SWITCH,
		/* 30 */ O_BUTTER_3, O_BUTTER_4, O_BUTTER_1, O_BUTTER_2,
		/* 34 */ O_BUTTER_3, O_BUTTER_4, O_BUTTER_1, O_BUTTER_2,
		/* 38 */ O_STEEL, O_SLIME, O_BOMB, O_SWEET,
		/* 3c */ O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4,
		/* 40 */ O_BLADDER, O_BLADDER_1, O_BLADDER_2, O_BLADDER_3,
		/* 44 */ O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7,
		/* 48 */ O_BLADDER_8, O_BLADDER_9, O_EXPLODE_1, O_EXPLODE_1,
		/* 4c */ O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5,
		/* 50 */ O_PLAYER, O_PLAYER, O_PLAYER_BOMB, O_PLAYER_BOMB,
		/* 54 */ O_PLAYER_GLUED, O_PLAYER_GLUED, O_VOODOO, O_AMOEBA,
		/* 58 */ O_AMOEBA, O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3,
		/* 5c */ O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
		/* 60 */ O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4,
		/* 64 */ O_GHOST, O_GHOST, O_GHOST_EXPL_1, O_GHOST_EXPL_2,
		/* 68 */ O_GHOST_EXPL_3, O_GHOST_EXPL_4, O_GRAVESTONE, O_STONE_GLUED,
		/* 6c */ O_DIAMOND_GLUED, O_DIAMOND_KEY, O_TRAPPED_DIAMOND, O_GRAVESTONE,
		/* 70 */ O_WAITING_STONE, O_WAITING_STONE, O_CHASING_STONE, O_CHASING_STONE,
		/* 74 */ O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
		/* 78 */ O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,
		/* 7c */ O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,
	};

	if (c<G_N_ELEMENTS(firstboulder_import_table))
		return firstboulder_import_table[c];
	g_warning("Invalid 1stB element in imported file at cave data %d: %d", i, c);
	return O_UNKNOWN;
}

static GdElement
crazylight_import(guint8 c, int i)
{
	if (c<G_N_ELEMENTS(gd_crazylight_import_table))
		return gd_crazylight_import_table[c] & O_MASK;	/* & O_MASK: do not import "scanned" flag */
	g_warning("Invalid CrLi element in imported file at cave data %d: %d", i, c);
	return O_UNKNOWN;
}

static void
set_dirt_mod (Cave *cave, DirtModType mod_type)
{
	GdElement elem1, elem2;
	gboolean dirt=FALSE;
	GList *iter;
	int i;
	
	/* routine below can check for two different elements; if we need only one, 
	   set variables to same value */
	switch (mod_type) {
		default:
		case DIRT_MOD_NEVER:
			return;
		case DIRT_MOD_SLIME:
			elem1=elem2=O_SLIME;
			break;
		case DIRT_MOD_AMOEBA:
			elem1=elem2=O_AMOEBA;
			break;
		case DIRT_MOD_BOTH:
			elem1=O_SLIME;
			elem2=O_AMOEBA;
			break;
	};

	if (cave->map) {
		int x, y;
		for (y=0; y<cave->h; y++)
			for (x=0; x<cave->w; x++)
				if (cave->map[y][x]==elem1 || cave->map[y][x]==elem2)
					dirt=TRUE;
	}
	for (i=0; i<G_N_ELEMENTS(cave->random_fill); i++)
		if (cave->random_fill[i]==elem1 || cave->random_fill[i]==elem2)
			dirt=TRUE;
	for (iter=cave->objects; iter!=NULL; iter=iter->next) {
		GdObject *object=(GdObject *) iter->data;
		
		if (object->element==elem1 || object->fill_element==elem1 ||
		    object->element==elem2 || object->fill_element==elem2)
		    	dirt=TRUE;
	}
	if (dirt)
		cave->dirt_looks_like=O_DIRT2;
	return;
}




void
gd_cave_set_bd1_defaults(Cave *cave)
{
	cave->c64_scheduling=TRUE;
	cave->bd1_scheduling=TRUE;
	cave->lineshift=TRUE;
	cave->pal_timing=TRUE;
	cave->level_ckdelay[0]=12;	/* original ckdelay values as documented by peter broadribb. */
	cave->level_ckdelay[1]=6;
	cave->level_ckdelay[2]=3;
	cave->level_ckdelay[3]=1;
	cave->level_ckdelay[4]=0;

	/* set visible size for intermission */
	if (cave->intermission) {
		int i;
		
		for(i=0; i<5; i++)	/* intermissions are FAST */
			cave->level_ckdelay[i]=0;
		cave->x2=19;
		cave->y2=11;
	}
	cave->active_is_first_found=FALSE;
	cave->intermission_instantlife=TRUE;
	cave->intermission_rewardlife=FALSE;
}

void
gd_cave_set_bd2_defaults(Cave *cave)
{
	cave->lineshift=TRUE;
	cave->pal_timing=TRUE;
	cave->c64_scheduling=TRUE;
	cave->bd1_scheduling=TRUE;
	cave->magic_wall_stops_amoeba=FALSE;	/* marek roth bd inside faq 3.0 */
	cave->amoeba_timer_started_immediately=FALSE;
	cave->level_ckdelay[0]=12;	/* original ckdelay values as documented by peter broadribb. */
	cave->level_ckdelay[1]=6;
	cave->level_ckdelay[2]=3;
	cave->level_ckdelay[3]=1;
	cave->level_ckdelay[4]=0;

	/* set visible size for intermission */
	if (cave->intermission) {
		int i;
		
		for(i=0; i<5; i++)	/* intermissions are FAST */
			cave->level_ckdelay[i]=0;
		cave->x2=19;
		cave->y2=11;
	}
	cave->active_is_first_found=FALSE;
	cave->intermission_instantlife=TRUE;
	cave->intermission_rewardlife=FALSE;
}

void
gd_cave_set_plck_defaults(Cave *cave)
{
	cave->lineshift=TRUE;
	cave->pal_timing=TRUE;
	cave->c64_scheduling=TRUE;
	cave->bd1_scheduling=FALSE;
	cave->magic_wall_stops_amoeba=FALSE;	/* different from bd1 */
	cave->active_is_first_found=FALSE;
	cave->intermission_instantlife=TRUE;
	cave->intermission_rewardlife=FALSE;
}

void
gd_cave_set_1stb_defaults(Cave *cave)
{
	cave->lineshift=TRUE;
	cave->pal_timing=TRUE;
	cave->c64_scheduling=TRUE;
	cave->bd1_scheduling=FALSE;
	cave->amoeba_timer_started_immediately=FALSE;
	cave->amoeba_timer_wait_for_hatching=TRUE;
	cave->voodoo_dies_by_stone=TRUE;
	cave->voodoo_collects_diamonds=TRUE;
	cave->voodoo_can_be_destroyed=FALSE;
	cave->short_explosions=FALSE;
	cave->creatures_direction_auto_change_on_start=TRUE;
	cave->magic_wall_stops_amoeba=FALSE;
	cave->magic_timer_wait_for_hatching=TRUE;
	cave->enclosed_amoeba_to=O_PRE_DIA_1;	/* not immediately to diamond, but with animation */
	cave->dirt_looks_like=O_DIRT2;
	cave->intermission_instantlife=FALSE;
	cave->intermission_rewardlife=TRUE;
}

void
gd_cave_set_crdr_defaults(Cave *cave)
{
	cave->lineshift=TRUE;
	cave->pal_timing=TRUE;
	cave->c64_scheduling=TRUE;
	cave->bd1_scheduling=FALSE;
	cave->amoeba_timer_started_immediately=FALSE;
	cave->amoeba_timer_wait_for_hatching=TRUE;
	cave->voodoo_dies_by_stone=TRUE;
	cave->voodoo_collects_diamonds=TRUE;
	cave->voodoo_can_be_destroyed=FALSE;
	cave->short_explosions=FALSE;
	cave->magic_wall_stops_amoeba=FALSE;
	cave->magic_timer_wait_for_hatching=TRUE;
	cave->enclosed_amoeba_to=O_PRE_DIA_1;	/* not immediately to diamond, but with animation */
	cave->water_does_not_flow_down=TRUE;
	cave->intermission_instantlife=FALSE;
	cave->intermission_rewardlife=TRUE;
	cave->skeletons_worth_diamonds=1;	/* in crdr, skeletons can also be used to open the gate */
}

void
gd_cave_set_crli_defaults(Cave *cave)
{
	cave->lineshift=TRUE;
	cave->pal_timing=TRUE;
	cave->c64_scheduling=TRUE;
	cave->bd1_scheduling=FALSE;
	cave->amoeba_timer_started_immediately=FALSE;
	cave->amoeba_timer_wait_for_hatching=TRUE;
	cave->voodoo_dies_by_stone=TRUE;
	cave->voodoo_collects_diamonds=TRUE;
	cave->voodoo_can_be_destroyed=FALSE;
	cave->short_explosions=FALSE;
	cave->magic_wall_stops_amoeba=FALSE;
	cave->magic_timer_wait_for_hatching=TRUE;
	cave->enclosed_amoeba_to=O_PRE_DIA_1;	/* not immediately to diamond, but with animation */
	cave->intermission_instantlife=FALSE;
	cave->intermission_rewardlife=TRUE;
}





/****************************************************************************
 *
 * cave import routines.
 * take a cave, data, and maybe remaining bytes.
 * return the number of bytes read, -1 if error.
 *
 ****************************************************************************/



/*
  take care of required diamonds values==0 or >100.
  in original bd, the counter was only two-digit. so bd3 cave f
  says 150 diamonds required, but you only had to collect 50.
  also, gate opening is triggered by incrementing diamond
  count and THEN checking if more required; so if required was
  0, you had to collect 100. (also check crazy light 8 cave "1000")
  
  http://www.boulder-dash.nl/forum/viewtopic.php?t=88
*/

/* import bd1 cave data into our format. */
static int
cave_copy_from_bd1(Cave *cave, const guint8 *data, int remaining_bytes)
{
	int length, direction;
	int index;
	int level;
	guint8 code;
	GdObject object;
	int i;
	
	gd_error_set_context(cave->name);

	if (remaining_bytes<33) {
		g_critical("truncated BD1 cave data, %d bytes", remaining_bytes);
		return -1;
	}
	gd_cave_set_bd1_defaults(cave);

	/* cave number data[0] */
	cave->amoeba_slow_growth_time=data[1];
	cave->magic_wall_milling_time=data[1];
	cave->diamond_value=data[2];
	cave->extra_diamond_value=data[3];

	for (level=0; level < 5; level++) {
		cave->level_rand[level]=data[4 + level];
		cave->level_diamonds[level]=data[9 + level] % 100;	/* check comment above */
		if (cave->level_diamonds[level]==0)		/* gate opening is checked AFTER adding to diamonds collected, so 0 here means 100 to collect */
			cave->level_diamonds[level]=100;
		cave->level_time[level]=data[14 + level];
	}

	/* LogicDeLuxe extension: acid
		$16 Acid speed (unused in the original BD1)
		$17 Bit 2: if set, Acid's original position converts to explosion puff during spreading. Otherwise, Acid remains intact, ie. it's just growing. (unused in the original BD1)
		$1C Acid eats this element. (also Probability of element 1)
		
		there is no problem importing these; as other bd1 caves did not contain acid at all, so it does not matter
		how we set the values.
	*/
	cave->acid_eats_this=bd1_import(data[0x1c]&0x3F, 0x1c);	/* 0x1c index: same as probability1 !!!!! don't be surprised. we do a &0x3f because of this */
	cave->acid_spread_ratio=data[0x16]/255.0;	/* acid speed */
	cave->acid_turns_to=(data[0x17]&(1<<2))?O_EXPLODE_3:O_ACID;
	
	cave->color0=gd_c64_colors[0].rgb;	/* border - not important */
	cave->color1=gd_c64_colors[data[19]&0xf].rgb;
	cave->color2=gd_c64_colors[data[20]&0xf].rgb;
	cave->color3=gd_c64_colors[data[21]&0x7].rgb; 	/* lower 3 bits only (vic-ii worked this way) */
	cave->color4=cave->color3;	/* in bd1, amoeba was color3 */
	cave->color5=cave->color3;	/* no slime, but let it be color 3 */

	/* random fill */
	for (i=0; i < 4; i++) {
		cave->random_fill[i]=bd1_import(data[24+i], 24+i);
		cave->random_fill_probability[i]=data[28+i];
	}

	/*
	 * Decode the explicit cave data 
	 */
	object.levels=GD_OBJECT_LEVEL_ALL;
	index=32;
	while(data[index]!=0xFF && index<remaining_bytes && index<255) {
		code=data[index];
		/* crazy dream 3 extension: */
		if (code==0x0f) {
			int x1, y1, nx, ny, dx, dy;
			int x, y;
			
			/* as this one uses nonstandard dx dy values, create points instead */
			object.type=POINT;
			object.element=bd1_import(data[index+1], index+1);
			x1=data[index+2];
			y1=data[index+3]-2;
			nx=data[index+4];
			ny=data[index+5];
			dx=data[index+6];
			dy=data[index+7]+1;
			
			for (y=0; y<ny; y++)
				for (x=0; x<nx; x++) {
					int pos=x1+ y1*40+ y*dy*40 +x*dx;
					
					object.x1=pos%40;
					object.y1=pos/40;
					cave->objects=g_list_append (cave->objects, g_memdup (&object, sizeof (GdObject)));
				}
			object.type=NONE;	/* forget this so g_list_append will not apply below */
			index+=8;
		} else {
			/* object is code&3f, object type is upper 2 bits */
			object.element=bd1_import(code & 0x3F, index);
			switch ((code >> 6) & 3) {
			case 0:				/* 00: POINT */
				object.type=POINT;
				object.x1=data[index+1];
				object.y1=data[index+2]-2;
				if (object.x1>=cave->w || object.y1>=cave->h)
					g_warning("invalid point coordinates %d,%d at byte %d", object.x1, object.y1, index);

				index+=3;
				break;
			case 1:				/* 01: LINE */
				object.type=LINE;
				object.x1=data[index+1];
				object.y1=data[index+2]-2;
				length=data[index+3]-1;
				direction=data[index+4];
				if (direction>MV_UP_LEFT) {
					g_warning("invalid line direction %d at byte %d", direction, index);
					direction=MV_STILL;
				}
				object.x2=object.x1+length*gd_dx[direction+1];
				object.y2=object.y1+length*gd_dy[direction+1];
				if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2>=cave->h)
					g_warning("invalid line coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index-5);
				index+=5;
				break;
			case 2:				/* 10: FILLED RECTANGLE */
				object.type=FILLED_RECTANGLE;
				object.x1=data[index+1];
				object.y1=data[index+2] - 2;
				object.x2=object.x1+data[index+3]-1;	/* width */
				object.y2=object.y1+data[index+4]-1;	/* height */
				object.fill_element=bd1_import(data[index+5], index+5);
				if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2>=cave->h)
					g_warning("invalid filled rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
				
				index+=6;
				break;
			case 3:				/* 11: OPEN RECTANGLE (OUTLINE) */
				object.type=RECTANGLE;
				object.x1=data[index+1];
				object.y1=data[index+2]-2;
				object.x2=object.x1+data[index+3]-1;
				object.y2=object.y1+data[index+4]-1;
				if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2>=cave->h)
					g_warning("invalid rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
				index+=5;
				break;
			}
		}
		if (object.type!=NONE)
			cave->objects=g_list_append (cave->objects, g_memdup (&object, sizeof (GdObject)));
	}
	if (data[index]!=0xFF) {
		g_critical("import error, cave not delimited with 0xFF");
		return -1;
	}
	
	return index+1;
}

/* import bd2 cave data into our format. return number of bytes if pointer passed.
	this is pretty the same as above, only the encoding was different. */
static int
cave_copy_from_bd2 (Cave *cave, const guint8 *data, int remaining_bytes)
{
	int index;
	GdObject object;
	int i;
	int x, y, rx, ry;

	gd_error_set_context(cave->name);
	if (remaining_bytes<0x1A+5) {
		g_critical("truncated BD2 cave data, %d bytes", remaining_bytes);
		return -1;
	}
	gd_cave_set_bd2_defaults(cave);

	cave->amoeba_slow_growth_time=data[0];
	cave->magic_wall_milling_time=data[0];
	cave->diamond_value=data[1];
	cave->extra_diamond_value=data[2];

	for (i=0; i<5; i++) {
		cave->level_rand[i]=data[13+i];
		cave->level_diamonds[i]=data[8+i];
		if (cave->level_diamonds[i]==0)		/* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 needed */
			cave->level_diamonds[i]=1000;
		cave->level_time[i]=data[3+i];
	}

	for (i=0; i<4; i++) {
		cave->random_fill[i]=bd1_import(data[0x16 + i], 0x16+i);
		cave->random_fill_probability[i]=data[0x12 + i];
	}

	/*
	 * Decode the explicit cave data 
	 */
	index=0x1A;
	object.levels=GD_OBJECT_LEVEL_ALL;
	while (data[index]!=0xFF && index<remaining_bytes) {
		int nx, ny;
		unsigned int addr;
		int val, n, bytes;
		int length, direction;

		object.type=NONE;

		switch (data[index]) {
		case 0:				/* LINE */
			object.type=LINE;
			object.element=bd1_import(data[index+1], index+1);
			object.y1=data[index+2];
			object.x1=data[index+3];
			direction=data[index+4]/2;	/* they are multiplied by two - 0 is up, 2 is upright, 4 is right... */
			length=data[index+5]-1;
			if (direction>MV_UP_LEFT) {
				g_warning("invalid line direction %d at byte %d", direction, index);
				direction=MV_STILL;
			}
			object.x2=object.x1+length*gd_dx[direction+1];
			object.y2=object.y1+length*gd_dy[direction+1];
			if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2 >=cave->h)
				g_warning("invalid line coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
			index+=6;
			break;
		case 1:				/* OPEN RECTANGLE */
			object.type=RECTANGLE;
			object.element=bd1_import(data[index+1], index+1);
			object.y1=data[index+2];
			object.x1=data[index+3];
			object.y2=object.y1+data[index+4]-1;	/* height */
			object.x2=object.x1+data[index+5]-1;
			if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2 >=cave->h)
				g_warning("invalid rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
			index+=6;
			break;
		case 2:				/* FILLED RECTANGLE */
			object.type=FILLED_RECTANGLE;
			object.element=bd1_import(data[index+1], index+1);
			object.y1=data[index+2];
			object.x1=data[index+3];
			object.y2=object.y1+data[index+4]-1;
			object.x2=object.x1+data[index+5]-1;
			object.fill_element=bd1_import(data[index+6], index+6);
			if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2 >=cave->h)
				g_warning("invalid filled rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
			index+=7;
			break;
		case 3:				/* POINT */
			object.type=POINT;
			object.element=bd1_import(data[index+1], index+1);
			object.y1=data[index+2];
			object.x1=data[index+3];
			if (object.x1>=cave->w || object.y1>=cave->h)
				g_warning("invalid point coordinates %d,%d at byte %d", object.x1, object.y1, index);
			index+=4;
			break;
		case 4:				/* RASTER */
			object.type=RASTER;
			object.element=bd1_import(data[index+1], index+1);
			object.y1=data[index+2];
			object.x1=data[index+3];
			ny=data[index+4]-1;
			nx=data[index+5]-1;
			object.dy=data[index+6];
			object.dx=data[index+7];
			object.y2=object.y1+object.dy*ny;	/* calculate real */
			object.x2=object.x1+object.dx*nx;
			if (object.dy<1) object.dy=1;
			if (object.dx<1) object.dx=1;
			if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2 >=cave->h)
				g_warning("invalid raster coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
			index+=8;
			break;
		case 5:
			/* profi boulder extension: bitmap */
			object.element=bd1_import(data[index+1], index+1);
			bytes=data[index+2];	/* number of bytes in bitmap */
			if (bytes>=cave->w*cave->h/8)
				g_warning("invalid bitmap length at byte %d", index-4);
			addr=0;
			addr+=data[index+3];	/*msb */
			addr+=data[index+4] << 8;	/*lsb */
			addr-=0x0850;	/* this was a pointer to the cave work memory (used during game). */
			if (addr>=cave->w*cave->h)
				g_warning("invalid bitmap start address at byte %d", index-4);
			object.type=POINT;
			object.x1=addr%40;
			object.y1=addr/40;
			for (i=0; i<bytes; i++) {	/* for ("bytes" number of bytes) */
				val=data[index+5+i];
				for (n=0; n < 8; n++) {	/* for (8 bits in a byte) */
					if (val & 1)
						/* convert to single points... */
						cave->objects=g_list_append (cave->objects, g_memdup (&object, sizeof (GdObject)));
					val=val>>1;
					object.x1++;
					if (object.x1 >= cave->w) {
						object.x1=0;
						object.y1++;
					}
				}
			}
			index+=5+bytes;	/* 5 description bytes and "bytes" data bytes */
			object.type=NONE;	/* prevent appending object to list */
			break;
		case 6:				/* JOIN */
			object.type=JOIN;
			object.element=bd1_import(data[index+1], index+1);
			object.fill_element=bd1_import(data[index+2], index+2);
			object.dy=data[index+3]/40;
			object.dx=data[index+3]%40;	/* same byte!!! */
			index+=4;
			break;
		case 7:
			/* interesting this is set here, and not in the cave header */
			cave->slime_permeability_c64=data[index+1];
			index+=2;
			break;
		case 9:
			/* profi boulder extension by player: plck-like cave map. the import routine inserts it here. */
			if (cave->map!=NULL) {
				g_critical("contains more than one PLCK map");
				gd_cave_map_free(cave->map);
			}
			cave->map=gd_cave_map_new(cave, GdElement);
			for (x=0; x<cave->w; x++) {
				/* fill the first and the last row with steel wall. */
				cave->map[0][x]=O_STEEL;
				cave->map[cave->h-1][x]=O_STEEL;
			}
			n=0;	/* number of bytes read from map */
			for (y=1; y<cave->h-1; y++)	/* the first and the last rows are not stored. */
				for (x=0; x<cave->w; x+=2) {
					cave->map[y][x]=plck_nybble[data[index+3+n] >> 4];	/* msb 4 bits */
					cave->map[y][x+1]=plck_nybble[data[index+3+n] % 16];	/* lsb 4 bits */
					n++;
				}
			/* the position of inbox is stored. this is to check the cave */
			ry=data[index+1]-2;
			rx=data[index+2];
			/* at the start of the cave, bd scrolled to the last player placed during the drawing (setup) of the cave.
			   i think this is why a map also stored the coordinates of the player - we can use this to check its integrity */
			if (rx>=cave->w || ry<0 || ry>=cave->h || cave->map[ry][rx]!=O_INBOX)
				g_warning ("embedded PLCK map may be corrupted, player coordinates %d,%d", rx, rx);
			index+=3+n;
			break;
		default:
			g_warning ("unknown bd2 extension no. %02x at byte %d", data[index], index);
			index+=1;	/* skip that byte */
		}

		if (object.type!=NONE)
			cave->objects=g_list_append (cave->objects, g_memdup (&object, sizeof (GdObject)));
	}
	if (data[index]!=0xFF) {
		g_critical("import error, cave not delimited with 0xFF");
		return -1;
	}
	index++;	/* skip delimiter */
	index++;	/* animation byte - told the engine which objects to animate - to make game faster */

	/* the colors from the memory dump are appended here */
	cave->color0=gd_c64_colors[0].rgb;
	cave->color1=gd_c64_colors[data[index+0]&0xf].rgb;
	cave->color2=gd_c64_colors[data[index+1]&0xf].rgb;
	cave->color3=gd_c64_colors[data[index+2]&0x7].rgb; 	/* lower 3 bits only! */
	cave->color4=cave->color1;	/* in bd2, amoeba was color1 */
	cave->color5=cave->color1;	/* slime too */
	index+=3;
	
	return index;
}

/* import plck cave data into our format.
	length is always 512 bytes, and contains if it is an intermission cave. */
static int
cave_copy_from_plck (Cave *cave, const guint8 *data, int remaining_bytes)
{
	/* i don't really think that all this table is needed, but included to be complete. */
	/* this is for the dirt and expanding wall looks like effect. */
	/* it also contains the individual frames */
	static GdElement plck_graphic_table[]={
		/* 3000 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
		/* 3100 */ O_BUTTER_1, O_MAGIC_WALL, O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4, O_PRE_DIA_5, O_OUTBOX_CLOSED,
		/* 3200 */ O_AMOEBA, O_VOODOO, O_STONE, O_DIRT, O_DIAMOND, O_STEEL, O_PLAYER, O_BRICK,
		/* 3300 */ O_SPACE, O_OUTBOX_OPEN, O_GUARD_1, O_EXPLODE_1, O_EXPLODE_2, O_EXPLODE_3, O_MAGIC_WALL, O_MAGIC_WALL, 
		/* 3400 */ O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, 
		/* 3500 */ O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, 
		/* 3600 */ O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, 
		/* 3700 */ O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, 
		/* 3800 */ O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, 
	};
		
	int i;
	int x, y;
	
	gd_error_set_context(cave->name);
	if (remaining_bytes<512) {
		g_critical("truncated plck cave data!");
		return -1;
	}

	gd_cave_set_plck_defaults(cave);

	cave->intermission=data[0x1da]!=0;
	if (cave->intermission) {	/* set visible size for intermission */
		cave->x2=19;
		cave->y2=11;
	}

	/* cave selection table, was not part of cave data, rather given in game packers.
	 * if a new enough version of any2gdash is used, it will put information after the cave.
	 * detect this here and act accordingly */
	if ((data[0x1f0]==data[0x1f1]-1) && (data[0x1f0]==0x19 || data[0x1f0]==0x0e)) {
		int j;
		
		/* found selection table */
		cave->selectable=data[0x1f0]==0x19;
		g_strlcpy(cave->name, "              ", sizeof(cave->name));
		for (j=0; j<12; j++)
			cave->name[j]=data[0x1f2+j];
		g_strchomp(cave->name);	/* remove spaces */
	} else
		/* no selection info found, let intermissions be unselectable */
		cave->selectable=!cave->intermission;

	cave->diamond_value=data[0x1be];
	cave->extra_diamond_value=data[0x1c0];
	for (i=0; i < 5; i++) {
		/* plck doesnot really have levels, so just duplicate data five times */
		cave->level_time[i]=data[0x1ba];
		cave->level_diamonds[i]=data[0x1bc];
		if (cave->level_diamonds[i]==0)		/* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 needed */
			cave->level_diamonds[i]=1000;
		cave->level_ckdelay[i]=data[0x1b8];
	}

	cave->colorb=gd_c64_colors[data[0x1db]&0xf].rgb;	/* border */
	cave->color0=gd_c64_colors[data[0x1dd]&0xf].rgb;
	cave->color1=gd_c64_colors[data[0x1df]&0xf].rgb;
	cave->color2=gd_c64_colors[data[0x1e1]&0xf].rgb;
	cave->color3=gd_c64_colors[data[0x1e3]&0x7].rgb; 	/* lower 3 bits only! */
	cave->color4=cave->color3;	/* in plck, amoeba was color3 */
	cave->color5=cave->color3;	/* same for slime */

	cave->magic_wall_milling_time=data[0x1c6];
	cave->amoeba_slow_growth_time=data[0x1c4];
	cave->slime_permeability_c64=0;
	for (i=0; i < data[0x1c2]; i++)
		/* shift in this many msb 1's */
		cave->slime_permeability_c64=(0x100 | cave->slime_permeability_c64) >> 1;

	/* ... the cave is stored like a map. */
	cave->map=gd_cave_map_new(cave, GdElement);
	for (x=0; x < cave->w; x++) {
		/* fill the first and the last row with steel wall. */
		cave->map[0][x]=O_STEEL;
		cave->map[cave->h-1][x]=O_STEEL;
	}
	for (y=1; y < cave->h-1; y++)	/* two rows of steel wall. the second one is stored */
		for (x=0; x < cave->w; x += 2) {
			cave->map[y][x]=plck_nybble[data[(y+1)*20 + x/2] >> 4];	/* msb 4 bits: we do not check index ranges, as >>4 and %16 will result in 0..15 */
			cave->map[y][x+1]=plck_nybble[data[(y+1)*20 + x/2] % 16];	/* lsb 4 bits */
		}

	/* check for diego-effects */
	if ((data[0x1e5]==0x20 && data[0x1e6]==0x90 && data[0x1e7]==0x46) || (data[0x1e5]==0xa9 && data[0x1e6]==0x1c && data[0x1e7]==0x85)) {
		/* diego effects enabled. */
		cave->bouncing_stone_to=bd1_import(data[0x1ea], 0x1ea);
		cave->falling_diamond_to=bd1_import(data[0x1eb], 0x1eb);
		/* explosions: 0x1e was explosion 5, if this is set to default, we also do not read it,
		  as in our engine this would cause an O_EXPLODE_5 to stay there. */
		if (data[0x1ec]!=0x1e)
			cave->explosion_to=bd1_import(data[0x1ec], 0x1ec);
		/* pointer to element graphic.
		   two bytes/column (one element), that is data[xxx]%16/2.
		   also there are 16bytes/row.
		   that is, 0x44=stone, upper left character. 0x45=upper right, 0x54=lower right, 0x55=lower right.
		   so high nybble must be shifted right twice -> data[xxx]/16*4. */
		cave->dirt_looks_like=plck_graphic_table[(data[0x1ed]/16)*4 + (data[0x1ed]%16)/2];
		cave->expanding_wall_looks_like=plck_graphic_table[(data[0x1ee]/16)*4 + (data[0x1ee]%16)/2];
		cave->amoeba_threshold=data[0x1ef];
	}
	
	return 512;
}

/* import atg cave data into our format. */
static int
cave_copy_from_atg (Cave *cave, const guint8 *data, int remaining_bytes)
{
	int i, datapos;
	int x, y;


	/* cave name at start */
	g_strlcpy(cave->name, (char *)data, sizeof(cave->name));
	/* delete trailing spaces */
	g_strchomp (cave->name);
	gd_error_set_context(cave->name);

	cave->intermission=data[0x10];
	gd_cave_set_bd1_defaults(cave);
	cave->magic_wall_stops_amoeba=FALSE;	/* as in plc - i guess the game did not have the same bug as c64 bd1 */
	/* some cave data is stored after the cave map. and the map can have different sizes (but the size is fixed) */
	datapos=cave->intermission?0x7f:0x1b5;
	if (datapos+8>=remaining_bytes+1) {
		g_critical("truncated atg cave data");
		return -1;
	}

	cave->w=cave->intermission?20:40;
	cave->h=cave->intermission?12:22;
	if (cave->intermission) {
		cave->x2=19;
		cave->y2=11;
	}

	for (i=0; i < 5; i++) {
		/* doesnot really have levels, so just duplicate data */
		cave->level_time[i]=data[datapos+0];
		cave->level_diamonds[i]=data[datapos+1];
	}
	cave->diamond_value=data[datapos+2];
	cave->extra_diamond_value=data[datapos+3];
	for (i=0; i < data[datapos+4]; i++)
		/* shift in this many msb 1's */
		cave->slime_permeability_c64=(0x100 | cave->slime_permeability_c64) >> 1;
	/* datapos+5 is always 127 in the files, i assume it has no meaning */

	cave->amoeba_slow_growth_time=data[datapos+6];
	cave->magic_wall_milling_time=data[datapos+7];
	
	/* ... the cave is stored like a map. */
	cave->map=gd_cave_map_new (cave, GdElement);
	for (x=0; x<cave->w; x++) {
		/* fill the first and the last row with steel wall. */
		cave->map[0][x]=O_STEEL;
		cave->map[cave->h-1][x]=O_STEEL;
	}
	for (y=1; y<cave->h-1; y++)
		for (x=0; x<cave->w; x+=2) {
			cave->map[y][x]=plck_nybble[data[17+(y*cave->w+x)/2] >> 4];	/* msb 4 bits */
			cave->map[y][x+1]=plck_nybble[data[17+(y*cave->w+x)/2] % 16];	/* lsb 4 bits */
		}

	return datapos+8;
}

/* no one's delight boulder dash
	essentially: rle compressed plck maps. */
static int
cave_copy_from_dlb (Cave *cave, const guint8 *data, int remaining_bytes)
{
	guint8 decomp[512];
	enum {
		START,	/* initial state */
		SEPARATOR,	/* got a separator */
		RLE,	/* after a separator, got the byte to duplicate */
		NORMAL	/* normal, copy bytes till separator */
	} state;
	int pos, cavepos, i, x, y;
	guint8 byte, separator;

	gd_error_set_context(cave->name);
	gd_cave_set_plck_defaults(cave);	/* essentially the plck engine */

	for (i=0; i<5; i++) {
		/* does not really have levels, so just duplicate data five times */
		cave->level_time[i]=data[1];
		cave->level_diamonds[i]=data[2];
		if (cave->level_diamonds[i]==0)		/* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 needed */
			cave->level_diamonds[i]=1000;
		cave->level_ckdelay[i]=data[0];
	}
	cave->diamond_value=data[3];
	cave->extra_diamond_value=data[4];
	cave->slime_permeability_c64=0;
	for (i=0; i < data[5]; i++)
		/* shift in this many msb 1's */
		cave->slime_permeability_c64=(0x100 | cave->slime_permeability_c64) >> 1;
	cave->amoeba_slow_growth_time=data[6];	cave->magic_wall_milling_time=data[7];

	/* then 5 color bytes follow */
	cave->colorb=gd_c64_colors[data[8]&0xf].rgb;	/* border */
	cave->color0=gd_c64_colors[data[9]&0xf].rgb;
	cave->color1=gd_c64_colors[data[10]&0xf].rgb;
	cave->color2=gd_c64_colors[data[11]&0xf].rgb;
	cave->color3=gd_c64_colors[data[12]&0x7].rgb; 	/* lower 3 bits only! */
	cave->color4=cave->color3;	/* in plck, amoeba was color3 */
	cave->color5=cave->color3;	/* same for slime */

	/* cave map */
	pos=13;	/* those 13 bytes were the cave values above */
	cavepos=0;
	byte=0;					/* just to get rid of compiler warning */
	/* employ a state machine. */
	state=START;
	while (cavepos<400 && pos<remaining_bytes) {
		switch (state) {
		case START:
			/* first byte is a separator. remember it */
			separator=data[pos];
			/* after the first separator, no rle data, just copy. */
			state=NORMAL;
			break;
		case SEPARATOR:
			/* we had a separator. remember this byte, as this will be duplicated (or more) */
			byte=data[pos];
			state=RLE;
			break;
		case RLE:
			/* we had the first byte, duplicate this n times. */
			if (data[pos]==0xff) {
				/* if it is a 0xff, we will have another byte, which is also a length specifier. */
				/* and for this one, duplicate only 254 times */
				if (cavepos+254>400) {
					g_critical("DLB import error: RLE data overflows buffer");
					return -1;
				}
				for (i=0; i<254; i++)
					decomp[cavepos++]=byte;
			} else {
				/* if not 0xff, duplicate n times and back to copy mode */
				if (cavepos+data[pos]>400) {
					g_critical("DLB import error: RLE data overflows buffer");
					return -1;
				}
				for (i=0; i<data[pos]; i++)
					decomp[cavepos++]=byte;
				state=NORMAL;
			}
			break;
		case NORMAL:
			/* bytes duplicated; now only copy the remaining, till the next separator. */
			if (data[pos]==separator)
				state=SEPARATOR;
			else
				decomp[cavepos++]=data[pos];	/* copy this byte and state is still NORMAL */
			break;
		}
		pos++;
	}
	if (cavepos!=400) {
		g_critical("DLB import error: RLE processing, cave length %d, should be 400", cavepos);
		return -1;
	}

	/* process uncompressed map */
	cave->map=gd_cave_map_new (cave, GdElement);
	for (x=0; x<cave->w; x++) {
		/* fill the first and the last row with steel wall. */
		cave->map[0][x]=O_STEEL;
		cave->map[cave->h-1][x]=O_STEEL;
	}
	for (y=1; y<cave->h-1; y++)
		for (x=0; x<cave->w; x+=2) {
			cave->map[y][x]=plck_nybble[decomp[((y-1)*cave->w+x)/2] >> 4];	/* msb 4 bits */
			cave->map[y][x+1]=plck_nybble[decomp[((y-1)*cave->w+x)/2] % 16];	/* lsb 4 bits */
		}

	/* return number of bytes read from buffer */
	return pos;
}



/* import plck cave data into our format. */
static int
cave_copy_from_1stb (Cave *cave, const guint8 *data, int remaining_bytes)
{
	int i;
	int x, y;
	
	gd_error_set_context(cave->name);
	if (remaining_bytes<1024) {
		g_critical("truncated 1stb cave data!");
		return -1;
	}

	gd_cave_set_1stb_defaults(cave);

	/* copy name */
	g_strlcpy(cave->name, "              ", sizeof(cave->name));
	for (i=0; i<14; i++) {
		int c=data[0x3a0+i];
		
		/* import cave name; a conversion table is used for each character */
		if (c<0x40)
			c=gd_bd_internal_chars[c];
		else if (c==0x74)
			c=' ';
		else if (c==0x76)
			c='?';
		else
			c=' ';	/* don't know this, so change to space */
		if (i>0)
			c=g_ascii_tolower(c);
		
		cave->name[i]=c;
	}
	g_strchomp(cave->name);

	cave->intermission=data[0x389]!=0;
	/* if it is intermission but not scrollable */
	if (cave->intermission && !data[0x38c]) {
		cave->x2=19;
		cave->y2=11;
	}
		
	cave->diamond_value=100*data[0x379] + 10*data[0x379+1] + data[0x379+2];
	cave->extra_diamond_value=100*data[0x376] + 10*data[0x376+1] + data[0x376+2];
	for (i=0; i < 5; i++) {
		/* plck doesnot really have levels, so just duplicate data five times */
		cave->level_time[i]=100*data[0x370] + 10*data[0x370+1] + data[0x370+2];
		if (cave->level_time[i]==0)		/* same as gate opening after 0 diamonds */
			cave->level_time[i]=1000;
		cave->level_diamonds[i]=100*data[0x373] + 10*data[0x373+1] + data[0x373+2];
		if (cave->level_diamonds[i]==0)		/* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 (!) needed */
			cave->level_diamonds[i]=1000;
		cave->level_ckdelay[i]=data[0x38a];
	}
	/* also has no random data... */

	cave->colorb=gd_c64_colors[data[0x384]&0xf].rgb;	/* border */
	cave->color0=gd_c64_colors[data[0x385]&0xf].rgb;
	cave->color1=gd_c64_colors[data[0x386]&0xf].rgb;
	cave->color2=gd_c64_colors[data[0x387]&0xf].rgb;
	cave->color3=gd_c64_colors[data[0x388]&0x7].rgb; 	/* lower 3 bits only! */
	cave->color4=cave->color1;
	cave->color5=cave->color1;

	cave->magic_wall_milling_time=256*(int)data[0x37e]+data[0x37f];
	cave->amoeba_slow_growth_time=256*(int)data[0x37c]+data[0x37d];
	cave->slime_permeability_c64=data[0x38b];
	cave->amoeba_growth_prob=4.0/(data[0x382]+1);
	if (cave->amoeba_growth_prob>1)
		cave->amoeba_growth_prob=1;
	cave->amoeba_fast_growth_prob=4.0/(data[0x383]+1);
	if (cave->amoeba_fast_growth_prob>1)
		cave->amoeba_fast_growth_prob=1;
	
	if (data[0x380]!=0)
		cave->creatures_direction_auto_change_time=data[0x381];
	else
		cave->diagonal_movements=data[0x381]!=0;

	/* ... the cave is stored like a map. */
	cave->map=gd_cave_map_new (cave, GdElement);
	for (y=0; y < cave->h; y++)
		for (x=0; x < cave->w; x++)
			cave->map[y][x]=firstboulder_import(data[y*40+x], y*40+x);

	cave->magic_wall_sound=data[0x38d]==0xf1;
	/* 2d was a normal switch, 2e a changed one. */
	cave->creatures_backwards=data[0x38f]==0x2d;
	/* 2e horizontal, 2f vertical. we implement this by changing them */
	if (data[0x38e]==0x2f)
		for (y=0; y<cave->h; y++)
			for (x=0; x<cave->w; x++) {
				if (cave->map[y][x]==O_H_GROWING_WALL)
					cave->map[y][x]=O_V_GROWING_WALL;
			}

	cave->amoeba_threshold=256*(int)data[0x390] + data[0x390+1];
	cave->bonus_time=data[0x392];
	cave->penalty_time=data[0x393];
	cave->biter_delay_frame=data[0x394];
	cave->magic_wall_stops_amoeba=data[0x395]==0;	/* negated!! */
	cave->bomb_explode_to=firstboulder_import(data[0x396], 0x396);
	cave->explosion_to=firstboulder_import(data[0x397], 0x397);
	cave->bouncing_stone_to=firstboulder_import(data[0x398], 0x398);
	cave->diamond_birth_to=firstboulder_import(data[0x399], 0x399);
	cave->magic_diamond_to=firstboulder_import(data[0x39a], 0x39a);

	cave->bladder_converts_by=firstboulder_import(data[0x39b], 0x39b);
	cave->falling_diamond_to=firstboulder_import(data[0x39c], 0x39c);
	cave->biter_eat=firstboulder_import(data[0x39d], 0x39d);
	cave->slime_eats_1=firstboulder_import(data[0x39e], 0x39e);
	cave->slime_converts_1=firstboulder_import(data[0x39e]+3, 0x39e);
	cave->slime_eats_2=firstboulder_import(data[0x39f], 0x39f);
	cave->slime_converts_2=firstboulder_import(data[0x39f]+3, 0x39f);
	cave->magic_diamond_to=firstboulder_import(data[0x39a], 0x39a);
	
	/* length is always 1024 bytes */
	return 1024;
}


/* crazy dream */
static int
cave_copy_from_crdr (Cave *cave, const guint8 *data, int remaining_bytes)
{
	int i, index;
	guint8 checksum;
	GdObject object;
	gboolean growing_wall_dir_change;
	
	/* if we have name, convert */
	g_strlcpy(cave->name, "              ", sizeof(cave->name));
	for (i=0; i<14; i++) {
		int c=data[i];
		
		/* import cave name; a conversion table is used for each character */
		if (c<0x40)
			c=gd_bd_internal_chars[c];
		else if (c==0x74)
			c=' ';
		else if (c==0x76)
			c='?';
		else
			c=' ';
		if (i>0)
			c=g_ascii_tolower(c);
		
		cave->name[i]=c;
	}
	g_strchomp(cave->name);	/* remove trailing and leading spaces */
	cave->selectable=data[14]!=0;
	
	/* jump 15 bytes, 14 was the name and 15 selectability */
	data+=15;
	if (strncmp((char *)data+0x30, "V4\0020", 4)!=0)
		g_warning("unknown crdr version %c%c%c%c", data[0x30], data[0x31], data[0x32], data[0x33]);

	gd_error_set_context(cave->name);
	gd_cave_set_crdr_defaults(cave);
	
	for (i=0; i<5; i++) {
		cave->level_time[i]=(int)data[0x0]*100 + data[0x1]*10 + data[0x2];
		if (cave->level_time[i]==0)		/* same as gate opening after 0 diamonds */
			cave->level_time[i]=1000;
		cave->level_diamonds[i]=(int)data[0x3]*100+data[0x4]*10+data[0x5];
		if (cave->level_diamonds[i]==0)		/* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 (!) needed */
			cave->level_diamonds[i]=1000;
		cave->level_ckdelay[i]=data[0x1A];
		cave->level_rand[i]=data[0x40];
	}
	cave->extra_diamond_value=(int)data[0x6] * 100 + data[0x7] * 10 + data[0x8];
	cave->diamond_value=(int)data[0x9] * 100 + data[0xA] * 10 + data[0xB];
	cave->amoeba_slow_growth_time=(int)data[0xC] * 256 + data[0xD];
	cave->magic_wall_milling_time=(int)data[0xE] * 256 + data[0xF];
	if (data[0x10])
		cave->creatures_direction_auto_change_time=data[0x11];
	cave->colorb=gd_c64_colors[data[0x14]&0xf].rgb;	/* border */
	cave->color0=gd_c64_colors[data[0x15]&0xf].rgb;
	cave->color1=gd_c64_colors[data[0x16]&0xf].rgb;
	cave->color2=gd_c64_colors[data[0x17]&0xf].rgb;
	cave->color3=gd_c64_colors[data[0x18]&0x7].rgb; 	/* lower 3 bits only! */
	cave->color4=cave->color3;
	cave->color5=cave->color1;
	cave->intermission=data[0x19]!=0;
	/* if it is intermission but not scrollable */
	if (cave->intermission && !data[0x1c]) {
		cave->x2=19;
		cave->y2=11;
	}
	cave->slime_permeability_c64=data[0x1B];

	/* AMOEBA in crazy dash 8:
		jsr $2500		; generate true random
		and $94			; binary and the current "probability"
		cmp #$04		; compare to 4
		bcs out			; jump out (do not expand) if carry set, ie. result was less than 4.
		
		prob values can be like num=3, 7, 15, 31, 63, ... n lsb bits count.
		0..3>=4?  0..7>=4?  0..15>=4? and similar.
		this way, probability of growing is 4/(num+1)
	*/
	cave->amoeba_growth_prob=4.0/(data[0x12]+1);
	if (cave->amoeba_growth_prob>1)
		cave->amoeba_growth_prob=1;
	cave->amoeba_fast_growth_prob=4.0/(data[0x13]+1);
	if (cave->amoeba_fast_growth_prob>1)
		cave->amoeba_fast_growth_prob=1;
	/* growing wall direction change */
	/* 2e horizontal, 2f vertical */
	growing_wall_dir_change=data[0x1e]==0x2f;
	/* 2c was a normal switch, 2d a changed one. */
	cave->creatures_backwards=data[0x1f]==0x2d;
	cave->amoeba_threshold=256*(int)data[0x20]+data[0x21];
	cave->bonus_time=data[0x22];
	cave->penalty_time=data[0x23];
	cave->biter_delay_frame=data[0x24];
	cave->magic_wall_stops_amoeba=data[0x25]==0;	/* negated!! */
	cave->bomb_explode_to=crazydream_import_table[data[0x26]];
	cave->explosion_to=crazydream_import_table[data[0x27]];
	cave->bouncing_stone_to=crazydream_import_table[data[0x28]];
	cave->diamond_birth_to=crazydream_import_table[data[0x29]];
	cave->magic_diamond_to=crazydream_import_table[data[0x2a]];

	cave->bladder_converts_by=crazydream_import_table[data[0x2b]];
	cave->falling_diamond_to=crazydream_import_table[data[0x2c]];
	cave->biter_eat=crazydream_import_table[data[0x2d]];
	cave->slime_eats_1=crazydream_import_table[data[0x2e]];
	cave->slime_converts_1=crazydream_import_table[data[0x2e]+3];
	cave->slime_eats_2=crazydream_import_table[data[0x2f]];
	cave->slime_converts_2=crazydream_import_table[data[0x2f]+3];

	cave->diagonal_movements=(data[0x34]&1)!=0;
	cave->gravity_change_time=data[0x35];
	cave->pneumatic_hammer_frame=data[0x36];
	cave->hammered_wall_reappear_frame=data[0x37];
	cave->hammered_walls_reappear=data[0x3f]!=0;
	/*
		acid in crazy dream 8:
		jsr $2500	; true random
		cmp	$03a8	; compare to ratio
		bcs out		; if it was smaller, forget it for now.
		
		ie. random<=ratio, then acid grows.
	*/
	cave->acid_spread_ratio=data[0x38]/255.0;
	cave->acid_eats_this=crazydream_import_table[data[0x39]];
	switch(data[0x3a]&3) {
		case 0:
			cave->gravity=MV_UP; break;
		case 1:
			cave->gravity=MV_DOWN; break;
		case 2:
			cave->gravity=MV_LEFT; break;
		case 3:
			cave->gravity=MV_RIGHT; break;
	}
	cave->snap_explosions=(data[0x3a]&4)!=0;
	/* we do not know the values for these, so do not import */
	//	cave->dirt_looks_like... data[0x3c]
	//	cave->expanding_wall_looks_like... data[0x3b]
	for (i=0; i<4; i++) {
		cave->random_fill[i]=crazydream_import_table[data[0x41+i]];
		cave->random_fill_probability[i]=data[0x45+i];
	}
	
	data+=0x49;
	index=0;
	object.levels=GD_OBJECT_LEVEL_ALL;
	while (data[index]!=0xff) {
		int nx, ny;
		int length, direction;
		static int cx1, cy1, cw, ch;	/* for copy&paste */

		switch(data[index]) {
			case 1:	/* point */
				object.type=POINT;
				object.element=crazydream_import_table[data[index+1]];
				object.x1=data[index+2];
				object.y1=data[index+3];
				if (object.x1>=cave->w || object.y1>=cave->h)
					g_warning("invalid point coordinates %d,%d at byte %d", object.x1, object.y1, index);
				index+=4;
				break;
			case 2: /* rectangle */
				object.type=RECTANGLE;
				object.element=crazydream_import_table[data[index+1]];
				object.x1=data[index+2];
				object.y1=data[index+3];
				object.x2=object.x1+data[index+4]-1;
				object.y2=object.y1+data[index+5]-1;	/* height */
				if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2 >=cave->h)
					g_warning("invalid rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
				index+=6;
				break;
			case 3: /* fillrect */
				object.type=FILLED_RECTANGLE;
				object.element=crazydream_import_table[data[index+1]];
				object.fill_element=crazydream_import_table[data[index+1]];
				object.x1=data[index+2];
				object.y1=data[index+3];
				object.x2=object.x1+data[index+4]-1;
				object.y2=object.y1+data[index+5]-1;
				if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2 >=cave->h)
					g_warning("invalid filled rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
				index+=6;
				break;

			case 4: /* line */
				object.type=LINE;
				object.element=crazydream_import_table[data[index+1]];
				if(object.element==O_UNKNOWN)
					g_warning("%x", data[index+1]);
				object.x1=data[index+2];
				object.y1=data[index+3];
				length=data[index+4];

				direction=data[index+5];
				nx=((int)direction-128)%40;
				ny=((int)direction-128)/40;
				object.x2=object.x1+(length-1)*nx;
				object.y2=object.y1+(length-1)*ny;
				/* if either is bigger than one, we cannot treat this as a line. create points instead */
				if (ABS(nx)>=2 || ABS(ny>=2)) {
					object.type=POINT;
					for (i=0; i<length; i++) {
						cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(GdObject)));
						object.x1+=nx;
						object.y1+=ny;
					}
					object.type=NONE;	/* signal that no more list_appends needed */
				} else {
					/* this is a normal line, and will be appended. only do the checking here */
					if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2>=cave->h)
						g_warning("invalid line coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index-5);
				}

				index+=6;
				break;
			case 6: /* copy */
				cx1=data[index+1];
				cy1=data[index+2];
				cw=data[index+3];
				ch=data[index+4];
				object.type=NONE;
				if (cx1>=cave->w || cy1>=cave->h || cx1+cw>cave->w || cy1+ch>cave->h)
					g_warning("invalid copy coordinates %d,%d or size %d,%d at byte %d", cx1, cy1, cw, ch, index);
				index+=5;
				break;
			case 7: /* paste */
				gd_flatten_cave(cave, 0);	/* flatten cave, so a map is available. flatten at level 1 - level is not important */
				for (ny=0; ny<ch; ny++)
					for (nx=0; nx<cw; nx++)
						cave->map[data[index+2]+ny][data[index+1]+nx]=cave->map[cy1+ny][cx1+nx];
				object.type=NONE;
				index+=3;
				break;
			case 11: /* raster */
				object.type=RASTER;
				object.element=crazydream_import_table[data[index+1]];
				object.x1=data[index+2];
				object.y1=data[index+3];
				object.dx=data[index+4];
				object.dy=data[index+5];
				nx=data[index+6]-1;
				ny=data[index+7]-1;
				object.y2=object.y1+object.dy*ny;	/* calculate real */
				object.x2=object.x1+object.dx*nx;
				if (object.dy<1) object.dy=1;
				if (object.dx<1) object.dx=1;
				if (object.x1>=cave->w || object.y1>=cave->h || object.x2>=cave->w || object.y2 >=cave->h)
					g_warning("invalid raster coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
				index+=8;
				break;
			default:
				object.type=NONE;
				g_warning ("unknown crdr extension no. %02x at byte %d", data[index], index);
				index+=1;	/* skip that byte */
				break;
		}
		if (object.type!=NONE)
			cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(GdObject)));
	}
	index++;	/* skip $ff */

	/* growing wall direction change */
	if (growing_wall_dir_change) {
		GList *iter;
		int x, y;
		
		if (cave->map)
			for (y=0; y<cave->h; y++)
				for (x=0; x<cave->w; x++) {
					if (cave->map[y][x]==O_H_GROWING_WALL)
						cave->map[y][x]=O_V_GROWING_WALL;
				}
				
		for (iter=cave->objects; iter!=NULL; iter=iter->next) {
			GdObject *object=(GdObject *)iter->data;

			if (object->element==O_H_GROWING_WALL)
				object->element=O_V_GROWING_WALL;
			if (object->fill_element==O_H_GROWING_WALL)
				object->fill_element=O_V_GROWING_WALL;
		}
	}

	/* crazy dream 7 hack */	
	checksum=0;
	for (i=0; i<0x3b0; i++)
		checksum=checksum ^ data[i];

	if (g_str_equal(cave->name, "Crazy maze") && checksum==195)
		cave->skeletons_needed_for_pot=0;

	return 15+0x49+index;
}



/* crazy light contruction kit */
static int
cave_copy_from_crli (Cave *cave, const guint8 *data, int remaining_bytes)
{
	guint8 uncompressed[1024];
	int datapos, cavepos, i, x, y;
	guint8 checksum;
	gboolean cavefile;
	const char *versions[]={"V2.2", "V2.6", "V3.0"};
	enum {
		none,
		V2_2,	/*XXX whats the difference between 2.2 and 2.6?*/
		V2_6,
		V3_0
	} version=none;
	GdElement (*import) (guint8 c, int i)=NULL;	/* import function */

	gd_cave_set_crli_defaults(cave);
	
	/* detect if this is a cavefile */
	if (data[0]==0 && data[1]==0xc4 && data[2] == 'D' && data[3] == 'L' && data[4] == 'P') {
		datapos=5;	/* cavefile, skipping 0x00 0xc4 D L P */
		cavefile=TRUE;
	}
	else {
		datapos=15;	/* converted from snapshot, skip "selectable" and 14byte name */
		cavefile=FALSE;
	}

	/* if we have name, convert */
	if (!cavefile) {
		g_strlcpy(cave->name, "              ", sizeof(cave->name));
		for (i=0; i<14; i++) {
			int c=data[i+1];
			
			/* import cave name; a conversion table is used for each character */
			if (c<0x40)
				c=gd_bd_internal_chars[c];
			else if (c==0x74)
				c=' ';
			else if (c==0x76)
				c='?';
			else
				c=' ';
			if (i>0)
				c=g_ascii_tolower(c);
			
			cave->name[i]=c;
		}
		g_strchomp(cave->name);	/* remove trailing and leading spaces */
	}
	gd_error_set_context(cave->name);

	/* uncompress rle data */
	cavepos=0;
	while (cavepos<0x3b0) {	/* <- loop until the uncompressed reaches its size */
		if (datapos>=remaining_bytes) {
			g_critical("truncated crli cave data");
			return -1;
		}
		if (data[datapos] == 0xbf) {	/* magic value 0xbf is the escape byte */
			if (datapos+2>=remaining_bytes) {
				g_critical("truncated crli cave data");
				return -1;
			}
			if(data[datapos+2]+datapos>=sizeof(uncompressed)) {
				/* we would run out of buffer, this must be some error */
				g_critical("invalid crli cave data - RLE length value is too big");
				return -1;
			}
			/* 0xbf, number, byte to dup */
			for (i=0; i<data[datapos+2]; i++)
				uncompressed[cavepos++]=data[datapos+1];
				
			datapos+=3;
		}
		else
			uncompressed[cavepos++]=data[datapos++];
	}

	/* check crli version */	
	for (i=0; i<G_N_ELEMENTS(versions); i++)
		if (strncmp((char *)uncompressed+0x3a0, versions[i], 4)==0)
			version=i+1;

	/* v3.0 has falling wall and box, and no ghost. */
	import= version>=V3_0 ? crazylight_import:firstboulder_import;

	if (version==none) {
		g_warning("unknown crli version %c%c%c%c", uncompressed[0x3a0], uncompressed[0x3a1], uncompressed[0x3a2], uncompressed[0x3a3]);
		import=crazylight_import;
	}

	/* process map */
	cave->map=gd_cave_map_new (cave, GdElement);
	for (y=0; y<cave->h; y++)
		for (x=0; x<cave->w; x++) {
			int index=y*cave->w+x;
			
			cave->map[y][x]=import(uncompressed[index], index);
		}

	/* crli has no levels */	
	for (i=0; i<5; i++) {
		cave->level_time[i]=(int)uncompressed[0x370] * 100 + uncompressed[0x371] * 10 + uncompressed[0x372];
		if (cave->level_time[i]==0)		/* same as gate opening after 0 diamonds */
			cave->level_time[i]=1000;
		cave->level_diamonds[i]=(int)uncompressed[0x373] * 100 + uncompressed[0x374] * 10 + uncompressed[0x375];
		if (cave->level_diamonds[i]==0)		/* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 (!) needed */
			cave->level_diamonds[i]=1000;
		cave->level_ckdelay[i]=uncompressed[0x38A];
	}
	cave->extra_diamond_value=(int)uncompressed[0x376] * 100 + uncompressed[0x377] * 10 + uncompressed[0x378];
	cave->diamond_value=(int)uncompressed[0x379] * 100 + uncompressed[0x37A] * 10 + uncompressed[0x37B];
	cave->amoeba_slow_growth_time=(int)uncompressed[0x37C] * 256 + uncompressed[0x37D];
	cave->magic_wall_milling_time=(int)uncompressed[0x37E] * 256 + uncompressed[0x37F];
	if (uncompressed[0x380])
		cave->creatures_direction_auto_change_time=uncompressed[0x381];
	cave->colorb=gd_c64_colors[uncompressed[0x384]&0xf].rgb;	/* border */
	cave->color0=gd_c64_colors[uncompressed[0x385]&0xf].rgb;
	cave->color1=gd_c64_colors[uncompressed[0x386]&0xf].rgb;
	cave->color2=gd_c64_colors[uncompressed[0x387]&0xf].rgb;
	cave->color3=gd_c64_colors[uncompressed[0x388]&0x7].rgb; 	/* lower 3 bits only! */
	cave->color4=cave->color3;
	cave->color5=cave->color1;
	cave->intermission=uncompressed[0x389]!=0;
	/* if it is intermission but not scrollable */
	if (cave->intermission && !uncompressed[0x38c]) {
		cave->x2=19;
		cave->y2=11;
	}
	cave->slime_permeability_c64=uncompressed[0x38B];

	/* AMOEBA in crazy dash 8:
		jsr $2500		; generate true random
		and $94			; binary and the current "probability"
		cmp #$04		; compare to 4
		bcs out			; jump out (do not expand) if carry set, ie. result was less than 4.
		
		prob values can be like num=3, 7, 15, 31, 63, ... n lsb bits count.
		0..3>=4?  0..7>=4?  0..15>=4? and similar.
		this way, probability of growing is 4/(num+1)
	*/
	cave->amoeba_growth_prob=4.0/(uncompressed[0x382]+1);
	if (cave->amoeba_growth_prob>1)
		cave->amoeba_growth_prob=1;
	cave->amoeba_fast_growth_prob=4.0/(uncompressed[0x383]+1);
	if (cave->amoeba_fast_growth_prob>1)
		cave->amoeba_fast_growth_prob=1;
	/* 2c was a normal switch, 2d a changed one. */
	cave->creatures_backwards=uncompressed[0x38f]==0x2d;
	cave->magic_wall_sound=uncompressed[0x38d]==0xf1;
	/* 2e horizontal, 2f vertical. we implement this by changing them */
	if (uncompressed[0x38e]==0x2f)
		for (y=0; y<cave->h; y++)
			for (x=0; x<cave->w; x++) {
				if (cave->map[y][x]==O_H_GROWING_WALL)
					cave->map[y][x]=O_V_GROWING_WALL;
			}
	cave->amoeba_threshold=256*(int)uncompressed[0x390]+uncompressed[0x390+1];
	cave->bonus_time=uncompressed[0x392];
	cave->penalty_time=uncompressed[0x393];
	cave->biter_delay_frame=uncompressed[0x394];
	cave->magic_wall_stops_amoeba=uncompressed[0x395]==0;	/* negated!! */
	cave->bomb_explode_to=import(uncompressed[0x396], 0x396);
	cave->explosion_to=import(uncompressed[0x397], 0x397);
	cave->bouncing_stone_to=import(uncompressed[0x398], 0x398);
	cave->diamond_birth_to=import(uncompressed[0x399], 0x399);
	cave->magic_diamond_to=import(uncompressed[0x39a], 0x39a);

	cave->bladder_converts_by=import(uncompressed[0x39b], 0x39b);
	cave->falling_diamond_to=import(uncompressed[0x39c], 0x39c);
	cave->biter_eat=import(uncompressed[0x39d], 0x39d);
	cave->slime_eats_1=import(uncompressed[0x39e], 0x39e);
	cave->slime_converts_1=import(uncompressed[0x39e]+3, 0x39e);
	cave->slime_eats_2=import(uncompressed[0x39f], 0x39f);
	cave->slime_converts_2=import(uncompressed[0x39f]+3, 0x39f);

	/* v3.0 has some new properties. */
	if (version>=V3_0) {
		cave->diagonal_movements=uncompressed[0x3a4]!=0;
		cave->too_big_amoeba_to=import(uncompressed[0x3a6], 0x3a6);
		cave->enclosed_amoeba_to=import(uncompressed[0x3a7], 0x3a7);
		/*
			acid in crazy dream 8:
			jsr $2500	; true random
			cmp	$03a8	; compare to ratio
			bcs out		; if it was smaller, forget it for now.
			
			ie. random<=ratio, then acid grows.
		*/
		cave->acid_spread_ratio=uncompressed[0x3a8]/255.0;
		cave->acid_eats_this=import(uncompressed[0x3a9], 0x3a9);
		cave->expanding_wall_looks_like=import(uncompressed[0x3ab], 0x3ab);
		cave->dirt_looks_like=import(uncompressed[0x3ac], 0x3ac);
	} else {
		/* version is <= 3.0, so this is a 1stb cave. */
		/* the only parameters, for which this matters, are these: */
		if (uncompressed[0x380]!=0)
			cave->creatures_direction_auto_change_time=uncompressed[0x381];
		else
			cave->diagonal_movements=uncompressed[0x381]!=0;
	}

	if (cavefile)
		cave->selectable=!cave->intermission;	/* best we can do */
	else
		cave->selectable=!data[0];	/* given by converter */


	/* crazy dream 9 hack */	
	checksum=0;
	for (i=0; i<0x3b0; i++)
		checksum=checksum ^ uncompressed[i];

	/* check cave name and the checksum. both are hardcoded here */	
	if (g_str_equal(cave->name, "Rockfall") && checksum==216) {
		int i;
		
		GdObject object;

		object.type=RANDOM_FILL;
		object.levels=GD_OBJECT_LEVEL_ALL;
		for (i=0; i<5; i++)
			object.seed[i]=-1;

		object.x1=0;
		object.y1=0;
		object.x2=39;
		object.y2=21;
		
		object.element=O_BLADDER_SPENDER;	/* element to replace */
		
		object.fill_element=O_DIRT;	/* initial fill */
		object.random_fill_probability[0]=37;
		object.random_fill[0]=O_DIAMOND;
		object.random_fill_probability[1]=32;
		object.random_fill[1]=O_STONE;
		object.random_fill_probability[2]=2;
		object.random_fill[2]=O_ACID;
		object.random_fill_probability[3]=0;
		object.random_fill[3]=O_DIRT;
		cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(object)));
	}
	
	if (g_str_equal(cave->name, "Roll dice now!") && checksum==21) {
		GdObject object;
		int i;

		object.type=RANDOM_FILL;
		object.levels=GD_OBJECT_LEVEL_ALL;
		for (i=0; i<5; i++)
			object.seed[i]=-1;

		object.x1=0;
		object.y1=0;
		object.x2=39;
		object.y2=21;
		
		object.element=O_BLADDER_SPENDER;	/* element to replace */
		
		object.fill_element=O_DIRT;	/* initial fill */
		object.random_fill_probability[0]=0x18;
		object.random_fill[0]=O_STONE;
		object.random_fill_probability[1]=0x08;
		object.random_fill[1]=O_BUTTER_3;
		object.random_fill_probability[2]=0;
		object.random_fill[2]=O_DIRT;
		object.random_fill_probability[3]=0;
		object.random_fill[3]=O_DIRT;
		cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(object)));
	}
	
	if (g_str_equal(cave->name, "Random maze") && checksum==226) {
		GdObject object;
		int i;
		
		object.type=MAZE;
		object.levels=GD_OBJECT_LEVEL_ALL;
		for (i=0; i<5; i++)
			object.seed[i]=-1;
		object.horiz=50;
		object.x1=1;
		object.y1=4;
		object.x2=35;
		object.y2=20;
		object.element=O_NONE;
		object.fill_element=O_SPACE;
		object.dx=1;
		object.dy=1;

		cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(object)));
	}

	if (g_str_equal(cave->name, "Metamorphosis") && checksum==229) {
		GdObject object;
		int i;
		
		object.type=MAZE;
		object.levels=GD_OBJECT_LEVEL_ALL;
		for (i=0; i<5; i++)
			object.seed[i]=-1;
		object.horiz=50;
		object.x1=4;
		object.y1=1;
		object.x2=38;
		object.y2=19;
		object.element=O_NONE;
		object.fill_element=O_BLADDER_SPENDER;
		object.dx=1;
		object.dy=3;

		cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(object)));

		/* replace object - x1,y1,x2,y2 stay :) */
		object.type=RANDOM_FILL;
		object.element=O_BLADDER_SPENDER;	/* element to replace */
		object.fill_element=O_DIRT;	/* initial fill */
		object.random_fill_probability[0]=0x18;
		object.random_fill[0]=O_STONE;
		object.random_fill_probability[1]=0;
		object.random_fill[1]=O_DIRT;
		object.random_fill_probability[2]=0;
		object.random_fill[2]=O_DIRT;
		object.random_fill_probability[3]=0;
		object.random_fill[3]=O_DIRT;
		cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(object)));

		cave->creatures_backwards=TRUE;	/* XXX why? */
	}
	
	if (g_str_equal(cave->name, "All the way") && checksum==212) {
		GdObject object;
		int i;
		
		object.type=MAZE_UNICURSAL;
		object.levels=GD_OBJECT_LEVEL_ALL;
		for (i=0; i<5; i++)
			object.seed[i]=-1;
		object.horiz=50;
		object.x1=1;
		object.y1=1;
		object.x2=35;
		object.y2=19;
		object.element=O_BRICK;
		object.fill_element=O_PRE_DIA_1;
		object.dx=1;
		object.dy=1;
		cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(object)));

		/* a point which "breaks" the unicursal maze */		
		object.type=POINT;
		object.element=O_BRICK;
		object.x1=35;
		object.y1=18;
		cave->objects=g_list_append(cave->objects, g_memdup(&object, sizeof(object)));
	}
	
	return datapos;
}


GdCavefileFormat
gd_caveset_imported_format(const guint8 *buf)
{
	const char *s_bd1="GDashBD1";
	const char *s_bd2="GDashBD2";
	const char *s_plc="GDashPLC";
	const char *s_dlb="GDashDLB";
	const char *s_atg="GDashATG";
	const char *s_crl="GDashCRL";
	const char *s_crd="GDashCRD";
	const char *s_1st="GDash1ST";

	if (strncmp((char *)buf, s_bd1, 8)==0)	/* 8 bytes in s_... */
		return BD1;
	if (strncmp((char *)buf, s_bd2, 8)==0)	/* 8 bytes in s_... */
		return BD2;
	if (strncmp((char *)buf, s_plc, 8)==0)	/* 8 bytes in s_... */
		return PLC;
	if (strncmp((char *)buf, s_dlb, 8)==0)	/* 8 bytes in s_... */
		return DLB;
	if (strncmp((char *)buf, s_crl, 8)==0)	/* 8 bytes in s_... */
		return CRLI;
	if (strncmp((char *)buf, s_crd, 8)==0)	/* 8 bytes in s_... */
		return CRDR;
	if (strncmp((char *)buf, s_atg, 8)==0)	/* 8 bytes in s_... */
		return ATG;
	if (strncmp((char *)buf, s_1st, 8)==0)	/* 8 bytes in s_... */
		return FIRSTB;

	return UNKNOWN;
}


/** Load caveset from memory buffer.
	Loads the caveset from a memory buffer.
	File type is given in arg2.
	@result GList *caves.
*/
GList *
gd_caveset_import_from_buffer (const guint8 *buf, gsize length)
{
	gboolean numbering;
	int cavenum, intermissionnum, num;
	int cavelength, bufp;
	GList *caveset=NULL, *iter;
	guint32 encodedlength;
	GdCavefileFormat format;
	
	if (length!=-1 && length<12) {
		g_warning("buffer too short to be a GDash datafile");
		return NULL;
	}
	encodedlength=GUINT32_FROM_LE(*((guint32 *)(buf+8)));
	if (length!=-1 && encodedlength!=length-12) {
		g_warning("file length and data size mismatch in GDash datafile");
		return NULL;
	}
	format=gd_caveset_imported_format(buf);
	if (format==UNKNOWN) {
		g_warning("buffer does not contain a GDash datafile");
		return NULL;
	}
	
	buf+=12;
	length=encodedlength;
	
	bufp=0;

	cavenum=0;
	while (bufp<length) {
		Cave *newcave;
		int insertpos=-1;	/* default is to append cave to caveset; g_list_insert appends when pos=-1 */
		
		newcave=gd_cave_new();

		switch (format) {
		case BD1:				/* boulder dash 1 */
		case BD2:				/* boulder dash 2 */
			newcave->selectable=(cavenum<16) && (cavenum%4 == 0);
			newcave->intermission=cavenum>15;
			if (newcave->intermission)
				g_snprintf(newcave->name, sizeof(newcave->name), _("Intermission %d"), cavenum-15);
			else
				g_snprintf(newcave->name, sizeof(newcave->name), _("Cave %c"), 'A'+cavenum);

			switch(format) {
				case BD1: cavelength=cave_copy_from_bd1 (newcave, buf+bufp, length-bufp); break;
				case BD2: cavelength=cave_copy_from_bd2 (newcave, buf+bufp, length-bufp); break;
				default:
					g_assert_not_reached();
			};

			/* original bd1 had level order ABCDEFGH... and then the last four were the intermissions.
			 * those should be inserted between D-E, H-I... caves. */
			if (cavenum>15)
				insertpos=(cavenum-15)*5-1;
			break;

		case FIRSTB:
			cavelength=cave_copy_from_1stb(newcave, buf+bufp, length-bufp);
			/* every fifth cave (4+1 intermission) is selectable. */
			newcave->selectable=cavenum%5==0;
			break;

		case PLC:				/* peter liepa construction kit */
			cavelength=cave_copy_from_plck (newcave, buf+bufp, length-bufp);
			break;

		case DLB:	/* no one's delight boulder dash, something like rle compressed plck caves */
			/* but there are 20 of them, as if it was a bd1 or bd2 game. also num%5=4 is intermission. */
			/* we have to set intermission flag on our own, as the file did not contain the info explicitly */
			newcave->intermission=(cavenum%5)==4;
			if (newcave->intermission) {	/* also set visible size */
				newcave->x2=19;
				newcave->y2=11;
			}
			newcave->selectable=cavenum % 5 == 0;	/* original selection scheme */
			if (newcave->intermission)
				g_snprintf(newcave->name, sizeof(newcave->name), _("Intermission %d"), cavenum/5+1);
			else
				g_snprintf(newcave->name, sizeof(newcave->name), _("Cave %c"), 'A'+(cavenum%5+cavenum/5*4));

			cavelength=cave_copy_from_dlb (newcave, buf+bufp, length-bufp);
			break;

		case ATG:				/* atari games, imported from boulder rush */
			cavelength=cave_copy_from_atg(newcave, buf+bufp, length-bufp);	/* this also sets name */
			newcave->selectable=cavenum%5 == 0;
			/* atg games have no color info - use the no1 color table so they are different */
			newcave->color1=gd_c64_colors[0].rgb;
			newcave->color1=gd_c64_colors[no1_default_colors[cavenum*3+0]].rgb;
			newcave->color2=gd_c64_colors[no1_default_colors[cavenum*3+1]].rgb;
			newcave->color3=gd_c64_colors[no1_default_colors[cavenum*3+2]].rgb;
			newcave->color4=newcave->color3;
			newcave->color5=newcave->color3;
			break;

		case CRLI:
			cavelength=cave_copy_from_crli (newcave, buf+bufp, length-bufp);
			break;

		case CRDR:
			cavelength=cave_copy_from_crdr (newcave, buf+bufp, length-bufp);
			break;

		default:
			g_assert_not_reached();
			break;
		}

		gd_error_set_context(NULL);
		if (cavelength==-1) {
			gd_cave_free(newcave);
			g_critical("Aborting cave import.");
			break;
		} else
			caveset=g_list_insert(caveset, newcave, insertpos);
		cavenum++;
		bufp+=cavelength;
		
		/* hack: some dlb files contain junk data after 20 caves. */
		if (format==DLB && cavenum==20) {
			if (bufp<length)
				g_warning("excess data in dlb file, %d bytes", (int)(length-bufp));
			break;
		}
	}

	/* try to detect if plc caves are in standard layout. */
	/* that is, caveset looks like an original, (4 cave,1 intermission)+ */
	if (format==PLC)
		/* if no selection table stored by any2gdash */
		if ((buf[2+0x1f0]!=buf[2+0x1f1]-1) || (buf[2+0x1f0]!=0x19 && buf[2+0x1f0]!=0x0e)) {
			GList *iter;
			int n;
			gboolean standard;
			
			standard=(g_list_length(caveset)%5)==0;	/* cave count % 5 != 0 -> nonstandard */
			
			for (n=0, iter=caveset; iter!=NULL; n++, iter=iter->next) {
				Cave *cave=iter->data;
				
				if ((n%5==4 && !cave->intermission) || (n%5!=4 && cave->intermission))
					standard=FALSE;	/* 4 cave, 1 intermission */
			}
			
			/* if test passed, update selectability */
			if (standard)
				for (n=0, iter=caveset; iter!=NULL; n++, iter=iter->next) {
					Cave *cave=iter->data;

					/* update "selectable" */
					cave->selectable=(n%5)==0;
				}
		}

	/* try to give some names for the caves */	
	cavenum=1; intermissionnum=1;
	num=1;
	/* use numbering instead of letters, if following formats or too many caves (as we would run out of letters) */
	numbering=format==PLC || format==CRLI || g_list_length(caveset)>26;
	for (iter=caveset; iter!=NULL; iter=iter->next) {
		Cave *cave=(Cave *)iter->data;
		
		if (!g_str_equal(cave->name, ""))	/* if it already has a name, skip */
			continue;
		
		if (cave->intermission) {
			/* intermission */
			if (numbering)
				g_snprintf(cave->name, sizeof(cave->name), _("Intermission %02d"), num);
			else
				g_snprintf(cave->name, sizeof(cave->name), _("Intermission %d"), intermissionnum);
		} else {
			if (numbering)
				g_snprintf(cave->name, sizeof(cave->name), _("Cave %02d"), num);
			else
				g_snprintf(cave->name, sizeof(cave->name), _("Cave %c"), 'A'-1+cavenum);
		}
		
		num++;
		if (cave->intermission)
			intermissionnum++;
		else
			cavenum++;
	}	
	
	gd_error_set_context(NULL);
	
	return caveset;
}



