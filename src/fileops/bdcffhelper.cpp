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

#include <string>
#include <iomanip>
#include <stdexcept>
#include "fileops/bdcffhelper.hpp"
#include "misc/util.hpp"
#include "misc/printf.hpp"

/// Create the functor which checks if the string
/// has an attrib= prefix.
HasAttrib::HasAttrib(const std::string &attrib_)
    : attrib(attrib_) {
    attrib+='=';
}

/// Check if the given string has the prefix.
bool HasAttrib::operator()(const std::string &str) const {
    return gd_str_ascii_prefix(str, attrib);
}


/// Constructor: split string given by the separator given to attrib and param.
/// @param str The string to split.
/// @param separator Separator between attrib and param; default is =.
AttribParam::AttribParam(const std::string &str, char separator) {
    size_t equal=str.find(separator);
    if (equal==std::string::npos)
        throw std::runtime_error(SPrintf("No separator in line: '%s'") % str);
    attrib=str.substr(0, equal);
    param=str.substr(equal+1);
}


/// Create a new formatter.
/// @param F The name of the output string; for example
///         give it "Point" if intending to write a line like "Point=1 2 DIRT"
BdcffFormat::BdcffFormat(const std::string &f)
    :   name(f),
        firstparam(true) {
    os << std::setprecision(4) << std::fixed;
}

/// Get the output string.
/// @return The converted string.
std::string BdcffFormat::str() const {
    if (name.empty())
        return os.str();
    else
        return name+'='+os.str();
}

/// Start a new conversion with a new name.
/// @param f The new name.
void BdcffFormat::start_new(const std::string &f) {
    name=f;
    firstparam=true;
    os.str("");     /* clear output */
}
