/*
    This file is part of the KDE libraries

    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
              (C) 2001-2003 Dirk Mueller (mueller@kde.org)
              (C) 2002 Waldo Bastian (bastian@kde.org)
              (C) 2003 Apple Computer, Inc.
              (C) 2006-2010 Germain Garand (germain@ebooksfrance.org)

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

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.

    // regarding the LRU:
    // http://www.is.kyusan-u.ac.jp/~chengk/pub/papers/compsac00_A07-07.pdf
*/

#undef CACHE_DEBUG
//#define CACHE_DEBUG

#ifdef CACHE_DEBUG
#define CDEBUG qDebug()
#else
#define CDEBUG qDebug()
#endif

#undef LOADER_DEBUG
//#define LOADER_DEBUG

//#define PRELOAD_DEBUG

#include "loader.h"
#include "seed.h"
#include "woff.h"
#include <imload/imagepainter.h>
#include <imload/imagemanager.h>
#include <kfilterdev.h>

#include <assert.h>

// default cache size
#define DEFCACHESIZE 2096*1024

//#include <qasyncio.h>
//#include <qasyncimageio.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QBitmap>
#include <QMovie>
#include <QWidget>
#include <QtCore/QDebug>
#include <kurlauthorized.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/jobclasses.h>
#include <kio/scheduler.h>
#include <kcharsets.h>
#include <kiconloader.h>
#include <QDebug>
#include <kjobwidgets.h>

#include <khtml_global.h>
#include <khtml_part.h>

#ifdef IMAGE_TITLES
#include <qfile.h>
#include <kfilemetainfo.h>
#include <qtemporaryfile.h>
#endif

#include "html/html_documentimpl.h"
#include "css/css_stylesheetimpl.h"
#include "xml/dom_docimpl.h"

#include "blocked_icon.cpp"

#include <QPaintEngine>

using namespace khtml;
using namespace DOM;
using namespace khtmlImLoad;

#define MAX_LRU_LISTS 20
struct LRUList {
    CachedObject* m_head;
    CachedObject* m_tail;

    LRUList() : m_head(0), m_tail(0) {}
};

static LRUList m_LRULists[MAX_LRU_LISTS];
static LRUList* getLRUListFor(CachedObject* o);

CachedObjectClient::~CachedObjectClient()
{
}

CachedObject::~CachedObject()
{
    Cache::removeFromLRUList(this);
}

void CachedObject::finish()
{
    m_status = Cached;
}

bool CachedObject::isExpired() const
{
    if (!m_expireDate.isValid()) return false;
    QDateTime now = QDateTime::currentDateTime();
    return (now >= m_expireDate);
}

void CachedObject::setRequest(Request *_request)
{
    if ( _request && !m_request )
        m_status = Pending;

    if ( allowInLRUList() )
        Cache::removeFromLRUList( this );

    m_request = _request;

    if ( allowInLRUList() )
        Cache::insertInLRUList( this );
}

void CachedObject::ref(CachedObjectClient *c)
{
    if (m_preloadResult == PreloadNotReferenced) {
        if (isLoaded())
            m_preloadResult = PreloadReferencedWhileComplete;
        else if (m_prospectiveRequest)
            m_preloadResult = PreloadReferencedWhileLoading;
        else
            m_preloadResult = PreloadReferenced;
    }
    // unfortunately we can be ref'ed multiple times from the
    // same object,  because it uses e.g. the same foreground
    // and the same background picture. so deal with it.
    // Hence the use of a QHash rather than of a QSet.
    m_clients.insertMulti(c,c);
    Cache::removeFromLRUList(this);
    m_accessCount++;
}

void CachedObject::deref(CachedObjectClient *c)
{
    assert( c );
    assert( m_clients.count() );
    assert( !canDelete() );
    assert( m_clients.contains( c ) );

    Cache::flush();

    m_clients.take(c);

    if (allowInLRUList())
        Cache::insertInLRUList(this);
}

void CachedObject::setSize(int size)
{
    bool sizeChanged;

    if ( !m_next && !m_prev && getLRUListFor(this)->m_head != this )
        sizeChanged = false;
    else
        sizeChanged = ( size - m_size ) != 0;

    // The object must now be moved to a different queue,
    // since its size has been changed.
    if ( sizeChanged  && allowInLRUList())
        Cache::removeFromLRUList(this);

    m_size = size;

    if ( sizeChanged && allowInLRUList())
        Cache::insertInLRUList(this);
}

QTextCodec* CachedObject::codecForBuffer( const QString& charset, const QByteArray& buffer ) const
{
    // we don't use heuristicContentMatch here since it is a) far too slow and
    // b) having too much functionality for our case.

    uchar* d = ( uchar* ) buffer.data();
    int s = buffer.size();

    // BOM
    if ( s >= 3 &&
         d[0] == 0xef && d[1] == 0xbb && d[2] == 0xbf)
         return QTextCodec::codecForMib( 106 ); // UTF-8

    if ( s >= 2 && ((d[0] == 0xff && d[1] == 0xfe) ||
                    (d[0] == 0xfe && d[1] == 0xff)))
        return QTextCodec::codecForMib( 1000 ); // UCS-2

    // Link or @charset
    if(!charset.isEmpty())
    {
	QTextCodec* c = KCharsets::charsets()->codecForName(charset);
        if(c->mibEnum() == 11)  {
            // iso8859-8 (visually ordered)
            c = QTextCodec::codecForName("iso8859-8-i");
        }
        return c;
    }

    // Default
    return QTextCodec::codecForMib( 4 ); // latin 1
}

// -------------------------------------------------------------------------------------------

CachedCSSStyleSheet::CachedCSSStyleSheet(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy,
					 const char *accept)
    : CachedObject(url, CSSStyleSheet, _cachePolicy, 0)
{
    // Set the type we want (probably css or xml)
    QString ah = QLatin1String( accept );
    if ( !ah.isEmpty() )
        ah += ',';
    ah += "*/*;q=0.1";
    setAccept( ah );
    m_hadError = false;
    m_wasBlocked = false;
    m_err = 0;
    // load the file.
    // Style sheets block rendering, they are therefore the higher priority item.
    // Do |not| touch the priority value unless you conducted thorough tests and
    // can back your choice with meaningful data, testing page load time and
    // time to first paint.
    Cache::loader()->load(dl, this, false, -8);
    m_loading = true;
}

CachedCSSStyleSheet::CachedCSSStyleSheet(const DOMString &url, const QString &stylesheet_data)
    : CachedObject(url, CSSStyleSheet, KIO::CC_Verify, stylesheet_data.length())
{
    m_loading = false;
    m_status = Persistent;
    m_sheet = DOMString(stylesheet_data);
}

bool khtml::isAcceptableCSSMimetype( const DOM::DOMString& mimetype )
{
    // matches Mozilla's check (cf. nsCSSLoader.cpp)
    return mimetype.isEmpty() || mimetype == "text/css" || mimetype == "application/x-unknown-content-type";
}

void CachedCSSStyleSheet::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);

    if (!m_loading) {
	if (m_hadError)
	    c->error( m_err, m_errText );
	else
	    c->setStyleSheet( m_url, m_sheet, m_charset, m_mimetype );
    }
}

void CachedCSSStyleSheet::data( QBuffer &buffer, bool eof )
{
    if(!eof) return;
    buffer.close();
    setSize(buffer.buffer().size());

    m_charset = checkCharset( buffer.buffer() );
    QTextCodec* c = 0;
    if (!m_charset.isEmpty()) {
        c = KCharsets::charsets()->codecForName(m_charset);
        if(c->mibEnum() == 11)  c = QTextCodec::codecForName("iso8859-8-i");
    }
    else {
        c = codecForBuffer( m_charsetHint, buffer.buffer() );
        m_charset = c->name();
    }
    QString data = c->toUnicode( buffer.buffer().data(), m_size );
    // workaround Qt bugs
    m_sheet = static_cast<QChar>(data[0]) == QChar::ByteOrderMark ? DOMString(data.mid( 1 ) ) : DOMString(data);
    m_loading = false;

    checkNotify();
}

void CachedCSSStyleSheet::checkNotify()
{
    if(m_loading || m_hadError) return;

    CDEBUG << "finishedLoading" << m_url.string();

    for (QHashIterator<CachedObjectClient*,CachedObjectClient*> it( m_clients ); it.hasNext();)
        it.next().value()->setStyleSheet( m_url, m_sheet, m_charset, m_mimetype );
}


void CachedCSSStyleSheet::error( int err, const char* text )
{
    m_hadError = true;
    m_err = err;
    m_errText = text;
    m_loading = false;

    for (QHashIterator<CachedObjectClient*,CachedObjectClient*> it( m_clients ); it.hasNext();)
        it.next().value()->error( m_err, m_errText );
}

QString CachedCSSStyleSheet::checkCharset(const QByteArray& buffer ) const
{
    int s = buffer.size();
    if (s <= 12) return m_charset;

    // @charset has to be first or directly after BOM.
    // CSS 2.1 says @charset should win over BOM, but since more browsers support BOM
    // than @charset, we default to that.
    const char* d = buffer.data();
    if (strncmp(d, "@charset \"",10) == 0)
    {
        // the string until "; is the charset name
        const char *p = strchr(d+10, '"');
        if (p == 0) return m_charset;
        QString charset = QString::fromLatin1(d+10, p-(d+10));
        return charset;
    }
    return m_charset;
}

// -------------------------------------------------------------------------------------------

CachedScript::CachedScript(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, const char*)
    : CachedObject(url, Script, _cachePolicy, 0)
{
    // It's javascript we want.
    // But some websites think their scripts are <some wrong mimetype here>
    // and refuse to serve them if we only accept application/x-javascript.
    setAccept( QLatin1String("*/*") );
    // load the file.
    // Scripts block document parsing. They are therefore second in our list of most
    // desired resources.
    Cache::loader()->load(dl, this, false/*incremental*/, -6);
    m_loading = true;
    m_hadError = false;
}

CachedScript::CachedScript(const DOMString &url, const QString &script_data)
    : CachedObject(url, Script, KIO::CC_Verify, script_data.length())
{
    m_hadError = false;
    m_loading = false;
    m_status = Persistent;
    m_script = DOMString(script_data);
}

void CachedScript::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);

    if(!m_loading) c->notifyFinished(this);
}

void CachedScript::data( QBuffer &buffer, bool eof )
{
    if(!eof) return;
    buffer.close();
    setSize(buffer.buffer().size());

    QTextCodec* c = codecForBuffer( m_charset, buffer.buffer() );
    QString data = c->toUnicode( buffer.buffer().data(), m_size );
    m_script = static_cast<QChar>(data[0]) == QChar::ByteOrderMark ? DOMString(data.mid( 1 ) ) : DOMString(data);
    m_loading = false;
    checkNotify();
}

void CachedScript::checkNotify()
{
    if(m_loading) return;

    for (QHashIterator<CachedObjectClient*,CachedObjectClient*> it( m_clients ); it.hasNext();)
        it.next().value()->notifyFinished(this);
}

void CachedScript::error( int /*err*/, const char* /*text*/ )
{
    m_hadError = true;
    m_loading  = false;
    checkNotify();
}

// ------------------------------------------------------------------------------------------

static QString buildAcceptHeader()
{
    return "image/png, image/jpeg, video/x-mng, image/jp2, image/gif;q=0.5,*/*;q=0.1";
}

// -------------------------------------------------------------------------------------

CachedImage::CachedImage(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, const char*)
    : QObject(), CachedObject(url, Image, _cachePolicy, 0)
{
    i = new khtmlImLoad::Image(this);
    //p = 0;
    //pixPart = 0;
    bg = 0;
    scaled = 0;
    bgColor = qRgba( 0, 0, 0, 0 );
    m_status = Unknown;
    setAccept( buildAcceptHeader() );
    i->setShowAnimations(dl->showAnimations());
    m_loading = true;

    if ( KHTMLGlobal::defaultHTMLSettings()->isAdFiltered( url.string() ) ) {
        m_wasBlocked = true;
        CachedObject::finish();
    }
}

CachedImage::~CachedImage()
{
    clear();
    delete i;
}

void CachedImage::ref( CachedObjectClient *c )
{
    CachedObject::ref(c);

#ifdef LOADER_DEBUG
    qDebug() << "image" << this << "ref'd by client" << c;
#endif

    // for mouseovers, dynamic changes
    //### having both makes no sense
    if ( m_status >= Persistent && !pixmap_size().isNull() ) {
#ifdef LOADER_DEBUG
        qDebug() << "Notifying finished size:" << i->size().width() << "," << i->size().height();
#endif
        c->updatePixmap( QRect(QPoint(0, 0), pixmap_size()), this );
        c->notifyFinished( this );
    }
}

void CachedImage::deref( CachedObjectClient *c )
{
    CachedObject::deref(c);
/*    if(m && m_clients.isEmpty() && m->running())
        m->pause();*/
}

#define BGMINWIDTH      32
#define BGMINHEIGHT     32

QPixmap CachedImage::tiled_pixmap(const QColor& newc, int xWidth, int xHeight)
{

    // no error indication for background images
    if(m_hadError||m_wasBlocked) return *Cache::nullPixmap;

    // If we don't have size yet, nothing to draw yet
    if (i->size().width() == 0 || i->size().height() == 0)
        return *Cache::nullPixmap;

#ifdef __GNUC__
#warning "Needs some additional performance work"
#endif

    static QRgb bgTransparent = qRgba( 0, 0, 0, 0 );

    QSize s(pixmap_size());
    int w = xWidth;
    int h = xHeight;

    if (w == -1) xWidth = w = s.width();
    if (h == -1) xHeight = h = s.height();

    if ( ( (bgColor != bgTransparent) && (bgColor != newc.rgba()) ) ||
         ( bgSize != QSize(xWidth, xHeight)) )
    {
        delete bg; bg = 0;
    }

    if (bg)
        return *bg;

    const QPixmap* src; //source for pretiling, if any

    const QPixmap &r = pixmap(); //this is expensive
    if (r.isNull()) return r;

    //See whether we should scale
    if (xWidth != s.width() || xHeight != s.height()) {
        src = scaled_pixmap(xWidth, xHeight);
    } else {
        src = &r;
    }

    bgSize = QSize(xWidth, xHeight);

    //See whether we can - and should - pre-blend
    // ### this needs serious investigations. Not likely to help with transparent bgColor,
    // won't work with CSS3 multiple backgrounds. Does it help at all in Qt4? (ref: #114938)
    if (newc.isValid() && (r.hasAlpha() || r.hasAlphaChannel())) {
        bg = new QPixmap(xWidth, xHeight);
        bg->fill(newc);
        QPainter p(bg);
        p.drawPixmap(0, 0, *src);
        bgColor = newc.rgba();
        src     = bg;
    } else {
        bgColor = bgTransparent;
    }

    //See whether to pre-tile.
    if ( w*h < 8192 )
    {
        if ( r.width() < BGMINWIDTH )
            w = ((BGMINWIDTH-1) / xWidth + 1) * xWidth;
        if ( r.height() < BGMINHEIGHT )
            h = ((BGMINHEIGHT-1) / xHeight + 1) * xHeight;
    }

    if ( w != xWidth  || h != xHeight )
    {
        // qDebug() << "pre-tiling " << s.width() << "," << s.height() << " to " << w << "," << h;
        QPixmap* oldbg = bg;
        bg = new QPixmap(w, h);
        if (src->hasAlpha() || src->hasAlphaChannel()) {
            if (newc.isValid() && (bgColor != bgTransparent))
                bg->fill( bgColor );
            else
                bg->fill( Qt::transparent );
        }

        QPainter p(bg);
        p.drawTiledPixmap(0, 0, w, h, *src);
        p.end();

        if ( src == oldbg )
            delete oldbg;
    } else if (src && !bg) {
        // we were asked for the entire pixmap. Cache it.
        // ### goes against imload stuff, but it's far too expensive
        //     to recreate the full pixmap each time it's requested as
        //     we don't know what portion of it will be used eventually
        //     (by e.g. paintBackgroundExtended). It could be a few pixels of
        //     a huge image. See #140248/#1 for an obvious example.
        //     Imload probably needs to handle all painting in paintBackgroundExtended.
        bg = new QPixmap(*src);
    }

    if (bg)
        return *bg;

    return *src;
}


QPixmap* CachedImage::scaled_pixmap( int xWidth, int xHeight )
{
    // no error indication for background images
    if(m_hadError||m_wasBlocked) return Cache::nullPixmap;

    // If we don't have size yet, nothing to draw yet
    if (i->size().width() == 0 || i->size().height() == 0)
        return Cache::nullPixmap;

    if (scaled) {
        if (scaled->width() == xWidth && scaled->height() == xHeight)
            return scaled;
        delete scaled;
    }

    //### this is quite awful performance-wise. It should avoid
    // alpha if not needed, and go to pixmap, etc.
    QImage im(xWidth, xHeight, QImage::Format_ARGB32_Premultiplied);

    QPainter paint(&im);
    paint.setCompositionMode(QPainter::CompositionMode_Source);
    ImagePainter pi(i, QSize(xWidth, xHeight));
    pi.paint(0, 0, &paint);
    paint.end();

    scaled = new QPixmap(QPixmap::fromImage(im));

    return scaled;
}

QPixmap CachedImage::pixmap( ) const
{
    if (m_hadError)
        return *Cache::brokenPixmap;

    if(m_wasBlocked)
        return *Cache::blockedPixmap;

    int w = i->size().width();
    int h = i->size().height();

    if (i->hasAlpha() && QApplication::desktop()->paintEngine() &&
                        !QApplication::desktop()->paintEngine()->hasFeature(QPaintEngine::PorterDuff)) {
        QImage im(w, h, QImage::Format_ARGB32_Premultiplied);
        QPainter paint(&im);
        paint.setCompositionMode(QPainter::CompositionMode_Source);
        ImagePainter pi(i);
        pi.paint(0, 0, &paint);
        paint.end();
        return QPixmap::fromImage( im, Qt::NoOpaqueDetection );
    } else {
        QPixmap pm(w, h);
        if (i->hasAlpha())
            pm.fill(Qt::transparent);
        QPainter paint(&pm);
        paint.setCompositionMode(QPainter::CompositionMode_Source);
        ImagePainter pi(i);
        pi.paint(0, 0, &paint);
        paint.end();
        return pm;
    }
}


QSize CachedImage::pixmap_size() const
{
    if (m_wasBlocked) return Cache::blockedPixmap->size();
    if (m_hadError)   return Cache::brokenPixmap->size();
    if (i)            return i->size();
    return QSize();
}


void CachedImage::imageHasGeometry(khtmlImLoad::Image* /*img*/, int width, int height)
{
#ifdef LOADER_DEBUG
    qDebug() << this << "got geometry" << width << "x" << height;
#endif

    do_notify(QRect(0, 0, width, height));
}

void CachedImage::imageChange(khtmlImLoad::Image* /*img*/, QRect region)
{
#ifdef LOADER_DEBUG
    qDebug() << "Image" << this << "change" <<
        region.x() << "," << region.y() << ":" << region.width() << "x" << region.height();
#endif

    //### this is overly conservative -- I guess we need to also specify reason,
    //e.g. repaint vs. changed !!!
    delete bg;
    bg = 0;

    do_notify(region);
}

void CachedImage::doNotifyFinished()
{
    for (QHashIterator<CachedObjectClient*,CachedObjectClient*> it( m_clients ); it.hasNext();)
    {
        it.next().value()->notifyFinished(this);
    }
}

void CachedImage::imageError(khtmlImLoad::Image* /*img*/)
{
    error(0, 0);
}


void CachedImage::imageDone(khtmlImLoad::Image* /*img*/)
{
#ifdef LOADER_DEBUG
    qDebug() << "Image is done:" << this;
#endif
    m_status = Persistent;
    m_loading = false;
    doNotifyFinished();
}

// QRect CachedImage::valid_rect() const
// {
//     if (m_wasBlocked) return Cache::blockedPixmap->rect();
//     return (m_hadError ? Cache::brokenPixmap->rect() : m ? m->frameRect() : ( p ? p->rect() : QRect()) );
// }


void CachedImage::do_notify(const QRect& r)
{
    for (QHashIterator<CachedObjectClient*,CachedObjectClient*> it( m_clients ); it.hasNext();)
    {
#ifdef LOADER_DEBUG
        qDebug() << "image" << this << "notify of geom client" << it.peekNext().value();
#endif
        it.next().value()->updatePixmap( r, this);
    }
}

void CachedImage::setShowAnimations( KHTMLSettings::KAnimationAdvice showAnimations )
{
    if (i)
        i->setShowAnimations(showAnimations);
}

void CachedImage::clear()
{
    delete i;   i = new khtmlImLoad::Image(this);
    delete scaled;  scaled = 0;
    bgColor = qRgba( 0, 0, 0, 0xff );
    delete bg;  bg = 0;
    bgSize = QSize(-1,-1);

    setSize(0);
}

void CachedImage::data ( QBuffer &_buffer, bool eof )
{
#ifdef LOADER_DEBUG
    qDebug() << this << "buffersize =" << _buffer.buffer().size() << ", eof =" << eof << ", pos :" << _buffer.pos();
#endif
    i->processData((uchar*)_buffer.data().data(), _buffer.pos());

    _buffer.close();

    if (eof)
        i->processEOF();
}

void CachedImage::finish()
{
    CachedObject::finish();
    m_loading = false;
    QSize s = pixmap_size();
    setSize( s.width() * s.height() * 2);
}


void CachedImage::error( int /*err*/, const char* /*text*/ )
{
    clear();
    m_hadError = true;
    m_loading = false;
    do_notify(QRect(0, 0, 16, 16));
    for (QHashIterator<CachedObjectClient*,CachedObjectClient*> it( m_clients ); it.hasNext();)
        it.next().value()->notifyFinished(this);
}

// -------------------------------------------------------------------------------------------

CachedSound::CachedSound(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, const char*)
    : CachedObject(url, Sound, _cachePolicy, 0)
{
    setAccept( QLatin1String("*/*") ); // should be whatever phonon would accept...
    Cache::loader()->load(dl, this, false/*incremental*/, 2);
    m_loading = true;
}

void CachedSound::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);

    if(!m_loading) c->notifyFinished(this);
}

void CachedSound::data( QBuffer &buffer, bool eof )
{
    if(!eof) return;
    buffer.close();
    setSize(buffer.buffer().size());

    m_sound = buffer.buffer();
    m_loading = false;
    checkNotify();
}

void CachedSound::checkNotify()
{
    if(m_loading) return;

    for (QHashIterator<CachedObjectClient*,CachedObjectClient*> it( m_clients ); it.hasNext();)
        it.next().value()->notifyFinished(this);
}

void CachedSound::error( int /*err*/, const char* /*text*/ )
{
    m_loading = false;
    checkNotify();
}

// -------------------------------------------------------------------------------------------

CachedFont::CachedFont(DocLoader* dl, const DOMString &url, KIO::CacheControl _cachePolicy, const char*)
    : CachedObject(url, Font, _cachePolicy, 0)
{
    setAccept( QLatin1String("*/*") );
    // Fonts are desired early because their absence will lead to a page being rendered
    // with a default replacement, then the text being re-rendered with the new font when it arrives.
    // This can be fairly disturbing for the reader - more than missing images for instance.
    Cache::loader()->load(dl, this, false /*incremental*/, -4);
    m_loading = true;
}

void CachedFont::ref(CachedObjectClient *c)
{
    CachedObject::ref(c);

    if(!m_loading) c->notifyFinished(this);
}

void CachedFont::data( QBuffer &buffer, bool eof )
{
    if(!eof) return;
    buffer.close();
    m_font = buffer.buffer();

    // some fonts are compressed.
    QIODevice* dev = KFilterDev::device(&buffer, mimetype(), false /*autoDeleteInDevice*/);
    if (dev && dev->open( QIODevice::ReadOnly )) {
        m_font = dev->readAll();
        delete dev;
    }

    // handle decoding of WOFF fonts
    int woffStatus = eWOFF_ok;
    if (int need = WOFF::getDecodedSize( m_font.constData(), m_font.size(), &woffStatus)) {
        // qDebug() << "***************************** Got WOFF FoNT";
        m_hadError = true;
        do {
            if (WOFF_FAILURE(woffStatus))
                break;
            QByteArray wbuffer;
            wbuffer.resize( need );
            int len;
            woffStatus = eWOFF_ok;
            WOFF::decodeToBuffer(m_font.constData(), m_font.size(), wbuffer.data(), wbuffer.size(), &len, &woffStatus);
            if (WOFF_FAILURE(woffStatus))
                break;
            wbuffer.resize(len);
            m_font = wbuffer;
            m_hadError = false;
        } while (false);
    } else if (m_font.isEmpty()) {
        m_hadError = true;
    }
    else {
        // qDebug() << "******** #################### ********************* NON WOFF font";
    }
    setSize(m_font.size());

    m_loading = false;
    checkNotify();
}

void CachedFont::checkNotify()
{
    if(m_loading) return;

    for (QHashIterator<CachedObjectClient*,CachedObjectClient*> it( m_clients ); it.hasNext();)
        it.next().value()->notifyFinished(this);
}

void CachedFont::error( int /*err*/, const char* /*text*/ )
{
    m_loading = false;
    m_hadError = true;
    checkNotify();
}

// ------------------------------------------------------------------------------------------

Request::Request(DocLoader* dl, CachedObject *_object, bool _incremental, int _priority)
{
    object = _object;
    object->setRequest(this);
    incremental = _incremental;
    priority = _priority;
    m_docLoader = dl;
}

Request::~Request()
{
    object->setRequest(0);
}

// ------------------------------------------------------------------------------------------

DocLoader::DocLoader(KHTMLPart* part, DocumentImpl* doc)
{
    m_cachePolicy = KIO::CC_Verify;
    m_creationDate = QDateTime::currentDateTime();
    m_bautoloadImages = true;
    m_showAnimations = KHTMLSettings::KAnimationEnabled;
    m_part = part;
    m_doc = doc;

    Cache::docloader->append( this );
}

DocLoader::~DocLoader()
{
    clearPreloads();
    Cache::loader()->cancelRequests( this );
    Cache::docloader->removeAll( this );
}

void DocLoader::setCacheCreationDate(const QDateTime &_creationDate)
{
    if (_creationDate.isValid())
        m_creationDate = _creationDate;
    else
        m_creationDate = QDateTime::currentDateTime();
}

void DocLoader::setExpireDate(const QDateTime &_expireDate)
{
    m_expireDate = _expireDate;

#ifdef CACHE_DEBUG
    qDebug() << QDateTime::currentDateTime().secsTo(m_expireDate) << "seconds left until reload required.";
#endif
}

void DocLoader::setRelativeExpireDate(qint64 seconds)
{
    m_expireDate = m_creationDate.addSecs(seconds);
}

void DocLoader::insertCachedObject( CachedObject* o ) const
{
    m_docObjects.insert( o );
}

bool DocLoader::needReload(CachedObject *existing, const QString& fullURL)
{
    bool reload = false;
    if (m_cachePolicy == KIO::CC_Verify)
    {
       if (!m_reloadedURLs.contains(fullURL))
       {
          if (existing && existing->isExpired() && !existing->isPreloaded())
          {
             Cache::removeCacheEntry(existing);
             m_reloadedURLs.append(fullURL);
             reload = true;
          }
       }
    }
    else if ((m_cachePolicy == KIO::CC_Reload) || (m_cachePolicy == KIO::CC_Refresh))
    {
       if (!m_reloadedURLs.contains(fullURL))
       {
          if (existing && !existing->isPreloaded())
          {
             Cache::removeCacheEntry(existing);
          }
          if (!existing || !existing->isPreloaded()) {
              m_reloadedURLs.append(fullURL);
              reload = true;
          }
       }
    }
    return reload;
}

void DocLoader::registerPreload(CachedObject* resource)
{
    if (!resource || resource->isLoaded() || m_preloads.contains(resource))
        return;
    resource->increasePreloadCount();
    m_preloads.insert(resource);
    resource->setProspectiveRequest();
#ifdef PRELOAD_DEBUG
    fprintf(stderr, "PRELOADING %s\n", resource->url().string().toLatin1().data());
#endif
}

void DocLoader::clearPreloads()
{
    printPreloadStats();
    QSet<CachedObject*>::iterator end = m_preloads.end();
    for (QSet<CachedObject*>::iterator it = m_preloads.begin(); it != end; ++it) {
        CachedObject* res = *it;
        res->decreasePreloadCount();
        if (res->preloadResult() == CachedObject::PreloadNotReferenced || res->hadError())
            Cache::removeCacheEntry(res);
    }
    m_preloads.clear();
}

void DocLoader::printPreloadStats()
{
#ifdef PRELOAD_DEBUG
    unsigned scripts = 0;
    unsigned scriptMisses = 0;
    unsigned stylesheets = 0;
    unsigned stylesheetMisses = 0;
    unsigned images = 0;
    unsigned imageMisses = 0;
    QSet<CachedObject*>::iterator end = m_preloads.end();
    for (QSet<CachedObject*>::iterator it = m_preloads.begin(); it != end; ++it) {
        CachedObject* res = *it;
        if (res->preloadResult() == CachedObject::PreloadNotReferenced)
            fprintf(stderr,"!! UNREFERENCED PRELOAD %s\n", res->url().string().toLatin1().data());
        else if (res->preloadResult() == CachedObject::PreloadReferencedWhileComplete)
            fprintf(stderr,"HIT COMPLETE PRELOAD %s\n", res->url().string().toLatin1().data());
        else if (res->preloadResult() == CachedObject::PreloadReferencedWhileLoading)
            fprintf(stderr,"HIT LOADING PRELOAD %s\n", res->url().string().toLatin1().data());

        if (res->type() == CachedObject::Script) {
            scripts++;
            if (res->preloadResult() < CachedObject::PreloadReferencedWhileLoading)
                scriptMisses++;
        } else if (res->type() == CachedObject::CSSStyleSheet) {
            stylesheets++;
            if (res->preloadResult() < CachedObject::PreloadReferencedWhileLoading)
                stylesheetMisses++;
        } else {
            images++;
            if (res->preloadResult() < CachedObject::PreloadReferencedWhileLoading)
                imageMisses++;
        }
    }
    if (scripts)
        fprintf(stderr, "SCRIPTS: %d (%d hits, hit rate %d%%)\n", scripts, scripts - scriptMisses, (scripts - scriptMisses) * 100 / scripts);
    if (stylesheets)
        fprintf(stderr, "STYLESHEETS: %d (%d hits, hit rate %d%%)\n", stylesheets, stylesheets - stylesheetMisses, (stylesheets - stylesheetMisses) * 100 / stylesheets);
    if (images)
        fprintf(stderr, "IMAGES:  %d (%d hits, hit rate %d%%)\n", images, images - imageMisses, (images - imageMisses) * 100 / images);
#endif
}

static inline bool securityCheckUrl(const QUrl& fullURL, KHTMLPart* part, DOM::DocumentImpl* doc,
                                    bool doRedirectCheck, bool isImg)
{
    if (!fullURL.isValid())
        return false;
    if (part && part->onlyLocalReferences() && fullURL.scheme() != "file" && fullURL.scheme() != "data")
        return false;
    if (doRedirectCheck && doc) {
        if (isImg && part && part->forcePermitLocalImages() && fullURL.scheme() == "file")
            return true;
        else
            return KUrlAuthorized::authorizeUrlAction("redirect", doc->URL(), fullURL);
    }

    return true;
}

#define DOCLOADER_SECCHECK_IMP(doRedirectCheck,isImg) \
    QUrl fullURL(m_doc->completeURL(url.string())); \
    if (!securityCheckUrl(fullURL, m_part, m_doc, doRedirectCheck, isImg)) \
         return 0L;

#define DOCLOADER_SECCHECK(doRedirectCheck)     DOCLOADER_SECCHECK_IMP(doRedirectCheck, false)
#define DOCLOADER_SECCHECK_IMG(doRedirectCheck) DOCLOADER_SECCHECK_IMP(doRedirectCheck, true)

bool DocLoader::willLoadMediaElement( const DOM::DOMString &url)
{
    DOCLOADER_SECCHECK(true);

    return true;
}

CachedImage *DocLoader::requestImage( const DOM::DOMString &url)
{
    DOCLOADER_SECCHECK_IMG(true);

    CachedImage* i = Cache::requestObject<CachedImage, CachedObject::Image>( this, fullURL, 0);

    if (i && i->status() == CachedObject::Unknown && autoloadImages())
        Cache::loader()->load(this, i, true /*incremental*/);

    return i;
}

CachedCSSStyleSheet *DocLoader::requestStyleSheet( const DOM::DOMString &url, const QString& charset,
						   const char *accept, bool userSheet )
{
    DOCLOADER_SECCHECK(!userSheet);

    CachedCSSStyleSheet* s = Cache::requestObject<CachedCSSStyleSheet, CachedObject::CSSStyleSheet>( this, fullURL, accept );
    if ( s && !charset.isEmpty() ) {
        s->setCharsetHint( charset );
    }
    return s;
}

CachedScript *DocLoader::requestScript( const DOM::DOMString &url, const QString& charset)
{
    DOCLOADER_SECCHECK(true);
    if ( ! KHTMLGlobal::defaultHTMLSettings()->isJavaScriptEnabled(fullURL.host()) ||
           KHTMLGlobal::defaultHTMLSettings()->isAdFiltered(fullURL.url()))
	return 0L;

    CachedScript* s = Cache::requestObject<CachedScript, CachedObject::Script>( this, fullURL, 0 );
    if ( s && !charset.isEmpty() )
        s->setCharset( charset );
    return s;
}

CachedSound *DocLoader::requestSound( const DOM::DOMString &url )
{
      DOCLOADER_SECCHECK(true);
      CachedSound* s = Cache::requestObject<CachedSound, CachedObject::Sound>( this, fullURL, 0 );
      return s;
}

CachedFont *DocLoader::requestFont( const DOM::DOMString &url )
{
    DOCLOADER_SECCHECK(true);
    CachedFont* s = Cache::requestObject<CachedFont, CachedObject::Font>( this, fullURL, 0 );
    return s;
}

#undef DOCLOADER_SECCHECK

void DocLoader::setAutoloadImages( bool enable )
{
    if ( enable == m_bautoloadImages )
        return;

    m_bautoloadImages = enable;

    if ( !m_bautoloadImages ) return;

    for ( QSetIterator<CachedObject*> it( m_docObjects ); it.hasNext(); )
    {
        CachedObject* cur = it.next();
        if ( cur->type() == CachedObject::Image )
        {
            CachedImage *img = const_cast<CachedImage*>( static_cast<const CachedImage *>(cur) );

            CachedObject::Status status = img->status();
            if ( status != CachedObject::Unknown )
                continue;

            Cache::loader()->load(this, img, true /*incremental*/);
        }
    }
}

void DocLoader::setShowAnimations( KHTMLSettings::KAnimationAdvice showAnimations )
{
    if ( showAnimations == m_showAnimations ) return;
    m_showAnimations = showAnimations;

    for ( QSetIterator<CachedObject*> it( m_docObjects ); it.hasNext(); )
    {
        CachedObject* cur = it.next();
        if ( cur->type() == CachedObject::Image )
        {
            CachedImage *img = const_cast<CachedImage*>( static_cast<const CachedImage *>( cur ) );

            img->setShowAnimations( m_showAnimations );
        }
    }
}

// ------------------------------------------------------------------------------------------

Loader::Loader() : QObject()
{
    m_supportedImageTypes = khtmlImLoad::ImageManager::loaderDatabase()->supportedMimeTypes();
}

Loader::~Loader()
{
    qDeleteAll(m_requestsLoading);
}

void Loader::load(DocLoader* dl, CachedObject *object, bool incremental, int priority)
{
    Request *req = new Request(dl, object, incremental, priority);
    scheduleRequest(req);
    emit requestStarted( req->m_docLoader, req->object );
}

void Loader::scheduleRequest(Request* req)
{
#ifdef LOADER_DEBUG
  qDebug() << "starting Loader url =" << req->object->url().string();
#endif

    QUrl u(req->object->url().string());
    KIO::TransferJob* job = KIO::get( u, KIO::NoReload, KIO::HideProgressInfo /*no GUI*/);

    job->addMetaData("cache", KIO::getCacheControlString(req->object->cachePolicy()));
    if (!req->object->accept().isEmpty())
        job->addMetaData("accept", req->object->accept());
    if ( req->m_docLoader )
    {
        job->addMetaData( "referrer",  req->m_docLoader->doc()->URL().url() );
         KHTMLPart *part = req->m_docLoader->part();
        if (part )
        {
            job->addMetaData( "cross-domain", part->toplevelURL().url() );
            if (part->widget())
                KJobWidgets::setWindow(job, part->widget()->topLevelWidget());
        }
    }

    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotFinished(KJob*)) );
    connect( job, SIGNAL(mimetype(KIO::Job*,QString)), this, SLOT(slotMimetype(KIO::Job*,QString)) );
    connect( job, SIGNAL(data(KIO::Job*,QByteArray)),
             SLOT(slotData(KIO::Job*,QByteArray)));

    KIO::Scheduler::setJobPriority( job, req->priority );

    m_requestsLoading.insertMulti(job, req);
}

void Loader::slotMimetype( KIO::Job *j, const QString& s )
{
    Request *r = m_requestsLoading.value( j );
    if (!r)
        return;
    CachedObject *o = r->object;

    // Mozilla plain ignores any  mimetype that doesn't have / in it, and handles it as "",
    // including when being picky about mimetypes. Match that for better compatibility with broken servers.
    if (s.contains('/'))
        o->m_mimetype = s;
    else
        o->m_mimetype = "";
}

void Loader::slotFinished( KJob* job )
{
  KIO::TransferJob* j = static_cast<KIO::TransferJob*>(job);
  Request *r = m_requestsLoading.take( j );

  if ( !r )
    return;

  bool reqFailed = false;
  if (j->error()) {
      reqFailed = true;
  }
  else if (j->isErrorPage()) {
      if (r->object->type() == CachedObject::Image && m_supportedImageTypes.contains(r->object->m_mimetype)) {
          // Do not set the request as a failed, we asked for an image and got it
          // as the content of the error response (e.g. 404)
      }
      else {
          reqFailed = true;
      }
  }

  if (reqFailed) {
#ifdef LOADER_DEBUG
      qDebug() << "ERROR: job->error() =" << j->error() << ", job->isErrorPage() =" << j->isErrorPage();
#endif
      r->object->error( job->error(), job->errorText().toLatin1().constData() );
      emit requestFailed( r->m_docLoader, r->object );
  }
  else {
      QString cs = j->queryMetaData("charset");
      if (!cs.isEmpty()) r->object->setCharset(cs);
      r->object->data(r->m_buffer, true);
      emit requestDone( r->m_docLoader, r->object );
      QDateTime expireDate = QDateTime::fromTime_t(j->queryMetaData("expire-date").toLong());
#ifdef LOADER_DEBUG
      qDebug() << "url =" << j->url().url();
#endif
      r->object->setExpireDate(expireDate);

      if ( r->object->type() == CachedObject::Image ) {
          QString fn = j->queryMetaData("content-disposition-filename");
          static_cast<CachedImage*>( r->object )->setSuggestedFilename(fn);
#ifdef IMAGE_TITLES
          static_cast<CachedImage*>( r->object )->setSuggestedTitle(fn);
          QTemporaryFile tf;
          tf.open();
          tf.write((const char*)r->m_buffer.buffer().data(), r->m_buffer.size());
          tf.flush();
          KFileMetaInfo kfmi(tf.fileName());
          if (!kfmi.isEmpty()) {
              KFileMetaInfoItem i = kfmi.item("Name");
              if (i.isValid()) {
                  static_cast<CachedImage*>(r->object)->setSuggestedTitle(i.string());
              } else {
                  i = kfmi.item("Title");
                  if (i.isValid()) {
                      static_cast<CachedImage*>(r->object)->setSuggestedTitle(i.string());
                  }
              }
          }
#endif
      }
  }

  r->object->finish();

#ifdef LOADER_DEBUG
  qDebug() << "JOB FINISHED" << r->object << ":" << r->object->url().string();
#endif

  delete r;
}

void Loader::slotData( KIO::Job*job, const QByteArray &data )
{
    Request *r = m_requestsLoading.value(job);
    if(!r) {
        qDebug() << "got data for unknown request!";
        return;
    }

    if ( !r->m_buffer.isOpen() )
        r->m_buffer.open( QIODevice::WriteOnly );

    r->m_buffer.write( data.data(), data.size() );

    if(r->incremental)
        r->object->data( r->m_buffer, false );
}

int Loader::numRequests( DocLoader* dl ) const
{
    int res = 0;
    foreach( Request* req, m_requestsLoading)
        if ( req->m_docLoader == dl )
            res++;

    return res;
}

void Loader::cancelRequests( DocLoader* dl )
{
    QMutableHashIterator<KIO::Job*,Request*> lIt( m_requestsLoading );
    while ( lIt.hasNext() )
    {
        lIt.next();
        if ( lIt.value()->m_docLoader == dl )
        {
            //qDebug() << "canceling loading request for" << lIt.current()->object->url().string();
            KIO::Job *job = static_cast<KIO::Job *>( lIt.key() );
            Cache::removeCacheEntry( lIt.value()->object );
            delete lIt.value();
            lIt.remove();
            job->kill();
        }
    }
}

KIO::Job *Loader::jobForRequest( const DOM::DOMString &url ) const
{
    QHashIterator<KIO::Job*,Request*> it( m_requestsLoading );
    while (it.hasNext())
    {
        it.next();
        if ( it.value()->object && it.value()->object->url() == url )
            return static_cast<KIO::Job *>( it.key() );
    }

    return 0;
}

// ----------------------------------------------------------------------------


QHash<QString,CachedObject*> *Cache::cache;
QLinkedList<DocLoader*>    *Cache::docloader;
QLinkedList<CachedObject*> *Cache::freeList;
Loader *Cache::m_loader;

int Cache::maxSize = DEFCACHESIZE;
int Cache::totalSizeOfLRU;

QPixmap *Cache::nullPixmap;
QPixmap *Cache::brokenPixmap;
QPixmap *Cache::blockedPixmap;

void Cache::init()
{
    if ( !cache )
        cache = new QHash<QString,CachedObject*>();

    if ( !docloader )
        docloader = new QLinkedList<DocLoader*>;

    if ( !nullPixmap )
        nullPixmap = new QPixmap;

    if ( !brokenPixmap )
        brokenPixmap = new QPixmap(KHTMLGlobal::iconLoader()->loadIcon("image-missing", KIconLoader::Desktop, 16, KIconLoader::DisabledState));

    if ( !blockedPixmap ) {
        blockedPixmap = new QPixmap();
        blockedPixmap->loadFromData(blocked_icon_data, blocked_icon_len);
    }

    if ( !m_loader )
        m_loader = new Loader();

    if ( !freeList )
        freeList = new QLinkedList<CachedObject*>;
}

void Cache::clear()
{
    if ( !cache ) return;
#ifdef CACHE_DEBUG
    qDebug() << "CLEAR!";
    statistics();
#endif

#ifndef NDEBUG
    bool crash = false;
    foreach (CachedObject* co, *cache) {
        if (!co->canDelete()) {
            qDebug() << " Object in cache still linked to";
            qDebug() << " -> URL :" << co->url();
            qDebug() << " -> #clients :" << co->count();
            crash = true;
//         assert(co->canDelete());
        }
    }
    foreach (CachedObject* co, *freeList) {
        if (!co->canDelete()) {
            qDebug() << " Object in freelist still linked to";
            qDebug() << " -> URL :" << co->url();
            qDebug() << " -> #clients :" << co->count();
            crash = true;
            /*
            foreach (CachedObjectClient* cur, (*co->m_clients)))
            {
                if (dynamic_cast<RenderObject*>(cur)) {
                    qDebug() << " --> RenderObject";
                } else
                    qDebug() << " --> Something else";
            }*/
        }
//         assert(freeList->current()->canDelete());
    }
    assert(!crash);
#endif
    qDeleteAll(*cache);
    delete cache; cache = 0;
    delete nullPixmap; nullPixmap = 0;
    delete brokenPixmap; brokenPixmap = 0;
    delete blockedPixmap; blockedPixmap = 0;
    delete m_loader;  m_loader = 0;
    delete docloader; docloader = 0;
    qDeleteAll(*freeList);
    delete freeList; freeList = 0;
}

template<typename CachedObjectType, enum CachedObject::Type CachedType>
CachedObjectType* Cache::requestObject( DocLoader* dl, const QUrl& kurl, const char* accept )
{
    KIO::CacheControl cachePolicy = dl->cachePolicy();

    QString url = kurl.url();
    CachedObject* o = cache->value(url);

    if ( o && o->type() != CachedType ) {
        removeCacheEntry( o );
        o = 0;
    }

    if ( o && dl->needReload( o, url ) ) {
        o = 0;
        assert( !cache->contains( url ) );
    }

    if(!o)
    {
#ifdef CACHE_DEBUG
        qDebug() << "new:" << kurl.url();
#endif
        CachedObjectType* cot = new CachedObjectType(dl, url, cachePolicy, accept);
        cache->insert( url, cot );
        if ( cot->allowInLRUList() )
            insertInLRUList( cot );
        o = cot;
    }
#ifdef CACHE_DEBUG
    else {
        qDebug() << "using pending/cached:" << kurl.url();
    }
#endif


    dl->insertCachedObject( o );

    return static_cast<CachedObjectType *>(o);
}

void Cache::preloadStyleSheet( const QString &url, const QString &stylesheet_data)
{
    if (cache->contains(url))
        removeCacheEntry(cache->value(url));

    CachedCSSStyleSheet *stylesheet = new CachedCSSStyleSheet(url, stylesheet_data);
    cache->insert( url, stylesheet );
}

void Cache::preloadScript( const QString &url, const QString &script_data)
{
    if (cache->contains(url))
        removeCacheEntry(cache->value(url));

    CachedScript *script = new CachedScript(url, script_data);
    cache->insert( url, script );
}

void Cache::flush(bool force)
{
    init();

    if ( force || totalSizeOfLRU > maxSize + maxSize/4) {
        for ( int i = MAX_LRU_LISTS-1; i >= 0 && totalSizeOfLRU > maxSize; --i )
            while ( totalSizeOfLRU > maxSize && m_LRULists[i].m_tail )
                removeCacheEntry( m_LRULists[i].m_tail );

#ifdef CACHE_DEBUG
        statistics();
#endif
    }

    QMutableLinkedListIterator<CachedObject*> it(*freeList);
    while ( it.hasNext() ) {
        CachedObject* p = it.next();
        if ( p->canDelete() ) {
            it.remove();
            delete p;
        }
    }
}

void Cache::setSize( int bytes )
{
    maxSize = bytes;
    flush(true /* force */);
}

void Cache::statistics()
{
    // this function is for debugging purposes only
    init();

    int size = 0;
    int msize = 0;
    int movie = 0;
    int images = 0;
    int scripts = 0;
    int stylesheets = 0;
    int sound = 0;
    int fonts = 0;
    foreach (CachedObject* o, *cache)
    {
        switch(o->type()) {
        case CachedObject::Image:
        {
            //CachedImage *im = static_cast<CachedImage *>(o);
            images++;
            /*if(im->m != 0)
            {
                movie++;
                msize += im->size();
            }*/
            break;
        }
        case CachedObject::CSSStyleSheet:
            stylesheets++;
            break;
        case CachedObject::Script:
            scripts++;
            break;
        case CachedObject::Sound:
            sound++;
            break;
        case CachedObject::Font:
            fonts++;
            break;
        }
        size += o->size();
    }
    size /= 1024;

    qDebug() << "------------------------- image cache statistics -------------------";
    qDebug() << "Number of items in cache:" << cache->count();
    qDebug() << "Number of cached images:" << images;
    qDebug() << "Number of cached movies:" << movie;
    qDebug() << "Number of cached scripts:" << scripts;
    qDebug() << "Number of cached stylesheets:" << stylesheets;
    qDebug() << "Number of cached sounds:" << sound;
    qDebug() << "Number of cached fonts:" << fonts;
    qDebug() << "pixmaps:   allocated space approx." << size << "kB";
    qDebug() << "movies :   allocated space approx." << msize/1024 << "kB";
    qDebug() << "--------------------------------------------------------------------";
}

void Cache::removeCacheEntry( CachedObject *object )
{
    QString key = object->url().string();

    cache->remove( key );
    removeFromLRUList( object );

    foreach( DocLoader* dl, *docloader )
        dl->removeCachedObject( object );

    if ( !object->free() ) {
        Cache::freeList->append( object );
        object->m_free = true;
    }
}

static inline int FastLog2(unsigned int j)
{
   unsigned int log2;
   log2 = 0;
   if (j & (j-1))
       log2 += 1;
   if (j >> 16)
       log2 += 16, j >>= 16;
   if (j >> 8)
       log2 += 8, j >>= 8;
   if (j >> 4)
       log2 += 4, j >>= 4;
   if (j >> 2)
       log2 += 2, j >>= 2;
   if (j >> 1)
       log2 += 1;

   return log2;
}

static LRUList* getLRUListFor(CachedObject* o)
{
    int accessCount = o->accessCount();
    int queueIndex;
    if (accessCount == 0) {
        queueIndex = 0;
   } else {
        int sizeLog = FastLog2(o->size());
        queueIndex = sizeLog/o->accessCount() - 1;
        if (queueIndex < 0)
            queueIndex = 0;
        if (queueIndex >= MAX_LRU_LISTS)
            queueIndex = MAX_LRU_LISTS-1;
    }
   return &m_LRULists[queueIndex];
}

void Cache::removeFromLRUList(CachedObject *object)
{
    CachedObject *next = object->m_next;
    CachedObject *prev = object->m_prev;

    LRUList* list = getLRUListFor(object);
    CachedObject *&head = getLRUListFor(object)->m_head;

    if (next == 0 && prev == 0 && head != object) {
        return;
    }

    object->m_next = 0;
    object->m_prev = 0;

    if (next)
        next->m_prev = prev;
    else if (list->m_tail == object)
       list->m_tail = prev;

    if (prev)
        prev->m_next = next;
    else if (head == object)
        head = next;

    totalSizeOfLRU -= object->size();
}

void Cache::insertInLRUList(CachedObject *object)
{
    removeFromLRUList(object);

    assert( object );
    assert( !object->free() );
    assert( object->canDelete() );
    assert( object->allowInLRUList() );

    LRUList* list = getLRUListFor(object);

    CachedObject *&head = list->m_head;

    object->m_next = head;
    if (head)
        head->m_prev = object;
    head = object;

    if (object->m_next == 0)
        list->m_tail = object;

    totalSizeOfLRU += object->size();
}

// --------------------------------------

void CachedObjectClient::updatePixmap(const QRect&, CachedImage *) {}
void CachedObjectClient::setStyleSheet(const DOM::DOMString &/*url*/, const DOM::DOMString &/*sheet*/, const DOM::DOMString &/*charset*/, const DOM::DOMString &/*mimetype*/) {}
void CachedObjectClient::notifyFinished(CachedObject * /*finishedObj*/) {}
void CachedObjectClient::error(int /*err*/, const QString &/*text*/) {}

#undef CDEBUG

