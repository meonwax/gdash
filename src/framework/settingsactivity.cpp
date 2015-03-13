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

#include <cstring>
#include <glib/gi18n.h>

#include "framework/settingsactivity.hpp"
#include "framework/commands.hpp"
#include "input/gameinputhandler.hpp"
#include "cave/helper/colors.hpp"
#include "cave/gamerender.hpp"
#include "gfx/pixbuffactory.hpp"
#include "gfx/cellrenderer.hpp"
#include "gfx/screen.hpp"
#include "gfx/fontmanager.hpp"
#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "misc/util.hpp"
#include "settings.hpp"

#include "framework/thememanager.hpp"


class SelectKeyActivity: public Activity {
public:
    SelectKeyActivity(App *app, char const *title, char const *action, int *pkeycode)
    : Activity(app),
      title(title), action(action), pkeycode(pkeycode) {
    }
    virtual void keypress_event(KeyCode keycode, int gfxlib_keycode);
    virtual void redraw_event();

private:
    std::string title, action;
    int *pkeycode;
};


void SelectKeyActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    *pkeycode = gfxlib_keycode;
    app->enqueue_command(new PopActivityCommand(app));
}


void SelectKeyActivity::redraw_event() {
    int height=6*app->font_manager->get_line_height();
    int y1=(app->screen->get_height()-height)/2;    /* middle of the screen */
    int cx=2*app->font_manager->get_font_width_narrow(), cy=y1, cw=app->screen->get_width()-2*cx, ch=height;

    app->draw_window(cx, cy, cw, ch);
    app->screen->set_clip_rect(cx, cy, cw, ch);
    app->set_color(GD_GDASH_WHITE);
    app->blittext_n(-1, y1+app->font_manager->get_line_height(), title.c_str());
    app->blittext_n(-1, y1+3*app->font_manager->get_line_height(), action.c_str());
    app->screen->remove_clip_rect();

    app->screen->flip();
}


class ThemeSelectedCommand: public Command1Param<std::string> {
public:
    ThemeSelectedCommand(App *app, SettingsActivity *activity)
    :
        Command1Param<std::string>(app),
        activity(activity),
        filename(p1) {
    }

private:
    SettingsActivity *activity;
    std::string &filename;
    virtual void execute() {
        Logger l;
        if (CellRenderer::is_image_ok_for_theme(*app->pixbuf_factory, filename.c_str())) {
            /* copy theme to user config directory */
            std::string new_name = filename_for_new_theme(filename.c_str());
            install_theme(filename.c_str(), new_name.c_str());
            activity->load_themes();
        } else {
            /* if file is not ok as a theme, error was logged */
        }
        
        if (!l.empty()) {
            app->show_text_and_do_command(_("Cannot install theme!"), l.get_messages_in_one_string());
            l.clear();
        }
    }
};


void SettingsActivity::load_themes() {
    load_themes_list(*app->pixbuf_factory, themes, themenum);
}


SettingsActivity::SettingsActivity(App *app, Setting *settings_data)
: Activity(app)
{
    /* copy the pointer into the member variable */
    settings = settings_data;
    for (numsettings = 0; settings[numsettings].name != NULL; ++numsettings)
        ;
    numpages = settings[numsettings-1].page + 1;    /* number of pages: take it from the last setting */

    load_themes();

    yd = app->font_manager->get_line_height();
    y1.resize(numpages);
    for (unsigned page = 0; page < numpages; page++) {
        int num=0;
        for (unsigned n = 0; n < numsettings; n++)
            if (settings[n].page == page)
                num++;
        y1[page] = (app->screen->get_height() - num*yd) / 2;
    }
    current = 1;    /* 0th is presumably a page identifier */
    restart = false;
}


SettingsActivity::~SettingsActivity() {
    gd_theme = themes[themenum];
    if (restart)
        app->request_restart();
}


void SettingsActivity::keypress_event(KeyCode keycode, int gfxlib_keycode) {
    unsigned page = settings[current].page;
    switch (keycode) {
        case App::Up:
            current = gd_clamp(current-1, 0, numsettings-1);
            break;
        case App::Down:
            current = gd_clamp(current+1, 0, numsettings-1);
            break;
        case App::PageUp:
            if (page > 0)
                while (settings[current].page==page)
                    current--;    /* decrement until previous page is found */
            break;
        case App::PageDown:
            if (page < numpages-1)
                while (settings[current].page==page)
                    current++;    /* increment until previous page is found */
            break;

        /* CHANGE SETTINGS */
        case App::Left:    /* key left */
            switch (settings[current].type) {
                case TypePage:
                    break;
                case TypeBoolean:
                    *(bool *)settings[current].var=false;
                    break;
                case TypePercent:
                    *(int *)settings[current].var=gd_clamp(*(int *)settings[current].var - 5, 0, 100);
                    break;
                case TypeTheme:
                    themenum=gd_clamp(themenum-1, 0, themes.size()-1);
                    break;
                case TypeStringv:
                    *(int *)settings[current].var=gd_clamp(*(int *)settings[current].var-1, 0, g_strv_length((char **) settings[current].stringv)-1);
                    break;
                case TypeKey:
                    break;
            }
            if (settings[current].restart)
                restart=true;
            break;

        case App::Right:    /* key right */
            switch(settings[current].type) {
                case TypePage:
                    break;
                case TypeBoolean:
                    *(bool *)settings[current].var=true;
                    break;
                case TypePercent:
                    *(int *)settings[current].var=gd_clamp(*(int *)settings[current].var + 5, 0, 100);
                    break;
                case TypeTheme:
                    themenum=gd_clamp(themenum+1, 0, themes.size()-1);
                    break;
                case TypeStringv:
                    *(int *)settings[current].var=gd_clamp(*(int *)settings[current].var+1, 0, g_strv_length((char **) settings[current].stringv)-1);
                    break;
                case TypeKey:
                    break;
            }
            if (settings[current].restart)
                restart=true;
            break;

        case ' ':
        case App::Enter:
            switch (settings[current].type) {
                case TypePage:
                    break;
                case TypeBoolean:
                    *(bool *)settings[current].var=!*(bool *)settings[current].var;
                    break;
                case TypePercent:
                    *(int *)settings[current].var=gd_clamp(*(int *)settings[current].var + 5, 0, 100);
                    break;
                case TypeTheme:
                    themenum=(themenum+1)%themes.size();
                    break;
                case TypeStringv:
                    *(int *)settings[current].var=(*(int *)settings[current].var+1)%g_strv_length((char **) settings[current].stringv);
                    break;
                case TypeKey:
                    // TRANSLATORS: 35 chars max
                    app->enqueue_command(new PushActivityCommand(app, new SelectKeyActivity(app, _("Select key for action"), settings[current].name, (int *) settings[current].var)));
                    break;
            }
            if (settings[current].restart)
                restart=true;
            break;

        case 't':
        case 'T':
            // TRANSLATORS: 40 chars max
            app->select_file_and_do_command(_("Select Image for Theme"), g_get_home_dir(), "*.bmp;*.png", false, "", new ThemeSelectedCommand(app, this));
            break;

        case App::Escape:    /* finished options menu */
            app->enqueue_command(new PopActivityCommand(app));
            break;
    }
    
    if (settings[current].type == TypePage)
        current++;
    redraw_event();
}


void SettingsActivity::redraw_event() {
    app->clear_screen();
    // TRANSLATORS: 40 chars max
    app->title_line(CPrintf(_("GDash Options, page %d/%d")) % (settings[current].page+1) % numpages);
    // TRANSLATORS: 40 chars max. Change means to change the setting
    app->status_line(_("Crsr: Move   Space: change   Esc: exit"));
    app->set_color(GD_GDASH_GRAY1);
    // TRANSLATORS: 40 chars max
    app->blittext_n(-1, app->screen->get_height() - 2*app->font_manager->get_line_height(), _("Press T to install a new theme."));

    /* show settings */
    unsigned page = settings[current].page;
    unsigned linenum = 0;
    for (unsigned n = 0; n < numsettings; n++) {
        if (settings[n].page == page) {
            std::string value;
            switch(settings[n].type) {
                case TypePage:
                    break;
                case TypeBoolean:
                    value = *(bool *)settings[n].var? _("yes") : _("no");
                    break;
                case TypePercent:
                    value = SPrintf("%d%%") % *(int *)settings[n].var;
                    break;
                case TypeTheme:
                    if (themenum==0)
                        value = _("[Default]");
                    else {
                        char *thm = g_filename_display_basename(themes[themenum].c_str());
                        if (strrchr(thm, '.'))    /* remove extension */
                            *strrchr(thm, '.')='\0';
                        value = thm;
                        g_free(thm);
                    }
                    break;
                case TypeStringv:
                    value = settings[n].stringv[*(int *)settings[n].var];
                    break;
                case TypeKey:
                    value = app->gameinput->get_key_name_from_keycode(*(guint *)settings[n].var);
                    break;
            }
            
            int y = y1[page]+linenum*yd;
            if (settings[n].type != TypePage) {
                int x = 4*app->font_manager->get_font_width_narrow();
                app->blittext_n(x, y, CPrintf("%c%s  %c%s") % (current==n ? GD_COLOR_INDEX_YELLOW : GD_COLOR_INDEX_LIGHTBLUE) % settings[n].name % GD_COLOR_INDEX_GREEN % value);
            } else {
                int x = 2*app->font_manager->get_font_width_narrow();
                app->blittext_n(x, y, CPrintf("%c%s") % GD_COLOR_INDEX_WHITE % settings[n].name);
            }
            linenum++;
        }
    }
    app->screen->flip();
}
