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
#ifndef _GD_COLORS
#define _GD_COLORS

#include "config.h"

#include <iostream>

#include "misc/printf.hpp"

/// @brief A class which stores a color in a cave.
///
/// Can store different kind of colors:
/// - C64 colors,
/// - C64DTV colors,
/// - Atari colors,
/// - RGB colors.
///
/// Can convert any of them to RGB color during playing the game.
/// The conversion from paletted to RGB colors uses a palette,
/// determined by user preferences.
class GdColor {
public:
    /// This enum selects the type of the color used.
    enum Type {
        TypeRGB,
        TypeC64,
        TypeC64DTV,
        TypeAtari,
        TypeHSV,
        TypeInvalid,
    };
    /// Constructor, which created a color object, initialized to invalid.
    /// For convenience only; any color which is to be used must be initialized.
    GdColor() : type(TypeInvalid) { }
    static GdColor from_rgb(unsigned r, unsigned g, unsigned b);
    static GdColor from_hsv(unsigned short h, unsigned char s, unsigned char v);
    static GdColor from_c64(unsigned index);
    static GdColor from_atari(unsigned index);
    static GdColor from_atari_huesat(unsigned hue, unsigned sat);
    static GdColor from_c64dtv(unsigned index);
    static GdColor from_c64dtv_huesat(unsigned hue, unsigned sat);
    static GdColor from_gdash_index(unsigned c);    /* similar to c64 colors */

    bool operator==(const GdColor &rhs) const;
    /// Compare two color objects for inequality.
    /// @return True, if they are not equal.
    bool operator!=(const GdColor &rhs) const {
        return !(*this==rhs);
    }

    unsigned int get_r() const;
    unsigned int get_g() const;
    unsigned int get_b() const;
    unsigned int get_h() const;
    unsigned int get_s() const;
    unsigned int get_v() const;
    unsigned int get_uint() const;
    GdColor to_rgb() const;
    GdColor to_hsv() const;

    bool is_c64() const;

    int get_c64_index() const;

    static const char **get_c64_palette_names();
    static const char **get_atari_palette_names();
    static const char **get_c64dtv_palette_names();
    static const char **get_palette_types_names();

private:
    Type type;
    union {
        unsigned char index;
        struct {
            unsigned char r;
            unsigned char g;
            unsigned char b;
        } rgb;
        struct {
            unsigned short h;  /* [0;360) */
            unsigned char s;   /* [0;100] */
            unsigned char v;   /* [0;100] */
        } hsv;
    };

    GdColor(unsigned char r, unsigned char g, unsigned char b) :
        type(TypeRGB) {
        rgb.r = r;
        rgb.g = g;
        rgb.b = b;
    }
    GdColor(Type _type, int i) :
        type(_type) {
        index = i;
    }
    GdColor(unsigned int rgb);

    friend std::string visible_name(const GdColor &c);
    friend std::ostream &operator<<(std::ostream &os, const GdColor &c);
};

/* i/o operators and functions like for all other cave types */
std::ostream &operator<<(std::ostream &os, const GdColor &c);
std::string visible_name(const GdColor &c);
bool read_from_string(std::string const &s, GdColor &c);


/// Traditional C64 color indexes, plus one GDash special color.
enum GdColorIndex {
    GD_COLOR_INDEX_BLACK=0,
    GD_COLOR_INDEX_WHITE=1,
    GD_COLOR_INDEX_RED=2,
    GD_COLOR_INDEX_PURPLE=4,
    GD_COLOR_INDEX_CYAN=3,
    GD_COLOR_INDEX_GREEN=5,
    GD_COLOR_INDEX_BLUE=6,
    GD_COLOR_INDEX_YELLOW=7,
    GD_COLOR_INDEX_ORANGE=8,
    GD_COLOR_INDEX_BROWN=9,
    GD_COLOR_INDEX_LIGHTRED=10,
    GD_COLOR_INDEX_GRAY1=11,
    GD_COLOR_INDEX_GRAY2=12,
    GD_COLOR_INDEX_LIGHTGREEN=13,
    GD_COLOR_INDEX_LIGHTBLUE=14,
    GD_COLOR_INDEX_GRAY3=15,
    GD_COLOR_INDEX_MIDDLEBLUE=16,
};
enum GdColorIndexHelper {
    GD_COLOR_SETCOLOR=31,   /* for blittext */
};

// specialized for color codes for font manager
template <>
inline Printf const &Printf::operator%(GdColorIndex const &colorindex) const {
    /* +64 is needed so it is a normal ascii char in the encoding, not a string limiter \0
     * or a \n or whatever */
    char s[3] = { GD_COLOR_SETCOLOR, char(colorindex+64), 0 };
    return (*this) % s;
}


#define GD_GDASH_BLACK (GdColor::from_gdash_index(GD_COLOR_INDEX_BLACK))
#define GD_GDASH_WHITE (GdColor::from_gdash_index(GD_COLOR_INDEX_WHITE))
#define GD_GDASH_RED (GdColor::from_gdash_index(GD_COLOR_INDEX_RED))
#define GD_GDASH_PURPLE (GdColor::from_gdash_index(GD_COLOR_INDEX_PURPLE))
#define GD_GDASH_CYAN (GdColor::from_gdash_index(GD_COLOR_INDEX_CYAN))
#define GD_GDASH_GREEN (GdColor::from_gdash_index(GD_COLOR_INDEX_GREEN))
#define GD_GDASH_BLUE (GdColor::from_gdash_index(GD_COLOR_INDEX_BLUE))
#define GD_GDASH_YELLOW (GdColor::from_gdash_index(GD_COLOR_INDEX_YELLOW))
#define GD_GDASH_ORANGE (GdColor::from_gdash_index(GD_COLOR_INDEX_ORANGE))
#define GD_GDASH_BROWN (GdColor::from_gdash_index(GD_COLOR_INDEX_BROWN))
#define GD_GDASH_LIGHTRED (GdColor::from_gdash_index(GD_COLOR_INDEX_LIGHTRED))
#define GD_GDASH_GRAY1 (GdColor::from_gdash_index(GD_COLOR_INDEX_GRAY1))
#define GD_GDASH_GRAY2 (GdColor::from_gdash_index(GD_COLOR_INDEX_GRAY2))
#define GD_GDASH_LIGHTGREEN (GdColor::from_gdash_index(GD_COLOR_INDEX_LIGHTGREEN))
#define GD_GDASH_LIGHTBLUE (GdColor::from_gdash_index(GD_COLOR_INDEX_LIGHTBLUE))
#define GD_GDASH_GRAY3 (GdColor::from_gdash_index(GD_COLOR_INDEX_GRAY3))
#define GD_GDASH_MIDDLEBLUE (GdColor::from_gdash_index(GD_COLOR_INDEX_MIDDLEBLUE))

extern const GdColor gd_flash_color;
extern const GdColor gd_select_color;

#endif

