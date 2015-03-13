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

#include "cave/object/caveobjectfill.hpp"

#include "misc/printf.hpp"


/// Create a new fill object.
/// @param _start The starting coordinates of the fill.
/// @param _fill_element The inside of the area will be filled with this element.
CaveFill::CaveFill(CaveObject::Type _type, Coordinate _start, GdElementEnum _fill_element)
    :   CaveObject(_type),
        start(_start),
        fill_element(_fill_element) {
}

std::string CaveFill::get_coordinates_text() const {
    return SPrintf("%d,%d") % start.x % start.y;
}

void CaveFill::create_drag(Coordinate current, Coordinate displacement) {
    start=current;
}

void CaveFill::move(Coordinate current, Coordinate displacement) {
    if (start==current)     /* can only drag by the starting point */
        start+=displacement;
}

void CaveFill::move(Coordinate displacement) {
    start+=displacement;
}
