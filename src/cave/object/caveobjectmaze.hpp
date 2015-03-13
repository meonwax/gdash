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
#ifndef _GD_CAVEOBJECTMAZE
#define _GD_CAVEOBJECTMAZE

#include "config.h"

#include "cave/object/caveobject.hpp"
#include "cave/object/caveobjectrectangular.hpp"

/* forward declarations for maze object */
class RandomGenerator;
template <class T> class CaveMap;

/// Cave maze objects.
///
/// There are three types of them:
///     - perfect maze
///     - braid maze (with no dead ends)
///     - unicursal maze (which is a one long closed path).
/// This is stored in the maze_type member, but also the object type
/// is different for all three maze types.
///
/// Mazes have path and wall elements; and have path and wall widths.
/// They have random number seed values for all levels, so it is possible
/// to generate a different maze for all levels. If the seed
/// value is set to -1, a different maze is generated every time a cave is rendered.
/// The horizontal parameter is a value between 0 and 100; it determines how
/// much the maze generator algorithm prefers long horizontal paths. Default is 50.
///
/// If the size of the maze object is larger than the "common multiple" determined
/// by the wall and path width, the remaining part is filled with wall.
/// No border for the maze is drawn - if the user wants a border, he can use
/// a rectangle object.
class CaveMaze : public CaveRectangular {
public:
    enum MazeType {
        Perfect,
        Braid,
        Unicursal
    };

private:
    MazeType const maze_type;     ///< Type of the maze, perfect, braid, unicursal.
    GdInt wall_width;       ///< Width of the walls in the maze (in cells); >=1
    GdInt path_width;       ///< Width of the paths in the maze; >=1
    GdElement wall_element; ///< Walls are made of this element
    GdElement path_element; ///< Paths are made of this.
    GdInt horiz;            ///< 0..100, with greater numbers, horizontal paths are preferred
    GdIntLevels seed;       ///< Seed values for difficulty levels.

    static void mazegen(CaveMap<bool> &maze, RandomGenerator &rand, int x, int y, int horiz);
    static void braidmaze(CaveMap<bool> &maze, RandomGenerator &rand);
    static void unicursalmaze(CaveMap<bool> &maze, int &w, int &h);

public:
    CaveMaze(Coordinate _p1, Coordinate _p2, GdElementEnum _wall, GdElementEnum _path, MazeType _type);
    CaveMaze(): CaveRectangular(GD_MAZE), maze_type(Perfect) {}
    virtual CaveMaze *clone() const {
        return new CaveMaze(*this);
    };
    virtual void draw(CaveRendered &cave) const;
    void set_horiz(int _horiz) {
        horiz=_horiz;
    }
    void set_widths(int wall, int path);
    void set_seed(int s1, int s2, int s3, int s4, int s5);
    virtual std::string get_bdcff() const;
    virtual CaveMaze *clone_from_bdcff(const std::string &name, std::istream &is) const;

private:
    static PropertyDescription const descriptor[];

public:
    virtual PropertyDescription const *get_description_array() const;

    virtual GdElementEnum get_characteristic_element() const {
        return path_element;
    }
    virtual std::string get_description_markup() const;
};


#endif

