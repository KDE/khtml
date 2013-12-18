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

#include "storepassbar.h"

#include "khtmlviewbar.h"
#include "khtml_part.h"
#include "khtmlview.h"
#include <kcolorscheme.h>
#include <kconfiggroup.h>
#include <QPalette>

StorePassBar::StorePassBar( QWidget *parent ) :
  KHTMLViewBarWidget( true, parent )
{
  setupUi( centralWidget() );

  m_store->setIcon( QIcon::fromTheme( "document-save" ) );
  // Same as KStandardGuiItem::no()
  m_neverForThisSite->setIcon( QIcon::fromTheme( "process-stop" ) );
  m_doNotStore->setIcon( QIcon::fromTheme( "dialog-cancel" ) );
  centralWidget()->setFocusProxy( m_store );

  QPalette pal = palette();
  KColorScheme::adjustBackground(pal, KColorScheme::ActiveBackground);
  setPalette(pal);
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  connect( m_store, SIGNAL(clicked()), this, SIGNAL(storeClicked()) );
  connect( m_neverForThisSite, SIGNAL(clicked()), this,
    SIGNAL(neverForThisSiteClicked()) );
  connect( m_doNotStore, SIGNAL(clicked()), this,
    SIGNAL(doNotStoreClicked()) );

  m_store->setFocus();
}


void StorePassBar::setHost(const QString& host)
{
  if(host.isEmpty())
    m_label->setText( i18n("Do you want to store this password?") );
  else
    m_label->setText( i18n("Do you want to store this password for %1?", host) );
}



StorePass::StorePass( KHTMLPart *part ) :
  m_part( part )
{
  connect( &m_storePassBar, SIGNAL(storeClicked()), this, SLOT(slotStoreClicked()) );
  connect( &m_storePassBar, SIGNAL(neverForThisSiteClicked()), this,
    SLOT(slotNeverForThisSiteClicked()) );
  connect( &m_storePassBar, SIGNAL(doNotStoreClicked()), this,
    SLOT(slotDoNotStoreClicked()) );
}

StorePass::~StorePass()
{
}

void StorePass::saveLoginInformation(const QString& host, const QString& key,
  const QMap<QString, QString>& walletMap)
{
  KConfigGroup config( KSharedConfig::openConfig(), "HTML Settings" );
  if (!config.readEntry("OfferToSaveWebsitePassword", true))
    return;

  m_host = host;
  m_key = key;
  m_walletMap = walletMap;
  m_storePassBar.setHost(host);

  m_part->pTopViewBar()->addBarWidget( &m_storePassBar );
  m_part->pTopViewBar()->showBarWidget( &m_storePassBar );
}

void StorePass::removeBar()
{
  m_part->pTopViewBar()->hideCurrentBarWidget();
  m_walletMap.clear();
  m_host = m_key = "";
  m_storePassBar.setHost(m_host);
}

void StorePass::slotStoreClicked()
{
  m_part->saveToWallet(m_key, m_walletMap);
  removeBar();
}

void StorePass::slotNeverForThisSiteClicked()
{
  m_part->view()->addNonPasswordStorableSite(m_host);
  removeBar();
}

void StorePass::slotDoNotStoreClicked()
{
  removeBar();
}
