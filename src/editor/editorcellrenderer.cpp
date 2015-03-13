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

#include <cstdlib>

#include "cave/caverendered.hpp"
#include "cave/elementproperties.hpp"
#include "gtk/gtkpixmap.hpp"
#include "gtk/gtkpixbuffactory.hpp"
#include "editor/editorcellrenderer.hpp"

EditorCellRenderer::EditorCellRenderer(GTKPixbufFactory &pixbuf_factory_, const std::string &theme_file)
    :
    CellRenderer(pixbuf_factory_, theme_file),
    combo_pixbufs() {
    draw_editor_pixbufs();
}

void EditorCellRenderer::remove_cached() {
    for (unsigned i=0; i<G_N_ELEMENTS(combo_pixbufs); ++i)
        if (combo_pixbufs[i]) {
            g_object_unref(combo_pixbufs[i]);
            combo_pixbufs[i]=NULL;
        }
    CellRenderer::remove_cached();
}

/*
    draw an element - usually an arrow or something like that
    over another one.

    the destination element's editor drawing will be used.
    the source element will be a game element.
*/
void EditorCellRenderer::add_arrow_to_cell(GdElementEnum dest, GdElementEnum src, GdElementEnum arrow, PixbufFactory::Rotation r) {
    // if already drawn, return
    if (cells_pixbufs[gd_element_properties[dest].image]!=NULL)
        return;

    /* editor image <- game image */
    copy_cell(gd_element_properties[dest].image, abs(gd_element_properties[src].image_game));
    Pixbuf *arrow_pb = pixbuf_factory.create_rotated(cell_pixbuf(gd_element_properties[arrow].image), r);    /* arrow */
    arrow_pb->blit(*cells_pixbufs[gd_element_properties[dest].image], 0, 0);
    delete arrow_pb;
}

void EditorCellRenderer::copy_cell(int dest, int src) {
    g_assert(src<NUM_OF_CELLS);
    g_assert(dest<NUM_OF_CELLS);

    // if already created
    if (cells_pixbufs[dest]!=NULL)
        return;

    int pcs=cell_pixbuf(src).get_width();
    Pixbuf *d=pixbuf_factory.create(pcs, pcs);
    cells_pixbufs[dest]=d;
    cell_pixbuf(src).copy(0, 0, pcs, pcs, *d, 0, 0);
}

/*
    composite two elements.
*/
void EditorCellRenderer::create_composite_cell_pixbuf(GdElementEnum dest, GdElementEnum src1, GdElementEnum src2) {
    int dimg=gd_element_properties[dest].image;

    g_assert(dimg<NUM_OF_CELLS);
    if (cells_pixbufs[dimg]!=NULL)      // if already drawn
        return;

    // destination image=source1
    copy_cell(dimg, abs(gd_element_properties[src1].image_game));
    // composite source2 to destination
    // using gtk directly (this is not implemented in gdash pixbuf)
    GdkPixbuf *srcpb=static_cast<GTKPixbuf &>(cell_pixbuf(abs(gd_element_properties[src2].image_game))).get_gdk_pixbuf();
    GdkPixbuf *dstpb=static_cast<GTKPixbuf *>(cells_pixbufs[dimg])->get_gdk_pixbuf();
    gdk_pixbuf_composite(srcpb, dstpb, 0, 0, gdk_pixbuf_get_width(srcpb), gdk_pixbuf_get_height(srcpb), 0, 0, 1, 1, GDK_INTERP_NEAREST, 85);
}

/* load cells, eg. create cells_pb and combo_pb
   from a big pixbuf.
*/
void EditorCellRenderer::draw_editor_pixbufs() {
    /* draw some elements, combining them with arrows and the like */
    add_arrow_to_cell(O_STEEL_EATABLE, O_STEEL, O_EATABLE);
    add_arrow_to_cell(O_BRICK_EATABLE, O_BRICK, O_EATABLE);
    create_composite_cell_pixbuf(O_BRICK_NON_SLOPED, O_STEEL, O_BRICK);

    create_composite_cell_pixbuf(O_WALLED_KEY_1, O_KEY_1, O_BRICK);
    create_composite_cell_pixbuf(O_WALLED_KEY_2, O_KEY_2, O_BRICK);
    create_composite_cell_pixbuf(O_WALLED_KEY_3, O_KEY_3, O_BRICK);
    create_composite_cell_pixbuf(O_WALLED_DIAMOND, O_DIAMOND, O_BRICK);

    add_arrow_to_cell(O_FIREFLY_1, O_FIREFLY_1, O_DOWN_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_FIREFLY_2, O_FIREFLY_1, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_FIREFLY_3, O_FIREFLY_1, O_DOWN_ARROW, PixbufFactory::CounterClockWise);
    add_arrow_to_cell(O_FIREFLY_4, O_FIREFLY_1, O_DOWN_ARROW, PixbufFactory::None);

    add_arrow_to_cell(O_ALT_FIREFLY_1, O_ALT_FIREFLY_1, O_DOWN_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_ALT_FIREFLY_2, O_ALT_FIREFLY_1, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_ALT_FIREFLY_3, O_ALT_FIREFLY_1, O_DOWN_ARROW, PixbufFactory::CounterClockWise);
    add_arrow_to_cell(O_ALT_FIREFLY_4, O_ALT_FIREFLY_1, O_DOWN_ARROW, PixbufFactory::None);

    add_arrow_to_cell(O_H_EXPANDING_WALL, O_H_EXPANDING_WALL, O_LEFTRIGHT_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_V_EXPANDING_WALL, O_V_EXPANDING_WALL, O_LEFTRIGHT_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_EXPANDING_WALL, O_EXPANDING_WALL, O_EVERYDIR_ARROW);

    add_arrow_to_cell(O_H_EXPANDING_STEEL_WALL, O_H_EXPANDING_STEEL_WALL, O_LEFTRIGHT_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_V_EXPANDING_STEEL_WALL, O_V_EXPANDING_STEEL_WALL, O_LEFTRIGHT_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_EXPANDING_STEEL_WALL, O_EXPANDING_STEEL_WALL, O_EVERYDIR_ARROW);

    add_arrow_to_cell(O_BUTTER_1, O_BUTTER_1, O_DOWN_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_BUTTER_2, O_BUTTER_1, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_BUTTER_3, O_BUTTER_1, O_DOWN_ARROW, PixbufFactory::CounterClockWise);
    add_arrow_to_cell(O_BUTTER_4, O_BUTTER_1, O_DOWN_ARROW, PixbufFactory::None);

    add_arrow_to_cell(O_DRAGONFLY_1, O_DRAGONFLY_1, O_DOWN_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_DRAGONFLY_2, O_DRAGONFLY_1, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_DRAGONFLY_3, O_DRAGONFLY_1, O_DOWN_ARROW, PixbufFactory::CounterClockWise);
    add_arrow_to_cell(O_DRAGONFLY_4, O_DRAGONFLY_1, O_DOWN_ARROW, PixbufFactory::None);

    add_arrow_to_cell(O_COW_1, O_COW_1, O_DOWN_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_COW_2, O_COW_1, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_COW_3, O_COW_1, O_DOWN_ARROW, PixbufFactory::CounterClockWise);
    add_arrow_to_cell(O_COW_4, O_COW_1, O_DOWN_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_COW_ENCLOSED_1, O_COW_1, O_GLUED);

    add_arrow_to_cell(O_ALT_BUTTER_1, O_ALT_BUTTER_1, O_DOWN_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_ALT_BUTTER_2, O_ALT_BUTTER_1, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_ALT_BUTTER_3, O_ALT_BUTTER_1, O_DOWN_ARROW, PixbufFactory::CounterClockWise);
    add_arrow_to_cell(O_ALT_BUTTER_4, O_ALT_BUTTER_1, O_DOWN_ARROW, PixbufFactory::None);

    add_arrow_to_cell(O_PLAYER_GLUED, O_PLAYER, O_GLUED);
    add_arrow_to_cell(O_PLAYER, O_PLAYER, O_EXCLAMATION_MARK);
    add_arrow_to_cell(O_STONE_GLUED, O_STONE, O_GLUED);
    add_arrow_to_cell(O_DIAMOND_GLUED, O_DIAMOND, O_GLUED);
    add_arrow_to_cell(O_DIRT_GLUED, O_DIRT, O_GLUED);
    add_arrow_to_cell(O_STONE_F, O_STONE, O_DOWN_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_NUT_F, O_NUT, O_DOWN_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_FLYING_STONE_F, O_FLYING_STONE, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_MEGA_STONE_F, O_MEGA_STONE, O_DOWN_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_DIAMOND_F, O_DIAMOND, O_DOWN_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_FLYING_DIAMOND_F, O_FLYING_DIAMOND, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_FALLING_WALL, O_BRICK, O_EXCLAMATION_MARK);
    add_arrow_to_cell(O_FALLING_WALL_F, O_BRICK, O_DOWN_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_TIME_PENALTY, O_GRAVESTONE, O_EXCLAMATION_MARK);
    add_arrow_to_cell(O_NITRO_PACK_F, O_NITRO_PACK, O_DOWN_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_NITRO_PACK_EXPLODE, O_NITRO_PACK, O_EXCLAMATION_MARK);
    add_arrow_to_cell(O_CONVEYOR_LEFT, O_CONVEYOR_LEFT, O_DOWN_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_CONVEYOR_RIGHT, O_CONVEYOR_RIGHT, O_DOWN_ARROW, PixbufFactory::CounterClockWise);

    add_arrow_to_cell(O_STONEFLY_1, O_STONEFLY_1, O_DOWN_ARROW, PixbufFactory::ClockWise);
    add_arrow_to_cell(O_STONEFLY_2, O_STONEFLY_1, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_STONEFLY_3, O_STONEFLY_1, O_DOWN_ARROW, PixbufFactory::CounterClockWise);
    add_arrow_to_cell(O_STONEFLY_4, O_STONEFLY_1, O_DOWN_ARROW, PixbufFactory::None);

    add_arrow_to_cell(O_BITER_1, O_BITER_1, O_DOWN_ARROW, PixbufFactory::UpsideDown);
    add_arrow_to_cell(O_BITER_2, O_BITER_1, O_DOWN_ARROW, PixbufFactory::CounterClockWise);
    add_arrow_to_cell(O_BITER_3, O_BITER_1, O_DOWN_ARROW, PixbufFactory::None);
    add_arrow_to_cell(O_BITER_4, O_BITER_1, O_DOWN_ARROW, PixbufFactory::ClockWise);

    add_arrow_to_cell(O_PRE_INVIS_OUTBOX, O_OUTBOX_CLOSED, O_GLUED);
    add_arrow_to_cell(O_PRE_OUTBOX, O_OUTBOX_OPEN, O_GLUED);
    add_arrow_to_cell(O_INVIS_OUTBOX, O_OUTBOX_CLOSED, O_OUT);
    add_arrow_to_cell(O_OUTBOX, O_OUTBOX_OPEN, O_OUT);

    add_arrow_to_cell(O_UNKNOWN, O_STEEL, O_QUESTION_MARK);
    add_arrow_to_cell(O_WAITING_STONE, O_STONE, O_EXCLAMATION_MARK);

    /* blinking outbox: helps editor, drawing the cave is more simple */
    copy_cell(abs(gd_element_properties[O_PRE_OUTBOX].image_simple)+0, gd_element_properties[O_OUTBOX_OPEN].image_game);
    copy_cell(abs(gd_element_properties[O_PRE_OUTBOX].image_simple)+1, gd_element_properties[O_OUTBOX_OPEN].image_game);
    copy_cell(abs(gd_element_properties[O_PRE_OUTBOX].image_simple)+2, gd_element_properties[O_OUTBOX_OPEN].image_game);
    copy_cell(abs(gd_element_properties[O_PRE_OUTBOX].image_simple)+3, gd_element_properties[O_OUTBOX_OPEN].image_game);
    copy_cell(abs(gd_element_properties[O_PRE_OUTBOX].image_simple)+4, gd_element_properties[O_OUTBOX_CLOSED].image_game);
    copy_cell(abs(gd_element_properties[O_PRE_OUTBOX].image_simple)+5, gd_element_properties[O_OUTBOX_CLOSED].image_game);
    copy_cell(abs(gd_element_properties[O_PRE_OUTBOX].image_simple)+6, gd_element_properties[O_OUTBOX_CLOSED].image_game);
    copy_cell(abs(gd_element_properties[O_PRE_OUTBOX].image_simple)+7, gd_element_properties[O_OUTBOX_CLOSED].image_game);
}

void EditorCellRenderer::select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5) {
    CellRenderer::select_pixbuf_colors(c0, c1, c2, c3, c4, c5);
    draw_editor_pixbufs();
}


GdkPixbuf *EditorCellRenderer::get_element_pixbuf_with_border(int index) {
    if (!combo_pixbufs[index]) {
        /* create small size pixbuf if needed. */
        /* scale pixbuf to that specified by gtk */
        int x, y;
        gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &x, &y);
        GdkPixbuf *pixbuf=gdk_pixbuf_scale_simple(static_cast<GTKPixbuf &>(cell_pixbuf(index)).get_gdk_pixbuf(), x, y, GDK_INTERP_BILINEAR);
        /* draw a little black border around image, makes the icons look much better */
        GdkPixbuf *pixbuf_border=gdk_pixbuf_new(GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha(pixbuf), 8, x+2, y+2);
        gdk_pixbuf_fill(pixbuf_border, 0x000000ff);    /* RGBA: opaque black */
        gdk_pixbuf_copy_area(pixbuf, 0, 0, x, y, pixbuf_border, 1, 1);
        combo_pixbufs[index]=pixbuf_border;
        g_object_unref(pixbuf);
    }
    return combo_pixbufs[index];
}

/*
    returns a cell pixbuf, scaled to gtk icon size.
    it also adds a little black border, which makes them look much better
*/
GdkPixbuf *EditorCellRenderer::combo_pixbuf(GdElementEnum element) {
    /* which pixbuf to show? */
    int index=abs(gd_element_properties[element].image);
    return get_element_pixbuf_with_border(index);
}

/*
    returns a cell pixbuf, scaled to gtk icon size.
    it also adds a little black border, which makes them look much better
*/
GdkPixbuf *EditorCellRenderer::combo_pixbuf_simple(GdElementEnum element) {
    /* which pixbuf to show? */
    int index=abs(gd_element_properties[element].image_simple);
    return get_element_pixbuf_with_border(index);
}


/**
 * @brief Creates a pixbuf, which shows the cave.
 *
 * If width and height are given (nonzero),
 * scales pixbuf proportionally, so it fits in width*height
 * pixels. Itherwise return in original size.
 * Up to the caller to unref the returned pixbuf.
 *
 * @param cave The cave to draw
 * @param width The width of the pixbuf to draw (scales down to this)
 * @param height The height of the pixbuf to draw (scales down to this)
 * @param game_view If true, a more simplistic view is generated (no arrows on creatures)
 * @param border If true, a 2pixel black border is added
 */
GdkPixbuf *gd_drawcave_to_pixbuf(const CaveRendered *cave, EditorCellRenderer &cr, int width, int height, bool game_view, bool border) {

    int x1, y1, x2, y2;
    int borderadd=border?4:0, borderpos=border?2:0;

    g_assert(!cave->map.empty());
    if (game_view) {
        /* if showing the visible part only */
        x1=cave->x1;
        y1=cave->y1;
        x2=cave->x2;
        y2=cave->y2;
    } else {
        /* showing entire cave - for example, overview in editor */
        x1=0;
        y1=0;
        x2=cave->w-1;
        y2=cave->h-1;
    }

    cr.select_pixbuf_colors(cave->color0, cave->color1, cave->color2, cave->color3, cave->color4, cave->color5);

    /* get size of one cell in the original pixbuf */
    int cell_size=cr.get_cell_pixbuf_size();

    /* add two pixels black border: +4 +4 for width and height */
    GdkPixbuf *pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha(cr.cell_gdk_pixbuf(0)), 8, (x2-x1+1)*cell_size+borderadd, (y2-y1+1)*cell_size+borderadd);
    if (border)
        gdk_pixbuf_fill(pixbuf, 0x000000ff);    /* fill with opaque black, so border is black */

    /* take visible part into consideration */
    for (int y=y1; y<=y2; y++)
        for (int x=x1; x<=x2; x++) {
            GdElementEnum element=cave->map(x,y);
            int draw;

            if (game_view) {
                /* visual effects */
                switch (element) {
                    case O_DIRT:
                        element=cave->dirt_looks_like;
                        break;
                    case O_EXPANDING_WALL:
                    case O_H_EXPANDING_WALL:
                    case O_V_EXPANDING_WALL:
                        /* only change the view, if it is not brick wall (the default value). */
                        /* so arrows remain - as well as they always remaing for the steel expanding wall,
                           which has no visual effect. */
                        if (cave->expanding_wall_looks_like!=O_BRICK)
                            element=cave->expanding_wall_looks_like;
                        break;
                    case O_AMOEBA_2:
                        element=cave->amoeba_2_looks_like;
                        break;
                    default:
                        /* we check that this element has no visual effect. */
                        /* otherwise, we should have handled the element explicitely above! */
                        g_assert((gd_element_properties[element].flags & P_VISUAL_EFFECT) == 0);
                        break;
                }
                draw=abs(gd_element_properties[element].image_simple);                /* pixbuf like in the editor */
            } else
                draw=gd_element_properties[element].image;                /* pixbuf like in the editor */
            gdk_pixbuf_copy_area(cr.cell_gdk_pixbuf(draw), 0, 0, cell_size, cell_size, pixbuf, (x-x1)*cell_size+borderpos, (y-y1)*cell_size+borderpos);
        }

    /* if requested size is 0, return unscaled */
    if (width==0 || height==0)
        return pixbuf;

    /* decide which direction fits in rectangle */
    /* cells are squares... no need to know cell_size here */
    float scale;
    if ((float) gdk_pixbuf_get_width(pixbuf) / (float) gdk_pixbuf_get_height(pixbuf) >= (float) width / (float) height)
        scale=width / ((float) gdk_pixbuf_get_width(pixbuf));
    else
        scale=height / ((float) gdk_pixbuf_get_height(pixbuf));

    /* scale to specified size */
    GdkPixbuf *scaled=gdk_pixbuf_scale_simple(pixbuf, gdk_pixbuf_get_width(pixbuf)*scale, gdk_pixbuf_get_height(pixbuf)*scale, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);

    return scaled;
}
