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
#include <SDL.h>
#include <SDL_image.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "cavedb.h"
#include "config.h"
#include "cave.h"
#include "caveset.h"
#include "sdlgfx.h"
#include "settings.h"
#include "util.h"
#include "about.h"
#include "sound.h"

#include "sdlui.h"


char *gd_last_folder=NULL;
char *gd_caveset_filename=NULL;


/* operation successful, so we should remember file name. */
void
gd_caveset_file_operation_successful(const char *filename)
{
	/* if it is a bd file, remember new filename */
	if (g_str_has_suffix(filename, ".bd")) {
		char *stored;

		/* first make copy, then free and set pointer. we might be called with filename=caveset_filename */
		stored=g_strdup(filename);
		g_free(gd_caveset_filename);
		gd_caveset_filename=stored;
	} else {
		g_free(gd_caveset_filename);
		gd_caveset_filename=NULL;
	}
}

void
gd_open_caveset(const char *directory)
{
	char *filename;
	char *filter;
	
	/* if the caveset is edited, ask the user if to save. */
	/* if does not want to discard, get out here */
	if (!gd_discard_changes())
		return;
	
	if (!gd_last_folder)
		gd_last_folder=g_strdup(g_get_home_dir());
		
	filter=g_strjoinv(";", gd_caveset_extensions);
	filename=gd_select_file("SELECT CAVESET TO LOAD", directory?directory:gd_last_folder, filter, FALSE);
	g_free(filter);

	/* if file selected */	
	if (filename) {
		/* remember last directory */
		g_free(gd_last_folder);
		gd_last_folder=g_path_get_dirname(filename);

		gd_save_highscore(gd_user_config_dir);
		
		gd_caveset_load_from_file(filename, gd_user_config_dir);

		/* if successful loading and this is a bd file, and we load highscores from our own config dir */
		if (!gd_has_new_error() && g_str_has_suffix(filename, ".bd") && !gd_use_bdcff_highscore)
			gd_load_highscore(gd_user_config_dir);
		
		g_free(filename);
	} 
}





static int
filenamesort(gconstpointer a, gconstpointer b)
{
	gchar **a_=(gpointer) a, **b_=(gpointer) b;
	return g_ascii_strcasecmp(*a_, *b_);
}



/* set title line (the first line in the screen) to text */
void
gd_title_line(const char *format, ...)
{
	va_list args;
	char *text;

	va_start(args, format);
	text=g_strdup_vprintf(format, args);
	gd_blittext_n(gd_screen, -1, 0, GD_GDASH_WHITE, text);
	g_free(text);
	va_end(args);
}

/* set status line (the last line in the screen) to text */
void
gd_status_line(const char *text)
{
	gd_blittext_n(gd_screen, -1, gd_screen->h-gd_font_height(), GD_GDASH_GRAY2, text);
}




/* runs a file selection dialog */
/* returns new, full-path filename (to be freed later by the caller) */
/* glob: semicolon separated list of globs */
char *
gd_select_file(const char *title, const char *start_dir, const char *glob, gboolean for_save)
{
	enum {
		GD_NOT_YET,
		GD_YES,
		GD_JUMP,
		GD_ESCAPE,
		GD_QUIT,
		GD_NEW,
	};

	static char *current_dir=NULL;	/* static to not worry about freeing */
	const int yd=gd_line_height();
	const int names_per_page=gd_screen->h/yd-3;
	char *result;
	char *directory;
	int filestate;
	char **globs;
	gboolean redraw;

	if (glob==NULL || g_str_equal(glob, ""))
		glob="*";
	globs=g_strsplit_set(glob, ";", -1);

	/* remember current directory, as we step into others */
	if (current_dir)
		g_free(current_dir);
	current_dir=g_get_current_dir();

	gd_backup_and_dark_screen();
	gd_title_line(title);
	if (for_save)
		/* for saving, we allow the user to select a new filename. */
		gd_status_line("CRSR:SELECT  N:NEW  J:JUMP  ESC:CANCEL");
	else
		gd_status_line("MOVE: SELECT   J: JUMP   ESC: CANCEL");

	/* this is somewhat hackish; finds out the absolute path of start_dir. also tests is we can enter that directory */
	if (g_chdir(start_dir)==-1) {
		g_warning("cannot change to directory: %s", start_dir);
		/* stay in current_dir */
	}
	directory=g_get_current_dir();
	g_chdir(current_dir);

	result=NULL;
	filestate=GD_NOT_YET;
	while (!gd_quit && filestate==GD_NOT_YET) {
		int sel, state;
		GDir *dir;
		GPtrArray *files;
		const gchar *name;

		/* read directory */
		files=g_ptr_array_new();
		if (g_chdir(directory)==-1) {
			g_warning("cannot change to directory: %s", directory);
			filestate=GD_ESCAPE;
			break;
		}
		dir=g_dir_open(".", 0, NULL);
		if (!dir) {
			g_warning("cannot open directory: %s", directory);
			filestate=GD_ESCAPE;
			break;
		}
		while ((name=g_dir_read_name(dir))!=NULL) {
#ifdef G_OS_WIN32
			/* on windows, skip hidden files? */
#else
			/* on unix, skip file names starting with a '.' - those are hidden files */
			if (name[0]=='.')
				continue;
#endif
			if (g_file_test(name, G_FILE_TEST_IS_DIR))
				g_ptr_array_add(files, g_strdup_printf("%s%s", name, G_DIR_SEPARATOR_S));
			else {
				int i;
				gboolean match=FALSE;

				for (i=0; globs[i]!=NULL && match==FALSE; i++)
					if (g_pattern_match_simple(globs[i], name))
						match=TRUE;

				if (match)
					g_ptr_array_add(files, g_strdup(name));
			}
		}
		g_dir_close(dir);
		g_chdir(current_dir);	/* step back to directory where we started */

		/* add "directory up" and sort */
#ifdef G_OS_WIN32
		/* if we are NOT in a root directory */
		if (!g_str_has_suffix(directory, ":\\"))	/* root directory is "X:\" */
			g_ptr_array_add(files, g_strdup_printf("..%s", G_DIR_SEPARATOR_S));
#else
		if (!g_str_equal(directory, "/"))
			g_ptr_array_add(files, g_strdup_printf("..%s", G_DIR_SEPARATOR_S));
#endif
		g_ptr_array_sort(files, filenamesort);

		/* show current directory */
		gd_clear_line(gd_screen, 1*yd);
		gd_blittext_n(gd_screen, -1, 1*yd, GD_GDASH_YELLOW, gd_filename_to_utf8(directory));

		/* do file selection menu */
		sel=0;
		state=GD_NOT_YET;
		redraw=TRUE;
		while (state==GD_NOT_YET) {
			int page, i, cur;
			SDL_Event event;

			page=sel/names_per_page;

			if (redraw) {
				for (i=0, cur=page*names_per_page; i<names_per_page; i++, cur++) {
					int col;

					col=cur==sel?GD_GDASH_YELLOW:GD_GDASH_LIGHTBLUE;

					gd_clear_line(gd_screen, (i+2)*yd);
					if (cur<files->len)	/* may not be as much filenames as it would fit on the screen */
						gd_blittext_n(gd_screen, 0, (i+2)*yd, col, gd_filename_to_utf8(g_ptr_array_index(files, cur)));
				}
				SDL_Flip(gd_screen);

				redraw=FALSE;
			}

			/* check for incoming events */
			SDL_WaitEvent(NULL);
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
					case SDL_QUIT:
						state=GD_QUIT;
						gd_quit=TRUE;
						break;
						
					case SDL_KEYDOWN:
						switch (event.key.keysym.sym) {
							/* movements */
							case SDLK_UP:
								sel=gd_clamp(sel-1, 0, files->len-1);
								redraw=TRUE;
								break;
							case SDLK_DOWN:
								sel=gd_clamp(sel+1, 0, files->len-1);
								redraw=TRUE;
								break;
							case SDLK_PAGEUP:
								sel=gd_clamp(sel-names_per_page, 0, files->len-1);
								redraw=TRUE;
								break;
							case SDLK_PAGEDOWN:
								sel=gd_clamp(sel+names_per_page, 0, files->len-1);
								redraw=TRUE;
								break;
							case SDLK_HOME:
								sel=0;
								redraw=TRUE;
								break;
							case SDLK_END:
								sel=files->len-1;
								redraw=TRUE;
								break;
							
							/* jump to directory (name will be typed) */
							case SDLK_j:
								state=GD_JUMP;
								break;
							/* enter new filename - only if saving allowed */
							case SDLK_n:
								if (for_save)
									state=GD_NEW;
								break;
							
							/* select current file/directory */
							case SDLK_SPACE:
							case SDLK_RETURN:
								state=GD_YES;
								break;
								
							case SDLK_ESCAPE:
								state=GD_ESCAPE;
								break;
							
							default:
								/* other keys do nothing */
								break;	
						}
						break;

					default:
						/* other events are not interesting now */
						break;
				}	/* switch event.type */
			}	/* while pollevent */
		}	/* while state=nothing happened */

		/* now check the state variable to see what happend. maybe act upon it. */
		
		/* user requested to enter a new filename */
		if (state==GD_NEW) {
			char *name;
			char *new_name;
			char *extension_added;

			/* make up a suggested filename */
			if (!g_str_equal(gd_caveset_data->name, ""))
				extension_added=g_strdup_printf("%s.bd", gd_caveset_data->name);
			else
				extension_added=NULL;
			/* if extension added is null, g_build_path will sense that as the end of the list. */
			name=g_build_path(G_DIR_SEPARATOR_S, directory, extension_added, NULL);
			g_free(extension_added);
			new_name=gd_input_string("ENTER NEW FILE NAME", name);
			g_free(name);
			/* if enters a file name, remember that, and exit the function via setting filestate variable */
			if (new_name) {
				result=new_name;
				filestate=GD_YES;
			} else
				g_free(new_name);
		}

		/* user requested to ask for another directory name to jump to */
		if (state==GD_JUMP) {
			char *newdir;

			newdir=gd_input_string("JUMP TO DIRECTORY", directory);
			if (newdir) {
				/* first change to dir, then to newdir: newdir entered by the user might not be absolute. */
				if (g_chdir(directory)==-1 || g_chdir(newdir)==-1) {
					g_warning("cannot change to directory: %s", newdir);
					filestate=GD_ESCAPE;
					g_free(newdir);
					break;
				}

				g_free(directory);
				directory=g_get_current_dir();
				g_chdir(current_dir);
			}
		}
		else
		/* if selected any from the list, it can be a file or a directory. */
		if (state==GD_YES) {
			if (g_str_has_suffix(g_ptr_array_index(files, sel), G_DIR_SEPARATOR_S)) {
				/* directory selected */
				char *newdir;

				if (g_chdir(directory)==-1) {
					g_warning("cannot change to directory: %s", directory);
					filestate=GD_ESCAPE;
					break;
				}
				if (g_chdir(g_ptr_array_index(files, sel))==-1) {
					g_warning("cannot change to directory: %s", (char *)g_ptr_array_index(files, sel));
					filestate=GD_ESCAPE;
					break;
				}
				newdir=g_get_current_dir();
				g_chdir(current_dir);	/* step back to directory where we started */
				g_free(directory);
				directory=newdir;
			} else {
				/* file selected */
				result=g_build_path(G_DIR_SEPARATOR_S, directory, g_ptr_array_index(files, sel), NULL);
				filestate=GD_YES;
			}
		} else
			/* pass state to break loop */
			filestate=state;

		g_ptr_array_foreach(files, (GFunc) g_free, NULL);
		g_ptr_array_free(files, TRUE);
	}

	/* if selecting a file to write to, check if overwrite */
	if (filestate==GD_YES && result && for_save && g_file_test(result, G_FILE_TEST_EXISTS)) {
		gboolean said_yes, answer;

		answer=gd_ask_yes_no("File exists. Overwrite?", "No", "Yes", &said_yes);
		if (!answer || !said_yes) {		/* if did not answer or answered no, forget filename. we do not overwrite */
			g_free(result);
			result=NULL;
		}
	}

	g_strfreev(globs);
	gd_restore_screen();

	return result;
}





/*
 *
 * THEME HANDLING
 *
 */
static gboolean
is_image_ok_for_theme(const char *filename)
{
	SDL_Surface *surface;
	gboolean result=FALSE;

	surface=IMG_Load(filename);
	if (!surface)
		return FALSE;
	/* if the image is loaded */
	if (surface) {
		gd_error_set_context("%s", filename);
		if (gd_is_surface_ok_for_theme(surface))			/* if image passes all checks, result is "OK" */
			result=TRUE;
		gd_error_set_context(NULL);
		SDL_FreeSurface(surface);
	}

	return result;
}


static void
add_file_to_themes(GPtrArray *themes, const char *filename)
{
	int i;

	g_assert(filename!=NULL);

	/* if file name already in themes list, remove. */
	for (i=0; i<themes->len; i++)
		if (g_ptr_array_index(themes, i)!=NULL && g_str_equal(g_ptr_array_index(themes, i), filename))
			g_ptr_array_remove_index_fast(themes, i);

	if (is_image_ok_for_theme(filename))
		g_ptr_array_add(themes, g_strdup(filename));
}


static void
add_dir_to_themes(GPtrArray *themes, const char *directory_name)
{
	GDir *dir;
	const char *name;

	dir=g_dir_open(directory_name, 0, NULL);
	if (!dir)
		/* silently ignore unable-to-open directories */
		return;
	while((name=g_dir_read_name(dir))) {
		char *filename;
		char *lower;

		filename=g_build_filename(directory_name, name, NULL);
		lower=g_ascii_strdown(filename, -1);

		/* we only allow bmp and png files. converted to lowercase, to be able to check for .bmp */
		if ((g_str_has_suffix(lower, ".bmp") || g_str_has_suffix(lower, ".png")) && g_file_test(filename, G_FILE_TEST_IS_REGULAR))
			/* try to add the file. */
			add_file_to_themes(themes, filename);

		g_free(lower);
		g_free(filename);
	}
}

/* this is in glib 2.16, but that is too new for some users. */
/* also, this one treats NULL as the "lowest", so it will be at the start of the list */
static int
strcmp0(const char *str1, const char *str2)
{
	if (str1==NULL && str2==NULL)
		return 0;
	if (str1==NULL)
		return -1;
	if (str2==NULL)
		return 1;
	return strcmp(str1, str2);
}

/* will create a list of file names which can be used as themes. */
/* the first item will be a NULL to represent the default, built-in theme. */
GPtrArray *
gd_create_themes_list()
{
	GPtrArray *themes;
	int i;
	gboolean current_found;

	themes=g_ptr_array_new();
	g_ptr_array_add(themes, NULL);	/* this symbolizes the default theme */
	add_dir_to_themes(themes, gd_system_data_dir);
	add_dir_to_themes(themes, gd_user_config_dir);

	/* check if current theme is in the array */
	current_found=FALSE;
	for (i=0; i<themes->len; i++)
		if (strcmp0(gd_sdl_theme, g_ptr_array_index(themes, i))==0)
			current_found=TRUE;
	if (!current_found)
		add_file_to_themes(themes, gd_sdl_theme);

	return themes;
}


/*
 *   SDASH SETTINGS MENU
 */

void
gd_settings_menu()
{
	static GPtrArray *themes=NULL;
	gboolean finished;
	const char *yes="yes", *no="no";
	int themenum;
	typedef enum _settingtype {
		TypeBoolean,
		TypeScale,
		TypeTheme,
		TypePercent,
		TypeStringv,
		TypeKey,
	} SettingType;
	struct _setting {
		int page;
		SettingType type;
		char *name;
		void *var;
		const char **stringv;
	} settings[]= {
		{ 0, TypeBoolean, "Fullscreen", &gd_sdl_fullscreen },
		{ 0, TypeTheme,   "Theme", NULL },
		{ 0, TypeScale,   "Scale", &gd_sdl_scale },
		{ 0, TypeBoolean, "PAL emulation", &gd_sdl_pal_emulation },
		{ 0, TypePercent, "PAL scanline shade", &gd_pal_emu_scanline_shade },
		{ 0, TypeBoolean, "Even lines vertical scroll", &gd_even_line_pal_emu_vertical_scroll },
		{ 0, TypeBoolean, "Fine scrolling", &gd_fine_scroll },
		{ 0, TypeStringv, "C64 palette", &gd_c64_palette, gd_color_get_c64_palette_names() },
		{ 0, TypeStringv, "C64DTV palette", &gd_c64dtv_palette, gd_color_get_c64dtv_palette_names() },
		{ 0, TypeStringv, "Atari palette", &gd_atari_palette, gd_color_get_atari_palette_names() },
		{ 0, TypeStringv, "Preferred palette", &gd_preferred_palette, gd_color_get_palette_types_names() },

		{ 1, TypeBoolean, "Sound", &gd_sdl_sound },
		{ 1, TypePercent, "Music volume", &gd_sound_music_volume_percent },
		{ 1, TypePercent, "Cave volume", &gd_sound_chunks_volume_percent },
		{ 1, TypeBoolean, "Classic sounds only", &gd_classic_sound },
		{ 1, TypeBoolean, "16-bit mixing", &gd_sdl_16bit_mixing },
		{ 1, TypeBoolean, "44kHz mixing", &gd_sdl_44khz_mixing },
		{ 1, TypeBoolean, "Use BDCFF highscore", &gd_use_bdcff_highscore },
		{ 1, TypeBoolean, "Show caveset name at uncover", &gd_show_name_of_game },
		{ 1, TypeBoolean, "Show story", &gd_show_story },
		{ 1, TypeStringv, "Status bar colors", &gd_status_bar_type, gd_status_bar_type_get_names() },
		{ 1, TypeBoolean, "All caves selectable", &gd_all_caves_selectable },
		{ 1, TypeBoolean, "Import as all selectable", &gd_import_as_all_caves_selectable },
		{ 1, TypeBoolean, "No invisible outbox", &gd_no_invisible_outbox },
		{ 1, TypeBoolean, "Random colors", &gd_random_colors },

		{ 2, TypeKey,     "Key for left", &gd_sdl_key_left },
		{ 2, TypeKey,     "Key for right", &gd_sdl_key_right },
		{ 2, TypeKey,     "Key for up", &gd_sdl_key_up },
		{ 2, TypeKey,     "Key for down", &gd_sdl_key_down },
		{ 2, TypeKey,     "Key for fire", &gd_sdl_key_fire_1 },
		{ 2, TypeKey,     "Key for fire (alt.)", &gd_sdl_key_fire_2 },
		{ 2, TypeKey,     "Key for suicide", &gd_sdl_key_suicide },
	};
	const int numpages=settings[G_N_ELEMENTS(settings)-1].page+1;	/* take it from last element of settings[] */
	int n, page;
	int current;
	int y1[numpages], yd;

	/* optionally create the list of themes, and also find the current one in the list. */
	themes=gd_create_themes_list();
	themenum=-1;
	for (n=0; n<themes->len; n++)
		if (strcmp0(gd_sdl_theme, g_ptr_array_index(themes, n))==0)
			themenum=n;
	if (themenum==-1) {
		g_warning("theme %s not found in array", gd_sdl_theme);
		themenum=0;
	}

	/* check pages, and x1,y1 coordinates for each */
	yd=gd_font_height()+2;
	for (page=0; page<numpages; page++) {
		int num;

		num=0;
		for (n=0; n<G_N_ELEMENTS(settings); n++)
			if (settings[n].page==page)
				num++;
		y1[page]=(gd_screen->h-num*yd)/2;
	}

	gd_backup_and_dark_screen();
	gd_status_line("CRSR: MOVE   SPACE: CHANGE   ESC: EXIT");
	gd_blittext_n(gd_screen, -1, gd_screen->h-3*gd_line_height(), GD_GDASH_GRAY1, "Some changes require restart.");
	gd_blittext_n(gd_screen, -1, gd_screen->h-2*gd_line_height(), GD_GDASH_GRAY1, "Use T in the title for a new theme.");

	current=0;
	finished=FALSE;
	while (!finished && !gd_quit) {
		int linenum;
		SDL_Event event;

		page=settings[current].page;	/* take the current page number from the current setting line */
		gd_clear_line(gd_screen, 0);	/* clear for title line */
		gd_title_line("GDASH OPTIONS, PAGE %d/%d", page+1, numpages);

		/* show settings */
		linenum=0;
		for (n=0; n<G_N_ELEMENTS(settings); n++) {
			if (settings[n].page==page) {
				const GdColor c_name=GD_GDASH_LIGHTBLUE;
				const GdColor c_selected=GD_GDASH_YELLOW;
				const GdColor c_value=GD_GDASH_GREEN;

				int x, y;

				x=4*gd_font_width();
				y=y1[page]+linenum*yd;
				x=gd_blittext_n(gd_screen, x, y, current==n?c_selected:c_name, settings[n].name);
				x+=2*gd_font_width();
				switch(settings[n].type) {
					case TypeBoolean:
						x=gd_blittext_n(gd_screen, x, y, c_value, *(gboolean *)settings[n].var?yes:no);
						break;
					case TypeScale:
						x=gd_blittext_n(gd_screen, x, y, c_value, gd_scaling_name[*(GdScalingType *)settings[n].var]);
						break;
					case TypePercent:
						x=gd_blittext_printf_n(gd_screen, x, y, c_value, "%d%%", *(int *)settings[n].var);
						break;
					case TypeTheme:
						if (themenum==0)
							x=gd_blittext_n(gd_screen, x, y, c_value, "[Default]");
						else {
							char *thm;
							thm=g_filename_display_basename(g_ptr_array_index(themes, themenum));
							if (strrchr(thm, '.'))	/* remove extension */
								*strrchr(thm, '.')='\0';
							x=gd_blittext_n(gd_screen, x, y, c_value, thm);
							g_free(thm);
						}
						break;
					case TypeStringv:
						x=gd_blittext_n(gd_screen, x, y, c_value, settings[n].stringv[*(int *)settings[n].var]);
						break;
					case TypeKey:
						x=gd_blittext_n(gd_screen, x, y, c_value, gd_key_name(*(guint *)settings[n].var));
						break;
				}

				linenum++;
			}
		}
		SDL_Flip(gd_screen);

		/* we don't leave text on the screen after us. */
		/* so next iteration will have a nice empty screen to draw on :) */
		linenum=0;
		for (n=0; n<G_N_ELEMENTS(settings); n++) {
			if (settings[n].page==page) {
				gd_clear_line(gd_screen, y1[page]+linenum*yd);
				linenum++;
			}
		}

		SDL_WaitEvent(NULL);
		while (SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					gd_quit=TRUE;
					break;

				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						/* MOVEMENT */
						case SDLK_UP:	/* key up */
							current=gd_clamp(current-1, 0, G_N_ELEMENTS(settings)-1);
							break;
						case SDLK_DOWN:	/* key down */
							current=gd_clamp(current+1, 0, G_N_ELEMENTS(settings)-1);
							break;
						case SDLK_PAGEUP:
							if (page>0)
								while (settings[current].page==page)
									current--;	/* decrement until previous page is found */
							break;
						case SDLK_PAGEDOWN:
							if (page<numpages-1)
								while (settings[current].page==page)
									current++;	/* increment until previous page is found */
							break;

						/* CHANGE SETTINGS */
						case SDLK_LEFT:	/* key left */
							switch(settings[current].type) {
								case TypeBoolean:
									*(gboolean *)settings[current].var=FALSE;
									break;
								case TypeScale:
									*(int *)settings[current].var=MAX(0,(*(int *)settings[current].var)-1);
									break;
								case TypePercent:
									*(int *)settings[current].var=MAX(0,(*(int *)settings[current].var)-5);
									break;
								case TypeTheme:
									themenum=gd_clamp(themenum-1, 0, themes->len-1);
									break;
								case TypeStringv:
									*(int *)settings[current].var=gd_clamp(*(int *)settings[current].var-1, 0, g_strv_length((char **) settings[current].stringv)-1);
									break;
								case TypeKey:
									break;
							}
							break;

						case SDLK_RIGHT:	/* key right */
							switch(settings[current].type) {
								case TypeBoolean:
									*(gboolean *)settings[current].var=TRUE;
									break;
								case TypeScale:
									*(int *)settings[current].var=MIN(GD_SCALING_MAX-1,(*(int *)settings[current].var)+1);
									break;
								case TypePercent:
									*(int *)settings[current].var=MIN(100,(*(int *)settings[current].var)+5);
									break;
								case TypeTheme:
									themenum=gd_clamp(themenum+1, 0, themes->len-1);
									break;
								case TypeStringv:
									*(int *)settings[current].var=gd_clamp(*(int *)settings[current].var+1, 0, g_strv_length((char **) settings[current].stringv)-1);
									break;
								case TypeKey:
									break;
							}
							break;

						case SDLK_SPACE:	/* key left */
						case SDLK_RETURN:
							switch (settings[current].type) {
								case TypeBoolean:
									*(gboolean *)settings[current].var=!*(gboolean *)settings[current].var;
									break;
								case TypeScale:
									*(int *)settings[current].var=(*(int *)settings[current].var+1)%GD_SCALING_MAX;
									break;
								case TypePercent:
									*(int *)settings[current].var=CLAMP((*(int *)settings[current].var+5), 0, 100);
									break;
								case TypeTheme:
									themenum=(themenum+1)%themes->len;
									break;
								case TypeStringv:
									*(int *)settings[current].var=(*(int *)settings[current].var+1)%g_strv_length((char **) settings[current].stringv);
									break;
								case TypeKey:
									{
										int i;

										i=gd_select_key(settings[current].name);
										if (i>0)
											*(guint *)settings[current].var=i;
									}
									break;
							}
							break;

						case SDLK_ESCAPE:	/* finished options menu */
							finished=TRUE;
							break;

						default:
							/* other keys do nothing */
							break;
					}	/* switch keypress key */
			}	/* switch event type */
		}	/* while pollevent */
	}

	/* set the theme. other variables are already set by the above code. */
	g_free(gd_sdl_theme);
	gd_sdl_theme=NULL;
	if (themenum!=0)
		gd_sdl_theme=g_strdup(g_ptr_array_index(themes, themenum));
	gd_load_theme();	/* this loads the theme given in the global variable gd_sdl_theme. */

	/* forget list of themes */
	g_ptr_array_foreach(themes, (GFunc) g_free, NULL);
	g_ptr_array_free(themes, TRUE);

	gd_restore_screen();

	gd_sound_set_music_volume(gd_sound_music_volume_percent);
	gd_sound_set_chunk_volumes(gd_sound_chunks_volume_percent);
}



void
gd_install_theme()
{
	char *filename;

	filename=gd_select_file("SELECT IMAGE IMAGE FOR THEME", g_get_home_dir(), "*.bmp;*.png", FALSE);
	if (filename) {
		gd_clear_errors();
		if (is_image_ok_for_theme(filename)) {
			/* if file is said to be ok as a theme */
			char *basename, *new_filename;
			GError *error=NULL;
			gchar *contents;
			gsize length;

			/* make up new filename */
			basename=g_path_get_basename(filename);
			new_filename=g_build_path(G_DIR_SEPARATOR_S, gd_user_config_dir, basename, NULL);
			g_free(basename);

			/* copy theme to user config directory */
			if (g_file_get_contents(filename, &contents, &length, &error) && g_file_set_contents(new_filename, contents, length, &error)) {
				/* copied file. */
			}
			else {
				/* unable to copy file. */
				g_warning("%s", error->message);
			}
		} else {
			/* if file is not ok as a theme */
			g_warning("%s cannot be used as a theme", filename);
		}
	}
	g_free(filename);
}

void
gd_show_highscore(GdCave *highlight_cave, int highlight_line)
{
	GdCave *cave;
	const int screen_height=gd_screen->h/gd_line_height();
	int max=MIN(G_N_ELEMENTS(cave->highscore), screen_height-3);
	gboolean finished;
	GList *current;	/* current cave to view */
	
	if (highlight_cave)
		current=g_list_find(gd_caveset, highlight_cave);

	gd_backup_and_dark_screen();
	gd_title_line("THE HALL OF FAME");
	gd_status_line("CRSR: CAVE   SPACE: EXIT");

	current=NULL;
	finished=FALSE;
	while (!finished && !gd_quit) {
		int i;
		GdHighScore *scores;
		SDL_Event event;

		/* current cave or game */
		if (current)
			cave=current->data;
		else
			cave=NULL;
		if (!cave)
			scores=gd_caveset_data->highscore;
		else
			scores=cave->highscore;

		gd_clear_line(gd_screen, gd_font_height());
		gd_blittext_n(gd_screen, -1, gd_font_height(), GD_GDASH_YELLOW, cave?cave->name:gd_caveset_data->name);


		/* show scores */
		for (i=0; i<max; i++) {
			int c;

			gd_clear_line(gd_screen, (i+2)*gd_line_height());
			c=i/5%2?GD_GDASH_PURPLE:GD_GDASH_GREEN;
			if (cave==highlight_cave && i==highlight_line)
				c=GD_GDASH_WHITE;
			if (scores[i].score!=0)
				gd_blittext_printf_n(gd_screen, 0, (i+2)*gd_line_height(), c, "%2d %6d %s", i+1, scores[i].score, scores[i].name);
		}
		SDL_Flip(gd_screen);

		/* process events */
		SDL_WaitEvent(NULL);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					gd_quit=TRUE;
					break;
					
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						/* movements */
						case SDLK_LEFT:
						case SDLK_UP:
						case SDLK_PAGEUP:
							if (current!=NULL)
								current=current->prev;
							break;
						case SDLK_RIGHT:
						case SDLK_DOWN:
						case SDLK_PAGEDOWN:
							if (current!=NULL) {
								/* if showing a cave, go to next cave (if any) */
								if (current->next!=NULL)
									current=current->next;
							}
							else {
								/* if showing game, show first cave. */
								current=gd_caveset;
							}
							break;
						case SDLK_SPACE:
						case SDLK_ESCAPE:
						case SDLK_RETURN:
							finished=TRUE;
							break;
						default:
							/* other keys do nothing */
							break;
					}
				default:
					/* other events do nothing */
					break;
			}
		}
	}

	gd_restore_screen();
}



void
gd_show_cave_info(GdCave *show_cave)
{
	GdCave *cave;
	const int screen_height=gd_screen->h/gd_font_height();
	gboolean finished;
	GList *current=NULL;	/* current cave to view */

	if (show_cave)
		current=g_list_find(gd_caveset, show_cave);
	gd_backup_and_dark_screen();
	gd_title_line("CAVESET INFORMATION");
	gd_status_line("LEFT, RIGHT: CAVE   SPACE: EXIT");

	finished=FALSE;
	while (!finished && !gd_quit) {
		GString *text;
		int i;
		int y;
		char *wrapped;
		SDL_Event event;

		/* current cave or game */
		if (current)
			cave=current->data;
		else
			cave=NULL;

		for (i=1; i<screen_height-1; i++)
			gd_clear_line(gd_screen, i*gd_font_height());

		gd_blittext_n(gd_screen, -1, gd_font_height(), GD_GDASH_YELLOW, cave?cave->name:gd_caveset_data->name);
		y=gd_font_height()+2*gd_line_height();

		/* show data */
		text=g_string_new(NULL);
		if (!cave) {
			/* ... FOR CAVESET */
			/* ... FOR CAVE */
			if (!g_str_equal(gd_caveset_data->author, "")) {
				g_string_append_printf(text, "%s", gd_caveset_data->author);
				if (!g_str_equal(gd_caveset_data->date, ""))
					g_string_append_printf(text, ", %s", gd_caveset_data->date);
				g_string_append_c(text, '\n');
			}
			
			if (!g_str_equal(gd_caveset_data->description, ""))
				g_string_append_printf(text, "%s\n", gd_caveset_data->description);
				
			if (gd_caveset_data->story->len>0) {
				g_string_append(text, gd_caveset_data->story->str);
				/* if the cave story has no enter on the end, add one. */
				if (text->str[text->len-1]!='\n')
					g_string_append_c(text, '\n');
			}
			if (gd_caveset_data->remark->len>0) {
				g_string_append(text, gd_caveset_data->remark->str);
				/* if the cave story has no enter on the end, add one. */
				if (text->str[text->len-1]!='\n')
					g_string_append_c(text, '\n');
			}
		} else {
			/* ... FOR CAVE */
			if (!g_str_equal(cave->author, "")) {
				g_string_append_printf(text, "%s", cave->author);
				if (!g_str_equal(cave->date, ""))
					g_string_append_printf(text, ", %s", cave->date);
				g_string_append_c(text, '\n');
			}
			
			if (!g_str_equal(cave->description, ""))
				g_string_append_printf(text, "%s\n", cave->description);
				
			if (cave->story->len>0) {
				g_string_append(text, cave->story->str);
				/* if the cave story has no enter on the end, add one. */
				if (text->str[text->len-1]!='\n')
					g_string_append_c(text, '\n');
			}
			if (cave->remark->len>0) {
				g_string_append(text, cave->remark->str);
				/* if the cave story has no enter on the end, add one. */
				if (text->str[text->len-1]!='\n')
					g_string_append_c(text, '\n');
			}
		}
		wrapped=gd_wrap_text(text->str, gd_screen->w/gd_font_width()-2);
		gd_blittext_n(gd_screen, gd_font_width(), gd_line_height()*2, GD_GDASH_LIGHTBLUE, wrapped);
		g_free(wrapped);
		g_string_free(text, TRUE);
		SDL_Flip(gd_screen);

		/* process events */
		SDL_WaitEvent(NULL);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					gd_quit=TRUE;
					break;
					
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						/* movements */
						case SDLK_LEFT:
						case SDLK_UP:
						case SDLK_PAGEUP:
							if (current!=NULL)
								current=current->prev;
							break;
						case SDLK_RIGHT:
						case SDLK_DOWN:
						case SDLK_PAGEDOWN:
							if (current!=NULL) {
								/* if showing a cave, go to next cave (if any) */
								if (current->next!=NULL)
									current=current->next;
							}
							else {
								/* if showing game, show first cave. */
								current=gd_caveset;
							}
							break;
						case SDLK_SPACE:
						case SDLK_ESCAPE:
						case SDLK_RETURN:
							finished=TRUE;
							break;
						default:
							/* other keys do nothing */
							break;
					}
				default:
					/* other events do nothing */
					break;
			}
		}
	}

	gd_restore_screen();
}



static void
wait_for_keypress()
{
	gboolean stop;

	stop=FALSE;
	while (!gd_quit && !stop) {
		SDL_Event event;

		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					gd_quit=TRUE;
					break;

				case SDL_KEYDOWN:
					stop=TRUE;
					break;
			}
		}

		if (gd_fire())
			stop=TRUE;
		SDL_Delay(100);
	}

	gd_wait_for_key_releases();
}





void
gd_help(const char **strings)
{
	int y, n;
	int numstrings;
	int charwidth, x1;

	/* remember screen contents */
	gd_backup_and_dark_screen();

	gd_title_line("GDASH HELP");
	gd_status_line("SPACE: EXIT");

	numstrings=g_strv_length((gchar **) strings);

	charwidth=0;
	for (n=0; n<numstrings; n+=2)
		charwidth=MAX(charwidth, strlen(strings[n])+1+strlen(strings[n+1]));
	x1=gd_screen->w/2-charwidth*gd_font_width()/2;

	y=(gd_screen->h-numstrings*(gd_line_height())/2)/2;
	for (n=0; n<numstrings; n+=2) {
		int x;

		x=gd_blittext_printf_n(gd_screen, x1, y, GD_GDASH_YELLOW, "%s ", strings[n]);
		x=gd_blittext_printf_n(gd_screen, x, y, GD_GDASH_LIGHTBLUE, "%s", strings[n+1]);

		y+=gd_line_height();
	}

	SDL_Flip(gd_screen);

	wait_for_keypress();

	/* copy screen contents back */
	gd_restore_screen();
}

static void
draw_window(SDL_Rect *rect)
{
	rect->x-=2;
	rect->y-=2;
	rect->w+=4;
	rect->h+=4;
	SDL_FillRect(gd_screen, rect, SDL_MapRGB(gd_screen->format, 64, 64, 64));
	rect->x++;
	rect->y++;
	rect->w-=2;
	rect->h-=2;
	SDL_FillRect(gd_screen, rect, SDL_MapRGB(gd_screen->format, 128, 128, 128));
	rect->x++;
	rect->y++;
	rect->w-=2;
	rect->h-=2;
	SDL_FillRect(gd_screen, rect, SDL_MapRGB(gd_screen->format, 0, 0, 0));
}


void
gd_message(const char *message)
{
	SDL_Rect rect;
	int height, lines;
	int y1;
	char *wrapped;
	int x;
	
	wrapped=gd_wrap_text(message, 38);
	/* wrapped always has a \n on the end, so this returns at least 2 */
	lines=gd_lines_in_text(wrapped);

	height=(1+lines)*gd_font_height();
	g_print("%d", lines);
	gd_backup_and_dark_screen();
	gd_status_line("SPACE: CONTINUE");
	
	y1=(gd_screen->h-height)/2;
	rect.x=gd_font_width();
	rect.w=gd_screen->w-2*gd_font_width();
	rect.y=y1;
	rect.h=height;
	draw_window(&rect);
	SDL_SetClipRect(gd_screen, &rect);
	
	/* lines is at least 2. so 3 and above means multiple lines */
	if (lines>2)
		x=rect.x;
	else
		x=-1;

	gd_blittext_n(gd_screen, x, y1+gd_font_height(), GD_GDASH_WHITE, wrapped);
	SDL_Flip(gd_screen);
	wait_for_keypress();

	SDL_SetClipRect(gd_screen, NULL);
	gd_restore_screen();
	g_free(wrapped);
}

char *
gd_input_string(const char *title, const char *current)
{
	int y1;
	SDL_Rect rect;
	gboolean enter, escape;
	GString *text;
	int n;
	int height, width;

	gd_backup_and_black_screen();

	height=6*gd_line_height();
	y1=(gd_screen->h-height)/2;	/* middle of the screen */
	rect.x=8;
	rect.y=y1;
	rect.w=gd_screen->w-2*8;
	rect.h=height;
	draw_window(&rect);
	SDL_SetClipRect(gd_screen, &rect);

	width=rect.w/gd_font_width();

	gd_blittext_n(gd_screen, -1, y1+gd_line_height(), GD_GDASH_WHITE, title);

	text=g_string_new(current);

	/* setup keyboard */
	SDL_EnableUNICODE(1);

	enter=escape=FALSE;
	n=0;
	while (!gd_quit && !enter && !escape) {
		SDL_Event event;
		int len, x;

		n=(n+1)%10;	/* for blinking cursor */
		gd_clear_line(gd_screen, y1+3*gd_line_height());
		len=g_utf8_strlen(text->str, -1);
		if (len<width-1)
			x=-1; /* if fits on screen (+1 for cursor), centered */
		else
			x=rect.x+rect.w-(len+1)*gd_font_width();	/* otherwise show end, +1 for cursor */
		
		gd_blittext_printf_n(gd_screen, x, y1+3*gd_line_height(), GD_GDASH_WHITE, "%s%c", text->str, n>=5?'_':' ');
		SDL_Flip(gd_screen);

		while(SDL_PollEvent(&event))
			switch(event.type) {
				case SDL_QUIT:
					gd_quit=TRUE;
					break;

				case SDL_KEYDOWN:
					if (event.key.keysym.sym==SDLK_RETURN)
						enter=TRUE;
					else
					if (event.key.keysym.sym==SDLK_ESCAPE)
						escape=TRUE;
					else
					if (event.key.keysym.sym==SDLK_BACKSPACE || event.key.keysym.sym==SDLK_DELETE) {
						/* delete one character from the end */
						if (text->len!=0) {
							char *ptr=text->str+text->len;	/* string pointer + length: points to the terminating zero */

							ptr=g_utf8_prev_char(ptr);	/* step back one utf8 character */
							g_string_truncate(text, ptr-text->str);
						}
					}
					else
					if (event.key.keysym.unicode!=0)
						g_string_append_unichar(text, event.key.keysym.unicode);
					break;
			}

		SDL_Delay(100);
	}

	/* forget special keyboard settings we needed here */
	SDL_EnableUNICODE(0);
	/* restore screen */
	SDL_SetClipRect(gd_screen, NULL);
	gd_restore_screen();

	gd_wait_for_key_releases();

	/* if quit, return nothing. */
	if (gd_quit)
		return NULL;

	if (enter)
		return g_string_free(text, FALSE);
	/* here must be escape=TRUE, we return NULL as no string is really entered */
	g_string_free(text, TRUE);
	return NULL;
}

/* select a keysim for some action. returns -1 if error, or the keysym. */
int
gd_select_key(const char *title)
{
	const int height=5*gd_line_height();
	int y1;
	SDL_Rect rect;
	gboolean got_key;
	guint key=0;	/* default value to avoid compiler warning */

	y1=(gd_screen->h-height)/2;
	rect.x=gd_font_width();
	rect.w=gd_screen->w-2*gd_font_width();
	rect.y=y1;
	rect.h=height;
	gd_backup_and_black_screen();
	draw_window(&rect);
	gd_blittext_n(gd_screen, -1, y1+gd_line_height(), GD_GDASH_WHITE, title);
	gd_blittext_n(gd_screen, -1, y1+3*gd_line_height(), GD_GDASH_GRAY2, "Press desired key for action!");
	SDL_Flip(gd_screen);

	got_key=FALSE;
	while (!gd_quit && !got_key) {
		SDL_Event event;

		SDL_WaitEvent(&event);
		switch(event.type) {
			case SDL_QUIT:
				gd_quit=TRUE;
				break;

			case SDL_KEYDOWN:
				key=event.key.keysym.sym;
				got_key=TRUE;
				break;
			
			default:
				/* other events not interesting */
				break;
		}
	}

	/* restore screen */
	gd_restore_screen();

	gd_wait_for_key_releases();

	if (got_key)
		return key;
	return -1;
}



void
gd_error_console()
{
	GList *iter;
	GPtrArray *err;
	const int yd=gd_line_height();
	const int names_per_page=gd_screen->h/yd-3;
	gboolean exit, clear;
	int sel;
	gboolean redraw;
	
	if (!gd_errors) {
		gd_message("No error messages.");
		return;
	}

	err=g_ptr_array_new();
	for (iter=gd_errors; iter!=NULL; iter=iter->next)
		g_ptr_array_add(err, iter->data);

	/* the user has seen the errors, clear the "has new error" flag */
	gd_clear_error_flag();

	gd_backup_and_dark_screen();
	gd_title_line("GDASH ERRORS");
	gd_status_line("CRSR: SELECT    C: CLEAR    ESC: EXIT");

	exit=FALSE;
	clear=FALSE;

	/* show errors */
	sel=0;
	redraw=TRUE;
	while (!gd_quit && !exit && !clear) {
		int page, i, cur;

		page=sel/names_per_page;

		if (redraw) {
			if (err->len!=0) {
				for (i=0, cur=page*names_per_page; i<names_per_page; i++, cur++) {
					GdErrorMessage *m=g_ptr_array_index(err, cur);
					int col;

					col=cur==sel?GD_GDASH_YELLOW:GD_GDASH_LIGHTBLUE;

					gd_clear_line(gd_screen, (i+2)*yd);
					if (cur<err->len)	/* may not be as much filenames as it would fit on the screen */
						gd_blittext_n(gd_screen, 0, (i+2)*yd, col, m->message);
				}
			}
			SDL_Flip(gd_screen);

			redraw=FALSE;
		}

		gd_process_pending_events();

		/* cursor movement */
		if (gd_up())
			sel=gd_clamp(sel-1, 0, err->len-1), redraw=TRUE;
		if (gd_down())
			sel=gd_clamp(sel+1, 0, err->len-1), redraw=TRUE;
		if (gd_keystate[SDLK_PAGEUP])
			sel=gd_clamp(sel-names_per_page, 0, err->len-1), redraw=TRUE;
		if (gd_keystate[SDLK_PAGEDOWN])
			sel=gd_clamp(sel+names_per_page, 0, err->len-1), redraw=TRUE;
		if (gd_keystate[SDLK_HOME])
			sel=0, redraw=TRUE;
		if (gd_keystate[SDLK_END])
			sel=err->len-1, redraw=TRUE;

		if (gd_fire() || gd_keystate[SDLK_RETURN] || gd_keystate[SDLK_SPACE])
			/* show one error */
			if (err->len!=0) {
				gd_wait_for_key_releases();
				gd_show_error(g_ptr_array_index(err, sel));
			}

		if (gd_keystate[SDLK_c])
			clear=TRUE;

		if (gd_keystate[SDLK_ESCAPE])
			exit=TRUE;

		SDL_Delay(100);
	}

	/* wait until the user releases return and escape, as it might be passed to the caller accidentally */
	/* also wait because we do not want to process one enter keypress more than once */
	gd_wait_for_key_releases();

	g_ptr_array_free(err, TRUE);

	if (clear)
		gd_clear_errors();

	gd_restore_screen();
}


gboolean
gd_ask_yes_no(const char *question, const char *answer1, const char *answer2, gboolean *result)
{
	int height;
	int y1;
	SDL_Rect rect;
	gboolean success, escape;
	int n;

	gd_backup_and_black_screen();
	height=5*gd_line_height();
	y1=(gd_screen->h-height)/2;	/* middle of the screen */
	rect.x=8;
	rect.y=y1;
	rect.w=gd_screen->w-2*8;
	rect.h=height;
	draw_window(&rect);
	SDL_SetClipRect(gd_screen, &rect);

	gd_blittext_n(gd_screen, -1, y1+gd_line_height(), GD_GDASH_WHITE, question);
	gd_blittext_printf_n(gd_screen, -1, y1+3*gd_line_height(), GD_GDASH_WHITE, "N: %s, Y: %s", answer1, answer2);
	SDL_Flip(gd_screen);

	success=escape=FALSE;
	n=0;
	while (!gd_quit && !success && !escape) {
		SDL_Event event;

		n=(n+1)%10;

		while(SDL_PollEvent(&event))
			switch(event.type) {
				case SDL_QUIT:
					gd_quit=TRUE;
					break;

				case SDL_KEYDOWN:
					if (event.key.keysym.sym==SDLK_y) {	/* user pressed yes */
						*result=TRUE;
						success=TRUE;
					}
					else
					if (event.key.keysym.sym==SDLK_n) {	/* user pressed no */
						*result=FALSE;
						success=TRUE;
					}
					else
					if (event.key.keysym.sym==SDLK_ESCAPE)	/* user pressed escape */
						escape=TRUE;
					break;
			}

		SDL_Delay(100);
	}
	/* restore screen */
	gd_restore_screen();

	gd_wait_for_key_releases();

	SDL_SetClipRect(gd_screen, NULL);

	/* this will return true, if y or n pressed. returns false, if escape, or quit event */
	return success;
}


gboolean
gd_discard_changes()
{
	gboolean answered, result;

	/* if not edited, simply answer yes. */
	if (!gd_caveset_edited)
		return TRUE;

	/* if the caveset is edited, ask the user if to save. */
	answered=gd_ask_yes_no("New replays are added. Discard them?", "Cancel", "Discard", &result);
	if (!answered || !result)
		/* if does not want to discard, say false */
		return FALSE;

	return TRUE;
}



void
gd_show_license()
{
	char *wrapped;

	/* remember screen contents */
	gd_backup_and_dark_screen();

	gd_title_line("GDASH LICENSE");
	gd_status_line("SPACE: EXIT");

	wrapped=gd_wrap_text(gd_about_license, gd_screen->w/gd_font_width());
	gd_blittext_n(gd_screen, 0, gd_line_height(), GD_GDASH_LIGHTBLUE, wrapped);
	g_free(wrapped);
	SDL_Flip(gd_screen);

	wait_for_keypress();

	/* copy screen contents back */
	gd_restore_screen();
}



static int
help_writeattrib(int x, int y, const char *name, const char *content)
{
	const int yd=gd_line_height();

	gd_blittext_n(gd_screen, x, y, GD_GDASH_YELLOW, name);
	gd_blittext_n(gd_screen, x+10, y+yd, GD_GDASH_LIGHTBLUE, content);
	if (strchr(content, '\n'))
		y+=yd;

	return y+2*yd+yd/2;
}

static int
help_writeattribs(int x, int y, const char *name, const char *content[])
{
	const int yd=gd_line_height();

	if (content!=NULL && content[0]!=NULL) {
		int i;

		gd_blittext_n(gd_screen, x, y, GD_GDASH_YELLOW, name);

		y+=yd;
		for (i=0; content[i]!=NULL; i++) {
			gd_blittext_n(gd_screen, x+10, y, GD_GDASH_LIGHTBLUE, content[i]);

			y+=yd;
		}
	}

	return y+yd/2;
}


void
gd_about()
{
	int y;

	/* remember screen contents */
	gd_backup_and_dark_screen();

	gd_title_line("GDASH " PACKAGE_VERSION);
	gd_status_line("SPACE: EXIT");

	y=10;

	y=help_writeattrib(-1, y, "", gd_about_comments);
	y=help_writeattrib(10, y, "WEBSITE", gd_about_website);
	y=help_writeattribs(10, y, "AUTHORS", gd_about_authors);
	y=help_writeattribs(10, y, "ARTISTS", gd_about_artists);
	y=help_writeattribs(10, y, "DOCUMENTERS", gd_about_documenters);
	/* extern char *gd_about_translator_credits;  -  NO TRANSLATION IN SDASH */
	SDL_Flip(gd_screen);

	wait_for_keypress();

	/* copy screen contents back */
	gd_restore_screen();
}


void
gd_show_error(GdErrorMessage *error)
{
	char *wrapped;
	char **lines;
	int linenum;
	int y1, i, yd;

	wrapped=gd_wrap_text(error->message, gd_screen->w/gd_font_width()-2);
	gd_backup_and_dark_screen();
	lines=g_strsplit_set(wrapped, "\n", -1);
	linenum=g_strv_length(lines);

	yd=gd_line_height();
	y1=gd_screen->h/2-(linenum+1)*yd/2;

	gd_title_line("GDASH ERROR");
	gd_status_line("ANY KEY: CONTINUE");
	for (i=0; lines[i]!=NULL; i++)
		gd_blittext_n(gd_screen, 8, y1+(i+1)*yd, GD_GDASH_WHITE, lines[i]);
	SDL_Flip(gd_screen);

	wait_for_keypress();

	gd_restore_screen();
	g_free(wrapped);
	g_strfreev(lines);
}



/*
 *   SDASH REPLAYS MENU
 */
void
gd_replays_menu(void (*play_func) (GdCave *cave, GdReplay *replay), gboolean for_game)
{
	gboolean finished;
	/* an item stores a cave (to see its name) or a cave+replay */
	typedef struct _item {
		GdCave *cave;
		GdReplay *replay;
	} Item;
	int n, page;
	int current;
	const int lines_per_page=gd_screen->h/gd_line_height()-5;
	GList *citer;
	GPtrArray *items=NULL;
	GdCave *cave;
	Item i;
	
	items=g_ptr_array_new();
	/* for all caves */
	for (citer=gd_caveset; citer!=NULL; citer=citer->next) {
		GList *riter;
		
		cave=citer->data;
		/* if cave has replays... */
		if (cave->replays!=NULL) {
			/* add cave data */
			i.cave=cave;
			i.replay=NULL;
			g_ptr_array_add(items, g_memdup(&i, sizeof(i)));
			
			/* add replays, too */
			for (riter=cave->replays; riter!=NULL; riter=riter->next) {
				i.replay=(GdReplay *)riter->data;
				g_ptr_array_add(items, g_memdup(&i, sizeof(i)));
			}
		}
	}
	
	if (items->len==0) {
		gd_message("No replays.");
	} else {
		
		gd_backup_and_dark_screen();
		if (for_game)
			gd_status_line("CRSR:MOVE  SPACE:PLAY  S:SAVED  ESC:EXIT");
		else
			gd_status_line("CRSR: MOVE   SPACE: SAVE   ESC: EXIT");
		
		current=1;
		finished=FALSE;
		while (!finished && !gd_quit) {
			page=current/lines_per_page;	/* show 18 lines per page */
			SDL_Event event;
			
			/* show lines */
			gd_clear_line(gd_screen, 0);	/* for empty top row */
			for (n=0; n<lines_per_page; n++) {	/* for empty caves&replays rows */
				int y=(n+2)*gd_line_height();
				
				gd_clear_line(gd_screen, y);
			}

			gd_title_line("GDASH REPLAYS, PAGE %d/%d", page+1, items->len/lines_per_page+1);
			for (n=0; n<lines_per_page && page*lines_per_page+n<items->len; n++) {
				int pos=page*lines_per_page+n;
				GdColor col_cave=current==pos?GD_GDASH_YELLOW:GD_GDASH_LIGHTBLUE;	/* selected=yellow, otherwise blue */
				GdColor col=current==pos?GD_GDASH_YELLOW:GD_GDASH_GRAY3;	/* selected=yellow, otherwise blue */
				Item *i;
				int x, y;
				
				i=(Item *) g_ptr_array_index(items, pos);

				x=0;
				y=(n+2)*gd_line_height();
				
				if (!i->replay) {
					/* no replay pointer: this is a cave, so write its name. */
					x=gd_blittext_n(gd_screen, x, y, col_cave, i->cave->name);
				} else {
					const char *comm;
					int c;
					char buffer[100];
					
					/* successful or not */
					x=gd_blittext_printf_n(gd_screen, x, y, i->replay->success?GD_GDASH_GREEN:GD_GDASH_RED, " %c ", GD_BALL_CHAR);

					/* player name */
					g_utf8_strncpy(buffer, i->replay->player_name, 15);	/* name: maximum 15 characters */
					x=gd_blittext_n(gd_screen, x, y, col, buffer);
					/* put 16-length spaces */
					for (c=g_utf8_strlen(buffer, -1); c<16; c++)
						x=gd_blittext_n(gd_screen, x, y, col, " ");

					/* write comment */
					if (!g_str_equal(i->replay->comment, ""))
						comm=i->replay->comment;
					else
					/* or date */
					if (!g_str_equal(i->replay->date, ""))
						comm=i->replay->date;
					else
					/* or nothing */
						comm="-";
					g_utf8_strncpy(buffer, comm, 17);	/* comment or data: maximum 18 characters */
					x=gd_blittext_n(gd_screen, x, y, col, comm);
					/* put 20-length spaces */
					for (c=g_utf8_strlen(buffer, -1); c<17; c++)
						x=gd_blittext_n(gd_screen, x, y, col, " ");

					/* level */				
					x=gd_blittext_printf_n(gd_screen, x, y, col, " %d", i->replay->level+1);
					/* saved - check box */				
					x=gd_blittext_printf_n(gd_screen, x, y, col, " %c", i->replay->saved?GD_CHECKED_BOX_CHAR:GD_UNCHECKED_BOX_CHAR);
				}
			}
			SDL_Flip(gd_screen);	/* draw to usere's screen */
			
			SDL_WaitEvent(&event);
			switch (event.type) {
				case SDL_QUIT:
					gd_quit=TRUE;
					break;
					
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_UP:
							do {
								current=gd_clamp(current-1, 1, items->len-1);
							} while (((Item *)g_ptr_array_index(items, current))->replay==NULL && current>=1);
							break;
						case SDLK_DOWN:
							do {
								current=gd_clamp(current+1, 1, items->len-1);
							} while (((Item *)g_ptr_array_index(items, current))->replay==NULL && current<items->len);
							break;
						case SDLK_s:
							/* only allow toggling the "saved" flag, if we are in a game. not in the replay->video converter app. */
							if (for_game) {
								Item *i=(Item *)g_ptr_array_index(items, current);
								
								if (i->replay) {
									i->replay->saved=!i->replay->saved;
									gd_caveset_edited=TRUE;
								}
							}
							break;
						case SDLK_SPACE:
						case SDLK_RETURN:
							{
								Item *i=(Item *)g_ptr_array_index(items, current);
								
								if (i->replay) {
									gd_backup_and_black_screen();
									SDL_Flip(gd_screen);
									play_func(i->cave, i->replay);
									gd_restore_screen();
								}
							}
							break;
						case SDLK_ESCAPE:
							finished=TRUE;
							break;
						
						case SDLK_PAGEUP:
							current=gd_clamp(current-lines_per_page, 0, items->len-1);
							break;
							
						case SDLK_PAGEDOWN:
							current=gd_clamp(current+lines_per_page, 0, items->len-1);
							break;
							
						default:
							/* other keys do nothing */
							break;
					}
					break;
				
				default:
					/* other events do nothing */
					break;
			}
		}
		
		gd_restore_screen();
	}
	
	/* set the theme. other variables are already set by the above code. */
	/* forget list of themes */
	g_ptr_array_foreach(items, (GFunc) g_free, NULL);
	g_ptr_array_free(items, TRUE);
	
}


