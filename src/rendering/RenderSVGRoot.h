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

#ifndef RenderSVGRoot_h
#define RenderSVGRoot_h

#if ENABLE(SVG)
//#include "RenderContainer.h"
#include "RenderBox.h"
#include "FloatRect.h"

namespace WebCore
{

class SVGStyledElement;
class AffineTransform;

class RenderSVGRoot : public RenderBox /*RenderContainer in WebCore*/
{
public:
    RenderSVGRoot(SVGStyledElement *);
    ~RenderSVGRoot();

    bool isSVGRoot() const Q_DECL_OVERRIDE
    {
        return true;
    }
    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderSVGRoot";
    }

    short lineHeight(bool b) const Q_DECL_OVERRIDE;
    short baselinePosition(bool b) const Q_DECL_OVERRIDE;
    void calcMinMaxWidth() Q_DECL_OVERRIDE;

    void layout() Q_DECL_OVERRIDE;
    void paint(PaintInfo &, int parentX, int parentY) Q_DECL_OVERRIDE;

    virtual IntRect absoluteClippedOverflowRect();
    virtual void absoluteRects(Vector<IntRect> &rects, int tx, int ty);
    //virtual void addFocusRingRects(GraphicsContext*, int tx, int ty);

    AffineTransform absoluteTransform() const Q_DECL_OVERRIDE;

    bool fillContains(const FloatPoint &) const;
    bool strokeContains(const FloatPoint &) const;
    FloatRect relativeBBox(bool includeStroke = true) const Q_DECL_OVERRIDE;

    AffineTransform localTransform() const Q_DECL_OVERRIDE;

    FloatRect viewport() const;

    //virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

private:
    void calcViewport();

    //void applyContentTransforms(PaintInfo&, int parentX, int parentY);

    FloatRect m_viewport;
    IntRect m_absoluteBounds;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // RenderSVGRoot_h

