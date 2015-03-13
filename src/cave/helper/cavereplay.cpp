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

#include "config.h"

#include <glib.h>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cstring>

#include "cave/helper/cavereplay.hpp"
#include "cave/cavebase.hpp"


/* entries. */
/* type given for each element */
PropertyDescription const CaveReplay::descriptor[] = {
    /* default data */
    {"Level", GD_TYPE_INT, GD_ALWAYS_SAVE, NULL, GetterBase::create_new(&CaveReplay::level)},
    {"RandomSeed", GD_TYPE_INT, GD_ALWAYS_SAVE, NULL, GetterBase::create_new(&CaveReplay::seed)},
    {"Player", GD_TYPE_STRING, 0, NULL, GetterBase::create_new(&CaveReplay::player_name)},
    {"Date", GD_TYPE_STRING, 0, NULL, GetterBase::create_new(&CaveReplay::date)},
    {"Comment", GD_TYPE_STRING, 0, NULL, GetterBase::create_new(&CaveReplay::comment)},
    {"RecordedWith", GD_TYPE_STRING, 0, NULL, GetterBase::create_new(&CaveReplay::recorded_with)},
    {"Score", GD_TYPE_INT, 0, NULL, GetterBase::create_new(&CaveReplay::score)},
    {"Duration", GD_TYPE_INT, 0, NULL, GetterBase::create_new(&CaveReplay::duration)},
    {"Success", GD_TYPE_BOOLEAN, 0, NULL, GetterBase::create_new(&CaveReplay::success)},
    {"CheckSum", GD_TYPE_INT, 0, NULL, GetterBase::create_new(&CaveReplay::checksum)},
    {NULL}  /* end of array */
};

CaveReplay::CaveReplay() :
    current_playing_pos(0),
    level(1),
    seed(0),
    score(0),
    duration(0),
    success(false),
    checksum(0),
    wrong_checksum(false),
    saved(false) {
}


/* store movement in a replay */
void CaveReplay::store_movement(GdDirectionEnum player_move, bool player_fire, bool suicide) {
    g_assert(player_move == (player_move & REPLAY_MOVE_MASK));
    movements.push_back((player_move) | (player_fire ? REPLAY_FIRE_MASK : 0) | (suicide ? REPLAY_SUICIDE_MASK : 0));
}

/* get next available movement from a replay; store variables to player_move, player_fire, suicide */
/* return true if successful */
bool CaveReplay::get_next_movement(GdDirectionEnum &player_move, bool &player_fire, bool &suicide) {
    /* if no more available movements */
    if (current_playing_pos >= movements.size())
        return false;

    movement data = movements[current_playing_pos++];

    suicide = (data & REPLAY_SUICIDE_MASK) != 0;
    player_fire = (data & REPLAY_FIRE_MASK) != 0;
    player_move = (GdDirectionEnum)(data & REPLAY_MOVE_MASK);

    return true;
}

void CaveReplay::rewind() {
    current_playing_pos = 0;
}


#define REPLAY_BDCFF_UP "u"
#define REPLAY_BDCFF_UP_RIGHT "ur"
#define REPLAY_BDCFF_RIGHT "r"
#define REPLAY_BDCFF_DOWN_RIGHT "dr"
#define REPLAY_BDCFF_DOWN "d"
#define REPLAY_BDCFF_DOWN_LEFT "dl"
#define REPLAY_BDCFF_LEFT "l"
#define REPLAY_BDCFF_UP_LEFT "ul"
/* when not moving */
#define REPLAY_BDCFF_STILL "."
/* when the fire is pressed */
#define REPLAY_BDCFF_FIRE "F"
#define REPLAY_BDCFF_SUICIDE "k"

bool CaveReplay::load_one_from_bdcff(const std::string &str) {
    GdDirectionEnum dir;
    bool up, down, left, right;
    bool fire, suicide;
    int num = -1;
    unsigned int count, i;

    fire = suicide = up = down = left = right = false;
    for (i = 0; i < str.length(); i++)
        switch (str[i]) {
            case 'U':
                fire = true;
            case 'u':
                up = true;
                break;

            case 'D':
                fire = true;
            case 'd':
                down = true;
                break;

            case 'L':
                fire = true;
            case 'l':
                left = true;
                break;

            case 'R':
                fire = true;
            case 'r':
                right = true;
                break;

            case 'F':
                fire = true;
                break;

            case 'k':
                suicide = true;
                break;

            case '.':
                /* do nothing, as all other movements are false */
                break;

            case 'c':
            case 'C':
                /* bdcff 'combined' flags. do nothing. */
                break;

            default:
                if (g_ascii_isdigit(str[i])) {
                    if (num == -1)
                        sscanf(str.c_str() + i, "%d", &num);
                }
                break;
        }
    dir = gd_direction_from_keypress(up, down, left, right);
    count = 1;
    if (num != -1)
        count = num;
    for (i = 0; i < count; i++)
        store_movement(dir, fire, suicide);

    return true;
}

bool CaveReplay::load_from_bdcff(std::string const &str) {
    std::istringstream is(str);
    std::string one;
    bool result = true;
    while (is >> one)
        result = result && load_one_from_bdcff(one);

    return result;
}


const char *CaveReplay::direction_to_bdcff(GdDirectionEnum mov) {
    switch (mov) {
            /* not moving */
        case MV_STILL:
            return REPLAY_BDCFF_STILL;
            /* directions */
        case MV_UP:
            return REPLAY_BDCFF_UP;
        case MV_UP_RIGHT:
            return REPLAY_BDCFF_UP_RIGHT;
        case MV_RIGHT:
            return REPLAY_BDCFF_RIGHT;
        case MV_DOWN_RIGHT:
            return REPLAY_BDCFF_DOWN_RIGHT;
        case MV_DOWN:
            return REPLAY_BDCFF_DOWN;
        case MV_DOWN_LEFT:
            return REPLAY_BDCFF_DOWN_LEFT;
        case MV_LEFT:
            return REPLAY_BDCFF_LEFT;
        case MV_UP_LEFT:
            return REPLAY_BDCFF_UP_LEFT;
        default:
            g_assert_not_reached();    /* programmer error */
            return REPLAY_BDCFF_STILL;
    }
}

/* same as above; pressing fire will be a capital letter. */
const char *CaveReplay::direction_fire_to_bdcff(GdDirectionEnum dir, bool fire) {
    static char mov[10];

    strcpy(mov, direction_to_bdcff(dir));
    if (fire) {
        /* uppercase all letters */
        for (int i = 0; mov[i] != 0; i++)
            mov[i] = g_ascii_toupper(mov[i]);
    }

    return mov;
}

std::string CaveReplay::movements_to_bdcff() const {
    std::string str;

    for (unsigned pos = 0; pos < movements.size(); pos++) {
        int num = 1;
        movement data;

        /* if this is not the first movement, append a space. */
        if (!str.empty())
            str += ' ';

        /* if same byte appears, count number of occurrences - something like an rle compression. */
        /* be sure not to cross the array boundaries */
        while (pos < movements.size() - 1 && movements[pos] == movements[pos + 1]) {
            pos++;
            num++;
        }
        data = movements[pos];
        if (data & REPLAY_SUICIDE_MASK)
            str += REPLAY_BDCFF_SUICIDE;
        str += direction_fire_to_bdcff(GdDirectionEnum(data & REPLAY_MOVE_MASK), (data & REPLAY_FIRE_MASK) != 0);
        if (num != 1) {
            std::ostringstream s;

            s << num;
            str += s.str();
        }
    }

    return str;
}
