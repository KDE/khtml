/*
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2000-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
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

#ifndef KHTMLFONT_H
#define KHTMLFONT_H

#include <QCache>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>

#include "misc/shared.h"

namespace DOM
{
    class DOMString;
}

namespace khtml
{
class RenderStyle;
class CSSStyleSelector;

class CachedFontFamily;

class CachedFontInstance: public Shared<CachedFontInstance>
{
public:
    CachedFontInstance(CachedFontFamily* parent, int size);

    QFont        f;
    QFontMetrics fm; // note:stuff below is faster when applicable.
    // Also, do not rearrange these two fields --- one neefs f to initialize fm

    // We store cached width metrics as a two-level tree ---
    // top-level is row for the codepoint, second is column.
    // Some rows may have no information on them.
    // For a cell, value 255 means no known width (or a width >= 255)
    unsigned cachedCharWidth(unsigned short codePoint) {
        unsigned width = 0xFF;
        if (RowInfo* row = rows[codePoint >> 8])
            width = row->widths[codePoint & 0xFF];
        if (width != 0xFF)
            return width;
        else
            return calcAndCacheWidth(codePoint);
    }

    unsigned cachedCharWidth(const QChar& c) {
        return cachedCharWidth(c.unicode());
    }

    // query again all metrics
    void invalidate();

    // Simple cached metrics, set on creation
    int ascent;
    int descent;
    int height;
    int lineSpacing;
    mutable bool invalidated;

    ~CachedFontInstance();
private:
    unsigned calcAndCacheWidth(unsigned short codePoint);

    struct RowInfo
    {
        unsigned char widths[256];

        RowInfo() {
            for (int i = 0; i < 256; ++i)
                widths[i] = 0xFF;
        }
    };

    RowInfo* rows[256];
    CachedFontFamily* parent;
    int size; // the size we were created with
};

class FontDef
{
public:
    FontDef()
        : size( 0 ), italic( false ), smallCaps( false ), weight( 50 ) {}
    bool operator == ( const FontDef &other ) const {
        return ( family == other.family &&
                 size == other.size &&
                 italic == other.italic &&
                 smallCaps == other.smallCaps &&
                 weight == other.weight );
    }

    QString family;
    short int size;
    bool italic 		: 1;
    bool smallCaps 		: 1;
    unsigned int weight 		: 8;
};


class Font
{
    friend class RenderStyle;
    friend class CSSStyleSelector;

    static CachedFontInstance* defaultCFI;
public:
    static void initDefault();

    Font() : fontDef(), cfi( defaultCFI ), scFont( 0 ), letterSpacing( 0 ), wordSpacing( 0 ) {}
    Font( const FontDef &fd )
        :  fontDef( fd ), cfi( defaultCFI ), scFont( 0 ), letterSpacing( 0 ), wordSpacing( 0 )
        {}
    Font(const Font& o)
        : fontDef(o.fontDef), cfi(o.cfi), scFont(o.scFont), letterSpacing(o.letterSpacing), wordSpacing(o.wordSpacing) { if (o.scFont) scFont = new QFont(*o.scFont); }
    ~Font() { delete scFont; }

    bool operator == ( const Font &other ) const {
        return (fontDef == other.fontDef &&
                letterSpacing == other.letterSpacing &&
                wordSpacing == other.wordSpacing );
    }

    const FontDef& getFontDef() const { return fontDef; }

    void update( int logicalDpiY ) const;

    static void invalidateCachedFontFamily( const QString& familyName );
    static void markAllCachedFontsAsValid();

    /**
     * Draws a piece from the given piece of text.
     * @param p painter
     * @param x x-coordinate to begin drawing, always denotes leftmost position
     * @param y y-coordinate of baseline of text
     * @param str string to draw a piece from
     * @param slen total length of string
     * @param pos zero-based offset of beginning of piece
     * @param len length of piece
     * @param width additional pixels to be distributed equally among all
     *		spaces
     * @param d text direction
     * @param from begin with this position relative to @p pos, -1 to start
     *		at @p pos
     * @param to stop before this position relative to @p pos, -1 to use full
     *		length of piece
     * @param bg if valid, fill the background of the drawn piece with this
     *		color
     * @param uy y-coordinate of top position, used for background and text
     *		decoration painting
     * @param h total height of line, only used for background and text
     *		decoration painting
     * @param deco combined text decoration (see Decoration)
     */
    void drawText( QPainter *p, int x, int y, QChar *str, int slen, int pos, int len, int width,
                   Qt::LayoutDirection d, int from=-1, int to=-1, QColor bg=QColor(),
		   int uy=-1, int h=-1, int deco=0 ) const;

    /** returns the width of the given string chunk in pixels.
     *
     * The method also considers various styles like text-align and font-variant
     * @param str pointer to string
     * @param slen total length of string
     * @param pos zero-based position in string where to start measuring
     * @param len count of characters up to which the width should be determined
     * @param fast Use simplest/fastest algorithm (for e.g strings without special/combining chars)
     * @param start starting position of inline text box within str, only
     * used when toAdd is specified.
     * @param end ending position of inline text box within str, only
     * used when toAdd is specified.
     * @param toAdd amount of pixels to distribute evenly among all spaces of
     * str. Note that toAdd applies to all spaces within str, but only those
     * within [pos, pos+len) are counted towards the width.
     */
    int width( const QChar *str, int slen, int pos, int len, bool fast, int start=0, int end=0, int toAdd=0 ) const;
    /** return the width of the given char in pixels.
     *
     * The method also considers various styles like text-align and font-variant
     * @param str pointer to string
     * @param slen total length of string
     * @param pos zero-based position of char in string
     */
    int charWidth( const QChar *str, int slen, int pos, bool fast ) const;

    /** Text decoration constants.
     *
     * The enumeration constant values match those of ETextDecoration, but only
     * a subset is supported.
     */
    enum Decoration { UNDERLINE = 0x1, OVERLINE = 0x2, LINE_THROUGH= 0x4 };
    // Keep in sync with ETextDecoration

    /** draws text decoration
     * @param p painter
     * @param x x-coordinate
     * @param y top y-coordinate of line box
     * @param baseline baseline
     * @param width length of decoration in pixels
     * @param height height of line box
     * @param deco decoration to be drawn (see Decoration). The enumeration
     *		constants may be combined.
     */
    void drawDecoration(QPainter *p, int x, int y, int baseline, int width, int height, int deco) const;

    /** returns letter spacing
     */
    int getLetterSpacing() const { return letterSpacing; }
    /** returns word spacing
     */
    int getWordSpacing() const { return wordSpacing; }

    // for SVG
    int ascent() const { return cfi->ascent; }
    int descent() const { return cfi->descent; }
    int height() const { return cfi->height; }
    int lineSpacing() const { return cfi->lineSpacing; }
    float xHeight() const { return cfi->fm.xHeight(); }
    //FIXME: IMPLEMENT ME
    unsigned unitsPerEm() const { return 0; }
    int spaceWidth() const { return 0; }
    int tabWidth() const { return 8 * spaceWidth(); }
    
    bool isInvalidated() const { return cfi->invalidated; }
    void validate() const { cfi->invalidated = false; }

    // SVG helper function
    float floatWidth(QChar* str, int pos, int len, int extraCharsAvailable, int& charsConsumed, DOM::DOMString& glyphName) const;
    
    float floatWidth(QChar* str, int pos, int len) const;

private:
    static CachedFontFamily* queryFamily(const QString& name, int weight, bool italic);

    mutable FontDef fontDef;
    mutable WTF::SharedPtr<CachedFontInstance> cfi;
    mutable QFont *scFont;
    short letterSpacing;
    short wordSpacing;
};

} // namespace

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on; hl c++;
