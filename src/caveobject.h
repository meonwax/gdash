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
#ifndef _GD_CAVEOBJECT_H
#define _GD_CAVEOBJECT_H

#include "cave.h"

typedef enum _object_enum {
	NONE,				/* this one to be zero. */
	POINT,				/* single point of object1 */
	LINE,				/* line from (1) to (2) of object1 */
	RECTANGLE,			/* rectangle with corners (1) and (2) of object1 */
	FILLED_RECTANGLE,	/* rectangle with corners (1) and (2) of object1, filled with object2 */
	RASTER,				/* aligned plots */
	JOIN,				/* every object1 has an object2 next to it, relative (dx,dy) */
	FLOODFILL_REPLACE,	/* fill by replacing */
	FLOODFILL_BORDER,	/* fill to another element, a border */
	MAZE,				/* maze */
	MAZE_UNICURSAL,		/* unicursal maze */
	MAZE_BRAID,			/* braid maze */
	RANDOM_FILL,		/* random fill */
} GdObjectType;

typedef enum _gd_object_levels {
	GD_OBJECT_LEVEL1=1<<0,
	GD_OBJECT_LEVEL2=1<<1,
	GD_OBJECT_LEVEL3=1<<2,
	GD_OBJECT_LEVEL4=1<<3,
	GD_OBJECT_LEVEL5=1<<4,
	GD_OBJECT_LEVEL_ALL=GD_OBJECT_LEVEL1|GD_OBJECT_LEVEL2|GD_OBJECT_LEVEL3|GD_OBJECT_LEVEL4|GD_OBJECT_LEVEL5,
} GdObjectLevels;

extern GdObjectLevels gd_levels_mask[];

typedef struct _object {
	GdObjectType type;		/* type */
	GdObjectLevels levels;	/* levels to show this object on */
	
	int x1, y1;				/* (first) coordinate */
	int x2, y2;				/* second coordinate */
	int dx, dy;				/* distance of elements for raster or join */
	GdElement element, fill_element;		/* element type */
	gint32 seed[5];			/* for maze and random fill */
	int horiz;				/* for maze */

	gboolean c64_random;	/* random fill objects: use c64 random generator */	
	GdElement random_fill[4];
	int random_fill_probability[4];
} GdObject;

typedef struct _objdesc {
	char *name;
	char *x1;
	char *x2;
	char *dx;
	char *seed;
	char *element;
	char *fill_element;
	char *first_button, *second_button;
	char *horiz;
	char *c64_random;
} GdObjectDescription;

extern GdObjectDescription gd_object_description[];


void gd_cave_draw_object (Cave *cave, const GdObject *object, int level);
char *gd_object_to_bdcff(const GdObject *object);
char *gd_get_object_coordinates_text (GdObject *selected);
char *gd_get_object_description_markup (GdObject *selected);
GdObject *gd_object_new_from_string(char *str);

Cave *gd_cave_new_rendered(const Cave * data, const int level, guint32 seed);
void gd_flatten_cave (Cave * cave, const int level);

#endif	/* CAVEOBJECT.H */

