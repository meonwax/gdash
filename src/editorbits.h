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

#ifndef _EDITORBITS_H
#define _EDITORBITS_H

#include "caveobject.h"

char *gd_object_get_coordinates_text(GdObject *selected);
char *gd_object_get_description_markup(GdObject *selected);

typedef struct _gd_object_description {
    char *name;
    char *x1;
    char *x2;
    char *dx;
    char *seed;
    char *element;
    char *fill_element;
    char *first_button, *second_button;
    char *horiz;
    char *c64_random;
    char *mirror;
    char *flip;
} GdObjectDescription;

extern GdObjectDescription gd_object_description[];

#endif

