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

#include "fileops/loadfile.hpp"
#include "gtk/gtkpixbuffactory.hpp"
#include "cave/cavestored.hpp"
#include "cave/caveset.hpp"
#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "misc/util.hpp"
#include "cave/elementproperties.hpp"
#include "settings.hpp"

#include "gtk/gtkui.hpp"
#include "gtk/gtkapp.hpp"
#include "framework/commands.hpp"

/* pixbufs of icons and the like */
#include "icons.cpp"
/* title image and icon */
#include "gdash_icon_48.cpp"

static char *caveset_filename=NULL;
static char *last_folder=NULL;

void gd_register_stock_icons() {
    /* a table of icon data (guint8*, static arrays included from icons.h) and stock id. */
    static struct {
        const guint8 *data;
        const char *stock_id;
    } icons[]={
        { cave_editor, GD_ICON_CAVE_EDITOR },
        { move, GD_ICON_EDITOR_MOVE },
        { add_join, GD_ICON_EDITOR_JOIN },
        { add_freehand, GD_ICON_EDITOR_FREEHAND },
        { add_point, GD_ICON_EDITOR_POINT },
        { add_line, GD_ICON_EDITOR_LINE },
        { add_rectangle, GD_ICON_EDITOR_RECTANGLE },
        { add_filled_rectangle, GD_ICON_EDITOR_FILLRECT },
        { add_raster, GD_ICON_EDITOR_RASTER },
        { add_fill_border, GD_ICON_EDITOR_FILL_BORDER },
        { add_fill_replace, GD_ICON_EDITOR_FILL_REPLACE },
        { add_maze, GD_ICON_EDITOR_MAZE },
        { add_maze_uni, GD_ICON_EDITOR_MAZE_UNI },
        { add_maze_braid, GD_ICON_EDITOR_MAZE_BRAID },
        { snapshot, GD_ICON_SNAPSHOT },
        { restart_level, GD_ICON_RESTART_LEVEL },
        { random_fill, GD_ICON_RANDOM_FILL },
        { award, GD_ICON_AWARD },
        { to_top, GD_ICON_TO_TOP },
        { to_bottom, GD_ICON_TO_BOTTOM },
        { object_on_all, GD_ICON_OBJECT_ON_ALL },
        { object_not_on_all, GD_ICON_OBJECT_NOT_ON_ALL },
        { object_not_on_current, GD_ICON_OBJECT_NOT_ON_CURRENT },
        { replay, GD_ICON_REPLAY },
        { keyboard, GD_ICON_KEYBOARD },
        { image, GD_ICON_IMAGE },
    };

    GtkIconFactory *factory=gtk_icon_factory_new();
    for (unsigned i=0; i<G_N_ELEMENTS(icons); ++i) {
        /* 3rd param: copy pixels = false */
        GdkPixbuf *pixbuf=gdk_pixbuf_new_from_inline (-1, icons[i].data, FALSE, NULL);
        GtkIconSet *iconset=gtk_icon_set_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
        gtk_icon_factory_add(factory, icons[i].stock_id, iconset);
    }
    gtk_icon_factory_add_default(factory);
    g_object_unref(factory);
}


GdkPixbuf *gd_icon() {
    GTKPixbuf pb(sizeof(gdash_icon_48), gdash_icon_48);
    g_object_ref(pb.get_gdk_pixbuf());
    return pb.get_gdk_pixbuf();
}


/* return a list of image gtk_image_filter's. */
/* they have floating reference. */
/* the list is to be freed by the caller. */
static GList *image_load_filters() {
    GSList *formats=gdk_pixbuf_get_formats();
    GSList *iter;
    GtkFileFilter *all_filter;
    GList *filters=NULL;    /* new list of filters */

    all_filter=gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, _("All image files"));

    /* iterate the list of formats given by gdk. create file filters for each. */
    for (iter=formats; iter!=NULL; iter=iter->next) {
        GdkPixbufFormat *frm=(GdkPixbufFormat *)iter->data;

        if (!gdk_pixbuf_format_is_disabled(frm)) {
            GtkFileFilter *filter;
            char **extensions;
            int i;

            filter=gtk_file_filter_new();
            gtk_file_filter_set_name(filter, gdk_pixbuf_format_get_description(frm));
            extensions=gdk_pixbuf_format_get_extensions(frm);
            for (i=0; extensions[i]!=NULL; i++) {
                char *pattern;

                pattern=g_strdup_printf("*.%s", extensions[i]);
                gtk_file_filter_add_pattern(filter, pattern);
                gtk_file_filter_add_pattern(all_filter, pattern);
                g_free(pattern);
            }
            g_strfreev(extensions);

            filters=g_list_append(filters, filter);
        }
    }
    g_slist_free(formats);

    /* add "all image files" filter */
    filters=g_list_prepend(filters, all_filter);

    return filters;
}


/* file open dialog, with filters for image types gdk-pixbuf recognizes. */
char *gd_select_image_file(const char *title) {
    GtkWidget *dialog;
    GList *filters;
    GList *iter;
    int result;
    char *filename;

    dialog=gtk_file_chooser_dialog_new (title, guess_active_toplevel(), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

    /* obtain list of image filters, and add all to the window */
    filters=image_load_filters();
    for (iter=filters; iter!=NULL; iter=iter->next)
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), GTK_FILE_FILTER(iter->data));
    g_list_free(filters);

    result=gtk_dialog_run(GTK_DIALOG(dialog));
    if (result==GTK_RESPONSE_ACCEPT)
        filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    else
        filename=NULL;
    gtk_widget_destroy(dialog);

    return filename;
}


/** 
 * Try to guess which window is active.
 */
GtkWindow *guess_active_toplevel() {
    GtkWidget *parent=NULL;

    /* before doing anything, process updates, as windows may have been opened or closed right at the previous moment */
    gdk_window_process_all_updates();

    /* if we find a modal window, it is active. */
    GList *toplevels=gtk_window_list_toplevels();
    for (GList *iter=toplevels; iter!=NULL; iter=iter->next)
        if (gtk_window_get_modal(GTK_WINDOW(iter->data)))
            parent=(GtkWidget *)iter->data;

    /* if no modal window found, search for a focused toplevel */
    if (!parent)
        for (GList *iter=toplevels; iter!=NULL; iter=iter->next)
            if (gtk_window_has_toplevel_focus(GTK_WINDOW(iter->data)))
                parent=(GtkWidget *)iter->data;

    /* if any of them is focused, just choose the last from the list as a fallback. */
    if (!parent && toplevels)
        parent=(GtkWidget *) g_list_last(toplevels)->data;
    g_list_free(toplevels);

    if (parent)
        return GTK_WINDOW(parent);
    else
        return NULL;
}


/**
 * Show a message dialog, with the specified message type (warning, error, info) and texts.
 * 
 * @param type GtkMessageType - sets icon to show.
 * @param primary Primary text.
 * @param secondary Secondary (small) text - may be null.
 */
static void show_message(GtkMessageType type, const char *primary, const char *secondary) {
    GtkWidget *dialog=gtk_message_dialog_new(guess_active_toplevel(),
        GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        type, GTK_BUTTONS_OK,
        "%s", primary);
    gtk_window_set_title(GTK_WINDOW(dialog), "GDash");
    /* secondary message exists an is not empty string: */
    if (secondary && secondary[0]!=0)
        gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG (dialog), "%s", secondary);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}


void gd_warningmessage(const char *primary, const char *secondary) {
    show_message(GTK_MESSAGE_WARNING, primary, secondary);
}


void gd_errormessage(const char *primary, const char *secondary) {
    show_message(GTK_MESSAGE_ERROR, primary, secondary);
}


void gd_infomessage(const char *primary, const char *secondary) {
    show_message(GTK_MESSAGE_INFO, primary, secondary);
}


/**
 * If necessary, ask the user if he doesn't want to save changes to cave.
 * 
 * If the caveset has no modification, this function simply returns true.
 */
bool gd_discard_changes(CaveSet const& caveset) {
    /* save highscore on every ocassion when the caveset is to be removed from memory */
    caveset.save_highscore(gd_user_config_dir);

    /* caveset is not edited, so pretend user confirmed */
    if (!caveset.edited)
        return TRUE;

    GtkWidget *dialog=gtk_message_dialog_new(guess_active_toplevel(), GtkDialogFlags(0), GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("Caveset \"%s\" is edited or new replays are added. Discard changes?"), caveset.name.c_str());
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dialog), _("If you discard the caveset, all changes and new replays will be lost."));
    gtk_dialog_add_button(GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
    /* create a discard button with a trash icon and Discard text */
    GtkWidget *button=gtk_button_new_with_mnemonic(_("_Discard"));
    gtk_button_set_image(GTK_BUTTON (button), gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON));
    gtk_widget_show (button);
    gtk_dialog_add_action_widget(GTK_DIALOG (dialog), button, GTK_RESPONSE_YES);

    bool discard=gtk_dialog_run(GTK_DIALOG (dialog))==GTK_RESPONSE_YES;
    gtk_widget_destroy(dialog);

    /* return button pressed */
    return discard;
}


static void caveset_file_operation_successful(const char *filename) {
    /* save successful, so remember filename */
    /* first we make a copy, as it is possible that filename==caveset_filename (the pointers!) */
    char *uri;

    /* add to recent chooser */
    if (g_path_is_absolute(filename))
        uri=g_filename_to_uri(filename, NULL, NULL);
    else {
        /* make an absolute filename if needed */
        char *currentdir=g_get_current_dir();
        char *absolute=g_build_path(G_DIR_SEPARATOR_S, currentdir, filename, NULL);
        uri=g_filename_to_uri(absolute, NULL, NULL);
        g_free(currentdir);
        g_free(absolute);
    }
    gtk_recent_manager_add_item(gtk_recent_manager_get_default(), uri);
    g_free(uri);

    /* if it is a bd file, remember new filename */
    if (g_str_has_suffix(filename, ".bd")) {
        /* first make copy, then free and set pointer. we might be called with filename=caveset_filename */
        char *stored=g_strdup(filename);
        g_free(caveset_filename);
        caveset_filename=stored;
    } else {
        g_free(caveset_filename);
        caveset_filename=NULL;
    }
}


/**
 * Save caveset to specified directory, and pop up error message if failed.
 */
static void caveset_save(const gchar *filename, CaveSet &caveset) {
    try {
        caveset.save_to_file(filename);
        caveset_file_operation_successful(filename);
    } catch (std::exception& e) {
        gd_errormessage(e.what(), filename);
    }
}


/**
 * Pops up a "save file as" dialog to the user, to select a file to save the caveset to.
 * If selected, saves the file.
 * 
 * @param parent Parent window for the dialog.
 * @param caveset The caveset to save.
 */
void gd_save_caveset_as(CaveSet &caveset) {
    GtkWidget *dialog=gtk_file_chooser_dialog_new (_("Save File As"), guess_active_toplevel(), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkFileFilter *filter=gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("BDCFF cave sets (*.bd)"));
    gtk_file_filter_add_pattern(filter, "*.bd");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    filter=gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All files (*)"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), CPrintf("%s.bd") % caveset.name);

    char *filename=NULL;
    if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
        filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    /* check if .bd extension should be added */
    if (filename) {
        /* if it has no .bd extension, add one */
        if (!g_str_has_suffix(filename, ".bd")) {
            char *suffixed=g_strdup_printf("%s.bd", filename);
            g_free(filename);
            filename=suffixed;
        }
    }

    /* if we have a filename, do the save */
    if (filename) {
        caveset_save(filename, caveset);
    }
    g_free(filename);
    gtk_widget_destroy(dialog);
}


/**
 * Save the current caveset. If no filename is stored, asks the user for a new filename before proceeding.
 * 
 * @param parent Parent window for dialogs.
 * @param caveset Caveset to save.
 */
void gd_save_caveset(CaveSet &caveset) {
    if (!caveset_filename)
        /* if no filename remembered, rather start the save_as function, which asks for one. */
        gd_save_caveset_as(caveset);
    else
        /* if given, save. */
        caveset_save(caveset_filename, caveset);
}


/**
 * Pops up a file selection dialog; and loads the caveset selected.
 * 
 * Before doing anything, asks the user if he wants to save the current caveset.
 * If it is edited and not saved, this function will do nothing.
 */
void gd_open_caveset(const char *directory, CaveSet &caveset) {
    char *filename=NULL;

    /* if caveset is edited, and user does not want to discard changes */
    if (!gd_discard_changes(caveset))
        return;

    GtkWidget *dialog=gtk_file_chooser_dialog_new (_("Open File"), guess_active_toplevel(), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

    GtkFileFilter *filter=gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("GDash cave sets"));
    for (int i=0; gd_caveset_extensions[i]!=NULL; i++)
        gtk_file_filter_add_pattern(filter, gd_caveset_extensions[i]);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    /* if callback shipped with a directory name, show that directory by default */
    if (directory)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), directory);
    else
    if (last_folder)
        /* if we previously had an open command, the directory was remembered */
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), last_folder);
    else
        /* otherwise user home */
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), g_get_home_dir());

    int result=gtk_dialog_run(GTK_DIALOG (dialog));
    if (result==GTK_RESPONSE_ACCEPT) {
        filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        g_free(last_folder);
        last_folder=gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER(dialog));
    }

    /* WINDOWS GTK+ 20080926 HACK */
    /* gtk bug - sometimes the above widget destroy creates an error message. */
    /* so we delete the error flag here. */
    /* MacOS GTK+ hack */
    /* well, in MacOS, the file open dialog does report this error: */
    /* "Unable to find default local directory monitor type" - original text */
    /* "Vorgegebener Überwachungstyp für lokale Ordner konnte nicht gefunden werden" - german text */
    /* so better to always clear the error flag. */
    {
        Logger l;
        gtk_widget_destroy(dialog);
        l.clear();
    }

    try {
        caveset = create_from_file(filename);
    } catch (std::exception &e) {
        gd_errormessage(_("Error loading caveset."), e.what());
    }
    g_free(filename);
}


/**
 * Convenience function to create a label with centered text.
 * @param markup The text to show (in pango markup format)
 * @return The new GtkLabel.
 */
GtkWidget *gd_label_new_centered(const char *markup) {
    GtkWidget *lab=gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(lab), 0, 0.5);
    gtk_label_set_markup(GTK_LABEL(lab), markup);

    return lab;
}


/**
 * Convenience function to create a label with centered text.
 * @param markup The text to show (in pango markup format)
 * @return The new GtkLabel.
 */
GtkWidget *gd_label_new_leftaligned(const char *markup) {
    GtkWidget *lab=gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(lab), 0, 0.5);
    gtk_label_set_markup(GTK_LABEL(lab), markup);

    return lab;
}


void gd_show_errors(Logger &l, const char *title, bool always_show) {
    if (l.get_messages().empty() && !always_show)
        return;
    /* create text buffer */
    GtkTextIter iter;
    GdkPixbuf *pixbuf_error, *pixbuf_warning, *pixbuf_info;

    GtkWidget *dialog=gtk_dialog_new_with_buttons(title, guess_active_toplevel(), GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_default_size (GTK_WINDOW(dialog), 512, 384);
    GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start_defaults(GTK_BOX (GTK_DIALOG (dialog)->vbox), sw);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    /* get text and show it */
    GtkTextBuffer *buffer=gtk_text_buffer_new(NULL);
    GtkWidget *view=gtk_text_view_new_with_buffer (buffer);
    gtk_container_add(GTK_CONTAINER(sw), view);
    g_object_unref(buffer);

    pixbuf_error=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU, NULL);
    pixbuf_warning=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU, NULL);
    pixbuf_info=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU, NULL);
    Logger::Container const& messages=l.get_messages();
    for (Logger::ConstIterator error=messages.begin(); error!=messages.end(); ++error) {
        gtk_text_buffer_get_iter_at_offset(buffer, &iter, -1);
        if (error->sev<=ErrorMessage::Message)
            gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_info);
        else if (error->sev<=ErrorMessage::Warning)
            gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_warning);
        else
            gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_error);
        gtk_text_buffer_insert(buffer, &iter, error->message.c_str(), -1);
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    }
    g_object_unref(pixbuf_error);
    g_object_unref(pixbuf_warning);
    g_object_unref(pixbuf_info);

    /* set some tags */
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(view), 3);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 6);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG (dialog));
    gtk_widget_destroy(dialog);

    /* shown to the users - forget them. */
    l.clear();
}


/**
 * Creates a small dialog window with the given text and asks the user a yes/no question
 * @param primary The primary (upper) text to show in the dialog.
 * @param secondary The secondary (lower) text to show in the dialog. May be NULL.
 * @return true, if the user answered yes, no otherwise.
 */
bool gd_question_yesno(const char *primary, const char *secondary) {
    GtkWidget *dialog=gtk_message_dialog_new(guess_active_toplevel(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", primary);
    if (secondary && !g_str_equal(secondary, ""))
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary);
    int response=gtk_dialog_run(GTK_DIALOG (dialog));
    gtk_widget_destroy(dialog);

    return response==GTK_RESPONSE_YES;
}


/**
 * Adds a hint (text) to the lower part of the dialog.
 * Also adds a little INFO icon.
 * Turns off the separator of the dialog, as it looks nicer without.
 */
void gd_dialog_add_hint(GtkDialog *dialog, const char *hint) {
    /* turn off separator, as it does not look nice with the hint */
    gtk_dialog_set_has_separator(dialog, FALSE);

    GtkWidget *align=gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->vbox), align, FALSE, TRUE, 0);
    GtkWidget *hbox=gtk_hbox_new(FALSE, 6);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    GtkWidget *label=gd_label_new_centered(hint);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
}
