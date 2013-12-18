/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2000-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2003-2007 Apple Computer, Inc.
 *           (C) 2004-2006 Allan Sandfeld Jensen (kde@carewolf.com)
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
#ifndef RENDERSTYLE_H
#define RENDERSTYLE_H

/*
 * WARNING:
 * --------
 *
 * The order of the values in the enums have to agree with the order specified
 * in cssvalues.in, otherwise some optimizations in the parser will fail,
 * and produce invaliud results.
 */

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPalette>
#include <QApplication>

#include "dom/dom_misc.h"
#include "dom/dom_string.h"
#include "misc/khtmllayout.h"
#include "misc/shared.h"
#include "rendering/DataRef.h"
#include "rendering/font.h"
#include "rendering/SVGRenderStyle.h"

#include <assert.h>


#define SET_VAR(group,variable,value) \
    if (!(group->variable == value)) \
        group.access()->variable = value;

#ifndef ENABLE_DUMP
#ifndef NDEBUG
#define ENABLE_DUMP 1
#endif
#endif

namespace DOM {
    class DOMStringImpl;
    class QuotesValueImpl;
    class CounterImpl;
    class CSSValueListImpl;
}

namespace khtml {

    class CachedImage;
    class CachedObject;


//------------------------------------------------

//------------------------------------------------
// Box model attributes. Not inherited.

struct LengthBox
{
    LengthBox()
    {
    }
    LengthBox( LengthType t )
	: left( t ), right ( t ), top( t ), bottom( t ) {}

    Length left;
    Length right;
    Length top;
    Length bottom;
    Length& operator=(Length& len)
    {
    	left=len;
	right=len;
	top=len;
	bottom=len;
	return len;
    }

    bool operator==(const LengthBox& o) const
    {
    	return left==o.left && right==o.right && top==o.top && bottom==o.bottom;
    }


    bool nonZero() const { return !(left.isZero() && right.isZero() && top.isZero() && bottom.isZero()); }
};



enum EPosition {
    PSTATIC, PRELATIVE, PABSOLUTE, PFIXED
};

enum EFloat {
    FNONE = 0, FLEFT = 0x01, FRIGHT = 0x02, FLEFT_ALIGN = 0x05, FRIGHT_ALIGN = 0x06
};

enum EWordWrap {
    WWNORMAL = 0, WWBREAKWORD = 0x01
};

//------------------------------------------------
// Border attributes. Not inherited.


// These have been defined in the order of their precedence for border-collapsing. Do
// not change this order!
enum EBorderStyle {
    BNATIVE, BNONE, BHIDDEN, INSET, GROOVE, RIDGE, OUTSET, DOTTED, DASHED, SOLID, DOUBLE
};

class BorderValue
{
public:
    BorderValue() : width( 3 ), style( BNONE ) {}

    QColor color;
    unsigned short width : 12;
    EBorderStyle style : 6;

    bool nonZero(bool checkStyle = true) const {
        return width != 0 && !(checkStyle && style == BNONE);
    }

    bool isTransparent() const {
        return color.isValid() && color.alpha() == 0;
    }

    bool operator==(const BorderValue& o) const
    {
    	return width==o.width && style==o.style && color==o.color;
    }

    bool operator!=(const BorderValue& o) const
    {
        return !(*this == o);
    }
};

class OutlineValue : public BorderValue
{
public:
    OutlineValue() : _offset(0), _auto(false) {}

    bool operator==(const OutlineValue& o) const
    {
        return width==o.width && style==o.style && color==o.color && _offset == o._offset && _auto == o._auto;
    }

    bool operator!=(const OutlineValue& o) const
    {
        return !(*this == o);
    }

    int _offset;
    bool _auto;
};

enum EBorderPrecedence { BOFF, BTABLE, BCOLGROUP, BCOL, BROWGROUP, BROW, BCELL };

struct CollapsedBorderValue
{
    CollapsedBorderValue() :border(0), precedence(BOFF) {}
    CollapsedBorderValue(const BorderValue* b, EBorderPrecedence p) :border(b), precedence(p) {}

    int width() const { return border && border->nonZero() ? border->width : 0; }
    EBorderStyle style() const { return border ? border->style : BHIDDEN; }
    bool exists() const { return border; }
    QColor color() const { return border ? border->color : QColor(); }
    bool isTransparent() const { return border ? border->isTransparent() : true; }

    bool operator==(const CollapsedBorderValue& o) const
    {
        if (!border) return !o.border;
        if (!o.border) return false;
        return *border == *o.border && precedence == o.precedence;
    }

    const BorderValue* border;
    EBorderPrecedence precedence;
};

class BorderData : public Shared<BorderData>
{
public:
    BorderData() : Shared< khtml::BorderData >() {};
    BorderData(const khtml::BorderData& other) : Shared<khtml::BorderData>() {
        this->left = other.left;
        this->right = other.right;
        this->top = other.top;
        this->bottom = other.bottom;
    }

    BorderValue left;
    BorderValue right;
    BorderValue top;
    BorderValue bottom;

    bool hasBorder() const
    {
    	return left.nonZero() || right.nonZero() || top.nonZero() || bottom.nonZero();
    }

    unsigned short borderLeftWidth() const {
        if (left.style == BNONE || left.style == BHIDDEN || left.style == BNATIVE)
            return 0;
        return left.width;
    }

    unsigned short borderRightWidth() const {
        if (right.style == BNONE || right.style == BHIDDEN || right.style == BNATIVE)
            return 0;
        return right.width;
    }

    unsigned short borderTopWidth() const {
        if (top.style == BNONE || top.style == BHIDDEN || top.style == BNATIVE)
            return 0;
        return top.width;
    }

    unsigned short borderBottomWidth() const {
        if (bottom.style == BNONE || bottom.style == BHIDDEN || bottom.style == BNATIVE)
            return 0;
        return bottom.width;
    }

    bool operator==(const BorderData& o) const
    {
    	return left==o.left && right==o.right && top==o.top && bottom==o.bottom;
    }

};

class StyleSurroundData : public Shared<StyleSurroundData>
{
public:
    StyleSurroundData();

    StyleSurroundData(const StyleSurroundData& o );
    bool operator==(const StyleSurroundData& o) const;
    bool operator!=(const StyleSurroundData& o) const {
        return !(*this == o);
    }
    bool hasSamePBMData(const StyleSurroundData& o) const {
        return (margin == o.margin) && (padding == o.padding) && (border == o.border);
    }

    LengthBox offset;
    LengthBox margin;
    LengthBox padding;
    BorderData border;
};


//------------------------------------------------
// Box attributes. Not inherited.

enum EBoxSizing {
    BORDER_BOX, CONTENT_BOX
};

class StyleBoxData : public Shared<StyleBoxData>
{
public:
    StyleBoxData();

    StyleBoxData(const StyleBoxData& o );


    // copy and assignment
//    StyleBoxData(const StyleBoxData &other);
//    const StyleBoxData &operator = (const StyleBoxData &other);

    bool operator==(const StyleBoxData& o) const;
    bool operator!=(const StyleBoxData& o) const {
        return !(*this == o);
    }

    Length width;
    Length height;

    Length min_width;
    Length max_width;

    Length min_height;
    Length max_height;

    Length vertical_align;

    EBoxSizing box_sizing;

    signed int z_index :31;
    bool z_auto        : 1;
};

//------------------------------------------------
// Random visual rendering model attributes. Not inherited.

enum EOverflow {
    OVISIBLE, OHIDDEN, OSCROLL, OAUTO, OMARQUEE
};

enum EVerticalAlign {
    BASELINE, MIDDLE, SUB, SUPER, TEXT_TOP,
    TEXT_BOTTOM, TOP, BOTTOM, BASELINE_MIDDLE, LENGTH
};

enum EClear{
    CNONE = 0, CLEFT = 1, CRIGHT = 2, CBOTH = 3
};

enum ETableLayout {
    TAUTO, TFIXED
};

enum EUnicodeBidi {
    UBNormal, Embed, Override
};

class StyleVisualData : public Shared<StyleVisualData>
{
public:
    StyleVisualData();

    ~StyleVisualData();

    StyleVisualData(const StyleVisualData& o );

    bool operator==( const StyleVisualData &o ) const {
	return ( clip == o.clip && textDecoration == o.textDecoration &&
		 palette == o.palette );
    }
    bool operator!=( const StyleVisualData &o ) const {
        return !(*this == o);
    }

    LengthBox clip;
    unsigned textDecoration : 4; // Text decorations defined *only* by this element.

    QPalette palette;      //widget styling with IE attributes

};

//------------------------------------------------
enum EBackgroundBox {
    BGBORDER, BGPADDING, BGCONTENT
};

enum EBackgroundRepeat {
    REPEAT, REPEAT_X, REPEAT_Y, NO_REPEAT
};

enum EBackgroundSizeType {
    BGSLENGTH, BGSCONTAIN, BGSCOVER
};

enum EBackgroundAttachment {
    BGASCROLL, BGAFIXED, BGALOCAL
};

struct LengthSize {
    Length width;
    Length height;
};

struct BackgroundLayer {
public:
    BackgroundLayer();
    ~BackgroundLayer();

    CachedImage* backgroundImage() const { return m_image; }
    Length backgroundXPosition() const { return m_xPosition; }
    Length backgroundYPosition() const { return m_yPosition; }
    EBackgroundAttachment backgroundAttachment() const { return KDE_CAST_BF_ENUM(EBackgroundAttachment, m_bgAttachment); }
    EBackgroundBox backgroundClip() const { return KDE_CAST_BF_ENUM(EBackgroundBox, m_bgClip); }
    EBackgroundBox backgroundOrigin() const { return KDE_CAST_BF_ENUM(EBackgroundBox, m_bgOrigin); }
    EBackgroundRepeat backgroundRepeat() const { return KDE_CAST_BF_ENUM(EBackgroundRepeat, m_bgRepeat); }
    LengthSize backgroundSize() const { return m_backgroundSize; }
    EBackgroundSizeType backgroundSizeType() const { return KDE_CAST_BF_ENUM(EBackgroundSizeType, m_bgSizeType); }

    BackgroundLayer* next() const { return m_next; }
    BackgroundLayer* next() { return m_next; }

    bool isBackgroundImageSet() const { return m_imageSet; }
    bool isBackgroundXPositionSet() const { return m_xPosSet; }
    bool isBackgroundYPositionSet() const { return m_yPosSet; }
    bool isBackgroundAttachmentSet() const { return m_attachmentSet; }
    bool isBackgroundClipSet() const { return m_clipSet; }
    bool isBackgroundOriginSet() const { return m_originSet; }
    bool isBackgroundRepeatSet() const { return m_repeatSet; }
    bool isBackgroundSizeSet() const { return m_backgroundSizeSet; }

    void setBackgroundImage(CachedImage* i) { m_image = i; m_imageSet = true; }
    void setBackgroundXPosition(const Length& l) { m_xPosition = l; m_xPosSet = true; }
    void setBackgroundYPosition(const Length& l) { m_yPosition = l; m_yPosSet = true; }
    void setBackgroundAttachment(EBackgroundAttachment b) { m_bgAttachment = b; m_attachmentSet = true; }
    void setBackgroundClip(EBackgroundBox b) { m_bgClip = b; m_clipSet = true; }
    void setBackgroundOrigin(EBackgroundBox b) { m_bgOrigin = b; m_originSet = true; }
    void setBackgroundRepeat(EBackgroundRepeat r) { m_bgRepeat = r; m_repeatSet = true; }
    void setBackgroundSize(const LengthSize& b) { m_backgroundSize = b; m_backgroundSizeSet = true; }
    void setBackgroundSizeType(EBackgroundSizeType t) { m_bgSizeType = t; m_backgroundSizeSet = true; }

    void clearBackgroundImage() { m_imageSet = false; }
    void clearBackgroundXPosition() { m_xPosSet = false; }
    void clearBackgroundYPosition() { m_yPosSet = false; }
    void clearBackgroundAttachment() { m_attachmentSet = false; }
    void clearBackgroundClip() { m_clipSet = false; }
    void clearBackgroundOrigin() { m_originSet = false; }
    void clearBackgroundRepeat() { m_repeatSet = false; }
    void clearBackgroundSize() { m_backgroundSizeSet = false; }

    void setNext(BackgroundLayer* n) { if (m_next != n) { delete m_next; m_next = n; } }

    BackgroundLayer& operator=(const BackgroundLayer& o);
    BackgroundLayer(const BackgroundLayer& o);

    bool operator==(const BackgroundLayer& o) const;
    bool operator!=(const BackgroundLayer& o) const {
        return !(*this == o);
    }

    bool containsImage(CachedImage* c) const { if (c == m_image) return true; if (m_next) return m_next->containsImage(c); return false; }

    bool hasImage() const {
        if (m_image)
            return true;
        return m_next ? m_next->hasImage() : false;
    }
    bool hasFixedImage() const {
        if (m_image && m_bgAttachment == BGAFIXED)
            return true;
        return m_next ? m_next->hasFixedImage() : false;
    }

    void fillUnsetProperties();
    void cullEmptyLayers();

    CachedImage* m_image;

    Length m_xPosition;
    Length m_yPosition;

    KDE_BF_ENUM(EBackgroundAttachment) m_bgAttachment : 2;
    KDE_BF_ENUM(EBackgroundBox) m_bgClip : 2;
    KDE_BF_ENUM(EBackgroundBox) m_bgOrigin : 2;
    KDE_BF_ENUM(EBackgroundRepeat) m_bgRepeat : 2;
    KDE_BF_ENUM(EBackgroundSizeType) m_bgSizeType : 2;

    LengthSize m_backgroundSize;

    bool m_imageSet : 1;
    bool m_attachmentSet : 1;
    bool m_clipSet : 1;
    bool m_originSet : 1;
    bool m_repeatSet : 1;
    bool m_xPosSet : 1;
    bool m_yPosSet : 1;
    bool m_backgroundSizeSet : 1;

    BackgroundLayer* m_next;
};

class StyleBackgroundData : public Shared<StyleBackgroundData>
{
public:
    StyleBackgroundData();
    ~StyleBackgroundData() {}
    StyleBackgroundData(const StyleBackgroundData& o );

    bool operator==(const StyleBackgroundData& o) const;
    bool operator!=(const StyleBackgroundData &o) const {
	return !(*this == o);
    }

    BackgroundLayer m_background;
    QColor m_color;
    OutlineValue m_outline;
};

enum EQuoteContent {
    NO_QUOTE = 0, OPEN_QUOTE, CLOSE_QUOTE, NO_OPEN_QUOTE, NO_CLOSE_QUOTE
};

enum ContentType {
    CONTENT_NONE = 0, CONTENT_NORMAL, CONTENT_OBJECT,
    CONTENT_TEXT, CONTENT_COUNTER, CONTENT_QUOTE
};

struct ContentData {
    ContentData() : _contentType( CONTENT_NONE ), _nextContent(0) {}
    ContentData(const ContentData& o);
    ~ContentData();
    void clearContent();

    DOM::DOMStringImpl* contentText()
    { if (_contentType == CONTENT_TEXT) return _content.text; return 0; }
    CachedObject* contentObject()
    { if (_contentType == CONTENT_OBJECT) return _content.object; return 0; }
    DOM::CounterImpl* contentCounter()
    { if (_contentType == CONTENT_COUNTER) return _content.counter; return 0; }
    EQuoteContent contentQuote()
    { if (_contentType == CONTENT_QUOTE) return _content.quote; return NO_QUOTE; }

    ContentType _contentType;

    union {
        CachedObject* object;
        DOM::DOMStringImpl* text;
        DOM::CounterImpl* counter;
        EQuoteContent quote;
    } _content ;

    ContentData* _nextContent;
};

class StyleGeneratedData : public Shared<StyleGeneratedData>
{
public:
    StyleGeneratedData();
    ~StyleGeneratedData();
    StyleGeneratedData(const StyleGeneratedData& o );

    bool operator==(const StyleGeneratedData& o) const;
    bool operator!=(const StyleGeneratedData &o) const {
	return !(*this == o);
    }

    bool contentDataEquivalent(const StyleGeneratedData* otherStyle) const;
    bool counterDataEquivalent(const StyleGeneratedData* otherStyle) const;

    ContentData *content;
    DOM::CSSValueListImpl *counter_reset;
    DOM::CSSValueListImpl *counter_increment;
};

//------------------------------------------------
// CSS3 Marquee Properties

enum EMarqueeBehavior { MNONE, MSCROLL, MSLIDE, MALTERNATE, MUNFURL };
enum EMarqueeDirection { MAUTO = 0, MLEFT = 1, MRIGHT = -1, MUP = 2, MDOWN = -2, MFORWARD = 3, MBACKWARD = -3 };

class StyleMarqueeData : public Shared<StyleMarqueeData>
{
public:
    StyleMarqueeData();
    StyleMarqueeData(const StyleMarqueeData& o);

    bool operator==(const StyleMarqueeData& o) const;
    bool operator!=(const StyleMarqueeData& o) const {
        return !(*this == o);
    }

    Length increment;
    int speed;

    int loops; // -1 means infinite.

    EMarqueeBehavior behavior : 3;
    EMarqueeDirection direction : 3;
};

struct BorderRadii
{
    int horizontal;
    int vertical;

    BorderRadii(): horizontal(0), vertical(0) {}

    bool hasBorderRadius() const {
        return horizontal > 0 && vertical > 0;
    }

    bool operator==(const BorderRadii& o) const;
    bool operator!=(const BorderRadii& o) const {
        return !(*this == o);
    }    
};

class BorderRadiusData : public Shared<BorderRadiusData>
{
public:
    BorderRadiusData() : Shared<BorderRadiusData>() {};
    BorderRadiusData(const BorderRadiusData &other) : Shared<BorderRadiusData>(),
        topRight(other.topRight),
        bottomRight(other.bottomRight),
        bottomLeft(other.bottomLeft),
        topLeft(other.topLeft)
    {}

    bool operator==(const BorderRadiusData& o) const;
    bool operator!=(const BorderRadiusData& o) const {
        return !(*this == o);
    }

    bool hasBorderRadius() const;

    BorderRadii topRight, bottomRight, bottomLeft, topLeft;
};

// This struct holds information about shadows for the text-shadow and box-shadow properties.
struct ShadowData {
    ShadowData(int _x, int _y, int _blur, const QColor& _color)
    :x(_x), y(_y), blur(_blur), color(_color), next(0) {}
    ShadowData(const ShadowData& o);

    ~ShadowData() { delete next; }

    bool operator==(const ShadowData& o) const;
    bool operator!=(const ShadowData &o) const {
        return !(*this == o);
    }

    int x;
    int y;
    int blur;
    QColor color;
    ShadowData* next;
};

// This struct is for rarely used non-inherited CSS3 properties.  By grouping them together,
// we save space, and only allocate this object when someone actually uses
// a non-inherited CSS3 property.
class StyleCSS3NonInheritedData : public Shared<StyleCSS3NonInheritedData>
{
public:
    StyleCSS3NonInheritedData();
    ~StyleCSS3NonInheritedData() {}
    StyleCSS3NonInheritedData(const StyleCSS3NonInheritedData& o);

    bool operator==(const StyleCSS3NonInheritedData& o) const;
    bool operator!=(const StyleCSS3NonInheritedData &o) const {
        return !(*this == o);
    }

    float opacity;         // Whether or not we're transparent.
#ifdef APPLE_CHANGES	// ### we don't have those (yet)
    DataRef<StyleFlexibleBoxData> flexibleBox; // Flexible box properties
#endif
    DataRef<StyleMarqueeData> marquee; // Marquee properties
    DataRef<BorderRadiusData> borderRadius;
};

// This struct is for rarely used inherited CSS3 properties.  By grouping them together,
// we save space, and only allocate this object when someone actually uses
// an inherited CSS3 property.
class StyleCSS3InheritedData : public Shared<StyleCSS3InheritedData>
{
    public:
        StyleCSS3InheritedData();
        ~StyleCSS3InheritedData();
        StyleCSS3InheritedData(const StyleCSS3InheritedData& o);

        bool operator==(const StyleCSS3InheritedData& o) const;
        bool operator!=(const StyleCSS3InheritedData &o) const {
            return !(*this == o);
        }
        bool shadowDataEquivalent(const StyleCSS3InheritedData& o) const;

        ShadowData* textShadow;  // Our text shadow information for shadowed text drawing.
#ifdef APPLE_CHANGES
        EUserModify userModify : 2; // Flag used for editing state
        bool textSizeAdjust : 1;    // An Apple extension.  Not really CSS3 but not worth making a new struct over.
#endif
        KDE_BF_ENUM(EWordWrap) wordWrap : 1;
    private:
        StyleCSS3InheritedData &operator=(const StyleCSS3InheritedData &);
};

//------------------------------------------------
// Inherited attributes.
//
// the inherited-decoration and inherited-shadow attributes
// are inherited from the
// first parent which is block level
//

enum EWhiteSpace {
    NORMAL, PRE, NOWRAP, PRE_WRAP, PRE_LINE, KHTML_NOWRAP
};

enum ETextAlign {
    TAAUTO, LEFT, RIGHT, CENTER, JUSTIFY, KHTML_LEFT, KHTML_RIGHT, KHTML_CENTER
};

enum ETextTransform {
    CAPITALIZE, UPPERCASE, LOWERCASE, TTNONE
};

enum EDirection {
    LTR, RTL
};

enum ETextDecoration {
    TDNONE = 0x0 , UNDERLINE = 0x1, OVERLINE = 0x2, LINE_THROUGH= 0x4, BLINK = 0x8
};

enum EPageBreak {
    PBAUTO, PBALWAYS, PBAVOID,
    /* reserved for later use: */
    PBLEFT, PBRIGHT
};

class StyleInheritedData : public Shared<StyleInheritedData>
{
    StyleInheritedData& operator=(const StyleInheritedData&);
public:
    StyleInheritedData();
    ~StyleInheritedData();
    StyleInheritedData(const StyleInheritedData& o );

    bool operator==(const StyleInheritedData& o) const;
    bool operator != ( const StyleInheritedData &o ) const {
	return !(*this == o);
    }

    Length indent;
    // could be packed in a short but doesn't
    // make a difference currently because of padding
    Length line_height;

    CachedImage *style_image;

    khtml::Font font;
    QColor color;

    short border_hspacing;
    short border_vspacing;

    // Paged media properties.
    short widows;
    short orphans;

    DOM::QuotesValueImpl* quotes;
};


enum EEmptyCell {
    SHOW, HIDE
};

enum ECaptionSide {
    CAPTOP, CAPBOTTOM, CAPLEFT, CAPRIGHT
};


enum EListStyleType {
    // Symbols:
     LDISC, LCIRCLE, LSQUARE, LBOX, LDIAMOND,
    // Numeric:
     LDECIMAL, DECIMAL_LEADING_ZERO, ARABIC_INDIC, LAO, PERSIAN, URDU, THAI, TIBETAN,
    // Algorithmic:
     LOWER_ROMAN, UPPER_ROMAN, HEBREW, ARMENIAN, GEORGIAN,
    // Ideographic:
     CJK_IDEOGRAPHIC, JAPANESE_FORMAL, JAPANESE_INFORMAL,
     SIMP_CHINESE_FORMAL, SIMP_CHINESE_INFORMAL, TRAD_CHINESE_FORMAL, TRAD_CHINESE_INFORMAL,
    // Alphabetic:
     LOWER_GREEK, UPPER_GREEK, LOWER_ALPHA, LOWER_LATIN, UPPER_ALPHA, UPPER_LATIN,
     HIRAGANA, KATAKANA, HIRAGANA_IROHA, KATAKANA_IROHA,
    // Special:
     LNONE
};

inline bool isListStyleCounted(EListStyleType type)
{
    switch(type) {
    case LDISC: case LCIRCLE: case LSQUARE: case LBOX: case LDIAMOND:
    case LNONE:
        return false;
    default:
        return true;
    }
}

enum EListStylePosition { OUTSIDE, INSIDE };

enum EVisibility { VISIBLE, HIDDEN, COLLAPSE };

enum ECursor {
    CURSOR_AUTO, CURSOR_DEFAULT, CURSOR_CONTEXT_MENU, CURSOR_HELP, CURSOR_POINTER,
    CURSOR_PROGRESS, CURSOR_WAIT, CURSOR_CELL, CURSOR_CROSS, CURSOR_TEXT, CURSOR_VERTICAL_TEXT,
    CURSOR_ALIAS, CURSOR_COPY, CURSOR_MOVE, CURSOR_NO_DROP, CURSOR_NOT_ALLOWED,
    CURSOR_E_RESIZE, CURSOR_N_RESIZE, CURSOR_NE_RESIZE, CURSOR_NW_RESIZE, CURSOR_S_RESIZE, CURSOR_SE_RESIZE,
    CURSOR_SW_RESIZE, CURSOR_W_RESIZE, CURSOR_EW_RESIZE, CURSOR_NS_RESIZE, CURSOR_NESW_RESIZE, CURSOR_NWSE_RESIZE,
    CURSOR_COL_RESIZE, CURSOR_ROW_RESIZE, CURSOR_ALL_SCROLL, CURSOR_NONE
};

enum EUserInput {
    UI_ENABLED, UI_DISABLED, UI_NONE
};

//------------------------------------------------

enum EDisplay {
    INLINE, BLOCK, LIST_ITEM, RUN_IN,
    COMPACT, INLINE_BLOCK, TABLE, INLINE_TABLE,
    TABLE_ROW_GROUP, TABLE_HEADER_GROUP, TABLE_FOOTER_GROUP, TABLE_ROW,
    TABLE_COLUMN_GROUP, TABLE_COLUMN, TABLE_CELL,
    TABLE_CAPTION, NONE
};

class RenderStyle : public Shared<RenderStyle>
{
    friend class CSSStyleSelector;
public:
    KHTML_EXPORT static void cleanup();

    // pseudo elements
    enum PseudoId {
        NOPSEUDO, FIRST_LINE, FIRST_LETTER, SELECTION,
        BEFORE, AFTER, REPLACED, MARKER
    };

protected:

// !START SYNC!: Keep this in sync with the copy constructor in render_style.cpp

    // inherit
    struct InheritedFlags {
    // 64 bit inherited, update unused when adding to the struct, or the operator will break.
	bool operator==( const InheritedFlags &other ) const
        {    return _iflags ==other._iflags;    }
	bool operator!=( const InheritedFlags &other ) const
        {    return _iflags != other._iflags;   }

        union {
            struct {
                KDE_BF_ENUM(EEmptyCell) _empty_cells : 1 ;
                KDE_BF_ENUM(ECaptionSide) _caption_side : 2;
                KDE_BF_ENUM(EListStyleType) _list_style_type : 6;
                KDE_BF_ENUM(EListStylePosition) _list_style_position : 1;

                KDE_BF_ENUM(EVisibility) _visibility : 2;
                KDE_BF_ENUM(ETextAlign) _text_align : 4;
                KDE_BF_ENUM(ETextTransform) _text_transform : 2;
                unsigned _text_decorations : 4;
                KDE_BF_ENUM(ECursor) _cursor_style : 5;

                KDE_BF_ENUM(EDirection) _direction : 1;
                unsigned _border_collapse : 1 ;
                KDE_BF_ENUM(EWhiteSpace) _white_space : 3;
                // non CSS2 inherited
                unsigned _visuallyOrdered : 1;
                unsigned _htmlHacks : 1;
                KDE_BF_ENUM(EUserInput) _user_input : 2;

                unsigned _page_break_inside : 1; // AUTO/AVOID

                unsigned int unused : 27;
            } f;
            quint64 _iflags;
        };
    } inherited_flags;

// don't inherit
    struct NonInheritedFlags {
    // 64 bit non-inherited, update unused when adding to the struct, or the operator will break.
	bool operator==( const NonInheritedFlags &other ) const
        {   return _niflags == other._niflags;    }
	bool operator!=( const NonInheritedFlags &other ) const
        {   return _niflags != other._niflags;    }

        union {
            struct {
                KDE_BF_ENUM(EDisplay) _display : 5;
                KDE_BF_ENUM(EDisplay) _originalDisplay : 5;
                KDE_BF_ENUM(EOverflow) _overflowX : 4 ;
                KDE_BF_ENUM(EOverflow) _overflowY : 4 ;
                KDE_BF_ENUM(EVerticalAlign) _vertical_align : 4;
                KDE_BF_ENUM(EClear) _clear : 2;
                KDE_BF_ENUM(EPosition) _position : 2;
                KDE_BF_ENUM(EFloat) _floating : 3;
                KDE_BF_ENUM(ETableLayout) _table_layout : 1;
                unsigned _flowAroundFloats : 1;

                KDE_BF_ENUM(EPageBreak) _page_break_before : 3;
                KDE_BF_ENUM(EPageBreak) _page_break_after : 3;

                KDE_BF_ENUM(PseudoId) _styleType : 4;
                unsigned _hasClip : 1;
                unsigned _pseudoBits : 8;
                KDE_BF_ENUM(EUnicodeBidi) _unicodeBidi : 2;

                // non CSS2 non-inherited
                unsigned _textOverflow : 1; // Whether or not lines that spill out should be truncated with "..."

                unsigned _inherited_noninherited : 1;

                unsigned int unused : 10;
            } f;
            quint64 _niflags;
        };
    } noninherited_flags;

// non-inherited attributes
    DataRef<StyleBoxData> box;
    DataRef<StyleVisualData> visual;
    DataRef<StyleBackgroundData> background;
    DataRef<StyleSurroundData> surround;
    DataRef<StyleGeneratedData> generated;
    DataRef<StyleCSS3NonInheritedData> css3NonInheritedData;

// inherited attributes
    DataRef<StyleCSS3InheritedData> css3InheritedData;
    DataRef<StyleInheritedData> inherited;

// list of associated pseudo styles
    RenderStyle* pseudoStyle;

// SVG Style
    DataRef<khtml::SVGRenderStyle> m_svgStyle;

// !END SYNC!

// static default style
    static RenderStyle* _default;

private:
    RenderStyle(const RenderStyle*) {}

protected:
    void setBitDefaults()
    {
        inherited_flags.f._empty_cells = initialEmptyCells();
        inherited_flags.f._caption_side = initialCaptionSide();
	inherited_flags.f._list_style_type = initialListStyleType();
	inherited_flags.f._list_style_position = initialListStylePosition();
	inherited_flags.f._visibility = initialVisibility();
	inherited_flags.f._text_align = initialTextAlign();
	inherited_flags.f._text_transform = initialTextTransform();
	inherited_flags.f._text_decorations = initialTextDecoration();
	inherited_flags.f._cursor_style = initialCursor();
	inherited_flags.f._direction = initialDirection();
	inherited_flags.f._border_collapse = initialBorderCollapse();
	inherited_flags.f._white_space = initialWhiteSpace();
	inherited_flags.f._visuallyOrdered = false;
	inherited_flags.f._htmlHacks=false;
	inherited_flags.f._user_input = UI_NONE;
	inherited_flags.f._page_break_inside = true;
        inherited_flags.f.unused = 0;

	noninherited_flags._niflags = 0L; // for safety: without this, the equality method sometimes
	                                  // makes use of uninitialised bits according to valgrind

        noninherited_flags.f._display = noninherited_flags.f._originalDisplay = initialDisplay();
	noninherited_flags.f._overflowX = initialOverflowX();
	noninherited_flags.f._overflowY = initialOverflowY();
	noninherited_flags.f._vertical_align = initialVerticalAlign();
	noninherited_flags.f._clear = initialClear();
	noninherited_flags.f._position = initialPosition();
	noninherited_flags.f._floating = initialFloating();
	noninherited_flags.f._table_layout = initialTableLayout();
	noninherited_flags.f._flowAroundFloats= initialFlowAroundFloats();
        noninherited_flags.f._page_break_before = initialPageBreak();
        noninherited_flags.f._page_break_after = initialPageBreak();
	noninherited_flags.f._styleType = NOPSEUDO;
	noninherited_flags.f._hasClip = false;
        noninherited_flags.f._pseudoBits = 0;
	noninherited_flags.f._unicodeBidi = initialUnicodeBidi();
	noninherited_flags.f._textOverflow = initialTextOverflow();
        noninherited_flags.f._inherited_noninherited = false;
        noninherited_flags.f.unused = 0;
    }

public:

    RenderStyle();
    // used to create the default style.
    RenderStyle(bool);
    RenderStyle(const RenderStyle&);

    ~RenderStyle();

    void inheritFrom(const RenderStyle* inheritParent);
    void compactWith(const RenderStyle* similarStyle);

    PseudoId styleType() const { return KDE_CAST_BF_ENUM(PseudoId, noninherited_flags.f._styleType); }
    void setStyleType(PseudoId pi) { noninherited_flags.f._styleType = pi; }
    bool isGenerated() const {
        if (styleType() == AFTER || styleType() == BEFORE || styleType() == MARKER || styleType() == REPLACED)
            return true;
        else
            return false;
    }

    bool hasPseudoStyle(PseudoId pi) const;
    void setHasPseudoStyle(PseudoId pi, bool b=true);
    RenderStyle* getPseudoStyle(PseudoId pi) const;
    RenderStyle* addPseudoStyle(PseudoId pi);
    void removePseudoStyle(PseudoId pi);

    bool operator==(const RenderStyle& other) const;
    bool        isFloating() const { return !(noninherited_flags.f._floating == FNONE); }
    bool        hasMargin() const { return surround->margin.nonZero(); }
    bool        hasBorder() const { return surround->border.hasBorder(); }
    bool        hasPadding() const { return surround->padding.nonZero(); }
    bool        hasOffset() const { return surround->offset.nonZero(); }

    bool hasBackground() const {
        if (backgroundColor().isValid() && backgroundColor().alpha() > 0)
            return true;
        else
            return background->m_background.hasImage();
    }
    bool hasFixedBackgroundImage() const { return background->m_background.hasFixedImage(); }
    bool hasBackgroundImage()  const { return background->m_background.hasImage(); }

    bool visuallyOrdered() const { return inherited_flags.f._visuallyOrdered; }
    void setVisuallyOrdered(bool b) {  inherited_flags.f._visuallyOrdered = b; }

// attribute getter methods

    EDisplay 	display() const { return KDE_CAST_BF_ENUM(EDisplay, noninherited_flags.f._display); }
    EDisplay    originalDisplay() const { return KDE_CAST_BF_ENUM(EDisplay, noninherited_flags.f._originalDisplay); }

    Length  	left() const {  return surround->offset.left; }
    Length  	right() const {  return surround->offset.right; }
    Length  	top() const {  return surround->offset.top; }
    Length  	bottom() const {  return surround->offset.bottom; }

    EPosition 	position() const { return KDE_CAST_BF_ENUM(EPosition, noninherited_flags.f._position); }
    EFloat  	floating() const { return KDE_CAST_BF_ENUM(EFloat, noninherited_flags.f._floating); }

    Length  	width() const { return box->width; }
    Length  	height() const { return box->height; }
    Length  	minWidth() const { return box->min_width; }
    Length  	maxWidth() const { return box->max_width; }
    Length  	minHeight() const { return box->min_height; }
    Length  	maxHeight() const { return box->max_height; }

    const BorderData& border() const { return surround->border; }
    const BorderValue& borderLeft() const { return surround->border.left; }
    const BorderValue& borderRight() const { return surround->border.right; }
    const BorderValue& borderTop() const { return surround->border.top; }
    const BorderValue& borderBottom() const { return surround->border.bottom; }

    unsigned short  borderLeftWidth() const { return surround->border.borderLeftWidth(); }
    EBorderStyle    borderLeftStyle() const { return surround->border.left.style; }
    const QColor&  borderLeftColor() const { return surround->border.left.color; }
    bool borderLeftIsTransparent() const { return surround->border.left.isTransparent(); }
    unsigned short  borderRightWidth() const { return surround->border.borderRightWidth(); }
    EBorderStyle    borderRightStyle() const {  return surround->border.right.style; }
    const QColor&   borderRightColor() const {  return surround->border.right.color; }
    bool borderRightIsTransparent() const { return surround->border.right.isTransparent(); }
    unsigned short  borderTopWidth() const { return surround->border.borderTopWidth(); }
    EBorderStyle    borderTopStyle() const { return surround->border.top.style; }
    const QColor&  borderTopColor() const {  return surround->border.top.color; }
    bool borderTopIsTransparent() const { return surround->border.top.isTransparent(); }
    unsigned short  borderBottomWidth() const { return surround->border.borderBottomWidth(); }
    EBorderStyle    borderBottomStyle() const {  return surround->border.bottom.style; }
    const QColor&   borderBottomColor() const {  return surround->border.bottom.color; }
    bool borderBottomIsTransparent() const { return surround->border.bottom.isTransparent(); }

    unsigned short  outlineSize() const { return outlineWidth() + outlineOffset(); }
    unsigned short  outlineWidth() const
    { if(background->m_outline.style == BNONE || background->m_outline.style == BHIDDEN) return 0;
      else return background->m_outline.width; }
    EBorderStyle    outlineStyle() const { return background->m_outline.style; }
    bool outlineStyleIsAuto() const { return background->m_outline._auto; }
    const QColor &  outlineColor() const { return background->m_outline.color; }

    EOverflow overflowX() const { return KDE_CAST_BF_ENUM(EOverflow, noninherited_flags.f._overflowX); }
    EOverflow overflowY() const { return KDE_CAST_BF_ENUM(EOverflow, noninherited_flags.f._overflowY); }
    bool hidesOverflow() const {
        // either both overflow are visible or none are
        return overflowX() != OVISIBLE;
    }

    EVisibility visibility() const { return KDE_CAST_BF_ENUM(EVisibility, inherited_flags.f._visibility); }
    EVerticalAlign verticalAlign() const { return  KDE_CAST_BF_ENUM(EVerticalAlign, noninherited_flags.f._vertical_align); }
    Length verticalAlignLength() const { return box->vertical_align; }

    Length clipLeft() const { return visual->clip.left; }
    Length clipRight() const { return visual->clip.right; }
    Length clipTop() const { return visual->clip.top; }
    Length clipBottom() const { return visual->clip.bottom; }
    LengthBox clip() const { return visual->clip; }
    bool hasClip() const { return noninherited_flags.f._hasClip; }

    EUnicodeBidi unicodeBidi() const { return KDE_CAST_BF_ENUM(EUnicodeBidi, noninherited_flags.f._unicodeBidi); }

    EClear clear() const { return KDE_CAST_BF_ENUM(EClear, noninherited_flags.f._clear); }
    ETableLayout tableLayout() const { return KDE_CAST_BF_ENUM(ETableLayout, noninherited_flags.f._table_layout); }

    const QFont & font() const { return inherited->font.cfi->f; }
    // use with care. call font->update() after modifications
    const Font &htmlFont() { return inherited->font; }
    const QFontMetrics & fontMetrics() const { return inherited->font.cfi->fm; }

    const QColor & color() const { return inherited->color; }
    Length textIndent() const { return inherited->indent; }
    ETextAlign textAlign() const { return KDE_CAST_BF_ENUM(ETextAlign, inherited_flags.f._text_align); }
    ETextTransform textTransform() const { return KDE_CAST_BF_ENUM(ETextTransform, inherited_flags.f._text_transform); }
    int textDecorationsInEffect() const { return inherited_flags.f._text_decorations; }
    int textDecoration() const { return visual->textDecoration; }
    int wordSpacing() const { return inherited->font.wordSpacing; }
    int letterSpacing() const { return inherited->font.letterSpacing; }

    EDirection direction() const { return KDE_CAST_BF_ENUM(EDirection, inherited_flags.f._direction); }
    Length lineHeight() const { return inherited->line_height; }

    EWhiteSpace whiteSpace() const { return KDE_CAST_BF_ENUM(EWhiteSpace, inherited_flags.f._white_space); }
    bool autoWrap() const {
        if (whiteSpace() == NORMAL || whiteSpace() == PRE_WRAP || whiteSpace() == PRE_LINE)
            return true;
        // nowrap | pre
        return false;
    }
    bool preserveLF() const {
        if (whiteSpace() == PRE || whiteSpace() == PRE_WRAP || whiteSpace() == PRE_LINE)
            return true;
        // normal | nowrap
        return false;
    }
    bool preserveWS() const {
        if (whiteSpace() == PRE || whiteSpace() == PRE_WRAP)
            return true;
        // normal | nowrap | pre-line
        return false;
    }

    const QColor & backgroundColor() const { return background->m_color; }
    CachedImage *backgroundImage() const { return background->m_background.m_image; }
    EBackgroundRepeat backgroundRepeat() const { return static_cast<EBackgroundRepeat>(background->m_background.m_bgRepeat); }
    EBackgroundAttachment backgroundAttachment() const { return KDE_CAST_BF_ENUM(EBackgroundAttachment, background->m_background.m_bgAttachment); }
    Length backgroundXPosition() const { return background->m_background.m_xPosition; }
    Length backgroundYPosition() const { return background->m_background.m_yPosition; }
    BackgroundLayer* accessBackgroundLayers() { return &(background.access()->m_background); }
    const BackgroundLayer* backgroundLayers() const { return &(background->m_background); }

    // returns true for collapsing borders, false for separate borders
    bool borderCollapse() const { return inherited_flags.f._border_collapse; }
    short borderHorizontalSpacing() const { return inherited->border_hspacing; }
    short borderVerticalSpacing() const { return inherited->border_vspacing; }
    EEmptyCell emptyCells() const { return KDE_CAST_BF_ENUM(EEmptyCell, inherited_flags.f._empty_cells); }
    ECaptionSide captionSide() const { return KDE_CAST_BF_ENUM(ECaptionSide, inherited_flags.f._caption_side); }

    EListStyleType listStyleType() const { return KDE_CAST_BF_ENUM(EListStyleType, inherited_flags.f._list_style_type); }
    CachedImage *listStyleImage() const { return inherited->style_image; }
    EListStylePosition listStylePosition() const { return KDE_CAST_BF_ENUM(EListStylePosition, inherited_flags.f._list_style_position); }

    Length marginTop() const { return surround->margin.top; }
    Length marginBottom() const {  return surround->margin.bottom; }
    Length marginLeft() const {  return surround->margin.left; }
    Length marginRight() const {  return surround->margin.right; }

    Length paddingTop() const {  return surround->padding.top; }
    Length paddingBottom() const {  return surround->padding.bottom; }
    Length paddingLeft() const { return surround->padding.left; }
    Length paddingRight() const {  return surround->padding.right; }

    ECursor cursor() const { return KDE_CAST_BF_ENUM(ECursor, inherited_flags.f._cursor_style); }

    short widows() const { return inherited->widows; }
    short orphans() const { return inherited->orphans; }
    bool pageBreakInside() const { return inherited_flags.f._page_break_inside; }
    EPageBreak pageBreakBefore() const { return KDE_CAST_BF_ENUM(EPageBreak, noninherited_flags.f._page_break_before); }
    EPageBreak pageBreakAfter() const { return KDE_CAST_BF_ENUM(EPageBreak, noninherited_flags.f._page_break_after); }

    DOM::QuotesValueImpl* quotes() const { return inherited->quotes; }
    QString openQuote(int level) const;
    QString closeQuote(int level) const;

    // CSS3 Getter Methods
    EBoxSizing boxSizing() const { return box->box_sizing; }
    int outlineOffset() const {
        if (background->m_outline.style == BNONE || background->m_outline.style == BHIDDEN) return 0;
        return background->m_outline._offset;
    }
    ShadowData* textShadow() const { return css3InheritedData->textShadow; }
    EWordWrap wordWrap() const { return KDE_CAST_BF_ENUM(EWordWrap, css3InheritedData->wordWrap); }
    float opacity() const { return css3NonInheritedData->opacity; }
    EUserInput userInput() const { return KDE_CAST_BF_ENUM(EUserInput, inherited_flags.f._user_input); }

    Length marqueeIncrement() const { return css3NonInheritedData->marquee->increment; }
    int marqueeSpeed() const { return css3NonInheritedData->marquee->speed; }
    int marqueeLoopCount() const { return css3NonInheritedData->marquee->loops; }
    EMarqueeBehavior marqueeBehavior() const { return css3NonInheritedData->marquee->behavior; }
    EMarqueeDirection marqueeDirection() const { return css3NonInheritedData->marquee->direction; }
    bool textOverflow() const { return noninherited_flags.f._textOverflow; }

    bool hasBorderRadius() const { return css3NonInheritedData->borderRadius->hasBorderRadius(); }
    BorderRadii borderTopRightRadius() const {  return css3NonInheritedData->borderRadius->topRight; }
    BorderRadii borderTopLeftRadius () const {  return css3NonInheritedData->borderRadius->topLeft; }
    BorderRadii borderBottomRightRadius() const {  return css3NonInheritedData->borderRadius->bottomRight; }
    BorderRadii borderBottomLeftRadius () const {  return css3NonInheritedData->borderRadius->bottomLeft; }
    // End CSS3 Getters

// attribute setter methods

    void setDisplay(EDisplay v) {  noninherited_flags.f._display = v; }
    void setOriginalDisplay(EDisplay v) {  noninherited_flags.f._originalDisplay = v; }
    void setPosition(EPosition v) {  noninherited_flags.f._position = v; }
    void setFloating(EFloat v) {  noninherited_flags.f._floating = v; }

    void setLeft(Length v)  {  SET_VAR(surround,offset.left,v) }
    void setRight(Length v) {  SET_VAR(surround,offset.right,v) }
    void setTop(Length v)   {  SET_VAR(surround,offset.top,v) }
    void setBottom(Length v){  SET_VAR(surround,offset.bottom,v) }

    void setWidth(Length v)  { SET_VAR(box,width,v) }
    void setHeight(Length v) { SET_VAR(box,height,v) }

    void setMinWidth(Length v)  { SET_VAR(box,min_width,v) }
    void setMaxWidth(Length v)  { SET_VAR(box,max_width,v) }
    void setMinHeight(Length v) { SET_VAR(box,min_height,v) }
    void setMaxHeight(Length v) { SET_VAR(box,max_height,v) }

    void resetBorderTop() { SET_VAR(surround, border.top, BorderValue()) }
    void resetBorderRight() { SET_VAR(surround, border.right, BorderValue()) }
    void resetBorderBottom() { SET_VAR(surround, border.bottom, BorderValue()) }
    void resetBorderLeft() { SET_VAR(surround, border.left, BorderValue()) }
    void resetOutline() { SET_VAR(background, m_outline, OutlineValue()) }

    void setBackgroundColor(const QColor& v)    { SET_VAR(background, m_color, v) }

    void setBorderLeftWidth(unsigned short v)   {  SET_VAR(surround,border.left.width,v) }
    void setBorderLeftStyle(EBorderStyle v)     {  SET_VAR(surround,border.left.style,v) }
    void setBorderLeftColor(const QColor & v)   {  SET_VAR(surround,border.left.color,v) }
    void setBorderRightWidth(unsigned short v)  {  SET_VAR(surround,border.right.width,v) }
    void setBorderRightStyle(EBorderStyle v)    {  SET_VAR(surround,border.right.style,v) }
    void setBorderRightColor(const QColor & v)  {  SET_VAR(surround,border.right.color,v) }
    void setBorderTopWidth(unsigned short v)    {  SET_VAR(surround,border.top.width,v) }
    void setBorderTopStyle(EBorderStyle v)      {  SET_VAR(surround,border.top.style,v) }
    void setBorderTopColor(const QColor & v)    {  SET_VAR(surround,border.top.color,v) }
    void setBorderBottomWidth(unsigned short v) {  SET_VAR(surround,border.bottom.width,v) }
    void setBorderBottomStyle(EBorderStyle v)   {  SET_VAR(surround,border.bottom.style,v) }
    void setBorderBottomColor(const QColor & v) {  SET_VAR(surround,border.bottom.color,v) }
    void setOutlineWidth(unsigned short v) {  SET_VAR(background,m_outline.width,v) }
    void setOutlineStyle(EBorderStyle v, bool isAuto = false)
    {
        SET_VAR(background,m_outline.style,v)
        SET_VAR(background,m_outline._auto, isAuto)
    }
    void setOutlineColor(const QColor & v) {  SET_VAR(background,m_outline.color,v) }

    void setOverflowX(EOverflow v) {  noninherited_flags.f._overflowX = v; }
    void setOverflowY(EOverflow v) {  noninherited_flags.f._overflowY = v; }
    void setVisibility(EVisibility v) { inherited_flags.f._visibility = v; }
    void setVerticalAlign(EVerticalAlign v) { noninherited_flags.f._vertical_align = v; }
    void setVerticalAlignLength(Length l) { SET_VAR(box, vertical_align, l ) }

    void setClipLeft(Length v) { SET_VAR(visual,clip.left,v) }
    void setClipRight(Length v) { SET_VAR(visual,clip.right,v) }
    void setClipTop(Length v) { SET_VAR(visual,clip.top,v) }
    void setClipBottom(Length v) { SET_VAR(visual,clip.bottom,v) }
    void setClip( Length top, Length right, Length bottom, Length left );
    void setHasClip( bool b ) { noninherited_flags.f._hasClip = b; }

    void setUnicodeBidi( EUnicodeBidi b ) { noninherited_flags.f._unicodeBidi = b; }

    void setClear(EClear v) {  noninherited_flags.f._clear = v; }
    void setTableLayout(ETableLayout v) {  noninherited_flags.f._table_layout = v; }
    bool setFontDef(const khtml::FontDef & v) {
        // bah, this doesn't compare pointers. broken! (Dirk)
        if (!(inherited->font.fontDef == v)) {
            inherited.access()->font = Font( v );
            return true;
        }
        return false;
    }

    void setColor(const QColor & v) { SET_VAR(inherited,color,v) }
    void setTextIndent(Length v) { SET_VAR(inherited,indent,v) }
    void setTextAlign(ETextAlign v) { inherited_flags.f._text_align = v; }
    void setTextTransform(ETextTransform v) { inherited_flags.f._text_transform = v; }
    void addToTextDecorationsInEffect(int v) { inherited_flags.f._text_decorations |= v; }
    void setTextDecorationsInEffect(int v) { inherited_flags.f._text_decorations = v; }
    void setTextDecoration(unsigned v) { SET_VAR(visual, textDecoration, v); }
    void setDirection(EDirection v) { inherited_flags.f._direction = v; }
    void setLineHeight(Length v) { SET_VAR(inherited,line_height,v) }

    void setWhiteSpace(EWhiteSpace v) { inherited_flags.f._white_space = v; }

    void setWordSpacing(int v) { SET_VAR(inherited,font.wordSpacing,v) }
    void setLetterSpacing(int v) { SET_VAR(inherited,font.letterSpacing,v) }

    void clearBackgroundLayers() { background.access()->m_background = BackgroundLayer(); }
    void inheritBackgroundLayers(const BackgroundLayer& parent) { background.access()->m_background = parent; }
    void adjustBackgroundLayers();

    void setBorderCollapse(bool collapse) { inherited_flags.f._border_collapse = collapse; }
    void setBorderHorizontalSpacing(short v) { SET_VAR(inherited,border_hspacing,v) }
    void setBorderVerticalSpacing(short v) { SET_VAR(inherited,border_vspacing,v) }

    void setEmptyCells(EEmptyCell v) { inherited_flags.f._empty_cells = v; }
    void setCaptionSide(ECaptionSide v) { inherited_flags.f._caption_side = v; }

    void setListStyleType(EListStyleType v) { inherited_flags.f._list_style_type = v; }
    void setListStyleImage(CachedImage *v) {  SET_VAR(inherited,style_image,v)}
    void setListStylePosition(EListStylePosition v) { inherited_flags.f._list_style_position = v; }

    void resetMargin() { SET_VAR(surround, margin, LengthBox(Fixed)) }
    void setMarginTop(Length v)     {  SET_VAR(surround,margin.top,v) }
    void setMarginBottom(Length v)  {  SET_VAR(surround,margin.bottom,v) }
    void setMarginLeft(Length v)    {  SET_VAR(surround,margin.left,v) }
    void setMarginRight(Length v)   {  SET_VAR(surround,margin.right,v) }

    void resetPadding() { SET_VAR(surround, padding, LengthBox(Auto)) }
    void setPaddingTop(Length v)    {  SET_VAR(surround,padding.top,v) }
    void setPaddingBottom(Length v) {  SET_VAR(surround,padding.bottom,v) }
    void setPaddingLeft(Length v)   {  SET_VAR(surround,padding.left,v) }
    void setPaddingRight(Length v)  {  SET_VAR(surround,padding.right,v) }

    void setCursor( ECursor c ) { inherited_flags.f._cursor_style = c; }

    bool htmlHacks() const { return inherited_flags.f._htmlHacks; }
    void setHtmlHacks(bool b=true) { inherited_flags.f._htmlHacks = b; }

    bool flowAroundFloats() const { return  noninherited_flags.f._flowAroundFloats; }
    void setFlowAroundFloats(bool b=true) {  noninherited_flags.f._flowAroundFloats = b; }

    int zIndex() const { return box->z_auto? 0 : box->z_index; }
    void setZIndex(int v) { SET_VAR(box,z_auto,false ); SET_VAR(box, z_index, v); }
    bool hasAutoZIndex() const { return box->z_auto; }
    void setHasAutoZIndex() { SET_VAR(box, z_auto, true ); }

    void setWidows(short w) { SET_VAR(inherited, widows, w); }
    void setOrphans(short o) { SET_VAR(inherited, orphans, o); }
    void setPageBreakInside(bool b) { inherited_flags.f._page_break_inside = b; }
    void setPageBreakBefore(EPageBreak b) { noninherited_flags.f._page_break_before = b; }
    void setPageBreakAfter(EPageBreak b) { noninherited_flags.f._page_break_after = b; }

    void setQuotes(DOM::QuotesValueImpl* q);

    // CSS3 Setters
    void setBoxSizing( EBoxSizing b ) { SET_VAR(box,box_sizing,b); }
    void setOutlineOffset(unsigned short v) {  SET_VAR(background,m_outline._offset,v) }
    void setWordWrap(EWordWrap w) { SET_VAR(css3InheritedData, wordWrap, w); }
    void setTextShadow(ShadowData* val, bool add=false);
    void setOpacity(float f) { SET_VAR(css3NonInheritedData, opacity, f); }
    void setUserInput(EUserInput ui) { inherited_flags.f._user_input = ui; }

    void setMarqueeIncrement(const Length& f) { SET_VAR(css3NonInheritedData.access()->marquee, increment, f); }
    void setMarqueeSpeed(int f) { SET_VAR(css3NonInheritedData.access()->marquee, speed, f); }
    void setMarqueeDirection(EMarqueeDirection d) { SET_VAR(css3NonInheritedData.access()->marquee, direction, d); }
    void setMarqueeBehavior(EMarqueeBehavior b) { SET_VAR(css3NonInheritedData.access()->marquee, behavior, b); }
    void setMarqueeLoopCount(int i) { SET_VAR(css3NonInheritedData.access()->marquee, loops, i); }
    void setTextOverflow(bool b) { noninherited_flags.f._textOverflow = b; }

    void setBorderTopRightRadius(const BorderRadii& r) {
        SET_VAR(css3NonInheritedData.access()->borderRadius, topRight, r);
    }

    void setBorderTopLeftRadius(const BorderRadii& r) {
        SET_VAR(css3NonInheritedData.access()->borderRadius, topLeft, r);
    }

    void setBorderBottomRightRadius(const BorderRadii& r) {
        SET_VAR(css3NonInheritedData.access()->borderRadius, bottomRight, r);
    }

    void setBorderBottomLeftRadius(const BorderRadii& r) {
        SET_VAR(css3NonInheritedData.access()->borderRadius, bottomLeft, r);
    }

    // End CSS3 Setters

    QPalette palette() const { return visual->palette; }
    void setPaletteColor(QPalette::ColorGroup g, QPalette::ColorRole r, const QColor& c);
    void resetPalette() // Called when the desktop color scheme changes.
    {
        const_cast<StyleVisualData *>(visual.get())->palette = QApplication::palette();
    }

    bool useNormalContent() const { return generated->content == 0; }
    ContentData* contentData() const { return generated->content; }
    bool contentDataEquivalent(const RenderStyle* otherStyle) const
    {
        return generated->contentDataEquivalent(otherStyle->generated.get());
    }
    void addContent(DOM::DOMStringImpl* s);
    void addContent(CachedObject* o);
    void addContent(DOM::CounterImpl* c);
    void addContent(EQuoteContent q);
    void setContentNone();
    void setContentNormal();
    void setContentData(ContentData* content);

    DOM::CSSValueListImpl* counterReset() const { return generated->counter_reset; }
    DOM::CSSValueListImpl* counterIncrement() const { return generated->counter_increment; }
    void setCounterReset(DOM::CSSValueListImpl* v);
    void setCounterIncrement(DOM::CSSValueListImpl* v);
    bool hasCounterReset(const DOM::DOMString& c) const;
    bool hasCounterIncrement(const DOM::DOMString& c) const;
    short counterReset(const DOM::DOMString& c) const;
    short counterIncrement(const DOM::DOMString& c) const;

    bool inheritedNotEqual( RenderStyle *other ) const;

    enum Diff { Equal, NonVisible = Equal, Visible, Position, Layout, CbLayout };
    Diff diff( const RenderStyle *other ) const;

    bool isDisplayReplacedType() {
        return display() == INLINE_BLOCK ||/* display() == INLINE_BOX ||*/ display() == INLINE_TABLE;
    }
    bool isDisplayInlineType() {
        return display() == INLINE || isDisplayReplacedType();
    }
    bool isOriginalDisplayInlineType() {
        return originalDisplay() == INLINE || originalDisplay() == INLINE_BLOCK ||
               /*originalDisplay() == INLINE_BOX ||*/ originalDisplay() == INLINE_TABLE;
    }

    bool inheritedNoninherited() const { return noninherited_flags.f._inherited_noninherited; }
    void setInheritedNoninherited(bool b) { noninherited_flags.f._inherited_noninherited = b; }

#ifdef ENABLE_DUMP
    QString createDiff( const RenderStyle &parent ) const;
#endif

    // Initial values for all the properties
    static EBackgroundAttachment initialBackgroundAttachment() { return BGASCROLL; }
    static EBackgroundBox initialBackgroundClip() { return BGBORDER; }
    static EBackgroundBox initialBackgroundOrigin() { return BGPADDING; }
    static EBackgroundRepeat initialBackgroundRepeat() { return REPEAT; }
    static LengthSize initialBackgroundSize() { return LengthSize(); }
    static EBackgroundSizeType initialBackgroundSizeType() { return BGSLENGTH; }
    static bool initialBorderCollapse() { return false; }
    static EBorderStyle initialBorderStyle() { return BNONE; }
    static ECaptionSide initialCaptionSide() { return CAPTOP; }
    static EClear initialClear() { return CNONE; }
    static EDirection initialDirection() { return LTR; }
    static EDisplay initialDisplay() { return INLINE; }
    static EEmptyCell initialEmptyCells() { return SHOW; }
    static EFloat initialFloating() { return FNONE; }
    static EWordWrap initialWordWrap() { return WWNORMAL; }
    static EListStylePosition initialListStylePosition() { return OUTSIDE; }
    static EListStyleType initialListStyleType() { return LDISC; }
    static EOverflow initialOverflowX() { return OVISIBLE; }
    static EOverflow initialOverflowY() { return OVISIBLE; }
    static EPageBreak initialPageBreak() { return PBAUTO; }
    static bool initialPageBreakInside() { return true; }
    static EPosition initialPosition() { return PSTATIC; }
    static ETableLayout initialTableLayout() { return TAUTO; }
    static EUnicodeBidi initialUnicodeBidi() { return UBNormal; }
    static DOM::QuotesValueImpl* initialQuotes() { return 0; }
    static EBoxSizing initialBoxSizing() { return CONTENT_BOX; }
    static ETextTransform initialTextTransform() { return TTNONE; }
    static EVisibility initialVisibility() { return VISIBLE; }
    static EWhiteSpace initialWhiteSpace() { return NORMAL; }
    static Length initialBackgroundXPosition() { return Length(); }
    static Length initialBackgroundYPosition() { return Length(); }
    static short initialBorderHorizontalSpacing() { return 0; }
    static short initialBorderVerticalSpacing() { return 0; }
    static ECursor initialCursor() { return CURSOR_AUTO; }
    static QColor initialColor() { return Qt::black; }
    static CachedImage* initialBackgroundImage() { return 0; }
    static CachedImage* initialListStyleImage() { return 0; }
    static unsigned short initialBorderWidth() { return 3; }
    static int initialLetterWordSpacing() { return 0; }
    static Length initialSize() { return Length(); }
    static Length initialMinSize() { return Length(0, Fixed); }
    static Length initialMaxSize() { return Length(UNDEFINED, Fixed); }
    static Length initialOffset() { return Length(); }
    static Length initialMargin() { return Length(Fixed); }
    static Length initialPadding() { return Length(Auto); }
    static Length initialTextIndent() { return Length(Fixed); }
    static EVerticalAlign initialVerticalAlign() { return BASELINE; }
    static int initialWidows() { return 2; }
    static int initialOrphans() { return 2; }
    static Length initialLineHeight() { return Length(-100.0, Percent); }
    static ETextAlign initialTextAlign() { return TAAUTO; }
    static ETextDecoration initialTextDecoration() { return TDNONE; }
    static bool initialFlowAroundFloats() { return false; }
    static int initialOutlineOffset() { return 0; }
    static float initialOpacity() { return 1.0f; }
    static int initialMarqueeLoopCount() { return -1; }
    static int initialMarqueeSpeed() { return 85; }
    static Length initialMarqueeIncrement() { return Length(6, Fixed); }
    static EMarqueeBehavior initialMarqueeBehavior() { return MSCROLL; }
    static EMarqueeDirection initialMarqueeDirection() { return MAUTO; }
    static bool initialTextOverflow() { return false; }

    // SVG
    const khtml::SVGRenderStyle* svgStyle() const { return m_svgStyle.get(); }
    khtml::SVGRenderStyle* accessSVGStyle() { return m_svgStyle.access(); }
};

class RenderPageStyle {
    friend class CSSStyleSelector;
public:
    enum PageType { NO_PAGE = 0, ANY_PAGE, FIRST_PAGE, LEFT_PAGES, RIGHT_PAGES };

    RenderPageStyle();
    ~RenderPageStyle();

    PageType pageType() { return m_pageType; }

    RenderPageStyle* getPageStyle(PageType type);
    RenderPageStyle* addPageStyle(PageType type);
    void removePageStyle(PageType type);

    Length marginTop()    const { return margin.top;    }
    Length marginBottom() const { return margin.bottom; }
    Length marginLeft()   const { return margin.left;   }
    Length marginRight()  const { return margin.right;  }

    Length pageWidth()  const   { return m_pageWidth;   }
    Length pageHeight() const   { return m_pageHeight;  }

    void setMarginTop(Length v)     {  margin.top = v;    }
    void setMarginBottom(Length v)  {  margin.bottom = v; }
    void setMarginLeft(Length v)    {  margin.left = v;   }
    void setMarginRight(Length v)   {  margin.right = v;  }

    void setPageWidth(Length v)    {  m_pageWidth = v;   }
    void setPageHeight(Length v)   {  m_pageHeight = v;  }

protected:
    RenderPageStyle *next;
    PageType m_pageType;

    LengthBox margin;
    Length m_pageWidth;
    Length m_pageHeight;
};

} // namespace

#endif


