/**
 * This file is part of the KHTML renderer for KDE.
 *
 * Copyright (C) 2006, 2007 Maksim Orlovich (maksim@kde.org)
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
 */

#ifndef KHTML_MISC_TRANSLATOR_H
#define KHTML_MISC_TRANSLATOR_H

#include <QtCore/QMap>

//---------------------------------------------------------------------------------------------
/* This class is used to do remapping between different encodings reasonably compactly.
   It stores things as (L,R) tables, but reads left-side from memory as MemL */
template<typename L, typename R, typename MemL>
class IDTranslator
{
public:
    struct Info {
        MemL l;
        R    r;
    };

    IDTranslator(const Info* table) {
        for (const Info* cursor = table; cursor->l; ++cursor) {
            m_lToR.insert(cursor->l,  cursor->r);
            m_rToL.insert(cursor->r,  cursor->l);
        }
    }

    bool hasLeft(L l) {
        typename QMap<L,R>::iterator i = m_lToR.find(l);
        return i != m_lToR.end();
    }

    L toLeft(R r) {
        typename QMap<R,L>::iterator i( m_rToL.find(r) );
        if (i != m_rToL.end())
            return *i;
        return L();
    }

    R toRight(L l) {
        typename QMap<L,R>::iterator i = m_lToR.find(l);
        if (i != m_lToR.end())
            return *i;
        return R();
    }

private:
    QMap<L, R> m_lToR;
    QMap<R, L> m_rToL;
};

#define MAKE_TRANSLATOR(name,L,R,MR,table) static IDTranslator<L,R,MR>* s_##name; \
    static IDTranslator<L,R,MR>* name() { if (!s_##name) s_##name = new IDTranslator<L,R,MR>(table); \
        return s_##name; }


#endif
