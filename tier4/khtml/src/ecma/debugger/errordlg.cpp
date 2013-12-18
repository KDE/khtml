/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
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
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "errordlg.h"

#include <QLabel>
#include <QBoxLayout>


#include <klocalizedstring.h>
#include <kiconloader.h>

namespace KJSDebugger { 

KJSErrorDialog::KJSErrorDialog(QWidget *parent, const QString& errorMessage, bool showDebug)
  : KDialog( parent )
{
  setCaption( i18n("JavaScript Error") );
  setModal( true );
  setButtons( showDebug ? KDialog::Ok | KDialog::User1 : KDialog::Ok );
  setButtonGuiItem( KDialog::User1, KGuiItem("&Debug","system-run") );
  setDefaultButton( KDialog::Ok );

  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QLabel *iconLabel = new QLabel("",page);
  iconLabel->setPixmap(SmallIcon("dialog-error", KIconLoader::SizeMedium));

  QWidget *contents = new QWidget(page);
  QLabel *label = new QLabel(errorMessage,contents);
  m_dontShowAgainCb = new QCheckBox(i18n("&Do not show this message again"),contents);

  QVBoxLayout *vl = new QVBoxLayout(contents);
  vl->setMargin(0);
  vl->addWidget(label);
  vl->addWidget(m_dontShowAgainCb);

  QHBoxLayout *topLayout = new QHBoxLayout(page);
  topLayout->setMargin(0);
  topLayout->addWidget(iconLabel);
  topLayout->addWidget(contents);
  topLayout->addStretch(10);

  m_debugSelected = false;
  connect(this,SIGNAL(user1Clicked()),this,SLOT(slotUser1()));
}

KJSErrorDialog::~KJSErrorDialog()
{
}

void KJSErrorDialog::slotUser1()
{
  m_debugSelected = true;
  close();
}

}
