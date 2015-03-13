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
#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "cave.h"
#include "cavedb.h"
#include "gtkgfx.h"
#include "editorwidgets.h"

/*
 * A COMBO BOX with c64 colors.
 * use color_combo_new for creating and color_combo_get_color for getting color in gdash format.
 *
 */
/* this data field always stores the previously selected color. */
/* it is needed when a select atari or select rgb dialog is escaped, and we must set the original color. */
/* the combo box itself cannot store it, as is is already set to the select atari... or select rgb... line. */
#define GDASH_COLOR "gdash-color"

static GtkTreePath *selected_color_path=NULL;

enum {
    COL_COLOR_ACTION,
    COL_COLOR_CODE,
    COL_COLOR_NAME,
    COL_COLOR_PIXBUF,
};

typedef enum {
    COLOR_ACTION_NONE,
    COLOR_ACTION_SELECT_ATARI,
    COLOR_ACTION_SELECT_DTV,
    COLOR_ACTION_SELECT_RGB,
} ColorAction;

/*
 * creates a small pixbuf with the specified color
 */
static GdkPixbuf *
color_combo_pixbuf_for_gd_color(GdColor col)
{
    int x, y;
    guint32 pixel;
    GdkPixbuf *pixbuf;
    
    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &x, &y);

    pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, x, y);
    pixel=(gd_color_get_r(col)<<24) + (gd_color_get_g(col)<<16) + (gd_color_get_b(col)<<8);
    gdk_pixbuf_fill(pixbuf, pixel);

    return pixbuf;
}

/* set to a color - first check if that is a c64 color. */
void
gd_color_combo_set(GtkComboBox *combo, GdColor color)
{
    g_object_set_data(G_OBJECT(combo), GDASH_COLOR, GUINT_TO_POINTER(color));
    
    if (gd_color_is_c64(color)) {
        char *path;
        GtkTreeIter iter;
        
        path=g_strdup_printf("0:%d", gd_color_get_c64_index(color));
        gtk_tree_model_get_iter_from_string(gtk_combo_box_get_model(combo), &iter, path);
        g_free(path);
        gtk_combo_box_set_active_iter(combo, &iter);
    }
    else {
        GtkTreeModel *model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
        GtkTreeIter iter;
        GdkPixbuf *pixbuf;
        
        gtk_tree_model_get_iter(model, &iter, selected_color_path);

        pixbuf=color_combo_pixbuf_for_gd_color(color);
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COL_COLOR_CODE, color, COL_COLOR_PIXBUF, pixbuf, COL_COLOR_NAME, gd_color_get_visible_name(color), -1);
        g_object_unref(pixbuf);    /* now the tree store owns its own reference */

        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
    }
}

static gboolean
color_combo_drawing_area_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkDialog *dialog=GTK_DIALOG(data);
    
    gtk_dialog_response(dialog, GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), GDASH_COLOR)));

    return TRUE;
}



static void
color_combo_changed(GtkWidget *combo, gpointer data)
{
    GtkTreeModel *model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
    GtkTreeIter iter;
    ColorAction action;

    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
    gtk_tree_model_get(model, &iter, COL_COLOR_ACTION, &action, -1);
    switch (action) {
        case COLOR_ACTION_SELECT_RGB:
            {
                GtkWidget *dialog;
                GtkColorSelection *colorsel;
                 gint response;
                 GdkColor gc;
                 GdColor prevcol;
                 
                dialog=gtk_color_selection_dialog_new (_("Select Color"));
                gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gtk_widget_get_toplevel(combo)));

                colorsel=GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel);
                
                prevcol=GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(combo), GDASH_COLOR));
                gc.red=gd_color_get_r(prevcol)<<8;
                gc.green=gd_color_get_g(prevcol)<<8;
                gc.blue=gd_color_get_b(prevcol)<<8;
                gtk_color_selection_set_previous_color (colorsel, &gc);
                gtk_color_selection_set_current_color (colorsel, &gc);
                gtk_color_selection_set_has_palette (colorsel, TRUE);

                gtk_window_present_with_time(GTK_WINDOW(dialog), gtk_get_current_event_time());
                response=gtk_dialog_run(GTK_DIALOG (dialog));

                if (response==GTK_RESPONSE_OK) {
                    GdkColor gc;
                    GdColor color;
                    
                    gtk_color_selection_get_current_color(colorsel, &gc);
                    color=gd_color_get_from_rgb(gc.red>>8, gc.green>>8, gc.blue>>8);
                    gd_color_combo_set(GTK_COMBO_BOX(combo), color);
                } else {
                    gd_color_combo_set(GTK_COMBO_BOX(combo), prevcol);
                }

                gtk_widget_destroy (dialog);    
            }
            break;
        
        case COLOR_ACTION_SELECT_ATARI:
        case COLOR_ACTION_SELECT_DTV:
            {
                GtkWidget *dialog, *table, *frame;
                 GdColor prevcol;
                int i;
                int result;
                GdColor (*colorfunc) (int i);
                const char *title;
                
                switch (action) {
                    case COLOR_ACTION_SELECT_ATARI:
                        title=_("Select Atari Color");
                        colorfunc=gd_atari_color;
                        break;
                    case COLOR_ACTION_SELECT_DTV:
                        title=_("Select C64 DTV Color");
                        colorfunc=gd_c64dtv_color;
                        break;
                    default:
                        g_assert_not_reached();
                }
                
                dialog=gtk_dialog_new_with_buttons(title, GTK_WINDOW (gtk_widget_get_toplevel(combo)), GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
                table=gtk_table_new(16, 16, TRUE);
                for (i=0; i<256; i++) {
                    GtkWidget *da;
                    GdkColor color;
                    GdColor c;
                    
                    da=gtk_drawing_area_new();
                    c=colorfunc(i);
                    g_object_set_data(G_OBJECT(da), GDASH_COLOR, GUINT_TO_POINTER(i));    /* attach the color index as data */
                    gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK);
                    gtk_widget_set_tooltip_text(da, gd_color_get_visible_name(c));
                    g_signal_connect(G_OBJECT(da), "button_press_event", G_CALLBACK(color_combo_drawing_area_button_press_event), dialog);    /* mouse click */
                    color.red=gd_color_get_r(c)*256;    /* 256 as gdk expect 16-bit/component */
                    color.green=gd_color_get_g(c)*256;
                    color.blue=gd_color_get_b(c)*256;
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
                
                prevcol=GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(combo), GDASH_COLOR));
                result=gtk_dialog_run(GTK_DIALOG(dialog));
                if (result>=0)
                    gd_color_combo_set(GTK_COMBO_BOX(combo), colorfunc(result));
                else
                    gd_color_combo_set(GTK_COMBO_BOX(combo), prevcol);

                gtk_widget_destroy(dialog);
            }
            break;
        
        case COLOR_ACTION_NONE:
            {
                GdColor c;
                
                gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
                gtk_tree_model_get(model, &iter, COL_COLOR_CODE, &c, -1);
                g_object_set_data(G_OBJECT(combo), GDASH_COLOR, GUINT_TO_POINTER(c));
            }
            break;
    }
}
/* A GtkTreeViewRowSeparatorFunc that demonstrates how rows can be
 * rendered as separators. 
 */
static gboolean
color_combo_is_separator (GtkTreeModel *model, GtkTreeIter  *iter, gpointer data)
{
  GtkTreePath *path;
  gboolean result;

  path=gtk_tree_model_get_path(model, iter);
  result=!gtk_tree_path_compare(path, selected_color_path);
  gtk_tree_path_free (path);

  return result;
}

static void
color_combo_set_sensitive(GtkCellLayout *cell_layout, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
  g_object_set(cell, "sensitive", !gtk_tree_model_iter_has_child(tree_model, iter), NULL);
}

/* combo box creator. */
GtkWidget *
gd_color_combo_new(const GdColor color)
{
    /* this is the base of the cave editor element combo box.
        categories are autodetected by their integer values being >O_MAX */
    GtkTreeStore *store;
    GtkWidget *combo;
    GtkCellRenderer *renderer;
    GtkTreeIter iter, parent;
    int i;

    /* tree store for colors. every combo has its own, as the custom color can be different. */
    store=gtk_tree_store_new(4, G_TYPE_INT, G_TYPE_UINT, G_TYPE_STRING, GDK_TYPE_PIXBUF);
    
    /* add 16 c64 colors */
    gtk_tree_store_append(store, &parent, NULL);
    gtk_tree_store_set(store, &parent, COL_COLOR_CODE, 0, COL_COLOR_NAME, _("C64 Colors"), COL_COLOR_PIXBUF, NULL, -1);
    for (i = 0; i<16; i++) {
        GdkPixbuf *pixbuf;

        pixbuf=color_combo_pixbuf_for_gd_color(gd_c64_color(i));
        gtk_tree_store_append(store, &iter, &parent);
        gtk_tree_store_set(store, &iter, COL_COLOR_ACTION, COLOR_ACTION_NONE, COL_COLOR_CODE, gd_c64_color(i), COL_COLOR_NAME, _(gd_color_get_visible_name(gd_c64_color(i))), COL_COLOR_PIXBUF, pixbuf, -1);
        g_object_unref (pixbuf);
    }
    gtk_tree_store_append(store, &iter, NULL);
    if (!selected_color_path)    
        selected_color_path=gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, COL_COLOR_ACTION, COLOR_ACTION_SELECT_ATARI, COL_COLOR_CODE, 0, COL_COLOR_NAME, _("Atari color..."), COL_COLOR_PIXBUF, NULL, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, COL_COLOR_ACTION, COLOR_ACTION_SELECT_DTV, COL_COLOR_CODE, 0, COL_COLOR_NAME, _("C64DTV color..."), COL_COLOR_PIXBUF, NULL, -1);
    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, COL_COLOR_ACTION, COLOR_ACTION_SELECT_RGB, COL_COLOR_CODE, 0, COL_COLOR_NAME, _("RGB color..."), COL_COLOR_PIXBUF, NULL, -1);
    
    combo=gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    /* first column, object image */
    renderer=gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combo), renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "pixbuf", COL_COLOR_PIXBUF, NULL);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo), renderer, color_combo_set_sensitive, NULL, NULL);
    /* second column, object name */
    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combo), renderer, TRUE);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo), renderer, color_combo_set_sensitive, NULL, NULL);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", COL_COLOR_NAME, NULL);
    gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(combo), color_combo_is_separator, NULL, NULL);

    gd_color_combo_set(GTK_COMBO_BOX(combo), color);
    g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(color_combo_changed), NULL);

    return combo;
}

GdColor
gd_color_combo_get_color(GtkWidget *widget)
{
    GtkTreeModel *model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    GtkTreeIter iter;
    GdColor color;

    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter);
    gtk_tree_model_get(model, &iter, COL_COLOR_CODE, &color, -1);
    return color;
}

#undef GDASH_COLOR













/****************************************************
 *
 * create and return a button, which contains
 *    cave elements available in the editor.
 *    an initial value is also selected.
 */
#define GDASH_CELL "gdash-cell"
#define GDASH_ELEMENT "gdash-element"
#define GDASH_HOVER "gdash-hover"
#define GDASH_BUTTON "gdash-button"

#define GDASH_DIALOG "gdash-dialog"
#define GDASH_WINDOW_TITLE "gdash-window-title"
#define GDASH_DIALOG_VBOX "gdash-dialog-vbox"
#define GDASH_BUTTON_IMAGE "gdash-button-image"
#define GDASH_BUTTON_LABEL "gdash-button-label"

static int element_button_animcycle;

/* this draws one element in the element selector box. */
static gboolean
element_button_drawing_area_expose_event (const GtkWidget * widget, const GdkEventExpose * event, const gpointer data)
{
    GdkDrawable *cell=g_object_get_data(G_OBJECT(widget), GDASH_CELL);

    if (!widget->window)
        return FALSE;
    
    if (cell)
        gdk_draw_drawable (widget->window, widget->style->black_gc, cell, 0, 0, 2, 2, gd_cell_size_editor, gd_cell_size_editor);
    return TRUE;
}

/* this is called when entering and exiting a drawing area with the mouse. */
/* so they can be colored when the mouse is over. */
static gboolean
element_button_drawing_area_crossing_event (const GtkWidget *widget, const GdkEventCrossing *event, const gpointer data)
{
    g_object_set_data(G_OBJECT(widget), GDASH_HOVER, GINT_TO_POINTER(event->type==GDK_ENTER_NOTIFY));
    return FALSE;
}

static gboolean
redraw_timeout (gpointer data)
{
    GList *areas=(GList *)data;
    GList *iter;

    element_button_animcycle=(element_button_animcycle+1)&7;
    
    for (iter=areas; iter!=NULL; iter=iter->next) {
        GtkWidget *da=(GtkWidget *)iter->data;
        GdElement element;
        int draw;
        gboolean hover;
        
        /* which element is drawn? */
        element=(GdElement)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(da), GDASH_ELEMENT));
        hover=(gboolean)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(da), GDASH_HOVER));
        /* get pixbuf index */
        draw=gd_elements[element].image;
        if (draw<0)
            draw=-draw + element_button_animcycle;
        if (hover)
            draw+=NUM_OF_CELLS;
        /* set cell and queue draw if different from previous */
        /* at first start, previous is null, so always different. */
        if (g_object_get_data(G_OBJECT(da), GDASH_CELL)!=gd_editor_pixmap(draw)) {
            g_object_set_data(G_OBJECT(da), GDASH_CELL, gd_editor_pixmap(draw));
            gtk_widget_queue_draw(da);
        }
    }
    return TRUE;
}


void
gd_element_button_set(GtkWidget *button, const GdElement element)
{
    GtkWidget *image, *label;
    label=GTK_WIDGET(g_object_get_data(G_OBJECT(button), GDASH_BUTTON_LABEL));
    image=GTK_WIDGET(g_object_get_data(G_OBJECT(button), GDASH_BUTTON_IMAGE));

    gtk_image_set_from_pixbuf(GTK_IMAGE(image), gd_get_element_pixbuf_with_border(element));
    gtk_label_set_text(GTK_LABEL(label), _(gd_elements[element].name));
    g_object_set_data(G_OBJECT(button), GDASH_ELEMENT, GINT_TO_POINTER(element));
}

static void
element_button_da_clicked(GtkWidget *da, GdkEventButton *event, gpointer data)
{
    GtkDialog *dialog=GTK_DIALOG(data);
    GtkWidget *button;
    GdElement element;

    /* set the corresponding button to the element selected */
    element=(GdElement)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(da), GDASH_ELEMENT));
    button=GTK_WIDGET(g_object_get_data(G_OBJECT(da), GDASH_BUTTON));
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

static void
element_button_dialog_destroyed_free_list(GtkWidget *dialog, gpointer data)
{
    GList *areas=(GList *) data;

    g_source_remove_by_user_data(areas);
    g_list_free(areas);
}

static void
element_button_dialog_destroyed_null_pointer(GtkWidget *dialog, gpointer data)
{
    g_object_set_data(G_OBJECT(data), GDASH_DIALOG, NULL);
    g_object_set_data(G_OBJECT(data), GDASH_DIALOG_VBOX, NULL);
}

static void
element_button_dialog_close_button_clicked(GtkWidget *button, gpointer data)
{
    /* data is the dialog */
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void
element_button_clicked_func(GtkWidget *button, gboolean stay_open)
{
    static const GdElement elements[]= {
        /* normal */
        O_SPACE, O_DIRT, O_DIAMOND, O_STONE, O_MEGA_STONE, O_FLYING_DIAMOND, O_FLYING_STONE, O_NUT,
        O_BRICK, O_FALLING_WALL, O_BRICK_EATABLE, O_BRICK_NON_SLOPED, O_SPACE, O_STEEL, O_STEEL_EATABLE, O_STEEL_EXPLODABLE,
        
        O_INBOX, O_PRE_OUTBOX, O_PRE_INVIS_OUTBOX, O_PLAYER_GLUED, O_VOODOO, O_SPACE, O_SPACE, O_SKELETON,
        O_WALLED_KEY_1, O_WALLED_KEY_2, O_WALLED_KEY_3, O_WALLED_DIAMOND, O_STEEL_SLOPED_UP_RIGHT, O_STEEL_SLOPED_UP_LEFT, O_STEEL_SLOPED_DOWN_LEFT, O_STEEL_SLOPED_DOWN_RIGHT,
        
        O_AMOEBA, O_AMOEBA_2, O_SLIME, O_ACID, O_MAGIC_WALL, O_WATER, O_LAVA, O_REPLICATOR,
        O_KEY_1, O_KEY_2, O_KEY_3, O_DIAMOND_KEY, O_BRICK_SLOPED_DOWN_RIGHT, O_BRICK_SLOPED_DOWN_LEFT, O_BRICK_SLOPED_UP_LEFT, O_BRICK_SLOPED_UP_RIGHT,

        O_BOMB, O_CLOCK, O_POT, O_BOX, O_SWEET, O_PNEUMATIC_HAMMER, O_NITRO_PACK, O_TELEPORTER,
        O_DOOR_1, O_DOOR_2, O_DOOR_3, O_TRAPPED_DIAMOND, O_DIRT_SLOPED_UP_RIGHT, O_DIRT_SLOPED_UP_LEFT, O_DIRT_SLOPED_DOWN_LEFT, O_DIRT_SLOPED_DOWN_RIGHT, 

        O_GRAVITY_SWITCH, O_CREATURE_SWITCH, O_BITER_SWITCH, O_EXPANDING_WALL_SWITCH, O_REPLICATOR_SWITCH, O_SPACE, O_CONVEYOR_SWITCH, O_CONVEYOR_DIR_SWITCH,
        O_H_EXPANDING_WALL, O_V_EXPANDING_WALL, O_EXPANDING_WALL, O_SPACE, O_DIRT_BALL, O_DIRT_LOOSE, O_SPACE, O_NONE,
        
        O_CONVEYOR_LEFT, O_CONVEYOR_RIGHT, O_SPACE, O_SPACE, O_SPACE, O_DIRT_GLUED, O_DIAMOND_GLUED, O_STONE_GLUED,
        O_H_EXPANDING_STEEL_WALL, O_V_EXPANDING_STEEL_WALL, O_EXPANDING_STEEL_WALL, O_BLADDER_SPENDER, O_BLADDER, O_GHOST, O_WAITING_STONE, O_CHASING_STONE, 

        O_SPACE, O_FIREFLY_2, O_ALT_FIREFLY_2, O_SPACE, O_SPACE, O_BUTTER_2, O_ALT_BUTTER_2, O_SPACE, O_SPACE, O_STONEFLY_2, O_SPACE, O_COW_2, O_BITER_1, O_SPACE, O_SPACE, O_DRAGONFLY_2,
        O_FIREFLY_1, O_FIREFLY_3, O_ALT_FIREFLY_1, O_ALT_FIREFLY_3, O_BUTTER_1, O_BUTTER_3, O_ALT_BUTTER_1, O_ALT_BUTTER_3, O_STONEFLY_1, O_STONEFLY_3, O_COW_1, O_COW_3, O_BITER_4, O_BITER_2, O_DRAGONFLY_1, O_DRAGONFLY_3,
        O_SPACE, O_FIREFLY_4, O_ALT_FIREFLY_4, O_SPACE, O_SPACE, O_BUTTER_4, O_ALT_BUTTER_4, O_SPACE, O_SPACE, O_STONEFLY_4, O_SPACE, O_COW_4, O_BITER_3, O_SPACE, O_SPACE, O_DRAGONFLY_4,

        /* for effects */        
        O_DIAMOND_F, O_STONE_F, O_MEGA_STONE_F, O_FLYING_DIAMOND_F, O_FLYING_STONE_F, O_FALLING_WALL_F, O_NITRO_PACK_F, O_NUT_F, O_PRE_PL_1, O_PRE_PL_2, O_PRE_PL_3, O_PLAYER, O_PLAYER_BOMB, O_PLAYER_STIRRING, O_OUTBOX, O_INVIS_OUTBOX,

        O_BLADDER_1, O_BLADDER_2, O_BLADDER_3, O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7, O_BLADDER_8, O_DIRT2,
        O_COW_ENCLOSED_1, O_COW_ENCLOSED_2, O_COW_ENCLOSED_3, O_COW_ENCLOSED_4, O_COW_ENCLOSED_5, O_COW_ENCLOSED_6, O_COW_ENCLOSED_7,

        O_WATER_1, O_WATER_2, O_WATER_3, O_WATER_4, O_WATER_5, O_WATER_6, O_WATER_7, O_WATER_8,
        O_WATER_9, O_WATER_10, O_WATER_11, O_WATER_12, O_WATER_13, O_WATER_14, O_WATER_15, O_WATER_16,

        O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3, O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
        O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4, O_NUT_EXPL_1, O_NUT_EXPL_2, O_NUT_EXPL_3, O_NUT_EXPL_4, O_UNKNOWN,
        
        O_EXPLODE_1, O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5, O_TIME_PENALTY,
        O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4, O_PRE_DIA_5, O_NITRO_PACK_EXPLODE, O_NITRO_EXPL_1, O_NITRO_EXPL_2, O_NITRO_EXPL_3, O_NITRO_EXPL_4,
        O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4, O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
        O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4, O_GHOST_EXPL_1, O_GHOST_EXPL_2, O_GHOST_EXPL_3, O_GHOST_EXPL_4,


    };
        
    int cols=16;
    int i;
    GList *areas=NULL;
    GtkWidget *dialog, *expander, *table, *table2, *align, *vbox;
    int into_second=0;
    GdkColor c;
    
    /* if the dialog is already open, only show it. */
    dialog=(GtkWidget *)(g_object_get_data(G_OBJECT(button), GDASH_DIALOG));
    if (dialog) {
        gtk_window_present(GTK_WINDOW(dialog));

        return;
    }

    /* elements dialog with no buttons; clicking on an element will do the trick. */
    dialog=gtk_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_toplevel(button)));
    gtk_window_set_title(GTK_WINDOW(dialog), g_object_get_data(G_OBJECT(button), GDASH_WINDOW_TITLE));
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    /* associate the dialog with the button, so we know that it is open */
    g_object_set_data(G_OBJECT(button), GDASH_DIALOG, dialog);
    
    vbox=gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox);
    g_object_set_data(G_OBJECT(button), GDASH_DIALOG_VBOX, vbox);

    align=gtk_alignment_new(0, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(align), gtk_label_new(_("Normal elements")));
    gtk_box_pack_start_defaults(GTK_BOX(vbox), align);
    
    
    table=gtk_table_new(0, 0, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 6);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), table);

    expander=gtk_expander_new(_("For effects"));
    table2=gtk_table_new(0, 0, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(table2), 6);
    gtk_container_add(GTK_CONTAINER(expander), table2);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), expander);

    /* color for background around elements. */
    /* gdkcolors are 16bit, we should *256, but instead *200 so a bit darker. */
    c.red=gd_color_get_r(gd_current_background_color())*200;
    c.green=gd_color_get_g(gd_current_background_color())*200;
    c.blue=gd_color_get_b(gd_current_background_color())*200;
    /* create drawing areas */
    for (i=0; i<G_N_ELEMENTS(elements); i++) {
        GtkWidget *da;
        
        da=gtk_drawing_area_new();
        gtk_widget_modify_bg(da, GTK_STATE_NORMAL, &c);
        areas=g_list_prepend(areas, da);    /* put in list for animation timeout, that one will request redraw on them */
        gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK|GDK_LEAVE_NOTIFY_MASK|GDK_ENTER_NOTIFY_MASK);
        g_object_set_data(G_OBJECT(da), GDASH_ELEMENT, GINT_TO_POINTER(elements[i]));
        g_object_set_data(G_OBJECT(da), GDASH_BUTTON, button);    /* button to update on click */
        gtk_widget_set_size_request(da, gd_cell_size_editor+4, gd_cell_size_editor+4);    /* 2px border around them */
        gtk_widget_set_tooltip_text(da, _(gd_elements[elements[i]].name));
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
    g_timeout_add(40, redraw_timeout, areas);
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

GdElement
gd_element_button_get (GtkWidget *button)
{
    gpointer element=g_object_get_data (G_OBJECT(button), GDASH_ELEMENT);
    return (GdElement)GPOINTER_TO_INT(element);
}

/* the pixbufs might be changed during the lifetime of an element button. *
 * for example, when colors of a cave are redefined. so this is made
 * global and can be called. */
void
gd_element_button_update_pixbuf(GtkWidget *button)
{
    /* set the same as it already shows, but pixbuf update will be triggered by this */
    gd_element_button_set(button, gd_element_button_get(button));
}

static void
element_button_clicked_stay_open(GtkWidget *button, gpointer data)
{
    element_button_clicked_func(button, TRUE);
}

static void
element_button_clicked_modal(GtkWidget *button, gpointer data)
{
    element_button_clicked_func(button, FALSE);
}

/* set the title of the window associated with this element button. */
/* if the window is already open, also set the title there. */
void
gd_element_button_set_dialog_title(GtkWidget *button, const char *title)
{
    char *old_title;
    GtkWidget *dialog;

    /* get original title, and free if needed */    
    old_title=g_object_get_data(G_OBJECT(button), GDASH_WINDOW_TITLE);
    g_free(old_title);
    /* remember new title */
    g_object_set_data(G_OBJECT(button), GDASH_WINDOW_TITLE, title?g_strdup(title):g_strdup(_("Elements")));

    /* if it has its own window open at the moment, also set it */    
    dialog=g_object_get_data(G_OBJECT(button), GDASH_DIALOG);
    if (dialog)
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    
}

void
gd_element_button_set_dialog_sensitive(GtkWidget *button, gboolean sens)
{
    GtkWidget *vbox;

    vbox=g_object_get_data(G_OBJECT(button), GDASH_DIALOG_VBOX);
    if (vbox)
        gtk_widget_set_sensitive(vbox, sens);
}

/* frees the special title text, and optionally destroys the dialog, if exists. */
static void
element_button_destroyed(GtkWidget *button, gpointer data)
{
    char *title;
    GtkWidget *dialog;
    
    title=g_object_get_data(G_OBJECT(button), GDASH_WINDOW_TITLE);
    g_free(title);
    /* if it has a dialog open for any reason, close that also */
    dialog=g_object_get_data(G_OBJECT(button), GDASH_DIALOG);
    if (dialog)
        gtk_widget_destroy(dialog);
}

/* creates a new element button. optionally the dialog created by
 * the particular button can stay open, and accept many clicks.
 * also, it can have some title line, like "draw element" */
GtkWidget *
gd_element_button_new(GdElement initial_element, gboolean stays_open, const char *special_title)
{
    GtkWidget *button, *image, *label, *hbox;
    
    button=gtk_button_new();
    hbox=gtk_hbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(button), hbox);
    label=gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);    /* left-align label */
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
    image=gtk_image_new();
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(button), GDASH_BUTTON_IMAGE, image);
    g_object_set_data(G_OBJECT(button), GDASH_BUTTON_LABEL, label);

    gtk_widget_set_size_request(button, 96, -1);    /* minimum width 96px */
    g_signal_connect(G_OBJECT(button), "destroy", (GCallback) element_button_destroyed, NULL);
    if (stays_open)
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(element_button_clicked_stay_open), NULL);
    else
        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(element_button_clicked_modal), NULL);
    
    /* set the associated string which will be the title of the element box window opened */
    gd_element_button_set_dialog_title(button, special_title);

    gd_element_button_set(button, initial_element);
    return button;
}

#undef GDASH_BUTTON_IMAGE
#undef GDASH_BUTTON_LABEL
#undef GDASH_WINDOW_TITLE
#undef GDASH_DIALOG_VBOX
#undef GDASH_DIALOG
#undef GDASH_CELL
#undef GDASH_ELEMENT
#undef GDASH_HOVER
#undef GDASH_BUTTON

















enum 
{
  DIR_PIXBUF_COL,
  DIR_TEXT_COL,
  NUM_DIR_COLS
};

/* directions to be shown, and corresponding icons. */
static const GdDirection direction_combo_shown_directions[]= { MV_UP, MV_RIGHT, MV_DOWN, MV_LEFT };
static const char *direction_combo_shown_icons[]= { GTK_STOCK_GO_UP, GTK_STOCK_GO_FORWARD, GTK_STOCK_GO_DOWN, GTK_STOCK_GO_BACK };

GtkWidget *
gd_direction_combo_new(const GdDirection initial)
{
    GtkWidget *combo;
    static GtkListStore *store=NULL;
    GtkCellRenderer *renderer;
    int i;
    
    /* create list, if did not do so far. */
    if (!store) {
        GtkTreeIter iter;
        GtkWidget *cellview;
        GdkPixbuf *pixbuf;
        
        cellview=gtk_cell_view_new();
        store=gtk_list_store_new (NUM_DIR_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING);

        for (i=0; i<G_N_ELEMENTS(direction_combo_shown_directions); i++) {
            gtk_list_store_append(store, &iter);
            pixbuf=gtk_widget_render_icon(cellview, direction_combo_shown_icons[i], GTK_ICON_SIZE_MENU, NULL);
            gtk_list_store_set(store, &iter, DIR_PIXBUF_COL, pixbuf, DIR_TEXT_COL, _(gd_direction_get_visible_name(direction_combo_shown_directions[i])), -1);
        }

        gtk_widget_destroy(cellview);
    }

    /* create combo box and renderer for icon and text */
    combo=gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    renderer=gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combo), renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "pixbuf", DIR_PIXBUF_COL, NULL);
    renderer=gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combo), renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", DIR_TEXT_COL, NULL);

    /* set to initial value */
    for (i=0; i<G_N_ELEMENTS(direction_combo_shown_directions); i++)
        if (direction_combo_shown_directions[i]==initial)
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);

    return combo;
}

GdDirection
gd_direction_combo_get_direction(GtkWidget *combo)
{
    return (GdDirection) direction_combo_shown_directions[gtk_combo_box_get_active(GTK_COMBO_BOX(combo))];
}


/*****************************************************/











GtkWidget *
gd_scheduling_combo_new(const GdScheduling initial)
{
    GtkWidget *combo;
    int i;
    
    combo=gtk_combo_box_new_text();

    for (i=0; i<GD_SCHEDULING_MAX; i++)
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _(gd_scheduling_get_visible_name((GdScheduling) i)));

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), initial);
    
    return combo;
}

GdScheduling
gd_scheduling_combo_get_scheduling(GtkWidget *combo)
{
    return (GdScheduling)(gtk_combo_box_get_active(GTK_COMBO_BOX(combo)));
}

