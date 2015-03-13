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
#include "gtk/gtkui.hpp"
#include "framework/thememanager.hpp"
#include "gtk/gtkuisettings.hpp"



#define GDASH_KEYSIM_WHAT_FOR "gdash-keysim-what-for"
gboolean SettingsWindow::keysim_button_keypress_event(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    g_assert(event->type==GDK_KEY_PRESS);   /* must be true. */
    gtk_dialog_response(GTK_DIALOG(widget), event->keyval);
    return TRUE;    /* and say that we processed the key. */
}

void SettingsWindow::keysim_button_clicked_cb(GtkWidget *button, gpointer data) {
    const char *what_for=(const char *)g_object_get_data(G_OBJECT(button), GDASH_KEYSIM_WHAT_FOR);
    int *keyval=(int *) data;

    /* dialog which has its keypress event connected to the handler above */
    GtkWidget *dialog=gtk_dialog_new_with_buttons(_("Select Key"), GTK_WINDOW(gtk_widget_get_toplevel(button)),
        GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    GtkWidget *table = gtk_table_new(1,1, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    gtk_container_set_border_width(GTK_CONTAINER(table), 6);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), table);
    gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_leftaligned(_("Press key for action:")), 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_leftaligned(CPrintf("<b>%s</b>") % what_for), 0, 1, 1, 2);
    g_signal_connect(G_OBJECT(dialog), "key_press_event", G_CALLBACK(keysim_button_keypress_event), dialog);

    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_widget_show_all(dialog);
    int result=gtk_dialog_run(GTK_DIALOG(dialog));
    if (result>=0) {
        /* if positive, it must be a keyval. gtk_response_cancel and gtk_response delete is negative. */
        *keyval = result;
        gtk_button_set_label(GTK_BUTTON(button), gdk_keyval_name(*keyval));
    }
    gtk_widget_destroy(dialog);
}

GtkWidget *SettingsWindow::gd_keysim_button(const char *what_for, int *keyval)
{
    g_assert(keyval!=NULL);

    /* the button shows the current value in its name */
    GtkWidget *button = gtk_button_new_with_label(gdk_keyval_name(*keyval));
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(keysim_button_clicked_cb), keyval);
    g_object_set_data(G_OBJECT(button), GDASH_KEYSIM_WHAT_FOR, (gpointer) what_for);
    gtk_widget_set_tooltip_text(button, CPrintf(_("Click here to set the key for action: %s")) % what_for);

    return button;
}
#undef GDASH_KEYSIM_WHAT_FOR


/* settings window */
void SettingsWindow::bool_toggle(GtkWidget *widget, gpointer data) {
    bool *bl = (bool *) data;
    *bl = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}


void SettingsWindow::int_change(GtkWidget *widget, gpointer data) {
    int *value = (int *) data;
    *value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
}


void SettingsWindow::stringv_change(GtkWidget *widget, gpointer data) {
    int *ptr = (int *) data;
    *ptr = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    /* if nothing selected (for some reason), set to zero. */
    if (*ptr==-1)
        *ptr=0;
}


GtkWidget *SettingsWindow::combo_box_new_from_stringv(const char **str) {
    GtkWidget *combo = gtk_combo_box_new_text();
    for (unsigned i=0; str[i]!=NULL; i++)
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _(str[i])); // also translate
    return combo;
}


GtkWidget *SettingsWindow::combo_box_new_from_themelist(std::vector<std::string> const &strings) {
    GtkWidget *combo = gtk_combo_box_new_text();
    for (unsigned i=0; i != strings.size(); i++) {
        if (strings[i] == "")
            gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _("[Default]"));
        else {
            char *thm = g_filename_display_basename(strings[i].c_str());
            if (strrchr(thm, '.'))    /* remove extension */
                *strrchr(thm, '.')='\0';
            gtk_combo_box_append_text(GTK_COMBO_BOX(combo), thm);
            g_free(thm);
        }
    }
    return combo;
}


void SettingsWindow::do_settings_dialog(Setting *settings, PixbufFactory &pf) {
    GtkWidget *dialog=gtk_dialog_new_with_buttons(_("GDash Preferences"), guess_active_toplevel(),
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

    GtkWidget *notebook=gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(notebook), 9);
    gtk_box_pack_start_defaults(GTK_BOX (GTK_DIALOG (dialog)->vbox), notebook);

    std::vector<std::string> themes;
    int themenum;
    load_themes_list(pf, themes, themenum);

    int row = 0;
    GtkWidget *table = NULL;
    for (unsigned i=0; settings[i].name != NULL; i++) {

        GtkWidget *widget = NULL;

        switch (settings[i].type) {
            case TypePage:
                table = gtk_table_new(1, 1, FALSE);
                gtk_container_set_border_width(GTK_CONTAINER (table), 9);
                gtk_table_set_row_spacings(GTK_TABLE(table), 6);
                gtk_table_set_col_spacings(GTK_TABLE(table), 12);
                gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, gd_label_new_leftaligned(settings[i].name));
                row = 0;
                break;

            case TypeBoolean:
                widget = gtk_check_button_new();
                gtk_widget_set_tooltip_text(widget, _(settings[i].description));
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), *(bool *)settings[i].var);
                gtk_table_attach(GTK_TABLE(table), widget, 1, 2, row, row+1, GtkAttachOptions(GTK_EXPAND|GTK_FILL), GtkAttachOptions(0), 0, 0);
                g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(SettingsWindow::bool_toggle), settings[i].var);
                break;

            case TypePercent:
                widget = gtk_spin_button_new_with_range(0, 100, 5);
                gtk_widget_set_tooltip_text(widget, _(settings[i].description));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), *(int *)settings[i].var);
                g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(SettingsWindow::int_change), settings[i].var);
                gtk_table_attach(GTK_TABLE(table), widget, 1, 2, row, row+1, GtkAttachOptions(GTK_EXPAND|GTK_FILL), GtkAttachOptions(0), 0, 0);
                break;

            case TypeStringv:
                widget=SettingsWindow::combo_box_new_from_stringv(settings[i].stringv);
                gtk_widget_set_tooltip_text(widget, _(settings[i].description));
                gtk_combo_box_set_active(GTK_COMBO_BOX(widget), *(int *)settings[i].var);
                g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(SettingsWindow::stringv_change), settings[i].var);
                gtk_table_attach(GTK_TABLE(table), widget, 1, 2, row, row+1, GtkAttachOptions(GTK_EXPAND|GTK_FILL), GtkAttachOptions(0), 0, 0);
                break;
            
            case TypeTheme:
                widget=SettingsWindow::combo_box_new_from_themelist(themes);
                gtk_widget_set_tooltip_text(widget, _(settings[i].description));
                gtk_combo_box_set_active(GTK_COMBO_BOX(widget), themenum);
                g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(SettingsWindow::stringv_change), &themenum);
                gtk_table_attach(GTK_TABLE(table), widget, 1, 2, row, row+1, GtkAttachOptions(GTK_EXPAND|GTK_FILL), GtkAttachOptions(0), 0, 0);
                break;
            
            case TypeKey:
                widget = gd_keysim_button(settings[i].name, (int *) settings[i].var);
                gtk_widget_set_tooltip_text(widget, _(settings[i].description));
                gtk_table_attach(GTK_TABLE(table), widget, 1, 2, row, row+1, GtkAttachOptions(GTK_EXPAND|GTK_FILL), GtkAttachOptions(0), 0, 0);
                break;
        }
        
        if (widget) {
            GtkWidget *label = gd_label_new_leftaligned(_(settings[i].name));
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, GtkAttachOptions(GTK_EXPAND|GTK_FILL), GtkAttachOptions(0), 0, 0);
            gtk_label_set_mnemonic_widget(GTK_LABEL(label), widget);
        }

        row ++;
    }
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    
    gtk_widget_destroy(dialog);

    gd_theme = themes[themenum];
    /** @todo possible restart */
}
