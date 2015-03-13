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

#ifndef _GD_CAVEMAP
#define _GD_CAVEMAP

#include "config.h"

#include <stdexcept>

class CaveMapBase {
protected:
    CaveMapBase() {}
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

/// @todo korrektul megcsinalni a fillelest
/// @todo vagy inkabb set_size; igazi resize csak az editorhoz kell
template <typename T>
class CaveMap: public CaveMapBase {
public:
    /* constructor */
    CaveMap();
    CaveMap(int w, int h, const T& initial=T());
    CaveMap(const CaveMap&);
    CaveMap& operator=(const CaveMap&);
    ~CaveMap() { delete[] data; }
    
    /* functions */
    void set_size(int new_w, int new_h, const T& def=T());
    void resize(int new_w, int new_h, const T& def=T());
    void remove();
    void fill(const T& value);
    void set_wrap_type(WrapType t);
    bool empty() const { return w==0 || h==0; }
    int width() const { return w; }
    int height() const { return h; }

    /* GET functions which remember setting */
    T& get(int x, int y) { return (this->*getter)(x, y); }
    const T& get(int x, int y) const { return (const_cast<CaveMap<T> *>(this)->*getter)(x, y); }  /* constcast, but we return const& */

    T& operator()(int x, int y) { return get(x, y); }
    const T& operator()(int x, int y) const { return get(x, y); }

private:
    int w, h;
    /* pointer to the current get function */
    T& (CaveMap<T>::*getter)(int, int);
    T *data;

    /* GET function which throws an error if out of bounds. */  
    T& get_rangecheck(int x, int y);
    /* GET functions with perfect wrapping. */  
    T& get_perfect(int x, int y);
    /* GET functions with BD-style lineshift */ 
    T& get_lineshift(int x, int y);
};

template <typename T>
CaveMap<T>::CaveMap()
: w(0), h(0) {
    data=new T[0];
    /* the default wrap type is perfect wrap */
    set_wrap_type(RangeCheck);
}

template <typename T>
CaveMap<T>::CaveMap(int w, int h, const T& initial)
: w(w), h(h) {
    data=new T[w*h];
    fill(initial);
    /* the default wrap type is perfect wrap */
    set_wrap_type(RangeCheck);
}

template <typename T>
CaveMap<T>::CaveMap(const CaveMap<T>& orig)
: w(orig.w), h(orig.h), getter(orig.getter) {
    data=new T[w*h];
    for (int i=0; i<w*h; ++i)
        data[i]=orig.data[i];
}

template <typename T>
CaveMap<T>& CaveMap<T>::operator=(const CaveMap<T>& orig) {
    if (this==&orig)
        return *this;
    delete[] data;
    w=orig.w;
    h=orig.h;
    getter=orig.getter;
    data=new T[w*h];
    for (int i=0; i<w*h; ++i)
        data[i]=orig.data[i];
    return *this;
}

/* sets wrap type to perfect or lineshift. */
template <typename T>
void CaveMap<T>::set_wrap_type(WrapType t) {
    switch (t) {
        case RangeCheck: getter=&CaveMap<T>::get_rangecheck; break;
        case Perfect: getter=&CaveMap<T>::get_perfect; break;
        case LineShift: getter=&CaveMap<T>::get_lineshift; break;
    }
}

/* GET function with perfect wrapping. */   
template <typename T>
T& CaveMap<T>::get_rangecheck(int x, int y) {
    if (x<0 || y<0 || x>=w || y>=h)
        throw std::out_of_range("CaveMap::getrangecheck");
    return data[y*w+x];
}

/* GET function with perfect wrapping. */   
template <typename T>
T& CaveMap<T>::get_perfect(int x, int y) {
    perfect_wrap_coords(w, h, x, y);
    return data[y*w+x];
}

/* GET function with BD-style lineshift */  
template <typename T>
T& CaveMap<T>::get_lineshift(int x, int y) {
    lineshift_wrap_coords_both(w, h, x, y);
    return data[y*w+x];
}

/* set size of map; fill all with def */
template <typename T>
void CaveMap<T>::set_size(int new_w, int new_h, const T& def) {
    /* resize only if size is really new; otherwise only fill */
    if (new_w!=w || new_h!=h) {
        w=new_w;
        h=new_h;
        delete[] data;
        data=new T[w*h];
    }
    fill(def);
}

/* resize map to new size; new parts are filled with def */
template <typename T>
void CaveMap<T>::resize(int new_w, int new_h, const T& def) {
    int orig_w=w, orig_h=h;
    if (new_w==orig_w && new_h==orig_h) /* same size - do nothing */
        return;

    /* resize array */
    T *orig_data=data;
    data=new T[new_w*new_h];

    int min_w=std::min(orig_w, new_w), min_h=std::min(orig_h, new_h);
    
    /* copy useful data from original */
    for (int y=0; y<min_h; y++)
        for (int x=0; x<min_w; x++)
            data[y*new_w+x]=orig_data[y*orig_w+x];
    
    /* if new map is wider, fill right hand side, but only top part */
    if (new_w>orig_w)
        for (int y=0; y<min_h; y++)
            for (int x=orig_w; x<new_w; x++)
                data[y*new_w+x]=def;
    
    /* if new map is higher, fill the rest of the rows with def */
    if (new_h>orig_h)
        for (int y=orig_h; y<new_h; y++)
            for (int x=0; x<new_w; x++)
                data[y*new_w+x]=def;
    
    /* remember new sizes */
    w=new_w;
    h=new_h;
    
    delete[] orig_data;
}


template <typename T>
void CaveMap<T>::fill(const T& value) {
    for (int n=0; n<w*h; n++)
        data[n]=value;
}

template <typename T>
void CaveMap<T>::remove() {
    delete[] data;
    data=0;
    w=h=0;
}

#endif
