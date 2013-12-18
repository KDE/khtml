// -*- c++ -*-

/* This file is part of the KDE project
 *
 * Copyright (C) 2000 Richard Moore <rich@kde.org>
 *               2000 Wynn Wilkes <wynnw@caldera.com>
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

#ifndef KJAVAAPPLETSERVER_H
#define KJAVAAPPLETSERVER_H

#include "kjavaprocess.h"
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QSize>

/**
 * @short Communicates with a KJAS server to display and control Java applets.
 *
 * @author Richard J. Moore, rich@kde.org
 */

class KJavaAppletContext;
class KJavaAppletServerPrivate;
class JSStackFrame;

class KJavaAppletServer : public QObject
{
Q_OBJECT

public:
    /**
     * Create the applet server.  These shouldn't be used directly,
     * use allocateJavaServer instead
     */
    KJavaAppletServer();
    ~KJavaAppletServer();

    /**
     * A factory method that returns the default server. This is the way this
     * class is usually instantiated.
     */
    static KJavaAppletServer *allocateJavaServer();

    /**
     * When you are done using your reference to the AppletServer,  you must
     * dereference it by calling freeJavaServer().
     */
    static void freeJavaServer();

    /**
     * This allows the KJavaAppletWidget to display some feedback in a QLabel
     * while the applet is being loaded.  If the java process could not be
     * started, an error message is displayed instead.
     */
    static QString getAppletLabel();

    /**
     * Create an applet context with the specified id.
     */
    void createContext( int contextId, KJavaAppletContext* context );

    /**
     * Destroy the applet context with the specified id. All the applets in the
     * context will be destroyed as well.
     */
    void destroyContext( int contextId );

    /**
     * Create an applet in the specified context with the specified id. The applet
     * name, class etc. are specified in the same way as in the HTML APPLET tag.
     */
    bool createApplet( int contextId, int appletId,
                       const QString & name, const QString & clazzName,
                       const QString & baseURL, const QString & user,
                       const QString & password, const QString & authname,
                       const QString & codeBase, const QString & jarFile,
                       QSize size, const QMap<QString, QString>& params,
                       const QString & windowTitle );

    /**
     * This should be called by the KJavaAppletWidget
     */
    void initApplet( int contextId, int appletId );

    /**
     * Destroy an applet in the specified context with the specified id.
     */
    void destroyApplet( int contextId, int appletId );

    /**
     * Start the specified applet.
     */
    void startApplet( int contextId, int appletId );

    /**
     * Stop the specified applet.
     */
    void stopApplet( int contextId, int appletId );

    /**
     * Show java console.
     */
    void showConsole();

    /**
     * Send data we got back from a KJavaDownloader back to the appropriate
     * class loader.
     */
    void sendURLData( int loaderID, int code, const QByteArray& data );
    /**
     * Removes KJavaDownloader from the list (deletes it too).
     */
    void removeDataJob( int loaderID );

    /**
     * Shut down the KJAS server.
     */
    void quit();
    KJavaProcess* javaProcess() { return process; }

    QString appletLabel();

    void waitForReturnData(JSStackFrame *);
    void endWaitForReturnData();

    bool getMember(QStringList & args, QStringList & ret_args);
    bool putMember(QStringList & args);
    bool callMember(QStringList & args, QStringList & ret_args);
    void derefObject(QStringList & args);

    bool usingKIO();
protected:
    void setupJava( KJavaProcess* p );

    KJavaProcess* process;

protected Q_SLOTS:
    void slotJavaRequest( const QByteArray& qb );
    void checkShutdown();
    void timerEvent(QTimerEvent *);
    void killTimers();

private:
    KJavaAppletServerPrivate* const d;

};


class PermissionDialog : public QObject
{
    Q_OBJECT
public:
    PermissionDialog( QWidget* );
    ~PermissionDialog();

    QString exec( const QString & cert, const QString & perm );

private Q_SLOTS:
     void clicked();

private:
    QString m_button;
};

#endif // KJAVAAPPLETSERVER_H
