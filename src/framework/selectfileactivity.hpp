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

#ifndef _GD_SELECTFILEAPPLET
#define _GD_SELECTFILEAPPLET

#include <vector>
#include <string>

#include "framework/app.hpp"

template <typename T> class Command1Param;

class SelectFileActivity: public Activity {
public:
    SelectFileActivity(App *app, const char *title, const char *start_dir, const char *glob, bool for_save, const char *defaultname, SmartPtr<Command1Param<std::string> > command_when_successful);
    ~SelectFileActivity();
    
    virtual void keypress_event(KeyCode keycode, int gfxlib_keycode);
    virtual void redraw_event();
    virtual void pushed_event();
    
    void jump_to_directory(char const *jump_to);
    void file_selected(char const *filename);
    void file_selected_do_command();

private:
    SmartPtr<Command1Param<std::string> > command_when_successful;
    std::string title;
    bool for_save;
    int yd;
    unsigned names_per_page;
    char **globs;
    char *directory;
    char *directory_of_process;
    int sel;
    enum FileState {
        GD_NOT_YET,
        GD_YES,
        GD_JUMP,
        GD_ESCAPE,
        GD_QUIT,
        GD_NEW,
    };
    FileState filestate;
    std::vector<std::string> files;
    std::string defaultname;
    std::string start_dir;

    void read_dir();
    void process_enter();
};

#endif
