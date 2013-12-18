/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
              (C) 2009 Maksim Orlovich <maksim@kde.org>

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

#include "css/css_svgvalueimpl.h"
#include "svg/SVGException.h"

namespace DOM {
    
SVGColorImpl::SVGColorImpl()
    : m_colorType(SVG_COLORTYPE_UNKNOWN)
{
}

SVGColorImpl::SVGColorImpl(const DOMString& rgbColor)
    : m_colorType(SVG_COLORTYPE_RGBCOLOR)
{
    setRGBColor(rgbColor);
}

SVGColorImpl::SVGColorImpl(unsigned short colorType)
    : m_colorType(colorType)
{}

SVGColorImpl::SVGColorImpl(const QColor& c)
    : m_color(c)
    , m_colorType(SVG_COLORTYPE_RGBCOLOR)
{}


SVGColorImpl::~SVGColorImpl()
{}

unsigned short SVGColorImpl::colorType() const
{
    return m_colorType;
}

unsigned SVGColorImpl::rgbColor() const
{
    return m_color.rgb();
}

void SVGColorImpl::setRGBColor(const DOMString& rgbColor, int& ec)
{
    QColor color = SVGColorImpl::colorFromRGBColorString(rgbColor);
    if (color.isValid())
        m_color = color;
    else
        ec = SVGException::SVG_INVALID_VALUE_ERR;
}

QColor SVGColorImpl::colorFromRGBColorString(const DOMString& colorDOMString)
{
        
    // ### this used to disallow hsl, etc, but I think CSS3 color ought to win..
    // ### FIXME, same as canvas... where should the helper go.
    
    /*DOMString s = colorDOMString.stripWhiteSpace();
    // hsl, hsla and rgba are not in the SVG spec.
    // FIXME: rework css parser so it is more svg aware
    if (s.startsWith("hsl") || s.startsWith("rgba"))
        return Color();
    RGBA32 color;
    if (CSSParser::parseColor(color, s))
        return color;
    return Color();*/
    return QColor(colorDOMString.string());
}

void SVGColorImpl::setRGBColorICCColor(const DOMString& /* rgbColor */, const DOMString& /* iccColor */, int& /* ec */)
{
    // ### TODO: implement me!
}

void SVGColorImpl::setColor(unsigned short colorType, const DOMString& /* rgbColor */ , const DOMString& /* iccColor */, int& /*ec*/)
{
    // ### TODO: implement me!
    m_colorType = colorType;
}

DOMString SVGColorImpl::cssText() const
{
    if (m_colorType == SVG_COLORTYPE_RGBCOLOR)
        return m_color.name();
    
    // ### FIXME: other types?

    return DOMString();
}

const QColor& SVGColorImpl::color() const
{
    return m_color;
}

SVGPaintImpl::SVGPaintImpl()
    : SVGColorImpl()
    , m_paintType(SVG_PAINTTYPE_UNKNOWN)
{
}

SVGPaintImpl::SVGPaintImpl(const DOMString& uri)
    : SVGColorImpl()
    , m_paintType(SVG_PAINTTYPE_URI_RGBCOLOR)
{
    setUri(uri);
}

SVGPaintImpl::SVGPaintImpl(SVGPaintType paintType)
    : SVGColorImpl()
    , m_paintType(paintType)
{
}

SVGPaintImpl::SVGPaintImpl(SVGPaintType paintType, const DOMString& uri, const DOMString& rgbPaint, const DOMString&)
    : SVGColorImpl(rgbPaint)
    , m_paintType(paintType)
{
    setUri(uri);
}

SVGPaintImpl::SVGPaintImpl(const QColor& c)
    : SVGColorImpl(c)
    , m_paintType(SVG_PAINTTYPE_RGBCOLOR)
{
}

SVGPaintImpl::SVGPaintImpl(const DOMString& uri, const QColor& c)
    : SVGColorImpl(c)
    , m_paintType(SVG_PAINTTYPE_URI_RGBCOLOR)
{
    setUri(uri);
}

SVGPaintImpl::~SVGPaintImpl()
{
}

SVGPaintImpl* SVGPaintImpl::defaultFill()
{
    static SVGPaintImpl* _defaultFill = new SVGPaintImpl(Qt::black);
    return _defaultFill;
}

SVGPaintImpl* SVGPaintImpl::defaultStroke()
{
    static SVGPaintImpl* _defaultStroke = new SVGPaintImpl(SVG_PAINTTYPE_NONE);
    return _defaultStroke;
}

DOMString SVGPaintImpl::uri() const
{
    return m_uri;
}

void SVGPaintImpl::setUri(const DOMString& uri)
{
    m_uri = uri;
}

void SVGPaintImpl::setPaint(SVGPaintType paintType, const DOMString& uri, const DOMString& rgbPaint, const DOMString&, int&)
{
    m_paintType = paintType;

    if (m_paintType == SVG_PAINTTYPE_URI)
        setUri(uri);
    else if (m_paintType == SVG_PAINTTYPE_RGBCOLOR)
        setRGBColor(rgbPaint);
}

DOMString SVGPaintImpl::cssText() const
{
    if (m_paintType == SVG_PAINTTYPE_NONE)
        return "none";
    else if (m_paintType == SVG_PAINTTYPE_CURRENTCOLOR)
        return "currentColor";
    else if (m_paintType == SVG_PAINTTYPE_URI)
        return m_uri; //return "url(" + m_uri + ")";

    return SVGColorImpl::cssText();
}

}

// vim:ts=4

