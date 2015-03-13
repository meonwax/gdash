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

#include <glib/gi18n.h>

#include "about.hpp"

const char *About::license=
    "Permission to use, copy, modify, and/or distribute this software for any "
    "purpose with or without fee is hereby granted, provided that the above "
    "copyright notice and this permission notice appear in all copies."
    "\n\n"
    "THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES "
    "WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF "
    "MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR "
    "ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES "
    "WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN "
    "ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF "
    "OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.";

const char *About::artists[]= { "CWS",
                                0
                              };

const char *About::authors[]= { "Czirkos Zoltan <czirkos.zoltan@gmail.com>",
                                "Scale2x: Andrea Mazzoleni",
                                "hqx: Maxim Stepin, Cameron Zemek",
#ifdef HAVE_SDL
                                "SDL: Sam Lantinga <slouken@libsdl.org>",
                                "SDL png saver: Philip D. Bober <wildfire1138@mchsi.com>",
#endif
                                0
                              };

const char *About::documenters[]= {
    "Original engine docs: LogicDeLuxe",
    "Playing hints: Sendy",
    0
};

const char *About::comments=N_("Classic game similar to Emerald Mines. Collect diamonds and find exit!");

const char *About::translator_credits=N_("translator-credits");

const char *About::copyright="Copyright 2007-2013, GDash Project";

const char *About::website="http://code.google.com/p/gdash/";
