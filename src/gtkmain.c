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
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "cave.h"
#include "cavedb.h"
#include "caveengine.h"
#include "caveobject.h"
#include "cavesound.h"
#include "caveset.h"
#include "c64import.h"
#include "editor.h"
#include "config.h"
#include "gtkgfx.h"
#include "help.h"
#include "settings.h"
#include "gtkui.h"
#include "util.h"
#include "gameplay.h"
#include "editorexport.h"
#include "about.h"
#include "sound.h"

#include "gtkmain.h"

/* game info */
static gboolean paused=FALSE;
static gboolean fast_forward=FALSE;
static gboolean fullscreen=FALSE;
	
static gboolean restart;

typedef struct _gd_main_window {
	GtkWidget *window;
	GtkActionGroup *actions_normal, *actions_title, *actions_game, *actions_snapshot;
	GtkWidget *scroll_window;
	GtkWidget *drawing_area, *title_image;	   /* two things that could be drawn in the main window */
	GdkPixmap **title_pixmaps;
	GtkWidget *labels;		/* parts of main window which have to be shown or hidden */
	GtkWidget *label_score, *label_value, *label_time, *label_cave_name, *label_diamonds, *label_skeletons, *label_lives;		/* different text labels in main window */
	GtkWidget *label_key1, *label_key2, *label_key3, *label_gravity_will_change;
	GtkWidget *label_variables;
	GtkWidget *error_hbox, *error_label;
	GtkWidget *menubar, *toolbar;
	GtkWidget *replay_image_align;
} GDMainWindow;

static GDMainWindow main_window;

static gboolean key_lctrl=FALSE, key_rctrl=FALSE, key_right=FALSE, key_up=FALSE, key_down=FALSE, key_left=FALSE, key_suicide=FALSE;
static int mouse_cell_x=-1, mouse_cell_y=-1, mouse_cell_click=0;

static void init_mainwindow(Cave *);
static void install_game_timer();

Cave *snapshot=NULL;






static gboolean
fullscreen_idle_func(gpointer data)
{
	gtk_window_fullscreen(GTK_WINDOW(data));
	return FALSE;
}

/* set or unset fullscreen if necessary */
/* hack: gtk-win32 does not correctly handle fullscreen & removing widgets.
   so we put fullscreening call into a low priority idle function, which will be called
   after all window resizing & the like did take place. */
void
set_fullscreen(void)
{
	if (gd_gameplay.cave && fullscreen) {
		gtk_widget_hide(main_window.menubar);
		gtk_widget_hide(main_window.toolbar);
		g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc) fullscreen_idle_func, main_window.window, NULL);
	}
	else {
		g_idle_remove_by_data (main_window.window);
		gtk_window_unfullscreen (GTK_WINDOW(main_window.window));
		gtk_widget_show(main_window.menubar);
		gtk_widget_show(main_window.toolbar);
	}
}


/*
	uninstall game timers, if any installed.
*/
static void
uninstall_game_timeout()
{
	/* remove timeout associated to game play */
	while (g_source_remove_by_user_data (main_window.window)) {
		/* nothing */
	}
}

void
gd_main_stop_game()
{
	uninstall_game_timeout();
	init_mainwindow(NULL);
	gd_stop_game();
	set_fullscreen();
	/* if editor is active, go back to its window. */
	if (editor_window)
		gtk_window_present (GTK_WINDOW(editor_window));
}

void
gd_main_window_set_title()
{
	if (!g_str_equal(gd_caveset_data->name, "")) {
		char *text;

		text=g_strdup_printf("GDash - %s", gd_caveset_data->name);
		gtk_window_set_title (GTK_WINDOW(main_window.window), text);
		g_free (text);
	}
	else
		gtk_window_set_title (GTK_WINDOW(main_window.window), "GDash");
}





/* game over, and highscore is achieved. ask for name, and if given, record it! */
static void
game_over_highscore()
{
	char *text;
	int rank;

	text=g_strdup_printf(_("You have %d points, and achieved a highscore."), gd_gameplay.player_score);
	gd_infomessage(_("Game over!"), text);
	g_free(text);

	/* enter to highscore table */
	rank=gd_add_highscore(gd_caveset_data->highscore, gd_gameplay.player_name, gd_gameplay.player_score);
	gd_show_highscore(main_window.window, NULL, FALSE, NULL, rank);
}

static void
game_over_without_highscore()
{
	gchar *text;

	text=g_strdup_printf(_("You have %d points."), gd_gameplay.player_score);
	gd_infomessage(_("Game over!"), text);
	g_free(text);

}



/* this starts a new game */
static void
new_game(const char *player_name, const int cave, const int level)
{
	gd_new_game(player_name, cave, level);
	install_game_timer();
}


void
gd_main_new_game_snapshot(Cave *snapshot)
{
	gd_new_game_snapshot(snapshot);
	install_game_timer();
}

void
gd_main_new_game_test(Cave *test, int level)
{
	gd_new_game_test(test, level);
	install_game_timer();
}


static void
gd_main_new_game_replay(Cave *cave, GdReplay *replay)
{
	gd_new_game_replay(cave, replay);
	install_game_timer();
}


/************************************************
 *
 *	EVENTS
 *
 */

/* closing the window by the window manager */
static gboolean
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
	if (gd_discard_changes(main_window.window))
		gtk_main_quit ();
	return TRUE;
}

/* keypress. key_* can be event_type==gdk_key_press, as we
   connected this function to key press and key release.
 */
static gboolean
keypress_event (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
	gboolean press=event->type==GDK_KEY_PRESS;	/* true for press, false for release */
	switch (event->keyval) {
	case GDK_Up:
		key_up=press;
		return TRUE;
	case GDK_Down:
		key_down=press;
		return TRUE;
	case GDK_Left:
		key_left=press;
		return TRUE;
	case GDK_Right:
		key_right=press;
		return TRUE;
	case GDK_Control_R:
		key_rctrl=press;
		return TRUE;
	case GDK_Control_L:
		key_lctrl=press;
		return TRUE;
	case GDK_F2:
		key_suicide=press;
		return TRUE;
	}
	return FALSE;	/* if any other key, we did not process it. go on, let gtk handle it. */
}

/* for mouse play. */
/* mouse leaves drawing area event */
/* if pointer not inside window, no control of player. */
static gboolean
drawing_area_leave_event (const GtkWidget * widget, const GdkEventCrossing * event, const gpointer data)
{
	mouse_cell_x=-1;
	mouse_cell_y=-1;
	mouse_cell_click=0;
	return TRUE;
}

/* mouse button press event */
static gboolean
drawing_area_button_event (const GtkWidget * widget, const GdkEventButton * event, const gpointer data)
{
	/* see if it is a click or a release. */
	mouse_cell_click=event->type == GDK_BUTTON_PRESS;
	return TRUE;
}

/* mouse motion event */
static gboolean
drawing_area_motion_event (const GtkWidget * widget, const GdkEventMotion * event, const gpointer data)
{
	int x, y;
	GdkModifierType state;

	if (event->is_hint)
		gdk_window_get_pointer (event->window, &x, &y, &state);
	else {
		x=event->x;
		y=event->y;
		state=event->state;
	}

	mouse_cell_x=x / gd_cell_size_game;
	mouse_cell_y=y / gd_cell_size_game;
	return TRUE;
}



/*
 * draws the cave during game play
 */
static void
drawcave()
{
	int x, y, xd, yd;

	if (!main_window.drawing_area->window)
		return;
	if (!gd_gameplay.cave)	/* if already no cave, just return */
		return;
	if (!gd_gameplay.gfx_buffer)	/* if already no cave, just return */
		return;

	/* x and y are cave coordinates, which might not start from 0, as the top or the left hand side of the cave might not be visible. */
	/* on the other hand, xd and yd are screen coordinates (of course, multiply by cell size) */
	/* inside the loop, calculate cave coordinates, which might not be equal to screen coordinates. */
	/* top left part of cave:
     *     x=0 xd=0
     *         | 
	 *y=0  wwww|wwwwwwwwwwwwww
	 *yd=0 ----+--------------
	 *     w...|o    p........   visible part
	 *     w...|..d...........
	 */
	for (y=gd_gameplay.cave->y1, yd=0; y<=gd_gameplay.cave->y2; y++, yd++) {
		for (x=gd_gameplay.cave->x1, xd=0; x<=gd_gameplay.cave->x2; x++, xd++) {
			if (gd_gameplay.gfx_buffer[y][x] & GD_REDRAW) {
				gd_gameplay.gfx_buffer[y][x] &= ~GD_REDRAW;
				gdk_draw_drawable(main_window.drawing_area->window, main_window.drawing_area->style->black_gc, gd_game_pixmap(gd_gameplay.gfx_buffer[y][x]),
								  0, 0, xd*gd_cell_size_game, yd*gd_cell_size_game, gd_cell_size_game, gd_cell_size_game);
			}
		}
	}
}



static gboolean
drawing_area_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	int x1, y1, x2, y2, xd, yd;

	if (!widget->window)
		return FALSE;
	if (!gd_gameplay.cave)	/* if already no cave, just return */
		return FALSE;
	if (!gd_gameplay.gfx_buffer)	/* if already no cave, just return */
		return FALSE;

	/* redraw entire area, not specific rectangles.
	 * this function gets called only when the main window gets exposed
	 * by the user removing another window. */
	/* these are screen coordinates. */
	x1=event->area.x / gd_cell_size_game;
	y1=event->area.y / gd_cell_size_game;
	x2=(event->area.x + event->area.width-1) / gd_cell_size_game;
	y2=(event->area.y + event->area.height-1) / gd_cell_size_game;

	/* run through screen coordinates to refresh. */
	/* inside the loop, calculate cave coordinates, which might not be equal to screen coordinates. */
	/* top left part of cave:
     *     x=0 xd=0
     *         | 
	 *y=0  wwww|wwwwwwwwwwwwww
	 *yd=0 ----+--------------
	 *     w...|o    p........   visible part
	 *     w...|..d...........
	 */
	/* mark all cells to be redrawn with GD_REDRAW, and then call the normal drawcave routine. */
	for (yd=y1; yd<=y2; yd++) {
		int y=yd+gd_gameplay.cave->y1;
		for (xd=x1; xd<=x2; xd++) {
			int x=xd+gd_gameplay.cave->x1;
			if (gd_gameplay.gfx_buffer[y][x]!=-1) {
				if (!(gd_gameplay.gfx_buffer[y][x] & GD_REDRAW))
					gd_gameplay.gfx_buffer[y][x] |= GD_REDRAW;
			}
		}
	}
	drawcave();

	return TRUE;
}

/* focus leaves game play window. remember that keys are not pressed!
	as we don't receive key released events along with focus out. */
static gboolean
focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	key_lctrl=FALSE;
	key_rctrl=FALSE;
	key_right=FALSE;
	key_up=FALSE;
	key_down=FALSE;
	key_left=FALSE;
	key_suicide=FALSE;
	
	return FALSE;
}

/**********************************-
 *
 *	CALLBACKS
 *
 */

static void
help_cb(GtkWidget * widget, gpointer data)
{
	gd_show_game_help (((GDMainWindow *)data)->window);
}

static void
preferences_cb(GtkWidget *widget, gpointer data)
{
	gd_preferences(((GDMainWindow *)data)->window);
}

static void
quit_cb(GtkWidget * widget, const gpointer data)
{
	gtk_main_quit();
}

void
stop_game_cb(GtkWidget *widget, gpointer data)
{
	gd_main_stop_game();
}

static void
save_snapshot_cb(GtkWidget * widget, gpointer data)
{
	if (snapshot)
		gd_cave_free(snapshot);
	
	snapshot=gd_create_snapshot();
	gtk_action_group_set_sensitive (main_window.actions_snapshot, snapshot!=NULL);
}

static void
load_snapshot_cb(GtkWidget * widget, gpointer data)
{
	g_return_if_fail(snapshot!=NULL);
	gd_main_new_game_snapshot(snapshot);
}

/* restart level button clicked */
static void
restart_level_cb(GtkWidget * widget, gpointer data)
{
	g_return_if_fail(gd_gameplay.cave!=NULL);
	/* sets the restart variable, which will be interpreted by the iterate routine */
	restart=TRUE;
}

static void
about_cb(GtkWidget *widget, gpointer data)
{
	gtk_show_about_dialog(GTK_WINDOW(main_window.window), "program-name", "GDash", "license", gd_about_license, "wrap-license", TRUE, "authors", gd_about_authors, "version", PACKAGE_VERSION, "comments", _(gd_about_comments), "translator-credits", _(gd_about_translator_credits), "website", gd_about_website, "artists", gd_about_artists, "documenters", gd_about_documenters, NULL);
}

static void
open_caveset_cb(GtkWidget * widget, gpointer data)
{
	gd_open_caveset(main_window.window, NULL);
	gd_main_window_set_title();
}

static void
open_caveset_dir_cb(GtkWidget * widget, gpointer data)
{
	gd_open_caveset(main_window.window, gd_system_caves_dir);
	gd_main_window_set_title();
}

static void
save_caveset_as_cb(GtkWidget * widget, gpointer data)
{
	gd_save_caveset_as(main_window.window);
}

static void
save_caveset_cb(GtkWidget * widget, gpointer data)
{
	gd_save_caveset(main_window.window);
}

/* load internal game from the executable. those are inlined in caveset.c. */
static void
load_internal_cb(GtkWidget * widget, gpointer data)
{
	gd_load_internal(main_window.window, GPOINTER_TO_INT(data));
	gd_main_window_set_title();
}

static void
toggle_pause_cb(GtkWidget * widget, gpointer data)
{
	paused=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION (widget));

	if (paused)
		gd_no_sound();
}

static void
toggle_fullscreen_cb (GtkWidget * widget, gpointer data)
{
	fullscreen=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION (widget));
	set_fullscreen();
}

static void
toggle_fast_cb (GtkWidget * widget, gpointer data)
{
	fast_forward=gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));
}

static void
showheader()
{
	const Cave *cave=gd_gameplay.cave;
	int time;
	
	/* show time */
	time=gd_cave_time_show(cave, cave->time);
	if (gd_time_min_sec)
		gd_label_set_markup_printf(GTK_LABEL(main_window.label_time), _("Time: <b>%02d:%02d</b>"), time/60, time%60);
	else
		gd_label_set_markup_printf(GTK_LABEL(main_window.label_time), _("Time: <b>%03d</b>"), time);

	/* lives reamining in game */
	switch (gd_gameplay.type) {
		case GD_GAMETYPE_NORMAL:
			if (!cave->intermission)
				gd_label_set_markup_printf(GTK_LABEL(main_window.label_lives), _("Lives: <b>%d</b>"), gd_gameplay.player_lives);
			else
				gd_label_set_markup_printf(GTK_LABEL(main_window.label_lives), _("<b>Bonus life</b>"));
			break;
		case GD_GAMETYPE_SNAPSHOT:
			gd_label_set_markup_printf(GTK_LABEL(main_window.label_lives), _("Continuing from <b>snapshot</b>"));
			break;
		case GD_GAMETYPE_TEST:
			gd_label_set_markup_printf(GTK_LABEL(main_window.label_lives), _("<b>Testing</b> cave"));
			break;
		case GD_GAMETYPE_REPLAY:
			gd_label_set_markup_printf(GTK_LABEL(main_window.label_lives), _("Playing <b>replay</b>"));
			break;
		case GD_GAMETYPE_CONTINUE_REPLAY:
			gd_label_set_markup_printf(GTK_LABEL(main_window.label_lives), _("Continuing <b>replay</b>"));
			break;
	}
	/* game score */
	gd_label_set_markup_printf(GTK_LABEL(main_window.label_score), _("Score: <b>%d</b>"), gd_gameplay.player_score);

	/* diamond value */
	gd_label_set_markup_printf(GTK_LABEL(main_window.label_value), _("Diamond value: <b>%d</b>"), cave->diamond_value);

	/* diamonds needed */
	if (cave->diamonds_needed>0)
		gd_label_set_markup_printf(GTK_LABEL(main_window.label_diamonds), _("Diamonds: <b>%d</b>"), cave->diamonds_collected>=cave->diamonds_needed?0:cave->diamonds_needed-cave->diamonds_collected);
	else
		/* did not already count diamonds */
		gd_label_set_markup_printf(GTK_LABEL(main_window.label_diamonds), _("Diamonds: <b>??""?</b>"));	/* "" to avoid C trigraph ??< */


	/* skeletons collected */
	gd_label_set_markup_printf(GTK_LABEL(main_window.label_skeletons), _("Skeletons: <b>%d</b>"), cave->skeletons_collected);

	/* keys label */
	gd_label_set_markup_printf(GTK_LABEL(main_window.label_key1), _("Key 1: <b>%d</b>"), cave->key1);
	gd_label_set_markup_printf(GTK_LABEL(main_window.label_key2), _("Key 2: <b>%d</b>"), cave->key2);
	gd_label_set_markup_printf(GTK_LABEL(main_window.label_key3), _("Key 3: <b>%d</b>"), cave->key3);

	/* gravity label */
	gd_label_set_markup_printf(GTK_LABEL(main_window.label_gravity_will_change), _("Gravity change: <b>%d</b>"), gd_cave_time_show(cave, cave->gravity_will_change));

	if (editor_window && gd_show_test_label) {
		gd_label_set_markup_printf(GTK_LABEL(main_window.label_variables),
								"Speed: %dms, Amoeba timer: %ds %d, %ds %d, Magic wall timer: %ds\n"
								"Expanding wall: %s, Creatures: %ds, %s, Gravity: %s\n"
								"Kill player: %s, Sweet eaten: %s, Diamond key: %s",
								cave->speed,
								gd_cave_time_show(cave, cave->amoeba_time),
								cave->amoeba_state,
								gd_cave_time_show(cave, cave->amoeba_2_time),
								cave->amoeba_2_state,
								gd_cave_time_show(cave, cave->magic_wall_time),
// XXX							cave->magic_wall_state,
								cave->expanding_wall_changed?"vertical":"horizontal",
								gd_cave_time_show(cave, cave->creatures_direction_will_change),
								cave->creatures_backwards?"backwards":"forwards",
								gd_direction_get_visible_name(cave->gravity_disabled?MV_STILL:cave->gravity),
								cave->kill_player?"yes":"no",
								cave->sweet_eaten?"yes":"no",
								cave->diamond_key_collected?"yes":"no"
								);
	}
}


/*
 * init mainwindow
 * creates title screen or drawing area
 *
 */
gboolean title_animation_func(gpointer data)
{
	static int animcycle=0;
	int count;

	/* count the number of frames. */
	count=0;
	while (main_window.title_pixmaps[count]!=NULL)
		count++;

	if (gtk_window_has_toplevel_focus (GTK_WINDOW(main_window.window))) {
		animcycle=(animcycle+1)%count;
		gtk_image_set_from_pixmap(GTK_IMAGE(main_window.title_image), main_window.title_pixmaps[animcycle], NULL);
	}
	return TRUE;
}

void title_animation_remove()
{
	int i;

	g_source_remove_by_user_data(title_animation_func);
	i=0;
	while (main_window.title_pixmaps[i]!=NULL) {
		g_object_unref(main_window.title_pixmaps[i]);
		i++;
	}
	g_free(main_window.title_pixmaps);
	main_window.title_pixmaps=NULL;
}

static void
init_mainwindow(Cave *cave)
{
	if (cave) {
		char *name_escaped;

		/* cave drawing */
		if (main_window.title_image)
			gtk_widget_destroy (main_window.title_image->parent);	/* bit tricky, destroy the viewport which was automatically added */

		if (!main_window.drawing_area) {
			GtkWidget *align;

			/* put drawing area in an alignment, so window can be any large w/o problems */
			align=gtk_alignment_new(0.5, 0.5, 0, 0);
			gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(main_window.scroll_window), align);

			main_window.drawing_area=gtk_drawing_area_new();
			gtk_widget_set_events (main_window.drawing_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_LEAVE_NOTIFY_MASK);
			g_signal_connect (G_OBJECT(main_window.drawing_area), "button_press_event", G_CALLBACK(drawing_area_button_event), NULL);
			g_signal_connect (G_OBJECT(main_window.drawing_area), "button_release_event", G_CALLBACK(drawing_area_button_event), NULL);
			g_signal_connect (G_OBJECT(main_window.drawing_area), "motion_notify_event", G_CALLBACK(drawing_area_motion_event), NULL);
			g_signal_connect (G_OBJECT(main_window.drawing_area), "leave_notify_event", G_CALLBACK(drawing_area_leave_event), NULL);
			g_signal_connect (G_OBJECT(main_window.drawing_area), "expose_event", G_CALLBACK(drawing_area_expose_event), NULL);
			g_signal_connect (G_OBJECT(main_window.drawing_area), "destroy", G_CALLBACK(gtk_widget_destroyed), &main_window.drawing_area);
			gtk_container_add (GTK_CONTAINER (align), main_window.drawing_area);
			if (gd_mouse_play)
				gdk_window_set_cursor (main_window.drawing_area->window, gdk_cursor_new(GDK_CROSSHAIR));
		}
		/* set the minimum size of the scroll window: 20*12 cells */
		/* XXX adding some pixels for the scrollbars-here we add 24 */
		gtk_widget_set_size_request(main_window.scroll_window, 20*gd_cell_size_game+24, 12*gd_cell_size_game+24);
		gtk_widget_set_size_request(main_window.drawing_area, (cave->x2-cave->x1+1)*gd_cell_size_game, (cave->y2-cave->y1+1)*gd_cell_size_game);

		/* show cave data */
		gtk_widget_show(main_window.labels);
		if (editor_window && gd_show_test_label)
			gtk_widget_show(main_window.label_variables);
		else
			gtk_widget_hide(main_window.label_variables);
		gtk_widget_hide(main_window.error_hbox);

		name_escaped=g_markup_escape_text(cave->name, -1);
		gd_label_set_markup_printf(GTK_LABEL(main_window.label_cave_name), _("<b>%s</b>, level %d"), name_escaped, cave->rendered);
		g_free(name_escaped);
	}
	else {
		if (main_window.drawing_area)
			/* parent is the align, parent of align is the viewport automatically added. */
			gtk_widget_destroy (main_window.drawing_area->parent->parent);

		if (!main_window.title_image) {
			int w, h;

			/* title screen */
			main_window.title_pixmaps=gd_create_title_animation();
			main_window.title_image=gtk_image_new();
			g_signal_connect (G_OBJECT(main_window.title_image), "destroy", G_CALLBACK(gtk_widget_destroyed), &main_window.title_image);
			g_signal_connect (G_OBJECT(main_window.title_image), "destroy", G_CALLBACK(title_animation_remove), NULL);
			g_timeout_add(40, title_animation_func, title_animation_func);
			gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(main_window.scroll_window), main_window.title_image);

			/* resize the scrolling window so the image fits - a bit larger than the image so scroll bars do not appear*/
			gdk_drawable_get_size(GDK_DRAWABLE(main_window.title_pixmaps[0]), &w, &h);
			gtk_widget_set_size_request(main_window.scroll_window, w+24, h+24);
		}

		/* hide cave data */
		gtk_widget_hide(main_window.labels);
		gtk_widget_hide(main_window.label_variables);
		if (gd_has_new_error()) {
			gtk_widget_show(main_window.error_hbox);
			gtk_label_set(GTK_LABEL(main_window.error_label), ((GdErrorMessage *)(g_list_last(gd_errors)->data))->message);
		} else {
			gtk_widget_hide(main_window.error_hbox);
		}
	}

	/* show newly created widgets */
	gtk_widget_show_all(main_window.scroll_window);

	/* set or unset fullscreen if necessary */
	set_fullscreen ();

	/* enable menus and buttons of game */
	gtk_action_group_set_sensitive(main_window.actions_title, cave==NULL && !editor_window);
	gtk_action_group_set_sensitive(main_window.actions_game, cave!=NULL);
	gtk_action_group_set_sensitive(main_window.actions_snapshot, snapshot!=NULL);
	/* if playing a cave or editor window exists, no music. */
	if (cave || editor_window)
		gd_music_stop();
	else
		gd_music_play_random();
	if (editor_window)
		gtk_widget_set_sensitive(editor_window, cave==NULL);
	gtk_widget_hide(main_window.replay_image_align);	/* it will be shown if needed. */
}

/* SCROLLING
 *
 * scrolls to the player during game play.
 */
static void
scroll()
{
	static int scroll_desired_x=0, scroll_desired_y=0;
	static int scroll_speed_x=0, scroll_speed_y=0;
	GtkAdjustment *adjustment;
	int scroll_center_x, scroll_center_y;
	gboolean out_of_window=FALSE;
	gboolean changed;
	int i;
	int player_x, player_y;
	const Cave *cave=gd_gameplay.cave;
	gboolean exact_scroll;
	int scroll_divisor;

	scroll_divisor=12;	/* some sort of scrolling speed */
	if (gd_fine_scroll)
		scroll_divisor*=2;	/* as fine scrolling is 50hz, whereas normal is 25hz only */

	/* if cave not yet rendered, return. (might be the case for 50hz scrolling */
	if (gd_gameplay.cave==NULL)
		return;	
	/* no scrolling when pause button is pressed */	
	if (paused)
		return;

	/* check player state. */
	switch (gd_gameplay.cave->player_state) {
		case GD_PL_NOT_YET:
			exact_scroll=TRUE;
			break;

		case GD_PL_LIVING:
		case GD_PL_EXITED:
			exact_scroll=FALSE;
			break;

		case GD_PL_TIMEOUT:
		case GD_PL_DIED:
			/* do not scroll when the player is dead or cave time is over. */
			return;	/* return from function */
	}

	player_x=cave->player_x-cave->x1;
	player_y=cave->player_y-cave->y1;
	/* hystheresis size is this, multiplied by two.
	 * so player can move half the window without scrolling. */
	int scroll_start_x=main_window.scroll_window->allocation.width/4;
	int scroll_to_x=main_window.scroll_window->allocation.width/8;
	int scroll_start_y=main_window.scroll_window->allocation.height/4;
	int scroll_to_y=main_window.scroll_window->allocation.height/8;

	/* get the size of the window so we know where to place player.
	 * first guess is the middle of the screen.
	 * main_window.drawing_area->parent->parent is the viewport.
	 * +cellsize/2 gets the stomach of player :) so the very center */
	scroll_center_x=player_x*gd_cell_size_game + gd_cell_size_game/2-main_window.drawing_area->parent->parent->allocation.width/2;
	scroll_center_y=player_y*gd_cell_size_game + gd_cell_size_game/2-main_window.drawing_area->parent->parent->allocation.height/2;

	/* HORIZONTAL */
	/* hystheresis function.
	 * when scrolling left, always go a bit less left than player being at the middle.
	 * when scrolling right, always go a bit less to the right. */
	adjustment=gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW(main_window.scroll_window));
	if (exact_scroll)
		scroll_desired_x=scroll_center_x;
	else {
		if (adjustment->value+scroll_start_x<scroll_center_x)
			scroll_desired_x=scroll_center_x-scroll_to_x;
		if (adjustment->value-scroll_start_x>scroll_center_x)
			scroll_desired_x=scroll_center_x+scroll_to_x;
	}
	scroll_desired_x=CLAMP(scroll_desired_x, 0, adjustment->upper-adjustment->step_increment-adjustment->page_increment);
	/* check if active player is outside drawing area. if yes, we should wait for scrolling */
	if ((player_x*gd_cell_size_game)<adjustment->value || (player_x*gd_cell_size_game+gd_cell_size_game-1)>adjustment->value+main_window.drawing_area->parent->parent->allocation.width)
		/* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
		if (cave->player_x>=cave->x1 && cave->player_x<=cave->x2)
			out_of_window=TRUE;

	/* adaptive scrolling speed.
	 * gets faster with distance.
	 * minimum speed is 1, to allow scrolling precisely to the desired positions (important at borders).
	 */
	if (scroll_speed_x<ABS(scroll_desired_x-adjustment->value)/scroll_divisor+1)
		scroll_speed_x++;
	if (scroll_speed_x>ABS(scroll_desired_x-adjustment->value)/scroll_divisor+1)
		scroll_speed_x--;
	changed=FALSE;
	if (adjustment->value<scroll_desired_x) {
		for (i=0; i<scroll_speed_x; i++)
			if ((int)adjustment->value<scroll_desired_x)
				adjustment->value++;
		changed=TRUE;
	}
	if (adjustment->value > scroll_desired_x) {
		for (i=0; i < scroll_speed_x; i++)
			if ((int)adjustment->value>scroll_desired_x)
				adjustment->value--;
		changed=TRUE;
	}
	if (changed)
		gtk_adjustment_value_changed (adjustment);

	/* VERTICAL */
	adjustment=gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(main_window.scroll_window));
	if (exact_scroll)
		scroll_desired_y=scroll_center_y;
	else {
		if (adjustment->value + scroll_start_y < scroll_center_y)
			scroll_desired_y=scroll_center_y-scroll_to_y;
		if (adjustment->value-scroll_start_y > scroll_center_y)
			scroll_desired_y=scroll_center_y + scroll_to_y;
	}
	scroll_desired_y=CLAMP(scroll_desired_y, 0, adjustment->upper-adjustment->step_increment-adjustment->page_increment);
	/* check if active player is outside drawing area. if yes, we should wait for scrolling */
	if ((player_y*gd_cell_size_game)<adjustment->value || (player_y*gd_cell_size_game+gd_cell_size_game-1)>adjustment->value+main_window.drawing_area->parent->parent->allocation.height)
		/* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
		if (cave->player_y>=cave->y1 && cave->player_y<=cave->y2)
			out_of_window=TRUE;

	if (scroll_speed_y<ABS(scroll_desired_y-adjustment->value)/scroll_divisor+1)
		scroll_speed_y++;
	if (scroll_speed_y>ABS(scroll_desired_y-adjustment->value)/scroll_divisor+1)
		scroll_speed_y--;
	changed=FALSE;
	if (adjustment->value < scroll_desired_y) {
		for (i=0; i < scroll_speed_y; i++)
			if ((int)adjustment->value < scroll_desired_y)
				adjustment->value++;
		changed=TRUE;
	}
	if (adjustment->value > scroll_desired_y) {
		for (i=0; i < scroll_speed_y; i++)
			if ((int)adjustment->value > scroll_desired_y)
				adjustment->value--;
		changed=TRUE;
	}
	if (changed)
		gtk_adjustment_value_changed (adjustment);

	/* remember if player is visible inside window */
	gd_gameplay.out_of_window=out_of_window;

	/* if not yet born, we treat as visible. so cave will run. the user is unable to control an unborn player, so this is the right behaviour. */
	if (gd_gameplay.cave->player_state==GD_PL_NOT_YET)
		gd_gameplay.out_of_window=FALSE;
}








/*
 * START NEW GAME DIALOG
 *
 * show a dialog to the user so he can select the cave to start game at.
 *
 */

typedef struct _gd_jump_dialog {
	GtkWidget *dialog;
	GtkWidget *combo_cave;
	GtkWidget *spin_level;
	GtkWidget *entry_name;

	GtkWidget *image;
} GDJumpDialog;

/* keypress. key_* can be event_type==gdk_key_press, as we
   connected this function to key press and key release.
 */
static gboolean
new_game_keypress_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GDJumpDialog *jump_dialog=(GDJumpDialog *)data;
	int level=gtk_range_get_value (GTK_RANGE(jump_dialog->spin_level));

	switch (event->keyval) {
	case GDK_Left:
		level--;
		if (level<1)
			level=1;
		gtk_range_set_value (GTK_RANGE(jump_dialog->spin_level), level);
		return TRUE;
	case GDK_Right:
		level++;
		if (level>5)
			level=5;
		gtk_range_set_value (GTK_RANGE(jump_dialog->spin_level), level);
		return TRUE;
	case GDK_Return:
		gtk_dialog_response(GTK_DIALOG(jump_dialog->dialog), GTK_RESPONSE_ACCEPT);
		return TRUE;
	}
	return FALSE;	/* if any other key, we did not process it. go on, let gtk handle it. */
}


/* update pixbuf */
static void
jump_cave_changed_signal(GtkWidget *widget, gpointer data)
{
	GDJumpDialog *jump_dialog=(GDJumpDialog *)data;
	GdkPixbuf *cave_image;
	Cave *cave;

	/* loading cave, draw cave and scale to specified size. seed=0 */
	cave=gd_cave_new_from_caveset(gtk_combo_box_get_active(GTK_COMBO_BOX (jump_dialog->combo_cave)), gtk_range_get_value (GTK_RANGE(jump_dialog->spin_level))-1, 0);
	cave_image=gd_drawcave_to_pixbuf(cave, 320, 240, TRUE);
	gtk_image_set_from_pixbuf(GTK_IMAGE (jump_dialog->image), cave_image);
	g_object_unref(cave_image);

	/* freeing temporary cave data */
	gd_cave_free(cave);
}

static void
new_game_cb (const GtkWidget * widget, const gpointer data)
{
	static GdString player_name="";
	GtkWidget *table, *expander, *eventbox;
	GDJumpDialog jump_dialog;
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GList *iter;

	/* check if caveset is empty! */
	if (gd_caveset_count()==0) {
		gd_warningmessage(_("There are no caves in this cave set!"), NULL);
		return;
	}

	jump_dialog.dialog=gtk_dialog_new_with_buttons(_("Select cave to play"), GTK_WINDOW(main_window.window), GTK_DIALOG_NO_SEPARATOR | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_JUMP_TO, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG(jump_dialog.dialog), GTK_RESPONSE_ACCEPT);
	gtk_window_set_resizable (GTK_WINDOW(jump_dialog.dialog), FALSE);

	table=gtk_table_new(0, 0, FALSE);
	gtk_box_pack_start(GTK_BOX (GTK_DIALOG(jump_dialog.dialog)->vbox), table, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER (table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	/* name, which will be used for highscore & the like */
	gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_printf(_("Name:")), 0, 1, 0, 1);
	if (g_str_equal(player_name, ""))
		gd_strcpy(player_name, g_get_real_name());
	jump_dialog.entry_name=gtk_entry_new();
	/* little inconsistency below: max length has unicode characters, while gdstring will have utf-8.
	   however this does not make too much difference */
	gtk_entry_set_max_length(GTK_ENTRY(jump_dialog.entry_name), sizeof(GdString));
	gtk_entry_set_activates_default(GTK_ENTRY(jump_dialog.entry_name), TRUE);
	gtk_entry_set_text(GTK_ENTRY(jump_dialog.entry_name), player_name);
	gtk_table_attach_defaults(GTK_TABLE(table), jump_dialog.entry_name, 1, 2, 0, 1);

	gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_printf(_("Cave:")), 0, 1, 1, 2);

	/* store of caves: cave pointer, cave name, selectable */
	store=gtk_list_store_new(3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_BOOLEAN);
	for (iter=gd_caveset; iter; iter=g_list_next (iter)) {
		Cave *cave=iter->data;
		GtkTreeIter treeiter;

		gtk_list_store_insert_with_values (store, &treeiter, -1, 0, iter->data, 1, cave->name, 2, cave->selectable || gd_all_caves_selectable, -1);
	}
	jump_dialog.combo_cave=gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref(store);

	renderer=gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(jump_dialog.combo_cave), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(jump_dialog.combo_cave), renderer, "text", 1, "sensitive", 2, NULL);
	/* we put the combo in an event box, so we can receive keypresses on our own */
	eventbox=gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), jump_dialog.combo_cave);
	gtk_table_attach_defaults(GTK_TABLE(table), eventbox, 1, 2, 1, 2);

	gtk_table_attach_defaults(GTK_TABLE(table), gd_label_new_printf(_("Level:")), 0, 1, 2, 3);
	jump_dialog.spin_level=gtk_hscale_new_with_range(1.0, 5.0, 1.0);
	gtk_scale_set_value_pos(GTK_SCALE(jump_dialog.spin_level), GTK_POS_LEFT);
	gtk_table_attach_defaults(GTK_TABLE(table), jump_dialog.spin_level, 1, 2, 2, 3);

	g_signal_connect(G_OBJECT(jump_dialog.combo_cave), "changed", G_CALLBACK(jump_cave_changed_signal), &jump_dialog);
	gtk_widget_add_events(eventbox, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(eventbox), "key_press_event", G_CALLBACK(new_game_keypress_event), &jump_dialog);
	g_signal_connect(G_OBJECT(jump_dialog.spin_level), "value-changed", G_CALLBACK(jump_cave_changed_signal), &jump_dialog);

	/* this allows the user to select if he wants to see a preview of the cave */
	expander=gtk_expander_new(_("Preview"));
	gtk_expander_set_expanded(GTK_EXPANDER (expander), gd_show_preview);
	gtk_table_attach_defaults(GTK_TABLE(table), expander, 0, 2, 3, 4);
	jump_dialog.image=gtk_image_new();
	gtk_container_add(GTK_CONTAINER (expander), jump_dialog.image);

	gtk_widget_show_all(jump_dialog.dialog);
	gtk_widget_grab_focus(jump_dialog.combo_cave);
	gtk_editable_select_region(GTK_EDITABLE(jump_dialog.entry_name), 0, 0);
	/* set default and also trigger redrawing */
	gtk_combo_box_set_active(GTK_COMBO_BOX(jump_dialog.combo_cave), gd_caveset_last_selected);
	gtk_range_set_value(GTK_RANGE(jump_dialog.spin_level), 1);

	if (gtk_dialog_run (GTK_DIALOG(jump_dialog.dialog)) == GTK_RESPONSE_ACCEPT) {
		gd_strcpy(player_name, gtk_entry_get_text(GTK_ENTRY(jump_dialog.entry_name)));
		gd_caveset_last_selected=gtk_combo_box_get_active(GTK_COMBO_BOX(jump_dialog.combo_cave));
		gd_caveset_last_selected_level=gtk_range_get_value(GTK_RANGE(jump_dialog.spin_level))-1;
		new_game (player_name, gd_caveset_last_selected, gd_caveset_last_selected_level);
	}
	gd_show_preview=gtk_expander_get_expanded(GTK_EXPANDER (expander));	/* remember expander state-even if cancel pressed */
	gtk_widget_destroy(jump_dialog.dialog);
}



/* THE MAIN GAME TIMER */
/* called 25hz or 50hz, depending on fine or normal scroll */


/* for the gtk version, it seems nicer if we first draw, then scroll. */
/* this is because there is an expose event; scrolling the "old" drawing would draw the old, and then the new. */
/* (this is not true for the sdl version) */
static GTimer *timer=NULL;

static gboolean
main_int(gpointer data)
{
	static gboolean toggle=FALSE;	/* value irrelevant */
	int up, down, left, right;
	GdDirection player_move;
	gboolean fire;
	GdGameState state;
	int intended_delay, real_delay, new_delay;
	
	if (gd_gameplay.type==GD_GAMETYPE_REPLAY)
		gtk_widget_show(main_window.replay_image_align);
	else
		gtk_widget_hide(main_window.replay_image_align);
	
	/* if fine scrolling, this is called at a 50hz rate. */
	/* but the game needs to be updated at 25hz. this variable divides the rate, if needed. */
	toggle=!toggle;
	
	/* if not using fine scrolling, we are called at 25hz, so no need to use toggle. */
	/* if 50hz, toggle divides the frequency by two. */
	if (!gd_fine_scroll || toggle) {
		up=key_up;
		down=key_down;
		left=key_left;
		right=key_right;
		fire=key_lctrl || key_rctrl;

		/* compare mouse coordinates to player coordinates, and make up movements */
		if (gd_mouse_play && mouse_cell_x>=0) {
			down=down || (gd_gameplay.cave->player_y<mouse_cell_y);
			up=up || (gd_gameplay.cave->player_y>mouse_cell_y);
			left=left || (gd_gameplay.cave->player_x>mouse_cell_x);
			right=right || (gd_gameplay.cave->player_x<mouse_cell_x);
			fire=fire || mouse_cell_click;
		}

		/* call the game "interrupt" to do all things. */
		player_move=gd_direction_from_keypress(up, down, left, right);
		state=gd_game_main_int(player_move, fire, key_suicide, restart, !paused && !gd_gameplay.out_of_window, paused, fast_forward);
		restart=FALSE;

		/* the game "interrupt" gives signals to us, which we act upon: update status bar, resize the drawing area... */
		switch (state) {
			case GD_GAME_INVALID_STATE:
				g_assert_not_reached();
				break;
				
			case GD_GAME_CAVE_LOADED:
				gd_select_pixbuf_colors(gd_gameplay.cave->color0, gd_gameplay.cave->color1, gd_gameplay.cave->color2, gd_gameplay.cave->color3, gd_gameplay.cave->color4, gd_gameplay.cave->color5);
				init_mainwindow(gd_gameplay.cave);
				showheader();
				break;

			case GD_GAME_NO_MORE_LIVES:	/* <- only used by sdl version */
			case GD_GAME_NOTHING:
				/* normally continue. */
				break;

			case GD_GAME_LABELS_CHANGED:
			case GD_GAME_TIMEOUT_NOW:	/* <- maybe we should do something else for this */
				/* normal, but we are told that the labels (score, ...) might have changed. */
				showheader();
				break;

			case GD_GAME_STOP:
				gd_main_stop_game();
				return FALSE;	/* remove timeout */

			case GD_GAME_GAME_OVER:
				gd_main_stop_game();
				if (gd_is_highscore(gd_caveset_data->highscore, gd_gameplay.player_score))
					game_over_highscore();			/* achieved a high score! */
				else
					game_over_without_highscore();			/* no high score */
				return FALSE;	/* remove timeout */
		}
	}

	/* always do the drawing and the scrolling, as either we are doing 1/40ms, or 1/20ms, but scrolling and drawing is to be done for all calls. */
	/* but only if the drawing area already exists. */
	if (main_window.drawing_area) {
		drawcave();
		scroll();
	}

	/* now get elapsed time. so we measured sleeping time (after g_timeout_add and before we got called) and drawing time. */
	intended_delay=gd_fine_scroll?20:40;
	real_delay=g_timer_elapsed(timer, NULL)*1000;	/* elapsed time in milliseconds */
		
	/* add new timeout, and return false, so remove old one */
	/* check difference between real and measured time. */
	if (ABS(intended_delay-real_delay)>10)
		/* if difference is too big, do not trust the measured value. */
		new_delay=intended_delay;
	else
		/* calculate new value. */
		/* example: should be 40, measured 43. so intended-real=40-43=-3, now we will try 37ms */
		new_delay=intended_delay+(intended_delay-real_delay);
	g_timeout_add(new_delay, main_int, main_window.window);
	g_timer_start(timer);	/* start timer from zero, to measure new delay. */
	return FALSE; /* old timeout should not be called again */
}

static void
highscore_cb(GtkWidget *widget, gpointer data)
{
	gd_show_highscore(main_window.window, NULL, FALSE, NULL, -1);
}

static void
install_game_timer()
{
	/* remove timer, if it is installed for some reason */
	uninstall_game_timeout();

	if (!paused)
		gtk_window_present(GTK_WINDOW(main_window.window));

	/* install timeout which handles the game */
	/* the exact interval is set to 40 here, but does not matter at all. */
	/* the function will handle it; it calculates sets the timeout on each and every call. */
	g_timeout_add(40, main_int, main_window.window);
	if (!timer)
		timer=g_timer_new();
	g_timer_start(timer);
}


static void
show_errors_cb(GtkWidget *widget, gpointer data)
{
	gtk_widget_hide(main_window.error_hbox);	/* if the user is presented the error list, the label is to be hidden */
	gd_show_errors(main_window.window);
}

static void
cave_editor_cb()
{
	gd_open_cave_editor();
	/* to be sure no cave is playing. */
	/* this will also stop music. to be called after opening editor window, so the music stops. */
	init_mainwindow(NULL);
}

/* called from the menu when a recent file is activated. */
static void
recent_chooser_activated_cb(GtkRecentChooser *chooser, gpointer data)
{
	GtkRecentInfo *current;
	char *filename_utf8, *filename;

	current=gtk_recent_chooser_get_current_item(chooser);
	/* we do not support non-local files */
	if (!gtk_recent_info_is_local(current)) {
		char *display_name;

		display_name=gtk_recent_info_get_uri_display(current);
		gd_errormessage(_("Cannot load file from network link."), display_name);
		g_free(display_name);
		return;
	}

	/* if the edited caveset is to be saved, but user cancels */
	if (!gd_discard_changes(main_window.window))
		return;

	filename_utf8=gtk_recent_info_get_uri_display(current);
	filename=g_filename_from_utf8(filename_utf8, -1, NULL, NULL, NULL);
	/* ask for save first? */
	gd_open_caveset_in_ui(filename, gd_use_bdcff_highscore);

	/* things to do after loading. */
	gd_main_window_set_title();
	if (gd_has_new_error())
		gd_show_last_error(main_window.window);
	else
		gd_infomessage(_("Loaded caveset from file:"), filename_utf8);

	g_free(filename);
	g_free(filename_utf8);
}


enum _replay_fields {
	COL_REPLAY_CAVE_POINTER,
	COL_REPLAY_REPLAY_POINTER,
	COL_REPLAY_NAME,	/* cave or player name */
	COL_REPLAY_DATE,
	COL_REPLAY_SCORE,
	COL_REPLAY_SUCCESS,
	COL_REPLAY_SAVED,
	COL_REPLAY_COMMENT,
	COL_REPLAY_VISIBLE, /* set to true for replay lines, false for cave lines. so "saved" toggle and comment are not visible. */
	COL_REPLAY_MAX,
};

static void
show_replays_tree_view_row_activated_cb(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeModel *model=gtk_tree_view_get_model(view);
	GtkTreeIter iter;
	Cave *cave;
	GdReplay *replay;
	
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, COL_REPLAY_CAVE_POINTER, &cave, COL_REPLAY_REPLAY_POINTER, &replay, -1);
	if (cave!=NULL && replay!=NULL)
		gd_main_new_game_replay(cave, replay);
}

static void
show_replays_response_cb(GtkDialog *dialog, int response_id, gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
replay_comment_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data)
{
	GtkTreeModel *model=(GtkTreeModel *)data;
	GtkTreePath *path=gtk_tree_path_new_from_string(path_string);
	GtkTreeIter iter;	
	GdReplay *replay;

	/* get replay. */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get(model, &iter, COL_REPLAY_REPLAY_POINTER, &replay, -1);
	/* if not available, maybe the user edited a cave line? do nothing. */
	if (!replay)
		return;
	
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COL_REPLAY_COMMENT, new_text, -1);
	gd_strcpy(replay->comment, new_text);
	/* if this is a saved replay, now the caveset is edited. */
	if (replay->saved)
		gd_caveset_edited=TRUE;
}

static void
replay_saved_toggled(GtkCellRendererText *cell, const gchar *path_string, gpointer data)
{
	GtkTreeModel *model=(GtkTreeModel *)data;
	GtkTreePath *path=gtk_tree_path_new_from_string(path_string);
	GtkTreeIter iter;	
	GdReplay *replay;

	/* get replay. */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get(model, &iter, COL_REPLAY_REPLAY_POINTER, &replay, -1);
	/* if not available, maybe the user edited a cave line? do nothing. */
	if (!replay)
		return;
	
	replay->saved=!replay->saved;
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COL_REPLAY_SAVED, replay->saved, -1);
	/* we selected or unselected a replay for saving - the caveset is now edited. */
	gd_caveset_edited=TRUE;
}

static void
replay_play_button_clicked_cb(GtkWidget *widget, gpointer data)
{
	GtkTreeView *view=GTK_TREE_VIEW(data);
	GtkTreeSelection *selection=gtk_tree_view_get_selection(view);
	GtkTreeModel *model;
	gboolean got_selected;
	GtkTreeIter iter;
	GdReplay *replay;
	Cave *cave;
	
	got_selected=gtk_tree_selection_get_selected(selection, &model, &iter);
	if (!got_selected)	/* if nothing selected, return */
		return;

	gtk_tree_model_get(model, &iter, COL_REPLAY_CAVE_POINTER, &cave, COL_REPLAY_REPLAY_POINTER, &replay, -1);
	if (cave!=NULL && replay!=NULL)
		gd_main_new_game_replay(cave, replay);
}

/* enables or disables play button on selection change */
static void
replay_tree_view_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkWidget *button=GTK_WIDGET(data);
	GtkTreeModel *model;
	gboolean enable;
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GdReplay *replay;
		
		gtk_tree_model_get(model, &iter, COL_REPLAY_REPLAY_POINTER, &replay, -1);
		enable=replay!=NULL;
	} else
		enable=FALSE;
		
	gtk_widget_set_sensitive(button, enable);
	
	
}

static void
show_replays_cb(GtkWidget *widget, gpointer data)
{
	static GtkWidget *dialog=NULL;
	GtkWidget *scroll, *view, *label, *button;
	GList *iter;
	GtkTreeStore *model;
	GtkCellRenderer *renderer;
	
	/* if window already open, just show it and return */
	if (dialog) {
		gtk_window_present(GTK_WINDOW(dialog));
		return;
	}
	
	dialog=gtk_dialog_new_with_buttons(_("Replays"), GTK_WINDOW(main_window.window), GTK_DIALOG_NO_SEPARATOR, NULL);
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(gtk_widget_destroyed), &dialog);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(show_replays_response_cb), NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 480, 360);
	gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 6);
	
	/* scrolled window to show replays tree view */
	scroll=gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), scroll);

	label=gd_label_new_printf_centered(_("<i>Hint: When watching a replay, you can use the usual movement keys (left, right...) to "
	"stop the replay and immediately continue the playing of the cave yourself.</i>"));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);
	
	/* create store containing replays */
	model=gtk_tree_store_new(COL_REPLAY_MAX, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN);
	for (iter=gd_caveset; iter!=NULL; iter=iter->next) {
		GList *replayiter;
		Cave *cave=(Cave *)iter->data;
		
		/* if the cave has replays */
		if (cave->replays) {
			GtkTreeIter caveiter;
			
			gtk_tree_store_append(model, &caveiter, NULL);
			gtk_tree_store_set(model, &caveiter, COL_REPLAY_NAME, cave->name, -1);
			
			/* add each replay */
			for (replayiter=cave->replays; replayiter!=NULL; replayiter=replayiter->next) {
				GtkTreeIter riter;
				GdReplay *replay=(GdReplay *)replayiter->data;
				char score[20];
				
				/* we have to store the score as string, as for the cave lines the unset score field would also show zero */
				g_snprintf(score, sizeof(score), "%d", replay->score);
				gtk_tree_store_append(model, &riter, &caveiter);
				gtk_tree_store_set(model, &riter, COL_REPLAY_CAVE_POINTER, cave, COL_REPLAY_REPLAY_POINTER, replay, COL_REPLAY_NAME, replay->player_name,
					COL_REPLAY_DATE, replay->date, COL_REPLAY_SCORE, score, COL_REPLAY_SUCCESS, replay->success?GTK_STOCK_YES:GTK_STOCK_NO,
					COL_REPLAY_COMMENT, replay->comment, COL_REPLAY_SAVED, replay->saved, COL_REPLAY_VISIBLE, TRUE, -1);
			}
		}
	}
	
	view=gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));	/* create tree view which will show data */
	gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
	gtk_container_add(GTK_CONTAINER(scroll), view);

	renderer=gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 0, _("Name"), renderer, "text", COL_REPLAY_NAME, NULL);	/* 0 = column number */
	gtk_tree_view_column_set_expand(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 0), TRUE);	/* name column expands */

	renderer=gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 1, _("Date"), renderer, "text", COL_REPLAY_DATE, NULL);	/* 1 = column number */

	renderer=gtk_cell_renderer_pixbuf_new();
	g_object_set(G_OBJECT(renderer), "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 2, NULL, renderer, "stock-id", COL_REPLAY_SUCCESS, NULL);

	renderer=gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 3, _("Score"), renderer, "text", COL_REPLAY_SCORE, NULL);

	renderer=gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(replay_comment_edited), model);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 4, _("Comment"), renderer, "text", COL_REPLAY_COMMENT, "visible", COL_REPLAY_VISIBLE, NULL);
	/* doubleclick will play the replay */
	g_signal_connect(G_OBJECT(view), "row-activated", G_CALLBACK(show_replays_tree_view_row_activated_cb), NULL);
	gtk_tree_view_column_set_expand(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 4), TRUE);	/* name column expands */

	renderer=gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(replay_saved_toggled), model);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 5, _("Saved"), renderer, "active", COL_REPLAY_SAVED, "visible", COL_REPLAY_VISIBLE, NULL);

	/* play button */
	button=gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	gtk_widget_set_sensitive(button, FALSE);	/* not sensitive by default. when the user selects a line, it will be enabled */
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(replay_play_button_clicked_cb), view);
	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(view))), "changed", G_CALLBACK(replay_tree_view_selection_changed), button);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area), button);
	
	/* this must be added after the play button, so it is the rightmost one */
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
	
	gtk_widget_show_all(dialog);
}






/* a cave name is selected, update the text boxt with current cave's data */
static void
cave_info_combo_changed(GtkComboBox *widget, gpointer data)
{
	GtkTextBuffer *buffer=GTK_TEXT_BUFFER(data);
	GtkTextIter iter;
	int i;

	/* clear text buffer */	
	gtk_text_buffer_set_text(buffer, "", -1);
	gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

	i=gtk_combo_box_get_active(widget);
	if (i==0) {
		/* cave set data */
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, gd_caveset_data->name, -1, "heading", NULL);
		gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
		
		/* show properties with title only if they are not empty string */
		if (!g_str_equal(gd_caveset_data->description, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Description: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, gd_caveset_data->description, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(gd_caveset_data->author, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Author: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, gd_caveset_data->author, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(gd_caveset_data->date, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Date: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, gd_caveset_data->date, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(gd_caveset_data->difficulty, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Diffuculty: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, gd_caveset_data->difficulty, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(gd_caveset_data->remark, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Remark: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, gd_caveset_data->remark, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(gd_caveset_data->notes->str, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Notes:\n"), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, gd_caveset_data->notes->str, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
	}
	else {
		/* cave data */
		Cave *cave=gd_return_nth_cave(i-1);

		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, cave->name, -1, "heading", NULL);
		gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Type: "), -1, "name", NULL);
		gtk_text_buffer_insert(buffer, &iter, cave->intermission?_("Intermission"):_("Normal cave"), -1);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);

		/* show properties with title only if they are not empty string */
		if (!g_str_equal(cave->description, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Description: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, cave->description, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(cave->author, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Author: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, cave->author, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(cave->date, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Date: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, cave->date, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(cave->difficulty, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Difficulty: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, cave->difficulty, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(cave->remark, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Remark: "), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, cave->remark, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
		if (!g_str_equal(cave->notes->str, "")) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Notes:\n"), -1, "name", NULL);
			gtk_text_buffer_insert(buffer, &iter, cave->notes->str, -1);
			gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		}
	}
}


/* show info about cave or caveset */
static void
cave_info_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog, *view, *sw, *combo;
	char *text;
	GtkTextIter iter;
	GtkTextBuffer *buffer;
	GList *citer;
	gboolean paused_save;

	/* dialog window */
	dialog=gtk_dialog_new_with_buttons(_("Caveset information"), GTK_WINDOW(main_window.window),
		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 360, 480);

	/* create a combo box. 0=caveset, 1, 2, 3... = caves */
	combo=gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), combo, FALSE, FALSE, 6);
	text=g_strdup_printf("[%s]", gd_caveset_data->name);	/* caveset name = line 0 */
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text);
	g_free(text);
	for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
		Cave *c=citer->data;

		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), c->name);
	}

	/* create text buffer */
	buffer=gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
	gtk_text_buffer_create_tag (buffer, "heading", "weight", PANGO_WEIGHT_BOLD, "scale", PANGO_SCALE_X_LARGE, NULL);
	gtk_text_buffer_create_tag (buffer, "name", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "GDash " PACKAGE_VERSION "\n\n", -1, "heading", NULL);

	sw=gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), sw);
	view=gtk_text_view_new_with_buffer(buffer);
	gtk_container_add(GTK_CONTAINER (sw), view);
	g_object_unref(buffer);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);
	gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (view), 3);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 6);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view), 6);

	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(cave_info_combo_changed), buffer);
	/* select current cave. if no current cave, or not in the list, g_list_index will returns -1. */
	/* we must add +1 anyway, so in that cave it will be 0 -> caveset info. :) */
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), g_list_index(gd_caveset, gd_gameplay.original_cave)+1);

	gtk_widget_show_all(dialog);
	paused_save=paused;
	paused=TRUE;	/* set paused game, so it stops while the users sees the message box */
	gtk_dialog_run(GTK_DIALOG(dialog));
	paused=paused_save;
	gtk_widget_destroy(dialog);
}


/*
 *
 * Creates main window
 *
 *
 */
static void
gd_create_main_window(void)
{
	/* Menu UI */
	static GtkActionEntry action_entries_normal[]={
		{"PlayMenu", NULL, N_("_Play")},
		{"FileMenu", NULL, N_("_File")},
		{"SettingsMenu", NULL, N_("_Settings")},
		{"HelpMenu", NULL, N_("_Help")},
		{"Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK(quit_cb)},
		{"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK(about_cb)},
		{"Errors", GTK_STOCK_DIALOG_ERROR, N_("_Error console"), NULL, NULL, G_CALLBACK(show_errors_cb)},
		{"Help", GTK_STOCK_HELP, NULL, NULL, NULL, G_CALLBACK(help_cb)},
		{"CaveInfo", GTK_STOCK_DIALOG_INFO, N_("Caveset _information"), NULL, N_("Show information about the and its caves"), G_CALLBACK(cave_info_cb)},
	};

	static GtkActionEntry action_entries_title[]={
		{"GamePreferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK(preferences_cb)},
		{"NewGame", GTK_STOCK_MEDIA_PLAY, N_("_New game"), "<control>N", N_("Start new game"), G_CALLBACK(new_game_cb)},
		{"CaveEditor", GD_ICON_CAVE_EDITOR, N_("Cave _editor"), NULL, NULL, G_CALLBACK(cave_editor_cb)},
		{"OpenFile", GTK_STOCK_OPEN, NULL, NULL, NULL, G_CALLBACK(open_caveset_cb)},
		{"LoadInternal", GTK_STOCK_INDEX, N_("Load _internal game")},
		{"LoadRecent", GTK_STOCK_DIALOG_INFO, N_("Open _recent")},
		{"OpenCavesDir", GTK_STOCK_CDROM, N_("O_pen shipped"), NULL, NULL, G_CALLBACK(open_caveset_dir_cb)},
		{"SaveFile", GTK_STOCK_SAVE, NULL, NULL, NULL, G_CALLBACK(save_caveset_cb)},
		{"SaveAsFile", GTK_STOCK_SAVE_AS, NULL, NULL, NULL, G_CALLBACK(save_caveset_as_cb)},
		{"HighScore", GD_ICON_AWARD, N_("Hi_ghscores"), NULL, NULL, G_CALLBACK(highscore_cb)},
		{"ShowReplays", GD_ICON_REPLAY, N_("Show _replays"), NULL, NULL, G_CALLBACK(show_replays_cb)},
	};

	static GtkActionEntry action_entries_game[]={
		{"TakeSnapshot", GD_ICON_SNAPSHOT, N_("_Take snapshot"), "F5", NULL, G_CALLBACK(save_snapshot_cb)},
		{"Restart", GD_ICON_RESTART_LEVEL, N_("Re_start level"), "Escape", N_("Restart current level"), G_CALLBACK(restart_level_cb)},
		{"EndGame", GTK_STOCK_STOP, N_("_End game"), "F1", N_("End current game"), G_CALLBACK(stop_game_cb)},
	};

	static GtkActionEntry action_entries_snapshot[]={
		{"RevertToSnapshot", GTK_STOCK_UNDO, N_("_Revert to snapshot"), "F6", NULL, G_CALLBACK(load_snapshot_cb)},
	};

	static GtkToggleActionEntry action_entries_toggle[]={
		{"PauseGame", GTK_STOCK_MEDIA_PAUSE, NULL, "space", N_("Pause game"), G_CALLBACK(toggle_pause_cb), FALSE},
		{"FullScreen", GTK_STOCK_FULLSCREEN, NULL, "F11", N_("Fullscreen mode during play"), G_CALLBACK(toggle_fullscreen_cb), FALSE},
		{"FastForward", GTK_STOCK_MEDIA_FORWARD, N_("Fast for_ward"), "<control>F", N_("Fast forward"), G_CALLBACK(toggle_fast_cb), FALSE},
	};

	static const char *ui_info =
		"<ui>"
		"<menubar name='MenuBar'>"
		"<menu action='FileMenu'>"
		"<separator/>"
		"<menuitem action='OpenFile'/>"
		"<menuitem action='LoadRecent'/>"
		"<menuitem action='OpenCavesDir'/>"
		"<menuitem action='LoadInternal'/>"
		"<separator/>"
		"<menuitem action='CaveEditor'/>"
		"<separator/>"
		"<menuitem action='SaveFile'/>"
		"<menuitem action='SaveAsFile'/>"
		"<separator/>"
		"<menuitem action='Quit'/>"
		"</menu>"
		"<menu action='PlayMenu'>"
		"<menuitem action='NewGame'/>"
		"<menuitem action='PauseGame'/>"
		"<menuitem action='FastForward'/>"
		"<menuitem action='TakeSnapshot'/>"
		"<menuitem action='RevertToSnapshot'/>"
		"<menuitem action='Restart'/>"
		"<menuitem action='EndGame'/>"
		"<separator/>"
		"<menuitem action='CaveInfo'/>"
		"<menuitem action='HighScore'/>"
		"<menuitem action='ShowReplays'/>"
		"<separator/>"
		"<menuitem action='FullScreen'/>"
		"<menuitem action='GamePreferences'/>"
		"</menu>"
		"<menu action='HelpMenu'>"
		"<menuitem action='Help'/>"
		"<separator/>"
		"<menuitem action='Errors'/>"
		"<menuitem action='About'/>"
		"</menu>"
		"</menubar>"

		"<toolbar name='ToolBar'>"
		"<toolitem action='NewGame'/>"
		"<toolitem action='EndGame'/>"
		"<toolitem action='FullScreen'/>"
		"<separator/>"
		"<toolitem action='PauseGame'/>"
		"<toolitem action='FastForward'/>"
		"<toolitem action='Restart'/>"
		"<separator/>"
		"<toolitem action='CaveInfo'/>"
		"</toolbar>"
		"</ui>";

	GtkWidget *vbox, *hbox, *recent_chooser;
	GtkRecentFilter *recent_filter;
	GdkPixbuf *logo;
	GtkUIManager *ui;
	int i;
	const gchar **names;
	GtkWidget *menu;

	logo=gd_icon();
	gtk_window_set_default_icon (logo);
	g_object_unref(logo);

	main_window.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(main_window.window), 360, 300);
	g_signal_connect(G_OBJECT(main_window.window), "focus_out_event", G_CALLBACK(focus_out_event), NULL);
	g_signal_connect(G_OBJECT(main_window.window), "delete_event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(G_OBJECT(main_window.window), "key_press_event", G_CALLBACK(keypress_event), NULL);
	g_signal_connect(G_OBJECT(main_window.window), "key_release_event", G_CALLBACK(keypress_event), NULL);

	/* vertical box */
	vbox=gtk_vbox_new(FALSE, 0);
	gtk_container_add (GTK_CONTAINER (main_window.window), vbox);

	/* menu */
	main_window.actions_normal=gtk_action_group_new("main_window.actions_normal");
	gtk_action_group_set_translation_domain (main_window.actions_normal, PACKAGE);
	gtk_action_group_add_actions (main_window.actions_normal, action_entries_normal, G_N_ELEMENTS (action_entries_normal), &main_window);
	gtk_action_group_add_toggle_actions (main_window.actions_normal, action_entries_toggle, G_N_ELEMENTS (action_entries_toggle), NULL);
	main_window.actions_title=gtk_action_group_new("main_window.actions_title");
	gtk_action_group_set_translation_domain (main_window.actions_title, PACKAGE);
	gtk_action_group_add_actions (main_window.actions_title, action_entries_title, G_N_ELEMENTS (action_entries_title), &main_window);
	/* make this toolbar button always have a title */
	g_object_set (gtk_action_group_get_action (main_window.actions_title, "NewGame"), "is_important", TRUE, NULL);
	main_window.actions_game=gtk_action_group_new("main_window.actions_game");
	gtk_action_group_set_translation_domain (main_window.actions_game, PACKAGE);
	gtk_action_group_add_actions (main_window.actions_game, action_entries_game, G_N_ELEMENTS (action_entries_game), &main_window);
	main_window.actions_snapshot=gtk_action_group_new("main_window.actions_snapshot");
	gtk_action_group_set_translation_domain (main_window.actions_snapshot, PACKAGE);
	gtk_action_group_add_actions (main_window.actions_snapshot, action_entries_snapshot, G_N_ELEMENTS (action_entries_snapshot), &main_window);

	/* build the ui */
	ui=gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group (ui, main_window.actions_normal, 0);
	gtk_ui_manager_insert_action_group (ui, main_window.actions_title, 0);
	gtk_ui_manager_insert_action_group (ui, main_window.actions_game, 0);
	gtk_ui_manager_insert_action_group (ui, main_window.actions_snapshot, 0);
	gtk_window_add_accel_group (GTK_WINDOW(main_window.window), gtk_ui_manager_get_accel_group (ui));
	gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, NULL);
	main_window.menubar=gtk_ui_manager_get_widget (ui, "/MenuBar");
	gtk_box_pack_start(GTK_BOX (vbox), main_window.menubar, FALSE, FALSE, 0);
	main_window.toolbar=gtk_ui_manager_get_widget (ui, "/ToolBar");
	gtk_toolbar_set_style(GTK_TOOLBAR(main_window.toolbar), GTK_TOOLBAR_BOTH_HORIZ);
	gtk_box_pack_start(GTK_BOX (vbox), main_window.toolbar, FALSE, FALSE, 0);
	gtk_toolbar_set_tooltips (GTK_TOOLBAR (main_window.toolbar), TRUE);

	/* make a submenu, which contains the games compiled in. */
	i=0;
	menu=gtk_menu_new();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (ui, "/MenuBar/FileMenu/LoadInternal")), menu);
	names=gd_caveset_get_internal_game_names ();
	while (names[i]) {
		GtkWidget *menuitem=gtk_menu_item_new_with_label (names[i]);

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_show(menuitem);
		g_signal_connect (G_OBJECT(menuitem), "activate", G_CALLBACK(load_internal_cb), GINT_TO_POINTER (i));
		i++;
	}

	/* recent file chooser */
	recent_chooser=gtk_recent_chooser_menu_new();
	recent_filter=gtk_recent_filter_new();
	/* gdash file extensions */
	for (i=0; gd_caveset_extensions[i]!=NULL; i++)
		gtk_recent_filter_add_pattern(recent_filter, gd_caveset_extensions[i]);
	gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(recent_chooser), recent_filter);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (gtk_ui_manager_get_widget (ui, "/MenuBar/FileMenu/LoadRecent")), recent_chooser);
	g_signal_connect(G_OBJECT(recent_chooser), "item-activated", G_CALLBACK(recent_chooser_activated_cb), NULL);

	g_object_unref(ui);
	
	/* this hbox will contain the replay logo and the labels */
	hbox=gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* replay logo... */	
	main_window.replay_image_align=gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), main_window.replay_image_align, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_window.replay_image_align), gtk_image_new_from_stock(GD_ICON_REPLAY, GTK_ICON_SIZE_LARGE_TOOLBAR));
	
	/* ... labels. */
	main_window.labels=gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), main_window.labels);

	/* first hbox for labels ABOVE drawing */
	hbox=gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX (main_window.labels), hbox, FALSE, FALSE, 0);
	main_window.label_cave_name=gtk_label_new(NULL);	/* NAME label */
	gtk_label_set_ellipsize (GTK_LABEL(main_window.label_cave_name), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start_defaults(GTK_BOX (hbox), main_window.label_cave_name);	/* NAME label */
	gtk_misc_set_alignment(GTK_MISC(main_window.label_cave_name), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), main_window.label_key3=gtk_label_new(NULL), FALSE, FALSE, 0);	/* key3 label */
	gtk_box_pack_start(GTK_BOX(hbox), main_window.label_key2=gtk_label_new(NULL), FALSE, FALSE, 0);	/* key2 label */
	gtk_box_pack_start(GTK_BOX(hbox), main_window.label_key1=gtk_label_new(NULL), FALSE, FALSE, 0);	/* key1 label */

	/* second row of labels */
	hbox=gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(main_window.labels), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), main_window.label_lives=gtk_label_new(NULL), FALSE, FALSE, 0);	/* LIVES label */
	gtk_box_pack_end(GTK_BOX(hbox), main_window.label_diamonds=gtk_label_new(NULL), FALSE, FALSE, 0);	/* DIAMONDS label */
	gtk_box_pack_end(GTK_BOX(hbox), main_window.label_skeletons=gtk_label_new(NULL), FALSE, FALSE, 0);	/* SKELETONS label */

	/* third row */
	hbox=gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(main_window.labels), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), main_window.label_value=gtk_label_new(NULL), FALSE, FALSE, 0);	/* VALUE label */
	gtk_box_pack_start(GTK_BOX(hbox), main_window.label_score=gtk_label_new(NULL), FALSE, FALSE, 0);	/* SCORE label */
	gtk_box_pack_end(GTK_BOX(hbox), main_window.label_time=gtk_label_new(NULL), FALSE, FALSE, 0);	/* TIME label */
	gtk_box_pack_end(GTK_BOX(hbox), main_window.label_gravity_will_change=gtk_label_new(NULL), FALSE, FALSE, 0);	/* gravity label */

	main_window.scroll_window=gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(main_window.scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start_defaults(GTK_BOX (vbox), main_window.scroll_window);

	main_window.label_variables=gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX (vbox), main_window.label_variables, FALSE, FALSE, 0);

	hbox=gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), main_window.error_label=gtk_label_new(NULL), FALSE, FALSE, 0);	/* error label */
	main_window.error_hbox=hbox;

	gtk_widget_show_all(main_window.window);
	gtk_window_present(GTK_WINDOW(main_window.window));
}


/*
	main()
	function
 */

int
main(int argc, char *argv[])
{
	int quit=0;
	gboolean editor=FALSE;
	char *gallery_filename=NULL;
	char *png_filename=NULL, *png_size=NULL;
	char *save_cave_name=NULL;

	GError *error=NULL;
	GOptionContext *context;
	GOptionEntry entries[]={
		{"editor", 'e', 0, G_OPTION_ARG_NONE, &editor, N_("Start editor")},
		{"gallery", 'g', 0, G_OPTION_ARG_FILENAME, &gallery_filename, N_("Save caveset in a HTML gallery")},
		{"png", 'p', 0, G_OPTION_ARG_FILENAME, &png_filename, N_("Save cave C, level L in a PNG image. If no cave selected, uses a random one")},
		{"png_size", 's', 0, G_OPTION_ARG_STRING, &png_size, N_("Set PNG image size. Default is 128x96, set to 0x0 for unscaled")},
		{"save", 'S', 0, G_OPTION_ARG_FILENAME, &save_cave_name, N_("Save caveset in a BDCFF file")},
		{"quit", 'q', 0, G_OPTION_ARG_NONE, &quit, N_("Batch mode: quit after specified tasks")},
		{NULL}
	};

	context=gd_option_context_new();
	g_option_context_add_main_entries (context, entries, PACKAGE);	/* gdash (gtk version) parameters */
	g_option_context_add_group (context, gtk_get_option_group (TRUE));	/* add gtk parameters */
	g_option_context_parse (context, &argc, &argv, &error);
	g_option_context_free (context);
	if (error) {
		g_warning("%s", error->message);
		g_error_free(error);
	}

	/* show license? */
	if (gd_param_license) {
		char *wrapped=gd_wrap_text(gd_about_license, 72);

		g_print("%s", wrapped);
		g_free(wrapped);
		return 0;
	}

	gd_install_log_handler();

	gd_settings_init_dirs();
	gd_load_settings();

	gtk_set_locale();
	gd_settings_set_locale();
	
	gtk_init(&argc, &argv);

	gd_settings_init_translation();

	gd_cave_init();
	gd_cave_db_init();
	gd_cave_sound_db_init();
	gd_c64_import_init_tables();

	gd_caveset_clear();	/* this also creates the default cave */

	gd_clear_error_flag();

	gd_wait_before_game_over=FALSE;

	/* LOAD A CAVESET FROM A FILE, OR AN INTERNAL ONE */
	/* if remaining arguments, they are filenames */
	if (gd_param_cavenames && gd_param_cavenames[0]) {
		/* load caveset, "ignore" errors. */
		if (!gd_open_caveset_in_ui(gd_param_cavenames[0], gd_use_bdcff_highscore))
			g_critical (_("Errors during loading caveset from file '%s'"), gd_param_cavenames[0]);
	}
	else if (gd_param_internal) {
		/* if specified an internal caveset; if error, halt now */
		if (!gd_caveset_load_from_internal (gd_param_internal-1, gd_user_config_dir))
			g_critical (_("%d: no such internal caveset"), gd_param_internal);
	}

	/* if failed or nothing requested, load default */
	if (gd_caveset==NULL)
		gd_caveset_load_from_internal (0, gd_user_config_dir);

	/* always load c64 graphics, as it is the builtin, and we need an icon for the theme selector. */
	gd_loadcells_default();
	gd_create_pixbuf_for_builtin_gfx();

	/* load other theme, if specified in config. */
	if (gd_theme!=NULL && !g_str_equal(gd_theme, "")) {
		if (!gd_loadcells_file(gd_theme)) {
			/* forget the theme if we are unable to load */
			g_warning("Cannot load theme %s, switching to built-in theme", gd_theme);
			g_free(gd_theme);
			gd_theme=NULL;
			gd_loadcells_default();	/* load default gfx */
		}
	}

	/* after loading cells... see if generating a gallery. */
	if (gallery_filename)
		gd_save_html (gallery_filename, NULL);

	/* if cave or level values given, check range */
	if (gd_param_cave)
		if (gd_param_cave<1 || gd_param_cave>=gd_caveset_count () || gd_param_level<1 || gd_param_level>5)
			g_critical (_("Invalid cave or level number!\n"));

	/* save cave png */
	if (png_filename) {
		GError *error=NULL;
		GdkPixbuf *pixbuf;
		Cave *renderedcave;
		unsigned int size_x=128, size_y=96;	/* default size */

		if (gd_param_cave == 0)
			gd_param_cave=g_random_int_range(0, gd_caveset_count ())+1;

		if (png_size && (sscanf (png_size, "%ux%u", &size_x, &size_y) != 2))
			g_critical (_("Invalid image size: %s"), png_size);
		if (size_x<1 || size_y<1) {
			size_x=0;
			size_y=0;
		}

		/* rendering cave for png: seed=0 */
		renderedcave=gd_cave_new_from_caveset (gd_param_cave-1, gd_param_level-1, 0);
		pixbuf=gd_drawcave_to_pixbuf(renderedcave, size_x, size_y, TRUE);
		if (!gdk_pixbuf_save (pixbuf, png_filename, "png", &error, "compression", "9", NULL))
			g_critical ("Error saving PNG image %s: %s", png_filename, error->message);
		g_object_unref(pixbuf);
		gd_cave_free (renderedcave);

		/* avoid starting game */
		gd_param_cave=0;
	}

	if (save_cave_name)
		gd_caveset_save(save_cave_name, FALSE);

	/* if batch mode, quit now */
	if (quit)
		return 0;

	/* create window */
	gd_create_stock_icons();
	gd_create_main_window();
	gd_main_window_set_title();

	gd_sound_init();

	init_mainwindow(NULL);

	if (gd_param_cave) {
		/* if cave number given, start game */
		new_game (g_get_real_name(), gd_param_cave-1, gd_param_level-1);
	}
	else if (editor)
		cave_editor_cb (NULL, &main_window);
	
	gtk_main ();

	gd_save_highscore(gd_user_config_dir);

	gd_save_settings();

	return 0;
}
