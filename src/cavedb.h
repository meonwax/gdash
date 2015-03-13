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
#ifndef _GD_CAVEDB_H
#define _GD_CAVEDB_H



#include <glib.h>
#include "cave.h"

extern GdElements gd_elements[];


extern const GdStructDescriptor gd_cave_properties[];

extern GdPropertyDefault gd_cave_defaults_gdash[];

extern GdC64Color gd_c64_colors[16];

#define GD_C64_BLACK (gd_c64_colors[0].rgb)
#define GD_C64_WHITE (gd_c64_colors[1].rgb)
#define GD_C64_RED (gd_c64_colors[2].rgb)
#define GD_C64_PURPLE (gd_c64_colors[4].rgb)
#define GD_C64_CYAN (gd_c64_colors[3].rgb)
#define GD_C64_GREEN (gd_c64_colors[5].rgb)
#define GD_C64_BLUE (gd_c64_colors[6].rgb)
#define GD_C64_YELLOW (gd_c64_colors[7].rgb)
#define GD_C64_ORANGE (gd_c64_colors[8].rgb)
#define GD_C64_BROWN (gd_c64_colors[9].rgb)
#define GD_C64_LIGHTRED (gd_c64_colors[10].rgb)
#define GD_C64_GRAY1 (gd_c64_colors[11].rgb)
#define GD_C64_GRAY2 (gd_c64_colors[12].rgb)
#define GD_C64_LIGHTGREEN (gd_c64_colors[13].rgb)
#define GD_C64_LIGHTBLUE (gd_c64_colors[14].rgb)
#define GD_C64_GRAY3 (gd_c64_colors[15].rgb)

/* do some checks on the cave db */
void gd_cave_db_init();

#endif

