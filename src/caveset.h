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
#ifndef _GD_CAVESET_H
#define _GD_CAVESET_H

#include <glib.h>
#include "cave.h"

typedef struct _gd_caveset_data {
    GdString name;                /* Name of caveset */
    GdString description;        /* Some words about the caveset */
    GdString author;            /* Author */
    GdString difficulty;        /* difficulty of the caveset, for info purposes */
    GdString www;                /* link to author's webpage */
    GdString date;                /* date of creation */

    GString *story;                /* story for the caves */
    GString *remark;            /* notes about the game */
    
    GString *title_screen;        /* base64-encoded title screen image */
    GString *title_screen_scroll;    /* scrolling background for title screen image */

    GdString charset;            /* these are not used by gdash */
    GdString fontset;

    /* these are only for a game. */
    int initial_lives;            /* initial lives at game start */
    int maximum_lives;            /* maximum lives */
    int bonus_life_score;        /* bonus life / number of points */

    /* and this one the highscores */
    GdHighScore highscore[GD_HIGHSCORE_NUM];
} GdCavesetData;

extern const GdStructDescriptor gd_caveset_properties[];

extern GdCavesetData *gd_caveset_data;
extern GList *gd_caveset;
extern gboolean gd_caveset_edited;
extern int gd_caveset_last_selected;
extern int gd_caveset_last_selected_level;

extern char *gd_caveset_extensions[];

/* #included cavesets; configdir passed to look for .hsc file */
gboolean gd_caveset_load_from_internal(int caveset, const char *configdir);
const gchar **gd_caveset_get_internal_game_names();

/* caveset load from file; configdir passed to look for .hsc file */
gboolean gd_caveset_load_from_file(const char *filename, const char *configdir);
/* caveset save to bdcff file */
gboolean gd_caveset_save(const char *filename);

/* misc caveset functions */
int gd_caveset_count(void);
void gd_caveset_clear(void);
GdCave *gd_return_nth_cave(const int cave);
GdCave *gd_cave_new_from_caveset(const int cave, const int level, guint32 seed);

/* highscore in config directory */
void gd_save_highscore(const char* directory);
gboolean gd_load_highscore(const char *directory);

GdCavesetData *gd_caveset_data_new();
void gd_caveset_data_free(GdCavesetData *data);

/* check replays and optionally remove */
int gd_cave_check_replays(GdCave *cave, gboolean report, gboolean remove, gboolean repair);

gboolean gd_caveset_has_replays();


#endif                            /* _CAVESET_H */

