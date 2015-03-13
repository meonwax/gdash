/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
 *
 * These are modified versions of some functions originating from SDL_gfx.
 * See the comment below.
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

/*

Copyright (C) 2001-2011  Andreas Schiffler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

Andreas Schiffler -- aschiffler at ferzkopp dot net

*/

#include <SDL.h>
#include "sdl/gfxprim.hpp"

/*!
\brief Internal function to draw filled rectangle with alpha blending.

Assumes color is in destination format.

\param dst The surface to draw on.
\param x1 X coordinate of the first corner (upper left) of the rectangle.
\param y1 Y coordinate of the first corner (upper left) of the rectangle.
\param x2 X coordinate of the second corner (lower right) of the rectangle.
\param y2 Y coordinate of the second corner (lower right) of the rectangle.
\param color The color value of the rectangle to draw (0xRRGGBBAA).
*/
static void _HLineAlpha1(SDL_Surface * dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color)
{
    Uint8 cR = (color>>24) & 0xFF,
           cG = (color>>16) & 0xFF,
           cB = (color>>8) & 0xFF,
           cA = (color>>0) & 0xFF;

	Sint16 x;

	SDL_PixelFormat *format = dst->format;
    Uint32 Rmask = format->Rmask;
    Uint32 Gmask = format->Gmask;
    Uint32 Bmask = format->Bmask;
    Uint32 Amask = format->Amask;
    Uint32 Rshift = format->Rshift;
    Uint32 Gshift = format->Gshift;
    Uint32 Bshift = format->Bshift;
    Uint32 Ashift = format->Ashift;

	switch (format->BytesPerPixel) {
	case 1:
		{			/* Assuming 8-bpp */
			SDL_Palette *palette = format->palette;
			SDL_Color *colors = palette->colors;

            Uint8 *row = (Uint8 *) dst->pixels + y * dst->pitch;
            for (x = x1; x <= x2; x++) {
                Uint8 *pixel = row + x;

                Uint8 dR = colors[*pixel].r;
                Uint8 dG = colors[*pixel].g;
                Uint8 dB = colors[*pixel].b;

                dR = dR + ((cR - dR) * cA >> 8);
                dG = dG + ((cG - dG) * cA >> 8);
                dB = dB + ((cB - dB) * cA >> 8);

                *pixel = SDL_MapRGB(format, dR, dG, dB);
            }
		}
		break;

	case 2:
		{			/* Probably 15-bpp or 16-bpp */
			Uint16 dR = cR >> format->Rloss << format->Rshift;
			Uint16 dG = cG >> format->Gloss << format->Gshift;
			Uint16 dB = cB >> format->Bloss << format->Bshift;
			Uint16 dA = cA >> format->Aloss << format->Ashift;

            Uint16 *row = (Uint16 *) dst->pixels + y * dst->pitch / 2;
            for (x = x1; x <= x2; x++) {
                Uint16 *pixel = row + x;

                Uint16 R = ((*pixel & Rmask) + ((dR - (*pixel & Rmask)) * cA >> 8)) & Rmask;
                Uint16 G = ((*pixel & Gmask) + ((dG - (*pixel & Gmask)) * cA >> 8)) & Gmask;
                Uint16 B = ((*pixel & Bmask) + ((dB - (*pixel & Bmask)) * cA >> 8)) & Bmask;
                Uint16 A = ((*pixel & Amask) + ((dA - (*pixel & Amask)) * cA >> 8)) & Amask;
                
                *pixel = R | G | B | A;
            }
		}
		break;

	case 3:
		{			/* Slow 24-bpp mode, usually not used */
			Uint32 dR = cR >> format->Rloss << format->Rshift;
			Uint32 dG = cG >> format->Gloss << format->Gshift;
			Uint32 dB = cB >> format->Bloss << format->Bshift;
			Uint32 dA = cA >> format->Aloss << format->Ashift;

            for (x = x1; x <= x2; x++) {
                Uint32 *pixel = (Uint32 *) ((Uint8 *) dst->pixels + y * dst->pitch + x * 3);

                Uint32 R = ((*pixel & Rmask) + ((((dR - (*pixel & Rmask)) >> Rshift) * cA >> 8) << Rshift)) & Rmask;
                Uint32 G = ((*pixel & Gmask) + ((((dG - (*pixel & Gmask)) >> Gshift) * cA >> 8) << Gshift)) & Gmask;
                Uint32 B = ((*pixel & Bmask) + ((((dB - (*pixel & Bmask)) >> Bshift) * cA >> 8) << Bshift)) & Bmask;
                Uint32 A = ((*pixel & Amask) + ((((dA - (*pixel & Amask)) >> Ashift) * cA >> 8) << Ashift)) & Amask;

                *pixel = (*pixel & ~(Rmask | Gmask | Bmask | Amask)) | R | G | B | A;
            }
		}
		break;

	case 4:
		{			/* Probably :-) 32-bpp */
			Uint32 dR = cR >> format->Rloss << format->Rshift;
			Uint32 dG = cG >> format->Gloss << format->Gshift;
			Uint32 dB = cB >> format->Bloss << format->Bshift;
			Uint32 dA = cA >> format->Aloss << format->Ashift;

            Uint32 *row = (Uint32 *) dst->pixels + y * dst->pitch / 4;
            for (x = x1; x <= x2; x++) {
                Uint32 *pixel = row + x;

                Uint32 R = ((*pixel & Rmask) + ((((dR - (*pixel & Rmask)) >> Rshift) * cA >> 8) << Rshift)) & Rmask;
                Uint32 G = ((*pixel & Gmask) + ((((dG - (*pixel & Gmask)) >> Gshift) * cA >> 8) << Gshift)) & Gmask;
                Uint32 B = ((*pixel & Bmask) + ((((dB - (*pixel & Bmask)) >> Bshift) * cA >> 8) << Bshift)) & Bmask;
                Uint32 A = ((*pixel & Amask) + ((((dA - (*pixel & Amask)) >> Ashift) * cA >> 8) << Ashift)) & Amask;

                *pixel = R | G | B | A;
            }
		}
		break;
	}
}


/*!
\brief Draw horizontal line with blending.

\param dst The surface to draw on.
\param x1 X coordinate of the first point (i.e. left) of the line.
\param x2 X coordinate of the second point (i.e. right) of the line.
\param y Y coordinate of the points of the line.
\param color The color value of the line to draw (0xRRGGBBAA).
*/
static void hlineColor(SDL_Surface *dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color)
{
	/* Get clipping boundary and check visibility of hline */
	Sint16 left = dst->clip_rect.x;
	if (x2<left) {
		return;
	}
	Sint16 right = dst->clip_rect.x + dst->clip_rect.w - 1;
	if (x1>right) {
		return;
	}
	Sint16 top = dst->clip_rect.y;
	Sint16 bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
	if ((y<top) || (y>bottom)) {
		return;
	}

	/* Clip */
	if (x1 < left) {
		x1 = left;
	}
	if (x2 > right) {
		x2 = right;
	}

	/* Draw */
	_HLineAlpha1(dst, x1, x2, y, color);

    return;
}


int filledDiamondColor(SDL_Surface *dst, Sint16 xc, Sint16 yc, Sint16 r, Uint32 color) {
	if ((dst->clip_rect.w==0) || (dst->clip_rect.h==0)) {
		return 0;
	}

	if (r < 0) {
		return -1;
	}

	Sint16 x2 = xc + r;
	Sint16 left = dst->clip_rect.x;
	if (x2<left) {
		return 0;
	}
	Sint16 x1 = xc - r;
	Sint16 right = dst->clip_rect.x + dst->clip_rect.w - 1;
	if (x1>right) {
		return 0;
	}
	Sint16 y2 = yc + r;
	Sint16 top = dst->clip_rect.y;
	if (y2<top) {
		return 0;
	}
	Sint16 y1 = yc - r;
	Sint16 bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
	if (y1>bottom) {
		return 0;
	}

	/* Lock the surface */
	if (SDL_MUSTLOCK(dst)) {
		if (SDL_LockSurface(dst) < 0) {
            return -1;
		}
	}

	/* Draw */
    hlineColor(dst, xc-(r), xc+(r), yc, color);
    for (Sint16 v = 1; v <= r; ++v) {
        hlineColor(dst, xc-(r-v), xc+(r-v), yc-v, color);
        hlineColor(dst, xc-(r-v), xc+(r-v), yc+v, color);
    }

	/* Unlock the surface */
	if (SDL_MUSTLOCK(dst)) {
		SDL_UnlockSurface(dst);
	}

	return 0;
}


int filledCircleColor(SDL_Surface *dst, Sint16 xc, Sint16 yc, Sint16 r, Uint32 color) {
	if ((dst->clip_rect.w==0) || (dst->clip_rect.h==0)) {
		return 0;
	}

	if (r < 0) {
		return -1;
	}

	Sint16 x2 = xc + r;
	Sint16 left = dst->clip_rect.x;
	if (x2<left) {
		return 0;
	}
	Sint16 x1 = xc - r;
	Sint16 right = dst->clip_rect.x + dst->clip_rect.w - 1;
	if (x1>right) {
		return 0;
	}
	Sint16 y2 = yc + r;
	Sint16 top = dst->clip_rect.y;
	if (y2<top) {
		return 0;
	}
	Sint16 y1 = yc - r;
	Sint16 bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
	if (y1>bottom) {
		return 0;
	}

	/* Lock the surface */
	if (SDL_MUSTLOCK(dst)) {
		if (SDL_LockSurface(dst) < 0) {
            return -1;
		}
	}

	/* Draw */
    Sint16 x = 0, y = r;
    Sint16 kd = 1-r;
    while (x <= y) {
        hlineColor(dst, xc-y, xc+y, yc-x, color);
        if (x != 0)
            hlineColor(dst, xc-y, xc+y, yc+x, color);
        if (kd >= 0) {
            if (y != x) {
                hlineColor(dst, xc-x, xc+x, yc-y, color);
                hlineColor(dst, xc-x, xc+x, yc+y, color);
            }
            y = y-1;
            kd = kd + 2*(x-y) + 5;
        } else {
            kd = kd + 2*x + 3;
        }
        x = x+1;
    }

	/* Unlock the surface */
	if (SDL_MUSTLOCK(dst)) {
		SDL_UnlockSurface(dst);
	}

	return 0;
}
