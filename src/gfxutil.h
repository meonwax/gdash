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

#define GD_TITLE_SCREEN_MAX_WIDTH 320
#define GD_TITLE_SCREEN_MAX_HEIGHT 192
#define GD_TITLE_SCROLL_MAX_WIDTH 320
#define GD_TITLE_SCROLL_MAX_HEIGHT 32

void gd_pal_emulate_raw(gpointer pixels, int width, int height, int pitch, int rshift, int gshift, int bshift, int ashift);

void gd_scale2x_raw(guint8 *srcpix, int width, int height, int srcpitch, guint8 *dstpix, int dstpitch);
void gd_scale3x_raw(guint8 *srcpix, int width, int height, int srcpitch, guint8 *dstpix, int dstpitch);

