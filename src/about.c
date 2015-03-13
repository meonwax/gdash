/*
 * Copyright (c) 2007, 2008, 2009, Czirkos Zoltan <cirix@fw.hu>
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
#include <glib/gi18n.h>
#include "config.h"

const char *gd_about_license=
                "Copyright (c) 2007, 2008, 2009, Czirkos Zoltan <cirix@fw.hu>"
                "\n"
                "Permission to use, copy, modify, and/or distribute this software for any "
                "purpose with or without fee is hereby granted, provided that the above "
                "copyright notice and this permission notice appear in all copies."
                "\n"
                "THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES "
                "WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF "
                "MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR "
                "ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES "
                "WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN "
                "ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF "
                "OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.";


const char *gd_about_artists[]={ "CWS", NULL };
const char *gd_about_authors[]={ "Czirkos Zoltan <cirix@fw.hu>",
                                 "Scale2x: Andrea Mazzoleni",
#ifdef USE_SDL
                                 "SDL: Sam Lantinga <slouken@libsdl.org>",
                                 "SDL_gfx rotozoom: A. Schiffler",
                                 "SDL png saver: Philip D. Bober <wildfire1138@mchsi.com>",
#endif
                                NULL };
const char *gd_about_documenters[]={ "About original engine: LogicDeLuxe", "Playing hints: Sendy", NULL };
const char *gd_about_comments=N_("Classic game similar to Emerald Mines.\nCollect diamonds and find exit!");
const char *gd_about_translator_credits=N_("translator-credits");
const char *gd_about_website="http://jutas.eet.bme.hu/~cirix/gdash";

