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

#ifndef CAVEMAP_HPP_INCLUDED
#define CAVEMAP_HPP_INCLUDED

#include "config.h"

#include <stdexcept>


class CaveMapFuncs {
protected:
    CaveMapFuncs() {}
public:
    /* types */
    enum WrapType {
        RangeCheck,
        Perfect,
        LineShift
    };

    /* functions which process coordinates to be "wrapped" */
    static void perfect_wrap_coords(int w, int h, int &x, int &y);
    /* this lineshifting does not fix the y coordinates. if out of bounds, element will not be displayed. */
    /* if such an object appeared in the c64 game, well, it was a buffer overrun - ONLY TO BE USED WHEN DRAWING THE CAVE */
    static void lineshift_wrap_coords_only_x(int w, int &x, int &y);
    /* fix y coordinate, too: TO BE USED WHEN PLAYING THE CAVE */
    static void lineshift_wrap_coords_both(int w, int h, int &x, int &y);
};


inline void CaveMapFuncs::perfect_wrap_coords(int w, int h, int &x, int &y) {
    y = (y + h) % h;
    x = (x + w) % w;
}


inline void CaveMapFuncs::lineshift_wrap_coords_only_x(int w, int &x, int &y) {
    /* fit x coordinate within range, with correcting y at the same time.
     * allow going "off" the map at most 5 times. */
    if (x < 0) {
        x += 5 * w;
        y -= 5;
    }
    y += x / w; /* calculate how many times went off the map - add that many rows */
    x %= w;
    /* here do not change x to be >=0 and <= h-1 */
}


inline void CaveMapFuncs::lineshift_wrap_coords_both(int w, int h, int &x, int &y) {
    lineshift_wrap_coords_only_x(w, x, y);
    y = (y + h) % h;
}


/// @todo korrektul megcsinalni a fillelest
/// @todo vagy inkabb set_size; igazi resize csak az editorhoz kell
template <typename T>
class CaveMapBase: public CaveMapFuncs {
protected:
    /* constructor */
    CaveMapBase();
    CaveMapBase(int w, int h, const T &initial = T());
    CaveMapBase(const CaveMapBase &);
    CaveMapBase &operator=(const CaveMapBase &);
    ~CaveMapBase() {
        delete[] data;
    }
    int w, h;
    T *data;

public:
    /* functions */
    void set_size(int new_w, int new_h, const T &def = T());
    void resize(int new_w, int new_h, const T &def = T());
    void remove();
    void fill(const T &value);
    bool empty() const {
        return w == 0 || h == 0;
    }
    int width() const {
        return w;
    }
    int height() const {
        return h;
    }
};


template <typename T>
CaveMapBase<T>::CaveMapBase()
    : w(0), h(0), data(NULL) {
}


template <typename T>
CaveMapBase<T>::CaveMapBase(int w, int h, const T &initial)
    : w(w), h(h) {
    data = new T[w * h];
    fill(initial);
}


template <typename T>
CaveMapBase<T>::CaveMapBase(const CaveMapBase<T>& orig)
    : w(orig.w), h(orig.h) {
    data = new T[w * h];
    for (int i = 0; i < w * h; ++i)
        data[i] = orig.data[i];
}


template <typename T>
CaveMapBase<T>& CaveMapBase<T>::operator=(const CaveMapBase<T>& orig) {
    if (this == &orig)
        return *this;
    delete[] data;
    w = orig.w;
    h = orig.h;
    data = new T[w * h];
    for (int i = 0; i < w * h; ++i)
        data[i] = orig.data[i];
    return *this;
}


/* set size of map; fill all with def */
template <typename T>
void CaveMapBase<T>::set_size(int new_w, int new_h, const T &def) {
    /* resize only if size is really new; otherwise only fill */
    if (new_w != w || new_h != h) {
        w = new_w;
        h = new_h;
        delete[] data;
        data = new T[w * h];
    }
    fill(def);
}


/* resize map to new size; new parts are filled with def */
template <typename T>
void CaveMapBase<T>::resize(int new_w, int new_h, const T &def) {
    int orig_w = w, orig_h = h;
    if (new_w == orig_w && new_h == orig_h) /* same size - do nothing */
        return;

    /* resize array */
    T *orig_data = data;
    data = new T[new_w * new_h];

    int min_w = std::min(orig_w, new_w), min_h = std::min(orig_h, new_h);

    /* copy useful data from original */
    for (int y = 0; y < min_h; y++)
        for (int x = 0; x < min_w; x++)
            data[y * new_w + x] = orig_data[y * orig_w + x];

    /* if new map is wider, fill right hand side, but only top part */
    if (new_w > orig_w)
        for (int y = 0; y < min_h; y++)
            for (int x = orig_w; x < new_w; x++)
                data[y * new_w + x] = def;

    /* if new map is higher, fill the rest of the rows with def */
    if (new_h > orig_h)
        for (int y = orig_h; y < new_h; y++)
            for (int x = 0; x < new_w; x++)
                data[y * new_w + x] = def;

    /* remember new sizes */
    w = new_w;
    h = new_h;

    delete[] orig_data;
}


template <typename T>
void CaveMapBase<T>::fill(const T &value) {
    for (int n = 0; n < w * h; n++)
        data[n] = value;
}


template <typename T>
void CaveMapBase<T>::remove() {
    delete[] data;
    data = 0;
    w = h = 0;
}



/**
 * A fast cave map, which has neither line shifting and perfect wrapping,
 * nor range checking.
 * This is to be used in the editor and graphics engines. */
template <typename T>
class CaveMapFast: public CaveMapBase<T> {
public:
    CaveMapFast() {}
    CaveMapFast(int w, int h, const T &initial = T())
        : CaveMapBase<T>(w, h, initial) {}
    T &operator()(int x, int y) {
        return this->data[y * this->w + x];
    }
    const T &operator()(int x, int y) const {
        return this->data[y * this->w + x];
    }
};


/**
 * A clever cave map, which can do line shifting.
 * This is to be used for the game map. */
template <typename T>
class CaveMapClever: public CaveMapBase<T> {
public:
    CaveMapClever() {
        /* the default wrap type is perfect wrap */
        set_wrap_type(CaveMapBase<T>::RangeCheck);
    }
    CaveMapClever(int w, int h, const T &initial = T())
        : CaveMapBase<T>(w, h, initial) {
        set_wrap_type(CaveMapBase<T>::RangeCheck);
    }
    void set_wrap_type(typename CaveMapBase<T>::WrapType t);
    /* GET functions which remember setting */
    T &operator()(int x, int y) {
        return (this->*getter)(x, y);
    }
    const T &operator()(int x, int y) const {
        return (const_cast<CaveMapClever<T> *>(this)->*getter)(x, y);    /* constcast, but we return const& */
    }
private:
    /* pointer to the current get function */
    T &(CaveMapClever<T>::*getter)(int, int);
    /* GET function which throws an error if out of bounds. */
    T &get_rangecheck(int x, int y);
    /* GET functions with perfect wrapping. */
    T &get_perfect(int x, int y);
    /* GET functions with BD-style lineshift */
    T &get_lineshift(int x, int y);
};



/* sets wrap type to perfect or lineshift. */
template <typename T>
void CaveMapClever<T>::set_wrap_type(typename CaveMapBase<T>::WrapType t) {
    switch (t) {
        case CaveMapBase<T>::RangeCheck:
            this->getter = &CaveMapClever<T>::get_rangecheck;
            break;
        case CaveMapBase<T>::Perfect:
            this->getter = &CaveMapClever<T>::get_perfect;
            break;
        case CaveMapBase<T>::LineShift:
            this->getter = &CaveMapClever<T>::get_lineshift;
            break;
    }
}


/* GET function with perfect wrapping. */
template <typename T>
T &CaveMapClever<T>::get_rangecheck(int x, int y) {
    if (x < 0 || y < 0 || x >= this->w || y >= this->h)
        throw std::out_of_range("CaveMapClever::getrangecheck");
    return this->data[y * this->w + x];
}


/* GET function with perfect wrapping. */
template <typename T>
T &CaveMapClever<T>::get_perfect(int x, int y) {
    CaveMapBase<T>::perfect_wrap_coords(this->w, this->h, x, y);
    return this->data[y * this->w + x];
}


/* GET function with BD-style lineshift */
template <typename T>
T &CaveMapClever<T>::get_lineshift(int x, int y) {
    CaveMapBase<T>::lineshift_wrap_coords_both(this->w, this->h, x, y);
    return this->data[y * this->w + x];
}


#endif
