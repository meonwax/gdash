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

#include <glib.h>
#include <glib/gi18n.h>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <stdexcept>

#include "cave/cavetypes.hpp"
#include "cave/elementproperties.hpp"
#include "misc/printf.hpp"
#include "misc/logger.hpp"
#include "cave/helper/namevaluepair.hpp"


/* TRANSLATORS: None here means "no direction to move"; when there is no gravity while stirring the pot. */
static const char *direction_name[]= { N_("None"), N_("Up"), N_("Up+right"), N_("Right"), N_("Down+right"), N_("Down"), N_("Down+left"), N_("Left"), N_("Up+left") };
static const char *direction_filename[]= { "none", "up", "upright", "right", "downright", "down", "downleft", "left", "upleft" };

static const char *scheduling_name[]= { N_("Milliseconds"), "BD1", "BD2", "Construction Kit", "Crazy Dream 7", "Atari BD1", "Atari BD2/Construction Kit" };
static const char *scheduling_filename[]= { "ms", "bd1", "bd2", "plck", "crdr7", "bd1atari", "bd2ckatari" };

/* used for bdcff engine flag. */
static const char *engines_name[]= {"BD1", "BD2", "PLCK", "1stB", "Crazy Dream", "Crazy Light"};
static const char *engines_filename[]= {"BD1", "BD2", "PLCK", "1stB", "CrDr", "CrLi"};

/// Write a coordinate to an output stream.
/// Delimits the x and y components with space.
std::ostream &operator<<(std::ostream &os, Coordinate const &p) {
    return (os << p.x << ' ' << p.y);
}

/// Read a coordinate from an input stream.
/// Reads x and y coordinates; if both could be read, set p.
std::istream &operator>>(std::istream &is, Coordinate &p) {
    int x, y;
    is >> x >> y;
    /* only modify p if read both parameters correctly */
    if (is) {
        p.x=x;
        p.y=y;
    }
    return is;
}

/// Add a vector to a coordinate.
/// @param p The vector to add.
Coordinate &Coordinate::operator+=(Coordinate const &p) {
    x+=p.x;
    y+=p.y;
    return *this;
}

/// Add two coordinates (vectors).
Coordinate Coordinate::operator+(Coordinate const &rhs) const {
    return Coordinate(x+rhs.x, y+rhs.y);
}

/// Compare two coordinates for equality.
/// @return True, if they are the same.
bool Coordinate::operator==(Coordinate const &rhs) const {
    return x==rhs.x && y==rhs.y;
}

/// Get on-screen description of a coordinate.
std::string visible_name(Coordinate const &p) {
    std::ostringstream os;
    os<<'('<<p.x<<','<<p.y<<')';
    return os.str();
}

/// Drag the corners of the rectangle set by p1 and p2.
/// This is used for many objects in the editor. When clicking and dragging
/// one of the corners of a square, its size can be changed.
/// Whereas by clicking and dragging the edges, the whole square is moved.
/// We can detect clicking on the edges by comparing the coordinate clicked
/// with p1 and p2; all four possibilities have to be taken into account.
/// @param p1 One corner of the rectangle.
/// @param p2 The other corner of the rectangle.
/// @param current The coordinate clicked.
/// @param displacement The movement vector.
void Coordinate::drag_rectangle(Coordinate &p1, Coordinate &p2, Coordinate current, Coordinate displacement) {
    /* dragging objects which are box-shaped */
    if (current.x==p1.x && current.y==p1.y) {           /* try to drag (x1;y1) corner. */
        p1.x+=displacement.x;
        p1.y+=displacement.y;
    } else if (current.x==p2.x && current.y==p1.y) {    /* try to drag (x2;y1) corner. */
        p2.x+=displacement.x;
        p1.y+=displacement.y;
    } else if (current.x==p1.x && current.y==p2.y) {    /* try to drag (x1;y2) corner. */
        p1.x+=displacement.x;
        p2.y+=displacement.y;
    } else if (current.x==p2.x && current.y==p2.y) {    /* try to drag (x2;y2) corner. */
        p2.x+=displacement.x;
        p2.y+=displacement.y;
    } else {
        /* drag the whole thing */
        p1.x+=displacement.x;
        p1.y+=displacement.y;
        p2.x+=displacement.x;
        p2.y+=displacement.y;
    }
}


/// get on-screen visible "name" of an int
std::string visible_name(GdInt const &i) {
    std::ostringstream os;
    os<<i;
    return os.str();
}

/// get on-screen visible "name" of a probability
/// @todo change 1000000.0 to a constans EVERYWHERE in the code
/// @todo check everywhere when reading and writing if a +0.5 is needed, and explain why
std::string visible_name(GdProbability const &p) {
    std::ostringstream os;
    os<<std::fixed<<std::setprecision(2)<<p*100.0/1000000.0<<'%';
    return os.str();
}

/// get on-screen visible "name" of an int
const char *visible_name(GdBool const &b) {
    return b?N_("Yes"):N_("No");
}

/// get on-screen visible name of a direction
const char *visible_name(GdDirectionEnum dir) {
    g_assert(dir>=0 && unsigned(dir)<G_N_ELEMENTS(direction_name));
    return direction_name[dir];
}

/// get on-screen visible name of a scheduling
const char *visible_name(GdSchedulingEnum sched) {
    g_assert(sched>=0 && unsigned(sched)<G_N_ELEMENTS(scheduling_name));
    return scheduling_name[sched];
}

/// get on-screen visible name of a scheduling
const char *visible_name(GdEngineEnum eng) {
    g_assert(eng>=0 && unsigned(eng)<G_N_ELEMENTS(engines_name));
    return engines_name[eng];
}

/// get on-screen visible name of a scheduling
const char *visible_name(GdElementEnum elem) {
    return gd_element_properties[elem].visiblename;
}


/// Creates a CharToElementTable for conversion.
/// Adds all fixed elements, read from the gd_element_properties array.
CharToElementTable::CharToElementTable() {
    for (unsigned i=0; i<ArraySize; i++)
        table[i]=O_UNKNOWN;

    /* then set fixed characters */
    for (unsigned i=0; i<O_MAX; i++) {
        int c=gd_element_properties[i].character;

        if (c!=0) {
            /* check if already used for element */
            g_assert(table[c]==O_UNKNOWN);
            table[c]=GdElementEnum(i);
        }
    }
}

/**
 * @brief Return the GdElementEnum assigned to the character.
 *
 * @param i The character.
 * @return The element, or O_UNKNOWN if character is invalid.
 */
GdElementEnum CharToElementTable::get(unsigned i) const {
    if (i>=ArraySize || table[i]==O_UNKNOWN) {
        gd_warning(CPrintf("Invalid character representing element: %c") % char(i));
        return O_UNKNOWN;
    }
    return table[i];
}

/**
 * @brief Find an empty character to store the element in a map.
 * If finds a suitable character, also remembers.
 *
 * @param e The element to find place for.
 * @return The (new) character for the element.
 */
unsigned CharToElementTable::find_place_for(GdElementEnum e) {
    const char *not_allowed="<>&[]/=\\";

    // first check if it is already in the array.
    for (unsigned i=32; i<ArraySize; ++i)
        if (table[i]==e)
            return i;

    unsigned i;
    for (i=32; i<ArraySize; ++i)
        // if found a good empty char, break
        if (table[i]==O_UNKNOWN && strchr(not_allowed, i)==NULL)
            break;
    if (i>=ArraySize)
        throw std::runtime_error("no more characters");
    table[i]=e;
    return i;
}

/**
 * @brief Set an element assigned to a character.
 *
 * @param i The character.
 * @param e The element assigned.
 */
void CharToElementTable::set(unsigned i, GdElementEnum e) {
    if (i>=ArraySize) {
        gd_warning(CPrintf("Invalid character representing element: %c") % char(i));
        return;
    }

    if (table[i]!=O_UNKNOWN)
        gd_warning(CPrintf("Character %c already used by elements %s") % char(i) % visible_name(table[i]));

    table[i]=e;
}


static NameValuePair<GdElementEnum> name_to_element;

void gd_cave_types_init() {
    /* put names to a hash table */
    /* this is a helper for file read operations */

    for (int i=0; i<O_MAX; i++) {
        const char *key=gd_element_properties[i].filename;

        g_assert(key!=NULL && !gd_str_equal(key, ""));
        /* check if every name is used once */
        g_assert(!name_to_element.has_name(key));
        name_to_element.add(key, GdElementEnum(i));
    }
    /* for compatibility with tim stridmann's memorydump->bdcff converter... .... ... */
    name_to_element.add("HEXPANDING_WALL", O_H_EXPANDING_WALL);
    name_to_element.add("FALLING_DIAMOND", O_DIAMOND_F);
    name_to_element.add("FALLING_BOULDER", O_STONE_F);
    name_to_element.add("EXPLOSION1S", O_EXPLODE_1);
    name_to_element.add("EXPLOSION2S", O_EXPLODE_2);
    name_to_element.add("EXPLOSION3S", O_EXPLODE_3);
    name_to_element.add("EXPLOSION4S", O_EXPLODE_4);
    name_to_element.add("EXPLOSION5S", O_EXPLODE_5);
    name_to_element.add("EXPLOSION1D", O_PRE_DIA_1);
    name_to_element.add("EXPLOSION2D", O_PRE_DIA_2);
    name_to_element.add("EXPLOSION3D", O_PRE_DIA_3);
    name_to_element.add("EXPLOSION4D", O_PRE_DIA_4);
    name_to_element.add("EXPLOSION5D", O_PRE_DIA_5);
    name_to_element.add("WALL2", O_STEEL_EXPLODABLE);
    /* compatibility with old bd-faq (pre disassembly of bladder) */
    name_to_element.add("BLADDERd9", O_BLADDER_8);

    /* create table to show errors at the start of the application */
    CharToElementTable _ctet;

    /* check element database for faults. */
    for (int i=0; gd_element_properties[i].element!=-1; i++) {
        g_assert(gd_element_properties[i].element==i);
        /* game pixbuf should not use (generated) editor pixbuf */
        g_assert(abs(gd_element_properties[i].image_game)<NUM_OF_CELLS_X*NUM_OF_CELLS_Y);
        /* editor pixbuf should not be animated */
        g_assert(gd_element_properties[i].image>=0);
        if (gd_element_properties[i].flags&P_CAN_BE_HAMMERED)
            g_assert(gd_element_get_hammered(GdElementEnum(i))!=O_NONE);

        /* if its pair is not the same as itself, it is a scanned pair. */
        if (gd_element_properties[i].pair!=i) {
            /* check if it has correct scanned pair, a->b, b->a */
            g_assert(gd_element_properties[gd_element_properties[i].pair].pair==i);
            if (gd_element_properties[i].flags & P_SCANNED) {
                /* if this one is the scanned */
                /* check if non-scanned pair is not tagged as scanned */
                g_assert((gd_element_properties[gd_element_properties[i].pair].flags&P_SCANNED)==0);
                /* check if no ckdelay */
                g_assert(gd_element_properties[i].ckdelay==0);
            } else if (gd_element_properties[gd_element_properties[i].pair].flags & P_SCANNED) {
                /* if this one is the non-scanned */
                g_assert((gd_element_properties[gd_element_properties[i].pair].flags & P_SCANNED)!=0);
            } else {
                /* scan pair - one of them should be scanned */
                g_assert_not_reached();
            }
        }
    }

    g_assert(GD_SCHEDULING_MAX==G_N_ELEMENTS(scheduling_filename));
    g_assert(GD_SCHEDULING_MAX==G_N_ELEMENTS(scheduling_name));
    g_assert(MV_MAX==G_N_ELEMENTS(direction_filename));
    g_assert(MV_MAX==G_N_ELEMENTS(direction_name));
    g_assert(GD_ENGINE_MAX==G_N_ELEMENTS(engines_filename));
    g_assert(GD_ENGINE_MAX==G_N_ELEMENTS(engines_name));
}

/// Load an element from a stream, where it is stored in its name.
/// If loading fails, the stream is set to an error state.
/// If the element name is not found, an error state is also set.
/// @param is The istream to load from.
/// @param e The element to store to.
std::istream &operator>>(std::istream &is, GdElementEnum &e) {
    std::string s;
    if (is>>s) {
        if (!name_to_element.has_name(s))
            is.setstate(std::ios::failbit);
        else
            e=name_to_element.lookup_name(s);
    }
    return is;
}

/// Save a GdBool to a stream, by writing either "false" or "true".
std::ostream &operator<<(std::ostream &os, GdBool const &b) {
    os << (b?"true":"false");
    return os;
}

/// Convert a string to a GdBool.
/// If conversion succeeds, sets b; otherwise b is left untouched.
/// @param s The string to convert. Can contain 0, 1, true, false, on, off, yes, no.
/// @param b The GdBool to write to.
/// @return true, if the conversion succeeded.
bool read_from_string(const std::string &s, GdBool &b) {
    if (s=="1"
            || gd_str_ascii_caseequal(s, "true")
            || gd_str_ascii_caseequal(s, "on")
            || gd_str_ascii_caseequal(s, "yes")) {
        b=true;
        return true;
    } else if (s=="0"
               || gd_str_ascii_caseequal(s, "false")
               || gd_str_ascii_caseequal(s, "off")
               || gd_str_ascii_caseequal(s, "no")) {
        b=false;
        return true;
    }
    return false;
}

/// Save a GdInt to an ostream.
std::ostream &operator<<(std::ostream &os, GdInt const &i) {
    /* have to convert, or else it would be infinite recursion? */
    int conv=i;
    os << conv;
    return os;
}

/// Load a GdInt from a string.
/// If conversion succeeds, sets i; otherwise it is left untouched.
/// @param s The string to convert.
/// @param i The GdInt to write to.
/// @return true, if the conversion succeeded.
bool read_from_string(const std::string &s, GdInt &i) {
    std::istringstream is(s);
    /* was saved as a normal int */
    int read;
    bool success=(is>>read);
    if (success)
        i=read;
    return success;
}


/// Save a GdProbability to an ostream.
std::ostream &operator<<(std::ostream &os, GdProbability const &i) {
    double conv=i/1000000.0;
    os << conv;
    return os;
}

/// Load a GdProbability (stored as a floating point number) from a string.
/// If conversion succeeds, sets i; otherwise it is left untouched.
/// @param s The string to convert.
/// @param p The GdProbability to write to.
/// @return true, if the conversion succeeded.
bool read_from_string(const std::string &s, GdProbability &p) {
    std::istringstream is(s);
    double read;
    bool success=(is>>read) && (read>=0 && read<=1);
    if (success)
        p=read*1000000+0.5;
    return success;
}

/// Load a GdInt (stored as a floating point number) from a string.
/// If conversion succeeds, sets i; otherwise it is left untouched.
/// @param s The string to convert.
/// @param i The GdInt to write to.
/// @param conversion_ratio The number to multiply the converted value with. (1million for probabilities, cave width*height for ratios)
/// @return true, if the conversion succeeded.
bool read_from_string(const std::string &s, GdInt &i, double conversion_ratio) {
    std::istringstream is(s);
    double read;
    bool success=(is>>read) && (read>=0 && read<=1);
    if (success)
        i=read*conversion_ratio+0.5;
    return success;
}

/// Save a GdScheduling to an ostream with its name.
std::ostream &operator<<(std::ostream &os, GdScheduling const &s) {
    os << scheduling_filename[s];
    return os;
}

/// Read a GdScheduling from a string.
/// If conversion succeeds, sets sch; otherwise it is left untouched.
/// @param s The string to convert.
/// @param sch The GdScheduling to write to.
/// @return true, if the conversion succeeded.
bool read_from_string(const std::string &s, GdScheduling &sch) {
    for (unsigned i=0; i<G_N_ELEMENTS(scheduling_filename); ++i)
        if (gd_str_ascii_caseequal(s, scheduling_filename[i])) {
            sch=GdSchedulingEnum(i);
            return true;
        }
    return false;
}

/// Save a GdDirection to an ostream with its name.
std::ostream &operator<<(std::ostream &os, GdDirection const &d) {
    os << direction_name[d];
    return os;
}

/// Read a GdDirection from a string.
/// If conversion succeeds, sets d; otherwise it is left untouched.
/// @param s The string to convert.
/// @param d The GdDirection to write to.
/// @return true, if the conversion succeeded.
bool read_from_string(const std::string &s, GdDirection &d) {
    for (unsigned i=0; i<G_N_ELEMENTS(direction_filename); ++i)
        if (gd_str_ascii_caseequal(s, direction_filename[i])) {
            d=GdDirectionEnum(i);
            return true;
        }
    return false;
}

/// Save a GdElement to an ostream with its name.
std::ostream &operator<<(std::ostream &os, GdElement const &e) {
    os << gd_element_properties[e].filename;
    return os;
}

/// Read a GdElement from a string.
/// If conversion succeeds, sets e; otherwise it is left untouched.
/// @param s The string to convert.
/// @param e The GdElement to write to.
/// @return true, if the conversion succeeded.
bool read_from_string(const std::string &s, GdElement &e) {
    if (!name_to_element.has_name(s))
        return false;
    e=name_to_element.lookup_name(s);
    return true;
}

/// Read a GdEngine from a string.
/// If conversion succeeds, sets e; otherwise it is left untouched.
/// @param s The string to convert.
/// @param e The GdEngine to write to.
/// @return true, if the conversion succeeded.
bool read_from_string(const std::string &s, GdEngine &e) {
    for (unsigned i=0; i<G_N_ELEMENTS(engines_filename); ++i)
        if (gd_str_ascii_caseequal(s, engines_filename[i])) {
            e=GdEngineEnum(i);
            return true;
        }
    return false;
}
