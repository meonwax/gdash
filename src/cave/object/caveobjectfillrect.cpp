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

#include "cave/object/caveobjectfillrect.hpp"
#include "cave/elementproperties.hpp"

#include <glib/gi18n.h>

#include "fileops/bdcffhelper.hpp"
#include "cave/caverendered.hpp"
#include "misc/printf.hpp"

std::string CaveFillRect::get_bdcff() const
{
    BdcffFormat f("FillRect");
    f << p1 << p2 << border_element;
    if (border_element!=fill_element)
        f << fill_element;

    return f;
}

CaveFillRect* CaveFillRect::clone_from_bdcff(const std::string &name, std::istream &is) const
{
    Coordinate p1, p2;
    std::string s;
    GdElementEnum element, element_fill;
    if (!(is >> p1 >> p2 >> element))
        return NULL;
    if (is >> s) {  /* optional paramter - yuck */
        std::istringstream is(s);
        is >> element_fill;
    }
    else
        element_fill=element;

    return new CaveFillRect(p1, p2, element, element_fill);
}

/// Create filled rectangle cave object.
CaveFillRect::CaveFillRect(Coordinate _p1, Coordinate _p2, GdElementEnum _element, GdElementEnum _fill_element)
:   CaveRectangular(GD_FILLED_RECTANGLE, _p1, _p2),
    border_element(_element),
    fill_element(_fill_element)
{
}

void CaveFillRect::draw(CaveRendered &cave) const
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

    for (int y=y1; y<=y2; y++)
        for (int x=x1; x<=x2; x++)
            cave.store_rc(x, y, (y==y1 || y==y2 || x==x1 || x==x2) ? border_element : fill_element, this);
}

PropertyDescription const CaveFillRect::descriptor[] = {
    {"", GD_TAB, 0, N_("Draw")},
    {"", GD_TYPE_BOOLEAN_LEVELS, 0, N_("Levels"), GetterBase::create_new(&CaveFillRect::seen_on), N_("Levels on which this object is visible.")},
    {"", GD_TYPE_COORDINATE, 0, N_("Start"), GetterBase::create_new(&CaveFillRect::p1), N_("Specifies one of the corners of the object."), 0, 127},
    {"", GD_TYPE_COORDINATE, 0, N_("End"), GetterBase::create_new(&CaveFillRect::p2), N_("Specifies one of the corners of the object."), 0, 127},
    {"", GD_TYPE_ELEMENT, 0, N_("Border element"), GetterBase::create_new(&CaveFillRect::border_element), N_("The outline will be drawn with this element.")},
    {"", GD_TYPE_ELEMENT, 0, N_("Fill element"), GetterBase::create_new(&CaveFillRect::fill_element), N_("The insides of the rectangle will be filled with this element.")},
    {NULL},
};

PropertyDescription const* CaveFillRect::get_description_array() const
{
    return descriptor;
}

std::string CaveFillRect::get_description_markup() const
{
    return SPrintf(_("Rectangle from %d,%d to %d,%d of <b>%s</b>, filled with <b>%s</b>"))
        % p1.x % p1.y % p2.x % p2.y % gd_element_properties[border_element].lowercase_name % gd_element_properties[fill_element].lowercase_name;
}
