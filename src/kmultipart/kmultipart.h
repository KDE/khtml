/* This file is part of the KDE project
   Copyright (C) 2002 David Faure <david@mandrakesoft.com>

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

#ifndef __kmultipart_h__
#define __kmultipart_h__

#include <QPointer>

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <QtCore/QDate>

class HTTPFilterGZip;
class QTemporaryFile;
class KLineParser;

/**
 * http://wp.netscape.com/assist/net_sites/pushpull.html
 */
class KMultiPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    KMultiPart( QWidget *parentWidget,
                QObject *parent, const QVariantList& );
    virtual ~KMultiPart();

    virtual bool openFile() { return false; }
    virtual bool openUrl(const QUrl &url);

    virtual bool closeUrl();

protected:
    virtual void guiActivateEvent( KParts::GUIActivateEvent *e );
    void setPart( const QString& mimeType );

    void startOfData();
    void sendData( const QByteArray& line );
    void endOfData();

private Q_SLOTS:
    void reallySendData( const QByteArray& line );
    //void slotPopupMenu( KXMLGUIClient *cl, const QPoint &pos, const QUrl &u, const QString &mime, mode_t mode );
    void slotJobFinished( KJob *job );
    void slotData( KIO::Job *, const QByteArray & );
    //void updateWindowCaption();

    void slotPartCompleted();

    void startHeader();

    void slotProgressInfo();

private:
    KParts::BrowserExtension* m_extension;
    QPointer<KParts::ReadOnlyPart> m_part;
    bool m_isHTMLPart;
    bool m_partIsLoading;
    KIO::Job* m_job;
    QByteArray m_boundary;
    int m_boundaryLength;
    QString m_mimeType; // the one handled by m_part - store the kservice instead?
    QString m_nextMimeType; // while parsing headers
    QTemporaryFile* m_tempFile;
    KLineParser* m_lineParser;
    bool m_bParsingHeader;
    bool m_bGotAnyHeader;
    bool m_gzip;
    HTTPFilterGZip *m_filter;
    // Speed measurements
    long m_totalNumberOfFrames;
    long m_numberOfFrames;
    long m_numberOfFramesSkipped;
    QTime m_qtime;
    QTimer* m_timer;
};

#if 0
class KMultiPartBrowserExtension : public KParts::BrowserExtension
{
    //Q_OBJECT
public:
    KMultiPartBrowserExtension( KMultiPart *parent, const char *name = 0 );

    virtual int xOffset();
    virtual int yOffset();

//protected Q_SLOTS:
    void print();
    void reparseConfiguration();

private:
    KMultiPart *m_imgPart;
};
#endif

#endif
