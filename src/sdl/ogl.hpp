/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NEWOGL_HPP_INCLUDED
#define NEWOGL_HPP_INCLUDED

#include <SDL_opengl.h>

#include <glib.h>
#include <string>
#include <vector>
#include "sdl/sdlabstractscreen.hpp"

class SDLNewOGLScreen: public SDLAbstractScreen {
private:
    bool shader_support;
    bool timed_flips;
    double oglscaling;

    GLuint glprogram;
    std::vector<GLuint> shaders;
    GLuint texture;

    /// used when loading the xml
    std::string shadertext;
    static void start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
    static void end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);
    static void text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error);
    
    void set_uniform_float(char const *name, GLfloat value);
    void set_uniform_2float(char const *name, GLfloat value1, GLfloat value2);
    void set_texture_bilinear(bool bilinear);

public:
    SDLNewOGLScreen(PixbufFactory &pixbuf_factory);
    virtual void set_properties(int scaling_factor_, GdScalingType scaling_type_, bool pal_emulation_);
    virtual void set_title(char const *);
    virtual void configure_size();
    virtual void flip();
    virtual bool has_timed_flips() const;
    void uninit();
    ~SDLNewOGLScreen();
    virtual Pixmap *create_pixmap_from_pixbuf(Pixbuf const &pb, bool keep_alpha) const;
};

#endif
