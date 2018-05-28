/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGPathSegCurvetoQuadraticSmooth_h
#define SVGPathSegCurvetoQuadraticSmooth_h

#if ENABLE(SVG)

#include "SVGPathSeg.h"

namespace WebCore
{

class SVGPathSegCurvetoQuadraticSmoothAbs : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegCurvetoQuadraticSmoothAbs> create(float x, float y)
    {
        return adoptRef(new SVGPathSegCurvetoQuadraticSmoothAbs(x, y));
    }
    virtual ~SVGPathSegCurvetoQuadraticSmoothAbs();

    unsigned short pathSegType() const override
    {
        return PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS;
    }
    String pathSegTypeAsLetter() const override
    {
        return "T";
    }
    String toString() const override
    {
        return String::format("T %.6lg %.6lg", m_x, m_y);
    }

    void setX(float);
    float x() const;

    void setY(float);
    float y() const;

private:
    SVGPathSegCurvetoQuadraticSmoothAbs(float x, float y);

    float m_x;
    float m_y;
};

class SVGPathSegCurvetoQuadraticSmoothRel : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegCurvetoQuadraticSmoothRel> create(float x, float y)
    {
        return adoptRef(new SVGPathSegCurvetoQuadraticSmoothRel(x, y));
    }
    virtual ~SVGPathSegCurvetoQuadraticSmoothRel();

    unsigned short pathSegType() const override
    {
        return PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL;
    }
    String pathSegTypeAsLetter() const override
    {
        return "t";
    }
    String toString() const override
    {
        return String::format("t %.6lg %.6lg", m_x, m_y);
    }

    void setX(float);
    float x() const;

    void setY(float);
    float y() const;

private:
    SVGPathSegCurvetoQuadraticSmoothRel(float x, float y);

    float m_x;
    float m_y;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

