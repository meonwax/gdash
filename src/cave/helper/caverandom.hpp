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

#ifndef CAVERANDOM_HPP_INCLUDED
#define CAVERANDOM_HPP_INCLUDED

#include "config.h"

#include <glib.h>

/// maximum seed value for the cave random generator.
enum { GD_CAVE_SEED_MAX = 65535 };

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

