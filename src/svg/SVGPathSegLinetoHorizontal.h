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

#ifndef SVGPathSegLinetoHorizontal_h
#define SVGPathSegLinetoHorizontal_h

#if ENABLE(SVG)

#include "SVGPathSeg.h"

namespace WebCore
{

class SVGPathSegLinetoHorizontalAbs : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegLinetoHorizontalAbs> create(float x)
    {
        return adoptRef(new SVGPathSegLinetoHorizontalAbs(x));
    }
    virtual ~SVGPathSegLinetoHorizontalAbs();

    unsigned short pathSegType() const override
    {
        return PATHSEG_LINETO_HORIZONTAL_ABS;
    }
    String pathSegTypeAsLetter() const override
    {
        return "H";
    }
    String toString() const override
    {
        return String::format("H %.6lg", m_x);
    }

    void setX(float);
    float x() const;

private:
    SVGPathSegLinetoHorizontalAbs(float x);
    float m_x;
};

class SVGPathSegLinetoHorizontalRel : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegLinetoHorizontalRel> create(float x)
    {
        return adoptRef(new SVGPathSegLinetoHorizontalRel(x));
    }
    virtual ~SVGPathSegLinetoHorizontalRel();

    unsigned short pathSegType() const override
    {
        return PATHSEG_LINETO_HORIZONTAL_REL;
    }
    String pathSegTypeAsLetter() const override
    {
        return "h";
    }
    String toString() const override
    {
        return String::format("h %.6lg", m_x);
    }

    void setX(float);
    float x() const;

private:
    SVGPathSegLinetoHorizontalRel(float x);

    float m_x;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

