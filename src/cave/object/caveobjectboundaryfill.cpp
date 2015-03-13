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

#include <glib/gi18n.h>

#include "fileops/bdcffhelper.hpp"
#include "cave/caverendered.hpp"
#include "misc/printf.hpp"
#include "cave/elementproperties.hpp"

#include "cave/object/caveobjectboundaryfill.hpp"


std::string CaveBoundaryFill::get_bdcff() const {
    return BdcffFormat("BoundaryFill") << start << fill_element << border_element;
}

CaveBoundaryFill *CaveBoundaryFill::clone_from_bdcff(const std::string &name, std::istream &is) const {
    Coordinate start;
    GdElementEnum fill, boundary;
    if (!(is >> start >> fill >> boundary))
        return NULL;

    return new CaveBoundaryFill(start, fill, boundary);
}

/// Create a new boundary fill objects.
/// @param _start The starting coordinates of the fill.
/// @param _border_element This is the element which is drawn on the outline of the area.
/// @param _fill_element The inside of the area will be filled with this element.
CaveBoundaryFill::CaveBoundaryFill(Coordinate _start, GdElementEnum _fill_element, GdElementEnum _border_element)
    :   CaveFill(GD_FLOODFILL_BORDER, _start, _fill_element),
        border_element(_border_element) {
}

/// Recursive function to fill an area of the cave, until the boundary is reached.
/// This function fills the insides of the area recursively with
/// the border element (!) - this way it is easy for it to not write
/// to a cell twice.
/// @param cave The cave to do the drawing in.
/// @param x The x coordinate to draw at.
/// @param y The y coordinate to draw at.
void CaveBoundaryFill::draw_proc(CaveRendered &cave, int x, int y) const {
    /* fill with border so we do not come back */
    cave.store_rc(x, y, border_element, this);

    if (x>0 && cave.map(x-1, y)!=border_element) draw_proc(cave, x-1, y);
    if (y>0 && cave.map(x, y-1)!=border_element) draw_proc(cave, x, y-1);
    if (x<cave.w-1 && cave.map(x+1, y)!=border_element) draw_proc(cave, x+1, y);
    if (y<cave.h-1 && cave.map(x, y+1)!=border_element) draw_proc(cave, x, y+1);
}

/// Draw the object.
/// First it fills the insides of the area using draw_proc.
/// Then it finds its own drawing using the object_order() map of the cave;
/// the elements drawn by it are replaced with the fill element.
/// @param cave The cave to draw in.
/// @param level The level the cave is rendered on.
void CaveBoundaryFill::draw(CaveRendered &cave) const {
    /* check bounds */
    if (start.x<0 || start.y<0 || start.x>=cave.w || start.y>=cave.h)
        return;

    /* this procedure fills the area with the border element. */
    draw_proc(cave, start.x, start.y);

    /* after the fill, we change all filled cells to the fill_element. */
    /* we find those by looking at the object_order map, that was filled by draw_proc/store_rc */
    for (int y=0; y<cave.h; y++)
        for (int x=0; x<cave.w; x++)
            if (cave.objects_order(x, y)==this)
                cave.map(x, y)=fill_element;
}

PropertyDescription const CaveBoundaryFill::descriptor[] = {
    {"", GD_TAB, 0, N_("Boundary fill")},
    {"", GD_TYPE_BOOLEAN_LEVELS, 0, N_("Levels"), GetterBase::create_new(&CaveBoundaryFill::seen_on), N_("Levels on which this object is visible.")},
    {"", GD_TYPE_COORDINATE, 0, N_("Start"), GetterBase::create_new(&CaveBoundaryFill::start), N_("The coordinate to start the fill at."), 0, 127},
    {"", GD_TYPE_ELEMENT, 0, N_("Border element"), GetterBase::create_new(&CaveBoundaryFill::border_element), N_("This is the border of the fill. The closed area must be bounded by this element.")},
    {"", GD_TYPE_ELEMENT, 0, N_("Fill element"), GetterBase::create_new(&CaveBoundaryFill::fill_element), N_("This is the element the closed area will be filled with.")},
    {NULL},
};

PropertyDescription const *CaveBoundaryFill::get_description_array() const {
    return descriptor;
}

std::string CaveBoundaryFill::get_description_markup() const {
    return SPrintf(_("Boundary fill from %d,%d of <b>%ms</b>, border <b>%ms</b>"))
           % start.x % start.y % gd_element_properties[fill_element].lowercase_name % gd_element_properties[border_element].lowercase_name;
}
