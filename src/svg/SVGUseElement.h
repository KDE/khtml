/*
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGUseElement_h
#define SVGUseElement_h

#if ENABLE(SVG)
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"
#include "SVGURIReference.h"

namespace WebCore
{

class SVGElementInstance;
class SVGLength;

class SVGUseElement : public SVGStyledTransformableElement,
    public SVGTests,
    public SVGLangSpace,
    public SVGExternalResourcesRequired,
    public SVGURIReference
{
public:
    SVGUseElement(const QualifiedName &, Document *);
    virtual ~SVGUseElement();

    SVGElementInstance *instanceRoot() const;
    SVGElementInstance *animatedInstanceRoot() const;

    bool isValid() const override
    {
        return SVGTests::isValid();
    }

    void insertedIntoDocument() override;
    void removedFromDocument() override;
    void buildPendingResource() override;

    void parseMappedAttribute(MappedAttribute *) override;
    using DOM::NodeImpl::childrenChanged;
    void childrenChanged(bool changedByParser = false, Node *beforeChange = nullptr, Node *afterChange = nullptr, int childCountDelta = 0) override;

    void svgAttributeChanged(const QualifiedName &) override;
    void recalcStyle(StyleChange = NoChange) override;

    RenderObject *createRenderer(RenderArena *, RenderStyle *) override;
    void attach() override;
    void detach() override;

    Path toClipPath() const override;

    static void removeDisallowedElementsFromSubtree(Node *element);

    // KHTML ElementImpl pure virtual method
    quint32 id() const override
    {
        return SVGNames::useTag.id();
    }
protected:
    const SVGElement *contextElement() const override
    {
        return this;
    }

private:
    ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGExternalResourcesRequired, bool, ExternalResourcesRequired, externalResourcesRequired)
    ANIMATED_PROPERTY_FORWARD_DECLARATIONS(SVGURIReference, String, Href, href)

    ANIMATED_PROPERTY_DECLARATIONS(SVGUseElement, SVGLength, SVGLength, X, x)
    ANIMATED_PROPERTY_DECLARATIONS(SVGUseElement, SVGLength, SVGLength, Y, y)
    ANIMATED_PROPERTY_DECLARATIONS(SVGUseElement, SVGLength, SVGLength, Width, width)
    ANIMATED_PROPERTY_DECLARATIONS(SVGUseElement, SVGLength, SVGLength, Height, height)

private:
    friend class SVGElement;
    SVGElementInstance *instanceForShadowTreeElement(Node *element) const;

private:
    // Instance tree handling
    void buildInstanceTree(SVGElement *target, SVGElementInstance *targetInstance, bool &foundCycle);
    void handleDeepUseReferencing(SVGElement *use, SVGElementInstance *targetInstance, bool &foundCycle);

    // Shadow tree handling
    PassRefPtr<SVGSVGElement> buildShadowTreeForSymbolTag(SVGElement *target, SVGElementInstance *targetInstance);
    void alterShadowTreeForSVGTag(SVGElement *target);

    void buildShadowTree(SVGElement *target, SVGElementInstance *targetInstance);

#if ENABLE(SVG) && ENABLE(SVG_USE)
    void expandUseElementsInShadowTree(Node *element);
    void expandSymbolElementsInShadowTree(Node *element);
#endif

    void attachShadowTree();

    // "Tree connector"
    void associateInstancesWithShadowTreeElements(Node *target, SVGElementInstance *targetInstance);

    SVGElementInstance *instanceForShadowTreeElement(Node *element, SVGElementInstance *instance) const;
    void transferUseAttributesToReplacedElement(SVGElement *from, SVGElement *to) const;

    RefPtr<SVGElement> m_shadowTreeRootElement;
    RefPtr<SVGElementInstance> m_targetElementInstance;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
