/*
 * Copyright (C) 2008-2009 Fredrik Höglund <fredrik@kde.org>
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
 */

#include "borderarcstroker.h"
#include <cmath>

#if defined(__GNUC__)
#  define K_ALIGN(n) __attribute((aligned(n)))
#  if defined(__SSE__)
#    define HAVE_SSE
#  endif
#elif defined(__INTEL_COMPILER)
#  define K_ALIGN(n) __declspec(align(n))
#  define HAVE_SSE
#else
#  define K_ALIGN(n)
#endif

#ifdef HAVE_SSE
#  include <xmmintrin.h>
#endif


namespace khtml {


// This is a helper class used by BorderArcStroker
class KCubicBezier
{
public:
    KCubicBezier() {}
    KCubicBezier(const QPointF &p0, const QPointF &p1, const QPointF &p2, const QPointF &p3);

    void split(KCubicBezier *a, KCubicBezier *b, qreal t = .5) const;
    KCubicBezier section(qreal t1, qreal t2) const;
    KCubicBezier leftSection(qreal t) const;
    KCubicBezier rightSection(qreal t) const;
    KCubicBezier derivative() const;
    QPointF pointAt(qreal t) const;
    QPointF deltaAt(qreal t) const;
    QLineF normalVectorAt(qreal t) const;
    qreal slopeAt(qreal t) const;
    qreal tAtLength(qreal l) const;
    qreal tAtIntersection(const QLineF &line) const;
    QLineF chord() const;
    qreal length() const;
    qreal convexHullLength() const;

    QPointF p0() const { return QPointF(x0, y0); }
    QPointF p1() const { return QPointF(x1, y1); }
    QPointF p2() const { return QPointF(x2, y2); }
    QPointF p3() const { return QPointF(x3, y3); }

public:
    qreal x0, y0, x1, y1, x2, y2, x3, y3;
};


KCubicBezier::KCubicBezier(const QPointF &p0, const QPointF &p1, const QPointF &p2, const QPointF &p3)
{
    x0 = p0.x();
    y0 = p0.y();
    x1 = p1.x();
    y1 = p1.y();
    x2 = p2.x();
    y2 = p2.y();
    x3 = p3.x();
    y3 = p3.y();
}

// Splits the curve at t, using the de Casteljau algorithm.
// This function is not atomic, so do not pass a pointer to the curve being split.
void KCubicBezier::split(KCubicBezier *a, KCubicBezier *b, qreal t) const
{
    a->x0 = x0;
    a->y0 = y0;

    b->x3 = x3;
    b->y3 = y3;

    a->x1 = x0 + (x1 - x0) * t;
    a->y1 = y0 + (y1 - y0) * t;

    b->x2 = x2 + (x3 - x2) * t;
    b->y2 = y2 + (y3 - y2) * t;

    // The point on the line (p1, p2).
    const qreal x = x1 + (x2 - x1) * t;
    const qreal y = y1 + (y2 - y1) * t;

    a->x2 = a->x1 + (x - a->x1) * t;
    a->y2 = a->y1 + (y - a->y1) * t;

    b->x1 = x + (b->x2 - x) * t;
    b->y1 = y + (b->y2 - y) * t;

    a->x3 = b->x0 = a->x2 + (b->x1 - a->x2) * t;
    a->y3 = b->y0 = a->y2 + (b->y1 - a->y2) * t;
}

// Returns a new bezier curve that is the interval between t1 and t2 in this curve
KCubicBezier KCubicBezier::section(qreal t1, qreal t2) const
{
    return leftSection(t2).rightSection(t1 / t2);
}

// Returns a new bezier curve that is the section of this curve left of t
KCubicBezier KCubicBezier::leftSection(qreal t) const
{
    KCubicBezier left, right;
    split(&left, &right, t);
    return left;
}

// Returns a new bezier curve that is the section of this curve right of t
KCubicBezier KCubicBezier::rightSection(qreal t) const
{
    KCubicBezier left, right;
    split(&left, &right, t);
    return right;
}

// Returns the point at t
QPointF KCubicBezier::pointAt(qreal t) const
{
    const qreal m_t = 1 - t;
    const qreal m_t2 = m_t * m_t;
    const qreal m_t3 = m_t2 * m_t;
    const qreal t2 = t * t;
    const qreal t3 = t2 * t;

    const qreal x = m_t3 * x0 + 3 * t * m_t2 * x1 + 3 * t2 * m_t * x2 + t3 * x3;
    const qreal y = m_t3 * y0 + 3 * t * m_t2 * y1 + 3 * t2 * m_t * y2 + t3 * y3;

    return QPointF(x, y);
}

// Returns the delta at t, i.e. the first derivative of the cubic equation at t.
QPointF KCubicBezier::deltaAt(qreal t) const
{
    const qreal m_t = 1 - t;
    const qreal m_t2 = m_t * m_t;
    const qreal t2 = t * t;

    const qreal dx = 3 * (x1 - x0) * m_t2 + 3 * (x2 - x1) * 2 * t * m_t + 3 * (x3 - x2) * t2;
    const qreal dy = 3 * (y1 - y0) * m_t2 + 3 * (y2 - y1) * 2 * t * m_t + 3 * (y3 - y2) * t2;

    return QPointF(dx, dy);
}

// Returns the slope at t (dy / dx)
qreal KCubicBezier::slopeAt(qreal t) const
{
    const QPointF delta = deltaAt(t);
    return qFuzzyIsNull(delta.x()) ?
                (delta.y() < 0 ? -1 : 1) : delta.y() / delta.x();
}

// Returns the normal vector at t
QLineF KCubicBezier::normalVectorAt(qreal t) const
{
    const QPointF point = pointAt(t);
    const QPointF delta = deltaAt(t);

    return QLineF(point, point + delta).normalVector();
}

// Returns a curve that is the first derivative of this curve.
// The first derivative of a cubic bezier curve is a quadratic bezier curve,
// but this function elevates it to a cubic curve.
KCubicBezier KCubicBezier::derivative() const
{
    KCubicBezier c;

    c.x0 = 3 * (x1 - x0);
    c.x1 = x1 - x0 + 2 * (x2 - x1);
    c.x2 = 2 * (x2 - x1) + x3 - x2;
    c.x3 = 3 * (x3 - x2);

    c.y0 = 3 * (y1 - y0);
    c.y1 = y1 - y0 + 2 * (y2 - y1);
    c.y2 = 2 * (y2 - y1) + y3 - y2;
    c.y3 = 3 * (y3 - y2);

    return c;
}

// Returns the sum of the lengths of the lines connecting the control points
qreal KCubicBezier::convexHullLength() const
{
#ifdef HAVE_SSE
    K_ALIGN(16) float vals[4];

    __m128 m1, m2;
    m1 = _mm_set_ps(0, x1 - x0, x2 - x1, x3 - x2);
    m2 = _mm_set_ps(0, y1 - y0, y2 - y1, y3 - y2);
    m1 = _mm_add_ps(_mm_mul_ps(m1, m1), _mm_mul_ps(m2, m2));
    _mm_store_ps(vals, _mm_sqrt_ps(m1));

    return vals[0] + vals[1] + vals[2];
#else
    return QLineF(x0, y0, x1, y1).length() + QLineF(x1, y1, x2, y2).length() + QLineF(x2, y2, x3, y3).length();
#endif
}

// Returns the chord, i.e. the line that connects the two endpoints of the curve
QLineF KCubicBezier::chord() const
{
    return QLineF(x0, y0, x3, y3);
}

// Returns the arc length of the bezier curve, computed by recursively subdividing the
// curve until the difference between the length of the convex hull of the section
// and its chord is less than .01 device units.
qreal KCubicBezier::length() const
{
    const qreal error = .01;
    const qreal len = convexHullLength();

    if ((len - chord().length()) > error) {
        KCubicBezier left, right;
        split(&left, &right);
        return left.length() + right.length();
    }

    return len;
}

// Returns the value for the t parameter that corresponds to the point
// l device units from the starting point of the curve.
qreal KCubicBezier::tAtLength(qreal l) const
{
    const qreal error = 0.1;

    if (l <= 0)
        return 0;

    qreal len = length();

    if (l > len || qFuzzyCompare(l + 1.0, len + 1.0))
        return 1;

    qreal upperT = 1;
    qreal lowerT = 0;

    while (1)
    {
        const qreal t = lowerT + (upperT - lowerT) / 2;
        const qreal len = leftSection(t).length();

        if (qAbs(l - len) < error)
            return t;

        if (l > len)
            lowerT = t;
        else
            upperT = t;
    }
}

// Returns the value of t at the point where the given line intersects
// the bezier curve. The line is treated as an infinitely long line.
// Note that this implementation assumes that there is a single point of
// intersection, that both control points are on the same side of
// the curve, and that the curve is not self intersecting.
qreal KCubicBezier::tAtIntersection(const QLineF &line) const
{
    const qreal error = 0.1;

    // Extend the line to make it a reasonable approximation of an infinitely long line
    const qreal len = line.length();
    const QLineF l = QLineF(line.pointAt(1.0 / len * -1e10), line.pointAt(1.0 / len * 1e10));

    // Check if the line intersects the curve at all
    if (chord().intersect(l, 0) != QLineF::BoundedIntersection)
        return 1;

    qreal upperT = 1;
    qreal lowerT = 0;

    KCubicBezier c = *this;

    while (1)
    {
        const qreal t = lowerT + (upperT - lowerT) / 2;
        if (c.length() < error)
            return t;

        KCubicBezier left, right;
        c.split(&left, &right);

        // If the line intersects the left section
        if (left.chord().intersect(l, 0) == QLineF::BoundedIntersection) {
            upperT = t;
            c = left;
        } else {
            lowerT = t;
            c = right;
        }
    }
}



// ----------------------------------------------------------------------------



BorderArcStroker::BorderArcStroker()
{
}

BorderArcStroker::~BorderArcStroker()
{
}

QPainterPath BorderArcStroker::createStroke(qreal *nextOffset) const
{
    const QRectF outerRect = rect;
    const QRectF innerRect = rect.adjusted(hlw, vlw, -hlw, -vlw);

    // Avoid hitting the assert below if the radius is smaller than the border width
    if (!outerRect.isValid() || !innerRect.isValid())
        return QPainterPath();

    QPainterPath innerPath, outerPath;
    innerPath.arcMoveTo(innerRect, angle);
    outerPath.arcMoveTo(outerRect, angle);
    innerPath.arcTo(innerRect, angle, sweepLength);
    outerPath.arcTo(outerRect, angle, sweepLength);

    Q_ASSERT(qAbs(sweepLength) <= 90);
    Q_ASSERT(innerPath.elementCount() == 4 && outerPath.elementCount() == 4);

    const KCubicBezier inner(innerPath.elementAt(0), innerPath.elementAt(1), innerPath.elementAt(2), innerPath.elementAt(3));
    const KCubicBezier outer(outerPath.elementAt(0), outerPath.elementAt(1), outerPath.elementAt(2), outerPath.elementAt(3));

    qreal a = std::fmod(angle, qreal(360.0));
    if (a < 0)
        a += 360.0;

    // Figure out which border we're starting from
    qreal initialWidth;
    if ((a >= 0 && a < 90) || (a >= 180 && a < 270))
        initialWidth = sweepLength > 0 ? hlw : vlw;
    else
        initialWidth = sweepLength > 0 ? vlw : hlw;

    const qreal finalWidth = qMax(qreal(0.1), QLineF(outer.p3(), inner.p3()).length());
    const qreal dashAspect  = (pattern[0] / initialWidth);
    const qreal spaceAspect = (pattern[1] / initialWidth);
    const qreal length = inner.length();

    QPainterPath path;
    qreal innerStart = 0, outerStart = 0;
    qreal pos = 0;
    bool dash = true;

    qreal offset = fmod(patternOffset, pattern[0] + pattern[1]);
    if (offset < 0) {
        offset += pattern[0] + pattern[1];
    }

    if (offset > 0) {
        if (offset >= pattern[0]) {
            offset -= pattern[0];
            dash = false;
        } else
            dash = true;
        pos = -offset;
    }

    while (innerStart < 1) {
        const qreal lineWidth = QLineF(outer.pointAt(outerStart), inner.pointAt(innerStart)).length();
        const qreal length = lineWidth * (dash ? dashAspect : spaceAspect);
        pos += length;

        if (pos > 0) {
            const qreal innerEnd = inner.tAtLength(pos);
            const QLineF normal = inner.normalVectorAt(innerEnd);
            const qreal outerEnd = outer.tAtIntersection(normal);

            if (dash) {
                // The outer and inner curves of the dash
                const KCubicBezier a = outer.section(outerStart, outerEnd);
                const KCubicBezier b = inner.section(innerStart, innerEnd);

                // Add the dash to the path
                path.moveTo(a.p0());
                path.cubicTo(a.p1(), a.p2(), a.p3());
                path.lineTo(b.p3());
                path.cubicTo(b.p2(), b.p1(), b.p0());
                path.closeSubpath();
            }

            innerStart = innerEnd;
            outerStart = outerEnd;
        }

        dash = !dash;
    }

    if (nextOffset) {
        const qreal remainder = pos - length;

        if (dash)
            *nextOffset = -remainder;
        else
            *nextOffset = (pattern[0] / initialWidth) * finalWidth - remainder;
   }

    return path;
}

} // namespace khtml

