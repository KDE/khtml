/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Simon Hausmann <hausmann@kde.org>
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
#include "khtml_run.h"
#include "khtmlpart_p.h"
#include <kio/job.h>
#include <QDebug>
#include <klocalizedstring.h>
#include "khtml_ext.h"
#include <QImage>

KHTMLRun::KHTMLRun( KHTMLPart *part, khtml::ChildFrame *child, const QUrl &url,
                    const KParts::OpenUrlArguments& args,
                    const KParts::BrowserArguments &browserArgs,
                    bool hideErrorDialog )
    : KParts::BrowserRun( url, args, browserArgs, part, part->widget() ? part->widget()->topLevelWidget() : 0,
                          false, false, hideErrorDialog ),
  m_child( child )
{
    // Don't use an external browser for parts of a webpage we are rendering. (iframes at least are one example)
    setEnableExternalBrowser(false);

    // get the wheel to start spinning
    part->started(0L);
}

//KHTMLPart *KHTMLRun::htmlPart() const
//{ return static_cast<KHTMLPart *>(part()); }

void KHTMLRun::foundMimeType( const QString &_type )
{
    //qDebug() << this << _type;
    Q_ASSERT(!hasFinished());
    QString mimeType = _type; // this ref comes from the job, we lose it when using KIO again

    bool requestProcessed = static_cast<KHTMLPart *>(part())->processObjectRequest( m_child, KRun::url(), mimeType );

    if ( requestProcessed )
        setFinished( true );
    else {
        if ( hasFinished() ) // abort was called (this happens with the activex fallback for instance)
            return;
        // Couldn't embed -> call BrowserRun::handleNonEmbeddable()
        KService::Ptr selectedService;
        KParts::BrowserRun::NonEmbeddableResult res = handleNonEmbeddable( mimeType, &selectedService );
        if ( res == KParts::BrowserRun::Delayed )
            return;
        setFinished( res == KParts::BrowserRun::Handled );
        if ( hasFinished() ) { // saved or canceled -> flag completed
            m_child->m_bCompleted = true;
            static_cast<KHTMLPart *>(part())->checkCompleted();
        } else {
            // "Open" selected, possible with a specific application
            if (selectedService) {
                KRun::setPreferredService(selectedService->desktopEntryName());
            } else {
                KRun::displayOpenWithDialog(QList<QUrl>() << url(), part()->widget(), false /*tempfile*/, suggestedFileName());
                setFinished(true);
            }
        }
    }

    if ( hasFinished() ) {
        // qDebug() << "finished";
        return;
    }

    //qDebug() << _type << " couldn't open";
    KRun::foundMimeType( mimeType );

    // "open" is finished -> flag completed
    m_child->m_bCompleted = true;
    static_cast<KHTMLPart *>(part())->checkCompleted();
}

void KHTMLRun::handleError(KJob*)
{
    // Tell KHTML that loading failed.
    static_cast<KHTMLPart *>(part())->processObjectRequest( m_child, QUrl(), QString() );
    setJob(0);
}

void KHTMLRun::save(const QUrl & url, const QString & suggestedFilename)
{
    KHTMLPopupGUIClient::saveURL( part()->widget(), i18n( "Save As" ), url, arguments().metaData(), QString(), 0, suggestedFilename );
}

