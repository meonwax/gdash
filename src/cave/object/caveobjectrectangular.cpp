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

#include "cave/object/caveobjectrectangular.hpp"

#include "misc/printf.hpp"

CaveRectangular::CaveRectangular(CaveObject::Type type, Coordinate _p1, Coordinate _p2)
:   CaveObject(type),
    p1(_p1),
    p2(_p2)
{
}

void CaveRectangular::create_drag(Coordinate current, Coordinate displacement)
{
    p2=current;
}

void CaveRectangular::move(Coordinate current, Coordinate displacement)
{
    Coordinate::drag_rectangle(p1, p2, current, displacement);
}

void CaveRectangular::move(Coordinate displacement)
{
    p1+=displacement;
    p2+=displacement;
}

std::string CaveRectangular::get_coordinates_text() const
{
    return SPrintf("%d,%d-%d,%d") % p1.x % p1.y % p2.x % p2.y;
}
