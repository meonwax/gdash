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
#include <glib/gstdio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "cave.h"
#include "caveobject.h"
#include "c64import.h"
#include "config.h"
#include "util.h"

#include "caveset.h"

#include "levels.h"


#define BDCFF_VERSION "0.5"


/* this stores the caves. */
GList *gd_caveset;
/* this is a pseudo cave, which holds the attributes of the game.
	it also holds the default values for size and others.
	a template when creating new caves, and with loading of bdcff files
	(attributes in [game], not in [cave] section)
	also, this stores highscores for the game
*/
Cave *gd_default_cave;
gboolean gd_caveset_edited;
static int cavesize[6], intermissionsize[6];



/****************************************
 *
 * highscores saving in config dir 
 *
 */
/* calculates an adler checksum, for which it uses all
   elements of all cave-rendereds. */
static guint32
caveset_checksum()
{
	guint32 a=1, b=0;
	GList *iter;
	
	for (iter=gd_caveset; iter!=NULL; iter=iter->next) {
		Cave *rendered=gd_cave_new_rendered(iter->data, 0, 0);	/* seed=0 */
		int x, y;
		
		for (y=0; y<rendered->h; y++)
			for (x=0; x<rendered->w; x++) {
				a+=rendered->map[y][x];
				b+=a;
				
				a%=65521;
				b%=65521;
			}
		gd_cave_free(rendered);
	}
	return (b<<16) + a;
}

/* adds highscores of a cave to a keyfile given in userdat. this is
   a g_list_foreach function.
   it guesses the index, which is written to the file; by checking
   the caveset. */
static void
cave_highscore_to_keyfile_func(gpointer cav, gpointer userdat)
{
	Cave *cave=(Cave *)cav;
	GKeyFile *keyfile=(GKeyFile *)userdat;
	int index, i;
	char cavstr[10];

	/* here we guess the number... */
	if (cav==gd_default_cave)
		index=0;
	else
		index=g_list_index(gd_caveset, cave)+1;
	g_snprintf(cavstr, sizeof(cavstr), "%d", index);

	for (i=0; i<G_N_ELEMENTS(cave->highscore); i++)
		if (cave->highscore[i].score>0) {	/* only save, if score is not zero */
			char rankstr[10];
			char *str;
			
			g_snprintf(rankstr, sizeof(rankstr), "%d", i+1);
			str=g_strdup_printf("%d %s", cave->highscore[i].score, cave->highscore[i].name);
			g_key_file_set_string(keyfile, cavstr, rankstr, str);
			g_free(str);
		}
	g_key_file_set_comment(keyfile, cavstr, NULL, cave->name, NULL);
}

const char *filename_for_cave_highscores(const char *directory)
{
	char *fname, *canon;
	static char *outfile=NULL;
	guint32 checksum;
	
	g_free(outfile);

	checksum=caveset_checksum();
	
	canon=g_strdup(gd_default_cave->name);
	g_strcanon(canon, "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", '_');
	fname=g_strdup_printf("%08x-%s.hsc", checksum, canon);
    outfile=g_build_path (G_DIR_SEPARATOR_S, directory, fname, NULL);
    g_free(fname);
    g_free(canon);
	
	return outfile;
}

/* save highscores of the current cave to the configuration directory.
   this one chooses a filename on its own.
   it is used to save highscores for imported caves.
 */
void
gd_save_highscore(const char *directory)
{
	GKeyFile *keyfile;
	gchar *data;
	GError *error=NULL;
	
	keyfile=g_key_file_new();
	
	cave_highscore_to_keyfile_func(gd_default_cave, keyfile);
	g_list_foreach(gd_caveset, cave_highscore_to_keyfile_func, keyfile);
	
	data=g_key_file_to_data(keyfile, NULL, &error);
	g_key_file_free(keyfile);

	if (strlen(data)>0) {
		g_mkdir_with_parents(directory, 0700);
	    g_file_set_contents(filename_for_cave_highscores(directory), data, -1, &error);
	}
    g_free(data);
}

gboolean
cave_highscores_load_from_keyfile(Cave *cave, GKeyFile *keyfile, int i)
{
	char cavstr[10];
	char **keys;
	int j;
	
	/* check if keyfile has the group in question */
	g_snprintf(cavstr, sizeof(cavstr), "%d", i);
	if (!g_key_file_has_group(keyfile, cavstr))
		/* if the cave had no highscore, there is no group. this is normal! */
		return FALSE;

	/* for all keys... we ignore the keys itself, as rebuilding the sorted list is more simple */
	keys=g_key_file_get_keys(keyfile, cavstr, NULL, NULL);
	for (j=0; keys[j]!=NULL; j++) {
		int score;
		char *str;
		
		str=g_key_file_get_string(keyfile, cavstr, keys[j], NULL);
		if (!str)	/* ?! not really possible but who knows */
			continue;
		
		if (strchr(str, ' ')!=NULL && sscanf(str, "%d", &score)==1)	{
			GdHighScore hs;
			
			hs.score=score;
			g_strlcpy(hs.name, strchr(str, ' ')+1, sizeof(hs.name));
			gd_cave_add_highscore(cave, hs);
		} else
			g_warning("Invalid line in highscore file: %s", str);
		g_free(str);
	}
	g_strfreev(keys);
	
	return TRUE;
}


/* load highscores from a file saved in the configuration directory.
   the file name is guessed automatically.
   if there is some highscore for a cave, then they are deleted.
*/
gboolean
gd_load_highscore(const char *directory)
{
	GList *iter;
	GKeyFile *keyfile;
	gboolean success;
	const char *filename;
	GError *error=NULL;
	int i;
	
	filename=filename_for_cave_highscores(directory);
	keyfile=g_key_file_new();
	success=g_key_file_load_from_file(keyfile, filename, 0, &error);
	if (!success) {
		g_key_file_free(keyfile);
		/* skip file not found errors; report everything else. it is considered a normal thing when there is no .hsc file yet */
		if (error->domain==G_FILE_ERROR && error->code==G_FILE_ERROR_NOENT)
			return TRUE;

		g_warning("%s", error->message);
		return FALSE;
	}
	
	gd_cave_clear_highscore(gd_default_cave);
	cave_highscores_load_from_keyfile(gd_default_cave, keyfile, 0);	/* try to load for game */

	/* try to load for all caves */
	for (iter=gd_caveset, i=1; iter!=NULL; iter=iter->next, i++) {
		Cave *cave=iter->data;
		
		gd_cave_clear_highscore(cave);
		cave_highscores_load_from_keyfile(cave, keyfile, i);
	}
	
	g_key_file_free(keyfile);
	return TRUE;
}


static void
write_highscore_func(Cave *cave, GString *fout)
{
	int i, hs;
	
	g_string_append(fout, "[highscore]\n");
	hs=0;
	for (i=0; i<G_N_ELEMENTS(cave->highscore); i++)
		if (cave->highscore[i].score>0) {	/* only save, if score is not zero */
			g_string_append_printf(fout, "%d %s %d\n", hs+1, cave->highscore[i].name, cave->highscore[i].score);
			hs++;
		}
	g_string_append(fout, "[/highscore]\n\n");
}







/** Clears all caves in the caveset. This creates the list store if needed */
void
gd_caveset_clear ()
{
	if (gd_caveset) {
		g_list_foreach(gd_caveset, (GFunc) gd_cave_free, NULL);
		g_list_free(gd_caveset);
		gd_caveset=NULL;
		gd_cave_free(gd_default_cave);
		gd_default_cave=NULL;
	}

	/* always newly create this */
	/* create pseudo cave containing default values */
	gd_default_cave=gd_cave_new ();
	g_strlcpy(gd_default_cave->name, _("New caveset"), sizeof(gd_default_cave->name));
}


/* return number of caves currently in memory. */
int
gd_caveset_count ()
{
	return g_list_length (gd_caveset);
}



/* return index of first selectable cave */
int
gd_caveset_first_selectable ()
{
	GList *iter;
	int i;
	
	for (i=0, iter=gd_caveset; iter!=NULL; i++, iter=iter->next) {
		Cave *cave=(Cave *)iter->data;
		
		if (cave->selectable)
			return i;
	}

	g_warning("no selectable cave in caveset!");	
	return 0;
}








static gboolean
cave_process_tags_func(char *attrib, char *param, Cave *cave)
{
	int i;
	char **params;
	int paramcount;
	gboolean identifier_found;
	
	params=g_strsplit_set(param, " ", -1);
	paramcount=g_strv_length(params);
	identifier_found=FALSE;
	
	/* engine flag */
	if (g_ascii_strcasecmp(attrib, "Engine")==0) {
		identifier_found=TRUE;

		if (g_ascii_strcasecmp(param, "BD1")==0)
			gd_cave_set_bd1_defaults(cave);
		else
		if (g_ascii_strcasecmp(param, "BD2")==0)
			gd_cave_set_bd2_defaults(cave);
		else
		if (g_ascii_strcasecmp(param, "PLCK")==0)
			gd_cave_set_plck_defaults(cave);
		else
		if (g_ascii_strcasecmp(param, "1stB")==0)
			gd_cave_set_1stb_defaults(cave);
		else
		if (g_ascii_strcasecmp(param, "CrDr")==0)
			gd_cave_set_crdr_defaults(cave);
		else
		if (g_ascii_strcasecmp(param, "CrLi")==0)
			gd_cave_set_crli_defaults(cave);
		else
			g_warning(_("invalid parameter \"%s\" for attribute %s"), param, attrib);
	}
	else
	/* colors attribute is a mess, have to process explicitly */
	if (g_ascii_strcasecmp(attrib, "Colors")==0) {
		/* Colors=[border background] foreground1 foreground2 foreground3 [amoeba slime] */
		identifier_found=TRUE;
		
		if (paramcount==3) {
			/* only color1,2,3 */
			cave->colorb=0x000000;	/* border */
			cave->color0=0x000000;	/* background */
			cave->color1=gd_get_color_from_string (params[0]);
			cave->color2=gd_get_color_from_string (params[1]);
			cave->color3=gd_get_color_from_string (params[2]);
			cave->color4=cave->color3;	/* amoeba */
			cave->color5=cave->color1;	/* slime */
		} else
		if (paramcount==5) {
			/* bg,color0,1,2,3 */
			cave->colorb=gd_get_color_from_string (params[0]);
			cave->color0=gd_get_color_from_string (params[1]);
			cave->color1=gd_get_color_from_string (params[2]);
			cave->color2=gd_get_color_from_string (params[3]);
			cave->color3=gd_get_color_from_string (params[4]);
			cave->color4=cave->color3;	/* amoeba */
			cave->color5=cave->color1;	/* slime */
		} else
		if (paramcount==7) {
			/* bg,color0,1,2,3,amoeba,slime */
			cave->colorb=gd_get_color_from_string (params[0]);
			cave->color0=gd_get_color_from_string (params[1]);
			cave->color1=gd_get_color_from_string (params[2]);
			cave->color2=gd_get_color_from_string (params[3]);
			cave->color3=gd_get_color_from_string (params[4]);
			cave->color4=gd_get_color_from_string (params[5]);	/* amoeba */
			cave->color5=gd_get_color_from_string (params[6]);	/* slime */
		} else {
			g_warning("invalid number of color strings: %s", param);
			gd_cave_set_random_colors(cave);	/* just create some random */
		}
	} else
	{
		/* check all known tags. do not exit this loop if identifier_found==true...
		   as there are more lines in the array which have the same identifier. */
		int paramindex=0;
		for (i=0; gd_cave_properties[i].identifier!=NULL; i++)
			if (g_ascii_strcasecmp (gd_cave_properties[i].identifier, attrib)==0) {
				/* found the identifier */
				gpointer value=G_STRUCT_MEMBER_P (cave, gd_cave_properties[i].offset);
				int *ivalue=value;	/* these point to the same, but to avoid the awkward cast syntax */
				GdElement *evalue=value;
				GdDirection *dvalue=value;
				GdScheduling *svalue=value;
				gboolean *bvalue=value;
				double *fvalue=value;
				int j;

				identifier_found=TRUE;
				
				if (gd_cave_properties[i].type==GD_TYPE_STRING) {
					/* strings are treated different, as occupy the whole length of the line */
					g_strlcpy(value, param, sizeof(GdString));
					continue;
				}

				/* not a string, so use scanf calls */
				/* ALSO, if no more parameters, skip */
				for (j=0; j<gd_cave_properties[i].count && params[paramindex]!=NULL; j++) {
					gboolean success=FALSE;
					gdouble res;
					int n;

					switch (gd_cave_properties[i].type) {
					case GD_TYPE_STRING:
						/* handled above */
					case GD_TAB:
					case GD_LABEL:
					case GD_LEVEL_LABEL:
						/* do nothing */
						break;
					case GD_TYPE_BOOLEAN:
						success=sscanf(params[paramindex], "%d", bvalue+j)==1;
						if (!success) {
							if (g_ascii_strcasecmp (params[paramindex], "true")==0 || g_ascii_strcasecmp (params[paramindex], "on")==0 || g_ascii_strcasecmp (params[paramindex], "yes")==0) {
								bvalue[j]=TRUE;
								success=TRUE;
							}
							else if (g_ascii_strcasecmp (params[paramindex], "false")==0 || g_ascii_strcasecmp (params[paramindex], "off")==0 || g_ascii_strcasecmp (params[paramindex], "no")==0) {
								bvalue[j]=FALSE;
								success=TRUE;
							}
						}
						break;
					case GD_TYPE_INT:
						success=sscanf (params[paramindex], "%d", ivalue+j)==1;
						break;
					case GD_TYPE_PROBABILITY:
						res=g_ascii_strtod (params[paramindex], NULL);
						if (errno==0 && res >= 0 && res <= 1) {
							fvalue[j]=res;
							success=TRUE;
						}
						break;
					case GD_TYPE_RATIO:
						res=g_ascii_strtod (params[paramindex], NULL);
						if (errno==0 && res >= 0 && res <= 1) {
							ivalue[j]=(int)(res*cave->w*cave->h+0.5);
							success=TRUE;
						}
						break;
					case GD_TYPE_ELEMENT:
						evalue[j]=gd_get_element_from_string (params[paramindex]);
						success=TRUE;	/* this shows error message on its own, do treat as always succeeded */
						break;
					case GD_TYPE_DIRECTION:
						if (g_ascii_strcasecmp(params[paramindex], "down")==0) {
							dvalue[j]=MV_DOWN;
							success=TRUE;
						} else if (g_ascii_strcasecmp(params[paramindex], "up")==0) {
							dvalue[j]=MV_UP;
							success=TRUE;
						} else if (g_ascii_strcasecmp(params[paramindex], "left")==0) {
							dvalue[j]=MV_LEFT;
							success=TRUE;
						} else if (g_ascii_strcasecmp(params[paramindex], "right")==0) {
							dvalue[j]=MV_RIGHT;
							success=TRUE;
						}
						break;
					case GD_TYPE_SCHEDULING:
						for (n=0; n<GD_SCHEDULING_MAX; n++)
							if (g_ascii_strcasecmp(params[paramindex], gd_scheduling_filename[n])==0) {
								svalue[j]=(GdScheduling)n;
								success=TRUE;
							}
						break;
					case GD_TYPE_COLOR:
					case GD_TYPE_EFFECT:
						/* shoud have handled this elsewhere */
						break;
					}

					if (!success)
						g_warning(_("invalid parameter \"%s\" for attribute %s"), params[paramindex], attrib);
					else
						paramindex++;
				}
			}
	}
	g_strfreev(params);
	
	/* a ghrfunc should return true if the identifier is to be removed */
	return identifier_found;
}

static void
cave_report_tags_func(char *attrib, char *param, gpointer data)
{
	Cave *cave=(Cave *)data;
	g_warning("Unknown tag %s", attrib);
	g_hash_table_insert(cave->tags, g_strdup(attrib), g_strdup(param));
}

static void
cave_process_tags(Cave *cave, GHashTable *tags, GList *maplines)
{
	char *value;

	/* first check name, so we can report errors correctly */
	value=g_hash_table_lookup(tags, "Name");
	if (value)
		cave_process_tags_func("Name", value, cave);
	if (g_str_equal(cave->name, ""))
		gd_error_set_context("<unnamed cave>");
	else {
		if (cave!=gd_default_cave)
			gd_error_set_context("Cave '%s'", cave->name);
		else
			gd_error_set_context("Game '%s'", cave->name);
	}

	/* process lame engine tag first so its settings may be overwritten later */
	value=g_hash_table_lookup(tags, "Engine");
	if (value) {
		cave_process_tags_func("Engine", value, cave);
		g_hash_table_remove(tags, "Engine");
	}

	/* check if this is an intermission, so we can set to cavesize or intermissionsize */
	value=g_hash_table_lookup(tags, "Intermission");
	if (value) {
		cave_process_tags_func("Intermission", value, cave);
		g_hash_table_remove(tags, "Intermission");
	}
	if (cave->intermission) {
		/* set to IntermissionSize */
		cave->w=intermissionsize[0];
		cave->h=intermissionsize[1];
		cave->x1=intermissionsize[2];
		cave->y1=intermissionsize[3];
		cave->x2=intermissionsize[4];
		cave->y2=intermissionsize[5];
	} else {
		/* set to CaveSize */
		cave->w=cavesize[0];
		cave->h=cavesize[1];
		cave->x1=cavesize[2];
		cave->y1=cavesize[3];
		cave->x2=cavesize[4];
		cave->y2=cavesize[5];
	}

	/* process size at the beginning... as ratio types depend on this. */
	value=g_hash_table_lookup(tags, "Size");
	if (value) {
		cave_process_tags_func("Size", value, cave);
		g_hash_table_remove(tags, "Size");
	}
	
	/* these are read from the hash table, but also have some implications */
	/* these also set predictability */
	if (g_hash_table_lookup(tags, "SlimePermeability"))
		cave->slime_predictable=FALSE;
	if (g_hash_table_lookup(tags, "SlimePermeabilityC64"))
		cave->slime_predictable=TRUE;
	/* these set scheduling type. framedelay takes precedence, if there are both, so we check it later. */
	if (g_hash_table_lookup(tags, "CaveDelay"))
		cave->scheduling=GD_SCHEDULING_PLCK;
	if (g_hash_table_lookup(tags, "FrameTime"))
		cave->scheduling=GD_SCHEDULING_MILLISECONDS;
	
	/* process all tags */
	g_hash_table_foreach_remove(tags, (GHRFunc) cave_process_tags_func, cave);
	g_hash_table_foreach_remove(tags, (GHRFunc) cave_report_tags_func, cave);
	
	/* and at the end, when read all tags (especially the size= tag) */
	/* process map, if any. */
	/* only report if map read is bigger than size= specified. */
	/* some old bdcff files use smaller intermissions than the one specified. */
	if (maplines) {
		int x, y, length=g_list_length(maplines);
		GList *iter;
		
		/* create map and fill with initial border, in case that map strings are shorter or somewhat */
		cave->map=gd_cave_map_new(cave, GdElement);
		for (y=0; y<cave->h; y++)
			for (x=0; x<cave->w; x++)
				cave->map[y][x]=cave->initial_border;
		
		if (length!=cave->h && length!=(cave->y2-cave->y1+1))
			g_warning("map error: cave height=%d (%d visible), map rows=%d", cave->h, cave->y2-cave->y1+1, length);
		for (iter=maplines, y=0; y<length && iter!=NULL; iter=iter->next, y++) {
			const char *line=iter->data;
			int slen=strlen(line);

			if (slen!=cave->w && slen!=(cave->x2-cave->x1+1))
				g_warning("map error in row %d: cave width=%d (%d visible), map row length=%d", y, cave->w, cave->x2-cave->x1+1, slen);
			/* use number of cells from cave or string, whichever is smaller. so will not overwrite array! */
			for (x=0; x<MIN(cave->w, slen); x++)
				cave->map[y][x]=gd_get_element_from_character (line[x]);
		}
	}
}

/* sets the cavesize array to default values */
static void
set_cavesize_defaults()
{
	cavesize[0]=40;
	cavesize[1]=22;
	cavesize[2]=0;
	cavesize[3]=0;
	cavesize[4]=39;
	cavesize[5]=21;
}

/* sets the cavesize array to default values */
static void
set_intermissionsize_defaults()
{
	intermissionsize[0]=40;
	intermissionsize[1]=22;
	intermissionsize[2]=0;
	intermissionsize[3]=0;
	intermissionsize[4]=19;
	intermissionsize[5]=11;
}

static gboolean
caveset_load_from_bdcff (const char *contents)
{
	char **lines;
	int lineno;
	Cave *cave;
	gboolean reading_map=FALSE;
	gboolean reading_mapcodes=FALSE;
	gboolean reading_highscore=FALSE;
	gboolean reading_objects=FALSE;
	char version_read[100]="0.32";	/* assume version to be 0.32, also when the file does not specify it explicitly */
	GList *mapstrings=NULL;
	int linenum;
	GHashTable *tags;
	GdObjectLevels levels=GD_OBJECT_LEVEL_ALL;

	gd_caveset_clear();
	
	set_cavesize_defaults();
	set_intermissionsize_defaults();
	gd_create_char_to_element_table();
	
	tags=g_hash_table_new_full(gd_str_case_hash, gd_str_case_equal, NULL, NULL);

	/* split into lines */
	lines=g_strsplit_set (contents, "\n", 0);

	/* attributes read will be set in cave. if no [cave]; they are stored in the default cave; like in a [game] */
	cave=gd_default_cave;

	linenum=g_strv_length(lines);
	for (lineno=0; lineno<linenum; lineno++) {
		char *line=lines[lineno];
		
		gd_error_set_context("Line %d", lineno+1);

		if (strchr (line, '\r'))
			*strchr (line, '\r')=0;
		if (strlen (line)==0)
			continue;			/* skip empty lines */
		
		/* just skip comments. be aware that map lines may start with a semicolon... */
		if (!reading_map && line[0]==';')
			continue;
			
		/* STARTING WITH A BRACKET [ IS A SECTION */
		if (line[0]=='[') {
			if (g_ascii_strcasecmp (line, "[cave]")==0) {
				/* new cave */
				if (mapstrings) {
					g_warning("incorrect file format: new [cave] section, but already read some map lines");
					g_list_free(mapstrings);
					mapstrings=NULL;
				}
				cave_process_tags(gd_default_cave, tags, NULL);
				cave=gd_cave_new_from_cave(gd_default_cave);
				gd_cave_clear_strings(cave);	/* forget name of caveset, description of caveset... */
				gd_caveset=g_list_append (gd_caveset, cave);
				gd_cave_clear_highscore(cave);	/* copy cave, but do not copy highscore of game */
			}
			else if (g_ascii_strcasecmp (line, "[/cave]")==0) {
				cave_process_tags(cave, tags, mapstrings);
				g_list_free(mapstrings);
				mapstrings=NULL;
				/* set this to point the pseudo-cave which holds default values */
				cave=gd_default_cave;
			}
			else if (g_ascii_strcasecmp (line, "[map]")==0) {
				reading_map=TRUE;
				if (mapstrings!=NULL) {
					g_warning("incorrect file format: new [map] section, but already read some map lines");
					g_list_free(mapstrings);
					mapstrings=NULL;
				}
			}
			else if (g_ascii_strcasecmp (line, "[/map]")==0) {
				reading_map=FALSE;
			}
			else if (g_ascii_strcasecmp (line, "[mapcodes]")==0) {
				reading_mapcodes=TRUE;
			}
			else if (g_ascii_strcasecmp (line, "[/mapcodes]")==0) {
				reading_mapcodes=FALSE;
			}
			else if (g_ascii_strcasecmp (line, "[highscore]")==0) {
				reading_highscore=TRUE;
			}
			else if (g_ascii_strcasecmp (line, "[/highscore]")==0) {
				reading_highscore=FALSE;
			}
			else if (g_ascii_strcasecmp (line, "[objects]")==0) {
				reading_objects=TRUE;
			}
			else if (g_ascii_strcasecmp (line, "[/objects]")==0) {
				reading_objects=FALSE;
			}
			else if (g_ascii_strncasecmp (line, "[level=", strlen("[level="))==0) {
				int l[5];
				int num;
				char *nums;
				
				nums=strchr(line, '=')+1;	/* there IS an equal sign, and we also skip that, so this points to the numbers */
				num=sscanf(nums, "%d,%d,%d,%d,%d", l+0, l+1, l+2, l+3, l+4);
				levels=0;
				if (num==0) {
					g_warning("invalid Levels tag: %s", line);
					levels=GD_OBJECT_LEVEL_ALL;
				} else {
					int n;
					
					for (n=0; n<num; n++)
						if (l[n]<=5 && l[n]>=1)
							levels|=gd_levels_mask[l[n]-1];
						else
							g_warning("invalid level number %d", l[n]);
				}
				
			}
			else if (g_ascii_strcasecmp (line, "[/level]")==0) {
				levels=GD_OBJECT_LEVEL_ALL;
			}
			else if (g_ascii_strcasecmp(line, "[game]")==0) {
			}
			else if (g_ascii_strcasecmp(line, "[/game]")==0) {
			}
			else if (g_ascii_strcasecmp(line, "[BDCFF]")==0) {
			}
			else if (g_ascii_strcasecmp(line, "[/BDCFF]")==0) {
			}
			else
				g_warning("unknown section: \"%s\"", line);
			
			continue;
		}

		if (reading_map) {
			/* just append to the mapstrings list. we will process it later */
			mapstrings=g_list_append(mapstrings, line);
			
			continue;
		}

		/* strip leading and trailing spaces AFTER checking if we are reading a map. map lines might begin or end with spaces */
		g_strstrip(line);
		
		if (reading_highscore) {
			gchar **split;
			GdHighScore hs;
			int words;
			int i;
			
			split=g_strsplit_set(line, " ", -1);
			words=g_strv_length(split);
			if (sscanf(split[words-1], "%d", &hs.score)!=1) {
				g_warning (_("highscore format incorrect"));
			}
			g_strlcpy(hs.name, split[1], sizeof(GdString));	/* first word */
			for (i=2; i<words-1; i++) {
				g_strlcat(hs.name, " ", sizeof(GdString));	/* space and... */
				g_strlcat(hs.name, split[i], sizeof(GdString)); /* ... next word */
			}
			
			gd_cave_add_highscore(cave, hs);
			
			continue;
		}
		
		if (reading_objects) {
			GdObject *new_object;
			
			new_object=gd_object_new_from_string(line);
			if (new_object) {
				new_object->levels=levels;	/* apply levels to new object */
				cave->objects=g_list_append(cave->objects, new_object);
			}
			else
				g_critical("invalid object specification: %s", line);
				
			continue;
		}
		/* has an equal sign ->  some_attrib=parameters  type line. */
		
		if (strchr (line, '=')!=NULL) {
			char *attrib, *param;

			attrib=line;	/* attrib is from the first char */
			param=strchr(line, '=')+1;	/* param is after equal sign */
			*strchr (line, '=')=0;	/* delete equal sign - line is therefore splitted */
			
			if (reading_mapcodes) {
				if (g_ascii_strcasecmp ("Length", attrib)==0) {
					/* we do not support map code width!=1 */
					if (strcmp (param, "1")!=0)
						g_warning(_("Only one-character map codes are currently supported!"));
				} else
					/* the first character of the attribute is the element code itself */
					gd_char_to_element[(int)attrib[0]]=gd_get_element_from_string(param);
			}
			/* BDCFF version */
			else if (g_ascii_strcasecmp ("Version", attrib)==0) {
				g_strlcpy(version_read, param, sizeof(version_read));
			}
			/* CAVES=x */
			else if (g_ascii_strcasecmp (attrib, "Caves")==0) {
				/* BDCFF files sometimes state how many caves they have */
				/* we ignore this field. */
			}
			/* LEVELS=x */
			else if (g_ascii_strcasecmp (attrib, "Levels")==0) {
				/* BDCFF files sometimes state how many levels they have */
				/* we ignore this field. */
			}
			else if (g_ascii_strcasecmp (attrib, "CaveSize")==0) {
				int i;
				
				i=sscanf(param, "%d %d %d %d %d %d", cavesize+0, cavesize+1, cavesize+2, cavesize+3, cavesize+4, cavesize+5);
				/* allowed: 2 or 6 numbers */
				if (i==2) {
					cavesize[2]=0;
					cavesize[3]=0;
					cavesize[4]=cavesize[0]-1;
					cavesize[5]=cavesize[1]-1;
				} else
					if (i!=6) {
						set_cavesize_defaults();
						g_warning("invalid CaveSize tag: %s", line);
					}
			}
			else if (g_ascii_strcasecmp (attrib, "IntermissionSize")==0) {
				int i;
				
				i=sscanf(param, "%d %d %d %d %d %d", intermissionsize+0, intermissionsize+1, intermissionsize+2, intermissionsize+3, intermissionsize+4, intermissionsize+5);
				/* allowed: 2 or 6 numbers */
				if (i==2) {
					intermissionsize[2]=0;
					intermissionsize[3]=0;
					intermissionsize[4]=intermissionsize[0]-1;
					intermissionsize[5]=intermissionsize[1]-1;
				} else
					if (i!=6) {
						set_intermissionsize_defaults();
						g_warning("invalid IntermissionSize tag: %s", line);
					}
			}
			else
			/* CHECK IF IT IS AN EFFECT */
			if (g_ascii_strcasecmp(attrib, "Effect")==0) {
				char **params;
				
				params=g_strsplit_set(param, " ", -1);
				/* an effect command has two parameters */
				if (g_strv_length(params)==2) {
					int i;
					
					for (i=0; gd_cave_properties[i].identifier!=NULL; i++) {
						/* we have to search for this effect */
						if (gd_cave_properties[i].type==GD_TYPE_EFFECT && g_ascii_strcasecmp(params[0], gd_cave_properties[i].identifier)==0) {
							/* found identifier */
							gpointer value=G_STRUCT_MEMBER_P (cave, gd_cave_properties[i].offset);

							*((GdElement *) value)=gd_get_element_from_string (params[1]);
							break;
						}
					}
					if (gd_cave_properties[i].identifier==NULL) {
						/* if we didn't find first element name */

						/* for compatibility with tim stridmann's memorydump->bdcff converter... .... ... */
						if (g_ascii_strcasecmp(params[0], "BOUNCING_BOULDER")==0)
							cave->bouncing_stone_to=gd_get_element_from_string (params[1]);
						else if (g_ascii_strcasecmp(params[0], "EXPLOSION3S")==0)
							cave->explosion_to=gd_get_element_from_string (params[1]);
						/* falling with one l... */
						else if (g_ascii_strcasecmp(params[0], "STARTING_FALING_DIAMOND")==0)
							cave->falling_diamond_to=gd_get_element_from_string (params[1]);
						/* dirt lookslike */
						else if (g_ascii_strcasecmp(params[0], "DIRT")==0)
							cave->dirt_looks_like=gd_get_element_from_string (params[1]);
						else if (g_ascii_strcasecmp(params[0], "HEXPANDING_WALL")==0 && g_ascii_strcasecmp(params[1], "STEEL_HEXPANDING_WALL")==0) {
							cave->expanding_wall_looks_like=O_STEEL;
						}
						else
							/* didn't find at all */
							g_warning(_("invalid effect name \"%s\""), params[0]);
					}
				} else
					g_warning(_("Invalid effect specification \"%s\""), param);
				g_strfreev(params);
			}
			else
				/* if nothing else that should be treated explicitly, save to hash table */
				g_hash_table_insert(tags, g_strdup(attrib), g_strdup(param));
				
			continue;
		}
		
		g_critical("cannot parse line: %s", line);
	}
	if (mapstrings) {
		g_warning("incorrect file format: end of file, but still have some map lines read");
		g_list_free(mapstrings);
		mapstrings=NULL;
	}
	g_strfreev (lines);
	if (g_hash_table_size(tags)!=0) {
		g_warning("incorrect file format: some attributes are outside cave and map sections");
	}
	g_hash_table_destroy(tags);

	/* the [game] section had some values which are default if not specified in [cave] sections. */
	/* these are used only for loading, so forget them now */
	if (gd_default_cave->map) {
		g_warning(_("Invalid BDCFF: [game] section has a map"));
		gd_cave_map_free (gd_default_cave->map);
		gd_default_cave->map=NULL;
	}
	if (gd_default_cave->objects) {
		g_warning(_("Invalid BDCFF: [game] section has drawing objects defined"));
		/* free objects */
		g_list_foreach (gd_default_cave->objects, (GFunc) g_free, NULL);
		/* free list */
		g_list_free (gd_default_cave->objects);
		gd_default_cave->objects=NULL;
	}
	
	/* forget defaults which were given in [game] section. those are useless... also, this is needed for saving. */
	gd_cave_set_defaults (gd_default_cave);
	gd_error_set_context(NULL);
	
	/* old bdcff files hack. explanation follows. */
	/* there were 40x22 caves in c64 bd, intermissions were also 40x22, but the visible */
	/* part was the upper left corner, 20x12. 40x22 caves are needed, as 20x12 caves would */
	/* look different (random cave elements needs the correct size.) */
	/* also, in older bdcff files, there is no size= tag. caves default to 40x22 and 20x12. */
	/* even the explicit drawrect and other drawing instructions, which did set up intermissions */
	/* to be 20x12, are deleted. very very bad decision. */
	/* here we try to detect and correct this. */
	if (g_str_equal(version_read, "0.32")) {
		GList *iter;
		
		g_warning("No BDCFF version, or 0.32. Using unspecified-intermission-size hack.");
		
		for (iter=gd_caveset; iter!=NULL; iter=iter->next) {
			Cave *cave=(Cave *)iter->data;
			
			/* only applies to intermissions */
			/* not applied to mapped caves, as maps are filled with initial border, if the map read is smaller */
			if (cave->intermission && !cave->map) {
				/* we do not set the cave to 20x12, rather to 40x22 with 20x12 visible. */
				GdObject object;

				cave->w=40;
				cave->h=22;
				cave->x1=0; cave->y1=0;
				cave->x2=19; cave->y2=11;
				
				/* and cover the invisible area */
				object.type=FILLED_RECTANGLE;
				object.x1=0; object.y1=11;	/* 11, because this will also be the border */
				object.x2=39; object.y2=21;
				object.element=cave->initial_border;
				object.fill_element=cave->initial_border;
				cave->objects=g_list_prepend(cave->objects, g_memdup(&object, sizeof(object)));

				object.x1=19; object.y1=0;	/* 19, as it is also the border */
				cave->objects=g_list_prepend(cave->objects, g_memdup(&object, sizeof(object)));	/* another */
			}
		}
	}

	if (!g_str_equal(version_read, BDCFF_VERSION))
		g_warning("BDCFF version %s, loaded caveset may have errors.", version_read);

	/* newly loaded file is not edited. */
	gd_caveset_edited=FALSE;
	/* if there was some error message */
	if (gd_has_new_error())
		g_warning("The BDCFF file contained errors.");
	return !gd_has_new_error();
}



/** Load caveset from file.
	Loads the caveset from a file.	

	File type is autodetected by extension.
	@param filename Name of file.
	@result FALSE if failed
*/
gboolean
gd_caveset_load_from_file (const char *filename, const char *configdir)
{
	GError *error=NULL;
	gsize length;
	char *buf;
	char *name;
	char *c;
	gboolean read;
	GList *new_caveset;
	struct stat st;

	gd_clear_error_flag();
	gd_error_set_context(gd_filename_to_utf8(filename));
	
	if (g_stat(filename, &st)!=0) {
		g_warning("cannot stat() file");
		gd_error_set_context(NULL);
		return FALSE;
	}
	if (st.st_size>1048576) {
		g_warning("file bigger than 1MiB, refusing to load");
		gd_error_set_context(NULL);
		return FALSE;
	}
	read=g_file_get_contents (filename, &buf, &length, &error);
	if (!read) {
		g_warning("%s", error->message);
		g_error_free(error);
		gd_error_set_context(NULL);
		return FALSE;
	}
	gd_error_set_context(NULL);

	/* BDCFF */
	if (gd_caveset_imported_format((guint8 *) buf)==UNKNOWN) {
		/* try to load as bdcff */
		gboolean result;

		result=caveset_load_from_bdcff(buf);	/* bdcff: start another function */
		g_free(buf);
		return result;
	}

	/* try to load as a binary file, as we know the format */
	new_caveset=gd_caveset_import_from_buffer ((guint8 *) buf, length);
	g_free(buf);

	/* if unable to load, exit here. error was reported by import_from_buffer() */	
	if (!new_caveset)
		return FALSE;

	/* no serious error :) */

	gd_caveset_clear();		/* only clear caveset here. if file read was unsuccessful, caveset remains in memory. */
	gd_caveset=new_caveset;
	gd_caveset_edited=FALSE;	/* newly loaded cave is not edited */
	
	/* and make up a caveset name from the filename. */
	name=g_path_get_basename(filename);
	g_strlcpy(gd_default_cave->name, name, sizeof(gd_default_cave->name));
	g_free(name);
	/* convert underscores to spaces */
	while ((c=strchr (gd_default_cave->name, '_'))!=NULL)
		*c=' ';
	/* remove extension */
	if ((c=strrchr (gd_default_cave->name, '.'))!=NULL)
		*c=0;

	/* try to load highscore */
	gd_load_highscore(configdir);
	return !gd_has_new_error();
}

Cave *
gd_return_nth_cave(const int cave)
{
	return g_list_nth_data(gd_caveset, cave);
}

/* pick a cave, identified by a number, and render it with level number. */
Cave *
gd_cave_new_from_caveset (const int cave, const int level, guint32 seed)
{
	return gd_cave_new_rendered (gd_return_nth_cave(cave), level, seed);
}

const gchar **
gd_caveset_get_internal_game_names ()
{
	return level_names;
}

/* load some caveset from the #include file. */
gboolean
gd_caveset_load_from_internal (const int i, const char *configdir)
{
	if (!(i >= 0 && i < G_N_ELEMENTS (level_pointers) - 1))
		return FALSE;

	gd_caveset_clear ();
	gd_caveset=gd_caveset_import_from_buffer (level_pointers[i], -1);
	g_strlcpy(gd_default_cave->name, level_names[i], sizeof(gd_default_cave->name));
	
	gd_load_highscore(configdir);	

	return TRUE;
}

/* output properties of a cave to a file.
	this is a glist foreach func. */
static void
caveset_save_cave_func (Cave *cave, GString *fout)
{
	int i, j;
	gboolean parameter_written=FALSE, should_write=FALSE;
	GString *line;
	const char *identifier=NULL;

	g_string_append (fout, "[cave]\n");
	write_highscore_func(cave, fout);
	
	line=g_string_new (NULL);

	for (i=0; gd_cave_properties[i].identifier!=NULL; i++) {
		gpointer value, default_value;

		if (gd_cave_properties[i].type==GD_TAB || gd_cave_properties[i].type==GD_LABEL || gd_cave_properties[i].type==GD_LEVEL_LABEL)
			/* used only by the gui */
			continue;
		
		/* these are handled explicitly */
		if (gd_cave_properties[i].flags & GD_DONT_SAVE)
			continue;
			
		if (gd_cave_properties[i].flags & GD_ALWAYS_SAVE)
			should_write=TRUE;

		/* string data */
		/* write together with identifier, as one string per line. */
		if (gd_cave_properties[i].type==GD_TYPE_STRING) {
			/* treat strings as special - do not even write identifier if no string. */
			char *value=G_STRUCT_MEMBER_P (cave, gd_cave_properties[i].offset);

			if (strlen (value)>0)
				g_string_append_printf (fout, "%s=%s\n", gd_cave_properties[i].identifier, value);
			continue;
		}
		
		/* if identifier is empty string, the property is handled below. bdcff=yuck */
		if (g_str_equal (gd_cave_properties[i].identifier, ""))
			continue;

		/* if identifier differs from the previous, write out the line collected, and start a new one */
		if (!identifier || strcmp (gd_cave_properties[i].identifier, identifier)!=0) {
			if (should_write) {
				/* write lines only which carry information other than the default settings */
				g_string_append (fout, line->str);
				g_string_append_c (fout, '\n');
			}

			if (gd_cave_properties[i].type==GD_TYPE_EFFECT)
				g_string_printf(line, "Effect=");
			else
				g_string_printf (line, "%s=", gd_cave_properties[i].identifier);
			parameter_written=FALSE;	/* no value written yet */
			should_write=FALSE;

			/* remember identifier */
			identifier=gd_cave_properties[i].identifier;
		}

		value=G_STRUCT_MEMBER_P (cave, gd_cave_properties[i].offset);
		default_value=G_STRUCT_MEMBER_P (gd_default_cave, gd_cave_properties[i].offset);
		for (j=0; j<gd_cave_properties[i].count; j++) {
			char buf[G_ASCII_DTOSTR_BUF_SIZE];

			/* separate values by spaces. of course no space required for the first one */
			if (parameter_written)
				g_string_append_c (line, ' ');
			parameter_written=TRUE;	/* at least one value written, so write space the next time */
			switch (gd_cave_properties[i].type) {
			case GD_TYPE_BOOLEAN:
				g_string_append_printf (line, "%s", ((gboolean *) value)[j] ? "true" : "false");
				if (((gboolean *) value)[j]!=((gboolean *) default_value)[j])
					should_write=TRUE;
				break;
			case GD_TYPE_INT:
				g_string_append_printf (line, "%d", ((int *) value)[j]);
				if (((int *) value)[j]!=((int *) default_value)[j])
					should_write=TRUE;
				break;
			case GD_TYPE_RATIO:
				g_ascii_formatd (buf, sizeof (buf), "%6.4f", ((int *) value)[j]/(double)(cave->w*cave->h));
				g_string_append_printf (line, "%s", buf);
				if (((int *) value)[j]!=((int *) default_value)[j])
					should_write=TRUE;
				break;
			case GD_TYPE_PROBABILITY:
				g_ascii_formatd (buf, sizeof (buf), "%6.4f", ((double *) value)[j]);
				g_string_append_printf (line, "%s", buf);

				if (((double *) value)[j]!=((double *) default_value)[j])
					should_write=TRUE;
				break;
			case GD_TYPE_ELEMENT:
				g_string_append_printf (line, "%s", gd_elements[((GdElement *) value)[j]].filename);
				if (((GdElement *) value)[j]!=((GdElement *) default_value)[j])
					should_write=TRUE;
				break;
			case GD_TYPE_EFFECT:
				/* for effects, the property identifier is the effect name. "Effect=" is hardcoded; see above. */
				g_string_append_printf (line, "%s %s", gd_cave_properties[i].identifier, gd_elements[((GdElement *) value)[j]].filename);
				if (((GdElement *) value)[j]!=((GdElement *) default_value)[j])
					should_write=TRUE;
				break;
			case GD_TYPE_COLOR:
				g_string_append_printf (line, "%s", gd_get_color_name(((GdColor *) value)[j]));
				should_write=TRUE;
				break;
			case GD_TYPE_DIRECTION:
				switch(((GdDirection *) value)[j]) {
					case MV_UP:
						g_string_append_printf (line, "up"); break;
					case MV_DOWN:
						g_string_append_printf (line, "down"); break;
					case MV_LEFT:
						g_string_append_printf (line, "left"); break;
					case MV_RIGHT:				
						g_string_append_printf (line, "right"); break;
					default:
						g_assert_not_reached();
				}
				if (((GdDirection *) value)[j]!=((GdDirection *) default_value)[j])
					should_write=TRUE;
				break;
			case GD_TYPE_SCHEDULING:
				g_assert_not_reached();
				break;
			case GD_TAB:
			case GD_LABEL:
			case GD_LEVEL_LABEL:
			case GD_TYPE_STRING:
				break;
			}
		}
	}
	/* write remaining data */
	if (should_write) {
		g_string_append (fout, line->str);
		g_string_append_c (fout, '\n');
		g_string_assign (line, "");
	}
	
	/* properties which are handled explicitly. these cannot be handled with the loop above,
	   as they have some special meaning. for example, slime_permeability=x sets permeability to
	   x, and sets predictable to false. bdcff format is simply inconsistent in these aspects.
	   explanations below.
	   */
	/* slime permeability is always set explicitly, as it also sets predictability. */
	if (!cave->slime_predictable) {
		char buf[G_ASCII_DTOSTR_BUF_SIZE];
		
		g_ascii_formatd (buf, sizeof (buf), "%6.4f", cave->slime_permeability);
		g_string_append_printf(fout, "SlimePermeability=%s\n", buf);
	}
	else
		g_string_append_printf(fout, "SlimePermeabilityC64=%d\n", cave->slime_permeability_c64);
	/* same for cavedelay and frametime */
	if (cave->scheduling==GD_SCHEDULING_MILLISECONDS)
		g_string_append_printf(fout, "FrameTime=%d %d %d %d %d\n", cave->level_speed[0], cave->level_speed[1], cave->level_speed[2], cave->level_speed[3], cave->level_speed[4]);
	else {
		g_string_append_printf(fout, "CaveDelay=%d %d %d %d %d\n", cave->level_ckdelay[0], cave->level_ckdelay[1], cave->level_ckdelay[2], cave->level_ckdelay[3], cave->level_ckdelay[4]);
		if (cave->scheduling!=GD_SCHEDULING_PLCK) /* plck is the default, if not milliseconds */
			g_string_append_printf(fout, "CaveScheduling=%s\n", gd_scheduling_filename[cave->scheduling]);
	}

	/* save unknown tags as they are */
	if (cave->tags) {
		GList *hashkeys;
		GList *iter;
		
		hashkeys=g_hash_table_get_keys(cave->tags);
		for (iter=hashkeys; iter!=NULL; iter=iter->next) {
			gchar *key=(gchar *)iter->data;

			g_string_append_printf(fout, "%s=%s\n", key, (const char *) g_hash_table_lookup(cave->tags, key));
		}
		g_list_free(hashkeys);
	}

	/* map */
	if (cave->map) {
		int x, y;

		g_string_append (fout, "[map]\n");
		g_string_set_size (line, cave->w);
		/* save map */
		for (y=0; y < cave->h; y++) {
			for (x=0; x < cave->w; x++) {
				/* check if character is non-zero; the ...save() should have assigned a character to every element */
				g_assert(gd_elements[cave->map[y][x]].character_new!=0);
				line->str[x]=gd_elements[cave->map[y][x]].character_new;
			}
			g_string_append (fout, line->str);
			g_string_append_c (fout, '\n');
		}
		g_string_append (fout, "[/map]\n");
	}
	g_string_free (line, TRUE);

	/* save drawing objects */
	if (cave->objects) {
		GList *listiter;

		g_string_append (fout, "[objects]\n");
		for (listiter=cave->objects; listiter; listiter=g_list_next (listiter)) {
			GdObject *object=listiter->data;
			char *text;
			
			text=gd_object_to_bdcff(object);
			/* not for all levels? */
			if (object->levels!=GD_OBJECT_LEVEL_ALL) {
				int i;
				gboolean once;	/* true if already written one number */
				
				g_string_append(fout, "[Level=");
				once=FALSE;
				for (i=0; i<5; i++) {
					if (object->levels & gd_levels_mask[i]) {
						if (once)	/* if written at least one number, we need a comma */
							g_string_append_c(fout, ',');
						g_string_append_printf(fout, "%d", i+1);
						once=TRUE;
					}
				}
				g_string_append(fout, "]\n");
			}
			g_string_append(fout, text);
			g_string_append_c(fout, '\n');
			if (object->levels!=GD_OBJECT_LEVEL_ALL)
				g_string_append(fout, "[/Level]\n");
			g_free(text);
			
		}
		g_string_append_printf (fout, "[/objects]\n");
	}
	g_string_append (fout, "[/cave]\n\n");
}

/* save cave in bdcff format. */
gboolean
gd_caveset_save (const char *filename)
{
	int i;
	GList *iter;
	GString *contents;
	GError *error=NULL;
	gboolean write_mapcodes=FALSE;
	
	if (error) {
		g_error_free(error);
		error=NULL;
	}
	
	/* check if we need an own mapcode table ------ */
	/* copy original characters to character_new fields; new elements will be added to that one */
	for (i=0; i<O_MAX; i++)
		gd_elements[i].character_new=gd_elements[i].character;
	/* also regenerate this table as we use it */
	gd_create_char_to_element_table();
	/* check all caves */
	for (iter=gd_caveset; iter!=NULL; iter=iter->next) {
		Cave *cave=(Cave *)iter->data;
		
		/* if they have a map (random elements+object based maps do not need characters) */
		if (cave->map) {
			int x, y;
			
			/* check every element of map */
			for(y=0; y<cave->h; y++)
				for (x=0; x<cave->w; x++) {
					GdElement e=cave->map[y][x];
					
					/* if no character assigned */
					if (gd_elements[e].character_new==0) {
						int j;
						
						/* we found at least one, so later we have to write the mapcodes */
						write_mapcodes=TRUE;
						
						/* find a character which is not yet used for any element */
						for (j=32; j<128; j++) {
							/* the string contains the characters which should not be used. */
							if (strchr("<>&[]/=\\", j)==NULL && gd_char_to_element[j]==O_UNKNOWN)
								break;
						}
						/* if no more space... XXX */
						g_assert(j!=128);
						
						gd_elements[e].character_new=j;
						/* we also record to this table, as we use it ^^ a few lines above */
						gd_char_to_element[j]=e;
					}
				}
			
		}
	}

	contents=g_string_sized_new (32000);
	g_string_append(contents, "[BDCFF]\n");
	g_string_append_printf(contents, "Version=%s\n\n", BDCFF_VERSION);

	/* this flag was set above if we need to write mapcodes */	
	if (write_mapcodes) {	
		int i;
		
		g_string_append(contents, "[mapcodes]\n");
		g_string_append(contents, "Length=1\n");
		for (i=0; i<O_MAX; i++) {
			/* if no character assigned by specification BUT (AND) we assigned one */
			if (gd_elements[i].character==0 && gd_elements[i].character_new!=0)
				g_string_append_printf(contents, "%c=%s\n", gd_elements[i].character_new, gd_elements[i].filename);
		}
		g_string_append(contents, "[/mapcodes]\n");
		g_string_append(contents, "\n");
	}

	g_string_append (contents, "[game]\n");
	write_highscore_func(gd_default_cave, contents);
	/* save the default pseudo-cave: only the strings */
	for (i=0; gd_cave_properties[i].identifier!=NULL; i++) {
		if (gd_cave_properties[i].type==GD_TYPE_STRING) {
			/* treat strings as special - do not even write identifier if no string. */
			char *value=G_STRUCT_MEMBER_P (gd_default_cave, gd_cave_properties[i].offset);

			if (strlen (value) > 0)
				g_string_append_printf (contents, "%s=%s\n", gd_cave_properties[i].identifier, value);
		}
	}
	g_string_append (contents, "Levels=5\n\n");
	
	/* for all caves */
	g_list_foreach (gd_caveset, (GFunc) caveset_save_cave_func, contents);
	g_string_append (contents, "[/game]\n\n");
	g_string_append (contents, "[/BDCFF]\n");

	if (!g_file_set_contents (filename, contents->str, contents->len, &error)) {
		/* could not save properly */
		g_warning("%s: %s", filename, error->message);
		g_error_free(error);
		g_string_free (contents, TRUE);
		return FALSE;
	}

	/* saved OK */
	gd_caveset_edited=FALSE;
	g_string_free(contents, TRUE);
	return TRUE;
}

