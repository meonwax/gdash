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
#ifndef _GD_EDITOR_AUTOWIDGETS
#define _GD_EDITOR_AUTOWIDGETS

#include "config.h"

#include <gtk/gtkwidget.h>

class Reflective;
struct PropertyDescription;

class EditorAutoUpdate {
private:
    EditorAutoUpdate(EditorAutoUpdate const &);     // deliberately not implemented
    void operator=(EditorAutoUpdate const &);       // deliberately not implemented
public:
    Reflective *r;                      ///< A reflective object to work on
    Reflective *def;                    ///< A reflective object, if any, which stores default value
    PropertyDescription const *descr;   ///< A pointer to the description of the property this is working on
    GtkWidget *widget;                  ///< A widget to show on the screen. May not be the widget which stores data!
    bool expand_vertically;             ///< For longstrings - fill window with widget.

    void update_cave() const;
    void reload() const;
    EditorAutoUpdate(Reflective *r, Reflective *def, PropertyDescription const *descr, void (*cave_update_cb)());

private:
    void (*cave_update_cb)();           ///< A function to call when update happens
    void (*reload_cb)(GtkWidget *);        ///< A function to call for the widget when values have to be reloaded from the Reflective object
};

#endif
