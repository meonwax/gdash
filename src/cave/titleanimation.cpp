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

#include "title.cpp"

#include <memory>

#include "misc/printf.hpp"
#include "misc/logger.hpp"
#include "gfx/pixbuf.hpp"
#include "gfx/pixmap.hpp"
#include "gfx/pixbuffactory.hpp"
#include "cave/helper/colors.hpp"
#include "cave/cavetypes.hpp"
#include "cave/titleanimation.hpp"

std::vector<Pixbuf *> get_title_animation_pixbuf(const GdString& title_screen, const GdString& title_screen_scroll, bool one_frame_only, PixbufFactory& pixbuf_factory) {
    typedef std::auto_ptr<Pixbuf> PixbufPtr;

    std::vector<Pixbuf *> animation;

    PixbufPtr screen, tile;
    try {
        if (title_screen!="")
            screen=PixbufPtr(pixbuf_factory.create_from_base64(title_screen.c_str()));
        if (screen.get()!=NULL && screen->has_alpha() && title_screen_scroll!="")
            tile=PixbufPtr(pixbuf_factory.create_from_base64(title_screen_scroll.c_str()));
    } catch (std::exception& e) {
        gd_message(CPrintf("Caveset is storing an invalid title screen image: %s") % e.what());
        return animation;
    }

    if (tile.get()!=NULL && tile->get_height()>40) {
        gd_message("Caveset is storing an oversized tile image");
        tile.release();
    }

    /* if no special title image or unable to load that one, load the built-in */
    if (screen.get()==NULL) {
        /* the screen */
        screen=PixbufPtr(pixbuf_factory.create_from_inline(sizeof(gdash_screen), gdash_screen));
        /* the tile to be put under the screen */
        tile=PixbufPtr(pixbuf_factory.create_from_inline(sizeof(gdash_tile), gdash_tile));
        g_assert(screen.get()!=NULL);
        g_assert(tile.get()!=NULL);
    }

    /* if no tile, let it be black. */
    if (tile.get()==NULL) {
        /* one-row pixbuf, so no animation; totally black. */
        tile=PixbufPtr(pixbuf_factory.create(screen->get_width(), 1));
        tile->fill(GdColor::from_rgb(0,0,0));
    }

    /* do not allow more than 40 frames of animation */
    g_assert(tile->get_height()<40);

    /* create a big image, which is one tile larger than the title image size */
    Pixbuf *bigone=pixbuf_factory.create(screen->get_width(), screen->get_height()+tile->get_height());
    /* and fill it with the tile. use copy(), so pixbuf data is initialized! */
    for (int y=0; y<screen->get_height()+tile->get_height(); y+=tile->get_height())
        for (int x=0; x<screen->get_width(); x+=tile->get_width())
            tile->copy(*bigone, x, y);

    int framenum=one_frame_only ? 1 : tile->get_height();
    for (int i=0; i<framenum; i++) {
        Pixbuf *frame=pixbuf_factory.create(screen->get_width(), screen->get_height());
        // copy part of the big tiled image
        bigone->copy(0, i, screen->get_width(), screen->get_height(), *frame, 0, 0);
        // and composite it with the title image
        screen->blit(*frame, 0, 0);
        // copy to array
        animation.push_back(frame);
    }
    delete bigone;

    return animation;
}

std::vector<Pixmap *> get_title_animation_pixmap(const GdString& title_screen, const GdString& title_screen_scroll, bool one_frame_only, PixbufFactory& pixbuf_factory) {
    std::vector<Pixbuf *> pixbufs;
    pixbufs = get_title_animation_pixbuf(title_screen, title_screen_scroll, one_frame_only, pixbuf_factory);
    if (pixbufs.empty())
        pixbufs=get_title_animation_pixbuf(GdString(), GdString(), one_frame_only, pixbuf_factory);

    std::vector<Pixmap *> pixmaps;
    for (unsigned i=0; i<pixbufs.size(); ++i) {
        pixmaps.push_back(pixbuf_factory.create_pixmap_from_pixbuf(*pixbufs[i], false));
        delete pixbufs[i];
    }

    return pixmaps;
}
