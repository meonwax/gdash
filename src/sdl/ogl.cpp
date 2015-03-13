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

/* we define our own version of these function pointers, as the sdl headers
 * might not define it (for example, on the mac) depending on their version. */
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
typedef void (APIENTRYP MY_PFNGETSHADERINFOLOGPROC) (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
#define MY_GL_SHADING_LANGUAGE_VERSION       0x8B8C

/* the function pointers as got from opengl. all are prefixed with my_,
 * to avoid collision with global function names (would cause problem on the mac). */
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
static MY_PFNGETSHADERINFOLOGPROC my_glGetShaderInfoLog = 0;


void * my_glGetProcAddress(char const *name) {
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


SDLNewOGLScreen::~SDLNewOGLScreen() {
    uninit();
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
    return new SDLPixmap(newsurface);
}


void SDLNewOGLScreen::set_title(char const *title) {
    SDL_WM_SetCaption(title, NULL);
}


bool SDLNewOGLScreen::has_timed_flips() const {
    return timed_flips;
}
 
 
void SDLNewOGLScreen::set_texture_bilinear(bool bilinear) {
    if (bilinear) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

 
void SDLNewOGLScreen::start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error) {
    SDLNewOGLScreen *dis = static_cast<SDLNewOGLScreen *>(user_data);
    dis->shadertext = "";
    
    for (unsigned i = 0; attribute_names[i] != NULL; ++i) {
        if (g_str_equal(attribute_names[i], "filter")) {
            if (g_str_equal(attribute_values[i], "nearest"))
                dis->set_texture_bilinear(false);
            if (g_str_equal(attribute_values[i], "linear"))
                dis->set_texture_bilinear(true);
        }
    }
}


/**
 * Checks if there was an error when compiling the shader.
 * Logs the error message with gd_debug() if an error occured.
 * @param shd The shader object. */
static void log_shader_log(GLuint shd) {
    /* if we have the getinfo proc, try to retrieve info about compiling */
    if (my_glGetShaderInfoLog) {
        GLchar buf[8192];
        GLsizei length;
        my_glGetShaderInfoLog(shd, sizeof(buf)/sizeof(buf[0]), &length, buf);
        /* maybe :D */
        if (sizeof(GLchar) == sizeof(char)) {
            if (!g_str_equal(buf, ""))
                gd_warning((char *) buf);
        }
    }
}


void SDLNewOGLScreen::end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error) {
    SDLNewOGLScreen *dis = static_cast<SDLNewOGLScreen *>(user_data);
    
    if (g_str_equal(element_name, "vertex")) {
        GLuint shd = my_glCreateShader(GL_VERTEX_SHADER);
        char const *source = dis->shadertext.c_str();
        my_glShaderSource(shd, 1, &source, 0);
        my_glCompileShader(shd);
        log_shader_log(shd);
        if (glGetError() != 0)
            throw std::runtime_error("vertex shader cannot be compiled");
        my_glAttachShader(dis->glprogram, shd);
        dis->shaders.push_back(shd);
    }

    if (g_str_equal(element_name, "fragment")) {
        GLuint shd = my_glCreateShader(GL_FRAGMENT_SHADER);
        char const *source = dis->shadertext.c_str();
        my_glShaderSource(shd, 1, &source, 0);
        my_glCompileShader(shd);
        /* if we have the getinfo proc, try to retrieve info about compiling */
        log_shader_log(shd);
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
    set_texture_bilinear(false);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    /* configure shaders */
    my_glCreateProgram = (MY_PFNGLCREATEPROGRAMPROC) my_glGetProcAddress("glCreateProgram");
    my_glUseProgram = (MY_PFNGLUSEPROGRAMPROC) my_glGetProcAddress("glUseProgram");
    my_glDeleteProgram = (MY_PFNGLDELETEPROGRAMPROC) my_glGetProcAddress("glDeleteProgram");
    my_glCreateShader = (MY_PFNGLCREATESHADERPROC) my_glGetProcAddress("glCreateShader");
    my_glDeleteShader = (MY_PFNGLDELETESHADERPROC) my_glGetProcAddress("glDeleteShader");
    my_glShaderSource = (MY_PFNGLSHADERSOURCEPROC) my_glGetProcAddress("glShaderSource");
    my_glCompileShader = (MY_PFNGLCOMPILESHADERPROC) my_glGetProcAddress("glCompileShader");
    my_glAttachShader = (MY_PFNGLATTACHSHADERPROC) my_glGetProcAddress("glAttachShader");
    my_glDetachShader = (MY_PFNGLDETACHSHADERPROC) my_glGetProcAddress("glDetachShader");
    my_glLinkProgram = (MY_PFNGLLINKPROGRAMPROC) my_glGetProcAddress("glLinkProgram");
    my_glGetUniformLocation = (MY_PFNGLGETUNIFORMLOCATIONPROC) my_glGetProcAddress("glGetUniformLocation");
    my_glUniform1f = (MY_PFNGLUNIFORM1FPROC) my_glGetProcAddress("glUniform1f");
    my_glUniform2f = (MY_PFNGLUNIFORM2FPROC) my_glGetProcAddress("glUniform2f");
    /* this function is not really important, no problem if it is null, so do not test below */
    my_glGetShaderInfoLog = (MY_PFNGETSHADERINFOLOGPROC) my_glGetProcAddress("glGetShaderInfoLog");

    shader_support = my_glCreateProgram && my_glUseProgram && my_glCreateShader
        && my_glDeleteShader && my_glShaderSource && my_glCompileShader && my_glAttachShader
        && my_glDetachShader && my_glLinkProgram && my_glGetUniformLocation
        && my_glUniform1f && my_glUniform2f;

    glprogram = 0;
    if (shader_support) {
        gd_debug("have shader support");
        const GLubyte *glsl_version = glGetString(MY_GL_SHADING_LANGUAGE_VERSION);
        if (glsl_version) {
            gd_debug(CPrintf("shader language version %s") % (char*) glsl_version);
        }
    }
    if (shader_support && gd_shader != "") {
        try {
            gd_debug(CPrintf("loading shader %s") % gd_shader);
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
            /* configure the program with sizes */
            set_uniform_2float("rubyInputSize", w, h);
            set_uniform_2float("rubyTextureSize", w, h);
            set_uniform_2float("rubyOutputSize", w*oglscaling, h*oglscaling);
        } catch (std::exception const & e) {
            set_texture_bilinear(false);
            gd_warning(e.what());
        }
    }
}


/**
 * Set the value of an uniform float in the shader program.
 * @param name The name of the variable.
 * @param value The new value of the variable.
 */
void SDLNewOGLScreen::set_uniform_float(char const *name, GLfloat value) {
    if (glprogram == 0)
        return;
    GLint location = my_glGetUniformLocation(glprogram, name);
    if (location != -1)  /* if such variable exists */
        my_glUniform1f(location, value);
}


/**
 * Set the value of an uniform vec2 in the shader program.
 * @param name The name of the variable.
 * @param value The new value of the variable.
 */
void SDLNewOGLScreen::set_uniform_2float(char const *name, GLfloat value1, GLfloat value2) {
    if (glprogram == 0)
        return;
    GLint location = my_glGetUniformLocation(glprogram, name);
    if (location != -1)  /* if such variable exists */
        my_glUniform2f(location, value1, value2);
}


void SDLNewOGLScreen::uninit() {
    if (texture != 0)
        glDeleteTextures(1, &texture);
    texture = 0;
    /* delete shaders and program */
    while (!shaders.empty()) {
        my_glDeleteShader(shaders.back());
        shaders.pop_back();
    }
    if (glprogram != 0)
        my_glDeleteProgram(glprogram);
    glprogram = 0;
    /* delete sdl stuff */
    if (surface != NULL)
        SDL_FreeSurface(surface);
    surface = NULL;
    if (SDL_WasInit(SDL_INIT_VIDEO))
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


void SDLNewOGLScreen::flip() {
    glClear(GL_COLOR_BUFFER_BIT);

    /* copy the surface to the video card as the texture (one and only texture we use) */
    /* here the texture format on the video card is rgba. it could be rgb, but our internal
     * sdl back buffer is rgba, and if they are not the same format (rgb<->rgba), a swizzle
     * copy must occur inside the opengl driver. and that would cost a lot of cpu.
     * the sdl back buffer must be rgba, as the pixmaps drawn are also rgba (they have transparency
     * info). if the back buffer were rgb and the pixmaps rgba, the sdl blit would be slow.
     * so better make everything rgba. */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    /* seed the rng */
    if (glprogram != 0) {
        /* now configure the shader with some sizes and coordinates */
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


