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

#include <glib.h>
#include "cave/helper/caverandom.hpp"

/// Create object; initialize randomly
RandomGenerator::RandomGenerator() {
    rand=g_rand_new();
}

/// Create object.
/// @param seed Random number seed to be used.
RandomGenerator::RandomGenerator(unsigned int seed) {
    rand=g_rand_new_with_seed(seed);
}

/// Standard assignment operator.
RandomGenerator& RandomGenerator::operator=(const RandomGenerator &rhs) {
    /* handle self-assignment */
    if (this==&rhs)
        return *this;
        
    g_rand_free(rand);
    rand=g_rand_copy(rhs.rand);
    
    return *this;
}

/// Copy constructor.
RandomGenerator::RandomGenerator(const RandomGenerator &other) {
    rand=g_rand_copy(other.rand);
}

/// Destructor.
RandomGenerator::~RandomGenerator() {
    g_rand_free(rand);
}

/// Set seed to given number, to generate a series of random numbers.
/// @param seed The seed value.
void RandomGenerator::set_seed(unsigned int seed) {
    g_rand_set_seed(rand, seed);
}

/// Generater a random boolean. 50% false, 50% true.
bool RandomGenerator::rand_boolean() {
    return g_rand_boolean(rand)!=FALSE;
}

/// Generate a random integer, [begin, end).
/// @param begin Start of interval, inclusive.
/// @param end End of interval, non-inclusive.
int RandomGenerator::rand_int_range(int begin, int end) {
    return g_rand_int_range(rand, begin, end);
}

/// Generate a random 32-bit unsigned integer.
unsigned int RandomGenerator::rand_int() {
    return g_rand_int(rand);
}


/// Constructor. Initializes generator to a random series.
C64RandomGenerator::C64RandomGenerator() {
    /* no seed given, but do something sensible */
    set_seed(g_random_int_range(0, 65536));
}

/// Generate random number.
/// @return Random number between 0 and 255.
unsigned int C64RandomGenerator::random() {
    unsigned int temp_rand_1, temp_rand_2, carry, result;

    temp_rand_1=(rand_seed_1&0x0001) << 7;
    temp_rand_2=(rand_seed_2 >> 1)&0x007F;
    result=(rand_seed_2)+((rand_seed_2&0x0001) << 7);
    carry=(result >> 8);
    result=result&0x00FF;
    result=result+carry+0x13;
    carry=(result >> 8);
    rand_seed_2=result&0x00FF;
    result=rand_seed_1+carry+temp_rand_1;
    carry=(result >> 8);
    result=result&0x00FF;
    result=result+carry+temp_rand_2;
    rand_seed_1=result&0x00FF;

    return rand_seed_1;
}

/// Set seed. The same as set_seed(int), but 2*8 bits must be given.
/// @param seed1 First 8 bits of seed value.
/// @param seed2 Second 8 bits of seed value.
void C64RandomGenerator::set_seed(int seed1, int seed2) {
    rand_seed_1=seed1%256;
    rand_seed_2=seed2%256;
}

/// Set seed.
/// @param seed Seed value. Only lower 16 bits will be used.
void C64RandomGenerator::set_seed(int seed) {
    rand_seed_1=seed/256%256;
    rand_seed_2=seed%256;
}


