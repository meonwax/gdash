/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef CAVEHIGHSCORE_HPP_INCLUDED
#define CAVEHIGHSCORE_HPP_INCLUDED

#include "config.h"

#include <string>
#include <vector>

/// A structure which holds a player name and a high score.
/// The HighScoreTable will store these sorted.
struct HighScore {
    std::string name;
    int score;
    HighScore(const std::string &name_, int score_) : name(name_), score(score_) {}
    HighScore() : name(), score(0) {}
};


/// A HighScoreTable for a cave or a caveset.
class HighScoreTable {
private:
    std::vector<HighScore> table;   ///< The table
    enum { GD_HIGHSCORE_NUM = 20 }; ///< Maximum size

public:
    /// Return nth entry
    HighScore &operator[](unsigned n) {
        return table.at(n);
    }
    const HighScore &operator[](unsigned n) const {
        return table.at(n);
    }
    /// Check if the table has at least one entry. @return True, if there is an entry.
    bool has_highscore() {
        return !table.empty();
    }
    bool is_highscore(int score) const;

    int add(const std::string &name, int score);
    void clear() {
        table.clear();
    }
    unsigned size() const {
        return table.size();
    }
};

#endif
