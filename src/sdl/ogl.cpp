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

#include <SDL_image.h>
#include <stdexcept>

#include "sdl/ogl.hpp"
#include "sdl/sdlpixbuf.hpp"
#include "settings.hpp"
#include "misc/printf.hpp"
#include "misc/logger.hpp"


class SDLNewOGLPixmap: public Pixmap {
protected:
    SDL_Surface *surface;

    SDLNewOGLPixmap(const SDLNewOGLPixmap &);               // copy ctor not implemented
    SDLNewOGLPixmap &operator=(const SDLNewOGLPixmap &);    // operator= not implemented

public:
    SDLNewOGLPixmap(SDL_Surface *surface_) : surface(surface_) {}
    ~SDLNewOGLPixmap() { SDL_FreeSurface(surface); }
    virtual int get_width() const { return surface->w; }
    virtual int get_height() const { return surface->h; }
};


/* apientryp comes from sdl, sets the calling convention for the function */
typedef GLuint (APIENTRYP MY_PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRYP MY_PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP MY_PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP MY_PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP MY_PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP MY_PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRYP MY_PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP MY_PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP MY_PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar* *string, const GLint *length);
typedef void (APIENTRYP MY_PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef GLint (APIENTRYP MY_PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP MY_PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRYP MY_PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);

static MY_PFNGLCREATEPROGRAMPROC my_glCreateProgram = 0;
static MY_PFNGLLINKPROGRAMPROC my_glLinkProgram = 0;
static MY_PFNGLUSEPROGRAMPROC my_glUseProgram = 0;
static MY_PFNGLDELETEPROGRAMPROC my_glDeleteProgram = 0;
static MY_PFNGLCREATESHADERPROC my_glCreateShader = 0;
static MY_PFNGLDELETESHADERPROC my_glDeleteShader = 0;
static MY_PFNGLSHADERSOURCEPROC my_glShaderSource = 0;
static MY_PFNGLCOMPILESHADERPROC my_glCompileShader = 0;
static MY_PFNGLATTACHSHADERPROC my_glAttachShader = 0;
static MY_PFNGLDETACHSHADERPROC my_glDetachShader = 0;
static MY_PFNGLGETUNIFORMLOCATIONPROC my_glGetUniformLocation = 0;
static MY_PFNGLUNIFORM1FPROC my_glUniform1f = 0;
static MY_PFNGLUNIFORM2FPROC my_glUniform2f = 0;


void * glGetProcAddress(char const *name) {
    void *ptr = SDL_GL_GetProcAddress(name);
    if (ptr)
        return ptr;
    /* try with ARB */
    ptr = SDL_GL_GetProcAddress(CPrintf("%sARB") % name);    
    return ptr;
}


SDLNewOGLScreen::SDLNewOGLScreen(PixbufFactory &pixbuf_factory)
 : SDLAbstractScreen(pixbuf_factory) {
    shader_support = false;
    timed_flips = false;
    oglscaling = 1;

    glprogram = 0;
    texture = 0;
}


void SDLNewOGLScreen::set_properties(int scaling_factor_, GdScalingType scaling_type_, bool pal_emulation_) {
    oglscaling = scaling_factor_;
    /* the other two are not used by this screen implementation */
}


Pixmap *SDLNewOGLScreen::create_pixmap_from_pixbuf(Pixbuf const &pb, bool keep_alpha) const {
    SDL_Surface *to_copy = static_cast<SDLPixbuf const &>(pb).get_surface();
    SDL_Surface *newsurface = SDL_CreateRGBSurface(keep_alpha ? SDL_SRCALPHA : 0, to_copy->w, to_copy->h, 32,
                              surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
    SDL_SetAlpha(to_copy, 0, SDL_ALPHA_OPAQUE);
    SDL_BlitSurface(to_copy, NULL, newsurface, NULL);
    return new SDLNewOGLPixmap(newsurface);
}


void SDLNewOGLScreen::set_title(char const *title) {
    SDL_WM_SetCaption(title, NULL);
}


bool SDLNewOGLScreen::has_timed_flips() const {
    return timed_flips;
}
 
 
void SDLNewOGLScreen::start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error) {
    SDLNewOGLScreen *dis = static_cast<SDLNewOGLScreen *>(user_data);
    dis->shadertext = "";
    
    for (unsigned i = 0; attribute_names[i] != NULL; ++i) {
        if (g_str_equal(attribute_names[i], "filter")) {
            if (g_str_equal(attribute_values[i], "nearest")) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
            if (g_str_equal(attribute_values[i], "linear")) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
        }
    }
}


void SDLNewOGLScreen::end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error) {
    SDLNewOGLScreen *dis = static_cast<SDLNewOGLScreen *>(user_data);
    
    if (g_str_equal(element_name, "vertex")) {
        int shd = my_glCreateShader(GL_VERTEX_SHADER);
        char const *source = dis->shadertext.c_str();
        my_glShaderSource(shd, 1, &source, 0);
        my_glCompileShader(shd);
        if (glGetError() != 0)
            throw std::runtime_error("vertex shader cannot be compiled");
        my_glAttachShader(dis->glprogram, shd);
        dis->shaders.push_back(shd);
    }

    if (g_str_equal(element_name, "fragment")) {
        int shd = my_glCreateShader(GL_FRAGMENT_SHADER);
        char const *source = dis->shadertext.c_str();
        my_glShaderSource(shd, 1, &source, 0);
        my_glCompileShader(shd);
        if (glGetError() != 0)
            throw std::runtime_error("fragment shader cannot be compiled");
        my_glAttachShader(dis->glprogram, shd);
        dis->shaders.push_back(shd);
    }

}


void SDLNewOGLScreen::text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error) {
    SDLNewOGLScreen *dis = static_cast<SDLNewOGLScreen *>(user_data);
    dis->shadertext += std::string(text, text+text_len);
}


void SDLNewOGLScreen::set_uniform_float(char const *name, GLfloat value) {
    if (glprogram == 0)
        return;
    GLint location = my_glGetUniformLocation(glprogram, name);
    if (location != -1)  /* if such variable exists */
        my_glUniform1f(location, value);
}


void SDLNewOGLScreen::set_uniform_2float(char const *name, GLfloat value1, GLfloat value2) {
    if (glprogram == 0)
        return;
    GLint location = my_glGetUniformLocation(glprogram, name);
    if (location != -1)  /* if such variable exists */
        my_glUniform2f(location, value1, value2);
}


void SDLNewOGLScreen::configure_size() {
    uninit();

    /* init screen */
    SDL_InitSubSystem(SDL_INIT_VIDEO);
    /* for some reason, keyboard settings must be done here */
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    SDL_EnableUNICODE(1);
    /* icon */
    SDL_RWops *rwop = SDL_RWFromConstMem(Screen::gdash_icon_32_png, Screen::gdash_icon_32_size);
    SDL_Surface *icon = IMG_Load_RW(rwop, 1);  // 1 = automatically closes rwop
    SDL_WM_SetIcon(icon, NULL);
    SDL_FreeSurface(icon);

    /* create buffer */
    surface = SDL_CreateRGBSurface(SDL_SRCALPHA, w, h, 32, Pixbuf::rmask, Pixbuf::gmask, Pixbuf::bmask, Pixbuf::amask);
    Uint32 col = SDL_MapRGBA(surface->format, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_FillRect(surface, NULL, col);

    /* create screen */
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);      // no need to have one
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);    // no need to have one
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    /* if doing fine scrolling, try to swap every frame. otherwise, every second frame. */
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, gd_fine_scroll ? 1 : 2);

    Uint32 flags = SDL_OPENGL;
    SDL_Surface *surface = SDL_SetVideoMode(w*oglscaling, h*oglscaling, 0, flags | (gd_fullscreen ? SDL_FULLSCREEN : 0));
    if (gd_fullscreen && !surface)
        surface = SDL_SetVideoMode(w*oglscaling, h*oglscaling, 0, flags);      // try the same, without fullscreen
    if (!surface)
        throw std::runtime_error("cannot initialize sdl video");
    /* do not show mouse cursor */
    SDL_ShowCursor(SDL_DISABLE);
    /* warp mouse pointer so cursor cannot be seen, if the above call did nothing for some reason */
    SDL_WarpMouse(w - 1, h - 1);

    {
        /* report parameters got. */
        int red, green, blue, double_buffer, swap_control;
        SDL_GL_GetAttribute(SDL_GL_RED_SIZE,     &red);
        SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE,   &green);
        SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE,    &blue);
        SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &double_buffer);
        SDL_GL_GetAttribute(SDL_GL_SWAP_CONTROL, &swap_control);
        gd_debug(CPrintf("red:%d green:%d blue:%d double_buffer:%d swap_control:%d")
                 % red % green % blue % double_buffer % swap_control);

        timed_flips = double_buffer != 0 && swap_control != 0;
    }

    /* opengl mode setting */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glDisable(GL_POLYGON_SMOOTH);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    /* opengl view initialization */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w * oglscaling, h * oglscaling);
    glOrtho(0.0, w * oglscaling, h * oglscaling, 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    /* set up textures & stuff; some of these may be used when
     * we are using the fixed pipeline, not a shader */
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    /* configure shaders */
    my_glCreateProgram = (MY_PFNGLCREATEPROGRAMPROC) glGetProcAddress("glCreateProgram");
    my_glUseProgram = (MY_PFNGLUSEPROGRAMPROC) glGetProcAddress("glUseProgram");
    my_glDeleteProgram = (MY_PFNGLDELETEPROGRAMPROC) glGetProcAddress("glDeleteProgram");
    my_glCreateShader = (MY_PFNGLCREATESHADERPROC) glGetProcAddress("glCreateShader");
    my_glDeleteShader = (MY_PFNGLDELETESHADERPROC) glGetProcAddress("glDeleteShader");
    my_glShaderSource = (MY_PFNGLSHADERSOURCEPROC) glGetProcAddress("glShaderSource");
    my_glCompileShader = (MY_PFNGLCOMPILESHADERPROC) glGetProcAddress("glCompileShader");
    my_glAttachShader = (MY_PFNGLATTACHSHADERPROC) glGetProcAddress("glAttachShader");
    my_glDetachShader = (MY_PFNGLDETACHSHADERPROC) glGetProcAddress("glDetachShader");
    my_glLinkProgram = (MY_PFNGLLINKPROGRAMPROC) glGetProcAddress("glLinkProgram");
    my_glGetUniformLocation = (MY_PFNGLGETUNIFORMLOCATIONPROC) glGetProcAddress("glGetUniformLocation");
    my_glUniform1f = (MY_PFNGLUNIFORM1FPROC) glGetProcAddress("glUniform1f");
    my_glUniform2f = (MY_PFNGLUNIFORM2FPROC) glGetProcAddress("glUniform2f");

    shader_support = my_glCreateProgram && my_glUseProgram && my_glCreateShader
        && my_glDeleteShader && my_glShaderSource && my_glCompileShader && my_glAttachShader
        && my_glDetachShader && my_glLinkProgram && my_glGetUniformLocation
        && my_glUniform1f && my_glUniform2f;

    glprogram = 0;
    if (shader_support && gd_shader != "") {
        try {
            gd_debug("have shader support, loading shader");
            glprogram = my_glCreateProgram();

            /* load file */
            gchar *programtext = NULL;
            gsize length;
            if (!g_file_get_contents(gd_shader.c_str(), &programtext, &length, NULL)) {
                gd_shader = "";
                throw std::runtime_error("cannot load shader file");
            }
            /* parse file */
            GMarkupParser parser = {
                start_element,
                end_element,
                text,
                NULL /* passthrough */,
                NULL /* error */
            };
            GMarkupParseContext * parsecontext = g_markup_parse_context_new(&parser, G_MARKUP_TREAT_CDATA_AS_TEXT, this, NULL);
            bool success = g_markup_parse_context_parse(parsecontext, programtext, length, NULL);
            g_markup_parse_context_free(parsecontext);
            g_free(programtext);
            
            if (!success)
                throw std::runtime_error("cannot parse shader file markup");

            my_glLinkProgram(glprogram);
            if (glGetError() != 0) {
                throw std::runtime_error("shader program cannot be linked");
            }
            my_glUseProgram(glprogram);
            if (glGetError() != 0)
                throw std::runtime_error("shader program cannot be used");

            /* now configure the shader with some sizes and coordinates */
            set_uniform_2float("rubyInputSize", w, h);
            set_uniform_2float("rubyTextureSize", w, h);
            set_uniform_2float("rubyOutputSize", w*oglscaling, h*oglscaling);
        } catch (std::exception const & e) {
            gd_warning(e.what());
        }
    }
}


void SDLNewOGLScreen::uninit() {
    if (texture != 0)
        glDeleteTextures(1, &texture);
    texture = 0;
    while (!shaders.empty()) {
        my_glDeleteShader(shaders.back());
        shaders.pop_back();
    }
    if (glprogram != 0)
        my_glDeleteProgram(glprogram);
    glprogram = 0;
    if (surface != NULL)
        SDL_FreeSurface(surface);
    surface = NULL;
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


void SDLNewOGLScreen::flip() {
    glClear(GL_COLOR_BUFFER_BIT);

    /* copy the surface to the video card as the texture (one and only texture we use) */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    /* seed the rng */
    if (glprogram != 0) {
        set_uniform_float("randomSeed", g_random_double());
        set_uniform_float("RADIAL_DISTORTION", shader_pal_radial_distortion / 100.0);
        set_uniform_float("CHROMA_TO_LUMA_STRENGTH", shader_pal_chroma_to_luma_strength / 100.0);
        set_uniform_float("LUMA_TO_CHROMA_STRENGTH", shader_pal_luma_to_chroma_strength / 100.0);
        set_uniform_float("SCANLINE_SHADE_LUMA", shader_pal_scanline_shade_luma / 100.0);
        set_uniform_float("PHOSPHOR_SHADE", shader_pal_phosphor_shade / 100.0);
        set_uniform_float("RANDOM_SCANLINE_DISPLACE", shader_pal_random_scanline_displace / 100.0);
        set_uniform_float("RANDOM_Y", shader_pal_random_y / 100.0);
        set_uniform_float("RANDOM_UV", shader_pal_random_uv / 100.0);
        set_uniform_float("LUMA_X_BLUR", shader_pal_luma_x_blur / 100.0);
        set_uniform_float("CHROMA_X_BLUR", shader_pal_chroma_x_blur / 100.0);
        set_uniform_float("CHROMA_Y_BLUR", shader_pal_chroma_y_blur / 100.0);
    }

    /* and now draw a retangle */
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(w*oglscaling, 0);
    glTexCoord2f(0, 1); glVertex2f(0, h*oglscaling);
    glTexCoord2f(1, 1); glVertex2f(w*oglscaling, h*oglscaling);
    glEnd();

    SDL_GL_SwapBuffers();
}


SDLNewOGLScreen::~SDLNewOGLScreen() {
    uninit();
}

