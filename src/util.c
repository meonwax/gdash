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
#include <glib.h>
#include <glib/gi18n.h>
#include "util.h"
#include "settings.h"

GList *gd_errors=NULL;
gboolean gd_new_error=FALSE;
static char *error_context=NULL;


static void
error_free(GdErrorMessage *error)
{
	g_free(error->message);
	g_free(error);
}

void
gd_clear_error_flag()
{
	gd_new_error=FALSE;
}

void
gd_clear_errors()
{
	g_list_foreach(gd_errors, (GFunc) error_free, NULL);
	g_list_free(gd_errors);
	gd_errors=NULL;
	gd_clear_error_flag();
}

void
gd_error_set_context(const char *format, ...)
{
	va_list ap;
	
	g_free(error_context);
	error_context=NULL;
	if (!format)
		return;
	va_start(ap, format);
	error_context=g_strdup_vprintf(format, ap);
	va_end(ap);
}

static void
log_func(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	GdErrorMessage *error;
	error=g_new(GdErrorMessage, 1);
	error->flags=log_level & G_LOG_LEVEL_MASK;
	if (error_context)
		error->message=g_strdup_printf("%s: %s", error_context, message);
	else
		error->message=g_strdup(message);

	if ((log_level&G_LOG_LEVEL_MASK) <= G_LOG_LEVEL_WARNING)
		gd_new_error=TRUE;
	
	gd_errors=g_list_append(gd_errors, error);
	/* also call default handler to print to console; but with processed string */
	g_log_default_handler(log_domain, log_level, error->message, user_data);
}

gboolean gd_has_new_error()
{
	return gd_new_error;
}

void
gd_install_log_handler()
{
	g_log_set_default_handler (log_func, NULL);
}






/* returns a static string which contains the utf8 representation of the filename in system encoding */
const char *
gd_filename_to_utf8(const char *filename)
{
	static char *utf8=NULL;
	GError *error=NULL;
	
	g_free(utf8);
	utf8=g_filename_to_utf8(filename, -1,  NULL, NULL, &error);
	if (error) {
		g_error_free(error);
		utf8=g_strdup(filename);	/* use without conversion */
	}
	return utf8;
}




static const char *
find_file_try_path(const char *path, const char *filename)
{
	static char *result=NULL;
	
	g_free(result);
	
	result=g_build_path(G_DIR_SEPARATOR_S, path, filename, NULL);
	/* if the file exists, return the full filename */
	if (g_file_test(result, G_FILE_TEST_EXISTS))
		return result;
		
	/* otherwise return nothing */
	return NULL;
	
}


/* tries to find a file in the gdash installation and returns a path (owned by this function, not to be g_free()d) */
const char *
gd_find_file(const char *filename)
{
	const char *result;
	
	result=find_file_try_path(gd_user_config_dir, filename);
	if (result!=NULL)
		return result;
		
	result=find_file_try_path(gd_system_data_dir, filename);
	if (result!=NULL)
		return result;
		
	result=find_file_try_path(".", filename);
	if (result!=NULL)
		return result;
	
	/* for testing */
	result=find_file_try_path("./sound", filename);
	if (result!=NULL);
		return result;
		
	g_critical("cannot find file: %s", gd_filename_to_utf8(filename));
}

