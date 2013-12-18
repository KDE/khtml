/* This file is part of the KDE project
 *
 * Copyright (C) 2001 George Staikos <staikos@kde.org>
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
#ifndef KSSLKEYGEN_P_H
#define KSSLKEYGEN_P_H

#include <QWizardPage>
#include "ui_keygenwizard2.h"

class KSSLKeyGenWizardPage2 : public QWizardPage
{
    Q_OBJECT
public:
    KSSLKeyGenWizardPage2(QWidget* parent);
    ~KSSLKeyGenWizardPage2() { delete ui2; }
    /*reimp*/ bool isComplete() const;
    QString password() const;

private Q_SLOTS:
    void slotPassChanged();

private:
    Ui_KGWizardPage2 *ui2;
};

#endif /* KSSLKEYGEN_P_H */
