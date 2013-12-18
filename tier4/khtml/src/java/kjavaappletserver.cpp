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

#include "kjavaappletserver.h"

#include "kjavaappletcontext.h"
#include "kjavaprocess.h"
#include "kjavadownloader.h"

#include <QDebug>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocalizedstring.h>
#include <kparts/browserextension.h>

#include <kio/job.h>
#include <kprotocolmanager.h>
#include <qsslcertificate.h>

#include <QtCore/QTimer>
#include <QtCore/QPointer>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QSslSocket>
#include <QApplication>
#include <QLabel>
#include <QDialog>
#include <QPushButton>
#include <QLayout>
#include <QtCore/QRegExp>

#include <stdlib.h>
#include <assert.h>
#include <QtCore/QAbstractEventDispatcher>
#include <qstandardpaths.h>

#define KJAS_CREATE_CONTEXT    (char)1
#define KJAS_DESTROY_CONTEXT   (char)2
#define KJAS_CREATE_APPLET     (char)3
#define KJAS_DESTROY_APPLET    (char)4
#define KJAS_START_APPLET      (char)5
#define KJAS_STOP_APPLET       (char)6
#define KJAS_INIT_APPLET       (char)7
#define KJAS_SHOW_DOCUMENT     (char)8
#define KJAS_SHOW_URLINFRAME   (char)9
#define KJAS_SHOW_STATUS       (char)10
#define KJAS_RESIZE_APPLET     (char)11
#define KJAS_GET_URLDATA       (char)12
#define KJAS_URLDATA           (char)13
#define KJAS_SHUTDOWN_SERVER   (char)14
#define KJAS_JAVASCRIPT_EVENT   (char)15
#define KJAS_GET_MEMBER        (char)16
#define KJAS_CALL_MEMBER       (char)17
#define KJAS_PUT_MEMBER        (char)18
#define KJAS_DEREF_OBJECT      (char)19
#define KJAS_AUDIOCLIP_PLAY    (char)20
#define KJAS_AUDIOCLIP_LOOP    (char)21
#define KJAS_AUDIOCLIP_STOP    (char)22
#define KJAS_APPLET_STATE      (char)23
#define KJAS_APPLET_FAILED     (char)24
#define KJAS_DATA_COMMAND      (char)25
#define KJAS_PUT_URLDATA       (char)26
#define KJAS_PUT_DATA          (char)27
#define KJAS_SECURITY_CONFIRM  (char)28
#define KJAS_SHOW_CONSOLE      (char)29


class JSStackFrame;

typedef QMap< int, KJavaKIOJob* > KIOJobMap;
typedef QMap< int, JSStackFrame* > JSStack;

class JSStackFrame {
public:
    JSStackFrame(JSStack & stack, QStringList & a)
    : jsstack(stack), args(a), ticket(counter++), ready(false), exit (false) {
        jsstack.insert( ticket, this );
    }
    ~JSStackFrame() {
        jsstack.remove( ticket );
    }
    JSStack & jsstack;
    QStringList & args;
    int ticket;
    bool ready : 1;
    bool exit : 1;
    static int counter;
};

int JSStackFrame::counter = 0;

class KJavaAppletServerPrivate
{
friend class KJavaAppletServer;
private:
   KJavaAppletServerPrivate() {}
   ~KJavaAppletServerPrivate() {
   }
   int counter;
   QMap< int, QPointer<KJavaAppletContext> > contexts;
   QString appletLabel;
   JSStack jsstack;
   KIOJobMap kiojobs;
   bool javaProcessFailed;
   bool useKIO;
   //int locked_context;
   //QValueList<QByteArray> java_requests;
};

static KJavaAppletServer* self = 0;

KJavaAppletServer::KJavaAppletServer()
    : d(new KJavaAppletServerPrivate)
{
    process = new KJavaProcess();

    connect( process, SIGNAL(received(QByteArray)),
             this,    SLOT(slotJavaRequest(QByteArray)) );

    setupJava( process );

    if( process->startJava() ) {
        d->appletLabel = i18n( "Loading Applet" );
        d->javaProcessFailed = false;
    }
    else {
        d->appletLabel = i18n( "Error: java executable not found" );
        d->javaProcessFailed = true;
    }
}

KJavaAppletServer::~KJavaAppletServer()
{
    disconnect(process, 0, 0, 0); // first disconnect from process.
    quit();

    delete process;
    process = 0;
    delete d;
}

QString KJavaAppletServer::getAppletLabel()
{
    if( self )
        return self->appletLabel();
    else
        return QString();
}

QString KJavaAppletServer::appletLabel()
{
    return d->appletLabel;
}

KJavaAppletServer* KJavaAppletServer::allocateJavaServer()
{
   if( self == 0 )
   {
      self = new KJavaAppletServer();
      self->d->counter = 0;
   }

   ++(self->d->counter);
   return self;
}

void KJavaAppletServer::freeJavaServer()
{
    --(self->d->counter);

    if( self->d->counter == 0 )
    {
        //instead of immediately quitting here, set a timer to kill us
        //if there are still no servers- give us one minute
        //this is to prevent repeated loading and unloading of the jvm
        KConfig config( "konquerorrc" );
        KConfigGroup group = config.group( "Java/JavaScript Settings" );
        if( group.readEntry( "ShutdownAppletServer", true )  )
        {
            const int value = group.readEntry( "AppletServerTimeout", 60 );
            QTimer::singleShot( value*1000, self, SLOT(checkShutdown()) );
        }
    }
}

void KJavaAppletServer::checkShutdown()
{
    if( self->d->counter == 0 )
    {
        delete self;
        self = 0;
    }
}

void KJavaAppletServer::setupJava( KJavaProcess *p )
{
    KConfig configFile ( "konquerorrc" );
    KConfigGroup config(&configFile, "Java/JavaScript Settings" );

    QString jvm_path = "java";

    QString jPath = config.readPathEntry( "JavaPath", QString() );
    if ( !jPath.isEmpty() && jPath != "java" )
    {
        // Cut off trailing slash if any
        if( jPath[jPath.length()-1] == '/' )
            jPath.remove(jPath.length()-1, 1);

        QDir dir( jPath );
        if( dir.exists( "bin/java" ) )
        {
            jvm_path = jPath + "/bin/java";
        }
        else if (dir.exists( "/jre/bin/java" ) )
        {
            jvm_path = jPath + "/jre/bin/java";
        }
        else if( QFile::exists(jPath) )
        {
            //check here to see if they entered the whole path the java exe
            jvm_path = jPath;
        }
    }

    //check to see if jvm_path is valid and set d->appletLabel accordingly
    p->setJVMPath( jvm_path );

    // Prepare classpath variable
    QString kjava_class = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kjava/kjava.jar");
    // qDebug() << "kjava_class = " << kjava_class;
    if( kjava_class.isNull() ) // Should not happen
        return;

    QDir dir( kjava_class );
    dir.cdUp();
    // qDebug() << "dir = " << dir.absolutePath();

    const QStringList entries = dir.entryList(QDir::nameFiltersFromString( "*.jar" ));
    // qDebug() << "entries = " << entries.join( ":" );

    QString classes;
    {
        QStringList::ConstIterator it = entries.begin();
        const QStringList::ConstIterator itEnd = entries.end();
        for( ; it != itEnd; ++it )
        {
            if( !classes.isEmpty() )
                classes += ':';
            classes += dir.absoluteFilePath( *it );
        }
    }
    p->setClasspath( classes );

    // Fix all the extra arguments
    const QString extraArgs = config.readEntry( "JavaArgs" );
    p->setExtraArgs( extraArgs );

    if( config.readEntry( "UseSecurityManager", true ) )
    {
        QString class_file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kjava/kjava.policy" );
        p->setSystemProperty( "java.security.policy", class_file );

        p->setSystemProperty( "java.security.manager",
                              "org.kde.kjas.server.KJASSecurityManager" );
    }

    d->useKIO = config.readEntry("UseKio", false);
    if( d->useKIO )
    {
        p->setSystemProperty( "kjas.useKio", QString() );
    }

    //check for http proxies...
    if( KProtocolManager::useProxy() )
    {
        // only proxyForUrl honors automatic proxy scripts
        // we do not know the applet url here so we just use a dummy url
        // this is a workaround for now
        // FIXME
        const QUrl dummyURL( "http://www.kde.org/" );
        const QString httpProxy = KProtocolManager::proxyForUrl(dummyURL);
        // qDebug() << "httpProxy is " << httpProxy;

        const QUrl url( httpProxy );
        p->setSystemProperty( "http.proxyHost", url.host() );
        p->setSystemProperty( "http.proxyPort", QString::number( url.port() ) );
    }

    //set the main class to run
    p->setMainClass( "org.kde.kjas.server.Main" );
}

void KJavaAppletServer::createContext( int contextId, KJavaAppletContext* context )
{
//    qDebug() << "createContext: " << contextId;
    if ( d->javaProcessFailed ) return;

    d->contexts.insert( contextId, context );

    QStringList args;
    args.append( QString::number( contextId ) );
    process->send( KJAS_CREATE_CONTEXT, args );
}

void KJavaAppletServer::destroyContext( int contextId )
{
//    qDebug() << "destroyContext: " << contextId;
    if ( d->javaProcessFailed ) return;
    d->contexts.remove( contextId );

    QStringList args;
    args.append( QString::number( contextId ) );
    process->send( KJAS_DESTROY_CONTEXT, args );
}

bool KJavaAppletServer::createApplet( int contextId, int appletId,
                             const QString & name, const QString & clazzName,
                             const QString & baseURL, const QString & user,
                             const QString & password, const QString & authname,
                             const QString & codeBase, const QString & jarFile,
                             QSize size, const QMap<QString,QString>& params,
                             const QString & windowTitle )
{
//    qDebug() << "createApplet: contextId = " << contextId     << endl
//              << "              appletId  = " << appletId      << endl
//              << "              name      = " << name          << endl
//              << "              clazzName = " << clazzName     << endl
//              << "              baseURL   = " << baseURL       << endl
//              << "              codeBase  = " << codeBase      << endl
//              << "              jarFile   = " << jarFile       << endl
//              << "              width     = " << size.width()  << endl
//              << "              height    = " << size.height() << endl;

    if ( d->javaProcessFailed ) return false;

    QStringList args;
    args.append( QString::number( contextId ) );
    args.append( QString::number( appletId ) );

    //it's ok if these are empty strings, I take care of it later...
    args.append( name );
    args.append( clazzName );
    args.append( baseURL );
    args.append( user );
    args.append( password );
    args.append( authname );
    args.append( codeBase );
    args.append( jarFile );

    args.append( QString::number( size.width() ) );
    args.append( QString::number( size.height() ) );

    args.append( windowTitle );

    //add on the number of parameter pairs...
    const int num = params.count();
    const QString num_params = QString("%1").arg( num, 8 );
    args.append( num_params );

    QMap< QString, QString >::ConstIterator it = params.begin();
    const QMap< QString, QString >::ConstIterator itEnd = params.end();

    for( ; it != itEnd; ++it )
    {
        args.append( it.key() );
        args.append( it.value() );
    }

    process->send( KJAS_CREATE_APPLET, args );

    return true;
}

void KJavaAppletServer::initApplet( int contextId, int appletId )
{
    if ( d->javaProcessFailed ) return;
    QStringList args;
    args.append( QString::number( contextId ) );
    args.append( QString::number( appletId ) );

    process->send( KJAS_INIT_APPLET, args );
}

void KJavaAppletServer::destroyApplet( int contextId, int appletId )
{
    if ( d->javaProcessFailed ) return;
    QStringList args;
    args.append( QString::number(contextId) );
    args.append( QString::number(appletId) );

    process->send( KJAS_DESTROY_APPLET, args );
}

void KJavaAppletServer::startApplet( int contextId, int appletId )
{
    if ( d->javaProcessFailed ) return;
    QStringList args;
    args.append( QString::number(contextId) );
    args.append( QString::number(appletId) );

    process->send( KJAS_START_APPLET, args );
}

void KJavaAppletServer::stopApplet( int contextId, int appletId )
{
    if ( d->javaProcessFailed ) return;
    QStringList args;
    args.append( QString::number(contextId) );
    args.append( QString::number(appletId) );

    process->send( KJAS_STOP_APPLET, args );
}

void KJavaAppletServer::showConsole() {
    if ( d->javaProcessFailed ) return;
    QStringList args;
    process->send( KJAS_SHOW_CONSOLE, args );
}

void KJavaAppletServer::sendURLData( int loaderID, int code, const QByteArray& data )
{
    QStringList args;
    args.append( QString::number(loaderID) );
    args.append( QString::number(code) );

    process->send( KJAS_URLDATA, args, data );
}

void KJavaAppletServer::removeDataJob( int loaderID )
{
    const KIOJobMap::iterator it = d->kiojobs.find( loaderID );
    if (it != d->kiojobs.end()) {
        it.value()->deleteLater();
        d->kiojobs.erase( it );
    }
}

void KJavaAppletServer::quit()
{
    const QStringList args;

    process->send( KJAS_SHUTDOWN_SERVER, args );
    process->waitForFinished( 10000 );
}

void KJavaAppletServer::slotJavaRequest( const QByteArray& qb )
{
    // qb should be one command only without the length string,
    // we parse out the command and it's meaning here...
    QString cmd;
    QStringList args;
    int index = 0;
    const int qb_size = qb.size();

    //get the command code
    const char cmd_code = qb[ index++ ];
    ++index; //skip the next sep

    //get contextID
    QString contextID;
    while( index < qb_size && qb[index] != 0 )
    {
        contextID += qb[ index++ ];
    }
    bool ok;
    const int ID_num = contextID.toInt( &ok ); // context id or kio job id
    /*if (d->locked_context > -1 &&
        ID_num != d->locked_context &&
        (cmd_code == KJAS_JAVASCRIPT_EVENT ||
         cmd_code == KJAS_APPLET_STATE ||
         cmd_code == KJAS_APPLET_FAILED))
    {
        / * Don't allow requests from other contexts if we're waiting
         * on a return value that can trigger JavaScript events
         * /
        d->java_requests.push_back(qb);
        return;
    }*/
    ++index; //skip the sep

    if (cmd_code == KJAS_PUT_DATA) {
        // rest of the data is for kio put
        if (ok) {
            KIOJobMap::iterator it = d->kiojobs.find( ID_num );
            if (ok && it != d->kiojobs.end()) {
                QByteArray qba;
                qba = QByteArray::fromRawData(qb.data() + index, qb.size() - index - 1);
                it.value()->data(qba);
                qba = QByteArray::fromRawData(qb.data() + index, qb.size() - index - 1);
            }
            // qDebug() << "PutData(" << ID_num << ") size=" << qb.size() - index;
        } else
            qCritical() << "PutData error " << ok << endl;
        return;
    }
    //now parse out the arguments
    while( index < qb_size )
    {
        int sep_pos = qb.indexOf( (char) 0, index );
        if (sep_pos < 0) {
            qCritical() << "Missing separation byte" << endl;
            sep_pos = qb_size;
        }
        //qDebug() << "KJavaAppletServer::slotJavaRequest: "<< QString::fromLocal8Bit( qb.data() + index, sep_pos - index );
        args.append( QString::fromLocal8Bit( qb.data() + index, sep_pos - index ) );
        index = sep_pos + 1; //skip the sep
    }
    //here I should find the context and call the method directly
    //instead of emitting signals
    switch( cmd_code )
    {
        case KJAS_SHOW_DOCUMENT:
            cmd = QLatin1String( "showdocument" );
            break;

        case KJAS_SHOW_URLINFRAME:
            cmd = QLatin1String( "showurlinframe" );
            break;

        case KJAS_SHOW_STATUS:
            cmd = QLatin1String( "showstatus" );
            break;

        case KJAS_RESIZE_APPLET:
            cmd = QLatin1String( "resizeapplet" );
            break;

        case KJAS_GET_URLDATA:
            if (ok && !args.empty() ) {
                d->kiojobs.insert(ID_num, new KJavaDownloader(ID_num, args.first()));
                // qDebug() << "GetURLData(" << ID_num << ") url=" << args.first();
            } else
                qCritical() << "GetURLData error " << ok << " args:" << args.size() << endl;
            return;
        case KJAS_PUT_URLDATA:
            if (ok && !args.empty()) {
                KJavaUploader* const job = new KJavaUploader(ID_num, args.first());
                d->kiojobs.insert(ID_num, job);
                job->start();
                // qDebug() << "PutURLData(" << ID_num << ") url=" << args.first();
            } else
                qCritical() << "PutURLData error " << ok << " args:" << args.size() << endl;
            return;
        case KJAS_DATA_COMMAND:
            if (ok && !args.empty()) {
                const int cmd = args.first().toInt( &ok );
                KIOJobMap::iterator it = d->kiojobs.find( ID_num );
                if (ok && it != d->kiojobs.end())
                    it.value()->jobCommand( cmd );
                // qDebug() << "KIO Data command: " << ID_num << " " << args.first();
            } else
                qCritical() << "KIO Data command error " << ok << " args:" << args.size() << endl;
            return;
        case KJAS_JAVASCRIPT_EVENT:
            cmd = QLatin1String( "JS_Event" );

            if(!args.empty()) {
                 // qDebug() << "Javascript request: "<< contextID
                 //             << " code: " << args[0] << endl;
            } else {
                qCritical() << "Expected args not to be empty!" << endl;
            }

            break;
        case KJAS_GET_MEMBER:
        case KJAS_PUT_MEMBER:
        case KJAS_CALL_MEMBER: {
            if(!args.empty()) {
                const int ticket = args[0].toInt();
                JSStack::iterator it = d->jsstack.find(ticket);
                if (it != d->jsstack.end()) {
                    // qDebug() << "slotJavaRequest: " << ticket;
                    args.pop_front();
                    it.value()->args.operator=(args); // just in case ..
                    it.value()->ready = true;
                    it.value()->exit = true;
                } else {
                    // qDebug() << "Error: Missed return member data";
                }
            } else {
                qCritical() << "Expected args not to be empty!" << endl;
            }
            return;
        }
        case KJAS_AUDIOCLIP_PLAY:
            cmd = QLatin1String( "audioclip_play" );
            if(!args.empty()) {
                // qDebug() << "Audio Play: url=" << args[0];
            } else
                qCritical() << "Expected args not to be empty!" << endl;

            break;
        case KJAS_AUDIOCLIP_LOOP:
            cmd = QLatin1String( "audioclip_loop" );
            if(!args.empty()) {
                // qDebug() << "Audio Loop: url=" << args[0];
            } else
                qCritical() << "Expected args not to be empty!" << endl;

            break;
        case KJAS_AUDIOCLIP_STOP:
            cmd = QLatin1String( "audioclip_stop" );
            if(!args.empty()) {
                // qDebug() << "Audio Stop: url=" << args[0];
            } else
                qCritical() << "Expected args not to be empty!" << endl;

            break;
        case KJAS_APPLET_STATE:
            if(args.size() > 1) {
                // qDebug() << "Applet State Notification for Applet " << args[0] << ". New state=" << args[1];
            } else
                qCritical() << "Expected args not to be empty!" << endl;

            cmd = QLatin1String( "AppletStateNotification" );
            break;
        case KJAS_APPLET_FAILED:
            if(args.size() > 1) {
                // qDebug() << "Applet " << args[0] << " Failed: " << args[1];
            } else
                qCritical() << "Expected args not to be empty!" << endl;

            cmd = QLatin1String( "AppletFailed" );
            break;
        case KJAS_SECURITY_CONFIRM: {
            QStringList sl;
            QString answer( "invalid" );

            if (!QSslSocket::supportsSsl()) {
                answer = "nossl";
            } else if (args.size() > 2) {
                const int certsnr = args[1].toInt();
                Q_ASSERT(args.size() > certsnr + 1);
                QString text;
                for (int i = certsnr - 1; i >= 0; --i) {
                    const QByteArray &arg = args[i + 2].toLatin1();
                    QSslCertificate cert(arg);
                    if (!cert.isNull()) {
#if 0 // KDE 5 TODO: finish port
                        if (cert.isSigner())
                            text += i18n("Signed by (validation: %1)", KSSLCertificate::verifyText(cert.validate()));
                        else
                            text += i18n("Certificate (validation: %1)", KSSLCertificate::verifyText(cert.validate()));
                        text += "\n";
                        QString subject = cert.getSubject() + QChar('\n');
                        QRegExp reg(QString("/[A-Z]+="));
                        int pos = 0;
                        while ((pos = subject.indexOf(reg, pos)) > -1)
                            subject.replace(pos, 1, QString("\n    "));
                        text += subject.mid(1);
#else
                        text += "TODO Security confirm";
#endif
                    }
                }
                // qDebug() << "Security confirm " << args.first() << certs.count();
                if ( !text.isEmpty() ) {
                    answer = PermissionDialog( qApp->activeWindow() ).exec( text, args[0] );
                }
            }
            sl.push_front( answer );
            sl.push_front( QString::number(ID_num) );
            process->send( KJAS_SECURITY_CONFIRM, sl );
            return;
        }
        default:
            return;
            break;
    }


    if( !ok )
    {
        qCritical() << "could not parse out contextID to call command on" << endl;
        return;
    }

    KJavaAppletContext* const context = d->contexts[ ID_num ];
    if( context )
        context->processCmd( cmd, args );
    else if (cmd != "AppletStateNotification")
        qCritical() << "no context object for this id" << endl;
}

void KJavaAppletServer::killTimers()
{
    QAbstractEventDispatcher::instance()->unregisterTimers(this);
}

void KJavaAppletServer::endWaitForReturnData() {
    // qDebug() << "KJavaAppletServer::endWaitForReturnData";
    killTimers();
    JSStack::iterator it = d->jsstack.begin();
    JSStack::iterator itEnd = d->jsstack.end();
    for (; it != itEnd; ++it)
        it.value()->exit = true;
}

void KJavaAppletServer::timerEvent(QTimerEvent *) {
    endWaitForReturnData();
    // qDebug() << "KJavaAppletServer::timerEvent timeout";
}

void KJavaAppletServer::waitForReturnData(JSStackFrame * frame) {
    // qDebug() << ">KJavaAppletServer::waitForReturnData";
    killTimers();
    startTimer(15000);
    while (!frame->exit) {
        QAbstractEventDispatcher::instance()->processEvents (QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
    }
    if (d->jsstack.size() <= 1)
        killTimers();
    // qDebug() << "<KJavaAppletServer::waitForReturnData stacksize:" << d->jsstack.size();
}

bool KJavaAppletServer::getMember(QStringList & args, QStringList & ret_args) {
    JSStackFrame frame( d->jsstack, ret_args );
    args.push_front( QString::number(frame.ticket) );

    process->send( KJAS_GET_MEMBER, args );
    waitForReturnData( &frame );

    return frame.ready;
}

bool KJavaAppletServer::putMember( QStringList & args ) {
    QStringList ret_args;
    JSStackFrame frame( d->jsstack, ret_args );
    args.push_front( QString::number(frame.ticket) );

    process->send( KJAS_PUT_MEMBER, args );
    waitForReturnData( &frame );

    return frame.ready && ret_args.count() > 0 && ret_args[0].toInt();
}

bool KJavaAppletServer::callMember(QStringList & args, QStringList & ret_args) {
    JSStackFrame frame( d->jsstack, ret_args );
    args.push_front( QString::number(frame.ticket) );

    process->send( KJAS_CALL_MEMBER, args );
    waitForReturnData( &frame );

    return frame.ready;
}

void KJavaAppletServer::derefObject( QStringList & args ) {
    process->send( KJAS_DEREF_OBJECT, args );
}

bool KJavaAppletServer::usingKIO() {
    return d->useKIO;
}


PermissionDialog::PermissionDialog( QWidget* parent )
    : QObject(parent), m_button("no")
{}

QString PermissionDialog::exec( const QString & cert, const QString & perm ) {
    QPointer<QDialog> dialog = new QDialog( static_cast<QWidget*>(parent()) );

    dialog->setObjectName("PermissionDialog");
    QSizePolicy sizeplcy( QSizePolicy::Minimum, QSizePolicy::Minimum);
    sizeplcy.setHeightForWidth(dialog->sizePolicy().hasHeightForWidth());
    dialog->setSizePolicy(sizeplcy);
    dialog->setModal( true );
    dialog->setWindowTitle( i18n("Security Alert") );

    QVBoxLayout* const dialogLayout = new QVBoxLayout( dialog );
    dialogLayout->setObjectName("dialogLayout");

    dialogLayout->addWidget( new QLabel( i18n("Do you grant Java applet with certificate(s):"), dialog ) );
    dialogLayout->addWidget( new QLabel( cert, dialog ) );
    dialogLayout->addWidget( new QLabel( i18n("the following permission"), dialog ) );
    dialogLayout->addWidget( new QLabel( perm, dialog ) );
    QSpacerItem* const spacer2 = new QSpacerItem( 20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding );
    dialogLayout->addItem( spacer2 );

    QHBoxLayout* const buttonLayout = new QHBoxLayout();
    buttonLayout->setMargin(0);
    buttonLayout->setObjectName("buttonLayout");

    QPushButton* const no = new QPushButton( i18n("&No"), dialog );
    no->setDefault( true );
    buttonLayout->addWidget( no );

    QPushButton* const reject = new QPushButton( i18n("&Reject All"), dialog );
    buttonLayout->addWidget( reject );

    QPushButton* const yes = new QPushButton( i18n("&Yes"), dialog );
    buttonLayout->addWidget( yes );

    QPushButton* const grant = new QPushButton( i18n("&Grant All"), dialog );
    buttonLayout->addWidget( grant );
    dialogLayout->addLayout( buttonLayout );
    dialog->resize( dialog->minimumSizeHint() );
    //clearWState( WState_Polished );

    connect( no, SIGNAL(clicked()), this, SLOT(clicked()) );
    connect( reject, SIGNAL(clicked()), this, SLOT(clicked()) );
    connect( yes, SIGNAL(clicked()), this, SLOT(clicked()) );
    connect( grant, SIGNAL(clicked()), this, SLOT(clicked()) );

    dialog->exec();
    delete dialog;

    return m_button;
}

PermissionDialog::~PermissionDialog()
{}

void PermissionDialog::clicked()
{
    m_button = sender()->objectName();
    static_cast<const QWidget*>(sender())->parentWidget()->close();
}

