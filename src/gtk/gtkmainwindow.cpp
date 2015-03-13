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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gtk/gtkmainwindow.hpp"

#include "framework/commands.hpp"
#include "framework/app.hpp"
#include "framework/settingsactivity.hpp"
#include "framework/replaymenuactivity.hpp"
#include "framework/gameactivity.hpp"
#include "framework/volumeactivity.hpp"
#include "gtk/gtkui.hpp"
#include "gtk/gtkapp.hpp"
#include "settings.hpp"
#include "gtk/help.hpp"
#include "cave/caveset.hpp"
#include "misc/logger.hpp"


class GdMainWindow {
public:
    GtkWidget *window;
    GTKApp *app;
    GtkActionGroup *actions_normal, *actions_game;
    GtkWidget *menubar;
    CaveSet *caveset;
    bool fullscreen;
    NextAction &na;

    static gboolean main_window_set_fullscreen_idle_func(gpointer data);
    void main_window_set_fullscreen();

    static gboolean delete_event(GtkWidget *widget, GdkEventAny *event, gpointer data);
    static void help_cb(GtkWidget *widget, gpointer data);
    static void volume_cb(GtkWidget *widget, gpointer data);
    static void preferences_cb(GtkWidget *widget, gpointer data);
    static void keyboard_preferences_cb(GtkWidget *widget, gpointer data);
    static void quit_cb(GtkWidget *widget, gpointer data);
    static void end_game_cb(GtkWidget *widget, gpointer data);
    static void random_colors_cb(GtkWidget *widget, gpointer data);
    static void take_snapshot_cb(GtkWidget *widget, gpointer data);
    static void revert_to_snapshot_cb(GtkWidget *widget, gpointer data);
    static void restart_level_cb(GtkWidget *widget, gpointer data);
    static void about_cb(GtkWidget *widget, gpointer data);
    static void open_caveset_cb(GtkWidget *widget, gpointer data);
    static void open_caveset_dir_cb(GtkWidget *widget, gpointer data);
    static void save_caveset_as_cb(GtkWidget *widget, gpointer data);
    static void save_caveset_cb(GtkWidget *widget, gpointer data);
    static void highscore_cb(GtkWidget *widget, gpointer data);
    static void show_errors_cb(GtkWidget *widget, gpointer data);
    static void cave_editor_cb(GtkWidget *widget, gpointer data);
    static void recent_chooser_activated_cb(GtkRecentChooser *chooser, gpointer data);
    static void toggle_fullscreen_cb (GtkWidget *widget, gpointer data);
    static void pause_game_cb(GtkWidget *widget, gpointer data);
    static void show_replays_cb(GtkWidget *widget, gpointer data);
    static void cave_info_cb(GtkWidget *widget, gpointer data);
    
    GdMainWindow(bool add_menu, NextAction &na);
    ~GdMainWindow();
};


/* Menu UI */
static GtkActionEntry action_entries_normal[]={
    {"PlayMenu", NULL, N_("Play")},
    {"FileMenu", NULL, N_("File")},
    {"HelpMenu", NULL, N_("Help")},
    {"Quit", GTK_STOCK_QUIT, NULL, "", NULL, G_CALLBACK(GdMainWindow::quit_cb)},
    {"Errors", GTK_STOCK_DIALOG_ERROR, N_("_Error console"), NULL, NULL, G_CALLBACK(GdMainWindow::show_errors_cb)},
    {"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK(GdMainWindow::about_cb)},
    {"Help", GTK_STOCK_HELP, NULL, "", NULL, G_CALLBACK(GdMainWindow::help_cb)},
    {"CaveInfo", GTK_STOCK_DIALOG_INFO, N_("Caveset _information"), NULL, N_("Show information about the game and its caves"), G_CALLBACK(GdMainWindow::cave_info_cb)},
    {"GamePreferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK(GdMainWindow::preferences_cb)},
    {"KeyboardPreferences", GD_ICON_KEYBOARD, N_("Keyboard settings"), NULL, N_("Set keyboard settings"), G_CALLBACK(GdMainWindow::keyboard_preferences_cb)},
#ifdef HAVE_SDL
    {"Volume", GTK_STOCK_PREFERENCES, N_("_Sound volume"), "F9", NULL, G_CALLBACK(GdMainWindow::volume_cb)},
#endif
    {"CaveEditor", GD_ICON_CAVE_EDITOR, N_("Cave _editor"), NULL, NULL, G_CALLBACK(GdMainWindow::cave_editor_cb)},
    {"OpenFile", GTK_STOCK_OPEN, NULL, "", NULL, G_CALLBACK(GdMainWindow::open_caveset_cb)},
    {"LoadRecent", GTK_STOCK_DIALOG_INFO, N_("Open _recent")},
    {"OpenCavesDir", GTK_STOCK_CDROM, N_("O_pen shipped"), NULL, NULL, G_CALLBACK(GdMainWindow::open_caveset_dir_cb)},
    {"SaveFile", GTK_STOCK_SAVE, NULL, "", NULL, G_CALLBACK(GdMainWindow::save_caveset_cb)},
    {"SaveAsFile", GTK_STOCK_SAVE_AS, NULL, NULL, NULL, G_CALLBACK(GdMainWindow::save_caveset_as_cb)},
    {"HighScore", GD_ICON_AWARD, N_("Hi_ghscores"), NULL, NULL, G_CALLBACK(GdMainWindow::highscore_cb)},
    {"ShowReplays", GD_ICON_REPLAY, N_("Show _replays"), NULL, N_("List replays which are recorded for caves in this caveset"), G_CALLBACK(GdMainWindow::show_replays_cb)},
    {"FullScreen", GTK_STOCK_FULLSCREEN, NULL, "F11", N_("Fullscreen mode"), G_CALLBACK(GdMainWindow::toggle_fullscreen_cb)},
};

static GtkActionEntry action_entries_game[]={
    {"TakeSnapshot", GD_ICON_SNAPSHOT, N_("_Take snapshot"), NULL, NULL, G_CALLBACK(GdMainWindow::take_snapshot_cb)},
    {"RevertToSnapshot", GTK_STOCK_UNDO, N_("_Revert to snapshot"), NULL, NULL, G_CALLBACK(GdMainWindow::revert_to_snapshot_cb)},
    {"RandomColors", GTK_STOCK_SELECT_COLOR, N_("Random _colors"), NULL, NULL, G_CALLBACK(GdMainWindow::random_colors_cb)},
    {"RestartLevel", GD_ICON_RESTART_LEVEL, N_("Re_start level"), NULL, N_("Restart current level"), G_CALLBACK(GdMainWindow::restart_level_cb)},
    {"PauseGame", GTK_STOCK_MEDIA_PAUSE, N_("_Pause game"), NULL, N_("Restart current level"), G_CALLBACK(GdMainWindow::pause_game_cb)},
    {"EndGame", GTK_STOCK_STOP, N_("_End game"), NULL, N_("End current game"), G_CALLBACK(GdMainWindow::end_game_cb)},
};


static const char *ui_info =
    "<ui>"
    "<menubar name='MenuBar'>"
    "<menu action='FileMenu'>"
    "<separator/>"
    "<menuitem action='OpenFile'/>"
    "<menuitem action='LoadRecent'/>"
    "<menuitem action='OpenCavesDir'/>"
    "<separator/>"
    "<menuitem action='SaveFile'/>"
    "<menuitem action='SaveAsFile'/>"
    "<separator/>"
    "<menuitem action='CaveInfo'/>"
    "<menuitem action='HighScore'/>"
    "<menuitem action='ShowReplays'/>"
    "<menuitem action='CaveEditor'/>"
    "<separator/>"
    "<menuitem action='Quit'/>"
    "</menu>"
    "<menu action='PlayMenu'>"
    "<menuitem action='PauseGame'/>"
    "<menuitem action='TakeSnapshot'/>"
    "<menuitem action='RevertToSnapshot'/>"
    "<menuitem action='RestartLevel'/>"
    "<menuitem action='EndGame'/>"
    "<menuitem action='RandomColors'/>"
    "<separator/>"
    "<menuitem action='Volume'/>"
    "<menuitem action='FullScreen'/>"
    "<menuitem action='KeyboardPreferences'/>"
    "<menuitem action='GamePreferences'/>"
    "</menu>"
    "<menu action='HelpMenu'>"
    "<menuitem action='Help'/>"
    "<separator/>"
    "<menuitem action='Errors'/>"
    "<menuitem action='About'/>"
    "</menu>"
    "</menubar>"
    "</ui>";


class SetNextActionAndGtkQuitCommand : public Command {
public:
    SetNextActionAndGtkQuitCommand(App *app, NextAction &na, NextAction to_what): Command(app), na(na), to_what(to_what) {}
private:
    virtual void execute() {
        na = to_what;
        gtk_main_quit();
    }
    NextAction &na;
    NextAction to_what;
};


gboolean GdMainWindow::main_window_set_fullscreen_idle_func(gpointer data) {
    gtk_window_fullscreen(GTK_WINDOW(data));
    return FALSE;  /* do not call again */
}


/* set or unset fullscreen if necessary */
/* hack: gtk-win32 does not correctly handle fullscreen & removing widgets.
   so we put fullscreening call into a low priority idle function, which will be called
   after all window resizing & the like did take place. */
void GdMainWindow::main_window_set_fullscreen() {
    if (fullscreen) {
        gtk_widget_hide(menubar);
        g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc) main_window_set_fullscreen_idle_func, window, NULL);
    }
    else {
        gtk_window_unfullscreen(GTK_WINDOW(window));
        gtk_widget_show(menubar);
    }
}


gboolean GdMainWindow::delete_event(GtkWidget *widget, GdkEventAny *event, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->quit_event();
    return TRUE;
}


GdMainWindow::~GdMainWindow() {
    delete app;
    gtk_widget_destroy(window);
}


void GdMainWindow::cave_editor_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->start_editor();
}


void GdMainWindow::help_cb(GtkWidget *widget, gpointer data) {
    gd_show_game_help(((GdMainWindow *)data)->window);
}


void GdMainWindow::preferences_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->show_settings(gd_get_game_settings_array());
}


void GdMainWindow::keyboard_preferences_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->show_settings(gd_get_keyboard_settings_array(main_window->app->gameinput));
}


void GdMainWindow::volume_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->keypress_event(App::F9, 0);
}


void GdMainWindow::quit_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->quit_event();
}


void GdMainWindow::end_game_cb(GtkWidget *widget, gpointer data) {
    /* synthetic keypress */
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->keypress_event(GameActivity::EndGameKey, 0);
}


void GdMainWindow::take_snapshot_cb(GtkWidget *widget, gpointer data) {
    /* synthetic keypress */
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->keypress_event(GameActivity::TakeSnapshotKey, 0);
}


void GdMainWindow::random_colors_cb(GtkWidget *widget, gpointer data) {
    /* synthetic keypress */
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->keypress_event(GameActivity::RandomColorKey, 0);
}


void GdMainWindow::revert_to_snapshot_cb(GtkWidget *widget, gpointer data) {
    /* synthetic keypress */
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->keypress_event(GameActivity::RevertToSnapshotKey, 0);
}


void GdMainWindow::restart_level_cb(GtkWidget *widget, gpointer data) {
    /* synthetic keypress */
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->keypress_event(GameActivity::RestartLevelKey, 0);
}


void GdMainWindow::about_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->show_about_info();
}

void GdMainWindow::open_caveset_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->enqueue_command(new SelectFileToLoadIfDiscardableCommand(main_window->app, ""));
}


void GdMainWindow::open_caveset_dir_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->enqueue_command(new SelectFileToLoadIfDiscardableCommand(main_window->app, gd_system_caves_dir));
}


void GdMainWindow::save_caveset_as_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->enqueue_command(new SaveFileAsCommand(main_window->app));
}


void GdMainWindow::save_caveset_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->enqueue_command(new SaveFileCommand(main_window->app));
}


void GdMainWindow::highscore_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->enqueue_command(new ShowHighScoreCommand(main_window->app, NULL, -1));
}


void GdMainWindow::show_errors_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->enqueue_command(new ShowErrorsCommand(main_window->app, get_active_logger()));
}


/* called from the menu when a recent file is activated. */
void GdMainWindow::recent_chooser_activated_cb(GtkRecentChooser *chooser, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    GtkRecentInfo *current;
    char *filename_utf8, *filename;

    current = gtk_recent_chooser_get_current_item(chooser);
    /* we do not support non-local files */
    if (!gtk_recent_info_is_local(current)) {
        char *display_name=gtk_recent_info_get_uri_display(current);
        gd_errormessage(_("GDash cannot load file from a network link."), display_name);
        g_free(display_name);
        return;
    }

    filename_utf8 = gtk_recent_info_get_uri_display(current);
    filename = g_filename_from_utf8(filename_utf8, -1, NULL, NULL, NULL);
    main_window->app->enqueue_command(new AskIfChangesDiscardedCommand(main_window->app, new OpenFileCommand(main_window->app, filename)));
    gtk_recent_info_unref(current);
    g_free(filename);
    g_free(filename_utf8);
}


void GdMainWindow::toggle_fullscreen_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->fullscreen = !main_window->fullscreen;
    main_window->main_window_set_fullscreen();
}


void GdMainWindow::pause_game_cb(GtkWidget *widget, gpointer data) {
    /* synthetic keypress */
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->keypress_event(GameActivity::PauseKey, 0);
}


void GdMainWindow::show_replays_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    App *app = main_window->app;
    app->enqueue_command(new RestartWithTitleScreenCommand(app));
    app->enqueue_command(new PushActivityCommand(app, new ReplayMenuActivity(app)));
}


void GdMainWindow::cave_info_cb(GtkWidget *widget, gpointer data) {
    GdMainWindow *main_window = static_cast<GdMainWindow *>(data);
    main_window->app->enqueue_command(new ShowCaveInfoCommand(main_window->app));
}


GdMainWindow::GdMainWindow(bool add_menu, NextAction &na)
: na(na)
{
    this->fullscreen = false;
    window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 200);
    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(GdMainWindow::delete_event), this);

    /* vertical box */
    GtkWidget *vbox=gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER (window), vbox);

    /* menu */
    if (add_menu) {
        actions_normal=gtk_action_group_new("actions_normal");
        gtk_action_group_set_translation_domain(actions_normal, PACKAGE);
        gtk_action_group_add_actions(actions_normal, action_entries_normal, G_N_ELEMENTS(action_entries_normal), this);
        
        actions_game=gtk_action_group_new("actions_game");
        gtk_action_group_set_translation_domain(actions_game, PACKAGE);
        gtk_action_group_add_actions(actions_game, action_entries_game, G_N_ELEMENTS(action_entries_game), this);

        /* build the ui */
        GtkUIManager *ui=gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(ui, actions_normal, 0);
        gtk_ui_manager_insert_action_group(ui, actions_game, 0);
        gtk_window_add_accel_group(GTK_WINDOW(window), gtk_ui_manager_get_accel_group(ui));
        gtk_ui_manager_add_ui_from_string(ui, ui_info, -1, NULL);
        menubar=gtk_ui_manager_get_widget(ui, "/MenuBar");
        gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

        /* recent file chooser */
        GtkWidget *recent_chooser = gtk_recent_chooser_menu_new();
        GtkRecentFilter *recent_filter = gtk_recent_filter_new();
        /* gdash file extensions */
        for (int i=0; gd_caveset_extensions[i]!=NULL; i++)
            gtk_recent_filter_add_pattern(recent_filter, gd_caveset_extensions[i]);
        gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(recent_chooser), recent_filter);
        gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(recent_chooser), TRUE);
        gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent_chooser), 10);
        gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(recent_chooser), GTK_RECENT_SORT_MRU);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM (gtk_ui_manager_get_widget (ui, "/MenuBar/FileMenu/LoadRecent")), recent_chooser);
        g_signal_connect(G_OBJECT(recent_chooser), "item-activated", G_CALLBACK(GdMainWindow::recent_chooser_activated_cb), this);

        g_object_unref(ui);
    } else {
        actions_game = NULL;
    }

    /* scroll window which contains the cave or the title image, so this is the main content of the window */
    GtkWidget *align=gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_start(GTK_BOX(vbox), align, TRUE, TRUE, 0);

    /* drawing area for game */
    GtkWidget *drawing_area=gtk_drawing_area_new();
    gtk_widget_set_double_buffered(drawing_area, FALSE);
    gtk_container_add(GTK_CONTAINER(align), drawing_area);

    gtk_widget_show_all(window);
    gtk_window_present(GTK_WINDOW(window));

    /* initialize the app framework */
    app = new GTKApp(drawing_area, actions_game);
}


void gd_main_window_gtk_run(CaveSet *caveset, NextAction &na) {
    GdMainWindow *main_window = new GdMainWindow(true, na);

    /* configure the app */
    main_window->app->caveset = caveset;
    main_window->app->push_activity(new TitleScreenActivity(main_window->app));
    main_window->app->set_no_activity_command(new SetNextActionAndGtkQuitCommand(main_window->app, na, Quit));
    main_window->app->set_quit_event_command(new AskIfChangesDiscardedCommand(main_window->app, new PopAllActivitiesCommand(main_window->app)));
    main_window->app->set_request_restart_command(new SetNextActionAndGtkQuitCommand(main_window->app, na, Restart));
    main_window->app->set_start_editor_command(new SetNextActionAndGtkQuitCommand(main_window->app, na, StartEditor));

    /* and run */
    gtk_main();
    
    delete main_window;
    /* process all pending events before returning,
     * because the widget destroying created many. without processing them,
     * the editor window would not disappear! */
    while (gtk_events_pending())
        gtk_main_iteration();
}


void gd_main_window_gtk_run_a_game(GameControl *game) {
    NextAction na = StartTitle;      // because the funcs below need one to work with
    GdMainWindow *main_window = new GdMainWindow(false, na);

    /* configure */
    main_window->app->set_quit_event_command(new SetNextActionAndGtkQuitCommand(main_window->app, na, Quit));
    main_window->app->set_no_activity_command(new SetNextActionAndGtkQuitCommand(main_window->app, na, Quit));
    main_window->app->push_activity(new GameActivity(main_window->app, game));

    /* and run */
    gtk_main();
    
    delete main_window;
    /* process all pending events before returning,
     * because the widget destroying created many. without processing them,
     * the editor window would not disappear! */
    while (gtk_events_pending())
        gtk_main_iteration();

}
