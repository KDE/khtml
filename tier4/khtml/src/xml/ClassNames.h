/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef ClassNames_h
#define ClassNames_h

#include "misc/AtomicString.h"
#include <wtf/Vector.h>
#include <wtf/OwnPtr.h>

using khtml::AtomicString;

namespace DOM {

    class ClassNames {
        typedef Vector<khtml::AtomicString, 8> ClassNameVector;
    public:
        ClassNames()
        {
        }

        bool contains(const khtml::AtomicString& str) const
        {
            if (!m_nameVector)
                return false;

            size_t size = m_nameVector->size();
            for (size_t i = 0; i < size; ++i) {
                if (m_nameVector->at(i) == str)
                    return true;
            }

            return false;
        }

        void parseClassAttribute(const DOMString&, bool inCompatMode);

        size_t size() const { return m_nameVector ? m_nameVector->size() : 0; }
        void clear() { if (m_nameVector) m_nameVector->clear(); }
        const khtml::AtomicString& operator[](size_t i) const { ASSERT(m_nameVector); return m_nameVector->at(i); }

    private:
        OwnPtr<ClassNameVector> m_nameVector;
    };

    inline static bool isClassWhitespace(const QChar& c)
    {
        unsigned short u = c.unicode();
        if (u > 0x20)
            return false;
        return u == ' ' || u == '\r' || u == '\n' || u == '\t' || u == '\f';
    }

} // namespace DOM

#endif // ClassNames_h
