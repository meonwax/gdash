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

#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cassert>

#include "cave/cavetypes.hpp"

#include "misc/printf.hpp"

/// All conversion specifiers which are recognized by the class in the format string, eg. printf %s, %d, %c etc.
const char *Printf::conv_specifiers="sdiucfgx";
/// Conversion modifiers supported.
std::string Printf::flag_characters="0-+lhm";


/**
 * @brief This function html-markups a string.
 * It will change <, >, &, and \n characters into &lt; &gt; &amp; and <br>.
 */
std::string Printf::html_markup_text(const std::string &of_what) {
    std::string result;

    for (unsigned i=0; i<of_what.size(); ++i) {
        switch (of_what[i]) {
            case '<':
                result+="&lt;";
                break;
            case '>':
                result+="&lt;";
                break;
            case '&':
                result+="&amp;";
                break;
            case '\n':
                result+="\n<br>\n";
                break;
            default:
                result+=of_what[i];
                break;
        }
    }

    return result;
}


/// Printf object constructor.
/// @param percent Character which specifies a conversion. Default is %, as in misc/printf.
/// @param format The format string.
Printf::Printf(const std::string &format_, char percent)
    :
    format(format_),
    inserted_chars(0) {
    size_t pos, nextpos=0;
    // search for the next conversion specifier.
    // the position is stored in pos.
    // if a %% is found, it is replaced with %, and the search is continued.
    while ((pos=format.find(percent, nextpos))!=std::string::npos) {
        if (pos+1==format.length())
            throw std::runtime_error("unterminated conversion specifier at the end of the string");
        if (format[pos+1]==percent) {
            /* this is just a percent sign */
            format.erase(pos, 1);
            nextpos=pos+1;
        } else {
            /* this is a conversion specifier. */
            size_t last=format.find_first_of(conv_specifiers, pos+1);
            if (last==std::string::npos)
                throw std::runtime_error("unterminated conversion specifier");

            // ok we found something like %-5s. get the conversion type (s), and get
            // the manipulator (-5).
            Conversion c;
            c.pos = pos;
            c.conv = format[last];
            c.manip = format.substr(pos+1, last-pos-1);
            c.html_markup = format.find('m') != std::string::npos;
            conversions.push_back(c);
            // now delete the conversion specifier from the string.
            format.erase(pos, last-pos+1);
            nextpos=pos;
        }
    }
}

/// This function finds the next conversion specifier in the format string,
/// and sets the ostringstream accordingly.
/// This is put in a separate function, so the templated function (operator%) is
/// not too long - to prevent code bloat.
/// Also it removes the conversion specifier from the format string.
/// @param os The ostringstream to setup according to the next found conversion specifier.
/// @param pos The position, which is the char position of the original conversion specifier.
void Printf::configure_ostream(std::ostringstream &os, Conversion &conversion) const {
    // process format specifier.
    // the type of the variable to be written is mainly handled by
    // the c++ type system. here we only take care of the small
    // differences.
    if (conversion.conv=='x')
        os<<std::hex;   // %x used to print in hexadecimal

    // if we have a manipulator, is it at the beginning of the string
    while (flag_characters.find_first_of(conversion.manip[0])!=std::string::npos) {
        switch (conversion.manip[0]) {
            case '-':
                os << std::left;
                break;
            case '0':
                os.fill('0');
                break;
            case '+':
                os << std::showpos;
                break;
            case 'l':
            case 'h':
                // do nothing; for compatibility with printf;
                break;
            case 'm':
                // html markup; already noted it in the conversion
                break;
            default:
                assert(!"don't know how to process flag character");
                break;
        }
        conversion.manip.erase(0, 1);  // erase processed flag from the string
    }
    // if the manipulator is not empty, it must be a width [. precision].
    if (!conversion.manip.empty()) {
        std::istringstream is(conversion.manip);
        unsigned width;
        is>>width;
        if (is)
            os.width(width);
        is.clear(); // clear error state, as we might not have had a width specifier
        char c;
        if (is>>c) {
            unsigned precision;
            is>>precision;
            if (!is)
                throw std::runtime_error("invalid precision");
            os << std::fixed;   // so it is the same as printf %6.4 -> [3.1400]
            os.precision(precision);
        }
    }
}

/// This function puts the contents of the ostringstream to the
/// string at the given position.
/// @param os The ostringstream, which should already contain the data formatted.
/// @param pos The position to insert the contents of the string at.
void Printf::insert_converted(std::string const &str, Conversion &conversion) const {
    // add inserted_chars to the originally calculated position - as
    // before this conversion, the already finished conversions added that much characters before the current position
    format.insert(conversion.pos + inserted_chars, str);
    // and remember the next successive position
    inserted_chars += str.length();
}
