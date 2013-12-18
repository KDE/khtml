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

#include "kjavaprocess.h"

#include <QDebug>
#include <kshell.h>
#include <kprotocolmanager.h>

#include <QtCore/QTextStream>
#include <QtCore/QMap>

class KJavaProcessPrivate
{
friend class KJavaProcess;
private:
    QString jvmPath;
    QString classPath;
    QString mainClass;
    QString extraArgs;
    QString classArgs;
    QMap<QString, QString> systemProps;
};

KJavaProcess::KJavaProcess( QObject* parent )
	: QProcess( parent ),
	d(new KJavaProcessPrivate)

{
    connect( this, SIGNAL(readyReadStandardOutput()),
             this, SLOT(slotReceivedData()) );
    connect( this, SIGNAL(finished(int,QProcess::ExitStatus)),
             this, SLOT(slotExited()) );
    connect( this, SIGNAL(error(QProcess::ProcessError)),
             this, SLOT(slotExited()) );

    d->jvmPath = "java";
    d->mainClass = "-help";
}

KJavaProcess::~KJavaProcess()
{
    if ( state() != NotRunning )
    {
        // qDebug() << "stopping java process";
        stopJava();
    }
    delete d;
}

bool KJavaProcess::isRunning()
{
   return state() != NotRunning;
}

bool KJavaProcess::startJava()
{
   return invokeJVM();
}

void KJavaProcess::stopJava()
{
   killJVM();
}

void KJavaProcess::setJVMPath( const QString& path )
{
   d->jvmPath = path;
}

void KJavaProcess::setClasspath( const QString& classpath )
{
    d->classPath = classpath;
}

void KJavaProcess::setSystemProperty( const QString& name,
                                      const QString& value )
{
   d->systemProps.insert( name, value );
}

void KJavaProcess::setMainClass( const QString& className )
{
   d->mainClass = className;
}

void KJavaProcess::setExtraArgs( const QString& args )
{
   d->extraArgs = args;
}

void KJavaProcess::setClassArgs( const QString& args )
{
   d->classArgs = args;
}

//Private Utility Functions used by the two send() methods
QByteArray KJavaProcess::addArgs( char cmd_code, const QStringList& args )
{
    //the buffer to store stuff, etc.
    QByteArray buff;
    QTextStream output( &buff, QIODevice::ReadWrite );
    const char sep = 0;

    //make space for the command size: 8 characters...
    const QByteArray space( "        " );
    output << space;

    //write command code
    output << cmd_code;

    //store the arguments...
    if( args.isEmpty() )
    {
        output << sep;
    }
    else
    {
        QStringList::ConstIterator it = args.begin();
        const QStringList::ConstIterator itEnd = args.end();
        for( ; it != itEnd; ++it )
        {
            if( !(*it).isEmpty() )
            {
                output << (*it).toLocal8Bit();
            }
            output << sep;
        }
    }

    return buff;
}

void KJavaProcess::storeSize( QByteArray* buff )
{
    const int size = buff->size() - 8;  //subtract out the length of the size_str
    const QString size_str = QString("%1").arg( size, 8 );
    // qDebug() << "KJavaProcess::storeSize, size = " << size_str;

    for( int i = 0; i < 8; ++i )
        buff->data()[ i ] = size_str[i].toLatin1();
}

void KJavaProcess::send( char cmd_code, const QStringList& args )
{
    if( isRunning() )
    {
        QByteArray buff = addArgs( cmd_code, args );
        storeSize( &buff );
        // qDebug() << "<KJavaProcess::send " << (int)cmd_code;
        write( buff );
    }
}

void KJavaProcess::send( char cmd_code, const QStringList& args,
                         const QByteArray& data )
{
    if( isRunning() )
    {
        // qDebug() << "KJavaProcess::send, qbytearray is size = " << data.size();

        QByteArray buff = addArgs( cmd_code, args );
        buff += data;

        storeSize( &buff );
        write( buff );
    }
}

bool KJavaProcess::invokeJVM()
{
    QStringList args;

    if( !d->classPath.isEmpty() )
    {
        args << "-classpath";
        args << d->classPath;
    }

    //set the system properties, iterate through the qmap of system properties
    QMap<QString,QString>::ConstIterator it = d->systemProps.constBegin();
    const QMap<QString,QString>::ConstIterator itEnd = d->systemProps.constEnd();

    for( ; it != itEnd; ++it )
    {
        if( !it.key().isEmpty() )
        {
            QString currarg = "-D" + it.key();
            if( !it.value().isEmpty() )
                currarg += '=' + it.value();
            args << currarg;
        }
    }

    //load the extra user-defined arguments
    if( !d->extraArgs.isEmpty() )
    {
        KShell::Errors err;
        args += KShell::splitArgs( d->extraArgs, KShell::AbortOnMeta, &err );
        if( err != KShell::NoError )
            qWarning() << "Extra args for JVM cannot be parsed, arguments = " << d->extraArgs;

    }

    args << d->mainClass;

    if ( !d->classArgs.isNull() )
        args << d->classArgs;

    // qDebug() << "Invoking JVM" << d->jvmPath << "now...with arguments = " << KShell::joinArgs(args);

    setProcessChannelMode(QProcess::SeparateChannels);
    start(d->jvmPath, args);

    return waitForStarted();
}

void KJavaProcess::killJVM()
{
   closeReadChannel( StandardOutput );
   terminate();
}

/*  In this method, read one command and send it to the d->appletServer
 *  then return, so we don't block the event handling
 */
void KJavaProcess::slotReceivedData()
{
    //read out the length of the message,
    //read the message and send it to the applet server
    char length[9] = { 0 };
    const int num_bytes = read( length, 8 );
    if( num_bytes == -1 )
    {
        qCritical() << "could not read 8 characters for the message length!!!!" << endl;
        return;
    }

    const QString lengthstr( length );
    bool ok;
    const int num_len = lengthstr.toInt( &ok );
    if( !ok )
    {
        qCritical() << "could not parse length out of: " << lengthstr << endl;
        return;
    }

    //now parse out the rest of the message.
    char* const msg = new char[num_len];
    const int num_bytes_msg = read( msg, num_len );
    if( num_bytes_msg == -1 || num_bytes_msg != num_len )
    {
        qCritical() << "could not read the msg, num_bytes_msg = " << num_bytes_msg << endl;
        delete[] msg;
        return;
    }

    emit received( QByteArray( msg, num_len ) );
    delete[] msg;
}

void KJavaProcess::slotExited()
{
    int status = -1;
    if ( exitStatus() == NormalExit ) {
     status = exitCode();
    }
    // qDebug() << "jvm exited with status " << status; 
    emit exited(status);
}

