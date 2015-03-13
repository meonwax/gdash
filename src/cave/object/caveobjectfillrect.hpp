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
#ifndef _GD_CAVEOBJECTFILLRECT
#define _GD_CAVEOBJECTFILLRECT

#include "config.h"

#include "cave/object/caveobject.hpp"
#include "cave/object/caveobjectrectangular.hpp"

/// Filled rectangle cave object.
/// The rectangle with a border of 1 cells in width is filled with another element.
class CaveFillRect : public CaveRectangular {
    GdElement border_element;   ///< Border of the rectangle is this element.
    GdElement fill_element;     ///< Insides of the rectangle is this element.

public:
    CaveFillRect(Coordinate _p1, Coordinate _p2, GdElementEnum _element, GdElementEnum _fill_element);
    CaveFillRect(): CaveRectangular(GD_FILLED_RECTANGLE) {}
    virtual CaveFillRect *clone() const {
        return new CaveFillRect(*this);
    };
    virtual void draw(CaveRendered &cave) const;
    virtual std::string get_bdcff() const;
    virtual CaveFillRect *clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const *get_description_array() const;

    virtual GdElementEnum get_characteristic_element() const {
        return fill_element;
    }
    virtual std::string get_description_markup() const;
};


#endif

