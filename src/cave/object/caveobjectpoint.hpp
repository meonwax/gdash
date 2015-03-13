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
#ifndef CAVEOBJECTPOINT_HPP_INCLUDED
#define CAVEOBJECTPOINT_HPP_INCLUDED

#include "config.h"

#include "cave/object/caveobject.hpp"

/* POINT */
class CavePoint: public CaveObject {
private:
    Coordinate p;
    GdElement element;

public:
    CavePoint(Coordinate _p, GdElementEnum _element);
    CavePoint(): CaveObject(GD_POINT) {}
    virtual void draw(CaveRendered &cave) const;
    virtual CavePoint *clone() const;
    virtual std::string get_bdcff() const;
    virtual CavePoint *clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const *get_description_array() const;

    virtual void create_drag(Coordinate current, Coordinate displacement);
    virtual void move(Coordinate current, Coordinate displacement);
    virtual void move(Coordinate displacement);
    virtual std::string get_coordinates_text() const;
    virtual std::string get_description_markup() const;
    virtual GdElementEnum get_characteristic_element() const;
};


#endif
