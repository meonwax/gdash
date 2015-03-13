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

#include "config.h"

#include <algorithm>
#include "cave/helper/cavemap.hpp"

void CaveMapBase::perfect_wrap_coords(int w, int h, int &x, int &y) {
    y=(y+h)%h;
    x=(x+w)%w;
}

void CaveMapBase::lineshift_wrap_coords_only_x(int w, int &x, int &y) {
    /* fit x coordinate within range, with correcting y at the same time */
    while (x>=w) {
        y++;
        x-=w;
    }
    while (x<0) {   /* out of bounds on the left... */
        y--;    /* previous row */
        x+=w;
    }
    /* here do not change x to be >=0 and <= h-1 */
}

void CaveMapBase::lineshift_wrap_coords_both(int w, int h, int &x, int &y) {
    lineshift_wrap_coords_only_x(w, x, y);
    y=(y+h)%h;
}
