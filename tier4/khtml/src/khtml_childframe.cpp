/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999-2001 Lars Knoll <knoll@kde.org>
 *                     1999-2001 Antti Koivisto <koivisto@kde.org>
 *                     2000-2001 Simon Hausmann <hausmann@kde.org>
 *                     2000-2001 Dirk Mueller <mueller@kde.org>
 *                     2000 Stefan Schimanski <1Stein@gmx.de>
 *                     2001-2005 George Staikos <staikos@kde.org>
 *                     2010 Maksim Orlovich <maksim@kde.org>
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
#include "khtml_childframe_p.h"
#include "khtmlpart_p.h"
#include "khtml_part.h"

namespace khtml {

ChildFrame::ChildFrame() : QObject (0) {
    setObjectName( "khtml_child_frame" );
    m_jscript = 0L;
    m_bCompleted = false; m_bPreloaded = false; m_type = Frame; m_bNotify = false;
    m_bPendingRedirection = false;
}

ChildFrame::~ChildFrame() {
    if (m_run)
        m_run.data()->abort();
    delete m_jscript;
}

const char* ChildFrame::typeString() const
{
    switch (m_type) {
    case khtml::ChildFrame::Frame:
        return "frame";
    case khtml::ChildFrame::IFrame:
        return "iframe";
    case khtml::ChildFrame::Object:
        return "object";
    default:
        return "HUH???";
    }
}

static QDebug& ind(const QDebug& dbgIn, int ind)
{
    QDebug& dbg = const_cast<QDebug&>(dbgIn);

    for (int i = 0; i < ind; ++i)
        dbg << " ";
    return dbg;
}

void ChildFrame::dump(int i)
{
    ind(qDebug(), i) << typeString() << m_name
                << this << m_part.data()
                << "url:" << (m_part.isNull() ?
                                QString::fromLatin1("") :
                                m_part->url().toString())
                << "el:" << (m_partContainerElement.isNull() ?
                                QString::fromLatin1("") :
                                DOM::getPrintableName(m_partContainerElement.data()->id()))
                << "sn:" << m_serviceName << "st:" << m_serviceType
                << "kr:" << m_run.data() << "comp:" << m_bCompleted;
}

void ChildFrame::dumpFrameTree(KHTMLPart* part)
{
    static int i = 0;

    KHTMLPartPrivate* d = part->impl();
    
    if (!d->m_objects.isEmpty()) {
        ind(qDebug(), i) << "objects:";
        i += 4;

        for (QList<khtml::ChildFrame*>::Iterator io = d->m_objects.begin(); io != d->m_objects.end(); ++io) {
            khtml::ChildFrame* cf = (*io);
            cf->dump(i);
            if (KHTMLPart* p = ::qobject_cast<KHTMLPart*>(cf->m_part.data())) {
                i += 4;
                dumpFrameTree(p);
                i -= 4;
            }
        }
        i -= 4;
    }

    if (!d->m_frames.isEmpty()) {
        ind(qDebug(), i) << "frames:";
        i += 4;

        for (QList<khtml::ChildFrame*>::Iterator ifr = d->m_frames.begin(); ifr != d->m_frames.end(); ++ifr) {
            khtml::ChildFrame* cf = (*ifr);
            cf->dump(i);
            if (KHTMLPart* p = ::qobject_cast<KHTMLPart*>(cf->m_part.data())) {
                i += 4;
                dumpFrameTree(p);
                i -= 4;
            }
        }
        i -= 4;
    }
}


} // namespace khtml


KHTMLFrameList::Iterator KHTMLFrameList::find( const QString &name )
{
    Iterator it = begin();
    const Iterator e = end();

    for (; it!=e; ++it )
        if ( (*it)->m_name==name )
            break;

    return it;
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
