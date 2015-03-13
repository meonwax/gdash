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
#include <list>
#include <set>
#include <glib/gi18n.h>

#include "cave/cavestored.hpp"
#include "gtk/gtkui.hpp"
#include "gtk/gtkuisettings.hpp"
#include "cave/object/caveobject.hpp"
#include "cave/caveset.hpp"
#include "misc/printf.hpp"
#include "gtk/help.hpp"
#include "gtk/gtkscreen.hpp"
#include "editor/editorwidgets.hpp"
#include "editor/editorautowidgets.hpp"
#include "editor/exportcrli.hpp"
#include "editor/exporthtml.hpp"
#include "misc/logger.hpp"
#include "fileops/c64import.hpp"
#include "cave/gamerender.hpp"
#include "misc/util.hpp"
#include "cave/elementproperties.hpp"
#include "editor/editorcellrenderer.hpp"
#include "editor/exporthtml.hpp"
#include "settings.hpp"
#include "gtk/gtkpixbuffactory.hpp"
#include "framework/commands.hpp"
#include "framework/gameactivity.hpp"
#include "cave/gamecontrol.hpp"
#include "cave/titleanimation.hpp"

#include "sdl/sdlmainwindow.hpp"
#include "gtk/gtkmainwindow.hpp"

#include "cave/object/caveobjectboundaryfill.hpp"
#include "cave/object/caveobjectcopypaste.hpp"
#include "cave/object/caveobjectfillrect.hpp"
#include "cave/object/caveobjectfloodfill.hpp"
#include "cave/object/caveobjectjoin.hpp"
#include "cave/object/caveobjectline.hpp"
#include "cave/object/caveobjectmaze.hpp"
#include "cave/object/caveobjectpoint.hpp"
#include "cave/object/caveobjectrandomfill.hpp"
#include "cave/object/caveobjectraster.hpp"
#include "cave/object/caveobjectrectangle.hpp"

#include "editor/editor.hpp"


enum EditTool {
    TOOL_POINT,
    TOOL_LINE,
    TOOL_RECTANGLE,
    TOOL_FILLED_RECTANGLE,
    TOOL_RASTER,
    TOOL_JOIN,
    TOOL_FLOODFILL_REPLACE,
    TOOL_FLOODFILL_BORDER,
    TOOL_MAZE,
    TOOL_MAZE_UNICURSAL,
    TOOL_MAZE_BRAID,
    TOOL_RANDOM_FILL,
    TOOL_COPY_PASTE,
    TOOL_MOVE,
    TOOL_FREEHAND,
    TOOL_VISIBLE_REGION
};

static GtkWidget *caveset_popup, *object_list_popup, *drawing_area_popup, *level_scale, *new_object_level_combo;
static GtkActionGroup *actions, *actions_edit_tools, *actions_edit_cave, *actions_edit_caveset, *actions_edit_map,
    *actions_edit_random, *actions_edit_object, *actions_edit_one_object, *actions_cave_selector, *actions_toggle,
    *actions_clipboard_paste, *actions_edit_undo, *actions_edit_redo, *actions_clipboard;
static GtkWidget *gd_editor_window;
static GTKPixbufFactory *editor_pixbuf_factory;
EditorCellRenderer *editor_cell_renderer;
static guint timeout_id;

static EditTool action;  /* activated edit tool, like move, plot, line... can be a gdobject, or there are other indexes which have meanings. */
static int edit_level; /* level shown in the editor... does not really affect editing */

static int clicked_x, clicked_y, mouse_x, mouse_y;  /* variables for mouse movement handling */
static gboolean button1_clicked;    /* true if we got button1 press event, then set to false on release */

static std::list<CaveStored> undo_caves, redo_caves;
static gboolean undo_move_flag=FALSE;   /* this is set to false when the mouse is clicked. on any movement, undo is saved and set to true */

static CaveStored *edited_cave; /* cave data actually edited in the editor */
static CaveRendered *rendered_cave;   /* a cave with all objects rendered, and to be drawn */

static CaveObjectStore object_clipboard;  /* cave object clipboard. */
static std::list<CaveStored> cave_clipboard;    /* copied caves */


static GtkWidget *scroll_window, *scroll_window_objects;
static GtkWidget *iconview_cavelist, *drawing_area;
static GTKScreen *screen;
static GtkWidget *toolbars;
static GtkWidget *element_button, *fillelement_button;
static GtkWidget *label_coordinate, *label_object, *label_first_element, *label_second_element;
static GHashTable *cave_pixbufs;    /* currently better choice than std::map as it has a destroy notifier to free the pixbufs */

static CaveMap<int> gfx_buffer;
static CaveMap<bool> object_highlight_map;

static GtkListStore *object_list;
static GtkWidget *object_list_tree_view;
static std::set<CaveObject *> selected_objects;

static CaveSet *caveset;     ///< The caveset edited.

/* objects index */
enum ObjectListColumns {
    INDEX_COLUMN,
    LEVELS_PIXBUF_COLUMN,
    TYPE_PIXBUF_COLUMN,
    ELEMENT_PIXBUF_COLUMN,
    TEXT_COLUMN,
    POINTER_COLUMN,
    NUM_EDITOR_COLUMNS
};

/* cave index */
enum CaveIconViewColumns {
    CAVE_COLUMN,
    NAME_COLUMN,
    PIXBUF_COLUMN,
    NUM_CAVESET_COLUMNS
};


struct GdObjectDescription {
    const char *first_button, *second_button;
};

/* description of coordinates, elements - used by editor properties dialog. */
static GdObjectDescription gd_object_description[]={
    /* for drawing objects */
    /* plot */ { N_("Draw"), NULL },
    /* line */ { N_("Draw"), NULL },
    /* rect */ { N_("Draw"), NULL },
    /* fldr */ { N_("Draw"), N_("Fill") },
    /* rast */ { N_("Draw"), NULL },
    /* join */ { N_("Find"), N_("Draw") },
    /* fldf */ { N_("Fill"), NULL },
    /* bouf */ { N_("Border"), N_("Fill") },
    /* maze */ { N_("Wall"), N_("Path") },
    /* umaz */ { N_("Wall"), N_("Path") },
    /* bmaz */ { N_("Wall"), N_("Path") },
    /* rndf */ { NULL, NULL },
    /* cpps */ { NULL, NULL },
    /* for other tools */
    /* move */ { NULL, NULL },
    /* free */ { N_("Draw"), NULL },
    /* visi */ { NULL, NULL },
};

/* edit tools; check variable "action". this is declared here, as it stores the names of cave drawing objects */
/* this is also used to give objects an icon, so the first part of this array must correspond to the CaveObject::Type enum! */
static GtkRadioActionEntry action_objects[]={
    {"Plot", GD_ICON_EDITOR_POINT, N_("_Point"), "F2", N_("Draw single element"), TOOL_POINT},
    {"Line", GD_ICON_EDITOR_LINE, N_("_Line"), "F4", N_("Draw line of elements"), TOOL_LINE},
    {"Rectangle", GD_ICON_EDITOR_RECTANGLE, N_("_Outline"), "F5", N_("Draw rectangle outline"), TOOL_RECTANGLE},
    {"FilledRectangle", GD_ICON_EDITOR_FILLRECT, N_("R_ectangle"), "F6", N_("Draw filled rectangle"), TOOL_FILLED_RECTANGLE},
    {"Raster", GD_ICON_EDITOR_RASTER, N_("Ra_ster"), NULL, N_("Draw raster"), TOOL_RASTER},
    {"Join", GD_ICON_EDITOR_JOIN, N_("_Join"), NULL, N_("Draw join"), TOOL_JOIN},
    {"FloodFillReplace", GD_ICON_EDITOR_FILL_REPLACE, N_("_Flood fill"), NULL, N_("Fill by replacing elements"), TOOL_FLOODFILL_REPLACE},
    {"FloodFillBorder", GD_ICON_EDITOR_FILL_BORDER, N_("_Boundary fill"), NULL, N_("Fill a closed area"), TOOL_FLOODFILL_BORDER},
    {"Maze", GD_ICON_EDITOR_MAZE, N_("Ma_ze"), NULL, N_("Draw maze"), TOOL_MAZE},
    {"UnicursalMaze", GD_ICON_EDITOR_MAZE_UNI, N_("U_nicursal maze"), NULL, N_("Draw unicursal maze"), TOOL_MAZE_UNICURSAL},
    {"BraidMaze", GD_ICON_EDITOR_MAZE_BRAID, N_("Bra_id maze"), NULL, N_("Draw braid maze"), TOOL_MAZE_BRAID},
    {"RandomFill", GD_ICON_RANDOM_FILL, N_("R_andom fill"), NULL, N_("Draw random elements"), TOOL_RANDOM_FILL},
    {"CopyPaste", GTK_STOCK_COPY, N_("_Copy and Paste"), "" /* deliberately not null or it would use ctrl+c hotkey */, N_("Copy and paste area"), TOOL_COPY_PASTE},

    {"Move", GD_ICON_EDITOR_MOVE, N_("_Move"), "F1", N_("Move object"), TOOL_MOVE},
    {"Freehand", GD_ICON_EDITOR_FREEHAND, N_("F_reehand"), "F3", N_("Draw freely"), TOOL_FREEHAND},
    {"Visible", GTK_STOCK_ZOOM_FIT, N_("Set _visible region"), NULL, N_("Select visible region of cave during play"), TOOL_VISIBLE_REGION},
};


/* forward function declarations */
static void set_status_label_for_cave(CaveStored *cave);
static void select_cave_for_edit(CaveStored *);
static void select_tool(int tool);
static void render_cave();
static void object_properties(CaveObject *object);

static struct NewObjectVisibleOn {
    const char *text;
    int switch_to_level;
} new_objects_visible_on[] = {
    { N_("All levels"), 0 },
    { N_("Level 2 and up"), 1 },
    { N_("Level 3 and up"), 2 },
    { N_("Level 4 and up"), 3 },
    { N_("Level 5 only"), 4 },
};


/*****************************************
 * OBJECT LIST
 * which shows objects in a cave,
 * and also stores current selection.
 */

static gboolean object_list_selection_changed_signal_disabled=FALSE;

static bool object_list_is_selected(CaveObject *o) {
    return selected_objects.count(o)!=0;
}

static bool object_list_is_not_selected(CaveObject *o) {
    return selected_objects.count(o)==0;
}

static CaveObjectStore
object_list_copy_of_selected() {
    g_assert(edited_cave!=NULL);
    CaveObjectStore l;

    for (std::set<CaveObject *>::const_iterator it=selected_objects.begin(); it!=selected_objects.end(); ++it)
        l.push_back_adopt((*it)->clone());    /* save copy in list */

    return l;
}

static int
object_list_count_selected() {
    return selected_objects.size();
}

static bool
object_list_is_any_selected() {
    return !selected_objects.empty();
}

/* this function is to be used only when there is one object in the set. */
static CaveObject*
object_list_first_selected() {
    if (!object_list_is_any_selected())
        return NULL;

    return *selected_objects.begin();
}

static void
object_list_clear_selection() {
    if (object_list_is_any_selected()) {
        selected_objects.clear();
        object_list_selection_changed_signal_disabled = TRUE;
        gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)));
        object_list_selection_changed_signal_disabled = FALSE;
        render_cave();
    }
}


/* check if current iter points to the same object as data. if it is, select the iter */
static gboolean
object_list_select_object_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    CaveObject *object;
    gtk_tree_model_get(model, iter, POINTER_COLUMN, &object, -1);
    if (object==data) {
        gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW (object_list_tree_view)), iter);
        return TRUE;
    }
    return FALSE;
}

static void
object_list_add_to_selection(CaveObject *object) {
    gtk_tree_model_foreach(GTK_TREE_MODEL(object_list), (GtkTreeModelForeachFunc) object_list_select_object_func, object);
}

/* check if current iter points to the same object as data. if it is, select the iter */
static gboolean
object_list_unselect_object_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    CaveObject *object;

    gtk_tree_model_get(model, iter, POINTER_COLUMN, &object, -1);
    if (object==data) {
        gtk_tree_selection_unselect_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW (object_list_tree_view)), iter);
        return TRUE;
    }
    return FALSE;
}

static void
object_list_remove_from_selection(CaveObject *object) {
    gtk_tree_model_foreach(GTK_TREE_MODEL (object_list), (GtkTreeModelForeachFunc) object_list_unselect_object_func, object);
}


static void
object_list_select_one_object(CaveObject *object) {
    object_list_clear_selection();
    object_list_add_to_selection(object);
}

static void
object_list_selection_changed_signal(GtkTreeSelection *selection, gpointer data) {
    if (object_list_selection_changed_signal_disabled)
        return;

    /* check all selected objects, and set all selected objects to highlighted */
    GtkTreeModel *model;
    GList *selected_rows=gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)), &model);
    selected_objects.clear();
    for (GList *siter=selected_rows; siter!=NULL; siter=siter->next) {
        GtkTreePath *path=(GtkTreePath *) siter->data;
        GtkTreeIter iter;
        gtk_tree_model_get_iter(model, &iter, path);
        CaveObject *object;
        gtk_tree_model_get(model, &iter, POINTER_COLUMN, &object, -1);
        selected_objects.insert(object);
    }
    g_list_foreach(selected_rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(selected_rows);

    /* object highlight all to false */
    object_highlight_map.fill(false);

    /* check all selected objects, and set all selected objects to highlighted */
    for (std::set<CaveObject *>::const_iterator it=selected_objects.begin(); it!=selected_objects.end(); ++it) {
        for (int y=0; y<rendered_cave->h; y++)
            for (int x=0; x<rendered_cave->w; x++)
                if (rendered_cave->objects_order(x, y)==(*it))  /* if element at x,y was created by this object... */
                    object_highlight_map(x, y)=true;
    }

    /* how many selected objects? */
    int count=selected_objects.size();

    /* enable actions */
    gtk_action_group_set_sensitive(actions_edit_one_object, count==1);
    gtk_action_group_set_sensitive(actions_edit_object, count!=0);
    gtk_action_group_set_sensitive(actions_clipboard, count!=0);

    if (count==0) {
        /* NO object selected -> show general cave info */
        set_status_label_for_cave(edited_cave);
    }
    else if (count==1) {
        /* exactly ONE object is selected */
        CaveObject *object=object_list_first_selected();

        std::string text=object->get_description_markup();
        gtk_label_set_markup(GTK_LABEL(label_object), text.c_str());
    }
    else if (count>1)
        /* more than one object is selected */
        gtk_label_set_markup(GTK_LABEL(label_object), CPrintf(_("%d objects selected")) % count);
}


/* for popup menu, by properties key */
static void
object_list_show_popup_menu(GtkWidget *widget, gpointer data) {
    gtk_menu_popup(GTK_MENU(object_list_popup), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

/* for popup menu, by right-click */
static gboolean
object_list_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->type==GDK_BUTTON_PRESS && event->button==3) {
        gtk_menu_popup(GTK_MENU(object_list_popup), NULL, NULL, NULL, NULL, event->button, event->time);
        return TRUE;
    }
    return FALSE;
}

/*
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
 * we rerender the cave.
 */
static gboolean object_list_row_changed_bool=FALSE;

static void
object_list_row_changed(GtkTreeModel *model, GtkTreePath *changed_path, GtkTreeIter *changed_iter, gpointer user_data) {
    /* set the flag so we know that the next delete signal is emitted by a drag-and-drop */
    object_list_row_changed_bool=TRUE;
}

static gboolean
object_list_new_order_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    CaveObjectStore *list=static_cast<CaveObjectStore *>(data);
    CaveObject *object;

    gtk_tree_model_get(model, iter, POINTER_COLUMN, &object, -1);
    list->push_back_adopt(object->clone());

    return FALSE;   /* continue traversing */
}

/* XXX this does not reorder, rather creates a brand new list... */
static void
object_list_row_delete(GtkTreeModel *model, GtkTreePath *changed_path, gpointer user_data) {
    if (object_list_row_changed_bool) {
        CaveObjectStore new_order;

        /* reset the flag, as we handle the reordering here - so its signal handler will not interfere */
        object_list_row_changed_bool=FALSE;

        /* this will build the new object order to the list */
        gtk_tree_model_foreach(model, object_list_new_order_func, &new_order);

        edited_cave->objects=new_order;

        object_list_clear_selection();      /* XXX */

        render_cave();
    }
}

/* row activated - set properties of object */
static void
object_list_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    GtkTreeModel *model=gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    CaveObject *object;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, POINTER_COLUMN, &object, -1);
    object_properties(object);
    /* render cave after changing object properties */
    render_cave();
}


static void
help_cb(GtkWidget *widget, gpointer data) {
    gd_show_editor_help (gd_editor_window);
}


/****************************************************
 *
 * FUNCTIONS FOR UNDO
 *
 */

static void
redo_free_all() {
    redo_caves.clear();
}

/* delete the cave copies saved for undo. */
static void
undo_free_all() {
    redo_free_all();
    undo_caves.clear();
}

/* save a copy of the current state of edited cave. this is to be used
   internally; as it does not delete redo. */
#define UNDO_STEPS 10
static void
undo_save_current_state() {
    /* if more than four, forget some (should only forget one) */
    while (undo_caves.size() >= UNDO_STEPS)
        undo_caves.pop_front();

    undo_caves.push_back(*edited_cave);
}

/* save a copy of the current state of edited cave, after some operation.
   this destroys the redo list, as from that point that is useless. */
static void
undo_save() {
    g_return_if_fail(edited_cave!=NULL);

    /* we also use this function to set the edited flag, as it is called for any action */
    caveset->edited=TRUE;

    /* remove from pixbuf hash: delete its pixbuf */
    /* as now we know that this cave is really edited. */
    g_hash_table_remove(cave_pixbufs, edited_cave);

    undo_save_current_state();
    redo_free_all();

    /* now we have a cave to do an undo from, so sensitize the menu item */
    gtk_action_group_set_sensitive (actions_edit_undo, TRUE);
    gtk_action_group_set_sensitive (actions_edit_redo, FALSE);
}

static void undo_do_one_step() {
    /* push current to redo list. we do not check the redo list size, as the undo size limits it automatically... */
    redo_caves.push_front(*edited_cave);

    /* copy to edited one, and free backup */
    *edited_cave=undo_caves.back();
    undo_caves.pop_back();

    /* call to renew editor window */
    select_cave_for_edit(edited_cave);
}

/* this function is for "internal" use.
 * some cave editing functions are much easier
 * if they can use the undo facility when
 * the user presses a cancel button.
 * in that case, that undo step should not be
 * visible in the redo list.
 */
static void undo_do_one_step_but_no_redo() {
    /* copy to edited one, and free backup */
    *edited_cave=undo_caves.back();
    undo_caves.pop_back();

    /* call to renew editor window */
    select_cave_for_edit(edited_cave);
}

/* do the undo - callback */
static void
undo_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail(!undo_caves.empty());
    g_return_if_fail(edited_cave!=NULL);

    undo_do_one_step();
}

/* do the redo - callback */
static void
redo_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail(!redo_caves.empty());
    g_return_if_fail(edited_cave!=NULL);

    /* push back current to undo */
    undo_save_current_state();

    /* and select the first from redo */
    *edited_cave=redo_caves.front();
    redo_caves.pop_front();

    /* call to renew editor window */
    select_cave_for_edit(edited_cave);
}


static void editor_window_set_title() {
    if (edited_cave) {
        /* editing a specific cave */
        gtk_window_set_title(GTK_WINDOW(gd_editor_window), CPrintf(_("GDash Cave Editor - %s")) % edited_cave->name);
    } else {
        /* editing the caveset */
        gtk_window_set_title(GTK_WINDOW(gd_editor_window), CPrintf(_("GDash Cave Editor - %s")) % caveset->name);
    }
}


/*
 * property editor
 *
 * edit properties of a reflective object.
 */
static void edit_properties_create_window(const char *title, bool show_cancel, GtkWidget *& dialog, GtkWidget *&notebook) {
    dialog=gtk_dialog_new_with_buttons(title, GTK_WINDOW(gd_editor_window), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    if (show_cancel)
        gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 360, 240);

    /* tabbed notebook */
    notebook=gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), notebook, TRUE, TRUE, 6);
}

static void edit_properties_set_random_max(GtkWidget *widget, gpointer data) {
    GtkSpinButton *spin=GTK_SPIN_BUTTON(widget);
    EditorAutoUpdate *next_eau=static_cast<EditorAutoUpdate *>(data);
    // the maximum of the next spin button is the current value of this
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(next_eau->widget), 0, gtk_spin_button_get_value_as_int(spin));
}

static void edit_properties_add_widgets(GtkWidget *notebook, std::list<EditorAutoUpdate *> &eau_s, PropertyDescription const prop_desc[], Reflective *str, Reflective *def_str, void (*cave_render_cb)()) {
    GtkWidget *table=NULL;
    int row=0;
    EditorAutoUpdate *previous_random_probability=0;

    /* create the entry widgets */
    for (unsigned i=0; prop_desc[i].identifier!=NULL; i++) {
        if (prop_desc[i].flags&GD_DONT_SHOW_IN_EDITOR)
            continue;

        if (prop_desc[i].type==GD_TAB) {
            /* create a table which will be the widget inside */
            table=gtk_table_new(1, 1, FALSE);
            gtk_container_set_border_width(GTK_CONTAINER(table), 12);
            gtk_table_set_col_spacings(GTK_TABLE(table), 12);
            gtk_table_set_row_spacings(GTK_TABLE(table), 3);
            /* and to the notebook */
            gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, gtk_label_new(_(prop_desc[i].name)));
            row=0;
            continue;
        }
        g_assert (table!=NULL);

        // name of the setting
        if (prop_desc[i].name!=0) {
            GtkWidget *label;
            if (prop_desc[i].type==GD_LABEL)    // if this is a label, make it bold
                label=gd_label_new_leftaligned(CPrintf("<b>%s</b>")%_(prop_desc[i].name));
            else
                label=gd_label_new_leftaligned(_(prop_desc[i].name));
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row+1,
                GtkAttachOptions(GTK_FILL|GTK_SHRINK),
                GtkAttachOptions(GTK_FILL|GTK_SHRINK), 0, 0);
        }
        EditorAutoUpdate *eau=new EditorAutoUpdate(str, def_str, &prop_desc[i], cave_render_cb);
        gtk_table_attach(GTK_TABLE(table), eau->widget, 1, 2, row, row+1,
            GtkAttachOptions(GTK_FILL|GTK_EXPAND),
            GtkAttachOptions(GTK_FILL|(eau->expand_vertically?GTK_EXPAND:GTK_SHRINK)), 0, 0);
        eau_s.push_back(eau);
        
        // if this is setting is for a c64 random generator fill, add an extra changed
        if (prop_desc[i].flags&GD_BD_PROBABILITY) {
            if (previous_random_probability) {
                g_signal_connect(previous_random_probability->widget, "changed", G_CALLBACK(edit_properties_set_random_max), eau);
                // and also update right now
                gtk_spin_button_set_range(GTK_SPIN_BUTTON(eau->widget), 0, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(previous_random_probability->widget)));
            }
            // and remember this for the next iteration
            previous_random_probability=eau;
        }

        row++;
    }
}

/* returns true if edited. */
/* str is the struct to be edited. */
/* def_str is another struct of the same type, which holds default values. */
/* show_cancel: true, then a cancel button is also shown. */

/* ALWAYS EDITS the object given! It is up to the caller to restore the
 * state of the object if the response is false. */
static bool edit_properties(const char *title, Reflective *str, Reflective *def_str, bool show_cancel, void (*cave_render_cb)()) {
    std::list<EditorAutoUpdate *> eau_s;

    GtkWidget *dialog, *notebook;
    edit_properties_create_window(title, show_cancel, dialog, notebook);

    edit_properties_add_widgets(notebook, eau_s, str->get_description_array(), str, def_str, cave_render_cb);

    /* running the dialog */
    gtk_widget_show_all(dialog);
    int result=gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    for (std::list<EditorAutoUpdate *>::const_iterator it=eau_s.begin(); it!=eau_s.end(); ++it)
        delete *it;

    return result==GTK_RESPONSE_ACCEPT;
}


/* call edit_properties for a cave, then do some cleanup.
 * for example, if the size changed, the map has to be resized,
 * etc. also the user may be warned about resizing the visible area. */
static void cave_properties(CaveStored *cave, gboolean show_cancel) {
    // check if full cave visible
    bool full_visible=(cave->x1==0 && cave->y1==0 && cave->x2==cave->w-1 && cave->y2==cave->h-1);

    // Edit a copy only.
    // Then later decide what to do.
    CaveStored copy(*cave);
    CaveStored def_cave;
    bool edited=edit_properties(_("Cave Properties"), &copy, &def_cave, show_cancel, NULL);
    
    // Yes, the user pressed ok - copy the edited version to the original.
    if (edited) {
        bool size_changed=(cave->w!=copy.w || cave->h!=copy.h);       // but first remember this

        undo_save();
        *cave=copy;
        caveset->edited=true;

        // if the size changes, we have work to do.
        if (size_changed) {
            // if cave has a map, resize it also
            if (!cave->map.empty())
                cave->map.resize(cave->w, cave->h, cave->initial_border);

            // if originally the full cave was visible, we set the visible area to the full cave, here again.
            if (full_visible) {
                cave->x1=0;
                cave->y1=0;
                cave->x2=cave->w-1;
                cave->y2=cave->h-1;
            }
            // do some validation
            gd_cave_correct_visible_size(*cave);

            // check ratios, so they are not larger than number of cells in cave (width*height)
            PropertyDescription const *descriptor=cave->get_description_array();
            for (unsigned i=0; descriptor[i].identifier!=NULL; i++)
                if (descriptor[i].flags&GD_BDCFF_RATIO_TO_CAVE_SIZE) {
                    std::auto_ptr<GetterBase> const &prop=descriptor[i].prop;
                    switch (descriptor[i].type) {
                        case GD_TYPE_INT:
                            if (cave->get<GdInt>(prop)>=cave->w*cave->h)
                                cave->get<GdInt>(prop)=cave->w*cave->h;
                            break;
                        case GD_TYPE_INT_LEVELS:
                            for (unsigned j=0; j<prop->count; ++j)
                                if (cave->get<GdIntLevels>(prop)[j]>=cave->w*cave->h)
                                    cave->get<GdIntLevels>(prop)[j]=cave->w*cave->h;
                            break;
                        default:
                            break;
                    }
                }
        }
        /* redraw, recreate, re-everything */
        select_cave_for_edit(cave);

        // if the size of the cave changed, inform the user that he should check the visible region.
        // also, automatically select the tool.
        if (size_changed && !full_visible) {
            select_tool(TOOL_VISIBLE_REGION);
            gd_warningmessage(_("You have changed the size of the cave. Please check the size of the visible region!"),
                _("The visible area of the cave during the game can be smaller than the whole cave. If you resize "
                  "the cave, the area to be shown cannot be guessed automatically. The tool to set this behavior is "
                  "now selected, and shows the current visible area. Use drag and drop to change the position and "
                  "size of the rectangle."));

        }
    }
}


/* edit the properties of the caveset - nothing special. */
static void caveset_properties(bool show_cancel) {
    CaveSet def_val;  /* to create an instance with default values */

    CaveSet original=*caveset;
    bool result=edit_properties(_("Cave Set Properties"), caveset, &def_val, true, NULL);
    if (!result)
        *caveset=original;
    else {
        editor_window_set_title();
        caveset->edited=true;
    }
}


/// Select cave edit tool.
/// This activates the gtk action in question, so
/// toolbar and everything else will be updated
static void select_tool(int tool) {
    /* find specific object type in action_objects array. */
    /* (the array indexes and tool integers may not correspond) */
    for (unsigned i=0; i<G_N_ELEMENTS(action_objects); i++)
        if (tool==action_objects[i].value)
            gtk_action_activate(gtk_action_group_get_action(actions_edit_tools, action_objects[i].name));
}


static void
set_status_label_for_cave(CaveStored *cave) {
    char *name_escaped;

    name_escaped=g_markup_escape_text(cave->name.c_str(), -1);
    gtk_label_set_markup(GTK_LABEL (label_object),
        CPrintf(_("<b>%s</b>, %s, %dx%d, time %d:%02d, diamonds %d"))
        % name_escaped
        % (cave->selectable?_("selectable"):_("not selectable"))
        % cave->w
        % cave->h
        % (cave->level_time[edit_level]/60)
        % (cave->level_time[edit_level]%60)
        % cave->level_diamonds[edit_level]);
    g_free(name_escaped);
}


static gboolean
set_status_label_for_caveset_count_caves(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    int *i=(int *)data;

    (*i)++;
    return FALSE;   /* to continue counting */
}

static void
set_status_label_for_caveset() {
    char *name_escaped;
    int count;

    if (iconview_cavelist) {
        count=0;
        gtk_tree_model_foreach(gtk_icon_view_get_model(GTK_ICON_VIEW(iconview_cavelist)), (GtkTreeModelForeachFunc) set_status_label_for_caveset_count_caves, &count);
    }
    else
        count=caveset->caves.size();

    name_escaped=g_markup_escape_text(caveset->name.c_str(), -1);
    gtk_label_set_markup(GTK_LABEL (label_object), CPrintf(_("<b>%s</b>, %d caves")) % name_escaped % count);
    g_free(name_escaped);
}


/*
    render cave - ie. draw it as a map,
    so it can be presented to the user.
*/
static void render_cave() {
    g_return_if_fail(edited_cave!=NULL);

    /* rendering cave for editor: seed=random, so the user sees which elements are truly random */
    if (!rendered_cave) {
        // if does not exist at all, create now
        delete rendered_cave;
        rendered_cave=new CaveRendered(*edited_cave, edit_level, g_random_int_range(0, GD_CAVE_SEED_MAX));
    }
    else
        // otherwise only recreate the map
        rendered_cave->create_map(*edited_cave, edit_level);

    /* set size of map; also clear it, as colors might have changed etc. */
    gfx_buffer.set_size(rendered_cave->w, rendered_cave->h, -1);
    object_highlight_map.set_size(rendered_cave->w, rendered_cave->h, false);

    /* we disable this, so things do not get updated object by object. */
    /* also we MUST disable this, as the selection change signal would ruin our selected_objects set */
    object_list_selection_changed_signal_disabled=TRUE;

    /* fill object list store with the objects. */
    gtk_list_store_clear(object_list);

    int i=0;
    for(CaveObjectStore::const_iterator it=edited_cave->objects.begin(); it!=edited_cave->objects.end(); ++it) {
        CaveObject *object=*it;
        GdkPixbuf *element=NULL;
        
        GdElementEnum characteristic=object->get_characteristic_element();
        if (characteristic!=O_NONE)
            element=editor_cell_renderer->combo_pixbuf(characteristic);

        const char *levels_stock;
        if (object->is_seen_on_all())            /* if on all levels */
            levels_stock=GD_ICON_OBJECT_ON_ALL;
        else                                            /* not on all levels... visible on current level? */
            if (object->seen_on[edit_level])
                levels_stock=GD_ICON_OBJECT_NOT_ON_ALL;
            else
                levels_stock=GD_ICON_OBJECT_NOT_ON_CURRENT;

        std::string text=object->get_coordinates_text();
        /* use atomic insert with values */
        GtkTreeIter treeiter;
        gtk_list_store_insert_with_values(object_list, &treeiter, i, INDEX_COLUMN, i, LEVELS_PIXBUF_COLUMN, levels_stock,
            TYPE_PIXBUF_COLUMN, action_objects[object->type].stock_id,
            ELEMENT_PIXBUF_COLUMN, element, TEXT_COLUMN, text.c_str(), POINTER_COLUMN, object, -1);

        /* also do selection as now we have the iter in hand */
        if (selected_objects.count(object)!=0)
            gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)), &treeiter);

        i++;
    }
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(object_list_tree_view));

    /* and now that it is filled, we call the selection changed signal by hand. */
    object_list_selection_changed_signal_disabled=FALSE;
    object_list_selection_changed_signal(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)), NULL);

    gtk_action_group_set_sensitive (actions_edit_map, !edited_cave->map.empty());   /* map actions when we have map */
    gtk_action_group_set_sensitive (actions_edit_random, edited_cave->map.empty()); /* random fill actions when no map */

    /* if no object is selected, show normal cave info */
    if (selected_objects.empty())
        set_status_label_for_cave(edited_cave);
}


/// Edit properties of an object.
static void object_properties(CaveObject *object) {
    if (object==NULL) {
        g_return_if_fail(object_list_count_selected()==1);
        object=object_list_first_selected();    /* select first from list... should be only one selected */
    }

    std::list<EditorAutoUpdate *> eau_s;
    undo_save();

    CaveObject *before_edit=object->clone();
    GtkWidget *dialog, *notebook;
    edit_properties_create_window(_("Object Properties"), true, dialog, notebook);
    edit_properties_add_widgets(notebook, eau_s, object->get_description_array(), object, before_edit, render_cave);
    gtk_widget_show_all(dialog);
    bool changed=gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT;
    gtk_widget_destroy(dialog);
    delete before_edit;

    if (changed) {
        if (object->is_invisible()) {
            object->enable_on_all();

            gd_warningmessage(_("The object should be visible on at least one level."), _("Enabled this object on all levels."));
        }
        render_cave();
    } else
        undo_do_one_step_but_no_redo();

    for (std::list<EditorAutoUpdate *>::const_iterator it=eau_s.begin(); it!=eau_s.end(); ++it)
        delete (*it);
}


/* this is the same scroll routine as the one used for the game. only the parameters are changed a bit. */
static void drawcave_timeout_scroll(int player_x, int player_y) {
    static int scroll_desired_x=0, scroll_desired_y=0;
    GtkAdjustment *adjustment;
    int scroll_center_x, scroll_center_y;
    int i;
    /* hystheresis size is this, multiplied by two. */
    int cs=editor_cell_renderer->get_cell_size();
    int scroll_start_x=scroll_window->allocation.width/2-2*cs;
    int scroll_start_y=scroll_window->allocation.height/2-2*cs;
    int scroll_speed=cs/4;

    /* get the size of the window so we know where to place player.
     * first guess is the middle of the screen.
     * drawing_area->parent->parent is the viewport.
     * +cellsize/2 gets the stomach of player :) so the very center */
    scroll_center_x=player_x * cs + cs/2 - drawing_area->parent->parent->allocation.width/2;
    scroll_center_y=player_y * cs + cs/2 - drawing_area->parent->parent->allocation.height/2;

    /* HORIZONTAL */
    /* hystheresis function.
     * when scrolling left, always go a bit less left than player being at the middle.
     * when scrolling right, always go a bit less to the right. */
    adjustment=gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll_window));
    if (adjustment->value+scroll_start_x<scroll_center_x)
        scroll_desired_x=scroll_center_x-scroll_start_x;
    if (adjustment->value-scroll_start_x>scroll_center_x)
        scroll_desired_x=scroll_center_x+scroll_start_x;

    scroll_desired_x=CLAMP(scroll_desired_x, 0, adjustment->upper - adjustment->step_increment - adjustment->page_increment);
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

    scroll_desired_y=CLAMP(scroll_desired_y, 0, adjustment->upper - adjustment->step_increment - adjustment->page_increment);
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
static gboolean drawing_area_draw_timeout(gpointer data) {
    static int animcycle=0;
    static bool player_blinking=0;
    static bool hand_cursor = false;
    
    bool editor_window_is_sensitive = gtk_widget_get_sensitive(gd_editor_window);

    /* if nothing to draw or nowhere to draw :) exit.
     * this is necessary as the interrupt is not uninstalled when the selector is running. */
    if (!drawing_area || !rendered_cave)
        return TRUE;

    g_return_val_if_fail(!gfx_buffer.empty(), TRUE);

    /* when mouse over a drawing object, cursor changes */
    if (mouse_x >= 0 && mouse_y >= 0) {
        bool new_hand_cursor = rendered_cave->objects_order(mouse_x, mouse_y)!=NULL;

        if (hand_cursor!=new_hand_cursor) {
            hand_cursor = new_hand_cursor;
            gdk_window_set_cursor(drawing_area->window, hand_cursor?gdk_cursor_new(GDK_HAND1):NULL);
        }
    }

    /* only do cell animations when window is active.
     * otherwise... user is testing the cave, animation would just waste cpu. */
    if (gtk_window_has_toplevel_focus(GTK_WINDOW(gd_editor_window)))
        animcycle=(animcycle+1) & 7;

    if (animcycle==0)           /* player blinking is started at the beginning of animation sequences. */
        player_blinking=g_random_int_range(0, 4)==0;   /* 1/4 chance of blinking, every sequence. */

    int cs = editor_cell_renderer->get_cell_size();
    bool drawn = false;
    for (int y=0; y<rendered_cave->h; y++) {
        for (int x=0; x<rendered_cave->w; x++) {
            int draw;

            if (gd_game_view)
                draw=gd_element_properties[rendered_cave->map(x, y)].image_simple;
            else
                draw=gd_element_properties[rendered_cave->map(x, y)].image;
            /* special case is player - sometimes blinking :) */
            if (player_blinking && (rendered_cave->map(x, y)==O_INBOX))
                draw=gd_element_properties[O_PLAYER_BLINK].image_simple;
            /* the biter switch also shows its state */
            if (rendered_cave->map(x, y)==O_BITER_SWITCH)
                draw=gd_element_properties[O_BITER_SWITCH].image_simple+rendered_cave->biter_delay_frame;

            /* negative value means animation */
            if (draw<0)
                draw=-draw+animcycle;

            /* object coloring */
            if (editor_window_is_sensitive) {
                /* if the editor is active */
                if (action==TOOL_VISIBLE_REGION) {
                    /* if showing visible region, different color applies for: */
                    if (x>=rendered_cave->x1 && x<=rendered_cave->x2 && y>=rendered_cave->y1 && y<=rendered_cave->y2)
                        draw+=NUM_OF_CELLS;
                    if (x==rendered_cave->x1 || x==rendered_cave->x2 || y==rendered_cave->y1 || y==rendered_cave->y2)
                        draw+=NUM_OF_CELLS; /* once again */
                } else {
                    if (object_highlight_map(x,y)) /* if it is a selected object, make it colored */
                        draw+=2*NUM_OF_CELLS;
                    else if (gd_colored_objects && rendered_cave->objects_order(x, y)!=0)
                        /* if it belongs to any other element, make it colored a bit */
                        draw+=NUM_OF_CELLS;
                }
            } else {
                /* if the editor is inactive */
                draw += NUM_OF_CELLS;
            }

            /* the drawing itself */
            if (gfx_buffer(x,y)!=draw) {
                screen->blit(editor_cell_renderer->cell(draw), x*cs, y*cs);
                gfx_buffer(x, y)=draw;
                drawn = true;
            }
        }
    }
    if (drawn)
        screen->flip();

    /* draw a mark for fill objects */
    for (int y=0; y<rendered_cave->h; y++) {
        for (int x=0; x<rendered_cave->w; x++) {
            /* for fill objects, we show their origin */
            if (object_highlight_map(x, y)
                && (rendered_cave->objects_order(x, y)->type==CaveObject::GD_FLOODFILL_BORDER
                    || rendered_cave->objects_order(x, y)->type==CaveObject::GD_FLOODFILL_REPLACE)) {
                Coordinate c = dynamic_cast<CaveFill&>(*rendered_cave->objects_order(x, y)).get_start_coordinate();
                /* if this one is the starting coordinate, mark it */
                if (c.x == x && c.y == y) {
                    gdk_draw_rectangle(drawing_area->window, drawing_area->style->white_gc, FALSE, c.x*cs, c.y*cs, cs-1, cs-1);
                    gdk_draw_line(drawing_area->window, drawing_area->style->white_gc, c.x*cs, c.y*cs, (c.x+1)*cs-1, (c.y+1)*cs-1);
                    gdk_draw_line(drawing_area->window, drawing_area->style->white_gc, c.x*cs, (c.y+1)*cs-1, (c.x+1)*cs-1, c.y*cs);
                    gfx_buffer(c.x, c.y)=-1;  /* so it will always be redrawn */
                }
            }
        }
    }

    /* draw a mark for the mouse pointer */
    if (mouse_x>=0 && mouse_y>=0) {
        /* this is the cell the mouse is over */
        gdk_draw_rectangle(drawing_area->window, drawing_area->style->white_gc, FALSE, mouse_x*cs, mouse_y*cs, cs-1, cs-1);
        /* always redraw this cell the next frame - the easiest way to always get rid of the rectangle when the mouse is moved */
        gfx_buffer(mouse_x, mouse_y)=-1;
    }

    /* automatic scrolling */
    if (mouse_x>=0 && mouse_y>=0 && button1_clicked)
        drawcave_timeout_scroll(mouse_x, mouse_y);

    return TRUE;
}


/*
 * cave drawing area expose event.
 */
static gboolean
drawing_area_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    if (!widget->window || gfx_buffer.empty() || rendered_cave==NULL)
        return FALSE;
    int cs=editor_cell_renderer->get_cell_size();

    /* calculate which area to update */
    int x1 = event->area.x/cs;
    int y1 = event->area.y/cs;
    int x2 = (event->area.x+event->area.width-1)/cs;
    int y2 = (event->area.y+event->area.height-1)/cs;
    /* when resizing the cave, we may get events which store the old size, if the drawing area is not yet resized. */
    if (x1<0) x1=0;
    if (y1<0) x1=0;
    if (x2>=rendered_cave->w) x2=rendered_cave->w-1;
    if (y2>=rendered_cave->h) y2=rendered_cave->h-1;

    /* tag them as "to be redrawn" */
    for (int y=y1; y<=y2; y++)
        for (int x=x1; x<=x2; x++)
            gfx_buffer(x, y) = -1;
    return TRUE;
}


static void drawing_area_destroyed(GtkWidget *widget, gpointer data) {
    delete screen;
    screen = NULL;
    drawing_area = NULL;
}


/***************************************************
 *
 * mouse events
 *
 *
 */

static void
edited_cave_add_object(CaveObject *object) {
    undo_save();

    /* only visible on level... */
    int act=gtk_combo_box_get_active(GTK_COMBO_BOX(new_object_level_combo));
    int lev=new_objects_visible_on[act].switch_to_level;
    for (int i=0; i<lev; ++i)
        object->seen_on[i]=false;

    edited_cave->push_back_adopt(object);
    render_cave();      /* new object created, so re-render cave */
    object_list_select_one_object(edited_cave->objects.back()); /* also make it selected; so it can be edited further */
}


/* mouse button press event */
static gboolean
drawing_area_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    g_return_val_if_fail (edited_cave!=NULL, FALSE);

    /* right click opens popup */
    if (event->button==3) {
        gtk_menu_popup(GTK_MENU(drawing_area_popup), NULL, NULL, NULL, NULL, event->button, event->time);
        return TRUE;
    }

    /* this should be also false for doubleclick! so we do not do if (event->tye....) */
    button1_clicked = event->type==GDK_BUTTON_PRESS && event->button==1;

    int cs = editor_cell_renderer->get_cell_size();
    clicked_x=((int) event->x)/cs;
    clicked_y=((int) event->y)/cs;
    /* middle button picks element from screen */
    if (event->button==2) {
        if (event->state & GDK_CONTROL_MASK)
            gd_element_button_set(fillelement_button, GdElementEnum(rendered_cave->map( clicked_x, clicked_y)));
        else
            gd_element_button_set(element_button, GdElementEnum(rendered_cave->map( clicked_x, clicked_y)));
        return TRUE;
    }

    /* we do not handle anything other than buttons3,2 above, and button 1 */
    if (event->button!=1)
        return FALSE;

    /* if double click, open element properties window.
     * if no element selected, open cave properties window.
     * (if mouse if over an element, the first click selected it.) */
    /* do not allow this doubleclick for visible region mode as that one does not select objects */
    if (event->type==GDK_2BUTTON_PRESS && action!=TOOL_VISIBLE_REGION) {
        if (rendered_cave->objects_order(clicked_x, clicked_y))
            object_properties(rendered_cave->objects_order(clicked_x, clicked_y));
        else
            cave_properties(edited_cave, TRUE);
        return TRUE;
    }

    switch (action) {
        case TOOL_MOVE:
            /* action=move: now the user is selecting an object by the mouse click */
            if ((event->state & GDK_CONTROL_MASK) || (event->state & GDK_SHIFT_MASK)) {
                CaveObject *clicked_on_object=rendered_cave->objects_order(clicked_x, clicked_y);
                /* CONTROL or SHIFT PRESSED: multiple selection */
                /* check if clicked on an object. */
                if (clicked_on_object!=0) {
                    if (object_list_is_selected(clicked_on_object))
                        object_list_remove_from_selection(clicked_on_object);
                    else
                        object_list_add_to_selection(clicked_on_object);
                }
            } else {
                CaveObject *clicked_on_object=rendered_cave->objects_order(clicked_x, clicked_y);
                /* CONTROL NOT PRESSED: single selection */
                /* if the object clicked is not currently selected, we select it. if it is, do nothing, so a multiple selection remains. */
                if (clicked_on_object) {
                    /* check if currently selected. if yes, do nothing, as it would make multi-object drag&drops impossible. */
                    if (!object_list_is_selected(clicked_on_object))
                        object_list_select_one_object(clicked_on_object);
                } else
                    /* if clicking on a non-object, deselect all */
                    object_list_clear_selection();
            }

            /* prepare for undo */
            undo_move_flag=FALSE;
            break;

        case TOOL_FREEHAND:
            /* freehand tool: draw points in each place. */
            /* if already the same element there, which is placed by an object, skip! */
            if (rendered_cave->map(clicked_x, clicked_y)!=gd_element_button_get(element_button) || rendered_cave->objects_order(clicked_x, clicked_y)==NULL)
                /* it places a point. and later, when dragging the mouse, it may place another points. */
                edited_cave_add_object(new CavePoint(Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button)));
            break;

        case TOOL_VISIBLE_REGION:
            /* new click... prepare for undo! */
            undo_move_flag=FALSE;
            /* do nothing, the motion event will matter */
            break;

        case TOOL_POINT:
            edited_cave_add_object(new CavePoint(Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button)));
            break;

        case TOOL_LINE:
            edited_cave_add_object(new CaveLine(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button)));
            break;

        case TOOL_RECTANGLE:
            edited_cave_add_object(new CaveRectangle(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button)));
            break;

        case TOOL_FILLED_RECTANGLE:
            edited_cave_add_object(new CaveFillRect(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button), gd_element_button_get(fillelement_button)));
            break;

        case TOOL_RASTER:
            edited_cave_add_object(new CaveRaster(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y), Coordinate(2, 2), gd_element_button_get(element_button)));
            break;

        case TOOL_JOIN:
            edited_cave_add_object(new CaveJoin(Coordinate(0, 0), gd_element_button_get(element_button), gd_element_button_get(fillelement_button)));
            break;

        case TOOL_FLOODFILL_REPLACE:
            edited_cave_add_object(new CaveFloodFill(Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button), rendered_cave->map(clicked_x, clicked_y)));
            break;

        case TOOL_FLOODFILL_BORDER:
            edited_cave_add_object(new CaveBoundaryFill(Coordinate(clicked_x, clicked_y), gd_element_button_get(fillelement_button), gd_element_button_get(element_button)));
            break;

        case TOOL_MAZE:
            edited_cave_add_object(new CaveMaze(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button), gd_element_button_get(fillelement_button), CaveMaze::Perfect));
            break;

        case TOOL_MAZE_UNICURSAL:
            edited_cave_add_object(new CaveMaze(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button), gd_element_button_get(fillelement_button), CaveMaze::Unicursal));
            break;

        case TOOL_MAZE_BRAID:
            edited_cave_add_object(new CaveMaze(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y), gd_element_button_get(element_button), gd_element_button_get(fillelement_button), CaveMaze::Braid));
            break;

        case TOOL_RANDOM_FILL:
            edited_cave_add_object(new CaveRandomFill(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y)));
            break;

        case TOOL_COPY_PASTE:
            edited_cave_add_object(new CaveCopyPaste(Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y), Coordinate(clicked_x, clicked_y)));
            break;
        default:
            g_assert_not_reached();
    }
    return TRUE;
}

static gboolean
drawing_area_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->type==GDK_BUTTON_RELEASE && event->button==1)
        button1_clicked=FALSE;
    return TRUE;
}

/* mouse leaves drawing area event */
static gboolean
drawing_area_leave_event(GtkWidget *widget, GdkEventCrossing * event, gpointer data) {
    /* do not check if it as enter event, as we did not connect that one. */
    gtk_label_set_text (GTK_LABEL (label_coordinate), "[x:   y:   ]");
    mouse_x=-1;
    mouse_y=-1;
    return FALSE;
}

/* mouse motion event */
static gboolean
drawing_area_motion_event(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    int x, y, dx, dy;
    GdkModifierType state;

    /* just to be sure. */
    if (!edited_cave)
        return FALSE;

    if (event->is_hint)
        gdk_window_get_pointer (event->window, &x, &y, &state);
    else {
        x=event->x;
        y=event->y;
        state=GdkModifierType(event->state);
    }

    int cs=editor_cell_renderer->get_cell_size();
    x/=cs;
    y/=cs;

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
        gtk_label_set_markup(GTK_LABEL (label_coordinate), CPrintf("[x:%d y:%d]") % x % y);
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
    if (action==TOOL_VISIBLE_REGION) {
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
        gd_cave_correct_visible_size(*edited_cave);

        /* instead of re-rendering the cave, we just copy the changed values. */
        rendered_cave->x1=edited_cave->x1;
        rendered_cave->x2=edited_cave->x2;
        rendered_cave->y1=edited_cave->y1;
        rendered_cave->y2=edited_cave->y2;
        return TRUE;
    }

    if (action==TOOL_FREEHAND) {
        /* the freehand tool is different a bit. it draws single points automatically */
        /* but only to places where there is no such object already. */
        if (rendered_cave->map(x, y)!=gd_element_button_get (element_button) || rendered_cave->objects_order(x, y)==NULL) {
            edited_cave->push_back_adopt(new CavePoint(Coordinate(x, y), gd_element_button_get(element_button)));
            render_cave();  /* we do this here by hand; do not use changed flag; otherwise object_list_add_to_selection wouldn't work */
            object_list_add_to_selection(edited_cave->objects.back());  /* this way all points will be selected together when using freehand */
        }
        return TRUE;
    }

    if (!object_list_is_any_selected())
        return TRUE;

    if (object_list_count_selected()==1) {
        /* MOVING, DRAGGING A SINGLE OBJECT **************************/
        CaveObject *object=object_list_first_selected();

        switch (action) {
        case TOOL_MOVE:
            /* MOVING AN EXISTING OBJECT */
            if (undo_move_flag==FALSE) {
                undo_save();
                undo_move_flag=TRUE;
            }

            object->move(Coordinate(clicked_x, clicked_y), Coordinate(dx, dy));
            break;

        /* DRAGGING THE MOUSE, WHEN THE OBJECT WAS JUST CREATED */
        case TOOL_POINT:
        case TOOL_FLOODFILL_BORDER:
        case TOOL_FLOODFILL_REPLACE:
        case TOOL_LINE:
        case TOOL_RECTANGLE:
        case TOOL_FILLED_RECTANGLE:
        case TOOL_RASTER:
        case TOOL_MAZE:
        case TOOL_MAZE_UNICURSAL:
        case TOOL_MAZE_BRAID:
        case TOOL_RANDOM_FILL:
        case TOOL_COPY_PASTE:
        case TOOL_JOIN:
            object->create_drag(Coordinate(x, y), Coordinate(dx, dy));
            break;
        default:
            g_assert_not_reached();
        }
    } else
    if (object_list_count_selected()>1 && action==TOOL_MOVE) {
        /* MOVING MULTIPLE OBJECTS */
        if (undo_move_flag==FALSE) {
            undo_save();
            undo_move_flag=TRUE;
        }

        for (std::set<CaveObject *>::iterator it=selected_objects.begin(); it!=selected_objects.end(); ++it)
            (*it)->move(Coordinate(dx, dy));
    }

    clicked_x=x;
    clicked_y=y;
    render_cave();

    return TRUE;
}

/****************************************************/


/* to remember size of window */
static gboolean
editor_window_configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
    gd_editor_window_width = event->width;
    gd_editor_window_height = event->height;

    return FALSE;
}

/* destroy editor window - do some cleanups */
static void
editor_window_destroy_event(GtkWidget *widget, gpointer data) {
    /* remove drawing interrupt. */
    g_source_remove(timeout_id);
    /* if cave is drawn, free. */
    delete rendered_cave;
    rendered_cave = NULL;
    
    delete editor_cell_renderer;
    editor_cell_renderer = NULL;
    delete editor_pixbuf_factory;
    editor_pixbuf_factory = NULL;

    /* we destroy the icon view explicitly. so the caveset gets recreated... gd_main_stop_game will need that, as it checks all caves for replay. */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);

    g_hash_table_destroy(cave_pixbufs);
}


static gboolean
editor_window_delete_event(GtkWidget *widget, GdkEventAny *event, gpointer data) {
    gtk_main_quit();
    return TRUE;
}


/****************************************************
 *
 * CAVE SELECTOR ICON VIEW
 *
 *
 */

static gboolean
icon_view_update_pixbufs_timeout(gpointer data) {
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
        CaveStored *cave;
        GdkPixbuf *pixbuf, *pixbuf_in_icon_view;

        gtk_tree_model_get(model, &iter, CAVE_COLUMN, &cave, PIXBUF_COLUMN, &pixbuf_in_icon_view, -1);
        pixbuf=(GdkPixbuf *) g_hash_table_lookup(cave_pixbufs, cave);

        /* if we have no pixbuf, generate one. */
        if (!pixbuf) {
            pixbuf_in_icon_view=NULL;   /* to force update below */
            CaveRendered *rendered=new CaveRendered(*cave, 0, 0);   /* render at level 1, seed=0 */
            pixbuf=gd_drawcave_to_pixbuf(rendered, *editor_cell_renderer, 128, 128, true, true);                   /* draw 128x128 icons at max */
            if (!cave->selectable) {
                GdkPixbuf *colored=gdk_pixbuf_composite_color_simple(pixbuf, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), GDK_INTERP_NEAREST, 160, 1, gd_flash_color.get_uint(), gd_flash_color.get_uint());
                g_object_unref(pixbuf); /* forget original */
                pixbuf=colored;
            }
            delete rendered;
            g_hash_table_insert(cave_pixbufs, cave, pixbuf);

            created++;  /* created at least one, it took time */
        }

        /* if generated a new pixbuf, or the icon view does not contain the pixbuf: */
        if (pixbuf!=pixbuf_in_icon_view)
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, PIXBUF_COLUMN, pixbuf, -1);

        gtk_tree_path_next (path);
    }
    gtk_tree_path_free (path);

    return finish;
}

static void
icon_view_update_pixbufs() {
    g_timeout_add_full(G_PRIORITY_LOW, 10, icon_view_update_pixbufs_timeout, NULL, NULL);
}


/* this is also called as an item activated signal. */
/* so we do not use its parameters. */
static void
icon_view_edit_cave_cb() {
    GList *list;
    GtkTreeIter iter;
    GtkTreeModel *model;
    CaveStored *cave;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW(iconview_cavelist));
    g_return_if_fail (list!=NULL);

    model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
    gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
    gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL); /* free the list of paths */
    g_list_free(list);
    select_cave_for_edit (cave);
    gtk_combo_box_set_active(GTK_COMBO_BOX(new_object_level_combo), 0); /* always default to level 1 */
}

static void
icon_view_rename_cave_cb(GtkWidget *widget, gpointer data) {
    GList *list;
    GtkTreeIter iter;
    GtkTreeModel *model;
    CaveStored *cave;
    GtkWidget *dialog, *entry;
    int result;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW(iconview_cavelist));
    g_return_if_fail (list!=NULL);

    /* use first element, as icon view is configured to enable only one selection */
    model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
    gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
    gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL); /* free the list of paths */
    g_list_free(list);

    dialog=gtk_dialog_new_with_buttons(_("Cave Name"), GTK_WINDOW(gd_editor_window), GtkDialogFlags(GTK_DIALOG_NO_SEPARATOR|GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    entry=gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_entry_set_text(GTK_ENTRY(entry), cave->name.c_str());
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, FALSE, 6);

    gtk_widget_show(entry);
    result=gtk_dialog_run(GTK_DIALOG(dialog));
    if (result==GTK_RESPONSE_ACCEPT) {
        cave->name=gtk_entry_get_text(GTK_ENTRY(entry));
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, NAME_COLUMN, cave->name.c_str(), -1);
    }
    gtk_widget_destroy(dialog);
}

static void
icon_view_cave_make_selectable_cb(GtkWidget *widget, gpointer data) {
    GList *list;
    GtkTreeIter iter;
    GtkTreeModel *model;
    CaveStored *cave;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW(iconview_cavelist));
    g_return_if_fail (list!=NULL);

    model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
    gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
    gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL); /* free the list of paths */
    g_list_free(list);
    if (!cave->selectable) {
        cave->selectable=TRUE;
        /* we remove its pixbuf, as its color will be different */
        g_hash_table_remove(cave_pixbufs, cave);
    }
    icon_view_update_pixbufs();
}

static void
icon_view_cave_make_unselectable_cb(GtkWidget *widget, gpointer data) {
    GList *list;
    GtkTreeIter iter;
    GtkTreeModel *model;
    CaveStored *cave;

    list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW(iconview_cavelist));
    g_return_if_fail (list!=NULL);

    model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
    gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
    gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL); /* free the list of paths */
    g_list_free(list);
    if (cave->selectable) {
        cave->selectable=FALSE;
        /* we remove its pixbuf, as its color will be different */
        g_hash_table_remove(cave_pixbufs, cave);
    }
    icon_view_update_pixbufs();
}

static void
icon_view_selection_changed_cb(GtkWidget *widget, gpointer data) {
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
        CaveStored *cave;

        model=gtk_icon_view_get_model (GTK_ICON_VIEW (widget));

        gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
        gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);

        set_status_label_for_cave(cave);    /* status bar now shows some basic data for cave */
    }
    else
        gtk_label_set_markup(GTK_LABEL (label_object), CPrintf(_("%d caves selected")) % count);
    g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
    g_list_free(list);
}


/* for caveset icon view */
static void
icon_view_destroyed (GtkIconView * icon_view, gpointer data) {
    GtkTreePath *path;
    GtkTreeModel *model;
    GtkTreeIter iter;

    /* caveset should be an empty list, as the icon view stores the current caves and order */
    g_assert(caveset->caves.empty());

    model=gtk_icon_view_get_model(icon_view);
    path=gtk_tree_path_new_first();
    while (gtk_tree_model_get_iter (model, &iter, path)) {
        CaveStored *cave;

        gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
        /* make a new list from the new order obtained from the icon view */
        caveset->caves.push_back_adopt(cave);

        gtk_tree_path_next (path);
    }
    gtk_tree_path_free (path);
}



static void
icon_view_add_cave(GtkListStore *store, CaveStored *cave) {
    GtkTreeIter treeiter;
    GdkPixbuf *cave_pixbuf;
    static GdkPixbuf *missing_image=NULL;

    if (!missing_image) {
        missing_image=gtk_widget_render_icon(gd_editor_window, GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG, NULL);
    }

    /* if we already know the pixbuf, set it. */
    cave_pixbuf=(GdkPixbuf *) g_hash_table_lookup(cave_pixbufs, cave);
    if (cave_pixbuf==NULL)
        cave_pixbuf=missing_image;
    gtk_list_store_insert_with_values (store, &treeiter, -1, CAVE_COLUMN, cave, NAME_COLUMN, cave->name.c_str(), PIXBUF_COLUMN, cave_pixbuf, -1);
}

/* does nothing else but sets caveset_edited to true. called by "reordering" (drag&drop), which is implemented by gtk+ by inserting and deleting */
/* we only connect this signal after adding all caves to the icon view, so it is only activated by the user! */
static void
icon_view_row_inserted(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    caveset->edited=TRUE;
}

/* for popup menu, by properties key */
static void
icon_view_popup_menu(GtkWidget *widget, gpointer data) {
    gtk_menu_popup(GTK_MENU(caveset_popup), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

/* for popup menu, by right-click */
static gboolean
icon_view_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
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
select_cave_for_edit(CaveStored *cave) {
    object_list_clear_selection();

    gtk_action_group_set_sensitive (actions_edit_object, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_edit_one_object, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_edit_cave, cave!=NULL);
    gtk_action_group_set_sensitive (actions_edit_caveset, cave==NULL);
    gtk_action_group_set_sensitive (actions_edit_tools, cave!=NULL);
    gtk_action_group_set_sensitive (actions_edit_map, FALSE);   /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_edit_random, FALSE);    /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_toggle, cave!=NULL);
    /* this is sensitized by an icon selector callback. */
    gtk_action_group_set_sensitive (actions_cave_selector, FALSE);  /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_clipboard, FALSE);  /* will be enabled later if needed */
    gtk_action_group_set_sensitive (actions_clipboard_paste,
        (cave!=NULL && !object_clipboard.empty())
        || (cave==NULL && !cave_clipboard.empty()));
    gtk_action_group_set_sensitive (actions_edit_undo, cave!=NULL && !undo_caves.empty());
    gtk_action_group_set_sensitive (actions_edit_redo, cave!=NULL && !redo_caves.empty());

    /* select cave */
    edited_cave=cave;

    /* if cave data given, show it. */
    if (cave) {
        if (iconview_cavelist)
            gtk_widget_destroy(iconview_cavelist);

        if (gd_show_object_list)
            gtk_widget_show (scroll_window_objects);

        /* create pixbufs for these colors */
        editor_cell_renderer->select_pixbuf_colors(edited_cave->color0, edited_cave->color1, edited_cave->color2, edited_cave->color3, edited_cave->color4, edited_cave->color5);
        gd_element_button_update_pixbuf(element_button);
        gd_element_button_update_pixbuf(fillelement_button);

        /* put drawing area in an alignment, so window can be any large w/o problems */
        if (!drawing_area) {
            GtkWidget *align=gtk_alignment_new(0.5, 0.5, 0, 0);
            gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll_window), align);
            gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_window), GTK_SHADOW_NONE);

            drawing_area = gtk_drawing_area_new();
            screen = new GTKScreen(drawing_area);
            mouse_x=mouse_y=-1;
            /* enable some events */
            gtk_widget_add_events(drawing_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK);
            g_signal_connect(G_OBJECT(drawing_area), "destroy", G_CALLBACK(drawing_area_destroyed), NULL);
            g_signal_connect(G_OBJECT(drawing_area), "button_press_event", G_CALLBACK(drawing_area_button_press_event), NULL);
            g_signal_connect(G_OBJECT(drawing_area), "button_release_event", G_CALLBACK(drawing_area_button_release_event), NULL);
            g_signal_connect(G_OBJECT(drawing_area), "motion_notify_event", G_CALLBACK(drawing_area_motion_event), NULL);
            g_signal_connect(G_OBJECT(drawing_area), "leave_notify_event", G_CALLBACK(drawing_area_leave_event), NULL);
            g_signal_connect(G_OBJECT(drawing_area), "expose_event", G_CALLBACK(drawing_area_expose_event), NULL);
            gtk_container_add(GTK_CONTAINER(align), drawing_area);
        }
        delete rendered_cave;
        rendered_cave=0;
        render_cave();
        int cs=editor_cell_renderer->get_cell_size();
        screen->set_size(edited_cave->w*cs, edited_cave->h*cs);
    }
    else {
        /* if no cave given, show selector. */
        /* forget undo caves */
        undo_free_all();

        gfx_buffer.remove();
        object_highlight_map.remove();
        delete rendered_cave;
        rendered_cave=0;

        gtk_list_store_clear(object_list);
        gtk_widget_hide(scroll_window_objects);

        if (drawing_area)
            gtk_widget_destroy(drawing_area->parent->parent);
        /* parent is the align, parent of align is the viewport automatically added. */

        if (!iconview_cavelist) {
            GtkListStore *cave_list;

            gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll_window), GTK_SHADOW_IN);

            /* create list store for caveset */
            cave_list=gtk_list_store_new(NUM_CAVESET_COLUMNS, G_TYPE_POINTER, G_TYPE_STRING, GDK_TYPE_PIXBUF);
            for (unsigned n=0; n<caveset->caves.size(); n++)
                icon_view_add_cave(cave_list, &caveset->cave(n));
            /* we only connect this signal after adding all caves to the icon view, so it is only activated by the user! */
            g_signal_connect(G_OBJECT(cave_list), "row-inserted", G_CALLBACK(icon_view_row_inserted), NULL);
            /* forget caveset; now we store caves in the GtkListStore */
            caveset->caves.clear_i_own_them();

            iconview_cavelist=gtk_icon_view_new_with_model (GTK_TREE_MODEL (cave_list));
            g_object_unref(cave_list);  /* now the icon view holds the reference */
            icon_view_update_pixbufs(); /* create icons */
            g_signal_connect(G_OBJECT(iconview_cavelist), "destroy", G_CALLBACK(icon_view_destroyed), &iconview_cavelist);
            g_signal_connect(G_OBJECT(iconview_cavelist), "destroy", G_CALLBACK(gtk_widget_destroyed), &iconview_cavelist);
            g_signal_connect(G_OBJECT(iconview_cavelist), "popup-menu", G_CALLBACK(icon_view_popup_menu), NULL);
            g_signal_connect(G_OBJECT(iconview_cavelist), "button-press-event", G_CALLBACK(icon_view_button_press_event), NULL);

            gtk_icon_view_set_text_column(GTK_ICON_VIEW(iconview_cavelist), NAME_COLUMN);
            gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW (iconview_cavelist), PIXBUF_COLUMN);
            gtk_icon_view_set_item_width(GTK_ICON_VIEW(iconview_cavelist), 128+24); /* 128 is the size of the icons */
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
    gtk_widget_show_all(scroll_window);

    /* hide toolbars if not editing a cave */
    if (edited_cave)
        gtk_widget_show (toolbars);
    else
        gtk_widget_hide (toolbars);

    editor_window_set_title();
}

/****************************************************/
static void
cave_random_setup_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail(edited_cave->map.empty());

    undo_save();

    GtkWidget *dialog, *notebook;
    edit_properties_create_window(_("Cave Random Fill"), false, dialog, notebook);

    std::list<EditorAutoUpdate *> eau_s;
    edit_properties_add_widgets(notebook, eau_s, CaveStored::random_dialog, edited_cave, edited_cave, render_cave);

    /* hint label */
    gd_dialog_add_hint(GTK_DIALOG(dialog), _("Hint: The random fill works by generating a random number between 0 and "
    "255, then choosing an element from the list above. If the number generated is bigger than probability 1, "
    "initial fill is chosen. If smaller than probability 1, but bigger than probability 2, the first element is chosen "
    "and so on. GDash will make sure that the probability values come in descending order."));

    gtk_widget_show_all(dialog);
    int result=gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    for (std::list<EditorAutoUpdate *>::const_iterator it=eau_s.begin(); it!=eau_s.end(); ++it)
        delete *it;

    /* if not accepted by user, revert to original */
    if (result!=GTK_RESPONSE_ACCEPT)
        undo_do_one_step_but_no_redo();
}


/*******************************************
 *
 * CAVE
 *
 *******************************************/
static void
save_cave_png (GdkPixbuf *pixbuf) {
    /* if no filename given, */
    GtkWidget *dialog;
    GtkFileFilter *filter;
    GError *error=NULL;
    char *filename=NULL;

    /* check if in cave editor */
    g_return_if_fail (edited_cave!=NULL);

    dialog=gtk_file_chooser_dialog_new(_("Save Cave as PNG Image"), GTK_WINDOW(gd_editor_window), GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    filter=gtk_file_filter_new();
    gtk_file_filter_set_name (filter, _("PNG files"));
    gtk_file_filter_add_pattern (filter, "*.png");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), CPrintf("%s.png") % edited_cave->name);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
        filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
        if (!g_str_has_suffix(filename, ".png")) {
            char *suffixed=g_strdup_printf("%s.png", filename);

            g_free(filename);
            filename=suffixed;
        }

        gdk_pixbuf_save(pixbuf, filename , "png", &error, "compression", "9", NULL);
    }
    if (error) {
        gd_errormessage(error->message, NULL);
        g_error_free(error);
    }
    gtk_widget_destroy(dialog);
    g_free(filename);
}

/* CAVE OVERVIEW
   this creates a pixbuf of the cave, and scales it down to fit the screen if needed.
   it is then presented to the user, with the option to save it in png */
static void cave_overview(gboolean simple_view) {
    /* view the RENDERED one, and the entire cave */
    GdkPixbuf *pixbuf=gd_drawcave_to_pixbuf(rendered_cave, *editor_cell_renderer, 0, 0, simple_view, true), *scaled;
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
    dialog=gtk_dialog_new_with_buttons(_("Cave Overview"), GTK_WINDOW(gd_editor_window), GtkDialogFlags(GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT), NULL);
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
cave_overview_cb(GtkWidget *widget, gpointer data) {
    cave_overview(FALSE);
}

static void
cave_overview_simple_cb(GtkWidget *widget, gpointer data) {
    cave_overview(TRUE);
}


/*
    shrink cave
    if last line or last row is just steel wall (or (invisible) outbox).
    used after loading a game for playing.
    after this, ew and eh will contain the effective width and height.
 */
static void
rendered_cave_auto_shrink(CaveRendered *cave) {
    enum {
        STEEL_ONLY,
        STEEL_OR_OTHER,
        NO_SHRINK
    } empty;

    /* set to maximum size, then try to shrink */
    cave->x1=0; cave->y1=0;
    cave->x2=cave->w-1; cave->y2=cave->h-1;

    /* search for empty, steel-wall-only last rows. */
    /* clear all lines, which are only steel wall.
     * and clear only one line, which is steel wall, but also has a player or an outbox. */
    empty=STEEL_ONLY;
    do {
        for (int y=cave->y2-1; y<=cave->y2; y++)
            for (int x=cave->x1; x<=cave->x2; x++)
                switch (cave->map(x, y)) {
                case O_STEEL:   /* if steels only, this is to be deleted. */
                    break;
                case O_PRE_OUTBOX:
                case O_PRE_INVIS_OUTBOX:
                case O_INBOX:
                    if (empty==STEEL_OR_OTHER)
                        empty=NO_SHRINK;
                    if (empty==STEEL_ONLY)  /* if this, delete only this one, and exit. */
                        empty=STEEL_OR_OTHER;
                    break;
                default:        /* anything else, that should be left in the cave. */
                    empty=NO_SHRINK;
                    break;
                }
        if (empty!=NO_SHRINK)   /* shrink if full steel or steel and player/outbox. */
            cave->y2--;         /* one row shorter */
    }
    while (empty==STEEL_ONLY);  /* if found just steels, repeat. */

    /* search for empty, steel-wall-only first rows. */
    empty=STEEL_ONLY;
    do {
        for (int y=cave->y1; y<=cave->y1+1; y++)
            for (int x=cave->x1; x<=cave->x2; x++)
                switch (cave->map(x, y)) {
                case O_STEEL:
                    break;
                case O_PRE_OUTBOX:
                case O_PRE_INVIS_OUTBOX:
                case O_INBOX:
                    /* shrink only lines, which have only ONE player or outbox. this is for bd4 intermission 2, for example. */
                    if (empty==STEEL_OR_OTHER)
                        empty=NO_SHRINK;
                    if (empty==STEEL_ONLY)
                        empty=STEEL_OR_OTHER;
                    break;
                default:
                    empty=NO_SHRINK;
                    break;
                }
        if (empty!=NO_SHRINK)
            cave->y1++;
    }
    while (empty==STEEL_ONLY);  /* if found one, repeat. */

    /* empty last columns. */
    empty=STEEL_ONLY;
    do {
        for (int y=cave->y1; y<=cave->y2; y++)
            for (int x=cave->x2-1; x<=cave->x2; x++)
                switch (cave->map(x, y)) {
                case O_STEEL:
                    break;
                case O_PRE_OUTBOX:
                case O_PRE_INVIS_OUTBOX:
                case O_INBOX:
                    if (empty==STEEL_OR_OTHER)
                        empty=NO_SHRINK;
                    if (empty==STEEL_ONLY)
                        empty=STEEL_OR_OTHER;
                    break;
                default:
                    empty=NO_SHRINK;
                    break;
                }
        if (empty!=NO_SHRINK)
            cave->x2--;         /* just remember that one column shorter. g_free will know the size of memchunk, no need to realloc! */
    }
    while (empty==STEEL_ONLY);  /* if found one, repeat. */

    /* empty first columns. */
    empty=STEEL_ONLY;
    do {
        for (int y=cave->y1; y<=cave->y2; y++)
            for (int x=cave->x1; x<=cave->x1+1; x++)
                switch (cave->map(x, y)) {
                case O_STEEL:
                    break;
                case O_PRE_OUTBOX:
                case O_PRE_INVIS_OUTBOX:
                case O_INBOX:
                    if (empty==STEEL_OR_OTHER)
                        empty=NO_SHRINK;
                    if (empty==STEEL_ONLY)
                        empty=STEEL_OR_OTHER;
                    break;
                default:
                    empty=NO_SHRINK;
                    break;
                }
        if (empty!=NO_SHRINK)
            cave->x1++;
    }
    while (empty==STEEL_ONLY);  /* if found one, repeat. */
}


/* automatically shrink cave
 */
static void
auto_shrink_cave_cb(GtkWidget *widget, gpointer data) {
    undo_save();
    /* shrink the rendered cave, as it has all object and the like converted to a map. */
    rendered_cave_auto_shrink(rendered_cave);
    /* then copy the results to the original */
    edited_cave->x1=rendered_cave->x1;
    edited_cave->y1=rendered_cave->y1;
    edited_cave->x2=rendered_cave->x2;
    edited_cave->y2=rendered_cave->y2;

    /* re-render cave; after that, selecting visible region tool allows the user to see the result, maybe modify */
    render_cave();  /* not really needed? does not hurt, anyway. */
    select_tool(TOOL_VISIBLE_REGION);
}


/*
 *
 * SET CAVE COLORS WITH INSTANT UPDATE TOOL.
 *
 */
static gboolean cave_colors_colorchange_update_disabled;

/* helper: update pixmaps and the like */
static void
cave_colors_update_element_pixbufs() {
    if (cave_colors_colorchange_update_disabled)
        return;
    /* select new colors - render cave does not do this */
    editor_cell_renderer->select_pixbuf_colors(edited_cave->color0, edited_cave->color1, edited_cave->color2, edited_cave->color3, edited_cave->color4, edited_cave->color5);
    /* update element buttons in editor (under toolbar) */
    gd_element_button_update_pixbuf(element_button);
    gd_element_button_update_pixbuf(fillelement_button);
    /* clear gfx buffer, so every element gets redrawn */
    gfx_buffer.fill(-1);
    /* for object list update with new pixbufs */
    render_cave();
}


/* when the random colors button is pressed, first we change the colors of the cave. */
/* then we update the combo boxes one by one (they are in a glist *), but before that, */
/* we disable their updating behaviour. otherwise they would re-render pixmaps one by one, */
/* and they would also want to change the cave itself - the cave which already contains the */
/* changed colors! */
static void
cave_colors_random_combo_cb(GtkWidget *widget, gpointer data) {
    std::list<EditorAutoUpdate *> *eau_s=static_cast<std::list<EditorAutoUpdate *> *>(data);
    int new_index;

    new_index=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    if (new_index==0)   /* 0 is the "set random..." text, so we do nothing */
        return;

    /* create colors */
    gd_cave_set_random_colors(*edited_cave, GdColor::Type(new_index-1));    /* -1: 0 is "set random...", 1 is rgb... */

    /* and update combo boxes from cave */
    cave_colors_colorchange_update_disabled=TRUE;   /* this is needed, otherwise all combos would want to update the pixbufs, one by one */
    for (std::list<EditorAutoUpdate *>::const_iterator it=eau_s->begin(); it!=eau_s->end(); ++it)
        (*it)->reload();
    cave_colors_colorchange_update_disabled=FALSE;
    cave_colors_update_element_pixbufs();

    /* set back to "select random..." text */
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
}

/* set cave colors with instant update */
static void cave_colors_cb(GtkWidget *widget, gpointer data) {
    undo_save();

    /* when editing colors, turn off the colored objects viewing for a while. */
    bool colored_objects_backup=gd_colored_objects;
    gd_colored_objects=false;

    GtkWidget *dialog, *notebook;
    edit_properties_create_window(_("Cave Colors"), false, dialog, notebook);

    GtkWidget *random_combo=gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), random_combo, FALSE, FALSE, 0);
    gtk_box_reorder_child(GTK_BOX(GTK_DIALOG(dialog)->action_area), random_combo, 0);

    std::list<EditorAutoUpdate *> eau_s;
    edit_properties_add_widgets(notebook, eau_s, CaveStored::color_dialog, edited_cave, edited_cave, cave_colors_update_element_pixbufs);

    /* a combo box which has a callback that sets random colors */
    gtk_combo_box_append_text(GTK_COMBO_BOX(random_combo), _("Set random...")); /* will be active=0 */
    for (int i=0; GdColor::get_palette_types_names()[i]!=NULL; i++)
        gtk_combo_box_append_text(GTK_COMBO_BOX(random_combo), _(GdColor::get_palette_types_names()[i]));
    gtk_combo_box_set_active(GTK_COMBO_BOX(random_combo), 0);
    g_signal_connect(random_combo, "changed", G_CALLBACK(cave_colors_random_combo_cb), &eau_s);

    /* hint label */
    gd_dialog_add_hint(GTK_DIALOG(dialog), _("Hint: As the palette can be changed for C64 and Atari colors, "
    "it is not recommended to use different types together (for example, RGB color for background, Atari color for Slime.)"));

    gtk_widget_show_all(dialog);
    cave_colors_colorchange_update_disabled=FALSE;
    int result=gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    for (std::list<EditorAutoUpdate *>::const_iterator it=eau_s.begin(); it!=eau_s.end(); ++it)
        delete *it;
    /* if the new colors were not accepted by the user (escape pressed), we undo the changes. */
    if (result!=GTK_RESPONSE_ACCEPT)
        undo_do_one_step_but_no_redo();

    /* restore colored objects setting. */
    gd_colored_objects=colored_objects_backup;
}


/***************************************************
 *
 * CAVE EDITING CALLBACKS
 *
 */

/* delete selected cave drawing element or cave.
*/
static void
delete_selected_cb(GtkWidget *widget, gpointer data) {
    /* deleting caves or cave object. */
    if (edited_cave==NULL) {
        /* WE ARE DELETING ONE OR MORE CAVES HERE */

        /* first we ask the user if he is sure, as no undo is implemented yet */
        gboolean response = gd_question_yesno(_("Do you really want to delete cave(s)?"), _("This operation cannot be undone."));

        if (!response)
            return;
        GList *list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (iconview_cavelist));
        g_return_if_fail (list!=NULL);  /* list should be not empty. otherwise why was the button not insensitized? */

        /* if anything was selected */
        GtkTreeModel *model = gtk_icon_view_get_model(GTK_ICON_VIEW(iconview_cavelist));
        /* for all caves selected, convert to tree row references - we must delete them for the icon view, so this is necessary */
        GList *references=NULL;
        for (GList *listiter = list; listiter!=NULL; listiter=listiter->next)
            references=g_list_append(references, gtk_tree_row_reference_new(model, (GtkTreePath *)listiter->data));
        g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(list);

        /* now check the list of references and delete each cave */
        for (GList *listiter = references; listiter!=NULL; listiter=listiter->next) {
            GtkTreeRowReference *reference=(GtkTreeRowReference *)listiter->data;
            GtkTreePath *path = gtk_tree_row_reference_get_path(reference);
            GtkTreeIter iter;
            gtk_tree_model_get_iter(model, &iter, path);
            CaveStored *cave;
            gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            delete cave;    /* and also free memory associated. */
            g_hash_table_remove(cave_pixbufs, cave);
        }
        g_list_foreach(references, (GFunc) gtk_tree_row_reference_free, NULL);
        g_list_free(references);

        /* this modified the caveset */
        caveset->edited=TRUE;
    } else {
        /* WE ARE DELETING A CAVE OBJECT HERE */
        g_return_if_fail(object_list_is_any_selected());

        undo_save();

        /* delete all objects */
        edited_cave->objects.remove_if(object_list_is_selected);
        object_list_clear_selection();
        render_cave();
    }
}

/* put selected drawing elements to bottom.
*/
static void send_to_back_selected_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail(object_list_is_any_selected());

    undo_save();
    std::stable_partition(edited_cave->objects.begin(), edited_cave->objects.end(), object_list_is_selected);
    render_cave();
}

/* bring selected drawing element to top. */
static void bring_to_front_selected_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail(object_list_is_any_selected());

    undo_save();
    std::stable_partition(edited_cave->objects.begin(), edited_cave->objects.end(), object_list_is_not_selected);
    render_cave();
}

/* enable currently selected objects on the currently viewed level only. */
static void
show_object_this_level_only_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    for (std::set<CaveObject *>::iterator it=selected_objects.begin(); it!=selected_objects.end(); ++it) {
        CaveObject *obj=*it;

        obj->disable_on_all();
        obj->seen_on[edit_level]=true;
    }
    render_cave();
}

/* enable currently selected objects on all levels */
static void
show_object_all_levels_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    for (std::set<CaveObject *>::iterator it=selected_objects.begin(); it!=selected_objects.end(); ++it) {
        CaveObject *obj=*it;

        obj->enable_on_all();
    }
    render_cave();
}

/* enable currently selected objects on the currently viewed level only. */
static void
show_object_on_this_level_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    for (std::set<CaveObject *>::iterator it=selected_objects.begin(); it!=selected_objects.end(); ++it) {
        CaveObject *obj=*it;

        obj->seen_on[edit_level]=true;
    }
    render_cave();
}

/* enable currently selected objects on the currently viewed level only. */
static void
hide_object_on_this_level_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail (object_list_is_any_selected());

    undo_save();

    int disappear=0;
    for (std::set<CaveObject *>::iterator it=selected_objects.begin(); it!=selected_objects.end(); ++it) {
        CaveObject *obj=*it;

        obj->seen_on[edit_level]=false;
        /* an object should be visible on at least one level. */
        /* if it disappeared, switch it back, and remember that we will show an error message. */
        if (obj->is_invisible()) {
            obj->seen_on[edit_level]=true;
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
copy_selected_cb(GtkWidget *widget, gpointer data) {
    if (edited_cave==NULL) {
        /* WE ARE NOW COPYING CAVES FROM A CAVESET */
        GList *list, *listiter;
        GtkTreeModel *model;

        list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW (iconview_cavelist));
        g_return_if_fail (list!=NULL);  /* list should be not empty. otherwise why was the button not insensitized? */

        /* forget old clipboard */
        cave_clipboard.clear();

        /* now for all caves selected */
        /* we do not need references here (as in cut), as we do not modify the treemodel */
        model=gtk_icon_view_get_model(GTK_ICON_VIEW(iconview_cavelist));
        for (listiter=list; listiter!=NULL; listiter=listiter->next) {
            CaveStored *cave=NULL;
            GtkTreeIter iter;

            gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) listiter->data);
            gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
            /* add to clipboard: prepend must be used for correct order */
            /* here, a COPY is added to the clipboard */
            cave_clipboard.push_front(*cave);
        }
        g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(list);

        /* enable pasting */
        gtk_action_group_set_sensitive (actions_clipboard_paste, TRUE);
    } else {
        /* delete contents of clipboard */
        object_clipboard=object_list_copy_of_selected();

        /* enable pasting */
        gtk_action_group_set_sensitive (actions_clipboard_paste, TRUE);
    }
}

/* paste object or cave from clipboard
*/
static void
paste_clipboard_cb(GtkWidget *widget, gpointer data) {
    if (edited_cave==NULL) {
        /* WE ARE IN THE CAVESET ICON VIEW */
        GtkListStore *store=GTK_LIST_STORE(gtk_icon_view_get_model(GTK_ICON_VIEW(iconview_cavelist)));

        for (std::list<CaveStored>::const_iterator it=cave_clipboard.begin(); it!=cave_clipboard.end(); ++it)
            icon_view_add_cave(store, new CaveStored(*it));
        icon_view_update_pixbufs();

        /* this modified the caveset */
        caveset->edited=TRUE;
    } else {
        /* WE ARE IN THE CAVE EDITOR */
        std::list<CaveObject *> newly_added_objects;

        g_return_if_fail (!object_clipboard.empty());

        /* we have a list of newly added (pasted) objects, so after pasting we can
           select them. this is necessary, as only after pasting is render_cave() called,
           which adds the new objects to the gtkliststore. otherwise that one would not
           contain cave objects. the clipboard also cannot be used, as pointers are different */
        for (CaveObjectStore::const_iterator it=object_clipboard.begin(); it!=object_clipboard.end(); ++it) {
            CaveObject *cloned=(*it)->clone();
            edited_cave->objects.push_back_adopt(cloned);
            newly_added_objects.push_back(cloned);
        }

        render_cave();
        object_list_clear_selection();
        for_each(newly_added_objects.begin(), newly_added_objects.end(), object_list_add_to_selection);
    }
}


/* cut an object, or cave(s) from the caveset. */
static void
cut_selected_cb(GtkWidget *widget, gpointer data) {
    if (edited_cave==NULL) {
        /* WE ARE NOW CUTTING CAVES FROM A CAVESET */
        GList *list, *listiter;
        GtkTreeModel *model;
        GList *references=NULL;

        list=gtk_icon_view_get_selected_items (GTK_ICON_VIEW (iconview_cavelist));
        g_return_if_fail (list!=NULL);  /* list should be not empty. otherwise why was the button not insensitized? */

        /* forget old clipboard */
        cave_clipboard.clear();

        /* if anything was selected */
        model=gtk_icon_view_get_model (GTK_ICON_VIEW (iconview_cavelist));
        /* for all caves selected, convert to tree row references - we must delete them for the icon view, so this is necessary */
        for (listiter=list; listiter!=NULL; listiter=listiter->next)
            references=g_list_append(references, gtk_tree_row_reference_new(model, (GtkTreePath *)listiter->data));
        g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(list);

        for (listiter=references; listiter!=NULL; listiter=listiter->next) {
            GtkTreeRowReference *reference=(GtkTreeRowReference *) listiter->data;
            GtkTreePath *path;
            CaveStored *cave=NULL;
            GtkTreeIter iter;

            path=gtk_tree_row_reference_get_path(reference);
            gtk_tree_model_get_iter (model, &iter, path);
            gtk_tree_model_get (model, &iter, CAVE_COLUMN, &cave, -1);
            /* prepend must be used for correct order */
            /* here, the cave is not copied, but the pointer is moved to the clipboard */
            cave_clipboard.push_front(*cave);
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            /* remove its pixbuf */
            g_hash_table_remove(cave_pixbufs, cave);
        }
        g_list_foreach(references, (GFunc) gtk_tree_row_reference_free, NULL);
        g_list_free(references);

        /* enable pasting */
        gtk_action_group_set_sensitive (actions_clipboard_paste, TRUE);

        /* this modified the caveset */
        caveset->edited=TRUE;
    } else {
        /* EDITED OBJECT IS NOT NULL, SO WE ARE CUTTING OBJECTS */
        undo_save();

        /* delete contents of clipboard */
        object_clipboard.clear();

        object_clipboard=object_list_copy_of_selected();
        edited_cave->objects.remove_if(object_list_is_selected);

        /* enable pasting */
        gtk_action_group_set_sensitive (actions_clipboard_paste, TRUE);

        object_list_clear_selection();
        render_cave();
    }
}

static void
select_all_cb(GtkWidget *widget, gpointer data) {
    if (edited_cave==NULL)  /* in game editor */
        gtk_icon_view_select_all(GTK_ICON_VIEW(iconview_cavelist));     /* SELECT ALL CAVES */
    else                    /* in cave editor */
        gtk_tree_selection_select_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)));       /* SELECT ALL OBJECTS */
}

/* delete map from cave */
static void
remove_map_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail (edited_cave!=NULL);
    g_return_if_fail (!edited_cave->map.empty());

    gboolean response = gd_question_yesno(_("Do you really want to remove cave map?"), _("This operation destroys all cave objects."));

    if (response) {
        undo_save(); /* changing cave; save for undo */

        edited_cave->map.remove();
        /* map deleted; redraw cave */
        render_cave();
    }
}


/* flatten cave -> pack everything in a map */
static void
flatten_cave_cb(GtkWidget *widget, gpointer data) {
    gboolean response;

    g_return_if_fail (edited_cave!=NULL);

    if (edited_cave->objects.empty()) {
        gd_infomessage(_("This cave has no objects."), NULL);
        return;
    }

    response=gd_question_yesno(_("Do you really want to flatten cave?"), _("This operation merges all cave objects currently seen in a single map. Further objects may later be added, but the ones already seen will behave like the random fill elements; they will not be editable."));

    if (response) {
        undo_save();    /* changing; save for undo */

        CaveRendered rendered(*edited_cave, edit_level, 0);   /* render cave at specified level to obtain map. seed=0 */
        edited_cave->map=rendered.map;      /* copy new map to cave */
        edited_cave->objects.clear();       /* forget objects */
        render_cave();      /* redraw */
    }
}

/* shift cave map left, one step. */
static void
shift_left_cb(GtkWidget *widget, gpointer data) {
    CaveMap<GdElementEnum> mapcopy(edited_cave->map);
    mapcopy.set_wrap_type(CaveMapBase::Perfect);
    for (int y=0; y<edited_cave->h; y++)
        for (int x=0; x<edited_cave->w; x++)
            edited_cave->map(x, y)=mapcopy(x+1, y);
    render_cave();
}

/* shift cave map right, one step. */
static void
shift_right_cb(GtkWidget *widget, gpointer data) {
    CaveMap<GdElementEnum> mapcopy(edited_cave->map);
    mapcopy.set_wrap_type(CaveMapBase::Perfect);
    for (int y=0; y<edited_cave->h; y++)
        for (int x=0; x<edited_cave->w; x++)
            edited_cave->map(x, y)=mapcopy(x-1, y);
    render_cave();
}

/* shift cave map up, one step. */
static void
shift_up_cb(GtkWidget *widget, gpointer data) {
    CaveMap<GdElementEnum> mapcopy(edited_cave->map);
    mapcopy.set_wrap_type(CaveMapBase::Perfect);
    for (int y=0; y<edited_cave->h; y++)
        for (int x=0; x<edited_cave->w; x++)
            edited_cave->map(x, y)=mapcopy(x, y+1);
    render_cave();
}

/* shift cave map down, one step. */
static void
shift_down_cb(GtkWidget *widget, gpointer data) {
    CaveMap<GdElementEnum> mapcopy(edited_cave->map);
    mapcopy.set_wrap_type(CaveMapBase::Perfect);
    for (int y=0; y<edited_cave->h; y++)
        for (int x=0; x<edited_cave->w; x++)
            edited_cave->map(x, y)=mapcopy(x, y-1);
    render_cave();
}


static void
set_engine_default_cb(GtkWidget *widget, gpointer data) {
    GdEngineEnum e=GdEngineEnum(GPOINTER_TO_INT(data));

    g_assert(e>=0 && e<GD_ENGINE_MAX);
    g_assert(edited_cave!=NULL);

    undo_save();
    C64Import::cave_set_engine_defaults(*edited_cave, e);
// TODO 
//  props=gd_struct_explain_defaults_in_string(CaveStored::descriptor, gd_get_engine_default_array(e));
//  gd_infomessage(_("The following properties are set:"), props);
//  g_free(props);
}


static void
save_html_cb(GtkWidget *widget, gpointer data) {
    char *htmlname=NULL, *suggested_name;
    /* if no filename given, */
    GtkWidget *dialog;
    GtkFileFilter *filter;
    CaveStored *edited;

    edited=edited_cave;
    /* destroy icon view so it does not interfere with saving (when icon view is active, caveset=NULL) */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);

    dialog=gtk_file_chooser_dialog_new(_("Save Cave Set in HTML"), GTK_WINDOW(gd_editor_window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    filter=gtk_file_filter_new();
    gtk_file_filter_set_name (filter, _("HTML files"));
    gtk_file_filter_add_pattern (filter, "*.html");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    suggested_name=g_strdup_printf("%s.html", caveset->name.c_str());
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), suggested_name);
    g_free(suggested_name);

    if (gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
        htmlname=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy(dialog);

    /* saving if got filename */
    if (htmlname) {
        Logger l;
        gd_save_html(htmlname, gd_editor_window, *caveset);
        gd_show_errors(l, _("Errors - Saving Gallery to File"));
    }
    g_free(htmlname);
    select_cave_for_edit(edited);   /* go back to edited cave or recreate icon view */
}


/* export cave to a crli editor format */
static void export_cavefile_cb(GtkWidget *widget, gpointer data) {
    char *outname=NULL;
    /* if no filename given, */
    GtkWidget *dialog;

    g_return_if_fail(edited_cave!=NULL);

    dialog=gtk_file_chooser_dialog_new(_("Export Cave as CrLi Cave File"), GTK_WINDOW(gd_editor_window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), edited_cave->name.c_str());
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
        outname=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy(dialog);

    /* if accepted, save. */
    if (outname) {
        Logger l;
        gd_export_cave_to_crli_cavefile(edited_cave, edit_level, outname);
        gd_show_errors(l, _("Errors - Exporting Cave to CrLi Format"));
        g_free(outname);
    }
}


/* export complete caveset to a crli cave pack */
static void export_cavepack_cb(GtkWidget *widget, gpointer data) {
    char *outname=NULL;
    /* if no filename given, */
    GtkWidget *dialog;
    CaveStored *edited;

    edited=edited_cave;
    /* destroy icon view so it does not interfere with saving (when icon view is active, caveset=NULL) */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);

    dialog=gtk_file_chooser_dialog_new(_("Export Cave as CrLi Cave Pack"), GTK_WINDOW(gd_editor_window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), caveset->name.c_str());
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
        outname=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy(dialog);

    /* saving if got filename */
    if (outname) {
        Logger l;
        std::vector<CaveStored *> caves_to_export;
        for (unsigned i=0; i<caveset->caves.size(); ++i)
            caves_to_export.push_back(&caveset->cave(i));
        gd_export_caves_to_crli_cavepack(caves_to_export, edit_level, outname);
        gd_show_errors(l, _("Errors - Exporting Caves to File"));
        g_free(outname);
    }

    select_cave_for_edit(edited);   /* go back to edited cave or recreate icon view */
}


/* test selected level. */
static void
play_level_cb(GtkWidget *widget, gpointer data) {
    g_return_if_fail(edited_cave!=NULL);
    gtk_widget_set_sensitive(gd_editor_window, FALSE);
    GameControl *game = GameControl::new_test(edited_cave, edit_level);
    gd_main_window_sdl_run_a_game(game);
    gtk_widget_set_sensitive(gd_editor_window, TRUE);
}


static void
object_properties_cb(GtkWidget *widget, gpointer data) {
    object_properties(NULL);
}


/* edit caveset properties */
static void
set_caveset_properties_cb(GtkWidget *widget, gpointer data) {
    caveset_properties(true);
}


static void
cave_properties_cb(const GtkWidget *widget, const gpointer data) {
    cave_properties (edited_cave, TRUE);
}


/************************************************
 *
 * MANAGING CAVES
 *
 */

/* go to cave selector */
static void
cave_selector_cb(GtkWidget *widget, gpointer data) {
    select_cave_for_edit(NULL);
}

/* view next cave */
static void
previous_cave_cb(GtkWidget *widget, gpointer data) {
    int i;
    g_return_if_fail(edited_cave!=NULL);
    g_return_if_fail(caveset->has_caves());

    i=caveset->cave_index(edited_cave);
    g_return_if_fail(i!=-1);
    i=(i-1+caveset->caves.size())%caveset->caves.size();
    select_cave_for_edit(&caveset->cave(i));
}

/* go to cave selector */
static void
next_cave_cb(GtkWidget *widget, gpointer data) {
    int i;
    g_return_if_fail(edited_cave!=NULL);
    g_return_if_fail(caveset->has_caves());

    i=caveset->cave_index(edited_cave);
    g_return_if_fail(i!=-1);
    i=(i+1)%caveset->caves.size();
    select_cave_for_edit(&caveset->cave(i));
}

/* create new cave */
static void
new_cave_cb(GtkWidget *widget, gpointer data) {
    CaveStored *newcave;
    GtkWidget *dialog, *entry_name, *entry_desc, *intermission_check, *table;

    dialog=gtk_dialog_new_with_buttons(_("Create New Cave"), GTK_WINDOW(gd_editor_window), GtkDialogFlags(GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_NEW, GTK_RESPONSE_ACCEPT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    table=gtk_table_new(0, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER (table), 6);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);

    /* some properties - name */
    gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_leftaligned (_("Name:")), 0, 1, 0, 1);
    entry_name=gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry_name), TRUE);
    gtk_entry_set_text(GTK_ENTRY(entry_name), _("New cave"));
    gtk_table_attach_defaults(GTK_TABLE(table), entry_name, 1, 2, 0, 1);

    /* description */
    gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_leftaligned (_("Description:")), 0, 1, 1, 2);
    entry_desc=gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry_desc), TRUE);
    gtk_table_attach_defaults(GTK_TABLE(table), entry_desc, 1, 2, 1, 2);

    /* intermission */
    gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_leftaligned (_("Intermission:")), 0, 1, 2, 3);
    intermission_check=gtk_check_button_new();
    gtk_widget_set_tooltip_text(intermission_check, _("Intermission caves are usually small and fast caves, which are not required to be solved. The player will not lose a life if he is not successful. The game always proceeds to the next cave. If you set this check box, the size of the cave will also be set to 20x12, as that is the standard size for intermissions."));
    gtk_table_attach_defaults(GTK_TABLE(table), intermission_check, 1, 2, 2, 3);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        /* new cave */
        newcave=new CaveStored;

        /* set some defaults */
        gd_cave_set_random_colors(*newcave, GdColor::Type(gd_preferred_palette));
        newcave->name=gtk_entry_get_text(GTK_ENTRY(entry_name));
        newcave->description=gtk_entry_get_text(GTK_ENTRY(entry_desc));
        newcave->author=g_get_real_name();
        newcave->intermission=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(intermission_check));
        /* if the user says that he creates an intermission, it is immediately resized */
        if (newcave->intermission) {
            newcave->w=20;
            newcave->h=12;
            gd_cave_correct_visible_size(*newcave);
        }
        newcave->date=gd_get_current_date();

        select_cave_for_edit(newcave);              /* close caveset icon view, and show cave */
        caveset->caves.push_back_adopt(newcave);  /* append here, as the icon view may only be destroyed now by select_cave_for_edit */
        caveset->edited=TRUE;
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
open_caveset_cb(GtkWidget *widget, gpointer data) {
    /* destroy icon view so it does not interfere */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    g_hash_table_remove_all(cave_pixbufs);
    gd_open_caveset(NULL, *caveset);
    select_cave_for_edit(NULL);
}

static void
open_installed_caveset_cb(GtkWidget *widget, gpointer data) {
    /* destroy icon view so it does not interfere */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    g_hash_table_remove_all(cave_pixbufs);
    gd_open_caveset(gd_system_caves_dir, *caveset);
    select_cave_for_edit(NULL);
}

static void
save_caveset_as_cb(GtkWidget *widget, gpointer data) {
    CaveStored *edited;

    edited=edited_cave;
    /* destroy icon view so it does not interfere with the cave order, and the order of caves is saved */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    gd_save_caveset_as(*caveset);
    select_cave_for_edit(edited);   /* go back to edited cave or recreate icon view */
}

static void
save_caveset_cb(GtkWidget *widget, gpointer data) {
    CaveStored *edited;

    edited=edited_cave;
    /* destroy icon view so it does not interfere with the cave order, and the order of caves is saved */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    gd_save_caveset(*caveset);
    select_cave_for_edit(edited);   /* go back to edited cave or recreate icon view */
}

static void
new_caveset_cb(GtkWidget *widget, gpointer data) {
    GDate *date;
    char datestr[128];

    /* destroy icon view so it does not interfere */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);
    if (gd_discard_changes(*caveset)) {
        *caveset=CaveSet();
        g_hash_table_remove_all(cave_pixbufs);
    }
    select_cave_for_edit(NULL);

    date=g_date_new();
    g_date_set_time_t(date, time(NULL));
    g_date_strftime(datestr, sizeof(datestr), "%Y-%m-%d", date);
    caveset->date=datestr;
    g_date_free(date);

    caveset->author=g_get_real_name();

    caveset_properties(false);  /* false=do not show cancel button */
}


static void
remove_all_unknown_tags_cb(GtkWidget *widget, gpointer data) {
    /* remember which cave was edited, or null if icon view. */
    CaveStored *edited=edited_cave;
    /* destroy icon view so it does not interfere with the cave order, and the order of caves is saved */
    if (iconview_cavelist)
        gtk_widget_destroy(iconview_cavelist);

    gboolean response=gd_question_yesno(_("Do you really want to remove unknown cave tags?"), _("This operation removes all unknown tags associated with all caves. Unknown tags might come from another BDCFF-compatible game or an older version of GDash. Those cave options cannot be interpreted by GDash, and therefore if you use this caveset in this application, they are of no use."));

    if (response)
        for (unsigned n=0; n<caveset->caves.size(); ++n)
            caveset->cave(n).unknown_tags="";

    select_cave_for_edit(edited);   /* go back to edited cave or recreate icon view */
}


/* make all caves selectable */
static void
selectable_all_cb(GtkWidget *widget, gpointer data) {
    gtk_widget_destroy(iconview_cavelist);  /* to generate caveset */
    for (unsigned n=0; n<caveset->caves.size(); ++n) {
        CaveStored &cave=caveset->cave(n);

        if (!cave.selectable) {
            cave.selectable=TRUE;
            g_hash_table_remove(cave_pixbufs, &cave);
        }
    }
    select_cave_for_edit(NULL);
}

/* make all but intermissions selectable */
static void
selectable_all_but_intermissions_cb(GtkWidget *widget, gpointer data) {
    gtk_widget_destroy(iconview_cavelist);  /* to generate caveset */
    for (unsigned n=0; n<caveset->caves.size(); ++n) {
        CaveStored &cave=caveset->cave(n);
        gboolean desired=!cave.intermission;

        if (cave.selectable!=desired) {
            cave.selectable=desired;
            g_hash_table_remove(cave_pixbufs, &cave);
        }
    }
    select_cave_for_edit(NULL);
}


/* make all after intermissions selectable */
static void
selectable_all_after_intermissions_cb(GtkWidget *widget, gpointer data) {
    gboolean was_intermission=TRUE; /* treat the 'zeroth' cave as intermission, so the very first cave will be selectable */

    gtk_widget_destroy(iconview_cavelist);  /* to generate caveset */
    for (unsigned n=0; n<caveset->caves.size(); ++n) {
        CaveStored &cave=caveset->cave(n);
        gboolean desired;

        desired=!cave.intermission && was_intermission; /* selectable if this is a normal cave, and the previous one was an interm. */
        if (cave.selectable!=desired) {
            cave.selectable=desired;
            g_hash_table_remove(cave_pixbufs, &cave);
        }

        was_intermission=cave.intermission; /* remember for next iteration */
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
level_scale_changed_cb(GtkWidget *widget, gpointer data) {
    edit_level=gtk_range_get_value (GTK_RANGE (widget))-1;
    render_cave();              /* re-render cave */
}

/* new objects are created on this level - combo change updates variable */
static void
new_object_combo_changed_cb(GtkWidget *widget, gpointer data) {
    gtk_range_set_value(GTK_RANGE(level_scale), new_objects_visible_on[gtk_combo_box_get_active(GTK_COMBO_BOX(widget))].switch_to_level);
}


/* edit tool selected */
static void action_select_tool_cb(GtkWidget *widget, gpointer data) {
    action=EditTool(gtk_radio_action_get_current_value(GTK_RADIO_ACTION (widget)));

    /* first button - mainly for draw */
    gtk_label_set_text(GTK_LABEL(label_first_element), _(gd_object_description[action].first_button));
    gtk_widget_set_sensitive(element_button, gd_object_description[action].first_button!=NULL);
    gd_element_button_set_dialog_sensitive(element_button, gd_object_description[action].first_button!=NULL);
    if (gd_object_description[action].first_button) {
        char *title=g_strdup_printf(_("%s Element"), _(gd_object_description[action].first_button));
        gd_element_button_set_dialog_title(element_button, title);
        g_free(title);
    } else
        gd_element_button_set_dialog_title(element_button, _("Draw Element"));

    /* second button - mainly for fill */
    gtk_label_set_text(GTK_LABEL(label_second_element), _(gd_object_description[action].second_button));
    gtk_widget_set_sensitive(fillelement_button, gd_object_description[action].second_button!=NULL);
    gd_element_button_set_dialog_sensitive(fillelement_button, gd_object_description[action].second_button!=NULL);
    if (gd_object_description[action].second_button) {
        char *title=g_strdup_printf(_("%s Element"), _(gd_object_description[action].second_button));
        gd_element_button_set_dialog_title(fillelement_button, title);
        g_free(title);
    } else
        gd_element_button_set_dialog_title(fillelement_button, _("Fill Element"));
}

static void
toggle_game_view_cb(GtkWidget *widget, gpointer data) {
    gd_game_view=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION (widget));
}

static void
toggle_colored_objects_cb(GtkWidget *widget, gpointer data) {
    gd_colored_objects=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION (widget));
}

static void
toggle_object_list_cb(GtkWidget *widget, gpointer data) {
    gd_show_object_list=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION (widget));
    if (gd_show_object_list && edited_cave)
        gtk_widget_show(scroll_window_objects);    /* show the scroll window containing the view */
    else
        gtk_widget_hide(scroll_window_objects);
}

static void
toggle_test_label_cb(GtkWidget *widget, gpointer data) {
    gd_show_test_label=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget));
}


static void
close_editor_cb(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}


static void
remove_bad_replays_cb(GtkWidget *widget, gpointer data) {
    /* if the iconview is active, we destroy it, as it interferes with GList *caveset.
       after highscore editing, we call editcave(null) to recreate */
    gboolean has_iconview=iconview_cavelist!=NULL;
    GString *report;
    int sum;

    if (has_iconview)
        gtk_widget_destroy(iconview_cavelist);

    report=g_string_new(NULL);
    sum=0;
    for (unsigned n=0; n<caveset->caves.size(); n++) {
        CaveStored &cave=caveset->cave(n);
        int removed;

        removed=gd_cave_check_replays(cave, FALSE, TRUE, FALSE);
        sum+=removed;
        if (removed>0)
            g_string_append_printf(report, _("%s: removed %d replay(s)\n"), cave.name.c_str(), removed);
    }

    if (has_iconview)
        select_cave_for_edit(NULL);
    if (sum>0) {
        gd_infomessage(_("Some replays were removed."), report->str);
        caveset->edited=TRUE;
    }
    g_string_free(report, TRUE);
}


static void
mark_all_replays_as_working_cb(GtkWidget *widget, gpointer data) {
    /* if the iconview is active, we destroy it, as it interferes with GList *caveset. */
    gboolean has_iconview=iconview_cavelist!=NULL;
    GString *report;
    int sum;

    if (has_iconview)
        gtk_widget_destroy(iconview_cavelist);

    report=g_string_new(NULL);
    sum=0;
    for (unsigned n=0; n<caveset->caves.size(); ++n) {
        CaveStored &cave=caveset->cave(n);
        int changed;

        changed=gd_cave_check_replays(cave, FALSE, FALSE, TRUE);
        sum+=changed;
        if (changed>0)
            g_string_append_printf(report, _("%s: marked %d replay(s) as working ones\n"), cave.name.c_str(), changed);
    }

    if (has_iconview)
        select_cave_for_edit(NULL);
    if (sum>0) {
        gd_warningmessage(_("Some replay checksums were recalculated. This does not mean that those replays actually play correctly!"), report->str);
        caveset->edited=TRUE;
    }
    g_string_free(report, TRUE);
}


/* set image from gd_caveset_data title screen. */
static void setup_caveset_title_image_load_image(GtkWidget *image) {
    if (caveset->title_screen!="") {
        std::vector<Pixbuf *> title_images = get_title_animation_pixbuf(caveset->title_screen, caveset->title_screen_scroll, true, *editor_pixbuf_factory);
        if (!title_images.empty()) {
            GdkPixbuf *bigone=static_cast<GTKPixbuf *>(title_images[0])->get_gdk_pixbuf();
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), bigone);
            delete title_images[0];
        } else {
            // could not load, so clear it
            caveset->title_screen="";
            caveset->title_screen_scroll="";
            gtk_image_clear(GTK_IMAGE(image));
        }
    } else {
        gtk_image_clear(GTK_IMAGE(image));
    }
}

/* forgets caveset title image. */
static void setup_caveset_title_image_clear_cb(GtkWidget *widget, gpointer data) {
    GtkWidget *image=(GtkWidget *)data;

    caveset->title_screen="";
    caveset->title_screen_scroll="";
    setup_caveset_title_image_load_image(image);
}

/* load image from disk */
static void
setup_caveset_title_image_load_image_into_string(const char *title, GtkWidget *parent, GtkWidget *image, std::string &string, int maxwidth, int maxheight) {
    char *filename;
    GdkPixbuf *pixbuf;
    GError *error=NULL;
    gchar *buffer;
    gsize bufsize;
    gchar *base64;
    int width, height;

    filename=gd_select_image_file(title);
    if (!filename)  /* no filename -> do nothing */
        return;

    /* check image format and size */
    if (!gdk_pixbuf_get_file_info(filename, &width, &height)) {
        gd_errormessage(_("Error loading image file."), _("Cannot recognize file format."));
        return;
    }
    /* check for maximum size */
    if (height>maxheight || width>maxwidth) {
        gd_errormessage(_("The image selected is too big!"), CPrintf(_("Maximum sizes: %dx%d pixels")) % maxwidth % maxheight);
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
    g_object_unref(pixbuf); /* not needed anymore */
    if (error) {
        /* cannot load image - do nothing */
        gd_errormessage("Internal error: %s", error->message);
        g_error_free(error);
        return;
    }
    base64=g_base64_encode((guchar *) buffer, bufsize);
    g_free(buffer); /* binary data can be freed */
    string=base64;
    g_free(base64); /* copied to string so can be freed */
    setup_caveset_title_image_load_image(image);
    caveset->edited=TRUE;
}

static void
setup_caveset_title_image_load_screen_cb(GtkWidget *widget, gpointer data) {
    setup_caveset_title_image_load_image_into_string(_("Select Image File for Title Screen"), gtk_widget_get_toplevel(widget),
        (GtkWidget *)data, caveset->title_screen, GD_TITLE_SCREEN_MAX_WIDTH, GD_TITLE_SCREEN_MAX_HEIGHT);
}

static void
setup_caveset_title_image_load_tile_cb(GtkWidget *widget, gpointer data) {
    setup_caveset_title_image_load_image_into_string(_("Select Image File for Background Tile"), gtk_widget_get_toplevel(widget),
        (GtkWidget *)data, caveset->title_screen_scroll, GD_TITLE_SCROLL_MAX_WIDTH, GD_TITLE_SCROLL_MAX_HEIGHT);
}

/* load images for the caveset title screen */
static void
setup_caveset_title_image_cb(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkWidget *frame, *align, *image, *bbox;
    GtkWidget *setbutton, *settilebutton, *clearbutton;
    char *hint;

    dialog=gtk_dialog_new_with_buttons(_("Set Title Image"), GTK_WINDOW(gd_editor_window), GtkDialogFlags(0),
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

    /* an align, so the image shrinks to its size, and not the vbox determines its width. */
    align=gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 6);
    /* a frame around the image */
    frame=gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(align), frame);
    image=gtk_image_new();
    gtk_widget_set_size_request(image, GD_TITLE_SCREEN_MAX_WIDTH, GD_TITLE_SCREEN_MAX_HEIGHT);  /* max title image size */
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
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}


static void
create_lowercase_element_names() {
    /* run only once */
    static bool lowercase_names_created=false;
    if (lowercase_names_created)
        return;
    lowercase_names_created=true;
    
    bool lowercase_names=true;
    /* TRANSLATORS: in some languages (for example, german) nouns must be written capitalized.
     * In other languages, nouns can be written uncapitalized.
     * When the name of an element is put in a sentence, this has to be taken into account.
     * 
     * For example, the element name in English is "Brick wall",
     * and it is possible to write "Line of brick wall" (uppercase B -> lowercase b in the sentence).
     * In German, the same is "Ziegelmauer", and "Linie aus Ziegelmauer" (Z remains in upper case).
     * 
     * If the language you are translating to writes nouns capitalized (for example, German),
     * translate this string to "lowercase-element-names-no".
     * Otherwise translate this to "lowercase-element-values-yes".
     */
    if (gd_str_equal(gettext("lowercase-element-names-yes"), "lowercase-element-names-no"))
        lowercase_names=false;
    for (int i=0; gd_element_properties[i].element!=-1; ++i) {
        /* if it has a name, create a lowercase name (of the translated one). will be used by the editor */
        if (gd_element_properties[i].visiblename) {
            if (lowercase_names)
                /* the function allocates a new string, but it is needed as long as the app is running */
                gd_element_properties[i].lowercase_name =
                    gd_tostring_free(g_utf8_strdown(gettext(gd_element_properties[i].visiblename), -1));
            else
                /* only translate, no lowercase. */
                gd_element_properties[i].lowercase_name = gettext(gd_element_properties[i].visiblename);
        }
    }
}


static void preferences_cb(GtkWidget *widget, gpointer data) {
    GTKPixbufFactory pf;
    SettingsWindow::do_settings_dialog(gd_get_game_settings_array(), pf);
}


/* normal menu items */
static GtkActionEntry action_entries_normal[]={
    {"FileMenu", NULL, N_("_File")},
    {"EditMenu", NULL, N_("_Edit")},
    {"ViewMenu", NULL, N_("_View")},
    {"ToolsMenu", NULL, N_("_Tools")},
    {"HelpMenu", NULL, N_("_Help")},
    {"ObjectMenu", NULL, N_("_Object")},
    {"CaveMenu", NULL, N_("_Cave")},
    {"CaveSetMenu", NULL, N_("Cave_set")},
    {"Close", GTK_STOCK_CLOSE, NULL, NULL, N_("Close cave editor"), G_CALLBACK(close_editor_cb)},
    {"NewCave", GTK_STOCK_NEW, N_("New _cave"), NULL, N_("Create new cave"), G_CALLBACK(new_cave_cb)},
    {"Help", GTK_STOCK_HELP, NULL, NULL, NULL, G_CALLBACK(help_cb)},
    {"SaveFile", GTK_STOCK_SAVE, NULL, NULL, N_("Save cave set to file"), G_CALLBACK(save_caveset_cb)},
    {"SaveAsFile", GTK_STOCK_SAVE_AS, NULL, NULL, N_("Save cave set as new file"), G_CALLBACK(save_caveset_as_cb)},
    {"OpenFile", GTK_STOCK_OPEN, NULL, NULL, N_("Load cave set from file"), G_CALLBACK(open_caveset_cb)},
    {"OpenInstalledFile", GTK_STOCK_CDROM, N_("O_pen shipped"), NULL, N_("Load shipped cave set from file"), G_CALLBACK(open_installed_caveset_cb)},
    {"SelectAll", GTK_STOCK_SELECT_ALL, NULL, "<control>A", N_("Select all items"), G_CALLBACK(select_all_cb)},
    {"CaveSetProps", GTK_STOCK_PROPERTIES, N_("Cave set _properties"), NULL, N_("Set properties of cave set"), G_CALLBACK(set_caveset_properties_cb)},
    {"CaveSetTitleImage", GD_ICON_IMAGE, N_("Cave set _title image"), NULL, N_("Set caveset title image"), G_CALLBACK(setup_caveset_title_image_cb)},
    {"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK(preferences_cb)},
};

/* cave_selector menu items */
static const GtkActionEntry action_entries_cave_selector[]={
    {"CaveRoleMenu", NULL, N_("_Role in caveset")},
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
    {"SelectableMenu", NULL, N_("_Selectable caves")},
    {"AllCavesSelectable", NULL, N_("All _caves"), NULL, N_("Make all caves selectable as game start"), G_CALLBACK(selectable_all_cb)},
    {"AllButIntermissionsSelectable", NULL, N_("All _but intermissions"), NULL, N_("Make all caves but intermissions selectable as game start"), G_CALLBACK(selectable_all_but_intermissions_cb)},
    {"AllAfterIntermissionsSelectable", NULL, N_("All _after intermissions"), NULL, N_("Make all caves after intermissions selectable as game start"), G_CALLBACK(selectable_all_after_intermissions_cb)},
    {"RemoveAllUnknownTags", NULL, N_("Remove all unknown tags"), NULL, N_("Removes all unknown tags found in the BDCFF file"), G_CALLBACK(remove_all_unknown_tags_cb)},
    {"RemoveBadReplays", NULL, N_("Remove bad replays"), NULL, N_("Removes replays which won't play as they have their caves modified."), G_CALLBACK(remove_bad_replays_cb)},
    {"MarkAllReplaysAsWorking", NULL, N_("Fix replay checksums"), NULL, N_("Treats all replays with wrong checksums as working ones."), G_CALLBACK(mark_all_replays_as_working_cb)},
};

/* cave editing menu items */
static const GtkActionEntry action_entries_edit_cave[]={
    {"MapMenu", NULL, N_("_Map")},
    {"ExportAsCrLiCave", GTK_STOCK_CONVERT, N_("_Export as CrLi cave file"), NULL, NULL, G_CALLBACK(export_cavefile_cb)},
    {"CaveSelector", GTK_STOCK_INDEX, NULL, "Escape", N_("Open cave selector"), G_CALLBACK(cave_selector_cb)},
    {"NextCave", GTK_STOCK_GO_FORWARD, N_("_Next cave"), "Page_Down", N_("Next cave"), G_CALLBACK(next_cave_cb)},
    {"PreviousCave", GTK_STOCK_GO_BACK, N_("_Previous cave"), "Page_Up", N_("Previous cave"), G_CALLBACK(previous_cave_cb)},
    {"Test", GTK_STOCK_MEDIA_PLAY, N_("_Test"), "<control>T", N_("Test cave"), G_CALLBACK(play_level_cb)},
    {"CaveProperties", GTK_STOCK_PROPERTIES, N_("Ca_ve properties"), NULL, N_("Cave settings"), G_CALLBACK(cave_properties_cb)},
    {"EngineDefaults", NULL, N_("Set engine defaults")},
    {"CaveColors", GTK_STOCK_SELECT_COLOR, N_("Cave co_lors"), NULL, N_("Select cave colors"), G_CALLBACK(cave_colors_cb)},
    {"FlattenCave", NULL, N_("Convert to map"), NULL, N_("Flatten cave to a single cave map without objects"), G_CALLBACK(flatten_cave_cb)},
    {"Overview", GTK_STOCK_ZOOM_FIT, N_("O_verview"), NULL, N_("Full screen overview of cave"), G_CALLBACK(cave_overview_cb)},
    {"OverviewSimple", GTK_STOCK_ZOOM_FIT, N_("O_verview (simple)"), NULL, N_("Full screen overview of cave almost as in game"), G_CALLBACK(cave_overview_simple_cb)},
    {"AutoShrink", NULL, N_("_Auto shrink"), NULL, N_("Automatically set the visible region of the cave"), G_CALLBACK(auto_shrink_cave_cb)},
};

/* action entries which relate to a selected cave element (line, rectangle...) */
static const GtkActionEntry action_entries_edit_object[]={
    {"SendToBack", GD_ICON_TO_BOTTOM, N_("Send to _back"), "<control>End", N_("Send object to bottom of object list (draw first)"), G_CALLBACK(send_to_back_selected_cb)},
    {"BringToFront", GD_ICON_TO_TOP, N_("Bring to _front"), "<control>Home", N_("Bring object to front of object list (drawn last)"), G_CALLBACK(bring_to_front_selected_cb)},
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

/**
 * Start cave editor.
 */
static void create_cave_editor(CaveSet *cs) {
    /* toggle buttons: nonstatic as they use values from settings */
    /* also cannot make this global! */
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
            "<menuitem action='BringToFront'/>"
            "<menuitem action='SendToBack'/>"
            "<menuitem action='ShowOnThisLevel'/>"
            "<menuitem action='HideOnThisLevel'/>"
            "<menuitem action='OnlyOnThisLevel'/>"
            "<menuitem action='ShowOnAllLevels'/>"
            "<menuitem action='ObjectProperties'/>"
            "<separator/>"
            "<menuitem action='CaveProperties'/>"
        "</popup>"

        "<popup name='ObjectListPopup'>"
            "<menuitem action='Cut'/>"
            "<menuitem action='Copy'/>"
            "<menuitem action='Paste'/>"
            "<menuitem action='Delete'/>"
            "<separator/>"
            "<menuitem action='BringToFront'/>"
            "<menuitem action='SendToBack'/>"
            "<menuitem action='ShowOnThisLevel'/>"
            "<menuitem action='HideOnThisLevel'/>"
            "<menuitem action='OnlyOnThisLevel'/>"
            "<menuitem action='ShowOnAllLevels'/>"
            "<menuitem action='ObjectProperties'/>"
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

        "<menubar name='MenuBar'>"
            "<menu action='FileMenu'>"
                "<menuitem action='NewCave'/>"
                "<menuitem action='NewCaveset'/>"
                "<separator/>"
                "<menuitem action='OpenFile'/>"
                "<menuitem action='OpenInstalledFile'/>"
                "<separator/>"
                "<menuitem action='SaveFile'/>"
                "<menuitem action='SaveAsFile'/>"
                "<separator/>"
                "<menuitem action='ExportCavePack'/>"
                "<menuitem action='ExportAsCrLiCave'/>"
                "<menuitem action='SaveHTML'/>"
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
                "<menu action='ObjectMenu'>"
                    "<menuitem action='BringToFront'/>"
                    "<menuitem action='SendToBack'/>"
                    "<menuitem action='ShowOnThisLevel'/>"
                    "<menuitem action='HideOnThisLevel'/>"
                    "<menuitem action='OnlyOnThisLevel'/>"
                    "<menuitem action='ShowOnAllLevels'/>"
                    "<separator/>"
                    "<menuitem action='ObjectProperties'/>"
                "</menu>"
                "<menu action='CaveMenu'>"
                    "<menu action='CaveRoleMenu'>"
                        "<menuitem action='RenameCave'/>"
                        "<menuitem action='MakeSelectable'/>"
                        "<menuitem action='MakeUnselectable'/>"
                    "</menu>"
                    "<menuitem action='SetupRandom'/>"
                    "<menuitem action='CaveColors'/>"
                    "<menu action='MapMenu'>"
                        "<menuitem action='ShiftLeft'/>"
                        "<menuitem action='ShiftRight'/>"
                        "<menuitem action='ShiftUp'/>"
                        "<menuitem action='ShiftDown'/>"
                        "<separator/>"
                        "<menuitem action='FlattenCave'/>"
                        "<menuitem action='RemoveMap'/>"
                    "</menu>"
                    "<separator/>"
                    "<menuitem action='EngineDefaults'/>"
                    "<menuitem action='CaveProperties'/>"
                "</menu>"
                "<menu action='CaveSetMenu'>"
                    "<menuitem action='CaveSetTitleImage'/>"
                    "<menu action='SelectableMenu'>"
                        "<menuitem action='AllCavesSelectable'/>"
                        "<menuitem action='AllButIntermissionsSelectable'/>"
                        "<menuitem action='AllAfterIntermissionsSelectable'/>"
                    "</menu>"
                    "<separator/>"
                    "<menuitem action='RemoveAllUnknownTags'/>"
                    "<menuitem action='RemoveBadReplays'/>"
                    "<menuitem action='MarkAllReplaysAsWorking'/>"
                    "<separator/>"
                    "<menuitem action='CaveSetProps'/>"
                "</menu>"
                "<separator/>"
                "<menuitem action='Preferences'/>"
            "</menu>"
            "<menu action='ViewMenu'>"
                "<menuitem action='EditCave'/>"
                "<menuitem action='PreviousCave'/>"
                "<menuitem action='NextCave'/>"
                "<menuitem action='CaveSelector'/>"
                "<separator/>"
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

    if (gd_editor_window) {
        /* if exists, only show it to the user. */
        gtk_window_present(GTK_WINDOW(gd_editor_window));
        return;
    }
    
    caveset=cs;
    
    create_lowercase_element_names();

    /* hash table which stores cave pointer -> pixbufs. deleting a pixbuf calls g_object_unref. */
    cave_pixbufs=g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);

    gd_editor_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(gd_editor_window), gd_editor_window_width, gd_editor_window_height);
    g_signal_connect(G_OBJECT(gd_editor_window), "destroy", G_CALLBACK(gtk_widget_destroyed), &gd_editor_window);
    g_signal_connect(G_OBJECT(gd_editor_window), "destroy", G_CALLBACK(editor_window_destroy_event), NULL);
    g_signal_connect(G_OBJECT(gd_editor_window), "delete-event", G_CALLBACK(editor_window_delete_event), NULL);
    
    editor_pixbuf_factory=new GTKPixbufFactory(GdScalingType(gd_cell_scale_editor), gd_pal_emulation_editor);
    editor_cell_renderer=new EditorCellRenderer(*editor_pixbuf_factory, gd_theme);

    vbox=gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER (gd_editor_window), vbox);

    /* menu and toolbar */
    actions_edit_tools=gtk_action_group_new("edit_tools");
    gtk_action_group_set_translation_domain (actions_edit_tools, PACKAGE);
    gtk_action_group_add_radio_actions (actions_edit_tools, action_objects, G_N_ELEMENTS(action_objects), -1, G_CALLBACK(action_select_tool_cb), NULL);

    actions=gtk_action_group_new("edit_normal");
    gtk_action_group_set_translation_domain (actions, PACKAGE);
    gtk_action_group_add_actions (actions, action_entries_normal, G_N_ELEMENTS(action_entries_normal), NULL);

    actions_edit_object=gtk_action_group_new("edit_object");
    gtk_action_group_set_translation_domain (actions_edit_object, PACKAGE);
    gtk_action_group_add_actions (actions_edit_object, action_entries_edit_object, G_N_ELEMENTS(action_entries_edit_object), NULL);

    actions_edit_one_object=gtk_action_group_new("edit_one_object");
    gtk_action_group_set_translation_domain (actions_edit_one_object, PACKAGE);
    gtk_action_group_add_actions (actions_edit_one_object, action_entries_edit_one_object, G_N_ELEMENTS(action_entries_edit_one_object), NULL);

    actions_edit_map=gtk_action_group_new("edit_map");
    gtk_action_group_set_translation_domain (actions_edit_map, PACKAGE);
    gtk_action_group_add_actions (actions_edit_map, action_entries_edit_map, G_N_ELEMENTS(action_entries_edit_map), NULL);

    actions_edit_random=gtk_action_group_new("edit_random");
    gtk_action_group_set_translation_domain (actions_edit_random, PACKAGE);
    gtk_action_group_add_actions (actions_edit_random, action_entries_edit_random, G_N_ELEMENTS(action_entries_edit_random), NULL);

    actions_clipboard=gtk_action_group_new("clipboard");
    gtk_action_group_set_translation_domain (actions_clipboard, PACKAGE);
    gtk_action_group_add_actions (actions_clipboard, action_entries_clipboard, G_N_ELEMENTS(action_entries_clipboard), NULL);

    actions_clipboard_paste=gtk_action_group_new("clipboard_paste");
    gtk_action_group_set_translation_domain (actions_clipboard_paste, PACKAGE);
    gtk_action_group_add_actions (actions_clipboard_paste, action_entries_clipboard_paste, G_N_ELEMENTS(action_entries_clipboard_paste), NULL);

    actions_edit_undo=gtk_action_group_new("edit_undo");
    gtk_action_group_set_translation_domain (actions_edit_undo, PACKAGE);
    gtk_action_group_add_actions (actions_edit_undo, action_entries_edit_undo, G_N_ELEMENTS(action_entries_edit_undo), NULL);

    actions_edit_redo=gtk_action_group_new("edit_redo");
    gtk_action_group_set_translation_domain (actions_edit_redo, PACKAGE);
    gtk_action_group_add_actions (actions_edit_redo, action_entries_edit_redo, G_N_ELEMENTS(action_entries_edit_redo), NULL);

    actions_edit_cave=gtk_action_group_new("edit_cave");
    gtk_action_group_set_translation_domain (actions_edit_cave, PACKAGE);
    gtk_action_group_add_actions (actions_edit_cave, action_entries_edit_cave, G_N_ELEMENTS(action_entries_edit_cave), NULL);
    g_object_set (gtk_action_group_get_action (actions_edit_cave, "Test"), "is_important", TRUE, NULL);
    g_object_set (gtk_action_group_get_action (actions_edit_cave, "CaveSelector"), "is_important", TRUE, NULL);

    actions_edit_caveset=gtk_action_group_new("edit_caveset");
    gtk_action_group_set_translation_domain (actions_edit_caveset, PACKAGE);
    gtk_action_group_add_actions (actions_edit_caveset, action_entries_edit_caveset, G_N_ELEMENTS(action_entries_edit_caveset), NULL);

    actions_cave_selector=gtk_action_group_new("cave_selector");
    gtk_action_group_set_translation_domain (actions_cave_selector, PACKAGE);
    gtk_action_group_add_actions (actions_cave_selector, action_entries_cave_selector, G_N_ELEMENTS(action_entries_cave_selector), NULL);

    actions_toggle=gtk_action_group_new("toggles");
    gtk_action_group_set_translation_domain (actions_toggle, PACKAGE);
    gtk_action_group_add_toggle_actions (actions_toggle, action_entries_toggle, G_N_ELEMENTS(action_entries_toggle), NULL);

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
    menu=gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM (gtk_ui_manager_get_widget (ui, "/MenuBar/EditMenu/CaveMenu/EngineDefaults")), menu);
    for (int i=0; i<GD_ENGINE_MAX; i++) {
        GtkWidget *menuitem=gtk_menu_item_new_with_label(visible_name(GdEngineEnum(i)));

        gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuitem);
        gtk_widget_show (menuitem);
        g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(set_engine_default_cb), GINT_TO_POINTER (i));
    }

    /* TOOLBARS */
    toolbars=gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), toolbars, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbars), gtk_ui_manager_get_widget (ui, "/ToolBar"), FALSE, FALSE, 0);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR (gtk_ui_manager_get_widget (ui, "/ToolBar")), TRUE);
    gtk_toolbar_set_style(GTK_TOOLBAR(gtk_ui_manager_get_widget (ui, "/ToolBar")), GTK_TOOLBAR_BOTH_HORIZ);
    gtk_window_add_accel_group(GTK_WINDOW(gd_editor_window), gtk_ui_manager_get_accel_group(ui));

    /* get popups and attach them to the window, so they are not destroyed (the window holds the ref) */
    drawing_area_popup=gtk_ui_manager_get_widget (ui, "/DrawingAreaPopup");
    gtk_menu_attach_to_widget(GTK_MENU(drawing_area_popup), gd_editor_window, NULL);
    object_list_popup=gtk_ui_manager_get_widget (ui, "/ObjectListPopup");
    gtk_menu_attach_to_widget(GTK_MENU(object_list_popup), gd_editor_window, NULL);
    caveset_popup=gtk_ui_manager_get_widget (ui, "/CavesetPopup");
    gtk_menu_attach_to_widget(GTK_MENU(caveset_popup), gd_editor_window, NULL);

    g_object_unref(actions);
    g_object_unref(ui);

    /* combo boxes under toolbar */
    hbox_combo=gtk_hbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(toolbars), hbox_combo, FALSE, FALSE, 0);

    /* currently shown level - gtkscale */
    level_scale=gtk_hscale_new_with_range (1.0, 5.0, 1.0);
    gtk_scale_set_digits (GTK_SCALE(level_scale), 0);
    gtk_scale_set_value_pos (GTK_SCALE(level_scale), GTK_POS_LEFT);
    g_signal_connect(G_OBJECT(level_scale), "value-changed", G_CALLBACK(level_scale_changed_cb), NULL);
    gtk_box_pack_end_defaults(GTK_BOX(hbox_combo), level_scale);
    gtk_box_pack_end(GTK_BOX(hbox_combo), gtk_label_new(_("Level shown:")), FALSE, FALSE, 0);

     /* "new object will be placed on" - combo */
    new_object_level_combo=gtk_combo_box_new_text();
    for (unsigned i=0; i<G_N_ELEMENTS(new_objects_visible_on); ++i)
        gtk_combo_box_append_text(GTK_COMBO_BOX(new_object_level_combo), _(new_objects_visible_on[i].text));
    g_signal_connect(G_OBJECT(new_object_level_combo), "changed", G_CALLBACK(new_object_combo_changed_cb), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(new_object_level_combo), 0);
    gtk_box_pack_end_defaults(GTK_BOX(hbox_combo), new_object_level_combo);
    gtk_box_pack_end(GTK_BOX(hbox_combo), gtk_label_new(_("Draw on:")), FALSE, FALSE, 0);

     /* draw element */
    gtk_box_pack_start(GTK_BOX(hbox_combo), label_first_element=gtk_label_new(NULL), FALSE, FALSE, 0);
    element_button=gd_element_button_new(O_DIRT, TRUE, NULL);   /* combo box of object, default element dirt (not really important what it is) */
    gtk_widget_set_tooltip_text(element_button, _("Element used to draw points, lines, and rectangles. You can use middle-click to pick one from the cave."));
    gtk_box_pack_start_defaults(GTK_BOX(hbox_combo), element_button);

    gtk_box_pack_start(GTK_BOX(hbox_combo), label_second_element=gtk_label_new(NULL), FALSE, FALSE, 0);
    fillelement_button=gd_element_button_new(O_SPACE, TRUE, NULL);  /* combo box, default element space (not really important what it is) */
    gtk_widget_set_tooltip_text (fillelement_button, _("Element used to fill rectangles, and second element of joins. You can use Ctrl + middle-click to pick one from the cave."));
    gtk_box_pack_start_defaults(GTK_BOX(hbox_combo), fillelement_button);

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

    object_list=gtk_list_store_new(NUM_EDITOR_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
    object_list_tree_view=gtk_tree_view_new_with_model(GTK_TREE_MODEL (object_list));
    g_object_unref(object_list);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW (object_list_tree_view))), "changed", G_CALLBACK(object_list_selection_changed_signal), NULL);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(object_list_tree_view)), GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(object_list_tree_view), TRUE);
    gtk_container_add(GTK_CONTAINER(scroll_window_objects), object_list_tree_view);
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
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "stock-id", LEVELS_PIXBUF_COLUMN, NULL);
    renderer=gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "stock-id", TYPE_PIXBUF_COLUMN, NULL);
    renderer=gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", ELEMENT_PIXBUF_COLUMN, NULL);
    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", TEXT_COLUMN, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (object_list_tree_view), column);

    /* something like a statusbar, maybe that would be nicer */
    hbox=gtk_hbox_new(FALSE, 6);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    label_coordinate=gtk_label_new("[x:   y:   ]");
    gtk_box_pack_start(GTK_BOX(hbox), label_coordinate, FALSE, FALSE, 0);
    label_object=gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), label_object, FALSE, FALSE, 0);

    edit_level=0;       /* view: level 1 */
    /* here we force selection and update, by calling the function twice with different args */
    select_tool(TOOL_LINE);
    select_tool(TOOL_MOVE);
    timeout_id = g_timeout_add(40, drawing_area_draw_timeout, NULL);

    gtk_widget_show_all(gd_editor_window);
    gtk_window_present(GTK_WINDOW(gd_editor_window));
    /* to remember size; only attach signal after showing */
    g_signal_connect(G_OBJECT(gd_editor_window), "configure-event", G_CALLBACK(editor_window_configure_event), NULL);

    select_cave_for_edit(NULL);
}


void gd_cave_editor_run(CaveSet *caveset) {
    create_cave_editor(caveset);
    gtk_main();
    gtk_widget_destroy(gd_editor_window);
    /* process all pending events before returning,
     * because the widget destroying created many. without processing them,
     * the editor window would not disappear! */
    while (gtk_events_pending())
        gtk_main_iteration();
}
