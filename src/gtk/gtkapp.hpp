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

#include "framework/app.hpp"

class CaveSet;

class GTKApp: public App {
private:
    GThread *timer_thread;
    GtkWidget *toplevel, *drawing_area;
	gulong focus_handler, keypress_handler, keyrelease_handler, expose_handler;
    GtkActionGroup *actions_game;
    bool quit_thread;
    int timer_events;

public:
    GTKApp(GtkWidget *drawing_area, GtkActionGroup *actions_game);
    ~GTKApp();

    virtual void select_file_and_do_command(const char *title, const char *start_dir, const char *glob, bool for_save, const char *defaultname, SmartPtr<Command1Param<std::string> > command_when_successful);
    virtual void ask_yesorno_and_do_command(char const *question, const char *yes_answer, char const *no_answer, SmartPtr<Command> command_when_yes, SmartPtr<Command> command_when_no);
    virtual void show_text_and_do_command(char const *title_line, std::string const &text, SmartPtr<Command> command_after_exit = SmartPtr<Command>());
    virtual void show_about_info();
    virtual void input_text_and_do_command(char const *title_line, char const *default_text, SmartPtr<Command1Param<std::string> > command_when_successful);
    virtual void game_active(bool active);
    virtual void show_settings(Setting *settings);
    virtual void show_message(std::string const &primary, std::string const &secondary, SmartPtr<Command> command_after_exit);

    static gboolean timing_event_idle_func(gpointer data);
    static gpointer timing_thread(gpointer data);
};
