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

#include <glib.h>

#include "cave/particle.hpp"

ParticleSet::ParticleSet(int count, float size, float opacity, float p0x, float p0y, float dp0x, float dp0y, float v0x, float v0y, float dvx, float dvy, const GdColor &color)
    : color(color)
    , life(1000)
    , is_new(true)
    , size(size)
    , opacity(opacity)
    , particles(count) {
    for (size_t i = 0; i < particles.size(); ++i) {
        Particle &p = particles[i];
        p.px = p0x + g_random_double_range(-dp0x, dp0x);
        p.py = p0y + g_random_double_range(-dp0y, dp0y);
        p.vx = v0x + g_random_double_range(-dvx, dvx);
        p.vy = v0y + g_random_double_range(-dvy, dvy);
    }
}


void ParticleSet::move(int dt_ms) {
    float dt = dt_ms/1000.0;

    for (size_t i = 0; i < particles.size(); ++i) {
        Particle &p = particles[i];
        p.px += p.vx * dt;
        p.py += p.vy * dt;
    }

    life -= dt_ms;
}


void ParticleSet::normalize(double factor) {
    if (!is_new)
        return;
    is_new = false;

    size *= factor;
    for (size_t i = 0; i < particles.size(); ++i) {
        Particle &p = particles[i];
        p.px *= factor;
        p.py *= factor;
        p.vx *= factor;
        p.vy *= factor;
    }
}
