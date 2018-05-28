/*
    Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>

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

#ifndef SVGStyledLocatableElement_h
#define SVGStyledLocatableElement_h

#if ENABLE(SVG)
#include "SVGLocatable.h"
#include "SVGStyledElement.h"

namespace WebCore
{

class SVGElement;

class SVGStyledLocatableElement : public SVGStyledElement,
    virtual public SVGLocatable
{
public:
    SVGStyledLocatableElement(const QualifiedName &, Document *);
    virtual ~SVGStyledLocatableElement();

    bool isStyledLocatable() const override
    {
        return true;
    }

    SVGElement *nearestViewportElement() const override;
    SVGElement *farthestViewportElement() const override;

    FloatRect getBBox() const override;
    AffineTransform getCTM() const override;
    AffineTransform getScreenCTM() const override;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGStyledLocatableElement_h
