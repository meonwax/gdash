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
#include <math.h>
#include <stdlib.h>
#include "settings.h"
#include "config.h"
#include "gfxutil.h"

/* pal emulation for 32-bit rgba images. */

/* used:
    y=0.299r+0.587g+0.114b
    u=b-y=-0.299r-0.587g+0.886b
    v=r-y=0.701r-0.587g-0.114b

    r=(r-y)+y=v+y
    b=(b-y)+y=u+y
    g=(y-0.299r-0.114b)/0.587=...=y-0.509v-0.194u

    we multiply every floating point value with 256:

    y=77r+150g+29b
    u=g-y=-77r-150g+227b
    v=r-y=179r-150g-29b

    256*r=v+y
    65536*g=256y-130v-50u
    256*b=u+y
*/
#define CROSSTALK_SIZE 16


typedef struct _yuv {
    /* we use values *256 here for fixed point math, so 8bits is not enough */
    int y, u, v;
} YUV;

static void
luma_blur (YUV **yuv, YUV **work, int width, int height)
{
    /* convolution "matrices" could be 5 numbers, ie. x-2, x-1, x, x+1, x+2... */
    /* but the output already has problems for x-1 and x+1. as the game only
       pal_emus cells, not complete screens */
    /* convolution "matrix" for luminance */
    static const int lconv[] = {
        1, 3, 1,    /* 2 4 2 */
    };
    /* for left edge of image. */
    static const int lconv_left[] = {
        1, 5, 3,
    };
    /* for right edge of image. */
    static const int lconv_right[] = {
        3, 5, 1,
    };
    static int ldiv=0, ldiv_left=0, ldiv_right=0;
    static gboolean first_run_done=FALSE;
    
    int y, x;

    if (!first_run_done) {
        first_run_done=TRUE;

        ldiv=lconv[0]+lconv[1]+lconv[2];
        ldiv_left=lconv_left[0]+lconv_left[1]+lconv_left[2];
        ldiv_right=lconv_right[0]+lconv_right[1]+lconv_right[2];
    }

    /* apply convolution matrix */
    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            int xm, xp;
            YUV n;    /* new value of this pixel in yuv */

            /* turnaround coordinates */
            xm=x==0?width-1:x-1;
            xp=x==width-1?0:x+1;

            /* luma blur */
            if (x==0)
                n.y=(yuv[y][xm].y*lconv_left[0]+yuv[y][x].y*lconv_left[1]+yuv[y][xp].y*lconv_left[2])/ldiv_left;
            else if (x==width-1)
                n.y=(yuv[y][xm].y*lconv_right[0]+yuv[y][x].y*lconv_right[1]+yuv[y][xp].y*lconv_right[2])/ldiv_right;
            else
                n.y=(yuv[y][xm].y*lconv[0]+yuv[y][x].y*lconv[1]+yuv[y][xp].y*lconv[2])/ldiv;
            
            work[y][x].y=n.y;
        }
    }

    for (y=0; y<height; y++)
        for (x=0; x<width; x++)
            yuv[y][x].y=work[y][x].y;
}


static void
chroma_blur(YUV **yuv, YUV **work, int width, int height)
{
    /* convolution "matrix" for chrominance */
    /* x-2, x-1, x, x+1, x+2 */
    static const int cconv[] = {
        1, 1, 1, 1, 1,
    };
    static int cdiv;
    static gboolean first_run_done=FALSE;
    
    int y, x;

    if (!first_run_done) {
        first_run_done=TRUE;

        /* calculate divisors from convolution matrices */
        cdiv=cconv[0]+cconv[1]+cconv[2]+cconv[3]+cconv[4];
    }

    /* apply convolution matrix */
    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            int xm, xm2, xp, xp2;

            /* turnaround coordinates */
            xm2=(x-2+width)%width;
            xm=(x-1+width)%width;
            xp=(x+1)%width;
            xp2=(x+1)%width;

            /* chroma blur */
            work[y][x].u=(yuv[y][xm2].u*cconv[0]+yuv[y][xm].u*cconv[1]+yuv[y][x].u*cconv[2]+yuv[y][xp].u*cconv[3]+yuv[y][xp2].u*cconv[4])/cdiv;
            work[y][x].v=(yuv[y][xm2].v*cconv[0]+yuv[y][xm].v*cconv[1]+yuv[y][x].v*cconv[2]+yuv[y][xp].v*cconv[3]+yuv[y][xp2].v*cconv[4])/cdiv;
        }
    }
    
    for (y=0; y<height; y++)
        for (x=0; x<width; x++) {
            yuv[y][x].u=work[y][x].u;
            yuv[y][x].v=work[y][x].v;
        }
}

static void
chroma_crosstalk_to_luma(YUV **yuv, YUV **work, int width, int height)
{
    /* arrays to store things */
    static int crosstalk_sin[CROSSTALK_SIZE];
    static int crosstalk_cos[CROSSTALK_SIZE];
    /* crosstalk will be amplitude/div; we use these two to have integer arithmetics */
    const int crosstalk_amplitude=192;
    const int crosstalk_div=256;
    const int crosstalk_div_edge=384;
    static gboolean crosstalk_calculated=FALSE;

    if (!crosstalk_calculated) {
        crosstalk_calculated=TRUE;
        int i;

        for (i=0; i<CROSSTALK_SIZE; i++) {
            double f;

            f=(double)i/CROSSTALK_SIZE*2.0*G_PI*2;
            crosstalk_sin[i]=crosstalk_amplitude*sin(f);
            crosstalk_cos[i]=crosstalk_amplitude*cos(f);
        }
    }
    
    int y, x;

    /* apply edge detection matrix */
    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {
            const int conv[]={-1, 1, 0, 0, 0,};
            int xm, xm2, xp, xp2;

            /* turnaround coordinates */
            xm2=(x-2+width)%width;
            xm=(x-1+width)%width;
            xp=(x+1)%width;
            xp2=(x+2)%width;

            /* edge detect */
            work[y][x].u=yuv[y][xm2].u*conv[0]+yuv[y][xm].u*conv[1]+yuv[y][x].u*conv[2]+yuv[y][xp].u*conv[3]+yuv[y][xp2].u*conv[4];
            work[y][x].v=yuv[y][xm2].v*conv[0]+yuv[y][xm].v*conv[1]+yuv[y][x].v*conv[2]+yuv[y][xp].v*conv[3]+yuv[y][xp2].v*conv[4];
        }
    }
    
    for (y=0; y<height; y++)
        for (x=0; x<width; x++) {
            int div=(x==0 || x==width-1)?crosstalk_div_edge:crosstalk_div;
            if (y/2%2==1)        /* y/2: for interlacing. */
                yuv[y][x].y+=(crosstalk_sin[x%CROSSTALK_SIZE]*work[y][x].u-crosstalk_cos[x%CROSSTALK_SIZE]*work[y][x].v)/div;    /* odd lines (/2) */
            else
                yuv[y][x].y+=(crosstalk_sin[x%CROSSTALK_SIZE]*work[y][x].u+crosstalk_cos[x%CROSSTALK_SIZE]*work[y][x].v)/div;    /* even lines (/2) */
        }
}


static void
scanline_shade(YUV **yuv, YUV **work, int width, int height)
{
    int y, x;
    int shade;

    if (gd_pal_emu_scanline_shade<0)
        gd_pal_emu_scanline_shade=0;
    if (gd_pal_emu_scanline_shade>100)
        gd_pal_emu_scanline_shade=100;
    shade=gd_pal_emu_scanline_shade*256/100;

    /* apply shade for every second row */
    for (y=0; y<height; y++)
        if (y%2==1)
            for (x=0; x<width; x++)
                yuv[y][x].y=yuv[y][x].y*shade/256;
}    


static inline int
clamp(int value, int min, int max)
{
    if (value<min)
        return min;
    if (value>max)
        return max;
    return value;
}



void
gd_pal_emulate_raw(gpointer pixels, int width, int height, int pitch, int rshift, int gshift, int bshift, int ashift)
{
    int y, x;
    guint8* srcpix = (guint8*)pixels;
    YUV **yuv, **alpha;    /* alpha is not really yuv, only y will be used. luma blur touches only y. */
    YUV **work;

    /* memory for yuv images */
    yuv=g_new(YUV *, height);
    for (y=0; y<height; y++)
        yuv[y]=g_new(YUV, width);
    work=g_new(YUV *, height);
    for (y=0; y<height; y++)
        work[y]=g_new(YUV, width);
    alpha=g_new(YUV *, height);
    for (y=0; y<height; y++)
        alpha[y]=g_new(YUV, width);

    /* convert to yuv */
    for (y=0; y<height; y++) {
        guint32 *row=(guint32 *)(srcpix+y*pitch);

        for (x=0; x<width; x++) {
            int r, g, b;

            r=(row[x]>>rshift)&0xff;
            g=(row[x]>>gshift)&0xff;
            b=(row[x]>>bshift)&0xff;

            /* now y, u, v will contain values * 256 */
            yuv[y][x].y= 77*r+150*g+ 29*b;
            yuv[y][x].u=-37*r -74*g+111*b;
            yuv[y][x].v=157*r-131*g -26*b;
            
            /* alpha is copied as is, and is not *256 */
            alpha[y][x].y=(row[x]>>ashift)&0xff;
        }
    }
    
    /* we give them an array to "work" in, so that is not free()d and malloc() four times */
    luma_blur(yuv, work, width, height);
    chroma_blur(yuv, work, width, height);
    chroma_crosstalk_to_luma(yuv, work, width, height);
    scanline_shade(yuv, work, width, height);
    
    luma_blur(alpha, work, width, height);

    /* convert back to rgb */
    for (y=0; y<height; y++) {
        guint32 *row=(guint32 *)(srcpix+y*pitch);

        for (x=0; x<width; x++) {
            int r, g, b;

            /* back to rgb */
            r=clamp((256*yuv[y][x].y                +292*yuv[y][x].v+32768)/65536, 0, 255);
            g=clamp((256*yuv[y][x].y-101*yuv[y][x].u-149*yuv[y][x].v+32768)/65536, 0, 255);
            b=clamp((256*yuv[y][x].y+519*yuv[y][x].u                +32768)/65536, 0, 255);

            /* alpha channel is preserved, others are converted back from yuv */
            row[x]=(alpha[y][x].y<<ashift) | (r<<rshift) | (g<<gshift) | (b<<bshift);
        }

    }

    /* free array */
    for (y=0; y<height; y++)
        g_free(yuv[y]);
    g_free(yuv);
    for (y=0; y<height; y++)
        g_free(alpha[y]);
    g_free(alpha);
    for (y=0; y<height; y++)
        g_free(work[y]);
    g_free(work);
}
#undef CROSSTALK_SIZE





/* somewhat optimized implementation of the Scale2x algorithm. */
/* http://scale2x.sourceforge.net */
void
gd_scale2x_raw(guint8 *srcpix, int width, int height, int srcpitch, guint8 *dstpix, int dstpitch)
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
gd_scale3x_raw(guint8 *srcpix, int width, int height, int srcpitch, guint8 *dstpix, int dstpitch)
{
    int y, x;
       guint32 E0, E1, E2, E3, E4, E5, E6, E7, E8, A, B, C, D, E, F, G, H, I;

    for (y=0; y<height; ++y) {
        int ym, yp;
        int ny=y*3;    /* new coordinate */

        ym=MAX(y-1, 0);
        yp=MIN(y+1, height-1);

        for (x=0; x<width; ++ x) {
            int xm, xp;
            int nx=x*3;    /* new coordinate */

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


