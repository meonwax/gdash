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

#include "cave/object/caveobjectraster.hpp"
#include "cave/elementproperties.hpp"

#include <glib/gi18n.h>

#include "fileops/bdcffhelper.hpp"
#include "cave/caverendered.hpp"
#include "misc/printf.hpp"

std::string CaveRaster::get_bdcff() const
{
    Coordinate number;
    number.x=((p2.x-p1.x)/dist.x+1);
    number.y=((p2.y-p1.y)/dist.y+1);

    return BdcffFormat("Raster") << p1 << number << dist << element;
}

CaveRaster* CaveRaster::clone_from_bdcff(const std::string &name, std::istream &is) const
{
    Coordinate p1, n, d, p2;
    GdElementEnum element;

    if (!(is >> p1 >> n >> d >> element))
        return NULL;
    p2.x=p1.x+(n.x-1)*d.x;
    p2.y=p1.y+(n.y-1)*d.y;

    return new CaveRaster(p1, p2, d, element);
}

CaveRaster::CaveRaster(Coordinate _p1, Coordinate _p2, Coordinate _dist, GdElementEnum _element)
:   CaveRectangular(GD_RASTER, _p1, _p2),
    dist(_dist),
    element(_element)
{
}

void CaveRaster::draw(CaveRendered &cave) const
{
    /* reorder coordinates if not drawing from northwest to southeast */
    int x1=p1.x, y1=p1.y;
    int x2=p2.x, y2=p2.y;
    int dx=dist.x, dy=dist.y;

    if (y1>y2)
        std::swap(y1, y2);
    if (x1>x2)
        std::swap(x1, x2);
    if (dy<1) dy=1; /* make sure we do not have an infinite loop */
    if (dx<1) dx=1;

    for (int y=y1; y<=y2; y+=dy)
        for (int x=x1; x<=x2; x+=dx)
            cave.store_rc(x, y, element, this);
}

PropertyDescription const CaveRaster::descriptor[] = {
    {"", GD_TAB, 0, N_("Raster")},
    {"", GD_TYPE_BOOLEAN_LEVELS, 0, N_("Levels"), GetterBase::create_new(&CaveRaster::seen_on), N_("Levels on which this object is visible.")},
    {"", GD_TYPE_COORDINATE, 0, N_("Start corner"), GetterBase::create_new(&CaveRaster::p1), N_("Specifies one of the corners of the object."), 0, 127},
    {"", GD_TYPE_COORDINATE, 0, N_("End corner"), GetterBase::create_new(&CaveRaster::p2), N_("Specifies one of the corners of the object."), 0, 127},
    {"", GD_TYPE_COORDINATE, 0, N_("Distance"), GetterBase::create_new(&CaveRaster::dist), N_("The horizontal and vertical distance between elements."), 1, 40},
    {"", GD_TYPE_ELEMENT, 0, N_("Element"), GetterBase::create_new(&CaveRaster::element), N_("The element to draw.")},
    {NULL},
};

PropertyDescription const* CaveRaster::get_description_array() const
{
    return descriptor;
}

std::string CaveRaster::get_coordinates_text() const
{
    return SPrintf("%d,%d-%d,%d (%d,%d)") % p1.x % p1.y % p2.x % p2.y % dist.x % dist.y;
}

std::string CaveRaster::get_description_markup() const
{
    return SPrintf(_("Raster from %d,%d to %d,%d of <b>%s</b>, distance %+d,%+d"))
        % p1.x % p1.y % p2.x % p2.y % gd_element_properties[element].lowercase_name % dist.x % dist.y;
}
