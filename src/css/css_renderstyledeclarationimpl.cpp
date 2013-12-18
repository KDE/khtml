/**
 * css_renderstyledeclarationimpl.cpp
 *
 * Copyright 2004  Zack Rusin <zack@kde.org>
 * Copyright 2004,2005 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "css_renderstyledeclarationimpl.h"

#include "rendering/render_style.h"
#include "rendering/render_object.h"

#include "cssproperties.h"
#include "cssvalues.h"

#include <dom/dom_exception.h>

using namespace DOM;
using namespace khtml;

// List of all properties we know how to compute, omitting shorthands.
static const int computedProperties[] = {
    CSS_PROP_BACKGROUND_COLOR,
    CSS_PROP_BACKGROUND_CLIP,
    CSS_PROP_BACKGROUND_IMAGE,
    CSS_PROP_BACKGROUND_REPEAT,
    CSS_PROP_BACKGROUND_ATTACHMENT,
    CSS_PROP_BACKGROUND_POSITION,
    CSS_PROP_BACKGROUND_POSITION_X,
    CSS_PROP_BACKGROUND_POSITION_Y,
    CSS_PROP_BORDER_COLLAPSE,
    CSS_PROP_BORDER_SPACING,
    CSS_PROP__KHTML_BORDER_HORIZONTAL_SPACING,
    CSS_PROP__KHTML_BORDER_VERTICAL_SPACING,
    CSS_PROP__KHTML_BORDER_TOP_RIGHT_RADIUS,
    CSS_PROP__KHTML_BORDER_BOTTOM_RIGHT_RADIUS,
    CSS_PROP__KHTML_BORDER_BOTTOM_LEFT_RADIUS,
    CSS_PROP__KHTML_BORDER_TOP_LEFT_RADIUS,
    CSS_PROP_BORDER_TOP_RIGHT_RADIUS,
    CSS_PROP_BORDER_BOTTOM_RIGHT_RADIUS,
    CSS_PROP_BORDER_BOTTOM_LEFT_RADIUS,
    CSS_PROP_BORDER_TOP_LEFT_RADIUS,
    CSS_PROP_BORDER_TOP_COLOR,
    CSS_PROP_BORDER_RIGHT_COLOR,
    CSS_PROP_BORDER_BOTTOM_COLOR,
    CSS_PROP_BORDER_LEFT_COLOR,
    CSS_PROP_BORDER_TOP_STYLE,
    CSS_PROP_BORDER_RIGHT_STYLE,
    CSS_PROP_BORDER_BOTTOM_STYLE,
    CSS_PROP_BORDER_LEFT_STYLE,
    CSS_PROP_BORDER_TOP_WIDTH,
    CSS_PROP_BORDER_RIGHT_WIDTH,
    CSS_PROP_BORDER_BOTTOM_WIDTH,
    CSS_PROP_BORDER_LEFT_WIDTH,
    CSS_PROP_BOTTOM,
    CSS_PROP_BOX_SIZING,
    CSS_PROP_CAPTION_SIDE,
    CSS_PROP_CLEAR,
    CSS_PROP_COLOR,
    CSS_PROP_CURSOR,
    CSS_PROP_DIRECTION,
    CSS_PROP_DISPLAY,
    CSS_PROP_EMPTY_CELLS,
    CSS_PROP_FLOAT,
    CSS_PROP_FONT_FAMILY,
    CSS_PROP_FONT_SIZE,
    CSS_PROP_FONT_STYLE,
    CSS_PROP_FONT_VARIANT,
    CSS_PROP_FONT_WEIGHT,
    CSS_PROP_HEIGHT,
    CSS_PROP_LEFT,
    CSS_PROP_LETTER_SPACING,
    CSS_PROP_LINE_HEIGHT,
    CSS_PROP_LIST_STYLE_IMAGE,
    CSS_PROP_LIST_STYLE_POSITION,
    CSS_PROP_LIST_STYLE_TYPE,
    CSS_PROP_MARGIN_TOP,
    CSS_PROP_MARGIN_RIGHT,
    CSS_PROP_MARGIN_BOTTOM,
    CSS_PROP_MARGIN_LEFT,
    CSS_PROP__KHTML_MARQUEE_DIRECTION,
    CSS_PROP__KHTML_MARQUEE_INCREMENT,
    CSS_PROP__KHTML_MARQUEE_REPETITION,
    CSS_PROP__KHTML_MARQUEE_STYLE,
    CSS_PROP_MAX_HEIGHT,
    CSS_PROP_MAX_WIDTH,
    CSS_PROP_MIN_HEIGHT,
    CSS_PROP_MIN_WIDTH,
    CSS_PROP_OPACITY,
    CSS_PROP_ORPHANS,
    CSS_PROP_OUTLINE_STYLE,
    CSS_PROP_OVERFLOW,
    CSS_PROP_OVERFLOW_X,
    CSS_PROP_OVERFLOW_Y,
    CSS_PROP_PADDING_TOP,
    CSS_PROP_PADDING_RIGHT,
    CSS_PROP_PADDING_BOTTOM,
    CSS_PROP_PADDING_LEFT,
    CSS_PROP_PAGE_BREAK_AFTER,
    CSS_PROP_PAGE_BREAK_BEFORE,
    CSS_PROP_PAGE_BREAK_INSIDE,
    CSS_PROP_POSITION,
    CSS_PROP_RIGHT,
    CSS_PROP_TABLE_LAYOUT,
    CSS_PROP_TEXT_ALIGN,
    CSS_PROP_TEXT_DECORATION,
    CSS_PROP_TEXT_INDENT,
    CSS_PROP_TEXT_OVERFLOW,    
    CSS_PROP_TEXT_SHADOW,
    CSS_PROP_TEXT_TRANSFORM,
    CSS_PROP_TOP,
    CSS_PROP_UNICODE_BIDI,
    CSS_PROP_VERTICAL_ALIGN,
    CSS_PROP_VISIBILITY,
    CSS_PROP_WHITE_SPACE,
    CSS_PROP_WIDOWS,
    CSS_PROP_WIDTH,
    CSS_PROP_WORD_SPACING,
    CSS_PROP_Z_INDEX
};

const unsigned numComputedProperties = sizeof(computedProperties) / sizeof(computedProperties[0]);


static CSSValueImpl *valueForLength(const Length &length, int max)
{
    if (length.isPercent()) {
        return new CSSPrimitiveValueImpl(length.percent(), CSSPrimitiveValue::CSS_PERCENTAGE);
    }
    else {
        return new CSSPrimitiveValueImpl(length.minWidth(max), CSSPrimitiveValue::CSS_PX);
    }
}

static CSSValueImpl *valueForLength2(const Length &length)
{
    if (length.isPercent()) {
        return new CSSPrimitiveValueImpl(length.percent(), CSSPrimitiveValue::CSS_PERCENTAGE);
    }
    else {
        return new CSSPrimitiveValueImpl(length.value(), CSSPrimitiveValue::CSS_PX);
    }
}

static CSSValueImpl *valueForBorderStyle(EBorderStyle style)
{
    switch (style) {
    case khtml::BNATIVE:
        return new CSSPrimitiveValueImpl(CSS_VAL__KHTML_NATIVE);
    case khtml::BNONE:
        return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
    case khtml::BHIDDEN:
        return new CSSPrimitiveValueImpl(CSS_VAL_HIDDEN);
    case khtml::INSET:
        return new CSSPrimitiveValueImpl(CSS_VAL_INSET);
    case khtml::GROOVE:
        return new CSSPrimitiveValueImpl(CSS_VAL_GROOVE);
    case khtml::RIDGE:
         return new CSSPrimitiveValueImpl(CSS_VAL_RIDGE);
    case khtml::OUTSET:
        return new CSSPrimitiveValueImpl(CSS_VAL_OUTSET);
    case khtml::DOTTED:
        return new CSSPrimitiveValueImpl(CSS_VAL_DOTTED);
    case khtml::DASHED:
        return new CSSPrimitiveValueImpl(CSS_VAL_DASHED);
    case khtml::SOLID:
        return new CSSPrimitiveValueImpl(CSS_VAL_SOLID);
    case khtml::DOUBLE:
        return new CSSPrimitiveValueImpl(CSS_VAL_DOUBLE);
    }
    Q_ASSERT( 0 );
    return 0;
}

static CSSValueImpl *valueForBorderRadii(BorderRadii radii)
{
    CSSPrimitiveValueImpl* h = new CSSPrimitiveValueImpl(radii.horizontal, CSSPrimitiveValue::CSS_PX);
    CSSPrimitiveValueImpl* v = new CSSPrimitiveValueImpl(radii.vertical, CSSPrimitiveValue::CSS_PX);
    return new CSSPrimitiveValueImpl(new PairImpl(h, v));
}

static CSSValueImpl *valueForTextAlign(ETextAlign align)
{
    switch (align) {
    case khtml::TAAUTO:
        return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
    case khtml::LEFT:
        return new CSSPrimitiveValueImpl(CSS_VAL_LEFT);
    case khtml::RIGHT:
        return new CSSPrimitiveValueImpl(CSS_VAL_RIGHT);
    case khtml::CENTER:
        return new CSSPrimitiveValueImpl(CSS_VAL_CENTER);
    case khtml::JUSTIFY:
        return new CSSPrimitiveValueImpl(CSS_VAL_JUSTIFY);
    case khtml::KHTML_LEFT:
        return new CSSPrimitiveValueImpl(CSS_VAL__KHTML_LEFT);
    case khtml::KHTML_RIGHT:
        return new CSSPrimitiveValueImpl(CSS_VAL__KHTML_RIGHT);
    case khtml::KHTML_CENTER:
        return new CSSPrimitiveValueImpl(CSS_VAL__KHTML_CENTER);
    }
    Q_ASSERT( 0 );
    return 0;
}

DOMString khtml::stringForListStyleType(EListStyleType type)
{
    switch (type) {
        case khtml::LDISC:
            return "disc";
        case khtml::LCIRCLE:
            return "circle";
        case khtml::LSQUARE:
            return "square";
        case khtml::LBOX:
            return "box";
        case khtml::LDIAMOND:
            return "-khtml-diamond";
        case khtml::LDECIMAL:
            return "decimal";
        case khtml::DECIMAL_LEADING_ZERO:
            return "decimal-leading-zero";
        case khtml::ARABIC_INDIC:
            return "-khtml-arabic-indic";
        case khtml::LAO:
            return "-khtml-lao";
        case khtml::PERSIAN:
            return "-khtml-persian";
        case khtml::URDU:
            return "-khtml-urdu";
        case khtml::THAI:
            return "-khtml-thai";
        case khtml::TIBETAN:
            return "-khtml-tibetan";
        case khtml::LOWER_ROMAN:
            return "lower-roman";
        case khtml::UPPER_ROMAN:
            return "upper-roman";
        case khtml::HEBREW:
            return "hebrew";
        case khtml::ARMENIAN:
            return "armenian";
        case khtml::GEORGIAN:
            return "georgian";
        case khtml::CJK_IDEOGRAPHIC:
            return "cjk-ideographic";
        case khtml::JAPANESE_FORMAL:
            return "-khtml-japanese-formal";
        case khtml::JAPANESE_INFORMAL:
            return "-khtml-japanese-informal";
        case khtml::SIMP_CHINESE_FORMAL:
            return "-khtml-simp-chinese-formal";
        case khtml::SIMP_CHINESE_INFORMAL:
            return "-khtml-simp-chinese-informal";
        case khtml::TRAD_CHINESE_FORMAL:
            return "-khtml-trad-chinese-formal";
        case khtml::TRAD_CHINESE_INFORMAL:
            return "-khtml-trad-chinese-informal";
        case khtml::LOWER_GREEK:
            return "lower-greek";
        case khtml::UPPER_GREEK:
            return "-khtml-upper-greek";
        case khtml::LOWER_ALPHA:
            return "lower-alpha";
        case khtml::UPPER_ALPHA:
            return "upper-alpha";
        case khtml::LOWER_LATIN:
            return "lower-latin";
        case khtml::UPPER_LATIN:
            return "upper-latin";
        case khtml::HIRAGANA:
            return "hiragana";
        case khtml::KATAKANA:
            return "katakana";
        case khtml::HIRAGANA_IROHA:
            return "hiragana-iroha";
        case khtml::KATAKANA_IROHA:
            return "katakana_iroha";
        case khtml::LNONE:
            return "none";
    }
    Q_ASSERT( 0 );
    return "";
}

static CSSPrimitiveValueImpl* valueForColor(QColor color)
{
    if (color.isValid())
        return new CSSPrimitiveValueImpl(color.rgba());
    else
        return new CSSPrimitiveValueImpl(khtml::transparentColor);
}

static CSSValueImpl* valueForShadow(const ShadowData *shadow)
{
    if (!shadow)
        return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
    CSSValueListImpl *list = new CSSValueListImpl(CSSValueListImpl::Comma);
    for (const ShadowData *s = shadow; s; s = s->next) {
        CSSPrimitiveValueImpl *x = new CSSPrimitiveValueImpl(s->x, CSSPrimitiveValue::CSS_PX);
        CSSPrimitiveValueImpl *y = new CSSPrimitiveValueImpl(s->y, CSSPrimitiveValue::CSS_PX);
        CSSPrimitiveValueImpl *blur = new CSSPrimitiveValueImpl(s->blur, CSSPrimitiveValue::CSS_PX);
        CSSPrimitiveValueImpl *color = valueForColor(s->color);
        list->append(new ShadowValueImpl(x, y, blur, color));
    }
    return list;
}

static CSSValueImpl *getPositionOffsetValue(RenderObject *renderer, int propertyID)
{
    if (!renderer)
        return 0;

    RenderStyle *style = renderer->style();
    if (!style)
        return 0;

    Length l;
    switch (propertyID) {
    case CSS_PROP_LEFT:
        l = style->left();
        break;
    case CSS_PROP_RIGHT:
        l = style->right();
        break;
    case CSS_PROP_TOP:
        l = style->top();
        break;
    case CSS_PROP_BOTTOM:
        l = style->bottom();
        break;
    default:
        return 0;
    }

    if (renderer->isPositioned())
        return valueForLength(l, renderer->contentWidth());

    if (renderer->isRelPositioned())
        // FIXME: It's not enough to simply return "auto" values for one offset if the other side is defined.
        // In other words if left is auto and right is not auto, then left's computed value is negative right.
        // So we should get the opposite length unit and see if it is auto.
        return valueForLength(l, renderer->contentWidth());

    return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
 }

RenderStyleDeclarationImpl::RenderStyleDeclarationImpl( DOM::NodeImpl *node )
    : CSSStyleDeclarationImpl(0), m_node(node)
{
    //qDebug() << "Render Style Declaration created";
}

RenderStyleDeclarationImpl::~RenderStyleDeclarationImpl()
{
    //qDebug() << "Render Style Declaration destroyed";
}

DOM::DOMString RenderStyleDeclarationImpl::cssText() const
{
    DOMString result;

    for (unsigned i = 0; i < numComputedProperties; i++) {
        if (i != 0)
            result += " ";
        result += getPropertyName(computedProperties[i]);
        result += ": ";
        result += getPropertyValue(computedProperties[i]);
        result += ";";
    }

    return result;
}

void RenderStyleDeclarationImpl::setCssText( DOM::DOMString )
{
    // ### report that this sucka is read only
}

CSSValueImpl *RenderStyleDeclarationImpl::getPropertyCSSValue( int propertyID ) const
{
    NodeImpl *node = m_node.get();
    if (!node)
        return 0;

    // Make sure our layout is up to date before we allow a query on these attributes.
    DocumentImpl* docimpl = node->document();
    if (docimpl) {
        docimpl->updateLayout();
    }

    RenderStyle *style = node->computedStyle();
    if (!style)
        return 0;
    RenderObject *renderer = node->renderer(); // can be NULL

    // temporary(?) measure to handle with missing render object
    // check how we can better deal with it on a case-by-case basis
#define RETURN_NULL_ON_NULL(ptr) if(ptr == 0) return 0;

    switch(propertyID)
    {
    case CSS_PROP_BACKGROUND_COLOR:
        return valueForColor(style->backgroundColor());
    case CSS_PROP_BACKGROUND_CLIP:
        switch (style->backgroundLayers()->backgroundClip()) {
            case BGBORDER:
                return new CSSPrimitiveValueImpl(CSS_VAL_BORDER_BOX);
            case BGPADDING:
                return new CSSPrimitiveValueImpl(CSS_VAL_PADDING_BOX);
            case BGCONTENT:
                return new CSSPrimitiveValueImpl(CSS_VAL_CONTENT_BOX);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_BACKGROUND_IMAGE:
        if (style->backgroundImage())
            return new CSSPrimitiveValueImpl(style->backgroundImage()->url(),
                                             CSSPrimitiveValue::CSS_URI);
        return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
    case CSS_PROP_BACKGROUND_REPEAT:
        switch (style->backgroundRepeat()) {
        case khtml::REPEAT:
            return new CSSPrimitiveValueImpl(CSS_VAL_REPEAT);
        case khtml::REPEAT_X:
            return new CSSPrimitiveValueImpl(CSS_VAL_REPEAT_X);
        case khtml::REPEAT_Y:
            return new CSSPrimitiveValueImpl(CSS_VAL_REPEAT_Y);
        case khtml::NO_REPEAT:
            return new CSSPrimitiveValueImpl(CSS_VAL_NO_REPEAT);
        default:
            Q_ASSERT( 0 );
        }
        break;
    case CSS_PROP_BACKGROUND_ATTACHMENT:
        switch (style->backgroundAttachment()) {
        case khtml::BGASCROLL:
            return new CSSPrimitiveValueImpl(CSS_VAL_SCROLL);
        case khtml::BGAFIXED:
            return new CSSPrimitiveValueImpl(CSS_VAL_FIXED);
        case khtml::BGALOCAL:
            return new CSSPrimitiveValueImpl(CSS_VAL_LOCAL);
        default:
            Q_ASSERT( 0 );
        }
        break;
    case CSS_PROP_BACKGROUND_POSITION:
    {
        RETURN_NULL_ON_NULL(renderer);
        DOMString string;
        Length length(style->backgroundXPosition());
        if (length.isPercent())
            string = QString(QString::number(length.percent()) + "%");
        else
            string = QString(QString::number(length.minWidth(renderer->contentWidth())) + "px");
        string += " ";
        length = style->backgroundYPosition();
        if (length.isPercent())
            string += QString(QString::number(length.percent()) + "%");
        else
            string += QString(QString::number(length.minWidth(renderer->contentWidth())) + "px");
        return new CSSPrimitiveValueImpl(string, CSSPrimitiveValue::CSS_STRING);
    }
    case CSS_PROP_BACKGROUND_POSITION_X:
        RETURN_NULL_ON_NULL(renderer);
        return valueForLength(style->backgroundXPosition(), renderer->contentWidth());
    case CSS_PROP_BACKGROUND_POSITION_Y:
        RETURN_NULL_ON_NULL(renderer);
        return valueForLength(style->backgroundYPosition(), renderer->contentHeight());
    case CSS_PROP_BORDER_COLLAPSE:
        if (style->borderCollapse())
            return new CSSPrimitiveValueImpl(CSS_VAL_COLLAPSE);
        else
            return new CSSPrimitiveValueImpl(CSS_VAL_SEPARATE);
    case CSS_PROP_BORDER_SPACING:
    {
        QString string(QString::number(style->borderHorizontalSpacing()) +
                       "px " +
                       QString::number(style->borderVerticalSpacing()) +
                       "px");
        return new CSSPrimitiveValueImpl(DOMString(string), CSSPrimitiveValue::CSS_STRING);
    }
    case CSS_PROP__KHTML_BORDER_HORIZONTAL_SPACING:
        return new CSSPrimitiveValueImpl(style->borderHorizontalSpacing(),
                                         CSSPrimitiveValue::CSS_PX);
    case CSS_PROP__KHTML_BORDER_VERTICAL_SPACING:
        return new CSSPrimitiveValueImpl(style->borderVerticalSpacing(),
                                         CSSPrimitiveValue::CSS_PX);
    case CSS_PROP__KHTML_BORDER_TOP_RIGHT_RADIUS:
    case CSS_PROP_BORDER_TOP_RIGHT_RADIUS:
	return valueForBorderRadii(style->borderTopRightRadius());
    case CSS_PROP__KHTML_BORDER_BOTTOM_RIGHT_RADIUS:
    case CSS_PROP_BORDER_BOTTOM_RIGHT_RADIUS:
	return valueForBorderRadii(style->borderBottomRightRadius());
    case CSS_PROP__KHTML_BORDER_BOTTOM_LEFT_RADIUS:
    case CSS_PROP_BORDER_BOTTOM_LEFT_RADIUS:
	return valueForBorderRadii(style->borderBottomLeftRadius());
    case CSS_PROP__KHTML_BORDER_TOP_LEFT_RADIUS:
    case CSS_PROP_BORDER_TOP_LEFT_RADIUS:
	return valueForBorderRadii(style->borderTopLeftRadius());
    case CSS_PROP_BORDER_TOP_COLOR:
        return valueForColor(style->borderTopColor());
    case CSS_PROP_BORDER_RIGHT_COLOR:
        return valueForColor(style->borderRightColor());
    case CSS_PROP_BORDER_BOTTOM_COLOR:
        return valueForColor(style->borderBottomColor());
    case CSS_PROP_BORDER_LEFT_COLOR:
        return valueForColor(style->borderLeftColor());
    case CSS_PROP_BORDER_TOP_STYLE:
        return valueForBorderStyle(style->borderTopStyle());
    case CSS_PROP_BORDER_RIGHT_STYLE:
        return valueForBorderStyle(style->borderRightStyle());
    case CSS_PROP_BORDER_BOTTOM_STYLE:
        return valueForBorderStyle(style->borderBottomStyle());
    case CSS_PROP_BORDER_LEFT_STYLE:
        return valueForBorderStyle(style->borderLeftStyle());
    case CSS_PROP_BORDER_TOP_WIDTH:
        return new CSSPrimitiveValueImpl( style->borderTopWidth(), CSSPrimitiveValue::CSS_PX );
    case CSS_PROP_BORDER_RIGHT_WIDTH:
        return new CSSPrimitiveValueImpl( style->borderRightWidth(), CSSPrimitiveValue::CSS_PX );
    case CSS_PROP_BORDER_BOTTOM_WIDTH:
        return new CSSPrimitiveValueImpl( style->borderBottomWidth(), CSSPrimitiveValue::CSS_PX );
    case CSS_PROP_BORDER_LEFT_WIDTH:
        return new CSSPrimitiveValueImpl( style->borderLeftWidth(), CSSPrimitiveValue::CSS_PX );
    case CSS_PROP_BOTTOM:
        RETURN_NULL_ON_NULL(renderer);
        return getPositionOffsetValue(renderer, CSS_PROP_BOTTOM);
    case CSS_PROP_BOX_SIZING:
        if (style->boxSizing() == BORDER_BOX)
            return new CSSPrimitiveValueImpl(CSS_VAL_BORDER_BOX);
        else
            return new CSSPrimitiveValueImpl(CSS_VAL_CONTENT_BOX);
    case CSS_PROP_CAPTION_SIDE:
        switch (style->captionSide()) {
        case CAPLEFT:
            return new CSSPrimitiveValueImpl(CSS_VAL_LEFT);
        case CAPRIGHT:
            return new CSSPrimitiveValueImpl(CSS_VAL_RIGHT);
        case CAPTOP:
            return new CSSPrimitiveValueImpl(CSS_VAL_TOP);
        case CAPBOTTOM:
            return new CSSPrimitiveValueImpl(CSS_VAL_BOTTOM);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_CLEAR:
        switch (style->clear()) {
        case CNONE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
        case CLEFT:
            return new CSSPrimitiveValueImpl(CSS_VAL_LEFT);
        case CRIGHT:
            return new CSSPrimitiveValueImpl(CSS_VAL_RIGHT);
        case CBOTH:
            return new CSSPrimitiveValueImpl(CSS_VAL_BOTH);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_CLIP:
        break;
    case CSS_PROP_COLOR:
        return valueForColor(style->color());
    case CSS_PROP_CONTENT:
        break;
    case CSS_PROP_COUNTER_INCREMENT:
        break;
    case CSS_PROP_COUNTER_RESET:
        break;
    case CSS_PROP_CURSOR:
        switch (style->cursor()) {
        case CURSOR_AUTO:
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        case CURSOR_DEFAULT:
            return new CSSPrimitiveValueImpl(CSS_VAL_DEFAULT);
        case CURSOR_NONE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
        case CURSOR_CONTEXT_MENU:
            return new CSSPrimitiveValueImpl(CSS_VAL_CONTEXT_MENU);
        case CURSOR_HELP:
            return new CSSPrimitiveValueImpl(CSS_VAL_HELP);
        case CURSOR_POINTER:
            return new CSSPrimitiveValueImpl(CSS_VAL_POINTER);
        case CURSOR_PROGRESS:
            return new CSSPrimitiveValueImpl(CSS_VAL_PROGRESS);
        case CURSOR_WAIT:
            return new CSSPrimitiveValueImpl(CSS_VAL_WAIT);
        case CURSOR_CELL:
            return new CSSPrimitiveValueImpl(CSS_VAL_CELL);
        case CURSOR_CROSS:
            return new CSSPrimitiveValueImpl(CSS_VAL_CROSSHAIR);
        case CURSOR_TEXT:
            return new CSSPrimitiveValueImpl(CSS_VAL_TEXT);
        case CURSOR_VERTICAL_TEXT:
            return new CSSPrimitiveValueImpl(CSS_VAL_VERTICAL_TEXT);
        case CURSOR_ALIAS:
            return new CSSPrimitiveValueImpl(CSS_VAL_ALIAS);
        case CURSOR_COPY:
            return new CSSPrimitiveValueImpl(CSS_VAL_COPY);
        case CURSOR_MOVE:
            return new CSSPrimitiveValueImpl(CSS_VAL_MOVE);
        case CURSOR_NO_DROP:
            return new CSSPrimitiveValueImpl(CSS_VAL_NO_DROP);
        case CURSOR_NOT_ALLOWED:
            return new CSSPrimitiveValueImpl(CSS_VAL_NOT_ALLOWED);
        case CURSOR_E_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_E_RESIZE);
        case CURSOR_N_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_N_RESIZE);
        case CURSOR_NE_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NE_RESIZE);
        case CURSOR_NW_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NW_RESIZE);
        case CURSOR_S_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_S_RESIZE);
        case CURSOR_SE_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_SE_RESIZE);
        case CURSOR_SW_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_SW_RESIZE);
        case CURSOR_W_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_W_RESIZE);
        case CURSOR_EW_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_EW_RESIZE);
        case CURSOR_NS_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NS_RESIZE);
        case CURSOR_NESW_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NESW_RESIZE);
        case CURSOR_NWSE_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NWSE_RESIZE);
        case CURSOR_COL_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_COL_RESIZE);
        case CURSOR_ROW_RESIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_ROW_RESIZE);
        case CURSOR_ALL_SCROLL:
            return new CSSPrimitiveValueImpl(CSS_VAL_ALL_SCROLL);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_DIRECTION:
        switch (style->direction()) {
        case LTR:
            return new CSSPrimitiveValueImpl(CSS_VAL_LTR);
        case RTL:
            return new CSSPrimitiveValueImpl(CSS_VAL_RTL);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_DISPLAY:
        switch (style->display()) {
        case INLINE:
            return new CSSPrimitiveValueImpl(CSS_VAL_INLINE);
        case BLOCK:
            return new CSSPrimitiveValueImpl(CSS_VAL_BLOCK);
        case LIST_ITEM:
            return new CSSPrimitiveValueImpl(CSS_VAL_LIST_ITEM);
        case RUN_IN:
            return new CSSPrimitiveValueImpl(CSS_VAL_RUN_IN);
        case COMPACT:
            return new CSSPrimitiveValueImpl(CSS_VAL_COMPACT);
        case INLINE_BLOCK:
            return new CSSPrimitiveValueImpl(CSS_VAL_INLINE_BLOCK);
        case TABLE:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE);
        case INLINE_TABLE:
            return new CSSPrimitiveValueImpl(CSS_VAL_INLINE_TABLE);
        case TABLE_ROW_GROUP:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE_ROW_GROUP);
        case TABLE_HEADER_GROUP:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE_HEADER_GROUP);
        case TABLE_FOOTER_GROUP:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE_FOOTER_GROUP);
        case TABLE_ROW:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE_ROW);
        case TABLE_COLUMN_GROUP:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE_COLUMN_GROUP);
        case TABLE_COLUMN:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE_COLUMN);
        case TABLE_CELL:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE_CELL);
        case TABLE_CAPTION:
            return new CSSPrimitiveValueImpl(CSS_VAL_TABLE_CAPTION);
        case NONE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
        }
        Q_ASSERT( 0 );
        break;
    case CSS_PROP_EMPTY_CELLS:
        switch (style->emptyCells()) {
        case SHOW:
            return new CSSPrimitiveValueImpl(CSS_VAL_SHOW);
        case HIDE:
            return new CSSPrimitiveValueImpl(CSS_VAL_HIDE);
        }
        Q_ASSERT( 0 );
        break;
    case CSS_PROP_FLOAT:
    {
        switch (style->floating()) {
        case FNONE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
        case FLEFT:
            return new CSSPrimitiveValueImpl(CSS_VAL_LEFT);
        case FRIGHT:
            return new CSSPrimitiveValueImpl(CSS_VAL_RIGHT);
        case FLEFT_ALIGN:
            return new CSSPrimitiveValueImpl(CSS_VAL__KHTML_LEFT);
        case FRIGHT_ALIGN:
            return new CSSPrimitiveValueImpl(CSS_VAL__KHTML_RIGHT);
        }
        Q_ASSERT( 0 );
        break;
    }
    case CSS_PROP_FONT_FAMILY:
    {
        FontDef def = style->htmlFont().getFontDef();
        return new CSSPrimitiveValueImpl(DOMString(def.family), CSSPrimitiveValue::CSS_STRING);
    }
    case CSS_PROP_FONT_SIZE:
    {
        FontDef def = style->htmlFont().getFontDef();
        return new CSSPrimitiveValueImpl(def.size, CSSPrimitiveValue::CSS_PX);
    }
    case CSS_PROP_FONT_STYLE:
    {
        // FIXME: handle oblique
        FontDef def = style->htmlFont().getFontDef();
        if (def.italic)
            return new CSSPrimitiveValueImpl(CSS_VAL_ITALIC);
        else
            return new CSSPrimitiveValueImpl(CSS_VAL_NORMAL);
    }
    case CSS_PROP_FONT_VARIANT:
    {
        FontDef def = style->htmlFont().getFontDef();
        if (def.smallCaps)
            return new CSSPrimitiveValueImpl(CSS_VAL_SMALL_CAPS);
        else
            return new CSSPrimitiveValueImpl(CSS_VAL_NORMAL);
    }
    case CSS_PROP_FONT_WEIGHT:
    {
        // FIXME: this does not reflect the full range of weights
        // that can be expressed with CSS
        FontDef def = style->htmlFont().getFontDef();
        if (def.weight == QFont::Bold)
            return new CSSPrimitiveValueImpl(CSS_VAL_BOLD);
        else
            return new CSSPrimitiveValueImpl(CSS_VAL_NORMAL);
    }
    case CSS_PROP_HEIGHT:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->contentHeight(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->height());
    case CSS_PROP_LEFT:
        RETURN_NULL_ON_NULL(renderer);
        return getPositionOffsetValue(renderer, CSS_PROP_LEFT);
    case CSS_PROP_LETTER_SPACING:
        if (style->letterSpacing() == 0)
            return new CSSPrimitiveValueImpl(CSS_VAL_NORMAL);
        return new CSSPrimitiveValueImpl(style->letterSpacing(), CSSPrimitiveValue::CSS_PX);
    case CSS_PROP_LINE_HEIGHT:
    {
        // Note: internally a specified <number> value gets encoded as a percentage,
        // so the isPercent() case corresponds to the <number> case;
        // values < 0  are used to mark "normal"; and specified %%
        // get computed down to px by the time they get to RenderStyle
        // already
        Length length(style->lineHeight());
        if (length.isNegative())
            return new CSSPrimitiveValueImpl(CSS_VAL_NORMAL);
        if (length.isPercent()) {
            //XXX: merge from webcore the computedStyle/specifiedStyle distinction in rendering/font.h
            float computedSize = style->htmlFont().getFontDef().size;
            return new CSSPrimitiveValueImpl((int)(length.percent() * computedSize) / 100, CSSPrimitiveValue::CSS_PX);
        }
        else {
            return new CSSPrimitiveValueImpl(length.value(), CSSPrimitiveValue::CSS_PX);
        }
    }
    case CSS_PROP_LIST_STYLE_IMAGE:
        if (style->listStyleImage())
            return new CSSPrimitiveValueImpl(style->listStyleImage()->url(), CSSPrimitiveValue::CSS_URI);
        return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
    case CSS_PROP_LIST_STYLE_POSITION:
        switch (style->listStylePosition()) {
        case OUTSIDE:
            return new CSSPrimitiveValueImpl(CSS_VAL_OUTSIDE);
        case INSIDE:
            return new CSSPrimitiveValueImpl(CSS_VAL_INSIDE);
        }
        Q_ASSERT( 0 );
        break;
    case CSS_PROP_LIST_STYLE_TYPE:
        return new CSSPrimitiveValueImpl(stringForListStyleType(style->listStyleType()), CSSPrimitiveValue::CSS_STRING);
    case CSS_PROP_MARGIN_TOP:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->marginTop(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->marginTop());
    case CSS_PROP_MARGIN_RIGHT:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->marginRight(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->marginRight());
    case CSS_PROP_MARGIN_BOTTOM:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->marginBottom(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->marginBottom());
    case CSS_PROP_MARGIN_LEFT:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->marginLeft(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->marginLeft());
    case CSS_PROP__KHTML_MARQUEE:
        // FIXME: unimplemented
        break;
    case CSS_PROP__KHTML_MARQUEE_DIRECTION:
        switch (style->marqueeDirection()) {
        case MFORWARD:
            return new CSSPrimitiveValueImpl(CSS_VAL_FORWARDS);
        case MBACKWARD:
            return new CSSPrimitiveValueImpl(CSS_VAL_BACKWARDS);
        case MAUTO:
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        case MUP:
            return new CSSPrimitiveValueImpl(CSS_VAL_UP);
        case MDOWN:
            return new CSSPrimitiveValueImpl(CSS_VAL_DOWN);
        case MLEFT:
            return new CSSPrimitiveValueImpl(CSS_VAL_LEFT);
        case MRIGHT:
            return new CSSPrimitiveValueImpl(CSS_VAL_RIGHT);
        }
        Q_ASSERT(0);
        return 0;
    case CSS_PROP__KHTML_MARQUEE_INCREMENT:
        RETURN_NULL_ON_NULL(renderer);
        return valueForLength(style->marqueeIncrement(), renderer->contentWidth());
    case CSS_PROP__KHTML_MARQUEE_REPETITION:
        if (style->marqueeLoopCount() < 0)
            return new CSSPrimitiveValueImpl(CSS_VAL_INFINITE);
        return new CSSPrimitiveValueImpl(style->marqueeLoopCount(), CSSPrimitiveValue::CSS_NUMBER);
    case CSS_PROP__KHTML_MARQUEE_SPEED:
        // FIXME: unimplemented
        break;
    case CSS_PROP__KHTML_MARQUEE_STYLE:
        switch (style->marqueeBehavior()) {
        case MNONE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
        case MSCROLL:
            return new CSSPrimitiveValueImpl(CSS_VAL_SCROLL);
        case MSLIDE:
            return new CSSPrimitiveValueImpl(CSS_VAL_SLIDE);
        case MALTERNATE:
            return new CSSPrimitiveValueImpl(CSS_VAL_ALTERNATE);
        case MUNFURL:
            return new CSSPrimitiveValueImpl(CSS_VAL_UNFURL);
        }
        Q_ASSERT(0);
        return 0;
    case CSS_PROP_MAX_HEIGHT:
        RETURN_NULL_ON_NULL(renderer);
        return new CSSPrimitiveValueImpl( renderer->availableHeight(),
                                          CSSPrimitiveValue::CSS_PX );
        break;
    case CSS_PROP_MAX_WIDTH:
        RETURN_NULL_ON_NULL(renderer);
        return new CSSPrimitiveValueImpl( renderer->maxWidth(),
                                          CSSPrimitiveValue::CSS_PX );
        break;
    case CSS_PROP_MIN_HEIGHT:
        RETURN_NULL_ON_NULL(renderer);
        return new CSSPrimitiveValueImpl( renderer->contentHeight(),
                                          CSSPrimitiveValue::CSS_PX );
        break;
    case CSS_PROP_MIN_WIDTH:
        RETURN_NULL_ON_NULL(renderer);
        return new CSSPrimitiveValueImpl( renderer->minWidth(),
                                          CSSPrimitiveValue::CSS_PX );
        break;
    case CSS_PROP_OPACITY:
        return new CSSPrimitiveValueImpl(style->opacity(), CSSPrimitiveValue::CSS_NUMBER);
    case CSS_PROP_ORPHANS:
        return new CSSPrimitiveValueImpl(style->orphans(), CSSPrimitiveValue::CSS_NUMBER);
    case CSS_PROP_OUTLINE_COLOR:
        break;
    case CSS_PROP_OUTLINE_OFFSET:
        break;
    case CSS_PROP_OUTLINE_STYLE:
        if (style->outlineStyleIsAuto())
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        return valueForBorderStyle(style->outlineStyle());
    case CSS_PROP_OUTLINE_WIDTH:
        break;
    case CSS_PROP_OVERFLOW:
    case CSS_PROP_OVERFLOW_X:
    case CSS_PROP_OVERFLOW_Y: {
        EOverflow overflow;
        switch (propertyID) {
        case CSS_PROP_OVERFLOW_X:
            overflow = style->overflowX();
            break;
        case CSS_PROP_OVERFLOW_Y:
            overflow = style->overflowY();
            break;
        default:
            overflow = qMax(style->overflowX(), style->overflowY());
        }
        switch (overflow) {
        case OVISIBLE:
            return new CSSPrimitiveValueImpl(CSS_VAL_VISIBLE);
        case OHIDDEN:
            return new CSSPrimitiveValueImpl(CSS_VAL_HIDDEN);
        case OSCROLL:
            return new CSSPrimitiveValueImpl(CSS_VAL_SCROLL);
        case OAUTO:
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        case OMARQUEE:
            return new CSSPrimitiveValueImpl(CSS_VAL_MARQUEE);
        }
        Q_ASSERT(0);
        return 0;
    }
    case CSS_PROP_PADDING_TOP:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->paddingTop(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->paddingTop());
    case CSS_PROP_PADDING_RIGHT:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->paddingRight(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->paddingRight());
    case CSS_PROP_PADDING_BOTTOM:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->paddingBottom(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->paddingBottom());
    case CSS_PROP_PADDING_LEFT:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->paddingLeft(), CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->paddingLeft());
    case CSS_PROP_PAGE_BREAK_AFTER:
        switch (style->pageBreakAfter()) {
        case PBAUTO:
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        case PBALWAYS:
            return new CSSPrimitiveValueImpl(CSS_VAL_ALWAYS);
        case PBAVOID:
            return new CSSPrimitiveValueImpl(CSS_VAL_AVOID);
        case PBLEFT:
            return new CSSPrimitiveValueImpl(CSS_VAL_LEFT);
        case PBRIGHT:
            return new CSSPrimitiveValueImpl(CSS_VAL_RIGHT);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_PAGE_BREAK_BEFORE:
        switch (style->pageBreakBefore()) {
        case PBAUTO:
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        case PBALWAYS:
            return new CSSPrimitiveValueImpl(CSS_VAL_ALWAYS);
        case PBAVOID:
            return new CSSPrimitiveValueImpl(CSS_VAL_AVOID);
        case PBLEFT:
            return new CSSPrimitiveValueImpl(CSS_VAL_LEFT);
        case PBRIGHT:
            return new CSSPrimitiveValueImpl(CSS_VAL_RIGHT);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_PAGE_BREAK_INSIDE:
        if (style->pageBreakInside())
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        else
            return new CSSPrimitiveValueImpl(CSS_VAL_AVOID);
        Q_ASSERT(0);
        break;
    case CSS_PROP_POSITION:
        switch (style->position()) {
        case PSTATIC:
            return new CSSPrimitiveValueImpl(CSS_VAL_STATIC);
        case PRELATIVE:
            return new CSSPrimitiveValueImpl(CSS_VAL_RELATIVE);
        case PABSOLUTE:
            return new CSSPrimitiveValueImpl(CSS_VAL_ABSOLUTE);
        case PFIXED:
            return new CSSPrimitiveValueImpl(CSS_VAL_FIXED);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_QUOTES:
        break;
    case CSS_PROP_RIGHT:
        RETURN_NULL_ON_NULL(renderer);
        return getPositionOffsetValue(renderer, CSS_PROP_RIGHT);
    case CSS_PROP_SIZE:
        break;
    case CSS_PROP_TABLE_LAYOUT:
        switch (style->tableLayout()) {
        case TAUTO:
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        case TFIXED:
            return new CSSPrimitiveValueImpl(CSS_VAL_FIXED);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_TEXT_ALIGN:
        return valueForTextAlign(style->textAlign());
    case CSS_PROP_TEXT_DECORATION:
    {
        QString string;
        if (style->textDecoration() & khtml::UNDERLINE)
            string += "underline";
        if (style->textDecoration() & khtml::OVERLINE) {
            if (string.length() > 0)
                string += " ";
            string += "overline";
        }
        if (style->textDecoration() & khtml::LINE_THROUGH) {
            if (string.length() > 0)
                string += " ";
            string += "line-through";
        }
        if (style->textDecoration() & khtml::BLINK) {
            if (string.length() > 0)
                string += " ";
            string += "blink";
        }
        if (string.length() == 0)
            string = "none";
        return new CSSPrimitiveValueImpl(DOMString(string), CSSPrimitiveValue::CSS_STRING);
    }
    case CSS_PROP_TEXT_INDENT:
        RETURN_NULL_ON_NULL(renderer);
        return valueForLength(style->textIndent(), renderer->contentWidth());
    case CSS_PROP_TEXT_SHADOW:
        return valueForShadow(style->textShadow());
    case CSS_PROP_TEXT_TRANSFORM:
        switch (style->textTransform()) {
        case CAPITALIZE:
            return new CSSPrimitiveValueImpl(CSS_VAL_CAPITALIZE);
        case UPPERCASE:
            return new CSSPrimitiveValueImpl(CSS_VAL_UPPERCASE);
        case LOWERCASE:
            return new CSSPrimitiveValueImpl(CSS_VAL_LOWERCASE);
        case TTNONE:
            return new CSSPrimitiveValueImpl(CSS_VAL_NONE);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_TOP:
        RETURN_NULL_ON_NULL(renderer);
        return getPositionOffsetValue(renderer, CSS_PROP_TOP);
    case CSS_PROP_UNICODE_BIDI:
        switch (style->unicodeBidi()) {
        case UBNormal:
            return new CSSPrimitiveValueImpl(CSS_VAL_NORMAL);
        case Embed:
            return new CSSPrimitiveValueImpl(CSS_VAL_EMBED);
        case Override:
            return new CSSPrimitiveValueImpl(CSS_VAL_BIDI_OVERRIDE);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_VERTICAL_ALIGN:
    {
        switch (style->verticalAlign()) {
        case BASELINE:
            return new CSSPrimitiveValueImpl(CSS_VAL_BASELINE);
        case MIDDLE:
            return new CSSPrimitiveValueImpl(CSS_VAL_MIDDLE);
        case SUB:
            return new CSSPrimitiveValueImpl(CSS_VAL_SUB);
        case SUPER:
            return new CSSPrimitiveValueImpl(CSS_VAL_SUPER);
        case TEXT_TOP:
            return new CSSPrimitiveValueImpl(CSS_VAL_TEXT_TOP);
        case TEXT_BOTTOM:
            return new CSSPrimitiveValueImpl(CSS_VAL_TEXT_BOTTOM);
        case TOP:
            return new CSSPrimitiveValueImpl(CSS_VAL_TOP);
        case BOTTOM:
            return new CSSPrimitiveValueImpl(CSS_VAL_BOTTOM);
        case BASELINE_MIDDLE:
            return new CSSPrimitiveValueImpl(CSS_VAL__KHTML_BASELINE_MIDDLE);
        case LENGTH:
            RETURN_NULL_ON_NULL(renderer);
            return valueForLength(style->verticalAlignLength(), renderer->contentWidth());
        }
        Q_ASSERT(0);
        break;
    }
    case CSS_PROP_VISIBILITY:
        switch (style->visibility()) {
            case khtml::VISIBLE:
                return new CSSPrimitiveValueImpl(CSS_VAL_VISIBLE);
            case khtml::HIDDEN:
                return new CSSPrimitiveValueImpl(CSS_VAL_HIDDEN);
            case khtml::COLLAPSE:
                return new CSSPrimitiveValueImpl(CSS_VAL_COLLAPSE);
        }
        Q_ASSERT(0);
        break;
    case CSS_PROP_WHITE_SPACE:
    {
        switch (style->whiteSpace()) {
        case NORMAL:
            return new CSSPrimitiveValueImpl(CSS_VAL_NORMAL);
        case PRE:
            return new CSSPrimitiveValueImpl(CSS_VAL_PRE);
        case PRE_WRAP:
            return new CSSPrimitiveValueImpl(CSS_VAL_PRE_WRAP);
        case PRE_LINE:
            return new CSSPrimitiveValueImpl(CSS_VAL_PRE_LINE);
        case NOWRAP:
            return new CSSPrimitiveValueImpl(CSS_VAL_NOWRAP);
        case KHTML_NOWRAP:
            return new CSSPrimitiveValueImpl(CSS_VAL__KHTML_NOWRAP);
        }
        Q_ASSERT(0);
        break;
    }
    case CSS_PROP_WIDOWS:
        return new CSSPrimitiveValueImpl(style->widows(), CSSPrimitiveValue::CSS_NUMBER);
    case CSS_PROP_WIDTH:
        if (renderer)
            return new CSSPrimitiveValueImpl(renderer->contentWidth(),
                                             CSSPrimitiveValue::CSS_PX);
        return valueForLength2(style->width());
    case CSS_PROP_WORD_SPACING:
        return new CSSPrimitiveValueImpl(style->wordSpacing(), CSSPrimitiveValue::CSS_PX);
    case CSS_PROP_Z_INDEX:
        if (style->hasAutoZIndex())
            return new CSSPrimitiveValueImpl(CSS_VAL_AUTO);
        return new CSSPrimitiveValueImpl(style->zIndex(), CSSPrimitiveValue::CSS_NUMBER);
    case CSS_PROP_BACKGROUND:
        break;
    case CSS_PROP_BORDER:
        break;
    case CSS_PROP_BORDER_COLOR:
        break;
    case CSS_PROP_BORDER_STYLE:
        break;
    case CSS_PROP_BORDER_TOP:
        RETURN_NULL_ON_NULL(renderer);
        return new CSSPrimitiveValueImpl( renderer->borderTop(),
                                         CSSPrimitiveValue::CSS_PX );
        break;
    case CSS_PROP_BORDER_RIGHT:
        RETURN_NULL_ON_NULL(renderer);
        return new CSSPrimitiveValueImpl( renderer->borderRight(),
                                         CSSPrimitiveValue::CSS_PX );
        break;
    case CSS_PROP_BORDER_BOTTOM:
        RETURN_NULL_ON_NULL(renderer);
        return new CSSPrimitiveValueImpl( renderer->borderBottom(),
                                         CSSPrimitiveValue::CSS_PX );
        break;
    case CSS_PROP_BORDER_LEFT:
        RETURN_NULL_ON_NULL(renderer);
        return new CSSPrimitiveValueImpl( renderer->borderLeft(),
                                         CSSPrimitiveValue::CSS_PX );
        break;
    case CSS_PROP_BORDER_WIDTH:
        break;
    case CSS_PROP_FONT:
        break;
    case CSS_PROP_LIST_STYLE:
        break;
    case CSS_PROP_MARGIN:
        break;
    case CSS_PROP_OUTLINE:
        break;
    case CSS_PROP_PADDING:
        break;
    case CSS_PROP_SCROLLBAR_BASE_COLOR:
        break;
    case CSS_PROP_SCROLLBAR_FACE_COLOR:
        break;
    case CSS_PROP_SCROLLBAR_SHADOW_COLOR:
        break;
    case CSS_PROP_SCROLLBAR_HIGHLIGHT_COLOR:
        break;
    case CSS_PROP_SCROLLBAR_3DLIGHT_COLOR:
        break;
    case CSS_PROP_SCROLLBAR_DARKSHADOW_COLOR:
        break;
    case CSS_PROP_SCROLLBAR_TRACK_COLOR:
        break;
    case CSS_PROP_SCROLLBAR_ARROW_COLOR:
        break;
    case CSS_PROP__KHTML_FLOW_MODE:
        break;
    case CSS_PROP__KHTML_USER_INPUT:
        break;
    case CSS_PROP_TEXT_OVERFLOW:
        if (style->textOverflow())
            return new CSSPrimitiveValueImpl(CSS_VAL_ELLIPSIS);
        else
            return new CSSPrimitiveValueImpl(CSS_VAL_CLIP);
        break;
    default:
        qWarning() << "Unhandled property:" << getPropertyName(propertyID);
        //Q_ASSERT( 0 );
        break;
    }
    return 0;
}

#undef RETURN_NULL_ON_NULL

DOMString RenderStyleDeclarationImpl::getPropertyValue( int propertyID ) const
{
    CSSValueImpl* value = getPropertyCSSValue(propertyID);
    if (value) {
        DOMString val = value->cssText();
        delete value;
        return val;
    }
    return "";
}

bool RenderStyleDeclarationImpl::getPropertyPriority( int ) const
{
    // All computed styles have a priority of false (not "important").
    return false;
}

void RenderStyleDeclarationImpl::removeProperty(int, DOM::DOMString*)
{
    // ### emit error since we're read-only
}

void RenderStyleDeclarationImpl::removePropertiesInSet(const int*, unsigned)
{
     // ### emit error since we're read-only
}

bool RenderStyleDeclarationImpl::setProperty ( int, const DOM::DOMString &, bool, int &ec)
{
    ec = DOMException::NO_MODIFICATION_ALLOWED_ERR;
    return false;
}

bool RenderStyleDeclarationImpl::setProperty ( int, const DOM::DOMString&, bool )
{
    // ### emit error since we're read-only
    return false;
}

void RenderStyleDeclarationImpl::setProperty ( int, int, bool )
{
    // ### emit error since we're read-only
}

void RenderStyleDeclarationImpl::setLengthProperty( int, const DOM::DOMString&, bool,
                                                    bool )
{
    // ### emit error since we're read-only
}

void RenderStyleDeclarationImpl::setProperty( const DOMString& )
{
    // ### emit error since we're read-only
}

unsigned long RenderStyleDeclarationImpl::length() const
{
    return numComputedProperties;
}

DOM::DOMString RenderStyleDeclarationImpl::item( unsigned long i ) const
{
    if (i >= numComputedProperties)
        return DOMString();

    return getPropertyName(computedProperties[i]);
}

CSSProperty RenderStyleDeclarationImpl::property( int id ) const
{
    CSSProperty prop;
    prop.m_id = id;
    prop.m_important = false;

    CSSValueImpl* v = getPropertyCSSValue( id );
    if ( !v )
        v = new CSSPrimitiveValueImpl;
    prop.setValue( v );
    return prop;
}

