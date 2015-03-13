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
#ifndef _GD_CAVEOBJECT
#define _GD_CAVEOBJECT

#include "config.h"

#include "cave/cavetypes.hpp"
#include "cave/helper/reflective.hpp"

/* forward declarations */
class CaveRendered;

/// Cave object abstract base class, from which all cave objects inherit.
class CaveObject : public Reflective {
public:
    /// The type of the object.
    /// This is an enumeration of all possible object types.
    enum Type {
        GD_POINT,                ///< single point of object1
        GD_LINE,                ///< line from (1) to (2) of object1
        GD_RECTANGLE,            ///< rectangle with corners (1) and (2) of object1
        GD_FILLED_RECTANGLE,    ///< rectangle with corners (1) and (2) of object1, filled with object2
        GD_RASTER,                ///< aligned plots
        GD_JOIN,                ///< every object1 has an object2 next to it, relative (dx,dy)
        GD_FLOODFILL_REPLACE,    ///< fill by replacing
        GD_FLOODFILL_BORDER,    ///< fill to another element, a border
        GD_MAZE,                ///< maze
        GD_MAZE_UNICURSAL,        ///< unicursal maze
        GD_MAZE_BRAID,            ///< braid maze
        GD_RANDOM_FILL,            ///< random fill
        GD_COPY_PASTE,            ///< copy & paste with optional mirror and flip
    };

    ///< The type of the object. Set by the constructor, and cannot be changed.
    ///< Maybe a virtual function should be made, but not worth.
    Type const type;
    /// Levels on which this object is visible on.
    GdBoolLevels seen_on;

    bool is_seen_on_all() const;
    bool is_invisible() const;
    void enable_on_all();
    void disable_on_all();

    /// Virtual destructor.
    virtual ~CaveObject() {}

    /// Clone object - create a newly allocated, exact copy.
    /// This will be the virtual constructor. All derived objects must implement this.
    virtual CaveObject *clone() const=0;

    /// Draw the object on a specified level, in the specified cave. All derived objects implement this.
    /// @param cave The mapped (for game) cave to draw on
    virtual void draw(CaveRendered &cave) const=0;

    /// Get BDCFF description of object. All derived objects must implement.
    virtual std::string get_bdcff() const=0;

    /// Create a new object, and load properties from an istream.
    /// As sometimes the object name in the bdcff file also stores information about
    /// an object (for example, AddBackwards, we must also pass the name.
    /// Implementations should return NULL, if a read error occurs.
    virtual CaveObject *clone_from_bdcff(const std::string &name, std::istream &is) const=0;

    /// Object factory.
    static CaveObject *create_from_bdcff(const std::string &str);

    /* for the editor */
    /// Move an object after creating it in the editor; dragging the mouse at the moment when the
    /// button is still pressed.
    /// @param current The coordinate clicked.
    /// @param displacement The movement vector after the last click.
    virtual void create_drag(Coordinate current, Coordinate displacement)=0;

    /// This function is called when a single object is moved in the editor by drag&drop.
    /// @param current The coordinate clicked.
    /// @param displacement The movement vector after the last click.
    virtual void move(Coordinate current, Coordinate displacement)=0;

    /// This function is called when multiple objects are dragged in the editor.
    /// @param displacement The movement vector.
    virtual void move(Coordinate displacement)=0;

    /// Get a short string which shows the coordinates of the object in the object list of the editor.
    virtual std::string get_coordinates_text() const=0;

    /// Get an element which is very characteristic of the object - to show in the object list.
    /// If there is none, this returns O_NONE (for example a copy&paste object).
    virtual GdElementEnum get_characteristic_element() const=0;

    /// Get a string which describes the object in one sentence. It is translated to the current language.
    virtual std::string get_description_markup() const=0;

protected:
    /// Protected constructor for all derived classes.
    /// This class has no default constructor, to make sure the type variable is always set.
    explicit CaveObject(Type t): type(t) {
        enable_on_all();
    }
};

/// Initializes cave object factory. Must be called at program start.
void gd_cave_objects_init();

#endif
