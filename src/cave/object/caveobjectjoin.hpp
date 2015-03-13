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
#ifndef _GD_CAVEOBJECTJOIN
#define _GD_CAVEOBJECTJOIN

#include "config.h"

#include "cave/object/caveobject.hpp"

/// Cave join object.
/// This one checks the entire cave, looking for search_element; if it finds one,
/// it draws put_element in a distance determined by the vector dist.
/// Whether the algorithm checks the cave from top to bottom or from bottom to
/// top, is determined by the backwards variable.
/// When the dist vector is pointing to down (or right with y=0), the search
/// for elements should go backwards; otherwise an element already drawn
/// will be checked again (or maybe a search_element is overwritten).
/// However this choice cannot be automatized, as compatibility with C64 games
/// must be retained, where all joins worked in a top to bottom fashion.
class CaveJoin : public CaveObject {
private:
    Coordinate dist;            ///< Distance to draw the new element at.
    GdElement search_element;   ///< Search this element.
    GdElement put_element;      ///< Draw this element as a pair for every search_element found.
    GdBool backwards;           ///< If true, search goes from bottom to top.
public:
    CaveJoin(Coordinate _dist, GdElementEnum _search_element, GdElementEnum _put_element, bool _backward=false);
    CaveJoin(): CaveObject(GD_JOIN) {}
    virtual void draw(CaveRendered &cave) const;
    virtual CaveJoin *clone() const {
        return new CaveJoin(*this);
    };
    virtual std::string get_bdcff() const;
    virtual CaveJoin *clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const *get_description_array() const;

    virtual void create_drag(Coordinate current, Coordinate displacement);
    virtual void move(Coordinate current, Coordinate displacement);
    virtual void move(Coordinate displacement);
    virtual std::string get_coordinates_text() const;
    virtual GdElementEnum get_characteristic_element() const {
        return put_element;
    }
    virtual std::string get_description_markup() const;
};


#endif

