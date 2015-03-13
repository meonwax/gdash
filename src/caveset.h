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
#ifndef _GD_CAVESET_H
#define _GD_CAVESET_H

#include <glib.h>
#include "cave.h"

extern Cave *gd_default_cave;
extern GList *gd_caveset;
extern gboolean gd_caveset_edited;


gboolean gd_caveset_load_from_internal(int caveset, const char *configdir);
gboolean gd_caveset_load_from_file(const char *filename, const char *configdir);

gboolean gd_caveset_save(const char *filename);


const gchar **gd_caveset_get_internal_game_names();
int gd_caveset_count(void);
void gd_caveset_clear(void);
Cave *gd_return_nth_cave(const int cave);

int gd_caveset_first_selectable ();


Cave *gd_cave_new_from_caveset(const int cave, const int level, guint32 seed);


void gd_save_highscore(const char* directory);
gboolean gd_load_highscore(const char *directory);

#endif							/* _CAVESET_H */

