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


#ifndef ERROR_DLG_H
#define ERROR_DLG_H

#include <QString>
#include <QCheckBox>
#include <QWidget>
#include <kdialog.h>

namespace KJSDebugger {
  class KJSErrorDialog : public KDialog {
    Q_OBJECT
  public:
    KJSErrorDialog(QWidget *parent, const QString& errorMessage, bool showDebug);
    virtual ~KJSErrorDialog();

    bool debugSelected() const { return m_debugSelected; }
    bool dontShowAgain() const { return m_dontShowAgainCb->isChecked(); }

  protected Q_SLOTS:
    virtual void slotUser1();

  private:
    QCheckBox *m_dontShowAgainCb;
    bool m_debugSelected;
  };
}

#endif

// kate: indent-width 2; replace-tabs on; tab-width 2; space-indent on;
