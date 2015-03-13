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
#ifndef GD_PIXBUF
#define GD_PIXBUF

#include <glib.h>
#include <vector>

#include "config.h"

class GdColor;

/// @defgroup Graphics

/// @ingroup Graphics
/// A class which represents a 32-bit RGBA image in memory.
class Pixbuf {
public:

    /* these masks are always for RGBA ordering in memory.
       that is what gdk-pixbuf uses, and also what sdl uses.
       no BGR, no ABGR.
     */
    #if G_BYTE_ORDER == G_BIG_ENDIAN
        static const guint32 rmask = 0xff000000;
        static const guint32 gmask = 0x00ff0000;
        static const guint32 bmask = 0x0000ff00;
        static const guint32 amask = 0x000000ff;

        static const guint32 rshift = 24;
        static const guint32 gshift = 16;
        static const guint32 bshift = 8;
        static const guint32 ashift = 0;
    #else
        static const guint32 rmask = 0x000000ff;
        static const guint32 gmask = 0x0000ff00;
        static const guint32 bmask = 0x00ff0000;
        static const guint32 amask = 0xff000000;

        static const guint32 rshift = 0;
        static const guint32 gshift = 8;
        static const guint32 bshift = 16;
        static const guint32 ashift = 24;
    #endif

    /// Creates a guint32 value, which can be raw-written to a pixbuf memory area.
    static guint32 rgba_pixel_from_color(const GdColor& col, unsigned a);

    /// Get the width of the pixbuf in pixels.
    virtual int get_width() const=0;

    /// Get the height of the pixbuf in pixels.
    virtual int get_height() const=0;

    /// @brief Blit pixbuf to another pixbuf (for an area).
    /// @param dest The destination pixbuf.
    /// @param x Upper left coordinate of source area to copy.
    /// @param y Upper left coordinate of source area to copy.
    /// @param w Width of area to copy.
    /// @param h Height of area to copy.
    /// @param dx Destination coordinate, upper left corner.
    /// @param dy Destination coordinate, upper left corner.
    virtual void blit_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const=0;

    /// @brief Blit pixbuf to another pixbuf (for an area).
    /// @param dest The destination pixbuf.
    /// @param x Upper left coordinate of source area to copy.
    /// @param y Upper left coordinate of source area to copy.
    /// @param w Width of area to copy.
    /// @param h Height of area to copy.
    /// @param dx Destination coordinate, upper left corner.
    /// @param dy Destination coordinate, upper left corner.
    /// The purpose of this function is to give an overloaded name for blit_full,
    /// without requiring derived classes to say "using Pixbuf::blit".
    void blit(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const {
        blit_full(x, y, w, h, dest, dx, dy);
    }

    /// @brief Blit the whole pixbuf to another pixbuf.
    /// @param dest The destination pixbuf.
    /// @param dx Destination coordinate, upper left corner.
    /// @param dy Destination coordinate, upper left corner.
    void blit(Pixbuf& dest, int dx, int dy) const {
        blit_full(0, 0, get_width(), get_height(), dest, dx, dy);
    }

    /// @brief Copy pixbuf to another pixbuf (copy area). No alpha-blending, no nothing, just copies data.
    /// @param dest The destination pixbuf.
    /// @param x Upper left coordinate of source area to copy.
    /// @param y Upper left coordinate of source area to copy.
    /// @param w Width of area to copy.
    /// @param h Height of area to copy.
    /// @param dx Destination coordinate, upper left corner.
    /// @param dy Destination coordinate, upper left corner.
    virtual void copy_full(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const=0;

    /// @brief Copy pixbuf to another pixbuf (copy area). No alpha-blending, no nothing, just copies data.
    /// @param dest The destination pixbuf.
    /// @param x Upper left coordinate of source area to copy.
    /// @param y Upper left coordinate of source area to copy.
    /// @param w Width of area to copy.
    /// @param h Height of area to copy.
    /// @param dx Destination coordinate, upper left corner.
    /// @param dy Destination coordinate, upper left corner.
    /// The purpose of this function is to give an overloaded name for blit_full,
    /// without requiring derived classes to say "using Pixbuf::blit".
    void copy(int x, int y, int w, int h, Pixbuf &dest, int dx, int dy) const {
        copy_full(x, y, w, h, dest, dx, dy);
    }

    /// @brief Copy pixbuf to another pixbuf (copy area). No alpha-blending, no nothing, just copies data.
    /// @param dest The destination pixbuf.
    /// @param dx Destination coordinate, upper left corner.
    /// @param dy Destination coordinate, upper left corner.
    void copy(Pixbuf& dest, int dx, int dy) const {
        copy_full(0, 0, get_width(), get_height(), dest, dx, dy);
    }

    /// @brief Check if the pixbuf has an alpha channel.
    /// @return true, if it has one.
    virtual bool has_alpha() const=0;

    /// @brief Fill the given area of the pixbuf with the specified color.
    /// No blending takes place!
    virtual void fill_rect(int x, int y, int w, int h, const GdColor& c)=0;
    void fill(const GdColor &c) {
        fill_rect(0, 0, get_width(), get_height(), c);
    }

    virtual unsigned char *get_pixels() const=0;
    virtual int get_pitch() const=0;

    /// Get pointer to row number n, cast to a 32-bit unsigned integer.
    /// Using this, one can: pixbuf.get_row(y)[x]=0xRRGGBBAA or 0xAABBGGRR;
    guint32* get_row(int y) { return reinterpret_cast<guint32*>(get_pixels()+y*get_pitch()); }
    const guint32* get_row(int y) const { return reinterpret_cast<guint32*>(get_pixels()+y*get_pitch()); }
    /// Generic get/putpixel.
    guint32& operator()(int x, int y) { return get_row(y)[x]; }
    /// Generic getpixel.
    guint32 const& operator()(int x, int y) const { return get_row(y)[x]; }
    
    /// Lock a surface - enable only the current thread to access the pixbuf.
    virtual void lock() const {}

    /// Unlock the surface.
    virtual void unlock() const {}

    virtual ~Pixbuf() {}

    static std::vector<unsigned char> c64_gfx_data_from_pixbuf(Pixbuf const& image);
};

#endif

