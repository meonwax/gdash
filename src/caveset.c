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
#include "cave.h"
#include "cavedb.h"
#include "caveobject.h"
#include "c64import.h"
#include "config.h"
#include "util.h"
#include "bdcff.h"

#include "caveset.h"

#include "levels.h"


/* this stores the caves. */
GList *gd_caveset;
/* the data of the caveset: name, highscore, max number of lives, etc. */
GdCavesetData *gd_caveset_data;
/* is set to true, when the caveset was edited since the last save. */
gboolean gd_caveset_edited;

#define CAVESET_OFFSET(property) (G_STRUCT_OFFSET(GdCavesetData, property))

const GdStructDescriptor
gd_caveset_properties[] = {
	/* default data */
	{"", GD_TAB, 0, N_("Caveset data")},
	{"Name", GD_TYPE_STRING, 0, N_("Name"), CAVESET_OFFSET(name), 1, N_("Name of the game")},
	{"Description", GD_TYPE_STRING, 0, N_("Description"), CAVESET_OFFSET(description), 1, N_("Some words about the game")},
	{"Author", GD_TYPE_STRING, 0, N_("Author"), CAVESET_OFFSET(author), 1, N_("Name of author")},
	{"Date", GD_TYPE_STRING, 0, N_("Date"), CAVESET_OFFSET(date), 1, N_("Date of creation")},
	{"WWW", GD_TYPE_STRING, 0, N_("WWW"), CAVESET_OFFSET(www), 1, N_("Web page or e-mail address")},
	{"Difficulty", GD_TYPE_STRING, 0, N_("Difficulty"), CAVESET_OFFSET(difficulty), 1, N_("Difficulty (informative)")},
	{"Remark", GD_TYPE_STRING, 0, N_("Remark"), CAVESET_OFFSET(remark), 1, N_("Remark (informative)")},

	{"Lives", GD_TYPE_INT, 0, N_("Initial lives"), CAVESET_OFFSET(initial_lives), 1, N_("Number of lives you get at game start."), 3, 9},
	{"Lives", GD_TYPE_INT, 0, N_("Maximum lives"), CAVESET_OFFSET(maximum_lives), 1, N_("Maximum number of lives you can have by collecting bonus points."), 3, 99},
	{"BonusLife", GD_TYPE_INT, 0, N_("Bonus life score"), CAVESET_OFFSET(bonus_life_score), 1, N_("Number of points to collect for a bonus life."), 100, 5000},
	{NULL},
};


static GdPropertyDefault
caveset_defaults[] = {
	/* default data */
	{CAVESET_OFFSET(initial_lives), 3},
	{CAVESET_OFFSET(maximum_lives), 9},
	{CAVESET_OFFSET(bonus_life_score), 500},
	{-1},
};


GdCavesetData *
gd_caveset_data_new()
{
	GdCavesetData *data;
	
	data=g_new0(GdCavesetData, 1);
	gd_struct_set_defaults_from_array(data, gd_caveset_properties, caveset_defaults);
	
	return data;
}


void
gd_caveset_data_free(GdCavesetData *data)
{
	g_free(data);
}




/********************************************************************************
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
		Cave *rendered=gd_cave_new_rendered(iter->data, 0, 0);	/* level=1, seed=0 */
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

/* adds highscores of one cave to a keyfile given in userdat. this is
   a g_list_foreach function.
   it guesses the index, which is written to the file; by checking
   the caveset (checking the index of cav in gd_caveset).
   groups in the keyfile cannot be cave names, as more caves in the
   caveset may have the same name. */
static void
cave_highscore_to_keyfile_func(GKeyFile *keyfile, const char *name, int index, GdHighScore *scores)
{
	int i;
	char cavstr[10];

	/* name of key group is the index */
	g_snprintf(cavstr, sizeof(cavstr), "%d", index);

	/* save highscores */
	for (i=0; i<GD_HIGHSCORE_NUM; i++)
		if (scores[i].score>0) {	/* only save, if score is not zero */
			char rankstr[10];
			char *str;
			
			/* key: rank */
			g_snprintf(rankstr, sizeof(rankstr), "%d", i+1);
			/* value: the score. for example: 510 Rob Hubbard */
			str=g_strdup_printf("%d %s", scores[i].score, scores[i].name);
			g_key_file_set_string(keyfile, cavstr, rankstr, str);
			g_free(str);
		}
	g_key_file_set_comment(keyfile, cavstr, NULL, name, NULL);
}

/* make up a filename for the current caveset, to save highscores in. */
/* returns the file name; owned by the function (no need to free()) */
static const char *
filename_for_cave_highscores(const char *directory)
{
	char *fname, *canon;
	static char *outfile=NULL;
	guint32 checksum;
	
	g_free(outfile);

	checksum=caveset_checksum();

	if (g_str_equal(gd_caveset_data->name, ""))
		canon=g_strdup("highscore-");
	else	
		canon=g_strdup(gd_caveset_data->name);
	/* allowed chars in the highscore file name; others are replaced with _ */
	g_strcanon(canon, "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", '_');
	fname=g_strdup_printf("%08x-%s.hsc", checksum, canon);
    outfile=g_build_path(G_DIR_SEPARATOR_S, directory, fname, NULL);
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
	GList *iter;
	int i;
	
	keyfile=g_key_file_new();

	/* put the caveset highscores in the keyfile */	
	cave_highscore_to_keyfile_func(keyfile, gd_caveset_data->name, 0, gd_caveset_data->highscore);
	/* and put the highscores of all caves in the keyfile */
	for (iter=gd_caveset, i=1; iter!=NULL; iter=iter->next, i++) {
		Cave *cave=(Cave *)iter->data;
		
		cave_highscore_to_keyfile_func(keyfile, cave->name, i, cave->highscore);
	}
	
	data=g_key_file_to_data(keyfile, NULL, &error);
	/* don't know what might happen... report to the user and forget. */
	if (error) {
		g_warning("%s", error->message);
		g_error_free(error);
		return;
	}
	if (!data) {
		g_warning("g_key_file_to_data returned NULL");
		return;
	}
	g_key_file_free(keyfile);

	/* if data came out empty, we do nothing. */
	if (strlen(data)>0) {
		g_mkdir_with_parents(directory, 0700);
	    g_file_set_contents(filename_for_cave_highscores(directory), data, -1, &error);
	}
    g_free(data);
}


/* load cave highscores from a parsed GKeyFile. */
/* i is the keyfile group, a number which is 0 for the game, and 1+ for caves. */
static gboolean
cave_highscores_load_from_keyfile(GKeyFile *keyfile, int i, GdHighScore *scores)
{
	char cavstr[10];
	char **keys;
	int j;
	
	/* check if keyfile has the group in question */
	g_snprintf(cavstr, sizeof(cavstr), "%d", i);
	if (!g_key_file_has_group(keyfile, cavstr))
		/* if the cave had no highscore, there is no group. this is normal! */
		return FALSE;
	
	/* first clear highscores for the cave */
	gd_clear_highscore(scores);

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
			gd_strcpy(hs.name, strchr(str, ' ')+1);	/* we skip the space by adding +1 */
			gd_add_highscore(scores, hs);	/* add to the list, sorted. does nothing, if no more space for this score */
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
	
	/* try to load for game */
	cave_highscores_load_from_keyfile(keyfile, 0, gd_caveset_data->highscore);

	/* try to load for all caves */
	for (iter=gd_caveset, i=1; iter!=NULL; iter=iter->next, i++) {
		Cave *cave=iter->data;
		
		cave_highscores_load_from_keyfile(keyfile, i, cave->highscore);
	}
	
	g_key_file_free(keyfile);
	return TRUE;
}















/* Load caveset from file.
	Loads the caveset from a file.	

	File type is autodetected by extension.
	param filename: Name of file.
	result: FALSE if failed
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
	if (gd_caveset_imported_format((guint8 *) buf)==GD_FORMAT_UNKNOWN) {
		/* try to load as bdcff */
		gboolean result;

		result=gd_caveset_load_from_bdcff(buf);	/* bdcff: start another function */
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
	gd_strcpy(gd_caveset_data->name, name);
	g_free(name);
	/* convert underscores to spaces */
	while ((c=strchr (gd_caveset_data->name, '_'))!=NULL)
		*c=' ';
	/* remove extension */
	if ((c=strrchr (gd_caveset_data->name, '.'))!=NULL)
		*c=0;

	/* try to load highscore */
	gd_load_highscore(configdir);
	return !gd_has_new_error();
}





/********************************************************************************
 *
 * #included caves
 *
 */

/* return the names of #include builtin caves */
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

	gd_caveset_clear();
	gd_caveset=gd_caveset_import_from_buffer(level_pointers[i], -1);
	gd_strcpy(gd_caveset_data->name, level_names[i]);
	
	gd_load_highscore(configdir);	

	return TRUE;
}






/********************************************************************************
 *
 * Misc caveset functions
 *
 */

/** Clears all caves in the caveset. also to be called at application start */
void
gd_caveset_clear()
{
	if (gd_caveset) {
		g_list_foreach(gd_caveset, (GFunc) gd_cave_free, NULL);
		g_list_free(gd_caveset);
		gd_caveset=NULL;
	}
	
	if (gd_caveset_data) {
		g_free(gd_caveset_data);
		gd_caveset_data=NULL;
	}

	/* always newly create this */
	/* create pseudo cave containing default values */
	gd_caveset_data=gd_caveset_data_new();
	gd_strcpy(gd_caveset_data->name, _("New caveset"));
}


/* return number of caves currently in memory. */
int
gd_caveset_count()
{
	return g_list_length (gd_caveset);
}



/* return index of first selectable cave */
int
gd_caveset_first_selectable()
{
	GList *iter;
	int i;
	
	for (i=0, iter=gd_caveset; iter!=NULL; i++, iter=iter->next) {
		Cave *cave=(Cave *)iter->data;
		
		if (cave->selectable)
			return i;
	}

	g_warning("no selectable cave in caveset!");	
	/* and return the first one. */
	return 0;
}

/* return a cave identified by its index */
Cave *
gd_return_nth_cave(const int cave)
{
	return g_list_nth_data(gd_caveset, cave);
}


/* pick a cave, identified by a number, and render it with level number. */
Cave *
gd_cave_new_from_caveset(const int cave, const int level, guint32 seed)
{
	return gd_cave_new_rendered (gd_return_nth_cave(cave), level, seed);
}



gboolean
gd_caveset_save(const char *filename)
{
	GPtrArray *saved;
	char *contents;
	GError *error=NULL;
	gboolean success;
	
	saved=g_ptr_array_sized_new(500);
	gd_caveset_save_to_bdcff(saved);
	g_ptr_array_add(saved, NULL);	/* so it can be used for strjoinv */
#ifdef G_OS_WIN32
	contents=g_strjoinv("\r\n", (char **)saved->pdata);
#else
	contents=g_strjoinv("\n", (char **)saved->pdata);
#endif
	g_ptr_array_foreach(saved, (GFunc) g_free, NULL);
	g_ptr_array_free(saved, TRUE);
	
	gd_clear_error_flag();	
	success=g_file_set_contents(filename, contents, -1, &error);
	if (!success) {
		g_critical(error->message);
		g_error_free(error);
	} else
		/* remember that it is saved */
		gd_caveset_edited=FALSE;

	g_free(contents);
		
	return success;
}
