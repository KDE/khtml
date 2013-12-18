/**
 * This file is part of the HTML rendering engine for KDE.
 *
 * Copyright (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *
 *           (C) Hebrew algorithm by herouth@netvision.net.il
 *                               and schlpbch@iam.unibe.ch
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

#include "rendering/enumerate.h"

#include <QtCore/QCharRef>
#include <QtCore/QList>

namespace khtml {

namespace Enumerate {

QString toRoman( int number, bool upper )
{
    if (number < 1 || number > 3999) return QString::number(number);
    QString roman;
    static const QChar ldigits[] = { 'i', 'v', 'x', 'l', 'c', 'd', 'm' };
    static const QChar udigits[] = { 'I', 'V', 'X', 'L', 'C', 'D', 'M' };
    const QChar *digits = upper ? udigits : ldigits;
    int i, d = 0;

    do
    {
        int num = number % 10;

        if ( num % 5 < 4 )
            for ( i = num % 5; i > 0; i-- )
                roman.prepend( digits[ d ] );

        if ( num >= 4 && num <= 8)
            roman.prepend( digits[ d+1 ] );

        if ( num == 9 )
            roman.prepend( digits[ d+2 ] );

        if ( num % 5 == 4 )
            roman.prepend( digits[ d ] );

        number /= 10;
        d += 2;
    }
    while ( number );

    return roman;
}

QString toGeorgian( int number )
{
    // numbers from table at http://xml-maiden.com/numbering/table.xhtml
    QString georgian;
    const QChar tenthousand = 0x10F5;
    static const QChar thousands[9] = {0x10E9, 0x10EA, 0x10EB, 0x10EC,
                                       0x10ED, 0x10EE, 0x10F4, 0x10EF, 0x10F0 };
    static const QChar hundreds[9] = {0x10E0, 0x10E1, 0x10E2, 0x10F3, 0x10E4,
                                      0x10E5, 0x10E6, 0x10E7, 0x10E8 };
    static const QChar tens[9] = {0x10D8, 0x10D9, 0x10DA, 0x10DB, 0x10DC,
                                  0x10F2, 0x10DD, 0x10DE, 0x10DF };
    static const QChar units[9] = {0x10D0, 0x10D1, 0x10D2, 0x10D3, 0x10D4,
                                   0x10D5, 0x10D6, 0x10F1, 0x10D7 };

    if (number < 1 || number > 19999) return QString::number(number);
    if (number >= 10000) {
        georgian.append(tenthousand);
        number = number - 10000;
    }
    if (number >= 1000) {
        georgian.append(thousands[number/1000-1]);
        number = number % 1000;
    }
    if (number >= 100) {
        georgian.append(hundreds[number/100-1]);
        number = number % 100;
    }
    if (number >= 10) {
        georgian.append(tens[number/10-1]);
        number = number % 10;
    }
    if (number >= 1)  {
        georgian.append(units[number-1]);
    }

    return georgian;
}

QString toArmenian( int number )
{
    QString armenian;
    int thousands = 0x57b;
    int hundreds = 0x572;
    int tens = 0x569;
    int units = 0x560;

    // The standard defines values upto 9999, but 7000 is odd
    if (number < 1 || number > 6999) return QString::number(number);
    if (number >= 1000) {
        armenian.append(QChar(thousands+number/1000));
        number = number % 1000;
    }
    if (number >= 100) {
        armenian.append(QChar(hundreds+number/100));
        number = number % 100;
    }
    if (number >= 10) {
        armenian.append(QChar(tens+number/10));
        number = number % 10;
    }
    if (number >= 1)  {
        armenian.append(QChar(units+number));
    }

    return armenian;
}

QString toHebrew( int number ) {
    static const QChar tenDigit[] = {1497, 1499, 1500, 1502, 1504, 1505, 1506, 1508, 1510};

    QString letter;
    if (number < 1) return QString::number(number);
    if (number>999) {
	letter = toHebrew(number/1000) + QLatin1Char('\'');
	number = number%1000;
    }

    int hunderts = (number/400);
    if (hunderts > 0) {
	for(int i=0; i<hunderts; i++) {
	    letter += QChar(1511 + 3);
	}
    }
    number = number % 400;
    if ((number / 100) != 0) {
        letter += QChar (1511 + (number / 100) -1);
    }
    number = number % 100;
    int tens = number/10;
    if (tens > 0 && !(number == 15 || number == 16)) {
	letter += tenDigit[tens-1];
    }
    if (number == 15 || number == 16) { // special because of religious
	letter += QChar(1487 + 9);       // reasons
    	letter += QChar(1487 + number - 9);
    } else {
        number = number % 10;
        if (number != 0) {
            letter += QChar (1487 + number);
        }
    }
    return letter;
}

static inline QString toLatin( int number, int base ) {
    if (number < 1) return QString::number(number);
    QList<QChar> letters;
    while(number > 0) {
        number--; // number 0 is letter a
        QChar letter = (QChar) (base + (number % 26));
        letters.prepend(letter);
        number /= 26;
    }
    QString str;
    str.reserve(letters.size());
    int i=0;
    while(!letters.isEmpty()) {
        str[i++] = letters.front();
        letters.pop_front();
    }
    return str;
}

QString toLowerLatin( int number ) {
    return toLatin( number, 'a' );
}

QString toUpperLatin( int number ) {
    return toLatin( number, 'A' );
}

static inline QString toAlphabetic( int number, int base, const QChar alphabet[] ) {
    if (number < 1) return QString::number(number);
    QList<QChar> letters;
    while(number > 0) {
        number--; // number 0 is letter 1
        QChar letter = alphabet[number % base];
        letters.prepend(letter);
        number /= base;
    }
    QString str;
    str.reserve(letters.size());
    int i=0;
    while(!letters.isEmpty()) {
        str[i++] = letters.front();
        letters.pop_front();
    }
    return str;
}

QString toHiragana( int number ) {
    static const QChar hiragana[48] = {0x3042, 0x3044, 0x3046, 0x3048, 0x304A, 0x304B, 0x304D,
                                 0x304F, 0x3051, 0x3053, 0x3055, 0x3057, 0x3059, 0x305B, 0x305D,
                                 0x305F, 0x3061, 0x3064, 0x3066, 0x3068, 0x306A, 0x306B,
                                 0x306C, 0x306D, 0x306E, 0x306F, 0x3072, 0x3075, 0x3078,
                                 0x307B, 0x307E, 0x307F, 0x3080, 0x3081, 0x3082, 0x3084, 0x3086,
                                 0x3088, 0x3089, 0x308A, 0x308B, 0x308C, 0x308D, 0x308F,
                                 0x3090, 0x3091, 0x9092, 0x3093};
    return toAlphabetic( number, 48, hiragana );
}

QString toHiraganaIroha( int number ) {
    static const QChar hiragana[47] = {0x3044, 0x308D, 0x306F, 0x306B, 0x307B, 0x3078, 0x3068,
                                 0x3061, 0x308A, 0x306C, 0x308B, 0x3092, 0x308F, 0x304B,
                                 0x3088, 0x305F, 0x308C, 0x305D, 0x3064, 0x306D, 0x306A,
                                 0x3089, 0x3080, 0x3046, 0x3090, 0x306E, 0x304A, 0x304F, 0x3084,
                                 0x307E, 0x3051, 0x3075, 0x3053, 0x3048, 0x3066, 0x3042, 0x3055,
                                 0x304D, 0x3086, 0x3081, 0x307F, 0x3057, 0x3091, 0x3072, 0x3082,
                                 0x305B, 0x3059 };
    return toAlphabetic( number, 47, hiragana );
}

QString toKatakana( int number ) {
    static const QChar katakana[48] = {0x30A2, 0x30A4, 0x30A6, 0x30A8, 0x30AA, 0x30AB, 0x30AD,
                                 0x30AF, 0x30B1, 0x30B3, 0x30B5, 0x30B7, 0x30B9, 0x30BB,
                                 0x30BD, 0x30BF, 0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CA,
                                 0x30CB, 0x30CC, 0x30CD, 0x30CE, 0x30CF, 0x30D2, 0x30D5,
                                 0x30D8, 0x30DB, 0x30DE, 0x30DF, 0x30E0, 0x30E1, 0x30E2,
                                 0x30E4, 0x30E6, 0x30E8, 0x30E9, 0x30EA, 0x30EB, 0x30EC,
                                 0x30ED, 0x30EF, 0x30F0, 0x30F1, 0x90F2, 0x30F3};
    return toAlphabetic( number, 48, katakana );
}

QString toKatakanaIroha( int number ) {
    static const QChar katakana[47] = {0x30A4, 0x30ED, 0x30CF, 0x30CB, 0x30DB, 0x30D8, 0x30C8,
                                 0x30C1, 0x30EA, 0x30CC, 0x30EB, 0x30F2, 0x30EF, 0x30AB,
                                 0x30E8, 0x30BF, 0x30EC, 0x30ED, 0x30C4, 0x30CD, 0x30CA,
                                 0x30E9, 0x30E0, 0x30A6, 0x30F0, 0x30CE, 0x30AA, 0x30AF,
                                 0x30E4, 0x30DE, 0x30B1, 0x30D5, 0x30B3, 0x30A8, 0x30C6,
                                 0x30A2, 0x30B5, 0x30AD, 0x30E6, 0x30E1, 0x30DF, 0x30B7,
                                 0x30F1, 0x30D2, 0x30E2, 0x30BB, 0x90B9};
    return toAlphabetic( number, 47, katakana );
}

QString toLowerGreek( int number ) {
    static const QChar greek[24] = { 0x3B1, 0x3B2, 0x3B3, 0x3B4, 0x3B5, 0x3B6,
                                     0x3B7, 0x3B8, 0x3B9, 0x3BA, 0x3BB, 0x3BC,
                                     0x3BD, 0x3BE, 0x3BF, 0x3C0, 0x3C1, 0x3C3,
                                     0x3C4, 0x3C5, 0x3C6, 0x3C7, 0x3C8, 0x3C9};

    return toAlphabetic( number, 24, greek );
}

QString toUpperGreek( int number ) {
    // The standard claims to be base 24, but only lists 19 letters.
    static const QChar greek[19] = { 0x391, 0x392, 0x393, 0x394, 0x395, 0x396, 0x397, 0x398,
                               0x399, 0x39A, 0x39B, 0x39C, 0x39D, 0x39E, 0x39F,
                               0x3A0, 0x3A1, 0x3A3, 0x3A9};

    return toAlphabetic( number, 19, greek );
}

static inline QString toNumeric( int number, int base ) {
    QString letter = QString::number(number);
    for(int i = 0; i < letter.length(); i++) {
        if (letter[i].isDigit())
        letter[i] = QChar(letter[i].digitValue()+base);
    }
    return letter;
}

QString toArabicIndic( int number ) {
    return toNumeric(number, 0x660);
}

QString toPersianUrdu( int number ) {
    return toNumeric(number, 0x6F0);
}

QString toLao( int number ) {
    return toNumeric(number, 0xED0);
}

QString toThai( int number ) {
    return toNumeric(number, 0xE50);
}

QString toTibetan( int number ) {
    return toNumeric(number, 0xF20);
}

static inline QString toIdeographic(int number, const QChar digits[], const QChar digitmarkers[]) {
    if (number < 0 || number > 9999) return QString::number(number);

    QString grp = QString::number(number);

    // ### Append group markers to handle numbers > 9999

    QString str;

    // special case
    if (number < 20 && number >= 10) {
        str.append(digitmarkers[0]);
        str.append(digits[grp[1].digitValue()]);
        return str;
    }

    int len = grp.length();
    bool collapseZero = false;
    for (int i = 0; i < len ; i++) {
        int digit = grp[i].digitValue();
        // Add digit markers to digits > 0
        if ((len-i-1) > 0 && digit > 0)
            str.append(digitmarkers[(len-i-2)]);
        // Add digit, but collapse consecutive zeros
        if (!collapseZero || digit > 0) {
            str.append(digits[digit]);

            if (digit == 0)
                collapseZero = true;
            else
                collapseZero = false;
        }
    }
    return str;
}

QString toTradChineseFormal( int number ) {
//     static const QChar groupMarkers[3] = {0x4e07, 0x4ebf, 0x5146};
    static const QChar digitMarkers[3] = {0x4e07, 0x4ebf, 0x5146};
    static const QChar digits[10] = {0x96f6, 0x4e00,
                                     0x4ebc, 0x4e09,
                                     0x56db, 0x4e94,
                                     0x516d, 0x4e03,
                                     0x516b, 0x4e5d};
    return toIdeographic(number, digits, digitMarkers);
}

QString toTradChineseInformal( int number ) {
//     static const QChar groupMarkers[3] = {0x842c, 0x5104, 0x5146};
    static const QChar digitMarkers[3] = {0x842c, 0x5104, 0x5146};
    static const QChar digits[10] = {0x96f6, 0x4e00,
                                     0x4ebc, 0x4e09,
                                     0x56db, 0x4e94,
                                     0x516d, 0x4e03,
                                     0x516b, 0x4e5d};
    return toIdeographic(number, digits, digitMarkers);
}

QString toSimpChineseFormal( int number ) {
//     static const QChar groupMarkers[3] = {0x4e07, 0x5104, 0x5146};
    static const QChar digitMarkers[3] = {0x4e07, 0x4ebf, 0x5146};
    static const QChar digits[10] = {0x96f6, 0x58f9,
                                     0x8cb3, 0x53c3,
                                     0x8086, 0x4f0d,
                                     0x9678, 0x67d2,
                                     0x634c, 0x7396};
    return toIdeographic(number, digits, digitMarkers);
}

QString toSimpChineseInformal( int number ) {
//     static const QChar groupMarkers[3] = {0x842c, 0x5104, 0x5146};
    static const QChar digitMarkers[3] = {0x842c, 0x5104, 0x5146};
    static const QChar digits[10] = {0x96f6, 0x58f9,
                                     0x8cb3, 0x53c3,
                                     0x8086, 0x4f0d,
                                     0x9678, 0x67d2,
                                     0x634c, 0x7396};
    return toIdeographic(number, digits, digitMarkers);
}

QString toJapaneseFormal( int number ) {
//     static const QChar groupMarkers[3] = {0x4e07, 0x5104, 0x5146};
    static const QChar digitMarkers[3] = {0x62fe, 0x4f70, 0x4edf};
    static const QChar digits[10] = {0x96f6, 0x58f9,
                                     0x8cb3, 0x53c3,
                                     0x8086, 0x4f0d,
                                     0x9678, 0x67d2,
                                     0x634c, 0x7396};
    return toIdeographic(number, digits, digitMarkers);
}

QString toJapaneseInformal( int number ) {
//     static const QChar groupMarkers[3] = {0x842c, 0x5104, 0x5146};
    static const QChar digitMarkers[3] = {0x842c, 0x5104, 0x5146};
    static const QChar digits[10] = {0x96f6, 0x58f9,
                                     0x8d30, 0x53c1,
                                     0x8086, 0x4f0d,
                                     0x9646, 0x67d2,
                                     0x634c, 0x7396};
    return toIdeographic(number, digits, digitMarkers);
}

}} // namespace
