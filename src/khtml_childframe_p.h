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
#ifndef khtml_childframe_p_h
#define khtml_childframe_p_h

#include <kparts/partmanager.h>
#include <kparts/statusbarextension.h>
#include <kparts/browserextension.h>
#include <kparts/scriptableextension.h>

#include "html/html_objectimpl.h"
#include "khtml_run.h"
#include "ecma/kjs_proxy.h"

namespace khtml {
class KHTML_EXPORT ChildFrame : public QObject
{
    Q_OBJECT
public:
    enum Type { Frame, IFrame, Object };

    ChildFrame();
    ~ChildFrame();

    QWeakPointer<DOM::HTMLPartContainerElementImpl> m_partContainerElement;
    QWeakPointer<KParts::BrowserExtension> m_extension;
    QWeakPointer<KParts::ScriptableExtension> m_scriptable;

    // Despite QPointer's shortcomings, we actually want it and not QPointer
    // here, since Window::getOwnPropertySlot needs to access it, and
    // QPointer's access path is cheaper
    QPointer<KParts::ReadOnlyPart> m_part;

    QString m_serviceName;
    QString m_serviceType;
    KJSProxy *m_jscript;
    bool m_bCompleted;
    QString m_name;
    KParts::OpenUrlArguments m_args;
    KParts::BrowserArguments m_browserArgs;
    QWeakPointer<KHTMLRun> m_run;
    QUrl m_workingURL;
    Type m_type;
    QStringList m_params;
    bool m_bPreloaded;
    bool m_bNotify;
    bool m_bPendingRedirection;

    // Debug stuff
    const char* typeString() const;
    void dump(int indent);
    static void dumpFrameTree(KHTMLPart* part);
}; // ChildFrame

} // namespace khtml

struct KHTMLFrameList : public QList<khtml::ChildFrame*>
{
    Iterator find( const QString &name ) KHTML_NO_EXPORT;
};

typedef KHTMLFrameList::ConstIterator ConstFrameIt;
typedef KHTMLFrameList::Iterator FrameIt;

#endif
