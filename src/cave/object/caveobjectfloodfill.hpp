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
#ifndef _GD_CAVEOBJECTFLOODFILL
#define _GD_CAVEOBJECTFLOODFILL

#include "config.h"

#include "cave/object/caveobject.hpp"
#include "cave/object/caveobjectfill.hpp"

/// A cave objects which fills a connected area.
class CaveFloodFill : public CaveFill {
private:
    GdElement search_element;       ///< Replace this element when filling.
    void draw_proc(CaveRendered &cave, int x, int y) const;

public:
    CaveFloodFill(Coordinate _start, GdElementEnum _fill_element, GdElementEnum _search_element);
    CaveFloodFill(): CaveFill(GD_FLOODFILL_REPLACE) {}
    virtual void draw(CaveRendered &cave) const;
    virtual CaveFloodFill *clone() const {
        return new CaveFloodFill(*this);
    };
    virtual std::string get_bdcff() const;
    virtual CaveFloodFill *clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const *get_description_array() const;

    virtual std::string get_description_markup() const;
};


#endif

