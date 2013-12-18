/*
 * CSS Media Query
 *
 * Copyright (C) 2005, 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
 *           (C) 2008 Germain Garand <germain@ebooksfrance.org>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#include "css_mediaquery.h"
#include "css_valueimpl.h"
#include "css/css_stylesheetimpl.h"
#include "css/cssparser.h"
#include "cssstyleselector.h"
#include "css/cssvalues.h"
#include "khtml_part.h"
#include "khtmlview.h"
#include "rendering/render_style.h"
#include "dom/dom_string.h"
#include "xml/dom_stringimpl.h"
#include <QHash>
#include <limits.h>
#include <QDesktopWidget>
#include <QColormap>

using namespace DOM;
using namespace khtml;

MediaQuery::MediaQuery(Restrictor r, const DOMString& mediaType, QList<MediaQueryExp*>* exprs)
    : m_restrictor(r)
    , m_mediaType(mediaType)
    , m_expressions(exprs)
{
    if (!m_expressions)
        m_expressions = new QList<MediaQueryExp*>;
}

MediaQuery::~MediaQuery()
{
    if (m_expressions) {
        qDeleteAll(*m_expressions);
        delete m_expressions;
    }
}

bool MediaQuery::operator==(const MediaQuery& other) const
{
    if (m_restrictor != other.m_restrictor
        || m_mediaType != other.m_mediaType
        || m_expressions->size() != other.m_expressions->size())
        return false;

    for (int i = 0; i < m_expressions->size(); ++i) {
        MediaQueryExp* exp = m_expressions->at(i);
        MediaQueryExp* oexp = other.m_expressions->at(i);
        if (!(*exp == *oexp))
            return false;
    }

    return true;
}

DOMString MediaQuery::cssText() const
{
    DOMString text;
    switch (m_restrictor) {
        case MediaQuery::Only:
            text += "only ";
            break;
        case MediaQuery::Not:
            text += "not ";
            break;
        case MediaQuery::None:
        default:
            break;
    }
    text += m_mediaType;
    for (int i = 0; i < m_expressions->size(); ++i) {
        MediaQueryExp* exp = m_expressions->at(i);
        text += " and (";
        text += exp->mediaFeature();
        if (exp->value()) {
            text += ": ";
            text += exp->value()->cssText();
        }
        text += ")";
    }
    return text;
}

//---------------------------------------------------------------------------

MediaQueryExp::MediaQueryExp(const DOMString& mediaFeature, ValueList* valueList)
    : m_mediaFeature(mediaFeature)
    , m_value(0)
{
    m_viewportDependent = ( m_mediaFeature == "width" || 
                            m_mediaFeature == "height" ||
                            m_mediaFeature == "min-width" ||
                            m_mediaFeature == "min-height" ||
                            m_mediaFeature == "max-width" ||
                            m_mediaFeature == "max-height" ||
                            m_mediaFeature == "orientation" ||
                            m_mediaFeature == "aspect-ratio" ||
                            m_mediaFeature == "min-aspect-ratio" ||
                            m_mediaFeature == "max-aspect-ratio" );
    if (valueList) {
        if (valueList->size() == 1) {
            Value* value = valueList->current();

            if (value->id != 0)
                m_value = new CSSPrimitiveValueImpl(value->id);
            else if (value->unit == CSSPrimitiveValue::CSS_STRING)
                m_value = new CSSPrimitiveValueImpl(domString(value->string), (CSSPrimitiveValue::UnitTypes) value->unit);
            else if ((value->unit >= CSSPrimitiveValue::CSS_NUMBER &&
                      value->unit <= CSSPrimitiveValue::CSS_KHZ) ||
                      value->unit == CSSPrimitiveValue::CSS_DPI  || value->unit == CSSPrimitiveValue::CSS_DPCM)
                m_value = new CSSPrimitiveValueImpl(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit);
            if (m_value)
                m_value->ref();
            valueList->next();
        } else if (valueList->size() > 1) {
            // create list of values
            // currently accepts only <integer>/<integer>

            CSSValueListImpl* list = new CSSValueListImpl();
            Value* value = 0;
            bool isValid = true;

            while ((value = valueList->current()) && isValid) {
                if (value->unit == Value::Operator && value->iValue == '/')
                    list->append(new CSSPrimitiveValueImpl(DOMString("/"), CSSPrimitiveValue::CSS_STRING));
                else if (value->unit == CSSPrimitiveValue::CSS_NUMBER)
                    list->append(new CSSPrimitiveValueImpl(value->fValue, CSSPrimitiveValue::CSS_NUMBER));
                else
                    isValid = false;

                value = valueList->next();
            }

            if (isValid) {
                m_value = list;
                m_value->ref();
            } else
                delete list;
        }
    }
}

MediaQueryExp::~MediaQueryExp()
{
    if (m_value)
        m_value->deref();
}

//---------------------------------------------------------------------------

// when adding features, update also m_viewportDependent test if applicable
#define CSS_MEDIAQUERY_NAMES_FOR_EACH_MEDIAFEATURE(macro) \
    macro(color, "color") \
    macro(color_index, "color-index") \
    macro(grid, "grid") \
    macro(monochrome, "monochrome") \
    macro(height, "height") \
    macro(width, "width") \
    macro(device_aspect_ratio, "device-aspect-ratio") \
    macro(device_pixel_ratio, "-khtml-device-pixel-ratio") \
    macro(device_height, "device-height") \
    macro(device_width, "device-width") \
    macro(orientation, "orientation") \
    macro(aspect_ratio, "aspect-ratio") \
    macro(resolution, "resolution") \
    macro(scan, "scan") \
    macro(max_color, "max-color") \
    macro(max_color_index, "max-color-index") \
    macro(max_device_aspect_ratio, "max-device-aspect-ratio") \
    macro(max_device_pixel_ratio, "-khtml-max-device-pixel-ratio") \
    macro(max_device_height, "max-device-height") \
    macro(max_device_width, "max-device-width") \
    macro(max_aspect_ratio, "max-aspect-ratio") \
    macro(max_resolution, "max-resolution") \
    macro(max_height, "max-height") \
    macro(max_monochrome, "max-monochrome") \
    macro(max_width, "max-width") \
    macro(min_color, "min-color") \
    macro(min_color_index, "min-color-index") \
    macro(min_device_aspect_ratio, "min-device-aspect-ratio") \
    macro(min_device_pixel_ratio, "-khtml-min-device-pixel-ratio") \
    macro(min_device_height, "min-device-height") \
    macro(min_device_width, "min-device-width") \
    macro(min_resolution, "min-resolution") \
    macro(min_aspect_ratio, "min-aspect-ratio") \
    macro(min_height, "min-height") \
    macro(min_monochrome, "min-monochrome") \
    macro(min_width, "min-width") \
// end of macro

enum MediaFeaturePrefix { MinPrefix, MaxPrefix, NoPrefix };

typedef bool (*EvalFunc)(CSSValueImpl*, RenderStyle*, KHTMLPart*,  MediaFeaturePrefix);
typedef QHash<DOMString, EvalFunc> FunctionMap;
static FunctionMap* gFunctionMap = 0;

MediaQueryEvaluator::MediaQueryEvaluator(bool mediaFeatureResult)
    : m_part(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
{
}

MediaQueryEvaluator:: MediaQueryEvaluator(const DOMString& acceptedMediaType, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_part(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
{
}

MediaQueryEvaluator:: MediaQueryEvaluator(const char* acceptedMediaType, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_part(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
{
}

MediaQueryEvaluator:: MediaQueryEvaluator(const DOMString& acceptedMediaType, KHTMLPart* part, RenderStyle* style)
    : m_mediaType(acceptedMediaType.lower())
    , m_part(part)
    , m_style(style)
    , m_expResult(false) // doesn't matter when we have m_part and m_style
{
}

MediaQueryEvaluator::~MediaQueryEvaluator()
{
}

bool MediaQueryEvaluator::mediaTypeMatch(const DOMString& mediaTypeToMatch) const
{
    return mediaTypeToMatch.isEmpty()
        || !strcasecmp("all", mediaTypeToMatch)
        || !strcasecmp(m_mediaType, mediaTypeToMatch);
}

bool MediaQueryEvaluator::mediaTypeMatchSpecific(const char* mediaTypeToMatch) const
{
    // Like mediaTypeMatch, but without the special cases for "" and "all".
    assert(mediaTypeToMatch);
    assert(mediaTypeToMatch[0] != '\0');
    assert(strcasecmp(DOMString("all"), mediaTypeToMatch));
    return !strcasecmp(m_mediaType, mediaTypeToMatch);
}

static bool applyRestrictor(MediaQuery::Restrictor r, bool value)
{
    return r == MediaQuery::Not ? !value : value;
}

bool MediaQueryEvaluator::eval(const MediaListImpl* mediaList, CSSStyleSelector* styleSelector) const
{
    if (!mediaList)
        return true;

    const QList<MediaQuery*>* queries = mediaList->mediaQueries();
    if (!queries->size())
        return true; // empty query list evaluates to true

    // iterate over queries, stop if any of them eval to true (OR semantics)
    bool result = false;
    for (int i = 0; i < queries->size() && !result; ++i) {
        MediaQuery* query = queries->at(i);
        if (mediaTypeMatch(query->mediaType())) {
            const QList<MediaQueryExp*>* exps = query->expressions();
            // iterate through expressions, stop if any of them eval to false
            // (AND semantics)
            int j = 0;
            for (; j < exps->size(); ++j) {
                bool exprResult = eval(exps->at(j));
                if (styleSelector && exps->at(j)->isViewportDependent())
                    styleSelector->addViewportDependentMediaQueryResult(exps->at(j), exprResult);
                if (!exprResult)
                    break;
            }

            // assume true if we are at the end of the list,
            // otherwise assume false
            result = applyRestrictor(query->restrictor(), exps->size() == j);
        } else
            result = applyRestrictor(query->restrictor(), false);
    }

    return result;
}

static bool parseAspectRatio(CSSValueImpl* value, int& h, int& v)
{
    if (value->isValueList()){
        CSSValueListImpl* valueList = static_cast<CSSValueListImpl*>(value);
        if (valueList->length() == 3) {
            CSSValueImpl* i0 = valueList->item(0);
            CSSValueImpl* i1 = valueList->item(1);
            CSSValueImpl* i2 = valueList->item(2);
            if (i0->isPrimitiveValue() && static_cast<CSSPrimitiveValueImpl*>(i0)->primitiveType() == CSSPrimitiveValue::CSS_NUMBER
                && i1->isPrimitiveValue() && static_cast<CSSPrimitiveValueImpl*>(i1)->primitiveType() == CSSPrimitiveValue::CSS_STRING
                && i2->isPrimitiveValue() && static_cast<CSSPrimitiveValueImpl*>(i2)->primitiveType() == CSSPrimitiveValue::CSS_NUMBER) {
                DOMString str = static_cast<CSSPrimitiveValueImpl*>(i1)->getStringValue();
                if (!str.isNull() && str.length() == 1 && str[0] == '/') {
                    h = (int)static_cast<CSSPrimitiveValueImpl*>(i0)->floatValue(CSSPrimitiveValue::CSS_NUMBER);
                    v = (int)static_cast<CSSPrimitiveValueImpl*>(i2)->floatValue(CSSPrimitiveValue::CSS_NUMBER);
                    return true;
                }
            }
        }
    }
    return false;
}

template<typename T>
bool compareValue(T a, T b, MediaFeaturePrefix op)
{
    switch (op) {
    case MinPrefix:
        return a >= b;
    case MaxPrefix:
        return a <= b;
    case NoPrefix:
        return a == b;
    }
    return false;
}

static bool numberValue(CSSValueImpl* value, float& result)
{
    if (value->isPrimitiveValue()
        && static_cast<CSSPrimitiveValueImpl*>(value)->primitiveType() == CSSPrimitiveValue::CSS_NUMBER) {
        result = static_cast<CSSPrimitiveValueImpl*>(value)->floatValue(CSSPrimitiveValue::CSS_NUMBER);
        return true;
    }
    return false;
}

static bool color_indexMediaFeatureEval(CSSValueImpl* value, RenderStyle*, KHTMLPart* part,  MediaFeaturePrefix op)
{
    KHTMLPart* rootPart = part;
    while (rootPart->parentPart()) rootPart = rootPart->parentPart();
    DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
    QPaintDevice *pd = doc->paintDevice(); 
    bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
    unsigned int numColors = 0;
    if (printing) {
        numColors = pd->colorCount();
    } else {
        int sn = QApplication::desktop()->screenNumber( rootPart->view() );
        numColors = QApplication::desktop()->screen(sn)->colorCount();
    }
    if (numColors == INT_MAX)
        numColors = UINT_MAX;
    if (value)
    {
        float number = 0;
        return numberValue(value, number) && compareValue(numColors, static_cast<unsigned int>(number), op);
    }

    return numColors;
}

static bool colorMediaFeatureEval(CSSValueImpl* value, RenderStyle*, KHTMLPart* part,  MediaFeaturePrefix op)
{
    KHTMLPart* rootPart = part;
    while (rootPart->parentPart()) rootPart = rootPart->parentPart();
    DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
    QPaintDevice *pd = doc->paintDevice(); 
    bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
    int bitsPerComponent = 0;
    if (printing) {
        if (pd->colorCount() > 2)
            bitsPerComponent = pd->depth()/3;
        // assume printer is either b&w or color.
    } else {
        int sn = QApplication::desktop()->screenNumber( rootPart->view() );
        if (QColormap::instance(sn).mode() != QColormap::Gray )
            bitsPerComponent = QApplication::desktop()->screen(sn)->depth()/3;
    }
    if (value && bitsPerComponent)
    {
        float number = 0;
        return numberValue(value, number) && compareValue(bitsPerComponent, static_cast<int>(number), op);
    }
    return bitsPerComponent;
}

static bool monochromeMediaFeatureEval(CSSValueImpl* value, RenderStyle*, KHTMLPart* part,  MediaFeaturePrefix op)
{
    KHTMLPart* rootPart = part;
    while (rootPart->parentPart()) rootPart = rootPart->parentPart();
    DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
    QPaintDevice *pd = doc->paintDevice(); 
    bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
    int depth = 0;
    if (printing) {
        if (pd->colorCount() < 2)
            depth = 1;
        // assume printer is either b&w or color.
    } else {
        int sn = QApplication::desktop()->screenNumber( rootPart->view() );
        if (QApplication::desktop()->screen(sn)->depth() == 1)
            depth = 1;
        else if (QColormap::instance(sn).mode() == QColormap::Gray)
            depth = QApplication::desktop()->screen(sn)->depth();
    }
    if (value)
    {
        float number = 0;
        return numberValue(value, number) && compareValue(depth, static_cast<int>(number), op);
    }
    return depth;
}

static bool device_aspect_ratioMediaFeatureEval(CSSValueImpl* value, RenderStyle*, KHTMLPart* part,  MediaFeaturePrefix op)
{
    if (value) {
        KHTMLPart* rootPart = part;
        while (rootPart->parentPart()) rootPart = rootPart->parentPart();
        DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
        QPaintDevice *pd = doc->paintDevice(); 
        bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
        QRect sg;
        int h = 0, v = 0;
        if (printing) {
            sg = QRect(0, 0, pd->width(), pd->height());
        } else {
            sg = QApplication::desktop()->screen(QApplication::desktop()->screenNumber( rootPart->view() ))->rect();
        }
        if (parseAspectRatio(value, h, v))
            return v != 0  && compareValue(sg.width()*v, sg.height()*h, op);
        return false;
    }

    // ({,min-,max-}device-aspect-ratio)
    // assume if we have a device, its aspect ratio is non-zero
    return true;
}

static bool aspect_ratioMediaFeatureEval(CSSValueImpl* value, RenderStyle*, KHTMLPart* part,  MediaFeaturePrefix op)
{
    if (value) {
        KHTMLPart* rootPart = part;
        while (rootPart->parentPart()) rootPart = rootPart->parentPart();
        DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
        QPaintDevice *pd = doc->paintDevice(); 
        bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
        QSize vs;
        int h = 0, v = 0;
        if (printing) {
            vs= QSize(pd->width(), pd->height());
        } else {
            vs= QSize(part->view()->visibleWidth(), part->view()->visibleHeight());
        }
        if (parseAspectRatio(value, h, v))
            return v != 0  && compareValue(vs.width()*v, vs.height()*h, op);
        return false;
    }
    // ({,min-,max-}aspect-ratio)
    // assume if we have a viewport, its aspect ratio is non-zero
    return true;
}

static bool orientationMediaFeatureEval(CSSValueImpl* value, RenderStyle*, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    if (value) {
       CSSPrimitiveValueImpl* pv = static_cast<CSSPrimitiveValueImpl*>(value);
       if (!value->isPrimitiveValue() || pv->primitiveType() != CSSPrimitiveValue::CSS_IDENT ||
           (pv->getIdent() != CSS_VAL_PORTRAIT && pv->getIdent() != CSS_VAL_LANDSCAPE))
          return false;
                                            
        KHTMLPart* rootPart = part;
        while (rootPart->parentPart()) rootPart = rootPart->parentPart();
        DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
        QPaintDevice *pd = doc->paintDevice(); 
        bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
        if (printing) {
            if (pd->width() > pd->height())
                return (pv->getIdent() == CSS_VAL_LANDSCAPE);
        } else {
            if (part->view()->visibleWidth() > part->view()->visibleHeight())
               return (pv->getIdent() == CSS_VAL_LANDSCAPE);
        }
        return (pv->getIdent() == CSS_VAL_PORTRAIT);
    }
    return false;
}

static bool device_pixel_ratioMediaFeatureEval(CSSValueImpl *value, RenderStyle*, KHTMLPart* part, MediaFeaturePrefix op)
{
    if (value)
        return value->isPrimitiveValue() && compareValue(part->zoomFactor()/100.0, static_cast<CSSPrimitiveValueImpl*>(value)->floatValue(), op);

    return part->zoomFactor() != 0;
}

static bool gridMediaFeatureEval(CSSValueImpl* value, RenderStyle*, KHTMLPart* /*part*/,  MediaFeaturePrefix op)
{
    // if output device is bitmap, grid: 0 == true
    // assume we have bitmap device
    float number;
    if (value && numberValue(value, number))
        return compareValue(static_cast<int>(number), 0, op);
    return false;
}

// for printing media, we'll make the approximation that the device height == the paged box's height
static bool device_heightMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix op)
{
    if (value) {
        KHTMLPart* rootPart = part;
        while (rootPart->parentPart()) rootPart = rootPart->parentPart();
        DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
        QPaintDevice *pd = doc->paintDevice(); 
        bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
        int height;
        if (printing)
            height = pd->height();
        else {
            height = QApplication::desktop()->screen(QApplication::desktop()->screenNumber( rootPart->view() ))->rect().height();
            doc = static_cast<DOM::DocumentImpl*>(part->document().handle());
        }
        int logicalDpiY = doc->logicalDpiY();
        return value->isPrimitiveValue() && compareValue(height, static_cast<CSSPrimitiveValueImpl*>(value)->computeLength(style,logicalDpiY), op);
    }
    // ({,min-,max-}device-height)
    // assume if we have a device, assume non-zero
    return true;
}

static bool device_widthMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix op)
{
    if (value) {
        KHTMLPart* rootPart = part;
        while (rootPart->parentPart()) rootPart = rootPart->parentPart();
        DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
        QPaintDevice *pd = doc->paintDevice(); 
        bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
        int width;
        if (printing)
            width = pd->width();
        else {
            width = QApplication::desktop()->screen(QApplication::desktop()->screenNumber( rootPart->view() ))->rect().width();
            doc = static_cast<DOM::DocumentImpl*>(part->document().handle());
        }
        int logicalDpiY = doc->logicalDpiY();
        return value->isPrimitiveValue() && compareValue(width, static_cast<CSSPrimitiveValueImpl*>(value)->computeLength(style,logicalDpiY), op);
    }
    // ({,min-,max-}device-width)
    // assume if we have a device, assume non-zero
    return true;
}

// cf. http://www.w3.org/TR/css3-mediaqueries/#device-width 
// "For continuous media, this is the width of the viewport (as described by CSS2, section 9.1.1 [CSS2]). 
//  For paged media, this is the width of the page box (as described by CSS2, section 13.2 [CSS2])"

static bool widthMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix op)
{
    KHTMLPart* rootPart = part;
    while (rootPart->parentPart()) rootPart = rootPart->parentPart();
    DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
    QPaintDevice *pd = doc->paintDevice(); 
    bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
    int width;
    if (printing)
        width = pd->width();
    else {
        width = part->view()->visibleWidth();
        doc = static_cast<DOM::DocumentImpl*>(part->document().handle());
    }
    int logicalDpiY = doc->logicalDpiY();
    if (value)
        return value->isPrimitiveValue() && compareValue(width, static_cast<CSSPrimitiveValueImpl*>(value)->computeLength(style, logicalDpiY), op);

    return width > 0;
}

static bool heightMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix op)
{
    KHTMLPart* rootPart = part;
    while (rootPart->parentPart()) rootPart = rootPart->parentPart();
    DOM::DocumentImpl *doc =  static_cast<DOM::DocumentImpl*>(rootPart->document().handle());
    QPaintDevice *pd = doc->paintDevice(); 
    bool printing = pd ? (pd->devType() == QInternal::Printer) : false;
    int height;
    if (printing)
        height = pd->height();
    else {
        height = part->view()->visibleHeight();
        doc = static_cast<DOM::DocumentImpl*>(part->document().handle());
    }
    int logicalDpiY = doc->logicalDpiY();
    if (value)
        return value->isPrimitiveValue() && compareValue(height, static_cast<CSSPrimitiveValueImpl*>(value)->computeLength(style, logicalDpiY), op);

    return height > 0;
}

static bool resolutionMediaFeatureEval(CSSValueImpl* value, RenderStyle*, KHTMLPart* part,  MediaFeaturePrefix op)
{
    DOM::DocumentImpl *d = static_cast<DOM::DocumentImpl*>(part->document().handle());
    int logicalDpiY = d ? d->logicalDpiY() : 0;

    if (value && logicalDpiY)
        return value->isPrimitiveValue() && compareValue(logicalDpiY, static_cast<CSSPrimitiveValueImpl*>(value)->getDPIResolution(), op);

    return logicalDpiY != 0;
}

static bool scanMediaFeatureEval(CSSValueImpl* /*value*/, RenderStyle*, KHTMLPart* /*part*/,  MediaFeaturePrefix)
{
    // no support for tv media type.
    return false;
}

// rest of the functions are trampolines which set the prefix according to the media feature expression used

static bool min_color_indexMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return color_indexMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_color_indexMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return color_indexMediaFeatureEval(value, style, part, MinPrefix);
}

static bool min_colorMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return colorMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_colorMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return colorMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_monochromeMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return monochromeMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_monochromeMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return monochromeMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_device_aspect_ratioMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return device_aspect_ratioMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_device_aspect_ratioMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return device_aspect_ratioMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_aspect_ratioMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return aspect_ratioMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_aspect_ratioMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return aspect_ratioMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_device_pixel_ratioMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return device_pixel_ratioMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_device_pixel_ratioMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return device_pixel_ratioMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_heightMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return heightMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_heightMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return heightMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_widthMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return widthMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_widthMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return widthMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_device_heightMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return device_heightMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_device_heightMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return device_heightMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_device_widthMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return device_widthMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_device_widthMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return device_widthMediaFeatureEval(value, style, part, MaxPrefix);
}

static bool min_resolutionMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return resolutionMediaFeatureEval(value, style, part, MinPrefix);
}

static bool max_resolutionMediaFeatureEval(CSSValueImpl* value, RenderStyle* style, KHTMLPart* part,  MediaFeaturePrefix /*op*/)
{
    return resolutionMediaFeatureEval(value, style, part, MaxPrefix);
}

static void createFunctionMap()
{
    // Create the table.
    gFunctionMap = new FunctionMap;
#define ADD_TO_FUNCTIONMAP(name, str)  \
    gFunctionMap->insert(DOMString(str), name##MediaFeatureEval);
    CSS_MEDIAQUERY_NAMES_FOR_EACH_MEDIAFEATURE(ADD_TO_FUNCTIONMAP);
#undef ADD_TO_FUNCTIONMAP
}

void MediaQueryEvaluator::cleanup() // static
{
    delete gFunctionMap;
    gFunctionMap = 0;        
}

bool MediaQueryEvaluator::eval(const MediaQueryExp* expr) const
{
    if (!m_part || !m_style)
        return m_expResult;

    if (!gFunctionMap)
        createFunctionMap();

    // call the media feature evaluation function. Assume no prefix
    // and let trampoline functions override the prefix if prefix is
    // used
    FunctionMap::ConstIterator func = gFunctionMap->constFind( expr->mediaFeature() );
    if (func != gFunctionMap->constEnd())
        return (*func.value())(expr->value(), m_style, m_part, NoPrefix);

    return false;
}
