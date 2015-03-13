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
#ifndef _GD_CAVESTORED
#define _GD_CAVESTORED

#include "config.h"

#include <list>
#include "cave/cavebase.hpp"
#include "cave/helper/cavehighscore.hpp"
#include "cave/helper/reflective.hpp"
#include "cave/helper/cavereplay.hpp"
#include "cave/object/caveobject.hpp"
#include "cave/helper/adoptingcontainer.hpp"

#define GD_TITLE_SCREEN_MAX_WIDTH 320
#define GD_TITLE_SCREEN_MAX_HEIGHT 192
#define GD_TITLE_SCROLL_MAX_WIDTH 320
#define GD_TITLE_SCROLL_MAX_HEIGHT 32

typedef AdoptingContainer<CaveObject> CaveObjectStore;

/// @ingroup Cave
/// A cave which is stored in the caveset and on the disk.
/// From this, a CaveRendered is created during the game. This one usually stores objects and different levels;
/// whereas a CaveRendered stores only a map, and can be used only on one level.
/// This has a highscore list, replays, and all other stuff, which is not needed during the game.
/// It derives from the CaveBase class (to have the properties which are also used during the game).
/// For loading and saving, it has all its own properties, and the properties from the base class in its
/// descriptor table and enums.
class CaveStored : public CaveBase, public Reflective {
public:
    CaveStored();

    /// This is for the adopting container.
    CaveStored *clone() const {
        return new CaveStored(*this);
    }

    /* reflective class function reimplementations */
    virtual PropertyDescription const *get_description_array() const {
        return descriptor;
    }
private:
    static PropertyDescription const descriptor[];

public:
    static PropertyDescription const color_dialog[];
    static PropertyDescription const random_dialog[];
    // Player's data
    HighScoreTable highscore;                   ///< Highscore table
    std::list<CaveReplay> replays;              ///< List of replays (demos) to this cave

    // Cave elements data - map + objects
    CaveMap<GdElementEnum> map;                   ///< cave map
    CaveObjectStore objects;                    ///< Stores cave drawing objects

    void set_gdash_defaults();

    /// For convenience - add object to cave.
    void push_back_adopt(CaveObject *x) {
        objects.push_back_adopt(x);
    }

    /* cave properties */
    GdString charset;                       ///< for compatibility; not used by gdash.
    GdString fontset;                       ///< for compatibility; not used by gdash.

    GdBool selectable;                      ///< is this selectable as an initial cave for a game?

    /* initial random fill */
    GdIntLevels level_rand;                 ///< Random seed.
    GdElement initial_fill;                 ///< cave filled initially with this element (if not overwritten by random_fill[x])
    GdElement initial_border;               ///< border around cave
    GdElement random_fill_1;                ///< Random fill element 1
    GdInt random_fill_probability_1;        ///< 0..255 "probability" of random fill element 1
    GdElement random_fill_2;                ///< Random fill element 2
    GdInt random_fill_probability_2;        ///< 0..255 "probability" of random fill element 2
    GdElement random_fill_3;                ///< Random fill element 3
    GdInt random_fill_probability_3;        ///< 0..255 "probability" of random fill element 3
    GdElement random_fill_4;                ///< Random fill element 4
    GdInt random_fill_probability_4;        ///< 0..255 "probability" of random fill element 4

    /* properties */
    GdIntLevels level_diamonds;                ///< Must collect diamonds, on level x
    GdIntLevels level_speed;                   ///< Time between game cycles in ms
    GdIntLevels level_ckdelay;                 ///< Timing in original game units
    GdIntLevels level_time;                    ///< Available time, per level
    GdIntLevels level_timevalue;               ///< points for each second remaining, when exiting level
    GdIntLevels level_magic_wall_time;         ///< magic wall 'on' state for each level (seconds)
    GdIntLevels level_amoeba_time;             ///< amoeba time for each level
    GdIntLevels level_amoeba_threshold;        ///< amoeba turns to stones; if count is bigger than this (number of cells)
    GdIntLevels level_amoeba_2_time;           ///< amoeba time for each level
    GdIntLevels level_amoeba_2_threshold;      ///< amoeba turns to stones; if count is bigger than this (number of cells)
    GdProbabilityLevels level_slime_permeability;      ///< true random slime
    GdIntLevels level_slime_permeability_c64;  ///< Appearing in bd 2
    GdIntLevels level_slime_seed_c64;          ///< predictable slime random seed
    GdIntLevels level_bonus_time;              ///< bonus time for clock collected.
    GdIntLevels level_penalty_time;            ///< Time penalty when voodoo destroyed.
    GdIntLevels level_hatching_delay_frame;    ///< Scan frames before Player's birth.
    GdIntLevels level_hatching_delay_time;     ///< Seconds before Player's birth.

    GdString unknown_tags;                  ///< stores read-but-not-understood strings from bdcff, so we can save them later.
};

#endif
