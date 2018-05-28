/*
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2007 Rob Buis <buis@kde.org>

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

#ifndef SVGStyledElement_h
#define SVGStyledElement_h

#if ENABLE(SVG)
#include "AffineTransform.h"
#include "Path.h"
#include "SVGElement.h"
#include "SVGLength.h"
#include "SVGResource.h"
#include "SVGStylable.h"

//khtml compat
#include "RenderStyle.h"

namespace WebCore
{

class SVGStyledElement : public SVGElement,
    public SVGStylable
{
public:
    SVGStyledElement(const QualifiedName &, Document *);
    virtual ~SVGStyledElement();

    bool isStyled() const override
    {
        return true;
    }
    virtual bool supportsMarkers() const
    {
        return false;
    }

    PassRefPtr<DOM::CSSValueImpl> getPresentationAttribute(const DOMString &name) override;
    DOM::CSSStyleDeclarationImpl *style() override
    {
        return StyledElement::style();
    }

    bool isKnownAttribute(const QualifiedName &);

    bool rendererIsNeeded(RenderStyle *) override;
    virtual SVGResource *canvasResource()
    {
        return nullptr;
    }

    /*virtual bool mapToEntry(const QualifiedName&, MappedAttributeEntry&) const;*/
    void parseMappedAttribute(MappedAttribute *) override;

    void svgAttributeChanged(const QualifiedName &) override;

    using DOM::NodeImpl::childrenChanged;
    virtual void childrenChanged(bool changedByParser = false, Node *beforeChange = nullptr, Node *afterChange = nullptr, int childCountDelta = 0);

    // Centralized place to force a manual style resolution. Hacky but needed for now.
    RenderStyle *resolveStyle(RenderStyle *parentStyle);

    void invalidateResourcesInAncestorChain() const;
    void detach() override;

    void setInstanceUpdatesBlocked(bool);

protected:
    virtual bool hasRelativeValues() const
    {
        return true;
    }

    static int cssPropertyIdForSVGAttributeName(const QualifiedName &);

private:
    ANIMATED_PROPERTY_DECLARATIONS(SVGStyledElement, String, String, ClassName, className)

    void updateElementInstance(SVGDocumentExtensions *) const;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGStyledElement
