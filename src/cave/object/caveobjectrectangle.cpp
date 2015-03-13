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

#include "cave/object/caveobjectrectangle.hpp"
#include "cave/elementproperties.hpp"

#include <glib/gi18n.h>

#include "fileops/bdcffhelper.hpp"
#include "cave/caverendered.hpp"
#include "misc/printf.hpp"

std::string CaveRectangle::get_bdcff() const
{
    return BdcffFormat("Rectangle") << p1 << p2 << element;
}

CaveRectangle* CaveRectangle::clone_from_bdcff(const std::string &name, std::istream &is) const
{
    Coordinate p1, p2;
    GdElementEnum element;
    if (!(is >> p1 >> p2 >> element))
        return NULL;
    return new CaveRectangle(p1, p2, element);
}


CaveRectangle::CaveRectangle(Coordinate _p1, Coordinate _p2, GdElementEnum _element)
:   CaveRectangular(GD_RECTANGLE, _p1, _p2),
    element(_element)
{
}

/* rectangle, frame only */
void CaveRectangle::draw(CaveRendered &cave) const
{
    /* reorder coordinates if not drawing from northwest to southeast */
    int x1=p1.x;
    int y1=p1.y;
    int x2=p2.x;
    int y2=p2.y;
    if (y1>y2)
        std::swap(y1, y2);
    if (x1>x2)
        std::swap(x1, x2);

    for (int x=x1; x<=x2; x++) {
        cave.store_rc(x, y1, element, this);
        cave.store_rc(x, y2, element, this);
    }
    for (int y=y1; y<=y2; y++) {
        cave.store_rc(x1, y, element, this);
        cave.store_rc(x2, y, element, this);
    }
}

PropertyDescription const CaveRectangle::descriptor[] = {
    {"", GD_TAB, 0, N_("Rectangle")},
    {"", GD_TYPE_BOOLEAN_LEVELS, 0, N_("Levels"), GetterBase::create_new(&CaveRectangle::seen_on), N_("Levels on which this object is visible.")},
    {"", GD_TYPE_COORDINATE, 0, N_("Start corner"), GetterBase::create_new(&CaveRectangle::p1), N_("Specifies one of the corners of the object."), 0, 127},
    {"", GD_TYPE_COORDINATE, 0, N_("End corner"), GetterBase::create_new(&CaveRectangle::p2), N_("Specifies one of the corners of the object."), 0, 127},
    {"", GD_TYPE_ELEMENT, 0, N_("Element"), GetterBase::create_new(&CaveRectangle::element), N_("The element to draw.")},
    {NULL},
};

PropertyDescription const* CaveRectangle::get_description_array() const
{
    return descriptor;
}

std::string CaveRectangle::get_description_markup() const
{
    return SPrintf(_("Rectangle from %d,%d to %d,%d of <b>%s</b>"))
        % p1.x % p1.y % p2.x % p2.y % gd_element_properties[element].lowercase_name;
}
