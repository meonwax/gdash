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

#ifndef AUTOGFREEPTR_HPP_INCLUDED
#define AUTOGFREEPTR_HPP_INCLUDED

#include <glib.h>
#include <algorithm>

/// An auto_ptr-like class which uses g_free on the objects to be deleted.
template <typename T>
class AutoGFreePtr {
private:
    T *ptr;

public:
    explicit AutoGFreePtr(T *ptr = NULL): ptr(ptr) {}
    AutoGFreePtr(AutoGFreePtr &the_other) {
        this->ptr = the_other.ptr;
        the_other.ptr = NULL;
    }
    /* copy and swap */
    AutoGFreePtr &operator=(AutoGFreePtr the_other) {
        std::swap(this->ptr, the_other.ptr);
    }
    ~AutoGFreePtr() {
        g_free(ptr);
    }
    operator T *() const {
        return ptr;
    }
};

#endif
