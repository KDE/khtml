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

#ifndef SCRIPTSDOCK_H
#define SCRIPTSDOCK_H

#include <QDockWidget>
#include <QHash>

class QTreeWidget;
class QTreeWidgetItem;

namespace KJSDebugger {
class DebugDocument;

class ScriptsDock : public QDockWidget
{
    Q_OBJECT
public:
    ScriptsDock(QWidget *parent = 0);
    ~ScriptsDock();

    void addDocument(DebugDocument *document);
    void documentDestroyed(DebugDocument *document);

Q_SIGNALS:
    void displayScript(KJSDebugger::DebugDocument *document);

private Q_SLOTS:
    void scriptSelected(QTreeWidgetItem *item, int column);

private:
    QTreeWidget *m_widget;
    QHash<DebugDocument*, QTreeWidgetItem*> m_documents;
    QHash<QString, QTreeWidgetItem*> m_headers;
};

}

#endif
