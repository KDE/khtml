/* This file is part of the KDE project
 *
 * Copyright (C) 2001 George Staikos <staikos@kde.org>
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


#ifndef _KSSLKEYGEN_H
#define _KSSLKEYGEN_H

#include <khtml_export.h>

#include <QtCore/QStringList>
#include <QWizard>


class KSSLKeyGenPrivate;

/**
 * KDE Key Generation dialog
 *
 * This is used to display a key generation dialog for cases such as the
 * html \<keygen\> tag.  It also does the certificate signing request generation.
 *
 * @author George Staikos <staikos@kde.org>
 * @see KSSL, KSSLCertificate, KSSLPKCS12
 * @short KDE Key Generation Dialog
 */
class KHTML_EXPORT KSSLKeyGen : public QWizard {
    Q_OBJECT
public:
    /**
     *  Construct a keygen dialog.
     *  @param parent the parent widget
     */
    explicit KSSLKeyGen(QWidget *parent=0L);

    /**
     *  Destroy this dialog.
     */
    virtual ~KSSLKeyGen();

    /**
     *  List the supported key sizes.
     *  @return the supported key sizes
     */
    static QStringList supportedKeySizes();

    /**
     *  Generate the certificate signing request.
     *  @param name the name for the certificate
     *  @param pass the password for the request
     *  @param bits the bitsize for the key
     *  @param e the value of the "e" parameter in RSA
     *  @return 0 on success, non-zero on error
     */
    int generateCSR(const QString& name, const QString& pass, int bits, int e = 0x10001);

    /**
     *  Set the key size.
     *  @param idx an index into supportedKeySizes()
     */
    void setKeySize(int idx);

private:
    /*reimp*/ bool validateCurrentPage();

private:
    KSSLKeyGenPrivate * const d;
};

#endif

