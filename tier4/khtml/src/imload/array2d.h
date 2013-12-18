/*
    Large image displaying library.

    Copyright (C) 2004,2005 Maks Orlovich (maksim@kde.org)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#ifndef ARRAY2D_H
#define ARRAY2D_H


namespace khtmlImLoad {

/**
 A little helper template for nicer 2D arrays. This is in no way
 fancy, though, so this shouldn't be copied, etc.
*/
template <typename T>
class Array2D
{
private:
    T* data;
    unsigned int cols;
    unsigned int rows;
public:
    Array2D(unsigned int _cols, unsigned int _rows):
        data(new T[_cols*_rows]), cols(_cols), rows(_rows)
    {}

    Array2D()
    {
        data = 0;
        rows = 0;
        cols = 0;
    }

    ~Array2D()
    {
        delete[] data;
    }

    //Note: this is meant to be used only on empty objects
    Array2D& operator=(const Array2D& other)
    {
        assert(data == 0);
        data = other.data;
        cols = other.cols;
        rows = other.rows;
        return *this;
    }

    T& at(unsigned int col, unsigned int row)
    {
        return data[row * cols + col];
    }

    const T& at(unsigned int col, unsigned int row) const
    {
        return data[row * cols + col];
    }
private:
    Array2D(const Array2D& other);
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
