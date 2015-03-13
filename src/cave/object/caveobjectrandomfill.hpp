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
#ifndef _GD_CAVEOBJECTRANDOMFILL
#define _GD_CAVEOBJECTRANDOMFILL

#include "config.h"

#include "cave/object/caveobject.hpp"
#include "cave/object/caveobjectrectangular.hpp"

/* RANDOM FILL OBJECT */
class CaveRandomFill : public CaveRectangular {
private:
    GdElement replace_only;
    GdBool c64_random;

    GdIntLevels seed;
    GdElement initial_fill;
    GdElement random_fill_1;                ///< Random fill element 1
    GdInt random_fill_probability_1;        ///< 0..255 "probability" of random fill element 1
    GdElement random_fill_2;                ///< Random fill element 2
    GdInt random_fill_probability_2;        ///< 0..255 "probability" of random fill element 2
    GdElement random_fill_3;                ///< Random fill element 3
    GdInt random_fill_probability_3;        ///< 0..255 "probability" of random fill element 3
    GdElement random_fill_4;                ///< Random fill element 4
    GdInt random_fill_probability_4;        ///< 0..255 "probability" of random fill element 4
public:
    CaveRandomFill(Coordinate _p1, Coordinate _p2);
    CaveRandomFill(): CaveRectangular(GD_RANDOM_FILL) {}
    virtual void draw(CaveRendered &cave) const;
    virtual CaveRandomFill *clone() const {
        return new CaveRandomFill(*this);
    };
    void set_random_fill(GdElementEnum initial, GdElementEnum e1, GdElementEnum e2, GdElementEnum e3, GdElementEnum e4);
    void set_random_prob(int i1, int i2, int i3, int i4);
    void set_seed(int s1, int s2, int s3, int s4, int s5);
    void set_c64_random(bool rand) {
        c64_random=rand;
    }
    void set_replace_only(GdElementEnum repl) {
        replace_only=repl;
    }
    virtual std::string get_bdcff() const;
    virtual CaveRandomFill *clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const *get_description_array() const;

    virtual GdElementEnum get_characteristic_element() const {
        return O_NONE;
    }
    virtual std::string get_description_markup() const;
};


#endif

