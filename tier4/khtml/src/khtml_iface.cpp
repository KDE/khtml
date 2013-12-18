/* This file is part of the KDE project
 *
 * Copyright (C) 2002 Stephan Kulow <coolo@kde.org>
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

#include "khtml_iface.h"
#include "khtml_part.h"
#include "khtmlview.h"
#include "khtml_ext.h"
#include <kio/global.h>
#include <QApplication>
#include <QtCore/QVariant>

KHTMLPartIface::KHTMLPartIface( KHTMLPart *_part )
    : QDBusAbstractAdaptor( _part ), part(_part)
{
}

KHTMLPartIface::~KHTMLPartIface()
{
}

QString KHTMLPartIface::url() const
{
    return part->url().toString();
}

void KHTMLPartIface::setJScriptEnabled( bool enable )
{
    part->setJScriptEnabled(enable);
}

bool KHTMLPartIface::jScriptEnabled() const
{
    return part->jScriptEnabled();
}

bool KHTMLPartIface::closeUrl()
{
    return part->closeUrl();
}

bool KHTMLPartIface::metaRefreshEnabled() const
{
    return part->metaRefreshEnabled();
}

void KHTMLPartIface::setDndEnabled( bool b )
{
    part->setDNDEnabled(b);
}

bool KHTMLPartIface::dndEnabled() const
{
    return part->dndEnabled();
}

void KHTMLPartIface::setJavaEnabled( bool enable )
{
    part->setJavaEnabled( enable );
}

bool KHTMLPartIface::javaEnabled() const
{
    return part->javaEnabled();
}

void KHTMLPartIface::setPluginsEnabled( bool enable )
{
    part->setPluginsEnabled( enable );
}

bool KHTMLPartIface::pluginsEnabled() const
{
    return part->pluginsEnabled();
}

void KHTMLPartIface::setAutoloadImages( bool enable )
{
    part->setAutoloadImages( enable );
}

bool KHTMLPartIface::autoloadImages() const
{
    return part->autoloadImages();
}

void KHTMLPartIface::setOnlyLocalReferences(bool enable)
{
    part->setOnlyLocalReferences(enable);
}

void KHTMLPartIface::setMetaRefreshEnabled( bool enable )
{
    part->setMetaRefreshEnabled(enable);
}

bool KHTMLPartIface::onlyLocalReferences() const
{
    return part->onlyLocalReferences();
}

bool KHTMLPartIface::setEncoding( const QString &name )
{
    return part->setEncoding(name);
}

QString KHTMLPartIface::encoding() const
{
    return part->encoding();
}

void KHTMLPartIface::setFixedFont( const QString &name )
{
    part->setFixedFont(name);

}

bool KHTMLPartIface::gotoAnchor( const QString &name )
{
    return part->gotoAnchor(name);
}

bool KHTMLPartIface::nextAnchor()
{
    return part->nextAnchor();
}

bool KHTMLPartIface::prevAnchor()
{
    return part->prevAnchor();
}

void KHTMLPartIface::activateNode()
{
    KParts::ReadOnlyPart* p = part->currentFrame();
    if ( p && p->widget() ) {
        QKeyEvent ev( QKeyEvent::KeyPress, Qt::Key_Return, 0, "\n" );
        QApplication::sendEvent( p->widget(), &ev );
    }
}

void KHTMLPartIface::selectAll()
{
    part->selectAll();
}

QString KHTMLPartIface::lastModified() const
{
    return part->lastModified();
}

void KHTMLPartIface::debugRenderTree()
{
    part->slotDebugRenderTree();
}

void KHTMLPartIface::debugDOMTree()
{
    part->slotDebugDOMTree();
}

void KHTMLPartIface::stopAnimations()
{
    part->slotStopAnimations();
}

void KHTMLPartIface::viewDocumentSource()
{
    part->slotViewDocumentSource();
}

void KHTMLPartIface::saveBackground(const QString &destination)
{
    QUrl back = part->backgroundURL();
    if (back.isEmpty())
        return;

    KIO::MetaData metaData;
    metaData["referrer"] = part->referrer();
    KHTMLPopupGUIClient::saveURL( part->widget(), back, QUrl( destination ), metaData );
}

void KHTMLPartIface::saveDocument(const QString &destination)
{
    QUrl srcURL( part->url() );

    if ( srcURL.fileName().isEmpty() )
        srcURL.setPath( srcURL.path() + "index.html" );

    KIO::MetaData metaData;
    // Referrer unknown?
    KHTMLPopupGUIClient::saveURL( part->widget(), srcURL, QUrl( destination ), metaData, part->cacheId() );
}

void KHTMLPartIface::setUserStyleSheet(const QString &styleSheet)
{
    part->setUserStyleSheet(styleSheet);
}

QString KHTMLPartIface::selectedText() const
{
    return part->selectedText();
}

void KHTMLPartIface::viewFrameSource()
{
    part->slotViewFrameSource();
}

QString KHTMLPartIface::evalJS(const QString &script)
{
    return part->executeScript(DOM::Node(), script).toString();
}

void KHTMLPartIface::print( bool quick ) {
    part->view()->print( quick );
}

