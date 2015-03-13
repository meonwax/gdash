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
#include <SDL.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "config.h"
#include "cave.h"
#include "caveset.h"
#include "sdl_gfx.h"
#include "settings.h"
#include "util.h"
#include "about.h"

#include "sdl_ui.h"

int filenamesort(gconstpointer a, gconstpointer b)
{
	gchar **a_=(gpointer) a, **b_=(gpointer) b;
	return g_ascii_strcasecmp(*a_, *b_);
}



/* runs a file selection dialog */
/* returns new, full-path filename (to be freed later by the caller) */
/* glob: semicolon separated list of globs */
enum {
	GD_NOT_YET,
	GD_YES,
	GD_JUMP,
	GD_ESCAPE,
	GD_QUIT,
	
};

int clamp(int val, int min, int max)
{
	g_assert(min<=max);
	
	if (val<min)
		return min;
	if (val>max)
		return max;
	return val;
}


char *
gd_select_file(const char *title, const char *start_dir, const char *glob)
{
	static char *current_dir=NULL;	/* static to not worry about freeing */
	const int yd=10;
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
	gd_blittext_n(gd_screen, -1, 0, GD_C64_WHITE, title);
	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "MOVE: SELECT   J: JUMP   ESC: CANCEL");

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
		gd_blittext_n(gd_screen, -1, 1*yd, GD_C64_YELLOW, gd_filename_to_utf8(directory));
		
		/* do file selection menu */
		sel=0;
		state=GD_NOT_YET;
		redraw=TRUE;
		while (state==GD_NOT_YET) {
			int page, i, cur;
			
			page=sel/names_per_page;
			
			if (redraw) {
				for (i=0, cur=page*names_per_page; i<names_per_page; i++, cur++) {
					int col;
					
					col=cur==sel?GD_C64_YELLOW:GD_C64_LIGHTBLUE;
					
					gd_clear_line(gd_screen, (i+2)*yd);
					if (cur<files->len)	/* may not be as much filenames as it would fit on the screen */
						gd_blittext_n(gd_screen, 0, (i+2)*yd, col, gd_filename_to_utf8(g_ptr_array_index(files, cur)));
				}
				SDL_Flip(gd_screen);
				
				redraw=FALSE;
			}

			/* check for incoming events */			
			gd_process_pending_events();
			if (gd_quit)
				state=GD_QUIT;
			
			/* cursor movement */
			if (gd_up()) {
				sel=clamp(sel-1, 0, files->len-1);
				redraw=TRUE;
			}
			if (gd_down()) {
				sel=clamp(sel+1, 0, files->len-1);
				redraw=TRUE;
			}
			if (gd_keystate[SDLK_PAGEUP])
				sel=clamp(sel-names_per_page, 0, files->len-1), redraw=TRUE;
			if (gd_keystate[SDLK_PAGEDOWN])
				sel=clamp(sel+names_per_page, 0, files->len-1), redraw=TRUE;
			if (gd_keystate[SDLK_HOME])
				sel=0, redraw=TRUE;
			if (gd_keystate[SDLK_END])
				sel=files->len-1, redraw=TRUE;
				
			if (gd_keystate[SDLK_j])
				state=GD_JUMP;

			if (gd_space_or_enter_or_fire())
				state=GD_YES;

			if (gd_keystate[SDLK_ESCAPE])
				state=GD_ESCAPE;

			SDL_Delay(100);
		}

		/* wait until the user releases return and escape, as it might be passed to the caller accidentally */
		/* also wait because we do not want to process one enter keypress more than once */
		gd_wait_for_key_releases();

		/* if selected any from the list, it can be a file or a directory. */		
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
	
	g_strfreev(globs);
	gd_restore_screen();
	
	return result;
}



void
gd_settings_menu()
{
	gboolean finished;
	const char *yes="yes", *no="no ";
	struct _setting {
		char *name;
		gboolean *var;
	} settings[]= {
		{ "Fullscreen", &gd_sdl_fullscreen },
		{ "Sound", &gd_sdl_sound },
		{ "16-bit mixing", &gd_sdl_16bit_mixing },
		{ "44khz mixing", &gd_sdl_44khz_mixing },
		{ "Easy play", &gd_easy_play },
		{ "Use BDCFF highscore", &gd_use_bdcff_highscore },
		{ "All caves selectable", &gd_all_caves_selectable },
		{ "Random colors", &gd_random_colors },
	};
	int n;
	int current;
	int width, x1;
	
	width=0;
	for (n=0; n<G_N_ELEMENTS(settings); n++)
		width=MAX(width, (strlen(settings[n].name)+1+strlen(yes))*8);	/* FIXME: 8=font width */
	x1=(gd_screen->w-width)/2;
		
	gd_backup_and_dark_screen();
	gd_blittext_n(gd_screen, -1, 0, GD_C64_WHITE, "GDASH OPTIONS");
	gd_blittext_n(gd_screen, -1, gd_screen->h-32, GD_C64_GRAY2, "Some changes require restart.");
	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "CRSR: MOVE   FIRE: CHANGE   ESC: EXIT");
	
	current=0;
	finished=FALSE;
	while (!finished && !gd_quit) {
		/* show settings */
		for (n=0; n<G_N_ELEMENTS(settings); n++) {
			int x;

			gd_clear_line(gd_screen, 20+n*10);
			x=x1;
			x=gd_blittext_n(gd_screen, x, 20+n*10, GD_C64_YELLOW, *settings[n].var?yes:no);
			x+=10;
			x=gd_blittext_n(gd_screen, x, 20+n*10, current==n?GD_C64_LIGHTGREEN:GD_C64_LIGHTBLUE, settings[n].name);
		}
		SDL_Flip(gd_screen);
		
		/* do events; keys will be processed below */
		gd_process_pending_events();

		/* cursor movement */
		if (gd_up())
			current=clamp(current-1, 0, G_N_ELEMENTS(settings)-1);
		if (gd_down())
			current=clamp(current+1, 0, G_N_ELEMENTS(settings)-1);
		if (gd_left())
			*settings[current].var=FALSE;
		if (gd_right())
			*settings[current].var=TRUE;
		if (gd_space_or_enter_or_fire())
			*settings[current].var=!*settings[current].var;
		if (gd_keystate[SDLK_ESCAPE])
			finished=TRUE;

		SDL_Delay(100);
	}
	
	gd_restore_screen();
}


void
gd_show_highscore(Cave *highlight_cave, int highlight_line)
{
	gboolean finished;
	GList *current=NULL;	/* current cave to view */
	
	gd_backup_and_dark_screen();
	gd_blittext_n(gd_screen, -1, 0, GD_C64_WHITE, "THE HALL OF FAME");
	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "CRSR: CAVE   SPACE: EXIT");
	
	current=NULL;
	finished=FALSE;
	while (!finished && !gd_quit) {
		Cave *cave;
		int i;
		
		/* current cave or game */
		if (current)
			cave=current->data;
		else
			cave=gd_default_cave;

		gd_clear_line(gd_screen, (1)*8);
		gd_blittext_n(gd_screen, -1, (1)*8, GD_C64_YELLOW, cave->name);

		for (i=0; i<20; i++)
			gd_clear_line(gd_screen, (i+2)*8);

		/* show scores */		
		for (i=0; i<G_N_ELEMENTS(cave->highscore); i++) {
			int c;
			
			c=i/5%2?GD_C64_PURPLE:GD_C64_GREEN;
			if (cave==highlight_cave && i==highlight_line)
				c=GD_C64_WHITE;
			if (cave->highscore[i].score!=0)
				gd_blittext_printf_n(gd_screen, 0, (i+3)*8, c, "%2d %6d %s", i+1, cave->highscore[i].score, cave->highscore[i].name);
		}
		SDL_Flip(gd_screen);

		/* do events; keys will be processed below */
		gd_process_pending_events();

		/* cursor movement */
		if (gd_left() || gd_up()) {
			/* if not showing game, step one cave back. if no previous cave, will be null, so we show game */
			if (current!=NULL)
				current=current->prev;
		}
		if (gd_right() || gd_down()) {
			if (current!=NULL) {
				/* if showing a cave, go to next cave (if any) */
				if (current->next!=NULL)
					current=current->next;
			}
			else {
				/* if showing game, show first cave. */
				current=gd_caveset;
			}
		}
		if (gd_space_or_enter_or_fire() || gd_keystate[SDLK_ESCAPE])
			finished=TRUE;
			
		SDL_Delay(150);
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
	
	gd_blittext_n(gd_screen, -1, 0, GD_C64_WHITE, "GDASH HELP");
	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "SPACE: EXIT");

	numstrings=g_strv_length((gchar **) strings);

	charwidth=0;
	for (n=0; n<numstrings; n+=2)
		charwidth=MAX(charwidth, strlen(strings[n])+1+strlen(strings[n+1]));
	x1=gd_screen->w/2-charwidth*8/2;	/* FIXME 8 */

	y=(gd_screen->h-numstrings*10/2)/2;
	for (n=0; n<numstrings; n+=2) {
		int x;
		
		x=gd_blittext_printf_n(gd_screen, x1, y, GD_C64_YELLOW, "%s ", strings[n]);
		x=gd_blittext_printf_n(gd_screen, x, y, GD_C64_LIGHTBLUE, "%s", strings[n+1]);
		
		y+=10;
	}

	SDL_Flip(gd_screen);
	
	wait_for_keypress();

	/* copy screen contents back */
	gd_restore_screen();
}





char *
gd_input_string(const char *title, const char *current)
{
	const int height=40;
	int y1;
	SDL_Rect rect;
	gboolean enter, escape;
	GString *text;
	int n;
	
	y1=(gd_screen->h-height)/2;
	rect.x=0;
	rect.w=gd_screen->w;
	rect.y=y1;
	rect.h=height;
	gd_backup_and_black_screen();
	SDL_FillRect(gd_screen, &rect, SDL_MapRGB(gd_screen->format, 0, 0, 0));

	gd_blittext_n(gd_screen, -1, y1+4, GD_C64_WHITE, title);
	
	text=g_string_new(current);

	/* setup keyboard */
	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	enter=escape=FALSE;
	n=0;
	while (!gd_quit && !enter && !escape) {
		SDL_Event event;
		
		n=(n+1)%10;
		gd_clear_line(gd_screen, y1+20);
		gd_blittext_printf_n(gd_screen, -1, y1+20, GD_C64_WHITE, "%s%c", text->str, n>=5?'_':' ');
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
	SDL_EnableKeyRepeat(0, 0);
	/* restore screen */
	gd_restore_screen();

	gd_wait_for_key_releases();
	
	/* if quit, return nothing. */
	if (gd_quit)
		return NULL;
	
	if (enter)
		return g_string_free(text, FALSE);
		
	/* here must be escape=TRUE */
	g_string_free(text, TRUE);
	return NULL;
}


void
gd_error_console()
{
	GList *iter;
	GPtrArray *err;
	const int yd=10;
	const int names_per_page=gd_screen->h/yd-3;
	gboolean exit, clear;
	int sel;
	gboolean redraw;

	err=g_ptr_array_new();	
	for (iter=gd_errors; iter!=NULL; iter=iter->next)
		g_ptr_array_add(err, iter->data);

	/* the user has seen the errors, clear the "has new error" flag */
	gd_clear_error_flag();
	
	gd_backup_and_dark_screen();
	gd_blittext_n(gd_screen, -1, 0, GD_C64_WHITE, "GDASH ERRORS");
	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "CRSR: SELECT    C: CLEAR    ESC: EXIT");

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
					
					col=cur==sel?GD_C64_YELLOW:GD_C64_LIGHTBLUE;
					
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
			sel=clamp(sel-1, 0, err->len-1), redraw=TRUE;
		if (gd_down())
			sel=clamp(sel+1, 0, err->len-1), redraw=TRUE;
		if (gd_keystate[SDLK_PAGEUP])
			sel=clamp(sel-names_per_page, 0, err->len-1), redraw=TRUE;
		if (gd_keystate[SDLK_PAGEDOWN])
			sel=clamp(sel+names_per_page, 0, err->len-1), redraw=TRUE;
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


void
gd_show_license()
{
	char *wrapped;
	
	/* remember screen contents */
	gd_backup_and_dark_screen();
	
	gd_blittext_n(gd_screen, -1, 0, GD_C64_WHITE, "GDASH LICENSE");
	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "SPACE: EXIT");

	wrapped=gd_wrap_text(gd_about_license, 40);
	gd_blittext_n(gd_screen, 0, 12, GD_C64_LIGHTBLUE, wrapped);
	g_free(wrapped);
	SDL_Flip(gd_screen);

	wait_for_keypress();	

	/* copy screen contents back */
	gd_restore_screen();
}



static int
help_writeattrib(int x, int y, const char *name, const char *content)
{
	const int yd=10;
	
	gd_blittext_n(gd_screen, x, y, GD_C64_YELLOW, name);
	gd_blittext_n(gd_screen, x+10, y+yd, GD_C64_LIGHTBLUE, content);
	if (strchr(content, '\n'))
		y+=yd;
	
	return y+2*yd+yd/2;
}

static int
help_writeattribs(int x, int y, const char *name, const char *content[])
{
	const int yd=10;
	
	if (content!=NULL && content[0]!=NULL) {
		int i;
		
		gd_blittext_n(gd_screen, x, y, GD_C64_YELLOW, name);

		y+=yd;
		for (i=0; content[i]!=NULL; i++) {
			gd_blittext_n(gd_screen, x+10, y, GD_C64_LIGHTBLUE, content[i]);
			
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
	
	gd_blittext_printf_n(gd_screen, -1, 0, GD_C64_WHITE, "GDASH %s", PACKAGE_VERSION);
	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "SPACE: EXIT");

	y=20;
	
	y=help_writeattrib(-1, y, "", gd_about_comments);
	y=help_writeattrib(10, y, "WEBSITE", gd_about_website);
	y=help_writeattribs(10, y, "AUTHORS", gd_about_authors);
	y=help_writeattribs(10, y, "ARTISTS", gd_about_artists);
	y=help_writeattribs(10, y, "DOCUMENTERS", gd_about_documenters);
	// extern char *gd_about_translator_credits;
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
	int y1, i;

	wrapped=gd_wrap_text(error->message, 38);	/* FIXME */
	gd_backup_and_dark_screen();
	lines=g_strsplit_set(wrapped, "\n", -1);
	linenum=g_strv_length(lines);

	y1=gd_screen->h/2-(linenum+1)*10/2;
#if 0
	rect.x=8;
	rect.w=gd_screen->w-16;
	rect.y=y1;
	rect.h=(linenum+1)*10;
	SDL_FillRect(gd_screen, &rect, SDL_MapRGB(gd_screen->format, 0, 0, 0));
#endif

	gd_blittext_n(gd_screen, -1, 0, GD_C64_WHITE, "GDASH ERROR");
	gd_blittext_n(gd_screen, -1, gd_screen->h-8, GD_C64_GRAY2, "ANY KEY: CONTINUE");
	for (i=0; lines[i]!=NULL; i++)
		gd_blittext_n(gd_screen, 8, y1+(i+1)*10, GD_C64_WHITE, lines[i]);
	SDL_Flip(gd_screen);
	
	wait_for_keypress();
	
	gd_restore_screen();
	g_free(wrapped);
	g_strfreev(lines);
}

