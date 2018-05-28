/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
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
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef RenderSVGContainer_h
#define RenderSVGContainer_h

#if ENABLE(SVG)

#include "RenderPath.h"
#include "SVGPreserveAspectRatio.h"

namespace WebCore
{

class SVGElement;

class RenderSVGContainer : public RenderObject
{
public:
    RenderSVGContainer(SVGStyledElement *);
    ~RenderSVGContainer();

    RenderObject *firstChild() const override
    {
        return m_firstChild;
    }
    RenderObject *lastChild() const override
    {
        return m_lastChild;
    }

    short/*khtml*/ int width() const override
    {
        return m_width;
    }
    int height() const override
    {
        return m_height;
    }

    virtual bool canHaveChildren() const;
    void addChild(RenderObject *newChild, RenderObject *beforeChild = nullptr) override;
    void removeChild(RenderObject *) override;

    virtual void destroy();
    void destroyLeftoverChildren();

    // uncomment if you know how line 64' ambiguoty should be solved in that case.
    // using khtml::RenderObject::removeChildNode;
    RenderObject *removeChildNode(RenderObject *) override;
    // uncomment if you know how line 64' ambiguoty should be solved in that case.
    // using khtml::RenderObject::appendChildNode;
    void appendChildNode(RenderObject *) override;
    // uncomment if you know how line 62' of the implementation ambiguoty should be solved in that case.
    // using khtml::RenderObject::insertChildNode;
    void insertChildNode(RenderObject *child, RenderObject *before) override;

    // Designed for speed.  Don't waste time doing a bunch of work like layer updating and repainting when we know that our
    // change in parentage is not going to affect anything.
    virtual void moveChildNode(RenderObject *child)
    {
        appendChildNode(child->parent()->removeChildNode(child));
    }

    void calcMinMaxWidth() override
    {
        setMinMaxKnown();
    }

    // Some containers do not want it's children
    // to be drawn, because they may be 'referenced'
    // Example: <marker> children in SVG
    void setDrawsContents(bool);
    bool drawsContents() const;

    bool isSVGContainer() const override
    {
        return true;
    }
    const char *renderName() const override
    {
        return "RenderSVGContainer";
    }

    bool requiresLayer() const override;
    short lineHeight(bool b) const override;
    short baselinePosition(bool b) const override;

    void layout() override;
    void paint(PaintInfo &, int parentX, int parentY) override;

    virtual IntRect absoluteClippedOverflowRect();
    virtual void absoluteRects(Vector<IntRect> &rects, int tx, int ty, bool topLevel = true);

    FloatRect relativeBBox(bool includeStroke = true) const override;

    virtual bool calculateLocalTransform();
    AffineTransform localTransform() const override;
    virtual AffineTransform viewportTransform() const;

    /*virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);*/

    bool childAllowed() const override
    {
        return true;
    }

protected:
    virtual void applyContentTransforms(PaintInfo &);
    virtual void applyAdditionalTransforms(PaintInfo &);

    void calcBounds();

private:
    int calcReplacedWidth() const;
    int calcReplacedHeight() const;

    RenderObject *m_firstChild;
    RenderObject *m_lastChild;

    int m_width;
    int m_height;

    bool selfWillPaint() const;

    bool m_drawsContents : 1;

protected:
    IntRect m_absoluteBounds;
    AffineTransform m_localTransform;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGContainer_h

