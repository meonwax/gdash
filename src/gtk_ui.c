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
#include <string.h>
#include "gtk_gfx.h"
#include "caveset.h"
#include "settings.h"
#include "util.h"
#include "gtk_ui.h"
#include "config.h"

/* pixbufs of icons and the like */
#include "icons.h"
/* title image and icon */
#include "title.h"

static char *caveset_filename=NULL;
static char *last_folder=NULL;




void
gd_create_stock_icons (void)
{
	static struct {
		const guint8 *data;
		char *stock_id;
	} icons[]={
		{ cave_editor, GD_ICON_CAVE_EDITOR},
		{ move, GD_ICON_EDITOR_MOVE},
		{ add_join, GD_ICON_EDITOR_JOIN},
		{ add_freehand, GD_ICON_EDITOR_FREEHAND},
		{ add_point, GD_ICON_EDITOR_POINT},
		{ add_line, GD_ICON_EDITOR_LINE},
		{ add_rectangle, GD_ICON_EDITOR_RECTANGLE},
		{ add_filled_rectangle, GD_ICON_EDITOR_FILLRECT},
		{ add_raster, GD_ICON_EDITOR_RASTER},
		{ add_fill_border, GD_ICON_EDITOR_FILL_BORDER},
		{ add_fill_replace, GD_ICON_EDITOR_FILL_REPLACE},
		{ add_maze, GD_ICON_EDITOR_MAZE}, 
		{ add_maze_uni, GD_ICON_EDITOR_MAZE_UNI},
		{ add_maze_braid, GD_ICON_EDITOR_MAZE_BRAID},
		{ snapshot, GD_ICON_SNAPSHOT},
		{ restart_level, GD_ICON_RESTART_LEVEL},
		{ random_fill, GD_ICON_RANDOM_FILL},
		{ to_top, GD_ICON_TO_TOP},
		{ to_bottom, GD_ICON_TO_BOTTOM},
		{ award, GD_ICON_AWARD},
	};

	GtkIconFactory *factory;
	int i;

	factory=gtk_icon_factory_new ();
	for (i=0; i < G_N_ELEMENTS (icons); i++) {
		GtkIconSet *iconset;
		GdkPixbuf *pixbuf;

		pixbuf=gdk_pixbuf_new_from_inline (-1, icons[i].data, FALSE, NULL);
		iconset=gtk_icon_set_new_from_pixbuf (pixbuf);
		g_object_unref (pixbuf);
		gtk_icon_factory_add (factory, icons[i].stock_id, iconset);
	}
	gtk_icon_factory_add_default (factory);
	g_object_unref (factory);
}


GdkPixbuf *
gd_icon (void)
{
	return gdk_pixbuf_new_from_inline (-1, gdash, FALSE, NULL);
}

GdkPixbuf *
gd_title (void)
{
	return gdk_pixbuf_new_from_inline (-1, gdash_screen, FALSE, NULL);
}





/************
 *
 * functions for the settings window.
 * these implement the list view, which shows available graphics.
 *
 */
enum {
	THEME_COL_FILENAME,
	THEME_COL_NAME,
	THEME_COL_PIXBUF,
	NUM_THEME_COLS
};

GtkTreeIter*
gd_add_png_to_store (GtkListStore *store, const char *filename)
{
	GdkPixbuf *pixbuf, *cell_pixbuf;
	char *seen_name;
	int cell_size;
	int cell_num=gd_elements[O_PLAYER].image_game;	/* load the image of the player from every pixbuf */
	GError *error=NULL;
	static GtkTreeIter iter;
	gboolean valid;

	pixbuf=gdk_pixbuf_new_from_file (filename, &error);
	if (error) {
		/* unlikely but check it */
		g_warning("%s\n", error->message);
		g_error_free(error);
		return NULL;
	}

	cell_size=gdk_pixbuf_get_width (pixbuf) / NUM_OF_CELLS_X;
	cell_pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha(pixbuf), 8, cell_size, cell_size);
	gdk_pixbuf_copy_area(pixbuf, (cell_num % NUM_OF_CELLS_X) * cell_size, (cell_num / NUM_OF_CELLS_X) * cell_size, cell_size, cell_size, cell_pixbuf, 0, 0);
	g_object_unref(pixbuf);
	g_assert(error==NULL);
	
	/* check list store to find if this file is already added. may happen if a theme is overwritten by the add theme button */
	valid=gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	while (valid) {
		char *other_filename;
		
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, THEME_COL_FILENAME, &other_filename, -1);
		if (other_filename && filename && g_str_equal(other_filename, filename))
			valid=gtk_list_store_remove(store, &iter);
		else
			valid=gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
	
	/* add to list */
	seen_name=g_filename_display_basename(filename);
	if (strrchr(seen_name, '.'))	/* remove extension */
		*strrchr(seen_name, '.')='\0';
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, THEME_COL_FILENAME, filename, THEME_COL_NAME, seen_name, THEME_COL_PIXBUF, cell_pixbuf, -1);
	g_free(seen_name);
	g_object_unref(cell_pixbuf);
	return &iter;
}


static void
add_dir_to_store(GtkListStore *store, const char *dirname)
{
	GDir *dir;
	GError *error=NULL;
	/* search directory */
	dir=g_dir_open(dirname, 0, &error);
	if (!error) {
		const char *name;

		while ((name=g_dir_read_name(dir))) {
			char *filename=g_build_filename(dirname, name, NULL);

			/* if image file can be loaded and proper size for theme */
			if (gd_is_png_ok_for_theme(filename))
				gd_add_png_to_store(store, filename);
			g_free(filename);
		}
		g_dir_close(dir);
	} else
		g_warning("%s", error->message);
}

GtkListStore *
gd_create_themes_list()
{
	GtkListStore *store;
	GtkTreeIter iter;
	
	store=gtk_list_store_new(NUM_THEME_COLS, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);

	/* default builtin theme */
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, THEME_COL_FILENAME, NULL, THEME_COL_NAME, _("Default C64 theme"), THEME_COL_PIXBUF, gd_pixbuf_for_builtin, -1);

	add_dir_to_store(store, gd_system_data_dir);
	add_dir_to_store(store, gd_user_config_dir);
	
	return store;
}



/**************************** 
 *
 * settings window
 *
 */
struct {
	char *name;
	int size;
} static sizes[]={
//	{N_("Half"), 0.5},
	{N_("Original"), 1},
	{N_("Double"), 2},
	{N_("Triple"), 3},
};

typedef struct pref_info {
	GtkWidget *treeview, *sizecombo;
	GtkWidget *image;
} pref_info;

static void
update(pref_info* info)
{
	GdkPixbuf *pb;
	GtkTreeIter iter;
	GtkTreeModel *model;
	int size;
	
	if (!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(info->treeview)), &model, &iter))
		return;
	gtk_tree_model_get(model, &iter, THEME_COL_PIXBUF, &pb, -1);
	size=gdk_pixbuf_get_width(pb)*sizes[gtk_combo_box_get_active(GTK_COMBO_BOX(info->sizecombo))].size;
	pb=gdk_pixbuf_scale_simple(pb, size, size, gd_gfx_interpolation?GDK_INTERP_BILINEAR:GDK_INTERP_NEAREST);
	if (gd_tv_emulation)
		gd_tv_pixbuf(pb);
	
	gtk_image_set_from_pixbuf(GTK_IMAGE(info->image), pb);
	g_object_unref(pb);
}

static void
changed (gpointer object, gpointer data)
{
	update((pref_info *)data);
}

static void
save (GtkWidget *widget, gpointer data)
{
	gboolean *bl=(gboolean *)data;
	
	*bl=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void
add_theme_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	int result;
	char *filename=NULL;
	pref_info *info=(pref_info *)data;
	
	dialog=gtk_file_chooser_dialog_new (_("Add Theme from PNG Image"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	filter=gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("PNG files"));
	gtk_file_filter_add_pattern (filter, "*.png");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	result=gtk_dialog_run (GTK_DIALOG (dialog));
	if (result==GTK_RESPONSE_ACCEPT)
		filename=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	/* read the state of the check button */
	gtk_widget_destroy (dialog);

	if (filename) {
		if (gd_is_png_ok_for_theme(filename)) {
			/* make up new filename */
			char *basename, *new_filename;
			GError *error=NULL;
			gchar *contents;
			gsize length;

			basename=g_path_get_basename(filename);
			new_filename=g_build_path(G_DIR_SEPARATOR_S, gd_user_config_dir, basename, NULL);
			g_free(basename);

			/* if file not exists, or exists BUT overwrite allowed */			
			if (!g_file_test(new_filename, G_FILE_TEST_EXISTS) || gd_ask_overwrite(NULL, new_filename)) {
				/* copy theme to user config directory */
				if (g_file_get_contents(filename, &contents, &length, &error) && g_file_set_contents(new_filename, contents, length, &error)) {
					GtkTreeIter *iter;
					
					iter=gd_add_png_to_store (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(info->treeview))), new_filename);
					if (iter)						/* select newly added. if there was an error, the old selection was not cleared */
						gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(info->treeview)), iter);
				}
				else {
					gd_errormessage(error->message, NULL);
					g_error_free(error);
				}
			}
			g_free(new_filename);
		}
		else
			gd_errormessage(_("The PNG image cannot be used as a GDash theme."), NULL);
	}
	g_free(filename);
}

void
gd_preferences (GtkWidget *parent)
{
	static struct {
		char *name;
		char *description;
		gboolean *value;
		gboolean update;			/* if sample pixbuf should be updated when clicking this option */
	} check_buttons[]={
		{N_("<b>Cave options</b>"), NULL, NULL},
		{N_("Ease levels"), N_("Make caves easier to play. Only one diamonds to collect, more time available, no expanding walls... This might render some caves unplayable!"), &gd_easy_play},
		{N_("All caves selectable"), N_("All caves can be selected at game start."), &gd_all_caves_selectable, FALSE},
		{N_("Mouse play (experimental!)"), N_("Use the mouse to play. The player will follow the cursor!"), &gd_mouse_play, FALSE},
		{N_("Use BDCFF highscore"), N_("Use BDCFF highscores. GDash rather stores highscores in its own configuration directory, instead of saving them in "
			" the *.bd file."), &gd_use_bdcff_highscore, FALSE},
		{N_("<b>Display options</b>"), NULL, NULL},
		{N_("Time as min:sec"), N_("Show times in minutes and seconds, instead of seconds only."), &gd_time_min_sec, FALSE},
		{N_("Random colors"), N_("Use randomly selected colors for C64 graphics."), &gd_random_colors, FALSE},
/*
   XXX currently dirt mod is not shown to the user.
		{N_("Allow dirt mod"), N_("Enable caves to use alternative dirt graphics. This applies only to imported caves, not BDCFF (*.bd) files."), &allow_dirt_mod, FALSE},
*/
		{N_("TV emulation"), N_("Use TV emulated graphics, ie. lines are striped."), &gd_tv_emulation, TRUE},
		{N_("Interpolation"), N_("Use interpolation for scaling of graphics."), &gd_gfx_interpolation, TRUE},
#ifdef GD_SOUND
		{N_("<b>Sound options</b>"), NULL, NULL},
		{N_("Sound"), N_("Play sounds. Enabling this setting requires a restart!"), &gd_sdl_sound, FALSE},
		{N_("16-bit mixing"), N_("Use 16-bit mixing of sounds. Try changing this setting if sound is clicky. Changing this setting requires a restart!"), &gd_sdl_16bit_mixing, FALSE},
#endif
	};

	GtkWidget *dialog, *table, *label, *button;
	pref_info info;
	int i, row;
	GtkWidget *sw, *align, *hbox;
	GtkListStore *store;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	char *filename;
	char *text;
	
	
	dialog=gtk_dialog_new_with_buttons (_("GDash Preferences"), GTK_WINDOW (parent), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);

	button=gtk_button_new_with_mnemonic(_("_Add theme"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	hbox=gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox);
	table=gtk_table_new (1, 1, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, 0, 0, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);

	/* game booleans */
	row=0;
	for (i=0; i<G_N_ELEMENTS(check_buttons); i++) {
		if (check_buttons[i].value) {
			GtkWidget *widget;
			
			widget=gtk_check_button_new_with_label (_(check_buttons[i].name));
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), *check_buttons[i].value);
			gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, row, row + 1);
			g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(save), check_buttons[i].value);
			/* if sample pixbuf should be updated */
			if (check_buttons[i].update)
				g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(changed), &info);
		} else {
			label=gtk_label_new (NULL);
			gtk_label_set_markup(GTK_LABEL(label), _(check_buttons[i].name));
			gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
			gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 2, row, row + 1);
		}		
		row ++;
	}
	
	/* gfx */
	store=gd_create_themes_list();
	info.treeview=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
		do {
			char *filename;
			
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, THEME_COL_FILENAME, &filename, -1);
			if ((!filename && !gd_theme) || (gd_theme && filename && g_str_equal(gd_theme, filename))) {
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(info.treeview)), &iter);
			}
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
	text=g_strdup_printf(_("This is the list of available themes. You can put .png files into the directory %s to see more."), gd_user_config_dir);
	gtk_widget_set_tooltip_text(info.treeview, text);
	g_free(text);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(info.treeview), FALSE);	/* don't need headers as everything is self-explaining */
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(info.treeview), -1, "", gtk_cell_renderer_pixbuf_new(), "pixbuf", THEME_COL_PIXBUF, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(info.treeview), -1, "", gtk_cell_renderer_text_new(), "text", THEME_COL_NAME, NULL);
	selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(info.treeview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(add_theme_cb), &info);

	sw=gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(sw), info.treeview);

	table=gtk_table_new (1, 1, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, 0, 0, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);

	label=gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Theme</b>"));
	gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
	gtk_table_attach(GTK_TABLE (table), label, 0, 2, 0, 1, GTK_EXPAND|GTK_FILL, 0, 0, 0);

	gtk_table_attach_defaults(GTK_TABLE(table), sw, 1, 2, 1, 2);
	
	/* cells size combo */
	info.sizecombo=gtk_combo_box_new_text();
	for (i=0; i<G_N_ELEMENTS(sizes); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(info.sizecombo), gettext(sizes[i].name));
		if (gd_cell_scale==sizes[i].size)
			gtk_combo_box_set_active(GTK_COMBO_BOX(info.sizecombo), i);
	}
	gtk_table_attach(GTK_TABLE(table), info.sizecombo, 1, 2, 2, 3, GTK_EXPAND|GTK_FILL, 0, 0, 0);
	
	align=gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_set_size_request(align, 64, 64);
	info.image=gtk_image_new();
	gtk_container_add(GTK_CONTAINER(align), info.image);
	gtk_table_attach(GTK_TABLE(table), align, 1, 2, 3, 4, GTK_EXPAND|GTK_FILL, 0, 0, 0);
	
	g_signal_connect(G_OBJECT(info.sizecombo), "changed", G_CALLBACK(changed), &info);
	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(info.treeview))), "changed", G_CALLBACK(changed), &info);
	update (&info);
	
	gtk_widget_show_all (dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	if (!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(info.treeview)), NULL, &iter))
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, THEME_COL_FILENAME, &filename, -1);
	if (filename) {
		if (gd_loadcells_file(filename)) {
			/* if successful, remember theme setting */
			g_free(gd_theme);
			gd_theme=g_strdup(filename);
		} else {
			/* do nothing */
			g_warning("%s: unable to load theme", filename);
		}
	}
	else {
		/* no filename means the builtin */
		gd_loadcells_default();
		g_free(gd_theme);
		gd_theme=NULL;
	}
	gd_cell_scale=sizes[gtk_combo_box_get_active(GTK_COMBO_BOX(info.sizecombo))].size;

	gtk_widget_destroy (dialog);
}















enum {
	HS_COLUMN_RANK,
	HS_COLUMN_NAME,
	HS_COLUMN_SCORE,
	HS_COLUMN_BOLD,
	NUM_COLUMNS,
};

/* a cave name is selected, update the list store with highscore data */
#define GD_LISTSTORE "gd-liststore-for-combo"
#define GD_HIGHLIGHT "gd-highlight"
static void
hs_cave_combo_changed(GtkComboBox *widget, gpointer data)
{
	Cave *cave;
	GtkListStore *store=GTK_LIST_STORE(g_object_get_data(G_OBJECT(widget), GD_LISTSTORE));
	GdHighScore *to_highlight=(GdHighScore *)g_object_get_data(G_OBJECT(widget), GD_HIGHLIGHT);
	GList *hiter;
	int i;
	
	gtk_list_store_clear(store);
	i=gtk_combo_box_get_active(widget);
	if (i==0)
		cave=gd_default_cave;
	else
		cave=gd_return_nth_cave(i-1);
	
	for(hiter=cave->highscore, i=1; hiter!=NULL; hiter=hiter->next, i++) {
		GtkTreeIter iter;
		GdHighScore *hs=hiter->data;
		
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, HS_COLUMN_RANK, i, HS_COLUMN_NAME, hs->name, HS_COLUMN_SCORE, hs->score,
			HS_COLUMN_BOLD, hs==to_highlight?PANGO_WEIGHT_BOLD:PANGO_WEIGHT_NORMAL, -1);
	}
}

static void
hs_clear_highscore(GtkWidget *widget, gpointer data)
{
	GtkComboBox *combo=GTK_COMBO_BOX(data);
	GtkListStore *store=GTK_LIST_STORE(g_object_get_data(G_OBJECT(combo), GD_LISTSTORE));
	Cave *cave;
	int i;
	
	i=gtk_combo_box_get_active(combo);
	if (i==0)
		cave=gd_default_cave;
	else
		cave=gd_return_nth_cave(i-1);

	/* if there is any entry, delete */
	if (cave->highscore)
		gd_cave_clear_highscore(cave);
	gtk_list_store_clear(store);		
}
	
void
gd_show_highscore(GtkWidget *parent, Cave *cave, gboolean show_clear_button, gpointer highlight)
{
	GtkWidget *dialog;
	int i;
	char *text;
	GtkListStore *store;
	GtkWidget *treeview, *sw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *combo;
	GList *iter;

	/* dialog window */
	dialog=gtk_dialog_new_with_buttons(_("Highscores"), GTK_WINDOW(parent), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL);
	
	store=gtk_list_store_new(NUM_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
	treeview=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

	renderer=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes(_("Rank"), renderer, "text", HS_COLUMN_RANK, "weight", HS_COLUMN_BOLD, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	renderer=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", HS_COLUMN_NAME, "weight", HS_COLUMN_BOLD, NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	renderer=gtk_cell_renderer_text_new();
	column=gtk_tree_view_column_new_with_attributes(_("Score"), renderer, "text", HS_COLUMN_SCORE, "weight", HS_COLUMN_BOLD, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	combo=gtk_combo_box_new_text();
	g_object_set_data(G_OBJECT(combo), GD_LISTSTORE, store);
	g_object_set_data(G_OBJECT(combo), GD_HIGHLIGHT, highlight);
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(hs_cave_combo_changed), NULL);
	text=g_strdup_printf("[%s]", gd_default_cave->name);
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text);
	if (!cave || cave==gd_default_cave)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	g_free(text);

	for (iter=gd_caveset, i=1; iter!=NULL; iter=iter->next, i++) {
		Cave *c=iter->data;

		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), c->name);

		/* if this one is the active, select it */
		if (c==cave)
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
	}

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), combo, FALSE, FALSE, 6);
	sw=gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(sw), treeview);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), sw);

	/* clear button */
	if (show_clear_button) {
		GtkWidget *button;
		
		button=gtk_button_new_from_stock(GTK_STOCK_CLEAR);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(hs_clear_highscore), combo);
	}

	/* close button */
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);

	gtk_window_set_default_size(GTK_WINDOW(dialog), 240, 320);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
#undef GD_LISTSTORE
#undef GD_HIGHLIGHT

/*
 * show a warning window
 */
static void
show_message (GtkMessageType type, const char *primary, const char *secondary)
{
    GtkWidget *dialog;

    dialog=gtk_message_dialog_new (NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        type, GTK_BUTTONS_OK,
        primary);
    gtk_window_set_title(GTK_WINDOW(dialog), "GDash");
   	/* secondary message exists an is not empty string: */
    if (secondary && secondary[0]!=0)
        gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), secondary);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

void
gd_warningmessage (const char *primary, const char *secondary)
{
	show_message (GTK_MESSAGE_WARNING, primary, secondary);
}

void
gd_errormessage (const char *primary, const char *secondary)
{
	show_message (GTK_MESSAGE_ERROR, primary, secondary);
}

void
gd_infomessage (const char *primary, const char *secondary)
{
	show_message (GTK_MESSAGE_INFO, primary, secondary);
}

/* if necessary, ask the user if he doesn't want to save changes to cave */
gboolean
gd_discard_changes (GtkWidget *parent)
{
	GtkWidget *dialog, *button;
	gboolean discard;

	/* save highscore on every ocassion when the caveset is to be removed from memory */
	gd_save_highscore(gd_user_config_dir);

	/* caveset is not edited, so pretend user confirmed */
	if (!gd_caveset_edited)
		return TRUE;

	dialog=gtk_message_dialog_new (GTK_WINDOW(parent), 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("Cave set is edited. Discard changes?"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), gd_default_cave->name);
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
	/* create a discard button with a trash icon and Discard text */
	button=gtk_button_new_with_label (_("_Discard"));
	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_stock (GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON));
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_YES);

	discard=gtk_dialog_run (GTK_DIALOG (dialog))==GTK_RESPONSE_YES;
	gtk_widget_destroy (dialog);

	/* return button pressed */
	return discard;
}

gboolean
gd_ask_overwrite(GtkWidget *parent, const char *filename)
{
	gboolean result;
	/* if exists, ask if overwrite */
	GtkWidget *dialog=gtk_message_dialog_new ((GtkWindow *)parent, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
		_("The file already exists. Do you want to overwrite it?"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), _("The file (%s) already exists, and will be overwritten."), filename);

	/* if file should be overwritten, change name */
	result=gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_YES;
	gtk_widget_destroy (dialog);
	return result;
}



/* save caveset to specified directory, and pop up error message if failed */
static void
caveset_save (const gchar *filename)
{
	gboolean saved;
	
	saved=gd_caveset_save(filename);
	if (!saved)
		gd_show_last_error();
	else {
		/* save successful, so remember filename */
		/* first we make a copy, as it is possible that filename==caveset_filename (the pointers!) */
		char *stored=g_strdup(filename);
		g_free(caveset_filename);
		caveset_filename=stored;
	}
	
}


void
gd_save_caveset_as_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	char *filename=NULL, *suggested_name;

	dialog=gtk_file_chooser_dialog_new (_("Save File As"), NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	
	filter=gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("BDCFF cave sets (*.bd)"));
	gtk_file_filter_add_pattern (filter, "*.bd");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	filter=gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files (*)"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
	
	suggested_name=g_strdup_printf("%s.bd", gd_default_cave->name);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), suggested_name);
	g_free(suggested_name);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		filename=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);

	/* check if .bd extension should be added */
	if (filename) {
		char *suffixed;
		
		/* if it has no .bd extension, add one */
		if (!g_str_has_suffix(filename, ".bd")) {
			suffixed=g_strdup_printf("%s.bd", filename);
			
			g_free(filename);
			filename=suffixed;
		}
	}

	/* if we have a filename, do the save */
	if (filename) {
		if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
			/* if exists, ask if overwrite */
			if (gd_ask_overwrite(NULL, filename))
				caveset_save(filename);
		} else
			/* if did not exist, simply save */
			caveset_save(filename);
	}
	g_free(filename);
}

void
gd_save_caveset_cb (GtkWidget *widget, gpointer data)
{
	if (!caveset_filename) {
		/* if no filename remembered, rather start the save_as function, which asks for one. */
		gd_save_caveset_as_cb (widget, NULL);
		return;
	}

	caveset_save (caveset_filename);
}





void
gd_open_caveset (GtkWidget *window, const char *directory)
{
	GtkWidget *dialog, *check;
	GtkFileFilter *filter;
	int result;
	char *filename=NULL;
	gboolean highscore_load_from_bdcff;
	
	/* if caveset is edited, and user does not want to forget it */
	if (!gd_discard_changes(window))
		return;

	dialog=gtk_file_chooser_dialog_new (_("Open File"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	check=gtk_check_button_new_with_mnemonic(_("Load _highscores from BDCFF file"));
	
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), check, FALSE, FALSE, 6);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), gd_use_bdcff_highscore);
	gtk_widget_show(check);

	filter=gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("GDash cave sets"));
	gtk_file_filter_add_pattern (filter, "*.bd");
	gtk_file_filter_add_pattern (filter, "*.gds");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	/* if callback shipped with a directory name, show that directory by default */
	if (directory)
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), directory);
	else
	if (last_folder)
		/* if we previously had an open command, the directory was remembered */
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), last_folder);
	else
		/* otherwise user home */
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());

	result=gtk_dialog_run (GTK_DIALOG (dialog));
	if (result==GTK_RESPONSE_ACCEPT) {
		filename=gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		g_free (last_folder);
		last_folder=gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
	}
	/* read the state of the check button */
	highscore_load_from_bdcff=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
	gtk_widget_destroy (dialog);

	if (filename) {
		g_free(caveset_filename);
		caveset_filename=NULL;
		if (g_str_has_suffix(filename, ".bd"))
			caveset_filename=g_strdup(filename);

		gd_caveset_load_from_file (filename, gd_user_config_dir);

		/* if successful loading and this is a bd file, and we load highscores from our own config dir */
		if (!gd_has_new_error() && g_str_has_suffix(filename, ".bd") && !highscore_load_from_bdcff)
			gd_load_highscore(gd_user_config_dir);

	}
	g_free (filename);

	if (gd_has_new_error())
		gd_show_last_error();
}

/* load an internal caveset, after checking that the current one is to be saved or not */
void
gd_load_internal(GtkWidget *parent, int i)
{
	if (gd_discard_changes(parent)) {
		g_free(caveset_filename);
		caveset_filename=NULL;	/* forget cave filename, as this one is not loaded from a file... */

		gd_caveset_load_from_internal (i, gd_user_config_dir);
		gd_infomessage(_("Loaded game:"), gd_default_cave->name);
	}
}



/* set a label's markup */
static void
label_set_markup_vprintf(GtkLabel *label, const char *format, va_list args)
{
	char *text;
	
	text=g_strdup_vprintf(format, args);
	gtk_label_set_markup(label, text);
	g_free(text);
}

/* create a label, and set its text by a printf; the text will be centered */
GtkWidget *
gd_label_new_printf_centered(const char *format, ...)
{
	va_list args;
	GtkWidget *label;
	
	label=gtk_label_new(NULL);

	va_start(args, format);
	label_set_markup_vprintf(GTK_LABEL(label), format, args);
	va_end(args);

	return label;
}

/* create a label, and set its text by a printf */
GtkWidget *
gd_label_new_printf(const char *format, ...)
{
	va_list args;
	GtkWidget *label;
	
	label=gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	va_start(args, format);
	label_set_markup_vprintf(GTK_LABEL(label), format, args);
	va_end(args);

	return label;
}

/* set a label's markup with a printf format string */
void
gd_label_set_markup_printf(GtkLabel *label, const char *format, ...)
{
	va_list args;
	
	va_start(args, format);
	label_set_markup_vprintf(label, format, args);
	va_end(args);
}



void
gd_show_errors ()
{
	/* create text buffer */
	GtkTextIter iter;
	GtkTextBuffer *buffer=gtk_text_buffer_new(NULL);
	GtkWidget *dialog, *sw, *view;
	GList *liter;
	int result;
	GdkPixbuf *pixbuf_error, *pixbuf_warning, *pixbuf_info;
	
	dialog=gtk_dialog_new_with_buttons (_("GDash Errors"), NULL, GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CLEAR, 1, GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 512, 384);
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), sw);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* get text and show it */
	view=gtk_text_view_new_with_buffer (buffer);
	gtk_container_add (GTK_CONTAINER (sw), view);
	g_object_unref (buffer);

	pixbuf_error=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU, NULL);
	pixbuf_warning=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU, NULL);
	pixbuf_info=gtk_widget_render_icon(view, GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU, NULL);
	for (liter=gd_errors; liter!=NULL; liter=liter->next) {
		GdErrorMessage *error=liter->data;
		
		gtk_text_buffer_get_iter_at_offset(buffer, &iter, -1);
		if (error->flags>=G_LOG_LEVEL_MESSAGE)
			gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_info);
		else
		if (error->flags<G_LOG_LEVEL_WARNING)
			gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_error);
		else
			gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf_warning);
		gtk_text_buffer_insert(buffer, &iter, error->message, -1);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	}
	g_object_unref(pixbuf_error);
	g_object_unref(pixbuf_warning);
	g_object_unref(pixbuf_info);

	/* set some tags */
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
	gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (view), 3);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 6);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view), 6);
	gtk_widget_show_all (dialog);
	result=gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	/* clear errors */
	if (result==1)
		gd_clear_errors();
}

void
gd_show_last_error()
{
    GtkWidget *dialog;
    int result;
    GdErrorMessage *m;
    
    if (!gd_errors)
    	return;
    	
    /* set new error flag to false, as the user now knows that some error has happened */
	gd_clear_error_flag();
	
	m=g_list_last(gd_errors)->data;

    dialog=gtk_message_dialog_new (NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
        m->message);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog), _("_Show all"), 1, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    gtk_window_set_title(GTK_WINDOW(dialog), "GDash");
    result=gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (result==1)
    	/* user requested to show all errors */
    	gd_show_errors();
}

