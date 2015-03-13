/*
 * Copyright (c) 2007-2013, Czirkos Zoltan http://code.google.com/p/gdash/
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BDCFFHELPER_HPP_INCLUDED
#define BDCFFHELPER_HPP_INCLUDED

#include "config.h"

#include <string>
#include <list>
#include <sstream>

#define BDCFF_VERSION "0.5"

/**
 *  Functor which checks if a string has another string as its prefix.
 *  eg, it will return true for "SlimePermeability=0.1" begins with "SlimePermeability"
 *  The check is case-insensitive!
 */
class HasAttrib {
    std::string attrib;
public:
    explicit HasAttrib(const std::string &attrib_);
    bool operator()(const std::string &str) const;
};


/// A class which splits a BDCFF line read
/// into two parts - an attribute name and parameters.
/// For example, "SlimePermeability=0.1" is split into
/// "SlimePermeability" (attrib) and "0.1" param.
class AttribParam {
public:
    std::string attrib;
    std::string param;
    explicit AttribParam(const std::string &str, char separator = '=');
};

typedef std::list<std::string> BdcffSection;
typedef BdcffSection::iterator BdcffSectionIterator;
typedef BdcffSection::const_iterator BdcffSectionConstIterator;

/**
 * A structure, which stores the lines of text in a bdcff file; and has a list
 * of these strings for each section.
 *
 * The main format of the bdcff file.
 * Contains: bdcff section; highscore for caveset, map codes, caveset properties, and caves data.
 * Caves data contains: highscore for cave, cave properties, maybe map, maybe objects, maybe replays, maybe demo (=legacy replay).
 */
struct BdcffFile {
    struct CaveInfo {
        BdcffSection highscore;
        BdcffSection properties;
        BdcffSection map;
        BdcffSection objects;
        std::list<BdcffSection> replays;
        BdcffSection demo;
    };

    BdcffSection bdcff;
    BdcffSection highscore;
    BdcffSection mapcodes;
    BdcffSection caveset_properties;
    std::list<CaveInfo> caves;
};


/** A class which helps outputting BDCFF lines like "Point=x y z".
 *
 * It stores a name (Point), and can be fed with parameters
 * using a standard operator<<.
 * If conversion is ready, the str() member function can be
 * used to get the BDCFF output.
 */
class BdcffFormat {
private:
    std::ostringstream os;  ///< for conversion
    std::string name;       ///< name of parameter, eg. Size
    bool firstparam;        ///< used internally do determine if a space is needed

public:
    explicit BdcffFormat(const std::string &f = "");
    template <typename T> BdcffFormat &operator<<(const T &param);
    void start_new(const std::string &f);
    std::string str() const;
    /** cast to string - return the string. */
    operator std::string() const {
        return str();
    }
};

/**
 * @brief Feed next output parameter to the formatter.
 * @param param The variable to write.
 * @return Itself, for linking << a << b << c.
 */
template <typename T>
BdcffFormat &BdcffFormat::operator<<(const T &param) {
    /* if this is not the first parameter, add a space */
    if (!firstparam)
        os << ' ';
    else
        firstparam = false;
    os << param;
    return *this;
}

#endif
