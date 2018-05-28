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

#ifndef SVGPathSegArc_h
#define SVGPathSegArc_h

#if ENABLE(SVG)

#include "SVGPathSeg.h"

#include <wtf/PassRefPtr.h>

namespace WebCore
{

class SVGPathSegArcAbs : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegArcAbs> create(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag)
    {
        return adoptRef(new SVGPathSegArcAbs(x, y, r1, r2, angle, largeArcFlag, sweepFlag));
    }

    virtual ~SVGPathSegArcAbs();

    unsigned short pathSegType() const override
    {
        return PATHSEG_ARC_ABS;
    }
    String pathSegTypeAsLetter() const override
    {
        return "A";
    }
    String toString() const override
    {
        return String::format("A %.6lg %.6lg %.6lg %d %d %.6lg %.6lg", m_r1, m_r2, m_angle, m_largeArcFlag, m_sweepFlag, m_x, m_y);
    }

    void setX(float x);
    float x() const;

    void setY(float y);
    float y() const;

    void setR1(float r1);
    float r1() const;

    void setR2(float r2);
    float r2() const;

    void setAngle(float angle);
    float angle() const;

    void setLargeArcFlag(bool largeArcFlag);
    bool largeArcFlag() const;

    void setSweepFlag(bool sweepFlag);
    bool sweepFlag() const;

private:
    SVGPathSegArcAbs(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag);

    float m_x;
    float m_y;
    float m_r1;
    float m_r2;
    float m_angle;

    bool m_largeArcFlag    : 1;
    bool m_sweepFlag : 1;
};

class SVGPathSegArcRel : public SVGPathSeg
{
public:
    static PassRefPtr<SVGPathSegArcRel> create(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag)
    {
        return adoptRef(new SVGPathSegArcRel(x, y, r1, r2, angle, largeArcFlag, sweepFlag));
    }
    virtual ~SVGPathSegArcRel();

    unsigned short pathSegType() const override
    {
        return PATHSEG_ARC_REL;
    }
    String pathSegTypeAsLetter() const override
    {
        return "a";
    }
    String toString() const override
    {
        return String::format("a %.6lg %.6lg %.6lg %d %d %.6lg %.6lg", m_r1, m_r2, m_angle, m_largeArcFlag, m_sweepFlag, m_x, m_y);
    }

    void setX(float x);
    float x() const;

    void setY(float y);
    float y() const;

    void setR1(float r1);
    float r1() const;

    void setR2(float r2);
    float r2() const;

    void setAngle(float angle);
    float angle() const;

    void setLargeArcFlag(bool largeArcFlag);
    bool largeArcFlag() const;

    void setSweepFlag(bool sweepFlag);
    bool sweepFlag() const;

private:
    SVGPathSegArcRel(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag);

    float m_x;
    float m_y;
    float m_r1;
    float m_r2;
    float m_angle;

    bool m_largeArcFlag : 1;
    bool m_sweepFlag : 1;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

