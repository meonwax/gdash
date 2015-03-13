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

#include "cave/object/caveobjectpoint.hpp"
#include "cave/elementproperties.hpp"

#include <glib/gi18n.h>

#include "fileops/bdcffhelper.hpp"
#include "cave/caverendered.hpp"
#include "misc/printf.hpp"

/* POINT */
CavePoint::CavePoint(Coordinate _p, GdElementEnum _element)
    :   CaveObject(GD_POINT),
        p(_p),
        element(_element) {
}

std::string CavePoint::get_bdcff() const {
    return BdcffFormat("Point") << p << element;
}

CavePoint *CavePoint::clone_from_bdcff(const std::string &name, std::istream &is) const {
    Coordinate p;
    GdElementEnum element;

    if (!(is >> p >> element))
        return NULL;
    return new CavePoint(p, element);
}

void CavePoint::draw(CaveRendered &cave) const {
    cave.store_rc(p.x, p.y, element, this);
}

PropertyDescription const CavePoint::descriptor[] = {
    {"", GD_TAB, 0, N_("Point")},
    {"", GD_TYPE_BOOLEAN_LEVELS, 0, N_("Levels"), GetterBase::create_new(&CavePoint::seen_on), N_("Levels on which this object is visible.")},
    {"", GD_TYPE_COORDINATE, 0, N_("Coordinate"), GetterBase::create_new(&CavePoint::p), N_("Specifies the coordinate of the single element to draw."), 0, 127},
    {"", GD_TYPE_ELEMENT, 0, N_("Element"), GetterBase::create_new(&CavePoint::element), N_("The element to draw.")},
    {NULL},
};

PropertyDescription const *CavePoint::get_description_array() const {
    return descriptor;
}

std::string CavePoint::get_coordinates_text() const {
    return SPrintf("%d,%d") % p.x % p.y;
}

void CavePoint::create_drag(Coordinate current, Coordinate displacement) {
    p=current;
}

void CavePoint::move(Coordinate current, Coordinate displacement) {
    p+=displacement;
}

void CavePoint::move(Coordinate displacement) {
    p+=displacement;
}

std::string CavePoint::get_description_markup() const {
    return SPrintf(_("Point of <b>%ms</b> at %d,%d"))
           % gd_element_properties[element].lowercase_name % p.x % p.y;
}
