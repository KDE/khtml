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

#ifndef SVGPathSegMoveto_h
#define SVGPathSegMoveto_h

#if ENABLE(SVG)

#include "SVGPathSeg.h"

namespace WebCore
{
class SVGPathSegMovetoAbs : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegMovetoAbs> create(float x, float y)
    {
        return adoptRef(new SVGPathSegMovetoAbs(x, y));
    }
    virtual ~SVGPathSegMovetoAbs();

    unsigned short pathSegType() const override
    {
        return PATHSEG_MOVETO_ABS;
    }
    String pathSegTypeAsLetter() const override
    {
        return "M";
    }
    String toString() const override
    {
        return String::format("M %.6lg %.6lg", m_x, m_y);
    }

    void setX(float);
    float x() const;

    void setY(float);
    float y() const;

private:
    SVGPathSegMovetoAbs(float x, float y);

    float m_x;
    float m_y;
};

class SVGPathSegMovetoRel : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegMovetoRel> create(float x, float y)
    {
        return adoptRef(new SVGPathSegMovetoRel(x, y));
    }
    virtual ~SVGPathSegMovetoRel();

    unsigned short pathSegType() const override
    {
        return PATHSEG_MOVETO_REL;
    }
    String pathSegTypeAsLetter() const override
    {
        return "m";
    }
    String toString() const override
    {
        return String::format("m %.6lg %.6lg", m_x, m_y);
    }

    void setX(float);
    float x() const;

    void setY(float);
    float y() const;

private:
    SVGPathSegMovetoRel(float x, float y);

    float m_x;
    float m_y;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

