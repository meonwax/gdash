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

#include "config.h"

#include "cave.h"
#include "cavedb.h"
#include "caveset.h"
#include "caveobject.h"

/* arrays for movements */
/* also no1 and bd2 cave data import helpers; line direction coordinates */
const int gd_dx[]={ 0, 0, 1, 1, 1, 0, -1, -1, -1, 0, 2, 2, 2, 0, -2, -2, -2 };
const int gd_dy[]={ 0, -1, -1, 0, 1, 1, 1, 0, -1, -2, -2, 0, 2, 2, 2, 0, -2 };

const char* gd_direction_name[]={ NULL, N_("Up"), N_("Up+right"), N_("Right"), N_("Down+right"), N_("Down"), N_("Down+left"), N_("Left"), N_("Up+left") };
const char* gd_direction_filename[]={ NULL, "up", "upright", "right", "downright", "down", "downleft", "left", "upleft" };
const char* gd_scheduling_name[]={ N_("Milliseconds"), N_("BD1"), N_("BD2"), N_("Construction Kit"), N_("Crazy Dream 7") };
const char* gd_scheduling_filename[]={ "ms", "bd1", "bd2", "plck", "crdr7" };

static GHashTable *name_to_element;
GdElement gd_char_to_element[256];

const GdColor gd_flash_color=0xFFFFC0;
const GdColor gd_select_color=0x8080FF;



const char *
gd_direction_get_visible_name(GdDirection dir)
{
	g_assert(dir>0 && dir<G_N_ELEMENTS(gd_direction_name));
	return gd_direction_name[dir];
}

const char *
gd_direction_get_filename(GdDirection dir)
{
	g_assert(dir>0 && dir<G_N_ELEMENTS(gd_direction_name));
	return gd_direction_filename[dir];
}

GdDirection
gd_direction_from_string(const char *str)
{
	int i;

	g_assert(str!=NULL);	
	for (i=1; i<G_N_ELEMENTS(gd_direction_filename); i++)
		if (g_ascii_strcasecmp(str, gd_direction_filename[i])==0)
			return (GdDirection) i;

	g_warning ("invalid direction name '%s', defaulting to down", str);
	return MV_DOWN;
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


/* search the element database for the specified character, and return the element. */
GdElement
gd_get_element_from_character (guint8 character)
{
	if (gd_char_to_element[character]!=O_UNKNOWN)
		return gd_char_to_element[character];

	g_warning ("Invalid character representing element: %c", character);
	return O_UNKNOWN;
}




/*
	do some init; this function is to be called at the start of the application
*/
void
gd_cave_init()
{
	int i;

	g_assert(MV_MAX==G_N_ELEMENTS(gd_dx));
	g_assert(MV_MAX==G_N_ELEMENTS(gd_dy));
	g_assert(GD_SCHEDULING_MAX==G_N_ELEMENTS(gd_scheduling_filename));
	g_assert(GD_SCHEDULING_MAX==G_N_ELEMENTS(gd_scheduling_name));
	
	/* put names to a hash table */
	/* this is a helper for file read operations */
	/* maps g_strdupped strings to elemenets (integers) */
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




void
gd_struct_set_defaults_from_array(gpointer str, const GdStructDescriptor *properties, GdPropertyDefault *defaults)
{
	int i;
	
	for (i=0; defaults[i].offset!=-1; i++) {
		gpointer pvalue=G_STRUCT_MEMBER_P(str, defaults[i].offset);
		int *ivalue=pvalue;	/* these point to the same, but to avoid the awkward cast syntax */
		GdElement *evalue=pvalue;
		GdDirection *dvalue=pvalue;
		GdScheduling *svalue=pvalue;
		gboolean *bvalue=pvalue;
		GdColor *cvalue=pvalue;
		double *fvalue=pvalue;
		int j, n;
		
		/* check which property we are talking about: find it in gd_cave_properties. */
		n=defaults[i].property_index;
		if (n==0) {
			while(properties[n].identifier!=NULL && properties[n].offset!=defaults[i].offset)
				n++;
			/* make sure we found it. */
			g_assert(properties[n].identifier!=NULL);
			
			/* remember so we will be fast later*/
			defaults[i].property_index=n;
		}
		
		for (j=0; j<properties[n].count; j++)
			switch (properties[n].type) {
			/* these are for the gui; do nothing */
			case GD_TAB:
			case GD_LABEL:
			case GD_LEVEL_LABEL:
			/* no default value for strings */
			case GD_TYPE_STRING:
				g_assert_not_reached();
				break;

			case GD_TYPE_INT:
			case GD_TYPE_RATIO:	/* this is also an integer */
				ivalue[j]=defaults[i].defval;
				break;
			case GD_TYPE_PROBABILITY:	/* floats are stored as integer, /million */
				fvalue[j]=defaults[i].defval/1000000.0;
				break;
			case GD_TYPE_BOOLEAN:
				bvalue[j]=defaults[i].defval!=0;
				break;
			case GD_TYPE_ELEMENT:
			case GD_TYPE_EFFECT:
				evalue[j]=(GdElement) defaults[i].defval;
				break;
			case GD_TYPE_COLOR:
				cvalue[j]=gd_c64_colors[defaults[i].defval].rgb;
				break;
			case GD_TYPE_DIRECTION:
				dvalue[j]=(GdDirection) defaults[i].defval;
				break;
			case GD_TYPE_SCHEDULING:
				svalue[j]=(GdScheduling) defaults[i].defval;
				break;
			}
	}
}

#if 0
char *
gd_struct_explain_defaults_in_string(const GdStructDescriptor *properties, GdPropertyDefault *defaults)
{
	GString *defs;
	int i;
	
	defs=g_string_new(NULL);
	
	for (i=0; defaults[i].offset!=-1; i++) {
		int j, n;
		
		/* check which property we are talking about: find it in gd_cave_properties. */
		n=defaults[i].property_index;
		if (n==0) {
			while(properties[n].identifier!=NULL && properties[n].offset!=defaults[i].offset)
				n++;
			/* make sure we found it. */
			g_assert(properties[n].identifier!=NULL);
			
			/* remember so we will be fast later*/
			defaults[i].property_index=n;
		}
		
		g_string_append_printf(defs, "%s: ", _(properties[n].name));
		
		for (j=0; j<properties[n].count; j++)
			switch (properties[n].type) {
			/* these are for the gui; should not be in the defaults array */
			case GD_TAB:
			case GD_LABEL:
			case GD_LEVEL_LABEL:
			/* no default value for strings */
			case GD_TYPE_STRING:
				g_assert_not_reached();
				break;

			case GD_TYPE_INT:
			case GD_TYPE_RATIO:	/* this is also an integer */
				g_string_append_printf(defs, "%d", defaults[i].defval);
				break;
			case GD_TYPE_PROBABILITY:	/* floats are stored as integer, /million*100 */
				g_string_append_printf(defs, "%6.4f%%", defaults[i].defval/10000.0);
				break;
			case GD_TYPE_BOOLEAN:
				g_string_append_printf(defs, "%s", defaults[i].defval?_("Yes"):_("No"));
				break;
			case GD_TYPE_ELEMENT:
			case GD_TYPE_EFFECT:
				g_string_append_printf(defs, "%s", _(gd_elements[defaults[i].defval].name));
				break;
			case GD_TYPE_COLOR:
				g_string_append_printf(defs, "%s", gd_get_color_name(gd_c64_colors[defaults[i].defval].rgb));
				break;
			case GD_TYPE_DIRECTION:
				g_string_append_printf(defs, "%s", gd_direction_name[defaults[i].defval]);
				break;
			case GD_TYPE_SCHEDULING:
				g_string_append_printf(defs, "%s", gd_scheduling_name[(GdScheduling) defaults[i].defval]);
				break;
			}

		g_string_append_printf(defs, "\n");
	}
	
	/* return char * data */
	return g_string_free(defs, FALSE);
}
#endif


void
gd_cave_set_defaults_from_array(Cave* cave, GdPropertyDefault *defaults)
{
	gd_struct_set_defaults_from_array(cave, gd_cave_properties, defaults);
}


/*
	load default values from description array
*/
void
gd_cave_set_gdash_defaults(Cave* cave)
{
	int i;

	gd_cave_set_defaults_from_array(cave, gd_cave_defaults_gdash);

	/* these did not fit into that */
	for (i=0; i<5; i++) {
		cave->level_rand[i]=i;
		cave->level_timevalue[i]=i+1;
	}
}



/* for quicksort. compares two highscores. */
int
gd_highscore_compare(gconstpointer a, gconstpointer b)
{
	const GdHighScore *ha=a;
	const GdHighScore *hb=b;
	return hb->score - ha->score;
}

void
gd_clear_highscore(GdHighScore *hs)
{
	int i;
	
	for (i=0; i<GD_HIGHSCORE_NUM; i++) {
		strcpy(hs[i].name, "");
		hs[i].score=0;
	}
}

gboolean
gd_has_highscore(GdHighScore *hs)
{
	return hs[0].score>0;
}

void
gd_cave_clear_highscore(Cave *cave)
{
	gd_clear_highscore(cave->highscore);
}


/* return true if score achieved is a highscore */
gboolean gd_is_highscore(GdHighScore *scores, int score)
{
	/* if score is above zero AND bigger than the last one */
	if (score>0 && score>scores[GD_HIGHSCORE_NUM-1].score)
		return TRUE;

	return FALSE;
}

int
gd_add_highscore(GdHighScore *scores, GdHighScore hs)
{
	int i;
	
	if (!gd_is_highscore(scores, hs.score))
		return -1;
		
	/* overwrite the last one */
	scores[GD_HIGHSCORE_NUM-1]=hs;
	/* and sort */
	qsort(scores, GD_HIGHSCORE_NUM, sizeof(GdHighScore), gd_highscore_compare);
	
	for (i=0; i<GD_HIGHSCORE_NUM; i++)
		if (g_str_equal(scores[i].name, hs.name) && scores[i].score==hs.score)
			return i;
			
	g_assert_not_reached();
	return -1;
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

/* used for exporting to crli caves. */
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








/*
	create new cave with default values.
	sets every value, also default size, diamond value etc.
*/
Cave *
gd_cave_new(void)
{
	Cave *cave=g_new0 (Cave, 1);

	/* hash table which stores unknown tags as strings. */
	cave->tags=g_hash_table_new_full(gd_str_case_hash, gd_str_case_equal, g_free, g_free);

	gd_cave_set_gdash_defaults (cave);

	return cave;
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

/*
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

/*
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

/*
	frees memory associated to cave
*/
void
gd_cave_free (Cave *cave)
{
	if (!cave)
		return;

	if (cave->tags)
		g_hash_table_destroy(cave->tags);
	
	if (cave->random)
		g_rand_free(cave->random);

	/* map */
	gd_cave_map_free(cave->map);
	/* rendered data */
	gd_cave_map_free(cave->objects_order);
	/* hammered walls to reappear data */
	gd_cave_map_free(cave->hammered_reappear);
	/* free objects */
	g_list_foreach (cave->objects, (GFunc) g_free, NULL);
	g_list_free (cave->objects);


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


/*
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









/* 
	C64 BD predictable random number generator.
	Used to load the original caves imported from c64 files.
	Also by the predictable slime.
*/
unsigned int
gd_cave_c64_random (Cave *cave)
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




/*
	shrink cave
	if last line or last row is just steel wall (or (invisible) outbox).
	used after loading a game for playing.
	after this, ew and eh will contain the effective width and height.
 */
void
gd_cave_auto_shrink (Cave *cave)
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

/*
	bd1 and similar engines had animation bits in cave data, to set which elements to animate (firefly, butterfly, amoeba).
	animating an element also caused some delay each frame; according to my measurements, around 2.6 ms/element.
*/
static void
cave_set_ckdelay_extra_for_animation(Cave *cave)
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
	
	cave_set_ckdelay_extra_for_animation(cave);

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
		
	gd_cave_correct_visible_size(cave);

	/* select number of milliseconds (for pal and ntsc) */
	cave->timing_factor=cave->pal_timing?1200:1000;
	cave->time*=cave->timing_factor;
	cave->magic_wall_milling_time*=cave->timing_factor;
	cave->amoeba_slow_growth_time*=cave->timing_factor;
	cave->hatching_delay_time*=cave->timing_factor;
	if (cave->hammered_walls_reappear)
		cave->hammered_reappear=gd_cave_map_new(cave, int);
}

/* cave diamonds needed can be set to n<=0. */
/* if so, count the diamonds at the time of the hatching, and decrement that value from */
/* the number of diamonds found. */
/* of course, this function is to be called from the cave engine, at the exact time of hatching. */
void
gd_cave_count_diamonds(Cave *cave)
{
	int x, y;
	
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
	elemdrawing[O_MAGIC_WALL]=gd_elements[cave->magic_wall_state == GD_MW_ACTIVE ? O_MAGIC_WALL : O_BRICK].image_game;
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




/* cave time is rounded _UP_ to seconds. so at the exact moment when it changes from
   2sec remaining to 1sec remaining, the player has exactly one second. when it changes
   to zero, it is the exact moment of timeout. */
/* internal time is milliseconds. */
int
gd_cave_time_show(Cave *cave, int internal_time)
{
	return (internal_time+cave->timing_factor-1)/cave->timing_factor;
}
