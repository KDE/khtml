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

#include "kmultipart.h"

#include "httpfiltergzip_p.h"
#include <klocalizedstring.h>
#include <kjobuidelegate.h>
#include <kio/job.h>
#include <QtCore/QFile>
#include <qtemporaryfile.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kpluginfactory.h>
#include <khtml_part.h>
#include <unistd.h>
#include <kxmlguifactory.h>
#include <QtCore/QTimer>
#include <QVBoxLayout>
#include <kaboutdata.h>

static KAboutData kmultipartAboutData()
{
    KAboutData aboutData( "kmultipart", QString(), i18n("KMultiPart"),
                                            "0.1",
                                            i18n( "Embeddable component for multipart/mixed" ),
                                            KAboutData::License_GPL,
                                            i18n("Copyright 2001-2011, David Faure <email>faure@kde.org</email>"));
    return aboutData;
}

K_PLUGIN_FACTORY(KMultiPartFactory, registerPlugin<KMultiPart>();)
K_EXPORT_PLUGIN(KMultiPartFactory("kmultipart"))

//#define DEBUG_PARSING

class KLineParser
{
public:
    KLineParser() {
        m_lineComplete = false;
    }
    void addChar( char c, bool storeNewline ) {
        if ( !storeNewline && c == '\r' )
            return;
        Q_ASSERT( !m_lineComplete );
        if ( storeNewline || c != '\n' ) {
            int sz = m_currentLine.size();
            m_currentLine.resize( sz+1 );
            m_currentLine[sz] = c;
        }
        if ( c == '\n' )
            m_lineComplete = true;
    }
    bool isLineComplete() const {
        return m_lineComplete;
    }
    QByteArray currentLine() const {
        return m_currentLine;
    }
    void clearLine() {
        Q_ASSERT( m_lineComplete );
        reset();
    }
    void reset() {
        m_currentLine.resize( 0 );
        m_lineComplete = false;
    }
private:
    QByteArray m_currentLine;
    bool m_lineComplete; // true when ending with '\n'
};

/* testcase:
   Content-type: multipart/mixed;boundary=ThisRandomString

--ThisRandomString
Content-type: text/plain

Data for the first object.

--ThisRandomString
Content-type: text/plain

Data for the second and last object.

--ThisRandomString--
*/


KMultiPart::KMultiPart( QWidget *parentWidget,
                        QObject *parent, const QVariantList& )
    : KParts::ReadOnlyPart( parent )
{
    m_filter = 0L;

    setComponentData(kmultipartAboutData());

    QWidget *box = new QWidget( parentWidget );
    box->setLayout( new QVBoxLayout( box ) );
    setWidget( box );

    m_extension = new KParts::BrowserExtension( this );

    m_part = 0L;
    m_isHTMLPart = false;
    m_job = 0L;
    m_lineParser = new KLineParser;
    m_tempFile = 0;

    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL(timeout()), this, SLOT(slotProgressInfo()) );
}

KMultiPart::~KMultiPart()
{
    // important: delete the nested part before the part or qobject destructor runs.
    // we now delete the nested part which deletes the part's widget which makes
    // _OUR_ m_widget 0 which in turn avoids our part destructor to delete the
    // widget ;-)
    // ### additional note: it _can_ be that the part has been deleted before:
    // when we're in a html frameset and the view dies first, then it will also
    // kill the htmlpart
    if ( m_part )
        delete static_cast<KParts::ReadOnlyPart *>( m_part );
    delete m_job;
    delete m_lineParser;
    if ( m_tempFile ) {
        m_tempFile->setAutoRemove( true );
        delete m_tempFile;
    }
    delete m_filter;
    m_filter = 0L;
}


void KMultiPart::startHeader()
{
    m_bParsingHeader = true; // we expect a header to come first
    m_bGotAnyHeader = false;
    m_gzip = false;
    // just to be sure for now
    delete m_filter;
    m_filter = 0L;
}


bool KMultiPart::openUrl(const QUrl &url)
{
    setUrl(url);
    m_lineParser->reset();
    startHeader();

    //m_mimeType = arguments().mimeType();

    // Hmm, args.reload is set to true when reloading, but this doesn't seem to be enough...
    // I get "HOLD: Reusing held slave for <url>", and the old data

    m_job = KIO::get( url,
                      arguments().reload() ? KIO::Reload : KIO::NoReload,
                      KIO::HideProgressInfo );

    emit started( 0 /*m_job*/ ); // don't pass the job, it would interfere with our own infoMessage

    connect( m_job, SIGNAL(result(KJob*)),
             this, SLOT(slotJobFinished(KJob*)) );
    connect( m_job, SIGNAL(data(KIO::Job*,QByteArray)),
             this, SLOT(slotData(KIO::Job*,QByteArray)) );

    m_numberOfFrames = 0;
    m_numberOfFramesSkipped = 0;
    m_totalNumberOfFrames = 0;
    m_qtime.start();
    m_timer->start( 1000 ); //1s

    return true;
}

// Yes, libkdenetwork's has such a parser already (MultiPart),
// but it works on the complete string, expecting the whole data to be available....
// The version here is asynchronous.
void KMultiPart::slotData( KIO::Job *job, const QByteArray &data )
{
    if (m_boundary.isNull())
    {
       QString tmp = job->queryMetaData("media-boundary");
       // qDebug() << "Got Boundary from kio-http '" << tmp << "'";
       if ( !tmp.isEmpty() ) {
           // as per r437578, sometimes we se something like this:
           // Content-Type: multipart/x-mixed-replace; boundary=--myboundary
           // ..
           // --myboundary
           // e.g. the hashes specified in the header are extra. However,
           // we also see the following on the w3c bugzilla:
           // boundary="------- =_aaaaaaaaaa0"
           // ..
           //--------- =_aaaaaaaaaa0
           // e.g. the hashes are accurate. For now, we consider the quoted
           // case to be quirk-free, and only apply the -- stripping quirk
           // when we're unquoted.
           if (tmp.startsWith(QLatin1String("--")) &&
               job->queryMetaData("media-boundary-kio-quoted") != "true")
               m_boundary = tmp.toLatin1();
           else
               m_boundary = QByteArray("--")+tmp.toLatin1();
           m_boundaryLength = m_boundary.length();
       }
    }
    // Append to m_currentLine until eol
    for ( int i = 0; i < data.size() ; ++i )
    {
        // Store char. Skip if '\n' and currently parsing a header.
        m_lineParser->addChar( data[i], !m_bParsingHeader );
        if ( m_lineParser->isLineComplete() )
        {
            QByteArray line = m_lineParser->currentLine();
#ifdef DEBUG_PARSING
            // qDebug() << "line.size()=" << line.size();
#endif
#ifdef DEBUG_PARSING
            // qDebug() << "[" << m_bParsingHeader << "] line='" << line << "'";
#endif
            if ( m_bParsingHeader )
            {
                if ( !line.isEmpty() )
                    m_bGotAnyHeader = true;
                if ( m_boundary.isNull() )
                {
                    if ( !line.isEmpty() ) {
#ifdef DEBUG_PARSING
                        // qDebug() << "Boundary is " << line;
#endif
                        m_boundary = line;
                        m_boundaryLength = m_boundary.length();
                    }
                }
                else if ( !qstrnicmp( line.data(), "Content-Encoding:", 17 ) )
                {
                    QString encoding = QString::fromLatin1(line.data()+17).trimmed().toLower();
                    if (encoding == "gzip" || encoding == "x-gzip") {
                        m_gzip = true;
                    } else {
                        // qDebug() << "FIXME: unhandled encoding type in KMultiPart: " << encoding;
                    }
                }
                // parse Content-Type
                else if ( !qstrnicmp( line.data(), "Content-Type:", 13 ) )
                {
                    Q_ASSERT( m_nextMimeType.isNull() );
                    m_nextMimeType = QString::fromLatin1( line.data() + 14 ).trimmed();
                    int semicolon = m_nextMimeType.indexOf( ';' );
                    if ( semicolon != -1 )
                        m_nextMimeType = m_nextMimeType.left( semicolon );
                    // qDebug() << "m_nextMimeType=" << m_nextMimeType;
                }
                // Empty line, end of headers (if we had any header line before)
                else if ( line.isEmpty() && m_bGotAnyHeader )
                {
                    m_bParsingHeader = false;
#ifdef DEBUG_PARSING
                    // qDebug() << "end of headers";
#endif
                    startOfData();
                }
                // First header (when we know it from kio_http)
                else if ( line == m_boundary )
                    ; // nothing to do
                else if ( !line.isEmpty() ) { // this happens with e.g. Set-Cookie:
                    // qDebug() << "Ignoring header " << line;
                }
            } else {
                if ( !qstrncmp( line, m_boundary, m_boundaryLength ) )
                {
#ifdef DEBUG_PARSING
                    // qDebug() << "boundary found!";
                    // qDebug() << "after it is " << line.data() + m_boundaryLength;
#endif
                    // Was it the very last boundary ?
                    if ( !qstrncmp( line.data() + m_boundaryLength, "--", 2 ) )
                    {
#ifdef DEBUG_PARSING
                        // qDebug() << "Completed!";
#endif
                        endOfData();
                        emit completed();
                    } else
                    {
                        char nextChar = *(line.data() + m_boundaryLength);
#ifdef DEBUG_PARSING
                        // qDebug() << "KMultiPart::slotData nextChar='" << nextChar << "'";
#endif
                        if ( nextChar == '\n' || nextChar == '\r' ) {
                            endOfData();
                            startHeader();
                        }
                        else {
                            // otherwise, false hit, it has trailing stuff
                            sendData(line);
                        }
                    }
                } else {
                    // send to part
                    sendData(line);
                }
            }
            m_lineParser->clearLine();
        }
    }
}

void KMultiPart::setPart( const QString& mimeType )
{
    KXMLGUIFactory *guiFactory = factory();
    if ( guiFactory ) // seems to be 0 when restoring from SM
        guiFactory->removeClient( this );
    // qDebug() << "KMultiPart::setPart " << mimeType;
    delete m_part;
    // Try to find an appropriate viewer component
    m_part = KMimeTypeTrader::createPartInstanceFromQuery<KParts::ReadOnlyPart>
             ( m_mimeType, widget(), this );
    widget()->layout()->addWidget( m_part->widget() );
    if ( !m_part ) {
        // TODO launch external app
        KMessageBox::error( widget(), i18n("No handler found for %1.", m_mimeType) );
        return;
    }
    // By making the part a child XMLGUIClient of ours, we get its GUI merged in.
    insertChildClient( m_part );
    m_part->widget()->show();

    connect( m_part, SIGNAL(completed()),
             this, SLOT(slotPartCompleted()) );
    connect( m_part, SIGNAL(completed(bool)),
             this, SLOT(slotPartCompleted()) );

    m_isHTMLPart = ( mimeType == "text/html" );
    KParts::BrowserExtension* childExtension = KParts::BrowserExtension::childObject( m_part );

    if ( childExtension )
    {

        // Forward signals from the part's browser extension
        // this is very related (but not exactly like) KHTMLPart::processObjectRequest

        connect( childExtension, SIGNAL(openUrlNotify()),
                 m_extension, SIGNAL(openUrlNotify()) );

        connect( childExtension, SIGNAL(openUrlRequestDelayed(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)),
                 m_extension, SIGNAL(openUrlRequest(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)) );

        connect( childExtension, SIGNAL(createNewWindow(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)),
                 m_extension, SIGNAL(createNewWindow(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)) );

        // Keep in sync with khtml_part.cpp
        connect( childExtension, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             m_extension, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );
        connect( childExtension, SIGNAL(popupMenu(QPoint,QUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             m_extension, SIGNAL(popupMenu(QPoint,QUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );

        if ( m_isHTMLPart )
            connect( childExtension, SIGNAL(infoMessage(QString)),
                     m_extension, SIGNAL(infoMessage(QString)) );
        // For non-HTML we prefer to show our infoMessage ourselves.

        childExtension->setBrowserInterface( m_extension->browserInterface() );

        connect( childExtension, SIGNAL(enableAction(const char*,bool)),
                 m_extension, SIGNAL(enableAction(const char*,bool)) );
        connect( childExtension, SIGNAL(setLocationBarUrl(QString)),
                 m_extension, SIGNAL(setLocationBarUrl(QString)) );
        connect( childExtension, SIGNAL(setIconUrl(QUrl)),
                 m_extension, SIGNAL(setIconUrl(QUrl)) );
        connect( childExtension, SIGNAL(loadingProgress(int)),
                 m_extension, SIGNAL(loadingProgress(int)) );
        if ( m_isHTMLPart ) // for non-HTML we have our own
            connect( childExtension, SIGNAL(speedProgress(int)),
                     m_extension, SIGNAL(speedProgress(int)) );
        connect( childExtension, SIGNAL(selectionInfo(KFileItemList)),
                 m_extension, SIGNAL(selectionInfo(KFileItemList)) );
        connect( childExtension, SIGNAL(selectionInfo(QString)),
                 m_extension, SIGNAL(selectionInfo(QString)) );
        connect( childExtension, SIGNAL(selectionInfo(QList<QUrl>)),
                 m_extension, SIGNAL(selectionInfo(QList<QUrl>)) );
        connect( childExtension, SIGNAL(mouseOverInfo(KFileItem)),
                 m_extension, SIGNAL(mouseOverInfo(KFileItem)) );
        connect( childExtension, SIGNAL(moveTopLevelWidget(int,int)),
                 m_extension, SIGNAL(moveTopLevelWidget(int,int)) );
        connect( childExtension, SIGNAL(resizeTopLevelWidget(int,int)),
                 m_extension, SIGNAL(resizeTopLevelWidget(int,int)) );
    }

    m_partIsLoading = false;
    // Load the part's plugins too.
    // ###### This is a hack. The bug is that KHTMLPart doesn't load its plugins
    // if className != "Browser/View".
    loadPlugins( this, m_part, m_part->componentData() );
    // Get the part's GUI to appear
    if ( guiFactory )
        guiFactory->addClient( this );
}

void KMultiPart::startOfData()
{
    // qDebug() << "KMultiPart::startOfData";
    Q_ASSERT( !m_nextMimeType.isNull() );
    if( m_nextMimeType.isNull() )
        return;

    if ( m_gzip )
    {
        // We can't use KFilterDev because it assumes it can read as much data as necessary
        // from the underlying device. It's a pull strategy, while KMultiPart has to do
        // a push strategy.
        m_filter = new HTTPFilterGZip;
        connect(m_filter, SIGNAL(output(QByteArray)), this, SLOT(reallySendData(QByteArray)));
    }

    if ( m_mimeType != m_nextMimeType )
    {
        // Need to switch parts (or create the initial one)
        m_mimeType = m_nextMimeType;
        setPart( m_mimeType );
    }
    Q_ASSERT( m_part );
    // Pass args (e.g. reload)
    m_part->setArguments( arguments() );
    KParts::BrowserExtension* childExtension = KParts::BrowserExtension::childObject( m_part );
    if ( childExtension )
        childExtension->setBrowserArguments( m_extension->browserArguments() );

    m_nextMimeType.clear();
    if ( m_tempFile ) {
        m_tempFile->setAutoRemove( true );
        delete m_tempFile;
        m_tempFile = 0;
    }
    if ( m_isHTMLPart )
    {
        KHTMLPart* htmlPart = static_cast<KHTMLPart *>( static_cast<KParts::ReadOnlyPart *>( m_part ) );
        htmlPart->begin( url() );
    }
    else
    {
        // ###### TODO use a QByteArray and a data: URL instead
        m_tempFile = new QTemporaryFile;
        m_tempFile->open();
    }
}

void KMultiPart::sendData( const QByteArray& line )
{
    if ( m_filter )
    {
        m_filter->slotInput( line );
    }
    else
    {
        reallySendData( line );
    }
}

void KMultiPart::reallySendData( const QByteArray& line )
{
    if ( m_isHTMLPart )
    {
        KHTMLPart* htmlPart = static_cast<KHTMLPart *>( static_cast<KParts::ReadOnlyPart *>( m_part ) );
        htmlPart->write( line.data(), line.size() );
    }
    else if ( m_tempFile )
    {
        m_tempFile->write( line.data(), line.size() );
    }
}

void KMultiPart::endOfData()
{
    Q_ASSERT( m_part );
    if ( m_isHTMLPart )
    {
        KHTMLPart* htmlPart = static_cast<KHTMLPart *>( static_cast<KParts::ReadOnlyPart *>( m_part ) );
        htmlPart->end();
    } else if ( m_tempFile )
    {
        const QString tempFileName = m_tempFile->fileName();
        m_tempFile->close();
        if ( m_partIsLoading )
        {
            // The part is still loading the last data! Let it proceed then
            // Otherwise we'd keep canceling it, and nothing would ever show up...
            // qDebug() << "KMultiPart::endOfData part isn't ready, skipping frame";
            ++m_numberOfFramesSkipped;
            m_tempFile->setAutoRemove( true );
        }
        else
        {
            // qDebug() << "KMultiPart::endOfData opening " << tempFileName;
            QUrl url(tempFileName);
            m_partIsLoading = true;
            (void) m_part->openUrl( url );
        }
        delete m_tempFile;
        m_tempFile = 0L;
    }
}

void KMultiPart::slotPartCompleted()
{
    if ( !m_isHTMLPart )
    {
        Q_ASSERT( m_part );
        // Delete temp file used by the part
        Q_ASSERT( m_part->url().isLocalFile() );
	// qDebug() << "slotPartCompleted deleting " << m_part->url().toLocalFile();
        (void) unlink( QFile::encodeName( m_part->url().toLocalFile() ) );
        m_partIsLoading = false;
        ++m_numberOfFrames;
        // Do not emit completed from here.
    }
}

bool KMultiPart::closeUrl()
{
    m_timer->stop();
    if ( m_part )
        return m_part->closeUrl();
    return true;
}

void KMultiPart::guiActivateEvent( KParts::GUIActivateEvent * )
{
    // Not public!
    //if ( m_part )
    //    m_part->guiActivateEvent( e );
}

void KMultiPart::slotJobFinished( KJob *job )
{
    if ( job->error() )
    {
        // TODO use khtml's error:// scheme
        job->uiDelegate()->showErrorMessage();
        emit canceled( job->errorString() );
    }
    else
    {
        /*if ( m_khtml->view()->contentsY() == 0 )
        {
            const KParts::OpenUrlArguments args = arguments();
            m_khtml->view()->setContentsPos( args.xOffset(), args.yOffset() );
        }*/

        emit completed();

        //QTimer::singleShot( 0, this, SLOT(updateWindowCaption()) );
    }
    m_job = 0L;
}

void KMultiPart::slotProgressInfo()
{
    int time = m_qtime.elapsed();
    if ( !time ) return;
    if ( m_totalNumberOfFrames == m_numberOfFrames + m_numberOfFramesSkipped )
        return; // No change, don't overwrite statusbar messages if any
    //qDebug() << m_numberOfFrames << " in " << time << " milliseconds";
    QString str( "%1 frames per second, %2 frames skipped per second" );
    str = str.arg( 1000.0 * (double)m_numberOfFrames / (double)time );
    str = str.arg( 1000.0 * (double)m_numberOfFramesSkipped / (double)time );
    m_totalNumberOfFrames = m_numberOfFrames + m_numberOfFramesSkipped;
    //qDebug() << str;
    emit m_extension->infoMessage( str );
}


#if 0
KMultiPartBrowserExtension::KMultiPartBrowserExtension( KMultiPart *parent, const char *name )
    : KParts::BrowserExtension( parent, name )
{
    m_imgPart = parent;
}

int KMultiPartBrowserExtension::xOffset()
{
    return m_imgPart->doc()->view()->contentsX();
}

int KMultiPartBrowserExtension::yOffset()
{
    return m_imgPart->doc()->view()->contentsY();
}

void KMultiPartBrowserExtension::print()
{
    static_cast<KHTMLPartBrowserExtension *>( m_imgPart->doc()->browserExtension() )->print();
}

void KMultiPartBrowserExtension::reparseConfiguration()
{
    static_cast<KHTMLPartBrowserExtension *>( m_imgPart->doc()->browserExtension() )->reparseConfiguration();
    m_imgPart->doc()->setAutoloadImages( true );
}
#endif

#include "kmultipart.moc"
