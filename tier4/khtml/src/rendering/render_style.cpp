/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002-2005 Apple Computer, Inc.
 * Copyright (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
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

#include "render_style.h"

#include <xml/dom_stringimpl.h>
#include <css/cssstyleselector.h>
#include <css/css_valueimpl.h>

using namespace khtml;
using namespace DOM;

/* CSS says Fixed for the default padding value, but we treat variable as 0 padding anyways, and like
 * this is works fine for table paddings aswell
 */
StyleSurroundData::StyleSurroundData()
    : margin( Fixed ), padding( Auto )
{
}

StyleSurroundData::StyleSurroundData(const StyleSurroundData& o )
    : Shared<StyleSurroundData>(),
      offset( o.offset ), margin( o.margin ), padding( o.padding ),
      border( o.border )
{
}

bool StyleSurroundData::operator==(const StyleSurroundData& o) const
{
    return offset==o.offset && margin==o.margin &&
	padding==o.padding && border==o.border;
}

StyleBoxData::StyleBoxData()
    : z_index( 0 ), z_auto( true )
{
    min_width = min_height = RenderStyle::initialMinSize();
    max_width = max_height = RenderStyle::initialMaxSize();
    box_sizing = RenderStyle::initialBoxSizing();
}

StyleBoxData::StyleBoxData(const StyleBoxData& o )
    : Shared<StyleBoxData>(),
      width( o.width ), height( o.height ),
      min_width( o.min_width ), max_width( o.max_width ),
      min_height ( o.min_height ), max_height( o.max_height ),
      box_sizing( o.box_sizing),
      z_index( o.z_index ), z_auto( o.z_auto )
{
}

bool StyleBoxData::operator==(const StyleBoxData& o) const
{
    return
	    width == o.width &&
	    height == o.height &&
	    min_width == o.min_width &&
	    max_width == o.max_width &&
	    min_height == o.min_height &&
	    max_height == o.max_height &&
	    box_sizing == o.box_sizing &&
	    vertical_align == o.vertical_align &&
	    z_index == o.z_index &&
	    z_auto == o.z_auto;
}

StyleVisualData::StyleVisualData()
     : textDecoration(RenderStyle::initialTextDecoration()),
      palette( QApplication::palette() )
{
}

StyleVisualData::~StyleVisualData() {
}

StyleVisualData::StyleVisualData(const StyleVisualData& o )
    : Shared<StyleVisualData>(),
      clip( o.clip ), textDecoration(o.textDecoration),
      palette( o.palette )
{
}

BackgroundLayer::BackgroundLayer()
:m_image(RenderStyle::initialBackgroundImage()),
 m_bgAttachment(RenderStyle::initialBackgroundAttachment()),
 m_bgClip(RenderStyle::initialBackgroundClip()),
 m_bgOrigin(RenderStyle::initialBackgroundOrigin()),
 m_bgRepeat(RenderStyle::initialBackgroundRepeat()),
 m_bgSizeType(RenderStyle::initialBackgroundSizeType()),
 m_backgroundSize(RenderStyle::initialBackgroundSize()),
 m_next(0)
{
    m_imageSet = m_attachmentSet = m_clipSet = m_originSet =
            m_repeatSet = m_xPosSet = m_yPosSet = m_backgroundSizeSet = false;
}

BackgroundLayer::BackgroundLayer(const BackgroundLayer& o)
{
    m_next = o.m_next ? new BackgroundLayer(*o.m_next) : 0;
    m_image = o.m_image;
    m_xPosition = o.m_xPosition;
    m_yPosition = o.m_yPosition;
    m_bgAttachment = o.m_bgAttachment;
    m_bgClip = o.m_bgClip;
    m_bgOrigin = o.m_bgOrigin;
    m_bgRepeat = o.m_bgRepeat;
    m_backgroundSize = o.m_backgroundSize;
    m_bgSizeType = o.m_bgSizeType;
    m_imageSet = o.m_imageSet;
    m_attachmentSet = o.m_attachmentSet;
    m_clipSet = o.m_clipSet;
    m_originSet = o.m_originSet;
    m_repeatSet = o.m_repeatSet;
    m_xPosSet = o.m_xPosSet;
    m_yPosSet = o.m_yPosSet;
    m_backgroundSizeSet = o.m_backgroundSizeSet;
}

BackgroundLayer::~BackgroundLayer()
{
    delete m_next;
}

BackgroundLayer& BackgroundLayer::operator=(const BackgroundLayer& o) {
    if (m_next != o.m_next) {
        delete m_next;
        m_next = o.m_next ? new BackgroundLayer(*o.m_next) : 0;
    }

    m_image = o.m_image;
    m_xPosition = o.m_xPosition;
    m_yPosition = o.m_yPosition;
    m_bgAttachment = o.m_bgAttachment;
    m_bgClip = o.m_bgClip;
    m_bgOrigin = o.m_bgOrigin;
    m_bgRepeat = o.m_bgRepeat;
    m_backgroundSize = o.m_backgroundSize;
    m_bgSizeType = o.m_bgSizeType;

    m_imageSet = o.m_imageSet;
    m_attachmentSet = o.m_attachmentSet;
    m_originSet = o.m_originSet;
    m_repeatSet = o.m_repeatSet;
    m_xPosSet = o.m_xPosSet;
    m_yPosSet = o.m_yPosSet;
    m_backgroundSizeSet = o.m_backgroundSizeSet;

    return *this;
}

bool BackgroundLayer::operator==(const BackgroundLayer& o) const {
    return m_image == o.m_image && m_xPosition == o.m_xPosition && m_yPosition == o.m_yPosition &&
           m_bgAttachment == o.m_bgAttachment && m_bgClip == o.m_bgClip && m_bgOrigin == o.m_bgOrigin && m_bgRepeat == o.m_bgRepeat &&
           m_backgroundSize.width == o.m_backgroundSize.width && m_bgSizeType == o.m_bgSizeType && m_backgroundSize.height == o.m_backgroundSize.height &&
           m_imageSet == o.m_imageSet && m_attachmentSet == o.m_attachmentSet && m_repeatSet == o.m_repeatSet &&
           m_xPosSet == o.m_xPosSet && m_yPosSet == o.m_yPosSet && m_backgroundSizeSet == o.m_backgroundSizeSet &&
           ((m_next && o.m_next) ? *m_next == *o.m_next : m_next == o.m_next);
}

void BackgroundLayer::fillUnsetProperties()
{
    BackgroundLayer* curr;
    for (curr = this; curr && curr->isBackgroundImageSet(); curr = curr->next()) {};
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (BackgroundLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_image = pattern->m_image;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isBackgroundXPositionSet(); curr = curr->next()) {};
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (BackgroundLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_xPosition = pattern->m_xPosition;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isBackgroundYPositionSet(); curr = curr->next()) {};
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (BackgroundLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_yPosition = pattern->m_yPosition;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isBackgroundAttachmentSet(); curr = curr->next()) {};
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (BackgroundLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_bgAttachment = pattern->m_bgAttachment;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isBackgroundClipSet(); curr = curr->next()) {};
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (BackgroundLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_bgClip = pattern->m_bgClip;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isBackgroundOriginSet(); curr = curr->next()) {};
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (BackgroundLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_bgOrigin = pattern->m_bgOrigin;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isBackgroundRepeatSet(); curr = curr->next()) {};
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (BackgroundLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_bgRepeat = pattern->m_bgRepeat;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isBackgroundSizeSet(); curr = curr->next()) {};
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (BackgroundLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_backgroundSize = pattern->m_backgroundSize;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }
}

void BackgroundLayer::cullEmptyLayers()
{
    BackgroundLayer *next;
    for (BackgroundLayer *p = this; p; p = next) {
        next = p->m_next;
        if (next && !next->isBackgroundImageSet() &&
            !next->isBackgroundXPositionSet() && !next->isBackgroundYPositionSet() &&
            !next->isBackgroundAttachmentSet() && !next->isBackgroundClipSet() &&
            !next->isBackgroundOriginSet() && !next->isBackgroundRepeatSet() &&
            !next->isBackgroundSizeSet()) {
            delete next;
            p->m_next = 0;
            break;
        }
    }
}

StyleBackgroundData::StyleBackgroundData()
{}

StyleBackgroundData::StyleBackgroundData(const StyleBackgroundData& o)
    : Shared<StyleBackgroundData>(), m_background(o.m_background), m_outline(o.m_outline)
{}

bool StyleBackgroundData::operator==(const StyleBackgroundData& o) const
{
    return m_background == o.m_background && m_color == o.m_color && m_outline == o.m_outline;
}

StyleGeneratedData::StyleGeneratedData() : Shared<StyleGeneratedData>(), content(0), counter_reset(0), counter_increment(0) {}

StyleGeneratedData::~StyleGeneratedData()
{
    if (counter_reset) counter_reset->deref();
    if (counter_increment) counter_increment->deref();
    delete content;
}

StyleGeneratedData::StyleGeneratedData(const StyleGeneratedData& o)
    : Shared<StyleGeneratedData>(), content(0),
      counter_reset(o.counter_reset), counter_increment(o.counter_increment)
{
    if (o.content) content = new ContentData(*o.content);
    if (counter_reset) counter_reset->ref();
    if (counter_increment) counter_increment->ref();
}

bool StyleGeneratedData::contentDataEquivalent(const StyleGeneratedData* otherStyle) const
{
    ContentData* c1 = content;
    ContentData* c2 = otherStyle->content;

    while (c1 && c2) {
        if (c1->_contentType != c2->_contentType)
            return false;
        if (c1->_contentType == CONTENT_TEXT) {
            DOM::DOMString c1Str(c1->_content.text);
            DOM::DOMString c2Str(c2->_content.text);
            if (c1Str != c2Str)
                return false;
        }
        else if (c1->_contentType == CONTENT_OBJECT) {
            if (c1->_content.object != c2->_content.object)
                return false;
        }
        else if (c1->_contentType == CONTENT_COUNTER) {
            if (c1->_content.counter != c2->_content.counter)
                return false;
        }
        else if (c1->_contentType == CONTENT_QUOTE) {
            if (c1->_content.quote != c2->_content.quote)
                return false;
        }

        c1 = c1->_nextContent;
        c2 = c2->_nextContent;
    }

    return !c1 && !c2;
}

static bool compareCounterActList(const CSSValueListImpl* ca, const CSSValueListImpl* cb) {
    // weeee....
    CSSValueListImpl* a = const_cast<CSSValueListImpl*>(ca);
    CSSValueListImpl* b = const_cast<CSSValueListImpl*>(cb);

    if (!a && !b) return true;
    if (!a || !b) return false;
    if (a->length() != b->length()) return false;
    for(uint i=0; i< a->length(); i++) {
        CSSValueImpl *ai =  a->item(i);
        CSSValueImpl *bi =  b->item(i);
        assert(ai && ai->cssValueType() == CSSValue::CSS_CUSTOM);
        assert(bi && bi->cssValueType() == CSSValue::CSS_CUSTOM);
        CounterActImpl* caa = static_cast<CounterActImpl*>(ai);
        CounterActImpl* cab = static_cast<CounterActImpl*>(bi);
        if (caa->value() != cab->value()) return false;
        if (caa->counter() != cab->counter()) return false;
    }
    return true;
}

bool StyleGeneratedData::counterDataEquivalent(const StyleGeneratedData* otherStyle) const
{
    return compareCounterActList(counter_reset, otherStyle->counter_reset) &&
           compareCounterActList(counter_increment, otherStyle->counter_increment);
}

bool StyleGeneratedData::operator==(const StyleGeneratedData& o) const
{
    return contentDataEquivalent(&o) && counterDataEquivalent(&o);
}

StyleMarqueeData::StyleMarqueeData()
{
    increment = RenderStyle::initialMarqueeIncrement();
    speed = RenderStyle::initialMarqueeSpeed();
    direction = RenderStyle::initialMarqueeDirection();
    behavior = RenderStyle::initialMarqueeBehavior();
    loops = RenderStyle::initialMarqueeLoopCount();
}

StyleMarqueeData::StyleMarqueeData(const StyleMarqueeData& o)
:Shared<StyleMarqueeData>(), increment(o.increment), speed(o.speed), loops(o.loops),
 behavior(o.behavior), direction(o.direction)
{}

bool StyleMarqueeData::operator==(const StyleMarqueeData& o) const
{
    return (increment == o.increment && speed == o.speed && direction == o.direction &&
            behavior == o.behavior && loops == o.loops);
}

bool BorderRadii::operator==(const BorderRadii& o) const
{
    return horizontal == o.horizontal &&
           vertical   == o.vertical;
}

bool BorderRadiusData::operator==(const BorderRadiusData& o) const
{
    return topRight == o.topRight && bottomRight == o.bottomRight
         && bottomLeft == o.bottomLeft && topLeft == o.topLeft;
}

bool BorderRadiusData::hasBorderRadius() const
{
    return topRight.hasBorderRadius()   || bottomRight.hasBorderRadius() ||
           bottomLeft.hasBorderRadius() || topLeft.hasBorderRadius();
}

StyleCSS3NonInheritedData::StyleCSS3NonInheritedData()
:Shared<StyleCSS3NonInheritedData>()
, opacity(RenderStyle::initialOpacity())
{
}

StyleCSS3NonInheritedData::StyleCSS3NonInheritedData(const StyleCSS3NonInheritedData& o)
:Shared<StyleCSS3NonInheritedData>(),
 opacity(o.opacity),
#ifdef APPLE_CHANGES
 flexibleBox(o.flexibleBox),
#endif
 marquee(o.marquee),
 borderRadius(o.borderRadius)
{
}

bool StyleCSS3NonInheritedData::operator==(const StyleCSS3NonInheritedData& o) const
{
    return
     opacity == o.opacity &&
#ifdef APPLE_CHANGES
     flexibleBox == o.flexibleBox &&
#endif
     marquee == o.marquee &&
     borderRadius == o.borderRadius;
}

StyleCSS3InheritedData::StyleCSS3InheritedData()
:Shared<StyleCSS3InheritedData>(), textShadow(0), wordWrap(RenderStyle::initialWordWrap())
#ifdef APPLE_CHANGES
, userModify(READ_ONLY), textSizeAdjust(RenderStyle::initialTextSizeAdjust())
#endif
{

}

StyleCSS3InheritedData::StyleCSS3InheritedData(const StyleCSS3InheritedData& o)
:Shared<StyleCSS3InheritedData>()
{
    textShadow = o.textShadow ? new ShadowData(*o.textShadow) : 0;
    wordWrap = o.wordWrap;
#ifdef APPLE_CHANGES
    userModify = o.userModify;
    textSizeAdjust = o.textSizeAdjust;
#endif
}

StyleCSS3InheritedData::~StyleCSS3InheritedData()
{
    delete textShadow;
}

bool StyleCSS3InheritedData::operator==(const StyleCSS3InheritedData& o) const
{
    return shadowDataEquivalent(o) && (wordWrap == o.wordWrap)
#ifdef APPLE_CHANGES
            && (userModify == o.userModify) && (textSizeAdjust == o.textSizeAdjust)
#endif
    ;
}

bool StyleCSS3InheritedData::shadowDataEquivalent(const StyleCSS3InheritedData& o) const
{
    if ((!textShadow && o.textShadow) || (textShadow && !o.textShadow))
        return false;
    if ((textShadow && o.textShadow) && (*textShadow != *o.textShadow))
        return false;
    return true;
}

StyleInheritedData::StyleInheritedData()
    : indent( RenderStyle::initialTextIndent() ), line_height( RenderStyle::initialLineHeight() ),
      style_image( RenderStyle::initialListStyleImage() ),
      font(), color( RenderStyle::initialColor() ),
      border_hspacing( RenderStyle::initialBorderHorizontalSpacing() ),
      border_vspacing( RenderStyle::initialBorderVerticalSpacing() ),
      widows( RenderStyle::initialWidows() ), orphans( RenderStyle::initialOrphans() ),
      quotes(0)
{
}

StyleInheritedData::~StyleInheritedData()
{
    if (quotes) quotes->deref();
}

StyleInheritedData::StyleInheritedData(const StyleInheritedData& o )
    : Shared<StyleInheritedData>(),
      indent( o.indent ), line_height( o.line_height ), style_image( o.style_image ),
      font( o.font ), color( o.color ),
      border_hspacing( o.border_hspacing ),
      border_vspacing( o.border_vspacing ),
      widows(o.widows), orphans(o.orphans)
{
    quotes = o.quotes;
    if (quotes) quotes->ref();
}

bool StyleInheritedData::operator==(const StyleInheritedData& o) const
{
    return
	indent == o.indent &&
	line_height == o.line_height &&
        border_hspacing == o.border_hspacing &&
        border_vspacing == o.border_vspacing &&
	style_image == o.style_image &&
	font == o.font &&
	color == o.color &&
        border_hspacing == o.border_hspacing &&
        border_vspacing == o.border_vspacing &&
        quotes == o.quotes &&
        widows == o.widows &&
        orphans == o.orphans ;

    // doesn't work because structs are not packed
    //return memcmp(this, &o, sizeof(*this))==0;
}

RenderStyle::RenderStyle()
{
//    counter++;
    if (!_default)
	_default = new RenderStyle(true);

    box = _default->box;
    visual = _default->visual;
    background = _default->background;
    surround = _default->surround;
    generated = _default->generated;
    css3NonInheritedData = _default->css3NonInheritedData;
    css3InheritedData = _default->css3InheritedData;

    inherited = _default->inherited;

    m_svgStyle = _default->m_svgStyle;

    setBitDefaults();

    pseudoStyle = 0;
}

RenderStyle::RenderStyle(bool)
{
    // Let the font cache create its initial value.
    // We need this because attach can call styleForElement
    // for things with display:none parents, and then we need to be
    // able to provide some sort of fallback font data to operate on.
    Font::initDefault();

    setBitDefaults();

    box.init();
    visual.init();
    background.init();
    surround.init();
    generated.init();
    css3NonInheritedData.init();
#ifdef APPLE_CHANGES	// ### yet to be merged
    css3NonInheritedData.access()->flexibleBox.init();
#endif
    css3NonInheritedData.access()->marquee.init();
    css3NonInheritedData.access()->borderRadius.init();
    css3InheritedData.init();
    inherited.init();

    m_svgStyle.init();

    pseudoStyle = 0;
}

RenderStyle::RenderStyle(const RenderStyle& o)
    : Shared<RenderStyle>(),
      inherited_flags( o.inherited_flags ), noninherited_flags( o.noninherited_flags ),
      box( o.box ), visual( o.visual ), background( o.background ), surround( o.surround ), generated(o.generated),
      css3NonInheritedData( o.css3NonInheritedData ), css3InheritedData( o.css3InheritedData ),
      inherited( o.inherited ), pseudoStyle( 0 ), m_svgStyle(o.m_svgStyle)
{}

void RenderStyle::inheritFrom(const RenderStyle* inheritParent)
{
    css3InheritedData = inheritParent->css3InheritedData;
    inherited = inheritParent->inherited;
    inherited_flags = inheritParent->inherited_flags;

    // SVG
    if (m_svgStyle != inheritParent->m_svgStyle)
        m_svgStyle.access()->inheritFrom(inheritParent->m_svgStyle.get());

    // Simulate ":after,:before { white-space: pre-line }"
    if (styleType() == AFTER || styleType() == BEFORE)
        setWhiteSpace(PRE_LINE);
}

void RenderStyle::compactWith(const RenderStyle* similarStyle)
{
    if (this == similarStyle) return;

    if (box.get() != similarStyle->box.get() && box == similarStyle->box)
        box = similarStyle->box;
    if (visual.get() != similarStyle->visual.get() && visual == similarStyle->visual)
        visual = similarStyle->visual;
    if (background.get() != similarStyle->background.get() && background == similarStyle->background)
        background = similarStyle->background;
    if (surround.get() != similarStyle->surround.get() && surround == similarStyle->surround)
        surround = similarStyle->surround;
    if (generated.get() != similarStyle->generated.get() && generated == similarStyle->generated)
        generated = similarStyle->generated;
    if (css3NonInheritedData.get() != similarStyle->css3NonInheritedData.get() && css3NonInheritedData == similarStyle->css3NonInheritedData)
        css3NonInheritedData = similarStyle->css3NonInheritedData;
    if (css3InheritedData.get() != similarStyle->css3InheritedData.get() && css3InheritedData == similarStyle->css3InheritedData)
        css3InheritedData = similarStyle->css3InheritedData;
    if (inherited.get() != similarStyle->inherited.get() && inherited == similarStyle->inherited)
        inherited = similarStyle->inherited;
}

RenderStyle::~RenderStyle()
{
    RenderStyle *ps = pseudoStyle;
    RenderStyle *prev = 0;

    while (ps) {
        prev = ps;
        ps = ps->pseudoStyle;
	// to prevent a double deletion.
	// this works only because the styles below aren't really shared
	// Dirk said we need another construct as soon as these are shared
        prev->pseudoStyle = 0;
        prev->deref();
    }
}

bool RenderStyle::operator==(const RenderStyle& o) const
{
// compare everything except the pseudoStyle pointer
    return (inherited_flags == o.inherited_flags &&
            noninherited_flags == o.noninherited_flags &&
	    box == o.box &&
            visual == o.visual &&
            background == o.background &&
            surround == o.surround &&
            generated == o.generated &&
            css3NonInheritedData == o.css3NonInheritedData &&
            css3InheritedData == o.css3InheritedData &&
            inherited == o.inherited &&
            m_svgStyle == o.m_svgStyle); // SVG
}

enum EPseudoBit { NO_BIT = 0x0,
                  FIRST_LINE_BIT = 0x1, FIRST_LETTER_BIT = 0x2, SELECTION_BIT = 0x4,
                  BEFORE_BIT = 0x8, AFTER_BIT = 0x10, MARKER_BIT = 0x20,
                  REPLACED_BIT = 0x40
                  };

static int pseudoBit(RenderStyle::PseudoId pseudo)
{
    switch (pseudo) {
        case RenderStyle::BEFORE:
            return BEFORE_BIT;
        case RenderStyle::AFTER:
            return AFTER_BIT;
        case RenderStyle::MARKER:
            return MARKER_BIT;
        case RenderStyle::REPLACED:
            return REPLACED_BIT;
        case RenderStyle::FIRST_LINE:
            return FIRST_LINE_BIT;
        case RenderStyle::FIRST_LETTER:
            return FIRST_LETTER_BIT;
        case RenderStyle::SELECTION:
            return SELECTION_BIT;
        default:
            return NO_BIT;
    }
}

bool RenderStyle::hasPseudoStyle(PseudoId pseudo) const
{
    return (pseudoBit(pseudo) & noninherited_flags.f._pseudoBits) != 0;
}

void RenderStyle::setHasPseudoStyle(PseudoId pseudo, bool b)
{
    if (b)
        noninherited_flags.f._pseudoBits |= pseudoBit(pseudo);
    else
        noninherited_flags.f._pseudoBits &= ~(pseudoBit(pseudo));
}

RenderStyle* RenderStyle::getPseudoStyle(PseudoId pid) const
{
    if (!hasPseudoStyle(pid)) return 0;

    RenderStyle *ps = 0;
    if (noninherited_flags.f._styleType==NOPSEUDO)
        for (ps = pseudoStyle; ps; ps = ps->pseudoStyle)
            if (ps->noninherited_flags.f._styleType==pid)
                break;
    return ps;
}

RenderStyle* RenderStyle::addPseudoStyle(PseudoId pid)
{
    if (hasPseudoStyle(pid)) return getPseudoStyle(pid);

    RenderStyle *ps = 0;

        switch (pid) {
          case FIRST_LETTER:             // pseudo-elements (FIRST_LINE has a special handling)
        case SELECTION:
          case BEFORE:
          case AFTER:
            ps = new RenderStyle();
            break;
          default:
            ps = new RenderStyle(*this); // use the real copy constructor to get an identical copy
        }
        ps->ref();
        ps->noninherited_flags.f._styleType = pid;
        ps->pseudoStyle = pseudoStyle;

        pseudoStyle = ps;

    setHasPseudoStyle(pid, true);

    return ps;
}

void RenderStyle::removePseudoStyle(PseudoId pid)
{
    RenderStyle *ps = pseudoStyle;
    RenderStyle *prev = this;

    while (ps) {
        if (ps->noninherited_flags.f._styleType==pid) {
            prev->pseudoStyle = ps->pseudoStyle;
            ps->deref();
            return;
        }
        prev = ps;
        ps = ps->pseudoStyle;
    }

    setHasPseudoStyle(pid, false);
}


bool RenderStyle::inheritedNotEqual( RenderStyle *other ) const
{
    return
	(
	    inherited_flags != other->inherited_flags ||
            inherited != other->inherited ||
            css3InheritedData != other->css3InheritedData || 
            m_svgStyle->inheritedNotEqual(other->m_svgStyle.get())
	    );
}

/*
  compares two styles. The result gives an idea of the action that
  needs to be taken when replacing the old style with a new one.

  CbLayout: The containing block of the object needs a relayout.
  Layout: the RenderObject needs a relayout after the style change
  Visible: The change is visible, but no relayout is needed
  NonVisible: The object does need neither repaint nor relayout after
       the change.

  ### TODO:
  A lot can be optimised here based on the display type, lots of
  optimizations are unimplemented, and currently result in the
  worst case result causing a relayout of the containing block.
*/
RenderStyle::Diff RenderStyle::diff( const RenderStyle *other ) const
{
    if (m_svgStyle != other->m_svgStyle)
        return Layout;
    // we anyway assume they are the same
// 	EDisplay _display : 5;

    // NonVisible:
// 	ECursor _cursor_style : 4;
//	EUserInput _user_input : 2;	as long as :enabled is not impl'd

// ### this needs work to know more exactly if we need a relayout
//     or just a repaint

// non-inherited attributes
//     DataRef<StyleBoxData> box;
//     DataRef<StyleVisualData> visual;
//     DataRef<StyleSurroundData> surround;

// inherited attributes
//     DataRef<StyleInheritedData> inherited;

    if ( *box.get() != *other->box.get() ||
         *visual.get() != *other->visual.get() ||
         (*surround.get() != *other->surround.get()
           && (other->position() == PSTATIC || other->position() != position())) ||
         !(inherited->indent == other->inherited->indent) ||
         !(inherited->line_height == other->inherited->line_height) ||
         !(inherited->style_image == other->inherited->style_image) ||
         !(inherited->font == other->inherited->font) ||
         !(inherited->border_hspacing == other->inherited->border_hspacing) ||
         !(inherited->border_vspacing == other->inherited->border_vspacing) ||
         !(inherited_flags.f._visuallyOrdered == other->inherited_flags.f._visuallyOrdered) ||
         !(inherited_flags.f._htmlHacks == other->inherited_flags.f._htmlHacks) ||
         !(noninherited_flags.f._textOverflow == other->noninherited_flags.f._textOverflow) )
        return CbLayout;

    // changes causing Layout changes:

// only for tables:
// 	_border_collapse
// 	EEmptyCell _empty_cells : 2 ;
// 	ECaptionSide _caption_side : 2;
//     ETableLayout _table_layout : 1;
//     EPosition _position : 2;
//     EFloat _floating : 2;
    if ( ((int)noninherited_flags.f._display) >= TABLE ) {
        if ( !(inherited_flags.f._empty_cells == other->inherited_flags.f._empty_cells) ||
             !(inherited_flags.f._caption_side == other->inherited_flags.f._caption_side) ||
             !(inherited_flags.f._border_collapse == other->inherited_flags.f._border_collapse) ||
             !(noninherited_flags.f._table_layout == other->noninherited_flags.f._table_layout) ||
             !(noninherited_flags.f._position == other->noninherited_flags.f._position) ||
             !(noninherited_flags.f._floating == other->noninherited_flags.f._floating) ||
             !(noninherited_flags.f._flowAroundFloats == other->noninherited_flags.f._flowAroundFloats) ||
             !(noninherited_flags.f._unicodeBidi == other->noninherited_flags.f._unicodeBidi) )
            return CbLayout;
    }

// only for lists:
// 	EListStyleType _list_style_type : 5 ;
// 	EListStylePosition _list_style_position :1;
    if (noninherited_flags.f._display == LIST_ITEM ) {
        if ( !(inherited_flags.f._list_style_type == other->inherited_flags.f._list_style_type) ||
             !(inherited_flags.f._list_style_position == other->inherited_flags.f._list_style_position) )
            return Layout;
    }

// ### These could be better optimised
// 	ETextAlign _text_align : 3;
// 	ETextTransform _text_transform : 4;
// 	EDirection _direction : 1;
// 	EWhiteSpace _white_space : 2;
//     EClear _clear : 2;
    if ( !(inherited_flags.f._text_align == other->inherited_flags.f._text_align) ||
	 !(inherited_flags.f._text_transform == other->inherited_flags.f._text_transform) ||
	 !(inherited_flags.f._direction == other->inherited_flags.f._direction) ||
	 !(inherited_flags.f._white_space == other->inherited_flags.f._white_space) ||
	 !(noninherited_flags.f._clear == other->noninherited_flags.f._clear)
	)
        return Layout;

    // Overflow returns a layout hint.
    if (noninherited_flags.f._overflowX != other->noninherited_flags.f._overflowX ||
        noninherited_flags.f._overflowY != other->noninherited_flags.f._overflowY)
        return Layout;

// only for inline:
//     EVerticalAlign _vertical_align : 4;

    if ( !(noninherited_flags.f._display == INLINE) &&
         !(noninherited_flags.f._vertical_align == other->noninherited_flags.f._vertical_align) )
	    return Layout;

    if (*surround.get() != *other->surround.get()) {
        assert( other->position() != PSTATIC );                      // this style is positioned or relatively positioned
        if ( surround->hasSamePBMData(*other->surround.get()) &&    // padding/border/margin are identical
             (other->position() == PRELATIVE ||
               (!(other->left().isAuto() && other->right().isAuto()) &&  // X isn't static
                !(other->top().isAuto() && other->bottom().isAuto())) ))   // neither is Y
           // therefore only the offset is different
           return Position;
        return Layout;
    }

    // Visible:
// 	EVisibility _visibility : 2;
// 	int _text_decorations : 4;
//     DataRef<StyleBackgroundData> background;
    if (inherited->color != other->inherited->color ||
        !(inherited_flags.f._visibility == other->inherited_flags.f._visibility) ||
        !(inherited_flags.f._text_decorations == other->inherited_flags.f._text_decorations) ||
        !(noninherited_flags.f._hasClip == other->noninherited_flags.f._hasClip) ||
        visual->textDecoration != other->visual->textDecoration ||
        *background.get() != *other->background.get() ||
        css3NonInheritedData->opacity != other->css3NonInheritedData->opacity ||
        !css3InheritedData->shadowDataEquivalent(*other->css3InheritedData.get())
       )
        return Visible;

    RenderStyle::Diff ch = Equal;
    // Check for visible pseudo-changes:
    if (hasPseudoStyle(FIRST_LINE) != other->hasPseudoStyle(FIRST_LINE))
        ch = Visible;
    else
    if (hasPseudoStyle(FIRST_LINE) && other->hasPseudoStyle(FIRST_LINE))
        ch = getPseudoStyle(FIRST_LINE)->diff(other->getPseudoStyle(FIRST_LINE));

    if (ch != Equal) return ch;

    // Check for visible pseudo-changes:
    if (hasPseudoStyle(SELECTION) != other->hasPseudoStyle(SELECTION))
        ch = Visible;
    else
    if (hasPseudoStyle(SELECTION) && other->hasPseudoStyle(SELECTION))
        ch = getPseudoStyle(SELECTION)->diff(other->getPseudoStyle(SELECTION));

    return ch;
}


RenderStyle* RenderStyle::_default = 0;

void RenderStyle::cleanup()
{
    delete _default;
    _default = 0;
}

void RenderStyle::setPaletteColor(QPalette::ColorGroup g, QPalette::ColorRole r, const QColor& c)
{
    visual.access()->palette.setColor(g,r,c);
}

void RenderStyle::adjustBackgroundLayers()
{
    if (backgroundLayers()->next()) {
        // First we cull out layers that have no properties set.
        accessBackgroundLayers()->cullEmptyLayers();

        // Next we repeat patterns into layers that don't have some properties set.
        accessBackgroundLayers()->fillUnsetProperties();
    }
}

void RenderStyle::setClip( Length top, Length right, Length bottom, Length left )
{
    StyleVisualData *data = visual.access();
    data->clip.top = top;
    data->clip.right = right;
    data->clip.bottom = bottom;
    data->clip.left = left;
}

void RenderStyle::setQuotes(DOM::QuotesValueImpl* q)
{
    DOM::QuotesValueImpl *t = inherited->quotes;
    inherited.access()->quotes = q;
    if (q) q->ref();
    if (t) t->deref();
}

QString RenderStyle::openQuote(int level) const
{
    if (inherited->quotes)
        return inherited->quotes->openQuote(level);
    else if (level>1)
        return "'";
    else
        return "\""; // 0 is default quotes
}

QString RenderStyle::closeQuote(int level) const
{
    if (inherited->quotes)
        return inherited->quotes->closeQuote(level);
    else if (level)
        return "'";
    else
        return "\""; // 0 is default quotes
}

void RenderStyle::addContent(CachedObject* o)
{
    if (!o)
        return; // The object is null. Nothing to do. Just bail.

    StyleGeneratedData *t_generated = generated.access();

    ContentData* lastContent = t_generated->content;
    while (lastContent && lastContent->_nextContent)
        lastContent = lastContent->_nextContent;

    ContentData* newContentData = new ContentData;

    if (lastContent)
        lastContent->_nextContent = newContentData;
    else
        t_generated->content = newContentData;

    //    o->ref();
    newContentData->_content.object = o;
    newContentData->_contentType = CONTENT_OBJECT;
}

void RenderStyle::addContent(DOM::DOMStringImpl* s)
{
    if (!s)
        return; // The string is null. Nothing to do. Just bail.

    StyleGeneratedData *t_generated = generated.access();

    ContentData* lastContent = t_generated->content;
    while (lastContent && lastContent->_nextContent)
        lastContent = lastContent->_nextContent;

    if (lastContent) {
        if (lastContent->_contentType == CONTENT_TEXT) {
            // We can augment the existing string and share this ContentData node.
            DOMStringImpl* oldStr = lastContent->_content.text;
            DOMStringImpl* newStr = oldStr->copy();
            newStr->ref();
            oldStr->deref();
            newStr->append(s);
            lastContent->_content.text = newStr;
            return;
        }
    }

    ContentData* newContentData = new ContentData;

    if (lastContent)
        lastContent->_nextContent = newContentData;
    else
        t_generated->content = newContentData;

    newContentData->_content.text = s;
    newContentData->_content.text->ref();
    newContentData->_contentType = CONTENT_TEXT;

}

void RenderStyle::addContent(DOM::CounterImpl* c)
{
    if (!c)
        return;

    StyleGeneratedData *t_generated = generated.access();

    ContentData* lastContent = t_generated->content;
    while (lastContent && lastContent->_nextContent)
        lastContent = lastContent->_nextContent;

    ContentData* newContentData = new ContentData;

    if (lastContent)
        lastContent->_nextContent = newContentData;
    else
        t_generated->content = newContentData;

    c->ref();
    newContentData->_content.counter = c;
    newContentData->_contentType = CONTENT_COUNTER;
}

void RenderStyle::addContent(EQuoteContent q)
{
    if (q == NO_QUOTE)
        return;

    StyleGeneratedData *t_generated = generated.access();

    ContentData* lastContent = t_generated->content;
    while (lastContent && lastContent->_nextContent)
        lastContent = lastContent->_nextContent;

    ContentData* newContentData = new ContentData;

    if (lastContent)
        lastContent->_nextContent = newContentData;
    else
        t_generated->content = newContentData;

    newContentData->_content.quote = q;
    newContentData->_contentType = CONTENT_QUOTE;
}

// content: normal is the same as having no content at all
void RenderStyle::setContentNormal() {
    if (generated->content != 0) {
        delete generated->content;
        generated.access()->content = 0;
    }
}

// content: none, add an empty content node
void RenderStyle::setContentNone() {
    setContentNormal();
    generated.access()->content = new ContentData;
}

void RenderStyle::setContentData(ContentData *data) {
    if (data != generated->content) {
        if (data)
            generated.access()->content = new ContentData(*data);
        else
            generated.access()->content = 0;
    }
}

ContentData::ContentData(const ContentData& o) : _contentType(o._contentType)
{
    switch (_contentType) {
        case CONTENT_OBJECT:
            _content.object = o._content.object;
            break;
        case CONTENT_TEXT:
            _content.text = o._content.text;
            _content.text->ref();
            break;
        case CONTENT_COUNTER:
            _content.counter = o._content.counter;
            _content.counter->ref();
            break;
        case CONTENT_QUOTE:
            _content.quote = o._content.quote;
            break;
        case CONTENT_NONE:
        default:
            break;
    }

    _nextContent = o._nextContent ? new ContentData(*o._nextContent) : 0;
}

ContentData::~ContentData()
{
    clearContent();
}

void ContentData::clearContent()
{
    delete _nextContent;
    _nextContent = 0;

    switch (_contentType)
    {
        case CONTENT_OBJECT:
            _content.object = 0;
            break;
        case CONTENT_TEXT:
            _content.text->deref();
            _content.text = 0;
            break;
        case CONTENT_COUNTER:
            _content.counter->deref();
            _content.counter = 0;
            break;
        case CONTENT_QUOTE:
            _content.quote = NO_QUOTE;
            break;
        default:
            ;
    }
}

void RenderStyle::setTextShadow(ShadowData* val, bool add)
{
    StyleCSS3InheritedData* css3Data = css3InheritedData.access();
    if (!add) {
        delete css3Data->textShadow;
        css3Data->textShadow = val;
        return;
    }

    ShadowData* last = css3Data->textShadow;
    while (last->next) last = last->next;
    last->next = val;
}

ShadowData::ShadowData(const ShadowData& o)
:x(o.x), y(o.y), blur(o.blur), color(o.color)
{
    next = o.next ? new ShadowData(*o.next) : 0;
}

bool ShadowData::operator==(const ShadowData& o) const
{
    if ((next && !o.next) || (!next && o.next) ||
        (next && o.next && *next != *o.next))
        return false;

    return x == o.x && y == o.y && blur == o.blur && color == o.color;
}

static bool hasCounter(const DOM::DOMString& c, CSSValueListImpl *l)
{
    int len = l->length();
    for(int i=0; i<len; i++) {
        CounterActImpl* ca = static_cast<CounterActImpl*>(l->item(i));
        Q_ASSERT(ca != 0);
        if (ca->m_counter == c) return true;
    }
    return false;
}

bool RenderStyle::hasCounterReset(const DOM::DOMString& c) const
{
    if (generated->counter_reset)
        return hasCounter(c, generated->counter_reset);
    else
        return false;
}

bool RenderStyle::hasCounterIncrement(const DOM::DOMString& c) const
{
    if (generated->counter_increment)
        return hasCounter(c, generated->counter_increment);
    else
        return false;
}

short RenderStyle::counterReset(const DOM::DOMString& c) const
{
    if (generated->counter_reset) {
        int len = generated->counter_reset->length();
        int value = 0;
        // Return the last matching counter-reset
        for(int i=0; i<len; i++) {
            CounterActImpl* ca = static_cast<CounterActImpl*>(generated->counter_reset->item(i));
            Q_ASSERT(ca != 0);
            if (ca->m_counter == c) value = ca->m_value;
        }
        return value;
    }
    else
        return 0;
}

short RenderStyle::counterIncrement(const DOM::DOMString& c) const
{
    if (generated->counter_increment) {
        int len = generated->counter_increment->length();
        int value = 0;
        // Return the sum of matching counter-increments
        for(int i=0; i<len; i++) {
            CounterActImpl* ca = static_cast<CounterActImpl*>(generated->counter_increment->item(i));
            Q_ASSERT(ca != 0);
            if (ca->m_counter == c) value += ca->m_value;
        }
        return value;
    }
    else
        return 0;
}

void RenderStyle::setCounterReset(CSSValueListImpl *l)
{
    CSSValueListImpl *t = generated->counter_reset;
    generated.access()->counter_reset = l;
    if (l) l->ref();
    if (t) t->deref();
}

void RenderStyle::setCounterIncrement(CSSValueListImpl *l)
{
    CSSValueListImpl *t = generated->counter_increment;
    generated.access()->counter_increment = l;
    if (l) l->ref();
    if (t) t->deref();
}

#ifdef ENABLE_DUMP

static QString describeFont( const QFont &f)
{
    QString res = '\'' + f.family() + "' ";

    if ( f.pointSize() > 0)
        res += QString::number( f.pointSize() ) + "pt";
    else
        res += QString::number( f.pixelSize() ) + "px";

    if ( f.bold() )
        res += " bold";
    if ( f.italic() )
        res += " italic";
    if ( f.underline() )
        res += " underline";
    if ( f.overline() )
        res += " overline";
    if ( f.strikeOut() )
        res += " strikeout";
    return res;
}

QString RenderStyle::createDiff( const RenderStyle &parent ) const
{
    QString res;
      if ( color().isValid() && parent.color() != color() )
        res += " [color=" + color().name() + ']';
    if ( backgroundColor().isValid() && parent.backgroundColor() != backgroundColor() )
        res += " [bgcolor=" + backgroundColor().name() + ']';
    if ( parent.font() != font() )
        res += " [font=" + describeFont( font() ) + ']';

    return res;
}
#endif

RenderPageStyle::RenderPageStyle() : next(0), m_pageType(ANY_PAGE)
{
}

RenderPageStyle::~RenderPageStyle()
{
    delete next;
}

RenderPageStyle* RenderPageStyle::getPageStyle(PageType type)
{
    RenderPageStyle *ps = 0;
    for (ps = this; ps; ps = ps->next)
        if (ps->m_pageType==type)
            break;
    return ps;
}

RenderPageStyle* RenderPageStyle::addPageStyle(PageType type)
{
    RenderPageStyle *ps = getPageStyle(type);

    if (!ps)
    {
        ps = new RenderPageStyle(*this); // use the real copy constructor to get an identical copy
        ps->m_pageType = type;

        ps->next = next;
        next = ps;
    }

    return ps;
}

void RenderPageStyle::removePageStyle(PageType type)
{
    RenderPageStyle *ps = next;
    RenderPageStyle *prev = this;

    while (ps) {
        if (ps->m_pageType==type) {
            prev->next = ps->next;
            delete ps;
            return;
        }
        prev = ps;
        ps = ps->next;
    }
}
