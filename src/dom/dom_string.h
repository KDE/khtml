/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999 Lars Knoll (knoll@kde.org)
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
#ifndef _DOM_DOMString_h_
#define _DOM_DOMString_h_

#include <khtml_export.h>
#include <QDebug>
#include <QtCore/QString>
#include <limits.h>

namespace DOM {

class DOMStringImpl;

/**
 * This class implements the basic string we use in the DOM. We do not use
 * QString for 2 reasons: Memory overhead, and the missing explicit sharing
 * of strings we need for the DOM.
 *
 * All DOMStrings are explicitly shared (they behave like pointers), meaning
 * that modifications to one instance will also modify all others. If you
 * wish to get a DOMString that is independent, use copy().
 */
class KHTML_EXPORT DOMString
{
    friend class CharacterDataImpl;
    friend KHTML_EXPORT bool operator==( const DOMString &a, const char *b );
public:
    /**
     * default constructor. Gives an empty DOMString
     */
    DOMString() : impl(0) {}

    DOMString(const QChar *str, uint len);
    DOMString(const QString &);
    DOMString(const char *str);
    /**
     * @since 4.2
     */
    DOMString(const char *str, uint len);
    DOMString(DOMStringImpl *i);

    virtual ~DOMString();

    // assign and copy
    DOMString(const DOMString &str);
    DOMString &operator =(const DOMString &str);

    /**
     * append str to this string
     */
    DOMString &operator += (const DOMString &str);
    /**
     * add two DOMString's
     */
    DOMString operator + (const DOMString &str);

    void insert(DOMString str, uint pos);

    /**
     * The character at position i of the DOMString. If i >= length(), the
     * character returned will be 0.
     */
    const QChar &operator [](unsigned int i) const;

    int find(const QChar c, int start = 0) const;
    int reverseFind(const QChar c, int start = -1) const;

    DOMString substring(unsigned pos, unsigned len = UINT_MAX) const;

    uint length() const;
    void truncate( unsigned int len );
    void remove(unsigned int pos, int len=1);
    /**
     * Splits the string into two. The original string gets truncated to pos, and the rest is returned.
     */
    DOMString split(unsigned int pos);

    /**
     * Returns a lowercase version of the string
     */
    DOMString lower() const;
    /**
     * Returns an uppercase version of the string
     */
    DOMString upper() const;

    QChar *unicode() const;
    // for WebCore API compatibility
    inline QChar *characters() const { return unicode(); }
    QString string() const;

    int toInt() const;
    int toInt(bool* ok) const;
    float toFloat(bool* ok = 0) const;
    bool percentage(int &_percentage) const;

    static DOMString number(float f);

    DOMString copy() const;

    bool isNull()  const { return (impl == 0); }
    bool isEmpty()  const;

    bool endsWith(const DOMString& str) const;
    bool startsWith(const DOMString& str) const;

    /**
     * @internal get a handle to the imlementation of the DOMString
     * Use at own risk!!!
     */
    DOMStringImpl *implementation() const { return impl; }

    static DOMString format(const char* format, ...)
#if defined(__GNUC__)
    __attribute__ ((format (printf, 1, 2)))
#endif
     ;

protected:
    DOMStringImpl *impl;
};

inline QDebug operator<<(QDebug stream, const DOMString &string) {
	return (stream << (string.implementation() ? string.string() : QString::fromLatin1("null")));
}

KHTML_EXPORT bool operator==( const DOMString &a, const DOMString &b );
KHTML_EXPORT bool operator==( const DOMString &a, const QString &b );
KHTML_EXPORT bool operator==( const DOMString &a, const char *b );
inline bool operator!=( const DOMString &a, const DOMString &b ) { return !(a==b); }
inline bool operator!=( const DOMString &a, const QString &b ) { return !(a==b); }
inline bool operator!=( const DOMString &a, const char *b )  { return !(a==b); }
inline bool strcmp( const DOMString &a, const DOMString &b ) { return a != b; }

// returns false when equal, true otherwise (ignoring case)
KHTML_EXPORT bool strcasecmp( const DOMString &a, const DOMString &b );
KHTML_EXPORT bool strcasecmp( const DOMString& a, const char* b );

}
#endif
