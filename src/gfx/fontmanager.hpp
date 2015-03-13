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

#ifndef _GD_GFX_FONTS
#define _GD_GFX_FONTS

#include <list>
#include <glib.h>
#include <string>
#include <vector>

#include "cave/helper/colors.hpp"
#include "gfx/pixmap.hpp"

class Pixbuf;
class Pixmap;
class PixbufFactory;
class Screen;

/* these can't be larger than 31, or they mess up utf8 coding or are the same as some ascii letter */
#define GD_DOWN_CHAR char(1)
#define GD_LEFT_CHAR char(2)
#define GD_UP_CHAR char(3)
#define GD_RIGHT_CHAR char(4)

#define GD_BALL_CHAR char(5)
#define GD_UNCHECKED_BOX_CHAR char(6)
#define GD_CHECKED_BOX_CHAR char(7)

#define GD_PLAYER_CHAR char(8)
#define GD_DIAMOND_CHAR char(9)
#define GD_NEWLINE_CHAR char(10)    // this cannot be used as a separate character, as it is the newline char
#define GD_SKELETON_CHAR char(11)
#define GD_KEY_CHAR char(12)
#define GD_FULL_BOX_CHAR char(13)


#define GD_ACUTE_CHAR char(16)
#define GD_UMLAUT_CHAR char(17)
#define GD_DOUBLE_ACUTE_CHAR char(18)

#define GD_UNKNOWN_CHAR char(127)


class RenderedFont;

/// @ingroup Graphics
/**
 * @brief The class which manages font rendering and drawing.
 *
 * After creating a font manager, its blittext functions
 * can be used to draw text on the screen using C64 fonts.
 * The blittext function draws wide (16x8), the blittext_n
 * function draws narrow (8x8) characters. (Of course,
 * these sizes are enlarged by the pixbuf factory.)
 *
 */
class FontManager : public PixmapStorage {
private:
    /// Raw font data. Its format is very similar
    /// to the one used by cells.
    /// A font map consists of 128 chars, each usually 8x8 pixel.
    /// The 128 chars are in a table, 16 horizontally x 8 vertically.
    std::vector<unsigned char> font;

    /// The size of the font pixbufs, in pixels. This may be sized up when drawing.
    unsigned int font_size;

    /// The currently selected color
    GdColor current_color;

    /// The pixbuf factory used to render the fonts.
    PixbufFactory const &pixbuf_factory;

    /// Rendered fonts are stored in a list for caching.
    typedef std::list<RenderedFont *> container;
    /// Cached fonts for narrow and wide letters.
    container _narrow, _wide;

    /// @brief Return with the narrow rendered font.
    /// If it does not exist yet, create. If too many
    /// rendered characters are in the cache, delete the
    /// oldest (least recently used) one.
    /// @param c The color to draw the font.
    /// @return The font created for the color.
    RenderedFont *narrow(const GdColor &c);

    /// @brief Return with the wide rendered font.
    /// If it does not exist yet, create. If too many
    /// rendered characters are in the cache, delete the
    /// oldest (least recently used) one.
    /// @param c The color to draw the font.
    /// @return The font created for the color.
    RenderedFont *wide(const GdColor &c);

    /// @brief Draw a piece of text with wide or narrow font.
    /// @param screen The screen or pixmap to draw on.
    /// @param x The x coordinate to start drawing at. If -1 is given,
    ///     the text will be centered on the screen.
    /// @param y The y coordinate to draw at.
    /// @param text The UTF8 text to draw. Can contain more lines.
    ///     Can contain accented characters, most of which will be drawn
    ///     without an accent.
    /// @param widefont True for the wide font, false for the narrow one.
    /// @return The x coordinate at which the drawing was ended.
    ///     By this, more text can be written with successive blittext calls.
    int blittext_internal(Screen &screen, int x, int y, char const *text, bool widefont);

    FontManager(const FontManager &);   // not implemented
    FontManager &operator=(const FontManager &); // not implemented

    bool loadfont_image(Pixbuf const &loadcells_image);
    bool loadfont_file(const std::string &filename);

public:
    /// @brief Creates a font manager.
    FontManager(const PixbufFactory &pixbuf_factory_, const std::string &theme_file);

    /// @brief Destructor.
    virtual ~FontManager();

    /// @brief Set the color for the next piece of text drawn.
    void set_color(GdColor const &color) {
        current_color = color;
    }

    /// @brief Write text on the screen with the wide font.
    /// For more info, look at blittext_internal.
    int blittext(Screen &screen, int x, int y, char const *text) {
        return blittext_internal(screen, x, y, text, true);
    }

    /// @brief Write text on the screen with the wide font.
    /// For more info, look at blittext_internal.
    int blittext(Screen &screen, int x, int y, const GdColor &color, char const *text) {
        set_color(color);
        return blittext_internal(screen, x, y, text, true);
    }

    /// @brief Write text on the screen with the narrow font.
    /// For more info, look at blittext_internal.
    int blittext_n(Screen &screen, int x, int y, char const *text) {
        return blittext_internal(screen, x, y, text, false);
    }

    /// @brief Write text on the screen with the narrow font.
    /// For more info, look at blittext_internal.
    int blittext_n(Screen &screen, int x, int y, const GdColor &color, char const *text) {
        set_color(color);
        return blittext_internal(screen, x, y, text, false);
    }

    /// @brief Set font of the manager.
    /// Destroys cache and starts drawing with a different font.
    void load_theme(const std::string &theme_file);

    /// @brief Destroy cache.
    void clear();
    
    /// @brief Implement PixmapStorage.
    virtual void release_pixmaps();

    /// @brief Get scale factor for drawing. @todo Remove?
    int get_pixmap_scale() const;

    /// @brief Get height of font in pixels.
    int get_font_height() const;

    /// @brief Get line height in pixels - a bit taller than font height.
    int get_line_height() const;

    /// @brief Get width of wide font in pixels.
    int get_font_width_wide() const;

    /// @brief Get width of narrow font in pixels.
    int get_font_width_narrow() const;

    /// @brief This function checks if a file is suitable to be used as a GDash font theme.
    static bool is_image_ok_for_theme(PixbufFactory &pixbuf_factory, const char *image);
    static bool is_pixbuf_ok_for_theme(const Pixbuf &surface);
};


#endif
