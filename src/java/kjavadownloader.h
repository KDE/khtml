/* This file is part of the KDE project
 *
 * Copyright (C) 2000 Wynn Wilkes <wynnw@caldera.com>
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


#ifndef KJAVADOWNLOADER_H
#define KJAVADOWNLOADER_H

#include <QtCore/QObject>

/**
 * @short A class for handling downloads from KIO
 *
 * This class handles a KIO::get job and passes the data
 * back to the AppletServer.
 *
 * @author Wynn Wilkes, wynnw@calderasystems.com
 */

class KJob;
namespace KIO {
    class Job;
}

class KJavaDownloaderPrivate;
class KJavaUploaderPrivate;

class KJavaKIOJob : public QObject
{
Q_OBJECT
public:
    virtual ~KJavaKIOJob();
    virtual void jobCommand( int cmd ) = 0;
    virtual void data( const QByteArray& qb );
};

class KJavaDownloader : public KJavaKIOJob
{
Q_OBJECT

public:
    KJavaDownloader( int ID, const QString& url );
    ~KJavaDownloader();

    virtual void jobCommand( int cmd );
protected Q_SLOTS:
    void slotData( KIO::Job*, const QByteArray& );
    void slotConnected( KIO::Job* );
    void slotMimetype( KIO::Job*, const QString& );
    void slotResult( KJob* );

private:
    KJavaDownloaderPrivate* const d;

};

class KJavaUploader : public KJavaKIOJob
{
Q_OBJECT

public:
    KJavaUploader( int ID, const QString& url );
    ~KJavaUploader();

    virtual void jobCommand( int cmd );
    virtual void data( const QByteArray& qb );
    void start();
protected Q_SLOTS:
    void slotDataRequest( KIO::Job*, QByteArray& );
    void slotResult( KJob* );
private:
    KJavaUploaderPrivate* const d;

};
#endif
