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

#include "config.h"

#include <glib/gi18n.h>
#include <fstream>
#include <stdexcept>
#include <cstdio>
#include "cave/cavestored.hpp"
#include "misc/printf.hpp"
#include "misc/logger.hpp"
#include "fileops/bdcffload.hpp"
#include "fileops/bdcffsave.hpp"
#include "cave/caveset.hpp"


/* list of possible extensions which can be opened */
const char *gd_caveset_extensions[]= {"*.gds", "*.bd", "*.bdr", "*.brc", "*.vsf", "*.mem", NULL};


PropertyDescription const CaveSet::descriptor[] = {
    /* default data */
    {"", GD_TAB, 0, N_("Caveset data")},
    {"Name", GD_TYPE_STRING, 0, N_("Name"), GetterBase::create_new(&CaveSet::name), N_("Name of the game")},
    {"Description", GD_TYPE_STRING, 0, N_("Description"), GetterBase::create_new(&CaveSet::description), N_("Some words about the game")},
    {"Author", GD_TYPE_STRING, 0, N_("Author"), GetterBase::create_new(&CaveSet::author), N_("Name of author")},
    {"Date", GD_TYPE_STRING, 0, N_("Date"), GetterBase::create_new(&CaveSet::date), N_("Date of creation")},
    {"WWW", GD_TYPE_STRING, 0, N_("WWW"), GetterBase::create_new(&CaveSet::www), N_("Web page or e-mail address")},
    {"Difficulty", GD_TYPE_STRING, 0, N_("Difficulty"), GetterBase::create_new(&CaveSet::difficulty), N_("Difficulty (informative)")},

    {"Charset", GD_TYPE_STRING, 0, N_("Character set"), GetterBase::create_new(&CaveSet::charset), N_("Theme used for displaying the game.")},
    {"Fontset", GD_TYPE_STRING, 0, N_("Font set"), GetterBase::create_new(&CaveSet::fontset), N_("Font used during the game.")},

    {"Lives", GD_TYPE_INT, 0, N_("Initial lives"), GetterBase::create_new(&CaveSet::initial_lives), N_("Number of lives you get at game start."), 3, 99},
    {"Lives", GD_TYPE_INT, 0, N_("Maximum lives"), GetterBase::create_new(&CaveSet::maximum_lives), N_("Maximum number of lives you can have by collecting bonus points."), 3, 99},
    {"BonusLife", GD_TYPE_INT, 0, N_("Bonus life score"), GetterBase::create_new(&CaveSet::bonus_life_score), N_("Number of points to collect for a bonus life."), 100, 5000},

    {"", GD_TAB, 0, N_("Story")},
    {"Story", GD_TYPE_LONGSTRING, 0, NULL, GetterBase::create_new(&CaveSet::story), N_("Long description of the game.")},
    {"", GD_TAB, 0, N_("Remark")},
    {"Remark", GD_TYPE_LONGSTRING, 0, NULL, GetterBase::create_new(&CaveSet::remark), N_("Remark (informative).")},

    {"TitleScreen", GD_TYPE_LONGSTRING, GD_DONT_SHOW_IN_EDITOR, N_("Title screen"), GetterBase::create_new(&CaveSet::title_screen), N_("Title screen image")},
    {"TitleScreenScroll", GD_TYPE_LONGSTRING, GD_DONT_SHOW_IN_EDITOR, N_("Title screen, scrolling"), GetterBase::create_new(&CaveSet::title_screen_scroll), N_("Scrolling background for title screen image")},

    {NULL},
};

CaveSet::CaveSet() {
    /* some bdcff defaults */
    initial_lives=3;
    maximum_lives=9;
    bonus_life_score=500;
}


/********************************************************************************
 *
 * highscores saving in config dir
 *
 */

/* calculates an adler checksum, for which it uses all
   elements of all cave-rendereds. */
unsigned CaveSet::checksum() const {
    unsigned a=1, b=0;
    for (unsigned int i=0; i<caves.size(); ++i) {
        CaveRendered rendered(*caves.at(i), 0, 0);  /* level=1, seed=0 */
        gd_cave_adler_checksum_more(rendered, a, b);
    }
    return (b<<16)+a;
}

/* adds highscores of one cave to a keyfile given in userdat. this is
   a g_list_foreach function.
   it guesses the index, which is written to the file; by checking
   the caveset (checking the index of cav in gd_caveset).
   groups in the keyfile cannot be cave names, as more caves in the
   caveset may have the same name. */
static void
cave_highscore_to_keyfile_func(GKeyFile *keyfile, const char *name, int index, HighScoreTable const &scores) {
    char cavstr[10];

    /* name of key group is the index */
    g_snprintf(cavstr, sizeof(cavstr), "%d", index);

    /* save highscores */
    for (unsigned i=0; i<scores.size(); i++)
        if (scores[i].score>0) {    /* only save, if score is not zero */
            char rankstr[10];
            char *str;

            /* key: rank */
            g_snprintf(rankstr, sizeof(rankstr), "%d", i+1);
            /* value: the score. for example: 510 Rob Hubbard */
            str=g_strdup_printf("%d %s", scores[i].score, scores[i].name.c_str());
            g_key_file_set_string(keyfile, cavstr, rankstr, str);
            g_free(str);
        }
    g_key_file_set_comment(keyfile, cavstr, NULL, name, NULL);
}

/* make up a filename for the current caveset, to save highscores in. */
/* returns the file name; owned by the function (no need to free()) */
const char *CaveSet::filename_for_cave_highscores(const char *directory) const {
    static char *outfile=NULL;

    g_free(outfile);

    guint32 cs=checksum();

    char *canon;
    if (name=="")
        canon = g_strdup("highscore-");
    else
        canon = g_strdup(name.c_str());
    /* allowed chars in the highscore file name; others are replaced with _ */
    g_strcanon(canon, "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", '_');
    char *fname=g_strdup_printf("%08x-%s.hsc", cs, canon);
    outfile=g_build_path(G_DIR_SEPARATOR_S, directory, fname, NULL);
    g_free(fname);
    g_free(canon);

    return outfile;
}

/* save highscores of the current cave to the configuration directory.
   this one chooses a filename on its own.
   it is used to save highscores for imported caves.
 */
void CaveSet::save_highscore(const char *directory) const {
    GKeyFile *keyfile=g_key_file_new();

    /* put the caveset highscores in the keyfile */
    cave_highscore_to_keyfile_func(keyfile, name.c_str(), 0, highscore);
    /* and put the highscores of all caves in the keyfile */
    for (unsigned int i=0; i<caves.size(); ++i) {
        CaveStored *cave=caves.at(i);
        cave_highscore_to_keyfile_func(keyfile, cave->name.c_str(), i+1, cave->highscore);
    }

    GError *error=NULL;
    char *data = g_key_file_to_data(keyfile, NULL, &error);
    /* don't know what might happen... report to the user and forget. */
    if (error) {
        gd_message(error->message);
        g_error_free(error);
        return;
    }
    if (!data) {
        gd_message("g_key_file_to_data returned NULL");
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
static bool
cave_highscores_load_from_keyfile(GKeyFile *keyfile, int i, HighScoreTable &scores) {
    char cavstr[10];

    /* check if keyfile has the group in question */
    g_snprintf(cavstr, sizeof(cavstr), "%d", i);
    if (!g_key_file_has_group(keyfile, cavstr))
        /* if the cave had no highscore, there is no group. this is normal! */
        return false;

    /* first clear highscores for the cave */
    scores.clear();

    /* for all keys... we ignore the keys itself, as rebuilding the sorted list is more simple */
    char **keys=g_key_file_get_keys(keyfile, cavstr, NULL, NULL);
    for (int j=0; keys[j]!=NULL; j++) {
        int score;
        char *str;

        str=g_key_file_get_string(keyfile, cavstr, keys[j], NULL);
        if (!str)   /* ?! not really possible but who knows */
            continue;

        if (strchr(str, ' ')!=NULL && sscanf(str, "%d", &score)==1)
            /* we skip the space by adding +1 */
            scores.add(strchr(str, ' ')+1, score);  /* add to the list, sorted. does nothing, if no more space for this score */
        else
            gd_message(CPrintf("Invalid line in highscore file: %s") % str);
        g_free(str);
    }
    g_strfreev(keys);

    return true;
}


/* load highscores from a file saved in the configuration directory.
   the file name is guessed automatically.
   if there is some highscore for a cave, then they are deleted.
*/
bool CaveSet::load_highscore(const char *directory) {
    const char *filename=filename_for_cave_highscores(directory);
    GKeyFile *keyfile=g_key_file_new();
    GError *error=NULL;
    bool success=g_key_file_load_from_file(keyfile, filename, G_KEY_FILE_NONE, &error);
    if (!success) {
        g_key_file_free(keyfile);
        /* skip file not found errors; report everything else. it is considered a normal thing when there is no .hsc file yet */
        if (error->domain==G_FILE_ERROR && error->code==G_FILE_ERROR_NOENT)
            return true;

        gd_warning(error->message);
        return false;
    }

    /* try to load for game */
    cave_highscores_load_from_keyfile(keyfile, 0, highscore);

    /* try to load for all caves */
    for (unsigned int i=0; i<caves.size(); ++i) {
        cave_highscores_load_from_keyfile(keyfile, i+1, caves.at(i)->highscore);
    }

    g_key_file_free(keyfile);
    return true;
}


/********************************************************************************
 *
 * Misc caveset functions
 *
 */

/* return index of first selectable cave */
int CaveSet::first_selectable_cave_index() const {
    for (unsigned int i=0; i<caves.size(); ++i) {
        if (caves.at(i)->selectable)
            return i;
    }

    gd_warning("no selectable cave in caveset!");
    /* and return the first one. */
    return 0;
}


void CaveSet::set_name_from_filename(const char *filename) {

    /* make up a caveset name from the filename. */
    char *name_str=g_path_get_basename(filename);
    /* convert underscores to spaces, remove extension */
    char *c;
    while ((c=strchr(name_str, '_'))!=NULL)
        *c=' ';
    if ((c=strrchr(name_str, '.'))!=NULL)
        *c=0;

    name=name_str;
    g_free(name_str);
}


/// Save caveset in BDCFF to the file.
/// @param filename The name of the file to write to.
/// @return true, if successful; false, if error.
void CaveSet::save_to_file(const char *filename) throw(std::runtime_error) {
    std::ofstream outfile;
    outfile.open(filename);
    if (!outfile)
        throw std::runtime_error(_("Could not open file for writing."));
    std::list<std::string> saved;
    save_to_bdcff(*this, saved);
    for (std::list<std::string>::const_iterator it=saved.begin(); it!=saved.end(); ++it)
        outfile << *it << std::endl;
    outfile.close();
    if (!outfile)
        throw std::runtime_error(_("Error writing to file."));
    /* remember savename and that now it is not edited */
    this->filename = filename;
    this->edited = false;
}

/// Check if there are any replays in the caveset.
/// @return True, if at least one of the caves has a replay.
bool CaveSet::has_replays() {
    /* for all caves */
    for (unsigned int i=0; i<caves.size(); ++i) {
        if (!caves.at(i)->replays.empty())
            return true;
    }

    /* no replays at all */
    return false;
}


/// Return the index of a cave.
/// 0 is the first.
/// @param cave The cave to look for
/// @return The index in the container, or -1 if not found
int CaveSet::cave_index(CaveStored const *cave) const {
    for (unsigned int i=0; i<caves.size(); ++i)
        if (caves.at(i)==cave)
            return i;

    return -1;  /* if not found */
}
