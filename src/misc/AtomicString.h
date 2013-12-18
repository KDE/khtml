/*
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef AtomicString_h
#define AtomicString_h

#include "AtomicStringImpl.h"
#include "dom/dom_string.h"

using DOM::DOMString;
using DOM::DOMStringImpl;

namespace khtml {

class AtomicString {
public:
    static void init();

    AtomicString() { }
    AtomicString(const char* s) : m_string(add(s)) { }
    AtomicString(const QChar* s, int length) : m_string(add(s, length)) { }
    AtomicString(const QChar* s) : m_string(add(s)) { }
    //AtomicString(const KJS::UString& s) : m_string(add(s)) { }
    //AtomicString(const KJS::Identifier& s) : m_string(add(s)) { }
    AtomicString(DOMStringImpl* imp) : m_string(add(imp)) { }
    AtomicString(AtomicStringImpl* imp) : m_string(imp) { }
    AtomicString(const DOMString& s) : m_string(add(s.implementation())) { }

    //static AtomicStringImpl* find(const KJS::Identifier&);

    operator const DOMString&() const { return m_string; }
    const DOMString& string() const { return m_string; };

    //operator KJS::UString() const;

    AtomicStringImpl* impl() const { return static_cast<AtomicStringImpl *>(m_string.implementation()); }
    
    const QChar* characters() const { return m_string.characters(); }
    unsigned length() const { return m_string.length(); }
    
    QChar operator[](unsigned int i) const { return m_string[i]; }
    
    /*FIXME: not yet implemented in DOMString
    bool contains(QChar c) const { return m_string.contains(c); }
    bool contains(const AtomicString& s, bool caseSensitive = true) const
        { return m_string.contains(s.string(), caseSensitive); }

    int find(QChar c, int start = 0) const { return m_string.find(c, start); }
    int find(const AtomicString& s, int start = 0, bool caseSentitive = true) const
        { return m_string.find(s.string(), start, caseSentitive); }
    
    bool startsWith(const AtomicString& s, bool caseSensitive = true) const
        { return m_string.startsWith(s.string(), caseSensitive); }
    bool endsWith(const AtomicString& s, bool caseSensitive = true) const
        { return m_string.endsWith(s.string(), caseSensitive); }

    int toInt(bool* ok = 0) const { return m_string.toInt(ok); }
    double toDouble(bool* ok = 0) const { return m_string.toDouble(ok); }
    float toFloat(bool* ok = 0) const { return m_string.toFloat(ok); }
    bool percentage(int& p) const { return m_string.percentage(p); }
    Length* toLengthArray(int& len) const { return m_string.toLengthArray(len); }
    Length* toCoordsArray(int& len) const { return m_string.toCoordsArray(len); }*/

    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return m_string.isEmpty(); }

    static void remove(DOMStringImpl*);

private:
    DOMString m_string;

    static DOMStringImpl* add(const char*);
    static DOMStringImpl* add(const QChar*, int length);
    static DOMStringImpl* add(const QChar*);
    static DOMStringImpl* add(DOMStringImpl*);
    //static PassRefPtr<DOMStringImpl> add(const KJS::UString&);
    //static PassRefPtr<DOMStringImpl> add(const KJS::Identifier&);
};

inline bool operator==(const AtomicString& a, const AtomicString& b) { return a.impl() == b.impl(); }
bool operator==(const AtomicString& a, const char* b);
//inline bool operator==(const AtomicString& a, const DOMString& b) { return equal(a.impl(), b.implementation()); }
inline bool operator==(const char* a, const AtomicString& b) { return b == a; }
/*inline bool operator==(const DOMString& a, const AtomicString& b) { return equal(a.implementation(), b.impl()); }*/

inline bool operator!=(const AtomicString& a, const AtomicString& b) { return a.impl() != b.impl(); }
/*inline bool operator!=(const AtomicString& a, const char *b) { return !(a == b); }
inline bool operator!=(const AtomicString& a, const String& b) { return !equal(a.impl(), b.impl()); }
inline bool operator!=(const char* a, const AtomicString& b) { return !(b == a); }
inline bool operator!=(const String& a, const AtomicString& b) { return !equal(a.impl(), b.impl()); }

inline bool equalIgnoringCase(const AtomicString& a, const AtomicString& b) { return equalIgnoringCase(a.impl(), b.impl()); }
inline bool equalIgnoringCase(const AtomicString& a, const char* b) { return equalIgnoringCase(a.impl(), b); }
inline bool equalIgnoringCase(const AtomicString& a, const String& b) { return equalIgnoringCase(a.impl(), b.impl()); }
inline bool equalIgnoringCase(const char* a, const AtomicString& b) { return equalIgnoringCase(a, b.impl()); }
inline bool equalIgnoringCase(const String& a, const AtomicString& b) { return equalIgnoringCase(a.impl(), b.impl()); }*/

// Define external global variables for the commonly used atomic strings.
#ifndef ATOMICSTRING_HIDE_GLOBALS
    extern const AtomicString nullAtom;
    extern const AtomicString emptyAtom;
    extern const AtomicString textAtom;
    extern const AtomicString commentAtom;
    extern const AtomicString starAtom;
#endif

}

#endif // AtomicString_h
