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
#ifndef _GD_pixbufmanipMAP
#define _GD_pixbufmanipMAP

#include "config.h"


/// @ingroup Graphics
/// A class which represents a drawable image.
/// It is different from a pixbuf in the sense that it is already converted to the graphics format of the screen.
class Pixmap {
public:
    virtual int get_width() const=0;
    virtual int get_height() const=0;
    virtual ~Pixmap() {}
};


/// @ingroup Graphics
/// A class which stores rendered Pixmap objects can be notified by a Screen object
/// when the resolution or the video mode change. The Screen object does this by calling
/// the PixmapStorage::release_pixmaps() function of the storage object. The storage object
/// should be able to recreate its Pixmap objects, i.e. it should be able to recover after
/// a resolution change.
class PixmapStorage {
public:
    /// Notifying the PixmapStorage object that its Pixmap objects are to be deleted.
    /// This function is called just before the resolution change.
    virtual void release_pixmaps() = 0;
};

#endif
