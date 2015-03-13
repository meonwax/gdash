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
#ifndef NAMEVALUEPAIR_HPP_INCLUDED
#define NAMEVALUEPAIR_HPP_INCLUDED

#include "config.h"

#include <string>
#include <map>
#include <stdexcept>
#include "misc/util.hpp"

template <typename T>
class NameValuePair {
private:
    /** Class for std::map to compare strings case insensitively. */
    struct StringAsciiCaseCompare {
        bool operator()(const std::string &s1, const std::string &s2) const {
            return gd_str_ascii_casecmp(s1, s2) < 0;
        }
    };
    typedef std::map<std::string, T, StringAsciiCaseCompare> NameToValueMap;
    NameToValueMap name_to_value;

public:
    bool has_name(const std::string &name) const {
        return name_to_value.find(name) != name_to_value.end();
    }
    T const &lookup_name(const std::string &name) const;
    void erase(const std::string &name) {
        name_to_value.erase(name);
    }
    void add(const std::string &name, const T &value) {
        name_to_value[name] = value;
    }
};


template <typename T>
T const &NameValuePair<T>::lookup_name(const std::string &name) const {
    typename NameToValueMap::const_iterator it = name_to_value.find(name);
    if (it == name_to_value.end())
        throw std::runtime_error(std::string("Cannot interpret name ") + name);
    return it->second;
}

#endif
