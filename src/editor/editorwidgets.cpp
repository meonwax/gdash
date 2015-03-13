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
#include "cave/helper/colors.hpp"
#include "cave/cavetypes.hpp"
#include "editor/editor.hpp"
#include "gtk/gtkui.hpp"
#include "cave/elementproperties.hpp"
#include "editor/editorcellrenderer.hpp"


/*
 * A COMBO BOX with c64 colors.
 * use color_combo_new for creating and color_combo_get_color for getting color in gdash format.
 *
 */
/* this data field always stores the previously selected color. */
/* it is needed when a select atari or select rgb dialog is escaped, and we must set the original color. */
/* the combo box itself cannot store it, as is is already set to the select atari... or select rgb... line. */
#define GDASH_COLOR "gdash-color"
#define GDASH_COLOR_INDEX "gdash-color-index"

static GtkTreePath *selected_color_path=NULL;

enum {
    COL_COLOR_ACTION,    /* some lines selected will trigger showing a dialog */
    COL_COLOR_NAME,        /* name of color (eg. c64 black), or action (select rgb color) */
    COL_COLOR_PIXBUF,    /* pixbuf to be shown (used for c64 colors, and currently selected color) */
    COL_COLOR_C64_INDEX,    /* used for c64 colors */
    COL_COLOR_MAX
};

typedef enum {
    COLOR_ACTION_NONE,
    COLOR_ACTION_SELECT_C64,
    COLOR_ACTION_SELECT_ATARI,
    COLOR_ACTION_SELECT_DTV,
    COLOR_ACTION_SELECT_RGB,
} ColorAction;

/*
 * creates a small pixbuf with the specified color
 */
static GdkPixbuf *
color_combo_pixbuf_for_gd_color(const GdColor &col) {
    int x, y;
    guint32 pixel;
    GdkPixbuf *pixbuf;

    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &x, &y);

    pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, x, y);
    pixel=(col.get_r()<<24) + (col.get_g()<<16) + (col.get_b()<<8);
    gdk_pixbuf_fill(pixbuf, pixel);

    return pixbuf;
}

/* set to a color - first check if that is a c64 color.
    if it is, simply ump to that item. if not, create
    a special item and select that one. */
void
gd_color_combo_set(GtkComboBox *combo, const GdColor &color) {
    GdColor *pcolor=static_cast<GdColor *>(g_object_get_data(G_OBJECT(combo), GDASH_COLOR));

    *pcolor=color;    /* set its own object to be a copy of the requested color */

    if (color.is_c64()) {
        GtkTreeIter iter;

        char *path=g_strdup_printf("0:%d", color.get_c64_index());
        gtk_tree_model_get_iter_from_string(gtk_combo_box_get_model(combo), &iter, path);
        g_free(path);
        gtk_combo_box_set_active_iter(combo, &iter);
    } else {
        GtkTreeModel *model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
        GtkTreeIter iter;

        gtk_tree_model_get_iter(model, &iter, selected_color_path);

        GdkPixbuf *pixbuf=color_combo_pixbuf_for_gd_color(color);
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COL_COLOR_PIXBUF, pixbuf, COL_COLOR_NAME, visible_name(color).c_str(), -1);
        g_object_unref(pixbuf);    /* now the tree store owns its own reference */

        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
    }
}

static gboolean
color_combo_drawing_area_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    GtkDialog *dialog=GTK_DIALOG(data);

    gtk_dialog_response(dialog, GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), GDASH_COLOR_INDEX)));

    return TRUE;
}


static void
color_combo_changed(GtkWidget *combo, gpointer data) {
    GdColor *pcolor=static_cast<GdColor *>(g_object_get_data(G_OBJECT(combo), GDASH_COLOR));
    GtkTreeModel *model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    GtkTreeIter iter;
    ColorAction action;

    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, COL_COLOR_ACTION, &action, -1);
    switch (action) {
        case COLOR_ACTION_SELECT_RGB: {
            GtkWidget *dialog;
            GtkColorSelection *colorsel;
            gint response;
            GdkColor gc;
            GdColor prevcol;

            prevcol=*pcolor;    /* remember previous setting */

            dialog=gtk_color_selection_dialog_new(_("Select Color"));
            gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_toplevel(combo)));
            colorsel=GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dialog)->colorsel);
            gc.red=prevcol.get_r()*257;         // yes, 257: 255*257=65535
            gc.green=prevcol.get_g()*257;
            gc.blue=prevcol.get_b()*257;
            gtk_color_selection_set_previous_color(colorsel, &gc);
            gtk_color_selection_set_current_color(colorsel, &gc);
            gtk_color_selection_set_has_palette(colorsel, TRUE);

            gtk_window_present_with_time(GTK_WINDOW(dialog), gtk_get_current_event_time());
            response=gtk_dialog_run(GTK_DIALOG(dialog));

            if (response==GTK_RESPONSE_OK) {
                GdkColor gc;

                gtk_color_selection_get_current_color(colorsel, &gc);
                gd_color_combo_set(GTK_COMBO_BOX(combo), GdColor::from_rgb(gc.red>>8, gc.green>>8, gc.blue>>8));
            } else {
                /* if not accepted, return button to original state */
                gd_color_combo_set(GTK_COMBO_BOX(combo), prevcol);
            }

            gtk_widget_destroy(dialog);
        }
        break;

        case COLOR_ACTION_SELECT_ATARI:
        case COLOR_ACTION_SELECT_DTV: {
            GtkWidget *dialog, *table, *frame;
            GdColor prevcol;
            int i;
            int result;
            GdColor(*colorfunc)(unsigned int i);
            const char *title;

            switch (action) {
                case COLOR_ACTION_SELECT_ATARI:
                    title=_("Select Atari Color");
                    colorfunc=&GdColor::from_atari;
                    break;
                case COLOR_ACTION_SELECT_DTV:
                    title=_("Select C64 DTV Color");
                    colorfunc=&GdColor::from_c64dtv;
                    break;
                default:
                    g_assert_not_reached();
            }

            dialog=gtk_dialog_new_with_buttons(title, GTK_WINDOW(gtk_widget_get_toplevel(combo)), GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
            table=gtk_table_new(16, 16, TRUE);
            for (i=0; i<256; i++) {
                GtkWidget *da;
                GdkColor color;
                GdColor c;

                da=gtk_drawing_area_new();
                c=colorfunc(i);
                g_object_set_data(G_OBJECT(da), GDASH_COLOR_INDEX, GUINT_TO_POINTER(i));    /* attach the color index as data */
                gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK);
                gtk_widget_set_tooltip_text(da, visible_name(c).c_str());
                g_signal_connect(G_OBJECT(da), "button_press_event", G_CALLBACK(color_combo_drawing_area_button_press_event), dialog);    /* mouse click */
                color.red=c.get_r()*256;    /* 256 as gdk expect 16-bit/component */
                color.green=c.get_g()*256;
                color.blue=c.get_b()*256;
                gtk_widget_modify_bg(da, GTK_STATE_NORMAL, &color);
                gtk_widget_set_size_request(da, 16, 16);
                gtk_table_attach_defaults(GTK_TABLE(table), da, i%16, i%16+1, i/16, i/16+1);
            }
            frame=gtk_frame_new(NULL);
            gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
            gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
            gtk_container_add(GTK_CONTAINER(frame), table);
            gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame);
            gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);

            prevcol=*pcolor;
            result=gtk_dialog_run(GTK_DIALOG(dialog));
            if (result>=0)
                gd_color_combo_set(GTK_COMBO_BOX(combo), colorfunc(result));
            else
                /* if not accepted, return button to original state */
                gd_color_combo_set(GTK_COMBO_BOX(combo), prevcol);

            gtk_widget_destroy(dialog);
        }
        break;

        case COLOR_ACTION_SELECT_C64: {
            int i;

            gtk_tree_model_get(model, &iter, COL_COLOR_C64_INDEX, &i, -1);
            *pcolor=GdColor::from_c64(i);    /* set the stored color to the c64 color[i] */
        }
        break;

        case COLOR_ACTION_NONE:
            /* do nothing - this is the special color, eg. rgb. for that, we already updated pcolor and the like */
            break;
    }
}


/* we use a non-visible, non-selectable row for the currently selected color (unless it is a c64 color). */
static gboolean
color_combo_is_separator(GtkTreeModel *model, GtkTreeIter  *iter, gpointer data) {
    GtkTreePath *path;
    gboolean result;

    path=gtk_tree_model_get_path(model, iter);
    result=!gtk_tree_path_compare(path, selected_color_path);
    gtk_tree_path_free(path);

    return result;
}

static void color_combo_set_sensitive(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data) {
    g_object_set(cell, "sensitive", !gtk_tree_model_iter_has_child(tree_model, iter), NULL);
}

static void color_combo_destroyed(GtkWidget *combo, gpointer data) {
    GdColor *pcolor=static_cast<GdColor *>(g_object_get_data(G_OBJECT(combo), GDASH_COLOR));

    delete pcolor;
}

/* combo box creator. */
GtkWidget *gd_color_combo_new(const GdColor &initial) {
    /* this is the base of the cave editor element combo box.
        categories are autodetected by their integer values being >O_MAX */
    GtkTreeStore *store;
    GtkWidget *combo;
    GtkCellRenderer *renderer;
    GtkTreeIter iter, parent;

    /* tree store for colors. every combo has its own, as the custom color can be different. */
    store=gtk_tree_store_new(COL_COLOR_MAX, G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_INT);

    /* add 16 c64 colors */
    gtk_tree_store_append(store, &parent, NULL);
    gtk_tree_store_set(store, &parent, COL_COLOR_NAME, _("C64 Colors"), COL_COLOR_PIXBUF, NULL, -1);
    for (int i=0; i<16; i++) {
        GdkPixbuf *pixbuf;

        pixbuf=color_combo_pixbuf_for_gd_color(GdColor::from_c64(i));
        gtk_tree_store_append(store, &iter, &parent);
        gtk_tree_store_set(store, &iter, COL_COLOR_ACTION, COLOR_ACTION_SELECT_C64, COL_COLOR_NAME, _(visible_name(GdColor::from_c64(i)).c_str()), COL_COLOR_PIXBUF, pixbuf, COL_COLOR_C64_INDEX, i, -1);
        g_object_unref(pixbuf);
    }
    gtk_tree_store_append(store, &iter, NULL);
    if (!selected_color_path)
        selected_color_path=gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, COL_COLOR_ACTION, COLOR_ACTION_SELECT_ATARI, COL_COLOR_NAME, _("Atari color..."), COL_COLOR_PIXBUF, NULL, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, COL_COLOR_ACTION, COLOR_ACTION_SELECT_DTV, COL_COLOR_NAME, _("C64DTV color..."), COL_COLOR_PIXBUF, NULL, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, COL_COLOR_ACTION, COLOR_ACTION_SELECT_RGB, COL_COLOR_NAME, _("RGB color..."), COL_COLOR_PIXBUF, NULL, -1);

    combo=gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    /* first column, object image */
    renderer=gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", COL_COLOR_PIXBUF, NULL);
    gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo), renderer, color_combo_set_sensitive, NULL, NULL);
    /* second column, object name */
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
    gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo), renderer, color_combo_set_sensitive, NULL, NULL);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", COL_COLOR_NAME, NULL);
    gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo), color_combo_is_separator, NULL, NULL);

    /* also a GdColor object is allocated for each combo. it will store its current value. */
    /* it will be deleted by the destroy signal. */
    GdColor *pcolor=new GdColor;
    g_object_set_data(G_OBJECT(combo), GDASH_COLOR, pcolor);

    gd_color_combo_set(GTK_COMBO_BOX(combo), initial);
    g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(color_combo_changed), NULL);
    g_signal_connect(G_OBJECT(combo), "destroy", G_CALLBACK(color_combo_destroyed), NULL);

    return combo;
}

const GdColor &gd_color_combo_get_color(GtkWidget *widget) {
    GdColor *pcolor=static_cast<GdColor *>(g_object_get_data(G_OBJECT(widget), GDASH_COLOR));
    return *pcolor;
}

#undef GDASH_COLOR
#undef GDASH_COLOR_INDEX


#define GDASH_CELL "gdash-cell"
#define GDASH_ELEMENT "gdash-element"
#define GDASH_HOVER "gdash-hover"
#define GDASH_BUTTON "gdash-button"

#define GDASH_DIALOG "gdash-dialog"
#define GDASH_WINDOW_TITLE "gdash-window-title"
#define GDASH_DIALOG_VBOX "gdash-dialog-vbox"

#define GDASH_IMAGE "gdash-image"
#define GDASH_LABEL "gdash-label"

static int element_button_animcycle=0;

/* this draws one element in the element selector box. */
static gboolean element_button_drawing_area_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    if (!widget->window)
        return FALSE;

    GdkDrawable *cell=(GdkDrawable *) g_object_get_data(G_OBJECT(widget), GDASH_CELL);
    if (cell)
        gdk_draw_drawable(widget->window, widget->style->black_gc, cell, 0, 0, 2, 2, -1, -1);
    return TRUE;
}

/* this is called when entering and exiting a drawing area with the mouse. */
/* so they can be colored when the mouse is over. */
static gboolean element_button_drawing_area_crossing_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
    g_object_set_data(G_OBJECT(widget), GDASH_HOVER, GINT_TO_POINTER(event->type==GDK_ENTER_NOTIFY));
    return FALSE;
}

static gboolean element_button_redraw_timeout(gpointer data) {
    GList *areas=(GList *)data;

    element_button_animcycle=(element_button_animcycle+1)%8;

    for (GList *iter=areas; iter!=NULL; iter=iter->next) {
        GtkWidget *da=(GtkWidget *)iter->data;
        GdElementEnum element;
        int draw;
        gboolean hover;

        /* which element is drawn? */
        element=(GdElementEnum)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(da), GDASH_ELEMENT));
        hover=(gboolean)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(da), GDASH_HOVER));
        /* get pixbuf index */
        draw=gd_element_properties[element].image;
        if (draw<0)
            draw=-draw + element_button_animcycle;
        if (hover)
            draw+=NUM_OF_CELLS;
        /* set cell and queue draw if different from previous */
        /* at first start, previous is null, so always different. */
        if (g_object_get_data(G_OBJECT(da), GDASH_CELL)!=editor_cell_renderer->cell_gdk_drawable(draw)) {
            g_object_set_data(G_OBJECT(da), GDASH_CELL, editor_cell_renderer->cell_gdk_drawable(draw));
            gtk_widget_queue_draw(da);
        }
    }
    return TRUE;
}


void gd_element_button_set(GtkWidget *button, GdElementEnum element) {
    GtkImage *image=GTK_IMAGE(g_object_get_data(G_OBJECT(button), GDASH_IMAGE));
    gtk_image_set_from_pixbuf(image, editor_cell_renderer->combo_pixbuf(element));

    GtkLabel *label=GTK_LABEL(g_object_get_data(G_OBJECT(button), GDASH_LABEL));
    gtk_label_set_text(label, _(visible_name(element)));

    g_object_set_data(G_OBJECT(button), GDASH_ELEMENT, GINT_TO_POINTER(element));
}

static void element_button_da_clicked(GtkWidget *da, GdkEventButton *event, gpointer data) {
    GtkDialog *dialog=GTK_DIALOG(data);

    /* set the corresponding button to the element selected */
    GdElementEnum element=(GdElementEnum)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(da), GDASH_ELEMENT));
    GtkWidget *button=GTK_WIDGET(g_object_get_data(G_OBJECT(da), GDASH_BUTTON));
    gd_element_button_set(button, element);

    /* if this is a modal window, then it is a does-not-stay-open element box. */
    /* so we issue a dialog response. */
    if (gtk_window_get_modal(GTK_WINDOW(dialog)))
        gtk_dialog_response(dialog, element);
    else {
        /* if not modal, it is a stay-open element box. */
        /* close if left mouse button; stay open for others. */
        if (event->button==1)
            gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

static void element_button_dialog_destroyed_free_list(GtkWidget *dialog, gpointer data) {
    GList *areas=(GList *) data;

    g_source_remove_by_user_data(areas);
    g_list_free(areas);
}

static void element_button_dialog_destroyed_null_pointer(GtkWidget *dialog, gpointer data) {
    g_object_set_data(G_OBJECT(data), GDASH_DIALOG, NULL);
    g_object_set_data(G_OBJECT(data), GDASH_DIALOG_VBOX, NULL);
}

static void element_button_dialog_close_button_clicked(GtkWidget *button, gpointer data) {
    /* data is the dialog */
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void element_button_clicked_func(GtkWidget *button, gboolean stay_open) {
    static GdElementEnum const elements[]= {
        /* normal */
        O_SPACE, O_DIRT, O_DIAMOND, O_STONE, O_MEGA_STONE, O_FLYING_DIAMOND, O_FLYING_STONE, O_NUT,
        O_BRICK, O_FALLING_WALL, O_BRICK_EATABLE, O_BRICK_NON_SLOPED, O_SPACE, O_STEEL, O_STEEL_EATABLE, O_STEEL_EXPLODABLE,

        O_INBOX, O_PRE_OUTBOX, O_PRE_INVIS_OUTBOX, O_PLAYER_GLUED, O_VOODOO, O_SPACE, O_SPACE, O_SKELETON,
        O_WALLED_KEY_1, O_WALLED_KEY_2, O_WALLED_KEY_3, O_WALLED_DIAMOND, O_STEEL_SLOPED_UP_RIGHT, O_STEEL_SLOPED_UP_LEFT, O_STEEL_SLOPED_DOWN_LEFT, O_STEEL_SLOPED_DOWN_RIGHT,

        O_AMOEBA, O_AMOEBA_2, O_SLIME, O_ACID, O_MAGIC_WALL, O_WATER, O_LAVA, O_REPLICATOR,
        O_KEY_1, O_KEY_2, O_KEY_3, O_DIAMOND_KEY, O_BRICK_SLOPED_DOWN_RIGHT, O_BRICK_SLOPED_DOWN_LEFT, O_BRICK_SLOPED_UP_LEFT, O_BRICK_SLOPED_UP_RIGHT,

        O_BOMB, O_CLOCK, O_POT, O_BOX, O_SWEET, O_PNEUMATIC_HAMMER, O_NITRO_PACK, O_ROCKET_LAUNCHER,
        O_DOOR_1, O_DOOR_2, O_DOOR_3, O_TRAPPED_DIAMOND, O_DIRT_SLOPED_UP_RIGHT, O_DIRT_SLOPED_UP_LEFT, O_DIRT_SLOPED_DOWN_LEFT, O_DIRT_SLOPED_DOWN_RIGHT,

        O_GRAVITY_SWITCH, O_CREATURE_SWITCH, O_BITER_SWITCH, O_EXPANDING_WALL_SWITCH, O_REPLICATOR_SWITCH, O_TELEPORTER, O_CONVEYOR_SWITCH, O_CONVEYOR_DIR_SWITCH,
        O_H_EXPANDING_WALL, O_V_EXPANDING_WALL, O_EXPANDING_WALL, O_SPACE, O_DIRT2, O_DIRT_BALL, O_DIRT_LOOSE, O_NONE,

        O_CONVEYOR_LEFT, O_CONVEYOR_RIGHT, O_SPACE, O_SPACE, O_SPACE, O_DIRT_GLUED, O_DIAMOND_GLUED, O_STONE_GLUED,
        O_H_EXPANDING_STEEL_WALL, O_V_EXPANDING_STEEL_WALL, O_EXPANDING_STEEL_WALL, O_BLADDER_SPENDER, O_BLADDER, O_GHOST, O_WAITING_STONE, O_CHASING_STONE,

        O_SPACE, O_FIREFLY_2, O_ALT_FIREFLY_2, O_SPACE, O_SPACE, O_BUTTER_2, O_ALT_BUTTER_2, O_SPACE, O_SPACE, O_STONEFLY_2, O_SPACE, O_COW_2, O_BITER_1, O_SPACE, O_SPACE, O_DRAGONFLY_2,
        O_FIREFLY_1, O_FIREFLY_3, O_ALT_FIREFLY_1, O_ALT_FIREFLY_3, O_BUTTER_1, O_BUTTER_3, O_ALT_BUTTER_1, O_ALT_BUTTER_3, O_STONEFLY_1, O_STONEFLY_3, O_COW_1, O_COW_3, O_BITER_4, O_BITER_2, O_DRAGONFLY_1, O_DRAGONFLY_3,
        O_SPACE, O_FIREFLY_4, O_ALT_FIREFLY_4, O_SPACE, O_SPACE, O_BUTTER_4, O_ALT_BUTTER_4, O_SPACE, O_SPACE, O_STONEFLY_4, O_SPACE, O_COW_4, O_BITER_3, O_SPACE, O_SPACE, O_DRAGONFLY_4,

        /* for effects */
        O_DIAMOND_F, O_STONE_F, O_MEGA_STONE_F, O_FLYING_DIAMOND_F, O_FLYING_STONE_F, O_FALLING_WALL_F, O_NITRO_PACK_F, O_NUT_F, O_PRE_PL_1, O_PRE_PL_2, O_PRE_PL_3, O_PLAYER, O_PLAYER_BOMB, O_PLAYER_STIRRING, O_OUTBOX, O_INVIS_OUTBOX,

        O_BLADDER_1, O_BLADDER_2, O_BLADDER_3, O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7, O_BLADDER_8, O_SPACE,
        O_COW_ENCLOSED_1, O_COW_ENCLOSED_2, O_COW_ENCLOSED_3, O_COW_ENCLOSED_4, O_COW_ENCLOSED_5, O_COW_ENCLOSED_6, O_COW_ENCLOSED_7,

        O_WATER_1, O_WATER_2, O_WATER_3, O_WATER_4, O_WATER_5, O_WATER_6, O_WATER_7, O_WATER_8,
        O_WATER_9, O_WATER_10, O_WATER_11, O_WATER_12, O_WATER_13, O_WATER_14, O_WATER_15, O_WATER_16,

        O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3, O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
        O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4, O_NUT_CRACK_1, O_NUT_CRACK_2, O_NUT_CRACK_3, O_NUT_CRACK_4, O_UNKNOWN,

        O_EXPLODE_1, O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5, O_TIME_PENALTY,
        O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4, O_PRE_DIA_5, O_NITRO_PACK_EXPLODE, O_NITRO_EXPL_1, O_NITRO_EXPL_2, O_NITRO_EXPL_3, O_NITRO_EXPL_4,
        O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4, O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
        O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4, O_GHOST_EXPL_1, O_GHOST_EXPL_2, O_GHOST_EXPL_3, O_GHOST_EXPL_4,
        
        O_ROCKET_1, O_ROCKET_2, O_ROCKET_3, O_ROCKET_4, O_PLAYER_ROCKET_LAUNCHER, O_SPACE, O_SPACE, O_SPACE,
        O_SPACE, O_SPACE, O_SPACE, O_SPACE, O_SPACE, O_SPACE, O_SPACE, O_SPACE, 
    };

    int cols=16;
    GList *areas=NULL;

    /* if the dialog is already open, only show it. */
    GtkWidget *dialog=(GtkWidget *)(g_object_get_data(G_OBJECT(button), GDASH_DIALOG));
    if (dialog) {
        gtk_window_present(GTK_WINDOW(dialog));

        return;
    }

    /* elements dialog with no buttons; clicking on an element will do the trick. */
    dialog=gtk_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_toplevel(button)));
    gtk_window_set_title(GTK_WINDOW(dialog), (char *) g_object_get_data(G_OBJECT(button), GDASH_WINDOW_TITLE));
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    /* associate the dialog with the button, so we know that it is open */
    g_object_set_data(G_OBJECT(button), GDASH_DIALOG, dialog);

    GtkWidget *vbox=gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox);
    g_object_set_data(G_OBJECT(button), GDASH_DIALOG_VBOX, vbox);

    GtkWidget *align=gtk_alignment_new(0, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(align), gtk_label_new(_("Normal elements")));
    gtk_box_pack_start_defaults(GTK_BOX(vbox), align);

    GtkWidget *table=gtk_table_new(0, 0, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 6);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), table);

    GtkWidget *expander=gtk_expander_new(_("For effects"));
    GtkWidget *table2=gtk_table_new(0, 0, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(table2), 6);
    gtk_container_add(GTK_CONTAINER(expander), table2);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), expander);

    /* color for background around elements. */
    /* gdkcolors are 16bit, we should *256, but instead *200 so a bit darker. */
    GdkColor c;
    c.red=editor_cell_renderer->background_color().get_r()*200;
    c.green=editor_cell_renderer->background_color().get_g()*200;
    c.blue=editor_cell_renderer->background_color().get_b()*200;
    /* create drawing areas */
    int pcs=editor_cell_renderer->get_cell_size();
    int into_second=0;
    for (unsigned i=0; i<G_N_ELEMENTS(elements); i++) {
        GtkWidget *da=gtk_drawing_area_new();
        gtk_widget_modify_bg(da, GTK_STATE_NORMAL, &c);
        areas=g_list_prepend(areas, da);    /* put in list for animation timeout, that one will request redraw on them */
        gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK|GDK_LEAVE_NOTIFY_MASK|GDK_ENTER_NOTIFY_MASK);
        g_object_set_data(G_OBJECT(da), GDASH_ELEMENT, GINT_TO_POINTER(elements[i]));
        g_object_set_data(G_OBJECT(da), GDASH_BUTTON, button);    /* button to update on click */
        gtk_widget_set_size_request(da, pcs+4, pcs+4);    /* 2px border around them */
        gtk_widget_set_tooltip_text(da, _(visible_name(elements[i])));
        g_signal_connect(G_OBJECT(da), "expose-event", G_CALLBACK(element_button_drawing_area_expose_event), GINT_TO_POINTER(elements[i]));
        g_signal_connect(G_OBJECT(da), "leave-notify-event", G_CALLBACK(element_button_drawing_area_crossing_event), NULL);
        g_signal_connect(G_OBJECT(da), "enter-notify-event", G_CALLBACK(element_button_drawing_area_crossing_event), NULL);
        g_signal_connect(G_OBJECT(da), "button-press-event", G_CALLBACK(element_button_da_clicked), dialog);
        if (elements[i]==O_DIAMOND_F)
            /* the dirt2 the first element to be put in the effect list; from that one, always use table2 */
            into_second=i;
        if (!into_second)
            gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(da), i%cols, i%cols+1, i/cols, i/cols+1);
        else {
            int j=i-into_second;

            gtk_table_attach_defaults(GTK_TABLE(table2), GTK_WIDGET(da), j%cols, j%cols+1, j/cols, j/cols+1);
        }

    }

    /* add a timeout which animates the drawing areas */
    g_timeout_add(40, element_button_redraw_timeout, areas);
    /* if the dialog is destroyed, we must free the list which contains the drawing areas */
    g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(element_button_dialog_destroyed_free_list), areas);
    /* also remember that the button no longer has its own dialog open */
    g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(element_button_dialog_destroyed_null_pointer), button);

    if (!stay_open) {
        gtk_widget_show_all(dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else {
        /* if it is a stay-open element box, add a button which (also) closes it */
        GtkWidget *close_button;

        close_button=gtk_button_new_from_stock(GTK_STOCK_CLOSE);
        gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog)->action_area), close_button, FALSE, FALSE, 0);
        g_signal_connect(G_OBJECT(close_button), "clicked", (GCallback) element_button_dialog_close_button_clicked, dialog);

        gtk_widget_show_all(dialog);
        gtk_window_present_with_time(GTK_WINDOW(dialog), gtk_get_current_event_time());
    }
}

GdElementEnum gd_element_button_get(GtkWidget *button) {
    gpointer element=g_object_get_data(G_OBJECT(button), GDASH_ELEMENT);
    return (GdElementEnum)GPOINTER_TO_INT(element);
}

/* the pixbufs might be changed during the lifetime of an element button. *
 * for example, when colors of a cave are redefined. so this is made
 * global and can be called. */
void gd_element_button_update_pixbuf(GtkWidget *button) {
    /* set the same as it already shows, but pixbuf update will be triggered by this */
    gd_element_button_set(button, gd_element_button_get(button));
}

static void element_button_clicked_stay_open(GtkWidget *button, gpointer data) {
    element_button_clicked_func(button, TRUE);
}

static void element_button_clicked_modal(GtkWidget *button, gpointer data) {
    element_button_clicked_func(button, FALSE);
}

/* set the title of the window associated with this element button. */
/* if the window is already open, also set the title there. */
void gd_element_button_set_dialog_title(GtkWidget *button, const char *title) {
    /* get original title, and free if needed */
    char *old_title=(char *) g_object_get_data(G_OBJECT(button), GDASH_WINDOW_TITLE);
    g_free(old_title);
    /* remember new title */
    g_object_set_data(G_OBJECT(button), GDASH_WINDOW_TITLE, title?g_strdup(title):g_strdup(_("Elements")));

    /* if it has its own window open at the moment, also set it */
    GtkWidget *dialog=(GtkWidget *) g_object_get_data(G_OBJECT(button), GDASH_DIALOG);
    if (dialog)
        gtk_window_set_title(GTK_WINDOW(dialog), title);

}

void gd_element_button_set_dialog_sensitive(GtkWidget *button, gboolean sens) {
    GtkWidget *vbox=(GtkWidget *) g_object_get_data(G_OBJECT(button), GDASH_DIALOG_VBOX);
    if (vbox)
        gtk_widget_set_sensitive(vbox, sens);
}

/* frees the special title text, and optionally destroys the dialog, if exists. */
static void element_button_destroyed(GtkWidget *button, gpointer data) {
    char *title=(char *) g_object_get_data(G_OBJECT(button), GDASH_WINDOW_TITLE);
    g_free(title);
    /* if it has a dialog open for any reason, close that also */
    GtkWidget *dialog=(GtkWidget *) g_object_get_data(G_OBJECT(button), GDASH_DIALOG);
    if (dialog)
        gtk_widget_destroy(dialog);
}

/**
 * creates a new element button. optionally the dialog created by
 * the particular button can stay open, and accept many clicks.
 * also, it can have some title line, like "draw element"
 */
GtkWidget *gd_element_button_new(GdElementEnum initial_element, gboolean stays_open, const char *special_title) {
    // the button
    GtkWidget *button=gtk_button_new();

    // the contents - icon of elemen + name
    GtkWidget *hbox=gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(button), hbox);
    GtkWidget *image=gtk_image_new();
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    GtkWidget *label=gd_label_new_leftaligned("");
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(button), GDASH_IMAGE, image);
    g_object_set_data(G_OBJECT(button), GDASH_LABEL, label);

    g_signal_connect(G_OBJECT(button), "destroy", (GCallback) element_button_destroyed, NULL);
    /* minimum width 96px */
    gtk_widget_set_size_request(button, 96, -1);
    if (stays_open)
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(element_button_clicked_stay_open), NULL);
    else
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(element_button_clicked_modal), NULL);

    /* set the associated string which will be the title of the element box window opened */
    gd_element_button_set_dialog_title(button, special_title);

    gd_element_button_set(button, initial_element);
    return button;
}

#undef GDASH_IMAGE
#undef GDASH_LABEL
#undef GDASH_WINDOW_TITLE
#undef GDASH_DIALOG_VBOX
#undef GDASH_DIALOG
#undef GDASH_CELL
#undef GDASH_ELEMENT
#undef GDASH_HOVER
#undef GDASH_BUTTON


/* directions to be shown, and corresponding icons. */
static GdDirection const direction_combo_shown_directions[]= { MV_UP, MV_RIGHT, MV_DOWN, MV_LEFT };
static const char *direction_combo_shown_icons[]= { GTK_STOCK_GO_UP, GTK_STOCK_GO_FORWARD, GTK_STOCK_GO_DOWN, GTK_STOCK_GO_BACK };

GtkWidget *gd_direction_combo_new(GdDirectionEnum initial) {
    static GtkListStore *store=NULL;
    enum {
        DIR_PIXBUF_COL,
        DIR_TEXT_COL,
        NUM_DIR_COLS
    };

    // create list, if did not do so far. a single one can be used for multiple combo boxes.
    if (!store) {
        // this cell view is used to render the icons
        GtkWidget *cellview=gtk_cell_view_new();
        store=gtk_list_store_new(NUM_DIR_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);

        for (unsigned i=0; i<G_N_ELEMENTS(direction_combo_shown_directions); i++) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            GdkPixbuf *pixbuf=gtk_widget_render_icon(cellview, direction_combo_shown_icons[i], GTK_ICON_SIZE_MENU, NULL);
            gtk_list_store_set(store, &iter, DIR_PIXBUF_COL, pixbuf, DIR_TEXT_COL, _(visible_name(direction_combo_shown_directions[i])), -1);
        }

        gtk_widget_destroy(cellview);
    }

    /* create combo box and renderer for icon and text */
    GtkWidget *combo=gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer *renderer;
    renderer=gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", DIR_PIXBUF_COL, NULL);
    renderer=gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", DIR_TEXT_COL, NULL);

    /* set to initial value */
    /* we have to find it in the array */
    for (unsigned i=0; i<G_N_ELEMENTS(direction_combo_shown_directions); i++)
        if (direction_combo_shown_directions[i]==initial)
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);

    return combo;
}

GdDirectionEnum gd_direction_combo_get_direction(GtkWidget *combo) {
    return (GdDirectionEnum) direction_combo_shown_directions[gtk_combo_box_get_active(GTK_COMBO_BOX(combo))];
}


/*****************************************************/


GtkWidget *gd_scheduling_combo_new(GdSchedulingEnum initial) {
    GtkWidget *combo;

    /* no icons, so a simple text combo box will suffice. */
    combo=gtk_combo_box_new_text();

    for (int i=0; i<GD_SCHEDULING_MAX; i++)
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _(visible_name((GdSchedulingEnum) i)));

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), initial);

    return combo;
}

GdSchedulingEnum gd_scheduling_combo_get_scheduling(GtkWidget *combo) {
    return (GdSchedulingEnum)(gtk_combo_box_get_active(GTK_COMBO_BOX(combo)));
}

