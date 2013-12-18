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


#ifndef KJAVAAPPLET_H
#define KJAVAAPPLET_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QMap>

/**
 * @short A Java applet
 *
 * This class encapsulates the data the Applet Server needs to load
 * the Applet class files, and set the proper size of the Applet.  It also
 * has an interface for applets to resize themselves.
 *
 * @author Richard J. Moore, rich@kde.org
 * @author Wynn Wilkes, wynnw@kde.org
 */

class KJavaApplet;
class KJavaAppletWidget;
class KJavaAppletContext;
class KJavaAppletPrivate;


class KJavaApplet : public QObject
{
Q_OBJECT

public:
    // states describing the life cycle of an applet.
    // keep in sync with applet state in KJASAppletStub.java !
    typedef enum {
         UNKNOWN      = 0,
         CLASS_LOADED = 1,
         INSTANCIATED = 2,
         INITIALIZED  = 3,
         STARTED      = 4,
         STOPPED      = 5,
         DESTROYED    = 6
    } AppletState;
    KJavaApplet( KJavaAppletWidget* _parent, KJavaAppletContext* _context = 0 );
    ~KJavaApplet();

    /**
     * Set the applet context'.
     */
    void setAppletContext( KJavaAppletContext* _context );

    /**
     * Specify the name of the class file to run. For example 'Lake.class'.
     */
    void setAppletClass( const QString& clazzName );

    /**
     * Get the name of the Class file the applet should run
     */
    QString& appletClass();

    /**
     * Set the URL of the document embedding the applet.
     */
    void setBaseURL( const QString& base );

    /**
     * get the Base URL of the document embedding the applet
     */
    QString& baseURL();

    /**
     * Set the codebase of the applet classes.
     */
    void setCodeBase( const QString& codeBase );

    /**
     * Get the codebase of the applet classes
     */
    QString& codeBase();

    /**
     * Set the list of archives at the Applet's codebase to search in for
     * class files and other resources
     */
    void setArchives( const QString& _archives );

    /**
     * Get the list of Archives that should be searched for class files
     * and other resources
     */
    QString& archives();

    /**
     * Set the name the applet should be called in its context
     */
    void setAppletName( const QString& name );

    /**
     * Get the name the applet should be called in its context
     */
    QString& appletName();

    /**
     * Set the size of the applet
     */
    void setSize( QSize size );

    /**
     * Get the size of the applet
     */
    QSize size();

    /**
     * Specify a parameter to be passed to the applet.
     */
    void setParameter( const QString& name, const QString& value );

    /**
     * Look up the parameter value for the given Parameter.  Returns
     * QString() if the name has not been set.
     */
    QString& parameter( const QString& name );

    /**
     * Get a reference to the Parameters and their values
     */
    QMap<QString,QString>& getParams();

    /**
     * Set the window title for swallowing
     */
    void setWindowName( const QString& title );

    /**
     * Get the window title this applet should use
     */
    QString& getWindowName();

    /**
     * Interface for applets to resize themselves
     */
    void resizeAppletWidget( int width, int height );

    /**
     * Send message to AppletServer to create this applet's
     * frame to be swallowed and download the applet classes
     */
    void create();

    /**
     * Send message to AppletServer to Initialize and show
     * this applet
     */
    void init();

    /**
     * Returns status of applet- whether it's been created or not
     */
    bool isCreated();

    /**
     * Run the applet.
     */
    void start();

    /**
     * Pause the applet.
     */
    void stop();

    /**
     * Returns the unique ID this applet is given
     */
    int  appletId();

    /**
     * Set the applet ID.
     */
    void setAppletId( int id );

    KJavaAppletContext* getContext() const { return context; }

    /**
     * Get/Set the user name
     */
    void setUser(const QString & _user) { username = _user; }
    const QString & user () const { return username; }

    /**
     * Get/Set the user password
     */
    void setPassword(const QString & _password) { userpassword = _password; }
    const QString & password () const { return userpassword; }

    /**
     * Get/Set the auth name
     */
    void setAuthName(const QString & _auth) { authname = _auth; }
    const QString & authName () const { return authname; }

    /**
    * called from the protocol engine
    * changes the status according to the one on the java side.
    * Do not call this yourself!
    */
    void stateChange ( const int newState );
    void setFailed ();
    AppletState state() const;
    bool failed() const;
    bool isAlive() const;
    /**
     * JavaScript coming from Java
     **/
    void jsData (const QStringList & args) { emit jsEvent (args); }
Q_SIGNALS:
    void jsEvent (const QStringList & args);
private:
    void showStatus( const QString &msg);
    KJavaAppletPrivate* const d;
    QMap<QString, QString> params;
    KJavaAppletContext*    context;
    int                    id;
    QString                username;
    QString                userpassword;
    QString                authname;
};

#endif // KJAVAAPPLET_H
