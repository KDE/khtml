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

#ifndef CALLSTACKDOCK_H
#define CALLSTACKDOCK_H

#include <QDockWidget>

class QTableWidget;
class QTableWidgetItem;

namespace KJS {
    class ExecState;
}

namespace KJSDebugger {
class DebugDocument;
struct InterpreterContext;

class CallStackDock : public QDockWidget
{
    Q_OBJECT
public:
    CallStackDock(QWidget *parent = 0);
    ~CallStackDock();

    void displayStack(InterpreterContext* ctx);
    void clearDisplay();

    KJS::ExecState* selectedFrameContext();
private Q_SLOTS:
    void slotViewItem(QTableWidgetItem* item);
Q_SIGNALS:
    void displayScript(KJSDebugger::DebugDocument* doc, int line);
private:
    InterpreterContext* m_activeCtx;
    QTableWidget *m_view;
};

}

#endif
