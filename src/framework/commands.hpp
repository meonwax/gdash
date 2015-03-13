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

#ifndef _GD_COMMANDS
#define _GD_COMMANDS

#include <string>

#include "misc/smartptr.hpp"

class Logger;

extern std::string gd_last_folder; // TO DELETE

class CaveStored;
class App;
class Activity;

/// @ingroup Framework
/**
 * Commands are little objects which are usually enqueued by an Activity
 * object in its parent App object to be executed after processing an
 * event.
 *
 * The Command class is abstract. The Command::execute() method is intended
 * to be overloaded by all descendants, as it is the method which should
 * perform the action.
 *
 * The memory management of Command objects is handler via the SmartPtr
 * class by the App.
 */
class Command {
public:
    /** Destructor. */
    virtual ~Command();

protected:
    /** Constructor. Command objects always know the App in which they work.
     * @param app The parent App. */
    Command(App *app): app(app) {}
    /** The parent App. */
    App *const app;

private:
    /** This method should perform the action which is the role of the Command. */
    virtual void execute() = 0;
    /** The App class if a friend of this class, as commands are supposed to be executed only their parent App. */
    friend class App;
};


/** A generic command with one parameter.
 * Usually this parameter will be assigned some value by an Activity,
 * and then given to the App to be executed. */
template <typename T>
class Command1Param: public Command {
protected:
    /** Ctor.
     * @param app The parent App. */
    Command1Param(App *app) : Command(app) {}
    /** The value to be remembered. */
    T p1;

public:
    /** Parametrize the Command.
     * @param p The value to copy into the command. */
    void set_param1(T const &p) {
        p1 = p;
    }
};


/** This Command will start a new GameActivity when executed.
 * But first, it should be parametrized by the name of the player,
 * which will be used by the game for highscores. */
class NewGameCommand: public Command1Param<std::string> {
public:
    /** Ctor.
     * @param app The parent app.
     * @param cavenum The index of the cave on which the game should start.
     * @param levelnum The level on which the game should start. */
    NewGameCommand(App *app, int cavenum, int levelnum);

private:
    virtual void execute();
    /** The name of the user. Set to p1 of the base class. */
    std::string &username;
    /** The cave and level number on which the game will start. */
    int cavenum, levelnum;
};


/** Pop the topmost activity in the parent App. */
class PopActivityCommand: public Command {
public:
    /** Ctor.
     * @param app The parent app. */
    PopActivityCommand(App *app): Command(app) {}

private:
    virtual void execute();
};


/** Pop all activities in the parent App.
 * After popping, the activities stack will be empty. However, there may be
 * some commands left to execute. This command can be used to restart
 * the activity flow of an App. */
class PopAllActivitiesCommand: public Command {
public:
    /** Ctor.
     * @param app The parent app. */
    PopAllActivitiesCommand(App *app): Command(app) {}
private:
    virtual void execute();
};


/** Push an activity to the App. */
class PushActivityCommand: public Command {
public:
    /** Ctor.
     * @param app The parent app.
     * @param activity_to_push The Activity to be pushed onto the stack. Will be deleted by the App. */
    PushActivityCommand(App *app, Activity *activity_to_push);
    /** Dtor.
     * If the Command is destructed before pushing the App, it will delete the Activity that did not
     * get pushed. */
    ~PushActivityCommand();

private:
    /** The activity to be pushed. */
    Activity *activity_to_push;
    virtual void execute();
};


/** Pop all activities, then create a new title screen activity. */
/** @todo Remove? Kinda trivial, why make a class for it. */
class RestartWithTitleScreenCommand: public Command {
public:
    /** Ctor.
     * @param app The parent app. */
    RestartWithTitleScreenCommand(App *app): Command(app) {}

private:
    virtual void execute();
};


/** Show the highscores in the caveset. */
/** @todo Maybe this should be a virtual function of the App like the file selector? */
class ShowHighScoreCommand: public Command {
public:
    /** Ctor.
     * @param app The parent App.
     * @param highlight_cave The cave to be highlighted.
     * @param highlight_line The line number to be highlighted in the highscore list. */
    ShowHighScoreCommand(App *app, CaveStored *highlight_cave, int highlight_line)
        :   Command(app),
            highlight_cave(highlight_cave),
            highlight_line(highlight_line) {}

private:
    CaveStored *highlight_cave;
    /** The line number to be highlighted in the highscore list. */
    int highlight_line;
    virtual void execute();
};


/** Show the info of caves in the CaveSet assigned to the App. */
class ShowCaveInfoCommand: public Command {
public:
    /** Ctor.
     * @param app The parent app. */
    ShowCaveInfoCommand(App *app): Command(app) {}

private:
    virtual void execute();
};


/** Do the Command given as parameter, if the caveset is not edited
 * or the user accepts discarding his edits. Usually the command
 * will come forwarded by some activity. For example, a SelectFileActivity
 * can be given an OpenFileCommand, which will be executed after
 * selecting the file. If the caveset was edited however, the application
 * first asks the user if the changes can be discarded, so the
 * OpenFileCommand (parametrized with the name of the file)
 * is not executed immediately, but given to an AskIfChangesDiscardedCommand
 * for possible execution. */
class AskIfChangesDiscardedCommand : public Command {
public:
    /** Ctor.
     * @param app The parent app.
     * @param command_if_ok The Command to be executed if the caveset is not edited or the edits can be discarded. */
    AskIfChangesDiscardedCommand(App *app, SmartPtr<Command> command_if_ok): Command(app), command_if_ok(command_if_ok) {}
private:
    /** Remember the command to be executed. */
    SmartPtr<Command> command_if_ok;
    virtual void execute();
};


/** Load a caveset, and then restart the App with a new title screen. It may also
 * show an error message instead, if the file loading was unsuccessful. */
class OpenFileCommand: public Command1Param<std::string> {
public:
    /** Ctor.
     * @param app The parent app, which also knows the CaveSet.
     * @param filename The file to be loaded. Can be given to the constructor, or omitted and set later via set_param1(). */
    OpenFileCommand(App *app, std::string const &filename = "")
        :   Command1Param<std::string>(app),
            filename(p1) {
        this->filename = filename;
    }

private:
    /** The name of the file to be loaded. The reference is pointed to
     * the inherited p1 string by the ctor. */
    std::string &filename;
    virtual void execute();
};


/** Save the file to the given filename (which is set by set_param1()).
 * It may also pop up an error message. */
class SaveCavesetToFileCommand: public Command1Param<std::string> {
public:
    /** Ctor.
     * @param app The parent app. */
    SaveCavesetToFileCommand(App *app)
        :   Command1Param<std::string>(app),
            filename(p1) {
    }

private:
    /** The filename. A reference of parameter1, pointed to it by the ctor. */
    std::string &filename;
    virtual void execute();
};


/** Save the CaveSet to the file, if its filename is known; otherwise, pop
 * up a file selection dialog for the user to select a filename to save to. */
class SaveFileCommand: public Command {
public:
    /** Ctor.
     * @param app The parent app, which also knows the caveset. */
    SaveFileCommand(App *app) : Command(app) { }

private:
    virtual void execute();
};


/** Pop up a file selector for the user to select a filename to save the caveset to;
 * if the selection is successful, save the file. */
class SaveFileAsCommand: public Command {
public:
    /** Ctor.
     * @param app The parent app, which also knows the caveset. */
    SaveFileAsCommand(App *app) : Command(app) { }

private:
    virtual void execute();
};


/** Select a file to be loaded and load it; but only if the currently loaded caveset is discardable. */
class SelectFileToLoadIfDiscardableCommand: public Command {
public:

    SelectFileToLoadIfDiscardableCommand(App *app, std::string const &start_dir) : Command(app), start_dir(start_dir) {}

private:
    std::string start_dir;
    virtual void execute();
};


/** Show the errors which are in the Logger. */
class ShowErrorsCommand: public Command {
public:
    /** Ctor.
     * @param app The parent app.
     * @param l The logger to read the error messages from. */
    ShowErrorsCommand(App *app, Logger &l): Command(app), l(l) {}

private:
    /** The logger is remembered to read the messages from it when executing the command. */
    Logger &l;
    virtual void execute();
};

#endif
