/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmial.com)
              (C) 2009 Maksim Orlovich (maksim@kde.org)

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

#ifndef _CSS_svg_valueimpl_h_
#define _CSS_svg_valueimpl_h_

#include <QColor>
#include "css/css_valueimpl.h"

namespace DOM {

class SVGCSSValueImpl : public CSSValueImpl {
public:
    virtual bool isSVGColor() const { return false; }
    virtual bool isSVGPaint() const { return false; }

    virtual unsigned short cssValueType() const { return DOM::CSSValue::CSS_SVG_VALUE; }
};

class SVGColorImpl : public SVGCSSValueImpl {
public:
    SVGColorImpl();
    SVGColorImpl(const DOMString& rgbColor);
    SVGColorImpl(const QColor& c);
    SVGColorImpl(unsigned short colorType);
    virtual ~SVGColorImpl();

    enum SVGColorType {
        SVG_COLORTYPE_UNKNOWN                   = 0,
        SVG_COLORTYPE_RGBCOLOR                  = 1,
        SVG_COLORTYPE_RGBCOLOR_ICCCOLOR         = 2,
        SVG_COLORTYPE_CURRENTCOLOR              = 3
    };

    // 'SVGColor' functions
    unsigned short colorType() const;

    unsigned rgbColor() const;
    
    static QColor colorFromRGBColorString(const DOMString&);
    void setRGBColor(const DOMString& rgbColor) { int ignored = 0; setRGBColor(rgbColor, ignored); }
    void setRGBColor(const DOMString& rgbColor, int&);
    void setRGBColorICCColor(const DOMString& rgbColor, const DOMString& iccColor, int&);
    void setColor(unsigned short colorType, const DOMString& rgbColor, const DOMString& iccColor, int&);

    virtual DOMString cssText() const;

    // Helpers
    const QColor& color() const;

    virtual bool isSVGColor() const { return true; }

    virtual unsigned short cssValueType() const { return DOM::CSSValue::CSS_SVG_VALUE; }
private:
    QColor m_color;
    unsigned short m_colorType;
};


class SVGPaintImpl : public SVGColorImpl {
public:
    enum SVGPaintType {
        SVG_PAINTTYPE_UNKNOWN               = 0,
        SVG_PAINTTYPE_RGBCOLOR              = 1,
        SVG_PAINTTYPE_RGBCOLOR_ICCCOLOR     = 2,
        SVG_PAINTTYPE_NONE                  = 101,
        SVG_PAINTTYPE_CURRENTCOLOR          = 102,
        SVG_PAINTTYPE_URI_NONE              = 103,
        SVG_PAINTTYPE_URI_CURRENTCOLOR      = 104,
        SVG_PAINTTYPE_URI_RGBCOLOR          = 105,
        SVG_PAINTTYPE_URI_RGBCOLOR_ICCCOLOR = 106,
        SVG_PAINTTYPE_URI                   = 107
    };

    SVGPaintImpl();
    SVGPaintImpl(const DOMString& uri);
    SVGPaintImpl(SVGPaintType);
    SVGPaintImpl(SVGPaintType, const DOMString& uri, const DOMString& rgbPaint = DOMString(), const DOMString& iccPaint = DOMString());
    SVGPaintImpl(const QColor& c);
    SVGPaintImpl(const DOMString& uri, const QColor& c);
    virtual ~SVGPaintImpl();

    // 'SVGPaint' functions
    SVGPaintType paintType() const { return m_paintType; }
    DOMString uri() const;

    void setUri(const DOMString&);
    void setPaint(SVGPaintType, const DOMString& uri, const DOMString& rgbPaint, const DOMString& iccPaint, int&);

    virtual DOMString cssText() const;
    
    static SVGPaintImpl* defaultFill();
    static SVGPaintImpl* defaultStroke();

    virtual bool isSVGPaint() const { return true; }
private:
    SVGPaintType m_paintType;
    DOMString m_uri;
};

} // namespace DOM

#endif 

// vim:ts=4
