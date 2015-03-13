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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <fstream>

#ifdef HAVE_SDL
/* on windows, sdl does some hack on the main function.
 * therefore we need to include this file, even if it seems
 * to do nothing. */
#include <SDL/SDL.h>
#endif

#include "cave/caveset.hpp"
#include "sound/sound.hpp"
#include "misc/util.hpp"
#include "misc/logger.hpp"
#include "misc/about.hpp"
#include "settings.hpp"
#include "framework/commands.hpp"
#include "fileops/loadfile.hpp"
#include "fileops/binaryimport.hpp"
#include "input/joystick.hpp"

#ifdef HAVE_SDL
#include "sdl/sdlmainwindow.hpp"
#endif
#ifdef HAVE_GTK
#include "gtk/gtkmainwindow.hpp"
#include "editor/editor.hpp"
#include "editor/editorcellrenderer.hpp"
#include "editor/exporthtml.hpp"
#include "gtk/gtkpixbuffactory.hpp"
#include "gtk/gtkui.hpp"
#endif

/* includes cavesets built in to the executable */
#include "levels.cpp"



int main(int argc, char *argv[]) {
    CaveSet caveset;
    int quit=0;
    gboolean editor=FALSE;
    char *gallery_filename=NULL;
    char *png_filename=NULL, *png_size=NULL;
    char *save_cave_name=NULL, *save_gds_name=NULL;
    gboolean force_quit_no_gtk;

    GError *error=NULL;
    GOptionEntry entries[] = {
        {"editor", 'e', 0, G_OPTION_ARG_NONE, &editor, N_("Start editor")},
        {"save-gallery", 'g', 0, G_OPTION_ARG_FILENAME, &gallery_filename, N_("Save caveset in a HTML gallery")},
        {"stylesheet", 0, 0, G_OPTION_ARG_STRING  /* not filename! */, &gd_html_stylesheet_filename, N_("Link stylesheet from file to a HTML gallery, eg. \"../style.css\"")},
        {"favicon", 0, 0, G_OPTION_ARG_STRING /* not filename! */, &gd_html_favicon_filename, N_("Link shortcut icon to a HTML gallery, eg. \"../favicon.ico\"")},
        {"save-png", 'p', 0, G_OPTION_ARG_FILENAME, &png_filename, N_("Save image of first cave to PNG")},
        {"png-size", 0, 0, G_OPTION_ARG_STRING, &png_size, N_("Set PNG image size. Default is 128x96, set to 0x0 for unscaled")},
        {"save-bdcff", 's', 0, G_OPTION_ARG_FILENAME, &save_cave_name, N_("Save caveset in a BDCFF file")},
        {"save-gds", 'd', 0, G_OPTION_ARG_FILENAME, &save_gds_name, N_("Save imported binary data to a GDS file. An input file name is required.")},
        {"quit", 'q', 0, G_OPTION_ARG_NONE, &quit, N_("Batch mode: quit after specified tasks")},
        {NULL}
    };

    GOptionContext *context=gd_option_context_new();
    g_option_context_add_main_entries(context, entries, PACKAGE);   /* gdash (gtk version) parameters */
    g_option_context_add_group(context, gtk_get_option_group(FALSE));   /* add gtk parameters */
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);
    if (error) {
        gd_warning(error->message);
        g_error_free(error);
    }

    /* show license? */
    if (gd_param_license) {
        /* print license and quit. */
        g_print("%s %s\n\n%s\n\n", PACKAGE_NAME, PACKAGE_VERSION, About::copyright);
        std::vector<std::string> wrapped_license = gd_wrap_text(About::license, 72);
        for (unsigned i = 0; i < wrapped_license.size(); ++i)
            g_print("%s\n", wrapped_license[i].c_str());
        return 0;
    }

    Logger global_logger;

    gd_settings_init();
    gd_settings_init_dirs();
    gd_load_settings();
    gd_settings_set_locale();
    gd_settings_init_translation();

    force_quit_no_gtk=FALSE;
    if (!gtk_init_check(&argc, &argv))
        force_quit_no_gtk=TRUE;


    gd_cave_types_init();
    gd_cave_objects_init();
    Joystick::init();

    /* if memory snapshot -> gds file conversion requested */
    if (save_gds_name != NULL) {
        Logger thislogger;
        if (gd_param_cavenames==NULL || g_str_equal(gd_param_cavenames, "")) {
            g_print("An input filename must be given for GDS conversion.\n");
            return 1;
        }
        std::vector<unsigned char> file = load_file_to_vector(gd_param_cavenames[0]);
        /* -1 because the file loader adds a terminating zero */
        std::vector<unsigned char> memory = load_memory_dump(&file[0], file.size()-1);
        std::vector<unsigned char> gds = gdash_binary_import(memory);
        std::fstream os(save_gds_name, std::ios::out | std::ios::binary);
        os.write((char *) &gds[0], gds.size());

        thislogger.clear();
    }

    /* LOAD A CAVESET FROM A FILE, OR AN INTERNAL ONE */
    /* if remaining arguments, they are filenames */
    try {
        if (gd_param_cavenames && gd_param_cavenames[0]) {
            caveset = create_from_file(gd_param_cavenames[0]);
        } else {
            /* if nothing requested, load default */
            caveset = create_from_buffer(level_pointers[0], -1);
            caveset.name = level_names[0];
            caveset.load_highscore(gd_user_config_dir);
        }
    } catch (std::exception &e) {
        /// @todo show error to the screen
        gd_critical(e.what());
    }

    /* see if generating a gallery. */
    /* but only if there are any caves at all. */
    if (caveset.has_caves() && gallery_filename)
        gd_save_html(gallery_filename, NULL, caveset);

    /* save cave png */
    if (png_filename) {
        unsigned int size_x=128, size_y=96; /* default size */

        if (png_size && (sscanf(png_size, "%ux%u", &size_x, &size_y) != 2))
            gd_warning(CPrintf(_("Invalid image size: %s")) % png_size);
        if (size_x<1 || size_y<1) {
            size_x=0;
            size_y=0;
        }

        /* rendering cave for png: seed=0 */
        CaveRendered renderedcave(caveset.cave(0), 0, 0);
        GTKPixbufFactory pf;
        EditorCellRenderer cr(pf, gd_theme);

        GdkPixbuf *pixbuf = gd_drawcave_to_pixbuf(&renderedcave, cr, size_x, size_y, true, false);
        GError *error=NULL;
        if (!gdk_pixbuf_save(pixbuf, png_filename, "png", &error, "compression", "9", NULL)) {
            gd_critical(CPrintf("Error saving PNG image %s: %s") % png_filename % error->message);
            g_error_free(error);
        }
        g_object_unref(pixbuf);
    }

    if (save_cave_name)
        caveset.save_to_file(save_cave_name);

    /* if batch mode, quit now */
    if (quit) {
        global_logger.clear();
        return 0;
    }
    if (force_quit_no_gtk) {
        gd_critical("Cannot initialize GUI");
        return 1;
    }

    gd_register_stock_icons();
    /* set gtk default icon */
    GdkPixbuf *logo = gd_icon();
    gtk_window_set_default_icon(logo);
    g_object_unref(logo);

    /* select first action */
    NextAction na;
    if (editor)
        na = StartEditor;
    else
        na = StartTitle;

restart_from_here:

    gd_sound_init();
    gd_sound_set_music_volume();
    gd_sound_set_chunk_volumes();

    while (na != Quit) {
        switch (na) {
            case StartTitle:
                gd_main_window_sdl_run(&caveset, na);
                //~ gd_main_window_gtk_run(&caveset, na);
                break;
            case StartEditor:
                na = StartTitle;
                gd_cave_editor_run(&caveset);
                break;
            case Restart:
                gd_sound_close();
                na = StartTitle;
                goto restart_from_here;
                break;
            case Quit:
                break;
        }
    }

    gd_sound_close();

    caveset.save_highscore(gd_user_config_dir);

    gd_save_settings();

    global_logger.clear();

    return 0;
}

