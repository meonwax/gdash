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
#ifndef _GD_ADOPTINGCONTAINER
#define _GD_ADOPTINGCONTAINER

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
    AdoptingContainer(const AdoptingContainer& orig);
    AdoptingContainer& operator=(const AdoptingContainer& rhs);
    ~AdoptingContainer();

    /// Return last element
    T * const back() const { return _my_base::back(); }
    /// Return nth element
    T * const at(unsigned n) const { return _my_base::at(n); }
    
public:
    using _my_base::empty;
    using _my_base::size;
    void clear();
    void clear_i_own_them() { _my_base::clear(); }
    
    /// Store object given in container; adopt it (take care of deleting the pointer).
    /// @param x The pointer to the object to store.
    void push_back_adopt(T *x) { this->push_back(x); }
    
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
    typename _my_base::iterator bound=std::stable_partition(begin(), end(), p);
    for (const_iterator it=begin(); it!=bound; ++it) {
        delete (*it);
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
    (*this)=orig;
}

/// Assignment operator
template <class T>
AdoptingContainer<T>& AdoptingContainer<T>::operator=(const AdoptingContainer<T>& rhs) {
    if (this==&rhs)
        return *this;
    for (unsigned i=0; i<size(); ++i)
        delete (*this)[i];
    this->resize(rhs.size());
    for (unsigned i=0; i<size(); ++i)
        (*this)[i]=rhs[i]->clone();
    return *this;
}

/// Destructor
template <class T>
AdoptingContainer<T>::~AdoptingContainer() {
    for (unsigned i=0; i<size(); ++i)
        delete (*this)[i];
}

/// Clear object from list.
template <class T>
void AdoptingContainer<T>::clear() {
    for (unsigned i=0; i<size(); ++i)
        delete _my_base::at(i);
    _my_base::clear();
}


#endif
