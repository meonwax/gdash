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

#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "cave/caveset.hpp"
#include "cave/cavestored.hpp"
#include "cave/titleanimation.hpp"
#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "cave/gamerender.hpp"
#include "editor/editorcellrenderer.hpp"
#include "settings.hpp"
#include "gtk/gtkpixbuffactory.hpp"

/**
 * Save caveset as html gallery.
 * @param htmlname filename
 * @param window show progress bar in a small dialog, transient for this window, if not NULL
 */
void gd_save_html(char *htmlname, GtkWidget *window, CaveSet &caveset) {
    char *pngbasename;      /* used as a base for img src= tags */
    char *pngoutbasename;   /* used as a base name for png output files */
    GtkWidget *dialog=NULL, *progress=NULL;
    std::string contents;
    GError *error=NULL;

    if (window) {
        dialog=gtk_dialog_new();
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        progress=gtk_progress_bar_new();
        gtk_window_set_title(GTK_WINDOW(dialog), _("Saving HTML gallery"));
        gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox), progress);
        gtk_widget_show_all(dialog);
    }

    if (g_str_has_suffix(htmlname, ".html")) {
        /* has html extension */
        pngoutbasename=g_strdup(htmlname);
        *g_strrstr(pngoutbasename, ".html")=0;
    } else {
        /* has no html extension */
        pngoutbasename=g_strdup(htmlname);
        htmlname=g_strdup_printf("%s.html", pngoutbasename);
    }
    pngbasename=g_path_get_basename(pngoutbasename);

    contents+="<HTML>\n";
    contents+="<HEAD>\n";
    contents+=SPrintf("<TITLE>%ms</TITLE>\n") % caveset.name;
    contents+="<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n";
    if (gd_html_stylesheet_filename)
        contents+=SPrintf("<link rel=\"stylesheet\" href=\"%s\">\n") % gd_html_stylesheet_filename;
    if (gd_html_favicon_filename)
        contents+=SPrintf("<link rel=\"shortcut icon\" href=\"%s\">\n") % gd_html_favicon_filename;
    contents+="</HEAD>\n\n";

    contents+="<BODY>\n";

    // CAVESET DATA
    contents+=SPrintf("<H1>%ms</H1>\n") % caveset.name;
    /* if the game has its own title screen */
    if (caveset.title_screen!="") {
        GTKPixbufFactory pf;

        /* create the title image and save it */
        std::vector<Pixbuf *> title_images = get_title_animation_pixbuf(caveset.title_screen, caveset.title_screen_scroll, true, pf);
        if (!title_images.empty()) {
            GdkPixbuf *title_image=static_cast<GTKPixbuf *>(title_images[0])->get_gdk_pixbuf();

            char *pngname=g_strdup_printf("%s_%03d.png", pngoutbasename, 0);  /* it is the "zeroth" image */
            gdk_pixbuf_save(title_image, pngname, "png", &error, "compression", "9", NULL);
            if (error) {
                gd_warning(error->message);
                g_error_free(error);
                error=NULL;
            }
            g_free(pngname);

            contents+=SPrintf("<IMAGE SRC=\"%s_%03d.png\" WIDTH=\"%d\" HEIGHT=\"%d\">\n") % pngbasename % 0 % gdk_pixbuf_get_width(title_image) % gdk_pixbuf_get_height(title_image);
            contents+="<BR>\n";

            delete title_images[0];
        }
    }
    contents+="<TABLE>\n";
    contents+=SPrintf("<TR><TH>%ms<TD>%d\n") % _("Caves") % caveset.caves.size();
    if (caveset.author!="")
        contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Author") % caveset.author;
    if (caveset.description!="")
        contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Description") % caveset.description;
    if (caveset.www!="")
        contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("WWW") % caveset.www;
    if (caveset.remark!="")
        contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Remark") % caveset.remark;
    if (caveset.story!="")
        contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Story") % caveset.story;
    contents+="</TABLE>\n";

    /* cave names, descriptions, hrefs */
    contents+="<DL>\n";
    for (unsigned n=0; n<caveset.caves.size(); n++) {
        CaveStored &cave=caveset.cave(n);

        contents+=SPrintf("<DT><A HREF=\"#cave%03d\">%s</A></DT>\n") % (n+1) % cave.name;
        if (cave.description!="")
            contents+=SPrintf("    <DD>%s</DD>\n")  % cave.description;
    }
    contents+="</DL>\n\n";

    GTKPixbufFactory pf;
    EditorCellRenderer cr(pf, gd_theme);
    for (unsigned i=0; i<caveset.caves.size(); i++) {
        if (progress) {
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), (float) i/caveset.caves.size());
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), CPrintf("%d/%d") % i % caveset.caves.size());
            gdk_window_process_all_updates();
        }

        /* rendering cave for png: seed=0 */
        CaveStored &cave=caveset.cave(i);
        CaveRendered rendered(cave, 0, 0);

        /* check cave to see if we have amoeba or magic wall. properties will be shown in html, if so. */
        bool has_amoeba=false, has_magic=false;
        for (int y=0; y<cave.h; y++)
            for (int x=0; x<cave.w; x++) {
                if (rendered.map(x, y)==O_AMOEBA)
                    has_amoeba=true;
                if (rendered.map(x, y)==O_MAGIC_WALL)
                    has_magic=true;
                break;
            }

        /* cave header */
        contents+=SPrintf("<A NAME=\"cave%03d\"></A>\n<H2>%ms</H2>\n") % (i+1) % cave.name;

        /* save image */
        char *pngname=g_strdup_printf("%s_%03d.png", pngoutbasename, i+1);
        GdkPixbuf *pixbuf=gd_drawcave_to_pixbuf(&rendered, cr, 0, 0, true, false);
        GError *error=NULL;
        gdk_pixbuf_save(pixbuf, pngname, "png", &error, "compression", "9", NULL);
        if (error) {
            gd_warning(error->message);
            g_error_free(error);
            error=NULL;
        }
        g_free(pngname);
        contents+=SPrintf("<IMAGE SRC=\"%s_%03d.png\" WIDTH=\"%d\" HEIGHT=\"%d\">\n") % pngbasename % (i+1) % gdk_pixbuf_get_width(pixbuf) % gdk_pixbuf_get_height(pixbuf);
        g_object_unref(pixbuf);

        contents+="<BR>\n";
        contents+="<TABLE>\n";
        if (cave.author!="")
            contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Author") % cave.author;
        if (cave.description!="")
            contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Description") % cave.description;
        if (cave.remark!="")
            contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Remark") % cave.remark;
        if (cave.story!="")
            contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Story") % cave.story;
        contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Type") % (cave.intermission ? _("Intermission") : _("Normal cave"));
        contents+=SPrintf("<TR><TH>%ms<TD>%ms\n") % _("Selectable as start") % (cave.selectable ? _("Yes") : _("No"));
        contents+=SPrintf("<TR><TH>%ms<TD>%d\n") % _("Diamonds needed") % cave.level_diamonds[0];
        contents+=SPrintf("<TR><TH>%ms<TD>%d\n") % _("Diamond value") % cave.diamond_value;
        contents+=SPrintf("<TR><TH>%ms<TD>%d\n") % _("Extra diamond value") % cave.extra_diamond_value;
        contents+=SPrintf("<TR><TH>%ms<TD>%d\n") % _("Time (s)") % cave.level_time[0];
        if (has_amoeba)
            contents+=SPrintf("<TR><TH>%ms<TD>%d, %d\n") % _("Amoeba threshold and time (s)") % cave.level_amoeba_threshold[0] % cave.level_amoeba_time[0];
        if (has_magic)
            contents+=SPrintf("<TR><TH>%ms<TD>%d\n") % _("Magic wall milling time (s)") % cave.level_magic_wall_time[0];
        contents+="</TABLE>\n";

        contents+="\n";

    }
    contents+="</BODY>\n";
    contents+="</HTML>\n";
    g_free(pngoutbasename);
    g_free(pngbasename);

    if (!g_file_set_contents(htmlname, contents.c_str(), contents.size(), &error)) {
        /* could not save properly */
        gd_critical(error->message);
        g_error_free(error);
    }

    if (dialog)
        gtk_widget_destroy(dialog);
}
