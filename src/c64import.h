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
#ifndef _GD_CAVEIMPORT_H
#define _GD_CAVEIMPORT_H

#include <glib.h>

extern const char *gd_bd_internal_chars;
extern const GdElement gd_crazylight_import_table[];

GList* gd_caveset_import_from_buffer (const guint8 *buf, gsize length);

void gd_cave_set_crli_defaults(Cave *cave);
void gd_cave_set_crdr_defaults(Cave *cave);
void gd_cave_set_1stb_defaults(Cave *cave);
void gd_cave_set_plck_defaults(Cave *cave);
void gd_cave_set_bd2_defaults(Cave *cave);
void gd_cave_set_bd1_defaults(Cave *cave);

#endif

