/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003 Dirk Mueller (mueller@kde.org)
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
#ifndef _DOM_DOMStringImpl_h_
#define _DOM_DOMStringImpl_h_

#include <QtCore/QString>

#include <limits.h>

#include "dom/dom_string.h"
#include "dom/dom_misc.h"
#include "misc/khtmllayout.h"
#include "misc/shared.h"

#define QT_ALLOC_QCHAR_VEC( N ) (QChar*) new char[ sizeof(QChar)*( N ) ]
#define QT_DELETE_QCHAR_VEC( P ) delete[] ((char*)( P ))

namespace DOM {

enum CaseSensitivity { CaseSensitive, CaseInsensitive };

class DOMStringImpl : public khtml::Shared<DOMStringImpl>
{
private:
    DOMStringImpl(const DOMStringImpl&);
    //DOMStringImpl& operator=(const DOMStringImpl&);
protected:
    DOMStringImpl() { s = 0, l = 0; m_hash = 0; m_inTable = 0; m_shallowCopy = 0; }
public:
    DOMStringImpl(const QChar *str, unsigned int len) : m_hash(0), m_inTable(0), m_shallowCopy(0) {
	bool havestr = str && len;
	s = QT_ALLOC_QCHAR_VEC( havestr ? len : 1 );
	if(str && len) {
	    memcpy( s, str, len * sizeof(QChar) );
	    l = len;
	} else {
	    // crash protection
	    s[0] = 0x0;
	    l = 0;
	}
    }

    explicit DOMStringImpl(const char *str);
    explicit DOMStringImpl(const char *str, uint len);
    explicit DOMStringImpl(const QChar &ch) : m_hash(0), m_inTable(0), m_shallowCopy(0) {
	s = QT_ALLOC_QCHAR_VEC( 1 );
	s[0] = ch;
	l = 1;
    }

    DOMStringImpl(const QChar* str, unsigned length, unsigned hash) : m_hash(hash), m_inTable(true), m_shallowCopy(0) {
	bool havestr = str && length;
	s = QT_ALLOC_QCHAR_VEC( havestr ? length : 1 );
	if(str && length) {
	    memcpy( s, str, length * sizeof(QChar) );
	    l = length;
	} else {
	    // crash protection
	    s[0] = 0x0;
	    l = 0;
	}
    }

    DOMStringImpl(const char* str, unsigned length, unsigned hash);

    enum ShallowCopyTag { ShallowCopy };
    // create DOMStringImpl without copying the data, it's really dangerous!
    // think a lot, before using it
    DOMStringImpl(ShallowCopyTag, QChar* str, unsigned length) : m_hash(0), m_inTable(0), m_shallowCopy(1) {
        s = str;
        l = length;
        ref(); // guard from automatic deletion
    }

    ~DOMStringImpl();

    void append(DOMStringImpl *str);
    void insert(DOMStringImpl *str, unsigned int pos);
    void truncate(int len);
    void remove(unsigned int pos, int len=1);
    DOMStringImpl *split(unsigned int pos);
    DOMStringImpl *copy() const {
        return new DOMStringImpl(s,l);
    }


    DOMStringImpl *substring(unsigned int pos, unsigned int len);
    DOMStringImpl *collapseWhiteSpace(bool preserveLF, bool preserveWS);

    const QChar &operator [] (int pos) { return s[pos]; }
    bool containsOnlyWhitespace() const;

    // ignores trailing garbage, unlike QString
    int toInt(bool* ok = 0) const;
    float toFloat(bool* ok = 0) const;

    khtml::Length* toLengthArray(int& len) const;
    khtml::Length* toCoordsArray(int& len) const;
    bool isLower() const;
    DOMStringImpl *lower() const;
    DOMStringImpl *upper() const;
    DOMStringImpl *capitalize(bool noFirstCap=false) const;
    DOMStringImpl *escapeHTML();

    QChar *unicode() const { return s; }
    uint length() const { return l; }
    QString string() const;

    bool endsWith(DOMStringImpl* str, CaseSensitivity cs = CaseSensitive) const;
    bool startsWith(DOMStringImpl* str, CaseSensitivity cs = CaseSensitive) const;

    DOMStringImpl* substring(unsigned pos, unsigned len = UINT_MAX) const;

    // Note: this actually computes the hash, so shouldn't be used lightly
    unsigned hash() const;
    
    // Unlike the above, these don't even cache the hash;
    // they return hashes for lowercase, and upper-case versions
    unsigned lowerHash() const;
    unsigned upperHash() const;

    // for WebCore API compatibility;
    static unsigned computeHash(const QChar* str, unsigned int length);
    static unsigned computeHash(const char* str) { return DOMStringImpl(str).hash(); }

    static DOMStringImpl* empty();

    QChar *s;
    unsigned int l;
    mutable unsigned m_hash;
    bool m_inTable : 1;
    bool m_shallowCopy : 1;
};

inline unsigned int qHash(const DOMString& key) {
    // 82610334 - hash value for empty string ""
    return key.implementation() ? key.implementation()->hash() : 82610334;
}

inline bool strcmp(const DOMStringImpl* a, const DOMStringImpl* b)
{
    if (!(a && b))
        return (a && a->l) || (b && b->l);
    return a->l != b->l || memcmp(a->s, b->s, a->l * sizeof(QChar));
}

bool strcasecmp(const DOMStringImpl* a, const DOMStringImpl* b);

}

namespace khtml
{
    struct StringHash;
}

namespace WTF
{
    // WebCore::StringHash is the default hash for StringImpl* and RefPtr<StringImpl>
    template<typename T> struct DefaultHash;
    template<> struct DefaultHash<DOM::DOMStringImpl*> {
        typedef khtml::StringHash Hash;
    };
    /*template<> struct DefaultHash<RefPtr<DOM::DOMStringImpl> > {
        typedef khtml::StringHash Hash;
    };*/

}

#endif
