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

#include "kjavaappletwidget.h"
#include "kjavaappletcontext.h"

#include <klocalizedstring.h>
#include <QDebug>
#include <kparts/browserextension.h>



class KJavaAppletPrivate
{
public:
   bool    reallyExists;
   bool    failed;
   QString className;
   QString appName;
   QString baseURL;
   QString codeBase;
   QString archives;
   QSize   size;
   QString windowName;
   KJavaApplet::AppletState state;

   KJavaAppletWidget* UIwidget;
};


KJavaApplet::KJavaApplet( KJavaAppletWidget* _parent,
                          KJavaAppletContext* _context )
    : d(new KJavaAppletPrivate), params()
{

    d->UIwidget = _parent;
    d->state = UNKNOWN;
    d->failed = false;

    if( _context )
        setAppletContext( _context );

    d->reallyExists = false;
}

KJavaApplet::~KJavaApplet()
{
    if ( d->reallyExists )
        context->destroy( this );

    delete d;
}

bool KJavaApplet::isCreated()
{
    return d->reallyExists;
}

void KJavaApplet::setAppletContext( KJavaAppletContext* _context )
{
    context = _context;
    context->registerApplet( this );
}

void KJavaApplet::setAppletClass( const QString& _className )
{
    d->className = _className;
}

QString& KJavaApplet::appletClass()
{
    return d->className;
}

QString& KJavaApplet::parameter( const QString& name )
{
    return params[ name ];
}

void KJavaApplet::setParameter( const QString& name, const QString& value )
{
    params.insert( name, value );
}

QMap<QString,QString>& KJavaApplet::getParams()
{
    return params;
}

void KJavaApplet::setBaseURL( const QString& baseURL )
{
    d->baseURL = baseURL;
}

QString& KJavaApplet::baseURL()
{
    return d->baseURL;
}

void KJavaApplet::setCodeBase( const QString& codeBase )
{
    d->codeBase = codeBase;
}

QString& KJavaApplet::codeBase()
{
    return d->codeBase;
}

void KJavaApplet::setSize( QSize size )
{
    d->size = size;
}

QSize KJavaApplet::size()
{
    return d->size;
}

void KJavaApplet::setArchives( const QString& _archives )
{
    d->archives = _archives;
}

QString& KJavaApplet::archives()
{
    return d->archives;
}

void KJavaApplet::resizeAppletWidget( int width, int height )
{
    // qDebug() << "KJavaApplet, id = " << id << ", ::resizeAppletWidget to " << width << ", " << height;

    QStringList sl;
    sl.push_back( QString::number( 0 ) ); // applet itself has id 0
    sl.push_back( QString( "eval" ) );    // evaluate next script
    sl.push_back( QString::number( KParts::LiveConnectExtension::TypeString ) );
    sl.push_back( QString( "this.setAttribute('WIDTH',%1);this.setAttribute('HEIGHT',%2)" ).arg( width ).arg( height ) );
    jsData( sl );
}

void KJavaApplet::setAppletName( const QString& name )
{
    d->appName = name;
}

void KJavaApplet::setWindowName( const QString& title )
{
    d->windowName = title;
}

QString& KJavaApplet::getWindowName()
{
    return d->windowName;
}

QString& KJavaApplet::appletName()
{
    return d->appName;
}

void KJavaApplet::create( )
{
    if (  !context->create( this ) )
        setFailed();
    d->reallyExists = true;
}

void KJavaApplet::init()
{
    context->init( this );
}

void KJavaApplet::start()
{
    context->start( this );
}

void KJavaApplet::stop()
{
    context->stop( this );
}

int KJavaApplet::appletId()
{
    return id;
}

void KJavaApplet::setAppletId( int _id )
{
    id = _id;
}

void KJavaApplet::stateChange( const int newStateInt ) {
    AppletState newState = (AppletState)newStateInt;
    bool ok = false;
    if (d->failed) {
        return;
    }
    switch ( newState ) {
        case CLASS_LOADED:
            ok = (d->state == UNKNOWN);
            break;
        case INSTANCIATED:
            ok = (d->state == CLASS_LOADED);
            if (ok) {
                showStatus(i18n("Initializing Applet \"%1\"...", appletName()));
            }
            break;
        case INITIALIZED:
            ok = (d->state == INSTANCIATED);
            if (ok) { 
                showStatus(i18n("Starting Applet \"%1\"...", appletName()));
                start();
            }
            break;
        case STARTED:
            ok = (d->state == INITIALIZED || d->state == STOPPED);
            if (ok) {    
                showStatus(i18n("Applet \"%1\" started", appletName()));
            }
            break;
        case STOPPED:
            ok = (d->state == INITIALIZED || d->state == STARTED);
            if (ok) {    
                showStatus(i18n("Applet \"%1\" stopped", appletName()));
            }
            break;
        case DESTROYED:
            ok = true;
            break;
        default:
            break;
    }
    if (ok) {
        d->state = newState;
    } else {
        qCritical() << "KJavaApplet::stateChange : don't want to switch from state "
            << d->state << " to " << newState << endl;
    } 
}

void KJavaApplet::showStatus(const QString &msg) {
    QStringList args;
    args << msg;
    context->processCmd("showstatus", args); 
}

void KJavaApplet::setFailed() {
    d->failed = true;
}

bool KJavaApplet::isAlive() const {
   return (
        !d->failed 
        && d->state >= INSTANCIATED
        && d->state < STOPPED
   ); 
}

KJavaApplet::AppletState KJavaApplet::state() const {
    return d->state;
}

bool KJavaApplet::failed() const {
    return d->failed;
}

