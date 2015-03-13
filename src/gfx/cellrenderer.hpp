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

#ifndef _GD_GFX_CELLS
#define _GD_GFX_CELLS

#include <vector>

#include "cave/cavetypes.hpp"
#include "cave/helper/colors.hpp"

class PixbufFactory;
class Pixbuf;
class Pixmap;

/// @ingroup Graphics
/// @brief The class which is responsible for rendering the cave pixbufs.
/// In its constructor, it takes a pixbuf factory as a parameter (for the graphics
/// subsystem), and the name of a theme file to load.
/// If the theme file cannot be loaded, or an empty string is given as file name,
/// it switches to the builtin default theme.
/// After loading the theme file, select_pixbuf_colors is used to select a color theme.
/// A pixbuf of a cell can be retrieved using cell(i).
class CellRenderer {
protected:
    /// The pixbuf which was loaded from the disk.
    Pixbuf *loaded;

    /// The pixbuf which stores rgb data of all images.
    Pixbuf *cells_all;

    bool is_c64_colored;

    /// The size of the loaded pixbufs
    unsigned cell_size;

    /// The cache to store the pixbufs already rendered.
    Pixbuf *cells_pixbufs[NUM_OF_CELLS];

    /// The cache to store the pixbufs already rendered.
    Pixmap *cells[3*NUM_OF_CELLS];

    /// If using c64 gfx, these store the current color theme.
    GdColor color0, color1, color2, color3, color4, color5;

    void create_colorized_cells();
    bool loadcells_image(Pixbuf *loadcells_image);
    bool loadcells_file(const std::string &filename);
    virtual void remove_cached();

    CellRenderer(const CellRenderer &); // not implemented
    CellRenderer &operator=(const CellRenderer &);  // not implemented

public:
    /// The pixbuf factory which is used to create pixbufs and pixmaps.
    /// Public, so can be used by other objects - why not.
    PixbufFactory &pixbuf_factory;

    /// Constructor. Loads a theme file (or nothing); uses pixbuf_factory as a gfx engine.
    CellRenderer(PixbufFactory &pixbuf_factory_, const std::string &theme_file);

    /// Destructor.
    virtual ~CellRenderer();

    /// @brief Loads a new theme.
    /// The theme_file can be a file name of a png file, or empty.
    void load_theme_file(const std::string &theme_file);

    /// @brief Returns a particular cell.
    Pixbuf &cell_pixbuf(unsigned i);

    /// @brief Returns a particular cell.
    Pixmap &cell(unsigned i);

    /// @brief Returns the size of the pixmaps stored.
    /// They are squares, so there is only one function, not two for width and height.
    int get_cell_size();

    /// @brief Returns the size of the pixbufs
    /// They are squares, so there is only one function, not two for width and height.
    int get_cell_pixbuf_size() {
        return cell_size;
    }

    /// Returns the background color currently used.
    const GdColor &get_background_color() const {
        return color0;
    }

    /// Returns true if the pixbuf factory used uses pal emulation.
    bool get_pal_emulation() const;

    /// @brief Select a color theme, when using C64 graphics.
    /// If no c64 graphics is used, then this function does nothing.
    virtual void select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5);

    /// @brief This function checks if a file is suitable to be used as a GDash theme.
    static bool is_image_ok_for_theme(PixbufFactory &pixbuf_factory, const char *image);
    static bool is_pixbuf_ok_for_theme(Pixbuf const &surface);
    static bool check_if_pixbuf_c64_png(Pixbuf const &image);
};


#endif
