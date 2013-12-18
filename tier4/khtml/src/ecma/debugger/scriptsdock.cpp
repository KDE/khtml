/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2006 Matt Broadstone (mbroadst@gmail.com)
 *  Copyright (C) 2008 Maks Orlovich (maksim@kde.org)
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

#include "scriptsdock.h"

#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QUrl>

#include "debugwindow.h"
#include "debugdocument.h"

using namespace KJS;
using namespace KJSDebugger;

ScriptsDock::ScriptsDock(QWidget *parent)
    : QDockWidget(i18n("Loaded Scripts"), parent)
{
    setFeatures(DockWidgetMovable | DockWidgetFloatable);
    m_widget = new QTreeWidget;
    m_widget->header()->hide();

    connect(m_widget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(scriptSelected(QTreeWidgetItem*,int)));

    setWidget(m_widget);
}

ScriptsDock::~ScriptsDock()
{
}

void ScriptsDock::documentDestroyed(DebugDocument *document)
{
    // We may be asked to remove a document held for reload repeatedly;
    // ignore later attempts
    if (!m_documents.contains(document))
        return;

    QTreeWidgetItem* fragment = m_documents[document];
    m_documents.remove(document);

    QTreeWidgetItem* file      = fragment->parent();
    QTreeWidgetItem* domain    = file->parent();

    delete file->takeChild(file->indexOfChild(fragment));

    if (file->childCount())
        return;

    // Need to clean up the file as well.
    delete domain->takeChild(domain->indexOfChild(file));

    if (domain->childCount())
      return;

    // ... and domain
    m_headers.remove(domain->text(0));
    delete m_widget->takeTopLevelItem(m_widget->indexOfTopLevelItem(domain));
}

void ScriptsDock::addDocument(DebugDocument *document)
{
    assert (!m_documents.contains(document));

    QString name = document->name();
    QString domain;
    QString favicon;

    if (document->url().isEmpty())
        domain = "????"; // ### KDE4.1: proper i18n'able string

    else
    {
        QUrl kurl(document->url());
        if (!kurl.host().isEmpty())
            domain = kurl.host();
        else
            domain = "localhost";

        favicon = KIO::favIconForUrl(kurl);
    }

    QTreeWidgetItem *parent = 0;
    if (m_headers.contains(domain))
        parent = m_headers[domain];
    else
    {
        parent = new QTreeWidgetItem(QStringList() << domain);
        if (!favicon.isEmpty()) {
            QIcon icon = QIcon::fromTheme(favicon);
            parent->setIcon(0, icon);
        }

        m_headers[domain] = parent;
        m_widget->invisibleRootItem()->addChild(parent);
    }

    // Now try to find a kid for the name
    QTreeWidgetItem* file = 0;
    for (int c = 0; c < parent->childCount(); ++c) {
        QTreeWidgetItem* cand = parent->child(c);
        if (cand->text(0) == name)
            file = cand;
    }

    if (!file)
        file = new QTreeWidgetItem(parent, QStringList() << name);

    // Now add the fragment
    QString lines = QString::number(1 + document->baseLine()) + "-" +
                    QString::number(1 + document->baseLine() + document->length() - 1);

    QTreeWidgetItem *child = new QTreeWidgetItem(file, QStringList() << lines);
    m_documents[document] = child;
}

void ScriptsDock::scriptSelected(QTreeWidgetItem *item, int /*column*/)
{
    DebugDocument *doc = m_documents.key(item);
    if (doc)
        emit displayScript(doc);
}


// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
