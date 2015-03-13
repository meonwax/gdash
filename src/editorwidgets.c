/*
 * Copyright (c) 2007, 2008 Czirkos Zoltan <cirix@fw.hu>
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "cave.h"
#include "cavedb.h"
#include "gtkgfx.h"
#include "editorwidgets.h"

/*
 * A COMBO BOX with c64 colors.
 * use color_combo_new for creating and color_button_get_color for getting color in gdash format.
 *
 * combo store rows 0-15 are c64 colors, 16 is a separator and 17 is "other".
 * internally, 16 is set to a custom selected color, so it can be seen in the combo.
 * selecting 17 triggers this, and then 16 is automatically selected.
 */
#define GDASH_COLOR "gdash-color"

/*
 * creates a small pixbuf with the specified color
 */
static GdkPixbuf *
color_pixbuf(GdColor rgb)
{
	int x, y;
    guint32 pixel;
    GdkPixbuf *pixbuf;
	
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &x, &y);

    pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, x, y);
    pixel=(((rgb>>16)&255)<<24) + (((rgb>>8)&255)<<16) + ((rgb&255)<<8);
    gdk_pixbuf_fill (pixbuf, pixel);
	return pixbuf;
}

/* selects a custom color: creates pixbuf, text, and selects row number 16. */
static void
color_combo_select_custom(GtkComboBox *combo, GdColor color)
{
	GtkTreeModel *model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
	GtkTreePath *path;
	GtkTreeIter iter;
	char text[16];
	GdkPixbuf *pixbuf;
	
	path=gtk_tree_path_new_from_indices(16, -1);
	gtk_tree_model_get_iter(model, &iter, path);

	g_object_set_data(G_OBJECT(combo), GDASH_COLOR, GUINT_TO_POINTER(color));

	sprintf(text, "#%02x%02x%02x", (color>>16)&255, (color>>8)&255, color&255);
	pixbuf=color_pixbuf(color);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 2, pixbuf, 1, text, -1);
	g_object_unref(pixbuf);

	gtk_tree_path_free(path);
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo), &iter);
}

/* set to a color - first check if that is a c64 color. */
void
gd_color_combo_set (GtkComboBox *combo, GdColor color)
{
	int i;
	
	for (i=0; i<16; i++) {
		if (gd_c64_colors[i].rgb==color) {
			gtk_combo_box_set_active(combo, i);
			return;
		}
	}
	
	color_combo_select_custom(combo, color);
}

/* changed signal. for selected row 17, pops up color selection dialog;
 * for <16, it is a normal c64 color.
 */
static void
color_combo_changed (GtkWidget *combo, gpointer data)
{
	GtkTreeModel *model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
	GtkTreeIter iter;
	int i;
	
	i=gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	if (i==17) {
		GtkWidget *dialog;
		GtkColorSelection *colorsel;
 		gint response;
 		GdkColor gc;
 		GdColor prevcol;
 		
		dialog=gtk_color_selection_dialog_new (_("Select Color"));
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gtk_widget_get_toplevel(combo)));

		colorsel=GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (dialog)->colorsel);
		
		prevcol=GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(combo), GDASH_COLOR));
		gc.red=((prevcol>>16)&255)<<8;
		gc.green=((prevcol>>8)&255)<<8;
		gc.blue=((prevcol>>0)&255)<<8;

		gtk_color_selection_set_previous_color (colorsel, &gc);
		gtk_color_selection_set_current_color (colorsel, &gc);
		gtk_color_selection_set_has_palette (colorsel, TRUE);

		gtk_window_present_with_time(GTK_WINDOW(dialog), gtk_get_current_event_time());
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		if (response == GTK_RESPONSE_OK) {
			GdkColor gc;
			GdColor color;
			
			gtk_color_selection_get_current_color (colorsel, &gc);
			color=((gc.red>>8)<<16) + ((gc.green>>8)<<8) + (gc.blue>>8);
			color_combo_select_custom(GTK_COMBO_BOX(combo), color);
		} else {
			gd_color_combo_set(GTK_COMBO_BOX(combo), prevcol);
		}

		gtk_widget_destroy (dialog);	

	} else
	if (i<16) {
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
		gtk_tree_model_get(model, &iter, 0, &i, -1);
		g_object_set_data(G_OBJECT(combo), GDASH_COLOR, GUINT_TO_POINTER(i));
	}	
}

/* A GtkTreeViewRowSeparatorFunc that demonstrates how rows can be
 * rendered as separators. This particular function does nothing 
 * useful and just turns the fourth row into a separator.
 */
static gboolean color_button_is_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GtkTreePath *path;
	gboolean result;

	path = gtk_tree_model_get_path (model, iter);
	result = gtk_tree_path_get_indices (path)[0] == 16;
	gtk_tree_path_free (path);

	return result;
}


/* combo box creator. */
GtkWidget *
gd_color_combo_new (const GdColor color)
{
	/* this is the base of the cave editor element combo box.
	    categories are autodetected by their integer values being >O_MAX */
	GtkTreeStore *store;
	GtkWidget *combo;
	GtkCellRenderer *renderer;
    GtkTreeIter iter;
    int i;

	/* tree store for colors. every combo has its own, as the custom color can be different. */
    store = gtk_tree_store_new (3, G_TYPE_UINT, G_TYPE_STRING, GDK_TYPE_PIXBUF);
    for (i = 0; i < 16; i++) {
        GdkPixbuf *pixbuf;

        pixbuf=color_pixbuf(gd_c64_colors[i].rgb);
        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter, 0, gd_c64_colors[i].rgb, 1, _(gd_c64_colors[i].name), 2, pixbuf, -1);
        g_object_unref (pixbuf);
    }
    gtk_tree_store_append (store, &iter, NULL);	/* will be the separator */
    gtk_tree_store_append (store, &iter, NULL);
    gtk_tree_store_set (store, &iter, 0, 0, 1, "Other", 2, NULL, -1);

	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), color_button_is_separator, NULL, NULL);
	/* first column, object image */
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "pixbuf", 2, NULL);

	/* second column, object name */
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", 1, NULL);

	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(color_combo_changed), NULL);
	gd_color_combo_set(GTK_COMBO_BOX(combo), color);

	return combo;
}

GdColor
gd_color_combo_get_color(GtkWidget *widget)
{
	return GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), GDASH_COLOR));
}

#undef GDASH_COLOR













/****************************************************
 *
 * create and return a button, which contains
 *	cave elements available in the editor.
 *	an initial value is also selected.
 */
#define GDASH_CELL "gdash-cell"
#define GDASH_ELEMENT "gdash-element"
#define GDASH_HOVER "gdash-hover"
static int button_animcycle;

static gboolean
button_drawing_area_expose_event (const GtkWidget * widget, const GdkEventExpose * event, const gpointer data)
{
	GdkDrawable *cell=g_object_get_data(G_OBJECT(widget), GDASH_CELL);

	if (!widget->window)
		return FALSE;
	
	if (cell)
		gdk_draw_drawable (widget->window, widget->style->black_gc, cell, 0, 0, 2, 2, gd_cell_size_editor, gd_cell_size_editor);
	gdk_draw_rectangle (widget->window, widget->style->black_gc, FALSE, 0, 0, gd_cell_size_editor+3, gd_cell_size_editor+3);
	gdk_draw_rectangle (widget->window, widget->style->black_gc, FALSE, 1, 1, gd_cell_size_editor+1, gd_cell_size_editor+1);
	return TRUE;
}

static gboolean
button_drawing_area_crossing_event (const GtkWidget *widget, const GdkEventCrossing *event, const gpointer data)
{
	g_object_set_data(G_OBJECT(widget), GDASH_HOVER, GINT_TO_POINTER(event->type==GDK_ENTER_NOTIFY));
	return FALSE;
}

static gboolean
redraw_timeout (gpointer data)
{
	GList *areas=(GList *)data;
	GList *iter;

	button_animcycle=(button_animcycle+1)&7;
	
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
			draw=-draw + button_animcycle;
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
gd_element_button_set (GtkWidget *button, const GdElement element)
{
	GtkWidget *image;

	image=gtk_image_new_from_pixbuf(gd_get_element_pixbuf_with_border (element));
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_show(image);	/* always display */
	gtk_button_set_label(GTK_BUTTON(button), _(gd_elements[element].name));
	g_object_set_data (G_OBJECT(button), GDASH_ELEMENT, GINT_TO_POINTER(element));
}

static void
element_button_da_clicked(GtkWidget *da, GdkEvent *event, gpointer data)
{
	GtkDialog *dialog=GTK_DIALOG(data);
	GdElement element;

	element=(GdElement)GPOINTER_TO_INT(g_object_get_data (G_OBJECT(da), GDASH_ELEMENT));
	gtk_dialog_response(dialog, element);
}

static void
element_button_clicked(GtkWidget *button, gpointer data)
{
	static const GdElement elements[]= {
		/* normal */
		O_SPACE, O_DIRT, O_DIAMOND, O_STONE, O_MEGA_STONE, O_DIRT_GLUED, O_DIAMOND_GLUED, O_STONE_GLUED,
		O_BRICK, O_BRICK_EATABLE, O_BRICK_NON_SLOPED, O_SPACE, O_SPACE, O_STEEL, O_STEEL_EATABLE, O_STEEL_EXPLODABLE,
		
		O_INBOX, O_PRE_OUTBOX, O_PRE_INVIS_OUTBOX, O_PLAYER_GLUED, O_VOODOO, O_SPACE, O_SPACE, O_SPACE,
		O_WALLED_KEY_1, O_WALLED_KEY_2, O_WALLED_KEY_3, O_WALLED_DIAMOND, O_STEEL_SLOPED_UP_RIGHT, O_STEEL_SLOPED_UP_LEFT, O_STEEL_SLOPED_DOWN_LEFT, O_STEEL_SLOPED_DOWN_RIGHT,
		
		O_AMOEBA, O_SLIME, O_ACID, O_MAGIC_WALL, O_WATER, O_BLADDER_SPENDER, O_FALLING_WALL, O_SPACE,
		O_KEY_1, O_KEY_2, O_KEY_3, O_DIAMOND_KEY, O_BRICK_SLOPED_DOWN_RIGHT, O_BRICK_SLOPED_DOWN_LEFT, O_BRICK_SLOPED_UP_LEFT, O_BRICK_SLOPED_UP_RIGHT,

		O_BOMB, O_CLOCK, O_TELEPORTER, O_POT, O_SKELETON, O_BOX, O_SWEET, O_PNEUMATIC_HAMMER, 
		O_DOOR_1, O_DOOR_2, O_DOOR_3, O_TRAPPED_DIAMOND, O_DIRT_SLOPED_UP_RIGHT, O_DIRT_SLOPED_UP_LEFT, O_DIRT_SLOPED_DOWN_LEFT, O_DIRT_SLOPED_DOWN_RIGHT, 

		O_GRAVITY_SWITCH, O_CREATURE_SWITCH, O_BITER_SWITCH, O_GROWING_WALL_SWITCH, O_SPACE, O_SPACE, O_SPACE, O_SPACE,
		O_H_GROWING_WALL, O_V_GROWING_WALL, O_GROWING_WALL, O_SPACE, O_SPACE, O_SPACE, O_SPACE, O_NONE,

		O_SPACE, O_GUARD_2, O_ALT_GUARD_2, O_SPACE, O_SPACE, O_BUTTER_2, O_ALT_BUTTER_2, O_SPACE, O_SPACE, O_STONEFLY_2, O_COW_2, O_SPACE, O_SPACE, O_SPACE, O_SPACE, O_SPACE,
		O_GUARD_1, O_GUARD_3, O_ALT_GUARD_1, O_ALT_GUARD_3, O_BUTTER_1, O_BUTTER_3, O_ALT_BUTTER_1, O_ALT_BUTTER_3, O_STONEFLY_1, O_STONEFLY_3, O_COW_1, O_COW_3, O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,
		O_SPACE, O_GUARD_4, O_ALT_GUARD_4, O_SPACE, O_SPACE, O_BUTTER_4, O_ALT_BUTTER_4, O_SPACE, O_SPACE, O_STONEFLY_4, O_COW_4, O_SPACE, O_BLADDER, O_GHOST, O_WAITING_STONE, O_CHASING_STONE,

		/* for effects */		
		O_DIRT2, O_DIAMOND_F, O_STONE_F, O_MEGA_STONE_F, O_FALLING_WALL_F, O_UNKNOWN, O_PRE_PL_1, O_PRE_PL_2, O_PRE_PL_3, O_PLAYER, O_PLAYER_BOMB, O_PLAYER_STIRRING, O_OUTBOX, O_INVIS_OUTBOX, O_TIME_PENALTY, O_GRAVESTONE,

		O_BLADDER_1, O_BLADDER_2, O_BLADDER_3, O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7, O_BLADDER_8, O_BLADDER_9,
		O_COW_ENCLOSED_1, O_COW_ENCLOSED_2, O_COW_ENCLOSED_3, O_COW_ENCLOSED_4, O_COW_ENCLOSED_5, O_COW_ENCLOSED_6, O_COW_ENCLOSED_7,

		O_WATER_1, O_WATER_2, O_WATER_3, O_WATER_4, O_WATER_5, O_WATER_6, O_WATER_7, O_WATER_8,
		O_WATER_9, O_WATER_10, O_WATER_11, O_WATER_12, O_WATER_13, O_WATER_14, O_WATER_15, O_WATER_16,

		O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3, O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7, O_SPACE,
		O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4, O_SPACE, O_SPACE, O_SPACE, O_SPACE,
		
		O_EXPLODE_1, O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5, O_SPACE,
		O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4, O_PRE_DIA_5, O_SPACE, O_SPACE, O_SPACE, O_SPACE, O_SPACE,
		O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4, O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
		O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4, O_GHOST_EXPL_1, O_GHOST_EXPL_2, O_GHOST_EXPL_3, O_GHOST_EXPL_4,


	};
		
	int cols=16;
	int i, result;
	GList *areas=NULL;
	GtkWidget *dialog, *expander, *table, *table2, *align;
	int into_second=0;

	/* elements dialog with no buttons; clicking on an element will do the trick. */
    dialog=gtk_dialog_new();
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_widget_get_toplevel(button)));
    gtk_window_set_title(GTK_WINDOW(dialog), _("Elements"));
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	align=gtk_alignment_new(0, 0.5, 0, 0);
	gtk_container_add(GTK_CONTAINER(align), gtk_label_new(_("Normal elements")));
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), align);
	
    table=gtk_table_new(0, 0, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 6);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), table);
    
    expander=gtk_expander_new(_("For effects"));
    table2=gtk_table_new(0, 0, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(table2), 6);
    gtk_container_add(GTK_CONTAINER(expander), table2);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), expander);

    /* create drawing areas */
    for (i=0; i<G_N_ELEMENTS(elements); i++) {
    	GtkWidget *da;
    	
		da=gtk_drawing_area_new();
		areas=g_list_prepend(areas, da);	/* put in list for animation timeout, that one will request redraw on them */
		gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK|GDK_LEAVE_NOTIFY_MASK|GDK_ENTER_NOTIFY_MASK);
        g_object_set_data(G_OBJECT(da), GDASH_ELEMENT, GINT_TO_POINTER(elements[i]));
		gtk_widget_set_size_request(da, gd_cell_size_editor+4, gd_cell_size_editor+4);
        gtk_widget_set_tooltip_text(da, _(gd_elements[elements[i]].name));
		g_signal_connect(G_OBJECT(da), "expose-event", G_CALLBACK(button_drawing_area_expose_event), GINT_TO_POINTER(elements[i]));
		g_signal_connect(G_OBJECT(da), "leave-notify-event", G_CALLBACK(button_drawing_area_crossing_event), NULL);
		g_signal_connect(G_OBJECT(da), "enter-notify-event", G_CALLBACK(button_drawing_area_crossing_event), NULL);
        g_signal_connect(G_OBJECT(da), "button-press-event", G_CALLBACK(element_button_da_clicked), dialog);
        if (elements[i]==O_DIRT2)
        	/* the dirt2 the first element to be put in the effect list; from that one, always use table2 */
        	into_second=i;
        if (!into_second)
	        gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(da), i%cols, i%cols+1, i/cols, i/cols+1);
	    else {
	    	int j=i-into_second;
	    	
	        gtk_table_attach_defaults(GTK_TABLE(table2), GTK_WIDGET(da), j%cols, j%cols+1, j/cols, j/cols+1);
	    }
	    
    }
    gtk_widget_show_all(dialog);
    g_timeout_add(40, redraw_timeout, areas);
	gtk_window_present_with_time(GTK_WINDOW(dialog), gtk_get_current_event_time());

    result=gtk_dialog_run(GTK_DIALOG(dialog));
    g_source_remove_by_user_data(areas);
    gtk_widget_destroy(dialog);
    g_list_free(areas);
    /* (if esc or delete event, do not change state) */
    if (result>=0)
		gd_element_button_set(button, (GdElement) result);
}

GdElement
gd_element_button_get (GtkWidget *button)
{
	gpointer element=g_object_get_data (G_OBJECT(button), GDASH_ELEMENT);
	return (GdElement)GPOINTER_TO_INT(element);
}

void
gd_element_button_update_pixbuf(GtkWidget *button)
{
	/* set the same as it already shows, but pixbuf update will be triggered by this */
	gd_element_button_set(button, gd_element_button_get(button));
}


GtkWidget *
gd_element_button_new (const GdElement initial_element)
{
	GtkWidget *button;

	button=gtk_button_new();
	gtk_widget_set_size_request(button, 128, -1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(element_button_clicked), NULL);
	gd_element_button_set(button, initial_element);
	gtk_button_set_alignment(GTK_BUTTON(button), 0, 0.5);
	return button;
}

#undef GDASH_CELL
#undef GDASH_ELEMENT
#undef GDASH_HOVER

enum 
{
  DIR_PIXBUF_COL,
  DIR_TEXT_COL,
  NUM_DIR_COLS
};

/* directions to be shown, and corresponding icons. */
static GdDirection shown_dirs[]= { MV_UP, MV_RIGHT, MV_DOWN, MV_LEFT };
static const char *dir_icons[]= { GTK_STOCK_GO_UP, GTK_STOCK_GO_FORWARD, GTK_STOCK_GO_DOWN, GTK_STOCK_GO_BACK };

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

		for (i=0; i<G_N_ELEMENTS(shown_dirs); i++) {
			gtk_list_store_append (store, &iter);
			pixbuf=gtk_widget_render_icon(cellview, dir_icons[i], GTK_ICON_SIZE_MENU, NULL);
			gtk_list_store_set (store, &iter, DIR_PIXBUF_COL, pixbuf, DIR_TEXT_COL, _(gd_direction_get_visible_name(shown_dirs[i])), -1);
		}

		gtk_widget_destroy(cellview);
	}

	/* create combo box and renderer for icon and text */
	combo=gtk_combo_box_new_with_model (GTK_TREE_MODEL(store));
    renderer=gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "pixbuf", DIR_PIXBUF_COL, NULL);
    renderer=gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", DIR_TEXT_COL, NULL);

	/* set to initial value */
	for (i=0; i<G_N_ELEMENTS(shown_dirs); i++)
		if (shown_dirs[i]==initial)
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);

	return combo;
}

GdDirection
gd_direction_combo_get(GtkWidget *combo)
{
	return (GdDirection) shown_dirs[gtk_combo_box_get_active(GTK_COMBO_BOX(combo))];
}


/*****************************************************/








GtkWidget *
gd_scheduling_combo_new(const GdScheduling initial)
{
	GtkWidget *combo;
	int i;
	
	combo=gtk_combo_box_new_text();

	for (i=0; i<GD_SCHEDULING_MAX; i++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _(gd_scheduling_name[i]));

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), initial);
	
	return combo;
}

GdScheduling
gd_scheduling_combo_get(GtkWidget *combo)
{
	return (GdScheduling)(gtk_combo_box_get_active(GTK_COMBO_BOX(combo)));
}

