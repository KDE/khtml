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

    RenderObject *firstChild() const Q_DECL_OVERRIDE
    {
        return m_firstChild;
    }
    RenderObject *lastChild() const Q_DECL_OVERRIDE
    {
        return m_lastChild;
    }

    short/*khtml*/ int width() const Q_DECL_OVERRIDE
    {
        return m_width;
    }
    int height() const Q_DECL_OVERRIDE
    {
        return m_height;
    }

    virtual bool canHaveChildren() const;
    void addChild(RenderObject *newChild, RenderObject *beforeChild = nullptr) Q_DECL_OVERRIDE;
    void removeChild(RenderObject *) Q_DECL_OVERRIDE;

    virtual void destroy();
    void destroyLeftoverChildren();

    // uncomment if you know how line 64' ambiguoty should be solved in that case.
    // using khtml::RenderObject::removeChildNode;
    RenderObject *removeChildNode(RenderObject *) Q_DECL_OVERRIDE;
    // uncomment if you know how line 64' ambiguoty should be solved in that case.
    // using khtml::RenderObject::appendChildNode;
    void appendChildNode(RenderObject *) Q_DECL_OVERRIDE;
    // uncomment if you know how line 62' of the implementation ambiguoty should be solved in that case.
    // using khtml::RenderObject::insertChildNode;
    void insertChildNode(RenderObject *child, RenderObject *before) Q_DECL_OVERRIDE;

    // Designed for speed.  Don't waste time doing a bunch of work like layer updating and repainting when we know that our
    // change in parentage is not going to affect anything.
    virtual void moveChildNode(RenderObject *child)
    {
        appendChildNode(child->parent()->removeChildNode(child));
    }

    void calcMinMaxWidth() Q_DECL_OVERRIDE
    {
        setMinMaxKnown();
    }

    // Some containers do not want it's children
    // to be drawn, because they may be 'referenced'
    // Example: <marker> children in SVG
    void setDrawsContents(bool);
    bool drawsContents() const;

    bool isSVGContainer() const Q_DECL_OVERRIDE
    {
        return true;
    }
    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderSVGContainer";
    }

    bool requiresLayer() const Q_DECL_OVERRIDE;
    short lineHeight(bool b) const Q_DECL_OVERRIDE;
    short baselinePosition(bool b) const Q_DECL_OVERRIDE;

    void layout() Q_DECL_OVERRIDE;
    void paint(PaintInfo &, int parentX, int parentY) Q_DECL_OVERRIDE;

    virtual IntRect absoluteClippedOverflowRect();
    virtual void absoluteRects(Vector<IntRect> &rects, int tx, int ty, bool topLevel = true);

    FloatRect relativeBBox(bool includeStroke = true) const Q_DECL_OVERRIDE;

    virtual bool calculateLocalTransform();
    AffineTransform localTransform() const Q_DECL_OVERRIDE;
    virtual AffineTransform viewportTransform() const;

    /*virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);*/

    bool childAllowed() const Q_DECL_OVERRIDE
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

