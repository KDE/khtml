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


#include "ksslkeygen.h"
#include "ksslkeygen_p.h"
#include "ui_keygenwizard.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kopenssl.h>
#include <kwallet.h>

#include <QProgressDialog>
#include <QStandardPaths>
#include <qplatformdefs.h>
#include <qtemporaryfile.h>

#include <assert.h>

KSSLKeyGenWizardPage2::KSSLKeyGenWizardPage2(QWidget* parent)
    : QWizardPage(parent)
{
    ui2 = new Ui_KGWizardPage2;
    ui2->setupUi(this);
    connect(ui2->_password1, SIGNAL(textChanged(QString)), this, SLOT(slotPassChanged()));
    connect(ui2->_password2, SIGNAL(textChanged(QString)), this, SLOT(slotPassChanged()));
}

bool KSSLKeyGenWizardPage2::isComplete() const
{
    return ui2->_password1->text() == ui2->_password2->text() && ui2->_password1->text().length() >= 4;
}

void KSSLKeyGenWizardPage2::slotPassChanged()
{
    emit completeChanged(); // well maybe it hasn't changed, but it might have; QWizard calls isComplete() to find out
}

QString KSSLKeyGenWizardPage2::password() const
{
    Q_ASSERT(isComplete());
    return ui2->_password1->text();
}

////

class KSSLKeyGenPrivate
{
public:
    KSSLKeyGenPrivate()
        : idx(-1)
    {
    }
    int idx;
    Ui_KGWizardPage1 *ui1;
    KSSLKeyGenWizardPage2* page2;
};

KSSLKeyGen::KSSLKeyGen(QWidget *parent)
    : QWizard(parent), d(new KSSLKeyGenPrivate)
{
#if KSSL_HAVE_SSL

    QWizardPage* page1 = new QWizardPage(this);
    page1->setTitle(i18n("KDE Certificate Request"));
    d->ui1 = new Ui_KGWizardPage1;
    d->ui1->setupUi(page1);
    addPage(page1);
    //setHelpEnabled(page1, false);

    d->page2 = new KSSLKeyGenWizardPage2(this);
    d->page2->setTitle(i18n("KDE Certificate Request - Password"));
    addPage(d->page2);
#else
    // tell him he doesn't have SSL
#endif
}


KSSLKeyGen::~KSSLKeyGen() {
    delete d->ui1;
    delete d;
}

bool KSSLKeyGen::validateCurrentPage() {
    if (currentPage() != d->page2)
        return true;

    assert(d->idx >= 0 && d->idx <= 3);   // for now

    // Generate the CSR
    int bits;
    switch (d->idx) {
    case 0:
        bits = 2048;
        break;
    case 1:
        bits = 1024;
        break;
    case 2:
        bits = 768;
        break;
    case 3:
        bits = 512;
        break;
    default:
        KMessageBox::sorry(this, i18n("Unsupported key size."), i18n("KDE SSL Information"));
        return false;
    }

    QProgressDialog *kpd = new QProgressDialog(this);
    kpd->setObjectName("progress dialog");
    kpd->setWindowTitle(i18n("KDE"));
    kpd->setLabelText(i18n("Please wait while the encryption keys are generated..."));
    kpd->setRange(0, 100);
    kpd->setValue(0);
    kpd->show();
    // FIXME - progress dialog won't show this way

    int rc = generateCSR("This CSR" /*FIXME */, d->page2->password(), bits, 0x10001 /* This is the traditional exponent used */);
    if (rc != 0) // error
        return false;

    kpd->setValue(100);

#if 0 // TODO: implement
    if (rc == 0 && KWallet::Wallet::isEnabled()) {
        rc = KMessageBox::questionYesNo(this, i18n("Do you wish to store the passphrase in your wallet file?"), QString(), KGuiItem(i18n("Store")), KGuiItem(i18n("Do Not Store")));
        if (rc == KMessageBox::Yes) {
            KWallet::Wallet *w = KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), winId());
            if (w) {
                // FIXME: store passphrase in wallet
                delete w;
            }
        }
    }
#endif

    kpd->deleteLater();
    return true;
}


int KSSLKeyGen::generateCSR(const QString& name, const QString& pass, int bits, int e) {
#if KSSL_HAVE_SSL
	KOSSL *kossl = KOSSL::self();
	int rc;

	X509_REQ *req = kossl->X509_REQ_new();
	if (!req) {
		return -2;
	}

	EVP_PKEY *pkey = kossl->EVP_PKEY_new();
	if (!pkey) {
		kossl->X509_REQ_free(req);
		return -4;
	}

	RSA *rsakey = kossl->RSA_generate_key(bits, e, NULL, NULL);
	if (!rsakey) {
		kossl->X509_REQ_free(req);
		kossl->EVP_PKEY_free(pkey);
		return -3;
	}

	rc = kossl->EVP_PKEY_assign(pkey, EVP_PKEY_RSA, (char *)rsakey);

	rc = kossl->X509_REQ_set_pubkey(req, pkey);

	// Set the subject
	X509_NAME *n = kossl->X509_NAME_new();

	kossl->X509_NAME_add_entry_by_txt(n, (char*)LN_countryName, MBSTRING_UTF8, (unsigned char*)name.toLocal8Bit().data(), -1, -1, 0);
	kossl->X509_NAME_add_entry_by_txt(n, (char*)LN_organizationName, MBSTRING_UTF8, (unsigned char*)name.toLocal8Bit().data(), -1, -1, 0);
	kossl->X509_NAME_add_entry_by_txt(n, (char*)LN_organizationalUnitName, MBSTRING_UTF8, (unsigned char*)name.toLocal8Bit().data(), -1, -1, 0);
	kossl->X509_NAME_add_entry_by_txt(n, (char*)LN_localityName, MBSTRING_UTF8, (unsigned char*)name.toLocal8Bit().data(), -1, -1, 0);
	kossl->X509_NAME_add_entry_by_txt(n, (char*)LN_stateOrProvinceName, MBSTRING_UTF8, (unsigned char*)name.toLocal8Bit().data(), -1, -1, 0);
	kossl->X509_NAME_add_entry_by_txt(n, (char*)LN_commonName, MBSTRING_UTF8, (unsigned char*)name.toLocal8Bit().data(), -1, -1, 0);
	kossl->X509_NAME_add_entry_by_txt(n, (char*)LN_pkcs9_emailAddress, MBSTRING_UTF8, (unsigned char*)name.toLocal8Bit().data(), -1, -1, 0);

	rc = kossl->X509_REQ_set_subject_name(req, n);


	rc = kossl->X509_REQ_sign(req, pkey, kossl->EVP_md5());

	// We write it to the database and then the caller can obtain it
	// back from there.  Yes it's inefficient, but it doesn't happen
	// often and this way things are uniform.

	const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kssl";
	QTemporaryFile csrFile(path + "csr_XXXXXX.der");
	csrFile.setAutoRemove(false);

	if (!csrFile.open()) {
		kossl->X509_REQ_free(req);
		kossl->EVP_PKEY_free(pkey);
		return -5;
	}

	QTemporaryFile p8File(path + "pkey_XXXXXX.p8");
	p8File.setAutoRemove(false);

	if (!p8File.open()) {
		kossl->X509_REQ_free(req);
		kossl->EVP_PKEY_free(pkey);
		return -5;
	}

	FILE *csr_fs = QT_FOPEN(QFile::encodeName(csrFile.fileName()), "r+");
	FILE *p8_fs = QT_FOPEN(QFile::encodeName(p8File.fileName()), "r+");

	kossl->i2d_X509_REQ_fp(csr_fs, req);

	kossl->i2d_PKCS8PrivateKey_fp(p8_fs, pkey,
			kossl->EVP_bf_cbc(), pass.toLocal8Bit().data(),
			pass.length(), 0L, 0L);

	// FIXME Write kconfig entry to store the filenames under the md5 hash

	kossl->X509_REQ_free(req);
	kossl->EVP_PKEY_free(pkey);

	fclose(csr_fs);
	fclose(p8_fs);

	return 0;
#else
	return -1;
#endif
}


QStringList KSSLKeyGen::supportedKeySizes() {
    QStringList x;

#if KSSL_HAVE_SSL
    x	<< i18n("2048 (High Grade)")
        << i18n("1024 (Medium Grade)")
        << i18n("768  (Low Grade)")
        << i18n("512  (Low Grade)");
#else
    x	<< i18n("No SSL support.");
#endif

    return x;
}

void KSSLKeyGen::setKeySize(int idx)
{
     d->idx = idx;
}

#include "moc_ksslkeygen.cpp"

#include "moc_ksslkeygen_p.cpp"
