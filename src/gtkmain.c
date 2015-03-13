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
#include "gfxutil.h"
#include "gtkmain.h"

/* game info */
static gboolean paused=FALSE;
static gboolean fast_forward=FALSE;
static gboolean fullscreen=FALSE;

typedef struct _gd_main_window {
    GtkWidget *window;
    GtkActionGroup *actions_normal, *actions_title, *actions_title_replay, *actions_game, *actions_snapshot;
    GtkWidget *scroll_window;
    GtkWidget *drawing_area, *title_image, *story_label;       /* three things that could be drawn in the main window */
    GdkPixmap **title_pixmaps;
    GtkWidget *labels;        /* parts of main window which have to be shown or hidden */
    GtkWidget *label_topleft, *label_topright;
    GtkWidget *label_bottomleft, *label_bottomright;
    GtkWidget *label_variables;
    GtkWidget *info_bar, *info_label;
    GtkWidget *menubar, *toolbar;
    GtkWidget *replay_image_align;
    
    GdGame *game;
} GdMainWindow;

static GdMainWindow main_window;

static gboolean key_fire_1=FALSE, key_fire_2=FALSE, key_right=FALSE, key_up=FALSE, key_down=FALSE, key_left=FALSE, key_suicide=FALSE;
static gboolean key_alt_status_bar=FALSE;
static gboolean restart;    /* keys which control the game, but are handled differently than the above */
static int mouse_cell_x=-1, mouse_cell_y=-1, mouse_cell_click=0;


static GdCave *snapshot=NULL;





static void main_stop_game_but_maybe_highscore();
static void main_free_game();




/************************************************
 *
 *    EVENTS
 *
 */

/* closing the window by the window manager.
 * ask the user if he want to save the cave first, and
 * only then do quit the main loop. */
static gboolean
main_window_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
    if (gd_discard_changes(main_window.window))
        gtk_main_quit();
    return TRUE;
}

/* keypress. key_* can be (event_type==gdk_key_press), as we
 * connected this function to key press and key release.
 */
static gboolean
main_window_keypress_event (GtkWidget * widget, GdkEventKey* event, gpointer data)
{
    gboolean press=event->type==GDK_KEY_PRESS;    /* true for press, false for release */
    
    if (event->keyval==gd_gtk_key_left) {
        key_left=press;
        return TRUE;
    }
    if (event->keyval==gd_gtk_key_right) {
        key_right=press;
        return TRUE;
    }
    if (event->keyval==gd_gtk_key_up) {
        key_up=press;
        return TRUE;
    }
    if (event->keyval==gd_gtk_key_down) {
        key_down=press;
        return TRUE;
    }
    if (event->keyval==gd_gtk_key_fire_1) {
        key_fire_1=press;
        return TRUE;
    }
    if (event->keyval==GDK_Shift_L) {
        key_alt_status_bar=press;
        return TRUE;
    }
    if (event->keyval==gd_gtk_key_fire_2) {
        key_fire_2=press;
        return TRUE;
    }
    if (event->keyval==gd_gtk_key_suicide) {
        key_suicide=press;
        return TRUE;
    }

    return FALSE;    /* if any other key, we did not process it. go on, let gtk handle it. */
}

/* focus leaves game play window. remember that keys are not pressed!
    as we don't receive key released events along with focus out. */
static gboolean
main_window_focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    key_fire_1=FALSE;
    key_fire_2=FALSE;
    key_right=FALSE;
    key_up=FALSE;
    key_down=FALSE;
    key_left=FALSE;
    key_suicide=FALSE;
    key_alt_status_bar=FALSE;
    
    return FALSE;
}

/* for mouse play. */
/* mouse leaves drawing area event */
/* if pointer not inside window, no control of player. */
static gboolean
drawing_area_leave_event(GtkWidget * widget, GdkEventCrossing* event, gpointer data)
{
    mouse_cell_x=-1;
    mouse_cell_y=-1;
    mouse_cell_click=0;
    return TRUE;
}

/* mouse button press event */
static gboolean
drawing_area_button_event(GtkWidget * widget, GdkEventButton * event, gpointer data)
{
    /* see if it is a click or a release. */
    mouse_cell_click=event->type == GDK_BUTTON_PRESS;
    return TRUE;
}

/* mouse motion event */
static gboolean
drawing_area_motion_event(GtkWidget * widget, GdkEventMotion * event, gpointer data)
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
 * draws the cave during game play. also called by the timers,
 * and the drawing area expose event.
 */
static void
drawing_area_draw_cave()
{
    int x, y, xd, yd;

    if (!main_window.drawing_area)
        return;
    if (!main_window.drawing_area->window)
        return;
    if (!main_window.game)
        return;
    if (!main_window.game->cave)    /* if already no cave, just return */
        return;
    if (!main_window.game->gfx_buffer)    /* if already no cave, just return */
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
    /* before drawing, process all pending events for the drawing area, so no confusion occurs. */
    for (y=main_window.game->cave->y1, yd=0; y<=main_window.game->cave->y2; y++, yd++) {
        for (x=main_window.game->cave->x1, xd=0; x<=main_window.game->cave->x2; x++, xd++) {
            if (main_window.game->gfx_buffer[y][x] & GD_REDRAW) {
                main_window.game->gfx_buffer[y][x] &= ~GD_REDRAW;
                gdk_draw_drawable(main_window.drawing_area->window, main_window.drawing_area->style->black_gc, gd_game_pixmap(main_window.game->gfx_buffer[y][x]),
                                  0, 0, xd*gd_cell_size_game, yd*gd_cell_size_game, gd_cell_size_game, gd_cell_size_game);
            }
        }
    }
}



/* CAVE DRAWING is done in an idle func. it requires much time, especially on windows. */
static gboolean draw_idle_func_installed=FALSE;

static gboolean draw_idle_func(gpointer data)
{
    drawing_area_draw_cave();
    
    draw_idle_func_installed=FALSE;
    return FALSE;
}

static void schedule_draw()
{
    /* update in an idle func, so we do not slow down the application, when there is no time to draw. */
    /* the priority must be very low, as gtk also does its things in idle funcs, ie. window resizing and */
    /* expose events. otherwise we would mess up the scrolling */
    if (!draw_idle_func_installed) {
        draw_idle_func_installed=TRUE;
        g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc) draw_idle_func, NULL, NULL);
    }
}




static gboolean
drawing_area_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    int x1, y1, x2, y2, xd, yd;

    if (!widget->window)
        return FALSE;
    if (!main_window.game)
        return FALSE;
    if (!main_window.game->cave)    /* if already no cave, just return */
        return FALSE;
    if (!main_window.game->gfx_buffer)    /* if already no cave, just return */
        return FALSE;

    /* redraw entire area, not specific rectangles.
     * this function gets called only when the main window gets exposed
     * by the user removing another window. */
    /* these are screen coordinates. */
    x1=event->area.x/gd_cell_size_game;
    y1=event->area.y/gd_cell_size_game;
    x2=(event->area.x+event->area.width-1)/gd_cell_size_game;
    y2=(event->area.y+event->area.height-1)/gd_cell_size_game;
    /* when resizing the cave, we may get events which store the old size, if the drawing area is not yet resized. */
    if (x1<0) x1=0;
    if (y1<0) x1=0;
    if (x2>=main_window.game->cave->w) x2=main_window.game->cave->w-1;
    if (y2>=main_window.game->cave->h) y2=main_window.game->cave->h-1;

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
        int y=yd+main_window.game->cave->y1;
        for (xd=x1; xd<=x2; xd++) {
            int x=xd+main_window.game->cave->x1;
            if (main_window.game->gfx_buffer[y][x]!=-1)
                main_window.game->gfx_buffer[y][x] |= GD_REDRAW;
        }
    }
    /* schedule drawing as an idle func. */
    schedule_draw();

    return TRUE;
}






/*
 * functions for the main window
 * - fullscreen
 * - game
 * - title animation
 */


static gboolean
main_window_set_fullscreen_idle_func(gpointer data)
{
    gtk_window_fullscreen(GTK_WINDOW(data));
    return FALSE;
}

/* set or unset fullscreen if necessary */
/* hack: gtk-win32 does not correctly handle fullscreen & removing widgets.
   so we put fullscreening call into a low priority idle function, which will be called
   after all window resizing & the like did take place. */
static void
main_window_set_fullscreen(gboolean ingame)
{
    if (ingame && fullscreen) {
        gtk_widget_hide(main_window.menubar);
        gtk_widget_hide(main_window.toolbar);
        g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc) main_window_set_fullscreen_idle_func, main_window.window, NULL);
    }
    else {
        g_idle_remove_by_data(main_window.window);
        gtk_window_unfullscreen(GTK_WINDOW(main_window.window));
        gtk_widget_show(main_window.menubar);
        gtk_widget_show(main_window.toolbar);
    }
}


/*
 * init mainwindow
 * creates title screen or drawing area
 *
 */
 
static gboolean main_window_title_animation_idle_func_installed=FALSE;

static gboolean
main_window_title_animation_idle_func(gpointer data)
{
    gtk_image_set_from_pixmap(GTK_IMAGE(main_window.title_image), (GdkPixmap *) data, NULL);

    main_window_title_animation_idle_func_installed=FALSE;
    return FALSE;
}

static void
main_window_title_animation_install_idle(GdkPixmap *pixbuf)
{
    if (!main_window_title_animation_idle_func_installed) {
        g_idle_add_full(G_PRIORITY_LOW, main_window_title_animation_idle_func, pixbuf, NULL);
        main_window_title_animation_idle_func_installed=TRUE;
    }
}

static gboolean
main_window_title_animation_func(gpointer data)
{
    static int animcycle=0;
    int count;
    
    /* if title image widget does not exist for some reason, we quit now */
    if (main_window.title_image==NULL)
        return FALSE;

    /* count the number of frames. */
    count=0;
    while (main_window.title_pixmaps[count]!=NULL)
        count++;

    if (gtk_window_has_toplevel_focus(GTK_WINDOW(main_window.window))) {
        animcycle=(animcycle+1)%count;
        /* do the drawing when we have time. */
        main_window_title_animation_install_idle(main_window.title_pixmaps[animcycle]);
    }
    return TRUE;
}

static void
main_window_title_animation_remove()
{
    int i;

    g_source_remove_by_user_data(main_window_title_animation_func);
    i=0;
    /* free animation frames, if any */
    if (main_window.title_pixmaps!=NULL) {
        while (main_window.title_pixmaps[i]!=NULL) {
            g_object_unref(main_window.title_pixmaps[i]);
            i++;
        }
        g_free(main_window.title_pixmaps);
        main_window.title_pixmaps=NULL;
    }
}

void
gd_main_window_set_title_animation()
{
    int w, h;

    /* if main window does not exist, ignore. */
    if (main_window.window==NULL)
        return;
    /* if exists but not showing a title image, that is an error. */
    g_return_if_fail(main_window.title_image!=NULL);
    
    main_window_title_animation_remove();
    main_window.title_pixmaps=gd_create_title_animation();
    /* set first frame immediately */
    gtk_image_set_from_pixmap(GTK_IMAGE(main_window.title_image), main_window.title_pixmaps[0], NULL);
    /* and if more frames, add animation timeout */
    if (main_window.title_pixmaps[1]!=NULL)
        g_timeout_add(40, main_window_title_animation_func, main_window_title_animation_func);    /* 25fps animation */

    /* resize the scrolling window so the image fits - a bit larger than the image so scroll bars do not appear*/
    gdk_drawable_get_size(GDK_DRAWABLE(main_window.title_pixmaps[0]), &w, &h);
    /* also some minimum size... 320x200 is some adhoc value only. */
    gtk_widget_set_size_request(main_window.scroll_window, MAX(w, 320)+24, MAX(h, 200)+24);

    /* with the title screen, it is usually smaller than in the game. shrink it. */
    gtk_window_resize(GTK_WINDOW(main_window.window), 1, 1);
}

static void
main_window_init_title()
{
    if (!main_window.title_image) {
        /* destroy current widget in main window */
        if (gtk_bin_get_child(GTK_BIN(main_window.scroll_window)))
            gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(main_window.scroll_window)));

        /* title screen */
        main_window.title_image=gtk_image_new();
        g_signal_connect (G_OBJECT(main_window.title_image), "destroy", G_CALLBACK(gtk_widget_destroyed), &main_window.title_image);
        g_signal_connect (G_OBJECT(main_window.title_image), "destroy", G_CALLBACK(main_window_title_animation_remove), NULL);
        gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(main_window.scroll_window), main_window.title_image);
        
        gd_main_window_set_title_animation();

        /* show newly created widgets */
        gtk_widget_show_all(main_window.scroll_window);
    }

    /* hide cave data */
    gtk_widget_hide(main_window.labels);
    gtk_widget_hide(main_window.label_variables);
    if (gd_has_new_error()) {
        gtk_label_set(GTK_LABEL(main_window.info_label), ((GdErrorMessage *)(g_list_last(gd_errors)->data))->message);
        gtk_widget_show(main_window.info_bar);
    } else {
        gtk_widget_hide(main_window.info_bar);
    }

    /* enable menus and buttons of game */
    gtk_action_group_set_sensitive(main_window.actions_title, !gd_editor_window);
    gtk_action_group_set_sensitive(main_window.actions_title_replay, !gd_editor_window && gd_caveset_has_replays());
    gtk_action_group_set_sensitive(main_window.actions_game, FALSE);
    gtk_action_group_set_sensitive(main_window.actions_snapshot, snapshot!=NULL);
    /* if editor window exists, no music. */
    if (gd_editor_window)
        gd_music_stop();
    else
        gd_music_play_random();
    if (gd_editor_window)
        gtk_widget_set_sensitive(gd_editor_window, TRUE);
    gtk_widget_hide(main_window.replay_image_align);

    /* set or unset fullscreen if necessary */
    main_window_set_fullscreen(FALSE);
}



static void
main_window_init_cave(GdCave *cave)
{
    char *name_escaped;
    
    g_assert(cave!=NULL);

    /* cave drawing */
    if (!main_window.drawing_area) {
        GtkWidget *align;

        /* destroy current widget in main window */
        if (gtk_bin_get_child(GTK_BIN(main_window.scroll_window)))
            gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(main_window.scroll_window)));

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

        /* show newly created widgets */
        gtk_widget_show_all(main_window.scroll_window);
    }
    /* set the minimum size of the scroll window: 20*12 cells */
    /* XXX adding some pixels for the scrollbars-here we add 24 */
    gtk_widget_set_size_request(main_window.scroll_window, 20*gd_cell_size_game+24, 12*gd_cell_size_game+24);
    gtk_widget_set_size_request(main_window.drawing_area, (cave->x2-cave->x1+1)*gd_cell_size_game, (cave->y2-cave->y1+1)*gd_cell_size_game);

    /* show cave data */
    gtk_widget_show(main_window.labels);
    if (main_window.game->type==GD_GAMETYPE_TEST && gd_show_test_label)
        gtk_widget_show(main_window.label_variables);
    else
        gtk_widget_hide(main_window.label_variables);
    gtk_widget_hide(main_window.info_bar);

    name_escaped=g_markup_escape_text(cave->name, -1);
    /* TRANSLATORS: cave name, level x */
    gd_label_set_markup_printf(GTK_LABEL(main_window.label_topleft), _("<b>%s</b>, level %d"), name_escaped, cave->rendered);
    g_free(name_escaped);

    /* enable menus and buttons of game */
    gtk_action_group_set_sensitive(main_window.actions_title, FALSE);
    gtk_action_group_set_sensitive(main_window.actions_title_replay, FALSE);
    gtk_action_group_set_sensitive(main_window.actions_game, TRUE);
    gtk_action_group_set_sensitive(main_window.actions_snapshot, snapshot!=NULL);

    gd_music_stop();
    if (gd_editor_window)
        gtk_widget_set_sensitive(gd_editor_window, FALSE);
    gtk_widget_hide(main_window.replay_image_align);    /* it will be shown if needed. */

    /* set or unset fullscreen if necessary */
    main_window_set_fullscreen(TRUE);
}

static void
main_window_init_story(GdCave *cave)
{
    char *escaped_name, *escaped_story;
    
    if (!main_window.story_label) {
        /* destroy current widget in main window */
        if (gtk_bin_get_child(GTK_BIN(main_window.scroll_window)))
            gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(main_window.scroll_window)));

        /* title screen */
        main_window.story_label=gtk_label_new(NULL);
        gtk_label_set_line_wrap(GTK_LABEL(main_window.story_label), TRUE);
        g_signal_connect(G_OBJECT(main_window.story_label), "destroy", G_CALLBACK(gtk_widget_destroyed), &main_window.story_label);
        gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(main_window.scroll_window), main_window.story_label);

        /* show newly created widgets */
        gtk_widget_show_all(main_window.scroll_window);
    }
    escaped_name=g_markup_escape_text(cave->name, -1);
    escaped_story=g_markup_escape_text(cave->story->str, -1);
    gd_label_set_markup_printf(GTK_LABEL(main_window.story_label), _("<big><b>%s</b></big>\n\n%s\n\n<i>Press fire to continue</i>"), escaped_name, escaped_story);
    g_free(escaped_name);
    g_free(escaped_story);
    /* the width of the label is 320, as the windows is usually not smaller than 320x200 */
    gtk_widget_set_size_request(main_window.story_label, 320, -1);

    /* hide cave data */
    gtk_widget_hide(main_window.labels);
    gtk_widget_hide(main_window.label_variables);
    if (gd_has_new_error()) {
        gtk_widget_show(main_window.info_bar);
        gtk_label_set(GTK_LABEL(main_window.info_label), ((GdErrorMessage *)(g_list_last(gd_errors)->data))->message);
    } else {
        gtk_widget_hide(main_window.info_bar);
    }

    /* enable menus and buttons of game */
    gtk_action_group_set_sensitive(main_window.actions_title, FALSE);
    gtk_action_group_set_sensitive(main_window.actions_title_replay, FALSE);
    gtk_action_group_set_sensitive(main_window.actions_game, TRUE);
    gtk_action_group_set_sensitive(main_window.actions_snapshot, snapshot!=NULL);
    /* if editor window exists, no music. */
    gd_music_stop();
    if (gd_editor_window)
        gtk_widget_set_sensitive(gd_editor_window, FALSE);
    gtk_widget_hide(main_window.replay_image_align);

    /* set or unset fullscreen if necessary */
    main_window_set_fullscreen(TRUE);
}



/* game over, and highscore is achieved.
 * ask for name, and if given, record it! */
static void
game_over_highscore()
{
    char *text;
    int rank;

    text=g_strdup_printf(_("You have %d points, and achieved a highscore."), main_window.game->player_score);
    gd_infomessage(_("Game over!"), text);
    g_free(text);

    /* enter to highscore table */
    rank=gd_add_highscore(gd_caveset_data->highscore, main_window.game->player_name, main_window.game->player_score);
    gd_show_highscore(main_window.window, NULL, FALSE, NULL, rank);
}

/* game over, but no highscore.
 * only pops up a simple dialog box. */
static void
game_over_without_highscore()
{
    gchar *text;

    text=g_strdup_printf(_("You have %d points."), main_window.game->player_score);
    gd_infomessage(_("Game over!"), text);
    g_free(text);

}


/* amoeba state to string */
static const char *
amoeba_state_string(GdAmoebaState a)
{
    switch(a) {
    case GD_AM_SLEEPING: return _("sleeping");            /* sleeping - not yet let out. */
    case GD_AM_AWAKE: return _("awake");                /* living, growing */
    case GD_AM_TOO_BIG: return _("too big");            /* grown too big, will convert to stones */
    case GD_AM_ENCLOSED: return _("enclosed");            /* enclosed, will convert to diamonds */
    }
    return _("unknown");
}

/* amoeba state to string */
static const char *
magic_wall_state_string(GdMagicWallState m)
{
    switch(m) {
    case GD_MW_DORMANT: return _("dormant");
    case GD_MW_ACTIVE: return _("active");
    case GD_MW_EXPIRED: return _("expired");
    }
    return _("unknown");
}



static void
main_int_set_labels()
{
    const GdCave *cave=main_window.game->cave;
    int time;
    
    /* lives reamining in game. */
    /* not trivial, but this sometimes DOES change during the running of a cave. */
    /* as when playing a replay, it might change from playing replay to continuing replay. */
    switch (main_window.game->type) {
        /* for a normal cave, show number of lives or "bonus life" if it is an intermission */
        case GD_GAMETYPE_NORMAL:
            if (!cave->intermission)
                gd_label_set_markup_printf(GTK_LABEL(main_window.label_topright), _("Lives: <b>%d</b>"), main_window.game->player_lives);
            else
                gd_label_set_markup_printf(GTK_LABEL(main_window.label_topright), _("<b>Bonus life</b>"));
            break;
            
        /* for other game types, the number of lives is zero. so show the game type */
        case GD_GAMETYPE_SNAPSHOT:
            gd_label_set_markup_printf(GTK_LABEL(main_window.label_topright), _("Continuing from <b>snapshot</b>"));
            break;
        case GD_GAMETYPE_TEST:
            gd_label_set_markup_printf(GTK_LABEL(main_window.label_topright), _("<b>Testing</b> cave"));
            break;
        case GD_GAMETYPE_REPLAY:
            gd_label_set_markup_printf(GTK_LABEL(main_window.label_topright), _("Playing <b>replay</b>"));
            break;
        case GD_GAMETYPE_CONTINUE_REPLAY:
            gd_label_set_markup_printf(GTK_LABEL(main_window.label_topright), _("Continuing <b>replay</b>"));
            break;
    }
    
    
    /* SECOND ROW OF STATUS BAR. */
    /* if the user is not pressing the left shift, show the normal status bar. otherwise, show the alternative. */
    if (!key_alt_status_bar) {
        /* NORMAL STATUS BAR */
        /* diamond value */
        if (cave->diamonds_needed>0)
            gd_label_set_markup_printf(GTK_LABEL(main_window.label_bottomleft), _("Diamonds: <b>%03d</b>  Value: <b>%02d</b>"), cave->diamonds_collected>=cave->diamonds_needed?0:cave->diamonds_needed-cave->diamonds_collected, cave->diamond_value);
        else
            gd_label_set_markup_printf(GTK_LABEL(main_window.label_bottomleft), _("Diamonds: <b>??""?</b>  Value: <b>%02d</b>"), cave->diamond_value);/* "" to avoid C trigraph ??< */

        /* show time & score */
        time=gd_cave_time_show(cave, cave->time);
        if (gd_time_min_sec)
            gd_label_set_markup_printf(GTK_LABEL(main_window.label_bottomright), "Time: <b>%02d:%02d</b>  Score: <b>%06d</b>", time/60, time%60, main_window.game->player_score);
        else
            gd_label_set_markup_printf(GTK_LABEL(main_window.label_bottomright), "Time: <b>%03d</b>  Score: <b>%06d</b>", time, main_window.game->player_score);
    } else {
        /* ALTERNATIVE STATUS BAR */
        gd_label_set_markup_printf(GTK_LABEL(main_window.label_bottomleft), _("Keys: <b>%d</b>, <b>%d</b>, <b>%d</b>"), cave->key1, cave->key2, cave->key3);
        gd_label_set_markup_printf(GTK_LABEL(main_window.label_bottomright), _("Skeletons: <b>%d</b>  Gravity change: <b>%d</b>"), cave->skeletons_collected, gd_cave_time_show(cave, cave->gravity_will_change));
    }

    if (gd_editor_window && gd_show_test_label) {
        gd_label_set_markup_printf(GTK_LABEL(main_window.label_variables),
                                _("Speed: %dms, Amoeba 1: %ds %s, 2: %ds %s, Magic wall: %ds %s\n"
                                "Expanding wall: %s, Creatures: %ds, %s, Gravity: %s\n"
                                "Kill player: %s, Sweet eaten: %s, Diamond key: %s, Diamonds: %d"),
                                cave->speed,
                                gd_cave_time_show(cave, cave->amoeba_time),
                                amoeba_state_string(cave->amoeba_state),
                                gd_cave_time_show(cave, cave->amoeba_2_time),
                                amoeba_state_string(cave->amoeba_2_state),
                                gd_cave_time_show(cave, cave->magic_wall_time),
                                magic_wall_state_string(cave->magic_wall_state),
                                cave->expanding_wall_changed?_("vertical"):_("horizontal"),
                                gd_cave_time_show(cave, cave->creatures_direction_will_change),
                                cave->creatures_backwards?_("backwards"):_("forwards"),
                                gettext(gd_direction_get_visible_name(cave->gravity_disabled?MV_STILL:cave->gravity)),
                                cave->kill_player?_("yes"):_("no"),
                                cave->sweet_eaten?_("yes"):_("no"),
                                cave->diamond_key_collected?_("yes"):_("no"),
                                cave->diamonds_collected
                                );
    }
}


/* THE MAIN GAME TIMER */
/* called at 50hz */
/* for the gtk version, it seems nicer if we first draw, then scroll. */
/* this is because there is an expose event; scrolling the "old" drawing would draw the old, and then the new. */
/* (this is not true for the sdl version) */
#if 0
static GTimer *timer=NULL;
static int called=0;
#endif



/* SCROLLING
 *
 * scrolls to the player during game play.
 */
static void
main_int_scroll()
{
    static int scroll_desired_x=0, scroll_desired_y=0;
    GtkAdjustment *adjustment;
    int scroll_center_x, scroll_center_y;
    gboolean out_of_window=FALSE;
    gboolean changed;
    int i;
    int player_x, player_y;
    const GdCave *cave=main_window.game->cave;
    gboolean exact_scroll=FALSE;    /* to avoid compiler warning */
    /* hystheresis size is this, multiplied by two.
     * so player can move half the window without scrolling. */
    int scroll_start_x=main_window.scroll_window->allocation.width/4;
    int scroll_to_x=main_window.scroll_window->allocation.width/8;
    int scroll_start_y=main_window.scroll_window->allocation.height/5;    /* window usually smaller vertically, so let the region be also smaller than the horizontal one */
    int scroll_to_y=main_window.scroll_window->allocation.height/10;
    int scroll_speed;

    /* if cave not yet rendered, return. (might be the case for 50hz scrolling */
    if (main_window.game==NULL || main_window.game->cave==NULL)
        return;    
    if (paused && main_window.game->cave->player_state!=GD_PL_NOT_YET)
        return;    /* no scrolling when pause button is pressed, BUT ALLOW SCROLLING when the player is not yet born */    
        
    /* max scrolling speed depends on the speed of the cave. */
    /* game moves cell_size_game* 1s/cave time pixels in a second. */
    /* scrolling moves scroll speed * 1s/scroll_time in a second. */
    /* these should be almost equal; scrolling speed a little slower. */
    /* that way, the player might reach the border with a small probability, */
    /* but the scrolling will not "oscillate", ie. turn on for little intervals as it has */
    /* caught up with the desired position. smaller is better. */
    scroll_speed=gd_cell_size_game*(gd_fine_scroll?20:40)/cave->speed;
        
    /* check player state. */
    switch (main_window.game->cave->player_state) {
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
            return;    /* return from function */
    }

    player_x=cave->player_x-cave->x1;
    player_y=cave->player_y-cave->y1;

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

    changed=FALSE;
    if (adjustment->value<scroll_desired_x) {
        for (i=0; i<scroll_speed; i++)
            if ((int)adjustment->value<scroll_desired_x)
                adjustment->value++;
        changed=TRUE;
    }
    if (adjustment->value>scroll_desired_x) {
        for (i=0; i<scroll_speed; i++)
            if ((int)adjustment->value>scroll_desired_x)
                adjustment->value--;
        changed=TRUE;
    }
    if (changed)
        gtk_adjustment_value_changed (adjustment);

    /* check if active player is outside drawing area. if yes, we should wait for scrolling */
    if ((player_x*gd_cell_size_game)<adjustment->value || (player_x*gd_cell_size_game+gd_cell_size_game-1)>adjustment->value+main_window.drawing_area->parent->parent->allocation.width)
        /* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
        if (cave->player_x>=cave->x1 && cave->player_x<=cave->x2)
            out_of_window=TRUE;




    /* VERTICAL */
    adjustment=gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(main_window.scroll_window));
    if (exact_scroll)
        scroll_desired_y=scroll_center_y;
    else {
        if (adjustment->value+scroll_start_y<scroll_center_y)
            scroll_desired_y=scroll_center_y-scroll_to_y;
        if (adjustment->value-scroll_start_y>scroll_center_y)
            scroll_desired_y=scroll_center_y+scroll_to_y;
    }
    scroll_desired_y=CLAMP(scroll_desired_y, 0, adjustment->upper-adjustment->step_increment-adjustment->page_increment);

    changed=FALSE;
    if (adjustment->value<scroll_desired_y) {
        for (i=0; i<scroll_speed; i++)
            if ((int)adjustment->value<scroll_desired_y)
                adjustment->value++;
        changed=TRUE;
    }
    if (adjustment->value > scroll_desired_y) {
        for (i=0; i<scroll_speed; i++)
            if ((int)adjustment->value>scroll_desired_y)
                adjustment->value--;
        changed=TRUE;
    }
    if (changed)
        gtk_adjustment_value_changed(adjustment);

    /* check if active player is outside drawing area. if yes, we should wait for scrolling */
    if ((player_y*gd_cell_size_game)<adjustment->value || (player_y*gd_cell_size_game+gd_cell_size_game-1)>adjustment->value+main_window.drawing_area->parent->parent->allocation.height)
        /* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
        if (cave->player_y>=cave->y1 && cave->player_y<=cave->y2)
            out_of_window=TRUE;

    /* remember if player is visible inside window */
    main_window.game->out_of_window=out_of_window;

    /* if not yet born, we treat as visible. so cave will run. the user is unable to control an unborn player, so this is the right behaviour. */
    if (main_window.game->cave->player_state==GD_PL_NOT_YET)
        main_window.game->out_of_window=FALSE;
        
    /* XXX    gdk_window_process_updates(main_window.drawing_area->window, TRUE); */
}

/* the timing thread runs in a separate thread. this variable is set to true,
 * then the function exits (and also the thread.) */
static gboolean main_int_quit_thread=FALSE;

static gboolean
main_int(gpointer data)
{
    static gboolean toggle=FALSE;    /* value irrelevant */
    int up, down, left, right;
    GdDirection player_move;
    gboolean fire;
    GdGameState state;
    
#if 0
    called++;
    if (called%100==0)
        g_message("%8d. call, avg %gms", called, g_timer_elapsed(timer, NULL)*1000/called);
#endif

    if (main_window.game->type==GD_GAMETYPE_REPLAY)
        gtk_widget_show(main_window.replay_image_align);
    else
        gtk_widget_hide(main_window.replay_image_align);
    
    up=key_up;
    down=key_down;
    left=key_left;
    right=key_right;
    fire=key_fire_1 || key_fire_2;

    /* compare mouse coordinates to player coordinates, and make up movements */
    if (gd_mouse_play && mouse_cell_x>=0) {
        down=down || (main_window.game->cave->player_y<mouse_cell_y);
        up=up || (main_window.game->cave->player_y>mouse_cell_y);
        left=left || (main_window.game->cave->player_x>mouse_cell_x);
        right=right || (main_window.game->cave->player_x<mouse_cell_x);
        fire=fire || mouse_cell_click;
    }

    /* call the game "interrupt" to do all things. */
    player_move=gd_direction_from_keypress(up, down, left, right);
    /* tell the interrupt that 20ms has passed. */
    state=gd_game_main_int(main_window.game, 20, player_move, fire, key_suicide, restart, !paused && !main_window.game->out_of_window, paused, fast_forward);

    /* the game "interrupt" gives signals to us, which we act upon: update status bar, resize the drawing area... */
    switch (state) {
        case GD_GAME_INVALID_STATE:
            g_assert_not_reached();
            break;
            
        case GD_GAME_SHOW_STORY:
            main_window_init_story(main_window.game->cave);
            main_int_set_labels();
            break;
            
        case GD_GAME_CAVE_LOADED:
            gd_select_pixbuf_colors(main_window.game->cave->color0, main_window.game->cave->color1, main_window.game->cave->color2, main_window.game->cave->color3, main_window.game->cave->color4, main_window.game->cave->color5);
            main_window_init_cave(main_window.game->cave);
            main_int_set_labels();
            restart=FALSE;    /* so we do not remember the restart key from a previous cave run */
            break;

        case GD_GAME_NO_MORE_LIVES:    /* <- only used by sdl version */
        case GD_GAME_SHOW_STORY_WAIT:    /* <- only used by sdl version */
        case GD_GAME_NOTHING:
            /* normally continue. */
            break;

        case GD_GAME_LABELS_CHANGED:
        case GD_GAME_TIMEOUT_NOW:    /* <- maybe we should do something else for this */
            /* normal, but we are told that the labels (score, ...) might have changed. */
            main_int_set_labels();
            break;

        case GD_GAME_STOP:
            gd_main_stop_game();
            return FALSE;    /* do not call again - it will be created later */

        case GD_GAME_GAME_OVER:
            main_stop_game_but_maybe_highscore();
            if (gd_is_highscore(gd_caveset_data->highscore, main_window.game->player_score))
                game_over_highscore();            /* achieved a high score! */
            else
                game_over_without_highscore();            /* no high score */
            main_free_game();
            return FALSE;    /* do not call again - it will be created later */
    }

    /* if drawing area already exists, draw cave. */
    /* remember that the drawings are cached, so if we did no change, this will barely do anything - so will not slow down. */
    if (main_window.drawing_area)
        schedule_draw();
    /* do the scrolling at the given interval. */
    /* but only if the drawing area already exists. */
    /* if fine scrolling, drawing is called at a 50hz rate. */
    /* if not, only at a 25hz rate */
    toggle=!toggle;
    if (main_window.drawing_area && (gd_fine_scroll || toggle))
        main_int_scroll();

    return TRUE;    /* call again */
}

/* this is a simple wrapper which makes the main_int to be callable as an idle func. */
/* when used as an idle func by the thread routine, it should run only once for every g_idle_add */
static gboolean
main_int_return_false_wrapper(gpointer data)
{
    main_int(data);
    return FALSE;
}

/* this function will run in its own thread, and add the main int as an idle func in every 20ms */
static gpointer
main_int_timer_thread(gpointer data)
{
    int interval_msec=20;

    /* wait before we first call it */    
    g_usleep(interval_msec*1000);
    while (!main_int_quit_thread) {
        /* add processing as an idle func */
        /* no need to lock the main loop context, as glib does that automatically */
        g_idle_add(main_int_return_false_wrapper, data);

        g_usleep(interval_msec*1000);
    }
    return NULL;
}

/*
    uninstall game timers, if any installed.
*/
static void
main_int_uninstall_timer()
{
    main_int_quit_thread=TRUE;
    /* remove timeout associated to game play */
    while (g_source_remove_by_user_data(main_window.window)) {
        /* nothing */
    }
}


static void
main_int_install_timer()
{
    GThread *thread;
    GError *error=NULL;
    
    /* remove timer, if it is installed for some reason */
    main_int_uninstall_timer();

    if (!paused)
        gtk_window_present(GTK_WINDOW(main_window.window));

#if 0
    if (!timer)
        timer=g_timer_new();
#endif

    /* this makes the main int load the first cave, and then we do the drawing. */
    main_int(main_window.window);
    gdk_window_process_all_updates();    /* so resizes will be done (?) */
    /* after that, install timer. create a thread with higher priority than normal: */
    /* so its priority will be higher than the main thread, which does the drawing etc. */
    /* if the scheduling thread wants to do something, it gets processed first. this makes */
    /* the intervals more even. */
    main_int_quit_thread=FALSE;
#ifdef G_THREADS_ENABLED
    thread=g_thread_create_full(main_int_timer_thread, main_window.window, 0, FALSE, FALSE, G_THREAD_PRIORITY_HIGH, &error);
#else
    thread=NULL;    /* if glib without thread support, we will use timeout source. */
#endif
    if (!thread) {
        /* if unable to create thread */
        if (error) {
            g_critical("%s", error->message);
            g_error_free(error);
        }
        /* use the main int as a timeout routine. */
        g_timeout_add(20, main_int, main_window.window);
    }
    
#if 0
    g_timer_start(timer);
    called=0;
#endif
}

static void
main_stop_game_but_maybe_highscore()
{
    main_int_uninstall_timer();
    gd_sound_off();    /* hack for game over dialog */

    main_window_init_title();
    /* if editor is active, go back to its window. */
    if (gd_editor_window)
        gtk_window_present(GTK_WINDOW(gd_editor_window));
}

static void
main_free_game()
{
    if (main_window.game) {
        gd_game_free(main_window.game);
        main_window.game=NULL;
    }
}

void
gd_main_stop_game()
{
    main_stop_game_but_maybe_highscore();
    main_free_game();
}



/* this starts a new game */
static void
main_new_game(const char *player_name, const int cave, const int level)
{
    if (main_window.game)
        gd_main_stop_game();
    
    main_window.game=gd_game_new(player_name, cave, level);
    main_int_install_timer();
}


static void
main_new_game_snapshot(GdCave *snapshot)
{
    if (main_window.game)
        gd_main_stop_game();
    
    main_window.game=gd_game_new_snapshot(snapshot);
    main_int_install_timer();
}

/* the new game for testing a level is global, not static.
 * it is used by the editor. */
void
gd_main_new_game_test(GdCave *test, int level)
{
    if (main_window.game)
        gd_main_stop_game();
    
    main_window.game=gd_game_new_test(test, level);
    main_int_install_timer();
}


static void
main_new_game_replay(GdCave *cave, GdReplay *replay)
{
    if (main_window.game)
        gd_main_stop_game();
    
    main_window.game=gd_game_new_replay(cave, replay);
    main_int_install_timer();
}








/*
 * set the main window title from the caveset name.
 * made global, as the editor might ocassionally call it.
 * 
 */
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





/**********************************-
 *
 *    CALLBACKS
 *
 */

static void
help_cb(GtkWidget * widget, gpointer data)
{
    gd_show_game_help (((GdMainWindow *)data)->window);
}

static void
preferences_cb(GtkWidget *widget, gpointer data)
{
    gd_preferences(((GdMainWindow *)data)->window);
}

static void
control_settings_cb(GtkWidget *widget, gpointer data)
{
    gd_control_settings(((GdMainWindow *)data)->window);
}

static void
quit_cb(GtkWidget * widget, const gpointer data)
{
    if (gd_discard_changes(main_window.window))
        gtk_main_quit ();
}

static void
stop_game_cb(GtkWidget *widget, gpointer data)
{
    gd_main_stop_game();
}

static void
save_snapshot_cb(GtkWidget * widget, gpointer data)
{
    if (snapshot)
        gd_cave_free(snapshot);
    
    snapshot=gd_create_snapshot(main_window.game);
    gtk_action_group_set_sensitive (main_window.actions_snapshot, snapshot!=NULL);
}

static void
load_snapshot_cb(GtkWidget * widget, gpointer data)
{
    g_return_if_fail(snapshot!=NULL);
    main_new_game_snapshot(snapshot);
}

/* restart level button clicked */
static void
restart_level_cb(GtkWidget * widget, gpointer data)
{
    g_return_if_fail(main_window.game!=NULL);
    g_return_if_fail(main_window.game->cave!=NULL);
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
    main_window_init_title();
}

static void
open_caveset_dir_cb(GtkWidget * widget, gpointer data)
{
    gd_open_caveset(main_window.window, gd_system_caves_dir);
    main_window_init_title();
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
    main_window_init_title();
}

static void
highscore_cb(GtkWidget *widget, gpointer data)
{
    gd_show_highscore(main_window.window, NULL, FALSE, NULL, -1);
}



static void
show_errors_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_hide(main_window.info_bar);    /* if the user is presented the error list, the label is to be hidden */
    gd_show_errors(main_window.window);
}

static void
cave_editor_cb()
{
    gd_open_cave_editor();
    /* to be sure no cave is playing. */
    /* this will also stop music. to be called after opening editor window, so the music stops. */
    main_window_init_title();
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
        gd_errormessage(_("GDash cannot load file from a network link."), display_name);
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
    main_window_init_title();
    gd_infomessage(_("Loaded caveset from file:"), filename_utf8);

    g_free(filename);
    g_free(filename_utf8);
}



static void
toggle_pause_cb(GtkWidget * widget, gpointer data)
{
    paused=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION (widget));

    if (paused)
        gd_sound_off();
}

static void
toggle_fullscreen_cb (GtkWidget * widget, gpointer data)
{
    fullscreen=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION (widget));
    main_window_set_fullscreen(main_window.game!=NULL);    /* we do not exactly know if in game, but try to guess */
}

static void
toggle_fast_cb (GtkWidget * widget, gpointer data)
{
    fast_forward=gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (widget));
}









/*
 * START NEW GAME DIALOG
 *
 * show a dialog to the user so he can select the cave to start game at.
 *
 */

typedef struct _new_game_dialog {
    GtkWidget *dialog;
    GtkWidget *combo_cave;
    GtkWidget *spin_level;
    GtkWidget *entry_name;

    GtkWidget *image;
} NewGameDialog;

/* keypress. key_* can be event_type==gdk_key_press, as we
   connected this function to key press and key release.
 */
static gboolean
new_game_keypress_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    NewGameDialog *jump_dialog=(NewGameDialog *)data;
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
    return FALSE;    /* if any other key, we did not process it. go on, let gtk handle it. */
}


/* update pixbuf */
static void
new_game_update_preview(GtkWidget *widget, gpointer data)
{
    NewGameDialog *jump_dialog=(NewGameDialog *)data;
    GdkPixbuf *cave_image;
    GdCave *cave;

    /* loading cave, draw cave and scale to specified size. seed=0 */
    cave=gd_cave_new_from_caveset(gtk_combo_box_get_active(GTK_COMBO_BOX (jump_dialog->combo_cave)), gtk_range_get_value (GTK_RANGE(jump_dialog->spin_level))-1, 0);
    cave_image=gd_drawcave_to_pixbuf(cave, 320, 240, TRUE, TRUE);
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
    NewGameDialog jump_dialog;
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
        GdCave *cave=iter->data;
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
    gtk_range_set_increments(GTK_RANGE(jump_dialog.spin_level), 1.0, 1.0);
    gtk_scale_set_value_pos(GTK_SCALE(jump_dialog.spin_level), GTK_POS_LEFT);
    gtk_table_attach_defaults(GTK_TABLE(table), jump_dialog.spin_level, 1, 2, 2, 3);

    g_signal_connect(G_OBJECT(jump_dialog.combo_cave), "changed", G_CALLBACK(new_game_update_preview), &jump_dialog);
    gtk_widget_add_events(eventbox, GDK_KEY_PRESS_MASK);
    g_signal_connect(G_OBJECT(eventbox), "key_press_event", G_CALLBACK(new_game_keypress_event), &jump_dialog);
    g_signal_connect(G_OBJECT(jump_dialog.spin_level), "value-changed", G_CALLBACK(new_game_update_preview), &jump_dialog);

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

    if (gtk_dialog_run (GTK_DIALOG(jump_dialog.dialog))==GTK_RESPONSE_ACCEPT) {
        gd_strcpy(player_name, gtk_entry_get_text(GTK_ENTRY(jump_dialog.entry_name)));
        gd_caveset_last_selected=gtk_combo_box_get_active(GTK_COMBO_BOX(jump_dialog.combo_cave));
        gd_caveset_last_selected_level=gtk_range_get_value(GTK_RANGE(jump_dialog.spin_level))-1;
        main_new_game (player_name, gd_caveset_last_selected, gd_caveset_last_selected_level);
    }
    gd_show_preview=gtk_expander_get_expanded(GTK_EXPANDER(expander));    /* remember expander state-even if cancel pressed */
    gtk_widget_destroy(jump_dialog.dialog);
}












enum _replay_fields {
    COL_REPLAY_CAVE_POINTER,
    COL_REPLAY_REPLAY_POINTER,
    COL_REPLAY_NAME,    /* cave or player name */
    COL_REPLAY_LEVEL,    /* level the replay is played on */
    COL_REPLAY_DATE,
    COL_REPLAY_SCORE,
    COL_REPLAY_SUCCESS,
    COL_REPLAY_SAVED,
    COL_REPLAY_COMMENT, /* set to a gtk stock icon if it has a comment */
    COL_REPLAY_VISIBLE, /* set to true for replay lines, false for cave lines. so "saved" toggle and comment are not visible. */
    COL_REPLAY_MAX,
};

static void
show_replays_tree_view_row_activated_cb(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    GtkTreeModel *model=gtk_tree_view_get_model(view);
    GtkTreeIter iter;
    GdCave *cave;
    GdReplay *replay;
    
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, COL_REPLAY_CAVE_POINTER, &cave, COL_REPLAY_REPLAY_POINTER, &replay, -1);
    if (cave!=NULL && replay!=NULL)
        main_new_game_replay(cave, replay);
}

static void
show_replays_dialog_response(GtkDialog *dialog, int response_id, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
show_replays_saved_toggled(GtkCellRendererText *cell, const gchar *path_string, gpointer data)
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
show_replays_play_button_clicked(GtkWidget *widget, gpointer data)
{
    GtkTreeView *view=GTK_TREE_VIEW(data);
    GtkTreeSelection *selection=gtk_tree_view_get_selection(view);
    GtkTreeModel *model;
    gboolean got_selected;
    GtkTreeIter iter;
    GdReplay *replay;
    GdCave *cave;
    
    got_selected=gtk_tree_selection_get_selected(selection, &model, &iter);
    if (!got_selected)    /* if nothing selected, return */
        return;

    gtk_tree_model_get(model, &iter, COL_REPLAY_CAVE_POINTER, &cave, COL_REPLAY_REPLAY_POINTER, &replay, -1);
    if (cave!=NULL && replay!=NULL)
        main_new_game_replay(cave, replay);
}

/* edit a replay comment for a given cave and a given replay.
   return true if comment is non empty.
*/
static gboolean
show_replays_edit_comment(GtkWindow *parent, GdCave *cave, GdReplay *replay)
{
    GtkWidget *dialog;
    GtkTextBuffer *buffer;
    GtkWidget *text, *sw;
    int response;
    
    dialog=gtk_dialog_new_with_buttons(_("Edit Replay Comment"), parent,
        GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_NO_SEPARATOR,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 480, 360);
    sw=gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    buffer=gtk_text_buffer_new(NULL);
    text=gtk_text_view_new_with_buffer(buffer);
    gtk_text_buffer_set_text(buffer, replay->comment->str, -1);
    gtk_container_add(GTK_CONTAINER(sw), text);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), sw);
    
    gtk_widget_show_all(dialog);
    response=gtk_dialog_run(GTK_DIALOG(dialog));
    if (response==GTK_RESPONSE_OK) {
        GtkTextIter iter_start, iter_end;
        char *text;
        
        gtk_text_buffer_get_iter_at_offset(buffer, &iter_start, 0);
        gtk_text_buffer_get_iter_at_offset(buffer, &iter_end, -1);
        text=gtk_text_buffer_get_text(buffer, &iter_start, &iter_end, TRUE);
        g_string_assign(replay->comment, text);
        gd_caveset_edited=TRUE;
        g_free(text);
    }
    gtk_widget_destroy(dialog);
    return replay->comment->len!=0;
}

static void
show_replays_edit_button_clicked(GtkWidget *widget, gpointer data)
{
    GtkTreeView *view=GTK_TREE_VIEW(data);
    GtkTreeSelection *selection=gtk_tree_view_get_selection(view);
    GtkTreeModel *model;
    gboolean got_selected;
    GtkTreeIter iter;
    GdReplay *replay;
    GdCave *cave;
    
    got_selected=gtk_tree_selection_get_selected(selection, &model, &iter);
    if (!got_selected)    /* if nothing selected, return */
        return;

    gtk_tree_model_get(model, &iter, COL_REPLAY_CAVE_POINTER, &cave, COL_REPLAY_REPLAY_POINTER, &replay, -1);
    if (cave!=NULL && replay!=NULL) {
        gboolean has_comment;
        
        has_comment=show_replays_edit_comment(GTK_WINDOW(gtk_widget_get_toplevel(widget)), cave, replay);
        gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COL_REPLAY_COMMENT, has_comment?GTK_STOCK_EDIT:"", -1);
    }
}



/* enables or disables play button on selection change */
static void
show_replays_tree_view_selection_changed(GtkTreeSelection *selection, gpointer data)
{
    GtkTreeModel *model;
    gboolean enable;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        GdReplay *replay;
        
        gtk_tree_model_get(model, &iter, COL_REPLAY_REPLAY_POINTER, &replay, -1);
        enable=replay!=NULL;
    } else
        enable=FALSE;

    gtk_widget_set_sensitive(GTK_WIDGET(data), enable);
}

static void
show_replays_cb(GtkWidget *widget, gpointer data)
{
    static GtkWidget *dialog=NULL;
    GtkWidget *scroll, *view, *button;
    GList *iter;
    GtkTreeStore *model;
    GtkCellRenderer *renderer;
    
    /* if window already open, just show it and return */
    if (dialog) {
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }
    
    dialog=gtk_dialog_new_with_buttons(_("Replays"), GTK_WINDOW(main_window.window), 0, NULL);
    g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(gtk_widget_destroyed), &dialog);
    g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(show_replays_dialog_response), NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 480, 360);
    
    gd_dialog_add_hint(GTK_DIALOG(dialog), _("Hint: When watching a replay, you can use the usual movement keys (left, right...) to "
    "stop the replay and immediately continue the playing of the cave yourself."));
    
    /* scrolled window to show replays tree view */
    scroll=gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scroll, TRUE, TRUE, 6);

    /* create store containing replays */
    model=gtk_tree_store_new(COL_REPLAY_MAX, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN);
    for (iter=gd_caveset; iter!=NULL; iter=iter->next) {
        GList *replayiter;
        GdCave *cave=(GdCave *)iter->data;
        
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
                const char *imagestock;
                
                /* we have to store the score as string, as for the cave lines the unset score field would also show zero */
                g_snprintf(score, sizeof(score), "%d", replay->score);
                gtk_tree_store_append(model, &riter, &caveiter);
                if (replay->wrong_checksum)
                    imagestock=GTK_STOCK_DIALOG_ERROR;
                else
                    imagestock=replay->success?GTK_STOCK_YES:GTK_STOCK_NO;
                gtk_tree_store_set(model, &riter, COL_REPLAY_CAVE_POINTER, cave, COL_REPLAY_REPLAY_POINTER, replay, COL_REPLAY_NAME, replay->player_name,
                    COL_REPLAY_LEVEL, replay->level+1, 
                    COL_REPLAY_DATE, replay->date, COL_REPLAY_SCORE, score, COL_REPLAY_SUCCESS, imagestock,
                    COL_REPLAY_COMMENT, replay->comment->len!=0?GTK_STOCK_EDIT:"", COL_REPLAY_SAVED, replay->saved, COL_REPLAY_VISIBLE, TRUE, -1);
            }
        }
    }
    
    view=gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));    /* create tree view which will show data */
    gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
    gtk_container_add(GTK_CONTAINER(scroll), view);

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 0, _("Name"), renderer, "text", COL_REPLAY_NAME, NULL);    /* 0 = column number */
    gtk_tree_view_column_set_expand(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 0), TRUE);    /* name column expands */

    renderer=gtk_cell_renderer_text_new();
    /* TRANSLATORS: "Lvl" here stands for Level. Some shorthand should be used.*/
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 1, _("Lvl"), renderer, "text", COL_REPLAY_LEVEL, "visible", COL_REPLAY_VISIBLE, NULL);    /* 0 = column number */
    gtk_tree_view_column_set_expand(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 0), TRUE);    /* name column expands */

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 2, _("Date"), renderer, "text", COL_REPLAY_DATE, NULL);    /* 1 = column number */

    renderer=gtk_cell_renderer_pixbuf_new();
    g_object_set(G_OBJECT(renderer), "stock-size", GTK_ICON_SIZE_MENU, NULL);
    /* TRANSLATORS: "S." here stands for Successful. A one-letter shorthand should be used.*/
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 3, _("S."), renderer, "stock-id", COL_REPLAY_SUCCESS, NULL);

    renderer=gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 4, _("Score"), renderer, "text", COL_REPLAY_SCORE, NULL);

    renderer=gtk_cell_renderer_pixbuf_new();
    g_object_set(G_OBJECT(renderer), "stock-size", GTK_ICON_SIZE_MENU, NULL);
    /* TRANSLATORS: "C." here stands for Comment. A one-letter shorthand should be used.*/
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 5, _("C."), renderer, "stock-id", COL_REPLAY_COMMENT, NULL);

    renderer=gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled", G_CALLBACK(show_replays_saved_toggled), model);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), 6, _("Saved"), renderer, "active", COL_REPLAY_SAVED, "visible", COL_REPLAY_VISIBLE, NULL);

    /* doubleclick will play the replay */
    g_signal_connect(G_OBJECT(view), "row-activated", G_CALLBACK(show_replays_tree_view_row_activated_cb), NULL);
    gtk_tree_view_column_set_expand(gtk_tree_view_get_column(GTK_TREE_VIEW(view), 4), TRUE);    /* name column expands */

    /* play button */
    button=gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
    gtk_widget_set_sensitive(button, FALSE);    /* not sensitive by default. when the user selects a line, it will be enabled */
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_replays_play_button_clicked), view);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(view))), "changed", G_CALLBACK(show_replays_tree_view_selection_changed), button);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area), button);
    
    /* comment edit button */
    button=gtk_button_new_from_stock(GTK_STOCK_EDIT);
    gtk_widget_set_sensitive(button, FALSE);    /* not sensitive by default. when the user selects a line, it will be enabled */
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_replays_edit_button_clicked), view);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(view))), "changed", G_CALLBACK(show_replays_tree_view_selection_changed), button);
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
        if (!g_str_equal(gd_caveset_data->story->str, "")) {
            gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Story:\n"), -1, "name", NULL);
            gtk_text_buffer_insert(buffer, &iter, gd_caveset_data->story->str, -1);
            gtk_text_buffer_insert(buffer, &iter, "\n", -1);
        }
        if (!g_str_equal(gd_caveset_data->remark->str, "")) {
            gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Remark:\n"), -1, "name", NULL);
            gtk_text_buffer_insert(buffer, &iter, gd_caveset_data->remark->str, -1);
            gtk_text_buffer_insert(buffer, &iter, "\n", -1);
        }
    }
    else {
        /* cave data */
        GdCave *cave=gd_return_nth_cave(i-1);

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

        if (!g_str_equal(cave->story->str, "")) {
            gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Story:\n"), -1, "name", NULL);
            gtk_text_buffer_insert(buffer, &iter, cave->story->str, -1);
            gtk_text_buffer_insert(buffer, &iter, "\n", -1);
        }
        if (!g_str_equal(cave->remark->str, "")) {
            gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, _("Remark:\n"), -1, "name", NULL);
            gtk_text_buffer_insert(buffer, &iter, cave->remark->str, -1);
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
    text=g_strdup_printf("[%s]", gd_caveset_data->name);    /* caveset name = line 0 */
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text);
    g_free(text);
    for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
        GdCave *c=citer->data;

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
    if (main_window.game && main_window.game->original_cave)
        /* currently playing a cave - show info for that */
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), g_list_index(gd_caveset, main_window.game->original_cave)+1);
    else
        /* show info for caveset */
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    

    gtk_widget_show_all(dialog);
    paused_save=paused;
    paused=TRUE;    /* set paused game, so it stops while the users sees the message box */
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    paused=paused_save;
}


/*
 *
 * Creates main window
 *
 *
 */
static void
create_main_window()
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
        {"CaveInfo", GTK_STOCK_DIALOG_INFO, N_("Caveset _information"), NULL, N_("Show information about the game and its caves"), G_CALLBACK(cave_info_cb)},
    };

    static GtkActionEntry action_entries_title[]={
        {"GamePreferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK(preferences_cb)},
        {"GameControlSettings", GD_ICON_KEYBOARD, N_("_Control keys"), NULL, NULL, G_CALLBACK(control_settings_cb)},
        {"NewGame", GTK_STOCK_MEDIA_PLAY, N_("_New game"), "<control>N", N_("Start new game"), G_CALLBACK(new_game_cb)},
        {"CaveEditor", GD_ICON_CAVE_EDITOR, N_("Cave _editor"), NULL, NULL, G_CALLBACK(cave_editor_cb)},
        {"OpenFile", GTK_STOCK_OPEN, NULL, NULL, NULL, G_CALLBACK(open_caveset_cb)},
        {"LoadInternal", GTK_STOCK_INDEX, N_("Load _internal game")},
        {"LoadRecent", GTK_STOCK_DIALOG_INFO, N_("Open _recent")},
        {"OpenCavesDir", GTK_STOCK_CDROM, N_("O_pen shipped"), NULL, NULL, G_CALLBACK(open_caveset_dir_cb)},
        {"SaveFile", GTK_STOCK_SAVE, NULL, NULL, NULL, G_CALLBACK(save_caveset_cb)},
        {"SaveAsFile", GTK_STOCK_SAVE_AS, NULL, NULL, NULL, G_CALLBACK(save_caveset_as_cb)},
        {"HighScore", GD_ICON_AWARD, N_("Hi_ghscores"), NULL, NULL, G_CALLBACK(highscore_cb)},
    };

    static GtkActionEntry action_entries_title_replay[]={
        {"ShowReplays", GD_ICON_REPLAY, N_("Show _replays"), NULL, N_("List replays which are recorded for caves in this caveset"), G_CALLBACK(show_replays_cb)},
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
        "<menuitem action='GameControlSettings'/>"
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
        "<toolitem action='ShowReplays'/>"
        "</toolbar>"
        "</ui>";

    GtkWidget *vbox, *hbox, *recent_chooser, *bar, *button;
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
    g_signal_connect(G_OBJECT(main_window.window), "focus_out_event", G_CALLBACK(main_window_focus_out_event), NULL);
    g_signal_connect(G_OBJECT(main_window.window), "delete_event", G_CALLBACK(main_window_delete_event), NULL);
    g_signal_connect(G_OBJECT(main_window.window), "key_press_event", G_CALLBACK(main_window_keypress_event), NULL);
    g_signal_connect(G_OBJECT(main_window.window), "key_release_event", G_CALLBACK(main_window_keypress_event), NULL);

    /* vertical box */
    vbox=gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (main_window.window), vbox);

    /* menu */
    main_window.actions_normal=gtk_action_group_new("main_window.actions_normal");
    gtk_action_group_set_translation_domain(main_window.actions_normal, PACKAGE);
    gtk_action_group_add_actions(main_window.actions_normal, action_entries_normal, G_N_ELEMENTS(action_entries_normal), &main_window);
    gtk_action_group_add_toggle_actions(main_window.actions_normal, action_entries_toggle, G_N_ELEMENTS(action_entries_toggle), NULL);
    main_window.actions_title=gtk_action_group_new("main_window.actions_title");
    gtk_action_group_set_translation_domain(main_window.actions_title, PACKAGE);
    gtk_action_group_add_actions(main_window.actions_title, action_entries_title, G_N_ELEMENTS(action_entries_title), &main_window);
    main_window.actions_title_replay=gtk_action_group_new("main_window.actions_title_replay");
    gtk_action_group_set_translation_domain(main_window.actions_title_replay, PACKAGE);
    gtk_action_group_add_actions(main_window.actions_title_replay, action_entries_title_replay, G_N_ELEMENTS(action_entries_title_replay), &main_window);
    /* make this toolbar button always have a title */
    g_object_set (gtk_action_group_get_action (main_window.actions_title, "NewGame"), "is_important", TRUE, NULL);
    main_window.actions_game=gtk_action_group_new("main_window.actions_game");
    gtk_action_group_set_translation_domain(main_window.actions_game, PACKAGE);
    gtk_action_group_add_actions(main_window.actions_game, action_entries_game, G_N_ELEMENTS(action_entries_game), &main_window);
    main_window.actions_snapshot=gtk_action_group_new("main_window.actions_snapshot");
    gtk_action_group_set_translation_domain(main_window.actions_snapshot, PACKAGE);
    gtk_action_group_add_actions(main_window.actions_snapshot, action_entries_snapshot, G_N_ELEMENTS(action_entries_snapshot), &main_window);

    /* build the ui */
    ui=gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group (ui, main_window.actions_normal, 0);
    gtk_ui_manager_insert_action_group (ui, main_window.actions_title, 0);
    gtk_ui_manager_insert_action_group (ui, main_window.actions_title_replay, 0);
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
        GtkWidget *menuitem=gtk_menu_item_new_with_label(names[i]);

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
    gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(recent_chooser), TRUE);
    gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent_chooser), 10);
    gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(recent_chooser), GTK_RECENT_SORT_MRU);
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
    main_window.label_topleft=gtk_label_new(NULL);    /* NAME label */
    gtk_label_set_ellipsize(GTK_LABEL(main_window.label_topleft), PANGO_ELLIPSIZE_END);    /* enable ellipsize, as the cave name might be a long string */
    gtk_misc_set_alignment(GTK_MISC(main_window.label_topleft), 0, 0.5);    /* as it will be expanded, we must set left justify (0) */
    gtk_box_pack_start(GTK_BOX(hbox), main_window.label_topleft, TRUE, TRUE, 0);    /* should expand, as it is ellipsized!! */

    gtk_box_pack_end(GTK_BOX(hbox), main_window.label_topright=gtk_label_new(NULL), FALSE, FALSE, 0);

    /* second row of labels */
    hbox=gtk_hbox_new(FALSE, 12);
    gtk_box_pack_start(GTK_BOX(main_window.labels), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), main_window.label_bottomleft=gtk_label_new(NULL), FALSE, FALSE, 0);    /* diamonds label */
    gtk_box_pack_end(GTK_BOX(hbox), main_window.label_bottomright=gtk_label_new(NULL), FALSE, FALSE, 0);    /* time, score label */


    /* scroll window which contains the cave or the title image, so this is the main content of the window */
    main_window.scroll_window=gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(main_window.scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start_defaults(GTK_BOX (vbox), main_window.scroll_window);

    main_window.label_variables=gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX (vbox), main_window.label_variables, FALSE, FALSE, 0);

    /* info bar */
    bar=gtk_info_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), bar, FALSE, FALSE, 0);
    gtk_info_bar_set_message_type(GTK_INFO_BAR(bar), GTK_MESSAGE_ERROR);
    main_window.info_label=gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(gtk_info_bar_get_content_area(GTK_INFO_BAR(bar))), main_window.info_label, FALSE, FALSE, 0);    /* error label */
    main_window.info_bar=bar;
    button=gtk_button_new_with_mnemonic(_("_Show"));
    gtk_box_pack_start(GTK_BOX(gtk_info_bar_get_action_area(GTK_INFO_BAR(bar))), button, FALSE, FALSE, 0);    /* error label */
    g_signal_connect(button, "clicked", G_CALLBACK(show_errors_cb), NULL);

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
    gboolean force_quit_no_gtk;

    GError *error=NULL;
    GOptionContext *context;
    GOptionEntry entries[]={
        {"editor", 'e', 0, G_OPTION_ARG_NONE, &editor, N_("Start editor")},
        {"gallery", 'g', 0, G_OPTION_ARG_FILENAME, &gallery_filename, N_("Save caveset in a HTML gallery")},
        {"stylesheet", 's', 0, G_OPTION_ARG_STRING    /* not filename! */, &gd_html_stylesheet_filename, N_("Link stylesheet from file to a HTML gallery, eg. \"../style.css\"")},
        {"favicon", 's', 0, G_OPTION_ARG_STRING    /* not filename! */, &gd_html_favicon_filename, N_("Link shortcut icon to a HTML gallery, eg. \"../favicon.ico\"")},
        {"png", 'p', 0, G_OPTION_ARG_FILENAME, &png_filename, N_("Save cave C, level L in a PNG image. If no cave selected, uses a random one")},
        {"png_size", 'P', 0, G_OPTION_ARG_STRING, &png_size, N_("Set PNG image size. Default is 128x96, set to 0x0 for unscaled")},
        {"save", 'S', 0, G_OPTION_ARG_FILENAME, &save_cave_name, N_("Save caveset in a BDCFF file")},
        {"quit", 'q', 0, G_OPTION_ARG_NONE, &quit, N_("Batch mode: quit after specified tasks")},
        {NULL}
    };

    context=gd_option_context_new();
    g_option_context_add_main_entries (context, entries, PACKAGE);    /* gdash (gtk version) parameters */
    g_option_context_add_group (context, gtk_get_option_group(FALSE));    /* add gtk parameters */
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
    
    force_quit_no_gtk=FALSE;
    if (!gtk_init_check(&argc, &argv)) {
        force_quit_no_gtk=TRUE;
    }

    gd_settings_init_translation();

    gd_cave_init();
    gd_cave_db_init();
    gd_cave_sound_db_init();
    gd_c64_import_init_tables();

    gd_caveset_clear();    /* this also creates the default cave */

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
    else
    /* if nothing requested, load default */
        gd_caveset_load_from_internal(0, gd_user_config_dir);

    /* always load c64 graphics, as it is the builtin, and we need an icon for the theme selector. */
    gd_loadcells_default();
    gd_create_pixbuf_for_builtin_theme();

    /* load other theme, if specified in config. */
    if (gd_theme!=NULL && !g_str_equal(gd_theme, "")) {
        if (!gd_loadcells_file(gd_theme)) {
            /* forget the theme if we are unable to load */
            g_warning("Cannot load theme %s, switching to built-in theme", gd_theme);
            g_free(gd_theme);
            gd_theme=NULL;
            gd_loadcells_default();    /* load default gfx */
        }
    }

    /* after loading cells... see if generating a gallery. */
    /* but only if there are any caves at all. */
    if (gd_caveset && gallery_filename)
        gd_save_html (gallery_filename, NULL);

    /* if cave or level values given, check range */
    if (gd_param_cave)
        if (gd_param_cave<1 || gd_param_cave>gd_caveset_count() || gd_param_level<1 || gd_param_level>5)
            g_critical (_("Invalid cave or level number!"));

    /* save cave png */
    if (png_filename) {
        GError *error=NULL;
        GdkPixbuf *pixbuf;
        GdCave *renderedcave;
        unsigned int size_x=128, size_y=96;    /* default size */

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
        pixbuf=gd_drawcave_to_pixbuf(renderedcave, size_x, size_y, TRUE, FALSE);
        if (!gdk_pixbuf_save (pixbuf, png_filename, "png", &error, "compression", "9", NULL))
            g_critical ("Error saving PNG image %s: %s", png_filename, error->message);
        g_object_unref(pixbuf);
        gd_cave_free (renderedcave);

        /* avoid starting game */
        gd_param_cave=0;
    }

    if (save_cave_name)
        gd_caveset_save(save_cave_name);

    /* if batch mode, quit now */
    if (quit)
        return 0;
    if (force_quit_no_gtk) {
        g_critical("Cannot initialize GUI");
        return 1;
    }

    /* create window */
    gd_register_stock_icons();

    create_main_window();
    gd_main_window_set_title();

    gd_sound_init(0);

#ifdef GD_SOUND
    gd_sound_set_music_volume(gd_sound_music_volume_percent);
    gd_sound_set_chunk_volumes(gd_sound_chunks_volume_percent);
#endif

    main_window_init_title();

#ifdef G_THREADS_ENABLED
    if (!g_thread_supported())
        g_thread_init(NULL);
#endif

    if (gd_param_cave) {
        /* if cave number given, start game */
        main_new_game(g_get_real_name(), gd_param_cave-1, gd_param_level-1);
    }
    else if (editor)
        cave_editor_cb(NULL, &main_window);

    gtk_main();

    gd_save_highscore(gd_user_config_dir);

    gd_save_settings();

    return 0;
}

