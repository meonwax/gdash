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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "gfx/screen.hpp"
#include "gtk/gtkapp.hpp"
#include "gtk/gtkscreen.hpp"
#include "gtk/gtkgameinputhandler.hpp"
#include "gtk/gtkpixbuffactory.hpp"
#include "gtk/gtkui.hpp"
#include "gtk/gtkuisettings.hpp"
#include "gfx/fontmanager.hpp"
#include "framework/commands.hpp"
#include "misc/about.hpp"
#include "settings.hpp"


static Activity::KeyCode activity_keycode_from_gdk_keyval(guint keyval) {
    switch (keyval) {
        case GDK_Up: return App::Up;
        case GDK_Down: return App::Down;
        case GDK_Left: return App::Left;
        case GDK_Right: return App::Right;
        case GDK_Page_Up: return App::PageUp;
        case GDK_Page_Down: return App::PageDown;
        case GDK_Home: return App::Home;
        case GDK_End: return App::End;
        case GDK_F1: return App::F1;
        case GDK_F2: return App::F2;
        case GDK_F3: return App::F3;
        case GDK_F4: return App::F4;
        case GDK_F5: return App::F5;
        case GDK_F6: return App::F6;
        case GDK_F7: return App::F7;
        case GDK_F8: return App::F8;
        case GDK_F9: return App::F9;
        case GDK_BackSpace: return App::BackSpace;
        case GDK_Return: return App::Enter;
        case GDK_Tab: return App::Tab;
        case GDK_Escape: return App::Escape;
        
        default:
            return gdk_keyval_to_unicode(keyval);
    }
}


/* keypress and key release. pass it to the app framework. */
static gboolean main_window_keypress_event(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    GTKApp *the_app = static_cast<GTKApp *>(data);
    bool press = event->type==GDK_KEY_PRESS;  /* true for press, false for release */

    if (press) {
        Activity::KeyCode keycode = activity_keycode_from_gdk_keyval(event->keyval);
        the_app->keypress_event(keycode, event->keyval);
    } else {
        the_app->gameinput->keyrelease(event->keyval);
    }

    return FALSE;   /* TODO what to do here? */
}


/* focus leaves game play window. remember that keys are not pressed!
    as we don't receive key released events along with focus out. */
static gboolean main_window_focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    GTKApp *the_app = static_cast<GTKApp *>(data);
    the_app->gameinput->clear_all_keypresses();
    return FALSE;
}


gboolean drawing_area_expose(GtkWidget *drawing_area, GdkEventExpose *event, gpointer data) {
    GTKApp *the_app = static_cast<GTKApp *>(data);
    the_app->redraw_event();
    return TRUE;
}


/* this function is an idle func which will run in the main thread.
 * the main thread has the applicaton, and also it is allowed to use gtk and the other libs. */
gboolean GTKApp::timing_event_idle_func(gpointer data)
{
    GTKApp *the_app = static_cast<GTKApp *>(data);
    /* the number of events received since the last processing.
     * if the computer is slow, or the window is moved by the user with the mouse,
     * then this may be more than one. */
    int timer_events_received = the_app->timer_events;
    /* process the event; but only do it once, even if the counter is lagging. */
    if (gtk_window_is_active(GTK_WINDOW(the_app->toplevel)) && timer_events_received > 0) {
        the_app->timer_event(20);
        the_app->process_commands();
        g_atomic_int_add(&the_app->timer_events, -timer_events_received);
    }
    return FALSE;
}


/* this function will run in its own thread, and add the main int as an idle func in every 20 ms */
gpointer GTKApp::timing_thread(gpointer data) {
    GTKApp *the_app = static_cast<GTKApp *>(data);
    int const interval_msec = 20;
    
    /* wait before we first call it */
    g_usleep(interval_msec*1000);
    while (!the_app->quit_thread) {
        /* add processing as an idle func. no need to lock the main loop context, as glib does
         * that automatically. the idle func will run in the main thread, which is allowed to
         * access gtk+ and all the other libs. */
        g_atomic_int_inc(&the_app->timer_events);
        g_idle_add_full(G_PRIORITY_LOW, timing_event_idle_func, the_app, NULL);
        g_usleep(interval_msec*1000);
    }
    return NULL;
}


GTKApp::GTKApp(GtkWidget *drawing_area, GtkActionGroup *actions_game)
:
    drawing_area(drawing_area),
    actions_game(actions_game)
{
    pixbuf_factory = new GTKPixbufFactory();
    pixbuf_factory->set_properties(GdScalingType(gd_cell_scale_game), gd_pal_emulation_game);
    font_manager = new FontManager(*pixbuf_factory, "");
    gameinput = new GTKGameInputHandler;
    screen = new GTKScreen(drawing_area);

    /* attach events */
    toplevel = gtk_widget_get_toplevel(drawing_area);
    focus_handler = g_signal_connect(G_OBJECT(toplevel), "focus_out_event", G_CALLBACK(main_window_focus_out_event), this);
    keypress_handler = g_signal_connect(G_OBJECT(toplevel), "key_press_event", G_CALLBACK(main_window_keypress_event), this);
    keyrelease_handler = g_signal_connect(G_OBJECT(toplevel), "key_release_event", G_CALLBACK(main_window_keypress_event), this);
    expose_handler = g_signal_connect(G_OBJECT(drawing_area), "expose_event", G_CALLBACK(drawing_area_expose), this);

    /* install timer. create a thread which will install idle funcs. */
    quit_thread = false;
    timer_events = 0;
#ifndef G_THREADS_ENABLED
    #error Thread support in Glib must be enabled
#endif
    timer_thread = g_thread_create(timing_thread, this, TRUE, NULL);
    g_assert(timer_thread != NULL);
    
    game_active(false);
}


GTKApp::~GTKApp() {
    /* quit thread */
    quit_thread = true;
    g_thread_join(timer_thread);
    /* disconnect any signals and timeout handlers */
	g_signal_handler_disconnect(G_OBJECT(toplevel), focus_handler);
	g_signal_handler_disconnect(G_OBJECT(toplevel), keypress_handler);
	g_signal_handler_disconnect(G_OBJECT(toplevel), keyrelease_handler);
	g_signal_handler_disconnect(G_OBJECT(drawing_area), expose_handler);
    while (g_idle_remove_by_data(this))
        ; /* remove all */
}


void GTKApp::select_file_and_do_command(const char *title, const char *start_dir, const char *glob, bool for_save, const char *defaultname, SmartPtr<Command1Param<std::string> > command_when_successful) {
    GtkWidget *dialog=gtk_file_chooser_dialog_new(title, GTK_WINDOW(toplevel),
        for_save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    if (for_save)
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    if (defaultname && !g_str_equal(defaultname, ""))
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), defaultname);

    /* add file filter based on the glob given */
    GtkFileFilter *filter=gtk_file_filter_new();
    gtk_file_filter_set_name(filter, glob);
    char **globs = g_strsplit_set(glob, ";", -1);
    for (int i=0; globs[i]!=NULL; i++)
        gtk_file_filter_add_pattern(filter, globs[i]);
    g_strfreev(globs);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    /* if shipped with a directory name, show that directory by default */
    if (start_dir && !g_str_equal(start_dir, ""))
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), start_dir);

    int result=gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        /* give the filename to the command */
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        command_when_successful->set_param1(filename);
        g_free(filename);
        enqueue_command(command_when_successful);
    }
    gtk_widget_destroy(dialog);
}


void GTKApp::ask_yesorno_and_do_command(char const *question, const char *yes_answer, char const *no_answer, SmartPtr<Command> command_when_yes, SmartPtr<Command> command_when_no) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel), GtkDialogFlags(0),
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "%s", question);

    GtkWidget *buttonno=gtk_button_new_with_mnemonic(no_answer);
    gtk_button_set_image(GTK_BUTTON(buttonno), gtk_image_new_from_stock(GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON));
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), buttonno, GTK_RESPONSE_NO);

    GtkWidget *buttonyes=gtk_button_new_with_mnemonic(yes_answer);
    gtk_button_set_image(GTK_BUTTON(buttonyes), gtk_image_new_from_stock(GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON));
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), buttonyes, GTK_RESPONSE_YES);

    gtk_widget_show_all(dialog);
    bool yes = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES;
    gtk_widget_destroy(dialog);
    
    if (yes)
        enqueue_command(command_when_yes);
    else
        enqueue_command(command_when_no);
}


void GTKApp::show_text_and_do_command(char const *title_line, std::string const &text, SmartPtr<Command> command_after_exit) {
    /* create text buffer */
    GtkTextIter iter;
    
    GtkWidget *dialog=gtk_dialog_new_with_buttons(title_line, GTK_WINDOW(toplevel), GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 512, 384);
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start_defaults(GTK_BOX (GTK_DIALOG(dialog)->vbox), sw);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    /* get text and show it */
    GtkTextBuffer *buffer=gtk_text_buffer_new(NULL);
    GtkWidget *view=gtk_text_view_new_with_buffer(buffer);
    gtk_container_add(GTK_CONTAINER(sw), view);
    g_object_unref(buffer);

    /* remove GD_COLOR_SETCOLOR "markup" */
    std::string text_stripped;
    for (unsigned i = 0; i != text.length(); ++i) {
        if (text[i] != GD_COLOR_SETCOLOR)
            text_stripped += text[i];
        else
            ++i;
    }
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, -1);
    gtk_text_buffer_insert(buffer, &iter, text_stripped.c_str(), -1);

    /* set some tags */
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(view), 3);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 6);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    enqueue_command(command_after_exit);
}


void GTKApp::show_about_info() {
    gtk_show_about_dialog(GTK_WINDOW(toplevel), "program-name", "GDash", "license", About::license, "wrap-license", TRUE, "copyright", About::copyright, "authors", About::authors, "version", PACKAGE_VERSION, "comments", _(About::comments), "translator-credits", _(About::translator_credits), "website", About::website, "artists", About::artists, "documenters", About::documenters, NULL);
}


void GTKApp::input_text_and_do_command(char const *title_line, char const *default_text, SmartPtr<Command1Param<std::string> > command_when_successful) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title_line, GTK_WINDOW(toplevel),
        GtkDialogFlags(GTK_DIALOG_NO_SEPARATOR|GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_entry_set_text(GTK_ENTRY(entry), default_text);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, FALSE, 6);

    gtk_widget_show_all(dialog);
    int result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
        command_when_successful->set_param1(gtk_entry_get_text(GTK_ENTRY(entry)));
        enqueue_command(command_when_successful);
    }
    gtk_widget_destroy(dialog);
}


void GTKApp::game_active(bool active) {
    if (actions_game)
        gtk_action_group_set_sensitive(actions_game, active);
}


void GTKApp::show_settings(Setting *settings) {
    SettingsWindow::do_settings_dialog(settings, *pixbuf_factory);
}


void GTKApp::show_message(std::string const &primary, std::string const &secondary, SmartPtr<Command> command_after_exit) {
    gd_infomessage(primary.c_str(), secondary.c_str());
}
