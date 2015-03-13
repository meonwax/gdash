/*
 * Copyright (c) 2007, 2008, 2009, Czirkos Zoltan <cirix@fw.hu>
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

#include <glib.h>
#include <glib/gi18n.h>
#include "editorbits.h"
#include "cavedb.h"

/* description of coordinates, elements - used by editor properties dialog. */
GdObjectDescription gd_object_description[]={
    { /* none */  NULL },
    { /* point */ N_("Point"), N_("Coordinates"), NULL, NULL, NULL, N_("Element"), NULL, N_("Draw"), NULL },
    { /* line */  N_("Line"), N_("Starting coordinates"), N_("Ending coordinates"), NULL, NULL, N_("Element"), NULL, N_("Draw"), NULL },
    { /* rect */  N_("Outline"), N_("Starting coordinates"), N_("Ending coordinates"), NULL, NULL, N_("Element"), NULL, N_("Draw"), NULL },
    { /* frect */ N_("Rectangle"), N_("Starting coordinates"), N_("Ending coordinates"), NULL, NULL, N_("Border element"), N_("Fill element"), N_("Draw"), N_("Fill") }, 
    { /* rastr */ N_("Raster"), N_("Starting coordinates"), N_("Ending coordinates"), N_("Distance"), NULL, N_("Element"), NULL, N_("Draw"), NULL },
    { /* join */  N_("Join"), NULL, NULL, N_("Distance"), NULL, N_("Search element"), N_("Add element"), N_("Find"), N_("Draw") },
    { /* flodr */ N_("Replace fill"), N_("Starting coordinates"), NULL, NULL, NULL, N_("Search element"), N_("Fill element"), NULL, N_("Replace") },
    { /* flodb */ N_("Fill to border"), N_("Starting coordinates"), NULL, NULL, NULL, N_("Border element"), N_("Fill element"), N_("Border"), N_("Fill")  },
    { /* maze */  N_("Maze"), N_("Starting coordinates"), N_("Ending coordinates"), N_("Wall and path"), N_("Random seed %d"), N_("Wall element"), N_("Path element"), N_("Wall"), N_("Path") , N_("Horizontal (%%)")},
    { /* umaze */ N_("Unicursal maze"), N_("Starting coordinates"), N_("Ending coordinates"), N_("Wall and path"), N_("Random seed %d"), N_("Wall element"), N_("Path element"), N_("Wall"), N_("Path") , N_("Horizontal (%%)")},
    { /* Bmaze */ N_("Braid maze"), N_("Starting coordinates"), N_("Ending coordinates"), N_("Wall and path"), N_("Random seed %d"), N_("Wall element"), N_("Path element"), N_("Wall"), N_("Path") , N_("Horizontal (%%)")},
    { /* random */N_("Random fill"), N_("Starting coordinates"), N_("Ending coordinates"), NULL, N_("Random seed %d"), N_("Replace only this element"), NULL, N_("Random 1"), N_("Initial"), NULL, N_("C64 random numbers")},
    { /* copyps */N_("Copy and paste"), N_("Starting coordinates"), N_("Width and height"), N_("Paste coordinates"), NULL, NULL, NULL, NULL, NULL, NULL, NULL, N_("Mirror"), N_("Flip")},
};


char *
gd_object_get_coordinates_text (GdObject *selected)
{
    g_return_val_if_fail (selected!=NULL, NULL);

    switch (selected->type) {
    case GD_POINT:
    case GD_FLOODFILL_BORDER:
    case GD_FLOODFILL_REPLACE:
        return g_strdup_printf("%d,%d", selected->x1, selected->y1);

    case GD_LINE:
    case GD_RECTANGLE:
    case GD_FILLED_RECTANGLE:
    case GD_RANDOM_FILL:
        return g_strdup_printf("%d,%d-%d,%d", selected->x1, selected->y1, selected->x2, selected->y2);

    case GD_RASTER:
    case GD_MAZE_UNICURSAL:
    case GD_MAZE_BRAID:
    case GD_MAZE:
        return g_strdup_printf("%d,%d-%d,%d (%d,%d)", selected->x1, selected->y1, selected->x2, selected->y2, selected->dx, selected->dy);

    case GD_JOIN:
        return g_strdup_printf("%+d,%+d", selected->dx, selected->dy);

    case GD_COPY_PASTE:
        return g_strdup_printf("%d,%d-%d,%d, %d,%d", selected->x1, selected->y1, selected->x2, selected->y2, selected->dx, selected->dy);

    case NONE:
        g_assert_not_reached();
    }

    return NULL;
}



char *
gd_object_get_description_markup (GdObject *selected)
{
    g_return_val_if_fail (selected != NULL, NULL);

    switch (selected->type) {
    case GD_POINT:
        return g_markup_printf_escaped(_("Point of <b>%s</b> at %d,%d"), gd_elements[selected->element].lowercase_name, selected->x1, selected->y1);

    case GD_LINE:
        return g_markup_printf_escaped(_("Line from %d,%d to %d,%d of <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name);

    case GD_RECTANGLE:
        return g_markup_printf_escaped(_("Rectangle from %d,%d to %d,%d of <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name);

    case GD_FILLED_RECTANGLE:
        return g_markup_printf_escaped(_("Rectangle from %d,%d to %d,%d of <b>%s</b>, filled with <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, gd_elements[selected->fill_element].lowercase_name);

    case GD_RASTER:
        return g_markup_printf_escaped(_("Raster from %d,%d to %d,%d of <b>%s</b>, distance %+d,%+d"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, selected->dx, selected->dy);

    case GD_JOIN:
        return g_markup_printf_escaped(_("Join <b>%s</b> to every <b>%s</b>, distance %+d,%+d"), gd_elements[selected->fill_element].lowercase_name, gd_elements[selected->element].lowercase_name, selected->dx, selected->dy);

    case GD_FLOODFILL_BORDER:
        return g_markup_printf_escaped(_("Floodfill from %d,%d of <b>%s</b>, border <b>%s</b>"), selected->x1, selected->y1, gd_elements[selected->fill_element].lowercase_name, gd_elements[selected->element].lowercase_name);

    case GD_FLOODFILL_REPLACE:
        return g_markup_printf_escaped(_("Floodfill from %d,%d of <b>%s</b>, replacing <b>%s</b>"), selected->x1, selected->y1, gd_elements[selected->fill_element].lowercase_name, gd_elements[selected->element].lowercase_name);

    case GD_MAZE:
        return g_markup_printf_escaped(_("Maze from %d,%d to %d,%d, wall <b>%s</b>, path <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, gd_elements[selected->fill_element].lowercase_name);

    case GD_MAZE_UNICURSAL:
        return g_markup_printf_escaped(_("Unicursal maze from %d,%d to %d,%d, wall <b>%s</b>, path <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, gd_elements[selected->fill_element].lowercase_name);

    case GD_MAZE_BRAID:
        return g_markup_printf_escaped(_("Braid maze from %d,%d to %d,%d, wall <b>%s</b>, path <b>%s</b>"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name, gd_elements[selected->fill_element].lowercase_name);

    case GD_RANDOM_FILL:
        return g_markup_printf_escaped(_("Random fill from %d,%d to %d,%d, replacing %s"), selected->x1, selected->y1, selected->x2, selected->y2, gd_elements[selected->element].lowercase_name);

    case GD_COPY_PASTE:
        return g_markup_printf_escaped(_("Copy from %d,%d-%d,%d, paste to %d,%d"), selected->x1, selected->y1, selected->x2, selected->y2, selected->dx, selected->dy);

        
    case NONE:
        g_assert_not_reached();
    }

    return NULL;
}



