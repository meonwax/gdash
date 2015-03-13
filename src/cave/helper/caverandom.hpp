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

#ifndef _GD_CAVE_RANDOM
#define _GD_CAVE_RANDOM

#include "config.h"

#include <glib.h>

/// maximum seed value for the cave random generator.
enum { GD_CAVE_SEED_MAX=65535 };

/**
 * @brief Wraps a GLib random generator to make it a C++ class.
 *
 * This is the main random generator, which is used during
 * playing the cave. The C64 random generator is only used when
 * creating the cave.
 */
class RandomGenerator {
private:
    /// The GRand wrapped - stores the internal state.
    GRand *rand;

public:
    RandomGenerator();
    explicit RandomGenerator(unsigned int seed);
    RandomGenerator(const RandomGenerator &other);
    RandomGenerator &operator=(const RandomGenerator &rhs);
    ~RandomGenerator();

    void set_seed(unsigned int seed);
    bool rand_boolean();
    int rand_int_range(int begin, int end);
    unsigned int rand_int();
};

/**
 * @brief Random number generator, which is compatible with the original game.
 *
 * C64 BD predictable random number generator.
 * Used to load the original caves imported from c64 files.
 * Also by the predictable slime.
 */
class C64RandomGenerator {
private:
    /// Internal state of random number generator.
    int rand_seed_1;
    /// Internal state of random number generator.
    int rand_seed_2;

public:
    C64RandomGenerator();

    void set_seed(int seed);
    void set_seed(int seed1, int seed2);
    unsigned int random();
};


#endif

