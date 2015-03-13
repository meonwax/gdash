/*
 * Copyright (c) 2007, 2008, 2009, Czirkos Zoltan <cirix@fw.hu>
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
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include "gfxutil.h"
#include "cave.h"
#include "cavedb.h"
#include "c64import.h"
#include "caveset.h"
#include "gtkgfx.h"
#include "gtkmain.h"
#include "config.h"
#include "help.h"
#include "caveobject.h"
#include "settings.h"
#include "gtkui.h"
#include "editorbits.h"
#include "editorexport.h"
#include "editorwidgets.h"
#include "util.h"

#include "editor.h"

static GtkWidget *caveset_popup, *object_list_popup, *drawing_area_popup, *level_scale, *new_object_level_combo;
static GtkActionGroup *actions, *actions_edit_tools, *actions_edit_cave, *actions_edit_caveset, *actions_edit_map,
    *actions_edit_random, *actions_edit_object, *actions_edit_one_object, *actions_cave_selector, *actions_toggle,
    *actions_clipboard_paste, *actions_edit_undo, *actions_edit_redo, *actions_clipboard;
GtkWidget *gd_editor_window;

static int action;    /* activated edit tool, like move, plot, line... can be a gdobject, or there are other indexes which have meanings. */
static int edit_level; /* level shown in the editor... does not really affect editing */

static int clicked_x, clicked_y, mouse_x, mouse_y;    /* variables for mouse movement handling */
static gboolean button1_clicked;    /* true if we got button1 press event, then set to false on release */

static GList *undo_caves=NULL, *redo_caves=NULL;
static gboolean undo_move_flag=FALSE;    /* this is set to false when the mouse is clicked. on any movement, undo is saved and set to true */

static GdCave *edited_cave;    /* cave data actually edited in the editor */
static GdCave *rendered_cave;    /* a cave with all objects rendered, and to be drawn */

static GList *object_clipboard=NULL;    /* cave object clipboard. */
static GList *cave_clipboard=NULL;    /* copied caves */


static GtkWidget *scroll_window, *scroll_window_objects;
static GtkWidget *iconview_cavelist, *drawing_area;
static GtkWidget *toolbars;
static GtkWidget *element_button, *fillelement_button;
static GtkWidget *label_coordinate, *label_object, *label_first_element, *label_second_element;
static GHashTable *cave_pixbufs;

static int **gfx_buffer;
static gboolean **object_highlight_map;

static GtkListStore *object_list;
static GtkWidget *object_list_tree_view;
static GList *selected_objects=NULL;

/* objects index */
enum _object_list_columns {
    INDEX_COLUMN,
    LEVELS_PIXBUF_COLUMN,
    TYPE_PIXBUF_COLUMN,
    ELEMENT_PIXBUF_COLUMN,
    FILL_PIXBUF_COLUMN,
    TEXT_COLUMN,
    POINTER_COLUMN,
    NUM_EDITOR_COLUMNS
};

/* cave index */
enum _cave_icon_view_columns {
    CAVE_COLUMN,
    NAME_COLUMN,
    PIXBUF_COLUMN,
    NUM_CAVESET_COLUMNS
};


#define FREEHAND -1
#define MOVE -2
#define VISIBLE_REGION -3

/* edit tools; check variable "action". this is declared here, as it stores the names of cave drawing objects */
static GtkRadioActionEntry action_objects[]={
    {"Move", GD_ICON_EDITOR_MOVE, N_("_Move"), "F1", N_("Move object"), MOVE},
    
    /********** the order of these real cave object should be the same as the GdObject enum ***********/
    {"Plot", GD_ICON_EDITOR_POINT, N_("_Point"), "F2", N_("Draw single element"), GD_POINT},
    {"Line", GD_ICON_EDITOR_LINE, N_("_Line"), "F4", N_("Draw line of elements"), GD_LINE},
    {"Rectangle", GD_ICON_EDITOR_RECTANGLE, N_("_Outline"), "F5", N_("Draw rectangle outline"), GD_RECTANGLE},
    {"FilledRectangle", GD_ICON_EDITOR_FILLRECT, N_("R_ectangle"), "F6", N_("Draw filled rectangle"), GD_FILLED_RECTANGLE},
    {"Raster", GD_ICON_EDITOR_RASTER, N_("Ra_ster"), NULL, N_("Draw raster"), GD_RASTER},
    {"Join", GD_ICON_EDITOR_JOIN, N_("_Join"), NULL, N_("Draw join"), GD_JOIN},
    {"FloodFillReplace", GD_ICON_EDITOR_FILL_REPLACE, N_("_Flood fill"), NULL, N_("Fill by replacing elements"), GD_FLOODFILL_REPLACE},
    {"FloodFillBorder", GD_ICON_EDITOR_FILL_BORDER, N_("_Boundary fill"), NULL, N_("Fill a closed area"), GD_FLOODFILL_BORDER},
    {"Maze", GD_ICON_EDITOR_MAZE, N_("Ma_ze"), NULL, N_("Draw maze"), GD_MAZE},
    {"UnicursalMaze", GD_ICON_EDITOR_MAZE_UNI, N_("U_nicursal maze"), NULL, N_("Draw unicursal maze"), GD_MAZE_UNICURSAL},
    {"BraidMaze", GD_ICON_EDITOR_MAZE_BRAID, N_("Bra_id maze"), NULL, N_("Draw braid maze"), GD_MAZE_BRAID},
    {"RandomFill", GD_ICON_RANDOM_FILL, N_("R_andom fill"), NULL, N_("Draw random elements"), GD_RANDOM_FILL},
    {"CopyPaste", GTK_STOCK_COPY, N_("_Copy and Paste"), "" /* not null or it would use ctrl+c hotkey */, N_("Copy and paste area"), GD_COPY_PASTE},
    /* end of real cave objects */
    
    {"Freehand", GD_ICON_EDITOR_FREEHAND, N_("F_reehand"), "F3", N_("Draw freely"), FREEHAND},
    {"Visible", GTK_STOCK_ZOOM_FIT, N_("Set _visible region"), NULL, N_("Select visible region of cave during play"), VISIBLE_REGION},
};


/* forward function declarations */
static void set_status_label_for_cave(GdCave *cave);
static void select_cave_for_edit(GdCave *);
static void select_tool(int tool);
static void render_cave();
static void object_properties(GdObject *object);


static
struct _new_object_visible_on {
    const char *text;
    int switch_to_level;
    int mask;
} new_objects_visible_on[] = {
    { N_("All levels"), 1, GD_OBJECT_LEVEL_ALL },
    { N_("Level 2 and up"), 2, GD_OBJECT_LEVEL2|GD_OBJECT_LEVEL3|GD_OBJECT_LEVEL4|GD_OBJECT_LEVEL5 },
    { N_("Level 3 and up"), 3, GD_OBJECT_LEVEL3|GD_OBJECT_LEVEL4|GD_OBJECT_LEVEL5 },
    { N_("Level 4 and up"), 4, GD_OBJECT_LEVEL4|GD_OBJECT_LEVEL5 },
    { N_("Level 5 only"), 5, GD_OBJECT_LEVEL5 },
    { N_("Current level only"), -1, 0 },    /* -1 means: do not change level seen. */
};














/*****************************************
 * OBJECT LIST
 * which shows objects in a cave,
 * and also stores current selection.
 */
/* return a list of selected objects. */

static int
object_list_count_selected()
{
    return g_list_length(selected_objects);
}

static gboolean
object_list_is_any_selected()
{
    return selected_objects!=NULL;
}

static GdObject*
object_list_first_selected()
{
    if (!object_list_is_any_selected())
        return NULL;
    
    return selected_objects->data;
}

/* check if current iter points to the same object as data. if it is, select the iter */
static gboolean
object_list_select_object_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    GdObject *object;

    gtk_tree_model_get (model, iter, POINTER_COLUMN, &object, -1);
    if (object==data) {
        gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (object_list_tree_view)), iter);
        return TRUE;
    }
    return FALSE;
}

/* check if current iter points to the same object as data. if it is, select the iter */
static gboolean
object_list_unselect_object_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    GdObject *object;

    gtk_tree_model_get (model, iter, POINTER_COLUMN, &object, -1);
    if (object==data) {
        gtk_tree_selection_unselect_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (object_list_tree_view)), iter);
        return TRUE;
    }
    return FALSE;
}

static void
object_list_clear_selection()
{
    GtkTreeSelection *selection;

    selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view));
    gtk_tree_selection_unselect_all(selection);
}

static void
object_list_add_to_selection(GdObject *object)
{
    gtk_tree_model_foreach(GTK_TREE_MODEL (object_list), (GtkTreeModelForeachFunc) object_list_select_object_func, object);
}

static void
object_list_remove_from_selection(GdObject *object)
{
    gtk_tree_model_foreach(GTK_TREE_MODEL (object_list), (GtkTreeModelForeachFunc) object_list_unselect_object_func, object);
}

static void
object_list_select_one_object(GdObject *object)
{
    object_list_clear_selection();
    object_list_add_to_selection(object);
}


static void
object_list_selection_changed_signal (GtkTreeSelection *selection, gpointer data)
{
    GtkTreeModel *model;
    GList *selected_rows, *siter;
    int x, y;
    int count;
    
    g_list_free(selected_objects);
    selected_objects=NULL;

    /* check all selected objects, and set all selected objects to highlighted */
    selected_rows=gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)), &model);
    for (siter=selected_rows; siter!=NULL; siter=siter->next) {
        GtkTreePath *path=siter->data;
        GtkTreeIter iter;
        GdObject *object;
        
        gtk_tree_model_get_iter(model, &iter, path);
        gtk_tree_model_get(model, &iter, POINTER_COLUMN, &object, -1);
        selected_objects=g_list_append(selected_objects, object);
    }
    g_list_foreach(selected_rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selected_rows);
    
    /* object highlight all to false */
    for (y=0; y<rendered_cave->h; y++)
        for (x=0; x<rendered_cave->w; x++)
            object_highlight_map[y][x]=FALSE;
    
    /* check all selected objects, and set all selected objects to highlighted */
    for (siter=selected_objects; siter!=NULL; siter=siter->next) {
        GdObject *object=siter->data;
        
        for (y=0; y<rendered_cave->h; y++)
            for (x=0; x<rendered_cave->w; x++)
                if (rendered_cave->objects_order[y][x]==object)
                    object_highlight_map[y][x]=TRUE;
        
    }
    
    /* how many selected objects? */
    count=g_list_length(selected_objects);

    /* enable actions */
    gtk_action_group_set_sensitive (actions_edit_one_object, count==1);
    gtk_action_group_set_sensitive (actions_edit_object, count!=0);
    gtk_action_group_set_sensitive (actions_clipboard, count!=0);

    if (count==0) {
        /* NO object selected -> show general cave info */
        set_status_label_for_cave(edited_cave);
    }
    else if (count==1) {
        /* exactly ONE object is selected */
        char *text;
        GdObject *object=object_list_first_selected();

        text=gd_object_get_description_markup (object);    /* to be g_freed */
        gtk_label_set_markup (GTK_LABEL (label_object), text);
        g_free(text);
    }
    else if (count>1)
        /* more than one object is selected */
        gd_label_set_markup_printf(GTK_LABEL(label_object), _("%d objects selected"), count);
}

/* for popup menu, by properties key */
static void
object_list_show_popup_menu(GtkWidget *widget, gpointer data)
{
    gtk_menu_popup(GTK_MENU(object_list_popup), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

/* for popup menu, by right-click */
static gboolean
object_list_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    if (event->type==GDK_BUTTON_PRESS && event->button==3) {
        gtk_menu_popup(GTK_MENU(object_list_popup), NULL, NULL, NULL, NULL, event->button, event->time);
        return TRUE;
    }
    return FALSE;
}

/*******
 * SIGNALS which are used when the object list is reordered by drag and drop.
 *
 * non-trivial.
 * drag and drop emits the following signals: insert, changed, and delete.
 * so there is a moment when the reordered row has TWO copies in the model.
 * therefore we cannot use the changed signal to see the new order.
 *
 * we also cannot use the delete signal on its own, as it is also emitted when
 * rendering the cave.
 *
 * instead, we connect to the changed and the delete signal. we have a flag,
 * named "rowchangedbool", which is set to true by the changed signal, and
 * therefore we know that the next delete signal is emitted by a reordering.
 * at that point, we have the new order, and _AFTER_ resetting the flag,
 * we rerender the cave. */
static gboolean object_list_row_changed_bool=FALSE;

static void
object_list_row_changed(GtkTreeModel *model, GtkTreePath *changed_path, GtkTreeIter *changed_iter, gpointer user_data)
{
    /* set the flag so we know that the next delete signal is emitted by a drag-and-drop */
    object_list_row_changed_bool=TRUE;
}

static gboolean
object_list_new_order_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    GList **list=(GList **)data;
    GdObject *object;
    
    gtk_tree_model_get(model, iter, POINTER_COLUMN, &object, -1);
    *list=g_list_append(*list, object);
    
    return FALSE;    /* continue traversing */
}

static void
object_list_row_delete(GtkTreeModel *model, GtkTreePath *changed_path, gpointer user_data)
{
    if (object_list_row_changed_bool) {
        GList *new_order=NULL;

        /* reset the flag, as we handle the reordering here. */
        object_list_row_changed_bool=FALSE;
        
        /* this will build the new object order to the list */
        gtk_tree_model_foreach(model, object_list_new_order_func, &new_order);
        
        g_list_free(edited_cave->objects);
        edited_cave->objects=new_order;

        render_cave();
    }
}

/* row activated - set properties of object */
static void
object_list_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    GtkTreeModel *model=gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    GdObject *object;
    
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, POINTER_COLUMN, &object, -1);
    object_properties(object);
    /* render cave after changing object properties */
    render_cave();
}














static void
help_cb(GtkWidget *widget, gpointer data)
{
    gd_show_editor_help (gd_editor_window);
}




/****************************************************
 *
 * FUNCTIONS FOR UNDO
 *
 */

static void
redo_free_all()
{
    g_list_foreach(redo_caves, (GFunc) gd_cave_free, NULL);
    g_list_free(redo_caves);
    redo_caves=NULL;
}

/* delete the cave copies saved for undo. */
static void
undo_free_all()
{
    redo_free_all();
    g_list_foreach(undo_caves, (GFunc) gd_cave_free, NULL);
    g_list_free(undo_caves);
    undo_caves=NULL;
}

/* save a copy of the current state of edited cave. this is to be used
   internally; as it does not delete redo. */
#define UNDO_STEPS 10
static void
undo_save_current_state()
{
    /* if more than four, forget some (should only forget one) */
    while (g_list_length(undo_caves) >= UNDO_STEPS) {
        gpointer deleted=g_list_first(undo_caves)->data;
        
        gd_cave_free((GdCave *)deleted);
        undo_caves=g_list_remove(undo_caves, deleted);
    }
    
    undo_caves=g_list_append(undo_caves, gd_cave_new_from_cave(edited_cave));
}

/* save a copy of the current state of edited cave, after some operation.
   this destroys the redo list, as from that point that is useless. */
static void
undo_save()
{
    g_return_if_fail(edited_cave!=NULL);

    /* we also use this function to set the edited flag, as it is called for any action */
    gd_caveset_edited=TRUE;

    /* remove from pixbuf hash: delete its pixbuf */
    /* as now we know that this cave is really edited. */
    g_hash_table_remove(cave_pixbufs, edited_cave);
    
    undo_save_current_state();
    redo_free_all();    
    
    /* now we have a cave to do an undo from, so sensitize the menu item */
    gtk_action_group_set_sensitive (actions_edit_undo, TRUE);
    gtk_action_group_set_sensitive (actions_edit_redo, FALSE);
}

static void
undo_do_one_step()
{
    GdCave *backup;

    /* revert to last in list */
    backup=(GdCave *)g_list_last(undo_caves)->data;
    
    /* push current to redo list. we do not check the redo list size, as the undo size limits it automatically... */
    redo_caves=g_list_prepend(redo_caves, gd_cave_new_from_cave(edited_cave));
    
    /* copy to edited one, and free backup */
    gd_cave_copy(edited_cave, backup);
    gd_cave_free(backup);
    undo_caves=g_list_remove(undo_caves, backup);

    /* call to renew editor window */
    select_cave_for_edit(edited_cave);
}

/* do the undo - callback */
static void
undo_cb(GtkWidget *widget, gpointer data)
{
    g_return_if_fail(undo_caves!=NULL);
    g_return_if_fail(edited_cave!=NULL);
    
    undo_do_one_step();
}

/* do the redo - callback */
static void
redo_cb(GtkWidget *widget, gpointer data)
{
    GdCave *backup;

    g_return_if_fail(redo_caves!=NULL);
    g_return_if_fail(edited_cave!=NULL);

    /* push back current to undo */
    undo_save_current_state();
    
    /* and select the first from redo */
    backup=g_list_first(redo_caves)->data;
    gd_cave_copy(edited_cave, backup);
    gd_cave_free(backup);
    
    redo_caves=g_list_remove(redo_caves, backup);
    /* call to renew editor window */
    select_cave_for_edit(edited_cave);
}




static void
editor_window_set_title()
{
    char *title;
    
    if (edited_cave) {
        /* editing a specific cave */
        title=g_strdup_printf(_("GDash Cave Editor - %s"), edited_cave->name);
        gtk_window_set_title (GTK_WINDOW (gd_editor_window), title);
        g_free(title);
    } else {
        /* editing the caveset */
        title=g_strdup_printf(_("GDash Cave Editor - %s"), gd_caveset_data->name);
        gtk_window_set_title (GTK_WINDOW (gd_editor_window), title);
        g_free(title);
    }
}















/************************************
 * property editor
 *
 * edit properties of a C struct.
 *
 */
#define GDASH_TYPE "gdash-type"
#define GDASH_VALUE "gdash-value"
#define GDASH_DEFAULT_VALUE "gdash-default-value"

/* every "set to default" button has a list in its user data connected to the clicked signal.
   this list holds a number of widgets, which are to be set to defaults.
   the widgets themselves hold the type and value; and also a pointer to the default value.
 */
static void
edit_properties_set_default_button(GtkWidget *widget, gpointer data)
{
    GList *rowwidgets=(GList *)data;
    GList *iter;
    
    for (iter=rowwidgets; iter!=NULL; iter=iter->next) {
        gpointer defvalue=g_object_get_data (G_OBJECT(iter->data), GDASH_DEFAULT_VALUE);

        switch (GPOINTER_TO_INT (g_object_get_data (iter->data, GDASH_TYPE))) {
        case GD_TYPE_BOOLEAN:
            gtk_toggle_button_set_active (iter->data, *(gboolean *)defvalue);
            break;
        case GD_TYPE_STRING:
            gtk_entry_set_text (iter->data, (char *)defvalue);
            break;
        case GD_TYPE_INT:
        case GD_TYPE_RATIO:    /* ratio: % in bdcff, integer in editor */
            gtk_spin_button_set_value (iter->data, *(int *)defvalue);
            break;
        case GD_TYPE_PROBABILITY:
            gtk_spin_button_set_value (iter->data, (*(int *) defvalue)*100.0/1E6);    /* *100%, /1E6 as it is stored in parts per million */
            break;
        case GD_TYPE_ELEMENT:
        case GD_TYPE_EFFECT:    /* effects are also elements; bdcff file is different. */
            gd_element_button_set (iter->data, *(GdElement *) defvalue);
            break;
        case GD_TYPE_COLOR:
            gd_color_combo_set(iter->data, *(GdColor *) defvalue);
            break;
        }
        
    }
}

/* for a destroyed signal; frees a list which is its user data */
static void
edit_properties_free_list_in_user_data(GtkWidget *widget, gpointer data)
{
    g_list_free((GList *) data);
}


/* returns true if edited. */
/* str is the struct to be edited. */
/* def_str is another struct of the same type, which holds default values. */
/* prop_desc describes the structure. */
/* other_tags stores another fields to be shown. */
/* show_cancel: true, then a cancel button is also shown. */
/* highlight: if given, those properties will be highlighted. */
static gboolean
edit_properties (const char *title, gpointer str, gpointer def_str, const GdStructDescriptor *prop_desc, GHashTable *other_tags, gboolean show_cancel)
{
    GtkWidget *dialog, *notebook, *table=NULL;
    GList *widgets=NULL;
    int i, j, row=0;
    int result;
    GtkTextBuffer *textbuffer=NULL;
    
    dialog=gtk_dialog_new_with_buttons (title, GTK_WINDOW (gd_editor_window), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    if (show_cancel)
        gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

    /* tabbed notebook */
    notebook=gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG (dialog)->vbox), notebook, TRUE, TRUE, 6);

    /* create the entry widgets */
    for (i=0; prop_desc[i].identifier!=NULL; i++) {
        GtkWidget *widget=NULL, *label;
        GtkWidget *button;
        GList *rowwidgets;
        
        if (prop_desc[i].flags&GD_DONT_SHOW_IN_EDITOR)
            continue;
        
        if (prop_desc[i].type==GD_TAB) {
            /* create a table which will be the widget inside */
            table=gtk_table_new(1, 1, FALSE);
            gtk_container_set_border_width (GTK_CONTAINER (table), 12);
            gtk_table_set_row_spacings (GTK_TABLE(table), 3);
            /* and to the notebook */
            gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, gtk_label_new(gettext(prop_desc[i].name)));
            row=0;
            continue;
        }

        /* long string: has its own notebook tab */        
        if (prop_desc[i].type==GD_TYPE_LONGSTRING) {
            GtkWidget *view;
            GtkWidget *scroll;
            
            textbuffer=gtk_text_buffer_new(NULL);
            view=gtk_text_view_new_with_buffer(textbuffer);
            gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
            gtk_widget_set_tooltip_text(view, prop_desc[i].tooltip);
            
            gtk_text_buffer_insert_at_cursor(textbuffer, G_STRUCT_MEMBER(GString *, str, prop_desc[i].offset)->str, -1);

            /* a text view in its scroll windows, so it can be any large */
            scroll=gtk_scrolled_window_new(NULL, NULL);
            gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
            gtk_container_add(GTK_CONTAINER(scroll), view);
            gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_ETCHED_IN);
            
            /* has its own notebook tab */
            gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll, gtk_label_new(_(prop_desc[i].name)));

            widgets=g_list_prepend (widgets, view);
            g_object_set_data (G_OBJECT (view), GDASH_TYPE, GINT_TO_POINTER (prop_desc[i].type));
            g_object_set_data (G_OBJECT (view), GDASH_VALUE, G_STRUCT_MEMBER_P(str, prop_desc[i].offset));
            continue;
        }

        g_assert (table!=NULL);
        
        if (prop_desc[i].type==GD_LABEL) {
            /* create a label. */
            label=gd_label_new_printf("<b>%s</b>", _(prop_desc[i].name));
            gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
            gtk_table_attach(GTK_TABLE(table), label, 0, 7, row, row+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
            
            if (prop_desc[i].flags&GD_SHOW_LEVEL_LABEL) {
                int i=0;
                for (i=0; i<5; i++)
                    gtk_table_attach(GTK_TABLE(table), gd_label_new_printf_centered(_("Level %d"), i+1), i+2, i+3, row, row+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 3, 0);
            }
            row++;
            continue;
        }

        /* name of setting */
        label=gtk_label_new(_(prop_desc[i].name));
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        rowwidgets=NULL;
        for (j=0; j < prop_desc[i].count; j++) {
            gpointer value=G_STRUCT_MEMBER_P (str, prop_desc[i].offset);
            gpointer defpoint=G_STRUCT_MEMBER_P (def_str, prop_desc[i].offset);
            char *defval=NULL;
            char *tip;

            switch (prop_desc[i].type) {
            case GD_TAB:
            case GD_LABEL:
            case GD_TYPE_LONGSTRING:
                /* handled above */
                g_assert_not_reached();
                break;
            case GD_TYPE_BOOLEAN:
                value=((gboolean *) value) + j;
                defpoint=((gboolean *) defpoint) + j;
                widget=gtk_check_button_new();
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), *(gboolean *) value);
                defval=g_strdup (*(gboolean *)defpoint ? _("Yes") : _("No"));
                break;
            case GD_TYPE_STRING:
                value=((GdString *) value) + j;
                defpoint=((GdString *) defpoint) + j;
                widget=gtk_entry_new();
                /* little inconsistency below: max length has unicode characters, while gdstring will have utf-8.
                   however this does not make too much difference */
                gtk_entry_set_max_length(GTK_ENTRY(widget), sizeof(GdString));
                gtk_entry_set_text (GTK_ENTRY (widget), (char *) value);
                defval=NULL;
                break;
            case GD_TYPE_RATIO:
            case GD_TYPE_INT:
                value=((int *) value) + j;
                defpoint=((int *) defpoint) + j;
                widget=gtk_spin_button_new_with_range (prop_desc[i].min, prop_desc[i].max, 1);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), *(int *) value);
                defval=g_strdup_printf("%d", *(int *) defpoint);
                break;
            case GD_TYPE_PROBABILITY:
                value=((int *) value)+j;
                defpoint=((int *) defpoint)+j;
                widget=gtk_spin_button_new_with_range (0.0, 100.0, 0.001);
                gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), *(int *)value*100.0/1E6);    /* *100% /1E6, as it is stored in parts per million */
                defval=g_strdup_printf("%.1f%%", *(int *) defpoint*100.0/1E6);
                break;
            case GD_TYPE_EFFECT:    /* effects also specify elements; only difference is bdcff. */
            case GD_TYPE_ELEMENT:
                value=((GdElement *) value) + j;
                defpoint=((GdElement *) defpoint) + j;
                widget=gd_element_button_new(*(GdElement *) value, FALSE, NULL);
                defval=g_strdup_printf("%s", _(gd_elements[*(GdElement *) defpoint].name));
                break;
            case GD_TYPE_COLOR:
                value=((GdColor *) value) + j;
                defpoint=((GdColor *) defpoint) + j;
                widget=gd_color_combo_new(*(GdColor *) value);
                defval=g_strdup_printf("%s", _(gd_color_get_visible_name(*(GdColor *) defpoint)));
                break;
            case GD_TYPE_DIRECTION:
                value=((GdDirection *) value)+j;
                defpoint=((GdDirection *) defpoint) + j;
                widget=gd_direction_combo_new(*(GdDirection *) value);
                defval=g_strdup_printf("%s", _(gd_direction_get_visible_name(*(GdDirection *)defpoint)));
                break;
            case GD_TYPE_SCHEDULING:
                value=((GdScheduling *) value)+j;
                defpoint=((GdScheduling *) defpoint) + j;
                widget=gd_scheduling_combo_new(*(GdScheduling *) value);
                defval=g_strdup_printf("%s", _(gd_scheduling_get_visible_name(*(GdScheduling *)defpoint)));
                break;
            }
            /* put widget into list so values can be extracted later */
            widgets=g_list_prepend(widgets, widget);
            rowwidgets=g_list_prepend(rowwidgets, widget);
            g_object_set_data(G_OBJECT(widget), GDASH_TYPE, GINT_TO_POINTER (prop_desc[i].type));
            g_object_set_data(G_OBJECT(widget), GDASH_VALUE, value);
            g_object_set_data(G_OBJECT(widget), GDASH_DEFAULT_VALUE, defpoint);
            /* put widget to table */
            gtk_table_attach(GTK_TABLE(table), widget, j+2, (prop_desc[i].count==1) ? 7 : j+3, row, row+1, GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_SHRINK, 3, 0);
            if (prop_desc[i].tooltip)
                tip=g_strdup_printf(_("%s\nDefault value: %s"), gettext(prop_desc[i].tooltip), defval ? defval : _("empty"));
            else
                tip=g_strdup_printf(_("Default value: %s"), defval ? defval : _("empty"));
            g_free(defval);
            gtk_widget_set_tooltip_text(widget, tip);
            g_free(tip);
        }
        
        if (prop_desc[i].type!=GD_TYPE_STRING) {
            button=gtk_button_new();
            gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
            gtk_widget_set_tooltip_text(button, _("Set to default value"));
            gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
            g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK(edit_properties_set_default_button), rowwidgets);
            /* this will free the list when the button is destroyed */
            g_signal_connect(G_OBJECT (button), "destroy", G_CALLBACK(edit_properties_free_list_in_user_data), rowwidgets);
            gtk_table_attach(GTK_TABLE(table), button, 1, 2, row, row+1, GTK_SHRINK, GTK_SHRINK, 3, 0);
        } else
            g_list_free(rowwidgets);
                     
        row++;
    }
    
    if (other_tags) {
        GList *hashkeys, *iter;
    
        /* add unknown tags to the last table - XXX */
        hashkeys=g_hash_table_get_keys(other_tags);
        for (iter=hashkeys; iter!=NULL; iter=iter->next) {
            GtkWidget *label, *entry;
            gchar *key=(gchar *)iter->data;
            char *tooltip_text;
            
            /* name of setting */
            label=gtk_label_new(key);
            gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1, GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
            
            entry=gtk_entry_new();
            gtk_entry_set_text(GTK_ENTRY(entry), (char *) g_hash_table_lookup(other_tags, key));
            tooltip_text=g_strdup_printf(_("Value of tag '%s'. This tag is not known to GDash. Clear the text field to remove it."), key);
            gtk_widget_set_tooltip_text(entry, tooltip_text);
            g_free(tooltip_text);
            widgets=g_list_prepend (widgets, entry);
            g_object_set_data(G_OBJECT(entry), GDASH_TYPE, GINT_TO_POINTER (-1));
            g_object_set_data(G_OBJECT(entry), GDASH_VALUE, key);
            gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row+1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_SHRINK, 3, 0);
            row++;
        }
        g_list_free(hashkeys);
    }
    
    /* running the dialog */
    gtk_widget_show_all (dialog);
    result=gtk_dialog_run(GTK_DIALOG(dialog));

    /* getting data */
    if (result==GTK_RESPONSE_ACCEPT) {
        GList *iter;

        /* read values from different spin buttons and ranges etc */
        for (iter=widgets; iter; iter=g_list_next (iter)) {
            gpointer value=g_object_get_data(G_OBJECT(iter->data), GDASH_VALUE);
            GtkTextIter iter_start, iter_end;
            GtkTextBuffer *buffer;
            char *text;

            switch (GPOINTER_TO_INT (g_object_get_data (iter->data, GDASH_TYPE))) {
            case -1:    /* from hash table */
                if (!g_str_equal(gtk_entry_get_text(GTK_ENTRY(iter->data)), ""))
                    /* "value" here is the key; the entry stores the value. */
                    /* both need to be copies */
                    g_hash_table_insert(other_tags, g_strdup(value), g_strdup(gtk_entry_get_text(GTK_ENTRY(iter->data))));
                else
                    g_hash_table_remove(other_tags, value);
                break;
            case GD_TYPE_LONGSTRING:
                /* the text_buffer_get_text needs a start and end iterator */
                buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(iter->data));
                gtk_text_buffer_get_iter_at_offset(buffer, &iter_start, 0);
                gtk_text_buffer_get_iter_at_offset(buffer, &iter_end, -1);
                text=gtk_text_buffer_get_text(buffer, &iter_start, &iter_end, TRUE);
                g_string_assign(*(GString **)value, text);
                g_free(text);
                break;
            case GD_TYPE_BOOLEAN:
                *(gboolean *) value=gtk_toggle_button_get_active(iter->data);
                break;
            case GD_TYPE_STRING:
                gd_strcpy((char *) value, gtk_entry_get_text(iter->data));
                break;
            case GD_TYPE_INT:
                *(int *) value=gtk_spin_button_get_value_as_int(iter->data);
                break;
            case GD_TYPE_RATIO:
                *(int *) value=gtk_spin_button_get_value_as_int(iter->data);
                break;
            case GD_TYPE_PROBABILITY:
                *(int *) value=gtk_spin_button_get_value(iter->data)/100.0*1E6+0.5;    /* /100% *1E6 (probability) +0.5 for rounding */
                break;
            case GD_TYPE_ELEMENT:
            case GD_TYPE_EFFECT:    /* effects are also elements; bdcff file is different. */
                *(GdElement *) value=gd_element_button_get(iter->data);
                break;
            case GD_TYPE_COLOR:
                *(GdColor *) value=gd_color_combo_get_color(iter->data);
                break;
            case GD_TYPE_DIRECTION:
                *(GdDirection *) value=gd_direction_combo_get_direction(iter->data);
                break;
            case GD_TYPE_SCHEDULING:
                *(GdScheduling *) value=gd_scheduling_combo_get_scheduling(iter->data);
                break;
            }
        }
    }
    g_list_free(widgets);
    /* this destroys everything inside the notebook also */
    gtk_widget_destroy (dialog);
    
    /* the name of the cave or caveset may have been changed */
    editor_window_set_title();
    
    return result==GTK_RESPONSE_ACCEPT;
}
#undef GDASH_TYPE
#undef GDASH_VALUE
#undef GDASH_DEFAULT_VALUE



/* call edit_properties for a cave, then do some cleanup.
 * for example, if the size changed, the map has to be resized,
 * etc. also the user may be warned about resizing the visible area. */
static void
cave_properties(GdCave *cave, gboolean show_cancel)
{
    GdCave *def_val;
    int old_w, old_h;
    int i;
    gboolean edited;
    gboolean size_changed, full_visible;
    
    /* remember old size, as the cave map might have to be resized */
    old_w=cave->w;
    old_h=cave->h;

    /* check if full cave visible */
    full_visible=(cave->x1==0 && cave->y1==0 && cave->x2==old_w-1 && cave->y2==old_h-1);
        
    def_val=gd_cave_new();
    undo_save();
    edited=edit_properties(_("Cave Properties"), cave, def_val, gd_cave_properties, cave->tags, show_cancel);
    gd_cave_free(def_val);
    
    size_changed=(cave->w!=old_w || cave->h!=old_h);

    /* if the size changes, we have work to do. */
    if (edited && size_changed) {
        /* if cave has a map, resize it also */
        if (cave->map) {
            /* create new map, with the new sizes */
            GdElement **new_map=gd_cave_map_new(cave, GdElement);
            int minx=MIN(old_w, cave->w);
            int miny=MIN(old_h, cave->h);
            int x, y;

            /* default value is the same as the border */
            for (y=0; y<cave->h; y++)
                for (x=0; x<cave->w; x++)
                    new_map[y][x]=cave->initial_border;
            /* make up the new map - either the old or the new is smaller */
            for (y=0; y<miny; y++)
                for (x=0; x<minx; x++)
                    /* copy values from rendered cave. */
                    new_map[y][x]=gd_cave_get_rc(rendered_cave, x, y);
            gd_cave_map_free(cave->map);
            cave->map=new_map;
        }
        
        /* if originally the full cave was visible, we set the visible area to the full cave, here again. */
        if (full_visible) {
            cave->x2=cave->w-1;
            cave->y2=cave->h-1;
        }
        
        /* do some validation */
        gd_cave_correct_visible_size(cave);

        /* check ratios, so they are not larger than number of cells in cave (width*height) */
        for (i=0; gd_cave_properties[i].identifier!=NULL; i++)
            if (gd_cave_properties[i].type==GD_TYPE_RATIO) {
                int *value=G_STRUCT_MEMBER_P(cave, gd_cave_properties[i].offset);
                int j;
                
                for (j=0; j<gd_cave_properties[i].count; j++)
                    if (value[j]>=cave->w*cave->h)
                        value[j]=cave->w*cave->h;
            }
    }
    
    if (edited) {
        /* redraw, recreate, re-everything */
        select_cave_for_edit(cave);
        gd_caveset_edited=TRUE;
    }

    /* if the size of the cave changed, inform the user that he should check the visible region. */
    /* also, automatically select the tool. */    
    if (size_changed && !full_visible) {
        select_tool(VISIBLE_REGION);
        gd_warningmessage(_("You have changed the size of the cave. Please check the size of the visible region!"),
            _("The visible area of the cave during the game can be smaller than the whole cave. If you resize "
              "the cave, the area to be shown cannot be guessed automatically. The tool to set this behavior is "
              "now selected, and shows the current visible area. Use drag and drop to change the position and "
              "size of the rectangle."));
        
    }
}


/* edit the properties of the caveset - nothing special. */
static void
caveset_properties(gboolean show_cancel)
{
    GdCavesetData *def_val;
    gboolean edited;
    
    def_val=gd_caveset_data_new();
    edited=edit_properties(_("Cave Set Properties"), gd_caveset_data, def_val, gd_caveset_properties, NULL, show_cancel);
    gd_caveset_data_free(def_val);
    gd_main_window_set_title();
    editor_window_set_title();
    
    if (edited)
        gd_caveset_edited=TRUE;
}



/* select cave edit tool.
   this activates the gtk action in question, so
   toolbar and everything else will be updated */
static void
select_tool(int tool)
{
    int i;
    
    /* find specific object type in action_objects array. */
    /* (the array indexes and tool integers do not correspond) */
    for (i=0; i<G_N_ELEMENTS(action_objects); i++)
        if (tool==action_objects[i].value)
            gtk_action_activate (gtk_action_group_get_action (actions_edit_tools, action_objects[i].name));
}



static void
set_status_label_for_cave(GdCave *cave)
{
    char *name_escaped;
    
    name_escaped=g_markup_escape_text(cave->name, -1);
    gd_label_set_markup_printf(GTK_LABEL (label_object), _("<b>%s</b>, %s, %dx%d, time %d:%02d, diamonds %d"), name_escaped, cave->selectable?_("selectable"):_("not selectable"), cave->w, cave->h, cave->level_time[edit_level]/60, cave->level_time[edit_level]%60, cave->level_diamonds[edit_level]);
    g_free(name_escaped);
}


static gboolean
set_status_label_for_caveset_count_caves(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    int *i=(int *)data;
    
    (*i)++;
    return FALSE;    /* to continue counting */
}

static void
set_status_label_for_caveset()
{
    char *name_escaped;
    int count;
    
    if (iconview_cavelist) {
        count=0;
        gtk_tree_model_foreach(gtk_icon_view_get_model(GTK_ICON_VIEW(iconview_cavelist)), (GtkTreeModelForeachFunc) set_status_label_for_caveset_count_caves, &count);
    }
    else
        count=gd_caveset_count();
    
    name_escaped=g_markup_escape_text(gd_caveset_data->name, -1);
    gd_label_set_markup_printf(GTK_LABEL (label_object), _("<b>%s</b>, %d caves"), name_escaped, count);
    g_free(name_escaped);
}






/*
    render cave - ie. draw it as a map,
    so it can be presented to the user.
*/
static void
render_cave()
{
    int i;
    int x, y;
    GList *iter;
    GList *selected;
    g_return_if_fail(edited_cave!=NULL);

    gd_cave_free(rendered_cave);
    /* rendering cave for editor: seed=random, so the user sees which elements are truly random */
    rendered_cave=gd_cave_new_rendered(edited_cave, edit_level, g_random_int());
    /* create a gfx buffer for displaying */
    gd_cave_map_free(gfx_buffer);
    gd_cave_map_free(object_highlight_map);
    gfx_buffer=gd_cave_map_new(rendered_cave, int);
    object_highlight_map=gd_cave_map_new(rendered_cave, gboolean);
    for (y=0; y<rendered_cave->h; y++)
        for (x=0; x<rendered_cave->w; x++) {
            gfx_buffer[y][x]=-1;
            object_highlight_map[y][x]=FALSE;
        }

    /* fill object list store with the objects. */
    /* save previous selection to a list */
    selected=g_list_copy(selected_objects);
    
    gtk_list_store_clear(object_list);
    for (iter=edited_cave->objects, i=0; iter; iter=g_list_next (iter), i++) {
        GdObject *object=iter->data;
        GtkTreeIter treeiter;
        GdkPixbuf *element, *fillelement;
        gchar *text;
        const char *levels_stock;

        text=gd_object_get_coordinates_text (object);
        switch (object->type) {    /* for ELEMENT */
            case GD_COPY_PASTE:
                element=NULL; break;
            default:
                element=gd_get_element_pixbuf_with_border(object->element); break;
        }

        switch (object->type) {    /* for FILL ELEMENT */
            case GD_FILLED_RECTANGLE:
            case GD_JOIN:
            case GD_FLOODFILL_BORDER:
            case GD_FLOODFILL_REPLACE:
            case GD_MAZE:
            case GD_MAZE_UNICURSAL:
            case GD_MAZE_BRAID:
                fillelement=gd_get_element_pixbuf_with_border(object->fill_element);
                break;
            default:
                fillelement=NULL;
                break;
        }
        
        /* if on all levels */
        if (object->levels==GD_OBJECT_LEVEL_ALL)
            levels_stock=GD_ICON_OBJECT_ON_ALL;
        else
            /* not on all levels... visible on current level? */
            if (object->levels&gd_levels_mask[edit_level])
                levels_stock=GD_ICON_OBJECT_NOT_ON_ALL;
            else
                levels_stock=GD_ICON_OBJECT_NOT_ON_CURRENT;

        /* use atomic insert with values */
        gtk_list_store_insert_with_values (object_list, &treeiter, i, INDEX_COLUMN, i, LEVELS_PIXBUF_COLUMN, levels_stock, TYPE_PIXBUF_COLUMN, action_objects[object->type].stock_id, ELEMENT_PIXBUF_COLUMN, element, FILL_PIXBUF_COLUMN, fillelement, TEXT_COLUMN, text, POINTER_COLUMN, object, -1);

        /* also do selection as now we have the iter in hand */
        if (g_list_index(selected, object)!=-1)
            gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (object_list_tree_view)), &treeiter);
        g_free(text);
    }
    gtk_tree_view_columns_autosize (GTK_TREE_VIEW (object_list_tree_view));
    g_list_free(selected);

    gtk_action_group_set_sensitive (actions_edit_map, edited_cave->map!=NULL);
    gtk_action_group_set_sensitive (actions_edit_random, edited_cave->map==NULL);

    /* if no object is selected, show normal cave info */
    if (selected_objects==NULL)
        set_status_label_for_cave(edited_cave);
}






/*****************************************************
 *
 * spin button with instant update
 *
 */
#define GDASH_DATA_POINTER "gdash-data-pointer"
static void
spin_button_with_update_changed(GtkWidget *widget, gpointer data)
{
    int value=gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    int *pi;
    
    pi=g_object_get_data(G_OBJECT(widget), GDASH_DATA_POINTER);
    if (*pi!=value) {
        *pi=value;
        render_cave();
    }
}

static GtkWidget *
spin_button_with_update_new(int min, int max, int *value)
{
    GtkWidget *spin;
    
    /* change range if needed */
    if (*value<min) min=*value;
    if (*value>max) max=*value;
    
    spin=gtk_spin_button_new_with_range (min, max, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), *value);
    g_object_set_data(G_OBJECT(spin), GDASH_DATA_POINTER, value);
    g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(spin_button_with_update_changed), NULL);
    
    return spin;
}

/*****************************************************
 *
 * element button with instant update
 *
 */
static void
element_button_with_update_changed(GtkWidget *widget, gpointer data)
{
    GdElement *elem=g_object_get_data(G_OBJECT(widget), GDASH_DATA_POINTER);
    GdElement new_elem;
    
    new_elem=gd_element_button_get(widget);
    if (*elem!=new_elem) {
        *elem=new_elem;
        render_cave();
    }
}

static GtkWidget *
element_button_with_update_new(GdElement *value)
{
    GtkWidget *button;
    
    button=gd_element_button_new(*value, FALSE, NULL);
    g_object_set_data(G_OBJECT(button), GDASH_DATA_POINTER, value);
    /* this "clicked" will be called after the button's own, internal clicked signal */
    g_signal_connect(button, "clicked", G_CALLBACK(element_button_with_update_changed), NULL);

    return button;
}
#undef GDASH_DATA_POINTER

/*****************************************************
 *
 * check button with instant update
 *
 */
static void
check_button_with_update_changed(GtkWidget *widget, gpointer data)
{
    gboolean *value=(gboolean *) data;
    
    *value=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    render_cave();    
}

static GtkWidget *
check_button_with_update_new(gboolean *value)
{
    GtkWidget *button;
    
    button=gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), *value);
    g_signal_connect(button, "toggled", G_CALLBACK(check_button_with_update_changed), value);

    return button;
}



/*****************************************************
 *
 * a new hscale which switches level
 *
 */
static void
random_setup_level_scale_changed(GtkWidget *widget, gpointer data)
{
    int value=gtk_range_get_value(GTK_RANGE(widget));
    gtk_range_set_value(GTK_RANGE(level_scale), value);
}

static GtkWidget *
hscale_new_switches_level()
{
    GtkWidget *scale;
    
    scale=gtk_hscale_new_with_range (1.0, 5.0, 1.0);
    gtk_range_set_value(GTK_RANGE(scale), edit_level+1);    /* internally we use level 0..4 */
    gtk_scale_set_digits(GTK_SCALE(scale), 0);
    gtk_scale_set_value_pos (GTK_SCALE(scale), GTK_POS_LEFT);
    g_signal_connect(G_OBJECT(scale), "value-changed", G_CALLBACK(random_setup_level_scale_changed), NULL);
    
    return scale;
}

/*****************************************************
 *
 * add a callback to a spin button's "focus in" so it updates the level hscale
 *
 */
#define GDASH_LEVEL_NUMBER "gdash-level-number"
static void
widget_focus_in_set_level_cb(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
    int value=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), GDASH_LEVEL_NUMBER));
    gtk_range_set_value(GTK_RANGE(data), value);
}

static void
widget_focus_in_add_level_set_hander(GtkWidget *spin, GtkScale *scale, int level)
{
    g_object_set_data(G_OBJECT(spin), GDASH_LEVEL_NUMBER, GINT_TO_POINTER(level));
    gtk_widget_add_events(spin, GDK_FOCUS_CHANGE_MASK);
    g_signal_connect(spin, "focus-in-event", G_CALLBACK(widget_focus_in_set_level_cb), scale);
}
#undef GDASH_LEVEL_NUMBER








/****************************************************/
#define GDASH_SPIN_PROBABILITIES "gdash-spin-probabilities"

static gboolean
random_setup_da_expose (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    int *pprob=(int *)data;
    int fill[256];
    int i, j;
    
    if (!widget->window)
        return TRUE;

    for (j=0; j<256; j++)
        fill[j]=4;    /* for initial fill */
    for (i=0; i<4; i++)
        for (j=0; j<pprob[i]; j++)
            fill[j]=i;    /* random fill no. i */

    /* draw 256 small vertical lines with the color needed */
    for (j=0; j<256; j++) {
        gdk_draw_line(widget->window, widget->style->fg_gc[fill[j]], j*2, 0, j*2, 15);    
        gdk_draw_line(widget->window, widget->style->fg_gc[fill[j]], j*2+1, 0, j*2+1, 15);    
    }
    return TRUE;
}

static GtkWidget *scales[4];

#define GDASH_DATA_POINTER "gdash-data-pointer"
static void
random_setup_probability_scale_changed(GtkWidget *widget, gpointer data)
{
    int *pprob;    /* all 4 values */
    int i,j;
    gboolean changed;
    
    pprob=g_object_get_data(G_OBJECT(widget), GDASH_SPIN_PROBABILITIES);
    
    /* as probabilites take precedence over each other, set them in a way that the user sees it. */
    /* this way, the scales move on their own sometimes */
    changed=FALSE;
    for(i=0; i<4; i++)
        for (j=i+1; j<4; j++)
            if (pprob[j]>pprob[i]) {
                changed=TRUE;
                pprob[i]=pprob[j];
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(scales[i]), pprob[i]);
            }
    if (changed)
        render_cave();
    
    /* redraw color bars and re-render cave */
    gtk_widget_queue_draw(GTK_WIDGET(data));
}

/* the small color "icons" which tell the user which color corresponds to which element */
static gboolean
random_setup_fill_color_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    if (widget->window) {
        GtkStyle *style;

        style=gtk_widget_get_style (widget);

        gdk_draw_rectangle(widget->window, style->bg_gc[GTK_STATE_NORMAL], TRUE, event->area.x, event->area.y, event->area.width, event->area.height);
    }

    return TRUE;
}


/* parent: parent window
   pborder: pointer to initial border
   pinitial: pointer to initial fill
   pfill: pointer to random_fill[4]
   pprob: pointer to random_fill_probability[4]
   pseed: pointer to random seed[5]
 */
 
static void
random_setup_widgets_to_table(GtkTable *table, int firstrow, GdElement *pborder, GdElement *pinitial, GdElement *pfill, int *pprob, int *pseed)
{
    int i, row;
    GtkWidget *da, *align, *frame, *spin, *label, *wid;
    GtkWidget *scale;
    const unsigned int cols[]={ 0xffff59, 0x59ffac, 0x5959ff, 0xff59ac, 0x000000 };

    row=firstrow;
    /* drawing area, which shows the portions of elements. */
    da=gtk_drawing_area_new();
    gtk_widget_set_size_request(da, 512, 16);
    g_signal_connect(G_OBJECT(da), "expose-event", G_CALLBACK(random_setup_da_expose), pprob);
    frame=gtk_frame_new(NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(frame), da);
    align=gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(align), frame);

    /* five random seed spin buttons. */
    if (pseed) {
        /* scale which sets the level1..5 shown - only if we also draw the random seed buttons */
        gtk_table_attach(table, gd_label_new_printf(_("Level shown")), 0, 1, row, row+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        scale=hscale_new_switches_level();
        gtk_table_attach_defaults(table, scale, 1, 3, row, row+1);
        row++;

        for (i=0; i<5; i++) {
            spin=spin_button_with_update_new(-1, 255, &pseed[i]);
            gtk_widget_set_tooltip_text(spin, _("Random seed value controls the predictable random number generator, which fills the cave initially. If set to -1, cave is totally random every time it is played."));
            widget_focus_in_add_level_set_hander(spin, GTK_SCALE(scale), i+1);

            gtk_table_attach(table, gd_label_new_printf(_("Random seed %d"), i+1), 0, 1, row, row+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
            gtk_table_attach_defaults(table, spin, 1, 3, row, row+1);

            row++;
        }

        gtk_table_attach_defaults(table, gtk_hseparator_new(), 0, 3, row, row+1);
        row++;
    }
    
    /* label */
    if (pborder) {
        gtk_table_attach(table, gd_label_new_printf(_("Initial border")), 0, 1, row, row+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(table, element_button_with_update_new(pborder), 1, 2, row, row+1);
        row++;
    }
    
    /* five rows: four random elements + one initial fill */
    for (i=4; i>=0; i--) {
        GtkWidget *colorbox, *frame, *hbox;
        GdkColor color;
        GdElement *pelem;

        color.red=(cols[i] >> 16)<<8;
        color.green=(cols[i] >> 8)<<8;
        color.blue=(cols[i] >> 0)<<8;
        
        gtk_widget_modify_fg (da, i, &color);    /* set fg color[i] for big drawing area */

        hbox=gtk_hbox_new(FALSE, 6);

        /* label */
        if (i==4)
            label=gd_label_new_printf(_("Initial fill"));
        else
            label=gd_label_new_printf(_("Random fill %d"), 4-i);
        gtk_box_pack_start_defaults(GTK_BOX(hbox), label);

        /* small drawing area with only one color */
        colorbox=gtk_drawing_area_new();
        gtk_widget_set_size_request(colorbox, 16, 16);
        gtk_widget_modify_bg (colorbox, GTK_STATE_NORMAL, &color);
        g_signal_connect(G_OBJECT(colorbox), "expose-event", G_CALLBACK(random_setup_fill_color_expose), NULL);

        frame=gtk_frame_new(NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
        gtk_container_add(GTK_CONTAINER(frame), colorbox);
        gtk_box_pack_end(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
        gtk_table_attach(table, hbox, 0, 1, row, row+1, GTK_FILL|GTK_SHRINK, GTK_SHRINK, 0, 0);


        /* element button */        
        pelem=(i==4)?pinitial:&(pfill[i]);
        wid=element_button_with_update_new(pelem);
        gtk_table_attach_defaults(table, wid, 1, 2, row, row+1);
        
        /* probability control, if this row is not for the initial fill */
        if (i!=4) {
            int *value=&(pprob[i]);    /* pointer to integer value to modify */
            
            wid=spin_button_with_update_new(0, 255, value);
            scales[i]=wid;
            g_object_set_data(G_OBJECT(wid), GDASH_SPIN_PROBABILITIES, pprob);
            g_signal_connect(wid, "value-changed", G_CALLBACK(random_setup_probability_scale_changed), da);    /* spin button updates color bars */
            gtk_table_attach_defaults(table, wid, 2, 3, row, row+1);
        }
        
        row++;
    }

    /* attach the drawing area for color bars - this gives the user a hint about the ratio of elements */
    row++;
    gtk_table_attach(table, align, 0, 3, row, row+1, GTK_SHRINK, GTK_SHRINK, 0, 0);
}
#undef GDASH_DATA_POINTER



















/***************************************************
 *
 * OBJECT_PROPERTIES
 *
 * edit properties of a cave drawing object.
 *
 */

/* these two functions implement a check box, which also selects
 * the level to view. every check box created remembers which level
 * it handles, and also they store pointers to the bitmask to update
 * automatically. */
#define GDASH_LEVEL_NUMBER "gdash-level-number"
#define GDASH_DATA_POINTER "gdash-data-pointer"
static void
check_button_level_toggled(GtkWidget *widget, gpointer data)
{
    gboolean current=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    GdObjectLevels *levels=g_object_get_data(G_OBJECT(widget), GDASH_DATA_POINTER);
    int level=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), GDASH_LEVEL_NUMBER));
    
    if (current) /* true */
        *levels=*levels | gd_levels_mask[level-1];
    else
        *levels=*levels & ~gd_levels_mask[level-1];
    render_cave();
}

static GtkWidget *
check_button_new_level_enable(GdObjectLevels *levels, int level)
{
    GtkWidget *check;
    char s[20];
    
    g_snprintf(s, sizeof(s), "%d", level);
    check=gtk_check_button_new_with_label(s);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), (*levels & gd_levels_mask[level-1])!=0);
    g_object_set_data(G_OBJECT(check), GDASH_LEVEL_NUMBER, GINT_TO_POINTER(level));
    g_object_set_data(G_OBJECT(check), GDASH_DATA_POINTER, levels);
    g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(check_button_level_toggled), NULL);
    
    return check;
}
#undef GDASH_LEVEL_NUMBER
#undef GDASH_DATA_POINTER


/* edit properties of an object */
static void
object_properties(GdObject *object)
{
    GtkWidget *dialog, *table, *hbox, *scale;
    int i=0;
    int n;
    int result;

    if (object==NULL) {
        g_return_if_fail(object_list_count_selected()==1);
        object=object_list_first_selected();    /* select first from list... should be only one selected */
    }

    dialog=gtk_dialog_new_with_buttons (_("Object Properties"), GTK_WINDOW (gd_editor_window), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    table=gtk_table_new(1, 3, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (table), 6);
    gtk_table_set_row_spacings (GTK_TABLE(table), 6);
    gtk_table_set_col_spacings (GTK_TABLE(table), 6);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG (dialog)->vbox), table, FALSE, FALSE, 0);

    /* i is the row in gtktable, where the label and value is placed.
     * then i is incremented. this is to avoid empty lines. the table
     * seemed nicer than hboxes. and it expands automatically. */

    /* object type */
    hbox=gtk_hbox_new(FALSE, 6);
    gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_("Type")), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 3, i, i+1);
    gtk_box_pack_start(GTK_BOX(hbox), gtk_image_new_from_stock(action_objects[object->type].stock_id, GTK_ICON_SIZE_MENU), FALSE, FALSE, 0);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), gd_label_new_printf(_(gd_object_description[object->type].name)));
    i++;

    /* LEVEL INFO ****************/
    /* hscale which selects current level shown */
    gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_("Level currently shown")), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
    scale=hscale_new_switches_level();
    gtk_table_attach(GTK_TABLE(table), scale, 1, 3, i, i+1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    /* separator */
    gtk_table_attach(GTK_TABLE(table), gtk_hseparator_new(), 0, 3, i, i+1, GTK_FILL, GTK_SHRINK, 0, 0);
    i++;

    /* levels checkboxes */
    hbox=gtk_hbox_new(TRUE, 6);
    gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_("Enabled on levels")), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 3, i, i+1);
    for (n=0; n<5; n++) {
        GtkWidget *wid;
        
        wid=check_button_new_level_enable(&object->levels, n+1);
        widget_focus_in_add_level_set_hander(wid, GTK_SCALE(scale), n+1);
        gtk_box_pack_start_defaults(GTK_BOX(hbox), wid);
    }
    i++;
    
    /* OBJECT PROPERTIES ************/
    if (gd_object_description[object->type].x1!=NULL) {
        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].x1)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), spin_button_with_update_new(0, edited_cave->w-1, &object->x1), 1, 2, i, i+1);
        gtk_table_attach_defaults(GTK_TABLE(table), spin_button_with_update_new(0, edited_cave->h-1, &object->y1), 2, 3, i, i+1);
        i++;
    }

    /* points only have one coordinate. for other elements, */
    if (gd_object_description[object->type].x2!=NULL) {
        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].x2)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), spin_button_with_update_new(0, edited_cave->w-1, &object->x2), 1, 2, i, i+1);
        gtk_table_attach_defaults(GTK_TABLE(table), spin_button_with_update_new(0, edited_cave->w-1, &object->y2), 2, 3, i, i+1);
        i++;
    }

    /* mazes have horiz */
    if (gd_object_description[object->type].horiz!=NULL) {
        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].horiz)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), spin_button_with_update_new(0, 100, &object->horiz), 1, 3, i, i+1);
        i++;
    }

    /* every element has an object parameter */
    if (gd_object_description[object->type].element!=NULL) {
        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].element)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), element_button_with_update_new(&object->element), 1, 3, i, i+1);
        i++;
    }
    
    /* joins and filled rectangles have a second object, but with different meaning */
    if (gd_object_description[object->type].fill_element!=NULL) {
        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].fill_element)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), element_button_with_update_new(&object->fill_element), 1, 3, i, i+1);
        i++;
    }

    /* rasters and joins have distance parameters */
    if (gd_object_description[object->type].dx!=NULL) {
        int rangemin=-40;
        
        /* for rasters and mazes, dx>=1 and dy>=1. for joins, these can be zero or even negative */
        if (object->type==GD_RASTER || object->type==GD_MAZE)
            rangemin=1;

        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].dx)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), spin_button_with_update_new(rangemin, 40, &object->dx), 1, 2, i, i+1);
        gtk_table_attach_defaults(GTK_TABLE(table), spin_button_with_update_new(rangemin, 40, &object->dy), 2, 3, i, i+1);
        i++;
    }

    /* mazes and random fills have seed params */
    if (gd_object_description[object->type].seed!=NULL) {
        int j;
        
        for (j=0; j<5; j++) {
            GtkWidget *spin;
            
            gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].seed), j+1), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
            gtk_table_attach_defaults(GTK_TABLE(table), spin=spin_button_with_update_new(-1, 1<<24, &object->seed[j]), 1, 3, i, i+1);
            widget_focus_in_add_level_set_hander(spin, GTK_SCALE(scale), j+1);
            gtk_widget_set_tooltip_text(spin, _("Random seed value controls the predictable random number generator. If set to -1, cave is totally random every time it is played."));
        i++;
        }
    }

    /* random fill object has a c64 random toggle */
    if (gd_object_description[object->type].c64_random!=NULL) {
        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].c64_random)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), check_button_with_update_new(&object->c64_random), 1, 3, i, i+1);
        i++;
    }

    /* mirror */
    if (gd_object_description[object->type].mirror!=NULL) {
        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].mirror)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), check_button_with_update_new(&object->mirror), 1, 3, i, i+1);
        i++;
    }

    /* flip */
    if (gd_object_description[object->type].flip!=NULL) {
        gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_object_description[object->type].flip)), 0, 1, i, i+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);
        gtk_table_attach_defaults(GTK_TABLE(table), check_button_with_update_new(&object->flip), 1, 3, i, i+1);
        i++;
    }

    if (object->type==GD_RANDOM_FILL)
        /* pborder (first) and pseed (last) parameter are not needed here */
        random_setup_widgets_to_table(GTK_TABLE(table), i, NULL, &object->fill_element, object->random_fill, object->random_fill_probability, NULL);
    
    undo_save();
    gtk_widget_show_all (dialog);
    result=gtk_dialog_run(GTK_DIALOG(dialog));
    /* levels */
    if (object->levels==0) {
        object->levels=GD_OBJECT_LEVEL_ALL;
        
        gd_warningmessage(_("The object should be visible on at least one level."), _("Enabled this object on all levels."));
    }
    gtk_widget_destroy (dialog);

    if (result==GTK_RESPONSE_REJECT)
        undo_do_one_step();
    else
        render_cave();
}









/* this is the same scroll routine as the one used for the game. only the parameters are changed a bit. */
static void
drawcave_timeout_scroll(int player_x, int player_y)
{
    static int scroll_desired_x=0, scroll_desired_y=0;
    GtkAdjustment *adjustment;
    int scroll_center_x, scroll_center_y;
    int i;
    /* hystheresis size is this, multiplied by two. */
    int scroll_start_x=scroll_window->allocation.width/2-2*gd_cell_size_editor;
    int scroll_start_y=scroll_window->allocation.height/2-2*gd_cell_size_editor;
    int scroll_speed=gd_cell_size_editor/4;
    
    /* get the size of the window so we know where to place player.
     * first guess is the middle of the screen.
     * drawing_area->parent->parent is the viewport.
     * +cellsize/2 gets the stomach of player :) so the very center */
    scroll_center_x=player_x * gd_cell_size_editor + gd_cell_size_editor/2 - drawing_area->parent->parent->allocation.width/2;
    scroll_center_y=player_y * gd_cell_size_editor + gd_cell_size_editor/2 - drawing_area->parent->parent->allocation.height/2;

    /* HORIZONTAL */
    /* hystheresis function.
     * when scrolling left, always go a bit less left than player being at the middle.
     * when scrolling right, always go a bit less to the right. */
    adjustment=gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll_window));
    if (adjustment->value+scroll_start_x<scroll_center_x)
        scroll_desired_x=scroll_center_x-scroll_start_x;
    if (adjustment->value-scroll_start_x>scroll_center_x)
        scroll_desired_x=scroll_center_x+scroll_start_x;

    scroll_desired_x=CLAMP (scroll_desired_x, 0, adjustment->upper - adjustment->step_increment - adjustment->page_increment);
    if (adjustment->value<scroll_desired_x) {
        for (i=0; i<scroll_speed; i++)
            if (adjustment->value<scroll_desired_x)
                adjustment->value++;
        gtk_adjustment_value_changed(adjustment);
    }
    if (adjustment->value>scroll_desired_x) {
        for (i=0; i<scroll_speed; i++)
            if (adjustment->value>scroll_desired_x)
                adjustment->value--;
        gtk_adjustment_value_changed(adjustment);
    }

    /* VERTICAL */
    adjustment=gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scroll_window));
    if (adjustment->value+scroll_start_y<scroll_center_y)
        scroll_desired_y=scroll_center_y-scroll_start_y;
    if (adjustment->value-scroll_start_y>scroll_center_y)
        scroll_desired_y=scroll_center_y+scroll_start_y;

    scroll_desired_y=CLAMP (scroll_desired_y, 0, adjustment->upper - adjustment->step_increment - adjustment->page_increment);
    if (adjustment->value<scroll_desired_y) {
        for (i=0; i<scroll_speed; i++)
            if (adjustment->value<scroll_desired_y)
                adjustment->value++;
        gtk_adjustment_value_changed(adjustment);
    }
    if (adjustment->value>scroll_desired_y) {
        for (i=0; i<scroll_speed; i++)
            if (adjustment->value>scroll_desired_y)
                adjustment->value--;
        gtk_adjustment_value_changed(adjustment);
    }
}


/* timeout 'interrupt', drawing cave in cave editor. */
static gboolean
drawing_area_draw_timeout(gpointer data)
{
    static int animcycle=0;
    static int player_blinking=0;
    static int cursor=0;
    int x, y, draw;

    /* if nothing to draw or nowhere to draw :) exit.
     * this is necessary as the interrupt is not uninstalled when the selector is running. */
    if (!drawing_area || !rendered_cave)
        return TRUE;

    g_return_val_if_fail (gfx_buffer!=NULL, TRUE);

    /* when mouse over a drawing object, cursor changes */
    if (mouse_x >= 0 && mouse_y >= 0) {
        int new_cursor=rendered_cave->objects_order[mouse_y][mouse_x] ? GDK_HAND1 : -1;
        if (cursor!=new_cursor) {
            cursor=new_cursor;
            gdk_window_set_cursor (drawing_area->window, cursor==-1?NULL:gdk_cursor_new(cursor));
        }
    }

    /* only do cell animations when window is active.
     * otherwise... user is testing the cave, animation would just waste cpu. */
    if (gtk_window_has_toplevel_focus (GTK_WINDOW (gd_editor_window)))
        animcycle=(animcycle+1) & 7;

    if (animcycle==0)            /* player blinking is started at the beginning of animation sequences. */
        player_blinking=g_random_int_range (0, 4)==0;    /* 1/4 chance of blinking, every sequence. */

    for (y=0; y<rendered_cave->h; y++) {
        for (x=0; x<rendered_cave->w; x++) {
            if (gd_game_view)
                draw=gd_elements[rendered_cave->map[y][x]].image_simple;
            else
                draw=gd_elements[rendered_cave->map[y][x]].image;
            /* special case is player - sometimes blinking :) */
            if (player_blinking && (rendered_cave->map[y][x]==O_INBOX))
                draw=gd_elements[O_PLAYER_BLINK].image_simple;
            /* the biter switch also shows its state */
            if (rendered_cave->map[y][x]==O_BITER_SWITCH)
                draw=gd_elements[O_BITER_SWITCH].image_simple+rendered_cave->biter_delay_frame;

            /* negative value means animation */
            if (draw<0)
                draw=-draw+animcycle;

            /* object coloring */
            if (action==VISIBLE_REGION) {
                /* if showing visible region, different color applies for: */
                if (x>=rendered_cave->x1 && x<=rendered_cave->x2 && y>=rendered_cave->y1 && y<=rendered_cave->y2)
                    draw+=NUM_OF_CELLS;
                if (x==rendered_cave->x1 || x==rendered_cave->x2 || y==rendered_cave->y1 || y==rendered_cave->y2)
                    draw+=NUM_OF_CELLS;    /* once again */
            } else {
                if (object_highlight_map[y][x]) /* if it is a selected object, make it colored */
                    draw+=2*NUM_OF_CELLS;
                else if (gd_colored_objects && rendered_cave->objects_order[y][x]!=NULL)
                    /* if it belongs to any other element, make it colored a bit */
                    draw+=NUM_OF_CELLS;
            }
            
            /* the drawing itself */
            if (gfx_buffer[y][x]!=draw) {
                gdk_draw_drawable(drawing_area->window, drawing_area->style->black_gc, gd_editor_pixmap(draw), 0, 0, x*gd_cell_size_editor, y*gd_cell_size_editor, gd_cell_size_editor, gd_cell_size_editor);
                gfx_buffer[y][x]=draw;
            }
            
            /* for fill objects, we show their origin */    
            if (object_highlight_map[y][x]
                && (((GdObject *)rendered_cave->objects_order[y][x])->type==GD_FLOODFILL_BORDER
                    || ((GdObject *)rendered_cave->objects_order[y][x])->type==GD_FLOODFILL_REPLACE)) {
                GdObject *object=rendered_cave->objects_order[y][x];
                int x=object->x1;
                int y=object->y1;

                /* only draw if inside bounds */
                if (x>=0 && x<rendered_cave->w && y>=0 && y<rendered_cave->h) {
                    gdk_draw_rectangle(drawing_area->window, drawing_area->style->white_gc, FALSE, x*gd_cell_size_editor, y*gd_cell_size_editor, gd_cell_size_editor-1, gd_cell_size_editor-1);
                    gdk_draw_line(drawing_area->window, drawing_area->style->white_gc, x*gd_cell_size_editor, y*gd_cell_size_editor, (x+1)*gd_cell_size_editor-1, (y+1)*gd_cell_size_editor-1);
                    gdk_draw_line(drawing_area->window, drawing_area->style->white_gc, x*gd_cell_size_editor, (y+1)*gd_cell_size_editor-1, (x+1)*gd_cell_size_editor-1, y*gd_cell_size_editor);
                    gfx_buffer[object->y1][object->x1]=-1;
                }
            }
        }
    }

    if (mouse_x>=0 && mouse_y>=0) {
        /* this is the cell the mouse is over */
        gdk_draw_rectangle(drawing_area->window, drawing_area->style->white_gc, FALSE, mouse_x*gd_cell_size_editor, mouse_y*gd_cell_size_editor, gd_cell_size_editor-1, gd_cell_size_editor-1);
        /* always redraw this cell the next frame - the easiest way to always get rid of the rectangle when the mouse is moved */
        gfx_buffer[mouse_y][mouse_x]=-1;
    }

    /* automatic scrolling */
    if (mouse_x>=0 && mouse_y>=0 && button1_clicked)
        drawcave_timeout_scroll(mouse_x, mouse_y);

    return TRUE;
}

/*
 *cave drawing area expose event.
 */
static gboolean
drawing_area_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    int x, y, x1, y1, x2, y2;

    if (!widget->window || gfx_buffer==NULL || rendered_cave==NULL)
        return FALSE;

    /* calculate which area to update */
    x1=event->area.x/gd_cell_size_editor;
    y1=event->area.y/gd_cell_size_editor;
    x2=(event->area.x+event->area.width-1)/gd_cell_size_editor;
    y2=(event->area.y+event->area.height-1)/gd_cell_size_editor;
    /* when resizing the cave, we may get events which store the old size, if the drawing area is not yet resized. */
    if (x1<0) x1=0;
    if (y1<0) x1=0;
    if (x2>=rendered_cave->w) x2=rendered_cave->w-1;
    if (y2>=rendered_cave->h) y2=rendered_cave->h-1;

    for (y=y1; y<=y2; y++)
        for (x=x1; x<=x2; x++)
            if (gfx_buffer[y][x]!=-1)
                gdk_draw_drawable(drawing_area->window, drawing_area->style->black_gc, gd_editor_pixmap(gfx_buffer[y][x]), 0, 0, x*gd_cell_size_editor, y*gd_cell_size_editor, gd_cell_size_editor, gd_cell_size_editor);
    return TRUE;
}




/***************************************************
 *
 * mouse events
 *
 *
 */

static int new_object_mask()
{
    int new_object_mask=new_objects_visible_on[gtk_combo_box_get_active(GTK_COMBO_BOX(new_object_level_combo))].mask;
    if (new_object_mask==0) /* 0 means - current level */
        new_object_mask=gd_levels_mask[edit_level];
    return new_object_mask;
}

static void add_object_to_edited_cave(GdObject *new_object)
{
    undo_save();
    edited_cave->objects=g_list_append(edited_cave->objects, new_object);
    render_cave();        /* new object created, so re-render cave */
    object_list_select_one_object(new_object);    /* also make it selected; so it can be edited further */
}

/* mouse button press event */
static gboolean
drawing_area_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    g_return_val_if_fail (edited_cave!=NULL, FALSE);

    /* right click opens popup */
    if (event->button==3) {
        gtk_menu_popup(GTK_MENU(drawing_area_popup), NULL, NULL, NULL, NULL, event->button, event->time);
        return TRUE;
    }

    /* this should be also false for doubleclick! so we do not do if (event->tye....) */    
    button1_clicked=event->type==GDK_BUTTON_PRESS && event->button==1;    
    
    clicked_x=((int) event->x)/gd_cell_size_editor;
    clicked_y=((int) event->y)/gd_cell_size_editor;
    /* middle button picks element from screen */
    if (event->button==2) {
        if (event->state & GDK_CONTROL_MASK)
            gd_element_button_set (fillelement_button, gd_cave_get_rc(rendered_cave, clicked_x, clicked_y));
        else if (event->state & GDK_SHIFT_MASK) {
            GdObjectType type;

            if (rendered_cave->objects_order[clicked_y][clicked_x])
                /* if there is an object, get type. if not, action defaults to move */
                type=((GdObject *) rendered_cave->objects_order[clicked_y][clicked_x])->type;
            else
                type=MOVE;

            select_tool(type);
        }
        else
            gd_element_button_set (element_button, gd_cave_get_rc(rendered_cave, clicked_x, clicked_y));
        return TRUE;
    }
    
    /* we do not handle anything other than buttons3,2 above, and button 1 */
    if (event->button!=1)
        return FALSE;

    /* if double click, open element properties window.
     * if no element selected, open cave properties window.
     * (if mouse if over an element, the first click selected it.) */
    /* do not allow this doubleclick for visible region mode as that one does not select objects */
    if (event->type==GDK_2BUTTON_PRESS && action!=VISIBLE_REGION) {
        if (rendered_cave->objects_order[clicked_y][clicked_x])
            object_properties (rendered_cave->objects_order[clicked_y][clicked_x]);
        else
            cave_properties (edited_cave, TRUE);
        return TRUE;
    }

    switch (action) {
        case MOVE:
            /* action=move: now the user is selecting an object by the mouse click */
            if ((event->state & GDK_CONTROL_MASK) || (event->state & GDK_SHIFT_MASK)) {
                /* CONTROL or SHIFT PRESSED: multiple selection */
                /* if control-click on a non-object, do nothing. */
                if (rendered_cave->objects_order[clicked_y][clicked_x]) {
                    if (object_highlight_map[clicked_y][clicked_x])        /* <- hackish way of checking if the clicked object is currently selected */
                        object_list_remove_from_selection(rendered_cave->objects_order[clicked_y][clicked_x]);
                    else
                        object_list_add_to_selection(rendered_cave->objects_order[clicked_y][clicked_x]);
                }
            } else {
                /* CONTROL NOT PRESSED: single selection */
                /* if the object clicked is not currently selected, we select it. if it is, do nothing, so a multiple selection remains. */
                if (rendered_cave->objects_order[clicked_y][clicked_x]==NULL) {
                    /* if clicking on a non-object, deselect all */
                    object_list_clear_selection();
                } else
                if (!object_highlight_map[clicked_y][clicked_x])        /* <- hackish way of checking if the clicked object is currently non-selected */
                    object_list_select_one_object(rendered_cave->objects_order[clicked_y][clicked_x]);
            }
            
            /* prepare for undo */
            undo_move_flag=FALSE;
            break;
            
        case FREEHAND:
            /* freehand tool: draw points in each place. */
            /* if already the same element there, which is placed by an object, skip! */
            if (gd_cave_get_rc(rendered_cave, clicked_x, clicked_y)!=gd_element_button_get(element_button) || rendered_cave->objects_order[clicked_y][clicked_x]==NULL) {
                GdObject *new_object;

                /* save undo only on every new click; dragging the mouse will not save it */
                undo_save();

                /* freehand places points */                
                new_object=gd_object_new_point(new_object_mask(), clicked_x, clicked_y, gd_element_button_get(element_button));
                edited_cave->objects=g_list_append(edited_cave->objects, new_object);
                render_cave();        /* new object created, so re-render cave */
                object_list_select_one_object(new_object);    /* also make it selected; so it can be edited further */
            }
            break;
            
        case VISIBLE_REGION:
            /* new click... prepare for undo! */
            undo_move_flag=FALSE;
            /* do nothing, the motion event will matter */
            break;
            
        case GD_POINT:
            add_object_to_edited_cave(gd_object_new_point(new_object_mask(), clicked_x, clicked_y, gd_element_button_get(element_button)));
            break;
            
        case GD_LINE:
            add_object_to_edited_cave(gd_object_new_line(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, gd_element_button_get(element_button)));
            break;

        case GD_RECTANGLE:
            add_object_to_edited_cave(gd_object_new_rectangle(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, gd_element_button_get(element_button)));
            break;

        case GD_FILLED_RECTANGLE:
            add_object_to_edited_cave(gd_object_new_filled_rectangle(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, gd_element_button_get(element_button),
                gd_element_button_get(fillelement_button)));
            break;

        case GD_RASTER:
            add_object_to_edited_cave(gd_object_new_raster(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, 2, 2, gd_element_button_get(element_button)));
            break;

        case GD_JOIN:
            add_object_to_edited_cave(gd_object_new_join(new_object_mask(), 0, 0, gd_element_button_get(element_button), gd_element_button_get(fillelement_button)));
            break;

        case GD_FLOODFILL_REPLACE:
            add_object_to_edited_cave(gd_object_new_floodfill_replace(new_object_mask(), clicked_x, clicked_y, gd_element_button_get(fillelement_button), rendered_cave->map[clicked_y][clicked_x]));
            break;

        case GD_FLOODFILL_BORDER: 
            add_object_to_edited_cave(gd_object_new_floodfill_border(new_object_mask(), clicked_x, clicked_y, gd_element_button_get(fillelement_button), gd_element_button_get(element_button)));
            break;

        case GD_MAZE:
            {
                const gint32 seeds[5]={0, 0, 0, 0, 0};
                add_object_to_edited_cave(gd_object_new_maze(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, 1, 1, gd_element_button_get(element_button), gd_element_button_get(fillelement_button), 50, seeds));
            }
            break;
            
        case GD_MAZE_BRAID:
            {
                const gint32 seeds[5]={0, 0, 0, 0, 0};
                add_object_to_edited_cave(gd_object_new_maze_braid(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, 1, 1, gd_element_button_get(element_button), gd_element_button_get(fillelement_button), 50, seeds));
            }
            break;
            
        case GD_MAZE_UNICURSAL:
            {
                const gint32 seeds[5]={0, 0, 0, 0, 0};
                add_object_to_edited_cave(gd_object_new_maze_unicursal(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, 1, 1, gd_element_button_get(element_button), gd_element_button_get(fillelement_button), 50, seeds));
            }
            break;

        case GD_RANDOM_FILL:
            {
                const gint32 seeds[5]={0, 0, 0, 0, 0};
                const GdElement elems[4]={gd_element_button_get(element_button), O_DIRT, O_DIRT, O_DIRT};
                const gint32 probs[4]={32, 0, 0, 0};
                add_object_to_edited_cave(gd_object_new_random_fill(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, seeds, gd_element_button_get(fillelement_button), elems, probs, O_NONE, FALSE));
            }
            break;
            
        case GD_COPY_PASTE:
            add_object_to_edited_cave(gd_object_new_copy_paste(new_object_mask(), clicked_x, clicked_y, clicked_x, clicked_y, clicked_x, clicked_y, FALSE, FALSE));
            break;
            
        default:
            g_assert_not_reached();
    }
    return TRUE;
}

static gboolean
drawing_area_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    if (event->type==GDK_BUTTON_RELEASE && event->button==1)
        button1_clicked=FALSE;
    return TRUE;
}

/* mouse leaves drawing area event */
static gboolean
drawing_area_leave_event(GtkWidget *widget, GdkEventCrossing * event, gpointer data)
{
    /* do not check if it as enter event, as we did not connect that one. */
    gtk_label_set_text(GTK_LABEL(label_coordinate), "[x:   y:   ]");
    mouse_x=-1;
    mouse_y=-1;
    return FALSE;
}

/* mouse motion event */
static gboolean
drawing_area_motion_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    int x, y, dx, dy;
    GdkModifierType state;

    /* just to be sure. */
    if (!edited_cave)
        return FALSE;

    if (event->is_hint)
        gdk_window_get_pointer(event->window, &x, &y, &state);
    else {
        x=event->x;
        y=event->y;
        state=event->state;
    }
    
    x/=gd_cell_size_editor;
    y/=gd_cell_size_editor;

    /* if button1 not pressed, remember this. we also use the motion event to see if it. hackish. */
    if (!(state&GDK_BUTTON1_MASK))
        button1_clicked=FALSE;

    /* check if event coordinates inside drawing area. when holding the mouse
     * button, gdk can send coordinates outside! */
    if (x<0 || y<0 || x>=rendered_cave->w || y>=rendered_cave->h)
        return TRUE;

    /* check if mouse has moved to another cell. also set label showing coordinate. */
    if (mouse_x!=x || mouse_y!=y) {
        mouse_x=x;
        mouse_y=y;
        gd_label_set_markup_printf(GTK_LABEL (label_coordinate), "[x:%d y:%d]", x, y);
    }
    
    /* if we do not remember button 1 press, then don't do anything. */
    /* this solves some misinterpretation of mouse events, when windows appear or mouse pointer exits the drawing area and enters again */
    if (!(state&GDK_BUTTON1_MASK) || !button1_clicked)
        return TRUE;

    dx=x-clicked_x;
    dy=y-clicked_y;
    /* if the mouse pointer did not move at least one cell in x or y direction, return */
    if (dx==0 && dy==0)
        return TRUE;
        
    /* changing visible region is different; independent of cave objects. */    
    if (action==VISIBLE_REGION) {
        /* save visible region flag only once */
        if (undo_move_flag==FALSE) {
            undo_save();
            undo_move_flag=TRUE;
        }
        
        /* try to drag (x1;y1) corner. */
        if (clicked_x==edited_cave->x1 && clicked_y==edited_cave->y1) {
            edited_cave->x1+=dx;
            edited_cave->y1+=dy;
        }
        else
            /* try to drag (x2;y1) corner. */
        if (clicked_x==edited_cave->x2 && clicked_y==edited_cave->y1) {
            edited_cave->x2+=dx;
            edited_cave->y1+=dy;
        }
        else
            /* try to drag (x1;y2) corner. */
        if (clicked_x==edited_cave->x1 && clicked_y==edited_cave->y2) {
            edited_cave->x1+=dx;
            edited_cave->y2+=dy;
        }
        else
            /* try to drag (x2;y2) corner. */
        if (clicked_x==edited_cave->x2 && clicked_y==edited_cave->y2) {
            edited_cave->x2+=dx;
            edited_cave->y2+=dy;
        }
        else {
            /* drag the whole */
            edited_cave->x1+=dx;
            edited_cave->y1+=dy;
            edited_cave->x2+=dx;
            edited_cave->y2+=dy;
        }
        clicked_x=x;
        clicked_y=y;
        
        /* check and adjust ranges if necessary */
        gd_cave_correct_visible_size(edited_cave);
        
        /* instead of re-rendering the cave, we just copy the changed values. */
        rendered_cave->x1=edited_cave->x1;
        rendered_cave->x2=edited_cave->x2;
        rendered_cave->y1=edited_cave->y1;
        rendered_cave->y2=edited_cave->y2;
        return TRUE;
    }

    if (action==FREEHAND) {
        /* the freehand tool is different a bit. it draws single points automatically */
        /* but only to places where there is no such object already. */
        if (gd_cave_get_rc(rendered_cave, x, y)!=gd_element_button_get (element_button) || rendered_cave->objects_order[y][x]==NULL) {
            GdObject *new_object;

            /* freehand places points */
            new_object=gd_object_new_point(new_object_mask(), x, y, gd_element_button_get(element_button));
            edited_cave->objects=g_list_append(edited_cave->objects, new_object);
            render_cave();    /* we do this here by hand; do not use changed flag; otherwise object_list_add_to_selection wouldn't work */
            object_list_add_to_selection(new_object);    /* this way all points will be selected together when using freehand */
        }
        return TRUE;
    }
    
    if (!object_list_is_any_selected())
        return TRUE;    /* nothing to move */
    
    if (object_list_count_selected()==1) {
        /* MOVING, DRAGGING A SINGLE OBJECT **************************/
        GdObject *object=object_list_first_selected();
        
        switch (action) {
        case MOVE:
            /* MOVING AN EXISTING OBJECT */
            if (undo_move_flag==FALSE) {
                undo_save();
                undo_move_flag=TRUE;
            }

            switch (object->type) {
                case GD_POINT:
                case GD_FLOODFILL_REPLACE:
                case GD_FLOODFILL_BORDER:
                    /* this one will be true for a point */
                    /* for other objects, only allow dragging by their origin */
                    if (clicked_x==object->x1 && clicked_y==object->y1) {
                        object->x1+=dx;
                        object->y1+=dy;
                    }
                    break;
                    
                case GD_LINE:
                    /* these are the easy ones. for a line, try if endpoints are dragged.
                     * if not, drag the whole line.
                     * for a point, the first if() will always match.
                     */
                    if (clicked_x==object->x1 && clicked_y==object->y1) {    /* this one will be true for a point */
                        object->x1+=dx;
                        object->y1+=dy;
                    }
                    else if (clicked_x==object->x2 && clicked_y==object->y2) {
                        object->x2+=dx;
                        object->y2+=dy;
                    }
                    else {
                        object->x1+=dx;
                        object->y1+=dy;
                        object->x2+=dx;
                        object->y2+=dy;
                    }
                    break;
                case GD_RECTANGLE:
                case GD_FILLED_RECTANGLE:
                case GD_RASTER:
                case GD_MAZE:
                case GD_MAZE_UNICURSAL:
                case GD_MAZE_BRAID:
                case GD_RANDOM_FILL:
                    /* dragging objects which are box-shaped */
                    /* XXX for raster, this not always works; as raster's x2,y2 may not be visible */
                    if (clicked_x==object->x1 && clicked_y==object->y1) {            /* try to drag (x1;y1) corner. */
                        object->x1+=dx;
                        object->y1+=dy;
                    }
                    else if (clicked_x==object->x2 && clicked_y==object->y1) {        /* try to drag (x2;y1) corner. */
                        object->x2 +=dx;
                        object->y1 +=dy;
                    }
                    else if (clicked_x==object->x1 && clicked_y==object->y2) {        /* try to drag (x1;y2) corner. */
                        object->x1+=dx;
                        object->y2+=dy;
                    }
                    else if (clicked_x==object->x2 && clicked_y==object->y2) {        /* try to drag (x2;y2) corner. */
                        object->x2+=dx;
                        object->y2+=dy;
                    }
                    else {
                        /* drag the whole thing */
                        object->x1+=dx;
                        object->y1+=dy;
                        object->x2+=dx;
                        object->y2+=dy;
                    }
                    break;
                case GD_JOIN:
                case GD_COPY_PASTE:
                    /* for join, dx dy are displacement, for paste, dx dy are new positions. */
                    object->dx+=dx;
                    object->dy+=dy;
                    break;
                case NONE:
                    g_assert_not_reached();
                    break;
            }
            break;

        /* DRAGGING THE MOUSE, WHEN THE OBJECT WAS JUST CREATED */
        case GD_POINT:
        case GD_FLOODFILL_BORDER:
        case GD_FLOODFILL_REPLACE:
            /* only dragging the created point further. wants the user another one, he must press the button again. */
            if (object->x1!=x || object->y1!=y) {
                object->x1=x;
                object->y1=y;
            }
            break;
        case GD_LINE:
        case GD_RECTANGLE:
        case GD_FILLED_RECTANGLE:
        case GD_RASTER:
        case GD_MAZE:
        case GD_MAZE_UNICURSAL:
        case GD_MAZE_BRAID:
        case GD_RANDOM_FILL:
            /* for these, dragging sets the second coordinate. */
            if (object->x2!=x || object->y2!=y) {
                object->x2=x;
                object->y2=y;
            }
            break;
        case GD_COPY_PASTE:
            /* dragging sets the second coordinate, but check paste coordinate */
            if (object->x2!=x || object->y2!=y) {
                object->x2=x;
                object->y2=y;
                object->dx=MIN(object->x1, object->x2);
                object->dy=MIN(object->y1, object->y2);
            }
            break;
        case GD_JOIN:
            /* dragging sets the distance of the new character placed. */
            object->dx+=dx;
            object->dy+=dy;
            break;
        case NONE:
            g_assert_not_reached();
            break;
        }
    } else
    if (object_list_count_selected()>1 && action==MOVE) {
        /* MOVING MULTIPLE OBJECTS */
        GList *iter;

        if (undo_move_flag==FALSE) {
            undo_save();
            undo_move_flag=TRUE;
        }
        
        for (iter=selected_objects; iter!=NULL; iter=iter->next) {
            GdObject *object=iter->data;
            
            switch (object->type) {
                case GD_COPY_PASTE:    /* dx dy are new coordinates */
                case GD_JOIN:    /* dx dy are displacement */
                    object->dx+=dx;
                    object->dy+=dy;
                    break;
                
                default:    /* lines, rectangles... */
                    object->x1+=dx;
                    object->y1+=dy;
                    object->x2+=dx;
                    object->y2+=dy;
                    break;
            }

        }
    }

    clicked_x=x;
    clicked_y=y;
    render_cave();

    return TRUE;
}

/****************************************************/


/* to remember size of window */
static gboolean
editor_window_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
    gd_editor_window_width=event->width;
    gd_editor_window_height=event->height;

    return FALSE;
}

/* destroy editor window - do some cleanups */
static gboolean
editor_window_destroy_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    /* remove drawing interrupt. */
    g_source_remove_by_user_data(drawing_area_draw_timeout);
    /* if cave is drawn, free. */
    gd_cave_free(rendered_cave);
    rendered_cave=NULL;
    
    /* we destroy the icon view explicitly. so the caveset gets recreated... gd_main_stop_game will need that, as it checks all caves for replay. */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    
    g_hash_table_destroy(cave_pixbufs);
    /* stop test is running. this also restores main window action sensitized states */
    gd_main_stop_game();

    return FALSE;
}









/****************************************************
 *
 * CAVE SELECTOR ICON VIEW
 *
 *
 */
 
static gboolean
icon_view_update_pixbufs_timeout(gpointer data)
{
    GtkTreePath *path;
    GtkTreeModel *model;
    GtkTreeIter iter;
    int created;
    gboolean finish;

    /* if no icon view found, remove interrupt. */
    if (!iconview_cavelist)
        return FALSE;

    model=gtk_icon_view_get_model(GTK_ICON_VIEW(iconview_cavelist));
    path=gtk_tree_path_new_first();
    
    created=0;
    /* render a maximum of 5 pixbufs at a time */
    while (created<5 && (finish=gtk_tree_model_get_iter (model, &iter, path))) {
        GdCave *cave;
        GdkPixbuf *pixbuf, *pixbuf_in_icon_view;

        gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, PIXBUF_COLUMN, &pixbuf_in_icon_view, -1);
        pixbuf=g_hash_table_lookup(cave_pixbufs, cave);

        /* if we have no pixbuf, generate one. */
        if (!pixbuf) {
            GdCave *rendered;

            pixbuf_in_icon_view=NULL;    /* to force update below */            
            rendered=gd_cave_new_rendered (cave, 0, 0);    /* render at level 1, seed=0 */
            pixbuf=gd_drawcave_to_pixbuf(rendered, 128, 128, TRUE, TRUE);                    /* draw 128x128 icons at max */
            if (!cave->selectable) {
                GdkPixbuf *colored;
                
                colored=gdk_pixbuf_composite_color_simple(pixbuf, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), GDK_INTERP_NEAREST, 160, 1, gd_flash_color, gd_flash_color);
                g_object_unref(pixbuf);    /* forget original */
                pixbuf=colored;
            }
            gd_cave_free(rendered);
            g_hash_table_insert(cave_pixbufs, cave, pixbuf);

            created++;    /* created at least one, it took time */
        }

        /* if generated a new pixbuf, or the icon view does not contain the pixbuf: */
        if (pixbuf!=pixbuf_in_icon_view)
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, PIXBUF_COLUMN, pixbuf, -1);

        gtk_tree_path_next (path);
    }
    gtk_tree_path_free(path);
    
    return finish;
}

static void
icon_view_update_pixbufs()
{
    g_timeout_add_full(G_PRIORITY_LOW, 10, icon_view_update_pixbufs_timeout, NULL, NULL);
}


/* this is also called as an item activated signal. */
/* so we do not use its parameters. */
static void
icon_view_edit_cave_cb()
{
    GList *list;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GdCave *cave;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW(iconview_cavelist));
    g_return_if_fail (list!=NULL);

    model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
    gtk_tree_model_get_iter (model, &iter, list->data);
    gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);    /* free the list of paths */
    g_list_free(list);
    select_cave_for_edit (cave);
    gtk_combo_box_set_active(GTK_COMBO_BOX(new_object_level_combo), 0); /* always default to level 1 */
}

static void
icon_view_rename_cave_cb(GtkWidget *widget, gpointer data)
{
    GList *list;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GdCave *cave;
    GtkWidget *dialog, *entry;
    int result;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW(iconview_cavelist));
    g_return_if_fail (list!=NULL);

    /* use first element, as icon view is configured to enable only one selection */
    model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
    gtk_tree_model_get_iter (model, &iter, list->data);
    gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);    /* free the list of paths */
    g_list_free(list);

    dialog=gtk_dialog_new_with_buttons(_("Cave Name"), GTK_WINDOW(gd_editor_window), GTK_DIALOG_NO_SEPARATOR|GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    entry=gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_entry_set_text(GTK_ENTRY(entry), cave->name);
    gtk_entry_set_max_length(GTK_ENTRY(entry), sizeof(GdString));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, FALSE, 6);

    gtk_widget_show(entry);
    result=gtk_dialog_run(GTK_DIALOG(dialog));
    if (result==GTK_RESPONSE_ACCEPT) {
        gd_strcpy(cave->name, gtk_entry_get_text(GTK_ENTRY(entry)));
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, NAME_COLUMN, cave->name, -1);
    }
    gtk_widget_destroy(dialog);
}

static void
icon_view_cave_make_selectable_cb(GtkWidget *widget, gpointer data)
{
    GList *list;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GdCave *cave;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW(iconview_cavelist));
    g_return_if_fail (list!=NULL);

    model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
    gtk_tree_model_get_iter (model, &iter, list->data);
    gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);    /* free the list of paths */
    g_list_free(list);
    if (!cave->selectable) {
        cave->selectable=TRUE;
        /* we remove its pixbuf, as its color will be different */
        g_hash_table_remove(cave_pixbufs, cave);
    }
    icon_view_update_pixbufs();
}

static void
icon_view_cave_make_unselectable_cb(GtkWidget *widget, gpointer data)
{
    GList *list;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GdCave *cave;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW(iconview_cavelist));
    g_return_if_fail (list!=NULL);

    model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
    gtk_tree_model_get_iter (model, &iter, list->data);
    gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);    /* free the list of paths */
    g_list_free(list);
    if (cave->selectable) {
        cave->selectable=FALSE;
        /* we remove its pixbuf, as its color will be different */
        g_hash_table_remove(cave_pixbufs, cave);
    }
    icon_view_update_pixbufs();
}

static void
icon_view_selection_changed_cb(GtkWidget *widget, gpointer data)
{
    GList *list;
    GtkTreeModel *model;
    int count;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW (widget));
    count=g_list_length(list);
    gtk_action_group_set_sensitive(actions_cave_selector, count==1);
    gtk_action_group_set_sensitive(actions_clipboard, count!=0);
    if (count==0)
        set_status_label_for_caveset();
    else
    if (count==1) {
        GtkTreeIter iter;
        GdCave *cave;

        model=gtk_icon_view_get_model (GTK_ICON_VIEW (widget));

        gtk_tree_model_get_iter (model, &iter, list->data);
        gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
        
        set_status_label_for_cave(cave);    /* status bar now shows some basic data for cave */
    }
    else
        gd_label_set_markup_printf(GTK_LABEL (label_object), _("%d caves selected"), count);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(list);
}


/* for caveset icon view */
static void
icon_view_destroyed (GtkIconView * icon_view, gpointer data)
{
    GtkTreePath *path;
    GtkTreeModel *model;
    GtkTreeIter iter;

    /* caveset should be an empty list, as the icon view stores the current caves and order */
    g_assert(gd_caveset==NULL);

    model=gtk_icon_view_get_model(icon_view);
    path=gtk_tree_path_new_first();
    while (gtk_tree_model_get_iter (model, &iter, path)) {
        GdCave *cave;

        gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
        /* make a new list from the new order obtained from the icon view */
        gd_caveset=g_list_append(gd_caveset, cave);

        gtk_tree_path_next (path);
    }
    gtk_tree_path_free(path);
}



static void
icon_view_add_cave(GtkListStore *store, GdCave *cave)
{
    GtkTreeIter treeiter;
    GdkPixbuf *cave_pixbuf;
    static GdkPixbuf *missing_image=NULL;
    
    if (!missing_image) {
        missing_image=gtk_widget_render_icon(gd_editor_window, GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG, NULL);
    }
    
    /* if we already know the pixbuf, set it. */
    cave_pixbuf=g_hash_table_lookup(cave_pixbufs, cave);
    if (cave_pixbuf==NULL)
        cave_pixbuf=missing_image;
    gtk_list_store_insert_with_values (store, &treeiter, -1, CAVE_COLUMN, cave, NAME_COLUMN, cave->name, PIXBUF_COLUMN, cave_pixbuf, -1);
}

/* does nothing else but sets caveset_edited to true. called by "reordering" (drag&drop), which is implemented by gtk+ by inserting and deleting */
/* we only connect this signal after adding all caves to the icon view, so it is only activated by the user! */
static void
icon_view_row_inserted(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    gd_caveset_edited=TRUE;
}

/* for popup menu, by properties key */
static void
icon_view_popup_menu(GtkWidget *widget, gpointer data)
{
    gtk_menu_popup(GTK_MENU(caveset_popup), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

/* for popup menu, by right-click */
static gboolean
icon_view_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    if (event->type==GDK_BUTTON_PRESS && event->button==3) {
        gtk_menu_popup(GTK_MENU(caveset_popup), NULL, NULL, NULL, NULL, event->button, event->time);
        return TRUE;
    }
    return FALSE;
}

/*
 * selects a cave for edit.
 * if given a cave, creates a drawing area, shows toolbars...
 * if given no cave, creates a gtk icon view for a game overview.
 */

static void
select_cave_for_edit(GdCave *cave)
{
    object_list_clear_selection();

    gtk_action_group_set_sensitive (actions_edit_object, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_edit_one_object, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_edit_cave, cave!=NULL);
    gtk_action_group_set_sensitive (actions_edit_caveset, cave==NULL);
    gtk_action_group_set_sensitive (actions_edit_tools, cave!=NULL);
    gtk_action_group_set_sensitive (actions_edit_map, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_edit_random, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_toggle, cave!=NULL);
    /* this is sensitized by an icon selector callback. */
    gtk_action_group_set_sensitive (actions_cave_selector, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_clipboard, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_clipboard_paste,
        (cave!=NULL && object_clipboard!=NULL)
        || (cave==NULL && cave_clipboard!=NULL));
    gtk_action_group_set_sensitive (actions_edit_undo, cave!=NULL && undo_caves!=NULL);
    gtk_action_group_set_sensitive (actions_edit_redo, cave!=NULL && redo_caves!=NULL);

    /* select cave */
    edited_cave=cave;

    /* if cave data given, show it. */
    if (cave) {
        if (iconview_cavelist)
            gtk_widget_destroy (iconview_cavelist);

        if (gd_show_object_list)
            gtk_widget_show (scroll_window_objects);

        /* create pixbufs for these colors */
        gd_select_pixbuf_colors(edited_cave->color0, edited_cave->color1, edited_cave->color2, edited_cave->color3, edited_cave->color4, edited_cave->color5);
        gd_element_button_update_pixbuf(element_button);
        gd_element_button_update_pixbuf(fillelement_button);

        /* put drawing area in an alignment, so window can be any large w/o problems */
        if (!drawing_area) {
            GtkWidget *align;

            align=gtk_alignment_new(0.5, 0.5, 0, 0);
            gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll_window), align);
            gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_window), GTK_SHADOW_NONE);

            drawing_area=gtk_drawing_area_new();
            mouse_x=mouse_y=-1;
            /* enable some events */
            gtk_widget_add_events(drawing_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK);
            g_signal_connect(G_OBJECT (drawing_area), "destroy", G_CALLBACK(gtk_widget_destroyed), &drawing_area);
            g_signal_connect(G_OBJECT (drawing_area), "button_press_event", G_CALLBACK(drawing_area_button_press_event), NULL);
            g_signal_connect(G_OBJECT (drawing_area), "button_release_event", G_CALLBACK(drawing_area_button_release_event), NULL);
            g_signal_connect(G_OBJECT (drawing_area), "motion_notify_event", G_CALLBACK(drawing_area_motion_event), NULL);
            g_signal_connect(G_OBJECT (drawing_area), "leave_notify_event", G_CALLBACK(drawing_area_leave_event), NULL);
            g_signal_connect(G_OBJECT (drawing_area), "expose_event", G_CALLBACK(drawing_area_expose_event), NULL);
            gtk_container_add (GTK_CONTAINER (align), drawing_area);
        }
        render_cave();
        gtk_widget_set_size_request(drawing_area, edited_cave->w*gd_cell_size_editor, edited_cave->h*gd_cell_size_editor);
    }
    else {
        /* if no cave given, show selector. */
        /* forget undo caves */
        undo_free_all();
        
        gd_cave_map_free(gfx_buffer);
        gfx_buffer=NULL;
        gd_cave_map_free(object_highlight_map);
        object_highlight_map=NULL;

        gtk_list_store_clear (object_list);
        gtk_widget_hide (scroll_window_objects);

        if (drawing_area)
            gtk_widget_destroy (drawing_area->parent->parent);
        /* parent is the align, parent of align is the viewport automatically added. */

        if (!iconview_cavelist) {
            GtkListStore *cave_list;
            GList *iter;

            gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_window), GTK_SHADOW_IN);

            /* create list store for caveset */
            cave_list=gtk_list_store_new(NUM_CAVESET_COLUMNS, G_TYPE_POINTER, G_TYPE_STRING, GDK_TYPE_PIXBUF);
            for (iter=gd_caveset; iter; iter=g_list_next (iter))
                icon_view_add_cave(cave_list, iter->data);
            /* we only connect this signal after adding all caves to the icon view, so it is only activated by the user! */
            g_signal_connect(G_OBJECT(cave_list), "row-inserted", G_CALLBACK(icon_view_row_inserted), NULL);
            /* forget caveset; now we store caves in the GtkListStore */
            g_list_free(gd_caveset);
            gd_caveset=NULL;

            iconview_cavelist=gtk_icon_view_new_with_model (GTK_TREE_MODEL (cave_list));
            g_object_unref(cave_list);    /* now the icon view holds the reference */
            icon_view_update_pixbufs();    /* create icons */
            g_signal_connect(G_OBJECT(iconview_cavelist), "destroy", G_CALLBACK(icon_view_destroyed), &iconview_cavelist);
            g_signal_connect(G_OBJECT(iconview_cavelist), "destroy", G_CALLBACK(gtk_widget_destroyed), &iconview_cavelist);
            g_signal_connect(G_OBJECT(iconview_cavelist), "popup-menu", G_CALLBACK(icon_view_popup_menu), NULL);
            g_signal_connect(G_OBJECT(iconview_cavelist), "button-press-event", G_CALLBACK(icon_view_button_press_event), NULL);

            gtk_icon_view_set_text_column(GTK_ICON_VIEW(iconview_cavelist), NAME_COLUMN);
            gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW (iconview_cavelist), PIXBUF_COLUMN);
            gtk_icon_view_set_item_width(GTK_ICON_VIEW(iconview_cavelist), 128+24);    /* 128 is the size of the icons */
            gtk_icon_view_set_reorderable(GTK_ICON_VIEW(iconview_cavelist), TRUE);
            gtk_icon_view_set_selection_mode(GTK_ICON_VIEW (iconview_cavelist), GTK_SELECTION_MULTIPLE);
            /* item (cave) activated. the enter button activates the menu item; this one is used for doubleclick */
            g_signal_connect(iconview_cavelist, "item-activated", G_CALLBACK(icon_view_edit_cave_cb), NULL);
            g_signal_connect(iconview_cavelist, "selection-changed", G_CALLBACK(icon_view_selection_changed_cb), NULL);
            gtk_container_add(GTK_CONTAINER (scroll_window), iconview_cavelist);
        }

        set_status_label_for_caveset();
    }
    /* show all items inside scrolled window. some may have been newly created. */
    gtk_widget_show_all (scroll_window);

    /* hide toolbars if not editing a cave */
    if (edited_cave)
        gtk_widget_show (toolbars);
    else
        gtk_widget_hide (toolbars);

    editor_window_set_title();
}

/****************************************************/
static void
cave_random_setup_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog, *table;
    int result;

    g_return_if_fail(edited_cave->map==NULL);

    /* save for undo here, as we do not have cancel button :P */
    undo_save();

    dialog=gtk_dialog_new_with_buttons (_("Cave Initial Random Fill"), GTK_WINDOW (gd_editor_window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    
    table=gtk_table_new(0, 0, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (table), 6);
    gtk_table_set_row_spacings (GTK_TABLE(table), 6);
    gtk_table_set_col_spacings (GTK_TABLE(table), 6);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), table);
    
    random_setup_widgets_to_table(GTK_TABLE(table), 0, &edited_cave->initial_border, &edited_cave->initial_fill, edited_cave->random_fill, edited_cave->random_fill_probability, edited_cave->level_rand);

    gtk_widget_show_all(dialog);
    result=gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    /* if cancel clicked, revert to original */
    if (result!=GTK_RESPONSE_ACCEPT)
        undo_do_one_step();
}


/*******************************************
 *
 * CAVE 
 *
 *******************************************/
static void
save_cave_png(GdkPixbuf *pixbuf)
{
    /* if no filename given, */
    GtkWidget *dialog;
    GtkFileFilter *filter;
    GError *error=NULL;
    char *suggested_name, *filename=NULL;

    /* check if in cave editor */
    g_return_if_fail (edited_cave!=NULL);

    dialog=gtk_file_chooser_dialog_new(_("Save Cave as PNG Image"), GTK_WINDOW (gd_editor_window), GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

    filter=gtk_file_filter_new();
    gtk_file_filter_set_name (filter, _("PNG files"));
    gtk_file_filter_add_pattern (filter, "*.png");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
    suggested_name=g_strdup_printf("%s.png", edited_cave->name);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), suggested_name);
    g_free(suggested_name);

    if (gtk_dialog_run (GTK_DIALOG (dialog))==GTK_RESPONSE_ACCEPT) {
        gboolean save=FALSE;

        filename=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        if (!g_str_has_suffix(filename, ".png")) {
            char *suffixed=g_strdup_printf("%s.png", filename);
            
            g_free(filename);
            filename=suffixed;
        }

        /* check if overwrite */
        if (g_file_test(filename, G_FILE_TEST_EXISTS))
            save=gd_ask_overwrite(filename);
        else
            save=TRUE;

        /* save it */
        if (save)
            gdk_pixbuf_save (pixbuf, filename , "png", &error, "compression", "9", NULL);
    }
    gtk_widget_destroy (dialog);

    if (error) {
        g_warning("%s: %s", filename, error->message);
        gd_show_last_error(gd_editor_window);
        g_error_free(error);
    }
    g_free(filename);
}

/* CAVE OVERVIEW
   this creates a pixbuf of the cave, and scales it down to fit the screen if needed.
   it is then presented to the user, with the option to save it in png */
static void
cave_overview(gboolean simple_view)
{
    /* view the RENDERED one, and the entire cave */
    GdkPixbuf *pixbuf=gd_drawcave_to_pixbuf(rendered_cave, 0, 0, simple_view, TRUE), *scaled;
    GtkWidget *dialog, *button;
    int sx, sy;
    double fx, fy;
    int response;
    
    /* be careful not to use entire screen, there are window title and close button */
    sx=gdk_screen_get_width(gdk_screen_get_default())-64;
    sy=gdk_screen_get_height(gdk_screen_get_default())-128;
    if (gdk_pixbuf_get_width(pixbuf)>sx)
        fx=(double) sx/gdk_pixbuf_get_width(pixbuf);
    else
        fx=1.0;
    if (gdk_pixbuf_get_height(pixbuf)>sy)
        fy=(double) sy/gdk_pixbuf_get_height(pixbuf);
    else
        fy=1.0;
    /* whichever is smaller */
    if (fx<fy)
        fy=fx;
    if (fy<fx)
        fx=fy;
    /* if we have to make it smaller */
    if (fx!=1.0 || fy!=1.0)
        scaled=gdk_pixbuf_scale_simple(pixbuf, gdk_pixbuf_get_width(pixbuf)*fx, gdk_pixbuf_get_height(pixbuf)*fy, GDK_INTERP_BILINEAR);
    else {
        scaled=pixbuf;
        g_object_ref(scaled);
    }
    
    /* simple dialog with this image only */
    dialog=gtk_dialog_new_with_buttons(_("Cave Overview"), GTK_WINDOW(gd_editor_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    button=gtk_button_new_with_mnemonic(_("Save as _PNG"));
    gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON));
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button, 1);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
    
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), gtk_image_new_from_pixbuf(scaled));

    gtk_widget_show_all(dialog);
    response=gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response==1)
        save_cave_png(pixbuf);
    g_object_unref(pixbuf);
    g_object_unref(scaled);
}


static void
cave_overview_cb(GtkWidget *widget, gpointer data)
{
    cave_overview(FALSE);
}

static void
cave_overview_simple_cb(GtkWidget *widget, gpointer data)
{
    cave_overview(TRUE);
}





/* automatically shrink cave
 */
static void
auto_shrink_cave_cb(GtkWidget *widget, gpointer data)
{
    undo_save();
    /* shrink the rendered cave, as it has all object and the like converted to a map. */
    gd_cave_auto_shrink(rendered_cave);
    /* then copy the results to the original */
    edited_cave->x1=rendered_cave->x1;
    edited_cave->y1=rendered_cave->y1;
    edited_cave->x2=rendered_cave->x2;
    edited_cave->y2=rendered_cave->y2;
    render_cave();
    /* selecting visible region tool allows the user to see the result, maybe modify */
    select_tool(VISIBLE_REGION);
}








/*
 *
 * SET CAVE COLORS WITH INSTANT UPDATE TOOL.
 *
 */
/* helper: update pixmaps and the like */
static void
cave_colors_update_element_pixbufs()
{
    int x,y;
    /* select new colors */
    gd_select_pixbuf_colors(edited_cave->color0, edited_cave->color1, edited_cave->color2, edited_cave->color3, edited_cave->color4, edited_cave->color5);
    /* update element buttons in editor (under toolbar) */
    gd_element_button_update_pixbuf(element_button);
    gd_element_button_update_pixbuf(fillelement_button);
    /* clear gfx buffer, so every element gets redrawn */
    for (y=0; y<edited_cave->h; y++)
        for (x=0; x<edited_cave->w; x++)
            gfx_buffer[y][x]=-1;
    /* object list update with new pixbufs */
    render_cave();
}

static gboolean cave_colors_colorchange_update_disabled;

#define GDASH_PCOLOR "gdash-pcolor"
/* when the user clicks the button and sets the color, we store the color to the cave,
   and re-render cave with new colors.
   
   this behavior can be disabled, as when randomly changing all colors, nothing should
   happen when we automatically update the buttons.
 */
static void
cave_colors_colorbutton_changed(GtkWidget *widget, gpointer data)
{
    if (!cave_colors_colorchange_update_disabled) {
        GdColor* value;
        
        value=(GdColor *)g_object_get_data(G_OBJECT(widget), GDASH_PCOLOR);
        g_assert(value!=NULL);
            
        *value=gd_color_combo_get_color(widget);    /* update cave data */
        cave_colors_update_element_pixbufs();
    }
}

/* when the random colors button is pressed, first we change the colors of the cave. */
/* then we update the combo boxes one by one (they are in a glist *), but before that, */
/* we disable their updating behaviour. otherwise they would re-render pixmaps one by one, */
/* and they would also want to change the cave itself - the cave which already contains the */
/* changed colors! */
static void
cave_colors_random_combo_cb(GtkWidget *widget, gpointer data)
{
    GList *combos=(GList *)data;
    GList *iter;
    int new_index;
    
    new_index=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    if (new_index==0)    /* 0 is the "set random..." text, so we do nothing */
        return;

    /* create colors */
    gd_cave_set_random_colors(edited_cave, (GdColorType) (new_index-1));    /* -1: 0 is "set random...", 1 is rgb... */

    /* and update combo boxes from cave */
    cave_colors_colorchange_update_disabled=TRUE;    /* this is needed, otherwise all combos would want to update the pixbufs, one by one */
    for (iter=combos; iter!=NULL; iter=iter->next) {
        GtkWidget *combo=(GtkWidget *)iter->data;
        GdColor* value=(GdColor *)g_object_get_data(G_OBJECT(combo), GDASH_PCOLOR);
        
        g_assert(value!=NULL);
        
        gd_color_combo_set(GTK_COMBO_BOX(combo), *value);
    }
    cave_colors_colorchange_update_disabled=FALSE;
    cave_colors_update_element_pixbufs();
    
    /* set back to "select random..." text */
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
}

/* set cave colors with instant update */
static void
cave_colors_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog, *table=NULL;
    GtkWidget *random_combo;
    GList *combos=NULL;
    int i, row=0;
    int result;
    gboolean colored_objects_backup;
    
    undo_save();
    
    /* when editing colors, turn off the colored objects viewing for a while. */
    colored_objects_backup=gd_colored_objects;
    gd_colored_objects=FALSE;

    dialog=gtk_dialog_new_with_buttons (_("Cave Colors"), GTK_WINDOW (gd_editor_window), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    /* random combo */
    random_combo=gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), random_combo, FALSE, FALSE, 0);
    /* close button */
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    
    table=gtk_table_new(1, 1, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 6);
    gtk_table_set_row_spacings (GTK_TABLE(table), 6);
    gtk_table_set_col_spacings (GTK_TABLE(table), 6);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), table);
    
    /* search for color properties, and create label&combo for each */
    for (i=0; gd_cave_properties[i].identifier!=NULL; i++)
        if (gd_cave_properties[i].type==GD_TYPE_COLOR) {
            GdColor *value=(GdColor *)G_STRUCT_MEMBER_P (edited_cave, gd_cave_properties[i].offset);
            GtkWidget *combo;

            gtk_table_attach(GTK_TABLE(table), gd_label_new_printf(_(gd_cave_properties[i].name)), 0, 1, row, row+1, GTK_FILL|GTK_SHRINK, GTK_FILL|GTK_SHRINK, 0, 0);

            combo=gd_color_combo_new(*value);
            combos=g_list_append(combos, combo);
            g_object_set_data(G_OBJECT(combo), GDASH_PCOLOR, value);
            gtk_widget_set_tooltip_text(combo, _(gd_cave_properties[i].tooltip));
            g_signal_connect(combo, "changed", G_CALLBACK(cave_colors_colorbutton_changed), value);
            gtk_table_attach(GTK_TABLE(table), combo, 1, 2, row, row+1, GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_SHRINK, 0, 0);
            row++;
        }
    
    /* a combo box which has a callback that sets random colors */
    gtk_combo_box_append_text(GTK_COMBO_BOX(random_combo), _("Set random..."));    /* will be active=0 */
    for (i=0; gd_color_get_palette_types_names()[i]!=NULL; i++)
        gtk_combo_box_append_text(GTK_COMBO_BOX(random_combo), _(gd_color_get_palette_types_names()[i]));
    gtk_combo_box_set_active(GTK_COMBO_BOX(random_combo), 0);
    g_signal_connect(random_combo, "changed", G_CALLBACK(cave_colors_random_combo_cb), combos);

    /* hint label */
    gd_dialog_add_hint(GTK_DIALOG(dialog), _("Hint: As the palette can be changed for C64 and Atari colors, "
    "it is not recommended to use different types together (for example, RGB color for background, Atari color for Slime.)"));
    
    gtk_widget_show_all(dialog);
    cave_colors_colorchange_update_disabled=FALSE;
    result=gtk_dialog_run(GTK_DIALOG(dialog));
    g_list_free(combos);
    gtk_widget_destroy(dialog);
    /* if the new colors were not accepted by the user (escape pressed), we undo the changes. */
    if (result!=GTK_RESPONSE_ACCEPT)
        undo_do_one_step();

    /* restore colored objects setting. */
    gd_colored_objects=colored_objects_backup;
}
#undef GDASH_PCOLOR









/***************************************************
 *
 * CAVE EDITING CALLBACKS
 *
 */

/* delete selected cave drawing element or cave.
*/
static void
delete_selected_cb(GtkWidget *widget, gpointer data)
{
    /* deleting caves or cave object. */
    if (edited_cave==NULL) {
        /* WE ARE DELETING ONE OR MORE CAVES HERE */
        GList *list, *listiter;
        GtkTreeModel *model;
        GList *references=NULL;
        gboolean response;
        
        /* first we ask the user if he is sure, as no undo is implemented yet */
        response=gd_question_yesno(_("Do you really want to delete cave(s)?"), _("This operation cannot be undone."));
        
        if (!response)
            return;
        list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW (iconview_cavelist));
        g_return_if_fail (list!=NULL);    /* list should be not empty. otherwise why was the button not insensitized? */
        
        /* if anything was selected */
        model=gtk_icon_view_get_model(GTK_ICON_VIEW(iconview_cavelist));
        /* for all caves selected, convert to tree row references - we must delete them for the icon view, so this is necessary */
        for (listiter=list; listiter!=NULL; listiter=listiter->next)
            references=g_list_append(references, gtk_tree_row_reference_new(model, listiter->data));
        g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(list);
        
        /* now check the list of references and delete each cave */
        for (listiter=references; listiter!=NULL; listiter=listiter->next) {
            GtkTreeRowReference *reference=listiter->data;
            GtkTreePath *path;
            GtkTreeIter iter;
            GdCave *cave;
            
            path=gtk_tree_row_reference_get_path(reference);
            gtk_tree_model_get_iter (model, &iter, path);
            gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            gd_cave_free(cave);    /* and also free memory associated. */
            g_hash_table_remove(cave_pixbufs, cave);
        }
        g_list_foreach(references, (GFunc) gtk_tree_row_reference_free, NULL);
        g_list_free(references);
        
        /* this modified the caveset */
        gd_caveset_edited=TRUE;
    } else {
        /* WE ARE DELETING A CAVE OBJECT HERE */
        GList *iter;

        g_return_if_fail(object_list_is_any_selected());
        
        undo_save();

        /* delete all objects */
        for (iter=selected_objects; iter!=NULL; iter=iter->next) {
            edited_cave->objects=g_list_remove(edited_cave->objects, iter->data);
            g_free(iter->data);
        }
        object_list_clear_selection();
        render_cave();
    }
}

/* put selected drawing elements to bottom.
*/
static void
bottom_selected_cb(GtkWidget *widget, gpointer data)
{
    GList *iter;
    
    g_return_if_fail (object_list_is_any_selected());

    undo_save();
    
    /* we reverse the list, as prepending changes the order */
    selected_objects=g_list_reverse(selected_objects);
    for (iter=selected_objects; iter!=NULL; iter=iter->next) {
        /* remove from original place */
        edited_cave->objects=g_list_remove(edited_cave->objects, iter->data);
        /* put to beginning */
        edited_cave->objects=g_list_prepend(edited_cave->objects, iter->data);
    }
    render_cave();
}

/* bring selected drawing element to top. */
static void
top_selected_cb(GtkWidget *widget, gpointer data)
{
    GList *iter;
    
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    for (iter=selected_objects; iter!=NULL; iter=iter->next) {
        /* remove from original place */
        edited_cave->objects=g_list_remove(edited_cave->objects, iter->data);
        /* put to beginning */
        edited_cave->objects=g_list_append(edited_cave->objects, iter->data);
    }
    render_cave();
}

/* enable currently selected objects on the currently viewed level only. */
static void
show_object_this_level_only_cb(GtkWidget *widget, gpointer data)
{
    GList *iter;
    
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    for (iter=selected_objects; iter!=NULL; iter=iter->next) {
        GdObject *obj=(GdObject *)iter->data;
        
        obj->levels=gd_levels_mask[edit_level];
    }
    render_cave();
}

/* enable currently selected objects on all levels */
static void
show_object_all_levels_cb(GtkWidget *widget, gpointer data)
{
    GList *iter;
    
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    for (iter=selected_objects; iter!=NULL; iter=iter->next) {
        GdObject *obj=(GdObject *)iter->data;
        
        obj->levels=GD_OBJECT_LEVEL_ALL;
    }
    render_cave();
}

/* enable currently selected objects on the currently viewed level only. */
static void
show_object_on_this_level_cb(GtkWidget *widget, gpointer data)
{
    GList *iter;
    
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    for (iter=selected_objects; iter!=NULL; iter=iter->next) {
        GdObject *obj=(GdObject *)iter->data;
        
        obj->levels|=gd_levels_mask[edit_level];
    }
    render_cave();
}

/* enable currently selected objects on the currently viewed level only. */
static void
hide_object_on_this_level_cb(GtkWidget *widget, gpointer data)
{
    GList *iter;
    int disappear;
    
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    disappear=0;
    for (iter=selected_objects; iter!=NULL; iter=iter->next) {
        GdObject *obj=(GdObject *)iter->data;
        
        obj->levels &= ~gd_levels_mask[edit_level];
        /* an object should be visible on at least one level. */
        /* if it disappeared, switch it back, and remember that we will show an error message. */
        if (obj->levels==0) {
            obj->levels=gd_levels_mask[edit_level];
            disappear++;
        }
    }
    render_cave();
    
    if (disappear>0)
        gd_warningmessage(_("At least one object would have been totally hidden (not visible on any of the levels)."), _("Enabled those objects on the current level."));
}

/* copy selected object or caves to clipboard.
*/
static void
copy_selected_cb(GtkWidget *widget, gpointer data)
{
    if (edited_cave==NULL) {
        /* WE ARE NOW COPYING CAVES FROM A CAVESET */
        GList *list, *listiter;
        GtkTreeModel *model;

        list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW (iconview_cavelist));
        g_return_if_fail (list!=NULL);    /* list should be not empty. otherwise why was the button not insensitized? */
        
        /* forget old clipboard */
        g_list_foreach(cave_clipboard, (GFunc) gd_cave_free, NULL);
        g_list_free(cave_clipboard);
        cave_clipboard=NULL;

        /* now for all caves selected */
        /* we do not need references here (as in cut), as we do not modify the treemodel */
        model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
        for (listiter=list; listiter!=NULL; listiter=listiter->next) {
            GdCave *cave=NULL;
            GtkTreeIter iter;
            
            gtk_tree_model_get_iter (model, &iter, listiter->data);
            gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
            /* add to clipboard: prepend must be used for correct order */
            /* here, a COPY is added to the clipboard */
            cave_clipboard=g_list_prepend(cave_clipboard, gd_cave_new_from_cave(cave));
        }
        g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(list);

        /* enable pasting */
        gtk_action_group_set_sensitive (actions_clipboard_paste, TRUE);
    } else {
        GList *iter;
        
        /* delete contents of clipboard */
        g_list_foreach(object_clipboard, (GFunc) g_free, NULL);
        object_clipboard=NULL;
        
        for(iter=selected_objects; iter!=NULL; iter=iter->next)
            object_clipboard=g_list_append(object_clipboard, g_memdup(iter->data, sizeof(GdObject)));
        /* enable pasting */
        gtk_action_group_set_sensitive (actions_clipboard_paste, TRUE);
    }
}

/* paste object or cave from clipboard
*/
static void
paste_clipboard_cb(GtkWidget *widget, gpointer data)
{
    if (edited_cave==NULL) {
        /* WE ARE IN THE CAVESET ICON VIEW */
        GList *iter;
        GtkListStore *store=GTK_LIST_STORE(gtk_icon_view_get_model(GTK_ICON_VIEW(iconview_cavelist)));
        
        for(iter=cave_clipboard; iter!=NULL; iter=iter->next) {
            GdCave *cave=iter->data;
            
            icon_view_add_cave(store, gd_cave_new_from_cave(cave));
        }
        icon_view_update_pixbufs();
        
        /* this modified the caveset */
        gd_caveset_edited=TRUE;
    } else {
        /* WE ARE IN THE CAVE EDITOR */
        GList *iter;
        GList *new_objects=NULL;
        
        g_return_if_fail (object_clipboard!=NULL);

        /* we have a list of newly added (pasted) objects, so after pasting we can
           select them. this is necessary, as only after pasting is render_cave() called,
           which adds the new objects to the gtkliststore. otherwise that one would not
           contain cave objects. the clipboard also cannot be used, as pointers are different */
        for (iter=object_clipboard; iter!=NULL; iter=iter->next) {
            GdObject *new_object=g_memdup(iter->data, sizeof(GdObject));

            edited_cave->objects=g_list_append(edited_cave->objects, new_object);
            new_objects=g_list_prepend(new_objects, new_object);    /* prepend is ok, as order does not matter */
        }

        render_cave();
        object_list_clear_selection();
        for (iter=new_objects; iter!=NULL; iter=iter->next)
            object_list_add_to_selection(iter->data);
        g_list_free(new_objects);
    }
}


/* cut an object, or cave(s) from the caveset. */
static void
cut_selected_cb(GtkWidget *widget, gpointer data)
{
    if (edited_cave==NULL) {
        /* WE ARE NOW CUTTING CAVES FROM A CAVESET */
        GList *list, *listiter;
        GtkTreeModel *model;
        GList *references=NULL;

        list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW (iconview_cavelist));
        g_return_if_fail (list!=NULL);    /* list should be not empty. otherwise why was the button not insensitized? */
        
        /* forget old clipboard */
        g_list_foreach(cave_clipboard, (GFunc) gd_cave_free, NULL);
        g_list_free(cave_clipboard);
        cave_clipboard=NULL;

        /* if anything was selected */
        model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
        /* for all caves selected, convert to tree row references - we must delete them for the icon view, so this is necessary */
        for (listiter=list; listiter!=NULL; listiter=listiter->next)
            references=g_list_append(references, gtk_tree_row_reference_new(model, listiter->data));
        g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(list);
        
        for (listiter=references; listiter!=NULL; listiter=listiter->next) {
            GtkTreeRowReference *reference=listiter->data;
            GtkTreePath *path;
            GdCave *cave=NULL;
            GtkTreeIter iter;
            
            path=gtk_tree_row_reference_get_path(reference);
            gtk_tree_model_get_iter (model, &iter, path);
            gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
            /* prepend must be used for correct order */
            /* here, the cave is not copied, but the pointer is moved to the clipboard */
            cave_clipboard=g_list_prepend(cave_clipboard, cave);
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            /* remove its pixbuf */
            g_hash_table_remove(cave_pixbufs, cave);
        }
        g_list_foreach(references, (GFunc) gtk_tree_row_reference_free, NULL);
        g_list_free(references);

        /* enable pasting */
        gtk_action_group_set_sensitive (actions_clipboard_paste, TRUE);
        
        /* this modified the caveset */
        gd_caveset_edited=TRUE;
    } else {
        GList *iter;
        /* EDITED OBJECT IS NOT NULL, SO WE ARE CUTTING OBJECTS */
        undo_save();
    
        /* delete contents of clipboard */
        g_list_foreach(object_clipboard, (GFunc) g_free, NULL);
        object_clipboard=NULL;
        
        /* do not make a copy; rather the clipboard holds the reference from now on. */
        for(iter=selected_objects; iter!=NULL; iter=iter->next) {
            object_clipboard=g_list_append(object_clipboard, iter->data);
            edited_cave->objects=g_list_remove(edited_cave->objects, iter->data);
        }

        /* enable pasting */
        gtk_action_group_set_sensitive (actions_clipboard_paste, TRUE);

        object_list_clear_selection();
        render_cave();
    }
}

static void
select_all_cb(GtkWidget *widget, gpointer data)
{
    if (edited_cave==NULL)    /* in game editor */
        gtk_icon_view_select_all(GTK_ICON_VIEW(iconview_cavelist));        /* SELECT ALL CAVES */
    else                    /* in cave editor */
        gtk_tree_selection_select_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)));        /* SELECT ALL OBJECTS */
}

/* delete all cave elements */
static void
clear_cave_elements_cb(GtkWidget *widget, gpointer data)
{
    g_return_if_fail (edited_cave!=NULL);

    if (edited_cave->objects) {
        gboolean response;
        
        response=gd_question_yesno(_("Do you really want to clear cave objects?"), NULL);
        if (response) {
            undo_save();    /* changing cave; save for undo */

            g_list_foreach(edited_cave->objects, (GFunc) g_free, NULL);
            edited_cave->objects=NULL;
            /* element deleted; redraw cave */
            render_cave();
        }
    }
}

/* delete map from cave */
static void
remove_map_cb(GtkWidget *widget, gpointer data)
{
    gboolean response;

    g_return_if_fail (edited_cave!=NULL);
    g_return_if_fail (edited_cave->map!=NULL);

    response=gd_question_yesno(_("Do you really want to remove cave map?"), _("This operation destroys all cave objects."));

    if (response) {
        undo_save(); /* changing cave; save for undo */

        gd_cave_map_free(edited_cave->map);
        edited_cave->map=NULL;
        /* map deleted; redraw cave */
        render_cave();
    }
}

/* flatten cave -> pack everything in a map */
static void
flatten_cave_cb(GtkWidget *widget, gpointer data)
{
    gboolean response;

    g_return_if_fail (edited_cave!=NULL);

    if (!edited_cave->objects) {
        gd_infomessage(_("This cave has no objects."), NULL);
        return;
    }

    response=gd_question_yesno(_("Do you really want to flatten cave?"), _("This operation merges all cave objects currently seen in a single map. Further objects may later be added, but the ones already seen will behave like the random fill elements; they will not be editable."));

    if (response) {
        undo_save();    /* changing; save for undo */

        gd_flatten_cave(edited_cave, edit_level);
        render_cave();
    }
}

/* shift cave map left, one step. */
static void
shift_left_cb(GtkWidget *widget, gpointer data)
{
    int y;
    
    g_return_if_fail(edited_cave!=NULL);
    g_return_if_fail(edited_cave->map!=NULL);
    
    for (y=0; y<edited_cave->h; y++) {
        GdElement temp;
        int x;
        
        temp=edited_cave->map[y][0];
        for (x=0; x<edited_cave->w-1; x++)
            edited_cave->map[y][x]=edited_cave->map[y][x+1];
        edited_cave->map[y][edited_cave->w-1]=temp; 
    }
    render_cave();
}

/* shift cave map right, one step. */
static void
shift_right_cb(GtkWidget *widget, gpointer data)
{
    int y;
    
    g_return_if_fail(edited_cave!=NULL);
    g_return_if_fail(edited_cave->map!=NULL);
    
    for (y=0; y<edited_cave->h; y++) {
        GdElement temp;
        int x;
        
        temp=edited_cave->map[y][edited_cave->w-1];
        for (x=edited_cave->w-2; x>=0; x--)
            edited_cave->map[y][x+1]=edited_cave->map[y][x];
        edited_cave->map[y][0]=temp; 
    }
    render_cave();
}

/* shift cave map up, one step. */
static void
shift_up_cb(GtkWidget *widget, gpointer data)
{
    int x, y;
    GdElement *temp;
    g_return_if_fail(edited_cave!=NULL);
    g_return_if_fail(edited_cave->map!=NULL);
    
    /* remember first line */
    temp=g_new(GdElement, edited_cave->w);
    for (x=0; x<edited_cave->w; x++)
        temp[x]=edited_cave->map[0][x];

    /* copy everything up */    
    for (y=0; y<edited_cave->h-1; y++)
        for (x=0; x<edited_cave->w; x++)
            edited_cave->map[y][x]=edited_cave->map[y+1][x];

    for (x=0; x<edited_cave->w; x++)
        edited_cave->map[edited_cave->h-1][x]=temp[x];
    g_free(temp);
    render_cave();
}

/* shift cave map down, one step. */
static void
shift_down_cb(GtkWidget *widget, gpointer data)
{
    int x, y;
    GdElement *temp;
    g_return_if_fail(edited_cave!=NULL);
    g_return_if_fail(edited_cave->map!=NULL);
    
    /* remember last line */
    temp=g_new(GdElement, edited_cave->w);
    for (x=0; x<edited_cave->w; x++)
        temp[x]=edited_cave->map[edited_cave->h-1][x];

    /* copy everything up */    
    for (y=edited_cave->h-2; y>=0; y--)
        for (x=0; x<edited_cave->w; x++)
            edited_cave->map[y+1][x]=edited_cave->map[y][x];

    for (x=0; x<edited_cave->w; x++)
        edited_cave->map[0][x]=temp[x];
    g_free(temp);
    render_cave();
}


static void
set_engine_default_cb(GtkWidget *widget, gpointer data)
{
    GdEngine e=(GdEngine) GPOINTER_TO_INT(data);
    
    g_assert(e>=0 && e<GD_ENGINE_INVALID);
    g_assert(edited_cave!=NULL);

    undo_save();
    gd_cave_set_engine_defaults(edited_cave, e);
//    props=gd_struct_explain_defaults_in_string(gd_cave_properties, gd_get_engine_default_array(e));
//    gd_infomessage(_("The following properties are set:"), props);
//    g_free(props);
}


static void
save_html_cb(GtkWidget *widget, gpointer data)
{
    char *htmlname=NULL, *suggested_name;
    /* if no filename given, */
    GtkWidget *dialog;
    GtkFileFilter *filter;
    GdCave *edited;
    
    edited=edited_cave;
    /* destroy icon view so it does not interfere with saving (when icon view is active, caveset=NULL) */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);

    dialog=gtk_file_chooser_dialog_new(_("Save Cave Set in HTML"), GTK_WINDOW (gd_editor_window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

    filter=gtk_file_filter_new();
    gtk_file_filter_set_name (filter, _("HTML files"));
    gtk_file_filter_add_pattern (filter, "*.html");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    suggested_name=g_strdup_printf("%s.html", gd_caveset_data->name);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), suggested_name);
    g_free(suggested_name);

    if (gtk_dialog_run (GTK_DIALOG (dialog))==GTK_RESPONSE_ACCEPT)
        htmlname=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (dialog);

    /* saving if got filename */
    if (htmlname) {
        gd_clear_error_flag();
        gd_save_html(htmlname, gd_editor_window);
        if (gd_has_new_error())
            gd_show_last_error(gd_editor_window);
    }

    g_free(htmlname);
    select_cave_for_edit(edited);    /* go back to edited cave or recreate icon view */
}



/* export cave to a crli editor format */
static void
export_cavefile_cb(GtkWidget *widget, gpointer data)
{
    char *outname=NULL;
    /* if no filename given, */
    GtkWidget *dialog;
    
    g_return_if_fail(edited_cave!=NULL);

    dialog=gtk_file_chooser_dialog_new(_("Export Cave as CrLi Cave File"), GTK_WINDOW (gd_editor_window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), edited_cave->name);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    
    if (gtk_dialog_run (GTK_DIALOG (dialog))==GTK_RESPONSE_ACCEPT)
        outname=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (dialog);

    /* if accepted, save. */
    if (outname) {
        gd_clear_error_flag();
        gd_export_cave_to_crli_cavefile(edited_cave, edit_level, outname);
        if(gd_has_new_error())
            gd_show_errors(gd_editor_window);
        g_free(outname);
    }
}


/* export complete caveset to a crli cave pack */
static void
export_cavepack_cb(GtkWidget *widget, gpointer data)
{
    char *outname=NULL;
    /* if no filename given, */
    GtkWidget *dialog;
    GdCave *edited;
    
    edited=edited_cave;
    /* destroy icon view so it does not interfere with saving (when icon view is active, caveset=NULL) */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);

    dialog=gtk_file_chooser_dialog_new(_("Export Cave as CrLi Cave Pack"), GTK_WINDOW (gd_editor_window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), gd_caveset_data->name);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dialog))==GTK_RESPONSE_ACCEPT)
        outname=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (dialog);

    /* saving if got filename */
    if (outname) {
        gd_clear_error_flag();
        gd_export_cave_list_to_crli_cavepack(gd_caveset, edit_level, outname);
        if(gd_has_new_error())
            gd_show_errors(gd_editor_window);
        g_free(outname);
    }

    select_cave_for_edit(edited);    /* go back to edited cave or recreate icon view */
}



/* test selected level. main window is brought to focus.
*/
static void
play_level_cb(GtkWidget *widget, gpointer data)
{
    g_return_if_fail (edited_cave!=NULL);
    gd_main_new_game_test(edited_cave, edit_level);
}



static void
object_properties_cb(GtkWidget *widget, gpointer data)
{
    object_properties(NULL);
}



/* edit caveset properties */
static void
set_caveset_properties_cb(GtkWidget *widget, gpointer data)
{
    caveset_properties(TRUE);
}


static void
cave_properties_cb(const GtkWidget *widget, const gpointer data)
{
    cave_properties (edited_cave, TRUE);
}



/************************************************
 *
 * MANAGING CAVES
 *
 */

/* go to cave selector */
static void
cave_selector_cb(GtkWidget *widget, gpointer data)
{
    select_cave_for_edit(NULL);
}

/* view next cave */
static void
previous_cave_cb(GtkWidget *widget, gpointer data)
{
    int i;
    g_return_if_fail(edited_cave!=NULL);
    g_return_if_fail(gd_caveset!=NULL);

    i=g_list_index(gd_caveset, edited_cave);
    g_return_if_fail(i!=-1);
    i=(i-1+gd_caveset_count())%gd_caveset_count();
    select_cave_for_edit(gd_return_nth_cave(i));
}

/* go to cave selector */
static void
next_cave_cb(GtkWidget *widget, gpointer data)
{
    int i;
    g_return_if_fail(edited_cave!=NULL);
    g_return_if_fail(gd_caveset!=NULL);

    i=g_list_index(gd_caveset, edited_cave);
    g_return_if_fail(i!=-1);
    i=(i+1)%gd_caveset_count();
    select_cave_for_edit(gd_return_nth_cave(i));
}

/* create new cave */
static void
new_cave_cb(GtkWidget *widget, gpointer data)
{
    GdCave *newcave;
    GtkWidget *dialog, *entry_name, *entry_desc, *intermission_check, *table;

    dialog=gtk_dialog_new_with_buttons(_("Create New Cave"), GTK_WINDOW(gd_editor_window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_NEW, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    table=gtk_table_new(0, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG (dialog)->vbox), table, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER (table), 6);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);

    /* some properties - name */
    gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_printf (_("Name:")), 0, 1, 0, 1);
    entry_name=gtk_entry_new();
    /* little inconsistency below: max length has unicode characters, while gdstring will have utf-8.
       however this does not make too much difference */
    gtk_entry_set_max_length(GTK_ENTRY(entry_name), sizeof(GdString));
    gtk_entry_set_activates_default(GTK_ENTRY(entry_name), TRUE);
    gtk_entry_set_text(GTK_ENTRY(entry_name), _("New cave"));
    gtk_table_attach_defaults(GTK_TABLE(table), entry_name, 1, 2, 0, 1);
    
    /* description */
    gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_printf (_("Description:")), 0, 1, 1, 2);
    entry_desc=gtk_entry_new();
    /* little inconsistency below: max length has unicode characters, while gdstring will have utf-8.
       however this does not make too much difference */
    gtk_entry_set_max_length(GTK_ENTRY(entry_desc), sizeof(GdString));
    gtk_entry_set_activates_default(GTK_ENTRY(entry_desc), TRUE);
    gtk_table_attach_defaults(GTK_TABLE(table), entry_desc, 1, 2, 1, 2);

    /* intermission */
    gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_printf (_("Intermission:")), 0, 1, 2, 3);
    intermission_check=gtk_check_button_new();
    gtk_widget_set_tooltip_text(intermission_check, _("Intermission caves are usually small and fast caves, which are not required to be solved. The player will not lose a life if he is not successful. The game always proceeds to the next cave. If you set this check box, the size of the cave will also be set to 20x12, as that is the standard size for intermissions."));
    gtk_table_attach_defaults(GTK_TABLE(table), intermission_check, 1, 2, 2, 3);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        /* new cave */    
        newcave=gd_cave_new();
        
        /* set some defaults */
        gd_cave_set_random_colors(newcave, gd_preferred_palette);
        gd_strcpy(newcave->name, gtk_entry_get_text(GTK_ENTRY(entry_name)));
        gd_strcpy(newcave->description, gtk_entry_get_text(GTK_ENTRY(entry_desc)));
        gd_strcpy(newcave->author, g_get_real_name());
        newcave->intermission=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(intermission_check));
        /* if the user says that he creates an intermission, it is immediately resized */
        if (newcave->intermission) {
            newcave->w=20;
            newcave->h=12;
            gd_cave_correct_visible_size(newcave);
        }
        gd_strcpy(newcave->date, gd_get_current_date());
        /* default speeds - sensible defaults, as in bd2. */
        newcave->level_speed[0]=180;
        newcave->level_speed[1]=160;
        newcave->level_speed[2]=140;
        newcave->level_speed[3]=120;
        newcave->level_speed[4]=120;

        select_cave_for_edit(newcave);        /* close caveset icon view, and show cave */
        gd_caveset=g_list_append(gd_caveset, newcave);    /* append here, as the icon view may only be destroyed now by select_cave_for_edit */
        gd_caveset_edited=TRUE;
    }
    gtk_widget_destroy(dialog);
}


/*
 * caveset file operations.
 * in each, we destroy the iconview, as it might store the modified order of caves!
 * then it is possible to load, save, and the like.
 * after any operation, activate caveset editor again
 */
static void
open_caveset_cb(GtkWidget *widget, gpointer data)
{
    /* destroy icon view so it does not interfere */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    g_hash_table_remove_all(cave_pixbufs);
    gd_open_caveset (gd_editor_window, NULL);
    gd_main_window_set_title();
    select_cave_for_edit(NULL);
}

static void
open_installed_caveset_cb(GtkWidget *widget, gpointer data)
{
    /* destroy icon view so it does not interfere */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    g_hash_table_remove_all(cave_pixbufs);
    gd_open_caveset (gd_editor_window, gd_system_caves_dir);
    gd_main_window_set_title();
    select_cave_for_edit(NULL);
}

static void
save_caveset_as_cb(GtkWidget *widget, gpointer data)
{
    GdCave *edited;
    
    edited=edited_cave;
    /* destroy icon view so it does not interfere with the cave order, and the order of caves is saved */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    gd_save_caveset_as(gd_editor_window);
    select_cave_for_edit(edited);    /* go back to edited cave or recreate icon view */
}

static void
save_caveset_cb(GtkWidget *widget, gpointer data)
{
    GdCave *edited;
    
    edited=edited_cave;
    /* destroy icon view so it does not interfere with the cave order, and the order of caves is saved */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    gd_save_caveset(gd_editor_window);
    select_cave_for_edit(edited);    /* go back to edited cave or recreate icon view */
}

static void
new_caveset_cb(GtkWidget *widget, gpointer data)
{
    GDate *date;
    
    /* destroy icon view so it does not interfere */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    if (gd_discard_changes(gd_editor_window)) {
        gd_caveset_clear();
        g_hash_table_remove_all(cave_pixbufs);
    }
    select_cave_for_edit(NULL);

    date=g_date_new();
    g_date_set_time_t(date, time(NULL));
    g_date_strftime(gd_caveset_data->date, sizeof(gd_caveset_data->date), "%Y-%m-%d", date);
    g_date_free(date);
    
    gd_strcpy(gd_caveset_data->author, g_get_real_name());

    caveset_properties(FALSE);    /* false=do not show cancel button */
}


static void
remove_all_unknown_tags_cb(GtkWidget *widget, gpointer data)
{
    GdCave *edited;
    gboolean response;
    
    edited=edited_cave;
    /* destroy icon view so it does not interfere with the cave order, and the order of caves is saved */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);

    response=gd_question_yesno(_("Do you really want to remove unknown cave tags?"), _("This operation removes all unknown tags associated with all caves. Unknown tags might come from another BDCFF-compatible game or an older version of GDash. Those cave options cannot be interpreted by GDash, and therefore if you use this caveset in this application, they are of no use."));

    if (response) {
        GList *iter;
        
        for(iter=gd_caveset; iter!=NULL; iter=iter->next) {
            GdCave *cave=(GdCave *)iter->data;
            
            g_hash_table_remove_all(cave->tags);
        }
    }

    select_cave_for_edit(edited);    /* go back to edited cave or recreate icon view */
}





/* make all caves selectable */
static void
selectable_all_cb(GtkWidget *widget, gpointer data)
{
    GList *iter;

    gtk_widget_destroy(iconview_cavelist);    /* to generate gd_caveset */
    for (iter=gd_caveset; iter!=NULL; iter=iter->next) {
        GdCave *cave=(GdCave *)iter->data;
        
        if (!cave->selectable) {
            cave->selectable=TRUE;
            g_hash_table_remove(cave_pixbufs, cave);
        }
    }
    select_cave_for_edit(NULL);
}

/* make all but intermissions selectable */
static void
selectable_all_but_intermissions_cb(GtkWidget *widget, gpointer data)
{
    GList *iter;
    
    gtk_widget_destroy(iconview_cavelist);    /* to generate gd_caveset */
    for (iter=gd_caveset; iter!=NULL; iter=iter->next) {
        GdCave *cave=(GdCave *)iter->data;
        gboolean desired=!cave->intermission;
        
        if (cave->selectable!=desired) {
            cave->selectable=desired;
            g_hash_table_remove(cave_pixbufs, cave);
        }
    }
    select_cave_for_edit(NULL);
}


/* make all after intermissions selectable */
static void
selectable_all_after_intermissions_cb(GtkWidget *widget, gpointer data)
{
    gboolean was_intermission=TRUE;    /* treat the 'zeroth' cave as intermission, so the very first cave will be selectable */
    GList *iter;
    
    gtk_widget_destroy(iconview_cavelist);    /* to generate gd_caveset */
    for (iter=gd_caveset; iter!=NULL; iter=iter->next) {
        GdCave *cave=(GdCave *)iter->data;
        gboolean desired;
        
        desired=!cave->intermission && was_intermission;    /* selectable if this is a normal cave, and the previous one was an interm. */
        if (cave->selectable!=desired) {
            cave->selectable=desired;
            g_hash_table_remove(cave_pixbufs, cave);
        }

        was_intermission=cave->intermission;    /* remember for next iteration */
    }
    select_cave_for_edit(NULL);
}





/******************************************************
 *
 * some necessary callbacks for the editor
 *
 *
 */

/* level shown in editor */
static void
level_scale_changed_cb(GtkWidget *widget, gpointer data)
{
    edit_level=gtk_range_get_value(GTK_RANGE (widget))-1;
    render_cave();                /* re-render cave */
}

/* new objects are created on this level - combo change updates variable */
static void
new_object_combo_changed_cb(GtkWidget *widget, gpointer data)
{
    int switch_to_level;

    switch_to_level=new_objects_visible_on[gtk_combo_box_get_active(GTK_COMBO_BOX(widget))].switch_to_level;
    if (switch_to_level>=1)
        gtk_range_set_value(GTK_RANGE(level_scale), switch_to_level);
}

/* edit tool selected */
static void
action_select_tool_cb(GtkWidget *widget, gpointer data)
{
    action=gtk_radio_action_get_current_value (GTK_RADIO_ACTION (widget));
    
    /* actions below zero are not real objects */
    if (action>0 || action==FREEHAND) {
        int number=action;    /* index in array */
        
        if (action==FREEHAND)    /* freehand tool places points, so we use the labels from the point */
            number=GD_POINT;
        
        /* first button - mainly for draw */
        gtk_label_set_text(GTK_LABEL(label_first_element), _(gd_object_description[number].first_button));
        gtk_widget_set_sensitive(element_button, gd_object_description[number].first_button!=NULL);
        gd_element_button_set_dialog_sensitive(element_button, gd_object_description[number].first_button!=NULL);
        if (gd_object_description[number].first_button) {
            char *title=g_strdup_printf(_("%s Element"), _(gd_object_description[number].first_button));
            gd_element_button_set_dialog_title(element_button, title);
            g_free(title);
        } else
            gd_element_button_set_dialog_title(element_button, _("Draw Element"));

        /* second button - mainly for fill */
        gtk_label_set_text(GTK_LABEL(label_second_element), _(gd_object_description[number].second_button));
        gtk_widget_set_sensitive(fillelement_button, gd_object_description[number].second_button!=NULL);
        gd_element_button_set_dialog_sensitive(fillelement_button, gd_object_description[number].second_button!=NULL);
        if (gd_object_description[number].second_button) {
            char *title=g_strdup_printf(_("%s Element"), _(gd_object_description[number].second_button));
            gd_element_button_set_dialog_title(fillelement_button, title);
            g_free(title);
        } else
            gd_element_button_set_dialog_title(fillelement_button, _("Fill Element"));
        
    } else {
        gtk_label_set_text(GTK_LABEL(label_first_element), NULL);
        gtk_widget_set_sensitive(element_button, FALSE);
        gd_element_button_set_dialog_sensitive(element_button, FALSE);
        gd_element_button_set_dialog_title(element_button, _("Draw Element"));
        gtk_label_set_text(GTK_LABEL(label_second_element), NULL);
        gtk_widget_set_sensitive(fillelement_button, FALSE);
        gd_element_button_set_dialog_sensitive(fillelement_button, FALSE);
        gd_element_button_set_dialog_title(fillelement_button, _("Fill Element"));
    }
}

static void
toggle_game_view_cb(GtkWidget *widget, gpointer data)
{
    gd_game_view=gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));
}

static void
toggle_colored_objects_cb(GtkWidget *widget, gpointer data)
{
    gd_colored_objects=gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));
}

static void
toggle_object_list_cb(GtkWidget *widget, gpointer data)
{
    gd_show_object_list=gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));
    if (gd_show_object_list && edited_cave)
        gtk_widget_show (scroll_window_objects);    /* show the scroll window containing the view */
    else
        gtk_widget_hide (scroll_window_objects);
}

static void
toggle_test_label_cb(GtkWidget *widget, gpointer data)
{
    gd_show_test_label=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget));
}


static void
close_editor_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(gd_editor_window);
}




static void
show_highscore_cb(GtkWidget *widget, gpointer data)
{
    /* if the iconview is active, we destroy it, as it interferes with GList *caveset.
       after highscore editing, we call editcave(null) to recreate */
    gboolean has_iconview=iconview_cavelist!=NULL;
    
    if (has_iconview)
        gtk_widget_destroy(iconview_cavelist);
    gd_show_highscore(gd_editor_window, edited_cave, TRUE, NULL, -1);
    if (has_iconview)
        select_cave_for_edit(NULL);
}

static void
remove_bad_replays_cb(GtkWidget *widget, gpointer data)
{
    /* if the iconview is active, we destroy it, as it interferes with GList *caveset.
       after highscore editing, we call editcave(null) to recreate */
    gboolean has_iconview=iconview_cavelist!=NULL;
    GList *citer;
    GString *report;
    int sum;
    
    if (has_iconview)
        gtk_widget_destroy(iconview_cavelist);
    
    report=g_string_new(NULL);
    sum=0;
    for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
        GdCave *cave=(GdCave *)citer->data;
        int removed;
        
        removed=gd_cave_check_replays(cave, FALSE, TRUE, FALSE);
        sum+=removed;
        if (removed>0)
            g_string_append_printf(report, _("%s: removed %d replay(s)\n"), cave->name, removed);
    }
    
    if (has_iconview)
        select_cave_for_edit(NULL);
    if (sum>0) {
        gd_infomessage(_("Some replays were removed."), report->str);
        gd_caveset_edited=TRUE;
    }
    g_string_free(report, TRUE);
}



static void
mark_all_replays_as_working_cb(GtkWidget *widget, gpointer data)
{
    /* if the iconview is active, we destroy it, as it interferes with GList *caveset.
       after highscore editing, we call editcave(null) to recreate */
    gboolean has_iconview=iconview_cavelist!=NULL;
    GList *citer;
    GString *report;
    int sum;
    
    if (has_iconview)
        gtk_widget_destroy(iconview_cavelist);
    
    report=g_string_new(NULL);
    sum=0;
    for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
        GdCave *cave=(GdCave *)citer->data;
        int changed;
        
        changed=gd_cave_check_replays(cave, FALSE, FALSE, TRUE);
        sum+=changed;
        if (changed>0)
            g_string_append_printf(report, _("%s: marked %d replay(s) as working ones\n"), cave->name, changed);
    }
    
    if (has_iconview)
        select_cave_for_edit(NULL);
    if (sum>0) {
        gd_warningmessage(_("Some replay checksums were recalculated. This does not mean that those replays actually play correctly!"), report->str);
        gd_caveset_edited=TRUE;
    }
    g_string_free(report, TRUE);
}







/* set image from gd_caveset_data title screen. */
static void
setup_caveset_title_image_load_image(GtkWidget *image)
{
    GdkPixbuf *bigone;

    bigone=gd_create_title_image();
    if (!bigone)
        gtk_image_clear(GTK_IMAGE(image));
    else {
        gtk_image_set_from_pixbuf(GTK_IMAGE(image), bigone);
        g_object_unref(bigone);
    }
}

/* forgets caveset title image. */
static void
setup_caveset_title_image_clear_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *image=(GtkWidget *)data;
    
    g_string_assign(gd_caveset_data->title_screen, "");
    g_string_assign(gd_caveset_data->title_screen_scroll, "");
    setup_caveset_title_image_load_image(image);
}

/* load image from disk */
static void
setup_caveset_title_image_load_image_into_string(const char *title, GtkWidget *parent, GtkWidget *image, GString *string, int maxwidth, int maxheight)
{
    char *filename;
    GdkPixbuf *pixbuf;
    GError *error=NULL;
    gchar *buffer;
    gsize bufsize;
    gchar *base64;
    int width, height;
    
    filename=gd_select_image_file(title, parent);
    if (!filename)    /* no filename -> do nothing */
        return;
    
    /* check image format and size */
    if (!gdk_pixbuf_get_file_info(filename, &width, &height)) {
        gd_errormessage(_("Error loading image file."), _("Cannot recognize file format."));
        return;
    }
    /* check for maximum size */
    if (height>maxheight || width>maxwidth) {
        char *secondary;
        
        secondary=g_strdup_printf(_("Maximum sizes: %dx%d pixels"), maxwidth, maxheight);
        gd_errormessage(_("The image selected is too big!"), secondary);
        g_free(secondary);
        return;
    }
    
    /* load pixbuf */
    pixbuf=gdk_pixbuf_new_from_file(filename, &error);
    if (!pixbuf) {
        /* cannot load image - do nothing */
        gd_errormessage(_("Error loading image file."), error->message);
        g_error_free(error);
        return;
    }
    
    /* now the image is loaded, "save" as png into a buffer */
    gdk_pixbuf_save_to_buffer(pixbuf, &buffer, &bufsize, "png", &error, "compression", "9", NULL);
    g_object_unref(pixbuf);    /* not needed anymore */
    if (error) {
        /* cannot load image - do nothing */
        gd_errormessage("Internal error: %s", error->message);
        g_error_free(error);
        return;
    }
    base64=g_base64_encode((guchar *) buffer, bufsize);
    g_free(buffer);    /* binary data can be freed */
    g_string_assign(string, base64);
    g_free(base64);    /* copied to string so can be freed */
    setup_caveset_title_image_load_image(image);
    gd_caveset_edited=TRUE;
}

static void
setup_caveset_title_image_load_screen_cb(GtkWidget *widget, gpointer data)
{
    setup_caveset_title_image_load_image_into_string(_("Select Image File for Title Screen"), gtk_widget_get_toplevel(widget),
        (GtkWidget *)data, gd_caveset_data->title_screen, GD_TITLE_SCREEN_MAX_WIDTH, GD_TITLE_SCREEN_MAX_HEIGHT);
}

static void
setup_caveset_title_image_load_tile_cb(GtkWidget *widget, gpointer data)
{
    setup_caveset_title_image_load_image_into_string(_("Select Image File for Background Tile"), gtk_widget_get_toplevel(widget),
        (GtkWidget *)data, gd_caveset_data->title_screen_scroll, GD_TITLE_SCROLL_MAX_WIDTH, GD_TITLE_SCROLL_MAX_HEIGHT);
}

/* load images for the caveset title screen */
static void
setup_caveset_title_image_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *frame, *align, *image, *bbox;
    GtkWidget *setbutton, *settilebutton, *clearbutton;
    int result;
    char *hint;
    
    dialog=gtk_dialog_new_with_buttons(_("Set Title Image"), GTK_WINDOW(gd_editor_window), 0,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    
    /* an align, so the image shrinks to its size, and not the vbox determines its width. */
    align=gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 6);
    /* a frame around the image */
    frame=gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(align), frame);
    image=gtk_image_new();
    gtk_widget_set_size_request(image, GD_TITLE_SCREEN_MAX_WIDTH, GD_TITLE_SCREEN_MAX_HEIGHT);    /* max title image size */
    gtk_container_add(GTK_CONTAINER(frame), image);
    
    setup_caveset_title_image_load_image(image);
    bbox=gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), bbox, FALSE, FALSE, 6);
    gtk_container_add(GTK_CONTAINER(bbox), setbutton=gtk_button_new_with_mnemonic(_("Load _image")));
    gtk_container_add(GTK_CONTAINER(bbox), settilebutton=gtk_button_new_with_mnemonic(_("Load _tile")));
    gtk_container_add(GTK_CONTAINER(bbox), clearbutton=gtk_button_new_from_stock(GTK_STOCK_CLEAR));
    g_signal_connect(clearbutton, "clicked", G_CALLBACK(setup_caveset_title_image_clear_cb), image);
    g_signal_connect(setbutton, "clicked", G_CALLBACK(setup_caveset_title_image_load_screen_cb), image);
    g_signal_connect(settilebutton, "clicked", G_CALLBACK(setup_caveset_title_image_load_tile_cb), image);
    
    hint=g_strdup_printf(_("Recommended image sizes are 320x176 pixels for title image and 8x8 pixels for the scrolling tile. Maximum sizes are %dx%d and %dx%d, respectively."), GD_TITLE_SCREEN_MAX_WIDTH, GD_TITLE_SCREEN_MAX_HEIGHT, GD_TITLE_SCROLL_MAX_WIDTH, GD_TITLE_SCROLL_MAX_HEIGHT);
    gd_dialog_add_hint(GTK_DIALOG(dialog), hint);
    g_free(hint);

    gtk_widget_show_all(dialog);
    result=gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    /* main window update title image */
    gd_main_window_set_title_animation();
}









/*
 *
 * start cave editor.
 *
 */

void
gd_open_cave_editor()
{
    /* normal menu items */
    static GtkActionEntry action_entries_normal[]={
        {"FileMenu", NULL, N_("_File")},
        {"CaveMenu", NULL, N_("_Cave")},
        {"EditMenu", NULL, N_("_Edit")},
        {"ViewMenu", NULL, N_("_View")},
        {"ToolsMenu", NULL, N_("_Tools")},
        {"HelpMenu", NULL, N_("_Help")},
        {"Close", GTK_STOCK_CLOSE, NULL, NULL, N_("Close cave editor"), G_CALLBACK(close_editor_cb)},
        {"NewCave", GTK_STOCK_NEW, N_("New _cave"), NULL, N_("Create new cave"), G_CALLBACK(new_cave_cb)},
        {"Help", GTK_STOCK_HELP, NULL, NULL, NULL, G_CALLBACK(help_cb)},
        {"SaveFile", GTK_STOCK_SAVE, NULL, NULL, N_("Save cave set to file"), G_CALLBACK(save_caveset_cb)},
        {"SaveAsFile", GTK_STOCK_SAVE_AS, NULL, NULL, N_("Save cave set as new file"), G_CALLBACK(save_caveset_as_cb)},
        {"OpenFile", GTK_STOCK_OPEN, NULL, NULL, N_("Load cave set from file"), G_CALLBACK(open_caveset_cb)},
        {"OpenInstalledFile", GTK_STOCK_CDROM, N_("O_pen shipped"), NULL, N_("Load shipped cave set from file"), G_CALLBACK(open_installed_caveset_cb)},
        {"HighScore", GD_ICON_AWARD, N_("Hi_ghscores"), NULL, NULL, G_CALLBACK(show_highscore_cb)},
        {"SelectAll", GTK_STOCK_SELECT_ALL, NULL, "<control>A", N_("Select all items"), G_CALLBACK(select_all_cb)},
        {"CaveSetProps", GTK_STOCK_PROPERTIES, N_("Cave set _properties"), NULL, N_("Set properties of cave set"), G_CALLBACK(set_caveset_properties_cb)},
        {"CaveSetTitleImage", GD_ICON_IMAGE, N_("Cave set _title image"), NULL, N_("Set caveset title image"), G_CALLBACK(setup_caveset_title_image_cb)},
    };

    /* cave_selector menu items */
    static const GtkActionEntry action_entries_cave_selector[]={
        {"EditCave", GD_ICON_CAVE_EDITOR, N_("_Edit cave"), NULL, N_("Edit selected cave"), G_CALLBACK(icon_view_edit_cave_cb)},
        {"RenameCave", NULL, N_("_Rename cave"), NULL, N_("Rename selected cave"), G_CALLBACK(icon_view_rename_cave_cb)},
        {"MakeSelectable", NULL, N_("Make cave _selectable"), NULL, N_("Make the cave selectable as game start"), G_CALLBACK(icon_view_cave_make_selectable_cb)},
        {"MakeUnselectable", NULL, N_("Make cave _unselectable"), NULL, N_("Make the cave unselectable as game start"), G_CALLBACK(icon_view_cave_make_unselectable_cb)},
    };

    /* caveset editing */
    static const GtkActionEntry action_entries_edit_caveset[]={
        {"NewCaveset", GTK_STOCK_NEW, N_("_New cave set"), "", N_("Create new cave set with no caves"), G_CALLBACK(new_caveset_cb)},
        {"SaveHTML", GTK_STOCK_FILE, N_("Save _HTML gallery"), NULL, N_("Save game in a HTML gallery"), G_CALLBACK(save_html_cb)},
        {"ExportCavePack", GTK_STOCK_CONVERT, N_("Export _CrLi cave pack"), NULL, NULL, G_CALLBACK(export_cavepack_cb)},
        {"SelectMenu", NULL, N_("_Make caves selectable")},
        {"AllCavesSelectable", NULL, N_("All _caves"), NULL, N_("Make all caves selectable as game start"), G_CALLBACK(selectable_all_cb)},
        {"AllButIntermissionsSelectable", NULL, N_("All _but intermissions"), NULL, N_("Make all caves but intermissions selectable as game start"), G_CALLBACK(selectable_all_but_intermissions_cb)},
        {"AllAfterIntermissionsSelectable", NULL, N_("All _after intermissions"), NULL, N_("Make all caves after intermissions selectable as game start"), G_CALLBACK(selectable_all_after_intermissions_cb)},
        {"RemoveAllUnknownTags", NULL, N_("Remove all unknown tags"), NULL, N_("Removes all unknown tags found in the BDCFF file"), G_CALLBACK(remove_all_unknown_tags_cb)},
        {"RemoveBadReplays", NULL, N_("Remove bad replays"), NULL, N_("Removes replays which won't play as they have their caves modified."), G_CALLBACK(remove_bad_replays_cb)},
        {"MarkAllReplaysAsWorking", NULL, N_("Fix replay checksums"), NULL, N_("Treats all replays with wrong checksums as working ones."), G_CALLBACK(mark_all_replays_as_working_cb)},
    };

    /* cave editing menu items */
    static const GtkActionEntry action_entries_edit_cave[]={
        {"ExportAsCrLiCave", GTK_STOCK_CONVERT, N_("_Export as CrLi cave file"), NULL, NULL, G_CALLBACK(export_cavefile_cb)},
        {"CaveSelector", GTK_STOCK_INDEX, NULL, "Escape", N_("Open cave selector"), G_CALLBACK(cave_selector_cb)},
        {"NextCave", GTK_STOCK_GO_FORWARD, N_("_Next cave"), "Page_Down", N_("Next cave"), G_CALLBACK(next_cave_cb)},
        {"PreviousCave", GTK_STOCK_GO_BACK, N_("_Previous cave"), "Page_Up", N_("Previous cave"), G_CALLBACK(previous_cave_cb)},
        {"Test", GTK_STOCK_MEDIA_PLAY, N_("_Test"), "<control>T", N_("Test cave"), G_CALLBACK(play_level_cb)},
        {"CaveProperties", GTK_STOCK_PROPERTIES, N_("Ca_ve properties"), NULL, N_("Cave settings"), G_CALLBACK(cave_properties_cb)},
        {"EngineDefaults", NULL, N_("Set engine defaults")},
        {"CaveColors", GTK_STOCK_SELECT_COLOR, N_("Cave co_lors"), NULL, N_("Select cave colors"), G_CALLBACK(cave_colors_cb)},
        {"RemoveObjects", GTK_STOCK_CLEAR, N_("Remove objects"), NULL, N_("Clear cave objects"), G_CALLBACK(clear_cave_elements_cb)},
        {"FlattenCave", NULL, N_("Convert to map"), NULL, N_("Flatten cave to a single cave map without objects"), G_CALLBACK(flatten_cave_cb)},
        {"Overview", GTK_STOCK_ZOOM_FIT, N_("O_verview"), NULL, N_("Full screen overview of cave"), G_CALLBACK(cave_overview_cb)},
        {"OverviewSimple", GTK_STOCK_ZOOM_FIT, N_("O_verview (simple)"), NULL, N_("Full screen overview of cave almost as in game"), G_CALLBACK(cave_overview_simple_cb)},
        {"AutoShrink", NULL, N_("_Auto shrink"), NULL, N_("Automatically set the visible region of the cave"), G_CALLBACK(auto_shrink_cave_cb)},
    };

    /* action entries which relate to a selected cave element (line, rectangle...) */
    static const GtkActionEntry action_entries_edit_object[]={
        {"Bottom", GD_ICON_TO_BOTTOM, N_("To _bottom"), "<control>End", N_("Push object to bottom"), G_CALLBACK(bottom_selected_cb)},
        {"Top", GD_ICON_TO_TOP, N_("To t_op"), "<control>Home", N_("Bring object to top"), G_CALLBACK(top_selected_cb)},
        {"ShowOnThisLevel", GTK_STOCK_ADD, N_("Show on this level"), NULL, N_("Enable object on currently visible level"), G_CALLBACK(show_object_on_this_level_cb)},
        {"HideOnThisLevel", GTK_STOCK_REMOVE, N_("Hide on this level"), NULL, N_("Disable object on currently visible level"), G_CALLBACK(hide_object_on_this_level_cb)},
        {"OnlyOnThisLevel", NULL, N_("Only on this level"), NULL, N_("Enable object only on the currently visible level"), G_CALLBACK(show_object_this_level_only_cb)},
        {"ShowOnAllLevels", NULL, N_("Show on all levels"), NULL, N_("Enable object on all levels"), G_CALLBACK(show_object_all_levels_cb)},
    };

    static const GtkActionEntry action_entries_edit_one_object[]={
        {"ObjectProperties", GTK_STOCK_PREFERENCES, N_("Ob_ject properties"), NULL, N_("Set object properties"), G_CALLBACK(object_properties_cb)},
    };

    /* map actions */
    static const GtkActionEntry action_entries_edit_map[]={
        {"MapMenu", NULL, N_("Map")},
        {"ShiftLeft", GTK_STOCK_GO_BACK, N_("Shift _left"), NULL, NULL, G_CALLBACK(shift_left_cb)},
        {"ShiftRight", GTK_STOCK_GO_FORWARD, N_("Shift _right"), NULL, NULL, G_CALLBACK(shift_right_cb)},
        {"ShiftUp", GTK_STOCK_GO_UP, N_("Shift _up"), NULL, NULL, G_CALLBACK(shift_up_cb)},
        {"ShiftDown", GTK_STOCK_GO_DOWN, N_("Shift _down"), NULL, NULL, G_CALLBACK(shift_down_cb)},
        {"RemoveMap", GTK_STOCK_CLEAR, N_("Remove m_ap"), NULL, N_("Remove cave map, if it has one"), G_CALLBACK(remove_map_cb)},
    };

    /* random element actions */
    static const GtkActionEntry action_entries_edit_random[]={
        {"SetupRandom", GD_ICON_RANDOM_FILL, N_("Setup cave _random fill"), NULL, N_("Setup initial fill random elements for the cave"), G_CALLBACK(cave_random_setup_cb)},
    };
    
    /* clipboard actions */
    static const GtkActionEntry action_entries_clipboard[]={
        {"Cut", GTK_STOCK_CUT, NULL, NULL, N_("Cut to clipboard"), G_CALLBACK(cut_selected_cb)},
        {"Copy", GTK_STOCK_COPY, NULL, NULL, N_("Copy to clipboard"), G_CALLBACK(copy_selected_cb)},
        {"Delete", GTK_STOCK_DELETE, NULL, "Delete", N_("Delete"), G_CALLBACK(delete_selected_cb)},
    };

    /* clipboard paste */
    static const GtkActionEntry action_entries_clipboard_paste[]={
        {"Paste", GTK_STOCK_PASTE, NULL, NULL, N_("Paste object from clipboard"), G_CALLBACK(paste_clipboard_cb)},
    };

    /* action entries for undo */
    static const GtkActionEntry action_entries_edit_undo[]={
        {"Undo", GTK_STOCK_UNDO, NULL, "<control>Z", N_("Undo last action"), G_CALLBACK(undo_cb)},
    };

    /* action entries for redo */
    static const GtkActionEntry action_entries_edit_redo[]={
        {"Redo", GTK_STOCK_REDO, NULL, "<control><shift>Z", N_("Redo last action"), G_CALLBACK(redo_cb)},
    };

    /* edit commands */
    /* defined at the top of the file! */

    /* toggle buttons: nonstatic as they use values from settings */
    const GtkToggleActionEntry action_entries_toggle[]={
        {"SimpleView", NULL, N_("_Animated view"), NULL, N_("Animated view"), G_CALLBACK(toggle_game_view_cb), gd_game_view},
        {"ColoredObjects", NULL, N_("_Colored objects"), NULL, N_("Cave objects are colored"), G_CALLBACK(toggle_colored_objects_cb), gd_colored_objects},
        {"ShowObjectList", GTK_STOCK_INDEX, N_("_Object list"), "F9", N_("Object list sidebar"), G_CALLBACK(toggle_object_list_cb), gd_show_object_list},
        {"ShowTestLabel", GTK_STOCK_INDEX, N_("_Show variables in test"), NULL, N_("Show a label during tests with some cave parameters"), G_CALLBACK(toggle_test_label_cb), gd_show_test_label}
    };

    static const char *ui_info =
        "<ui>"

        "<popup name='DrawingAreaPopup'>"
            "<menuitem action='Undo'/>"
            "<menuitem action='Redo'/>"
            "<separator/>"
            "<menuitem action='Cut'/>"
            "<menuitem action='Copy'/>"
            "<menuitem action='Paste'/>"
            "<menuitem action='Delete'/>"
            "<separator/>"
            "<menuitem action='Top'/>"
            "<menuitem action='Bottom'/>"
            "<separator/>"
            "<menuitem action='ShowOnThisLevel'/>"
            "<menuitem action='HideOnThisLevel'/>"
            "<menuitem action='OnlyOnThisLevel'/>"
            "<menuitem action='ShowOnAllLevels'/>"
            "<separator/>"
            "<menuitem action='ObjectProperties'/>"
            "<separator/>"
            "<menuitem action='CaveProperties'/>"
        "</popup>"

        "<popup name='CavesetPopup'>"
            "<menuitem action='Cut'/>"
            "<menuitem action='Copy'/>"
            "<menuitem action='Paste'/>"
            "<menuitem action='Delete'/>"
            "<separator/>"
            "<menuitem action='NewCave'/>"
            "<menuitem action='EditCave'/>"
            "<menuitem action='RenameCave'/>"
            "<menuitem action='MakeSelectable'/>"
            "<menuitem action='MakeUnselectable'/>"
        "</popup>"

        "<popup name='ObjectListPopup'>"
            "<menuitem action='Cut'/>"
            "<menuitem action='Copy'/>"
            "<menuitem action='Paste'/>"
            "<menuitem action='Delete'/>"
            "<separator/>"
            "<menuitem action='ShowOnThisLevel'/>"
            "<menuitem action='HideOnThisLevel'/>"
            "<menuitem action='OnlyOnThisLevel'/>"
            "<menuitem action='ShowOnAllLevels'/>"
            "<separator/>"
            "<menuitem action='Top'/>"
            "<menuitem action='Bottom'/>"
            "<menuitem action='ObjectProperties'/>"
        "</popup>"

        "<menubar name='MenuBar'>"
            "<menu action='FileMenu'>"
                "<menuitem action='NewCave'/>"
                "<menuitem action='NewCaveset'/>"
                "<menuitem action='EditCave'/>"
                "<menuitem action='RenameCave'/>"
                "<menuitem action='MakeSelectable'/>"
                "<menuitem action='MakeUnselectable'/>"
                "<separator/>"
                "<menuitem action='OpenFile'/>"
                "<menuitem action='OpenInstalledFile'/>"
                "<separator/>"
                "<menuitem action='SaveFile'/>"
                "<menuitem action='SaveAsFile'/>"
                "<separator/>"
                "<menuitem action='CaveSelector'/>"
                "<menuitem action='PreviousCave'/>"
                "<menuitem action='NextCave'/>"
                "<menuitem action='CaveSetProps'/>"
                "<menuitem action='CaveSetTitleImage'/>"
                "<menu action='SelectMenu'>"
                    "<menuitem action='AllCavesSelectable'/>"
                    "<menuitem action='AllButIntermissionsSelectable'/>"
                    "<menuitem action='AllAfterIntermissionsSelectable'/>"
                "</menu>"
                "<separator/>"
                "<menuitem action='ExportCavePack'/>"
                "<menuitem action='ExportAsCrLiCave'/>"
                "<menuitem action='SaveHTML'/>"
                "<menuitem action='RemoveAllUnknownTags'/>"
                "<menuitem action='RemoveBadReplays'/>"
                "<menuitem action='MarkAllReplaysAsWorking'/>"
                "<separator/>"
                "<menuitem action='Close'/>"
            "</menu>"
            "<menu action='EditMenu'>"
                "<menuitem action='Undo'/>"
                "<menuitem action='Redo'/>"
                "<separator/>"
                "<menuitem action='Cut'/>"
                "<menuitem action='Copy'/>"
                "<menuitem action='Paste'/>"
                "<menuitem action='Delete'/>"
                "<separator/>"
                "<menuitem action='SelectAll'/>"
                "<separator/>"
                "<menuitem action='Top'/>"
                "<menuitem action='Bottom'/>"
                "<menuitem action='ShowOnThisLevel'/>"
                "<menuitem action='HideOnThisLevel'/>"
                "<menuitem action='OnlyOnThisLevel'/>"
                "<menuitem action='ShowOnAllLevels'/>"
                "<menuitem action='ObjectProperties'/>"
                "<separator/>"
                "<menuitem action='SetupRandom'/>"
                "<menuitem action='CaveColors'/>"
                "<separator/>"
                "<menuitem action='HighScore'/>"
                "<menuitem action='EngineDefaults'/>"
                "<menuitem action='CaveProperties'/>"
            "</menu>"
            "<menu action='ViewMenu'>"
                "<menuitem action='Overview'/>"
                "<menuitem action='OverviewSimple'/>"
                "<separator/>"
                "<menuitem action='SimpleView'/>"
                "<menuitem action='ColoredObjects'/>"
                "<menuitem action='ShowObjectList'/>"
                "<menuitem action='ShowTestLabel'/>"
            "</menu>"
            "<menu action='ToolsMenu'>"
                "<menuitem action='Test'/>"
                "<separator/>"
                "<menuitem action='Move'/>"
                "<menuitem action='Plot'/>"
                "<menuitem action='Freehand'/>"
                "<menuitem action='Line'/>"
                "<menuitem action='Rectangle'/>"
                "<menuitem action='FilledRectangle'/>"
                "<menuitem action='Raster'/>"
                "<menuitem action='Join'/>"
                "<menuitem action='FloodFillBorder'/>"
                "<menuitem action='FloodFillReplace'/>"
                "<menuitem action='RandomFill'/>"
                "<menuitem action='Maze'/>"
                "<menuitem action='UnicursalMaze'/>"
                "<menuitem action='BraidMaze'/>"
                "<menuitem action='CopyPaste'/>"
                "<separator/>"
                "<menuitem action='Visible'/>"
                "<menuitem action='AutoShrink'/>"
                "<separator/>"
                "<menuitem action='FlattenCave'/>"
                "<menu action='MapMenu'>"
                    "<menuitem action='ShiftLeft'/>"
                    "<menuitem action='ShiftRight'/>"
                    "<menuitem action='ShiftUp'/>"
                    "<menuitem action='ShiftDown'/>"
                    "<separator/>"
                    "<menuitem action='RemoveMap'/>"
                "</menu>"
                "<menuitem action='RemoveObjects'/>"
            "</menu>"
            "<menu action='HelpMenu'>"
                "<menuitem action='Help'/>"
            "</menu>"
        "</menubar>"

        "<toolbar name='ToolBar'>"
            "<toolitem action='CaveSelector'/>"
            "<toolitem action='PreviousCave'/>"
            "<toolitem action='NextCave'/>"
            "<separator/>"
            "<toolitem action='ObjectProperties'/>"
            "<toolitem action='CaveProperties'/>"
            "<separator/>"
            "<toolitem action='Move'/>"
            "<toolitem action='Plot'/>"
            "<toolitem action='Freehand'/>"
            "<toolitem action='Line'/>"
            "<toolitem action='Rectangle'/>"
            "<toolitem action='FilledRectangle'/>"
            "<toolitem action='Raster'/>"
            "<toolitem action='Join'/>"
            "<toolitem action='FloodFillBorder'/>"
            "<toolitem action='FloodFillReplace'/>"
            "<toolitem action='RandomFill'/>"
            "<toolitem action='Maze'/>"
            "<toolitem action='UnicursalMaze'/>"
            "<toolitem action='BraidMaze'/>"
            "<toolitem action='CopyPaste'/>"
            "<separator/>"
            "<toolitem action='Test'/>"
        "</toolbar>"
        "</ui>";
    GtkWidget *vbox, *hbox;
    GtkUIManager *ui;
    GtkWidget *hbox_combo;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkWidget *menu;
    int i;

    if (gd_editor_window) {
        /* if exists, only show it to the user. */
        gtk_window_present (GTK_WINDOW (gd_editor_window));
        return;
    }
    
    /* hash table which stores cave pointer -> pixbufs. deleting a pixbuf calls g_object_unref. */
    cave_pixbufs=g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);

    gd_editor_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (gd_editor_window), gd_editor_window_width, gd_editor_window_height);
    g_signal_connect(G_OBJECT (gd_editor_window), "destroy", G_CALLBACK(gtk_widget_destroyed), &gd_editor_window);
    g_signal_connect(G_OBJECT (gd_editor_window), "destroy", G_CALLBACK(editor_window_destroy_event), NULL);

    vbox=gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (gd_editor_window), vbox);

    /* menu and toolbar */
    actions_edit_tools=gtk_action_group_new("edit_tools");
    gtk_action_group_set_translation_domain (actions_edit_tools, PACKAGE);
    gtk_action_group_add_radio_actions(actions_edit_tools, action_objects, G_N_ELEMENTS(action_objects), MOVE, G_CALLBACK(action_select_tool_cb), NULL);

    actions=gtk_action_group_new("edit_normal");
    gtk_action_group_set_translation_domain (actions, PACKAGE);
    gtk_action_group_add_actions(actions, action_entries_normal, G_N_ELEMENTS(action_entries_normal), NULL);

    actions_edit_object=gtk_action_group_new("edit_object");
    gtk_action_group_set_translation_domain (actions_edit_object, PACKAGE);
    gtk_action_group_add_actions(actions_edit_object, action_entries_edit_object, G_N_ELEMENTS(action_entries_edit_object), NULL);

    actions_edit_one_object=gtk_action_group_new("edit_one_object");
    gtk_action_group_set_translation_domain (actions_edit_one_object, PACKAGE);
    gtk_action_group_add_actions(actions_edit_one_object, action_entries_edit_one_object, G_N_ELEMENTS(action_entries_edit_one_object), NULL);

    actions_edit_map=gtk_action_group_new("edit_map");
    gtk_action_group_set_translation_domain (actions_edit_map, PACKAGE);
    gtk_action_group_add_actions(actions_edit_map, action_entries_edit_map, G_N_ELEMENTS(action_entries_edit_map), NULL);

    actions_edit_random=gtk_action_group_new("edit_random");
    gtk_action_group_set_translation_domain (actions_edit_random, PACKAGE);
    gtk_action_group_add_actions(actions_edit_random, action_entries_edit_random, G_N_ELEMENTS(action_entries_edit_random), NULL);

    actions_clipboard=gtk_action_group_new("clipboard");
    gtk_action_group_set_translation_domain (actions_clipboard, PACKAGE);
    gtk_action_group_add_actions(actions_clipboard, action_entries_clipboard, G_N_ELEMENTS(action_entries_clipboard), NULL);

    actions_clipboard_paste=gtk_action_group_new("clipboard_paste");
    gtk_action_group_set_translation_domain (actions_clipboard_paste, PACKAGE);
    gtk_action_group_add_actions(actions_clipboard_paste, action_entries_clipboard_paste, G_N_ELEMENTS(action_entries_clipboard_paste), NULL);

    actions_edit_undo=gtk_action_group_new("edit_undo");
    gtk_action_group_set_translation_domain (actions_edit_undo, PACKAGE);
    gtk_action_group_add_actions(actions_edit_undo, action_entries_edit_undo, G_N_ELEMENTS(action_entries_edit_undo), NULL);

    actions_edit_redo=gtk_action_group_new("edit_redo");
    gtk_action_group_set_translation_domain (actions_edit_redo, PACKAGE);
    gtk_action_group_add_actions(actions_edit_redo, action_entries_edit_redo, G_N_ELEMENTS(action_entries_edit_redo), NULL);

    actions_edit_cave=gtk_action_group_new("edit_cave");
    gtk_action_group_set_translation_domain (actions_edit_cave, PACKAGE);
    gtk_action_group_add_actions(actions_edit_cave, action_entries_edit_cave, G_N_ELEMENTS(action_entries_edit_cave), NULL);
    g_object_set (gtk_action_group_get_action (actions_edit_cave, "Test"), "is_important", TRUE, NULL);
    g_object_set (gtk_action_group_get_action (actions_edit_cave, "CaveSelector"), "is_important", TRUE, NULL);

    actions_edit_caveset=gtk_action_group_new("edit_caveset");
    gtk_action_group_set_translation_domain (actions_edit_caveset, PACKAGE);
    gtk_action_group_add_actions(actions_edit_caveset, action_entries_edit_caveset, G_N_ELEMENTS(action_entries_edit_caveset), NULL);

    actions_cave_selector=gtk_action_group_new("cave_selector");
    gtk_action_group_set_translation_domain (actions_cave_selector, PACKAGE);
    gtk_action_group_add_actions(actions_cave_selector, action_entries_cave_selector, G_N_ELEMENTS(action_entries_cave_selector), NULL);

    actions_toggle=gtk_action_group_new("toggles");
    gtk_action_group_set_translation_domain (actions_toggle, PACKAGE);
    gtk_action_group_add_toggle_actions(actions_toggle, action_entries_toggle, G_N_ELEMENTS(action_entries_toggle), NULL);

    ui=gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group(ui, actions, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_tools, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_map, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_random, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_object, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_one_object, 0);
    gtk_ui_manager_insert_action_group(ui, actions_clipboard, 0);
    gtk_ui_manager_insert_action_group(ui, actions_clipboard_paste, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_cave, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_caveset, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_undo, 0);
    gtk_ui_manager_insert_action_group(ui, actions_edit_redo, 0);
    gtk_ui_manager_insert_action_group(ui, actions_cave_selector, 0);
    gtk_ui_manager_insert_action_group(ui, actions_toggle, 0);
    gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget (ui, "/MenuBar"), FALSE, FALSE, 0);

    /* make a submenu, which contains the engine defaults compiled in. */
    i=0;
    menu=gtk_menu_new();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (ui, "/MenuBar/EditMenu/EngineDefaults")), menu);
    for (i=0; i<GD_ENGINE_INVALID; i++) {
        GtkWidget *menuitem=gtk_menu_item_new_with_label(gd_engines[i]);

        gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuitem);
        gtk_widget_show (menuitem);
        g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(set_engine_default_cb), GINT_TO_POINTER (i));
    }

    /* TOOLBARS */
    toolbars=gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), toolbars, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbars), gtk_ui_manager_get_widget (ui, "/ToolBar"), FALSE, FALSE, 0);
    gtk_toolbar_set_tooltips (GTK_TOOLBAR (gtk_ui_manager_get_widget (ui, "/ToolBar")), TRUE);
    gtk_toolbar_set_style(GTK_TOOLBAR(gtk_ui_manager_get_widget (ui, "/ToolBar")), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_window_add_accel_group(GTK_WINDOW (gd_editor_window), gtk_ui_manager_get_accel_group(ui));

    /* get popups and attach them to the window, so they are not destroyed (the window holds the ref) */    
    drawing_area_popup=gtk_ui_manager_get_widget (ui, "/DrawingAreaPopup");
    gtk_menu_attach_to_widget(GTK_MENU(drawing_area_popup), gd_editor_window, NULL);
    object_list_popup=gtk_ui_manager_get_widget (ui, "/ObjectListPopup");
    gtk_menu_attach_to_widget(GTK_MENU(object_list_popup), gd_editor_window, NULL);
    caveset_popup=gtk_ui_manager_get_widget (ui, "/CavesetPopup");
    gtk_menu_attach_to_widget(GTK_MENU(caveset_popup), gd_editor_window, NULL);
    
    g_object_unref (actions);
    g_object_unref (ui);

    /* combo boxes under toolbar */
    hbox_combo=gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(toolbars), hbox_combo, FALSE, FALSE, 0);

    /* currently shown level - gtkscale */
    level_scale=gtk_hscale_new_with_range (1.0, 5.0, 1.0);
    gtk_scale_set_digits(GTK_SCALE(level_scale), 0);
    gtk_scale_set_value_pos (GTK_SCALE(level_scale), GTK_POS_LEFT);
    g_signal_connect(G_OBJECT (level_scale), "value-changed", G_CALLBACK(level_scale_changed_cb), NULL);
    gtk_box_pack_end(GTK_BOX(hbox_combo), level_scale, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox_combo), gtk_label_new(_("Level shown:")), FALSE, FALSE, 0);

    /* "new object will be placed on" - combo */
    new_object_level_combo=gtk_combo_box_new_text();
    for (i=0; i<G_N_ELEMENTS(new_objects_visible_on); ++i)    
        gtk_combo_box_append_text(GTK_COMBO_BOX(new_object_level_combo), _(new_objects_visible_on[i].text));
    g_signal_connect(G_OBJECT(new_object_level_combo), "changed", G_CALLBACK(new_object_combo_changed_cb), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(new_object_level_combo), 0);
    gtk_box_pack_end(GTK_BOX(hbox_combo), new_object_level_combo, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox_combo), gtk_label_new(_("Draw on:")), FALSE, FALSE, 0);
    
    /* draw element */
    gtk_box_pack_start(GTK_BOX(hbox_combo), label_first_element=gtk_label_new(NULL), FALSE, FALSE, 0);
    element_button=gd_element_button_new(O_DIRT, TRUE, NULL);    /* combo box of object, default element dirt (not really important what it is) */
    gtk_widget_set_tooltip_text(element_button, _("Element used to draw points, lines, and rectangles. You can use middle-click to pick one from the cave."));
    gtk_box_pack_start(GTK_BOX(hbox_combo), element_button, TRUE, TRUE, 0);

    /* fill elements */
    gtk_box_pack_start(GTK_BOX(hbox_combo), label_second_element=gtk_label_new(NULL), FALSE, FALSE, 0);
    fillelement_button=gd_element_button_new(O_SPACE, TRUE, NULL);    /* combo box, default element space (not really important what it is) */
    gtk_widget_set_tooltip_text (fillelement_button, _("Element used to fill rectangles, and second element of joins. You can use Ctrl + middle-click to pick one from the cave."));
    gtk_box_pack_start(GTK_BOX(hbox_combo), fillelement_button, TRUE, TRUE, 0);
    
    /* hbox for drawing area and object list */
    hbox=gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);

    /* scroll window for drawing area and icon view ****************************************/
    scroll_window=gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), scroll_window);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    /* object list ***************************************/
    scroll_window_objects=gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), scroll_window_objects, FALSE, FALSE, 0);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_window_objects), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_window_objects), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    object_list=gtk_list_store_new(NUM_EDITOR_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
    object_list_tree_view=gtk_tree_view_new_with_model (GTK_TREE_MODEL (object_list));
    g_object_unref (object_list);
    g_signal_connect(G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (object_list_tree_view))), "changed", G_CALLBACK(object_list_selection_changed_signal), NULL);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)), GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(object_list_tree_view), TRUE);
    gtk_container_add (GTK_CONTAINER (scroll_window_objects), object_list_tree_view);
    /* two signals which are required to handle cave object drag-and-drop reordering */
    g_signal_connect(G_OBJECT(object_list), "row-changed", G_CALLBACK(object_list_row_changed), NULL);
    g_signal_connect(G_OBJECT(object_list), "row-deleted", G_CALLBACK(object_list_row_delete), NULL);
    /* object double-click: */
    g_signal_connect(G_OBJECT(object_list_tree_view), "row-activated", G_CALLBACK(object_list_row_activated), NULL);
    g_signal_connect(G_OBJECT(object_list_tree_view), "popup-menu", G_CALLBACK(object_list_show_popup_menu), NULL);
    g_signal_connect(G_OBJECT(object_list_tree_view), "button-press-event", G_CALLBACK(object_list_button_press_event), NULL);

    /* tree view column which holds all data */
    /* we do not allow sorting, as it disables drag and drop */
    column=gtk_tree_view_column_new();
    gtk_tree_view_column_set_spacing (column, 1);
    gtk_tree_view_column_set_title (column, _("_Objects"));
    renderer=gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "stock-id", LEVELS_PIXBUF_COLUMN, NULL);
    renderer=gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "stock-id", TYPE_PIXBUF_COLUMN, NULL);
    renderer=gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", ELEMENT_PIXBUF_COLUMN, NULL);
    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", TEXT_COLUMN, NULL);
    renderer=gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_end(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", FILL_PIXBUF_COLUMN, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (object_list_tree_view), column);
    
    /* something like a statusbar, maybe that would be nicer */
    hbox=gtk_hbox_new(FALSE, 6);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    label_coordinate=gtk_label_new("[x:   y:   ]");
    gtk_box_pack_start(GTK_BOX(hbox), label_coordinate, FALSE, FALSE, 0);
    label_object=gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), label_object, FALSE, FALSE, 0);

    edit_level=0;        /* view: level 1 */
    /* here we force selection and update, by calling the function twice */
    select_tool(GD_LINE);
    select_tool(MOVE);
    g_timeout_add(40, drawing_area_draw_timeout, drawing_area_draw_timeout);

    gtk_widget_show_all(gd_editor_window);
    gtk_window_present(GTK_WINDOW(gd_editor_window));
    /* to remember size; only attach signal after showing */
    g_signal_connect(G_OBJECT (gd_editor_window), "configure-event", G_CALLBACK(editor_window_configure_event), NULL);

    select_cave_for_edit(NULL);
}

