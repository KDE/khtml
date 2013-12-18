/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001-2003 Dirk Mueller ( mueller@kde.org )
 *           (C) 2002, 2004 Apple Computer, Inc.
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

#include "dom_stringimpl.h"

#include <string.h>
#include <QtCore/QMutableStringListIterator>
#include "misc/AtomicString.h"

using namespace DOM;
using namespace khtml;

DOMStringImpl::DOMStringImpl(const char *str) : m_hash(0), m_inTable(0), m_shallowCopy(0)
{
    if(str && *str)
    {
        l = strlen(str);
        s = QT_ALLOC_QCHAR_VEC( l );
        int i = l;
        QChar* ptr = s;
        while( i-- )
            *ptr++ = *str++;
    }
    else
    {
        s = QT_ALLOC_QCHAR_VEC( 1 );  // crash protection
        s[0] = 0x0; // == QChar::null;
        l = 0;
    }
}

DOMStringImpl::DOMStringImpl(const char *str, uint len) : m_hash(0), m_inTable(0), m_shallowCopy(0)
{
    if(str && *str)
    {
        l = len;
        s = QT_ALLOC_QCHAR_VEC( l );
        int i = l;
        QChar* ptr = s;
        while( i-- )
            *ptr++ = *str++;
    }
    else
    {
        s = QT_ALLOC_QCHAR_VEC( 1 );  // crash protection
        s[0] = 0x0; // == QChar::null;
        l = 0;
    }
}

DOMStringImpl::DOMStringImpl(const char* str, unsigned len/*gth*/, unsigned hash) : m_hash(hash), m_inTable(true), m_shallowCopy(0)
{
    if(str && *str)
    {
        l = len;
        s = QT_ALLOC_QCHAR_VEC( l );
        int i = l;
        QChar* ptr = s;
        while( i-- )
            *ptr++ = *str++;
    }
    else
    {
        s = QT_ALLOC_QCHAR_VEC( 1 );  // crash protection
        s[0] = 0x0; // == QChar::null;
        l = 0;
    }
}

DOMStringImpl::~DOMStringImpl()
{
    if (m_shallowCopy)
        return;
    if (m_inTable)
        khtml::AtomicString::remove(this);
    if (s)
        QT_DELETE_QCHAR_VEC(s);
}


// FIXME: should be a cached flag maybe.
bool DOMStringImpl::containsOnlyWhitespace() const
{
    if (!s)
        return true;

    for (uint i = 0; i < l; i++) {
        QChar c = s[i];
        if (c.unicode() <= 0x7F) {
            if (c.unicode() > ' ')
                return false;
        } else {
            if (c.direction() != QChar::DirWS)
                return false;
        }
    }
    return true;
}


void DOMStringImpl::append(DOMStringImpl *str)
{
    if(str && str->l != 0)
    {
        int newlen = l+str->l;
        QChar *c = QT_ALLOC_QCHAR_VEC(newlen);
        memcpy(c, s, l*sizeof(QChar));
        memcpy(c+l, str->s, str->l*sizeof(QChar));
        if(s) QT_DELETE_QCHAR_VEC(s);
        s = c;
        l = newlen;
    }
}

void DOMStringImpl::insert(DOMStringImpl *str, unsigned int pos)
{
    if(pos > l)
    {
        append(str);
        return;
    }
    if(str && str->l != 0)
    {
        int newlen = l+str->l;
        QChar *c = QT_ALLOC_QCHAR_VEC(newlen);
        memcpy(c, s, pos*sizeof(QChar));
        memcpy(c+pos, str->s, str->l*sizeof(QChar));
        memcpy(c+pos+str->l, s+pos, (l-pos)*sizeof(QChar));
        if(s) QT_DELETE_QCHAR_VEC(s);
        s = c;
        l = newlen;
    }
}

void DOMStringImpl::truncate(int len)
{
    if(len > (int)l) return;

    int nl = len < 1 ? 1 : len;
    QChar *c = QT_ALLOC_QCHAR_VEC(nl);
    memcpy(c, s, nl*sizeof(QChar));
    if(s) QT_DELETE_QCHAR_VEC(s);
    s = c;
    l = len;
}

void DOMStringImpl::remove(unsigned int pos, int len)
{
  if(pos >= l ) return;
  if(pos+len > l)
    len = l - pos;

  uint newLen = l-len;
  QChar *c = QT_ALLOC_QCHAR_VEC(newLen);
  memcpy(c, s, pos*sizeof(QChar));
  memcpy(c+pos, s+pos+len, (l-len-pos)*sizeof(QChar));
  if(s) QT_DELETE_QCHAR_VEC(s);
  s = c;
  l = newLen;
}

DOMStringImpl *DOMStringImpl::split(unsigned int pos)
{
  if( pos >=l ) return new DOMStringImpl();

  uint newLen = l-pos;
  DOMStringImpl *str = new DOMStringImpl(s + pos, newLen);
  truncate(pos);
  return str;
}

DOMStringImpl *DOMStringImpl::substring(unsigned int pos, unsigned int len)
{
  if( pos >=l ) return new DOMStringImpl();
  if( len == UINT_MAX || pos+len > l)
    len = l - pos;

  return new DOMStringImpl(s + pos, len);
}

// Collapses white-space according to CSS 2.1 rules
DOMStringImpl *DOMStringImpl::collapseWhiteSpace(bool preserveLF, bool preserveWS)
{
    if (preserveLF && preserveWS) return this;

    // Notice we are likely allocating more space than needed (worst case)
    QChar *n = QT_ALLOC_QCHAR_VEC(l);

    unsigned int pos = 0;
    bool collapsing = false;   // collapsing white-space
    bool collapsingLF = false; // collapsing around linefeed
    bool changedLF = false;
    for(unsigned int i=0; i<l; i++) {
        ushort ch = s[i].unicode();

        // We act on \r as we would on \n because CSS uses it to indicate new-line
        if (ch == '\r') ch = '\n';
        else
        // ### The XML parser lets \t through, for now treat them as spaces
        if (ch == '\t') ch = ' ';

        if (!preserveLF && ch == '\n') {
            // ### Not strictly correct according to CSS3 text-module.
            // - In ideographic languages linefeed should be ignored
            // - and in Thai and Khmer it should be treated as a zero-width space
            ch = ' '; // Treat as space
            changedLF = true;
        }

        if (collapsing) {
            if (ch == ' ')
                continue;
            if (ch == '\n') {
                collapsingLF = true;
                continue;
            }

            n[pos++] = (collapsingLF) ? QLatin1Char('\n') : QLatin1Char(' ');
            collapsing = false;
            collapsingLF = false;
        }
        else
        if (!preserveWS && ch == ' ') {
            collapsing = true;
            continue;
        }
        else
        if (!preserveWS && ch == '\n') {
            collapsing = true;
            collapsingLF = true;
            continue;
        }

        n[pos++] = ch;
    }
    if (collapsing)
        n[pos++] = ((collapsingLF) ? QLatin1Char('\n') : QLatin1Char(' '));

    if (pos == l && !changedLF) {
        QT_DELETE_QCHAR_VEC(n);
        return this;
    }
    else {
        DOMStringImpl* out = new DOMStringImpl();
        out->s = n;
        out->l = pos;

        return out;
    }
}

static Length parseLength(const QChar *s, unsigned int l)
{
    if (l == 0) {
        return Length(1, Relative);
    }

    unsigned i = 0;
    while (i < l && s[i].isSpace())
        ++i;
    if (i < l && (s[i] == '+' || s[i] == '-'))
        ++i;
    while (i < l && s[i].isDigit())
        ++i;

    bool ok;
    int r = QString::fromRawData(s, i).toInt(&ok);

    /* Skip over any remaining digits, we are not that accurate (5.5% => 5%) */
    while (i < l && (s[i].isDigit() || s[i] == '.'))
        ++i;

    /* IE Quirk: Skip any whitespace (20 % => 20%) */
    while (i < l && s[i].isSpace())
        ++i;

    if (ok) {
        if (i == l) {
            return Length(r, Fixed);
        } else {
            const QChar* next = s+i;

            if (*next == '%')
                return Length(static_cast<double>(r), Percent);

            if (*next == '*') 
                return Length(r, Relative);
        }
        return Length(r, Fixed);
    } else {
        if (i < l) {
            const QChar* next = s+i;
            
            if (*next == '*') 
                return Length(1, Relative);

            if (*next == '%')
                return Length(1, Relative);
        }
    }
    return Length(0, Relative);
}

khtml::Length* DOMStringImpl::toCoordsArray(int& len) const
{
    QString str(s, l);
    for(unsigned int i=0; i < l; i++) {
        QChar cc = s[i];
        if (cc > '9' || (cc < '0' && cc != '-' && cc != '*' && cc != '.'))
            str[i] = ' ';
    }
    str = str.simplified();

    len = str.count(' ') + 1;
    khtml::Length* r = new khtml::Length[len];

    int j = 0;
    int pos = 0;
    int pos2;

    while((pos2 = str.indexOf(QLatin1Char(' '), pos)) != -1) {
        r[j++] = parseLength((QChar *) str.unicode()+pos, pos2-pos);
        pos = pos2+1;
    }
    r[j] = parseLength((QChar *) str.unicode()+pos, str.length()-pos);

    return r;
}

khtml::Length* DOMStringImpl::toLengthArray(int& len) const
{
    QString str(s, l);
    str = str.simplified();

    len = str.count(QLatin1Char(',')) + 1;

    // If we have no commas, we have no array.
    if( len == 1 )
        return 0L;

    khtml::Length* r = new khtml::Length[len];

    int i = 0;
    int pos = 0;
    int pos2;

    while((pos2 = str.indexOf(QLatin1Char(','), pos)) != -1) {
        r[i++] = parseLength((QChar *) str.unicode()+pos, pos2-pos);
        pos = pos2+1;
    }

    /* IE Quirk: If the last comma is the last char skip it and reduce len by one */
    if (str.length()-pos > 0)
        r[i] = parseLength((QChar *) str.unicode()+pos, str.length()-pos);
    else
        len--;

    return r;
}

bool DOMStringImpl::isLower() const
{
    unsigned int i;
    for (i = 0; i < l; i++)
	if (s[i].toLower() != s[i])
	    return false;
    return true;
}

DOMStringImpl *DOMStringImpl::lower() const
{
    DOMStringImpl *c = new DOMStringImpl;
    if(!l) return c;

    c->s = QT_ALLOC_QCHAR_VEC(l);
    c->l = l;

    for (unsigned int i = 0; i < l; i++)
	c->s[i] = s[i].toLower();

    return c;
}

DOMStringImpl *DOMStringImpl::upper() const
{
    DOMStringImpl *c = new DOMStringImpl;
    if(!l) return c;

    c->s = QT_ALLOC_QCHAR_VEC(l);
    c->l = l;

    for (unsigned int i = 0; i < l; i++)
	c->s[i] = s[i].toUpper();

    return c;
}

DOMStringImpl *DOMStringImpl::capitalize(bool noFirstCap) const
{
    bool canCapitalize= !noFirstCap;
    DOMStringImpl *c = new DOMStringImpl;
    if(!l) return c;
    
    c->s = QT_ALLOC_QCHAR_VEC(l);
    c->l = l;

    for (unsigned int i=0; i<l; i++)
    {
        if (s[i].isLetterOrNumber() && canCapitalize)
        {
            c->s[i]=s[i].toUpper();
            canCapitalize=false;
        }
        else
        {
            c->s[i]=s[i];
            if (s[i].isSpace())
                canCapitalize=true;
        }
    }
    
    return c;
}

QString DOMStringImpl::string() const
{
    return QString(s, l);
}

int DOMStringImpl::toInt(bool* ok) const
{
    // match \s*[+-]?\d*
    unsigned i = 0;
    while (i < l && s[i].isSpace())
        ++i;
    if (i < l && (s[i] == '+' || s[i] == '-'))
        ++i;
    while (i < l && s[i].isDigit())
        ++i;

    return QString::fromRawData(s, i).toInt(ok);
}

float DOMStringImpl::toFloat(bool* ok) const
{
    return QString::fromRawData(s, l).toFloat(ok);
}

bool DOMStringImpl::endsWith(DOMStringImpl* str, CaseSensitivity cs) const
{
    if (l < str->l) return false;
    const QChar *a = s + l - 1; 
    const QChar *b = str->s + str->l - 1; 
    int i = str->l;
    if (cs == CaseSensitive) {
        while (i--) {
            if (*a != *b) return false;
            a--, b--;
        }
    } else {
        while (i--) {
            if (a->toLower() != b->toLower()) return false;
            a--, b--;
        }
    }
    return true;
}

bool DOMStringImpl::startsWith(DOMStringImpl* str, CaseSensitivity cs) const
{
    if (l < str->l) return false;
    const QChar *a = s;
    const QChar *b = str->s;
    int i = str->l;
    if (cs == CaseSensitive) {
        while (i--) {
            if (*a != *b) return false;
            a++, b++;
        }
    } else {
        while (i--) {
            if (a->toLower() != b->toLower()) return false;
            a++, b++;
        }
    }
    return true;
}

DOMStringImpl* DOMStringImpl::substring(unsigned pos, unsigned len) const
{
    if (pos >= l)
        return 0;
    if (len > l - pos)
        len = l - pos;
    return new DOMStringImpl(s + pos, len);
}


static const unsigned short amp[] = {'&', 'a', 'm', 'p', ';'};
static const unsigned short lt[] =  {'&', 'l', 't', ';'};
static const unsigned short gt[] =  {'&', 'g', 't', ';'};
static const unsigned short nbsp[] =  {'&', 'n', 'b', 's', 'p', ';'};

DOMStringImpl *DOMStringImpl::escapeHTML()
{
    unsigned outL = 0;
    for (unsigned int i = 0; i < l; ++i ) {
        if ( s[i] == '&' )
            outL += 5; //&amp;
        else if (s[i] == '<' || s[i] == '>')
            outL += 4; //&gt;/&lt;
        else if (s[i] == QChar::Nbsp)
            outL += 6; //&nbsp;
        else
            ++outL;
    }
    if (outL == l)
        return this;

    
    DOMStringImpl* toRet = new DOMStringImpl();
    toRet->s = QT_ALLOC_QCHAR_VEC(outL);
    toRet->l = outL;

    unsigned outP = 0;
    for (unsigned int i = 0; i < l; ++i ) {
        if ( s[i] == '&' ) {
            memcpy(&toRet->s[outP], amp, sizeof(amp));
            outP += 5; 
        } else if (s[i] == '<') {
            memcpy(&toRet->s[outP], lt, sizeof(lt));
            outP += 4;
        } else if (s[i] == '>') {
            memcpy(&toRet->s[outP], gt, sizeof(gt));
            outP += 4;
        } else if (s[i] == QChar::Nbsp) {
            memcpy(&toRet->s[outP], nbsp, sizeof(nbsp));
            outP += 6;
        } else {
            toRet->s[outP] = s[i];
            ++outP;
        }
    }
    return toRet;
}

enum NoFoldTag    { DoNotFold };
enum FoldLowerTag { FoldLower };
enum FoldUpperTag { FoldUpper };

static inline
unsigned short foldChar(unsigned short c, NoFoldTag)
{
    return c;
}

static inline
unsigned short foldChar(unsigned short c, FoldLowerTag)
{
    // ### fast path for first ones?
    return QChar::toLower(c);
}

static inline
unsigned short foldChar(unsigned short c, FoldUpperTag)
{
    // ### fast path for first ones?
    return QChar::toUpper(c);
}

// Paul Hsieh's SuperFastHash
// http://www.azillionmonkeys.com/qed/hash.html

// Golden ratio - arbitrary start value to avoid mapping all 0's to all 0's
// or anything like that.
const unsigned PHI = 0x9e3779b9U;


template<typename FoldTag>
static unsigned calcHash(const QChar* s, unsigned l, FoldTag foldMode)
{
  // Note: this is originally from KJS
  unsigned hash = PHI;
  unsigned tmp;

  int rem = l & 1;
  l >>= 1;

  // Main loop
  for (; l > 0; l--) {
    hash += foldChar(s[0].unicode(), foldMode);
    tmp = (foldChar(s[1].unicode(), foldMode) << 11) ^ hash;
    hash = (hash << 16) ^ tmp;
    s += 2;
    hash += hash >> 11;
  }

  // Handle end case
  if (rem) {
    hash += foldChar(s[0].unicode(), foldMode);
    hash ^= hash << 11;
    hash += hash >> 17;
  }

  // Force "avalanching" of final 127 bits
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 2;
  hash += hash >> 15;
  hash ^= hash << 10;

  // this avoids ever returning a hash code of 0, since that is used to
  // signal "hash not computed yet", using a value that is likely to be
  // effectively the same as 0 when the low bits are masked
  if (hash == 0)
    hash = 0x80000000;

  return hash;
}

unsigned DOMStringImpl::hash() const
{
    if (m_hash != 0) return m_hash;

    return m_hash = calcHash(s, l, DoNotFold);
}

unsigned DOMStringImpl::lowerHash() const
{
    return calcHash(s, l, FoldLower);
}

unsigned DOMStringImpl::upperHash() const
{
    return calcHash(s, l, FoldUpper);
}

unsigned DOMStringImpl::computeHash(const QChar* str, unsigned int length)
{
    return calcHash(str, length, DoNotFold);
}

DOMStringImpl* DOMStringImpl::empty()
{
    static DOMString e("");
    return e.implementation();
}

bool DOM::strcasecmp(const DOMStringImpl* a, const DOMStringImpl* b)
{
    if (!(a && b))
        return (a && a->l) || (b && b->l);
    if (a->l != b->l)
        return true;
    QChar* ai = a->s;
    QChar* bi = b->s;
    int l = a->l;
    while (l--) {
        if (*ai != *bi && ai->toLower() != bi->toLower())
            return true;
        ++ai, ++bi;
    }
    return false;
}

