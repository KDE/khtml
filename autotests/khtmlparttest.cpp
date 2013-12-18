/* This file is part of the KDE project
 *
 * Copyright (C) 2007 Germain Garand <germain@ebooksfrance.org>
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

#define QT_GUI_LIB 1
#define QT_WIDGETS_LIB 1
// TODO: include <QTestWidgets> with Qt 5 instead of the above defines

#include <khtml_part.h>
#include <QtTest/QtTest>
#include <khtmlview.h>
#include "khtmlparttest.h"
#include <csignal>
#include <cstdlib>

QTEST_MAIN(KHTMLPartTest)

void __abort(int) {
    std::signal(SIGABRT, SIG_DFL);
    std::signal(SIGSEGV, SIG_DFL);
    QVERIFY( false );
} 

void KHTMLPartTest::initTestCase()
{
    std::signal(SIGABRT, &__abort);
    std::signal(SIGSEGV, &__abort);
}

class MyKHTMLPart : public KHTMLPart
{
  public:
    MyKHTMLPart() : KHTMLPart( new KHTMLView(this, 0) ) {}
};

void KHTMLPartTest::testConstructKHTMLViewFromInitList()
{
    // test that KHTMLView can be built from a derived KHTMLPart's initialization list
    KHTMLPart* aPart = new MyKHTMLPart();
    QVERIFY(true);
    QVERIFY(aPart->view()->part() == aPart);
    delete aPart;
}

void KHTMLPartTest::testConstructKHTMLViewBeforePart()
{
    // test that a KHTMLView can be constructed before a KHTMLPart
    KHTMLView* view = new KHTMLView(0, 0);
    KHTMLPart* part = new KHTMLPart(view);
    QVERIFY(true);
    QVERIFY(view->part() == part);
    delete part;
}
