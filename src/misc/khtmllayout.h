/*
    This file is part of the KDE libraries

    Copyright (C) 1999 Lars Knoll (knoll@kde.org)
    Copyright (C) 2006, 2008 Apple Inc. All rights reserved.

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

    This widget holds some useful definitions needed for layouting Elements
*/
#ifndef HTML_LAYOUT_H
#define HTML_LAYOUT_H

#include <QVector>
#include <math.h>
#include <assert.h>

/*
 * this namespace contains definitions for various types needed for
 * layouting.
 */
namespace khtml
{

    const int UNDEFINED = -1;
    const int PERCENT_SCALE_FACTOR = 128;

    // alignment
    enum VAlign { VNone=0, Bottom, VCenter, Top, Baseline };
    enum HAlign { HDefault, Left, HCenter, Right, HNone = 0 };

    /*
     * %multiLength and %Length
     */
    enum LengthType { Auto = 0, Relative, Percent, Fixed, Static };
    struct Length
    {
	Length() : m_value(0) {}

        Length(LengthType t): m_value(t) {}

        Length(int v, LengthType t, bool q=false)
            : m_value((v * 16) | (q << 3) | t) { assert(t != Percent); }

        Length(double v, LengthType t, bool q = false)
            : m_value(static_cast<int>(v * PERCENT_SCALE_FACTOR) * 16 | (q << 3) | t) { assert(t == Percent); }

        bool operator==(const Length& o) const { return m_value == o.m_value; }
        bool operator!=(const Length& o) const { return m_value != o.m_value; }

        int value() const {
            assert(type() != Percent);
            return rawValue();
        }

        int rawValue() const { return (m_value & ~0xF) / 16; }

        double percent() const
        {
            assert(type() == Percent);
            return static_cast<double>(rawValue()) / PERCENT_SCALE_FACTOR;
        }

        LengthType type() const { return static_cast<LengthType>(m_value & 7); }

        void setValue(LengthType t, int value)
        {
            assert(t != Percent);
            setRawValue(t, value);
        }

        void setRawValue(LengthType t, int value) { m_value = value * 16 | (m_value & 0x8) | t; }

        void setValue(int value)
        {
            assert(!value || type() != Percent);
            setRawValue(value);
        }

        void setRawValue(int value) { m_value = value * 16 | (m_value & 0xF); }

        void setValue(LengthType t, double value)
        {
            assert(t == Percent);
            m_value = static_cast<int>(value * PERCENT_SCALE_FACTOR) * 16 | (m_value & 0x8) | t;
        }

        void setValue(double value)
        {
            assert(type() == Percent);
            m_value = static_cast<int>(value * PERCENT_SCALE_FACTOR) * 16 | (m_value & 0xF);
        }

	/*
	 * works only for certain types, returns UNDEFINED otherwise
	 */
	int width(int maxValue) const
	    {
		switch(type())
		{
		case Fixed:
		    return value();
		case Percent:
                    return maxValue * rawValue() / (100 * PERCENT_SCALE_FACTOR);
		case Auto:
		    return maxValue;
		default:
		    return UNDEFINED;
		}
	    }
	/*
	 * returns the minimum width value which could work...
	 */
	int minWidth(int maxValue) const
	    {
		switch(type())
		{
		case Fixed:
		    return value();
		case Percent:
		    return maxValue * rawValue() / (100 * PERCENT_SCALE_FACTOR);
		case Auto:
		default:
		    return 0;
		}
	    }

	int minWidthRounded(int maxValue) const
	    {
		switch(type())
		{
		case Fixed:
		    return value();
		case Percent:
		    return static_cast<int>(round(maxValue * percent() / 100.0));
		case Auto:
		default:
		    return 0;
		}
	    }

        bool isUndefined() const { return rawValue() == UNDEFINED; }
        bool isZero() const { return !(m_value & ~0xF); }
        bool isPositive() const { return rawValue() > 0; }
        bool isNegative() const { return rawValue() < 0; }

        bool isAuto() const { return type() == Auto; }
        bool isRelative() const { return type() == Relative; }
        bool isPercent() const { return type() == Percent; }
        bool isFixed() const { return type() == Fixed; }
        bool isQuirk() const { return (m_value >> 3) & 1; }

private:
        int m_value;
    };

}

#endif
