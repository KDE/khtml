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

#ifndef SVGPathSegLinetoVertical_h
#define SVGPathSegLinetoVertical_h

#if ENABLE(SVG)

#include "SVGPathSeg.h"

namespace WebCore
{

class SVGPathSegLinetoVerticalAbs : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegLinetoVerticalAbs> create(float y)
    {
        return adoptRef(new SVGPathSegLinetoVerticalAbs(y));
    }
    virtual~SVGPathSegLinetoVerticalAbs();

    unsigned short pathSegType() const override
    {
        return PATHSEG_LINETO_VERTICAL_ABS;
    }
    String pathSegTypeAsLetter() const override
    {
        return "V";
    }
    String toString() const override
    {
        return String::format("V %.6lg", m_y);
    }

    void setY(float);
    float y() const;

private:
    SVGPathSegLinetoVerticalAbs(float y);

    float m_y;
};

class SVGPathSegLinetoVerticalRel : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegLinetoVerticalRel> create(float y)
    {
        return adoptRef(new SVGPathSegLinetoVerticalRel(y));
    }
    virtual ~SVGPathSegLinetoVerticalRel();

    unsigned short pathSegType() const override
    {
        return PATHSEG_LINETO_VERTICAL_REL;
    }
    String pathSegTypeAsLetter() const override
    {
        return "v";
    }
    String toString() const override
    {
        return String::format("v %.6lg", m_y);
    }

    void setY(float);
    float y() const;

private:
    SVGPathSegLinetoVerticalRel(float y);

    float m_y;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

