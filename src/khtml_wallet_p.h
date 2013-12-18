/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999-2001 Lars Knoll <knoll@kde.org>
 *                     1999-2001 Antti Koivisto <koivisto@kde.org>
 *                     2000-2001 Simon Hausmann <hausmann@kde.org>
 *                     2000-2001 Dirk Mueller <mueller@kde.org>
 *                     2000 Stefan Schimanski <1Stein@gmx.de>
 *                     2001-2003 George Staikos <staikos@kde.org>
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
#if !defined khtml_wallet_p_h && !defined KHTML_NO_WALLET
#define khtml_wallet_p_h

#include <kcursor.h>
#include <kxmlguifactory.h>
#include <kparts/partmanager.h>
#include <kparts/statusbarextension.h>
#include <kparts/browserextension.h>
#include <kwallet.h>

#include <QtCore/QPointer>
#include <QtCore/QMap>
#include <QtCore/QTimer>

class KHTMLWalletQueue : public QObject
{
  Q_OBJECT
  public:
    KHTMLWalletQueue(QObject *parent) : QObject(parent) {
      wallet = 0L;
    }

    virtual ~KHTMLWalletQueue() {
      delete wallet;
      wallet = 0L;
    }

    KWallet::Wallet *wallet;
    typedef QPair<DOM::HTMLFormElementImpl*, QPointer<DOM::DocumentImpl> > Caller;
    typedef QList<Caller> CallerList;
    CallerList callers;
    QList<QPair<QString, QMap<QString, QString> > > savers;

  Q_SIGNALS:
    void walletOpened(KWallet::Wallet*);

  public Q_SLOTS:
    void walletOpened(bool success) {
      if (!success) {
        delete wallet;
        wallet = 0L;
      }
      emit walletOpened(wallet);
      if (wallet) {
        if (!wallet->hasFolder(KWallet::Wallet::FormDataFolder())) {
          wallet->createFolder(KWallet::Wallet::FormDataFolder());
        }
        for (CallerList::Iterator i = callers.begin(); i != callers.end(); ++i) {
          if ((*i).first && (*i).second) {
            (*i).first->walletOpened(wallet);
          }
        }
        wallet->setFolder(KWallet::Wallet::FormDataFolder());
        for (QList<QPair<QString, QMap<QString, QString> > >::Iterator i = savers.begin(); i != savers.end(); ++i) {
          wallet->writeMap((*i).first, (*i).second);
        }
      }
      callers.clear();
      savers.clear();
      wallet = 0L; // gave it away
    }
};

#endif
