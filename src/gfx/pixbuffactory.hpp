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
#ifndef GFX_PIXBUFFACTORY_H
#define GFX_PIXBUFFACTORY_H

#include "config.h"

class Pixbuf;
class Pixmap;
class GdColor;

extern const char *gd_scaling_name[];
extern const int gd_scaling_scale[];


/// Scaling types supported by the pixbuf engine
enum GdScalingType {
    GD_SCALING_ORIGINAL,    ///< no scaling
    GD_SCALING_2X,          ///< 2x nearest scaling
    GD_SCALING_2X_SCALE2X,    ///< 2x scaling with the scale2x filter
    GD_SCALING_2X_HQ2X,     ///< 2x scaling with the hq2x filter
    GD_SCALING_3X,          ///< 3x nearest scaling
    GD_SCALING_3X_SCALE3X,    ///< 3x scaling with the scale3x filter
    GD_SCALING_3X_HQ3X,       ///< 3x scaling with the hq2x filter
    GD_SCALING_4X,          ///< 4x nearest scaling
    GD_SCALING_4X_SCALE4X,    ///< 4x scaling (scale2x applied twice)
    GD_SCALING_4X_HQ4X,        ///< 4x scaling with hq4x
    GD_SCALING_MAX,
};


/// @ingroup Graphics
class PixbufFactory {
protected:
    GdScalingType scaling_type;
    bool pal_emulation;

    /// @brief Create a new pixbuf factory.
    PixbufFactory(GdScalingType scaling_type_, bool pal_emulation_);
public:
    /// @brief Return the scale factor of the pixbuf->pixmap.
    /// @return The scale factor, 1 is unscaled (original size)
    int get_pixmap_scale() const;

    /// @brief Set scaling type and pal emulation for factory.
    void set_properties(GdScalingType scaling_type_, bool pal_emulation_);

    /// @brief Returns true, if the factory uses pal emulation.
    bool get_pal_emulation() {
        return pal_emulation;
    }

    /// @brief Virtual destructor.
    virtual ~PixbufFactory() {}

    /// @brief Create a new pixbuf, with non-initialized memory.
    /// @param w The width of the pixbuf.
    /// @param h The height of the pixbuf.
    /// @return A newly allocated pixbuf object. Free with delete.
    virtual Pixbuf *create(int w, int h) const=0;

    /// @brief Create a new pixbuf, and load an image from memory.
    /// @param length The number of bytes of the image.
    /// @param data Pointer to the image in memory.
    /// @return A newly allocated pixbuf object. Free with delete.
    virtual Pixbuf *create_from_inline(int length, unsigned char const *data) const=0;

    /// @brief Create a new pixbuf, and load an image from a file.
    /// @param filename The name of the file to load.
    /// @return A newly allocated pixbuf object. Free with delete.
    virtual Pixbuf *create_from_file(const char *filename) const=0;

    /// @brief Create a new pixbuf, and load an image file from memory, which is base64 encoded..
    /// @param base64 Base64 encoded image string, delimited with zero.
    /// @return A newly allocated pixbuf object. Free with delete.
    Pixbuf *create_from_base64(const char *base64) const;

    /// @brief Composite pixbuf with color c, and return the new (composited) one
    /// @param c Color
    /// @param a Alpha value; 0 will be invisible, 255 will totally cover. Default is 128, which looks nice, and is accelerated by SDL.
    /// @return The new pixbuf.
    virtual Pixbuf *create_composite_color(const Pixbuf &src, const GdColor &c, unsigned char alpha=128) const=0;

    /// @brief Create a pixbuf, which is a part of this one.
    /// Pixels will be shared!
    virtual Pixbuf *create_subpixbuf(Pixbuf &src, int x, int y, int w, int h) const=0;

    /// @brief Create a newly allocated pixmap.
    /// @param pb The pixbuf to set the contents from.
    /// @param format_alpha Preserve alpha channel.
    /// @return A newly allocated pixmap object. Free with delete.
    virtual Pixmap *create_pixmap_from_pixbuf(Pixbuf const &pb, bool format_alpha) const=0;

    Pixbuf *create_scaled(const Pixbuf &src) const;

    /// Names of rotations.
    enum Rotation {
        None,
        CounterClockWise,
        UpsideDown,
        ClockWise
    };

    /// Creates a new, rotated pixbuf.
    virtual Pixbuf *create_rotated(const Pixbuf &src, Rotation r) const=0;
};

#endif
