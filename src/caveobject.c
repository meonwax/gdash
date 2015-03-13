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
#include "cave.h"
#include "cavedb.h"
#include "caveobject.h"



/* description of coordinates, elements - used by editor properties dialog. */
GdObjectDescription gd_object_description[]={
	{ /* none */  NULL },
	{ /* point */ N_("Point"), N_("Coordinates"), NULL, NULL, NULL, N_("Element"), NULL, N_("Draw"), NULL },
	{ /* line */  N_("Line"), N_("Starting coordinates"), N_("Ending coordinates"), NULL, NULL, N_("Element"), NULL, N_("Draw"), NULL },
	{ /* rect */  N_("Outline"), N_("Starting coordinates"), N_("Ending coordinates"), NULL, NULL, N_("Element"), NULL, N_("Draw"), NULL },
	{ /* frect */ N_("Rectangle"), N_("Starting coordinates"), N_("Ending coordinates"), NULL, NULL, N_("Border element"), N_("Fill element"), N_("Draw"), N_("Fill") }, 
	{ /* rastr */ N_("Raster"), N_("Starting coordinates"), N_("Ending coordinates"), N_("Distance"), NULL, N_("Element"), NULL, N_("Draw"), NULL },
	{ /* join */  N_("Join"), NULL, NULL, N_("Distance"), NULL, N_("Search element"), N_("Add element"), N_("Find"), N_("Draw") },
	{ /* flodr */ N_("Replace fill"), N_("Starting coordinates"), NULL, NULL, NULL, N_("Search element"), N_("Fill element"), NULL, N_("Replace") },
	{ /* flodb */ N_("Fill to border"), N_("Starting coordinates"), NULL, NULL, NULL, N_("Border element"), N_("Fill element"), N_("Border"), N_("Fill")  },
	{ /* maze */  N_("Maze"), N_("Starting coordinates"), N_("Ending coordinates"), N_("Wall and path"), N_("Random seed %d"), N_("Wall element"), N_("Path element"), N_("Wall"), N_("Path") , N_("Horizontal (%)")},
	{ /* umaze */ N_("Unicursal maze"), N_("Starting coordinates"), N_("Ending coordinates"), N_("Wall and path"), N_("Random seed %d"), N_("Wall element"), N_("Path element"), N_("Wall"), N_("Path") , N_("Horizontal (%)")},
	{ /* Bmaze */ N_("Braid maze"), N_("Starting coordinates"), N_("Ending coordinates"), N_("Wall and path"), N_("Random seed %d"), N_("Wall element"), N_("Path element"), N_("Wall"), N_("Path") , N_("Horizontal (%)")},
	{ /* random */N_("Random fill"), N_("Starting coordinates"), N_("Ending coordinates"), NULL, N_("Random seed %d"), N_("Replace only this element"), NULL, N_("Random 1"), N_("Initial"), NULL},
};


GdObjectLevels gd_levels_mask[]={GD_OBJECT_LEVEL1, GD_OBJECT_LEVEL2, GD_OBJECT_LEVEL3, GD_OBJECT_LEVEL4, GD_OBJECT_LEVEL5};




/* bdcff text description of object. caller should free string. */
char *
gd_object_to_bdcff(const GdObject *object)
{
	GString *str;
	int j;
	const char *type;
	
	switch (object->type) {
	case POINT:
		return g_strdup_printf ("Point=%d %d %s", object->x1, object->y1, gd_elements[object->element].filename);

	case LINE:
		return g_strdup_printf ("Line=%d %d %d %d %s", object->x1, object->y1, object->x2, object->y2, gd_elements[object->element].filename);

	case RECTANGLE:
		return g_strdup_printf ("Rectangle=%d %d %d %d %s", object->x1, object->y1, object->x2, object->y2, gd_elements[object->element].filename);

	case FILLED_RECTANGLE:
		/* if elements are not the same */
		if (object->fill_element!=object->element)
			return g_strdup_printf ("FillRect=%d %d %d %d %s %s", object->x1, object->y1, object->x2, object->y2, gd_elements[object->element].filename, gd_elements[object->fill_element].filename);
		/* they are the same */
		return g_strdup_printf ("FillRect=%d %d %d %d %s", object->x1, object->y1, object->x2, object->y2, gd_elements[object->element].filename);

	case RASTER:
		return g_strdup_printf ("Raster=%d %d %d %d %d %d %s", object->x1, object->y1, (object->x2-object->x1)/object->dx+1, (object->y2-object->y1)/object->dy+1, object->dx, object->dy, gd_elements[object->element].filename);

	case JOIN:
		return g_strdup_printf ("Add=%d %d %s %s", object->dx, object->dy, gd_elements[object->element].filename, gd_elements[object->fill_element].filename);

	case FLOODFILL_BORDER:
		return g_strdup_printf ("BoundaryFill=%d %d %s %s", object->x1, object->y1, gd_elements[object->fill_element].filename, gd_elements[object->element].filename);

	case FLOODFILL_REPLACE:
		return g_strdup_printf ("FloodFill=%d %d %s %s", object->x1, object->y1, gd_elements[object->fill_element].filename, gd_elements[object->element].filename);
		
	case MAZE:
	case MAZE_UNICURSAL:
	case MAZE_BRAID:
		switch(object->type) {
			case MAZE: type="perfect"; break;
			case MAZE_UNICURSAL: type="unicursal"; break;
			case MAZE_BRAID: type="braid"; break;
			default:
				g_assert_not_reached();
		}
		return g_strdup_printf ("Maze=%d %d %d %d %d %d %d %d %d %d %d %d %s %s %s", object->x1, object->y1, object->x2, object->y2, object->dx, object->dy, object->horiz, object->seed[0], object->seed[1], object->seed[2], object->seed[3], object->seed[4], gd_elements[object->element].filename, gd_elements[object->fill_element].filename, type);
		
	case RANDOM_FILL:
		str=g_string_new(NULL);
		g_string_append_printf(str, "RandomFill=%d %d %d %d %d %d %d %d %d %s", object->x1, object->y1, object->x2, object->y2, object->seed[0], object->seed[1], object->seed[2], object->seed[3], object->seed[4], gd_elements[object->fill_element].filename);	/* seed and initial fill */
		for (j=0; j<4; j++)
			if (object->random_fill_probability[j]!=0)
				g_string_append_printf(str, " %s %d", gd_elements[object->random_fill[j]].filename, object->random_fill_probability[j]);
		if (object->element!=O_NONE)
			g_string_append_printf(str, " %s", gd_elements[object->element].filename);
		
		/* free string but do not free char *; return char *. */
		return g_string_free(str, FALSE);
	
	case NONE:
		g_assert_not_reached();
	}
	
	return NULL;
}

/* create new object from bdcff description.
   return new object if ok; return null if failed.
 */
GdObject *
gd_object_new_from_string(char *str)
{
	char *equalsign;
	char *name, *param;
	GdObject object;
	char elem0[100], elem1[100];
	
	equalsign=strchr(str, '=');
	if (!equalsign)
		return NULL;

	/* split string by replacing the equal sign with zero */
	*equalsign='\0';
	name=str;
	param=equalsign+1;
	
	/* INDIVIDUAL POINT CAVE OBJECT */
	if (g_ascii_strcasecmp(name, "Point")==0) {
		object.type=POINT;
		if (sscanf(param, "%d %d %s", &object.x1, &object.y1, elem0)==3) {
			object.element=gd_get_element_from_string(elem0);
			return g_memdup(&object, sizeof (GdObject));
		}
		return NULL;
	}

	/* LINE OBJECT */
	if (g_ascii_strcasecmp(name, "Line")==0) {
		object.type=LINE;
		if (sscanf(param, "%d %d %d %d %s", &object.x1, &object.y1, &object.x2, &object.y2, elem0)==5) {
			object.element=gd_get_element_from_string(elem0);
			return g_memdup(&object, sizeof (GdObject));
		}
		return NULL;
	}

	/* RECTANGLE OBJECT */
	if (g_ascii_strcasecmp(name, "Rectangle")==0) {
		if (sscanf (param, "%d %d %d %d %s", &object.x1, &object.y1, &object.x2, &object.y2, elem0)==5) {
			object.type=RECTANGLE;
			object.element=gd_get_element_from_string (elem0);
			return g_memdup(&object, sizeof (GdObject));
		}
		return NULL;
	}

	/* FILLED RECTANGLE OBJECT */
	if (g_ascii_strcasecmp(name, "FillRect")==0) {
		int paramcount;
		
		paramcount=sscanf(param, "%d %d %d %d %s %s", &object.x1, &object.y1, &object.x2, &object.y2, elem0, elem1);
		object.type=FILLED_RECTANGLE;
		if (paramcount==6) {
			object.element=gd_get_element_from_string (elem0);
			object.fill_element=gd_get_element_from_string (elem1);
			return g_memdup (&object, sizeof (GdObject));
		}
		if (paramcount==5) {
			object.element=object.fill_element=gd_get_element_from_string (elem0);
			return g_memdup (&object, sizeof (GdObject));
		}
		return NULL;
	}

	/* RASTER */
	if (g_ascii_strcasecmp(name, "Raster")==0) {
		int nx, ny;

		if (sscanf (param, "%d %d %d %d %d %d %s", &object.x1, &object.y1, &nx, &ny, &object.dx, &object.dy, elem0)==7) {
			nx--; ny--;
			object.x2=object.x1 + nx*object.dx;
			object.y2=object.y1 + ny*object.dy;
			object.type=RASTER;
			object.element=gd_get_element_from_string (elem0);
			return g_memdup (&object, sizeof (GdObject));
		}
		return NULL;
	}

	/* JOIN */
	if (g_ascii_strcasecmp(name, "Join")==0 || g_ascii_strcasecmp(name, "Add")==0) {
		if (sscanf (param, "%d %d %s %s", &object.dx, &object.dy, elem0, elem1)==4) {
			object.type=JOIN;
			object.element=gd_get_element_from_string (elem0);
			object.fill_element=gd_get_element_from_string (elem1);
			return g_memdup (&object, sizeof (GdObject));
		}
		return NULL;
	}

	/* FILL TO BORDER OBJECT */
	if (g_ascii_strcasecmp(name, "BoundaryFill")==0) {
		if (sscanf (param, "%d %d %s %s", &object.x1, &object.y1, elem0, elem1)==4) {
			object.type=FLOODFILL_BORDER;
			object.fill_element=gd_get_element_from_string (elem0);
			object.element=gd_get_element_from_string (elem1);
			return g_memdup (&object, sizeof (GdObject));
		}
		return NULL;
	}

	/* REPLACE FILL OBJECT */
	if (g_ascii_strcasecmp(name, "FloodFill")==0) {
		if (sscanf (param, "%d %d %s %s", &object.x1, &object.y1, elem0, elem1)==4) {
			object.type=FLOODFILL_REPLACE;
			object.fill_element=gd_get_element_from_string (elem0);
			object.element=gd_get_element_from_string (elem1);
			return g_memdup (&object, sizeof (GdObject));
		}
		return NULL;
	}

	/* MAZE OBJECT */
	/* MAZE UNICURSAL OBJECT */
	/* BRAID MAZE OBJECT */
	if (g_ascii_strcasecmp(name, "Maze")==0) {
		char type[100]="perfect";
		
		if (sscanf (param, "%d %d %d %d %d %d %d %d %d %d %d %d %s %s %s", &object.x1, &object.y1, &object.x2, &object.y2, &object.dx, &object.dy, &object.horiz, &object.seed[0], &object.seed[1], &object.seed[2], &object.seed[3], &object.seed[4], elem0, elem1, type)>=14) {
			if (g_ascii_strcasecmp(type, "unicursal")==0)
				object.type=MAZE_UNICURSAL;
			else if (g_ascii_strcasecmp(type, "perfect")==0)
				object.type=MAZE;
			else if (g_ascii_strcasecmp(type, "braid")==0)
				object.type=MAZE_BRAID;
			else {
				g_warning("unknown maze type: %s, defaulting to perfect", type);
				object.type=MAZE;
			}
			object.element=gd_get_element_from_string (elem0);
			object.fill_element=gd_get_element_from_string (elem1);
			return g_memdup (&object, sizeof (GdObject));
		}
		return NULL;
	}
	
	/* RANDOM FILL OBJECT */
	if (g_ascii_strcasecmp(name, "RandomFill")==0) {
		static char **words=NULL;
		int l, i;

		object.type=RANDOM_FILL;
		if (sscanf(param, "%d %d %d %d", &object.x1, &object.y1, &object.x2, &object.y2)!=4)
			return NULL;
		if (words)
			g_strfreev(words);
		words=g_strsplit_set(param, " ", -1);
		l=g_strv_length(words);
		if (l<10 || l>19)
			return NULL;
		for (i=0; i<5; i++)
			if (sscanf(words[4+i], "%d", &object.seed[i])!=1)
				return NULL;
		object.fill_element=gd_get_element_from_string(words[9]);
		for (i=0; i<4; i++) {
			object.random_fill[i]=O_DIRT;
			object.random_fill_probability[i]=0;
		}
		for (i=10; i<l-1; i+=2) {
			object.random_fill[(i-10)/2]=gd_get_element_from_string(words[i]);
			if (sscanf(words[i+1], "%d", &object.random_fill_probability[(i-10)/2])==0)
				return NULL;
		}
		object.element=O_NONE;
		if (l>10 && l%2==1)
			object.element=gd_get_element_from_string(words[l-1]);
			
		return g_memdup (&object, sizeof (GdObject));
	}
	
	return NULL;
}



char *
gd_get_object_description_markup (GdObject *selected)
{
	g_return_val_if_fail (selected != NULL, NULL);

	switch (selected->type) {
	case POINT:
		return g_markup_printf_escaped(_("Point of <b>%s</b> at %d,%d"), gd_elements[selected->element].lowercase_name, selected->x1, selected->y1);

	case LINE:
		return g_markup_printf_escaped(_("Line from %d,%d to %d,%d of <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name);

	case RECTANGLE:
		return g_markup_printf_escaped(_("Rectangle from %d,%d to %d,%d of <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name);

	case FILLED_RECTANGLE:
		return g_markup_printf_escaped(_("Rectangle from %d,%d to %d,%d of <b>%s</b>, filled with <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, gd_elements[selected->fill_element].lowercase_name);

	case RASTER:
		return g_markup_printf_escaped(_("Raster from %d,%d to %d,%d of <b>%s</b>, distance %+d,%+d"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, selected->dx, selected->dy);

	case JOIN:
		return g_markup_printf_escaped(_("Join <b>%s</b> to every <b>%s</b>, distance %+d,%+d"), gd_elements[selected->fill_element].lowercase_name, gd_elements[selected->element].lowercase_name, selected->dx, selected->dy);

	case FLOODFILL_BORDER:
		return g_markup_printf_escaped(_("Floodfill from %d,%d of <b>%s</b>, border <b>%s</b>"), selected->x1, selected->y1, gd_elements[selected->fill_element].lowercase_name, gd_elements[selected->element].lowercase_name);

	case FLOODFILL_REPLACE:
		return g_markup_printf_escaped(_("Floodfill from %d,%d of <b>%s</b>, replacing <b>%s</b>"), selected->x1, selected->y1, gd_elements[selected->fill_element].lowercase_name, gd_elements[selected->element].lowercase_name);

	case MAZE:
		return g_markup_printf_escaped(_("Maze from %d,%d to %d,%d, wall <b>%s</b>, path <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, gd_elements[selected->fill_element].lowercase_name);

	case MAZE_UNICURSAL:
		return g_markup_printf_escaped(_("Unicursal maze from %d,%d to %d,%d, wall <b>%s</b>, path <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, gd_elements[selected->fill_element].lowercase_name);

	case MAZE_BRAID:
		return g_markup_printf_escaped(_("Braid maze from %d,%d to %d,%d, wall <b>%s</b>, path <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, gd_elements[selected->fill_element].lowercase_name);

	case RANDOM_FILL:
		return g_markup_printf_escaped(_("Random fill from %d,%d to %d,%d, replacing %s"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name);
		
	case NONE:
		g_assert_not_reached();
	}

	return NULL;
}



char *
gd_get_object_coordinates_text (GdObject *selected)
{
	g_return_val_if_fail (selected!=NULL, NULL);

	switch (selected->type) {
	case POINT:
	case FLOODFILL_BORDER:
	case FLOODFILL_REPLACE:
		return g_strdup_printf ("%d,%d", selected->x1, selected->y1);

	case LINE:
	case RECTANGLE:
	case FILLED_RECTANGLE:
	case RANDOM_FILL:
		return g_strdup_printf ("%d,%d-%d,%d", selected->x1, selected->y1, selected->x2, selected->y2);

	case RASTER:
	case MAZE_UNICURSAL:
	case MAZE_BRAID:
	case MAZE:
		return g_strdup_printf ("%d,%d-%d,%d (%d,%d)", selected->x1, selected->y1, selected->x2, selected->y2, selected->dx, selected->dy);

	case JOIN:
		return g_strdup_printf ("%+d,%+d", selected->dx, selected->dy);

	case NONE:
		g_assert_not_reached();
	}

	return NULL;
}








/** drawing a line, using bresenham's */
static void
draw_line (Cave *cave, const GdObject *object)
{
	int x, y, x1, y1, x2, y2;
	gboolean steep;
	int error, dx, dy, ystep;

	x1=object->x1;
	y1=object->y1, x2=object->x2;
	y2=object->y2;
	steep=ABS (y2 - y1) > ABS (x2 - x1);
	if (steep) {
		x=x1;
		x1=y1;
		y1=x;
		x=x2;
		x2=y2;
		y2=x;
	}
	if (x1 > x2) {
		x=x1;
		x1=x2;
		x2=x;
		x=y1;
		y1=y2;
		y2=x;
	}
	dx=x2 - x1;
	dy=ABS (y2 - y1);
	y=y1;
	error=0;
	ystep=(y1 < y2) ? 1 : -1;
	for (x=x1; x <= x2; x++) {
		if (steep)
			gd_cave_store_rc (cave, y, x, object->element, object);
		else
			gd_cave_store_rc (cave, x, y, object->element, object);
		error += dy;
		if (error * 2 >= dx) {
			y += ystep;
			error -= dx;
		}
	}
}



static void
draw_fill_replace_proc (Cave *cave, int x, int y, const GdObject *object)
{
	/* fill with border so we do not come back */
	gd_cave_store_rc (cave, x, y, object->fill_element, object);

	if (x>0 && gd_cave_get_rc(cave, x-1, y)==object->element) draw_fill_replace_proc(cave, x-1, y, object);
	if (y>0 && gd_cave_get_rc(cave, x, y-1)==object->element) draw_fill_replace_proc(cave, x, y-1, object);
	if (x<cave->w-1 && gd_cave_get_rc(cave, x+1, y)==object->element) draw_fill_replace_proc(cave, x+1, y, object);
	if (y<cave->h-1 && gd_cave_get_rc(cave, x, y+1)==object->element) draw_fill_replace_proc(cave, x, y+1, object);
}

static void
draw_fill_replace (Cave *cave, const GdObject *object)
{
	/* check bounds */
	if (object->x1<0 || object->y1<0 || object->x1>=cave->w || object->y1>=cave->h)
		return;
	if (object->element==object->fill_element)
		return;
	/* this procedure fills the area with the object->element. */
	draw_fill_replace_proc(cave, object->x1, object->y1, object);
}



static void
draw_fill_border_proc (Cave *cave, int x, int y, const GdObject *object)
{
	/* fill with border so we do not come back */
	gd_cave_store_rc (cave, x, y, object->element, object);

	if (x>0 && gd_cave_get_rc(cave, x-1, y)!=object->element) draw_fill_border_proc(cave, x-1, y, object);
	if (y>0 && gd_cave_get_rc(cave, x, y-1)!=object->element) draw_fill_border_proc(cave, x, y-1, object);
	if (x<cave->w-1 && gd_cave_get_rc(cave, x+1, y)!=object->element) draw_fill_border_proc(cave, x+1, y, object);
	if (y<cave->h-1 && gd_cave_get_rc(cave, x, y+1)!=object->element) draw_fill_border_proc(cave, x, y+1, object);
}

static void
draw_fill_border (Cave *cave, const GdObject *object)
{
	int x, y;

	/* check bounds */
	if (object->x1<0 || object->y1<0 || object->x1>=cave->w || object->y1>=cave->h)
		return;

	/* this procedure fills the area with the object->element. */
	draw_fill_border_proc(cave, object->x1, object->y1, object);
	
	/* after the fill, we change all filled cells to the fill_element. */
	/* we find those by looking at the object_order[][] */
	for (y=0; y<cave->h; y++)
		for (x=0; x<cave->w; x++)
			if (cave->objects_order[y][x]==object)
				cave->map[y][x]=object->fill_element;
}



/* rectangle, frame only */
static void
draw_rectangle(Cave *cave, const GdObject *object)
{
	int x1, y1, x2, y2, x, y;
	
	/* reorder coordinates if not drawing from northwest to southeast */
	x1=object->x1;
	y1=object->y1, x2=object->x2;
	y2=object->y2;
	if (y1 > y2) {
		y=y1;
		y1=y2;
		y2=y;
	}
	if (x1 > x2) {
		x=x1;
		x1=x2;
		x2=x;
	}
	for (x=x1; x <= x2; x++) {
		gd_cave_store_rc (cave, x, object->y1, object->element, object);
		gd_cave_store_rc (cave, x, object->y2, object->element, object);
	}
	for (y=y1; y <= y2; y++) {
		gd_cave_store_rc (cave, object->x1, y, object->element, object);
		gd_cave_store_rc (cave, object->x2, y, object->element, object);
	}
}



/* rectangle, filled one */
static void
draw_filled_rectangle(Cave *cave, const GdObject *object)
{
	int x1, y1, x2, y2, x, y;
	
	/* reorder coordinates if not drawing from northwest to southeast */
	x1=object->x1;
	y1=object->y1, x2=object->x2;
	y2=object->y2;
	if (y1 > y2) {
		y=y1;
		y1=y2;
		y2=y;
	}
	if (x1 > x2) {
		x=x1;
		x1=x2;
		x2=x;
	}
	for (y=y1; y <= y2; y++)
		for (x=x1; x <= x2; x++)
			gd_cave_store_rc (cave, x, y, (y==object->y1 || y==object->y2 || x==object->x1 || x==object->x2) ? object->element : object->fill_element, object);
}



/* something like ordered fill, increment is dx and dy. */
static void
draw_raster(Cave *cave, const GdObject *object)
{
	int x, y, x1, y1, x2, y2;
	int dx, dy;
	
	/* reorder coordinates if not drawing from northwest to southeast */
	x1=object->x1;
	y1=object->y1;
	x2=object->x2;
	y2=object->y2;
	if (y1 > y2) {
		y=y1;
		y1=y2;
		y2=y;
	}
	if (x1 > x2) {
		x=x1;
		x1=x2;
		x2=x;
	}
	dx=object->dx;
	dy=object->dy;
	for (y=y1; y <= y2; y += dy)
		for (x=x1; x <= x2; x += dx)
			gd_cave_store_rc (cave, x, y, object->element, object);
}



/* find every object, and put fill_element next to it. relative coordinates dx,dy */
static void
draw_join(Cave *cave, const GdObject *object)
{
	int x, y;

	for (y=0; y<cave->h; y++)
		for (x=0; x<cave->w; x++)
			if (cave->map[y][x]==object->element) {
				int nx=x + object->dx;
				int ny=y + object->dy;
				/* this one implements wraparound for joins. it is needed by many caves in profi boulder series */
				while (nx>=cave->w)
					nx-=cave->w, ny++;
				gd_cave_store_rc (cave, nx, ny, object->fill_element, object);
			}
}


/* create a maze in a gboolean **maze. */
/* recursive algorithm. */
static void
mazegen(GRand *rand, gboolean **maze, int width, int height, int x, int y, int horiz)
{
	int dirmask=15;

	maze[y][x]=TRUE;
	while (dirmask!=0) {
		int dir;

		dir=g_rand_int_range(rand, 0, 100)<horiz?2:0;		/* horiz or vert */
		/* if no horizontal movement possible, choose vertical */
		if (dir==2 && (dirmask&12)==0)
			dir=0;
		else if (dir==0 && (dirmask&3)==0)	/* and vice versa */
			dir=2;
		dir+=g_rand_int_range(rand, 0, 2);				/* dir */
		if (dirmask&(1<<dir)) {
			dirmask&=~(1<<dir);
			
			switch(dir) {
				case 0:	/* up */
					if (y>=2 && !maze[y-2][x]) {
						maze[y-1][x]=TRUE;
						mazegen(rand, maze, width, height, x, y-2, horiz);
					}
					break;
				case 1:	/* down */
					if (y<height-2 && !maze[y+2][x]) {
						maze[y+1][x]=TRUE;
						mazegen(rand, maze, width, height, x, y+2, horiz);
					}
					break;
				case 2:	/* left */
					if (x>=2 && !maze[y][x-2]) {
						maze[y][x-1]=TRUE;
						mazegen(rand, maze, width, height, x-2, y, horiz);
					}
					break;
				case 3:	/* right */
					if (x<width-2 && !maze[y][x+2]) {
						maze[y][x+1]=TRUE;
						mazegen(rand, maze, width, height, x+2, y, horiz);
					}
					break;
				default:
					g_assert_not_reached();
			}
		}
	}
}


#if 0
#define CELL_TO_POINTER(x,y) (GUINT_TO_POINTER(((y)<<16)+(x)))
#define X_FROM_POINTER(p) (GPOINTER_TO_UINT(p)&65535)
#define Y_FROM_POINTER(p) (GPOINTER_TO_UINT(p)>>16)
int
ptr_int_compare(gconstpointer p1, gconstpointer p2)
{
	return GPOINTER_TO_INT(p1)-GPOINTER_TO_INT(p2);
}

/* maze generation algorithm from crli */
static void
mazegen(GRand *rand, gboolean **maze, int width, int height, int x, int y, int horiz)
{
	GList *cells=NULL;

	cells=g_list_append(cells, CELL_TO_POINTER(x,y));
	maze[y][x]=TRUE;
	while (cells!=NULL) {
		GList *iter;
		
		iter=cells;
		while (iter!=NULL) {
			GList *next;
			int x, y;
			gboolean possible_dirs[4];
			
			x=X_FROM_POINTER(iter->data);
			y=Y_FROM_POINTER(iter->data);
			
			possible_dirs[0]=y>=2 && !maze[y-2][x];
			possible_dirs[1]=x>=2 && !maze[y][x-2];
			possible_dirs[2]=y<height-2 && !maze[y+2][x];
			possible_dirs[3]=x<width-2 && !maze[y][x+2];

			if (possible_dirs[0] || possible_dirs[1] || possible_dirs[2] || possible_dirs[3]) {
				/* there is at least one direction, so choose one */
				switch (g_rand_int_range(rand, 0, 4)) {
					case 0:
						if (possible_dirs[0]) {
							maze[y-1][x]=TRUE;
							maze[y-2][x]=TRUE;
							cells=g_list_insert_sorted(cells, CELL_TO_POINTER(x, y-2), ptr_int_compare);
						}
						break;
					case 1:
						if (possible_dirs[1]) {
							maze[y][x-1]=TRUE;
							maze[y][x-2]=TRUE;
							cells=g_list_insert_sorted(cells, CELL_TO_POINTER(x-2, y), ptr_int_compare);
						}
						break;
					case 2:
						if (possible_dirs[2]) {
							maze[y+1][x]=TRUE;
							maze[y+2][x]=TRUE;
							cells=g_list_insert_sorted(cells, CELL_TO_POINTER(x, y+2), ptr_int_compare);
						}
						break;
					case 3:
						if (possible_dirs[3]) {
							maze[y][x+1]=TRUE;
							maze[y][x+2]=TRUE;
							cells=g_list_insert_sorted(cells, CELL_TO_POINTER(x+2, y), ptr_int_compare);
						}
						break;
					default:
						g_assert_not_reached();
				}
				
				next=iter->next;
			}
			else {
				next=iter->next;
				
				cells=g_list_remove_link(cells, iter);
			}
			
			iter=next;
		}
	}
}
#undef CELL_TO_POINTER
#undef X_FROM_POINTER
#undef Y_FROM_POINTER
#endif

#if 0
#define CELL_TO_POINTER(x,y) (GUINT_TO_POINTER(((y)<<16)+(x)))
#define X_FROM_POINTER(p) (GPOINTER_TO_UINT(p)&65535)
#define Y_FROM_POINTER(p) (GPOINTER_TO_UINT(p)>>16)
/* growing tree maze generation algorithm. */
static void
mazegen(GRand *rand, gboolean **maze, int width, int height, int x, int y, int complexity)
{
	GPtrArray *cells;
	
	cells=g_ptr_array_sized_new(width*height/4);
	g_ptr_array_add(cells, CELL_TO_POINTER(x,y));
	maze[y][x]=TRUE;
	while (cells->len!=0) {
		int x, y;
		int possible_dirs[4], dirnum;
		int l;
		int i;

		if (complexity>=g_rand_int_range(rand, 0, 100))
			l=cells->len-1;
		else
			l=g_rand_int_range(rand, 0, cells->len);
		i=l;
		x=X_FROM_POINTER(g_ptr_array_index(cells, i));
		y=Y_FROM_POINTER(g_ptr_array_index(cells, i));

		dirnum=0;
		if (y>=2 && !maze[y-2][x])
			possible_dirs[dirnum++]=0;
		if (x>=2 && !maze[y][x-2])
			possible_dirs[dirnum++]=1;
		if (y<height-2 && !maze[y+2][x])
			possible_dirs[dirnum++]=2;
		if (x<width-2 && !maze[y][x+2])
			possible_dirs[dirnum++]=3;
		if (dirnum==0) /* no possible direction, remove from list */
			g_ptr_array_remove_index(cells, i);
		else {
			/* there is at least one direction, so choose one */
			int random_dir=possible_dirs[g_rand_int_range(rand, 0, dirnum)];
			
			switch (random_dir) {
				case 0:
					maze[y-1][x]=TRUE;
					maze[y-2][x]=TRUE;
					g_ptr_array_add(cells, CELL_TO_POINTER(x, y-2));
					break;
				case 1:
					maze[y][x-1]=TRUE;
					maze[y][x-2]=TRUE;
					g_ptr_array_add(cells, CELL_TO_POINTER(x-2, y));
					break;
				case 2:
					maze[y+1][x]=TRUE;
					maze[y+2][x]=TRUE;
					g_ptr_array_add(cells, CELL_TO_POINTER(x, y+2));
					break;
				case 3:
					maze[y][x+1]=TRUE;
					maze[y][x+2]=TRUE;
					g_ptr_array_add(cells, CELL_TO_POINTER(x+2, y));
					break;
				default:
					g_assert_not_reached();
			}
		}
	}
	g_ptr_array_free(cells, TRUE);
}
#undef CELL_TO_POINTER
#undef X_FROM_POINTER
#undef Y_FROM_POINTER
#endif

static void
braidmaze(GRand *rand, gboolean **maze, int w, int h)
{
	int x, y;
	
	for (y=0; y<h; y+=2)
		for (x=0; x<w; x+=2) {
			int closed=0, dirs=0;
			int closed_dirs[4];
			
			/* if it is the edge of the map, OR no path carved, then we can't go in that direction. */
			if (x<1 || !maze[y][x-1]) {
				closed++;	/* closed from this side. */
				/* if not the edge, we might open this wall (carve a path) to remove a dead end */
				if (x>0)
					closed_dirs[dirs++]=MV_LEFT;
			}
			/* other 3 directions similar */
			if (y<1 || !maze[y-1][x]) {
				closed++;
				if (y>0)
					closed_dirs[dirs++]=MV_UP;
			}
			if (x>=w-1 || !maze[y][x+1]) {
				closed++;
				if (x<w-1)
					closed_dirs[dirs++]=MV_RIGHT;
			}
			if (y>=h-1 || !maze[y+1][x]) {
				closed++;
				if (y<h-1)
					closed_dirs[dirs++]=MV_DOWN;
			}
			
			/* if closed from 3 sides, then it is a dead end. also check dirs!=0, that might fail for a 1x1 maze :) */
			if (closed==3 && dirs!=0) {
				/* make up a random direction, and open in that direction, so dead end is removed */
				int dir=closed_dirs[g_rand_int_range(rand, 0, dirs)];
				
				switch(dir) {
					case MV_LEFT:
						maze[y][x-1]=TRUE; break;
					case MV_UP:
						maze[y-1][x]=TRUE; break;
					case MV_RIGHT:
						maze[y][x+1]=TRUE; break;
					case MV_DOWN:
						maze[y+1][x]=TRUE; break;
				}
			}
		}
}




static void
draw_maze(Cave *cave, const GdObject *object, int seed)
{
	int x, y;
	gboolean **map;
	int x1=object->x1;
	int y1=object->y1;
	int x2=object->x2;
	int y2=object->y2;
	int w, h, path, wall;
	int xk, yk;
	GRand *rand;
	int i,j;

	/* change coordinates if not in correct order */
	if (y1>y2) {
		y=y1;
		y1=y2;
		y2=y;
	}
	if (x1>x2) {
		x=x1;
		x1=x2;
		x2=x;
	}
	wall=object->dx;
	if (wall<1)
		wall=1;
	path=object->dy;
	if (path<1)
		path=1;

	/* calculate the width and height of the maze.
		n=number of passages, path=path width, wall=wall width, maze=maze width.
	   if given the number of passages, the width of the maze is:
	   
	   n*path+(n-1)*wall=maze
	   n*path+n*wall-wall=maze
	   n*(path+wall)=maze+wall
	   n=(maze+wall)/(path+wall)
	 */
	/* number of passages for each side */
	w=(x2-x1+1+wall)/(path+wall);
	h=(y2-y1+1+wall)/(path+wall);
	/* and we calculate the size of the internal map */
	if (object->type==MAZE_UNICURSAL) {
		/* for unicursal maze, width and height must be mod2=0, and we will convert to paths&walls later */
		w=w/2*2;	/* XXX should be w/2*2-1 but does not matter */
		h=h/2*2;
	} else {
		/* for normal maze */
		w=2*(w-1)+1;
		h=2*(h-1)+1;
	}

	/* twodimensional boolean array to generate map in */	
	map=g_new(gboolean *, h);
	for (y=0; y<h; y++)
		map[y]=g_new0(gboolean, w);
	
	/* start generation, if map is big enough.
	   otherwise the application would crash, as the editor places maze objects during mouse click&drag that
	   have no sense */
	rand=g_rand_new_with_seed(seed==-1?g_rand_int(cave->random):seed);
	if (w>=1 && h>=1)
		mazegen(rand, map, w, h, 0, 0, object->horiz);
	if (object->type==MAZE_BRAID)
		braidmaze(rand, map, w, h);
	g_rand_free(rand);

	if (w>=1 && h>=1 && object->type==MAZE_UNICURSAL) {
		gboolean **unicursal;

		/* convert to unicursal maze */
		/* original:
			xxx x 
			  x x 
			xxxxx 

			unicursal:
			xxxxxxx xxx
			x     x x x
			xxxxx x x x
			    x x x x
			xxxxx xxx x
			x         x
			xxxxxxxxxxx
		*/

		unicursal=g_new(gboolean *, h*2-1);
		for (y=0; y<h*2-1; y++)
			unicursal[y]=g_new0(gboolean, w*2-1);
		
		for (y=0; y<h; y++)
			for(x=0; x<w; x++) {
				if (map[y][x]) {
					unicursal[y*2][x*2]=TRUE;
					unicursal[y*2][x*2+2]=TRUE;
					unicursal[y*2+2][x*2]=TRUE;
					unicursal[y*2+2][x*2+2]=TRUE;
					
					if (x<1 || !map[y][x-1]) unicursal[y*2+1][x*2]=TRUE;
					if (y<1 || !map[y-1][x]) unicursal[y*2][x*2+1]=TRUE;
					if (x>=w-1 || !map[y][x+1]) unicursal[y*2+1][x*2+2]=TRUE;
					if (y>=h-1 || !map[y+1][x]) unicursal[y*2+2][x*2+1]=TRUE;
				}
			}
		
		/* free original map */
		for (y=0; y<h; y++)
			g_free(map[y]);
		g_free(map);
		
		/* change to new map - the unicursal maze */
		map=unicursal;
		h=h*2-1;
		w=w*2-1;
	}
		
	/* copy map to cave with correct elements and size */
	/* now copy the map into the cave. the copying works like this...
	   pwpwp
	   xxxxx p
	   x x   w
	   x xxx p
	   x     w
	   xxxxx p
	   columns and rows denoted with "p" are to be drawn with path width, the others with wall width. */
	yk=y1;
	for (y=0; y<h; y++) {
		for (i=0; i<(y%2==0?path:wall); i++) {
			xk=x1;
			for (x=0; x<w; x++)
				for (j=0; j<(x%2==0?path:wall); j++)
					gd_cave_store_rc(cave, xk++, yk, map[y][x]?object->fill_element:object->element, object);

			/* if width is smaller than requested, fill with wall */
			for(x=xk; x<=x2; x++)
				gd_cave_store_rc(cave, x, yk, object->element, object);

			yk++;
		}
	}
	/* if height is smaller than requested, fill with wall */
	for (y=yk; y<=y2; y++)
		for (x=x1; x<=x2; x++)
			gd_cave_store_rc(cave, x, y, object->element, object);
	
	/* free map */
	for (y=0; y<h; y++)
		g_free(map[y]);
	g_free(map);
}


static void
draw_random_fill(Cave *cave, const GdObject *object, int seed)
{
	int x, y;
	int x1=object->x1;
	int y1=object->y1;
	int x2=object->x2;
	int y2=object->y2;
	GRand *rand;
	
	rand=g_rand_new_with_seed(seed==-1?g_rand_int(cave->random):seed);

	/* change coordinates if not in correct order */
	if (y1>y2) {
		y=y1;
		y1=y2;
		y2=y;
	}
	if (x1>x2) {
		x=x1;
		x1=x2;
		x2=x;
	}
	if (x1<0) x1=0;
	if (x2>=cave->w) x2=cave->w-1;
	if (y1<0) y1=0;
	if (y2>=cave->h) y2=cave->h-1;
	for (y=y1; y<=y2; y++)
		for (x=x1; x<=x2; x++) {
			unsigned int randm;
			GdElement element;
			
			randm=g_rand_int_range(rand, 0, 256);

			element=object->fill_element;
			if (randm<object->random_fill_probability[0])
				element=object->random_fill[0];
			if (randm<object->random_fill_probability[1])
				element=object->random_fill[1];
			if (randm<object->random_fill_probability[2])
				element=object->random_fill[2];
			if (randm<object->random_fill_probability[3])
				element=object->random_fill[3];
			
			if (object->element==O_NONE || gd_cave_get_rc(cave, x, y)==object->element)
				gd_cave_store_rc(cave, x, y, element, object);
		}
	g_rand_free(rand);
}



/* draw the specified game object into cave's data.
	also remember, which cell was set by which cave object. */
void
gd_cave_draw_object (Cave * cave, const GdObject *object, int level)
{
	g_assert (cave!=NULL);
	g_assert (cave->map!=NULL);
	g_assert (cave->objects_order!=NULL);
	g_assert (object!=NULL);

	switch (object->type) {
		case POINT:
			/* single point */
			gd_cave_store_rc(cave, object->x1, object->y1, object->element, object);
			break;

		case LINE:
			draw_line(cave, object);
			break;

		case RECTANGLE:
			draw_rectangle(cave, object);
			break;

		case FILLED_RECTANGLE:
			draw_filled_rectangle(cave, object);
			break;
			
		case RASTER:
			draw_raster(cave, object);
			break;

		case JOIN:
			draw_join(cave, object);
			break;

		case FLOODFILL_BORDER:
			draw_fill_border(cave, object);
			break;
			
		case FLOODFILL_REPLACE:
			draw_fill_replace(cave, object);
			break;
			
		case MAZE:
		case MAZE_UNICURSAL:
		case MAZE_BRAID:
			draw_maze(cave, object, object->seed[level]);
			break;

		case RANDOM_FILL:
			draw_random_fill(cave, object, object->seed[level]);
			break;
			
		case NONE:
			g_assert_not_reached();
			break;
		
		default:
			g_critical("Unknown object %d", object->type);
			break;
	}
}





/* load cave to play... also can be called rendering the cave elements */
Cave *
gd_cave_new_rendered (const Cave *data, const int level, const guint32 seed)
{
	Cave *cave;
	GdElement element;
	int x, y;
	GList *iter;

	/* make a copy */
	cave=gd_cave_new_from_cave (data);
	cave->rendered=level+1;
	
	cave->render_seed=seed;
	cave->random=g_rand_new_with_seed(cave->render_seed);

	/* maps needed during drawing and gameplay */
	cave->objects_order=gd_cave_map_new(cave, gpointer);

	cave->time=data->level_time[level];
	cave->timevalue=data->level_timevalue[level];
	cave->diamonds_needed=data->level_diamonds[level];

	if (!cave->map) {
		/* if we have no map, fill with predictable random generator. */
		cave->map=gd_cave_map_new (cave, GdElement);
		/* IF CAVE HAS NO MAP, USE THE RANDOM NUMBER GENERATOR */
		/* init c64 randomgenerator */
		if (data->level_rand[level]<0) {
			/* random seed=-1 -> totally random */
			cave->rand_seed_1=g_rand_int_range(cave->random, 0, 256);
			cave->rand_seed_2=g_rand_int_range(cave->random, 0, 256);
		}
		else {
			cave->rand_seed_1=0;
			cave->rand_seed_2=data->level_rand[level];
		}
		/* generate random fill
		 * start from row 1 (0 skipped), and fill also the borders on left and right hand side,
		 * as c64 did. this way works the original random generator the right way.
		 * also, do not fill last row, that is needed for the random seeds to be correct
		 * after filling! predictable slime will use it. */
		for (y=1; y<cave->h-1; y++) {
			for (x=0; x<cave->w; x++) {
				unsigned int randm;
				
				if (data->level_rand[level]<0)
					randm=g_rand_int_range(cave->random, 0, 256);	/* use the much better glib random generator */
				else
					randm=gd_cave_c64_random(cave);	/* use c64 */

				element=data->initial_fill;
				if (randm<data->random_fill_probability[0])
					element=data->random_fill[0];
				if (randm<data->random_fill_probability[1])
					element=data->random_fill[1];
				if (randm<data->random_fill_probability[2])
					element=data->random_fill[2];
				if (randm<data->random_fill_probability[3])
					element=data->random_fill[3];

				gd_cave_store_rc(cave, x, y, element, NULL);
			}
		}

		/* draw initial border */
		for (y=0; y<cave->h; y++) {
			gd_cave_store_rc(cave, 0, y, cave->initial_border, NULL);
			gd_cave_store_rc(cave, cave->w-1, y, cave->initial_border, NULL);
		}
		for (x=0; x<cave->w; x++) {
			gd_cave_store_rc(cave, x, 0, cave->initial_border, NULL);
			gd_cave_store_rc(cave, x, cave->h-1, cave->initial_border, NULL);
		}
	}
	else {
		/* IF CAVE HAS A MAP, SIMPLY USE IT... no need to fill with random elements */
		
		/* initialize c64 predictable random for slime. the values were taken from afl bd, see docs/internals.txt */
		cave->rand_seed_1=0;
		cave->rand_seed_2=0x1e;
	}
	
	/* render cave objects above random data or map */
	for (iter=data->objects; iter; iter=g_list_next (iter)) {
		GdObject *object=(GdObject *)iter->data;

		if (object->levels & gd_levels_mask[level])
			gd_cave_draw_object(cave, iter->data, level);
	}

	/* check if we use c64 ckdelay or milliseconds for timing */
	if (cave->scheduling==GD_SCHEDULING_MILLISECONDS)
		cave->speed=data->level_speed[level];		/* exact timing */
	else {
		cave->speed=120;	/* delay loop based timing... set something for first iteration, then later it will be calculated */
		cave->c64_timing=data->level_ckdelay[level];	/* this one may be used by iterate routine to calculate actual delay if c64scheduling is selected */
	}

	return cave;
}



/*
   render cave at specified level.
   copy result to the map; remove objects.
   the cave will be map-based.
 */
void
gd_flatten_cave(Cave *cave, const int level)
{
	Cave *rendered;

	g_return_if_fail(cave != NULL);

	/* render cave at specified level to obtain map. seed=0 */
	rendered=gd_cave_new_rendered(cave, level, 0);
	/* forget old map without objects */
	gd_cave_map_free(cave->map);
	/* copy new map to cave */
	cave->map=gd_cave_map_dup(rendered, map);
	gd_cave_free(rendered);

	/* forget objects */
	g_list_foreach(cave->objects, (GFunc) g_free, NULL);
	cave->objects=NULL;
}



