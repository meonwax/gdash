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

#include "config.h"

#include <glib/gi18n.h>

#include "framework/commands.hpp"
#include "framework/app.hpp"
#include "framework/gameactivity.hpp"
#include "framework/titlescreenactivity.hpp"
#include "fileops/loadfile.hpp"
#include "fileops/highscore.hpp"
#include "cave/caveset.hpp"
#include "cave/gamecontrol.hpp"
#include "misc/util.hpp"
#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "settings.hpp"

/// @todo MOVE THESE TO THE SETTINGS CPP
std::string gd_last_folder;


Command::~Command() {
}


NewGameCommand::NewGameCommand(App *app, int cavenum, int levelnum)
    : Command1Param<std::string>(app),
      username(p1),
      cavenum(cavenum),
      levelnum(levelnum) {
}


void NewGameCommand::execute() {
    gd_username = username;
    GameControl *game = GameControl::new_normal(app->caveset, username.c_str(), cavenum, levelnum);
    app->push_activity(new GameActivity(app, game));
}


void PopActivityCommand::execute() {
    app->pop_activity();
}


void PopAllActivitiesCommand::execute() {
    app->pop_all_activities();
}


PushActivityCommand::PushActivityCommand(App *app, Activity *activity_to_push)
    :
    Command(app),
    activity_to_push(activity_to_push) {
}


PushActivityCommand::~PushActivityCommand() {
    /* if it did not get pushed, delete */
    delete activity_to_push;
}


void PushActivityCommand::execute() {
    app->push_activity(activity_to_push);
    /* now the app handles it, do not delete */
    activity_to_push = NULL;
}


void RestartWithTitleScreenCommand::execute() {
    app->pop_all_activities();
    app->push_activity(new TitleScreenActivity(app));
}


void ShowHighScoreCommand::execute() {
    std::string text;

    // TRANSLATORS: showing highscore, categories
    text += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Caveset") % GD_COLOR_INDEX_YELLOW % app->caveset->name;

    for (int i = -1; i < (int) app->caveset->caves.size(); ++i) {
        /* is this one a cave? if so, store its pointer. */
        CaveStored *cave = (i == -1) ? NULL : &app->caveset->cave(i);
        /* select the highscore table; for the caveset or for the cave */
        HighScoreTable *scores = (cave == NULL) ? &app->caveset->highscore : &cave->highscore;

        if (scores->size() > 0) {
            /* if this is a cave, add its name */
            // TRANSLATORS: showing highscore, categories
            if (cave != NULL)
                text += SPrintf("\n%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Cave") % GD_COLOR_INDEX_YELLOW % cave->name;
            for (int n = 0; n < (int) scores->size(); n++) {
                bool highlight = cave == highlight_cave && n == highlight_line;
                text += SPrintf("%c%2d: %6d\t%s\n") % (highlight ? GD_COLOR_INDEX_RED : GD_COLOR_INDEX_LIGHTBLUE) % (n + 1) % (*scores)[n].score % (*scores)[n].name;
            }
        }
    }

    // TRANSLATORS: should be 40 chars max. Title of the highscore window/activity. (That's why F is capitalized in English)
    app->show_text_and_do_command(_("Hall of Fame"), text);
}


void ShowStatisticsCommand::execute() {
    bool has_levels = app->caveset->has_levels();
    
    /* count number of properties, and make a format string as well */
    unsigned numprops = 0;
    std::string statformat = has_levels ? " %d   " : "";  // will be 5 chars
    // TRANSLATORS: Stands for level. Should be exactly 5 chars, pad with spaces at the end if necessary
    std::string titleformat = has_levels ? _("Lev. ") : "";    // 5 chars
    while (CaveStored::cave_statistics_data[numprops].identifier != NULL) {
        titleformat += has_levels ? "%6s" : "%7s";
        statformat += has_levels ? "%6d" : "%7d";
        numprops++;
    }
    titleformat += "\n";
    statformat += "\n";
    /* create title line */
    SPrintf titleline(titleformat);
    for (unsigned j = 0; j < numprops; ++j) {
        titleline % g_dpgettext2(NULL, "Statistics", CaveStored::cave_statistics_data[j].name);
    }

    /* make table */
    std::string text;
    for (unsigned i = 0; i < app->caveset->caves.size(); ++i) {
        CaveStored &cave = app->caveset->cave(i);
        if (i != 0)
            text += "\n";
        text += SPrintf("%c%s: %c%s%c\n") % GD_COLOR_INDEX_WHITE % _("Cave") % GD_COLOR_INDEX_YELLOW % cave.name % GD_COLOR_INDEX_LIGHTBLUE;
        text += titleline;
        
        /* for each level */
        for (unsigned l = 0; l < (has_levels ? 5 : 1); ++l) {
            SPrintf p(statformat);
            if (has_levels)
                p % (l+1);
            for (unsigned j = 0; j < numprops; ++j) {
                PropertyDescription const & prop_desc = CaveStored::cave_statistics_data[j];
                g_assert(prop_desc.type == GD_TYPE_INT_LEVELS);
                p % cave.get<GdIntLevels>(prop_desc.prop)[l];
            }
            text += p;
        }
    }
    // TRANSLATORS: should be 40 chars max. Title of the caveset info window/activity. This is a window title, so it is capitalized in English.
    app->show_text_and_do_command(_("Caveset Playing Statistics"), text);
}


std::string caveinfo_text(CaveStored const &cave) {
    std::string s;

    s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Cave") % GD_COLOR_INDEX_YELLOW % cave.name;
    if (cave.author != "")
        s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Author") % GD_COLOR_INDEX_LIGHTBLUE % cave.author;
    if (cave.date != "")
        s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Date") % GD_COLOR_INDEX_LIGHTBLUE % cave.date;
    if (cave.description != "")
        s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Description") % GD_COLOR_INDEX_LIGHTBLUE % cave.description;
    if (cave.story != "")
        s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Story") % GD_COLOR_INDEX_LIGHTBLUE % cave.story;
    if (cave.remark != "")
        s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Remark") % GD_COLOR_INDEX_LIGHTBLUE % cave.remark;

    return s;
}


void ShowCaveInfoCommand::execute() {
    std::string s;

    if (app->caveset->name != "")
        s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Caveset") % GD_COLOR_INDEX_YELLOW % app->caveset->name;
    if (app->caveset->author != "")
        s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Author") % GD_COLOR_INDEX_LIGHTBLUE % app->caveset->author;
    if (app->caveset->date != "")
        s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Date") % GD_COLOR_INDEX_LIGHTBLUE % app->caveset->date;
    if (app->caveset->description != "")
        s += SPrintf("%c%s\n") % GD_COLOR_INDEX_LIGHTBLUE % app->caveset->description;
    if (app->caveset->story != "")
        s += SPrintf("%c%s\n") % GD_COLOR_INDEX_LIGHTBLUE % app->caveset->story;
    if (app->caveset->remark != "")
        s += SPrintf("%c%s\n") % GD_COLOR_INDEX_LIGHTBLUE % app->caveset->remark;
    for (unsigned i = 0; i < app->caveset->caves.size(); ++i) {
        if (s != "")
            s += "\n";
        CaveStored &cave = app->caveset->cave(i);
        s += caveinfo_text(cave);
    }

    // TRANSLATORS: should be 40 chars max. Title of the caveset info window/activity. Title capitalization (I) in English.
    app->show_text_and_do_command(thiscaveonly != NULL ? _("Cave Information") : _("Caveset Information"), s);
}


void AskIfChangesDiscardedCommand::execute() {
    if (!app->caveset->edited)
        app->enqueue_command(command_if_ok);
    else {
        /* if not edited, ask the user what to do. */
        // TRANSLATORS: 35 characters max.
        app->ask_yesorno_and_do_command(_("Caveset changed. Discard changes?"), _("Yes"), _("No"), command_if_ok, SmartPtr<Command>());
    }
}


void OpenFileCommand::execute() {
    gd_last_folder = gd_tostring_free(g_path_get_dirname(filename.c_str()));
    save_highscore(*app->caveset, gd_user_config_dir);

    try {
        *app->caveset = load_caveset_from_file(filename.c_str());
        load_highscore(*app->caveset, gd_user_config_dir);
        /* start a new title screen */
        app->enqueue_command(new RestartWithTitleScreenCommand(app));
    } catch (std::exception &e) {
        // TRANSLATORS: title of the "file open fail" error message window.
        app->show_message(_("Caveset Load Error"), e.what());
    }
}


void SaveCavesetToFileCommand::execute() {
    gd_last_folder = gd_tostring_free(g_path_get_dirname(filename.c_str()));
    try {
        app->caveset->save_to_file(filename.c_str());
        app->caveset->edited = false;
    } catch (std::exception &e) {
        // TRANSLATORS: title of the "file save fail" error window.
        app->show_message(_("Cave Save Error"), e.what());
    }
}


void SaveFileCommand::execute() {
    if (app->caveset->filename != "") {
        /* if we know a filename, do the save immediately */
        SmartPtr<SaveCavesetToFileCommand> command = new SaveCavesetToFileCommand(app);
        command->set_param1(app->caveset->filename);
        app->enqueue_command(command);
    } else {
        /* if no filename remembered, rather start the save_as function, which asks for one. */
        app->enqueue_command(new SaveFileAsCommand(app));
    }
}


void SaveFileAsCommand::execute() {
    // TRANSLATORS: 40 characters max. Title of the "save caveset as new file" window.
    app->select_file_and_do_command(_("Save Caveset As New File"), gd_last_folder.c_str(), "*.bd", true, CPrintf("%s.bd") % app->caveset->name, new SaveCavesetToFileCommand(app));
}


class SelectFileToLoadCommand: public Command {
public:
    SelectFileToLoadCommand(App *app, std::string const &start_dir): Command(app), start_dir(start_dir) {}

private:
    std::string start_dir;
    virtual void execute() {
        char *filter = g_strjoinv(";", (char **) gd_caveset_extensions);
        // TRANSLATORS: 40 characters max. Title of the "open file" window.
        app->select_file_and_do_command(_("Select Caveset to Load"), start_dir.c_str(), filter, false, "", new OpenFileCommand(app));
        g_free(filter);
    }
};


void SelectFileToLoadIfDiscardableCommand::execute() {
    SmartPtr<Command> selectfilecommand = new SelectFileToLoadCommand(app, start_dir);
    /* if edited, ask if discard, then load. otherwise load immediately. */
    if (app->caveset->edited) {
        app->enqueue_command(new AskIfChangesDiscardedCommand(app, selectfilecommand));
    } else {
        app->enqueue_command(selectfilecommand);
    }
}


void ShowErrorsCommand::execute() {
    std::string text;

    Logger::Container const &errors = l.get_messages();
    for (Logger::Container::const_iterator it = errors.begin(); it != errors.end(); ++it) {
        text += it->message;
        text += "\n\n";
    }
    // TRANSLATORS: 40 characters max. Title of the window which shows the error messages. Title capitalization (C) in English.
    app->show_text_and_do_command(_("Error Console"), text);
    l.clear();
}
