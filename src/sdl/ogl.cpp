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

// title image, TO REMOVE
#include "gdash_icon_32.cpp"

#include <SDL.h>
#include <stdexcept>
#include <cmath>


#include "cave/helper/colors.hpp"
#include "cave/particle.hpp"
#include "gfx/pixbuf.hpp"
#include "sdl/sdlpixbuf.hpp"
#include "sdl/ogl.hpp"
#include "settings.hpp"

OGLPixmap::OGLPixmap(Pixbuf const& pb) {
    unsigned char *oneblock = NULL;
    /* bytes of rows should come after each other, a requirement of glTexImage2D */
    if (pb.get_width()*4 != pb.get_pitch()) {
        oneblock = new unsigned char[pb.get_width()*4 * pb.get_height()];
        for (int y = 0; y < pb.get_height(); ++y)
            memcpy(oneblock + pb.get_width()*4*y, pb.get_row(y), pb.get_width()*4);
    }
    this->w = pb.get_width();
    this->h = pb.get_height();
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, oneblock != NULL ? oneblock : pb.get_pixels());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (oneblock)
        delete[] oneblock;
}


OGLPixmap::~OGLPixmap() {
    glDeleteTextures(1, &texid);
}


int OGLPixmap::get_width() const {
    return w;
}


int OGLPixmap::get_height() const {
    return h;
}






SDLOGLScreen::SDLOGLScreen() {
}


SDLOGLScreen::~SDLOGLScreen() {
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


void SDLOGLScreen::configure_size() {
    /* close window, if already exists, to create a new one */
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    /* init screen */
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    /* for some reason, keyboard settings must be done here */
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    SDL_EnableUNICODE(1);
    /* icon */
    SDLPixbuf icon(sizeof(gdash_icon_32), gdash_icon_32);
    SDL_WM_SetIcon(icon.get_surface(), NULL);
    set_title("GDash");

    /* create screen */
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  5);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
    
    Uint32 flags = SDL_OPENGL;
    SDL_Surface *surface = SDL_SetVideoMode(w, h, 0, flags | (gd_fullscreen?SDL_FULLSCREEN:0));
    if (gd_fullscreen && !surface)
        surface=SDL_SetVideoMode(w, h, 0, flags);        // try the same, without fullscreen
    if (!surface)
        throw std::runtime_error("cannot initialize sdl video");
    /* do not show mouse cursor */
    SDL_ShowCursor(SDL_DISABLE);
    /* warp mouse pointer so cursor cannot be seen, if the above call did nothing for some reason */
    SDL_WarpMouse(w-1, h-1);

    /* opengl mode setting */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glEnable(GL_BLEND);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    /* opengl view initialization */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    glOrtho(0.0, w, h, 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void SDLOGLScreen::set_title(char const *title) {
    SDL_WM_SetCaption(title, NULL);
}


bool SDLOGLScreen::must_redraw_all_before_flip() {
    return false;
}


void SDLOGLScreen::flip() {
    SDL_GL_SwapBuffers();
}


void SDLOGLScreen::fill_rect(int x, int y, int w, int h, const GdColor& c) {
    glDisable(GL_TEXTURE_2D);
    glColor3ub(c.get_r(), c.get_g(), c.get_b());
    glBegin(GL_TRIANGLE_STRIP);
        glVertex2i(x, y);
        glVertex2i(x+w, y);
        glVertex2i(x, y+h);
        glVertex2i(x+w, y+h);
    glEnd();
}


void SDLOGLScreen::set_clip_rect(int x1, int y1, int w, int h) {
    GLdouble clip0[4] = { 0, 1, 0, -(double)y1 };
    GLdouble clip1[4] = { 0, 1, 0, (double)(y1+h) };
    GLdouble clip2[4] = { 1, 0, 0, -(double)x1 };
    GLdouble clip3[4] = { 1, 0, 0, (double)(x1+w) };
    glClipPlane(GL_CLIP_PLANE0, clip0);
    glClipPlane(GL_CLIP_PLANE1, clip1);
    glClipPlane(GL_CLIP_PLANE2, clip2);
    glClipPlane(GL_CLIP_PLANE3, clip3);
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);
}


void SDLOGLScreen::remove_clip_rect() {
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
}


void SDLOGLScreen::blit_full(Pixmap const &src, int dx, int dy, int x, int y, int w, int h) const {
    OGLPixmap const &oglpm = static_cast<OGLPixmap const&>(src);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, oglpm.texid);
    glPushMatrix();
    glTranslatef(dx, dy, 0);
    glScalef(w, h, 0);
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0);
        glVertex2f(0, 0);
        glTexCoord2f(1, 0);
        glVertex2f(1, 0);
        glTexCoord2f(0, 1);
        glVertex2f(0, 1);
        glTexCoord2f(1, 1);
        glVertex2f(1, 1);
    glEnd();
    glPopMatrix();
}


void SDLOGLScreen::draw_particle_set(int dx, int dy, ParticleSet const &ps) {
    Uint8 r = ps.color.get_r();
    Uint8 g = ps.color.get_g();
    Uint8 b = ps.color.get_b();
    Uint8 a = ps.life / 1000.0 * ps.opacity * 255;
    int size = ceil(ps.size), size1 = size+1;

    glDisable(GL_TEXTURE_2D);
    glColor4ub(r, g, b, a);
    glBegin(GL_QUADS);
    for (ParticleSet::const_iterator it = ps.begin(); it != ps.end(); ++it) {
        int fx = dx + it->px, fy = dy + it->py;
        glVertex2f(fx, fy-size);
        glVertex2f(fx+size1, fy);
        glVertex2f(fx, fy+size1);
        glVertex2f(fx-size, fy);
    }
    glEnd();
}






Pixmap *SDLOGLPixbufFactory::create_pixmap_from_pixbuf(Pixbuf const &pb, bool format_alpha) const {
    SDLPixbuf *scaled = static_cast<SDLPixbuf *>(create_scaled(pb));
    Pixmap *pm = new OGLPixmap(*scaled);
    delete scaled;
    return pm;
}
