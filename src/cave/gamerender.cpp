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

#include <glib/gi18n.h>

#include "settings.hpp"
#include "cave/caverendered.hpp"
#include "cave/caveset.hpp"
#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "misc/util.hpp"

#include "gfx/pixbuf.hpp"
#include "gfx/pixmap.hpp"
#include "gfx/screen.hpp"
#include "gfx/pixbuffactory.hpp"
#include "gfx/pixbufmanip.hpp"
#include "gfx/cellrenderer.hpp"
#include "gfx/fontmanager.hpp"
#include "cave/gamecontrol.hpp"
#include "cave/elementproperties.hpp"

#include "cave/gamerender.hpp"

// TODO should not remember screen size in play_area_w and _h?

const char **gd_status_bar_colors_get_names() {
    static const char *types[]={
        "C64 BD",
        "1stB",
        "CrLi",
        "Final BD",
        "Atari BD",
        NULL
    };    
    return types;
}

GameRenderer::GameRenderer(Screen &screen_, CellRenderer &cells_, FontManager &font_manager_, GameControl &game_)
:   screen(screen_),
    cells(cells_),
    font_manager(font_manager_),
    game(game_),
    play_area_w(0), play_area_h(0),
    statusbar_height(0), statusbar_y1(0), statusbar_y2(0), statusbar_mid(0),
    out_of_window(false), show_replay_sign(true),
    scroll_x(0), scroll_y(0),
    scroll_desired_x(0), scroll_desired_y(0),
    story_background(NULL),
    statusbar_since(0) {
}

GameRenderer::~GameRenderer() {
}

void GameRenderer::set_show_replay_sign(bool srs) {
    show_replay_sign=srs;
}

/* just set current viewport to upper left. */
void GameRenderer::scroll_to_origin() {
    scroll_x=0;
    scroll_y=0;
}

/*
    logical_size: logical pixel size of playfield, usually larger than the screen.
    physical_size: visible part. (remember: player_x-x1!)

    center: the coordinates to scroll to.
    exact: scroll exactly
    start: start scrolling if difference is larger than
    to: scroll to, if started, until difference is smaller than
    current

    desired: the function stores its data here
    speed: pixels to scroll at once
    cell_size: size of one cell. used to determine if the play field is only a slightly larger than the screen, in that case no scrolling is desirable
*/
 bool GameRenderer::cave_scroll(int logical_size, int physical_size, int center, bool exact, int start, int to, int &current, int &desired, int speed, int cell_size) {
    int max=logical_size-physical_size;
    if (max<0)
        max=0;

    bool changed=false;

    /* if cave size smaller than the screen, no scrolling req'd */
    if (logical_size<physical_size) {
        desired=0;
        if (current!=0) {
            current=0;
            changed=true;
        }

        return changed;
    }

    /* if cave size is only a slightly larger than the screen, also no scrolling */
    if (logical_size<=physical_size+cell_size) {
        desired=max/2;  /* scroll to the middle of the cell */
    } else {
        /* hystheresis function.
         * when scrolling left, always go a bit less left than player being at the middle.
         * when scrolling right, always go a bit less to the right. */
        if (exact)
            desired=center;
        else {
            if (current+start<center)
                desired=center-to;
            if (current-start>center)
                desired=center+to;
        }
    }
    desired=gd_clamp(desired, 0, max);

    if (current<desired) {
        for (int i=0; i<speed; i++)
            if (current<desired)
                (current)++;
        changed=true;
    }
    if (current > desired) {
        for (int i=0; i<speed; i++)
            if (current>desired)
                (current)--;
        changed=true;
    }

    return changed;
}


/* SCROLLING
 *
 * scrolls to the player during game play.
 * called by drawcave
 * returns true, if player is not visible-ie it is out of the visible size in the drawing area.
 */
bool GameRenderer::scroll(int ms, bool exact_scroll) {
    int cell_size=cells.get_cell_size();

    /* max scrolling speed depends on the speed of the cave. */
    /* game moves cell_size_game* 1s/cave time pixels in a second. */
    /* scrolling moves scroll speed * 1s/scroll_time in a second. */
    /* these should be almost equal; scrolling speed a little slower. */
    /* that way, the player might reach the border with a small probability, */
    /* but the scrolling will not "oscillate", ie. turn on for little intervals as it has */
    /* caught up with the desired position. smaller is better. */
    int scroll_speed=cell_size*ms/game.played_cave->speed;

    int player_x=game.played_cave->player_x-game.played_cave->x1;   /* cell coordinates of player */
    int player_y=game.played_cave->player_y-game.played_cave->y1;
    int visible_x=(game.played_cave->x2-game.played_cave->x1+1)*cell_size;  /* pixel size of visible part of the cave (may be smaller in intermissions) */
    int visible_y=(game.played_cave->y2-game.played_cave->y1+1)*cell_size;

    bool changed=false;
    if (cave_scroll(visible_x, play_area_w, player_x*cell_size+cell_size/2-play_area_w/2, exact_scroll, play_area_w/4, play_area_w/8, scroll_x, scroll_desired_x, scroll_speed, cell_size))
        changed=true;
    if (cave_scroll(visible_y, play_area_h, player_y*cell_size+cell_size/2-play_area_h/2, exact_scroll, play_area_h/5, play_area_h/10, scroll_y, scroll_desired_y, scroll_speed, cell_size))
        changed=true;

    /* if scrolling, we should update entire screen. */
    if (changed) {
        for (int y=0; y<game.played_cave->h; y++)
            for (int x=0; x<game.played_cave->w; x++)
                game.gfx_buffer(x, y) |= GD_REDRAW;
    }

    /* check if active player is visible at the moment. */
    bool out_of_window=false;
    /* check if active player is outside drawing area. if yes, we should wait for scrolling */
    if ((player_x*cell_size)<scroll_x || (player_x*cell_size+cell_size-1)>scroll_x+play_area_w)
        /* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
        if (game.played_cave->player_x>=game.played_cave->x1 && game.played_cave->player_x<=game.played_cave->x2)
            out_of_window=true;
    if ((player_y*cell_size)<scroll_y || (player_y*cell_size+cell_size-1)>scroll_y+play_area_h)
        /* but only do the wait, if the player SHOULD BE visible, ie. he is inside the defined visible area of the cave */
        if (game.played_cave->player_y>=game.played_cave->y1 && game.played_cave->player_y<=game.played_cave->y2)
            out_of_window=true;

    /* if not yet born, we treat as visible. so cave will run. the user is unable to control an unborn player, so this is the right behaviour. */
    if (game.played_cave->player_state==GD_PL_NOT_YET)
        return false;
    return out_of_window;
}


void GameRenderer::drawcave(bool force_draw) {
    int cell_size=cells.get_cell_size();

    /* on-screen clipping rectangle */
    screen.set_clip_rect(0, statusbar_height, play_area_w, play_area_h);

    int scroll_y_aligned;
    if (cells.get_pal_emulation())
        scroll_y_aligned=scroll_y/2*2;      /* make it even (dividable by two) */
    else
        scroll_y_aligned=scroll_y;

    /* if the cave is smaller than the play area, add some pixels to make it centered */
    int xplus, yplus;
    int cave_pixel_w=(game.played_cave->x2-game.played_cave->x1+1)*cell_size;
    int cave_pixel_h=(game.played_cave->y2-game.played_cave->y1+1)*cell_size;
    if (play_area_w>cave_pixel_w)
        xplus=(play_area_w-cave_pixel_w)/2;
    else
        xplus=0;
    if (play_area_h>cave_pixel_h)
        yplus=(play_area_h-cave_pixel_h)/2;
    else
        yplus=0;

    /* here we draw all cells to be redrawn. the in-cell clipping will be done by the graphics
     * engine, we only clip full cells. */
    /* the x and y coordinates are cave physical coordinates.
     * xd and yd are relative to the visible area. */
    int x, y, xd, yd;
    for (y=game.played_cave->y1, yd=0; y<=game.played_cave->y2; y++, yd++) {
        int ys=yplus + yd*cell_size+statusbar_height-scroll_y_aligned;
        if (ys>=play_area_h+statusbar_height || ys+cell_size<statusbar_height)
            continue;   /* totally out of screen */
        for (x=game.played_cave->x1, xd=0; x<=game.played_cave->x2; x++, xd++) {
            if (force_draw || (game.gfx_buffer(x, y) & GD_REDRAW)) {    /* if it needs to be redrawn */
                // calculate on-screen coordinates
                int xs=xplus + xd*cell_size-scroll_x;
                if (xs>=play_area_w || xs+cell_size<0)
                    continue;   /* totally out of screen */
                int dr = game.gfx_buffer(x, y) & ~GD_REDRAW;
                screen.blit(cells.cell(dr), xs, ys);
                game.gfx_buffer(x, y) = dr;   /* now that we drew it */
            }
        }
    }
    
    /* now draw the particles */
    if (gd_particle_effects) {
        int xs=xplus - scroll_x;
        int ys=yplus + statusbar_height - scroll_y_aligned;
        std::list<ParticleSet>::const_iterator it;
        for (it = game.played_cave->particles.begin(); it != game.played_cave->particles.end(); ++it)
            screen.draw_particle_set(xs, ys, *it);
        /* and remember to redraw the whole cave */
        for (y=game.played_cave->y1; y<=game.played_cave->y2; y++)
            for (x=game.played_cave->x1; x<=game.played_cave->x2; x++)
                game.gfx_buffer(x, y) |= GD_REDRAW;
    }

    /* restore clipping to whole screen */
    screen.remove_clip_rect();
}

void GameRenderer::select_status_bar_colors() {
    GdColor (*color_indexer) (unsigned i);
    /* first, count the number of c64 colors the cave uses. */
    /* if it uses mostly c64 colors, we will use c64 colors for the status bar. */
    /* otherwise we will use gdash colors. */
    /* note that the atari original status bar color setting only uses the game colors. */
    int c64_col=0;
    if (game.played_cave->color0.is_c64()) c64_col++;
    if (game.played_cave->color1.is_c64()) c64_col++;
    if (game.played_cave->color2.is_c64()) c64_col++;
    if (game.played_cave->color3.is_c64()) c64_col++;
    if (game.played_cave->color4.is_c64()) c64_col++;
    if (game.played_cave->color5.is_c64()) c64_col++;
    if (c64_col>4)
        color_indexer=GdColor::from_c64;
    else
        color_indexer=GdColor::from_gdash_index;

    switch (gd_status_bar_colors) {
        case GD_STATUS_BAR_ORIGINAL:
            cols.background=color_indexer(GD_COLOR_INDEX_BLACK);
            cols.diamond_needed=color_indexer(GD_COLOR_INDEX_YELLOW);
            cols.diamond_collected=color_indexer(GD_COLOR_INDEX_YELLOW);
            cols.diamond_value=color_indexer(GD_COLOR_INDEX_WHITE);
            cols.score=color_indexer(GD_COLOR_INDEX_WHITE);
            cols.default_color=color_indexer(GD_COLOR_INDEX_WHITE);
            break;
        case GD_STATUS_BAR_1STB:
            cols.background=color_indexer(GD_COLOR_INDEX_BLACK);
            cols.diamond_needed=color_indexer(GD_COLOR_INDEX_YELLOW);
            cols.diamond_collected=color_indexer(GD_COLOR_INDEX_YELLOW);
            cols.score=color_indexer(GD_COLOR_INDEX_YELLOW);
            cols.diamond_value=color_indexer(GD_COLOR_INDEX_WHITE);
            cols.default_color=color_indexer(GD_COLOR_INDEX_WHITE);
            break;
        case GD_STATUS_BAR_CRLI:
            cols.background=color_indexer(GD_COLOR_INDEX_BLACK);
            cols.diamond_needed=color_indexer(GD_COLOR_INDEX_RED);
            cols.diamond_collected=color_indexer(GD_COLOR_INDEX_GREEN);
            cols.diamond_value=color_indexer(GD_COLOR_INDEX_CYAN);
            cols.score=color_indexer(GD_COLOR_INDEX_YELLOW);
            cols.default_color=color_indexer(GD_COLOR_INDEX_WHITE);
            break;
        case GD_STATUS_BAR_FINAL:
            cols.background=color_indexer(GD_COLOR_INDEX_BLACK);
            cols.diamond_needed=color_indexer(GD_COLOR_INDEX_RED);
            cols.diamond_collected=color_indexer(GD_COLOR_INDEX_GREEN);
            cols.diamond_value=color_indexer(GD_COLOR_INDEX_WHITE);
            cols.score=color_indexer(GD_COLOR_INDEX_WHITE);
            cols.default_color=color_indexer(GD_COLOR_INDEX_WHITE);
            break;
        case GD_STATUS_BAR_ATARI_ORIGINAL:
            cols.background=game.played_cave->color0;
            cols.diamond_needed=game.played_cave->color2;
            cols.diamond_collected=game.played_cave->color2;
            cols.diamond_value=game.played_cave->color3;
            cols.score=game.played_cave->color3;
            cols.default_color=game.played_cave->color3;
            break;
        default:
            g_assert_not_reached();
    }
}


void GameRenderer::set_random_colors() {
    if (game.played_cave==NULL)
        return;
    gd_cave_set_random_colors(*game.played_cave, GdColor::Type(gd_preferred_palette));
    set_colors_from_cave();
    redraw();
}


void GameRenderer::clear_header() {
    screen.fill_rect(0, 0, screen.get_width(), statusbar_height, cols.background);
}


bool GameRenderer::showheader_firstline(bool in_game) {
    bool first_line=false;  /* will be set to true, if we draw in the next few code lines. so the y coordinate of the second status line can be decided. */

    /* if playing a replay, tell the user! */
    switch (game.type) {
        case GameControl::TYPE_REPLAY:
            if (show_replay_sign) {
                // TRANSLATORS: the translated string must be at most 20 characters long
                font_manager.blittext(screen, -1, statusbar_y1, GD_GDASH_YELLOW, _("PLAYING REPLAY"));
                first_line=true;
            }
            else if (gd_show_name_of_game && !in_game) {
                /* if showing the name of the cave... */
                int len=g_utf8_strlen(game.caveset->name.c_str(), -1);
                if (screen.get_width()/font_manager.get_font_width_wide()>=len) /* if have place for double-width font */
                    font_manager.blittext(screen, -1, statusbar_y1, cols.default_color, game.caveset->name.c_str());
                else
                    font_manager.blittext_n(screen, -1, statusbar_y1, cols.default_color, game.caveset->name.c_str());
                first_line=true;
            }
            break;
        case GameControl::TYPE_CONTINUE_REPLAY:
            if (show_replay_sign) {
                // TRANSLATORS: the translated string must be at most 20 characters long
                font_manager.blittext(screen, -1, statusbar_y1, GD_GDASH_YELLOW, _("CONTINUING REPLAY"));
                first_line=true;
            }
            break;
        case GameControl::TYPE_SNAPSHOT:
            if (show_replay_sign) {
                // TRANSLATORS: the translated string must be at most 20 characters long
                font_manager.blittext(screen, -1, statusbar_y1, GD_GDASH_YELLOW, _("PLAYING SNAPSHOT"));
                first_line=true;
            }
            break;
        case GameControl::TYPE_TEST:
            if (show_replay_sign) {
                // TRANSLATORS: the translated string must be at most 20 characters long
                font_manager.blittext(screen, -1, statusbar_y1, GD_GDASH_YELLOW, _("TESTING CAVE"));
                first_line=true;
            }
            break;
        case GameControl::TYPE_NORMAL:
            /* normal game - but if not really playing */
            if (!in_game) {
                /* also inform about intermission, but not if playing a replay. also the replay saver should not show it! f */
                if (game.played_cave->intermission) {
                    // TRANSLATORS: the translated string must be at most 20 characters long
                    font_manager.blittext(screen, -1, statusbar_y1, cols.default_color, _("ONE LIFE EXTRA"));
                    first_line=true;
                }
                else
                if (gd_show_name_of_game) {
                        /* if not an intermission, we may show the name of the game (caveset) */
                        /* if showing the name of the cave... */
                        int len=g_utf8_strlen(game.caveset->name.c_str(), -1);
                        if (screen.get_width()/font_manager.get_font_width_wide()>=len) /* if have place for double-width font */
                        font_manager.blittext(screen, -1, statusbar_y1, cols.default_color, game.caveset->name.c_str());
                        else
                            font_manager.blittext_n(screen, -1, statusbar_y1, cols.default_color, game.caveset->name.c_str());
                        first_line=true;
                    }
                break;
            }
    }

    return first_line;
}


void GameRenderer::showheader_uncover() {
    clear_header();

    bool first_line=showheader_firstline(false);

    int cavename_y=first_line?statusbar_y2:statusbar_mid;
    /* "xy players, cave ab/3" */
    char *str;
    if (game.type==GameControl::TYPE_NORMAL)
        str=g_strdup_printf("%d%c, %s/%d", game.player_lives, GD_PLAYER_CHAR, game.played_cave->name.c_str(), int(game.played_cave->rendered_on+1));
    else
        /* if not a normal game, do not show number of remaining lives */
        str=g_strdup_printf("%s/%d", game.played_cave->name.c_str(), int(game.played_cave->rendered_on+1));
    int len=g_utf8_strlen(str, -1);
    if (screen.get_width()/font_manager.get_font_width_wide()>=len) /* if have place for double-width font */
        font_manager.blittext(screen, -1, cavename_y, cols.default_color, str);
    else
        font_manager.blittext_n(screen, -1, cavename_y, cols.default_color, str);
    g_free(str);
}


static char gravity_char(GdDirectionEnum dir) {
    switch(dir) {
        case MV_DOWN: return GD_DOWN_CHAR;
        case MV_LEFT: return GD_LEFT_CHAR;
        case MV_RIGHT: return GD_RIGHT_CHAR;
        case MV_UP: return GD_UP_CHAR;
        default: return '?';
    }
}


void GameRenderer::showheader_game(bool alternate_status_bar, bool fast_forward) {
    if (!game.game_header() || game.played_cave->player_state==GD_PL_NOT_YET) {
        showheader_uncover();
        return;
    }

    clear_header();

    if (game.played_cave->player_state==GD_PL_TIMEOUT && statusbar_since/50%4==0) {
        // TRANSLATORS: the translated string must be at most 20 characters long
        font_manager.blittext(screen, -1, statusbar_mid, GD_GDASH_WHITE, _("OUT OF TIME"));
        return;
    }

    /* y position of status bar */
    bool first_line=showheader_firstline(true);

    int y=first_line?statusbar_y2:statusbar_mid;

    if (alternate_status_bar) {
        /* ALTERNATIVE STATUS BAR BY PRESSING SHIFT */
        /* this will output a total of 20 chars */
        int x=(screen.get_width()-20*font_manager.get_font_width_wide())/2;

        x=font_manager.blittext(screen, x, y, cols.default_color, CPrintf("%c%02d ") % GD_PLAYER_CHAR % gd_clamp(game.player_lives, 0, 99)); /* max 99 in %2d */
        /* color numbers are not the same as key numbers! c3->k1, c2->k2, c1->k3 */
        /* this is how it was implemented in crdr7. */
        x=font_manager.blittext(screen, x, y, game.played_cave->color3, CPrintf("%c%1d ") % GD_KEY_CHAR % gd_clamp(int(game.played_cave->key1), 0, 9));  /* max 9 in %1d */
        x=font_manager.blittext(screen, x, y, game.played_cave->color2, CPrintf("%c%1d ") % GD_KEY_CHAR % gd_clamp(int(game.played_cave->key2), 0, 9));
        x=font_manager.blittext(screen, x, y, game.played_cave->color1, CPrintf("%c%1d ") % GD_KEY_CHAR % gd_clamp(int(game.played_cave->key3), 0, 9));
        if (game.played_cave->gravity_will_change>0) {
            x=font_manager.blittext(screen, x, y, cols.default_color, CPrintf("%c%02d ") % gravity_char(game.played_cave->gravity_next_direction) % gd_clamp(game.played_cave->time_visible(game.played_cave->gravity_will_change), 0, 99));
        } else {
            x=font_manager.blittext(screen, x, y, cols.default_color, CPrintf("%c%02d ") % gravity_char(game.played_cave->gravity) % 0);
        }
        x=font_manager.blittext(screen, x, y, cols.diamond_collected, CPrintf("%c%02d") % GD_SKELETON_CHAR % gd_clamp(int(game.played_cave->skeletons_collected), 0, 99));
    } else {
        int scale=font_manager.get_pixmap_scale();
        /* NORMAL STATUS BAR */
        /* will draw 18 chars (*16 pixels) + 1+10+11+10 pixels inside. */
        /* the two spaces available between scores etc must be divided into
         * three "small" spaces. */
        int x=(screen.get_width()-20*font_manager.get_font_width_wide())/2;
        int time_secs;

        /* cave time is rounded _UP_ to seconds. so at the exact moment when it changes from
           2sec remaining to 1sec remaining, the player has exactly one second. when it changes
           to zero, it is the exact moment of timeout. */
        time_secs=game.played_cave->time_visible(game.played_cave->time);

        x+=1*scale;
        if (fast_forward) {
            /* fast forward mode - show "FAST" */
            x=font_manager.blittext(screen, x, y, cols.default_color, CPrintf("%cFAST%c") % GD_DIAMOND_CHAR % GD_DIAMOND_CHAR);
        } else {
            /* normal speed mode - show diamonds NEEDED <> VALUE */
            /* or if collected enough diamonds,   <><><> VALUE */
            if (game.played_cave->diamonds_needed>game.played_cave->diamonds_collected) {
                if (game.played_cave->diamonds_needed>0)
                    x=font_manager.blittext(screen, x, y, cols.diamond_needed, CPrintf("%03d") % game.played_cave->diamonds_needed);
                else
                    /* did not already count diamonds needed */
                    x=font_manager.blittext(screen, x, y, cols.diamond_needed, CPrintf("%c%c%c") % GD_DIAMOND_CHAR % GD_DIAMOND_CHAR % GD_DIAMOND_CHAR);
            }
            else
                x=font_manager.blittext(screen, x, y, cols.default_color, CPrintf(" %c%c") % GD_DIAMOND_CHAR % GD_DIAMOND_CHAR);
            x=font_manager.blittext(screen, x, y, cols.default_color, CPrintf("%c") % GD_DIAMOND_CHAR);
            x=font_manager.blittext(screen, x, y, cols.diamond_value, CPrintf("%02d") % game.played_cave->diamond_value);
        }
        x+=10*scale;
        x=font_manager.blittext(screen, x, y, cols.diamond_collected, CPrintf("%03d") % game.played_cave->diamonds_collected);
        x+=11*scale;
        x=font_manager.blittext(screen, x, y, cols.default_color, CPrintf("%03d") % time_secs);
        x+=10*scale;
        x=font_manager.blittext(screen, x, y, cols.score, CPrintf("%06d") % game.player_score);
    }
}

void GameRenderer::showheader_pause() {
    clear_header();
    // TRANSLATORS: the translated string must be at most 20 characters long
    font_manager.blittext(screen, -1, statusbar_mid, cols.default_color, _("SPACEBAR TO RESUME"));
}

void GameRenderer::showheader_gameover() {
    clear_header();
    // TRANSLATORS: the translated string must be at most 20 characters long.
    // the c64 original had these spaces - you are allowed to do so.
    font_manager.blittext(screen, -1, statusbar_mid, cols.default_color, _("G A M E   O V E R"));
}


bool old_particle(ParticleSet const &ps) {
    return ps.life < 0;
}


void GameRenderer::set_colors_from_cave() {
    /* select colors, prepare drawing etc. */
    cells.select_pixbuf_colors(game.played_cave->color0, game.played_cave->color1, game.played_cave->color2, game.played_cave->color3, game.played_cave->color4, game.played_cave->color5);
    /* select status bar colors here, as some depend on actual cave colors */
    select_status_bar_colors();
    
    game.played_cave->dirt_particle_color = average_nonblack_colors_in_pixbuf(cells.cell_pixbuf(abs(gd_element_properties[O_DIRT].image_simple)));
    game.played_cave->stone_particle_color = average_nonblack_colors_in_pixbuf(cells.cell_pixbuf(abs(gd_element_properties[O_STONE].image_simple)));
    game.played_cave->diamond_particle_color = lightest_color_in_pixbuf(cells.cell_pixbuf(abs(gd_element_properties[O_DIAMOND].image_simple)));
    game.played_cave->explosion_particle_color = lightest_color_in_pixbuf(cells.cell_pixbuf(abs(gd_element_properties[O_EXPLODE_1].image_simple)));
    game.played_cave->magic_wall_particle_color = average_nonblack_colors_in_pixbuf(cells.cell_pixbuf(abs(gd_element_properties[O_MAGIC_WALL].image_simple)));
}


GameRenderer::State GameRenderer::main_int(int ms, GdDirectionEnum player_move, bool fire, bool &suicide, bool &restart, bool paused, bool alternate_status, bool fast_forward) {
    screen.start_drawing();

    /* tell the interrupt "x ms has passed" */
    GameControl::State state=game.main_int(ms, player_move, fire, suicide, restart, !paused && !out_of_window, fast_forward);
    /* always render the cave to the gfx buffer; however it may do nothing if animcycle was not changed. */
    if (!game.gfx_buffer.empty())
        game.played_cave->draw_indexes(game.gfx_buffer, game.covered, game.bonus_life_flash > 0, game.animcycle, gd_no_invisible_outbox);

    statusbar_since++;
    /* state of game, returned by gd_game_main_int */
    switch (state) {
        case GameControl::STATE_CAVE_LOADED:
            set_colors_from_cave();
            /* check the screen size, and scroll to origin */
            play_area_w=screen.get_width();
            statusbar_height=font_manager.get_font_height()*2+1*font_manager.get_pixmap_scale();
            play_area_h=screen.get_height()-statusbar_height;
            statusbar_y1=0;
            statusbar_y2=font_manager.get_font_height();
            statusbar_mid=(statusbar_height-font_manager.get_font_height())/2;
            scroll_to_origin();
            screen.set_title(CPrintf("GDash - %s/%d") % game.played_cave->name % (game.played_cave->rendered_on+1));
            break;

        case GameControl::STATE_SHOW_STORY:
            toggle=false;
            wrapped_story = gd_wrap_text(game.played_cave->story.c_str(), screen.get_width()/font_manager.get_font_width_narrow()-4);
            story_y=0;
            linesavailable=screen.get_height()/font_manager.get_line_height()-6;
            if (wrapped_story.size() < linesavailable)
                story_max_y=0;
            else
                story_max_y=wrapped_story.size()-linesavailable;

            drawstory();
            break;

        case GameControl::STATE_SHOW_STORY_WAIT:
            toggle=!toggle;
            if (toggle || ms==40) {
                bool changed=false;
                if (player_move==MV_DOWN && story_y<story_max_y) {
                    story_y++;
                    changed=true;
                }
                if (player_move==MV_UP && story_y>0) {
                    story_y--;
                    changed=true;
                }
                if (changed)
                    drawstory();
            }
            break;

        case GameControl::STATE_PREPARE_FIRST_FRAME:
            wrapped_story.clear();
            delete story_background;
            story_background = NULL;
            statusbar_since=0;
            suicide=false;  /* clear detected keypresses, so we do not "remember" them from previous cave runs */
            restart=false;
            break;

        case GameControl::STATE_FIRST_FRAME:
            screen.fill(cols.background); /* fill screen with status bar background color - cave might be smaller than the previous one! */
            showheader_game(alternate_status, fast_forward);
            break;

        case GameControl::STATE_NOTHING:
            /* normally continue. */
            break;

        case GameControl::STATE_LABELS_CHANGED:
            showheader_game(alternate_status, fast_forward);
            suicide=false;  /* clear detected keypresses, as cave was iterated and they were processed */
            break;

        case GameControl::STATE_TIMEOUT_NOW:
            statusbar_since=0;
            showheader_game(alternate_status, fast_forward);    /* also update the status bar here. */
            suicide=false;  /* clear detected keypresses, as cave was iterated and they were processed */
            break;

        case GameControl::STATE_NO_MORE_LIVES:
            showheader_gameover();
            break;

        case GameControl::STATE_STOP:
            /* not different here from game_over, but the difference is important for the ui. */
            break;

        case GameControl::STATE_GAME_OVER:
            /* not different here from game_over, but the difference is important for the ui. */
            break;
    }

    /* move the particles */
    std::list<ParticleSet>::iterator it;
    for (it = game.played_cave->particles.begin(); it != game.played_cave->particles.end(); ++it) {
        if (it->is_new)
            it->normalize(cells.get_cell_size());
        it->move(ms);
    }
    game.played_cave->particles.remove_if(old_particle);

    /* it seems nicer if we first scroll, and then draw. */
    /* scrolling will merely invalidate the whole gfx buffer. */
    /* if drawcave was before scrolling, it would draw, scroll would invalidate, and then it should be drawn again */
    /* only do the drawing if the cave already exists. */
    if (!game.gfx_buffer.empty()) {
        /* if fine scrolling, scroll at 50hz. if not, only scroll at every second call, so 25hz. */
        if (game.played_cave)
            out_of_window = scroll(ms, game.played_cave->player_state==GD_PL_NOT_YET);    /* do the scrolling. scroll exactly, if player is not yet alive */

        drawcave(false);    /* draw the cave. */

        /* may show pause header. but only if the cave already exists in a gfx buffer - or else we are seeing a story at the moment */
        if (paused) {
            if (statusbar_since/50%4==0)
                showheader_pause();
            else
                showheader_game(alternate_status, fast_forward);    /* true = show "playing replay" if necessary */
        }
    }
    
    screen.flip();

    switch (state) {
        case GameControl::STATE_CAVE_LOADED:
            return CaveLoaded;
        case GameControl::STATE_LABELS_CHANGED:
            return Iterated;
        case GameControl::STATE_STOP:
            return Stop;
        case GameControl::STATE_GAME_OVER:
            return GameOver;
        default:
            return Nothing;
    }
}

void GameRenderer::drawstory() {
    // create dark background
    if (story_background == NULL) {
        // create the pixbuf for it
        int w = screen.get_width() / cells.pixbuf_factory.get_pixmap_scale(),
            h = screen.get_height() / cells.pixbuf_factory.get_pixmap_scale();
        Pixbuf *background_pixbuf = cells.pixbuf_factory.create(w, h);
        GdElementEnum bgcells[8] = { O_STONE, O_DIAMOND, O_BRICK, O_DIRT, O_SPACE, O_SPACE, O_DIRT, O_SPACE };
        int cs = cells.get_cell_pixbuf_size();
        for (int y=0; y<h; y+=cs)
            for (int x=0; x<w; x+=cs)
                cells.cell_pixbuf(abs(gd_element_properties[bgcells[rand()%8]].image_game)).copy(*background_pixbuf, x, y);
        Pixbuf *dark_background_pixbuf = cells.pixbuf_factory.create_composite_color(*background_pixbuf, GdColor::from_rgb(0, 0, 0), 256*4/5);
        delete background_pixbuf;
        // this one should be the size of the screen again
        story_background = cells.pixbuf_factory.create_pixmap_from_pixbuf(*dark_background_pixbuf, false);
    }
    screen.blit(*story_background, 0, 0);

    // title line, status line
    font_manager.blittext_n(screen, -1, 0, GD_GDASH_GRAY2, game.played_cave->name.c_str());
    // TRANSLATORS: the translated string must be at most 40 characters long
    font_manager.blittext_n(screen, -1, screen.get_height()-font_manager.get_font_height(), GD_GDASH_GRAY2, _("UP, DOWN: MOVE    FIRE: CONTINUE"));

    // text
    for (unsigned l=0; l<linesavailable && story_y+l<wrapped_story.size(); ++l)
        font_manager.blittext_n(screen, font_manager.get_font_width_narrow()*2,
            l*font_manager.get_line_height()+font_manager.get_line_height()*3, GD_GDASH_WHITE, wrapped_story[story_y+l].c_str());

    // up & down arrow
    if (story_y<story_max_y)
        font_manager.blittext_n(screen, screen.get_width()-font_manager.get_font_width_narrow(),
            screen.get_height()-3*font_manager.get_line_height(), GD_GDASH_GRAY2, CPrintf("%c") % GD_DOWN_CHAR);
    if (story_y>0)
        font_manager.blittext_n(screen, screen.get_width()-font_manager.get_font_width_narrow(),
            font_manager.get_line_height()*2, GD_GDASH_GRAY2, CPrintf("%c") % GD_UP_CHAR);
}


void GameRenderer::redraw() {
    // if cave exists and colors are selected, it means that the cave was drawn
    if (game.played_cave && !game.gfx_buffer.empty()) {
        screen.fill(cols.background); /* fill screen with status bar background color - cave might be smaller than previous! */
        showheader_game(false, false);  /* just some hack values - they will be redrawn normally soon */
        drawcave(true);
    }
    // if story is not empty, redraw that
    else if (!wrapped_story.empty()) {
        drawstory();
    }
}
