/*
   This file is part of the KDE libraries
   Copyright (c) 2002 Waldo Bastian <bastian@kde.org>
   Copyright 2009 David Faure <faure@kde.org>

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

#ifndef HTTPFILTERGZIP_P_H
#define HTTPFILTERGZIP_P_H

#include <QObject>
class KFilterBase;

class HTTPFilterGZip : public QObject
{
    Q_OBJECT
public:
    HTTPFilterGZip();
    ~HTTPFilterGZip();

Q_SIGNALS:
    void output(const QByteArray &d);
    void error(const QString &);

public Q_SLOTS:
    void slotInput(const QByteArray &d);

private:
    bool m_firstData;
    bool m_finished;
    KFilterBase* m_gzipFilter;
};

#endif
