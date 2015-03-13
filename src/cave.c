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
#include <stdio.h>
#include <stdlib.h>

#include "cave.h"
#include "caveset.h"
#include "caveobject.h"

/* arrays for movements */
/* also no1 and bd2 cave data import helpers; line direction coordinates */
const int gd_dx[]={ 0, 0, 1, 1, 1, 0, -1, -1, -1, 0, 2, 2, 2, 0, -2, -2, -2 };
const int gd_dy[]={ 0, -1, -1, 0, 1, 1, 1, 0, -1, -2, -2, 0, 2, 2, 2, 0, -2 };
const char* gd_direction_name[]={ N_("None"), N_("Up"), N_("Up+right"), N_("Right"), N_("Down+right"), N_("Down"), N_("Down+left"), N_("Left"), N_("Up+left") };
const char* gd_scheduling_name[]={ N_("Milliseconds"), N_("BD1"), N_("Construction Kit"), N_("Crazy Dream 7") };
const char* gd_scheduling_filename[]={ "ms", "bd1", "plck", "crdr7" };

const GdColor gd_flash_color=0xFFFFC0;
const GdColor gd_select_color=0x8080FF;

GDC64Color gd_c64_colors[16]= {
#if 0
	{ "Black", 0x000000 },
	{ "White", 0xfcfcfc },
	{ "Red", 0xa80000 },
	{ "Cyan", 0x54fcfc },
	{ "Purple", 0xa800a8 },
	{ "Green", 0x00a800 },
	{ "Blue", 0x0000a8 },
	{ "Yellow", 0xfcfc00 },
	{ "Orange", 0xa85400 },
	{ "Brown", 0x802c00 },
	{ "LightRed", 0xfc5454 },
	{ "Gray1", 0x545454 },
	{ "Gray2", 0x808080 },
	{ "LightGreen", 0x54fc54 },
	{ "LightBlue", 0x5454fc },
	{ "Gray3", 0xa8a8a8 }
#endif

    /* vice new luminances */
	{ "Black", 0x000000 },
	{ "White", 0xffffff },
	{ "Red", 0x894036 },
	{ "Cyan", 0x7abfc7 },
	{ "Purple", 0x8a46ae },
	{ "Green", 0x68a941 },
	{ "Blue", 0x3e31a2 },
	{ "Yellow", 0xd0dc71 },
	{ "Orange", 0x905f25 },
	{ "Brown", 0x5c4700 },
	{ "LightRed", 0xbb776d },
	{ "Gray1", 0x555555 },
	{ "Gray2", 0x808080 },
	{ "LightGreen", 0xacea88 },
	{ "LightBlue", 0x7c70da },
	{ "Gray3", 0xababab }
};


/* elements description array. do not miss an index!
	the game will check if one is missing and stop the game.
	the identifier in the saved file might also not match, reading an "outbox" from
	the file should store an O_PRE_OUTBOX.
	
   images are: image in editor, image in editor - animated, image pixbuf, game image
*/
GdElements gd_elements[] = {
	{O_SPACE, N_("Space"), P_AMOEBA_CONSUMES, "SPACE", ' ', 0, 0, 0},
	{O_DIRT, N_("Dirt"), P_AMOEBA_CONSUMES|P_VISUAL_EFFECT|P_DIRT, "DIRT", '.', 2, 2, 2},
	{O_DIRT_SLOPED_UP_RIGHT, N_("Sloped dirt (up & right)"), P_DIRT|P_SLOPED_UP|P_SLOPED_RIGHT|P_AMOEBA_CONSUMES, "DIRTSLOPEDUPRIGHT", 0, 280, 280, 280},
	{O_DIRT_SLOPED_UP_LEFT, N_("Sloped dirt (up & left)"), P_DIRT|P_SLOPED_UP|P_SLOPED_LEFT|P_AMOEBA_CONSUMES, "DIRTSLOPEDUPLEFT", 0, 281, 281, 281},
	{O_DIRT_SLOPED_DOWN_LEFT, N_("Sloped dirt (down & left)"), P_DIRT|P_SLOPED_DOWN|P_SLOPED_LEFT|P_AMOEBA_CONSUMES, "DIRTSLOPEDDOWNLEFT", 0, 282, 282, 282},
	{O_DIRT_SLOPED_DOWN_RIGHT, N_("Sloped dirt (down & right)"), P_DIRT|P_SLOPED_DOWN|P_SLOPED_RIGHT|P_AMOEBA_CONSUMES, "DIRTSLOPEDDOWNRIGHT", 0, 283, 283, 283},
	{O_DIRT2, N_("Dirt 2"), P_DIRT|P_AMOEBA_CONSUMES, "DIRT2", 0, 3, 3, 3},
	{O_BRICK, N_("Brick wall"), P_SLOPED|P_CAN_BE_HAMMERED, "WALL", 'w', 5, 5, 5},
	{O_BRICK_SLOPED_UP_RIGHT, N_("Brick wall (up & right)"), P_SLOPED_UP|P_SLOPED_RIGHT|P_CAN_BE_HAMMERED, "WALLSLOPEDUPRIGHT", 0, 276, 276, 276},
	{O_BRICK_SLOPED_UP_LEFT, N_("Brick wall (up & left)"), P_SLOPED_UP|P_SLOPED_LEFT|P_CAN_BE_HAMMERED, "WALLSLOPEDUPLEFT", 0, 277, 277, 277},
	{O_BRICK_SLOPED_DOWN_LEFT, N_("Brick wall (down & left)"), P_SLOPED_DOWN|P_SLOPED_LEFT|P_CAN_BE_HAMMERED, "WALLSLOPEDDOWNLEFT", 0, 278, 278, 278},
	{O_BRICK_SLOPED_DOWN_RIGHT, N_("Brick wall (down & right)"), P_SLOPED_DOWN|P_SLOPED_RIGHT|P_CAN_BE_HAMMERED, "WALLSLOPEDDOWNRIGHT", 0, 279, 279, 279},
	{O_MAGIC_WALL, N_("Magic wall"), 0, "MAGICWALL", 'M', 184, -184, -184},
	{O_PRE_OUTBOX, N_("Outbox"), 0, "OUTBOX", 'X', 351, -291, 22},	/* 291, 292, 293, 294, 295, 296, 297, 298 */
	{O_OUTBOX, N_("Outbox (open)"), 0, "OUTBOXopen", 0, 299, 299, 22},
	{O_PRE_INVIS_OUTBOX, N_("Invisible outbox"), 0, "HIDDENOUTBOX", 'H', 288, 288, 22},
	{O_INVIS_OUTBOX, N_("Invisible outbox (open)"), 0, "HIDDENOUTBOXopen", 0, 300, 300, 22},
	{O_STEEL, N_("Steel wall"), P_NON_EXPLODABLE, "STEELWALL", 'W', 4, 4, 4},
	{O_STEEL_SLOPED_UP_RIGHT, N_("Steel wall (up & right)"), P_SLOPED_UP|P_SLOPED_RIGHT|P_NON_EXPLODABLE, "STEELWALLSLOPEDUPRIGHT", 0, 284, 284, 284},
	{O_STEEL_SLOPED_UP_LEFT, N_("Steel wall (up & left)"), P_SLOPED_UP|P_SLOPED_LEFT|P_NON_EXPLODABLE, "STEELWALLSLOPEDUPLEFT", 0, 285, 285, 285},
	{O_STEEL_SLOPED_DOWN_LEFT, N_("Steel wall (down & left)"), P_SLOPED_DOWN|P_SLOPED_LEFT|P_NON_EXPLODABLE, "STEELWALLSLOPEDDOWNLEFT", 0, 286, 286, 286},
	{O_STEEL_SLOPED_DOWN_RIGHT, N_("Steel wall (down & right)"), P_SLOPED_DOWN|P_SLOPED_RIGHT|P_NON_EXPLODABLE, "STEELWALLSLOPEDDOWNRIGHT", 0, 287, 287, 287},
	{O_STEEL_EXPLODABLE, N_("Explodable steel wall"), 0, "STEELWALLDESTRUCTABLE", 'E', 309, 309, 4},
	{O_STEEL_EATABLE, N_("Eatable steel wall"), 0, "STEELWALLEATABLE", 0, 339, 339, 4},
	{O_BRICK_EATABLE, N_("Eatable brick wall"), 0, "WALLEATABLE", 0, 340, 340, 5},
	{O_STONE, N_("Stone"), P_SLOPED, "BOULDER", 'r', 1, 1, 1, 156},	/* has ckdelay */
	{O_STONE_F, N_("Stone, falling"), 0, "BOULDERf", 'R', 314, 314, 1, 156},	/* has ckdelay */
	{O_MEGA_STONE, N_("Mega stone"), P_SLOPED, "MEGABOULDER", 0, 272, 272, 272, 156},	/* has ckdelay */
	{O_MEGA_STONE_F, N_("Mega stone, falling"), 0, "MEGABOULDERf", 0, 345, 345, 272, 156},	/* has ckdelay */
	{O_DIAMOND, N_("Diamond"), P_SLOPED, "DIAMOND", 'd', 248, -248, -248, 156},	/* has ckdelay */
	{O_DIAMOND_F, N_("Diamond, falling"), 0, "DIAMONDf", 'D', 315, 315, -248, 156},	/* has ckdelay */
	{O_BLADDER_SPENDER, N_("Bladder Spender"), 0, "BLADDERSPENDER", 0, 6, 6, 6, 20},	/* has ckdelay */
	{O_INBOX, N_("Inbox"), 0, "INBOX", 'P', 35, 35, 22},
	{O_H_GROWING_WALL, N_("Expanding wall, horizontal"), P_VISUAL_EFFECT, "HEXPANDINGWALL", 'x', 316, 316, 5, 111},	/* has ckdelay */
	{O_V_GROWING_WALL, N_("Expanding wall, vertical"), P_VISUAL_EFFECT, "VEXPANDINGWALL", 'v', 326, 326, 5, 111},	/* has ckdelay */
	{O_GROWING_WALL, N_("Expanding wall"), P_VISUAL_EFFECT, "EXPANDINGWALL", 'e', 343, 343, 5, 111},	/* has ckdelay */
	{O_GROWING_WALL_SWITCH, N_("Expanding wall switch"), 0, "EXPANDINGWALLSWITCH", 0, 40, 40, 40},
	{O_CREATURE_SWITCH, N_("Creature direction switch"), 0, "FIREFLYBUTTERFLYSWITCH", 0, 18, 18, 18},
	{O_BITER_SWITCH, N_("Biter switch"), 0, "BITERSWITCH", 0, 12, 12, 12},
	{O_ACID, N_("Acid"), 0, "ACID", 0, 20, 20, 20, 128},	/* has ckdelay */
	{O_FALLING_WALL, N_("Falling wall"), 0, "FALLINGWALL", 0, 342, 342, 5, 80},	/* has ckdelay */
	{O_FALLING_WALL_F, N_("Falling wall, falling"), 0, "FALLINGWALLf", 0, 344, 344, 5, 80},	/* has ckdelay */
	{O_BOX, N_("Box"), 0, "SOKOBANBOX", 0, 21, 21, 21},
	{O_TIME_PENALTY, N_("Time penalty"), P_NON_EXPLODABLE, "TIMEPENALTY", 0, 346, 346, 9},
	{O_GRAVESTONE, N_("Gravestone"), P_NON_EXPLODABLE, "GRAVESTONE", 'G', 9, 9, 9},
	{O_STONE_GLUED, N_("Glued stone"), P_SLOPED, "GLUEDBOULDER", 0, 334, 334, 1},
	{O_DIAMOND_GLUED, N_("Glued diamond"), P_SLOPED, "GLUEDDIAMOND", 0, 341, 341, -248},
	{O_DIAMOND_KEY, N_("Diamond key"), 0, "DIAMONDRELEASEKEY", 0, 11, 11, 11},
	{O_TRAPPED_DIAMOND, N_("Trapped diamond"), P_NON_EXPLODABLE, "TRAPPEDDIAMOND", 0, 10, 10, 10},
	{O_CLOCK, N_("Clock"), 0, "CLOCK", 0, 16, 16, 16},
	{O_DIRT_GLUED, N_("Glued dirt"), 0, "GLUEDDIRT", 0, 321, 321, 2},
	{O_KEY_1, N_("Key 1"), 0, "KEY1", 0, 67, 67, 67},
	{O_KEY_2, N_("Key 2"), 0, "KEY2", 0, 68, 68, 68},
	{O_KEY_3, N_("Key 3"), 0, "KEY3", 0, 69, 69, 69},
	{O_DOOR_1, N_("Door 1"), 0, "DOOR1", 0, 64, 64, 64},
	{O_DOOR_2, N_("Door 2"), 0, "DOOR2", 0, 65, 65, 65},
	{O_DOOR_3, N_("Door 3"), 0, "DOOR3", 0, 66, 66, 66},

	{O_POT, N_("Pot"), 0, "POT", 0, 63, 63, 63},
	{O_GRAVITY_SWITCH, N_("Gravity switch"), 0, "GRAVITY_SWITCH", 0, 70, 70, 70},
	{O_PNEUMATIC_HAMMER, N_("Pneumatic hammer"), 0, "PNEUMATIC_HAMMER", 0, 62, 62, 62},
	{O_TELEPORTER, N_("Teleporter"), 0, "TELEPORTER", 0, 61, 61, 61},
	{O_SKELETON, N_("Skeleton"), 0, "SKELETON", 0, 72, 72, 72},
	{O_WATER, N_("Water"), 0, "WATER", 0, 96, -96, -96, 100},	/* has ckdelay */
	{O_WATER_1, N_("Water (1)"), 0, "WATER1", 0, 96, -96, -96},
	{O_WATER_2, N_("Water (2)"), 0, "WATER2", 0, 96, -96, -96},
	{O_WATER_3, N_("Water (3)"), 0, "WATER3", 0, 96, -96, -96},
	{O_WATER_4, N_("Water (4)"), 0, "WATER4", 0, 96, -96, -96},
	{O_WATER_5, N_("Water (5)"), 0, "WATER5", 0, 96, -96, -96},
	{O_WATER_6, N_("Water (6)"), 0, "WATER6", 0, 96, -96, -96},
	{O_WATER_7, N_("Water (7)"), 0, "WATER7", 0, 96, -96, -96},
	{O_WATER_8, N_("Water (8)"), 0, "WATER8", 0, 96, -96, -96},
	{O_WATER_9, N_("Water (9)"), 0, "WATER9", 0, 96, -96, -96},
	{O_WATER_10, N_("Water (10)"), 0, "WATER10", 0, 96, -96, -96},
	{O_WATER_11, N_("Water (11)"), 0, "WATER11", 0, 96, -96, -96},
	{O_WATER_12, N_("Water (12)"), 0, "WATER12", 0, 96, -96, -96},
	{O_WATER_13, N_("Water (13)"), 0, "WATER13", 0, 96, -96, -96},
	{O_WATER_14, N_("Water (14)"), 0, "WATER14", 0, 96, -96, -96},
	{O_WATER_15, N_("Water (15)"), 0, "WATER15", 0, 96, -96, -96},
	{O_WATER_16, N_("Water (16)"), 0, "WATER16", 0, 96, -96, -96},
	{O_COW_1, N_("Cow (left)"), P_CCW, "COWl", 0, 317, -88, -88, 384},	/* has ckdelay */
	{O_COW_2, N_("Cow (up)"), P_CCW, "COWu", 0, 318, -88, -88, 384},	/* has ckdelay */
	{O_COW_3, N_("Cow (right)"), P_CCW, "COWr", 0, 319, -88, -88, 384},	/* has ckdelay */
	{O_COW_4, N_("Cow (down)"), P_CCW, "COWd", 0, 320, -88, -88, 384},	/* has ckdelay */
	{O_COW_ENCLOSED_1, N_("Cow (enclosed, 1)"), 0, "COW_ENCLOSED1", 0, 327, -88, -88, 120},	/* has ckdelay */
	{O_COW_ENCLOSED_2, N_("Cow (enclosed, 2)"), 0, "COW_ENCLOSED2", 0, 327, -88, -88, 120},	/* has ckdelay */
	{O_COW_ENCLOSED_3, N_("Cow (enclosed, 3)"), 0, "COW_ENCLOSED3", 0, 327, -88, -88, 120},	/* has ckdelay */
	{O_COW_ENCLOSED_4, N_("Cow (enclosed, 4)"), 0, "COW_ENCLOSED4", 0, 327, -88, -88, 120},	/* has ckdelay */
	{O_COW_ENCLOSED_5, N_("Cow (enclosed, 5)"), 0, "COW_ENCLOSED5", 0, 327, -88, -88, 120},	/* has ckdelay */
	{O_COW_ENCLOSED_6, N_("Cow (enclosed, 6)"), 0, "COW_ENCLOSED6", 0, 327, -88, -88, 120},	/* has ckdelay */
	{O_COW_ENCLOSED_7, N_("Cow (enclosed, 7)"), 0, "COW_ENCLOSED7", 0, 327, -88, -88, 120},	/* has ckdelay */
	{O_WALLED_DIAMOND, N_("Walled diamond"), P_CAN_BE_HAMMERED, "WALLED_DIAMOND", 0, 322, 322, 5},
	{O_WALLED_KEY_1, N_("Walled key 1"), P_CAN_BE_HAMMERED, "WALLED_KEY1", 0, 323, 323, 5},
	{O_WALLED_KEY_2, N_("Walled key 2"), P_CAN_BE_HAMMERED, "WALLED_KEY2", 0, 324, 324, 5},
	{O_WALLED_KEY_3, N_("Walled key 3"), P_CAN_BE_HAMMERED, "WALLED_KEY3", 0, 325, 325, 5},

	{O_AMOEBA, N_("Amoeba"), P_BLOWS_UP_FLIES, "AMOEBA", 'a', 192, -192, -192, 260},	/* has ckdelay */
	{O_SWEET, N_("Sweet"), 0, "SWEET", 0, 8, 8, 8},
	{O_VOODOO, N_("Voodoo doll"), P_BLOWS_UP_FLIES, "DUMMY", 'F', 7, 7, 7},
	{O_SLIME, N_("Slime"), 0, "SLIME", 's', 200, -200, -200, 211},	/* has ckdelay */
	{O_BLADDER, N_("Bladder"), 0, "BLADDER", 0, 176, -176, -176, 267},	/* has ckdelay */
	{O_BLADDER_1, N_("Bladder (1)"), 0, "BLADDERd1", 0, 176, -176, -176},
	{O_BLADDER_2, N_("Bladder (2)"), 0, "BLADDERd2", 0, 176, -176, -176},
	{O_BLADDER_3, N_("Bladder (3)"), 0, "BLADDERd3", 0, 176, -176, -176},
	{O_BLADDER_4, N_("Bladder (4)"), 0, "BLADDERd4", 0, 176, -176, -176},
	{O_BLADDER_5, N_("Bladder (5)"), 0, "BLADDERd5", 0, 176, -176, -176},
	{O_BLADDER_6, N_("Bladder (6)"), 0, "BLADDERd6", 0, 176, -176, -176},
	{O_BLADDER_7, N_("Bladder (7)"), 0, "BLADDERd7", 0, 176, -176, -176},
	{O_BLADDER_8, N_("Bladder (8)"), 0, "BLADDERd8", 0, 176, -176, -176},
	{O_BLADDER_9, N_("Bladder (9)"), 0, "BLADDERd9", 0, 176, -176, -176},

	{O_WAITING_STONE, N_("Waiting stone"), P_SLOPED, "WAITINGBOULDER", 0, 290, 290, 1, 176},	/* has ckdelay */
	{O_CHASING_STONE, N_("Chasing stone"), P_SLOPED, "CHASINGBOULDER", 0, 17, 17, 17, 269},	/* has ckdelay */
	{O_GHOST, N_("Ghost"), 0, "GHOST", 'g', 160, -160, -160, 50},	/* has ckdelay */
	{O_GUARD_1, N_("Guard, left"), P_EXPLODES_TO_SPACE | P_CCW, "FIREFLYl", 'Q', 310, -136, -136, 384},	/* has ckdelay */
	{O_GUARD_2, N_("Guard, up"), P_EXPLODES_TO_SPACE | P_CCW, "FIREFLYu", 'o', 311, -136, -136, 384},	/* has ckdelay */
	{O_GUARD_3, N_("Guard, right"), P_EXPLODES_TO_SPACE | P_CCW, "FIREFLYr", 'O', 312, -136, -136, 384},	/* has ckdelay */
	{O_GUARD_4, N_("Guard, down"), P_EXPLODES_TO_SPACE | P_CCW, "FIREFLYd", 'q', 313, -136, -136, 384},	/* has ckdelay */
	{O_ALT_GUARD_1, N_("Alternative guard, left"), P_EXPLODES_TO_SPACE, "A_FIREFLYl", 0, 301, -104, -104, 384},	/* has ckdelay */
	{O_ALT_GUARD_2, N_("Alternative guard, up"), P_EXPLODES_TO_SPACE, "A_FIREFLYu", 0, 302, -104, -104, 384},	/* has ckdelay */
	{O_ALT_GUARD_3, N_("Alternative guard, right"), P_EXPLODES_TO_SPACE, "A_FIREFLYr", 0, 303, -104, -104, 384},	/* has ckdelay */
	{O_ALT_GUARD_4, N_("Alternative guard, down"), P_EXPLODES_TO_SPACE, "A_FIREFLYd", 0, 304, -104, -104, 384},	/* has ckdelay */
	{O_BUTTER_1, N_("Butterfly, left"), P_EXPLODES_TO_DIAMONDS, "BUTTERFLYl", 'C', 330, -144, -144, 384},	/* has ckdelay */
	{O_BUTTER_2, N_("Butterfly, up"), P_EXPLODES_TO_DIAMONDS, "BUTTERFLYu", 'b', 331, -144, -144, 384},	/* has ckdelay */
	{O_BUTTER_3, N_("Butterfly, right"), P_EXPLODES_TO_DIAMONDS, "BUTTERFLYr", 'B', 332, -144, -144, 384},	/* has ckdelay */
	{O_BUTTER_4, N_("Butterfly, down"), P_EXPLODES_TO_DIAMONDS, "BUTTERFLYd", 'c', 333, -144, -144, 384},	/* has ckdelay */
	{O_ALT_BUTTER_1, N_("Alternative butterfly, left"), P_EXPLODES_TO_DIAMONDS | P_CCW, "A_BUTTERFLYl", 0, 305, -112, -112, 384},	/* has ckdelay */
	{O_ALT_BUTTER_2, N_("Alternative butterfly, up"), P_EXPLODES_TO_DIAMONDS | P_CCW, "A_BUTTERFLYu", 0, 306, -112, -112, 384},	/* has ckdelay */
	{O_ALT_BUTTER_3, N_("Alternative butterfly, right"), P_EXPLODES_TO_DIAMONDS | P_CCW, "A_BUTTERFLYr", 0, 307, -112, -112, 384},	/* has ckdelay */
	{O_ALT_BUTTER_4, N_("Alternative butterfly, down"), P_EXPLODES_TO_DIAMONDS | P_CCW, "A_BUTTERFLYd", 0, 308, -112, -112, 384},	/* has ckdelay */
	{O_STONEFLY_1, N_("Stonefly, left"), P_EXPLODES_TO_STONES, "STONEFLYl", 0, 335, -152, -152, 384},	/* has ckdelay */
	{O_STONEFLY_2, N_("Stonefly, up"), P_EXPLODES_TO_STONES, "STONEFLYu", 0, 336, -152, -152, 384},	/* has ckdelay */
	{O_STONEFLY_3, N_("Stonefly, right"), P_EXPLODES_TO_STONES, "STONEFLYr", 0, 337, -152, -152, 384},	/* has ckdelay */
	{O_STONEFLY_4, N_("Stonefly, down"), P_EXPLODES_TO_STONES, "STONEFLYd", 0, 338, -152, -152, 384},	/* has ckdelay */
	{O_BITER_1, N_("Biter, up"), 0, "BITERu", 0, 347, -168, -168, 518},	/* has ckdelay */
	{O_BITER_2, N_("Biter, right"), 0, "BITERr", 0, 348, -168, -168, 518},	/* has ckdelay */
	{O_BITER_3, N_("Biter, down"), 0, "BITERd", 0, 349, -168, -168, 518},	/* has ckdelay */
	{O_BITER_4, N_("Biter, left"), 0, "BITERl", 0, 350, -168, -168, 518},	/* has ckdelay */

	{O_PRE_PL_1, N_("Player birth (1)"), 0, "GUYBIRTH1", 0, 32, 32, 32},
	{O_PRE_PL_2, N_("Player birth (2)"), 0, "GUYBIRTH2", 0, 33, 33, 33},
	{O_PRE_PL_3, N_("Player birth (3)"), 0, "GUYBIRTH3", 0, 34, 34, 34},
	{O_PLAYER, N_("Player"), P_BLOWS_UP_FLIES | P_EXPLODES_TO_SPACE, "GUY", 0, 328, 328, 35, 32},	/* has ckdelay */
	{O_PLAYER_BOMB, N_("Player with bomb"), P_BLOWS_UP_FLIES | P_EXPLODES_TO_SPACE, "GUYBOMB", 0, 42, 42, 42, 25},	/* has ckdelay */
	{O_PLAYER_GLUED, N_("Glued player"), P_BLOWS_UP_FLIES | P_EXPLODES_TO_SPACE, "GUYGLUED", 0, 329, 329, 35},
	{O_PLAYER_STIRRING, N_("Player stirring"), P_BLOWS_UP_FLIES | P_EXPLODES_TO_SPACE, "GUYSTIRRING", 0, 256, -256, -256},

	{O_BOMB, N_("Bomb"), 0, "BOMB", 0, 48, 48, 48},
	{O_BOMB_TICK_1, N_("Ticking bomb (1)"), 0, "IGNITEDBOMB1", 0, 49, 49, 49},
	{O_BOMB_TICK_2, N_("Ticking bomb (2)"), 0, "IGNITEDBOMB2", 0, 50, 50, 50},
	{O_BOMB_TICK_3, N_("Ticking bomb (3)"), 0, "IGNITEDBOMB3", 0, 51, 51, 51},
	{O_BOMB_TICK_4, N_("Ticking bomb (4)"), 0, "IGNITEDBOMB4", 0, 52, 52, 52},
	{O_BOMB_TICK_5, N_("Ticking bomb (5)"), 0, "IGNITEDBOMB5", 0, 53, 53, 53},
	{O_BOMB_TICK_6, N_("Ticking bomb (6)"), 0, "IGNITEDBOMB6", 0, 54, 54, 54},
	{O_BOMB_TICK_7, N_("Ticking bomb (7)"), 0, "IGNITEDBOMB7", 0, 55, 55, 55},

	{O_PRE_CLOCK_1, N_("Clock birth (1)"), 0, "CLOCKBIRTH1", 0, 28, 28, 28, 280},	/* has ckdelay */
	{O_PRE_CLOCK_2, N_("Clock birth (2)"), 0, "CLOCKBIRTH2", 0, 29, 29, 29, 280},	/* has ckdelay */
	{O_PRE_CLOCK_3, N_("Clock birth (3)"), 0, "CLOCKBIRTH3", 0, 30, 30, 30, 280},	/* has ckdelay */
	{O_PRE_CLOCK_4, N_("Clock birth (4)"), 0, "CLOCKBIRTH4", 0, 31, 31, 31, 280},	/* has ckdelay */
	{O_PRE_DIA_1, N_("Diamond birth (1)"), 0, "DIAMONDBIRTH1", 0, 56, 56, 56, 280},	/* has ckdelay */
	{O_PRE_DIA_2, N_("Diamond birth (2)"), 0, "DIAMONDBIRTH2", 0, 57, 57, 57, 280},	/* has ckdelay */
	{O_PRE_DIA_3, N_("Diamond birth (3)"), 0, "DIAMONDBIRTH3", 0, 58, 58, 58, 280},	/* has ckdelay */
	{O_PRE_DIA_4, N_("Diamond birth (4)"), 0, "DIAMONDBIRTH4", 0, 59, 59, 59, 280},	/* has ckdelay */
	{O_PRE_DIA_5, N_("Diamond birth (5)"), 0, "DIAMONDBIRTH5", 0, 60, 60, 60, 280},	/* has ckdelay */
	{O_EXPLODE_1, N_("Explosion (1)"), 0, "EXPLOSION1", 0, 43, 43, 43, 280},	/* has ckdelay */
	{O_EXPLODE_2, N_("Explosion (2)"), 0, "EXPLOSION2", 0, 44, 44, 44, 280},	/* has ckdelay */
	{O_EXPLODE_3, N_("Explosion (3)"), 0, "EXPLOSION3", 0, 45, 45, 45, 280},	/* has ckdelay */
	{O_EXPLODE_4, N_("Explosion (4)"), 0, "EXPLOSION4", 0, 46, 46, 46, 280},	/* has ckdelay */
	{O_EXPLODE_5, N_("Explosion (5)"), 0, "EXPLOSION5", 0, 47, 47, 47, 280},	/* has ckdelay */
	{O_PRE_STONE_1, N_("Stone birth (1)"), 0, "BOULDERBIRTH1", 0, 36, 36, 36, 280},	/* has ckdelay */
	{O_PRE_STONE_2, N_("Stone birth (2)"), 0, "BOULDERBIRTH2", 0, 37, 37, 37, 280},	/* has ckdelay */
	{O_PRE_STONE_3, N_("Stone birth (3)"), 0, "BOULDERBIRTH3", 0, 38, 38, 38, 280},	/* has ckdelay */
	{O_PRE_STONE_4, N_("Stone birth (4)"), 0, "BOULDERBIRTH4", 0, 39, 39, 39, 280},	/* has ckdelay */
	{O_PRE_STEEL_1, N_("Steel birth (1)"), 0, "STEELWALLBIRTH1", 0, 24, 24, 24, 280},	/* has ckdelay */
	{O_PRE_STEEL_2, N_("Steel birth (2)"), 0, "STEELWALLBIRTH2", 0, 25, 25, 25, 280},	/* has ckdelay */
	{O_PRE_STEEL_3, N_("Steel birth (3)"), 0, "STEELWALLBIRTH3", 0, 26, 26, 26, 280},	/* has ckdelay */
	{O_PRE_STEEL_4, N_("Steel birth (4)"), 0, "STEELWALLBIRTH4", 0, 27, 27, 27, 280},	/* has ckdelay */
	{O_GHOST_EXPL_1, N_("Ghost explosion (1)"), 0, "GHOSTEXPLOSION1", 0, 80, 80, 80, 280},	/* has ckdelay */
	{O_GHOST_EXPL_2, N_("Ghost explosion (2)"), 0, "GHOSTEXPLOSION2", 0, 81, 81, 81, 280},	/* has ckdelay */
	{O_GHOST_EXPL_3, N_("Ghost explosion (3)"), 0, "GHOSTEXPLOSION3", 0, 82, 82, 82, 280},	/* has ckdelay */
	{O_GHOST_EXPL_4, N_("Ghost explosion (4)"), 0, "GHOSTEXPLOSION4", 0, 83, 83, 83, 280},	/* has ckdelay */
	{O_BOMB_EXPL_1, N_("Bomb explosion (1)"), 0, "BOMBEXPLOSION1", 0, 84, 84, 84, 280},	/* has ckdelay */
	{O_BOMB_EXPL_2, N_("Bomb explosion (2)"), 0, "BOMBEXPLOSION2", 0, 85, 85, 85, 280},	/* has ckdelay */
	{O_BOMB_EXPL_3, N_("Bomb explosion (3)"), 0, "BOMBEXPLOSION3", 0, 86, 86, 86, 280},	/* has ckdelay */
	{O_BOMB_EXPL_4, N_("Bomb explosion (4)"), 0, "BOMBEXPLOSION4", 0, 87, 87, 87, 280},	/* has ckdelay */

	{O_PLAYER_PNEUMATIC_LEFT, NULL /* Player using hammer, left */, P_BLOWS_UP_FLIES|P_EXPLODES_TO_SPACE, "GUYHAMMERl", 0, 265, 265, 265},
	{O_PLAYER_PNEUMATIC_RIGHT, NULL /* Player using hammer, right */, P_BLOWS_UP_FLIES|P_EXPLODES_TO_SPACE, "GUYHAMMERr", 0, 268, 268, 268},
	{O_PNEUMATIC_ACTIVE_LEFT, NULL /* Active hammer, left */, 0, "HAMMERACTIVEl", 0, 264, 264, 264},
	{O_PNEUMATIC_ACTIVE_RIGHT, NULL /* Active hammer, right */, 0, "HAMMERACTIVEr", 0, 269, 269, 269},

	{O_UNKNOWN, N_("Unknown element"), P_NON_EXPLODABLE, "UNKNOWN", 0, 289, 289, 4},
	{O_NONE, N_("No element"), 0, "NONE", 0, 79, 79, 79},
	{O_MAX},

	/* these are just helpers, for all the element -> image index information to be in this array */
	{O_FAKE_BONUS, NULL, 0, NULL, 0, 120, -120, -120},
	{O_OUTBOX_CLOSED, NULL, 0, NULL, 0, 22, 22, 22},	/* game graphics - also for imported diego effects, but don't know if it is used anywhere in original games */
	{O_OUTBOX_OPEN, NULL, 0, NULL, 0, 23, 23, 23},
	{O_COVERED, NULL, 0, NULL, 0, 128, -128, -128},
	{O_PLAYER_LEFT, NULL, 0, NULL, 0, 232, -232, -232},
	{O_PLAYER_RIGHT, NULL, 0, NULL, 0, 240, -240, -240},
	{O_PLAYER_TAP, NULL, 0, NULL, 0, 216, -216, -216},
	{O_PLAYER_BLINK, NULL, 0, NULL, 0, 208, -208, -208},
	{O_PLAYER_TAP_BLINK, NULL, 0, NULL, 0, 224, -224, -224},
	{O_CREATURE_SWITCH_ON, NULL, 0, NULL, 0, 19, 19, 19},
	{O_GROWING_WALL_SWITCH_HORIZ, NULL, 0, NULL, 0, 40, 40, 40},
	{O_GROWING_WALL_SWITCH_VERT, NULL, 0, NULL, 0, 41, 41, 41},
	{O_GRAVITY_SWITCH_ACTIVE, NULL, 0, NULL, 0, 71, 71, 71},

	{O_DOWN_ARROW, NULL, 0, NULL, 0, 73, 73, 73},
	{O_LEFTRIGHT_ARROW, NULL, 0, NULL, 0, 74, 74, 74},
	{O_EVERYDIR_ARROW, NULL, 0, NULL, 0, 75, 75, 75},
	{O_GLUED, NULL, 0, NULL, 0, 76, 76, 76},
	{O_OUT, NULL, 0, NULL, 0, 77, 77, 77},
	{O_EXCLAMATION_MARK, NULL, 0, NULL, 0, 78, 78, 78},
};








/* entries. */
/* type given for each element;
 * GD_TYPE_ELEMENT represents a combo box of gdash objects.
 * GD_TAB&LABEL represents a notebook tab or a label.
 * others are self-explanatory. */
const GdCaveProperties gd_cave_properties[] = {
	/* default data */
	{"", GD_TAB, GD_FOR_CAVESET, N_("Cave data")},
	{"Name", GD_TYPE_STRING, GD_FOR_CAVESET, N_("Name"), G_STRUCT_OFFSET(Cave, name), 1, N_("Name of game")},
	{"Description", GD_TYPE_STRING, GD_FOR_CAVESET, N_("Description"), G_STRUCT_OFFSET(Cave, description), 1, N_("Some words about the game")},
	{"Author", GD_TYPE_STRING, GD_FOR_CAVESET, N_("Author"), G_STRUCT_OFFSET(Cave, author), 1, N_("Name of author")},
	{"Date", GD_TYPE_STRING, GD_FOR_CAVESET, N_("Date"), G_STRUCT_OFFSET(Cave, date), 1, N_("Date of creation")},
	{"WWW", GD_TYPE_STRING, GD_FOR_CAVESET, N_("WWW"), G_STRUCT_OFFSET(Cave, www), 1, N_("Web page or e-mail address")},
	{"Difficulty", GD_TYPE_STRING, GD_FOR_CAVESET, N_("Difficulty"), G_STRUCT_OFFSET(Cave, difficulty), 1, N_("Difficulty (informative)")},
	{"Remark", GD_TYPE_STRING, GD_FOR_CAVESET, N_("Remark"), G_STRUCT_OFFSET(Cave, remark), 1, N_("Remark (informative)")},
	{"Selectable", GD_TYPE_BOOLEAN, 0, N_("Selectable as start"), G_STRUCT_OFFSET(Cave, selectable), 1, N_("This sets whether the game can be started at this cave."), TRUE},
	{"Intermission", GD_TYPE_BOOLEAN, GD_ALWAYS_SAVE, N_("Intermission"), G_STRUCT_OFFSET(Cave, intermission), 1, NULL, FALSE},
	{"IntermissionProperties.instantlife", GD_TYPE_BOOLEAN, 0, N_("   Instant life"), G_STRUCT_OFFSET(Cave, intermission_instantlife), 1, N_("If true, an extra life is given to the player, when the intermission cave is reached."), FALSE},
	{"IntermissionProperties.rewardlife", GD_TYPE_BOOLEAN, 0, N_("   Reward life"), G_STRUCT_OFFSET(Cave, intermission_rewardlife), 1, N_("If true, an extra life is given to the player, when the intermission cave is successfully finished."), TRUE},
	{"Size", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Width"), G_STRUCT_OFFSET(Cave, w), 1, N_("Width and height of cave."), 40, 12, 128},
	{"Size", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Height"), G_STRUCT_OFFSET(Cave, h), 1, N_("Width and height of cave."), 22, 12, 128},
	{"Size", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Visible, left"), G_STRUCT_OFFSET(Cave, x1), 1, N_("Visible parts of the cave, upper left and lower right corner."), 0, 0, 127},
	{"Size", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Visible, upper"), G_STRUCT_OFFSET(Cave, y1), 1, N_("Visible parts of the cave, upper left and lower right corner."), 0, 0, 127},
	{"Size", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Visible, right"), G_STRUCT_OFFSET(Cave, x2), 1, N_("Visible parts of the cave, upper left and lower right corner."), 39, 0, 127},
	{"Size", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Visible, lower"), G_STRUCT_OFFSET(Cave, y2), 1, N_("Visible parts of the cave, upper left and lower right corner."), 21, 0, 127},
	{"Colors", GD_TYPE_COLOR, GD_ALWAYS_SAVE|GD_DONT_SHOW_IN_EDITOR, N_("Border color"), G_STRUCT_OFFSET(Cave, colorb), 1, N_("Border color for C64 graphics. Only for compatibility, not used by GDash.")},
	{"Colors", GD_TYPE_COLOR, GD_ALWAYS_SAVE|GD_DONT_SHOW_IN_EDITOR, N_("Background color"), G_STRUCT_OFFSET(Cave, color0), 1, N_("Background color for C64 graphics"), 0},
	{"Colors", GD_TYPE_COLOR, GD_ALWAYS_SAVE|GD_DONT_SHOW_IN_EDITOR, N_("Color 1 (dirt)"), G_STRUCT_OFFSET(Cave, color1), 1, N_("Foreground color 1 for C64 graphics"), 8},
	{"Colors", GD_TYPE_COLOR, GD_ALWAYS_SAVE|GD_DONT_SHOW_IN_EDITOR, N_("Color 2 (steel wall)"), G_STRUCT_OFFSET(Cave, color2), 1, N_("Foreground color 2 for C64 graphics"), 11},
	{"Colors", GD_TYPE_COLOR, GD_ALWAYS_SAVE|GD_DONT_SHOW_IN_EDITOR, N_("Color 3 (brick wall)"), G_STRUCT_OFFSET(Cave, color3), 1, N_("Foreground color 3 for C64 graphics"), 1},
	{"Colors", GD_TYPE_COLOR, GD_ALWAYS_SAVE|GD_DONT_SHOW_IN_EDITOR, N_("Amoeba color"), G_STRUCT_OFFSET(Cave, color4), 1, N_("Amoeba color for C64 graphics"), 5},
	{"Colors", GD_TYPE_COLOR, GD_ALWAYS_SAVE|GD_DONT_SHOW_IN_EDITOR, N_("Slime color"), G_STRUCT_OFFSET(Cave, color5), 1, N_("Slime color for C64 graphics"), 6},
	{"Charset", GD_TYPE_STRING, GD_FOR_CAVESET, N_("Character set"), G_STRUCT_OFFSET(Cave, charset), 1, N_("Theme used for displaying the game. Not used by GDash.")},
	{"Fontset", GD_TYPE_STRING, GD_FOR_CAVESET, N_("Font set"), G_STRUCT_OFFSET(Cave, fontset), 1, N_("Font used during the game. Not used by GDash.")},

	/* difficulty */
	{"", GD_TAB, 0, N_("Difficulty")},
	{"", GD_LEVEL_LABEL, 0},
	{"", GD_LABEL, 0, N_("<b>Diamonds</b>")},
	{"DiamondsRequired", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Diamonds needed"), G_STRUCT_OFFSET(Cave, level_diamonds[0]), 5, N_("Here zero means automatically count diamonds before level start. If negative, the value is subtracted from that. This is useful for totally random caves."), 10, -100, 999},
	{"DiamondValue", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Score for diamonds"), G_STRUCT_OFFSET(Cave, diamond_value), 1, N_("Number of points per diamond collected, before opening the exit."), 0, 0, 100},
	{"DiamondValue", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Score for extra diamonds"), G_STRUCT_OFFSET(Cave, extra_diamond_value), 1, N_("Number of points per diamond collected, after opening the exit."), 0, 0, 100},
	{"", GD_LABEL, 0, N_("<b>Time</b>")},
	{"CaveTime", GD_TYPE_INT, GD_ALWAYS_SAVE, N_("Time (s)"), G_STRUCT_OFFSET(Cave, level_time[0]), 5, N_("Time available to solve cave, in seconds."), 999, 1, 999},
	{"PALTiming", GD_TYPE_BOOLEAN, 0, N_("   PAL timing (slower seconds)"), G_STRUCT_OFFSET(Cave, pal_timing), 1, N_("On the PAL version of the C64 computer, the timer was "
	"actually slower than normal seconds. This flag is used to compensate for this. Most original games are authored for the PAL version."), FALSE},
	{"TimeValue", GD_TYPE_INT, 0, N_("Score for time"), G_STRUCT_OFFSET(Cave, level_timevalue[0]), 5, N_("Points for each seconds remaining, when the player exits the level."), 1, 0, 50},
	{"CaveScheduling", GD_TYPE_SCHEDULING, GD_DONT_SAVE, N_("Scheduling type"), G_STRUCT_OFFSET(Cave, scheduling), 1, N_("This flag sets whether the game uses an emulation of the original timing (c64-style), or a more modern milliseconds-based timing. The original game used a delay (empty loop) based timing of caves; this is selected by setting this to BD1, Construction Kit or Crazy Dream 7."), GD_SCHEDULING_MILLISECONDS},
	{"CaveDelay", GD_TYPE_INT, GD_DONT_SAVE, N_("   Delay (c64-style)"), G_STRUCT_OFFSET(Cave, level_ckdelay[0]), 5, N_("The length of the delay loop between game frames. Used when milliseconds-based timing is inactive, ie. C64 scheduling is on."), 0, 0, 32},
	{"FrameTime", GD_TYPE_INT, GD_DONT_SAVE, N_("   Speed (ms)"), G_STRUCT_OFFSET(Cave, level_speed[0]), 5, N_("Number of milliseconds between game frames. Used when milliseconds-based timing is active, ie. C64 scheduling is off."), 200, 50, 500},
	{"HatchingDelay", GD_TYPE_INT, 0, N_("Hatching delay"), G_STRUCT_OFFSET(Cave, hatching_delay), 1, N_("This value sets how much the cave will move until the player enters the cave. If C64 scheduling is used, then this is in seconds; if not, then it is in frames."), 21, 1, 40},
	{"", GD_LABEL, 0, N_("<b>Elements</b>")},
	{"RandSeed", GD_TYPE_INT, 0, N_("Random seed value"), G_STRUCT_OFFSET(Cave, level_rand[0]), 5, N_("Random seed value controls the predictable random number generator, which fills the cave initially. If set to -1, cave is totally random every time it is played."), 0, -1, 255},

	/* initial fill */
	{"InitialBorder", GD_TYPE_ELEMENT, GD_DONT_SHOW_IN_EDITOR, NULL /* Initial border */, G_STRUCT_OFFSET(Cave, initial_border), 1, NULL, O_STEEL},
	{"InitialFill", GD_TYPE_ELEMENT, GD_DONT_SHOW_IN_EDITOR, NULL /* Initial fill */, G_STRUCT_OFFSET(Cave, initial_fill), 1, NULL, O_DIRT},
	{"RandomFill", GD_TYPE_ELEMENT, GD_DONT_SHOW_IN_EDITOR, NULL /* Random fill 1 */, G_STRUCT_OFFSET(Cave, random_fill[0]), 1, NULL, O_DIRT},
	{"RandomFill", GD_TYPE_INT, GD_DONT_SHOW_IN_EDITOR, NULL /* Probability 1 */, G_STRUCT_OFFSET(Cave, random_fill_probability[0]), 1, NULL, 0, 0, 255},
	{"RandomFill", GD_TYPE_ELEMENT, GD_DONT_SHOW_IN_EDITOR, NULL /* Random fill 2 */, G_STRUCT_OFFSET(Cave, random_fill[1]), 1, NULL, O_DIRT},
	{"RandomFill", GD_TYPE_INT, GD_DONT_SHOW_IN_EDITOR, NULL /* Probability 2 */, G_STRUCT_OFFSET(Cave, random_fill_probability[1]), 1, NULL, 0, 0, 255},
	{"RandomFill", GD_TYPE_ELEMENT, GD_DONT_SHOW_IN_EDITOR, NULL /* Random fill 3 */, G_STRUCT_OFFSET(Cave, random_fill[2]), 1, NULL, O_DIRT},
	{"RandomFill", GD_TYPE_INT, GD_DONT_SHOW_IN_EDITOR, NULL /* Probability 3 */, G_STRUCT_OFFSET(Cave, random_fill_probability[2]), 1, NULL, 0, 0, 255},
	{"RandomFill", GD_TYPE_ELEMENT, GD_DONT_SHOW_IN_EDITOR, NULL /* Random fill 4 */, G_STRUCT_OFFSET(Cave, random_fill[3]), 1, NULL, O_DIRT},
	{"RandomFill", GD_TYPE_INT, GD_DONT_SHOW_IN_EDITOR, NULL /* Probability 4 */, G_STRUCT_OFFSET(Cave, random_fill_probability[3]), 1, NULL, 0, 0, 255},

	/* PLAYER */
	{"", GD_TAB, 0, N_("Player")},
	/* player */
	{"", GD_LABEL, 0, N_("<b>Player movements</b>")},
	{"DiagonalMovement", GD_TYPE_BOOLEAN, 0, N_("Diagonal movements"), G_STRUCT_OFFSET(Cave, diagonal_movements), 1, N_("Controls if the player can move diagonally."), FALSE},
	{"ActiveGuyIsFirst", GD_TYPE_BOOLEAN, 0, N_("Uppermost player active"), G_STRUCT_OFFSET(Cave, active_is_first_found), 1, N_("In 1stB, cave is scrolled to the uppermost and leftmost player found, whereas in the original game to the last one. Chasing stones also follow the active player."), TRUE},
	{"SnapExplosions", GD_TYPE_BOOLEAN, 0, N_("Snap explosions"), G_STRUCT_OFFSET(Cave, snap_explosions), 1, N_("If this is set to true, snapping dirt will create small explosions."), FALSE},
	{"PushingBoulderProb", GD_TYPE_PROBABILITY, 0, N_("Probability of pushing stone (%)"), G_STRUCT_OFFSET(Cave, pushing_stone_prob), 1, N_("Chance of player managing to push a stone, every game cycle he tries. This is the normal probability."), 125000},
	{"PushingBoulderProb", GD_TYPE_PROBABILITY, 0, N_("Probability of pushing with sweet (%)"), G_STRUCT_OFFSET(Cave, pushing_stone_prob_sweet), 1, N_("Chance of player managing to push a stone, every game cycle he tries. This is used after eating sweet."), 1000000},
	{"", GD_LABEL, 0, N_("<b>Clock</b>")},
	{"BonusTime", GD_TYPE_INT, 0, N_("Time bonus for clock (s)"), G_STRUCT_OFFSET(Cave, penalty_time), 1, N_("Bonus time when a clock is collected."), 30, 0, 100},
	{"", GD_LABEL, 0, N_("<b>Pneumatic hammer</b>")},
	{"PneumaticHammer.frames", GD_TYPE_INT, 0, N_("Time for hammer (frames)"), G_STRUCT_OFFSET(Cave, pneumatic_hammer_frame), 1, N_("This is the number of game frames, a pneumatic hammer is required to break a wall."), 5, 1, 100},
	{"PneumaticHammer.wallsreappear", GD_TYPE_BOOLEAN, 0, N_("Hammered walls reappear"), G_STRUCT_OFFSET(Cave, hammered_walls_reappear), 1, N_("If this is set to true, walls broken with a pneumatic hammer will reappear later."), FALSE},
	{"PneumaticHammer.wallsreappearframes", GD_TYPE_INT, 0, N_("   Timer for reappear (frames)"), G_STRUCT_OFFSET(Cave, hammered_wall_reappear_frame), 1, N_("This sets the number of game frames, after hammered walls reappear, when the above setting is true."), 100, 1, 200},
	/* voodoo */
	{"", GD_LABEL, 0, N_("<b>Voodoo Doll</b>")},
	{"DummyProperties.diamondcollector", GD_TYPE_BOOLEAN, 0, N_("Can collect diamonds"), G_STRUCT_OFFSET(Cave, voodoo_collects_diamonds), 1, N_("Controls if a voodoo doll can collect diamonds for the player."), FALSE},
	{"DummyProperties.penalty", GD_TYPE_BOOLEAN, 0, N_("Dies if hit by a stone"), G_STRUCT_OFFSET(Cave, voodoo_dies_by_stone), 1, N_("Controls if the voodoo doll dies if it is hit by a stone. Then the player gets a time penalty."), FALSE},
	{"DummyProperties.destructable", GD_TYPE_BOOLEAN, 0, N_("Can be destroyed"), G_STRUCT_OFFSET(Cave, voodoo_can_be_destroyed), 1, N_("Controls if the voodoo can be destroyed by an explosion nearby. If not, it is converted to a gravestone, and you get a time penalty."), TRUE},
	{"PenaltyTime", GD_TYPE_INT, 0, N_("Time penalty (s)"), G_STRUCT_OFFSET(Cave, penalty_time), 1, N_("Penalty time when the voodoo is destroyed by a stone."), 30, 0, 100},

	/* ACTIVE 1 */
	{"", GD_TAB, 0, N_("Active elements")},
	/* magic wall */
	{"", GD_LABEL, 0, N_("<b>Magic Wall</b>")},
	{"MagicWallTime", GD_TYPE_INT, 0, N_("Milling time (s)"), G_STRUCT_OFFSET(Cave, magic_wall_milling_time), 1, N_("Magic wall will stop after this time, and it cannot be activated again."), 999, 0, 600},
	{"MagicWallProperties", GD_TYPE_ELEMENT, 0, N_("Converts diamond to"), G_STRUCT_OFFSET(Cave, magic_diamond_to), 1, N_("As a special effect, magic walls can convert diamonds to any other element."), O_STONE_F},
	{"MagicWallProperties", GD_TYPE_ELEMENT, 0, N_("Converts stone to"), G_STRUCT_OFFSET(Cave, magic_stone_to), 1, N_("As a special effect, magic walls can convert stones to any other element."), O_DIAMOND_F},
	{"MagicWallProperties.convertamoeba", GD_TYPE_BOOLEAN, 0, N_("Stops amoeba"), G_STRUCT_OFFSET(Cave, magic_wall_stops_amoeba), 1, N_("When the magic wall is activated, it can convert amoeba into diamonds."), TRUE},
	{"MagicWallProperties.waitforhatching", GD_TYPE_BOOLEAN, 0, N_("Timer waits for hatching"), G_STRUCT_OFFSET(Cave, magic_timer_wait_for_hatching), 1, N_("This determines if the magic wall timer starts before the player appearing. Magic can always be activated before that; but if this is set to true, the timer will not start."), FALSE},
	{"MagicWallProperties.sound", GD_TYPE_BOOLEAN, 0, N_("Has sound"), G_STRUCT_OFFSET(Cave, magic_wall_sound), 1, N_("If true, the activated magic wall will have sound."), TRUE},
	/* amoeba */
	{"", GD_LABEL, 0, N_("<b>Amoeba</b>")},
	{"AmoebaThreshold", GD_TYPE_RATIO, 0, N_("Threshold (cells)"), G_STRUCT_OFFSET(Cave, amoeba_threshold), 1, N_("If the amoeba grows more than this fraction of the cave, it is considered too big."), 200, 0, 16383},
	{"AmoebaTime", GD_TYPE_INT, 0, N_("Slow growth time (s)"), G_STRUCT_OFFSET(Cave, amoeba_slow_growth_time), 1, N_("After this time, amoeba will grow very quick."), 999, 0, 600},
	{"AmoebaProperties.immediately", GD_TYPE_BOOLEAN, 0, N_("Timer started immediately"), G_STRUCT_OFFSET(Cave, amoeba_timer_started_immediately), 1, N_("If this flag is enabled, the amoeba slow growth timer will start at the beginning of the cave, regardless of the amoeba being let free or not."), TRUE},
	{"AmoebaProperties.waitforhatching", GD_TYPE_BOOLEAN, 0, N_("Timer waits for hatching"), G_STRUCT_OFFSET(Cave, amoeba_timer_wait_for_hatching), 1, N_("This determines if the amoeba timer starts before the player appearing. Amoeba can always be activated before that; but if this is set to true, the timer will not start."), FALSE},
	{"AmoebaGrowthProb", GD_TYPE_PROBABILITY, 0, N_("Growth ratio, slow (%)"), G_STRUCT_OFFSET(Cave, amoeba_growth_prob), 1, N_("This sets the speed at which a slow amoeba grows."), 31250},
	{"AmoebaGrowthProb", GD_TYPE_PROBABILITY, 0, N_("Growth ratio, fast (%)"), G_STRUCT_OFFSET(Cave, amoeba_fast_growth_prob), 1, N_("This sets the speed at which a fast amoeba grows."), 250000},
	{"AmoebaProperties", GD_TYPE_ELEMENT, 0, N_("If too big, converts to"), G_STRUCT_OFFSET(Cave, too_big_amoeba_to), 1, N_("Controls which element an overgrown amoeba converts to."), O_STONE},
	{"AmoebaProperties", GD_TYPE_ELEMENT, 0, N_("If enclosed, converts to"), G_STRUCT_OFFSET(Cave, enclosed_amoeba_to), 1, N_("Controls which element an enclosed amoeba converts to."), O_DIAMOND},

	/* ACTIVE 2 */
	{"", GD_TAB, 0, N_("More elements")},
	/* slime */
	{"", GD_LABEL, 0, N_("<b>Slime</b>")},
	{"SlimePredictable", GD_TYPE_BOOLEAN, GD_DONT_SAVE, N_("Predictable"), G_STRUCT_OFFSET(Cave, slime_predictable), 1, N_("Controls if the predictable random generator is used for slime. It is required for compatibility with some older caves."), TRUE},
	{"SlimePermeabilityC64", GD_TYPE_INT, GD_DONT_SAVE, N_("Permeability (predictable, bit pattern)"), G_STRUCT_OFFSET(Cave, slime_permeability_c64), 1, N_("This controls the rate at which elements go through the slime. This one is for predictable slime, and the value is used for a bitwise AND function."), 0, 0, 255},
	{"SlimePermeability", GD_TYPE_PROBABILITY, GD_DONT_SAVE, N_("Permeability (unpredictable, %)"), G_STRUCT_OFFSET(Cave, slime_permeability), 1, N_("This controls the rate at which elements go through the slime. Higher values represent higher probability of passing. This one is for unpredictable slime."), 1000000},
	{"SlimeProperties", GD_TYPE_ELEMENT, 0, N_("Eats this and converts to"), G_STRUCT_OFFSET(Cave, slime_eats_1), 1, N_("Slime can let other elements than stone and diamond go through. It always lets a waiting or a chasing stone pass, though."), O_DIAMOND},
	{"SlimeProperties", GD_TYPE_ELEMENT, 0, N_("Eats this and converts to"), G_STRUCT_OFFSET(Cave, slime_converts_1), 1, N_("Slime can let other elements than stone and diamond go through. It always lets a waiting or a chasing stone pass, though."), O_DIAMOND_F},
	{"SlimeProperties", GD_TYPE_ELEMENT, 0, N_("Eats this and converts to"), G_STRUCT_OFFSET(Cave, slime_eats_2), 1, N_("Slime can let other elements than stone and diamond go through. It always lets a waiting or a chasing stone pass, though."), O_STONE},
	{"SlimeProperties", GD_TYPE_ELEMENT, 0, N_("Eats this and converts to"), G_STRUCT_OFFSET(Cave, slime_converts_2), 1, N_("Slime can let other elements than stone and diamond go through. It always lets a waiting or a chasing stone pass, though."), O_STONE_F},
	/* acid */
	{"", GD_LABEL, 0, N_("<b>Acid</b>")},
	{"AcidProperties", GD_TYPE_ELEMENT, 0, N_("Eats this element"), G_STRUCT_OFFSET(Cave, acid_eats_this), 1, N_("The element which acid eats. If it cannot find any, it simply disappears."), O_DIRT},
	{"AcidProperties", GD_TYPE_PROBABILITY, 0, N_("Spread ratio (%)"), G_STRUCT_OFFSET(Cave, acid_spread_ratio), 1, N_("The probability at which an acid will explode and eat neighbouring elements."), 31250},
	{"ACIDEffect", GD_TYPE_EFFECT, 0, N_("Turns to this when spreading"), G_STRUCT_OFFSET(Cave, acid_turns_to), 1, N_("If acid converts to explosion on spreading or not."), O_EXPLODE_3},
	/* biter */
	{"", GD_LABEL, 0, N_("<b>Biter</b>")},
	{"BiterProperties", GD_TYPE_INT, 0, N_("Delay (frame)"), G_STRUCT_OFFSET(Cave, biter_delay_frame), 1, N_("Number of frames biters wait between movements."), 0, 0, 3},
	{"BiterProperties", GD_TYPE_ELEMENT, 0, N_("Eat this"), G_STRUCT_OFFSET(Cave, biter_eat), 1, N_("Biters eat this element. (They always eat dirt.)"), O_DIAMOND},
	/* bladder */
	{"", GD_LABEL, 0, N_("<b>Bladder</b>")},
	{"BladderProperties", GD_TYPE_ELEMENT, 0, N_("Bladders convert to clock by touching"), G_STRUCT_OFFSET(Cave, bladder_converts_by), 1, NULL, O_VOODOO},
	/* water */
	{"", GD_LABEL, 0, N_("<b>Water</b>")},
	{"WaterProperties.doesnotflowdown", GD_TYPE_BOOLEAN, 0, N_("Water does not flow downwards"), G_STRUCT_OFFSET(Cave, water_does_not_flow_down), 1, N_("In CrDr, the water element had the odd property that it did not flow downwards, only in other directions. This flag emulates this behaviour."), FALSE},

	/* EFFECTS */
	{"", GD_TAB, 0, N_("Effects")},
	/* creature effects */
	{"", GD_LABEL, 0, N_("<b>Creature effects</b>")},
	{"EnemyDirectionProperties.startbackwards", GD_TYPE_BOOLEAN, 0, N_("Start backwards"), G_STRUCT_OFFSET(Cave, creatures_backwards), 1, NULL, FALSE},
	{"EnemyDirectionProperties.time", GD_TYPE_INT, 0, N_("Automatically turn (s)"), G_STRUCT_OFFSET(Cave, creatures_direction_auto_change_time), 1, N_("If this is greater than zero, creatures will automatically change direction in every x seconds."), 0, 0, 999},
	{"EnemyDirectionProperties.changeathatching", GD_TYPE_BOOLEAN, 0, N_("Automatically turn on start"), G_STRUCT_OFFSET(Cave, creatures_direction_auto_change_on_start), 1, N_("If this is set to true, creatures also turn at the start signal. If false, the first change in direction occurs only later."), FALSE},
	/* cave effects */
	{"", GD_LABEL, 0, N_("<b>Cave effects</b>")},
	{"EXPLOSIONEffect", GD_TYPE_EFFECT, 0, N_("Explosions convert to"), G_STRUCT_OFFSET(Cave, explosion_to), 1, N_("This element appears in places where an explosion happens."), O_SPACE},
	{"DIAMONDBIRTHEffect", GD_TYPE_EFFECT, 0, N_("Diamond births convert to"), G_STRUCT_OFFSET(Cave, diamond_birth_to), 1, NULL, O_DIAMOND},
	{"BOMBEXPLOSIONeffect", GD_TYPE_EFFECT, 0, N_("Bombs explode to"), G_STRUCT_OFFSET(Cave, bomb_explode_to), 1, NULL, O_BRICK},
	{"BOULDERfallingeffect", GD_TYPE_EFFECT, 0, N_("Falling stones convert to"), G_STRUCT_OFFSET(Cave, falling_stone_to), 1, N_("When a stone begins falling, it converts to this element."), O_STONE_F},
	{"BOULDERbouncingeffect", GD_TYPE_EFFECT, 0, N_("Bouncing stones convert to"), G_STRUCT_OFFSET(Cave, bouncing_stone_to), 1, N_("When a stone stops falling and rolling, it converts to this element."), O_STONE},
	{"DIAMONDfallingeffect", GD_TYPE_EFFECT, 0, N_("Falling diamonds convert to"), G_STRUCT_OFFSET(Cave, falling_diamond_to), 1, N_("When a diamond begins falling, it converts to this element."), O_DIAMOND_F},
	{"DIAMONDbouncingeffect", GD_TYPE_EFFECT, 0, N_("Bouncing diamonds convert to"), G_STRUCT_OFFSET(Cave, bouncing_diamond_to), 1, N_("When a diamond stops falling and rolling, it converts to this element."), O_DIAMOND},
	/* visual effects */
	{"", GD_LABEL, 0, N_("<b>Visual effects</b>")},
	{"EXPANDINGWALLLOOKSLIKEeffect", GD_TYPE_EFFECT, 0, N_("Expanding wall looks like"), G_STRUCT_OFFSET(Cave, expanding_wall_looks_like), 1, NULL, O_BRICK},
	{"DIRTLOOKSLIKEeffect", GD_TYPE_EFFECT, 0, N_("Dirt looks like"), G_STRUCT_OFFSET(Cave, dirt_looks_like), 1, NULL, O_DIRT},
	/* gravity */
	{"", GD_LABEL, 0, N_("<b>Gravitation effects</b>")},
	{"Gravitation", GD_TYPE_DIRECTION, 0, N_("Gravitation"), G_STRUCT_OFFSET(Cave, gravity), 1, N_("The direction where stones and diamonds fall."), MV_DOWN},
	{"SkeletonsForPot", GD_TYPE_INT, 0, N_("Skeletons needed for pot"), G_STRUCT_OFFSET(Cave, skeletons_needed_for_pot), 1, N_("The number of skeletons to be collected to be able to use a pot."), 5, 0, 50},
	{"GravitationChangeDelay", GD_TYPE_INT, 0, N_("Gravitation switch delay"), G_STRUCT_OFFSET(Cave, gravity_change_time), 1, N_("The gravitation changes after a while using the gravitation switch. This option sets the number of seconds to wait."), 10, 1, 60},

	/* COMPATIBILITY */
	{"", GD_TAB, 0, N_("Compatibility")},
	{"BorderProperties.lineshift", GD_TYPE_BOOLEAN, 0, N_("Line shifting border"), G_STRUCT_OFFSET(Cave, lineshift), 1, N_("If this is set to true, the player exiting on either side will appear one row lower or upper on the other side."), FALSE},
	{"ShortExplosions", GD_TYPE_BOOLEAN, 0, N_("Short explosions"), G_STRUCT_OFFSET(Cave, short_explosions), 1, N_("In 1stB, explosions were longer, took five cave frames to complete, as opposed to four in the original."), TRUE},
	{"SkeletonsWorthDiamonds", GD_TYPE_INT, 0, N_("Skeletons worth diamonds"), G_STRUCT_OFFSET(Cave, skeletons_worth_diamonds), 1, N_("The number of diamonds each skeleton is worth."), 0, 0, 10},
	{"GravityAffectsAll", GD_TYPE_BOOLEAN, 0, N_("Gravity change affects everything"), G_STRUCT_OFFSET(Cave, gravity_affects_all), 1, N_("If this is enabled, changing the gravity will also affect bladders (moving and pushing), bladder spenders, falling walls and waiting stones. Otherwise, those elements behave as gravity was always pointing downwards."), TRUE},
	{NULL}
};





static GHashTable *name_to_element;
GdElement gd_char_to_element[256];





/* for quicksort. compares two highscores. */
int gd_highscore_compare(gconstpointer a, gconstpointer b)
{
	const GdHighScore *ha=a;
	const GdHighScore *hb=b;
	return hb->score - ha->score;
}

/* return true if score achieved is a highscore */
gboolean gd_cave_is_highscore(Cave *cave, int score)
{
	/* if score is above zero AND bigger than the last one */
	if (score>0 && score>cave->highscore[G_N_ELEMENTS(cave->highscore)-1].score)
		return TRUE;

	return FALSE;
}

int
gd_cave_add_highscore(Cave *cave, GdHighScore hs)
{
	int i;
	
	if (!gd_cave_is_highscore(cave, hs.score))
		return -1;
		
	/* overwrite the last one */
	g_memmove(&cave->highscore[G_N_ELEMENTS(cave->highscore)-1], &hs, sizeof(hs));
	/* and sort */
	qsort(cave->highscore, G_N_ELEMENTS(cave->highscore), sizeof(GdHighScore), gd_highscore_compare);
	
	for (i=0; i<G_N_ELEMENTS(cave->highscore); i++)
		if (g_str_equal(cave->highscore[i].name, hs.name) && cave->highscore[i].score==hs.score)
			return i;
			
	g_assert_not_reached();
	return -1;
}



/* creates the character->element conversion table; using
   the fixed-in-the-bdcff characters. later, this table
   may be filled with more elements.
 */
void
gd_create_char_to_element_table()
{
	int i;
	
	/* fill all with unknown */
	for (i=0; i<G_N_ELEMENTS(gd_char_to_element); i++)
		gd_char_to_element[i]=O_UNKNOWN;

	/* then set fixed characters */
	for (i=0; i<O_MAX; i++) {
		int c=gd_elements[i].character;

		if (c) {
			if (gd_char_to_element[c]!=O_UNKNOWN)
				g_warning("Character %c already used for element %x", c, gd_char_to_element[c]);
			gd_char_to_element[c]=i;
		}
	}
}


/* for the case-insensitive hash keys */
gboolean
gd_str_case_equal(gconstpointer s1, gconstpointer s2)
{
	return g_ascii_strcasecmp(s1, s2)==0;
}

guint
gd_str_case_hash(gconstpointer v)
{
	char *upper;
	guint hash;

	upper=g_ascii_strup(v, -1);
	hash=g_str_hash(v);
	g_free(upper);
	return hash;
}





/*
	do some init; this function is to be called at the start of the application
*/
void
gd_cave_init ()
{
	int i;
	gboolean cells[NUM_OF_CELLS];
	
	for (i=0; i<NUM_OF_CELLS; i++)
		cells[i]=FALSE;
		
	g_assert(GD_SCHEDULING_MAX==G_N_ELEMENTS(gd_scheduling_filename));
	g_assert(GD_SCHEDULING_MAX==G_N_ELEMENTS(gd_scheduling_name));

	/* check element database for faults. */
	for (i=0; i < G_N_ELEMENTS (gd_elements); i++) {
		int j, m;
		
		if (gd_elements[i].element!=i)
			g_critical ("element: i:0x%x!=0x%x", i, gd_elements[i].element);
		/* if it has a name, create a lowercase name (of the translated one). will be used by the editor */
		if (gd_elements[i].name)
			/* the function allocates a new string, but it is needed as long as the app is running */
			gd_elements[i].lowercase_name=g_utf8_strdown(gettext(gd_elements[i].name), -1);
		
		/* we do not like generated pixbufs for games. only those that are in the png. */
		if (ABS(gd_elements[i].image_game)>NUM_OF_CELLS_X*NUM_OF_CELLS_Y)
			g_critical ("game pixbuf for element %x (%s) bigger than png size", i, gd_elements[i].name);
	
		if (gd_elements[i].image<0)
			g_critical ("editor pixbuf for element %x (%s) should not be animated", i, gd_elements[i].name);
			
		m=gd_elements[i].image<0?8:1;
		for (j=0; j<m; j++)
			cells[ABS(gd_elements[i].image)+j]=TRUE;
		m=gd_elements[i].image_simple<0?8:1;
		for (j=0; j<m; j++)
			cells[ABS(gd_elements[i].image_simple)+j]=TRUE;
		m=gd_elements[i].image_game<0?8:1;
		for (j=0; j<m; j++)
			cells[ABS(gd_elements[i].image_game)+j]=TRUE;
	}

	/* uncomment this, to print free indexes in cells array */
	/*
	g_print("Free pixbuf indexes: ");
	for (i=NUM_OF_CELLS_X*NUM_OF_CELLS_Y; i<NUM_OF_CELLS; i++) {
		if (cells[i]==FALSE)
			g_print("%d ", i);
	}
	g_print("\n");
	*/
	
	/* uncomment this, to show free element->character characters. */
	/*
	gd_create_char_to_element_table();
	g_print("Free characters: ");
	for (i=32; i<128; i++)
		if (gd_char_to_element[i]==O_UNKNOWN)
			g_print("%c", i);
	g_print("\n");
	*/

	/* check if any of the properties are designated as string arrays. they are not supported in
	 * file read/write and operations, also they do not even make any sense! */
	for (i = 0; gd_cave_properties[i].identifier!=NULL; i++) {
		if ((gd_cave_properties[i].type==GD_LABEL || gd_cave_properties[i].type==GD_TAB) && strcmp (gd_cave_properties[i].identifier, "")!=0) {
			g_critical ("ui lines in cave properties should not have identifiers: %s", gd_cave_properties[i].identifier);
			g_assert_not_reached();
		}
		if (gd_cave_properties[i].type==GD_TYPE_STRING && gd_cave_properties[i].count!=1) {
			g_critical ("string arrays not supported in cave properties: %s", gd_cave_properties[i].identifier);
			g_assert_not_reached();
		}
		if (gd_cave_properties[i].type==GD_TYPE_EFFECT && gd_cave_properties[i].count!=1) {
			g_critical ("effect arrays not supported in cave properties: %s", gd_cave_properties[i].identifier);
			g_assert_not_reached();
		}
	}

	/* put characters and names to array and hash table */
	/* this is a helper for file read operations */
	name_to_element=g_hash_table_new_full(gd_str_case_hash, gd_str_case_equal, g_free, NULL);

	for (i=0; i<O_MAX; i++) {
		char *key;
		g_assert(gd_elements[i].filename!=NULL && !g_str_equal(gd_elements[i].filename, ""));

		key=g_ascii_strup(gd_elements[i].filename, -1);

		if (g_hash_table_lookup_extended(name_to_element, key, NULL, NULL))
			g_warning("Name %s already used for element %x", key, i);
		g_hash_table_insert(name_to_element, key, GINT_TO_POINTER(i));
		/* ^^^ do not free "key", as hash table needs it during the whole time! */

		key=g_strdup_printf("SCANNED_%s", key);		/* new string */
		g_hash_table_insert(name_to_element, key, GINT_TO_POINTER(i));
		/* once again, do not free "key" ^^^ */
	}
	/* for compatibility with tim stridmann's memorydump->bdcff converter... .... ... */
	g_hash_table_insert(name_to_element, "HEXPANDING_WALL", GINT_TO_POINTER(O_H_GROWING_WALL));
	g_hash_table_insert(name_to_element, "FALLING_DIAMOND", GINT_TO_POINTER(O_DIAMOND_F));
	g_hash_table_insert(name_to_element, "FALLING_BOULDER", GINT_TO_POINTER(O_STONE_F));
	g_hash_table_insert(name_to_element, "EXPLOSION1S", GINT_TO_POINTER(O_EXPLODE_1));
	g_hash_table_insert(name_to_element, "EXPLOSION2S", GINT_TO_POINTER(O_EXPLODE_2));
	g_hash_table_insert(name_to_element, "EXPLOSION3S", GINT_TO_POINTER(O_EXPLODE_3));
	g_hash_table_insert(name_to_element, "EXPLOSION4S", GINT_TO_POINTER(O_EXPLODE_4));
	g_hash_table_insert(name_to_element, "EXPLOSION5S", GINT_TO_POINTER(O_EXPLODE_5));
	g_hash_table_insert(name_to_element, "EXPLOSION1D", GINT_TO_POINTER(O_PRE_DIA_1));
	g_hash_table_insert(name_to_element, "EXPLOSION2D", GINT_TO_POINTER(O_PRE_DIA_2));
	g_hash_table_insert(name_to_element, "EXPLOSION3D", GINT_TO_POINTER(O_PRE_DIA_3));
	g_hash_table_insert(name_to_element, "EXPLOSION4D", GINT_TO_POINTER(O_PRE_DIA_4));
	g_hash_table_insert(name_to_element, "EXPLOSION5D", GINT_TO_POINTER(O_PRE_DIA_5));
	g_hash_table_insert(name_to_element, "WALL2", GINT_TO_POINTER(O_STEEL_EXPLODABLE));

	/* create table to show errors at the start of the application */
	gd_create_char_to_element_table();
}


GdColor
gd_get_color_from_string (const char *color)
{
	int i, r, g, b;

	for (i=0; i<16; i++)
		if (g_ascii_strcasecmp(color, gd_c64_colors[i].name)==0)
			return gd_c64_colors[i].rgb;

	if (color[0]=='#')
		color++;
	if (sscanf(color, "%02x%02x%02x", &r, &g, &b)!=3) {
		i=g_random_int_range(0, 16);
		g_warning("Unkonwn color %s, using randomly chosen %s\n", color, gd_c64_colors[i].name);
		return gd_c64_colors[i].rgb;
	}
	return (r<<16)+(g<<8)+b;
}

const char*
gd_get_color_name (GdColor color)
{
	static char text[16];
	int i;

	for (i=0; i<G_N_ELEMENTS(gd_c64_colors); i++) {
		if (gd_c64_colors[i].rgb==color)
			return gd_c64_colors[i].name;
	}
	sprintf(text, "%02x%02x%02x", (color>>16)&255, (color>>8)&255, color&255);
	return text;
}

int
gd_get_c64_color_index (GdColor color)
{
	int i;

	for (i=0; i<G_N_ELEMENTS(gd_c64_colors); i++) {
		if (gd_c64_colors[i].rgb==color)
			return i;
	}
	g_warning("non-c64 color %02x%02x%02x", (color>>16)&255, (color>>8)&255, color&255);
	return g_random_int_range(1, 8);
}



/**
	load default values from description array
*/
void
gd_cave_set_defaults (Cave* cave)
{
	int i;
	
	for (i=0; gd_cave_properties[i].identifier!=NULL; i++) {
		gpointer pvalue=G_STRUCT_MEMBER_P(cave, gd_cave_properties[i].offset);
		int *ivalue=pvalue;	/* these point to the same, but to avoid the awkward cast syntax */
		GdElement *evalue=pvalue;
		GdDirection *dvalue=pvalue;
		GdScheduling *svalue=pvalue;
		gboolean *bvalue=pvalue;
		GdColor *cvalue=pvalue;
		double *fvalue=pvalue;
		int j;
		
		if (gd_cave_properties[i].type==GD_TYPE_STRING)
			continue;
			
		for (j=0; j<gd_cave_properties[i].count; j++)
			switch (gd_cave_properties[i].type) {
			case GD_TAB:
			case GD_LABEL:
			case GD_LEVEL_LABEL:
				/* these are for the gui; do nothing */
				break;
			case GD_TYPE_STRING:
				/* no default value for strings */
				break;
			case GD_TYPE_INT:
			case GD_TYPE_RATIO:	/* this is also an integer */
				ivalue[j]=gd_cave_properties[i].defval;
				break;
			case GD_TYPE_PROBABILITY:	/* floats are stored as integer, /million */
				fvalue[j]=gd_cave_properties[i].defval/1000000.0;
				break;
			case GD_TYPE_BOOLEAN:
				bvalue[j]=gd_cave_properties[i].defval!=0;
				break;
			case GD_TYPE_ELEMENT:
			case GD_TYPE_EFFECT:
				evalue[j]=(GdElement) gd_cave_properties[i].defval;
				break;
			case GD_TYPE_COLOR:
				cvalue[j]=gd_c64_colors[gd_cave_properties[i].defval].rgb;
				break;
			case GD_TYPE_DIRECTION:
				dvalue[j]=(GdDirection) gd_cave_properties[i].defval;
				break;
			case GD_TYPE_SCHEDULING:
				svalue[j]=(GdScheduling) gd_cave_properties[i].defval;
				break;
			}
	}

	/* these did not fit into that */
	for (i=0; i<5; i++) {
		cave->level_rand[i]=i;
		cave->level_timevalue[i]=i+1;
	}
}


/* check if cave visible part coordinates
   are outside cave sizes, or not in the right order.
   correct them if needed.
*/
void
gd_cave_correct_visible_size(Cave *cave)
{
	g_assert(cave!=NULL);

	/* change visible coordinates if they do not point to upperleft and lowerright */
	if (cave->x2<cave->x1) {
		int t=cave->x2;
		cave->x2=cave->x1;
		cave->x1=t;
	}
	if (cave->y2<cave->y1) {
		int t=cave->y2;
		cave->y2=cave->y1;
		cave->y1=t;
	}
	if (cave->x1<0)
		cave->x1=0;
	if (cave->y1<0)	
		cave->y1=0;
	if (cave->x2>cave->w-1)
		cave->x2=cave->w-1;
	if (cave->y2>cave->h-1)
		cave->y2=cave->h-1;
}



/**
	create new cave with default values.
	sets every value, also default size, diamond value etc.
*/
Cave *
gd_cave_new(void)
{
	Cave *cave=g_new0 (Cave, 1);

	/* hash table which stores unknown tags as strings. */
	cave->tags=g_hash_table_new_full(gd_str_case_hash, gd_str_case_equal, g_free, g_free);

	gd_cave_set_defaults (cave);

	return cave;
}

/*
  select random colors for a given cave.
  this function will select colors so that they should look somewhat nice; for example
  brick walls won't be the darkest colour, for example.
*/
void
gd_cave_set_random_colors(Cave *cave)
{
	const int bright_colors[]={1, 3, 7};
	const int dark_colors[]={2, 6, 8, 9, 11};

	cave->color0=gd_c64_colors[0].rgb;
	cave->color3=gd_c64_colors[bright_colors[g_random_int_range(0, G_N_ELEMENTS(bright_colors))]].rgb;
	do {
		cave->color1=gd_c64_colors[dark_colors[g_random_int_range(0, G_N_ELEMENTS(dark_colors))]].rgb;
	} while (cave->color1==cave->color3);
	do {
		cave->color2=gd_c64_colors[g_random_int_range(1, 16)].rgb;
	} while (cave->color1==cave->color2 || cave->color2==cave->color3);
	cave->color4=cave->color3;
	cave->color5=cave->color1;
}

/** C64 Boulder Dash I predictable random number generator.
	Used to load the original caves imported from c64 files.
	Also by the predictable slime.
*/
unsigned int
gd_c64_predictable_random (Cave *cave)
{
	unsigned int temp_rand_1, temp_rand_2, carry, result;

	temp_rand_1=(cave->rand_seed_1&0x0001) << 7;
	temp_rand_2=(cave->rand_seed_2 >> 1)&0x007F;
	result=(cave->rand_seed_2)+((cave->rand_seed_2&0x0001) << 7);
	carry=(result >> 8);
	result=result&0x00FF;
	result=result+carry+0x13;
	carry=(result >> 8);
	cave->rand_seed_2=result&0x00FF;
	result=cave->rand_seed_1+carry+temp_rand_1;
	carry=(result >> 8);
	result=result&0x00FF;
	result=result+carry+temp_rand_2;
	cave->rand_seed_1=result&0x00FF;

	return cave->rand_seed_1;
}

/*
	bd1 and similar engines had animation bits in cave data, to set which elements to animate (firefly, butterfly, amoeba).
	animating an element also caused some delay each frame; according to my measurements, around 2.6 ms/element.
*/
void
gd_cave_set_ckdelay_extra_for_animation(Cave *cave)
{
	int x, y;
	gboolean has_amoeba=FALSE, has_firefly=FALSE, has_butterfly=FALSE, has_slime=FALSE;
	g_assert(cave->map!=NULL);

	for (y=0; y<cave->h; y++)
		for (x=0; x<cave->w; x++) {
			switch (cave->map[y][x]&~SCANNED) {
				case O_GUARD_1:
				case O_GUARD_2:
				case O_GUARD_3:
				case O_GUARD_4:
					has_firefly=TRUE;
					break;
				case O_BUTTER_1:
				case O_BUTTER_2:
				case O_BUTTER_3:
				case O_BUTTER_4:
					has_butterfly=TRUE;
					break;
				case O_AMOEBA:
					has_amoeba=TRUE;
					break;
				case O_SLIME:
					has_slime=TRUE;
					break;
			}
		}
	cave->ckdelay_extra_for_animation=0;
	if (has_amoeba)
		cave->ckdelay_extra_for_animation+=2600;
	if (has_firefly)
		cave->ckdelay_extra_for_animation+=2600;
	if (has_butterfly)
		cave->ckdelay_extra_for_animation+=2600;
	if (has_amoeba)
		cave->ckdelay_extra_for_animation+=2600;
}


/* do some init - setup some cave variables before the game. */
void
gd_cave_setup_for_game(Cave *cave)
{
	int x, y;
	
	gd_cave_set_ckdelay_extra_for_animation(cave);

	/* find the player which will be the one to scroll to at the beginning of the game (before the player's birth) */
	if (cave->active_is_first_found) {
		/* uppermost player is active */
		for (y=cave->h-1; y>=0; y--)
			for (x=cave->w-1; x>=0; x--)
				if (cave->map[y][x]==O_INBOX) {
					cave->player_x=x;
					cave->player_y=y;
				}
	} else {
		/* lowermost player is active */
		for (y=0; y<cave->h; y++)
			for (x=0; x<cave->w; x++)
				if (cave->map[y][x]==O_INBOX) {
					cave->player_x=x;
					cave->player_y=y;
				}
	}
		
	/* if automatically counting diamonds. if this was negative,
	 * the sum will be this less than the number of all the diamonds in the cave */
	if (cave->diamonds_needed<=0) {
		for (y=0; y<cave->h; y++)
			for (x=0; x<cave->w; x++)
				if (cave->map[y][x]==O_DIAMOND)
					cave->diamonds_needed++;
		if (cave->diamonds_needed<0)
			/* if still below zero, let this be 0, so gate will be open immediately */
			cave->diamonds_needed=0;
	}

	gd_cave_correct_visible_size(cave);

	/* select number of milliseconds (for pal and ntsc) */
	cave->timing_factor=cave->pal_timing?1200:1000;
	cave->time*=cave->timing_factor;
	cave->magic_wall_milling_time*=cave->timing_factor;
	cave->amoeba_slow_growth_time*=cave->timing_factor;
	/* for all c64-type scheduling types, hatching delay means seconds. otherwise, it means frames, so we do not touch it. */
	if (cave->scheduling!=GD_SCHEDULING_MILLISECONDS)
		cave->hatching_delay*=cave->timing_factor;
	if (cave->hammered_walls_reappear)
		cave->hammered_reappear=gd_cave_map_new(cave, int);
}


/**
	Put an object to the specified position.
	Performs range checking.
	order is a pointer to the GdObject describing this object. Thus the editor can identify which cell was created by which object.
*/
void
gd_cave_store_rc (Cave *cave, const int x, const int y, const GdElement element, const void* order)
{
	/* check bounds */
	if (x>=0 && x<cave->w && y>=0 && y<cave->h && element!=O_NONE) {
		cave->map[y][x]=element;
		cave->objects_order[y][x]=(void *)order;
	}
}







/* cave maps.
   cave maps are continuous areas in memory. the allocated memory
   is width*height*bytes_per_cell long.
   the cave map[0] stores the pointer given by g_malloc().
   the map itself is also an allocated array of pointers to the
   beginning of rows.
   therefore:
   		rows=new (pointers to rows);
		rows[0]=new map
		rows[1..h-1]=rows[0]+width*bytes
		
	freeing this:
		free(rows[0])
		free(rows)
*/

/**
	allocate a cave map-like array, and initialize to zero.
	one cell is cell_size bytes long.
*/
gpointer
gd_cave_map_new_for_cave(const Cave *cave, const int cell_size)
{
	gpointer *rows;				/* this is void**, pointer to array of ... */
	int y;

	rows=g_new(gpointer, cave->h);
	rows[0]=g_malloc0 (cell_size*cave->w*cave->h);
	for (y=1; y<cave->h; y++)
		/* base pointer+num_of_bytes_per_element*width*number_of_row; as sizeof(char)=1 */
		rows[y]=(char *)rows[0]+cell_size*cave->w*y;
	return rows;
}

/**
	duplicate map

	if map is null, this also returns null.
*/
gpointer
gd_cave_map_dup_size (const Cave *cave, const gpointer map, const int cell_size)
{
	gpointer *rows;
	gpointer *maplines=(gpointer *)map;
	int y;

	if (!map)
		return NULL;

	rows=g_new (gpointer, cave->h);
	rows[0]=g_memdup (maplines[0], cell_size * cave->w * cave->h);

	for (y=1; y < cave->h; y++)
		rows[y]=(char *)rows[0]+cell_size*cave->w*y;

	return rows;
}

void
gd_cave_map_free(gpointer map)
{
	gpointer *maplines=(gpointer *) map;

	if (!map)
		return;

	g_free(maplines[0]);
	g_free(map);
}

void
gd_cave_clear_highscore(Cave *cave)
{
	int i;
	
	for (i=0; i<G_N_ELEMENTS(cave->highscore); i++) {
		strcpy(cave->highscore[i].name, "");
		cave->highscore[i].score=0;
	}
}

/* clear all properties of a cave, which are strings. used when reading a bdcff file */
void
gd_cave_clear_strings(Cave *cave)
{
	int i;

	for (i=0; gd_cave_properties[i].identifier!=NULL; i++)
		if (gd_cave_properties[i].type==GD_TYPE_STRING)
			g_strlcpy(G_STRUCT_MEMBER_P(cave, gd_cave_properties[i].offset), "", sizeof(GdString));
}

/**
	frees memory associated to cave
*/
void
gd_cave_free (Cave *cave)
{
	if (!cave)
		return;

	if (cave->tags)
		g_hash_table_destroy(cave->tags);
	gd_cave_clear_highscore(cave);
	
	if (cave->random)
		g_rand_free(cave->random);

	/* map */
	gd_cave_map_free (cave->map);
	/* rendered data */
	gd_cave_map_free (cave->objects_order);
	/* free objects */
	g_list_foreach (cave->objects, (GFunc) g_free, NULL);
	g_list_free (cave->objects);

	/* hammered walls to reappear data */
	gd_cave_map_free(cave->hammered_reappear);

	/* freeing main pointer */
	g_free (cave);
}

static void
hash_copy_foreach(const char *key, const char *value, GHashTable *dest)
{
	g_hash_table_insert(dest, g_strdup(key), g_strdup(value));
}

/* copy cave from src to destination, with duplicating dynamically allocated data */
void
gd_cave_copy(Cave *dest, const Cave *src)
{
	g_memmove(dest, src, sizeof(Cave));

	/* but duplicate dynamic data */
	dest->tags=g_hash_table_new_full(gd_str_case_hash, gd_str_case_equal, g_free, g_free);
	if (src->tags)
		g_hash_table_foreach(src->tags, (GHFunc) hash_copy_foreach, dest->tags);
	dest->map=gd_cave_map_dup (src, map);
	dest->hammered_reappear=gd_cave_map_dup(src, hammered_reappear);

	/* no reason to copy this */
	dest->objects_order=NULL;

	/* copy objects list */
	if (src->objects) {
		GList *iter;

		dest->objects=NULL;	/* new empty list */
		for (iter=src->objects; iter!=NULL; iter=iter->next)		/* do a deep copy */
			dest->objects=g_list_append (dest->objects, g_memdup (iter->data, sizeof (GdObject)));
	}

	/* copy random number generator */
	if (src->random)
		dest->random=g_rand_copy(src->random);
}

/* create new cave, which is a copy of the cave given. */
Cave *
gd_cave_new_from_cave (const Cave *orig)
{
	Cave *cave;

	cave=gd_cave_new();
	gd_cave_copy(cave, orig);

	return cave;
}



/**
	shrink cave
	if last line or last row is just steel wall (or (invisible) outbox).
	used after loading a game for playing.
	after this, ew and eh will contain the effective width and height.
 */
void
gd_cave_shrink (Cave *cave)
{

	int x, y;
	enum {
		STEEL_ONLY,
		STEEL_OR_OTHER,
		NO_SHRINK
	} empty;

	/* set to maximum size, then try to shrink */
	cave->x1=0; cave->y1=0;
	cave->x2=cave->w-1; cave->y2=cave->h-1;

	/* search for empty, steel-wall-only last rows. */
	/* clear all lines, which are only steel wall.
	 * and clear only one line, which is steel wall, but also has a player or an outbox. */
	empty=STEEL_ONLY;
	do {
		for (y=cave->y2-1; y <= cave->y2; y++)
			for (x=cave->x1; x <= cave->x2; x++)
				switch (gd_cave_get_rc (cave, x, y)) {
				case O_STEEL:	/* if steels only, this is to be deleted. */
					break;
				case O_PRE_OUTBOX:
				case O_PRE_INVIS_OUTBOX:
				case O_INBOX:
					if (empty==STEEL_OR_OTHER)
						empty=NO_SHRINK;
					if (empty==STEEL_ONLY)	/* if this, delete only this one, and exit. */
						empty=STEEL_OR_OTHER;
					break;
				default:		/* anything else, that should be left in the cave. */
					empty=NO_SHRINK;
					break;
				}
		if (empty!=NO_SHRINK)	/* shrink if full steel or steel and player/outbox. */
			cave->y2--;			/* one row shorter */
	}
	while (empty==STEEL_ONLY);	/* if found just steels, repeat. */

	/* search for empty, steel-wall-only first rows. */
	empty=STEEL_ONLY;
	do {
		for (y=cave->y1; y <= cave->y1+1; y++)
			for (x=cave->x1; x <= cave->x2; x++)
				switch (gd_cave_get_rc (cave, x, y)) {
				case O_STEEL:
					break;
				case O_PRE_OUTBOX:
				case O_PRE_INVIS_OUTBOX:
				case O_INBOX:
					/* shrink only lines, which have only ONE player or outbox. this is for bd4 intermission 2, for example. */
					if (empty==STEEL_OR_OTHER)
						empty=NO_SHRINK;
					if (empty==STEEL_ONLY)
						empty=STEEL_OR_OTHER;
					break;
				default:
					empty=NO_SHRINK;
					break;
				}
		if (empty!=NO_SHRINK)
			cave->y1++;
	}
	while (empty==STEEL_ONLY);	/* if found one, repeat. */

	/* empty last columns. */
	empty=STEEL_ONLY;
	do {
		for (y=cave->y1; y <= cave->y2; y++)
			for (x=cave->x2-1; x <= cave->x2; x++)
				switch (gd_cave_get_rc (cave, x, y)) {
				case O_STEEL:
					break;
				case O_PRE_OUTBOX:
				case O_PRE_INVIS_OUTBOX:
				case O_INBOX:
					if (empty==STEEL_OR_OTHER)
						empty=NO_SHRINK;
					if (empty==STEEL_ONLY)
						empty=STEEL_OR_OTHER;
					break;
				default:
					empty=NO_SHRINK;
					break;
				}
		if (empty!=NO_SHRINK)
			cave->x2--;			/* just remember that one column shorter. g_free will know the size of memchunk, no need to realloc! */
	}
	while (empty==STEEL_ONLY);	/* if found one, repeat. */

	/* empty first columns. */
	empty=STEEL_ONLY;
	do {
		for (y=cave->y1; y <= cave->y2; y++)
			for (x=cave->x1; x <= cave->x1+1; x++)
				switch (gd_cave_get_rc (cave, x, y)) {
				case O_STEEL:
					break;
				case O_PRE_OUTBOX:
				case O_PRE_INVIS_OUTBOX:
				case O_INBOX:
					if (empty==STEEL_OR_OTHER)
						empty=NO_SHRINK;
					if (empty==STEEL_ONLY)
						empty=STEEL_OR_OTHER;
					break;
				default:
					empty=NO_SHRINK;
					break;
				}
		if (empty!=NO_SHRINK)
			cave->x1++;
	}
	while (empty==STEEL_ONLY);	/* if found one, repeat. */
}











/* create an easy level.
	for now,
		invisible outbox -> outbox,
		clear expanding wall;
		only one diamond needed.
		time min 600s.
	*/

void
gd_cave_easy (Cave *cave)
{
	int x, y;
	
	g_assert(cave->map!=NULL);

	for (x=0; x<cave->w; x++)
		for (y=0; y<cave->h; y++)
			switch (gd_cave_get_rc(cave, x, y)) {
			case O_PRE_INVIS_OUTBOX:
				cave->map[y][x]=O_PRE_OUTBOX;
				break;
			case O_INVIS_OUTBOX:
				cave->map[x][x]=O_OUTBOX;
				break;
			case O_H_GROWING_WALL:
			case O_V_GROWING_WALL:
			case O_GROWING_WALL:
				cave->map[y][x]=O_BRICK;
				break;
			default:
				break;
			}
	if (cave->diamonds_needed>0)
		cave->diamonds_needed=1;
	if (cave->time < 600)
		cave->time=600;
}




/* search the element database for the specified character, and return the element. */
GdElement
gd_get_element_from_character (guint8 character)
{
	if (gd_char_to_element[character]!=O_UNKNOWN)
		return gd_char_to_element[character];

	g_warning ("Invalid character representing element: %c", character);
	return O_UNKNOWN;
}

/* search the element database for the specified name, and return the element */
GdElement
gd_get_element_from_string (const char *string)
{
	char *upper=g_ascii_strup(string, -1);
	gpointer value;
	gboolean found;

	found=g_hash_table_lookup_extended(name_to_element, upper, NULL, &value);
	g_free(upper);
	if (found)
		return (GdElement) (GPOINTER_TO_INT(value));

	g_warning("Invalid string representing element: %s", string);
	return O_UNKNOWN;
}





/* this one only updates the visible area! */
void
gd_drawcave_game(const Cave *cave, int **gfx_buffer, gboolean bonus_life_flash, gboolean paused)
{
	static int player_blinking=0;
	static int player_tapping=0;
	static int animcycle=0;
	int elemdrawing[O_MAX];
	int x, y, draw;

	g_assert(cave!=NULL);
	g_assert(cave->map!=NULL);
	g_assert(gfx_buffer!=NULL);
	
	animcycle=(animcycle+1) & 7;
	if (cave->last_direction) {	/* he is moving, so stop blinking and tapping. */
		player_blinking=0;
		player_tapping=0;
	}
	else {						/* he is idle, so animations can be done. */
		if (animcycle == 0) {	/* blinking and tapping is started at the beginning of animation sequences. */
			player_blinking=g_random_int_range (0, 4) == 0;	/* 1/4 chance of blinking, every sequence. */
			if (g_random_int_range (0, 16) == 0)	/* 1/16 chance of starting or stopping tapping. */
				player_tapping=!player_tapping;
		}
	}

	for (x=0; x<O_MAX; x++)
		elemdrawing[x]=gd_elements[x].image_game;
	if (bonus_life_flash)
		elemdrawing[O_SPACE]=gd_elements[O_FAKE_BONUS].image_game;
	elemdrawing[O_MAGIC_WALL]=gd_elements[cave->magic_wall_state == MW_ACTIVE ? O_MAGIC_WALL : O_BRICK].image_game;
	elemdrawing[O_CREATURE_SWITCH]=gd_elements[cave->creatures_backwards ? O_CREATURE_SWITCH_ON : O_CREATURE_SWITCH].image_game;
	elemdrawing[O_GROWING_WALL_SWITCH]=gd_elements[cave->expanding_wall_changed ? O_GROWING_WALL_SWITCH_VERT : O_GROWING_WALL_SWITCH_HORIZ].image_game;
	elemdrawing[O_GRAVITY_SWITCH]=gd_elements[cave->gravity_switch_active?O_GRAVITY_SWITCH_ACTIVE:O_GRAVITY_SWITCH].image_game;
	if (animcycle&2) {
		elemdrawing[O_PNEUMATIC_ACTIVE_LEFT]+=2;	/* also a hack, like biter_switch */
		elemdrawing[O_PNEUMATIC_ACTIVE_RIGHT]+=2;
		elemdrawing[O_PLAYER_PNEUMATIC_LEFT]+=2;
		elemdrawing[O_PLAYER_PNEUMATIC_RIGHT]+=2;
	}
	
	if ((cave->last_direction) == MV_STILL) {	/* player is idle. */
		if (player_blinking && player_tapping)
			draw=gd_elements[O_PLAYER_TAP_BLINK].image_game;
		else if (player_blinking)
			draw=gd_elements[O_PLAYER_BLINK].image_game;
		else if (player_tapping)
			draw=gd_elements[O_PLAYER_TAP].image_game;
		else
			draw=gd_elements[O_PLAYER].image_game;
	}
	else if (cave->last_horizontal_direction == MV_LEFT)
		draw=gd_elements[O_PLAYER_LEFT].image_game;
	else
		/* of course this is MV_RIGHT. */
		draw=gd_elements[O_PLAYER_RIGHT].image_game;
	elemdrawing[O_PLAYER]=draw;
	elemdrawing[O_PLAYER_GLUED]=draw;
	/* player with bomb does not blink or tap - no graphics drawn for that. running is drawn using w/o bomb cells */
	if (cave->last_direction!=MV_STILL)
		elemdrawing[O_PLAYER_BOMB]=draw;
	elemdrawing[O_INBOX]=gd_elements[cave->inbox_flash_toggle ? O_OUTBOX_OPEN : O_OUTBOX_CLOSED].image_game;
	elemdrawing[O_OUTBOX]=gd_elements[cave->inbox_flash_toggle ? O_OUTBOX_OPEN : O_OUTBOX_CLOSED].image_game;
	elemdrawing[O_BITER_SWITCH]=gd_elements[O_BITER_SWITCH].image_game+cave->biter_delay_frame;	/* XXX hack, not fit into gd_elements */
	/* visual effects */
	elemdrawing[O_DIRT]=elemdrawing[cave->dirt_looks_like];
	elemdrawing[O_GROWING_WALL]=elemdrawing[cave->expanding_wall_looks_like];
	elemdrawing[O_V_GROWING_WALL]=elemdrawing[cave->expanding_wall_looks_like];
	elemdrawing[O_H_GROWING_WALL]=elemdrawing[cave->expanding_wall_looks_like];

	for (y=cave->y1; y<=cave->y2; y++) {
		for (x=cave->x1; x<=cave->x2; x++) {
			GdElement actual=cave->map[y][x];

			/* if covered, real element is not important */
			if (actual & COVERED)
				draw=gd_elements[O_COVERED].image_game;
			else
				draw=elemdrawing[actual];

			/* if negative, animated. */
			if (draw<0)
				draw=-draw+animcycle;
			/* flash */
			if (cave->gate_open_flash || paused)
				draw+=NUM_OF_CELLS;

			/* set to buffer, with caching */
			if (gfx_buffer[y][x]!=draw)
				gfx_buffer[y][x]=draw | GD_REDRAW;
		}
	}
}

/*
	width: width of playfield.
	visible: visible part. (remember: player_x-x1!)
	
	center: the coordinates to scroll to.
	exact: scroll exactly
	start: start scrolling
	to: scroll to, if started
	current

	desired: the function stores its data here
	speed: the function stores its data here	
*/
gboolean
gd_cave_scroll(int width, int visible, int center, gboolean exact, int start, int to, int *current, int *desired, int *speed)
{
	int i;
	gboolean changed;
	
	changed=FALSE;

	/* HORIZONTAL */
	/* hystheresis function.
	 * when scrolling left, always go a bit less left than player being at the middle.
	 * when scrolling right, always go a bit less to the right. */
	if (width<visible) {
		*speed=0;
		*desired=0;
		if (*current!=0) {
			*current=0;
			changed=TRUE;
		}
		
		return changed;
	}
	
	if (exact)
		*desired=center;
	else {
		if (*current+start<center)
			*desired=center-to;
		if (*current-start>center)
			*desired=center+to;
	}
	*desired=CLAMP(*desired, 0, width-visible);

	/* adaptive scrolling speed.
	 * gets faster with distance.
	 * minimum speed is 1, to allow scrolling precisely to the desired positions (important at borders).
	 */
	if (*speed<ABS (*desired-*current)/12+1)
		(*speed)++;
	if (*speed>ABS (*desired-*current)/12+1)
		(*speed)--;
	if (*current < *desired) {
		for (i=0; i < *speed; i++)
			if (*current < *desired)
				(*current)++;
		changed=TRUE;
	}
	if (*current > *desired) {
		for (i=0; i < *speed; i++)
			if (*current > *desired)
				(*current)--;
		changed=TRUE;
	}
	
	return changed;
}


