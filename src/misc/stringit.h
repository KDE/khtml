/*
    This file is part of the KDE libraries

    Copyright (C) 1999 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2004 Apple Computer, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
//----------------------------------------------------------------------------
//
// KDE HTML Widget -- String class

#ifndef KHTMLSTRING_H
#define KHTMLSTRING_H

#include "dom/dom_string.h"

#include <QtCore/QList>
#include <QtCore/QString>

#include <assert.h>

using namespace DOM;

namespace khtml
{

class DOMStringIt
{
public:
    DOMStringIt()
	{ s = 0, l = 0; lines = 0; }
    DOMStringIt(QChar *str, uint len)
	{ s = str, l = len; lines = 0; }
    DOMStringIt(const QString &str)
	{ s = str.unicode(); l = str.length(); lines = 0; }

    DOMStringIt *operator++()
    {
        if(!pushedChar.isNull())
            pushedChar=0;
        else if(l > 0 ) {
            if (*s == QLatin1Char('\n'))
                lines++;
	    s++, l--;
        }
	return this;
    }
public:
    void push(const QChar& c) { /* assert(pushedChar.isNull());*/  pushedChar = c; }

    const QChar& operator*() const  { return pushedChar.isNull() ? *s : pushedChar; }
    const QChar* operator->() const { return pushedChar.isNull() ? s : &pushedChar; }

    bool escaped() const { return !pushedChar.isNull(); }
    uint length() const { return l+(!pushedChar.isNull()); }

    const QChar *current() const { return pushedChar.isNull() ? s : &pushedChar; }
    int lineCount() const { return lines; }

protected:
    QChar pushedChar;
    const QChar *s;
    int l;
    int lines;
};

class TokenizerString;

class TokenizerSubstring
{
    friend class TokenizerString;
public:    
    TokenizerSubstring() : m_length(0), m_current(0) {}
    TokenizerSubstring(const QString &str) : m_string(str), m_length(str.length()), m_current(m_length == 0 ? 0 : str.unicode()) {}
    TokenizerSubstring(const QChar *str, int length) : m_length(length), m_current(length == 0 ? 0 : str) {}

    void clear() { m_length = 0; m_current = 0; }

    void appendTo(QString &str) const {
        if (m_string.unicode() == m_current) {
            if (str.isEmpty())
                str = m_string;
            else
                str.append(m_string);
        } else {
            str.insert(str.length(), m_current, m_length);
        }
    }
private:
    QString m_string;
    int m_length;
    const QChar *m_current;
};

class TokenizerString
{

public:
    TokenizerString() : m_currentChar(0), m_lines(0), m_composite(false) {}
    TokenizerString(const QChar *str, int length) : m_currentString(str, length), m_currentChar(m_currentString.m_current), m_lines(0), m_composite(false) {}
    TokenizerString(const QString &str) : m_currentString(str), m_currentChar(m_currentString.m_current), m_lines(0), m_composite(false) {}
    TokenizerString(const TokenizerString &o) : m_pushedChar1(o.m_pushedChar1), m_pushedChar2(o.m_pushedChar2),
                                                m_currentString(o.m_currentString), m_substrings(o.m_substrings),
                                                m_lines(o.m_lines), m_composite(o.m_composite) { 
        m_currentChar = m_pushedChar1.isNull() ? m_currentString.m_current : &m_pushedChar1; 
    } 

    void clear();

    void append(const TokenizerString &);
    void prepend(const TokenizerString &);
    
    void push(QChar c) {
        if (m_pushedChar1.isNull()) {
            m_pushedChar1 = c;
	    m_currentChar = m_pushedChar1.isNull() ? m_currentString.m_current : &m_pushedChar1;
	} else {
            assert(m_pushedChar2.isNull());
            m_pushedChar2 = c;
        }
    }
    
    bool isEmpty() const { return !current(); }
    uint length() const;

    void advance() {
        if (!m_pushedChar1.isNull()) {
            m_pushedChar1 = m_pushedChar2;
            m_pushedChar2 = 0;
        } else if (m_currentString.m_current) {
            m_lines += *m_currentString.m_current++ == QLatin1Char('\n');
            if (--m_currentString.m_length == 0)
                advanceSubstring();
        }
	m_currentChar = m_pushedChar1.isNull() ? m_currentString.m_current: &m_pushedChar1;
    }
    uint count() const { return m_substrings.count(); }
    
    bool escaped() const { return !m_pushedChar1.isNull(); }

    int lineCount() const { return m_lines; }
    void resetLineCount() { m_lines = 0; }
    
    QString toString() const;

    void operator++() { advance(); }
    const QChar &operator*() const { return *current(); }
    const QChar *operator->() const { return current(); }
    
private:
    void append(const TokenizerSubstring &);
    void prepend(const TokenizerSubstring &);

    void advanceSubstring();
    const QChar *current() const { return m_currentChar; }

    QChar m_pushedChar1;
    QChar m_pushedChar2;
    TokenizerSubstring m_currentString;
    const QChar *m_currentChar;
    QList<TokenizerSubstring> m_substrings;
    int m_lines;
    bool m_composite;

};


class TokenizerQueue : public QList<TokenizerString>
{

public:
    TokenizerQueue() {}
    ~TokenizerQueue() {}
    void  push( const TokenizerString &t ) { prepend(t); }
    TokenizerString pop() {
        if (isEmpty()) 
            return TokenizerString();
        TokenizerString t(first());
        erase( begin() );
        return t;
    }
    TokenizerString& top() { return first(); }
    TokenizerString& bottom() { return last(); }
};

}

#endif

