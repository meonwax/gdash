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

#include "editor/editorwidgets.hpp"
#include "editor/editorautowidgets.hpp"
#include "misc/printf.hpp"
#include "cave/helper/colors.hpp"
#include "gtk/gtkui.hpp"
#include "cave/helper/reflective.hpp"

/*
 * ..._update_changed(GtkWidget *widget, gpointer data)
 * data=pointer to gdash property
 * g_object_get_data(GDASH_DATA_POINTER) -> pointer to EditorAutoUpdate
 * 
 */
#define GDASH_AUTOUPDATE_POINTER "gdash-autoupdate-pointer"
static void set_eau(GtkWidget *widget, EditorAutoUpdate *eau) {
    g_object_set_data(G_OBJECT(widget), GDASH_AUTOUPDATE_POINTER, eau);
}

static EditorAutoUpdate *get_eau(GtkWidget *widget) {
    return static_cast<EditorAutoUpdate *>(g_object_get_data(G_OBJECT(widget), GDASH_AUTOUPDATE_POINTER));
}
#undef GDASH_AUTOUPDATE_POINTER


/*****************************************************
 *
 * gdint editor
 *
 */
static void gdint_editwidget_changed_cb(GtkWidget *widget, gpointer data) {
    EditorAutoUpdate *eau=get_eau(widget);
    GdInt *pi=static_cast<GdInt *>(data);

    int value=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    if (*pi!=value) {
        *pi=value;
        eau->update_cave();
    }
}

static GtkWidget *gdint_editwidget_new(EditorAutoUpdate *eau, GdInt *value, int min, int max) {
    /* change range if needed */
    /// @todo this is to allow greater ranges based on current data. is this really needed? is this a good idea?
    if (*value<min) min=*value;
    if (*value>max) max=*value;

    GtkWidget *spin=gtk_spin_button_new_with_range(min, max, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), *value);
    set_eau(spin, eau);
    g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(gdint_editwidget_changed_cb), value);

    return spin;
}

static GtkWidget *gdint_editwidget_new(EditorAutoUpdate *eau, GdInt *value) {
    return gdint_editwidget_new(eau, value, eau->descr->min, eau->descr->max);
}

void gdint_editwidget_reload(GtkWidget *widget) {
    EditorAutoUpdate *eau=get_eau(widget);
    GdInt &i=eau->r->get<GdInt>(eau->descr->prop);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), i);
}


/*****************************************************
 *
 * gdprobability editor
 *
 */
static void gdprobability_editwidget_changed_cb(GtkWidget *widget, gpointer data) {
    EditorAutoUpdate *eau=get_eau(widget);
    GdProbability *pi=static_cast<GdProbability *>(data);

    int value=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget))/100.0*1000000.0; /* *100%, /1million (as it is stored that way) */
    if (*pi!=value) {
        *pi=value;
        eau->update_cave();
    }
}

static GtkWidget *gdprobability_editwidget_new(EditorAutoUpdate *eau, GdProbability *value) {
    GtkWidget *spin=gtk_spin_button_new_with_range(0.0, 100.0, 0.001);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), (*value)*100.0/1000000.0);     /* /1million * 100% */
    set_eau(spin, eau);
    g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(gdprobability_editwidget_changed_cb), value);

    return spin;
}


/*****************************************************
 *
 * gdelement editor
 *
 */
static void gdelement_editwidget_changed_cb(GtkWidget *widget, gpointer data) {
    EditorAutoUpdate *eau=get_eau(widget);
    GdElement *pe=static_cast<GdElement *>(data);

    GdElementEnum new_elem=gd_element_button_get(widget);
    if (*pe!=new_elem) {
        *pe=new_elem;
        eau->update_cave();
    }
}

static GtkWidget *gdelement_editwidget_new(EditorAutoUpdate *eau, GdElement *value) {
    GtkWidget *button=gd_element_button_new(*value, FALSE, NULL);
    set_eau(button, eau);
    /* this "clicked" will be called after the button's own, internal clicked signal */
    g_signal_connect(button, "clicked", G_CALLBACK(gdelement_editwidget_changed_cb), value);

    return button;
}


/*****************************************************
 *
 * check button with instant update
 *
 */
static void gdbool_editwidget_changed_cb(GtkWidget *widget, gpointer data) {
    EditorAutoUpdate *eau=get_eau(widget);
    GdBool *pb=static_cast<GdBool *>(data);

    bool new_value=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (*pb!=new_value) {
        *pb=new_value;
        eau->update_cave();
    }
}

static GtkWidget *gdbool_editwidget_new(EditorAutoUpdate *eau, GdBool *value, std::string const &label="") {
    GtkWidget *button;
    if (label=="")
        button=gtk_check_button_new();
    else
        button=gtk_check_button_new_with_label(label.c_str());
    set_eau(button, eau);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), *value);
    g_signal_connect(button, "toggled", G_CALLBACK(gdbool_editwidget_changed_cb), value);

    return button;
}


/*****************************************************
 *
 * gdscheduling editor
 *
 */
static void gdscheduling_editwidget_changed_cb(GtkWidget *widget, gpointer data) {
    GdScheduling *ps=static_cast<GdScheduling *>(data);

    GdScheduling new_value=gd_scheduling_combo_get_scheduling(widget);
    if (*ps!=new_value)
        *ps=new_value;
}

static GtkWidget *gdscheduling_editwidget_new(EditorAutoUpdate *eau, GdScheduling *value) {
    GtkWidget *combo=gd_scheduling_combo_new(*value);
    set_eau(combo, eau);
    g_signal_connect(combo, "changed", G_CALLBACK(gdscheduling_editwidget_changed_cb), value);

    return combo;
}


/*****************************************************
 *
 * gddirection editor
 *
 */
static void gddirection_editwidget_changed_cb(GtkWidget *widget, gpointer data) {
    GdDirection *pd=static_cast<GdDirection *>(data);

    GdDirection new_value=gd_direction_combo_get_direction(widget);
    if (*pd!=new_value)
        *pd=new_value;
}

static GtkWidget *gddirection_editwidget_new(EditorAutoUpdate *eau, GdDirection *value) {
    GtkWidget *combo=gd_direction_combo_new(*value);
    set_eau(combo, eau);
    g_signal_connect(combo, "changed", G_CALLBACK(gddirection_editwidget_changed_cb), value);

    return combo;
}


/*****************************************************
 *
 * gdstring editor
 *
 */
static void gdstring_editwidget_inserted_cb(GtkEntryBuffer *buffer, guint arg1, gchar *arg2, guint arg3, gpointer data) {
    GdString *ps=static_cast<GdString *>(data);
    
    *ps=gtk_entry_buffer_get_text(buffer);
}

static void gdstring_editwidget_deleted_cb(GtkEntryBuffer *buffer, guint arg1, guint arg2, gpointer data) {
    GdString *ps=static_cast<GdString *>(data);
    
    *ps=gtk_entry_buffer_get_text(buffer);
}

static GtkWidget *gdstring_editwidget_new(EditorAutoUpdate *eau, GdString *value) {
    GtkWidget *widget=gtk_entry_new();
    GtkEntryBuffer *buffer=gtk_entry_get_buffer(GTK_ENTRY(widget));
    gtk_entry_set_text(GTK_ENTRY(widget), (*value).c_str());
    set_eau(widget, eau);
    /* no changed signal; and destroyed won't work, as the buffer of the entry is already cleared. */
    g_signal_connect(buffer, "inserted-text", G_CALLBACK(gdstring_editwidget_inserted_cb), value);
    g_signal_connect(buffer, "deleted-text", G_CALLBACK(gdstring_editwidget_deleted_cb), value);

    return widget;
}


/*****************************************************
 *
 * gdlongstring editor
 *
 */
static void gdlongstring_editwidget_destroyed_cb(GtkWidget *widget, gpointer data) {
    GdString *ps=static_cast<GdString *>(data);
    GtkTextIter iter_start, iter_end;

    GtkTextBuffer *buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    gtk_text_buffer_get_iter_at_offset(buffer, &iter_start, 0);
    gtk_text_buffer_get_iter_at_offset(buffer, &iter_end, -1);
    char *text=gtk_text_buffer_get_text(buffer, &iter_start, &iter_end, TRUE);
    (*ps)=text;
    g_free(text);
}

static GtkWidget *gdlongstring_editwidget_new(EditorAutoUpdate *eau, GdString *value) {
    GtkTextBuffer *textbuffer=gtk_text_buffer_new(NULL);
    GtkWidget *view=gtk_text_view_new_with_buffer(textbuffer);
    set_eau(view, eau);
    g_signal_connect(view, "destroy", G_CALLBACK(gdlongstring_editwidget_destroyed_cb), value);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);

    gtk_text_buffer_insert_at_cursor(textbuffer, (*value).c_str(), -1);

    // a text view in its scroll windows, so it can be any large
    GtkWidget *scroll=gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), view);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_ETCHED_IN);

    return scroll;      // not the view!!
}


/*****************************************************
 *
 * gdcolor editor
 *
 */
static void gdcolor_editwidget_changed_cb(GtkWidget *widget, gpointer data) {
    EditorAutoUpdate *eau=get_eau(widget);
    GdColor *pc=static_cast<GdColor *>(data);

    GdColor value=gd_color_combo_get_color(widget);
    if (*pc!=value) {
        *pc=value;
        eau->update_cave();
    }
}

static GtkWidget *gdcolor_editwidget_new(EditorAutoUpdate *eau, GdColor *value) {
    GtkWidget *combo=gd_color_combo_new(*value);
    set_eau(combo, eau);
    g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(gdcolor_editwidget_changed_cb), value);

    return combo;
}

static void gdcolor_editwidget_reload(GtkWidget *widget) {
    EditorAutoUpdate *eau=get_eau(widget);
    GdColor &color=eau->r->get<GdColor>(eau->descr->prop);
    gd_color_combo_set(GTK_COMBO_BOX(widget), color);
}


void EditorAutoUpdate::update_cave() const {
    if (cave_update_cb)
        cave_update_cb();
}

void EditorAutoUpdate::reload() const {
    if (reload_cb)
        reload_cb(widget);
}

EditorAutoUpdate::EditorAutoUpdate(Reflective *r, Reflective *def, PropertyDescription const *descr, void (*cave_update_cb)())
:   r(r),
    def(def),
    descr(descr),
    widget(0),
    expand_vertically(false),
    cave_update_cb(cave_update_cb),
    reload_cb(0) {
    std::auto_ptr<GetterBase> const &prop = descr->prop;
    std::string defval;
    
    switch (descr->type) {
        case GD_TAB:
            // this is only for the gui, so we must not be called for this one
            g_assert_not_reached();
            break;
        case GD_LABEL:
            // abuse :)
            widget=gtk_hbox_new(TRUE, 3);
            if (descr->flags & GD_SHOW_LEVEL_LABEL)
                for (unsigned i=0; i<5; ++i)
                    gtk_container_add(GTK_CONTAINER(widget), gd_label_new_centered(CPrintf(_("Level %d")) % (i+1)));
            break;
        case GD_TYPE_LONGSTRING:
            expand_vertically=true;
            widget=gdlongstring_editwidget_new(this, &r->get<GdString>(prop));
            break;
        case GD_TYPE_STRING:
            widget=gdstring_editwidget_new(this, &r->get<GdString>(prop));
            break;
        case GD_TYPE_BOOLEAN:
            widget=gdbool_editwidget_new(this, &r->get<GdBool>(prop));
            defval=visible_name(def->get<GdBool>(prop));
            break;
        case GD_TYPE_BOOLEAN_LEVELS:
            widget=gtk_hbox_new(TRUE, 3);
            for (unsigned i=0; i<prop->count; ++i) {
                gtk_container_add(GTK_CONTAINER(widget), gdbool_editwidget_new(this, &r->get<GdBoolLevels>(prop)[i], SPrintf("%d")%(i+1)));
                if (i!=0)
                    defval+=", ";
                defval+=visible_name(def->get<GdBoolLevels>(prop)[i]);
            }
            break;
        case GD_TYPE_INT:
            widget=gdint_editwidget_new(this, &r->get<GdInt>(prop));
            defval=visible_name(def->get<GdInt>(prop));
            reload_cb=gdint_editwidget_reload;
            break;
        case GD_TYPE_INT_LEVELS:
            widget=gtk_hbox_new(TRUE, 3);
            for (unsigned i=0; i<prop->count; ++i) {
                gtk_container_add(GTK_CONTAINER(widget), gdint_editwidget_new(this, &r->get<GdIntLevels>(prop)[i]));
                if (i!=0)
                    defval+=", ";
                defval+=visible_name(def->get<GdIntLevels>(prop)[i]);
            }
            break;
        case GD_TYPE_PROBABILITY:
            widget=gdprobability_editwidget_new(this, &r->get<GdProbability>(prop));
            defval=visible_name(def->get<GdProbability>(prop));
            break;
        case GD_TYPE_PROBABILITY_LEVELS:
            widget=gtk_hbox_new(TRUE, 3);
            for (unsigned i=0; i<prop->count; ++i) {
                gtk_container_add(GTK_CONTAINER(widget), gdprobability_editwidget_new(this, &r->get<GdProbabilityLevels>(prop)[i]));
                if (i!=0)
                    defval+=", ";
                defval+=visible_name(def->get<GdProbabilityLevels>(prop)[i]);
            }
            break;
        case GD_TYPE_EFFECT:    /* effects also specify elements; only difference is bdcff. */
        case GD_TYPE_ELEMENT:
            widget=gdelement_editwidget_new(this, &r->get<GdElement>(prop));
            defval=visible_name(def->get<GdElement>(prop));
            break;
        case GD_TYPE_COLOR:
            widget=gdcolor_editwidget_new(this, &r->get<GdColor>(prop));
            defval=visible_name(def->get<GdColor>(prop));
            reload_cb=gdcolor_editwidget_reload;
            break;
        case GD_TYPE_DIRECTION:
            widget=gddirection_editwidget_new(this, &r->get<GdDirection>(prop));
            defval=visible_name(def->get<GdDirection>(prop));
            break;
        case GD_TYPE_SCHEDULING:
            widget=gdscheduling_editwidget_new(this, &r->get<GdScheduling>(prop));
            defval=visible_name(def->get<GdScheduling>(prop));
            break;
        case GD_TYPE_COORDINATE:
            widget=gtk_hbox_new(TRUE, 3);
            gtk_container_add(GTK_CONTAINER(widget), gdint_editwidget_new(this, &(r->get<Coordinate>(prop).x)));
            gtk_container_add(GTK_CONTAINER(widget), gdint_editwidget_new(this, &(r->get<Coordinate>(prop).y)));
            defval=visible_name(def->get<Coordinate>(prop));
            break;
    };
    
    std::string tip;

    if (descr->tooltip) {
        tip=_(descr->tooltip);
        if (defval!="")
            tip += SPrintf(_("\nDefault value: %s")) % _(defval.c_str());
        gtk_widget_set_tooltip_text(widget, tip.c_str());
    }
}
