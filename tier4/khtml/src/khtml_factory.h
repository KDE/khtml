/* This file is part of the KDE project
 *
 * Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2007 David Faure <faure@kde.org>
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
#ifndef __khtml_factory_h__
#define __khtml_factory_h__

#include <khtml_export.h>

#include <kpluginfactory.h>

class KHTMLFactory : public KPluginFactory
{
    Q_OBJECT
public:
    KHTMLFactory();
    virtual ~KHTMLFactory();

    virtual QObject *create(const char* iface,
                            QWidget *parentWidget,
                            QObject *parent,
                            const QVariantList &args,
                            const QString &keyword);
};

#endif
