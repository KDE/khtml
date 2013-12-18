/**
 * This file is part of the HTML rendering engine for KDE.
 *
 * Copyright (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
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

#ifndef ENUMERATE_H
#define ENUMERATE_H

class QString;

namespace khtml {

namespace Enumerate {

// Numeric
    QString toArabicIndic( int number );
    QString toLao( int number );
    QString toPersianUrdu( int number );
    QString toThai( int number );
    QString toTibetan( int number );

// Alphabetic
    QString toLowerLatin( int number );
    QString toUpperLatin( int number );
    QString toLowerGreek( int number );
    QString toUpperGreek( int number );
    QString toHiragana( int number );
    QString toHiraganaIroha( int number );
    QString toKatakana( int number );
    QString toKatakanaIroha( int number );

// Algorithmic
    QString toRoman( int number, bool upper );
    QString toHebrew( int number );
    QString toGeorgian( int number );
    QString toArmenian( int number );

// Ideographic
    QString toJapaneseFormal   ( int number );
    QString toJapaneseInformal ( int number );
    QString toSimpChineseFormal   ( int number );
    QString toSimpChineseInformal ( int number );
    QString toTradChineseFormal   ( int number );
    QString toTradChineseInformal ( int number );

}} // namespaces

#endif
