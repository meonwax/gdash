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
#include <gtk/gtk.h>
#include <stdlib.h>

#if 0
static void
put_pixel (GdkPixbuf *pixbuf, int x, int y, guchar red, guchar green, guchar blue, guchar alpha)
{
  int width, height, rowstride, n_channels;
  guchar *pixels, *p;

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
  g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
  g_assert (gdk_pixbuf_get_has_alpha (pixbuf));
  g_assert (n_channels == 4);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  g_assert (x >= 0 && x < width);
  g_assert (y >= 0 && y < height);

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  p = pixels + y * rowstride + x * n_channels;
  p[0] = red;
  p[1] = green;
  p[2] = blue;
  p[3] = alpha;
}
#endif

GdkPixbuf *pixbuf=NULL;
int width, height, rowstride, n_channels;
guchar *pixels;


static int
get_pixel (int x, int y)
{
  guchar *p;
  int r, g, b, a;
  int c;
  /* rgba table */
  int cols[]={
    /* abgr */
  	/* 0000 */ 0,
  	/* 0001 */ 0,
  	/* 0010 */ 0,
  	/* 0011 */ 0,
  	/* 0100 */ 0,
  	/* 0101 */ 0,
  	/* 0110 */ 0,
  	/* 0111 */ 0,
  	/* 1000 */ 1, /* black - background */
  	/* 1001 */ 2, /* red - foreg1 */
  	/* 1010 */ 5, /* green - amoeba xxxxx */
  	/* 1011 */ 4, /* yellow - foreg3 */
  	/* 1100 */ 6, /* blue - slime */
  	/* 1101 */ 3,	/* purple - foreg2 */
  	/* 1110 */ 7, /* black around arrows (used in editor) is coded as cyan */
  	/* 1111 */ 8, /* white is the arrow */
  };

  g_assert (x >= 0 && x < width);
  g_assert (y >= 0 && y < height);

  p = pixels + y * rowstride + x * n_channels;
  r=p[0];
  g=p[1];
  b=p[2];
  a=p[3];
  c=(a>>7)*8 + (b>>7)*4 + (g>>7)*2 + (r>>7)*1; /* lower 4 bits will be rgba */
  return cols[c];
}


int
main(int argc, char *argv[])
{
	int i;
	int w, h, x, y;
	
	gtk_init(&argc, &argv);
	pixbuf=gdk_pixbuf_new_from_file("cells_c64.png", NULL);
	g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
	g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
	g_assert (gdk_pixbuf_get_has_alpha (pixbuf));

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	g_assert (n_channels == 4);
	
	w=gdk_pixbuf_get_width(pixbuf);
	h=gdk_pixbuf_get_height(pixbuf);
	g_assert (w % 16 == 0);
	g_assert (h % 16 == 0);
	
	g_print("static const guchar c64_gfx[]={\n");
	g_print("/* cell size %d */\n", w/8);
	g_print("%d, \n", w/8);
	g_print("/* image data */\n");
	i=0;
	for (y=0; y<h; y++)
		for (x=0; x<w; x++) {
			g_print("%d, ", get_pixel(x, y));
			if (i++ >25) {
				g_print("\n");
				i=0;
			}
		}
	if (i!=0)
		g_print("\n");
	g_print("};\n");
	return 0;
}

