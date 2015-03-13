/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef CAVEOBJECTFLOODFILL_HPP_INCLUDED
#define CAVEOBJECTFLOODFILL_HPP_INCLUDED

#include "config.h"

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
    virtual CaveFloodFill *clone() const;
    virtual std::string get_bdcff() const;
    virtual CaveFloodFill *clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const *get_description_array() const;
    virtual std::string get_description_markup() const;
};


#endif

