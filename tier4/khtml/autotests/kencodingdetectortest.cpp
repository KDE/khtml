/* This file is part of the KDE libraries
    Copyright (c) 2009 Germain Garand <germain@ebooksfrance.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kencodingdetectortest.h"

#include <QTest>

#include <kencodingdetector.h>


static const char data1[] = "this should decode correctly";
static const char data2[] = "this is an invalid utf-8 byte: \xBF and another one: \xBE";

static KEncodingDetector* ed = 0;

void KEncodingDetectorTest::initTestCase()
{
    ed = new KEncodingDetector();
}

void KEncodingDetectorTest::testSetEncoding()
{
    QCOMPARE(ed->setEncoding( "iso8859-1", KEncodingDetector::UserChosenEncoding ), true);
    QCOMPARE(ed->setEncoding( "utf-8", KEncodingDetector::UserChosenEncoding ), true);
}

void KEncodingDetectorTest::testDecode()
{
    QString s = ed->decode( data1, sizeof(data1)-1);
    QCOMPARE(ed->decodedInvalidCharacters(), false);
    QString s2 = ed->decode( data2, sizeof(data2)-1);
    QCOMPARE(ed->decodedInvalidCharacters(), true);
    QCOMPARE(s, QString::fromLatin1(data1));

    ed->resetDecoder();
    QVERIFY(!ed->decodedInvalidCharacters());

    // set to automatic detection
    ed->setEncoding( "", KEncodingDetector::DefaultEncoding );

    // decodeWithBuffering should just accumulate the buffer here,
    // waiting for some HTML/XML encoding tags
    s = ed->decodeWithBuffering(data2, sizeof data2 -1);

    // shouldn't even decode anything yet, so:
    QCOMPARE(s.isEmpty(), true);
    QCOMPARE(ed->decodedInvalidCharacters(), false);

    // force encoding, as the high bytes must have switched the encoding
    // to anything *but* utf-8
    QCOMPARE(QString::fromLatin1("utf-8").startsWith(QString::fromLatin1(ed->encoding()), Qt::CaseInsensitive), false);
    ed->setEncoding( "utf-8", KEncodingDetector::UserChosenEncoding );
    QCOMPARE(QString::fromLatin1("utf-8").startsWith(QString::fromLatin1(ed->encoding()), Qt::CaseInsensitive), true);

    // force decoding now
    s = ed->flush();
    QCOMPARE(s.isEmpty(), false);
    QCOMPARE(ed->decodedInvalidCharacters(), true);

    // now check that resetDecoder() empties the buffer
    s2 = ed->decodeWithBuffering(data1, sizeof data1 -1);
    ed->resetDecoder();
    s2 = ed->flush();
    QCOMPARE(s2.isEmpty(), true);

    // check that buffered decoding with non-overridable specified codec decodes right away
    ed->setEncoding( "utf-8", KEncodingDetector::EncodingFromHTTPHeader );
    s = ed->decodeWithBuffering(data2, sizeof data2 -1);

    QCOMPARE( s.isEmpty(), false );
    QCOMPARE( ed->decodedInvalidCharacters(), true );
}

QTEST_MAIN(KEncodingDetectorTest)
