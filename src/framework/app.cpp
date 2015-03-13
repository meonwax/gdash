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

/**
 * @defgroup Framework
 *
 * This group is the documentation of the Activity the App and
 * the Command class, which together control the graphical presentation
 * of everything in GDash (except the editor) to the user.
 *
 * An Activity object is a screen on which the user can do something,
 * for example play the game, select a file or read about a cave. Most
 * of the activities fill the whole screen. (Some of them are small
 * windows centered on the screen, but still they are modal.) An
 * activity receives events from the application, and can organize its
 * inner workings according to these events. There are keypress, timer,
 * redraw and other kinds of events.
 *
 * The activities are contained in an App object which organizes them into
 * a stack. Every time an event is received from the user or the timer,
 * the App object forwards the events (maybe after some preprocessing)
 * to the topmost activity on the stack. New activities are put on top
 * of the stack, and an activity can quit by requesting the App to
 * pop it off the top of the stack.
 *
 * The App object has the responsibility to connect the low level user
 * interface (screen, keyboard, graphics objects) to the activities.
 * The main program (which may be an SDL or a GTK implementation)
 * creates the App object, and translates the operating system and
 * graphics library specific events to function calls on the App
 * object. For example, when the user presses the Escape key, the
 * .keyboard_event() method of the App is to be called; and after that,
 * the App will forward this event to the topmost activity. If it is
 * the title screen, the program will presumably quit. If it is the
 * game, the cave will be restarted. It is always only the topmost
 * activity, which will receive the events from the app. Those which
 * are occluded by these do not receive any events, and so they are
 * stopped.
 *
 * In order to provide a means of communication between activities,
 * Command objects can be created. Each activity is allowed to push
 * as many command objects as it wants into the command queue of the
 * App from any of its event methods. The App will check the command
 * queue after calling any of the event methods of the topmost activity,
 * and execute all commands which are queued.
 *
 * The starting of the game from the title screen is a typical use of
 * these commands objects. When the user presses space, his name is asked
 * for. After that, he can start the game by typing his name and pressing
 * enter. This can be implemented by the TitleScreenActivity, the
 * InputTextActivity and the GameActivity classes.
 * - First, the TitleScreenActivity creates the InputTextActivity.
 * - The InputTextActivity does not know, what it should do with the
 *   text the user typed.
 * - However, it is also given a NewGameCommand, which is a command
 *   of one string parameter. When this command is executed, it creates
 *   a GameActivity and gives it to the App to push it onto the stack.
 * - If the user presses enter after typing his name, the
 *   InputTextActivity is popped from the stack, and the GameActivity
 *   is pushed.
 * - If the user presses escape, the command is not executed, so after
 *   popping the InputTextActivity nothing else happens. And so the
 *   TitleScreenActivity is on the top again.
 *
 * The command queue also serves the purpose of preventing object
 * creation/destruction ordering problems from happening. For example,
 * in the above scenario, the InputTextActivity object should not be
 * destructed before setting the player name parameter in the NewGameCommand
 * and executing the command itself. So activities are also pushed and
 * popped by means of using PopActivityCommand and PushActivityCommand
 * commands. The above interaction in detail:
 * - The user presses enter after typing his name.
 * - The NewGameCommand is parametrized with the name of the user.
 * - A PopActivityCommand is enqueued in the App.
 * - The NewGameCommand is also enqueued.
 * - The App checks the command queue.
 * - First, it executes the PopActivityCommand, so the InputTextActivity
 *   is destructed. (The name and starting cave parameters are not lost,
 *   because they are in the NewGameCommand.)
 * - The NewGameCommand is executed. It creates the GameActivity and
 *   pushes it onto the top of the activity stack.
 * - The game runs.
 *
 * The App class has some methods which can be called for common tasks like
 * selecting a file or asking a user a yes or no question. By default, these
 * methods push the corresponding activity. However, they can be overridden
 * by inheriting from the App, to provide the user with toolkit-specific
 * dialog boxes, for example GTK+ file open dialogs or message dialogs.
 * Activities should use these methods to perform these tasks instead of
 * hard-coding anything.
 */

#include "config.h"

#include <algorithm>
#include <glib/gi18n.h>

#include "framework/app.hpp"
#include "framework/commands.hpp"
#include "cave/helper/colors.hpp"
#include "gfx/pixbuf.hpp"
#include "gfx/fontmanager.hpp"
#include "gfx/pixbuffactory.hpp"
#include "gfx/screen.hpp"
#include "framework/commands.hpp"
#include "input/gameinputhandler.hpp"

#include "misc/about.hpp"
#include "sound/sound.hpp"
#include "settings.hpp"

#include "framework/selectfileactivity.hpp"
#include "framework/askyesnoactivity.hpp"
#include "framework/showtextactivity.hpp"
#include "framework/inputtextactivity.hpp"
#include "framework/volumeactivity.hpp"
#include "framework/settingsactivity.hpp"
#include "framework/messageactivity.hpp"

#include "background.cpp"

std::string help_strings_to_string(char const **strings) {
    std::string s;
    for (int n=0; strings[n] != NULL; n+=2)
        s += SPrintf("%c%s  %c%s\n") % GD_COLOR_INDEX_YELLOW % strings[n] % GD_COLOR_INDEX_LIGHTBLUE % strings[n+1];
    return s;
}


App::App() {
    pixbuf_factory = NULL;
    font_manager = NULL;
    screen = NULL;
    gameinput = NULL;
    caveset = NULL;
    background_image = NULL;
}


App::~App() {
    pop_all_activities();
    gd_music_stop();
    release_pixmaps();
    delete gameinput;
    delete font_manager;
    delete pixbuf_factory;
    delete screen;
}


void App::release_pixmaps() {
    delete background_image;
    background_image = NULL;
}


void App::set_no_activity_command(SmartPtr<Command> command) {
    no_activity_command = command;
}


void App::set_quit_event_command(SmartPtr<Command> command) {
    quit_event_command = command;
}


void App::set_request_restart_command(SmartPtr<Command> command) {
    request_restart_command = command;
}


void App::set_start_editor_command(SmartPtr<Command> command) {
    start_editor_command = command;
}


void App::clear_screen() {
    if (!background_image) {
        Pixbuf *pixbuf = pixbuf_factory->create_from_inline(sizeof(background), background);
        background_image = pixbuf_factory->create_pixmap_from_pixbuf(*pixbuf, false);
        delete pixbuf;
        screen->register_pixmap_storage(this);
    }
    screen->blit(*background_image, 0, 0);
}


void App::draw_window(int rx, int ry, int rw, int rh) {
    int const size = pixbuf_factory->get_pixmap_scale();
    screen->fill_rect(rx-2*size, ry-2*size, rw+4*size, rh+4*size, GdColor::from_rgb(64,64,64));
    screen->fill_rect(rx-1*size, ry-1*size, rw+2*size, rh+2*size, GdColor::from_rgb(128,128,128));
    screen->fill_rect(rx, ry, rw, rh, GdColor::from_rgb(0,0,0));
}


void App::select_file_and_do_command(const char *title, const char *start_dir, const char *glob, bool for_save, const char *defaultname, SmartPtr<Command1Param<std::string> > command_when_successful) {
    enqueue_command(new PushActivityCommand(this, new SelectFileActivity(this, title, start_dir, glob, for_save, defaultname, command_when_successful)));
}


void App::ask_yesorno_and_do_command(char const *question, const char *yes_answer, char const *no_answer, SmartPtr<Command> command_when_yes, SmartPtr<Command> command_when_no) {
    enqueue_command(new PushActivityCommand(this, new AskYesNoActivity(this, question, yes_answer, no_answer, command_when_yes, command_when_no)));
}


void App::show_text_and_do_command(char const *title_line, std::string const &text, SmartPtr<Command> command_after_exit) {
    enqueue_command(new PushActivityCommand(this, new ShowTextActivity(this, title_line, text, command_after_exit)));
}


void App::show_settings(Setting *settings) {
    enqueue_command(new PushActivityCommand(this, new SettingsActivity(this, settings)));
}

void App::show_message(std::string const &primary, std::string const &secondary, SmartPtr<Command> command_after_exit) {
    enqueue_command(new PushActivityCommand(this, new MessageActivity(this, primary, secondary, command_after_exit)));
}

void App::show_about_info() {
    // TRANSLATORS: about dialog box categories.
    struct String {
        char const *title;
        char const *text;
    } strings[] = { { "GDash", About::comments }, { _("Copyright"), About::copyright}, { _("Website"), About::website }, { NULL, NULL } };
    // TRANSLATORS: about dialog box categories.
    struct StringArray {
        char const *title;
        char const **texts;
    } stringarrays[] = {
        { _("Authors"), About::authors },
        { _("Artists"), About::artists },
        { _("Documenters"), About::documenters },
        { NULL, NULL }
    };
    std::string text;

    for (String *s = strings; s->title != NULL; ++s) {
        text += SPrintf("%c%s\n%c%s\n\n") % GD_COLOR_INDEX_YELLOW % s->title % GD_COLOR_INDEX_LIGHTBLUE % s->text;
    }
    for (StringArray *s = stringarrays; s->title != NULL; ++s)  {
        text += SPrintf("%c%s%c\n") % GD_COLOR_INDEX_YELLOW % s->title % GD_COLOR_INDEX_LIGHTBLUE;
        for (char const **t = s->texts; *t != NULL; ++t) {
            text += SPrintf("%c%c %c%s\n") % GD_COLOR_INDEX_YELLOW % GD_PLAYER_CHAR % GD_COLOR_INDEX_LIGHTBLUE % *t;
        }
        text += '\n';
    }
    // TRANSLATORS: about dialog box categories.
    text += SPrintf("%c%s\n%c%s") % GD_COLOR_INDEX_YELLOW % _("License") % GD_COLOR_INDEX_LIGHTBLUE % About::license;

    show_text_and_do_command(PACKAGE_NAME PACKAGE_VERSION, text);
}


void App::input_text_and_do_command(char const *title_line, char const *default_text, SmartPtr<Command1Param<std::string> > command_when_successful) {
    enqueue_command(new PushActivityCommand(this, new InputTextActivity(this, title_line, default_text, command_when_successful)));
}


/** select color for the next text drawing */
void App::set_color(const GdColor &color) {
    font_manager->set_color(color);
}


/** write something to the screen, with normal characters.
 * x=-1 -> center horizontally */
int App::blittext_n(int x, int y, const char *text) {
    return font_manager->blittext_n(*screen, x, y, text);
}


/** set status line (the last line in the screen) to text */
void App::status_line(const char *text) {
    font_manager->blittext_n(*screen, -1, screen->get_height()-font_manager->get_font_height(), GD_GDASH_GRAY2, text);
}


/** set title line (the first line in the screen) to text */
void App::title_line(const char *text) {
    font_manager->blittext_n(*screen, -1, 0, GD_GDASH_WHITE, text);
}


void App::timer_event(int ms_elapsed) {
    if (topmost_activity()) {
        topmost_activity()->timer_event(ms_elapsed);
    }
    process_commands();
}


void App::timer2_event() {
    if (topmost_activity()) {
        topmost_activity()->timer2_event();
        process_commands();
    }
}


void App::keypress_event(Activity::KeyCode keycode, int gfxlib_keycode) {
    /* send it to the gameinput object. */
    gameinput->keypress(gfxlib_keycode);

    /* and process it. */
    switch (keycode) {
        case F9:
            /* if we have sound, key f9 will push a volume activity.
             * that activity will receive the keypresses. */
#ifdef HAVE_SDL
            if (dynamic_cast<VolumeActivity *>(topmost_activity()) == NULL) {
                enqueue_command(new PushActivityCommand(this, new VolumeActivity(this)));
            }
#endif // ifdef HAVE_SDL
            break;
        case F10:
            /* f10 is reserved for the menu in the gtk version. do not pass to the activity,
             * if we may get this somehow. */
            break;
        case F11:
            /* fullscreen switch. do not pass to the app. */
            break;
        default:
            /* other key. pass to the activity. */
            if (topmost_activity()) {
                topmost_activity()->keypress_event(keycode, gfxlib_keycode);
                process_commands();
            }
            break;
    }
}


void App::redraw_event() {
    if (topmost_activity()) {
        topmost_activity()->redraw_event();
        process_commands();
    }
}


void App::quit_event() {
    if (quit_event_command != NULL)
        quit_event_command->execute();
}


/* process pending commands */
void App::process_commands() {
    while (!command_queue.empty()) {
        SmartPtr<Command> command = command_queue.front();
        command_queue.pop();
        command->execute();
    }

    if (running_activities.empty() && no_activity_command != NULL) {
        no_activity_command->execute();
        no_activity_command.release();  /* process this one only once! so forget the object. */
    }
}


void App::request_restart() {
    request_restart_command->execute();
}


void App::start_editor() {
    start_editor_command->execute();
}


/* enqueue a command for executing after the event.
 * and empty command is allowed to be pushed, and has no effect. */
void App::enqueue_command(SmartPtr<Command> the_command) {
    if (the_command != NULL)
        command_queue.push(the_command);
}


void App::push_activity(Activity *the_activity) {
    /* the currently topmost activity will be hidden by the new one */
    if (topmost_activity()) {
        topmost_activity()->hidden_event();
    }
    running_activities.push(the_activity);
    the_activity->pushed_event();
    the_activity->shown_event();
    the_activity->redraw_event();
}


void App::pop_activity() {
    if (!running_activities.empty()) {
        Activity *popped_activity = running_activities.top();
        popped_activity->hidden_event();
        delete running_activities.top();
        running_activities.pop();
        /* send a redraw to the topmost one */
        if (!running_activities.empty()) {
            running_activities.top()->shown_event();
            running_activities.top()->redraw_event();
        }
    }
}


void App::pop_all_activities() {
    while (topmost_activity())
        pop_activity();
}


Activity *App::topmost_activity() {
    if (running_activities.empty())
        return NULL;
    return running_activities.top();
}
