/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __khtmlimage_h__
#define __khtmlimage_h__

#include "khtml_part.h"
#include <kpluginfactory.h>
#include <kparts/browserextension.h>
#include <kparts/statusbarextension.h>

#include "misc/loader_client.h"

class KHTMLPart;
class KComponentData;

namespace khtml
{
    class CachedImage;
}

/**
 * @internal
 * (Only exported for khtmlimage_init.cpp, i.e. the part)
 */
class KHTML_EXPORT KHTMLImageFactory : public KPluginFactory
{
    Q_OBJECT
public:
    KHTMLImageFactory();
    virtual ~KHTMLImageFactory();

    virtual QObject *create(const char* iface,
                            QWidget *parentWidget,
                            QObject *parent,
                            const QVariantList& args,
                            const QString &keyword);

    static const KAboutData &aboutData() { return *s_aboutData; }

private:
    static KAboutData *s_aboutData;
};

/**
 * @internal
 */
class KHTMLImage : public KParts::ReadOnlyPart, public khtml::CachedObjectClient
{
    Q_OBJECT
public:
    KHTMLImage( QWidget *parentWidget,
                QObject *parent, KHTMLPart::GUIProfile prof );
    virtual ~KHTMLImage();

    virtual bool openFile() { return true; } // grmbl, should be non-pure in part.h, IMHO

    virtual bool openUrl(const QUrl &url);

    virtual bool closeUrl();

    KHTMLPart *doc() const { return m_khtml; }

    virtual void notifyFinished( khtml::CachedObject *o );

protected:
    virtual void guiActivateEvent( KParts::GUIActivateEvent *e );

private Q_SLOTS:
    void restoreScrollPosition();

    void slotPopupMenu( const QPoint &global, const QUrl &url, mode_t mode,
                        const KParts::OpenUrlArguments &args,
                        const KParts::BrowserArguments &browserArgs,
                        KParts::BrowserExtension::PopupFlags flags,
                        const KParts::BrowserExtension::ActionGroupMap& actionGroups );

//    void slotImageJobFinished( KIO::Job *job );

//    void updateWindowCaption();

private:
    void disposeImage();

    QPointer<KHTMLPart> m_khtml;
    KParts::BrowserExtension *m_ext;
    KParts::StatusBarExtension *m_sbExt;
    QString m_mimeType;
    khtml::CachedImage *m_image;
    int m_xOffset, m_yOffset;
};

/**
 * @internal
 */
class KHTMLImageBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    KHTMLImageBrowserExtension( KHTMLImage *parent );

    virtual int xOffset();
    virtual int yOffset();

protected Q_SLOTS:
    void print();
    void reparseConfiguration();
    void disableScrolling();

private:
    KHTMLImage *m_imgPart;
};

#endif
