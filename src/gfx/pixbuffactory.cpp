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

#include <glib/gi18n.h>

#include "gfx/pixbuf.hpp"
#include "gfx/pixbuffactory.hpp"
#include "gfx/pixbufmanip.hpp"

/* names of scaling types supported. */
/* scale2x and scale3x are not translated: the license says that we should call it in its original name. */
const char *gd_scaling_name[]={N_("Original"), N_("2x nearest"), "Scale2x", "HQ2x", N_("3x nearest"), "Scale3x", "HQ3x", N_("4x nearest"), "Scale4x", "HQ4x", NULL};
/* scaling factors of scaling types supported. */
const int gd_scaling_scale[]={1, 2, 2, 2, 3, 3, 3, 4, 4, 4};

/* check the arrays on start */
static class _init {
public:
    _init() {
        g_assert(G_N_ELEMENTS(gd_scaling_name)==GD_SCALING_MAX+1);    /* +1 is the terminating NULL */
        g_assert(G_N_ELEMENTS(gd_scaling_scale)==GD_SCALING_MAX);
    }
} _init;

/* scales a pixbuf with the appropriate scaling type. */
Pixbuf *PixbufFactory::create_scaled(const Pixbuf &src) const
{
    int gd_scale=gd_scaling_scale[scaling_type];
    Pixbuf *scaled=this->create(src.get_width()*gd_scale, src.get_height()*gd_scale);

    switch (scaling_type) {
        case GD_SCALING_MAX:
            /* not a valid case, but to avoid compiler warning */
            g_assert_not_reached();
            break;

        case GD_SCALING_ORIGINAL:
            src.copy(*scaled, 0, 0);
            break;
        case GD_SCALING_2X:
            scale2xnearest(src, *scaled);
            break;
        case GD_SCALING_2X_SCALE2X:
            scale2x(src, *scaled);
            break;
        case GD_SCALING_2X_HQ2X:
            hq2x(src, *scaled);
            break;
        case GD_SCALING_3X:
            scale3xnearest(src, *scaled);
            break;
        case GD_SCALING_3X_SCALE3X:
            scale3x(src, *scaled);
            break;
        case GD_SCALING_3X_HQ3X:
            hq3x(src, *scaled);
            break;
        case GD_SCALING_4X:
            {
                Pixbuf *scale2x=this->create(src.get_width()*2, src.get_height()*2);
                scale2xnearest(src, *scale2x);
                scale2xnearest(*scale2x, *scaled);
                delete scale2x;
            }
            break;
        case GD_SCALING_4X_SCALE4X:
            /* scale2x applied twice. */
            {
                Pixbuf *scale2xpb=this->create(src.get_width()*2, src.get_height()*2);
                scale2x(src, *scale2xpb);
                scale2x(*scale2xpb, *scaled);
                delete scale2xpb;
            }
            break;
        case GD_SCALING_4X_HQ4X:
            hq4x(src, *scaled);
            break;
    }
    
    if (pal_emulation)
        pal_emulate(*scaled);

    return scaled;
}

PixbufFactory::PixbufFactory(GdScalingType scaling_type_, bool pal_emulation_)
:
    scaling_type(scaling_type_),
    pal_emulation(pal_emulation_)
{
}

int PixbufFactory::get_pixmap_scale() const
{
    return gd_scaling_scale[scaling_type];
}

void PixbufFactory::set_properties(GdScalingType scaling_type_, bool pal_emulation_)
{
    scaling_type=scaling_type_;
    pal_emulation=pal_emulation_;
}

Pixbuf *PixbufFactory::create_from_base64(const char *base64) const
{
    gsize len;
    guchar *decoded=g_base64_decode(base64, &len);
    // creating the pixbuf might fail, in that case, also free memory
    try {
        Pixbuf *pix=create_from_inline(len, decoded);
        delete decoded;
        return pix;
    } catch (...) {
        delete decoded;
        throw;
    }
}
