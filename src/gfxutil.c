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
#include "config.h"
#include "gfxutil.h"


/* pal emulation for 32-bit rgba images. */
/* same applies to x coordinates. */
void
gd_pal_emu(gpointer pixels, int width, int height, int pitch, int rshift, int gshift, int bshift, int ashift)
{
	/* convolution "matrices" could be 5 numbers, ie. x-2, x-1, x, x+1, x+2... */
	/* but the output already has problems for x-1 and x+1. as the game only
	   pal_emus cells, not complete screens */
	/* convolution "matrix" for luminance */
	static const int lconv[] = {
		1, 3, 1,
	};
	/* for left edge of image. */
	static const int lconv_left[] = {
		1, 4, 2,
	};
	/* for right edge of image. */
	static const int lconv_right[] = {
		2, 4, 1,
	};
	/* convolution "matrix" for chrominance */
	static const int cconv[] = {
		1, 2, 1,
	};
	int ldiv=0, ldiv_left=0, ldiv_right=0, cdiv=0;
	const int scan_shade=217;	/* of 256 */

	int y, x;
	guint8* srcpix = (guint8*)pixels;
	typedef struct _yuv {
		/* guint8s should be enough, but operations must work on larger numbers. */
		/* so they would be converted for computations anyway... */
		gint y, u, v;
	} YUV;
	YUV **yuv;
	
	/* calculate divisors from convolution matrices */
	ldiv=lconv[0]+lconv[1]+lconv[2];
	ldiv_left=lconv_left[0]+lconv_left[1]+lconv_left[2];
	ldiv_right=lconv_right[0]+lconv_right[1]+lconv_right[2];
	cdiv=cconv[0]+cconv[1]+cconv[2];
	g_assert(ldiv!=0);
	g_assert(cdiv!=0);
	
	/* memory for yuv images */
	yuv=g_new(YUV *, height);
	for (y=0; y<height; y++)
		yuv[y]=g_new(YUV, width);
	
	/* convert to yuv */
	for (y=0; y<height; y++) {
		guint32 *row=(guint32 *)(srcpix+y*pitch);
		
		for (x=0; x<width; x++) {
			int r, g, b;
			
			r=(row[x]>>rshift)&0xff;
			g=(row[x]>>gshift)&0xff;
			b=(row[x]>>bshift)&0xff;
			
			yuv[y][x].y=((66*r+129*g+25*b+128)>>8)+16;
			yuv[y][x].u=((-38*r-74*g+112*b+128)>>8)+128;
			yuv[y][x].v=((112*r-94*g-18*b+128)>>8)+128;
		}
	}
	
	/* chroma downsampling */
	for (y=0; y<height; y+=2) {
		int ym, yp;

		/* turnaround coordinates */
		ym=y==0?height-1:y-1;
		yp=y==height-1?0:y+1;

		for (x=0; x<width; x++) {
			yuv[yp][x].u=yuv[y][x].u;
			yuv[y][x].v=yuv[ym][x].v;
		}
	}
	
	/* apply convolution matrices and convert back to rgb */
	for (y=0; y<height; y++) {
		guint32 *row=(guint32 *)(srcpix+y*pitch);

		for (x=0; x<width; x++) {
			int xm, xp;
			YUV n;	/* new value of this pixel in yuv */
			int c, d, e, r, g, b;

			/* turnaround coordinates for chroma */						
			xm=x==0?width-1:x-1;
			xp=x==width-1?0:x+1;

			/* luma */			
			if (x==0)
				n.y=(yuv[y ][xm].y*lconv_left[0]+yuv[y ][x].y*lconv_left[1]+yuv[y ][xp].y*lconv_left[2])/ldiv_left;
			else if (x==width-1)
				n.y=(yuv[y ][xm].y*lconv_right[0]+yuv[y ][x].y*lconv_right[1]+yuv[y ][xp].y*lconv_right[2])/ldiv_right;
			else
				n.y=(yuv[y ][xm].y*lconv[0]+yuv[y ][x].y*lconv[1]+yuv[y ][xp].y*lconv[2])/ldiv;
			/* chroma */
			n.u=(yuv[y ][xm].u*cconv[0]+yuv[y ][x].u*cconv[1]+yuv[y ][xp].u*cconv[2])/cdiv;
			n.v=(yuv[y ][xm].v*cconv[0]+yuv[y ][x].v*cconv[1]+yuv[y ][xp].v*cconv[2])/cdiv;
			/* scanline shade */
			if (y%2==1)
				n.y=n.y*scan_shade/256;

			/* back to rgb */
			c=n.y-16;
			d=n.u-128;
			e=n.v-128;
			r=CLAMP((298*c+409*e+128)>>8, 0, 255);
			g=CLAMP((298*c-100*d-208*e+128)>>8, 0, 255);
			b=CLAMP((298*c+516*d+128)>>8, 0, 255);
			
			/* preserve alpha channel */
			row[x]=(((row[x]>>ashift)&0xff)<<ashift) | (r<<rshift) | (g<<gshift) | (b<<bshift);
		}
		
	}

	/* free array */
	for (y=0; y<height; y++)
		g_free(yuv[y]);
	g_free(yuv);
}






/* somewhat optimized implementation of the Scale2x algorithm. */
/* http://scale2x.sourceforge.net */
void
gd_scale2x(guint8 *srcpix, int width, int height, int srcpitch, guint8 *dstpix, int dstpitch)
{
   	guint32 E0, E1, E2, E3, B, D, E, F, H;
	int y, x;

	for (y=0; y<height; ++y) {
		int ym, yp;
		
		ym=MAX(y-1, 0);
		yp=MIN(y+1, height-1);

		for (x=0; x<width; ++x) {
			int xm, xp;
			
			xm=MAX(x-1, 0);
			xp=MIN(x+1, width-1);

			B = *(guint32*)(srcpix + (ym*srcpitch) + (4*x));
			D = *(guint32*)(srcpix + (y*srcpitch) + (4*xm));
			E = *(guint32*)(srcpix + (y*srcpitch) + (4*x));
			F = *(guint32*)(srcpix + (y*srcpitch) + (4*xp));
			H = *(guint32*)(srcpix + (yp*srcpitch) + (4*x));
			
			if (B != H && D != F) {
				E0 = D == B ? D : E;
				E1 = B == F ? F : E;
				E2 = D == H ? D : E;
				E3 = H == F ? F : E;
			} else {
				E0 = E;
				E1 = E;
				E2 = E;
				E3 = E;
			}

			*(guint32*)(dstpix + y*2*dstpitch + x*2*4) = E0;
			*(guint32*)(dstpix + y*2*dstpitch + (x*2+1)*4) = E1;
			*(guint32*)(dstpix + (y*2+1)*dstpitch + x*2*4) = E2;
			*(guint32*)(dstpix + (y*2+1)*dstpitch + (x*2+1)*4) = E3;
		}
	}
}



void
gd_scale3x(guint8 *srcpix, int width, int height, int srcpitch, guint8 *dstpix, int dstpitch)
{
	int y, x;
   	guint32 E0, E1, E2, E3, E4, E5, E6, E7, E8, A, B, C, D, E, F, G, H, I;

	for (y=0; y<height; ++y) {
		int ym, yp;
		int ny=y*3;	/* new coordinate */
		
		ym=MAX(y-1, 0);
		yp=MIN(y+1, height-1);

		for (x=0; x<width; ++ x) {
			int xm, xp;
			int nx=x*3;	/* new coordinate */
			
			xm=MAX(x-1, 0);
			xp=MIN(x+1, width-1);

			A = *(guint32*)(srcpix + (ym*srcpitch + 4*xm));
			B = *(guint32*)(srcpix + (ym*srcpitch + 4*x));
			C = *(guint32*)(srcpix + (ym*srcpitch + 4*xp));
			D = *(guint32*)(srcpix + (y*srcpitch + 4*xm));
			E = *(guint32*)(srcpix + (y*srcpitch + 4*x));
			F = *(guint32*)(srcpix + (y*srcpitch + 4*xp));
			G = *(guint32*)(srcpix + (yp*srcpitch + 4*xm));
			H = *(guint32*)(srcpix + (yp*srcpitch + 4*x));
			I = *(guint32*)(srcpix + (yp*srcpitch + 4*xp));
			
			if (B != H && D != F) {
				E0 = D == B ? D : E;
				E1 = (D == B && E != C) || (B == F && E != A) ? B : E;
				E2 = B == F ? F : E;
				E3 = (D == B && E != G) || (D == H && E != A) ? D : E;
				E4 = E;
				E5 = (B == F && E != I) || (H == F && E != C) ? F : E;
				E6 = D == H ? D : E;
				E7 = (D == H && E != I) || (H == F && E != G) ? H : E;
				E8 = H == F ? F : E;
			} else {
				E0 = E;
				E1 = E;
				E2 = E;
				E3 = E;
				E4 = E;
				E5 = E;
				E6 = E;
				E7 = E;
				E8 = E;
			}

			*(guint32*)(dstpix + ny*dstpitch + nx*4) = E0;
			*(guint32*)(dstpix + ny*dstpitch + (nx+1)*4) = E1;
			*(guint32*)(dstpix + ny*dstpitch + (nx+2)*4) = E2;
			*(guint32*)(dstpix + (ny+1)*dstpitch + nx*4) = E3;
			*(guint32*)(dstpix + (ny+1)*dstpitch + (nx+1)*4) = E4;
			*(guint32*)(dstpix + (ny+1)*dstpitch + (nx+2)*4) = E5;
			*(guint32*)(dstpix + (ny+2)*dstpitch + nx*4) = E6;
			*(guint32*)(dstpix + (ny+2)*dstpitch + (nx+1)*4) = E7;
			*(guint32*)(dstpix + (ny+2)*dstpitch + (nx+2)*4) = E8;
		}
	}
}

