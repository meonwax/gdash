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

#include <glib/gi18n.h>

#include "framework/commands.hpp"
#include "framework/app.hpp"
#include "framework/gameactivity.hpp"
#include "framework/titlescreenactivity.hpp"
#include "fileops/loadfile.hpp"
#include "cave/caveset.hpp"
#include "cave/gamecontrol.hpp"
#include "misc/util.hpp"
#include "misc/logger.hpp"
#include "settings.hpp"

// @todo MOVE THESE TO THE SETTINGS CPP
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
    GameControl *game=GameControl::new_normal(app->caveset, username.c_str(), cavenum, levelnum);
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
        CaveStored *cave = (i==-1) ? NULL : &app->caveset->cave(i);
        /* select the highscore table; for the caveset or for the cave */
        HighScoreTable *scores = (cave == NULL) ? &app->caveset->highscore : &cave->highscore;

        if (scores->size() > 0) {
            /* if this is a cave, add its name */
            // TRANSLATORS: showing highscore, categories
            if (cave != NULL)
                text += SPrintf("\n%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Cave") % GD_COLOR_INDEX_YELLOW % cave->name;
            for (int n=0; n < (int) scores->size(); n++) {
                bool highlight = cave == highlight_cave && n == highlight_line;
                text += SPrintf("%c%2d %6d %s\n") % (highlight ? GD_COLOR_INDEX_RED : GD_COLOR_INDEX_LIGHTBLUE) % (n+1) % (*scores)[n].score % (*scores)[n].name;
            }
        }
    }

    // TRANSLATORS: should be 40 chars max. Title of the highscore window/activity.
    app->show_text_and_do_command(_("Hall of Fame"), text);
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
    if (app->caveset->remark!="")
        s += SPrintf("%c%s\n") % GD_COLOR_INDEX_LIGHTBLUE % app->caveset->remark;
    for (unsigned i = 0; i < app->caveset->caves.size(); ++i) {
        CaveStored &cave = app->caveset->cave(i);
        s += SPrintf("\n%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Cave") % GD_COLOR_INDEX_YELLOW % cave.name;
        if (cave.author != "")
            s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Author") % GD_COLOR_INDEX_YELLOW % cave.author;
        if (cave.date != "")
            s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Date") % GD_COLOR_INDEX_YELLOW % cave.date;
        if (cave.description != "")
            s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Description") % GD_COLOR_INDEX_YELLOW % cave.description;
        if (cave.story != "")
            s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Story") % GD_COLOR_INDEX_LIGHTBLUE % cave.story;
        if (cave.remark != "")
            s += SPrintf("%c%s: %c%s\n") % GD_COLOR_INDEX_WHITE % _("Remark") % GD_COLOR_INDEX_LIGHTBLUE % cave.remark;
    }

    // TRANSLATORS: should be 40 chars max. Title of the caveset info window/activity.
    app->show_text_and_do_command(_("Caveset Information"), s);
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
    app->caveset->save_highscore(gd_user_config_dir);

    try {
        *app->caveset = create_from_file(filename.c_str());
        /* if this is a bd file, and we load highscores from our own config dir */
        if (g_str_has_suffix(filename.c_str(), ".bd") && !gd_use_bdcff_highscore)
            app->caveset->load_highscore(gd_user_config_dir);
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
    // TRANSLATORS: 40 characters max. Title of the window which shows the error messages.
    app->show_text_and_do_command(_("Error Console"), text);
    l.clear();
}
