/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2007 Harri Porten (porten@kde.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "khtmladaptorpart.h"
#include <kjs/object.h>
#include <QLabel>
#include <klocalizedstring.h>
AdaptorView::AdaptorView(QWidget* wparent, QObject* parent,
                         const QStringList& /*args*/)
    : KParts::ReadOnlyPart(parent)
{
    QLabel *placeHolder = new QLabel(i18n("Inactive"), wparent);
    placeHolder->setAlignment(Qt::AlignCenter);
    placeHolder->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    KParts::Part::setWidget(placeHolder);
}

bool AdaptorView::openFile()
{
    return true;
}

void AdaptorView::initScripting(KJS::ExecState * /*exec*/)
{
}

KJS::JSObject* AdaptorView::scriptObject()
{
    return new KJS::JSObject();
}

K_EXPORT_PLUGIN(KHTMLAdaptorPartFactory)

KHTMLAdaptorPartFactory::KHTMLAdaptorPartFactory()
{
}


QString variant2StringHelper(const QVariant &variant)
{
    return variant.toString();
}

QObject* KHTMLAdaptorPartFactory::create(const char* /*iface*/,
                                         QWidget* wparent,
                                         QObject *parent,
                                         const QVariantList &/*args*/,
                                         const QString& /*keyword*/)
{
    return new AdaptorView(wparent, parent, QStringList());
}

