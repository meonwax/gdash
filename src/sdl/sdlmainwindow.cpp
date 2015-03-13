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

#include "settings.hpp"
#include "framework/app.hpp"
#include "framework/commands.hpp"
#include "framework/gameactivity.hpp"
#include "framework/titlescreenactivity.hpp"
#include "framework/messageactivity.hpp"
#include "gfx/fontmanager.hpp"
#include "sdl/sdlpixbuffactory.hpp"
#include "sdl/sdlgameinputhandler.hpp"
#include "sdl/sdlscreen.hpp"

#include "sdl/sdlmainwindow.hpp"

// OPENGL
#include "sdl/ogl.hpp"



class SDLApp: public App {
public:
    SDLApp();
};

SDLApp::SDLApp() {
    //~ screen = new SDLScreen;
    //~ pixbuf_factory = new SDLPixbufFactory;
    // OPENGL
    screen = new SDLOGLScreen;
    pixbuf_factory = new SDLOGLPixbufFactory;
    pixbuf_factory->set_properties(GdScalingType(gd_cell_scale_game), gd_pal_emulation_game);
    font_manager = new FontManager(*pixbuf_factory, "");
    screen->register_pixmap_storage(font_manager);
    gameinput = new SDLGameInputHandler;
}


/* generate a user event */
static Uint32 timer_callback(Uint32 interval, void *param) {
    SDL_Event ev;

    ev.type=SDL_USEREVENT;
    SDL_PushEvent(&ev);

    return interval;
}


static Activity::KeyCode activity_keycode_from_sdl_key_event(SDL_KeyboardEvent const &ev) {
    switch (ev.keysym.sym) {
        case SDLK_UP:
            return App::Up;
        case SDLK_DOWN:
            return App::Down;
        case SDLK_LEFT:
            return App::Left;
        case SDLK_RIGHT:
            return App::Right;
        case SDLK_PAGEUP:
            return App::PageUp;
        case SDLK_PAGEDOWN:
            return App::PageDown;
        case SDLK_HOME:
            return App::Home;
        case SDLK_END:
            return App::End;
        case SDLK_F1:
            return App::F1;
        case SDLK_F2:
            return App::F2;
        case SDLK_F3:
            return App::F3;
        case SDLK_F4:
            return App::F4;
        case SDLK_F5:
            return App::F5;
        case SDLK_F6:
            return App::F6;
        case SDLK_F7:
            return App::F7;
        case SDLK_F8:
            return App::F8;
        case SDLK_F9:
            return App::F9;
        case SDLK_BACKSPACE:
            return App::BackSpace;
        case SDLK_RETURN:
            return App::Enter;
        case SDLK_TAB:
            return App::Tab;
        case SDLK_ESCAPE:
            return App::Escape;

        default:
            return ev.keysym.unicode;
    }
}


class SetNextActionCommandSDL : public Command {
public:
    SetNextActionCommandSDL(App *app, NextAction &na, NextAction to_what): Command(app), na(na), to_what(to_what) {}
private:
    virtual void execute() {
        na = to_what;
    }
    NextAction &na;
    NextAction to_what;
};


static void run_the_app(App &the_app, NextAction &na) {
    /* Running the APP */
    int const timer_ms = 20;
    SDL_InitSubSystem(SDL_INIT_TIMER);
    SDL_TimerID id = SDL_AddTimer(timer_ms, timer_callback, NULL);

    the_app.set_no_activity_command(new SetNextActionCommandSDL(&the_app, na, Quit));

    /* sdl_waitevent will wait until at least one event appears. */
    na = StartTitle;
    while (na == StartTitle && SDL_WaitEvent(NULL)) {
        /* and now we poll all the events, as there might be more than one. */
        bool had_timer_event1 = false;
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    the_app.quit_event();
                    break;
                case SDL_KEYDOWN:
                    if ((ev.key.keysym.sym == SDLK_F11)
                            || (ev.key.keysym.sym == SDLK_RETURN && (ev.key.keysym.mod & (KMOD_LALT | KMOD_RALT)))) {
                        gd_fullscreen = !gd_fullscreen;
                        the_app.screen->set_size(the_app.screen->get_width(), the_app.screen->get_height());
                        the_app.redraw_event();
                    }
                    else if ((ev.key.keysym.sym == SDLK_q) && (ev.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))) {
                        the_app.quit_event();
                    } else {
                        Activity::KeyCode keycode = activity_keycode_from_sdl_key_event(ev.key);
                        the_app.keypress_event(keycode, ev.key.keysym.sym);
                    }
                    break;
                case SDL_KEYUP:
                    the_app.gameinput->keyrelease(ev.key.keysym.sym);
                    break;
                case SDL_VIDEOEXPOSE:
                    the_app.redraw_event();
                    break;
                case SDL_USEREVENT:
                    had_timer_event1 = true;
                    break;
                case SDL_USEREVENT+1:
                    the_app.timer2_event();
                    break;
            } // switch ev.type
        } // while pollevent

        if (had_timer_event1)
            the_app.timer_event(timer_ms);

#ifdef HAVE_GTK
        // check if the g_main_loop has something to do. if it has,
        // it is usually gtk stuff - so let gtk live :D
        while (g_main_context_pending(NULL))
            g_main_context_iteration(NULL, FALSE);
#endif
    }

    SDL_RemoveTimer(id);
}

void gd_main_window_sdl_run(CaveSet *caveset, NextAction &na) {
    SDLApp the_app;

    /* normal application: title screen, menu etc */
    the_app.caveset = caveset;
    the_app.set_quit_event_command(new AskIfChangesDiscardedCommand(&the_app, new PopAllActivitiesCommand(&the_app)));
    the_app.set_request_restart_command(new SetNextActionCommandSDL(&the_app, na, Restart));
    the_app.set_start_editor_command(new SetNextActionCommandSDL(&the_app, na, StartEditor));

    the_app.push_activity(new TitleScreenActivity(&the_app));
    run_the_app(the_app, na);
}


void gd_main_window_sdl_run_a_game(GameControl *game) {
    SDLApp the_app;

    NextAction na = StartTitle;      // because the func below needs one to work with
    the_app.set_quit_event_command(new SetNextActionCommandSDL(&the_app, na, Quit));
    the_app.push_activity(new GameActivity(&the_app, game));
    run_the_app(the_app, na);
}
