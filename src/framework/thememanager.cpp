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

#include "misc/logger.hpp"
#include "gfx/pixbuffactory.hpp"
#include "gfx/cellrenderer.hpp"
#include "settings.hpp"

static void add_file_to_themes(PixbufFactory &pf, std::vector<std::string> &themes, const char *filename) {
    g_assert(filename!=NULL);

    /* if file name already in themes list, remove. */
    for (unsigned i=0; i<themes.size(); i++)
        if (themes[i]!="" && themes[i]==filename) {
            themes.erase(themes.begin()+i);
            i--;
        }

    if (CellRenderer::is_image_ok_for_theme(pf, filename))
        themes.push_back(filename);
}

static void add_dir_to_themes(PixbufFactory &pf, std::vector<std::string> &themes, const char *directory_name) {
    Logger l(true);       /* do not report error messages */

    GDir *dir=g_dir_open(directory_name, 0, NULL);
    if (!dir)
        /* silently ignore unable-to-open directories */
        return;
    char const *name;
    while ((name=g_dir_read_name(dir)) != NULL) {
        char *filename=g_build_filename(directory_name, name, NULL);
        char *lower=g_ascii_strdown(filename, -1);

        /* we only allow bmp and png files. converted to lowercase, to be able to check for .bmp */
        if ((g_str_has_suffix(lower, ".bmp") || g_str_has_suffix(lower, ".png")) && g_file_test(filename, G_FILE_TEST_IS_REGULAR))
            /* try to add the file. */
            add_file_to_themes(pf, themes, filename);

        g_free(lower);
        g_free(filename);
    }
    g_dir_close(dir);
}

/* will create a list of file names which can be used as themes. */
/* the first item will be an empty string to represent the default, built-in theme. */
static std::vector<std::string> settings_create_themes_list(PixbufFactory &pf) {
    std::vector<std::string> themes;

    themes.push_back("");    /* this symbolizes the default theme */
    if (gd_theme!="")
        add_file_to_themes(pf, themes, gd_theme.c_str());

    for (unsigned i=0; i<gd_themes_dirs.size(); ++i)
        add_dir_to_themes(pf, themes, gd_themes_dirs[i].c_str());

    return themes;
}


void load_themes_list(PixbufFactory &pf, std::vector<std::string> &themes, int &themenum) {
    themes = settings_create_themes_list(pf);
    themenum = -1;
    if (gd_theme != "") {
        for (unsigned n = 1; n < themes.size(); n++)
            if (themes[n] == gd_theme)
                themenum = n;
    } else
        themenum = 0;
    if (themenum == -1) {
        gd_warning(CPrintf("theme %s not found in array") % gd_theme);
        themenum = 0;
    }
}


std::string filename_for_new_theme(const char *theme_to_install) {
    // create a new filename, with all lowercase letters
    char *basename=g_path_get_basename(theme_to_install);
    char *utf8=g_filename_to_utf8(basename, -1, NULL, NULL, NULL);
    char *lowercase=g_utf8_strdown(utf8, -1);
    char *lowercase_sys=g_filename_from_utf8(lowercase, -1, NULL, NULL, NULL);

    // first dir in themes_dirs is the themes dir of the user
    char *new_filename = g_build_path(G_DIR_SEPARATOR_S, gd_themes_dirs[0].c_str(), lowercase_sys, NULL);
    g_free(lowercase_sys);
    g_free(lowercase);
    g_free(utf8);
    g_free(basename);

    std::string ret = new_filename;
    g_free(new_filename);
    return ret;
}


bool install_theme(const char *from, const char *to) {
    // first dir in themes_dirs is the themes dir of the user
    g_mkdir_with_parents(gd_themes_dirs[0].c_str(), 0700);
    GError *error=NULL;
    gchar *contents = NULL;
    gsize length;
    gboolean result = g_file_get_contents(from, &contents, &length, &error) && g_file_set_contents(to, contents, length, &error);
    if (error) {
        gd_critical(error->message);
        g_error_free(error);
    }
    g_free(contents);
    return result;
}
