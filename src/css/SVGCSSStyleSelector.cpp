/*
    Copyright (C) 2005 Apple Computer, Inc.
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
    Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
    Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
    Copyright (C) 2009 Maksim Orlovich <maksim@kde.org>

    Based on khtml css code by:
    Copyright (C) 1999-2003 Lars Knoll <knoll@kde.org>
    Copyright (C) 2003 Apple Computer, Inc.
    Copyright (C) 2004 Allan Sandfeld Jensen <kde@carewolf.com>
    Copyright (C) 2004 Germain Garand <germain@ebooksfrance.org>

    This file is part of the KDE project

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

#include "cssstyleselector.h"
#include "css_valueimpl.h"
#include "css_svgvalueimpl.h"
#include "cssvalues.h"

#include "SVGNames.h"
#include "SVGRenderStyle.h"
#include "SVGRenderStyleDefs.h"
#include "SVGStyledElement.h"

#include <wtf/MathExtras.h>

#define HANDLE_INHERIT(prop, Prop) \
if (isInherit) \
{\
    svgstyle->set##Prop(parentStyle->svgStyle()->prop());\
    return;\
}

#define HANDLE_INHERIT_AND_INITIAL(prop, Prop) \
HANDLE_INHERIT(prop, Prop) \
else if (isInitial) \
    svgstyle->set##Prop(SVGRenderStyle::initial##Prop());

#define HANDLE_INHERIT_COND(propID, prop, Prop) \
if (id == propID) \
{\
    svgstyle->set##Prop(parentStyle->svgStyle()->prop());\
    return;\
}

#define HANDLE_INITIAL_COND(propID, Prop) \
if (id == propID) \
{\
    svgstyle->set##Prop(SVGRenderStyle::initial##Prop());\
    return;\
}

#define HANDLE_INITIAL_COND_WITH_VALUE(propID, Prop, Value) \
if (id == propID) { \
    svgstyle->set##Prop(SVGRenderStyle::initial##Value()); \
    return; \
}

namespace khtml {

using namespace DOM;
using namespace WebCore;

static SVGPaintImpl* toPaint(CSSValueImpl* val)
{
    if (val->cssValueType() != DOM::CSSValue::CSS_SVG_VALUE)
        return 0;
    SVGCSSValueImpl* svgVal = static_cast<SVGCSSValueImpl*>(val);
    if (svgVal->isSVGPaint())
        return static_cast<SVGPaintImpl*>(svgVal);
    else
        return 0;
}

static SVGColorImpl* toColor(CSSValueImpl* val)
{
    if (val->cssValueType() != DOM::CSSValue::CSS_SVG_VALUE)
        return 0;
    SVGCSSValueImpl* svgVal = static_cast<SVGCSSValueImpl*>(val);
    if (svgVal->isSVGColor())
        return static_cast<SVGColorImpl*>(svgVal);
    else
        return 0;
}

static float roundToNearestGlyphOrientationAngle(float angle)
{
    angle = fabsf(fmodf(angle, 360.0f));

    if (angle <= 45.0f || angle > 315.0f)
        return 0.0f;
    else if (angle > 45.0f && angle <= 135.0f)
        return 90.0f;
    else if (angle > 135.0f && angle <= 225.0f)
        return 180.0f;

    return 270.0f;
}

static int angleToGlyphOrientation(float angle)
{
    angle = roundToNearestGlyphOrientationAngle(angle);

    if (angle == 0.0f)
        return GO_0DEG;
    else if (angle == 90.0f)
        return GO_90DEG;
    else if (angle == 180.0f)
        return GO_180DEG;
    else if (angle == 270.0f)
        return GO_270DEG;

    return -1;
}

static EColorInterpolation colorInterpolationForValue(DOM::CSSPrimitiveValueImpl* primitiveValue)
{
    if (!primitiveValue)
        return CI_AUTO;

    switch (primitiveValue->getIdent()) {
    case CSS_VAL_SRGB:
        return CI_SRGB;
    case CSS_VAL_LINEARRGB:
        return CI_LINEARRGB;
    case CSS_VAL_AUTO:
    default:
        return CI_AUTO;
    }
}

void CSSStyleSelector::applySVGRule(int id, DOM::CSSValueImpl* value)
{
    CSSPrimitiveValueImpl* primitiveValue = 0;
    if (value->isPrimitiveValue())
        primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);

    SVGRenderStyle* svgstyle = style->accessSVGStyle();
    unsigned short valueType = value->cssValueType();
    
    bool isInherit = parentNode && valueType == CSSPrimitiveValue::CSS_INHERIT;
    bool isInitial = valueType == CSSPrimitiveValue::CSS_INITIAL || (!parentNode && valueType == CSSPrimitiveValue::CSS_INHERIT);

    // What follows is a list that maps the CSS properties into their
    // corresponding front-end RenderStyle values. Shorthands(e.g. border,
    // background) occur in this list as well and are only hit when mapping
    // "inherit" or "initial" into front-end values.
    switch (id) {
        // ident only properties
        case CSS_PROP_ALIGNMENT_BASELINE:
        {
            HANDLE_INHERIT_AND_INITIAL(alignmentBaseline, AlignmentBaseline)
            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
            case CSS_VAL_AUTO:
                svgstyle->setAlignmentBaseline(AB_AUTO);
                break;
            case CSS_VAL_BASELINE:
                svgstyle->setAlignmentBaseline(AB_BASELINE);
                break;
            case CSS_VAL_BEFORE_EDGE:
                svgstyle->setAlignmentBaseline(AB_BEFORE_EDGE);
                break;
            case CSS_VAL_TEXT_BEFORE_EDGE:
                svgstyle->setAlignmentBaseline(AB_TEXT_BEFORE_EDGE);
                break;
            case CSS_VAL_MIDDLE:
                svgstyle->setAlignmentBaseline(AB_MIDDLE);
                break;
            case CSS_VAL_CENTRAL:
                svgstyle->setAlignmentBaseline(AB_CENTRAL);
                break;
            case CSS_VAL_AFTER_EDGE:
                svgstyle->setAlignmentBaseline(AB_AFTER_EDGE);
                break;
            case CSS_VAL_TEXT_AFTER_EDGE:
                svgstyle->setAlignmentBaseline(AB_TEXT_AFTER_EDGE);
                break;
            case CSS_VAL_IDEOGRAPHIC:
                svgstyle->setAlignmentBaseline(AB_IDEOGRAPHIC);
                break;
            case CSS_VAL_ALPHABETIC:
                svgstyle->setAlignmentBaseline(AB_ALPHABETIC);
                break;
            case CSS_VAL_HANGING:
                svgstyle->setAlignmentBaseline(AB_HANGING);
                break;
            case CSS_VAL_MATHEMATICAL:
                svgstyle->setAlignmentBaseline(AB_MATHEMATICAL);
                break;
            default:
                break;
            }
            break;
        }
        case CSS_PROP_BASELINE_SHIFT:
        {
            HANDLE_INHERIT_AND_INITIAL(baselineShift, BaselineShift);
            if (!primitiveValue)
                break;

            if (primitiveValue->getIdent()) {
                switch (primitiveValue->getIdent()) {
                case CSS_VAL_BASELINE:
                    svgstyle->setBaselineShift(BS_BASELINE);
                    break;
                case CSS_VAL_SUB:
                    svgstyle->setBaselineShift(BS_SUB);
                    break;
                case CSS_VAL_SUPER:
                    svgstyle->setBaselineShift(BS_SUPER);
                    break;
                default:
                    break;
                }
            } else {
                svgstyle->setBaselineShift(BS_LENGTH);
                svgstyle->setBaselineShiftValue(primitiveValue);
            }

            break;
        }
        case CSS_PROP_KERNING:
        {
            if (isInherit) {
                HANDLE_INHERIT_COND(CSS_PROP_KERNING, kerning, Kerning)
                return;
            }
            else if (isInitial) {
                HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_KERNING, Kerning, Kerning)
                return;
            }
            svgstyle->setKerning(primitiveValue);
            break;
        }
        case CSS_PROP_POINTER_EVENTS:
        {
            HANDLE_INHERIT_AND_INITIAL(pointerEvents, PointerEvents)
            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
            case CSS_VAL_NONE:
                svgstyle->setPointerEvents(PE_NONE);
                break;
            case CSS_VAL_STROKE:
                svgstyle->setPointerEvents(PE_STROKE);
                break;
            case CSS_VAL_FILL:
                svgstyle->setPointerEvents(PE_FILL);
                break;
            case CSS_VAL_PAINTED:
                svgstyle->setPointerEvents(PE_PAINTED);
                break;
            case CSS_VAL_VISIBLE:
                svgstyle->setPointerEvents(PE_VISIBLE);
                break;
            case CSS_VAL_VISIBLESTROKE:
                svgstyle->setPointerEvents(PE_VISIBLE_STROKE);
                break;
            case CSS_VAL_VISIBLEFILL:
                svgstyle->setPointerEvents(PE_VISIBLE_FILL);
                break;
            case CSS_VAL_VISIBLEPAINTED:
                svgstyle->setPointerEvents(PE_VISIBLE_PAINTED);
                break;
            case CSS_VAL_ALL:
                svgstyle->setPointerEvents(PE_ALL);
                break;
            default:
                break;
            }
            break;
        }
        case CSS_PROP_DOMINANT_BASELINE:
        {
            HANDLE_INHERIT_AND_INITIAL(dominantBaseline, DominantBaseline)

            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
            case CSS_VAL_AUTO:
                svgstyle->setDominantBaseline(DB_AUTO);
                break;
            case CSS_VAL_USE_SCRIPT:
                svgstyle->setDominantBaseline(DB_USE_SCRIPT);
                break;
            case CSS_VAL_NO_CHANGE:
                svgstyle->setDominantBaseline(DB_NO_CHANGE);
                break;
            case CSS_VAL_RESET_SIZE:
                svgstyle->setDominantBaseline(DB_RESET_SIZE);
                break;
            case CSS_VAL_IDEOGRAPHIC:
                svgstyle->setDominantBaseline(DB_IDEOGRAPHIC);
                break;
            case CSS_VAL_ALPHABETIC:
                svgstyle->setDominantBaseline(DB_ALPHABETIC);
                break;
            case CSS_VAL_HANGING:
                svgstyle->setDominantBaseline(DB_HANGING);
                break;
            case CSS_VAL_MATHEMATICAL:
                svgstyle->setDominantBaseline(DB_MATHEMATICAL);
                break;
            case CSS_VAL_CENTRAL:
                svgstyle->setDominantBaseline(DB_CENTRAL);
                break;
            case CSS_VAL_MIDDLE:
                svgstyle->setDominantBaseline(DB_MIDDLE);
                break;
            case CSS_VAL_TEXT_AFTER_EDGE:
                svgstyle->setDominantBaseline(DB_TEXT_AFTER_EDGE);
                break;
            case CSS_VAL_TEXT_BEFORE_EDGE:
                svgstyle->setDominantBaseline(DB_TEXT_BEFORE_EDGE);
                break;
            default:
                break;
            }

            break;
        }
        case CSS_PROP_COLOR_INTERPOLATION:
        {
            HANDLE_INHERIT_AND_INITIAL(colorInterpolation, ColorInterpolation);

            svgstyle->setColorInterpolation(colorInterpolationForValue(primitiveValue));
            break;
        }
        case CSS_PROP_COLOR_INTERPOLATION_FILTERS:
        {
            HANDLE_INHERIT_AND_INITIAL(colorInterpolationFilters, ColorInterpolationFilters)

            svgstyle->setColorInterpolationFilters(colorInterpolationForValue(primitiveValue));
            break;
        }
        case CSS_PROP_COLOR_RENDERING:
        {
            HANDLE_INHERIT_AND_INITIAL(colorRendering, ColorRendering)
            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
            case CSS_VAL_AUTO:
                svgstyle->setColorRendering(CR_AUTO);
                break;
            case CSS_VAL_OPTIMIZESPEED:
                svgstyle->setColorRendering(CR_OPTIMIZESPEED);
                break;
            case CSS_VAL_OPTIMIZEQUALITY:
                svgstyle->setColorRendering(CR_OPTIMIZEQUALITY);
                break;
            default:
                break;
            }
            break;
        }
        case CSS_PROP_CLIP_RULE:
        {
            HANDLE_INHERIT_AND_INITIAL(clipRule, ClipRule)
            if (primitiveValue)
                svgstyle->setClipRule((primitiveValue->getIdent() == CSS_VAL_NONZERO) ? RULE_NONZERO : RULE_EVENODD);
            break;
        }
        case CSS_PROP_FILL_RULE:
        {
            HANDLE_INHERIT_AND_INITIAL(fillRule, FillRule)
            if (primitiveValue)
                svgstyle->setFillRule((primitiveValue->getIdent() == CSS_VAL_NONZERO) ? RULE_NONZERO : RULE_EVENODD);
            break;
        }

        case CSS_PROP_STROKE_LINEJOIN:
        {
            HANDLE_INHERIT_AND_INITIAL(joinStyle, JoinStyle)
            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
                case CSS_VAL_MITER:
                    svgstyle->setJoinStyle(khtml::MiterJoin);
                    break;
                case CSS_VAL_ROUND:
                    svgstyle->setJoinStyle(khtml::RoundJoin);
                    break;
                case CSS_VAL_BEVEL:
                    svgstyle->setJoinStyle(khtml::BevelJoin);
                    break;
                default:
                    break;
            }
            break;
        }
        case CSS_PROP_IMAGE_RENDERING:
        {
            HANDLE_INHERIT_AND_INITIAL(imageRendering, ImageRendering)
            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
                case CSS_VAL_AUTO:
                    svgstyle->setImageRendering(IR_AUTO);
                    break;
                case CSS_VAL_OPTIMIZESPEED:
                    svgstyle->setImageRendering(IR_OPTIMIZESPEED);
                    break;
                case CSS_VAL_OPTIMIZEQUALITY:
                    svgstyle->setImageRendering(IR_OPTIMIZEQUALITY);
                    break;
                default:
                    break;
            }
            break;
        }
        case CSS_PROP_SHAPE_RENDERING:
        {
            HANDLE_INHERIT_AND_INITIAL(shapeRendering, ShapeRendering)
            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
                case CSS_VAL_AUTO:
                    svgstyle->setShapeRendering(SR_AUTO);
                    break;
                case CSS_VAL_OPTIMIZESPEED:
                    svgstyle->setShapeRendering(SR_OPTIMIZESPEED);
                    break;
                case CSS_VAL_CRISPEDGES:
                    svgstyle->setShapeRendering(SR_CRISPEDGES);
                    break;
                case CSS_VAL_GEOMETRICPRECISION:
                    svgstyle->setShapeRendering(SR_GEOMETRICPRECISION);
                    break;
                default:
                    break;
            }
            break;
        }
        case CSS_PROP_TEXT_RENDERING:
        {
            HANDLE_INHERIT_AND_INITIAL(textRendering, TextRendering)
            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
                case CSS_VAL_AUTO:
                    svgstyle->setTextRendering(TR_AUTO);
                    break;
                case CSS_VAL_OPTIMIZESPEED:
                    svgstyle->setTextRendering(TR_OPTIMIZESPEED);
                    break;
                case CSS_VAL_OPTIMIZELEGIBILITY:
                    svgstyle->setTextRendering(TR_OPTIMIZELEGIBILITY);
                    break;
                case CSS_VAL_GEOMETRICPRECISION:
                    svgstyle->setTextRendering(TR_GEOMETRICPRECISION);
                    break;
                default:
                    break;
            }

            break;
        }
        // end of ident only properties
        case CSS_PROP_FILL:
        {
            HANDLE_INHERIT_AND_INITIAL(fillPaint, FillPaint)
            if (SVGPaintImpl* paint = toPaint(value))
                svgstyle->setFillPaint(paint);
            break;
        }
        case CSS_PROP_STROKE:
        {
            HANDLE_INHERIT_AND_INITIAL(strokePaint, StrokePaint)
            if (SVGPaintImpl* paint = toPaint(value))
                svgstyle->setStrokePaint(paint);
            break;
        }
        case CSS_PROP_STROKE_WIDTH:
        {
            HANDLE_INHERIT_AND_INITIAL(strokeWidth, StrokeWidth)
            if (!primitiveValue)
                return;

            svgstyle->setStrokeWidth(primitiveValue);
            break;
        }

        case CSS_PROP_STROKE_DASHARRAY:
        {
            HANDLE_INHERIT_AND_INITIAL(strokeDashArray, StrokeDashArray)
            if (!primitiveValue && value && value->isValueList()) {
                CSSValueListImpl* dashes = static_cast<CSSValueListImpl*>(value);
                svgstyle->setStrokeDashArray(dashes);
            }
            break;
        }
        case CSS_PROP_STROKE_DASHOFFSET:
        {
            HANDLE_INHERIT_AND_INITIAL(strokeDashOffset, StrokeDashOffset)
            if (!primitiveValue)
                return;

            svgstyle->setStrokeDashOffset(primitiveValue);
            break;
        }
        case CSS_PROP_FILL_OPACITY:
        {
            HANDLE_INHERIT_AND_INITIAL(fillOpacity, FillOpacity)
            if (!primitiveValue)
                return;

            float f = 0.0f;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_PERCENTAGE)
                f = primitiveValue->floatValue() / 100.0f;
            else if (type == CSSPrimitiveValue::CSS_NUMBER)
                f = primitiveValue->floatValue();
            else
                return;

            svgstyle->setFillOpacity(f);
            break;
        }
        case CSS_PROP_STROKE_OPACITY:
        {
            HANDLE_INHERIT_AND_INITIAL(strokeOpacity, StrokeOpacity)
            if (!primitiveValue)
                return;

            float f = 0.0f;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_PERCENTAGE)
                f = primitiveValue->floatValue() / 100.0f;
            else if (type == CSSPrimitiveValue::CSS_NUMBER)
                f = primitiveValue->floatValue();
            else
                return;

            svgstyle->setStrokeOpacity(f);
            break;
        }

        case CSS_PROP_STOP_OPACITY:
        {
            HANDLE_INHERIT_AND_INITIAL(stopOpacity, StopOpacity)
            if (!primitiveValue)
                return;

            float f = 0.0f;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_PERCENTAGE)
                f = primitiveValue->floatValue() / 100.0f;
            else if (type == CSSPrimitiveValue::CSS_NUMBER)
                f = primitiveValue->floatValue();
            else
                return;

            svgstyle->setStopOpacity(f);
            break;
        }

        case CSS_PROP_MARKER_START:
        {
            HANDLE_INHERIT_AND_INITIAL(startMarker, StartMarker)
            if (!primitiveValue)
                return;

            String s;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_URI)
                s = primitiveValue->getStringValue();
            else
                return;

            svgstyle->setStartMarker(s);
            break;
        }
        case CSS_PROP_MARKER_MID:
        {
            HANDLE_INHERIT_AND_INITIAL(midMarker, MidMarker)
            if (!primitiveValue)
                return;

            String s;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_URI)
                s = primitiveValue->getStringValue();
            else
                return;

            svgstyle->setMidMarker(s);
            break;
        }
        case CSS_PROP_MARKER_END:
        {
            HANDLE_INHERIT_AND_INITIAL(endMarker, EndMarker)
            if (!primitiveValue)
                return;

            String s;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_URI)
                s = primitiveValue->getStringValue();
            else
                return;

            svgstyle->setEndMarker(s);
            break;
        }
        case CSS_PROP_STROKE_LINECAP:
        {
            HANDLE_INHERIT_AND_INITIAL(capStyle, CapStyle)
            if (!primitiveValue)
                break;

            switch (primitiveValue->getIdent()) {
            case CSS_VAL_BUTT:
                svgstyle->setCapStyle(ButtCap);
                break;
            case CSS_VAL_ROUND:
                svgstyle->setCapStyle(RoundCap);
                break;
            case CSS_VAL_SQUARE:
                svgstyle->setCapStyle(SquareCap);
                break;
            default:
                break;
            }
            break;
        }
        case CSS_PROP_STROKE_MITERLIMIT:
        {
            HANDLE_INHERIT_AND_INITIAL(strokeMiterLimit, StrokeMiterLimit)
            if (!primitiveValue)
                return;

            float f = 0.0f;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_NUMBER)
                f = primitiveValue->floatValue();
            else
                return;

            svgstyle->setStrokeMiterLimit(f);
            break;
        }
        case CSS_PROP_FILTER:
        {
            HANDLE_INHERIT_AND_INITIAL(filter, Filter)
            if (!primitiveValue)
                return;

            String s;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_URI)
                s = primitiveValue->getStringValue();
            else
                return;
            svgstyle->setFilter(s);
            break;
        }
        case CSS_PROP_MASK:
        {
            HANDLE_INHERIT_AND_INITIAL(maskElement, MaskElement)
            if (!primitiveValue)
                return;

            String s;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_URI)
                s = primitiveValue->getStringValue();
            else
                return;

            svgstyle->setMaskElement(s);
            break;
        }
        case CSS_PROP_CLIP_PATH:
        {
            HANDLE_INHERIT_AND_INITIAL(clipPath, ClipPath)
            if (!primitiveValue)
                return;

            String s;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_URI)
                s = primitiveValue->getStringValue();
            else
                return;

            svgstyle->setClipPath(s);
            break;
        }

        case CSS_PROP_TEXT_ANCHOR:
        {
            HANDLE_INHERIT_AND_INITIAL(textAnchor, TextAnchor)
            if (primitiveValue) {
                switch (primitiveValue->getIdent()) {
                    case CSS_VAL_START:
                        svgstyle->setTextAnchor(TA_START);
                        break;
                    case CSS_VAL_MIDDLE:
                        svgstyle->setTextAnchor(TA_MIDDLE);
                        break;
                    case CSS_VAL_END:
                        svgstyle->setTextAnchor(TA_END);
                        break;
                }
            }
            break;
        }

        case CSS_PROP_WRITING_MODE:
        {
            HANDLE_INHERIT_AND_INITIAL(writingMode, WritingMode)
            if (!primitiveValue)
                return;

            switch (primitiveValue->getIdent()) {
                case CSS_VAL_LR_TB:
                    svgstyle->setWritingMode(WM_LRTB);
                    break;
                case CSS_VAL_LR:
                    svgstyle->setWritingMode(WM_LR);
                    break;
                case CSS_VAL_RL_TB:
                    svgstyle->setWritingMode(WM_RLTB);
                    break;
                case CSS_VAL_RL:
                    svgstyle->setWritingMode(WM_RL);
                    break;
                case CSS_VAL_TB_RL:
                    svgstyle->setWritingMode(WM_TBRL);
                    break;
                case CSS_VAL_TB:
                    svgstyle->setWritingMode(WM_TB);
                    break;
                default:
                    break;
            }
            break;
        }

        case CSS_PROP_STOP_COLOR:
        {
            HANDLE_INHERIT_AND_INITIAL(stopColor, StopColor);

            SVGColorImpl* c = toColor(value);
            if (!c)
                break;

            QColor col;
            if (c->colorType() == SVGColorImpl::SVG_COLORTYPE_CURRENTCOLOR)
                col = style->color();
            else
                col = c->color();

            svgstyle->setStopColor(col);
            break;
        }

        case CSS_PROP_LIGHTING_COLOR:
        {
            HANDLE_INHERIT_AND_INITIAL(lightingColor, LightingColor);

            SVGColorImpl* c = toColor(value);
            if (!c)
                break;

            QColor col;
            if (c->colorType() == SVGColorImpl::SVG_COLORTYPE_CURRENTCOLOR)
                col = style->color();
            else
                col = c->color();

            svgstyle->setLightingColor(col);
            break;
        }
        case CSS_PROP_FLOOD_OPACITY:
        {
            HANDLE_INHERIT_AND_INITIAL(floodOpacity, FloodOpacity)
            if (!primitiveValue)
                return;

            float f = 0.0f;
            int type = primitiveValue->primitiveType();
            if (type == CSSPrimitiveValue::CSS_PERCENTAGE)
                f = primitiveValue->floatValue() / 100.0f;
            else if (type == CSSPrimitiveValue::CSS_NUMBER)
                f = primitiveValue->floatValue();
            else
                return;

            svgstyle->setFloodOpacity(f);
            break;
        }
        case CSS_PROP_FLOOD_COLOR:
        {
            QColor col;
            if (isInitial)
                col = SVGRenderStyle::initialFloodColor();
            else {
                SVGColorImpl* c = toColor(value);
                if (!c)
                    break;

                if (c->colorType() == SVGColorImpl::SVG_COLORTYPE_CURRENTCOLOR)
                    col = style->color();
                else
                    col = c->color();
            }

            svgstyle->setFloodColor(col);
            break;
        }
        case CSS_PROP_GLYPH_ORIENTATION_HORIZONTAL:
        {
            HANDLE_INHERIT_AND_INITIAL(glyphOrientationHorizontal, GlyphOrientationHorizontal)
            if (!primitiveValue)
                return;

            if (primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_DEG) {
                int orientation = angleToGlyphOrientation(primitiveValue->floatValue());
                ASSERT(orientation != -1);

                svgstyle->setGlyphOrientationHorizontal((EGlyphOrientation) orientation);
            }

            break;
        }
        case CSS_PROP_GLYPH_ORIENTATION_VERTICAL:
        {
            HANDLE_INHERIT_AND_INITIAL(glyphOrientationVertical, GlyphOrientationVertical)
            if (!primitiveValue)
                return;

            if (primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_DEG) {
                int orientation = angleToGlyphOrientation(primitiveValue->floatValue());
                ASSERT(orientation != -1);

                svgstyle->setGlyphOrientationVertical((EGlyphOrientation) orientation);
            } else if (primitiveValue->getIdent() == CSS_VAL_AUTO)
                svgstyle->setGlyphOrientationVertical(GO_AUTO);

            break;
        }
        case CSS_PROP_ENABLE_BACKGROUND:
            // Silently ignoring this property for now
            // http://bugs.webkit.org/show_bug.cgi?id=6022
            break;
        default:
            // If you crash here, it's because you added a css property and are not handling it
            // in either this switch statement or the one in CSSStyleSelector::applyProperty
            qWarning() << "unimplemented property" << id << getPropertyName(id).string();
            return;
    }
}

}

