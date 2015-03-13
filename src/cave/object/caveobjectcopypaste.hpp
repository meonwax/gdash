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
#ifndef _GD_CAVEOBJECTCOPYPASTE
#define _GD_CAVEOBJECTCOPYPASTE

#include "config.h"

#include "cave/object/caveobject.hpp"

/// Copy and paste cave object.
/// Copies the rectangle of p1,p2 to the destination.
/// This is not really a CaveRectangular, so it does not derive from that. Position p1,p2 here are
/// NOT the position of the object, but the source area.
class CaveCopyPaste : public CaveObject {
private:
    Coordinate p1, p2;      ///< Source area of copy. Order of p1 and p2 (lower left, upper right etc) is not important.
    Coordinate dest;        ///< Upper left coordinates of destination.
    GdBool mirror;          ///< Mirror the copy horizontally.
    GdBool flip;            ///< Mirror the copy vertically.

public:
    CaveCopyPaste(Coordinate _p1, Coordinate _p2, Coordinate _dest);
    CaveCopyPaste(): CaveObject(GD_COPY_PASTE) {}
    virtual void draw(CaveRendered &cave) const;
    virtual CaveCopyPaste *clone() const { return new CaveCopyPaste(*this); };
    void set_mirror_flip(bool _mirror, bool _flip);
    virtual std::string get_bdcff() const;
    virtual CaveCopyPaste* clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const* get_description_array() const;

    virtual void create_drag(Coordinate current, Coordinate displacement);
    virtual void move(Coordinate current, Coordinate displacement);
    virtual void move(Coordinate displacement);
    virtual std::string get_coordinates_text() const;
    virtual GdElementEnum get_characteristic_element() const { return O_NONE; }
    virtual std::string get_description_markup() const;
};


#endif

