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

#include "cave/object/caveobjectcopypaste.hpp"

#include <glib/gi18n.h>

#include "fileops/bdcffhelper.hpp"
#include "cave/caverendered.hpp"
#include "misc/printf.hpp"
#include "misc/logger.hpp"
#include "misc/util.hpp"

std::string CaveCopyPaste::get_bdcff() const {
    return BdcffFormat("CopyPaste") << p1 << p2 << dest << (mirror?"mirror":"nomirror") << (flip?"flip":"noflip");
}

CaveCopyPaste *CaveCopyPaste::clone_from_bdcff(const std::string &name, std::istream &is) const {
    Coordinate p1, p2, dest;
    std::string mirror="nomirror", flip="noflip";
    bool bmirror, bflip;

    if (!(is >> p1 >> p2 >> dest))
        return NULL;
    is >> mirror >> flip;
    if (gd_str_ascii_caseequal(mirror, "mirror"))
        bmirror=true;
    else if (gd_str_ascii_caseequal(mirror, "nomirror"))
        bmirror=false;
    else {
        bmirror=false;
        gd_warning(CPrintf("invalid setting for copypaste mirror property: %s") % mirror);
    }
    if (gd_str_ascii_caseequal(flip, "flip"))
        bflip=true;
    else if (gd_str_ascii_caseequal(flip, "noflip"))
        bflip=false;
    else {
        bflip=false;
        gd_warning(CPrintf("invalid setting for copypaste mirror property: %s") % flip);
    }

    CaveCopyPaste *cp=new CaveCopyPaste(p1, p2, dest);
    cp->set_mirror_flip(bmirror, bflip);

    return cp;
}

/// Create a copy and paste object.
/// Sets mirror and flip to false.
/// @param _p1 Corner of source area
/// @param _p2 Other corner of source area
/// @param _dest Upper left corner of destination area.
CaveCopyPaste::CaveCopyPaste(Coordinate _p1, Coordinate _p2, Coordinate _dest)
    :   CaveObject(GD_COPY_PASTE),
        p1(_p1),
        p2(_p2),
        dest(_dest),
        mirror(false),
        flip(false) {
}

/// Set mirror and flip coordinates.
/// They are initialized to false by the default constructor.
void CaveCopyPaste::set_mirror_flip(bool _mirror, bool _flip) {
    mirror=_mirror;
    flip=_flip;
}

void CaveCopyPaste::draw(CaveRendered &cave) const {
    int x1=p1.x, y1=p1.y, x2=p2.x, y2=p2.y;
    int w, h;

    /* reorder coordinates if not drawing from northwest to southeast */
    if (x2<x1)
        std::swap(x1, x2);
    if (y2<y1)
        std::swap(y1, y2);
    w=x2-x1+1;
    h=y2-y1+1;
    CaveMap<GdElementEnum> clipboard;
    clipboard.set_size(w, h);

    /* copy to "clipboard" */
    for (int y=0; y<h; y++)
        for (int x=0; x<w; x++)
            clipboard(x, y)=cave.map(x+x1, y+y1);

    for (int y=0; y<h; y++) {
        int ydisp=flip?h-1-y:y;
        for (int x=0; x<w; x++) {
            int xdisp=mirror?w-1-x:x;
            /* dx and dy are used here are "paste to" coordinates */
            cave.store_rc(dest.x+xdisp, dest.y+ydisp, clipboard(x, y), this);
        }
    }
}

PropertyDescription const CaveCopyPaste::descriptor[] = {
    {"", GD_TAB, 0, N_("Draw")},
    {"", GD_TYPE_BOOLEAN_LEVELS, 0, N_("Levels"), GetterBase::create_new(&CaveCopyPaste::seen_on), N_("Levels on which this object is visible.")},
    {"", GD_TYPE_COORDINATE, 0, N_("Start"), GetterBase::create_new(&CaveCopyPaste::p1), N_("Specifies one of the corners of the source area."), 0, 127},
    {"", GD_TYPE_COORDINATE, 0, N_("End"), GetterBase::create_new(&CaveCopyPaste::p2), N_("Specifies one of the corners of the source area."), 0, 127},
    {"", GD_TYPE_COORDINATE, 0, N_("Paste"), GetterBase::create_new(&CaveCopyPaste::dest), N_("Specifies the upper left corner of the destination area."), 0, 127},
    {"", GD_TYPE_BOOLEAN, 0, N_("Mirror"), GetterBase::create_new(&CaveCopyPaste::mirror), N_("If checked, the contents will be mirrored horizontally.")},
    {"", GD_TYPE_BOOLEAN, 0, N_("Flip"), GetterBase::create_new(&CaveCopyPaste::flip), N_("If checked, the contents will be mirrored vertically.")},
    {NULL},
};

PropertyDescription const *CaveCopyPaste::get_description_array() const {
    return descriptor;
}

std::string CaveCopyPaste::get_coordinates_text() const {
    return SPrintf("%d,%d-%d,%d (%d,%d)") % p1.x % p1.y % p2.x % p2.y % dest.x % dest.y;
}

void CaveCopyPaste::create_drag(Coordinate current, Coordinate displacement) {
    /* p1 is set fixed. now set p2. displacement is ignored. */
    p2=current;
    /* set the destination to be the same area as the source; so the user can move it. */
    dest.x=std::min(p1.x, p2.x);
    dest.y=std::min(p1.y, p2.y);
}

void CaveCopyPaste::move(Coordinate current, Coordinate displacement) {
    /* source area is unchanged; move only destination. */
    dest+=displacement;
}

void CaveCopyPaste::move(Coordinate displacement) {
    /* source area is unchanged; move only destination. */
    dest+=displacement;
}

std::string CaveCopyPaste::get_description_markup() const {
    return SPrintf(_("Copy from %d,%d-%d,%d, paste to %d,%d"))
           % p1.x % p1.y % p2.x % p2.y % dest.x % dest.y;
}
