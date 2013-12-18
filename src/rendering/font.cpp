/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
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

#include "font.h"

#include <config-khtml.h>

#if HAVE_ALLOCA_H
#  include <alloca.h>
#  else
#  if HAVE_MALLOC_H
#    include <malloc.h>
#  else
#    include <stdlib.h>
#  endif
#endif

#include <khtml_global.h>
#include <khtml_settings.h>

#include <QDebug>

#include <QtCore/QHash>
#include <QPainter>
#include <QFontDatabase>
#include <QtGlobal>

// for SVG
#include "dom/dom_string.h"

namespace khtml {

/** closes the current word and returns its width in pixels
 * @param fm metrics of font to be used
 * @param str string
 * @param pos zero-indexed position within @p str upon which all other
 *	indices are based
 * @param wordStart relative index pointing to the position where the word started
 * @param wordEnd relative index pointing one position after the word ended
 * @return the width in pixels. May be 0 if @p wordStart and @p wordEnd were
 *	equal.
 */
static inline int closeWordAndGetWidth(const QFontMetrics &fm, const QChar *str, int pos,
	int wordStart, int wordEnd)
{
    if (wordEnd <= wordStart) return 0;

    return fm.width(QString::fromRawData(str + pos + wordStart, wordEnd - wordStart));
}

static inline void drawDirectedText(QPainter *p, Qt::LayoutDirection d,
    int x, int y, const QString &str)
{
    QString qs = str;
    // Qt doesn't have a function to force a direction,
    // so we have to use a the unicode "RTO" character to
    //  (no, setLayoutDirection isn't enough)
    if (d == Qt::RightToLeft && str[0].direction() == QChar::DirL)
    {
        qs.prepend(QChar(0x202E)); // RIGHT-TO-LEFT OVERRIDE
    }
    else if (d == Qt::LeftToRight && str[0].direction() == QChar::DirR)
    {
        qs.prepend(QChar(0x202D)); // LEFT-TO-RIGHT OVERRIDE
    }
    
    Qt::LayoutDirection saveDir = p->layoutDirection();
    p->setLayoutDirection(d);
    // Qt 4 avoids rendering soft-hyphens by default.
    // Append normal hyphen instead.
    if (qs.endsWith(QChar(0xad)))
        qs.append(QChar('-'));
    p->drawText(x, y, qs);
    p->setLayoutDirection(saveDir);
}

/** closes the current word and draws it
 * @param p painter
 * @param d text direction
 * @param x current x position, will be inc-/decremented correctly according
 *	to text direction
 * @param y baseline of text
 * @param widths list of widths; width of word is expected at position
 *		wordStart
 * @param str string
 * @param pos zero-indexed position within @p str upon which all other
 *	indices are based
 * @param wordStart relative index pointing to the position where the word started,
 *	will be set to wordEnd after function
 * @param wordEnd relative index pointing one position after the word ended
 */
static inline void closeAndDrawWord(QPainter *p, Qt::LayoutDirection d,
	int &x, int y, const short widths[], const QChar *str, int pos,
	int &wordStart, int wordEnd)
{
    if (wordEnd <= wordStart) return;

    int width = widths[wordStart];
    if (d == Qt::RightToLeft)
      x -= width;

    drawDirectedText( p, d, x, y, QString::fromRawData(str + pos + wordStart, wordEnd - wordStart) );

    if (d != Qt::RightToLeft)
      x += width;

    wordStart = wordEnd;
}

void Font::drawText( QPainter *p, int x, int y, QChar *str, int slen, int pos, int len,
        int toAdd, Qt::LayoutDirection d, int from, int to, QColor bg, int uy, int h, int deco ) const
{
    if (!str) return;
    QString qstr = QString::fromRawData(str, slen);

    const QFontMetrics& fm = cfi->fm;

    // ### fixme for RTL
    if ( !scFont && !letterSpacing && !wordSpacing && !toAdd && from==-1 ) {
	// simply draw it
	// Due to some unfounded cause QPainter::drawText traverses the
        // *whole* string when painting, not only the specified
        // [pos, pos + len) segment. This makes painting *extremely* slow for
        // long render texts (in the order of several megabytes).
        // Hence, only hand over the piece of text of the actual inline text box
	drawDirectedText( p, d, x, y, QString::fromRawData(str + pos, len) );
    } else {
	if (from < 0) from = 0;
	if (to < 0) to = len;

	int numSpaces = 0;
	if ( toAdd ) {
	    for( int i = 0; i < len; ++i )
		if ( str[i+pos].category() == QChar::Separator_Space )
		    ++numSpaces;
	}

	if ( d == Qt::RightToLeft ) {
	    const int totWidth = width( str, slen, pos, len, false /*fast algo*/ );
	    x += totWidth + toAdd;
	}
	QString upper = qstr;
	QFontMetrics sc_fm = fm;
	if ( scFont ) {
	    // draw in small caps
	    upper = qstr.toUpper();
	    sc_fm = QFontMetrics( *scFont );
	}

	// ### sc could be optimized by only painting uppercase letters extra,
	// and treat the rest WordWise, but I think it's not worth it.
	// Somebody else may volunteer, and implement this ;-) (LS)

	// The mode determines whether the text is displayed character by
	// character, word by word, or as a whole
	enum { CharacterWise, WordWise, Whole }
	mode = Whole;
	if (!letterSpacing && !scFont && (wordSpacing || toAdd > 0))
	  mode = WordWise;
	else if (letterSpacing || scFont)
	  mode = CharacterWise;

	if (mode == Whole) {	// most likely variant is treated extra

	    if (to < 0) to = len;
	    const QString segStr(QString::fromRawData(str + pos + from, to - from));
	    const int preSegmentWidth = fm.width(QString::fromRawData(str + pos, len), from);
	    const int segmentWidth = fm.width(segStr);
	    const int eff_x = d == Qt::RightToLeft ? x - preSegmentWidth - segmentWidth
					: x + preSegmentWidth;

	    // draw whole string segment, with optional background
	    if ( bg.isValid() )
		p->fillRect( eff_x, uy, segmentWidth, h, bg );
	    drawDirectedText( p, d, eff_x, y, segStr );
	    if (deco)
	        drawDecoration(p, eff_x, uy, y - uy, segmentWidth - 1, h, deco);
	    return;
	}

	// We are using two passes. In the first pass, the widths are collected,
	// and stored. In the second, the actual characters are drawn.

	// For each letter in the text box, save the width of the character.
	// When word-wise, only the first letter contains the width, but of the
	// whole word.
	short* const widthList = (short *)alloca((to+1)*sizeof(short));

	// First pass: gather widths
	int preSegmentWidth = 0;
	int segmentWidth = 0;
        int lastWordBegin = 0;
	bool onSegment = from == 0;
	for( int i = 0; i < to; ++i ) {
	    if (i == from) {
                // Also close words on visibility boundary
	        if (mode == WordWise) {
	            const int width = closeWordAndGetWidth(fm, str, pos, lastWordBegin, i);

		    if (lastWordBegin < i) {
		        widthList[lastWordBegin] = (short)width;
		        lastWordBegin = i;
		        preSegmentWidth += width;
		    }
		}
		onSegment = true;
	    }

	    const QChar ch = str[pos+i];
	    bool lowercase = (ch.category() == QChar::Letter_Lowercase);
	    bool is_space = (ch.category() == QChar::Separator_Space);
	    int chw = 0;
	    if ( letterSpacing )
		chw += letterSpacing;
	    if ( (wordSpacing || toAdd) && is_space ) {
	        if (mode == WordWise) {
		    const int width = closeWordAndGetWidth(fm, str, pos, lastWordBegin, i);
		    if (lastWordBegin < i) {
		        widthList[lastWordBegin] = (short)width;
			lastWordBegin = i;
		        (onSegment ? segmentWidth : preSegmentWidth) += width;
		    }
		    ++lastWordBegin;		// ignore this space
		}
		chw += wordSpacing;
		if ( numSpaces ) {
		    const int a = toAdd/numSpaces;
		    chw += a;
		    toAdd -= a;
		    --numSpaces;
		}
	    }
	    if (is_space || mode == CharacterWise) {
	        chw += lowercase ? sc_fm.charWidth( upper, pos+i ) : fm.charWidth( qstr, pos+i );
		widthList[i] = (short)chw;

		(onSegment ? segmentWidth : preSegmentWidth) += chw;
	    }

	}

	// close last word
	Q_ASSERT(onSegment);
	if (mode == WordWise) {
	    const int width = closeWordAndGetWidth(fm, str, pos, lastWordBegin, to);
	    segmentWidth += width;
	    widthList[lastWordBegin] = (short)width;
	}

        if (d == Qt::RightToLeft) x -= preSegmentWidth;
	else x += preSegmentWidth;

        const int startx = d == Qt::RightToLeft ? x-segmentWidth : x;

	// optionally draw background
	if ( bg.isValid() )
	    p->fillRect( startx, uy, segmentWidth, h, bg );

	// second pass: do the actual drawing
        lastWordBegin = from;
	for( int i = from; i < to; ++i ) {
	    const QChar ch = str[pos+i];
	    bool lowercase = (ch.category() == QChar::Letter_Lowercase);
	    bool is_space = ch.category() == QChar::Separator_Space;
	    if ( is_space ) {
	        if (mode == WordWise) {
		    closeAndDrawWord(p, d, x, y, widthList, str, pos, lastWordBegin, i);
		    ++lastWordBegin;	// jump over space
		}
	    }
	    if (is_space || mode == CharacterWise) {
	        const int chw = widthList[i];
	        if (d == Qt::RightToLeft)
		    x -= chw;

		if ( scFont )
		    p->setFont( lowercase ? *scFont : cfi->f );

		 drawDirectedText( p, d, x, y, QString((lowercase ? upper : qstr)[pos+i]) );
#ifdef __GNUC__
  #warning "Light bloatery"
#endif

	        if (d != Qt::RightToLeft)
		    x += chw;
	    }

	}

	// don't forget to draw last word
	if (mode == WordWise) {
	    closeAndDrawWord(p, d, x, y, widthList, str, pos, lastWordBegin, to);
	}

	if (deco)
	    drawDecoration(p, startx, uy, y - uy, segmentWidth - 1, h, deco);

	if ( scFont )
	    p->setFont( cfi->f );
    }
}


int Font::width( const QChar *chs, int, int pos, int len, bool fast,int start, int end, int toAdd ) const
{
   if (!len) return 0;
   int w = 0;

   // #### Qt 4 has a major speed regression : QFontMetrics::width() is around 15 times slower than Qt 3's.
   // This is a great speed bottleneck as we are now spending up to 70% of the layout time in that method
   // (compared to around 5% before).
   // It as been reported to TT and acknowledged as issue N138867, but whether they intend to give it some
   // care in the near future is unclear :-/
   // 
   // #### Qt 4.4 RC is now *40* times slower than Qt 3.3. This is a complete and utter disaster.
   // New issue about this as N203591, because the report from 2006 was apparently discarded.
   //
   // This issue is now mostly addressed, by first scanning strings for complex/combining unicode characters,
   // and using the much faster, non-context-aware QFontMetrics::width(QChar) when none has been found.
   //
   // ... Even that, however, is ultra-slow with Qt4.5, so now we cache this width information as well.
    QFontMetrics& fm = cfi->fm;
    if ( scFont ) {
        const QString qstr = QString::fromRawData(chs+pos, len);
	const QString upper = qstr.toUpper();
	const QChar *uc = qstr.unicode();
	const QFontMetrics sc_fm( *scFont );
	if (fast) {
            for ( int i = 0; i < len; ++i ) {
	        if ( (uc+i)->category() == QChar::Letter_Lowercase )
		   w += sc_fm.width( upper[i] );
	        else
		    w += cfi->cachedCharWidth( qstr[i] );
            }
	} else {
            for ( int i = 0; i < len; ++i ) {
	        if ( (uc+i)->category() == QChar::Letter_Lowercase )
		   w += sc_fm.charWidth( upper, i );
	        else
		    w += fm.charWidth( qstr, i );
            }
        }
    } else {
	if (fast) {
            for ( int i = 0; i < len; ++i ) {
                w += cfi->cachedCharWidth( chs[i+pos] );
            }
        } else {
            const QString qstr = QString::fromRawData(chs+pos, len);
            w = fm.width( qstr );
        }
    }

    if ( letterSpacing )
	w += len*letterSpacing;

    if ( wordSpacing )
	// add amount
	for( int i = 0; i < len; ++i ) {
	    if( chs[i+pos].category() == QChar::Separator_Space )
		w += wordSpacing;
	}

    if ( toAdd ) {
        // first gather count of spaces
        int numSpaces = 0;
        for( int i = start; i != end; ++i )
            if ( chs[i].category() == QChar::Separator_Space )
                ++numSpaces;
        // distribute pixels evenly among spaces, but count only those within
        // [pos, pos+len)
        for ( int i = start; numSpaces && i != pos + len; i++ )
            if ( chs[i].category() == QChar::Separator_Space ) {
                const int a = toAdd/numSpaces;
                if ( i >= pos ) w += a;
                toAdd -= a;
                --numSpaces;
            }
    }

    return w;
}

int Font::charWidth( const QChar *chs, int slen, int pos, bool fast ) const
{
    int w;
	if ( scFont && chs[pos].category() == QChar::Letter_Lowercase ) {
	    QString str( chs, slen );
	    str[pos] = chs[pos].toUpper();
	    if (fast)
	        w = QFontMetrics( *scFont ).width( str[pos] );
            else
	        w = QFontMetrics( *scFont ).charWidth( str, pos );
	} else {
	    if (fast)
	        w = cfi->cachedCharWidth( chs[pos] );
	    else
	        w = cfi->fm.charWidth( QString::fromRawData( chs, slen ), pos );
	}
    if ( letterSpacing )
	w += letterSpacing;

    if ( wordSpacing && (chs+pos)->category() == QChar::Separator_Space )
		w += wordSpacing;
    return w;
}

/**
 We cache information on fonts, including what sizes they are scalable too,
 and their metrics since getting that info out of Qt is slow
*/

struct CachedFontFamilyKey // basically, FontDef minus the size (and for now SC)
{
    QString family;
    int     weight;
    bool    italic;

    CachedFontFamilyKey() {}

    CachedFontFamilyKey(const QString& resolvedFamily, int weight, bool italic) :
            family(resolvedFamily), weight(weight), italic(italic)
    {}

    bool operator == (const CachedFontFamilyKey& other) const {
        return (family == other.family) &&
               (weight == other.weight) &&
               (italic == other.italic);
    }
};

static inline uint qHash (const CachedFontFamilyKey& key) {
    return ::qHash(key.family) ^ ::qHash(key.weight) ^ ::qHash(key.italic);
}

class CachedFontFamily
{
public:
    CachedFontFamilyKey def;

    bool scaleable;
    QList<int> sizes; // if bitmap.

    QHash<int, CachedFontInstance*> instances;
    // Maps from pixel size.. This is a weak-reference --- the refcount
    // on the CFI itself determines the lifetime

    CachedFontInstance* queryFont(int pixelSize);
    void invalidateAllInstances();
    void markAllInstancesAsValid();
};

static QHash<CachedFontFamilyKey, CachedFontFamily*>* fontCache;
// This is a hash and not a cache since the top-level info is tiny,
// and the actual font instances have precise lifetime

void Font::invalidateCachedFontFamily( const QString& familyName ) // static
{
    if (!fontCache)
        return;
    QHash<CachedFontFamilyKey, CachedFontFamily*>::const_iterator i;
    QHash<CachedFontFamilyKey, CachedFontFamily*>::const_iterator end = fontCache->constEnd();
    for (i = fontCache->constBegin(); i != end; ++i) {
        if (i.key().family.contains( familyName, Qt::CaseInsensitive )) {
            i.value()->invalidateAllInstances();
        }
    }
}

void Font::markAllCachedFontsAsValid() // static
{
    if (!fontCache)
        return;
    QHash<CachedFontFamilyKey, CachedFontFamily*>::const_iterator i;
    QHash<CachedFontFamilyKey, CachedFontFamily*>::const_iterator end = fontCache->constEnd();
    for (i = fontCache->constBegin(); i != end; ++i) {
            i.value()->markAllInstancesAsValid();
    }
}

CachedFontFamily* Font::queryFamily(const QString& name, int weight, bool italic)
{
    if (!fontCache)
        fontCache = new QHash<CachedFontFamilyKey, CachedFontFamily*>;

    CachedFontFamilyKey key(name, weight, italic);

    CachedFontFamily* f = fontCache->value(key);
    if (!f) {
	// To query the sizes, we seem to have to make a font with right style to produce the stylestring
	QFont font(name);
	font.setItalic(italic);
	font.setWeight(weight);

	QFontDatabase db;
	QString styleString = db.styleString(font);
	f = new CachedFontFamily;
	f->def       = key;
	f->scaleable = db.isSmoothlyScalable(font.family(), styleString);

	if (!f->scaleable) {
	    /* Cache size info */
	    f->sizes = db.smoothSizes(font.family(), styleString);
	}

	fontCache->insert(key, f);
    }

    return f;
}


CachedFontInstance* CachedFontFamily::queryFont(int pixelSize)
{
    CachedFontInstance* cfi = instances.value(pixelSize);
    if (!cfi) {
	cfi = new CachedFontInstance(this, pixelSize);
	instances.insert(pixelSize, cfi);
    }
    return cfi;
}

void CachedFontFamily::invalidateAllInstances()
{
    QHash<int, CachedFontInstance*>::const_iterator i;
    QHash<int, CachedFontInstance*>::const_iterator end = instances.constEnd();
    for (i = instances.constBegin(); i != end; ++i) {
        i.value()->invalidate();
    }
}

void CachedFontFamily::markAllInstancesAsValid()
{
    QHash<int, CachedFontInstance*>::const_iterator i;
    QHash<int, CachedFontInstance*>::const_iterator end = instances.constEnd();
    for (i = instances.constBegin(); i != end; ++i) {
        i.value()->invalidated = false;
    }
}

CachedFontInstance::CachedFontInstance(CachedFontFamily* p, int sz):
    f(p->def.family), fm(f), invalidated(false), parent(p), size(sz)
{
    f.setItalic(p->def.italic);
    f.setWeight(p->def.weight);
    f.setPixelSize(sz);
    fm = QFontMetrics(f);

    // Prepare metrics caches
    for (int c = 0; c < 256; ++c)
	rows[c] = 0;

    ascent  = fm.ascent();
    descent = fm.descent();
    height  = fm.height();
    lineSpacing = fm.lineSpacing();
}

void CachedFontInstance::invalidate()
{
    QFont nf( f.family() );
    nf.setWeight( f.weight() );
    nf.setItalic( f.italic() );
    nf.setPixelSize( f.pixelSize() );
    f = nf;
    invalidated = true;
    fm = QFontMetrics(f);

    // Cleanup metrics caches
    for (int c = 0; c < 256; ++c) {
        delete rows[c];
        rows[c] = 0;
    }

    ascent  = fm.ascent();
    descent = fm.descent();
    height  = fm.height();
    lineSpacing = fm.lineSpacing();
}

CachedFontInstance::~CachedFontInstance()
{
    for (int c = 0; c < 256; ++c)
	delete rows[c];
    parent->instances.remove(size);
}

unsigned CachedFontInstance::calcAndCacheWidth(unsigned short codePoint)
{
    unsigned rowNum = codePoint >> 8;
    RowInfo* row = rows[rowNum];
    if (!row)
	row = rows[rowNum] = new RowInfo();

    unsigned width = fm.width(QChar(codePoint));
    return (row->widths[codePoint & 0xFF] = qMin(width, 0xFFu));
}

void Font::update(int logicalDpiY) const
{
    QString familyName = fontDef.family.isEmpty() ? KHTMLGlobal::defaultHTMLSettings()->stdFontName() : fontDef.family;
    CachedFontFamily* family = queryFamily(familyName, fontDef.weight, fontDef.italic);

    int size = fontDef.size;
    const int lDpiY = qMax(logicalDpiY, 96);

    // ok, now some magic to get a nice unscaled font
    // all other font properties should be set before this one!!!!
    if (!family->scaleable)
    {
        const QList<int> pointSizes = family->sizes;
        // lets see if we find a nice looking font, which is not too far away
        // from the requested one.
        // qDebug() << "khtml::setFontSize family = " << f.family() << " size requested=" << size;

        float diff = 1; // ### 100% deviation
        float bestSize = 0;

        QList<int>::ConstIterator it = pointSizes.begin();
        const QList<int>::ConstIterator itEnd = pointSizes.end();

        for( ; it != itEnd; ++it )
        {
            float newDiff = ((*it)*(lDpiY/72.) - float(size))/float(size);
            //qDebug() << "smooth font size: " << *it << " diff=" << newDiff;
            if(newDiff < 0) newDiff = -newDiff;
            if(newDiff < diff)
            {
                diff = newDiff;
                bestSize = *it;
            }
        }
        //qDebug() << "best smooth font size: " << bestSize << " diff=" << diff;
        if ( bestSize != 0 && diff < 0.2 ) // 20% deviation, otherwise we use a scaled font...
            size = (int)((bestSize*lDpiY) / 72);
    }

    // make sure we don't bust up X11
    // Also, Qt does not support sizing a QFont to zero.
    size = qMax(1, qMin(255, size));

//       qDebug("setting font to %s, italic=%d, weight=%d, size=%d", fontDef.family.toLatin1().constData(), fontDef.italic,
//    	   fontDef.weight, size );

    // Now request the font from the family
    cfi = family->queryFont(size);

    // small caps
    delete scFont;
    scFont = 0;

    if ( fontDef.smallCaps ) {
	scFont = new QFont( cfi->f );
	scFont->setPixelSize( qMax(1, cfi->f.pixelSize()*7/10) );
    }
}

CachedFontInstance* Font::defaultCFI;

void Font::initDefault()
{
    if (defaultCFI)
	return;

    // Create one for default family. It doesn't matter what size and DPI we use
    // since this is only used for throwaway computations
    // ### this may cache an instance we don't need though; but font family
    // is extremely likely to be used
    Font f;
    f.fontDef.size = 10;
    f.update(96);

    defaultCFI = f.cfi.get();
    defaultCFI->ref();
}

void Font::drawDecoration(QPainter *pt, int _tx, int _ty, int baseline, int width, int height, int deco) const
{
    Q_UNUSED(height);

    // thick lines on small fonts look ugly
    const int thickness = cfi->height > 20 ? cfi->fm.lineWidth() : 1;
    const QBrush brush = pt->pen().color();
    if (deco & UNDERLINE) {
        int underlineOffset = ( cfi->height + baseline ) / 2;
        if (underlineOffset <= baseline) underlineOffset = baseline+1;

        pt->fillRect(_tx, _ty + underlineOffset, width + 1, thickness, brush );
    }
    if (deco & OVERLINE) {
        pt->fillRect(_tx, _ty, width + 1, thickness, brush );
    }
    if (deco & LINE_THROUGH) {
        pt->fillRect(_tx, _ty + 2*baseline/3, width + 1, thickness, brush );
    }
}

// WebCore SVG
float Font::floatWidth(QChar* str, int pos, int len, int /*extraCharsAvailable*/, int& charsConsumed, DOM::DOMString& glyphName) const
{
    charsConsumed = len;
    glyphName = "";
    // ### see if svg can scan the string (cf. render_text.cpp - isSimpleChar()) to determine if fast algo can be used.
    return width(str, 0, pos, len, false /*fast algorithm*/);
}

float Font::floatWidth(QChar* str, int pos, int len) const
{
    // For now, this is slow but correct...
    // or rather it /would/ be correct if QFontMetricsF had charWidth(); 
    // so instead the approximate width is used
    QFontMetricsF fm(cfi->f);
    return float(fm.width(QString::fromRawData(str + pos, len)));
}

}

// kate: indent-width 4; replace-tabs off; tab-width 8; space-indent on; hl c++;
