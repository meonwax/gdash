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
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <string.h>
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
/* last selected-to-play cave */
int gd_caveset_last_selected;
int gd_caveset_last_selected_level;



/* list of possible extensions which can be opened */
char *gd_caveset_extensions[]={"*.gds", "*.bd", "*.bdr", "*.brc", NULL};



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

    {"Lives", GD_TYPE_INT, 0, N_("Initial lives"), CAVESET_OFFSET(initial_lives), 1, N_("Number of lives you get at game start."), 3, 9},
    {"Lives", GD_TYPE_INT, 0, N_("Maximum lives"), CAVESET_OFFSET(maximum_lives), 1, N_("Maximum number of lives you can have by collecting bonus points."), 3, 99},
    {"BonusLife", GD_TYPE_INT, 0, N_("Bonus life score"), CAVESET_OFFSET(bonus_life_score), 1, N_("Number of points to collect for a bonus life."), 100, 5000},

    {"Story", GD_TYPE_LONGSTRING, 0, N_("Story"), CAVESET_OFFSET(story), 1, N_("Long description of the game.")},
    {"Remark", GD_TYPE_LONGSTRING, 0, N_("Remark"), CAVESET_OFFSET(remark), 1, N_("Remark (informative).")},

    {"TitleScreen", GD_TYPE_LONGSTRING, GD_DONT_SHOW_IN_EDITOR, N_("Title screen"), CAVESET_OFFSET(title_screen), 1, N_("Title screen image")},
    {"TitleScreenScroll", GD_TYPE_LONGSTRING, GD_DONT_SHOW_IN_EDITOR, N_("Title screen, scrolling"), CAVESET_OFFSET(title_screen_scroll), 1, N_("Scrolling background for title screen image")},

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
    int i;

    data=g_new0(GdCavesetData, 1);

    /* create strings */
    for (i=0; gd_caveset_properties[i].identifier!=NULL; i++)
        if (gd_caveset_properties[i].type==GD_TYPE_LONGSTRING)
            G_STRUCT_MEMBER(GString *, data, gd_caveset_properties[i].offset)=g_string_new(NULL);

    gd_struct_set_defaults_from_array(data, gd_caveset_properties, caveset_defaults);

    return data;
}


void
gd_caveset_data_free(GdCavesetData *data)
{
    int i;

    /* free strings */
    for (i=0; gd_caveset_properties[i].identifier!=NULL; i++)
        if (gd_caveset_properties[i].type==GD_TYPE_LONGSTRING)
            g_string_free(G_STRUCT_MEMBER(GString *, data, gd_caveset_properties[i].offset), TRUE);

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
        GdCave *rendered;

        rendered=gd_cave_new_rendered(iter->data, 0, 0);    /* level=1, seed=0 */
        gd_cave_adler_checksum_more(rendered, &a, &b);
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
        if (scores[i].score>0) {    /* only save, if score is not zero */
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
        GdCave *cave=(GdCave *)iter->data;

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
        if (!str)    /* ?! not really possible but who knows */
            continue;

        if (strchr(str, ' ')!=NULL && sscanf(str, "%d", &score)==1)
            /* we skip the space by adding +1 */
            gd_add_highscore(scores, strchr(str, ' ')+1, score);    /* add to the list, sorted. does nothing, if no more space for this score */
        else
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
        GdCave *cave=iter->data;

        cave_highscores_load_from_keyfile(keyfile, i, cave->highscore);
    }

    g_key_file_free(keyfile);
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
static int
caveset_first_selectable_cave_index()
{
    GList *iter;
    int i;

    for (i=0, iter=gd_caveset; iter!=NULL; i++, iter=iter->next) {
        GdCave *cave=(GdCave *)iter->data;

        if (cave->selectable)
            return i;
    }

    g_warning("no selectable cave in caveset!");
    /* and return the first one. */
    return 0;
}

/* return a cave identified by its index */
GdCave *
gd_return_nth_cave(const int cave)
{
    return g_list_nth_data(gd_caveset, cave);
}


/* pick a cave, identified by a number, and render it with level number. */
GdCave *
gd_cave_new_from_caveset(const int cave, const int level, guint32 seed)
{
    return gd_cave_new_rendered (gd_return_nth_cave(cave), level, seed);
}







/* colors: 4: purple  3: ciklamen 2: orange 1: blue 0: green */

static GdElement brc_import_table[]=
{
    /* 0 */
    O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL, O_PRE_OUTBOX, O_OUTBOX, O_UNKNOWN, O_STEEL,
    O_H_EXPANDING_WALL, O_H_EXPANDING_WALL /* scanned */, O_FIREFLY_1 /* scanned */, O_FIREFLY_1 /* scanned */, O_FIREFLY_1, O_FIREFLY_2, O_FIREFLY_3, O_FIREFLY_4,
    /* 1 */
    O_BUTTER_1 /* scanned */, O_BUTTER_1 /* scanned */, O_BUTTER_1, O_BUTTER_2, O_BUTTER_3, O_BUTTER_4, O_PLAYER, O_PLAYER /* scanned */,
    O_STONE, O_STONE /* scanned */, O_STONE_F, O_STONE_F /* scanned */, O_DIAMOND, O_DIAMOND /* scanned */, O_DIAMOND_F, O_DIAMOND_F /* scanned */,
    /* 2 */
    O_NONE /* WILL_EXPLODE_THING */, O_EXPLODE_1, O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5, O_NONE /* WILL EXPLODE TO DIAMOND_THING */, O_PRE_DIA_1,
    O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4, O_PRE_DIA_5, O_AMOEBA, O_AMOEBA /* scanned */, O_SLIME, O_NONE,
    /* 3 */
    O_CLOCK, O_NONE /* clock eaten */, O_INBOX, O_PRE_PL_1, O_PRE_PL_2, O_PRE_PL_3, O_NONE, O_NONE,
    O_NONE, O_NONE, O_V_EXPANDING_WALL, O_NONE, O_VOODOO, O_UNKNOWN, O_EXPANDING_WALL, O_EXPANDING_WALL /* sc */,
    /* 4 */
    O_FALLING_WALL, O_FALLING_WALL_F, O_FALLING_WALL_F /* scanned */, O_UNKNOWN, O_ACID, O_ACID /* scanned */, O_NITRO_PACK, O_NITRO_PACK /* scanned */,
    O_NITRO_PACK_F, O_NITRO_PACK_F /* scanned */, O_NONE, O_NONE, O_NONE, O_NONE, O_NONE, O_NONE,
    /* 5 */
    O_NONE /* bomb explosion utolso */, O_UNKNOWN, O_NONE /* solid bomb glued */, O_UNKNOWN, O_STONE_GLUED, O_UNKNOWN, O_DIAMOND_GLUED, O_UNKNOWN,
    O_UNKNOWN, O_UNKNOWN, O_NONE, O_NONE, O_NONE, O_NONE, O_NONE, O_NONE,
    /* 6 */
    O_ALT_FIREFLY_1 /* scanned */, O_ALT_FIREFLY_1 /* scanned */, O_ALT_FIREFLY_1, O_ALT_FIREFLY_2, O_ALT_FIREFLY_3, O_ALT_FIREFLY_4, O_PLAYER_BOMB, O_PLAYER_BOMB /* scanned */,
    O_BOMB, O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3, O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
    /* 7 */
    O_BOMB_TICK_7, O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
};

static GdElement brc_effect_table[]=
{
    O_STEEL, O_DIRT, O_SPACE, O_STONE, O_STONE_F, O_STONE_GLUED, O_DIAMOND, O_DIAMOND_F, O_DIAMOND_GLUED, O_PRE_DIA_1,
    O_PLAYER, O_PRE_PL_1, O_PLAYER_BOMB, O_PRE_OUTBOX, O_OUTBOX, O_FIREFLY_1, O_FIREFLY_2, O_FIREFLY_3, O_FIREFLY_4,
    O_BUTTER_1, O_BUTTER_2, O_BUTTER_3, O_BUTTER_4, O_BRICK, O_MAGIC_WALL, O_H_EXPANDING_WALL, O_V_EXPANDING_WALL, O_EXPANDING_WALL,
    O_FALLING_WALL, O_FALLING_WALL_F, O_AMOEBA, O_SLIME, O_ACID, O_VOODOO, O_CLOCK, O_BOMB, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    O_ALT_FIREFLY_1, O_ALT_FIREFLY_2, O_ALT_FIREFLY_3, O_ALT_FIREFLY_4, O_ALT_BUTTER_1, O_ALT_BUTTER_2, O_ALT_BUTTER_3, O_ALT_BUTTER_4,
    O_EXPLODE_1, O_BOMB_EXPL_1, O_UNKNOWN,
};

static GdColor brc_color_table[]={
    0x518722, 0x3a96fa, 0xdb7618, 0xff3968,
    0x9b5fff, 0x0ee06c, 0xc25ea6, 0xf54826,
    0xf1ff26,
};

static GdColor brc_color_table_comp[]={
    0x582287, 0xfa9d39, 0x187ddb, 0x38ffd1,
    0xc1ff5e, 0xe00d81, 0x5dc27a, 0x27d3f5,
    0x3526ff,
};

static GdElement
brc_effect(guint8 byt)
{
    if (byt>=G_N_ELEMENTS(brc_effect_table)) {
        g_warning("invalid element identifier for brc effect: %02x", byt);
        return O_UNKNOWN;
    }

    return brc_effect_table[byt];
}


static void
brc_import(guint8 *data)
{
    int x, y;
    int level;
    /* we import 100 caves, and the put them in the correct order. */
    GdCave *imported[100];
    gboolean import_effect;

    g_assert(G_N_ELEMENTS(brc_color_table)==G_N_ELEMENTS(brc_color_table_comp));

    gd_caveset_clear();

    /* this is some kind of a version number */
    import_effect=FALSE;
    switch (data[23]) {
        case 0x0:
            /* nothing to do */
            break;
        case 0xde:
            /* import effects */
            import_effect=TRUE;
            break;
        default:
            g_warning("unknown brc version %02x", data[23]);
            break;
    }

    for (level=0; level<5; level++) {
        int cavenum;
        int i;

        for (cavenum=0; cavenum<20; cavenum++) {
            GdCave *cave;

            int c=5*20*24;    /* 5 levels, 20 caves, 24 bytes - max 40*2 properties for each cave */
            int datapos=(cavenum*5+level)*24+22;
            int colind;

            cave=gd_cave_new();
            imported[level*20+cavenum]=cave;
            if (cavenum<16)
                g_snprintf(cave->name, sizeof(GdString), "Cave %c/%d", 'A'+cavenum, level+1);
            else
                g_snprintf(cave->name, sizeof(GdString), "Intermission %d/%d", cavenum-15, level+1);

            /* fixed intermission caves; are smaller. */
            if (cavenum>=16) {
                cave->w=20;
                cave->h=12;
            }
            cave->map=gd_cave_map_new(cave, GdElement);

            for (y=0; y<cave->h; y++) {
                for (x=0; x<cave->w; x++) {
                    guint8 import;

                    import=data[y+level*24+cavenum*24*5+x*24*5*20];
                    // if (i==printcave) g_print("%2x", import);
                    if (import<G_N_ELEMENTS(brc_import_table))
                        cave->map[y][x]=brc_import_table[import];
                    else
                        cave->map[y][x]=O_UNKNOWN;
                }
            }

            for (i=0; i<5; i++) {
                cave->level_time[i]=data[0*c+datapos];
                cave->level_diamonds[i]=data[1*c+datapos];
                cave->level_magic_wall_time[i]=data[4*c+datapos];
                cave->level_amoeba_time[i]=data[5*c+datapos];
                cave->level_amoeba_threshold[i]=data[6*c+datapos];
                /* bonus time: 100 was added, so it could also be negative */
                cave->level_bonus_time[i]=(int)data[11*c+datapos+1]-100;
                cave->level_hatching_delay_frame[i]=data[10*c+datapos];

                /* this was not set in boulder remake. */
                cave->level_speed[i]=150;
            }
            cave->diamond_value=data[2*c+datapos];
            cave->extra_diamond_value=data[3*c+datapos];
            /* BRC PROBABILITIES */
            /* a typical code example:
                   46:if (random(slime*4)<4) and (tab[x,y+2]=0) then
                      Begin tab[x,y]:=0;col[x,y+2]:=col[x,y];tab[x,y+2]:=27;mat[x,y+2]:=9;Voice4:=2;end;
               where slime is the byte loaded from the file as it is.
               pascal random function generates a random number between 0..limit-1, inclusive, for random(limit).
               
               so a random number between 0..limit*4-1 is generated.
               for limit=1, 0..3, which is always < 4, so P=1.
               for limit=2, 0..7, 0..7 is < 4 in P=50%.
               for limit=3, 0..11, is < 4 in P=33%.
               So the probability is exactly 100%/limit. 
               just make sure we do not divide by zero for some broken input.
            */
            if (data[7*c+datapos]==0)
                g_warning("amoeba growth cannot be zero, error at byte %d", data[7*c+datapos]);
            else
                cave->amoeba_growth_prob=1E6/data[7*c+datapos]+0.5; /* 0.5 for rounding */
            if (data[8*c+datapos]==0)
                g_warning("amoeba growth cannot be zero, error at byte %d", data[8*c+datapos]);
            else
                cave->amoeba_fast_growth_prob=1E6/data[8*c+datapos]+0.5; /* 0.5 for rounding */
            cave->slime_predictable=FALSE;
            for (i=0; i<5; i++)
                cave->level_slime_permeability[i]=1E6/data[9*c+datapos]+0.5;   /* 0.5 for rounding */
            cave->acid_spread_ratio=1E6/data[10*c+datapos]+0.5; /* probability -> *1E6 */
            cave->pushing_stone_prob=1E6/data[11*c+datapos]+0.5;    /* br only allowed values 1..8 in here, but works the same way. prob -> *1E6 */
            cave->magic_wall_stops_amoeba=data[12*c+datapos+1]!=0;
            cave->intermission=cavenum>=16 || data[14*c+datapos+1]!=0;

            /* colors */
            colind=data[31*c+datapos]%G_N_ELEMENTS(brc_color_table);
            cave->colorb=0x000000;    /* fixed rgb black */
            cave->color0=0x000000;    /* fixed rgb black */
            cave->color1=brc_color_table[colind];
            cave->color2=brc_color_table_comp[colind];    /* complement */
            cave->color3=0xffffff;    /* white for brick */
            cave->color4=0xe5ad23;    /* fixed for amoeba */
            cave->color5=0x8af713;    /* fixed for slime */

            if (import_effect) {
                cave->amoeba_enclosed_effect=brc_effect(data[14*c+datapos+1]);
                cave->amoeba_too_big_effect=brc_effect(data[15*c+datapos+1]);
                cave->explosion_effect=brc_effect(data[16*c+datapos+1]);
                cave->bomb_explosion_effect=brc_effect(data[17*c+datapos+1]);
                /* 18 solid bomb explode to */
                cave->diamond_birth_effect=brc_effect(data[19*c+datapos+1]);
                cave->stone_bouncing_effect=brc_effect(data[20*c+datapos+1]);
                cave->diamond_bouncing_effect=brc_effect(data[21*c+datapos+1]);
                cave->magic_diamond_to=brc_effect(data[22*c+datapos+1]);
                cave->acid_eats_this=brc_effect(data[23*c+datapos+1]);
                /* slime eats: (diamond,boulder,bomb), (diamond,boulder), (diamond,bomb), (boulder,bomb) */
                cave->amoeba_enclosed_effect=brc_effect(data[14*c+datapos+1]);
            }
        }
    }

    /* put them in the caveset - take correct order into consideration. */
    for (level=0; level<5; level++) {
        int cavenum;

        for (cavenum=0; cavenum<20; cavenum++) {
            static const int reorder[]={0, 1, 2, 3, 16, 4, 5, 6, 7, 17, 8, 9, 10, 11, 18, 12, 13, 14, 15, 19};
            GdCave *cave=imported[level*20+reorder[cavenum]];
            gboolean only_dirt;
            int x, y;

            /* check if cave contains only dirt. that is an empty cave, and do not import. */
            only_dirt=TRUE;
            for (y=1; y<cave->h-1 && only_dirt; y++)
                for (x=1; x<cave->w-1 && only_dirt; x++)
                    if (cave->map[y][x]!=O_DIRT)
                        only_dirt=FALSE;

            /* append to caveset or forget it. */
            if (!only_dirt)
                gd_caveset=g_list_append(gd_caveset, cave);
            else
                gd_cave_free(cave);
        }
    }

#if 0
    /* debug TINGZ */
    g_print("  [CAVEA] [CAVEB] [CAVEC]\n");
    for (i=0; i<40; i++) {
        int datapos=22;
        g_print("%02d. %02x %02x   %02x %02x   %02x %02x", i, data[datapos+5*20*24*i], data[datapos+5*20*24*i+1], data[datapos+5*20*24*i+5*24], data[datapos+5*20*24*i+1+5*24], data[datapos+5*20*24*i+5*24*2], data[datapos+5*20*24*i+1+5*24*2]);
        g_print("\n");

    }
#endif
}







static void
caveset_name_set_from_filename(const char *filename)
{
    char *name;
    char *c;

    /* make up a caveset name from the filename. */
    name=g_path_get_basename(filename);
    gd_strcpy(gd_caveset_data->name, name);
    g_free(name);
    /* convert underscores to spaces */
    while ((c=strchr (gd_caveset_data->name, '_'))!=NULL)
        *c=' ';
    /* remove extension */
    if ((c=strrchr (gd_caveset_data->name, '.'))!=NULL)
        *c=0;
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
    gboolean read;
    GList *new_caveset;
    struct stat st;

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
    if (g_str_has_suffix(filename, ".brc") || g_str_has_suffix(filename, ".BRC")) {
        /* loading a boulder remake file */
        if (length!=96000) {
            g_warning("BRC files must be 96000 bytes long");
            gd_error_set_context(NULL);
            return FALSE;
        }
    }
    gd_error_set_context(NULL);


    if (g_str_has_suffix(filename, ".brc") || g_str_has_suffix(filename, "*.BRC")) {
        brc_import((guint8 *) buf);
        gd_caveset_edited=FALSE;    /* newly loaded cave is not edited */
        gd_caveset_last_selected=caveset_first_selectable_cave_index();
        gd_caveset_last_selected_level=0;
        g_free(buf);
        caveset_name_set_from_filename(filename);
        return TRUE;
    }

    /* BDCFF */
    if (gd_caveset_imported_get_format((guint8 *) buf)==GD_FORMAT_UNKNOWN) {
        /* try to load as bdcff */
        gboolean result;

        result=gd_caveset_load_from_bdcff(buf);    /* bdcff: start another function */
        gd_caveset_edited=FALSE;        /* newly loaded file is not edited. */
        gd_caveset_last_selected=caveset_first_selectable_cave_index();
        gd_caveset_last_selected_level=0;
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

    gd_caveset_clear();        /* only clear caveset here. if file read was unsuccessful, caveset remains in memory. */
    gd_caveset=new_caveset;
    gd_caveset_edited=FALSE;    /* newly loaded cave is not edited */
    gd_caveset_last_selected=caveset_first_selectable_cave_index();
    gd_caveset_last_selected_level=0;
    caveset_name_set_from_filename(filename);

    /* try to load highscore */
    gd_load_highscore(configdir);
    return TRUE;
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

    gd_caveset_edited=FALSE;
    gd_caveset_last_selected=caveset_first_selectable_cave_index();
    gd_caveset_last_selected_level=0;

    return TRUE;
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
    g_ptr_array_add(saved, NULL);    /* so it can be used for strjoinv */
#ifdef G_OS_WIN32
    contents=g_strjoinv("\r\n", (char **)saved->pdata);
#else
    contents=g_strjoinv("\n", (char **)saved->pdata);
#endif
    g_ptr_array_foreach(saved, (GFunc) g_free, NULL);
    g_ptr_array_free(saved, TRUE);

    success=g_file_set_contents(filename, contents, -1, &error);
    if (!success) {
        g_critical("%s", error->message);
        g_error_free(error);
    } else
        /* remember that it is saved */
        gd_caveset_edited=FALSE;

    g_free(contents);

    return success;
}



int
gd_cave_check_replays(GdCave *cave, gboolean report, gboolean remove, gboolean repair)
{
    GList *riter;
    int wrong=0;

    riter=cave->replays;
    while (riter!=NULL) {
        GdReplay *replay=(GdReplay *)riter->data;
        guint32 checksum;
        GdCave *rendered;
        GList *next=riter->next;

        rendered=gd_cave_new_rendered(cave, replay->level, replay->seed);
        checksum=gd_cave_adler_checksum(rendered);
        gd_cave_free(rendered);

        replay->wrong_checksum=FALSE;
        /* count wrong ones... the checksum might be changed later to "repair" */
        if (replay->checksum!=0 && checksum!=replay->checksum)
            wrong++;

        if (replay->checksum==0 || repair) {
            /* if no checksum found, add one. or if repair requested, overwrite old one. */
            replay->checksum=checksum;
        } else {
            /* if has a checksum, compare with this one. */
            if (replay->checksum!=checksum) {
                replay->wrong_checksum=TRUE;

                if (report)
                    g_warning("%s: replay played by %s at %s seems to be", cave->name, replay->player_name, replay->date);

                if (remove) {
                    /* may remove */
                    cave->replays=g_list_remove_link(cave->replays, riter);
                    gd_replay_free(replay);
                }
            }
        }

        /* advance to next list item which we remembered. the current one might have been deleted */
        riter=next;
    }

    return wrong;
}



gboolean gd_caveset_has_replays()
{
    GList *citer;

    /* for all caves */
    for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
        GdCave *cave=(GdCave *)citer->data;

        if (cave->replays)
            return TRUE;
    }

    /* if neither of the caves had a replay, */
    return FALSE;
}

