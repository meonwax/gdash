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

#include "misc/logger.hpp"
#include "misc/util.hpp"
#include "misc/printf.hpp"

#include "gfx/cellrenderer.hpp"
#include "gfx/pixbuf.hpp"
#include "gfx/pixmap.hpp"
#include "gfx/pixbuffactory.hpp"
#include "settings.hpp"


/* data */
#include "c64_gfx.cpp"

CellRenderer::CellRenderer(PixbufFactory &pixbuf_factory_, const std::string &theme_file)
    :   loaded(0),
        cells_all(0),
        is_c64_colored(false),
        cell_size(0),
        cells_pixbufs(),
        cells(),
        color0(GD_GDASH_BLACK),
        color1(GD_GDASH_MIDDLEBLUE),
        color2(GD_GDASH_LIGHTRED),
        color3(GD_GDASH_WHITE),
        color4(GD_GDASH_WHITE),
        color5(GD_GDASH_WHITE),
        pixbuf_factory(pixbuf_factory_) {
    load_theme_file(theme_file);
}


CellRenderer::~CellRenderer() {
    remove_cached();
    delete loaded;
    delete cells_all;
}

/* remove pixmaps from x server */
void CellRenderer::remove_cached() {
    /* if cells loaded, delete them */
    for (unsigned i=0; i<G_N_ELEMENTS(cells_pixbufs); ++i) {
        delete cells_pixbufs[i];
        cells_pixbufs[i]=0;
    }
    for (unsigned i=0; i<G_N_ELEMENTS(cells); ++i) {
        delete cells[i];
        cells[i]=0;
    }
    if (is_c64_colored) {
        delete cells_all;
        cells_all = 0;
    }
}

Pixbuf &CellRenderer::cell_pixbuf(unsigned i) {
    g_assert(i<G_N_ELEMENTS(cells_pixbufs));
    if (cells_all == NULL)
        create_colorized_cells();
    g_assert(cells_all != NULL);
    if (cells_pixbufs[i] == NULL)
        cells_pixbufs[i] = pixbuf_factory.create_subpixbuf(*cells_all, (i%NUM_OF_CELLS_X)*cell_size, (i/NUM_OF_CELLS_X)*cell_size, cell_size, cell_size);
    return *cells_pixbufs[i];
}

Pixmap &CellRenderer::cell(unsigned i) {
    g_assert(i<G_N_ELEMENTS(cells));
    if (cells[i]==0) {
        int type=i/NUM_OF_CELLS;    // 0=normal, 1=colored1, 2=colored2
        int index=i%NUM_OF_CELLS;
        Pixbuf &pb=cell_pixbuf(index);      // this is to be rendered as a pixmap, but may be colored

        switch (type) {
            case 0:
                cells[i]=pixbuf_factory.create_pixmap_from_pixbuf(pb, false);
                break;
            case 1: {
                Pixbuf *colored=pixbuf_factory.create_composite_color(pb, gd_flash_color);
                cells[i]=pixbuf_factory.create_pixmap_from_pixbuf(*colored, false);
                delete colored;
            }
            break;

            case 2: {
                Pixbuf *colored=pixbuf_factory.create_composite_color(pb, gd_select_color);
                cells[i]=pixbuf_factory.create_pixmap_from_pixbuf(*colored, false);
                delete colored;
            }
            break;
            default:
                g_assert_not_reached();
        }
    }
    return *cells[i];
}

/* check if given surface is ok to be a gdash theme. */
bool CellRenderer::is_pixbuf_ok_for_theme(const Pixbuf &surface) {
    if ((surface.get_width() % NUM_OF_CELLS_X != 0)
            || (surface.get_height() % NUM_OF_CELLS_Y != 0)
            || (surface.get_width() / NUM_OF_CELLS_X != surface.get_height() / NUM_OF_CELLS_Y)) {
        gd_critical(CPrintf("Image should contain %d cells in a row and %d in a column!") % int(NUM_OF_CELLS_X) % int(NUM_OF_CELLS_Y));
        return false;
    }
    if (surface.get_width() / NUM_OF_CELLS_X < 16) {
        gd_critical("The image should contain cells which are at least 16x16 pixels in size!");
    }
    if (!surface.has_alpha()) {
        gd_critical("Image should have an alpha channel!");
        return false;
    }

    return true;    /* passed checks */
}

bool CellRenderer::is_image_ok_for_theme(PixbufFactory &pixbuf_factory, const char *filename) {
    try {
        Pixbuf *image=pixbuf_factory.create_from_file(filename);
        /* if the image is loaded */
        SetLoggerContextForFunction scf(filename);
        bool result=is_pixbuf_ok_for_theme(*image);
        delete image;
        return result;
    } catch (...) {
        return false;
    }
}

/* load theme from image file. */
/* return true if successful. */
bool CellRenderer::loadcells_image(Pixbuf *image) {
    /* do some checks. if those fail, the error is already reported by the function */
    if (!is_pixbuf_ok_for_theme(*image)) {
        delete image;
        return false;
    }

    /* remove old stuff */
    remove_cached();
    delete loaded;
    loaded = NULL;

    /* load new stuff */
    cell_size = image->get_width()/NUM_OF_CELLS_X;
    loaded = image;

    if (check_if_pixbuf_c64_png(*loaded)) {
        /* c64 pixbuf with a small number of colors which can be changed */
        cells_all = NULL;
        is_c64_colored = true;
    } else {
        /* normal, "truecolor" pixbuf */
        cells_all = loaded;
        loaded = NULL;
        is_c64_colored = false;
    }
    return true;
}


/* load theme from image file. */
/* return true if successful. */
bool CellRenderer::loadcells_file(const std::string &filename) {
    /* load cell graphics */
    /* load from file */
    try {
        Pixbuf *image = pixbuf_factory.create_from_file(filename.c_str());
        return loadcells_image(image);
    } catch (std::exception &e) {
        gd_critical(CPrintf("%s: unable to load image (%s)") % filename % e.what());
        return false;
    }
}

/* load the theme specified in theme_file. */
/* if successful, ok. */
/* if fails, or no theme specified, load the builtin */
void CellRenderer::load_theme_file(const std::string &theme_file) {
    if (theme_file!="" && loadcells_file(theme_file)) {
        /* loaded from png file */
    } else {
        Pixbuf *image = pixbuf_factory.create_from_inline(sizeof(c64_gfx), c64_gfx);
        loadcells_image(image);
    }
}

int CellRenderer::get_cell_size() {
    return cell_size*pixbuf_factory.get_pixmap_scale();
}

bool CellRenderer::get_pal_emulation() const {
    return pixbuf_factory.get_pal_emulation();
}

void CellRenderer::select_pixbuf_colors(GdColor c0, GdColor c1, GdColor c2, GdColor c3, GdColor c4, GdColor c5) {
    if (c0!=color0 || c1!=color1 || c2!=color2 || c3!=color3 || c4!=color4 || c5!=color5) {
        /* if not the same colors as requested before */
        color0 = c0;
        color1 = c1;
        color2 = c2;
        color3 = c3;
        color4 = c4;
        color5 = c5;
        if (is_c64_colored)
            remove_cached();
    }
}



static inline int
c64_color_index(int h, int s, int v, int a) {
    if (a < 0x80)
        return 8;   /* transparent */
    if (v < 0x10)
        return 0;   /* black */
    if (s < 0x10)
        return 7;   /* editor white, arrows & etc */

    if (h < 30 || h >= 330)  /* around 0 */
        return 1;            /* red - foreg1 */
    if (h >= 270 && h < 330) /* around 300 */
        return 2;            /* purple - foreg2 */
    if (h >= 30 && h < 90)   /* around 60 */
        return 3;            /* yellow - foreg3 */
    if (h >= 90 && h < 150)  /* around 120 */
        return 4;            /* green - amoeba */
    if (h >= 210 && h < 270) /* around 240 */
        return 5;            /* slime */

    if (h >= 150 && h < 210) /* around 180 */
        return 6;            /* cyan - editor black */

    return 0;                /* should be unreachable */
}


/* returns true, if the given pixbuf seems to be a c64 imported image. */
bool CellRenderer::check_if_pixbuf_c64_png(Pixbuf const &image) {
    int wx=image.get_width()*4;       // 4 bytes/pixel
    int h=image.get_height();

    image.lock();
    bool c64_png = true;
    for (int y=0; y<h; y++) {
        const unsigned char *p=(const unsigned char *) image.get_row(y);
        for (int x=0; x<wx; x++)
            if (p[x]!=0 && p[x]!=255)
                c64_png=false;
    }
    image.unlock();

    return c64_png;
}


/** This function takes the loaded image, and transforms it using the selected
 * cave colors, to create cells_all.
 *
 * The process is as follows. All pixels are converted to HSV (hue, saturation,
 * value). The hues of the pixels should be 0 (red), 60 (yellow), 120 (green)
 * etc, n*60. This way will the routine recognize the colors.
 * After converting the pixels to HSV, the hue selects the cave color to use.
 * The resulting color will use the hue of the selected cave color, the product
 * of the saturations of the cave color and the original color, and the
 * product of the values:
 *
 *   Hresult = Hcave
 *   Sresult = Scave * Sloadedimage
 *   Vresult = Vcave * Vloadedimage.
 *
 * This allows for modulating the cave colors in saturation and value. If the
 * loaded image contains a dark purple color instead of RGB(255;0;255) purple,
 * the cave color will also be darkened at that pixel and so on. */
#include <iostream>
void CellRenderer::create_colorized_cells() {
    g_assert(is_c64_colored);
    g_assert(loaded != NULL);

    if (cells_all)
        delete cells_all;

    GdColor cols[9];    /* holds rgba for color indexes internally used */

    cols[0] = color0.to_hsv(); /* c64 background */
    cols[1] = color1.to_hsv(); /* foreg1 */
    cols[2] = color2.to_hsv(); /* foreg2 */
    cols[3] = color3.to_hsv(); /* foreg3 */
    cols[4] = color4.to_hsv(); /* amoeba */
    cols[5] = color5.to_hsv(); /* slime */
    cols[6] = GdColor::from_hsv(0, 0, 0);    /* black, opaque */
    cols[7] = GdColor::from_hsv(0, 0, 100);  /* white, opaque */
    cols[8] = GdColor::from_hsv(0, 0, 0);    /* for the transparent */

    int w = loaded->get_width(), h = loaded->get_height();
    cells_all = pixbuf_factory.create(w, h);

    cells_all->lock();
    loaded->lock();

    for (int y=0; y<h; y++) {
        const guint32 *p = loaded->get_row(y);
        guint32 *to = cells_all->get_row(y);
        for (int x=0; x<w; x++) {
            /* rgb values found in image */
            unsigned r = (p[x] & loaded->rmask) >> loaded->rshift;
            unsigned g = (p[x] & loaded->gmask) >> loaded->gshift;
            unsigned b = (p[x] & loaded->bmask) >> loaded->bshift;
            unsigned a = (p[x] & loaded->amask) >> loaded->ashift;

            GdColor in = GdColor::from_rgb(r, g, b).to_hsv();
            /* the color code from the original image (essentially the hue) will select the color index */
            unsigned index = c64_color_index(in.get_h(), in.get_s(), in.get_v(), a);
            GdColor newcol;
            if (index == 0 || index >= 6) {
                /* for the background and the editor colors, no shading is used */
                newcol = cols[index];
            } else {
                /* otherwise the saturation and value from the original image will modify it */
                unsigned s_multiplier = in.get_s();
                unsigned v_multiplier = in.get_v();
                newcol = GdColor::from_hsv(
                             cols[index].get_h(),
                             cols[index].get_s() * s_multiplier / 100,
                             cols[index].get_v() * v_multiplier / 100).to_rgb();
            }

            guint32 newcolword =
                newcol.get_r() << cells_all->rshift |
                newcol.get_g() << cells_all->gshift |
                newcol.get_b() << cells_all->bshift |
                a << cells_all->ashift;

            to[x] = newcolword;
        }
    }

    loaded->unlock();
    cells_all->unlock();
}
