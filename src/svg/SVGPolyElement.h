/*
    Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>

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

#ifndef SVGPolyElement_h
#define SVGPolyElement_h

#if ENABLE(SVG)
#include "SVGAnimatedPoints.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore
{

class SVGPolyElement :  public SVGStyledTransformableElement,
    public SVGTests,
    public SVGLangSpace,
    public SVGExternalResourcesRequired,
    public SVGAnimatedPoints
{
public:
    SVGPolyElement(const QualifiedName &, Document *);
    virtual ~SVGPolyElement();

    bool isValid() const Q_DECL_OVERRIDE
    {
        return SVGTests::isValid();
    }

    SVGPointList *points() const Q_DECL_OVERRIDE;
    SVGPointList *animatedPoints() const Q_DECL_OVERRIDE;

    void parseMappedAttribute(MappedAttribute *) Q_DECL_OVERRIDE;
    void svgAttributeChanged(const QualifiedName &) Q_DECL_OVERRIDE;

    bool rendererIsNeeded(RenderStyle *style) Q_DECL_OVERRIDE
    {
        return StyledElement::rendererIsNeeded(style);
    }
    bool supportsMarkers() const Q_DECL_OVERRIDE
    {
        return true;
    }

protected:
    const SVGElement *contextElement() const Q_DECL_OVERRIDE
    {
        return this;
    }

private:
    bool m_ignoreAttributeChanges : 1;
    mutable RefPtr<SVGPointList> m_points;

    ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGExternalResourcesRequired, bool, ExternalResourcesRequired, externalResourcesRequired)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
