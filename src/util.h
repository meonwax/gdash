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
#ifndef _GD_UTIL_H
#define _GD_UTIL_H

#include <glib.h>

/*******************************************
 *
 * ERROR HANDLING
 *
 */
typedef struct _gd_error_message {
    GLogLevelFlags flags;
    char *message;
} GdErrorMessage;

extern GList *gd_errors;

/* error handling */
void gd_error_set_context(const char *format, ...);
void gd_clear_error_flag();
void gd_install_log_handler();
gboolean gd_has_new_error();
void gd_clear_errors();









/* returns a static string which contains the utf8 representation of the filename in system encoding*/
const char *gd_filename_to_utf8(const char *filename);

/* tries to find a file in the gdash installation and returns a path (owned by this function, not to be g_free()d) */
const char *gd_find_file(const char *filename);

/* wrap a text to specified width */
char *gd_wrap_text(const char *orig, int width);
/* count lines in text (number of \n's + 1) */
int gd_lines_in_text(const char *text);

/* return current date in 2008-12-04 format */
const char *gd_get_current_date();
/* return current date in 2008-12-04 12:34 format */
const char *gd_get_current_date_time();
/* clamp integer into range */
int gd_clamp(int val, int min, int max);
#endif

