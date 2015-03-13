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

#include <gtk/gtk.h>

#include "gfx/cellrenderer.hpp"
#include "gtk/gtkpixmap.hpp"
#include "gtk/gtkpixbuf.hpp"
#include "gfx/pixbuffactory.hpp"

class GTKPixbufFactory;
class CaveRendered;

/**
 * @brief The EditorCellRenderer is a special cell renderer used in the GTK
 * version of the game, which has cells used only in the editor.
 *
 * When created (or colors are changed), some cells are also drawn. Most of
 * these cells are for example creatures with direction of movement arrows.
 */
class EditorCellRenderer: public CellRenderer {
private:
    /** Small pixbufs for the editor. */
    GdkPixbuf *combo_pixbufs[NUM_OF_CELLS];

    void add_arrow_to_cell(GdElementEnum dest, GdElementEnum src, GdElementEnum arrow, PixbufFactory::Rotation r=PixbufFactory::None);
    void copy_cell(int dest, int src);
    void create_composite_cell_pixbuf(GdElementEnum dest, GdElementEnum src1, GdElementEnum src2);
    void draw_editor_pixbufs();
    GdkPixbuf *get_element_pixbuf_with_border(int index);

public:
    /**
     * @brief Constructor.
     *
     * Loads a theme file (or "" for default theme); uses pixbuf_factory as a gfx engine.
     * @param pixbuf_factory_ The gfx engine to use.
     * @param theme_name The theme name from the caveset "charset" field.
     * @param default_theme_file The name of the file to load a theme from, if the specified theme is not found. Can be the empty string, to load the built-in theme.
     */
    EditorCellRenderer(GTKPixbufFactory &pixbuf_factory_, const std::string &theme_file);
    virtual void remove_cached();
    virtual void select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5);

    /** Returns cave background color currently used by the cell renderer. */
    const GdColor &background_color() const {
        return color0;
    }

    /** Convenience function which returns the GdkPixbuf* of a cell. */
    GdkPixbuf *cell_gdk_pixbuf(unsigned i) {
        return static_cast<GTKPixbuf &>(cell_pixbuf(i)).get_gdk_pixbuf();
    }

    /** Convenience function which returns the GdkDrawable* (pixmap) of a cell. */
    GdkDrawable *cell_gdk_drawable(unsigned i) {
        return static_cast<GTKPixmap &>(cell(i)).get_drawable();
    }

    /** @brief Returns a small picture of the element, for use in the editor.
     *  The picture will be GTK_ICON_SIZE_MENU, +1 pixel black border.
     *  For convenience, the picture is returned as a GdkPixbuf *.
     */
    GdkPixbuf *combo_pixbuf(GdElementEnum element);

    /** @brief Returns a small picture of the element, for use in the editor.
     *  The picture is the simplistic one (not showing movement arrows).
     *  The picture will be GTK_ICON_SIZE_MENU, +1 pixel black border.
     *  For convenience, the picture is returned as a GdkPixbuf *.
     */
    GdkPixbuf *combo_pixbuf_simple(GdElementEnum element);
};

GdkPixbuf *gd_drawcave_to_pixbuf(const CaveRendered *cave, EditorCellRenderer &cr, int width, int height, bool game_view, bool border);
