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

#include "cave/object/caveobjectline.hpp"
#include "cave/elementproperties.hpp"

#include <glib/gi18n.h>
#include <cstdlib>

#include "fileops/bdcffhelper.hpp"
#include "cave/caverendered.hpp"
#include "misc/printf.hpp"

/// Create a line cave object.
/// @param _p1 Starting point of the line.
/// @param _p2 Ending point of the line.
/// @param _element Element to draw.
CaveLine::CaveLine(Coordinate _p1, Coordinate _p2, GdElementEnum _element)
:   CaveObject(GD_LINE),
    p1(_p1),
    p2(_p2),
    element(_element)
{
}

std::string CaveLine::get_bdcff() const
{
    return BdcffFormat("Line") << p1 << p2 << element;
}

CaveLine* CaveLine::clone_from_bdcff(const std::string &name, std::istream &is) const
{
    Coordinate p1, p2;
    GdElementEnum element;
    if (!(is >> p1 >> p2 >> element))
        return NULL;
    return new CaveLine(p1, p2, element);
}


/// Draws the line.
/// Uses Bresenham's algorithm, as descriped in Wikipedia.
void CaveLine::draw(CaveRendered &cave) const
{
    int x1=p1.x;
    int y1=p1.y;
    int x2=p2.x;
    int y2=p2.y;
    bool steep=abs(y2-y1)>abs(x2-x1);
    if (steep) {
        std::swap(x1, y1);  /* yes, change x with y */
        std::swap(x2, y2);
    }
    if (x1>x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    int dx=x2-x1;
    int dy=abs(y2-y1);
    int error=0;
    int ystep=(y1<y2)?1:-1;

    int y=y1;
    for (int x=x1; x<=x2; x++) {
        if (steep)
            cave.store_rc(y, x, element, this); /* and here we change them back, if needed */
        else
            cave.store_rc(x, y, element, this);
        error+=dy;
        if (error*2>=dx) {
            y+=ystep;
            error-=dx;
        }
    }
}

PropertyDescription const CaveLine::descriptor[] = {
    {"", GD_TAB, 0, N_("Line")},
    {"", GD_TYPE_BOOLEAN_LEVELS, 0, N_("Levels"), GetterBase::create_new(&CaveLine::seen_on), N_("Levels on which this object is visible.")},
    {"", GD_TYPE_COORDINATE, 0, N_("Start"), GetterBase::create_new(&CaveLine::p1), N_("This is the start point of the line."), 0, 127},
    {"", GD_TYPE_COORDINATE, 0, N_("End"), GetterBase::create_new(&CaveLine::p2), N_("This is the end point of the line."), 0, 127},
    {"", GD_TYPE_ELEMENT, 0, N_("Element"), GetterBase::create_new(&CaveLine::element), N_("The element to draw.")},
    {NULL},
};

PropertyDescription const* CaveLine::get_description_array() const
{
    return descriptor;
}

std::string CaveLine::get_coordinates_text() const
{
    return SPrintf("%d,%d-%d,%d") % p1.x % p1.y % p2.x % p2.y;
}

void CaveLine::create_drag(Coordinate current, Coordinate displacement)
{
    p2=current;
}

void CaveLine::move(Coordinate current, Coordinate displacement)
{
    if (current==p1)
        p1+=displacement;   /* move endpoint 1 */
    else
    if (current==p2)
        p2+=displacement;   /* or move endpoint 2 */
    else {
        p1+=displacement;   /* or move the whole thing */
        p2+=displacement;
    }
}

void CaveLine::move(Coordinate displacement)
{
    p1+=displacement;
    p2+=displacement;
}

std::string CaveLine::get_description_markup() const
{
    return SPrintf(_("Line from %d,%d to %d,%d of <b>%s</b>"))
        % p1.x % p1.y % p2.x % p2.y % gd_element_properties[element].lowercase_name;
}
