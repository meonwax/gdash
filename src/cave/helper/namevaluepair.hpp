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
#ifndef _GD_NAMEVALUEPAIR
#define _GD_NAMEVALUEPAIR

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
        bool operator() (const std::string& s1, const std::string& s2) const;
    };
    typedef std::map<std::string, T, StringAsciiCaseCompare> NameToValueMap;
    NameToValueMap name_to_value;

public:
    bool has_name(const std::string& name) const;
    T const& lookup_name(const std::string& name) const;
    void erase(const std::string& name);
    void add(const std::string& name, const T& value);
};

template <typename T>
bool NameValuePair<T>::StringAsciiCaseCompare::operator()(const std::string& s1, const std::string& s2) const {
    return gd_str_ascii_casecmp(s1, s2)<0;
}

template <typename T>
bool NameValuePair<T>::has_name(const std::string& name) const {
    return name_to_value.find(name)!=name_to_value.end();
}

template <typename T>
T const& NameValuePair<T>::lookup_name(const std::string& name) const {
    typename NameToValueMap::const_iterator it = name_to_value.find(name);
    if (it==name_to_value.end())
        throw std::runtime_error(std::string("Cannot interpret name ")+name);
    return it->second;
}

template <typename T>
void NameValuePair<T>::add(const std::string& name, const T& value) {
    name_to_value[name]=value;
}

template <typename T>
void NameValuePair<T>::erase(const std::string& name) {
    name_to_value.erase(name);
}

#endif
