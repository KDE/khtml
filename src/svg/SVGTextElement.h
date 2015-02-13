/*
    Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>

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

#ifndef SVGTextElement_h
#define SVGTextElement_h

#if ENABLE(SVG)
#include "SVGTextPositioningElement.h"
#include "SVGTransformable.h"

#include <wtf/OwnPtr.h>

namespace WebCore
{

class SVGTextElement : public SVGTextPositioningElement,
    public SVGTransformable
{
public:
    SVGTextElement(const QualifiedName &, Document *);
    virtual ~SVGTextElement();

    void parseMappedAttribute(MappedAttribute *) Q_DECL_OVERRIDE;

    SVGElement *nearestViewportElement() const Q_DECL_OVERRIDE;
    SVGElement *farthestViewportElement() const Q_DECL_OVERRIDE;

    FloatRect getBBox() const Q_DECL_OVERRIDE;
    AffineTransform getCTM() const Q_DECL_OVERRIDE;
    AffineTransform getScreenCTM() const Q_DECL_OVERRIDE;
    AffineTransform animatedLocalTransform() const Q_DECL_OVERRIDE;
    AffineTransform *supplementalTransform() Q_DECL_OVERRIDE;

    RenderObject *createRenderer(RenderArena *, RenderStyle *) Q_DECL_OVERRIDE;
    bool childShouldCreateRenderer(Node *) const Q_DECL_OVERRIDE;

    void svgAttributeChanged(const QualifiedName &) Q_DECL_OVERRIDE;

    // KHTML ElementImpl pure virtual method
    quint32 id() const Q_DECL_OVERRIDE
    {
        return SVGNames::textTag.id();
    }
protected:
    const SVGElement *contextElement() const Q_DECL_OVERRIDE
    {
        return this;
    }

private:
    ANIMATED_PROPERTY_DECLARATIONS(SVGTextElement, SVGTransformList *, RefPtr<SVGTransformList>, Transform, transform)

    // Used by <animateMotion>
    OwnPtr<AffineTransform> m_supplementalTransform;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
