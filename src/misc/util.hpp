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
#ifndef _GD_UTIL
#define _GD_UTIL

#include "config.h"

#include <string>
#include <vector>

/// find file, looking in user directory and install directory
/// @return full path of file if found, "" if not
std::string gd_find_data_file(const std::string &filename, const std::vector<std::string>& dirs);

/// create a string form a char*, and free it with g_free
std::string gd_tostring_free(char *str);

/// @brief Wrap a text to lines of specified maximum width.
/// Honors original newlines as well.
/// @param input The UTF-8 text to wrap. May contain UTF-8 characters, and GD_COLOR_SETCOLOR characters.
/// @param width The maximum length of lines.
/// @return The vector of lines.
std::vector<std::string> gd_wrap_text(const char *input, int width);

/// @brief Returns current date in 2008-12-04 format.
std::string gd_get_current_date();

/// @brief Returns current date in 2008-12-04 12:34 format.
std::string gd_get_current_date_time();

/// clamp integer into range
int gd_clamp(int val, int min, int max);

/// remove leading and trailing spaces from s
void gd_strchomp(std::string &s);

/// compare two strings, ignoring case for ascii characters
int gd_str_ascii_casecmp(const std::string &s1, const std::string &s2);

bool gd_str_ascii_caseequal(const std::string &s1, const std::string &s2);

bool gd_str_equal(const char *a, const char *b);

bool gd_str_ascii_prefix(const std::string &str, const std::string &prefix);

#endif
