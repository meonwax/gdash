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

#include "gfx/pixmap.hpp"
#include "gfx/screen.hpp"
#include "sdl/sdlpixbuffactory.hpp"

#include <SDL_opengl.h>


class Pixbuf;
class GdColor;


class OGLPixmap: public Pixmap {
private:
    GLuint texid;
    int w, h;

    OGLPixmap& operator=(const OGLPixmap&);     // operator= not implemented
    OGLPixmap(const OGLPixmap& orig);           // copy ctor not implemented
    
    friend class SDLOGLScreen;

public:
    OGLPixmap(Pixbuf const& pb);
    ~OGLPixmap();

    virtual int get_width() const;
    virtual int get_height() const;
};


class SDLOGLScreen : public Screen {
private:
    SDLOGLScreen(const SDLOGLScreen &);               // not impl
    SDLOGLScreen& operator=(const SDLOGLScreen &);    // not impl
    int previous_configured_w, previous_configured_h;
    bool previous_configured_fullscreen;

public:
    SDLOGLScreen();
    ~SDLOGLScreen();
    virtual void configure_size();
    virtual void set_title(char const *title);
    virtual bool must_redraw_all_before_flip();
    virtual void flip();
    void reinit() {
        configure_size();
    }
    virtual void fill_rect(int x, int y, int w, int h, const GdColor& c);
    virtual void set_clip_rect(int x1, int y1, int w, int h);
    virtual void remove_clip_rect();
    virtual void blit_full(Pixmap const &src, int dx, int dy, int x, int y, int w, int h) const;
    virtual void draw_particle_set(int dx, int dy, ParticleSet const &ps);
};


class SDLOGLPixbufFactory: public SDLPixbufFactory {
public:
    virtual Pixmap *create_pixmap_from_pixbuf(Pixbuf const &pb, bool format_alpha) const;
};
