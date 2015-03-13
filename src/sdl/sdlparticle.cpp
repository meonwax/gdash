/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
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


#include <SDL.h>
#include "sdl/sdlparticle.hpp"
#include "settings.hpp"

/**
 * \brief Draw a horizontal line with alpha blending.
 * \param dst The surface to draw on.
 * \param x1 X coordinate of the start point (upper left) of the line.
 * \param x2 X coordinate of the end point (upper left) of the line.
 * \param y Y coordinate of the line.
 * \param color The color value of the line to draw (0xRRGGBBAA).
*/
static void hlineColor(SDL_Surface *dst, Sint16 x1, Sint16 x2, Sint16 y, Uint32 color) {
    {
        /* Get clipping boundary and check visibility of hline */
        Sint16 left = dst->clip_rect.x;
        Sint16 right = dst->clip_rect.x + dst->clip_rect.w - 1;
        Sint16 top = dst->clip_rect.y;
        Sint16 bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
        if (x2<left || x1>right || y<top || y>bottom)
            return;
        /* Clip */
        if (x1 < left)
            x1 = left;
        if (x2 > right)
            x2 = right;
    }

    SDL_PixelFormat *format = dst->format;

    Uint8 cR = (color>>24) & 0xFF,
          cG = (color>>16) & 0xFF,
          cB = (color>>8) & 0xFF,
          cA = (color>>0) & 0xFF;
    if (gd_pal_emulation_game && y%2==1)
        cA = cA * gd_pal_emu_scanline_shade / 100;

    Uint32 Rmask = format->Rmask;
    Uint32 Gmask = format->Gmask;
    Uint32 Bmask = format->Bmask;
    Uint32 Rshift = format->Rshift;
    Uint32 Gshift = format->Gshift;
    Uint32 Bshift = format->Bshift;
    Uint32 Rloss = format->Rloss;
    Uint32 Gloss = format->Gloss;
    Uint32 Bloss = format->Bloss;

    switch (format->BytesPerPixel) {
        case 1: {
            /* Assuming 8-bpp */
            SDL_Palette *palette = format->palette;
            SDL_Color *colors = palette->colors;

            Uint8 *row = (Uint8 *) dst->pixels + y * dst->pitch;
            for (Sint32 x = x1; x <= x2; x++) {
                Uint8 *pixel = row + x;

                Sint32 R = colors[*pixel].r;
                Sint32 G = colors[*pixel].g;
                Sint32 B = colors[*pixel].b;

                R = R + ((cR - R) * cA >> 8);
                G = G + ((cG - G) * cA >> 8);
                B = B + ((cB - B) * cA >> 8);

                *pixel = SDL_MapRGB(format, R, G, B);
            }
        }
        break;

        case 2: {
            /* Probably 15-bpp or 16-bpp */
            Uint16 *row = (Uint16 *) dst->pixels + y * dst->pitch / 2;
            for (Sint32 x = x1; x <= x2; x++) {
                Uint16 *pixel = row + x;

                Sint32 R = (*pixel & Rmask) >> Rshift << Rloss;
                Sint32 G = (*pixel & Gmask) >> Gshift << Gloss;
                Sint32 B = (*pixel & Bmask) >> Bshift << Bloss;

                R = R + ((cR - R) * cA >> 8);
                G = G + ((cG - G) * cA >> 8);
                B = B + ((cB - B) * cA >> 8);

                *pixel = R>>Rloss<<Rshift | G>>Gloss<<Gshift | B>>Bloss<<Bshift;
            }
        }
        break;

        case 3: {
            /* Slow 24-bpp mode, usually not used */
            Uint32 bitoff = ~(Rmask | Gmask | Bmask);
            for (Sint32 x = x1; x <= x2; x++) {
                Uint32 *pixel = (Uint32 *)((Uint8 *) dst->pixels + y * dst->pitch + x * 3);

                Sint32 R = (*pixel & Rmask) >> Rshift;
                Sint32 G = (*pixel & Gmask) >> Gshift;
                Sint32 B = (*pixel & Bmask) >> Bshift;

                R = R + ((cR - R) * cA >> 8);
                G = G + ((cG - G) * cA >> 8);
                B = B + ((cB - B) * cA >> 8);

                *pixel = (*pixel & bitoff) | R<<Rshift | G<<Gshift | B<<Bshift;
            }
        }
        break;

        case 4: {
            /* Probably :-) 32-bpp */
            Uint32 *row = (Uint32 *) dst->pixels + y * dst->pitch / 4;
            for (Sint32 x = x1; x <= x2; x++) {
                Uint32 *pixel = row + x;

                Uint8 R = (*pixel & Rmask) >> Rshift;
                Uint8 G = (*pixel & Gmask) >> Gshift;
                Uint8 B = (*pixel & Bmask) >> Bshift;

                R = R + ((cR - R) * cA >> 8);
                G = G + ((cG - G) * cA >> 8);
                B = B + ((cB - B) * cA >> 8);

                *pixel = R<<Rshift | G<<Gshift | B<<Bshift;
            }
        }
        break;
    }
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
