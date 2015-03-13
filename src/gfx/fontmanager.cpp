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

#include <vector>
#include <algorithm>

#include "gfx/pixbuf.hpp"
#include "gfx/pixmap.hpp"
#include "gfx/screen.hpp"
#include "gfx/pixbuffactory.hpp"
#include "cave/helper/colors.hpp"
#include "gfx/cellrenderer.hpp"
#include "gfx/fontmanager.hpp"
#include "misc/printf.hpp"
#include "misc/logger.hpp"

#include "c64_font.cpp"


/**
 * @brief A RenderedFont stores glyphs rendered for a given font and a given color.
 * This is an abstract base class, the narrow and wide font are derived from this.
 */
class RenderedFont {
public:
    /// Number of characters on the font map.
    enum {
        CHARS_X=32,
        CHARS_Y=4,
        NUM_OF_CHARS=CHARS_X*CHARS_Y
    };

    /// Constructor.
    /// @param bitmap_ Raw font data.
    /// @param color_ The color of drawing.
    /// @param pixbuf_factory_ The pixbuf factory used to create glyphs.
    RenderedFont(std::vector<unsigned char> const &bitmap_,  unsigned font_size_, GdColor const &color_, PixbufFactory const &pixbuf_factory_);

    /// Destructor.
    virtual ~RenderedFont();

    /// Return the pixmap for a given character; maybe after creating it.
    /// @param j The ASCII (or GDash) code of the character
    Pixmap const &get_character(int j) const;

    /// GdColor::get_uint() code of color, for easy searching in a font manager.
    guint32 uint;

protected:
    /// Raw font data.
    std::vector<unsigned char> const &bitmap;

    /// Font size (pixbufs)
    unsigned int font_size;

    /// Pixbuf factory for drawing.
    PixbufFactory const &pixbuf_factory;

    /// RGBA format of color in pixbufs.
    guint32 col;

    /// RGBA format of transparent pixel in pixbufs.
    guint32 transparent;

    /// The rendered glyphs are stored in this array as pixmaps.
    mutable Pixmap *_character[NUM_OF_CHARS];

    RenderedFont(const RenderedFont &);     // not implemented
    RenderedFont &operator=(const RenderedFont &);  // not implemented

private:
    /// Render a single character. The narrow and wide fonts implement this.
    virtual void render_character(int j) const=0;
};

class RenderedFontNarrow: public RenderedFont {
private:
    virtual void render_character(int j) const;

public:
    RenderedFontNarrow(std::vector<unsigned char> const &bitmap_,  unsigned font_size_, GdColor const &color, PixbufFactory const &pixbuf_factory_)
        : RenderedFont(bitmap_, font_size_, color, pixbuf_factory_) {}
};

class RenderedFontWide: public RenderedFont {
private:
    virtual void render_character(int j) const;

public:
    RenderedFontWide(std::vector<unsigned char> const &bitmap_,  unsigned font_size_, GdColor const &color, PixbufFactory const &pixbuf_factory_)
        : RenderedFont(bitmap_, font_size_, color, pixbuf_factory_) {}
};


RenderedFont::RenderedFont(std::vector<unsigned char> const &bitmap_, unsigned font_size_, GdColor const &color, PixbufFactory const &pixbuf_factory_)
    :   uint(color.get_uint()),
        bitmap(bitmap_),
        font_size(font_size_),
        pixbuf_factory(pixbuf_factory_),
        _character() {
    g_assert(font_size_*font_size_*NUM_OF_CHARS==bitmap_.size());
    col=Pixbuf::rgba_pixel_from_color(color, 0xff); /* opaque */
    transparent=Pixbuf::rgba_pixel_from_color(GdColor::from_rgb(0, 0, 0), 0x00);    /* color does not matter as totally transparent */
}

void RenderedFontNarrow::render_character(int j) const {
    int y1=(j/CHARS_X)*font_size;
    int x1=(j%CHARS_X)*font_size;

    Pixbuf *image=pixbuf_factory.create(font_size, font_size);
    image->lock();
    for (unsigned y=0; y<font_size; y++) {
        guint32 *p=image->get_row(y);
        for (unsigned x=0; x<font_size; x++) {
            /* the font array is encoded the same way as a c64-colored pixbuf. see c64_gfx_data...() */
            if (bitmap[(y1+y)*(CHARS_X*font_size)+x1+x]!=1)     /* 1 is black there!! */
                p[x]=col;    /* normal */
            else
                p[x]=transparent;    /* normal */
        }
    }
    image->unlock();
    _character[j]=pixbuf_factory.create_pixmap_from_pixbuf(*image, true);     // true=preserve alpha channel!
    delete image;
}

void RenderedFontWide::render_character(int j) const {
    int y1=(j/CHARS_X)*font_size;
    int x1=(j%CHARS_X)*font_size;

    Pixbuf *image=pixbuf_factory.create(font_size*2, font_size);
    image->lock();
    for (unsigned y=0; y<font_size; y++) {
        guint32 *p=image->get_row(y);
        for (unsigned x=0; x<font_size; x++) {
            /* the font array is encoded the same way as a c64-colored pixbuf. see c64_gfx_data...() */
            if (bitmap[(y1+y)*(CHARS_X*font_size)+x1+x]!=1) {     /* 1 is black there!! */
                p[0]=col;    /* normal */
                p[1]=col;
            } else {
                p[0]=transparent;    /* normal */
                p[1]=transparent;    /* normal */
            }
            p+=2;
        }
    }
    image->unlock();
    _character[j]=pixbuf_factory.create_pixmap_from_pixbuf(*image, true);     // true=preserve alpha channel!
    delete image;
}

RenderedFont::~RenderedFont() {
    for (unsigned i=0; i<NUM_OF_CHARS; ++i)
        delete _character[i];
}

Pixmap const &RenderedFont::get_character(int j) const {
    g_assert(j<NUM_OF_CHARS);
    if (_character[j]==0)
        render_character(j);
    return *_character[j];
}


/* check if given surface is ok to be a gdash theme. */
bool FontManager::is_pixbuf_ok_for_theme(const Pixbuf &surface) {
    if ((surface.get_width() % RenderedFont::CHARS_X != 0)
            || (surface.get_height() % RenderedFont::CHARS_Y != 0)
            || (surface.get_width() / RenderedFont::CHARS_X != surface.get_height() / RenderedFont::CHARS_Y)) {
        gd_critical(CPrintf("image should contain %d chars in a row and %d in a column!") % int(RenderedFont::CHARS_X) % int(RenderedFont::CHARS_Y));
        return false;
    }
    if (!surface.has_alpha()) {
        gd_critical("image should have an alpha channel!");
        return false;
    }

    return true;    /* passed checks */
}

bool FontManager::is_image_ok_for_theme(PixbufFactory &pixbuf_factory, const char *filename) {
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

bool FontManager::loadfont_image(Pixbuf const &image) {
    if (!is_pixbuf_ok_for_theme(image))
        return false;

    clear();
    font = Pixbuf::c64_gfx_data_from_pixbuf(image);
    font_size = image.get_width()/RenderedFont::CHARS_X;
    return true;
}

/* load theme from image file. */
/* return true if successful. */
bool FontManager::loadfont_file(const std::string &filename) {
    /* load cell graphics */
    /* load from file */
    try {
        Pixbuf *image=pixbuf_factory.create_from_file(filename.c_str());
        bool result=loadfont_image(*image);
        delete image;
        if (!result)
            gd_critical(CPrintf("%s: invalid font bitmap") % filename);
        return result;
    } catch (std::exception &e) {
        gd_critical(CPrintf("%s: unable to load image (%s)") % filename % e.what());
        return false;
    }
}

/* load the theme from the given file. */
/* if successful, ok. */
/* if fails, or no theme specified, load the builtin */
void FontManager::load_theme(const std::string &theme_file) {
    if (theme_file!="" && loadfont_file(theme_file)) {
        /* loaded from png file */
    } else {
        Pixbuf *image=pixbuf_factory.create_from_inline(sizeof(c64_font), c64_font);
        bool result=loadfont_image(*image);
        g_assert(result==true);       // to check the builting font
        delete image;
    }
}

FontManager::FontManager(const PixbufFactory &pixbuf_factory_, const std::string &theme_file)
    :
    current_color(GD_GDASH_WHITE),
    pixbuf_factory(pixbuf_factory_) {
    load_theme(theme_file);
}

FontManager::~FontManager() {
    clear();
}

struct FindRenderedFont {
    guint32 uint;
    FindRenderedFont(guint32 uint_): uint(uint_) {}
    bool operator()(const RenderedFont *font) const {
        return font->uint==uint;
    }
};

RenderedFont *FontManager::narrow(const GdColor &c) {
    // find font in list
    container::iterator it=find_if(_narrow.begin(), _narrow.end(), FindRenderedFont(c.get_uint()));
    if (it==_narrow.end()) {
        // if not found, create it
        RenderedFont *newfont=new RenderedFontNarrow(font, font_size, c, pixbuf_factory);
        _narrow.push_front(newfont);
        // if list became too long, remove one from the end
        if (_narrow.size()>16) {
            delete _narrow.back();
            _narrow.erase(--_narrow.end());
        }
    } else
        // put the font found to the beginning of the list
        std::swap(*_narrow.begin(), *it);
    return *_narrow.begin();
}

RenderedFont *FontManager::wide(const GdColor &c) {
    // find font in list
    container::iterator it=find_if(_wide.begin(), _wide.end(), FindRenderedFont(c.get_uint()));
    if (it==_wide.end()) {
        // if not found, create it
        RenderedFont *newfont=new RenderedFontWide(font, font_size, c, pixbuf_factory);
        _wide.push_front(newfont);
        // if list became too long, remove one from the end
        if (_wide.size()>16) {
            delete _wide.back();
            _wide.erase(--_wide.end());
        }
    } else {
        // put the font found to the beginning of the list
        std::swap(*_wide.begin(), *it);
    }
    return *_wide.begin();
}

/* function which draws characters on the screen. used internally. */
/* x=-1 -> center horizontally */
int FontManager::blittext_internal(Screen &screen, int x, int y, char const *text, bool widefont) {
    char *normalized = g_utf8_normalize(text, -1, G_NORMALIZE_ALL);
    gunichar *ucs = g_utf8_to_ucs4(normalized, -1, NULL, NULL, NULL);
    g_free(normalized);

    RenderedFont const *font = widefont ? wide(current_color) : narrow(current_color);
    int w = font->get_character(' ').get_width();
    int h = get_line_height();

    if (x==-1) {
        gunichar c;
        int len = 0;
        for (int i = 0; (c = ucs[i]) != '\0'; ++i) {
            if (c==GD_COLOR_SETCOLOR)
                i += 1; /* do not count; skip next char */
            else if (c>=0x300 && c<0x370)
                ;       /* do not count, diacritical. */
            else
                len++;  /* count char */
        }
        x = screen.get_width()/2 - (w*len)/2;
    }

    int xc=x;
    gunichar c;
    for (int i = 0; (c = ucs[i]) != '\0'; ++i) {
        if (c>=0x300 && c<0x370) {
            // unicode diacritical mark block
            switch (c) {
                case 0x301:
                    screen.blit(font->get_character(GD_ACUTE_CHAR), xc-w, y);
                    break;
                case 0x308:
                    screen.blit(font->get_character(GD_UMLAUT_CHAR), xc-w, y);
                    break;
                case 0x30B:
                    screen.blit(font->get_character(GD_DOUBLE_ACUTE_CHAR), xc-w, y);
                    break;
            }
            continue;
        }
        /* color change "request", next character is a gdash color code */
        if (c==GD_COLOR_SETCOLOR) {
            i++;
            c = ucs[i];
            /* 64 was added in colors.hpp, now subtract it */
            c-=64;
            current_color = GdColor::from_gdash_index(c);
            font = widefont ? wide(current_color) : narrow(current_color);

            continue;
        }

        /* some unicode hack - substitutions */
        switch (c) {
            case 0x00AB:
                c='<';
                break; /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
            case 0x00BB:
                c='>';
                break; /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
            case 0x2010:
                c='-';
                break; /* hyphen */
            case 0x2011:
                c='-';
                break; /* non-breaking hyphen */
            case 0x2012:
                c='-';
                break; /* figure dash */
            case 0x2013:
                c='-';
                break; /* en dash */
            case 0x2014:
                c='-';
                break; /* em dash */
            case 0x2015:
                c='-';
                break; /* horizontal bar */
            case 0x2018:
                c='\'';
                break; /* left single quotation mark */
            case 0x2019:
                c='\'';
                break; /* right single quotation mark */
            case 0x201A:
                c=',';
                break; /* low single comma quotation mark */
            case 0x201B:
                c='\'';
                break; /* high-reversed-9 quotation mark */
            case 0x201C:
                c='\"';
                break; /* left double quotation mark */
            case 0x201D:
                c='\"';
                break; /* right double quotation mark */
            case 0x201E:
                c='\"';
                break; /* low double quotation mark */
            case 0x201F:
                c='\"';
                break; /* double reversed comma quotation mark */
            case 0x2032:
                c='\'';
                break; /* prime */
            case 0x2033:
                c='\"';
                break; /* double prime */
            case 0x2034:
                c='\"';
                break; /* triple prime */
            case 0x2035:
                c='\'';
                break; /* reversed prime */
            case 0x2036:
                c='\"';
                break; /* reversed double prime */
            case 0x2037:
                c='\"';
                break; /* reversed triple prime */
            case 0x2039:
                c='<';
                break; /* single left-pointing angle quotation mark */
            case 0x203A:
                c='>';
                break; /* single right-pointing angle quotation mark */
        }

        if (c=='\n') {
            /* if it is an enter */
            y+=h;
            xc=x;
        } else {
            gunichar i;

            if (c<RenderedFont::NUM_OF_CHARS)
                i=c;
            else
                i=GD_UNKNOWN_CHAR;

            screen.blit(font->get_character(i), xc, y);
            xc+=w;
        }
    }

    g_free(ucs);
    return xc;
}

void FontManager::clear() {
    for (container::iterator it=_narrow.begin(); it!=_narrow.end(); ++it)
        delete *it;
    _narrow.clear();
    for (container::iterator it=_wide.begin(); it!=_wide.end(); ++it)
        delete *it;
    _wide.clear();
}

int FontManager::get_pixmap_scale() const {
    return pixbuf_factory.get_pixmap_scale();
}

int FontManager::get_font_height() const {
    return font_size*pixbuf_factory.get_pixmap_scale();
}

int FontManager::get_line_height() const {
    return (font_size*1.4)*pixbuf_factory.get_pixmap_scale();
}

int FontManager::get_font_width_wide() const {
    return font_size*2*pixbuf_factory.get_pixmap_scale();
}


int FontManager::get_font_width_narrow() const {
    return font_size*pixbuf_factory.get_pixmap_scale();
}
