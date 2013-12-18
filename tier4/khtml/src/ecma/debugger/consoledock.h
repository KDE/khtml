/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2006 Matt Broadstone (mbroadst@gmail.com)
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

#ifndef CONSOLEDOCK_H
#define CONSOLEDOCK_H

#include <QDockWidget>
#include <QString>

class QListWidget;
class QListWidgetItem;
class KHistoryComboBox;
class QPushButton;

namespace KJSDebugger {

struct InterpreterContext;

class ConsoleDock : public QDockWidget
{
    Q_OBJECT
public:
    ConsoleDock(QWidget *parent = 0);
    ~ConsoleDock();

    void reportResult(const QString& src, const QString& msg);

Q_SIGNALS: // Bah. This isn't a public header.
    void requestEval(const QString& code);

private Q_SLOTS:
    void slotUserRequestedEval();
    void slotPasteItem(QListWidgetItem* item);

private:
    QListWidget *consoleView;
    KHistoryComboBox *consoleInput;
    QPushButton *consoleInputButton;
};

}

#endif
