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

#ifndef GD_SMARTPTR
#define GD_SMARTPTR

#include <stdexcept>

/// @brief A generic, non-intrusive shared pointer class.
template <typename T>
class SmartPtr {
private:
    /// The object pointed to. If this is NULL, counter must also be NULL.
    T *rawptr;
    /// Pointer to the reference counter of the object pointed to.
    /// If this is NULL, rawptr must also be NULL.
    int *counter;

    /// For handling inherited pointed to objects.
    template <typename U> friend class SmartPtr;

    /// Like the copy constructor. Copy the pointer of the pointed to
    /// object to this SmartPtr.
    /// @param the_other The other smart pointer to copy.
    template <class U>
    void copy(SmartPtr<U> const &the_other) {
        rawptr = the_other.rawptr;
        counter = the_other.counter;
        if (counter != NULL)
            ++ *counter;
    }

public:
    /// Default ctor, which constructs a NULL smartpointer.
    SmartPtr() : rawptr(NULL), counter(NULL) {}

    /// Create a smart pointer, optionally pointing to a newly allocated
    /// object. If the pointer given as the first parameter is not NULL,
    /// it must have been allocated with new, and no other SmartPtr objects
    /// are allowed to point to it.
    /// @param rawptr The object to point to.
    template <typename U>
    SmartPtr(U *rawptr)
        : rawptr(rawptr), counter(NULL) {
        if (rawptr != NULL)
            counter = new int(1);
    }

    /// Simple copy ctor.
    /// @param the_other The other smart pointer to share the managed object with.
    SmartPtr(SmartPtr const &the_other) {
        copy(the_other);
    }

    /// Templated copy ctor to allow smart pointers to handle inheritance
    /// relationships between objects pointed to.
    /// @param the_other The other smart pointer to share the managed object with.
    template <typename U>
    SmartPtr(SmartPtr<U> const &the_other) {
        copy(the_other);
    }

    /// Simple assignment operator. May delete the object that was pointed to.
    /// @param the_other The other smart pointer to share the managed object with.
    SmartPtr &operator=(SmartPtr const &the_other) {
        if (this != &the_other) {
            release();
            copy(the_other);
        }
        return *this;
    }

    /// Templated assignment operator to allow smart pointers to handle inheritance
    /// relationships between objects pointed to.
    /// @param the_other The other smart pointer to share the managed object with.
    template <typename U>
    SmartPtr &operator=(SmartPtr<U> const &the_other) {
        if (this != &the_other) {
            release();
            copy(the_other);
        }
        return *this;
    }

    /// Destructor. If the destructed smart pointer was the last one pointing to
    /// the managed object, it will be deleted.
    ~SmartPtr() {
        release();
    }

    /// Dereferencing.
    T &operator*() const {
        if (rawptr == NULL)
            throw std::logic_error("Null SmartPtr");
        return *rawptr;
    }

    /// Dereferencing.
    T *operator->() const {
        if (rawptr == NULL)
            throw std::logic_error("Null SmartPtr");
        return rawptr;
    }

    /// Compare for equality: two smart pointers are equal if they point to the same object.
    bool operator==(void *ptr) const {
        return rawptr == ptr;
    }

    /// Compare for inequality: two smart pointers are inequal if they point to different objects.
    bool operator!=(void *ptr) const {
        return rawptr != ptr;
    }

    /// Release the managed object, and become a NULL pointer.
    /// If this smart pointer was the last one pointing to
    /// the managed object, it will be deleted.
    void release() {
        if (counter != NULL) {
            -- *counter;
            if (*counter == 0) {
                delete rawptr;
                delete counter;
            }
        }
        rawptr = NULL;
        counter = NULL;
    }
};


#endif
