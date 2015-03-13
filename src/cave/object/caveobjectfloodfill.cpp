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

#include "cave/object/caveobjectfloodfill.hpp"

#include <glib/gi18n.h>

#include "fileops/bdcffhelper.hpp"
#include "cave/caverendered.hpp"
#include "misc/printf.hpp"
#include "cave/elementproperties.hpp"

/// Create a floodfill object.
/// @param _start Start coordinates of fill.
/// @param _search_element Overwrite this element.
/// @param _fill_element Draw this element.
CaveFloodFill::CaveFloodFill(Coordinate _start, GdElementEnum _fill_element, GdElementEnum _search_element)
:   CaveFill(GD_FLOODFILL_REPLACE, _start, _fill_element),
    search_element(_search_element)
{
}

std::string CaveFloodFill::get_bdcff() const
{
    return BdcffFormat("FloodFill") << start << fill_element << search_element;
}

CaveFloodFill* CaveFloodFill::clone_from_bdcff(const std::string &name, std::istream &is) const
{
    Coordinate start;
    GdElementEnum fill, search;
    if (!(is >> start >> fill >> search))
        return NULL;

    return new CaveFloodFill(start, fill, search);
}

/// Standard recursive floodfill algorithm.
void CaveFloodFill::draw_proc(CaveRendered &cave, int x, int y) const
{
    cave.store_rc(x, y, fill_element, this);

    if (x>0 && cave.map(x-1, y)==search_element) draw_proc(cave, x-1, y);
    if (y>0 && cave.map(x, y-1)==search_element) draw_proc(cave, x, y-1);
    if (x<cave.w-1 && cave.map(x+1, y)==search_element) draw_proc(cave, x+1, y);
    if (y<cave.h-1 && cave.map(x, y+1)==search_element) draw_proc(cave, x, y+1);
}

void CaveFloodFill::draw(CaveRendered &cave) const
{
    /* check bounds */
    if (start.x<0 || start.y<0 || start.x>=cave.w || start.y>=cave.h)
        return;
    if (search_element==fill_element)
        return;
    /* this procedure fills the area with the object->element. */
    draw_proc(cave, start.x, start.y);
}

PropertyDescription const CaveFloodFill::descriptor[] = {
    {"", GD_TAB, 0, N_("Flood fill")},
    {"", GD_TYPE_BOOLEAN_LEVELS, 0, N_("Levels"), GetterBase::create_new(&CaveFloodFill::seen_on), N_("Levels on which this object is visible.")},
    {"", GD_TYPE_COORDINATE, 0, N_("Start"), GetterBase::create_new(&CaveFloodFill::start), N_("The coordinate to start the filling at."), 0, 127},
    {"", GD_TYPE_ELEMENT, 0, N_("Search element"), GetterBase::create_new(&CaveFloodFill::search_element), N_("The element which specifies the area to be filled, and will be overwritten.")},
    {"", GD_TYPE_ELEMENT, 0, N_("Fill element"), GetterBase::create_new(&CaveFloodFill::fill_element), N_("The element used to fill the area.")},
    {NULL},
};

PropertyDescription const* CaveFloodFill::get_description_array() const
{
    return descriptor;
}

std::string CaveFloodFill::get_description_markup() const
{
    return SPrintf(_("Flood fill from %d,%d of <b>%s</b>, replacing <b>%s</b>"))
        % start.x % start.y % gd_element_properties[fill_element].lowercase_name % gd_element_properties[search_element].lowercase_name;
}
