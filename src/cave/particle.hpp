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

#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include "config.h"

#include <vector>
#include "cave/helper/colors.hpp"

class ParticleSet {
public:
    /// One single particle.
    struct Particle {
        float px, py;         ///< Coordinate
        float vx, vy;         ///< Speed
    };

    /// This constructor creates a particle set, for the given cave coordinates.
    /// 0,0 is the top left corner of the cave; 1,1 is the bottom right corner of
    /// the top left cave cell. (So the max coordinates are the width and height
    /// of the cave.)
    /// @param cave_x Particle set starting x coordinate in cave coordinates.
    /// @param cave_y Particle set starting y coordinate in cave coordinates.
    /// @param dx Half the width of the region, in which originally particles are randomly generated.
    /// @param dy Half the height of the region, in which originally particles are randomly generated.
    /// @param vx Maximum original speed.
    /// @param vy Maximum original speed.
    ParticleSet(int count, float size, float opacity, float p0x, float p0y, float dp0x, float dp0y, float v0x, float v0y, float dvx, float dvy, const GdColor &color);
    /// Move the particles.
    /// @param dt_ms Time elapsed.
    void move(int dt_ms);
    /// Start using screen cordinates, if is_new is true.
    void normalize(double factor);

    typedef std::vector<Particle>::iterator iterator;
    typedef std::vector<Particle>::const_iterator const_iterator;

    iterator begin() {
        return particles.begin();
    }
    iterator end() {
        return particles.end();
    }
    const_iterator begin() const {
        return particles.begin();
    }
    const_iterator end() const {
        return particles.end();
    }

    GdColor color;
    int life;           ///< lifetime. starts from 1000, goes to 0.
    bool is_new;        ///< New particle set, the coordinates of which must be "normalized" to the cave screen coordinates
    float size;         ///< Size of the particles.
    float opacity;      ///< Opacity between 0 and 1. Values close to 1 not recommended.

private:
    std::vector<Particle> particles;
};

#endif
