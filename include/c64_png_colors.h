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
#ifndef GD_C64_PNG_COLORS_H
#define GD_C64_PNG_COLORS_H

static inline int
c64_png_colors(int r, int g, int b, int a)
{
	static const int c64_png_cols[]={
	/* abgr */
	/* 0000 */ 0,	/* transparent */
	/* 0001 */ 0,
	/* 0010 */ 0,
	/* 0011 */ 0,
	/* 0100 */ 0,
	/* 0101 */ 0,
	/* 0110 */ 0,
	/* 0111 */ 0,
	/* 1000 */ 1, /* black - background */
	/* 1001 */ 2, /* red - foreg1 */
	/* 1010 */ 5, /* green - amoeba */
	/* 1011 */ 4, /* yellow - foreg3 */
	/* 1100 */ 6, /* blue - slime */
	/* 1101 */ 3, /* purple - foreg2 */
	/* 1110 */ 7, /* black around arrows (used in editor) is coded as cyan */
	/* 1111 */ 8, /* white is the arrow */
	};

	int c=(a>>7)*8 + (b>>7)*4 + (g>>7)*2 + (r>>7)*1;
	
	return c64_png_cols[c];
}

#endif

