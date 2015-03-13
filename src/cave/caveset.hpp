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
#ifndef _GD_CAVESET
#define _GD_CAVESET

#include "config.h"

#include <stdexcept>
#include "cave/cavetypes.hpp"
#include "cave/helper/cavehighscore.hpp"
#include "cave/caverendered.hpp"
#include "cave/helper/reflective.hpp"
#include "cave/helper/adoptingcontainer.hpp"
#include "cave/cavestored.hpp"

/// @ingroup Cave
class CaveSet : public Reflective {
public:
    CaveSet();

    /* variables */
    GdString name;                  ///< Name of caveset
    GdString description;           ///< Some words about the caveset
    GdString author;                ///< Author
    GdString difficulty;            ///< difficulty of the caveset, for info purposes
    GdString www;                   ///< link to author's webpage
    GdString date;                  ///< date of creation

    GdString story;                 ///< story for the caves
    GdString remark;                ///< notes about the game

    GdString title_screen;          ///< base64-encoded title screen image
    GdString title_screen_scroll;   ///< scrolling background for title screen image

    GdString charset;
    GdString fontset;

    GdInt initial_lives;            ///< initial lives at game start
    GdInt maximum_lives;            ///< maximum lives
    GdInt bonus_life_score;         ///< bonus life / number of points
    HighScoreTable highscore;       ///< highscore table for caveset

    GdBool edited;                  ///< changed since last save
    GdString filename;              ///< Loaded from / save to this file
    GdInt last_selected_cave;       ///< If running a game, the index of the selected gave is stored here
    GdInt last_selected_level;      ///< If running a game, the level of the selected gave is stored here

    AdoptingContainer<CaveStored> caves;

    /* highscore in config directory */
    void save_highscore(const char *directory) const;
    bool load_highscore(const char *directory);

    void save_to_file(const char *filename) throw(std::runtime_error);
    void set_name_from_filename(const char *filename);

    bool has_replays();
    bool has_caves() const {
        return !caves.empty();
    }
    CaveStored &cave(unsigned i) const {
        return *(caves.at(i));
    }
    int cave_index(CaveStored const *cave) const;
    int first_selectable_cave_index() const;

private:
    const char *filename_for_cave_highscores(const char *directory) const;
    unsigned checksum() const;

// for reflective
public:
    static PropertyDescription const descriptor[];
    virtual PropertyDescription const *get_description_array() const {
        return descriptor;
    }
};

extern const char *gd_caveset_extensions[];

#endif                          /* _CAVESET_H */
