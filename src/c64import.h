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
#ifndef _GD_CAVEIMPORT_H
#define _GD_CAVEIMPORT_H

#include <glib.h>
#include "cave.h"

extern const char gd_bd_internal_chars[];
extern const GdElement gd_crazylight_import_table[];

/* file formats */
typedef enum _gd_cavefile_format {
    GD_FORMAT_UNKNOWN,    /* unknown format */
    GD_FORMAT_BD1,    /* boulder dash 1 */
    GD_FORMAT_BD1_ATARI,    /* boulder dash 1 atari version */
    GD_FORMAT_DC1,    /* boulder dash 1, deluxe caves 1 extension - non-sloped brick wall. */
    GD_FORMAT_BD2,    /* boulder dash 2 with rockford's extensions */
    GD_FORMAT_BD2_ATARI,    /* boulder dash 2, atari version */
    GD_FORMAT_PLC,    /* peter liepa construction kit */
    GD_FORMAT_PLC_ATARI,    /* peter liepa construction kit, atari version */
    GD_FORMAT_DLB,    /* no one's delight boulder dash */
    GD_FORMAT_CRLI,    /* crazy light construction kit */
    GD_FORMAT_CRDR_7,    /* crazy dream 7 */
    GD_FORMAT_CRDR_9,    /* crazy dream 9 - is a crli caveset with hardcoded mazes */
    GD_FORMAT_FIRSTB,    /* first boulder */
} GdCavefileFormat;

/* engines */
typedef enum _gd_engine {
    GD_ENGINE_BD1,
    GD_ENGINE_BD2,
    GD_ENGINE_PLCK,
    GD_ENGINE_1STB,
    GD_ENGINE_CRDR7,
    GD_ENGINE_CRLI,
    GD_ENGINE_INVALID,    /* fake */
} GdEngine;
extern const char *gd_engines[];


GdCavefileFormat gd_caveset_imported_get_format(const guint8 *buf);
GList* gd_caveset_import_from_buffer (const guint8 *buf, gsize length);

void gd_cave_set_engine_defaults(GdCave *cave, GdEngine engine);
GdEngine gd_cave_get_engine_from_string(const char *param);
GdPropertyDefault *gd_get_engine_default_array(GdEngine engine);

void gd_c64_import_init_tables();

#endif

