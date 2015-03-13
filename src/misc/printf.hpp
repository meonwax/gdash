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

#ifndef PRINTF_HPP_INCLUDED
#define PRINTF_HPP_INCLUDED

#include "config.h"

#include <string>
#include <sstream>
#include <deque>
#include <glib.h>

/**
 * A class which is able to process format strings like misc/printf.
 *
 * Usage:
 * std::string s = Printf("Hello, %s! %-5d") % "world" % 9;
 * The format string is passed in the constructor, and parameters
 * are fed using the % operator.
 *
 * The conversion specifiers work the same way as they did in misc/printf.
 * The % character introduces a conversion, after which manipulators
 * can be given; the conversion specifier is terminated with a character
 * which specifies the type of the conversion.
 *
 * The % operator is a template, so any variable can be fed, which has
 * a standard << operator outputting it to an ostream. Giving the correct
 * type of the variable is therefore not important; Printf("%d") % "hello"
 * will work. The main purpose of the format characters is to terminate the conversion
 * specifier, and to make the format strings compatible with those of printf. Sometimes they slightly
 * modify the conversion, eg. %x will print hexadecimal. %ms will print
 * a html-markup string, i.e. Printf("%ms") % "i<5" will have "i&lt;5".
 *
 * Conversions can be modified by giving a width or a width.precision.
 * The modifiers 0 (to print zero padded, %03d), + (to always show sign, %+5d)
 * and - (to print left aligned, %-9s) are supported. Width calculation is
 * UTF-8-aware.
 *
 * To use a Printf as a temporary object conveniently, all member functions are
 * const. Therefore non-const member variables are mutable.
 */
class Printf {
protected:
    /// The format string, which may already have the results of some conversions.
    mutable std::string format;

private:
    /// Characters inserted so far - during the conversion. To know where to insert the next string.
    mutable size_t inserted_chars;

    struct Conversion {
        size_t pos;         ///< Position to insert the converted string at (+inserted_chars)
        char conv;          ///< conversion type
        int width;          ///< width. zero if no padding, positive is left padding, negative if right padding
        char pad;           ///< padding char
        std::string manip;  ///< Manipulator
        bool html_markup;   ///< Must do HTML conversion (> to &gt; etc.) for this one
    };
    mutable std::deque<Conversion> conversions;

    static const char *conv_specifiers;
    static std::string flag_characters;
    static std::string html_markup_text(const std::string &of_what);
    static std::string pad_text(std::string, int width, char pad);

    void configure_ostream(std::ostringstream &os, Conversion &conversion) const;
    void insert_converted(std::string const &os, Conversion &conversion) const;

public:
    Printf(const std::string &format, char percent = '%');

    template <class TIP> Printf const &operator%(const TIP &x) const;
};

/// The % operator feeds the next data item into the string.
/// The function replaces the next specified conversion.
/// @param x The parameter to convert to string in the specified format.
/// @return The Printf object itself, so successive calls can be linked.
template <class TIP>
Printf const &Printf::operator%(TIP const &x) const {
    g_assert(!conversions.empty());
    Conversion conversion = conversions.front();
    conversions.pop_front();

    std::ostringstream os;
    configure_ostream(os, conversion);
    os << x;
    insert_converted(os.str(), conversion);

    return *this;
}


/// A specialized version of the Printf class which can be automatically
/// cast to a char const *.
class CPrintf : public Printf {
public:
    CPrintf(const std::string &format, char percent = '%') : Printf(format, percent) {}

    /// Convert result to const char *.
    operator char const *() const {
        return format.c_str();
    }

    template <class TIP>
    CPrintf const &operator%(TIP const &x) const {
        Printf::operator%(x);
        return *this;
    }
};


/// A specialized version of the Printf class which can be automatically
/// cast to an std::string.
class SPrintf : public Printf {
public:
    SPrintf(const std::string &format, char percent = '%') : Printf(format, percent) {}

    /// Convert result to std::string.
    operator std::string const &() const {
        return format;
    }

    template <class TIP>
    SPrintf const &operator%(TIP const &x) const {
        Printf::operator%(x);
        return *this;
    }
};
#endif
