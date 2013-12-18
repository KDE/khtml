/* This file is part of the KDE project
 *
 * Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2007 David Faure <faure@kde.org>
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

#ifndef KHTML_GLOBAL_H
#define KHTML_GLOBAL_H

#include <khtml_export.h>

#include <kparts/historyprovider.h>

class KIconLoader;
class KAboutData;
class KHTMLSettings;
class KHTMLPart;

namespace KParts
{
  class HistoryProvider;
}
namespace DOM
{
  class DocumentImpl;
}

/**
 * @internal
 * Exported for khtml test programs, but not installed
 */
class KHTML_EXPORT KHTMLGlobal
{
    friend class KHTMLViewPrivate;

public:
    KHTMLGlobal();
    ~KHTMLGlobal();

    static void registerPart( KHTMLPart *part );
    static void deregisterPart( KHTMLPart *part );

    static void registerDocumentImpl( DOM::DocumentImpl *doc );
    static void deregisterDocumentImpl( DOM::DocumentImpl *doc );

    static const KAboutData &aboutData();
    static KIconLoader *iconLoader();

    static KHTMLSettings *defaultHTMLSettings();

    // list of visited URLs
    static KParts::HistoryProvider *vLinks() {
        return KParts::HistoryProvider::self();
    }

    // Called when khtml part is unloaded
    static void finalCheck();

private:
    static void ref();
    static void deref();
private:
    static unsigned long s_refcnt;
    static KHTMLGlobal *s_self;
    static KIconLoader *s_iconLoader;
    static KAboutData *s_about;
    static KHTMLSettings *s_settings;
};

#endif

