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

#include <algorithm>
#include "cave/helper/cavehighscore.hpp"

/// Check if the achieved score will be put on the list.
/// @param score The score to be checked.
/// @return true, if the score is a highscore, and can be put on the list.
bool HighScoreTable::is_highscore(int score) const {
    /* if score is above zero AND bigger than the last one */
    if (score>0 && (table.size()<GD_HIGHSCORE_NUM || score>table.back().score))
        return true;

    return false;
}

/* for sorting. compares two highscores. */
static bool highscore_compare(const HighScore &a, const HighScore &b) {
    return b.score>a.score;
}

#include <glib.h>
/// Adds a player with some score to the highscore table.
/// Returns the new rank.
/// @param name The name of the player.
/// @param score The score achieved.
/// @return The index in the table, or -1 if did not fit.
int HighScoreTable::add(const std::string &name, int score) {
    if (!is_highscore(score))
        return -1;

    /* add to the end */
    table.push_back(HighScore(name, score));
    sort(table.begin(), table.end(), highscore_compare);
    /* if too big, remove the lowest ones (after sorting) */
    if (table.size()>GD_HIGHSCORE_NUM)
        table.resize(GD_HIGHSCORE_NUM);

    /* and find it so we can return an index */ 
    for (unsigned int i=0; i<table.size(); i++)
        if (table[i].name==name && table[i].score==score)
            return i;

    return -1;
}
