/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef DataRef_h
#define DataRef_h

#include <wtf/RefPtr.h>

namespace khtml {

template <class DATA>
class DataRef
{
public:
    DataRef()
    {
        data=0;
    }

    DataRef( const DataRef<DATA> &d )
    {
        data = d.data;
        data->ref();
    }

    ~DataRef()
    {
        if(data) data->deref();
    }

    const DATA* operator->() const
    {
        return data;
    }

    const DATA* get() const
    {
        return data;
    }

    DATA* access()
    {
        if (!data->hasOneRef())
        {
            data->deref();
            data = new DATA(*data);
            data->ref();
        }
        return data;
    }

    void init()
    {
        data = new DATA;
        data->ref();
    }

    DataRef<DATA>& operator=(const DataRef<DATA>& d)
    {
        if (data==d.data)
            return *this;
        if (data)
            data->deref();
        data = d.data;

        data->ref();

        return *this;
    }

    bool operator == ( const DataRef<DATA> &o ) const {
        return (*data == *(o.data) );
    }

    bool operator != ( const DataRef<DATA> &o ) const {
        return (*data != *(o.data) );
    }

private:
    DATA* data;
};

} // namespace khtml

#endif // DataRef_h
