/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
 *           (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderSVGText_h
#define RenderSVGText_h

#if ENABLE(SVG)

#include "AffineTransform.h"
#include "RenderSVGBlock.h"
#include "IntRect.h"

namespace WebCore
{

class SVGTextElement;

class RenderSVGText : public RenderSVGBlock
{
public:
    RenderSVGText(SVGTextElement *node);

    const char *renderName() const override
    {
        return "RenderSVGText";
    }

    bool isSVGText() const override
    {
        return true;
    }

    bool calculateLocalTransform();
    AffineTransform localTransform() const override
    {
        return m_localTransform;
    }

    void paint(PaintInfo &, int tx, int ty) override;
    /*virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);*/

    bool requiresLayer() const override;
    void layout() override;

    virtual void absoluteRects(Vector<IntRect> &, int tx, int ty, bool topLevel = true);
    virtual IntRect absoluteClippedOverflowRect();
    FloatRect relativeBBox(bool includeStroke = true) const override;

    InlineBox *createInlineBox(bool makePlaceHolderBox, bool isRootLineBox/*, bool isOnlyRun = false*/) override;

private:
    AffineTransform m_localTransform;
    IntRect m_absoluteBounds;
};

}

#endif // ENABLE(SVG)
#endif

