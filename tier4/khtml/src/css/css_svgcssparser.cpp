/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2007 Rob Buis <buis@kde.org>
    Copyright (C) 2005, 2006 Apple Computer, Inc.

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

#include "cssparser.h"
#include "cssproperties.h"
#include "cssvalues.h"

#include "css/css_valueimpl.h"
#include "css/css_svgvalueimpl.h"

using namespace std;

namespace DOM
{

bool CSSParser::parseSVGValue(int propId, bool important)
{
    Value* value = valueList->current();
    if (!value)
        return false;

    int id = value->id;

    bool valid_primitive = false;
    CSSValueImpl *parsedValue = 0;

    switch (propId) {
    /* The comment to the right defines all valid value of these
     * properties as defined in SVG 1.1, Appendix N. Property index */
    case CSS_PROP_ALIGNMENT_BASELINE:
    // auto | baseline | before-edge | text-before-edge | middle |
    // central | after-edge | text-after-edge | ideographic | alphabetic |
    // hanging | mathematical | inherit
        if (id == CSS_VAL_AUTO || id == CSS_VAL_BASELINE || id == CSS_VAL_MIDDLE ||
          (id >= CSS_VAL_BEFORE_EDGE && id <= CSS_VAL_MATHEMATICAL))
            valid_primitive = true;
        break;

    case CSS_PROP_BASELINE_SHIFT:
    // baseline | super | sub | <percentage> | <length> | inherit
        if (id == CSS_VAL_BASELINE || id == CSS_VAL_SUB || id >= CSS_VAL_SUPER)
            valid_primitive = true;
        else
            valid_primitive = validUnit(value, FLength|FPercent, false);
        break;

    case CSS_PROP_DOMINANT_BASELINE:
    // auto | use-script | no-change | reset-size | ideographic |
    // alphabetic | hanging | mathematical | central | middle |
    // text-after-edge | text-before-edge | inherit
        if (id == CSS_VAL_AUTO || id == CSS_VAL_MIDDLE ||
          (id >= CSS_VAL_USE_SCRIPT && id <= CSS_VAL_RESET_SIZE) ||
          (id >= CSS_VAL_CENTRAL && id <= CSS_VAL_MATHEMATICAL))
            valid_primitive = true;
        break;

    case CSS_PROP_ENABLE_BACKGROUND:
    // accumulate | new [x] [y] [width] [height] | inherit
        if (id == CSS_VAL_ACCUMULATE) // ### TODO: new
            valid_primitive = true;
        break;

    case CSS_PROP_MARKER_START:
    case CSS_PROP_MARKER_MID:
    case CSS_PROP_MARKER_END:
    case CSS_PROP_MASK:
        if (id == CSS_VAL_NONE)
            valid_primitive = true;
        else if (value->unit == CSSPrimitiveValue::CSS_URI) {
            parsedValue = new CSSPrimitiveValueImpl(domString(value->string), CSSPrimitiveValue::CSS_URI);
            if (parsedValue)
                valueList->next();
        }
        break;

    case CSS_PROP_CLIP_RULE:            // nonzero | evenodd | inherit
    case CSS_PROP_FILL_RULE:
        if (id == CSS_VAL_NONZERO || id == CSS_VAL_EVENODD)
            valid_primitive = true;
        break;

    case CSS_PROP_STROKE_MITERLIMIT:   // <miterlimit> | inherit
        valid_primitive = validUnit(value, FNumber|FNonNeg, false);
        break;

    case CSS_PROP_STROKE_LINEJOIN:   // miter | round | bevel | inherit
        if (id == CSS_VAL_MITER || id == CSS_VAL_ROUND || id == CSS_VAL_BEVEL)
            valid_primitive = true;
        break;

    case CSS_PROP_STROKE_LINECAP:    // butt | round | square | inherit
        if (id == CSS_VAL_BUTT || id == CSS_VAL_ROUND || id == CSS_VAL_SQUARE)
            valid_primitive = true;
        break;

    case CSS_PROP_STROKE_OPACITY:   // <opacity-value> | inherit
    case CSS_PROP_FILL_OPACITY:
    case CSS_PROP_STOP_OPACITY:
    case CSS_PROP_FLOOD_OPACITY:
        valid_primitive = (!id && validUnit(value, FNumber|FPercent, false));
        break;

    case CSS_PROP_SHAPE_RENDERING:
    // auto | optimizeSpeed | crispEdges | geometricPrecision | inherit
        if (id == CSS_VAL_AUTO || id == CSS_VAL_OPTIMIZESPEED ||
            id == CSS_VAL_CRISPEDGES || id == CSS_VAL_GEOMETRICPRECISION)
            valid_primitive = true;
        break;

    case CSS_PROP_TEXT_RENDERING:   // auto | optimizeSpeed | optimizeLegibility | geometricPrecision | inherit
        if (id == CSS_VAL_AUTO || id == CSS_VAL_OPTIMIZESPEED || id == CSS_VAL_OPTIMIZELEGIBILITY ||
       id == CSS_VAL_GEOMETRICPRECISION)
            valid_primitive = true;
        break;

    case CSS_PROP_IMAGE_RENDERING:  // auto | optimizeSpeed |
    case CSS_PROP_COLOR_RENDERING:  // optimizeQuality | inherit
        if (id == CSS_VAL_AUTO || id == CSS_VAL_OPTIMIZESPEED ||
                id == CSS_VAL_OPTIMIZEQUALITY)
            valid_primitive = true;
        break;

    case CSS_PROP_COLOR_PROFILE: // auto | sRGB | <name> | <uri> inherit
        if (id == CSS_VAL_AUTO || id == CSS_VAL_SRGB)
            valid_primitive = true;
        break;

    case CSS_PROP_COLOR_INTERPOLATION:   // auto | sRGB | linearRGB | inherit
    case CSS_PROP_COLOR_INTERPOLATION_FILTERS:  
        if (id == CSS_VAL_AUTO || id == CSS_VAL_SRGB || id == CSS_VAL_LINEARRGB)
            valid_primitive = true;
        break;

    case CSS_PROP_POINTER_EVENTS:
    // visiblePainted | visibleFill | visibleStroke | visible |
    // painted | fill | stroke | all | none |  inherit
        if (id == CSS_VAL_VISIBLE || id == CSS_VAL_NONE ||
          (id >= CSS_VAL_VISIBLEPAINTED && id <= CSS_VAL_ALL))
            valid_primitive = true;
        break;

    case CSS_PROP_TEXT_ANCHOR:    // start | middle | end | inherit
        if (id == CSS_VAL_START || id == CSS_VAL_MIDDLE || id == CSS_VAL_END)
            valid_primitive = true;
        break;

    case CSS_PROP_GLYPH_ORIENTATION_VERTICAL: // auto | <angle> | inherit
        if (id == CSS_VAL_AUTO) {
            valid_primitive = true;
            break;
        }
        /* fallthrough intentional */
    case CSS_PROP_GLYPH_ORIENTATION_HORIZONTAL: // <angle> (restricted to _deg_ per SVG 1.1 spec) | inherit
        if (value->unit == CSSPrimitiveValue::CSS_DEG || value->unit == CSSPrimitiveValue::CSS_NUMBER) {
            parsedValue = new CSSPrimitiveValueImpl(value->fValue, CSSPrimitiveValue::CSS_DEG);

            if (parsedValue)
                valueList->next();
        }
        break;

    case CSS_PROP_FILL:                 // <paint> | inherit
    case CSS_PROP_STROKE:               // <paint> | inherit
        {
            if (id == CSS_VAL_NONE)
                parsedValue = new SVGPaintImpl(SVGPaintImpl::SVG_PAINTTYPE_NONE);
            else if (id == CSS_VAL_CURRENTCOLOR)
                parsedValue = new SVGPaintImpl(SVGPaintImpl::SVG_PAINTTYPE_CURRENTCOLOR);
            else if (value->unit == CSSPrimitiveValue::CSS_URI) {
                CSSPrimitiveValueImpl* val;
                if (valueList->next() && (val = parseColorFromValue(valueList->current()/*, c, true*/))) {
                    parsedValue = new SVGPaintImpl(domString(value->string), val->getRGBColorValue());
                    delete val;
                } else
                    parsedValue = new SVGPaintImpl(SVGPaintImpl::SVG_PAINTTYPE_URI, domString(value->string));
            } else
                parsedValue = parseSVGPaint();

            if (parsedValue)
                valueList->next();
        }
        break;

    /*case CSS_PROP_Color:                // <color> | inherit
        if ((id >= CSS_VAL_Aqua && id <= CSS_VAL_Windowtext) ||
           (id >= CSS_VAL_Aliceblue && id <= CSS_VAL_Yellowgreen))
            parsedValue = new SVGColor(value->string);
        else
            parsedValue = parseSVGColor();

        if (parsedValue)
            valueList->next();
        break;*/

    case CSS_PROP_STOP_COLOR: // TODO : icccolor
    case CSS_PROP_FLOOD_COLOR:
    case CSS_PROP_LIGHTING_COLOR:
        if ((id >= CSS_VAL_AQUA && id <= CSS_VAL_WINDOWTEXT)/* ||
           (id >= CSS_VAL_Aliceblue && id <= CSS_VAL_Yellowgreen)*/)
            parsedValue = new SVGColorImpl(domString(value->string));
        else if (id == CSS_VAL_CURRENTCOLOR)
            parsedValue = new SVGColorImpl(SVGColorImpl::SVG_COLORTYPE_CURRENTCOLOR);
        else // TODO : svgcolor (iccColor)
            parsedValue = parseSVGColor();

        if (parsedValue)
            valueList->next();

        break;

    case CSS_PROP_WRITING_MODE:
    // lr-tb | rl_tb | tb-rl | lr | rl | tb | inherit
        if (id >= CSS_VAL_LR_TB && id <= CSS_VAL_TB)
            valid_primitive = true;
        break;

    case CSS_PROP_STROKE_WIDTH:         // <length> | inherit
    case CSS_PROP_STROKE_DASHOFFSET:
        valid_primitive = validUnit(value, FLength | FPercent, false);
        break;

    case CSS_PROP_STROKE_DASHARRAY:     // none | <dasharray> | inherit
        if (id == CSS_VAL_NONE)
            valid_primitive = true;
        else
            parsedValue = parseSVGStrokeDasharray();

        break;

    case CSS_PROP_KERNING:              // auto | normal | <length> | inherit
        if (id == CSS_VAL_AUTO || id == CSS_VAL_NORMAL)
            valid_primitive = true;
        else
            valid_primitive = validUnit(value, FLength, false);
        break;

    case CSS_PROP_CLIP_PATH:    // <uri> | none | inherit
    case CSS_PROP_FILTER:
        if (id == CSS_VAL_NONE)
            valid_primitive = true;
        else if (value->unit == CSSPrimitiveValue::CSS_URI) {
            parsedValue = new CSSPrimitiveValueImpl(domString(value->string), (CSSPrimitiveValue::UnitTypes) value->unit);
            if (parsedValue)
                valueList->next();
        }
        break;

    /* shorthand properties */
    case CSS_PROP_MARKER:
    {
        const int properties[3] = { CSS_PROP_MARKER_START, CSS_PROP_MARKER_MID,
                                    CSS_PROP_MARKER_END };
        return parseShortHand(propId, properties, 3, important);

    }
    default:
        // If you crash here, it's because you added a css property and are not handling it
        // in either this switch statement or the one in CSSParser::parseValue
        //ASSERT_WITH_MESSAGE(0, "unimplemented propertyID: %d", propId);
        //return false;
        break;
    }

    if (valid_primitive) {
        if (id != 0)
            parsedValue = new CSSPrimitiveValueImpl(id);
        else if (value->unit == CSSPrimitiveValue::CSS_STRING)
            parsedValue = new CSSPrimitiveValueImpl(domString(value->string), (CSSPrimitiveValue::UnitTypes)value->unit);
        else if (value->unit >= CSSPrimitiveValue::CSS_NUMBER && value->unit <= CSSPrimitiveValue::CSS_KHZ)
            parsedValue = new CSSPrimitiveValueImpl(value->fValue, (CSSPrimitiveValue::UnitTypes)value->unit);
        else if (value->unit >= Value::Q_EMS)
            parsedValue = new CSSQuirkPrimitiveValueImpl(value->fValue, CSSPrimitiveValue::CSS_EMS);
        valueList->next();
    }
    if (!parsedValue || (valueList->current() && !inShorthand())) {
        delete parsedValue;
        return false;
    }

    addProperty(propId, parsedValue, important);
    return true;
}

CSSValueImpl* CSSParser::parseSVGStrokeDasharray()
{
    CSSValueListImpl* ret = new CSSValueListImpl(CSSValueListImpl::Comma);
    Value* value = valueList->current();
    bool valid_primitive = true;
    while (value) {
        valid_primitive = validUnit(value, FLength | FPercent |FNonNeg, false);
        if (!valid_primitive)
            break;
        if (value->id != 0)
            ret->append(new CSSPrimitiveValueImpl(value->id));
        else if (value->unit >= CSSPrimitiveValue::CSS_NUMBER && value->unit <= CSSPrimitiveValue::CSS_KHZ)
            ret->append(new CSSPrimitiveValueImpl(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit));
        value = valueList->next();
        if (value && value->unit == Value::Operator && value->iValue == ',')
            value = valueList->next();
    }
    if (!valid_primitive) {
        delete ret;
        ret = 0;
    }

    return ret;
}

CSSValueImpl* CSSParser::parseSVGPaint()
{
    CSSPrimitiveValueImpl* val;

    if (!(val = parseColorFromValue(valueList->current()/*, c, true*/)))
        return new SVGPaintImpl();

    SVGPaintImpl* paint = new SVGPaintImpl(QColor(val->getRGBColorValue()));
    delete val;
    return paint;
}

CSSValueImpl* CSSParser::parseSVGColor()
{
    CSSPrimitiveValueImpl* val;
    if (!(val = parseColorFromValue(valueList->current()/*, c, true*/)))
        return 0;
    SVGColorImpl* color = new SVGColorImpl(QColor(val->getRGBColorValue()));
    delete val;
    return color;
}

}

