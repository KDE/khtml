/*
    Copyright (C) 2006 Nikolas Zimmermann <wildfox@kde.org>

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

#include "wtf/Platform.h"

#if ENABLE(SVG)
#include "SVGPaintServerSolid.h"

#include "RenderPath.h"

#include <QPainter>

namespace WebCore {

bool SVGPaintServerSolid::setup(QPainter* painter, QPainterPath* painterPath, const RenderObject* object, SVGPaintTargetType type, bool isPaintingText) const
{
    /*QPainter* painter(context ? context->platformContext() : 0);
    Q_ASSERT(painter);*/

    RenderStyle* renderStyle = object->style();
    // TODO? painter->setOpacity(renderStyle->opacity());

    QColor c = color();

    if ((type & ApplyToFillTargetType) && renderStyle->svgStyle()->hasFill()) {
        c.setAlphaF(renderStyle->svgStyle()->fillOpacity());

        QBrush brush(c);
        painter->setBrush(brush);
        if (painterPath)
            painterPath->setFillRule(renderStyle->svgStyle()->fillRule() == RULE_EVENODD ? Qt::OddEvenFill : Qt::WindingFill);
        /*context->setFillRule(renderStyle->svgStyle()->fillRule());*/

        if (isPaintingText && !(type & ApplyToStrokeTargetType))
            painter->setPen(c);
    }

    if ((type & ApplyToStrokeTargetType) && renderStyle->svgStyle()->hasStroke()) {
        c.setAlphaF(renderStyle->svgStyle()->strokeOpacity());

        QPen pen(c);
        setPenProperties(object, renderStyle, pen);
        painter->setPen(pen);

        /* if(isPaintingText()) ... */
    }

    return true;
}

} // namespace WebCore

#endif

// vim:ts=4
