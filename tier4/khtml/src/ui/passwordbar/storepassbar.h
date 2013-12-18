/* This file is part of the KDE project
 *
 * Copyright (C) 2009 Eduardo Robles Elvira <edulix at gmail dot com>
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
#ifndef __storepassbar_h__
#define __storepassbar_h__

#include "khtmlviewbarwidget.h"
#include "ui_storepassbar_base.h"

#include <QObject>
#include <QMap>

class KHTMLPart;

class StorePassBar : public KHTMLViewBarWidget, private Ui::StorePassBarBase
{
  Q_OBJECT
public:
  StorePassBar( QWidget *parent = 0 );
    
  void setHost(const QString& host);
    
Q_SIGNALS:
  void storeClicked();
  void neverForThisSiteClicked();
  void doNotStoreClicked();
};

class StorePass : public QObject
{
  Q_OBJECT
public:
  StorePass( KHTMLPart *part );
  ~StorePass();
    
  void saveLoginInformation(const QString& host, const QString& key,
    const QMap<QString, QString>& walletMap);
  void removeBar();

private Q_SLOTS:
  void slotStoreClicked();
  void slotNeverForThisSiteClicked();
  void slotDoNotStoreClicked();
   
private:
  KHTMLPart *m_part;
  
  StorePassBar m_storePassBar;
  QString m_host;
  QString m_key;
  QMap<QString, QString> m_walletMap;
};
#endif
