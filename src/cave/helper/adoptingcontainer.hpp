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
#ifndef ADOPTINGCONTAINER_HPP_INCLUDED
#define ADOPTINGCONTAINER_HPP_INCLUDED

#include "config.h"

#include <algorithm>
#include <vector>

/**
 * Class which stores objects' pointers, and adopts them.
 *
 * The main purpose of this class to create a container for the
 * objects, which is able to copy them using their clone() methods,
 * and adopts the pointers given to them to store.
 * Some functions can be inherited from the base class,
 * some need to be redefined to delete the pointers as needed.
 */
template <class T>
class AdoptingContainer: private std::vector<T *> {
private:
    typedef std::vector<T *> _my_base;
public:
    AdoptingContainer();
    AdoptingContainer(const AdoptingContainer &orig);
    AdoptingContainer &operator=(const AdoptingContainer &rhs);
    ~AdoptingContainer();

    /// Return last element
    T *const back() const {
        return _my_base::back();
    }
    /// Return nth element
    T *const at(unsigned n) const {
        return _my_base::at(n);
    }

public:
    using _my_base::empty;
    using _my_base::size;
    void clear();
    void clear_i_own_them() {
        _my_base::clear();
    }

    /// Store object given in container; adopt it (take care of deleting the pointer).
    /// @param x The pointer to the object to store.
    void push_back_adopt(T *x) {
        this->push_back(x);
    }

    typedef typename _my_base::const_iterator const_iterator;
    using _my_base::begin;
    using _my_base::end;

    template <class PRED> void remove_if(PRED p);
};

/// Remove elements from store, if p is true.
/// Only needed by the editor, to be able to remove some objects.
/// @param p Predicate to decide if a given element should be removed.
template <class T>
template <class PRED>
void AdoptingContainer<T>::remove_if(PRED p) {
    typename _my_base::iterator bound = std::stable_partition(begin(), end(), p);
    for (const_iterator it = begin(); it != bound; ++it) {
        delete(*it);
    }
    _my_base::erase(begin(), bound);
}

/// Creates an empty store
template <class T>
AdoptingContainer<T>::AdoptingContainer() {
}

/// Copy constructor
template <class T>
AdoptingContainer<T>::AdoptingContainer(const AdoptingContainer<T>& orig) {
    (*this) = orig;
}

/// Assignment operator
template <class T>
AdoptingContainer<T>& AdoptingContainer<T>::operator=(const AdoptingContainer<T>& rhs) {
    if (this == &rhs)
        return *this;
    for (unsigned i = 0; i < size(); ++i)
        delete(*this)[i];
    this->resize(rhs.size());
    for (unsigned i = 0; i < size(); ++i)
        (*this)[i] = rhs[i]->clone();
    return *this;
}

/// Destructor
template <class T>
AdoptingContainer<T>::~AdoptingContainer() {
    for (unsigned i = 0; i < size(); ++i)
        delete(*this)[i];
}

/// Clear object from list.
template <class T>
void AdoptingContainer<T>::clear() {
    for (unsigned i = 0; i < size(); ++i)
        delete _my_base::at(i);
    _my_base::clear();
}


#endif
