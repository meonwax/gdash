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

#include <algorithm>
#include "gfx/screen.hpp"

void Screen::set_size(int w, int h, bool fullscreen) {
    if (w != this->w || h != this->h || fullscreen != this->fullscreen) {
        /* notify pixmap storages before the actual change */
        for (std::vector<PixmapStorage *>::const_iterator it = pixmap_storages.begin(); it != pixmap_storages.end(); ++it)
            (*it)->release_pixmaps();
        /* then change the resolution */
        this->w = w;
        this->h = h;
        this->fullscreen = fullscreen;
        configure_size();
    }
}


void Screen::register_pixmap_storage(PixmapStorage *ps) {
    unregister_pixmap_storage(ps);
    pixmap_storages.push_back(ps);
}


void Screen::unregister_pixmap_storage(PixmapStorage *ps) {
    pixmap_storages.erase(std::remove(pixmap_storages.begin(), pixmap_storages.end(), ps), pixmap_storages.end());
}


bool Screen::must_redraw_all_before_flip() {
    return false;
}

void Screen::flip() {
}


void Screen::draw_particle_set(int dx, int dy, ParticleSet const &ps) {
}
