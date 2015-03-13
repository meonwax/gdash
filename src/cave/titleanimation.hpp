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

#ifndef GD_TITLEANIMATION
#define GD_TITLEANIMATION

#include "config.h"

#include <vector>
class Pixbuf;
class Pixmap;
class GdString;
class PixbufFactory;

/**
 * Create and return an array of pixbufs, which contain the title animation, or the first frame only.
 * Up to the caller to delete the pixbufs!
 */
std::vector<Pixbuf *> get_title_animation_pixbuf(const GdString& title_screen, const GdString& title_screen_scroll, bool one_frame_only, PixbufFactory& pixbuf_factory);
std::vector<Pixmap *> get_title_animation_pixmap(const GdString& title_screen, const GdString& title_screen_scroll, bool one_frame_only, PixbufFactory& pixbuf_factory);

#endif
