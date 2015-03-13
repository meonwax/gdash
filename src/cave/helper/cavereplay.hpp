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
#ifndef _GD_CAVEREPLAY
#define _GD_CAVEREPLAY

#include "config.h"

#include <string>
#include "cave/cavetypes.hpp"
#include "cave/helper/reflective.hpp"

class CaveReplay : public Reflective {
private:
    static const char *direction_to_bdcff(GdDirectionEnum mov);
    static const char *direction_fire_to_bdcff(GdDirectionEnum dir, bool fire);
    bool load_one_from_bdcff(const std::string &str);
    typedef unsigned char movement;
    std::vector<movement> movements;
    unsigned int current_playing_pos;
    enum {
        REPLAY_MOVE_MASK=0x0f,
        REPLAY_FIRE_MASK=0x10,
        REPLAY_SUICIDE_MASK=0x20,
    };

public:
    /* reflective */
    static PropertyDescription const descriptor[];
    virtual PropertyDescription const *get_description_array() const { return descriptor; }

    CaveReplay();

    /* i/o */
    std::string movements_to_bdcff() const;
    bool load_from_bdcff(const std::string& str);
    void store_movement(GdDirectionEnum player_move, bool player_fire, bool suicide);
    bool get_next_movement(GdDirectionEnum &player_move, bool &player_fire, bool &suicide);
    void rewind();
    unsigned int length() { return movements.size(); }

    GdInt level;            ///< replay for level n
    GdInt seed;                ///< seed the cave is to be rendered with
    GdString recorded_with;    ///< application name and version, which was used to create this replay

    GdString player_name;    ///< name of player in this replay
    GdString date;            ///< date when this replay was recorded
    GdString comment;        ///< comments on the replay

    GdInt score;            ///< score collected in this replay
    GdInt duration;            ///< seconds duration of replay
    GdBool success;            ///< true, if the player was successful
    GdInt checksum;            ///< checksum of rendered cave

    GdBool wrong_checksum;    ///< this replay's checksum is calculated, and it came out to be a mismatch with the cave
    GdBool saved;            ///< whether this replay is to be saved in the game
};

#endif    /* _GD_CAVEREPLAY_H */
