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
#ifndef _GD_CAVEOBJECTLINE
#define _GD_CAVEOBJECTLINE

#include "config.h"

#include "cave/object/caveobject.hpp"

/// A line of elements in a cave.
class CaveLine: public CaveObject {
private:
    Coordinate p1;      ///< Starting coordinate
    Coordinate p2;      ///< Ending coordinate
    GdElement element;  ///< Draw element.

public:
    CaveLine(Coordinate _p1, Coordinate _p2, GdElementEnum _element);
    CaveLine(): CaveObject(GD_LINE) {}
    virtual void draw(CaveRendered &cave) const;
    virtual CaveLine *clone() const { return new CaveLine(*this); }
    virtual std::string get_bdcff() const;
    virtual CaveLine* clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const* get_description_array() const;

    virtual void create_drag(Coordinate current, Coordinate displacement);
    virtual void move(Coordinate current, Coordinate displacement);
    virtual void move(Coordinate displacement);
    virtual std::string get_coordinates_text() const;
    virtual GdElementEnum get_characteristic_element() const { return element; }
    virtual std::string get_description_markup() const;
};

#endif

