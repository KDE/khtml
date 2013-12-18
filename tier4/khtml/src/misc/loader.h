/*
    This file is part of the KDE libraries

    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001-2003 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2003 Apple Computer, Inc

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
*/
#ifndef _khtml_loader_h
#define _khtml_loader_h

#include "loader_client.h"

#include <stdlib.h>
#include <QtCore/QObject>
#include <QPixmap>
#include <QtCore/QBuffer>
#include <QtCore/QStringList>
#include <QtCore/QTextCodec>
#include <QtCore/QTimer>
#include <QtCore/QSet>
#include <QtCore/QDateTime>
#include <QLinkedList>
#include <QUrl>

#include <kio/global.h>

#include <khtml_settings.h>
#include <dom/dom_string.h>
#include "imload/image.h"
#include "imload/imageowner.h"

class KHTMLPart;

class KJob;
namespace KIO {
  class Job;
}

namespace DOM
{
        class DocumentImpl;
}

namespace khtml
{
    class CachedObject;
    class Request;
    class DocLoader;

    /**
     * @internal
     *
     * A cached object. Classes who want to use this object should derive
     * from CachedObjectClient, to get the function calls in case the requested data has arrived.
     *
     * This class also does the actual communication with kio and loads the file.
     */
    class CachedObject
    {
    public:
	enum Type {
	    Image,
	    CSSStyleSheet,
	    Script,
	    Sound,
	    Font
	};

	enum Status {
	    Unknown,      // let imagecache decide what to do with it
	    New,          // inserting new image
            Pending,      // only partially loaded
	    Persistent,   // never delete this pixmap
	    Cached        // regular case
	};

        enum PreloadResult {
            PreloadNotReferenced = 0,
            PreloadReferenced,
            PreloadReferencedWhileLoading,
            PreloadReferencedWhileComplete
        };

        PreloadResult preloadResult() const { return m_preloadResult; }
        void setProspectiveRequest() { m_prospectiveRequest = true; }

	CachedObject(const DOM::DOMString &url, Type type, KIO::CacheControl _cachePolicy, int size)
            : m_url(url), m_type(type), m_cachePolicy(_cachePolicy),
             m_size(size)
    {
        m_status = Pending;
            m_accessCount = 0;
	    m_cachePolicy = _cachePolicy;
	    m_request = 0;
            m_deleted = false;
            m_free = false;
            m_hadError = false;
            m_wasBlocked = false;
            m_prev = m_next = 0;
            m_preloadCount = 0;
            m_preloadResult = PreloadNotReferenced;
            m_prospectiveRequest = false;
	}
        virtual ~CachedObject();

	virtual void data( QBuffer &buffer, bool eof) = 0;
	virtual void error( int err, const char *text ) = 0;

	const DOM::DOMString &url() const { return m_url; }
	Type type() const { return m_type; }

	virtual void ref(CachedObjectClient *consumer);
	virtual void deref(CachedObjectClient *consumer);

	int count() const { return m_clients.count(); }
        int accessCount() const { return m_accessCount; }

        bool isPreloaded() const { return m_preloadCount; }
        void increasePreloadCount() { ++m_preloadCount; }
        void decreasePreloadCount() { assert(m_preloadCount); --m_preloadCount; } 

	void setStatus(Status s) { m_status = s; }
	Status status() const { return m_status; }

        virtual void setCharset( const QString& /*charset*/ ) {}

        QTextCodec* codecForBuffer( const QString& charset, const QByteArray& buffer ) const;

	int size() const { return m_size; }

        bool isLoaded() const { return !m_loading; }
        bool hadError() const { return m_hadError; }

        bool free() const { return m_free; }

        KIO::CacheControl cachePolicy() const { return m_cachePolicy; }

        void setRequest(Request *_request);

        bool canDelete() const { return (m_clients.count() == 0 && !m_request && !m_preloadCount); }

       void setExpireDate(const QDateTime &_expireDate) {  m_expireDate = _expireDate; }

	bool isExpired() const;

	virtual void finish();

        /**
         * List of acceptable mimetypes separated by ",". A mimetype may contain a wildcard.
         */
        // e.g. "text/*"
        QString accept() const { return m_accept; }
        void setAccept(const QString &_accept) { m_accept = _accept; }

        // the mimetype that was eventually used
        QString mimetype() const { return m_mimetype; }

    protected:
        void setSize(int size);
        QHash<CachedObjectClient*,CachedObjectClient*> m_clients;
	DOM::DOMString m_url;
        QString m_accept;
        QString m_mimetype;
        Request *m_request;
	Type m_type;
	Status m_status;
        int m_accessCount;
       KIO::CacheControl m_cachePolicy;
       QDateTime m_expireDate;
       int m_size;
        unsigned m_preloadCount;
        PreloadResult m_preloadResult: 3;
        bool m_deleted : 1;
        bool m_loading : 1;
        bool m_free : 1;
	bool m_hadError : 1;
	bool m_wasBlocked : 1;
        bool m_prospectiveRequest: 1;

    private:
        bool allowInLRUList() const { return canDelete() && !m_free && status() != Persistent; }
        CachedObject* m_next;
        CachedObject* m_prev;
        friend class Cache;
        friend class ::KHTMLPart;
        friend class Loader;
    };


    bool isAcceptableCSSMimetype( const DOM::DOMString& mimetype );

    /**
     * a cached style sheet. also used for loading xml documents.
     *
     * ### rename to CachedTextDoc or something since it's more generic than just for css
     */
    class CachedCSSStyleSheet : public CachedObject
    {
    public:
	CachedCSSStyleSheet(DocLoader* dl, const DOM::DOMString &url, KIO::CacheControl cachePolicy,
			    const char *accept);
	CachedCSSStyleSheet(const DOM::DOMString &url, const QString &stylesheet_data);

	const DOM::DOMString &sheet() const { return m_sheet; }

	virtual void ref(CachedObjectClient *consumer);

	virtual void data( QBuffer &buffer, bool eof );
	virtual void error( int err, const char *text );

        void setCharsetHint( const QString& charset ) { m_charsetHint = charset; }
        void setCharset( const QString& charset ) { m_charset = charset; }
        QString checkCharset(const QByteArray& buffer ) const;


    protected:
        void checkNotify();

	DOM::DOMString m_sheet;
        QString m_charset;
        QString m_charsetHint;
	int m_err;
	QString m_errText;
    };

    /**
     * a cached script
     */
    class CachedScript : public CachedObject
    {
    public:
	CachedScript(DocLoader* dl, const DOM::DOMString &url, KIO::CacheControl cachePolicy, const char* accept );
	CachedScript(const DOM::DOMString &url, const QString &script_data);

	const DOM::DOMString &script() const { return m_script; }

	virtual void ref(CachedObjectClient *consumer);

	virtual void data( QBuffer &buffer, bool eof );
	virtual void error( int err, const char *text );

	void checkNotify();

        bool isLoaded() const { return !m_loading; }
        void setCharset( const QString& charset ) { m_charset = charset; }

    protected:
        QString m_charset;
	DOM::DOMString m_script;
    };

    class ImageSource;

    /**
     * a cached image
     */
    class CachedImage : public QObject, public CachedObject, public khtmlImLoad::ImageOwner
    {
	Q_OBJECT
    public:
	CachedImage(DocLoader* dl, const DOM::DOMString &url, KIO::CacheControl cachePolicy, const char* accept);
	virtual ~CachedImage();

	QPixmap pixmap() const;
	QPixmap* scaled_pixmap(int xWidth, int xHeight);
	QPixmap  tiled_pixmap(const QColor& bg, int xWidth = -1, int xHeight = -1);

        QSize pixmap_size() const;    // returns the size of the complete (i.e. when finished) loading
        //QRect valid_rect() const;     // returns the rectangle of pixmap that has been loaded already

        bool canRender() const { return !isErrorImage() && pixmap_size().width() > 0 && pixmap_size().height() > 0; }
        void ref(CachedObjectClient *consumer);
	virtual void deref(CachedObjectClient *consumer);


	virtual void data( QBuffer &buffer, bool eof );
	virtual void error( int err, const char *text );


        bool isComplete() const { return i && i->complete(); }
        bool isTransparent() const { return false; } //### isFullyTransparent; }
        bool isErrorImage() const { return m_hadError; }
        bool isBlockedImage() const { return m_wasBlocked; }
        const QString& suggestedFilename() const { return m_suggestedFilename; }
        void setSuggestedFilename( const QString& s ) { m_suggestedFilename = s;  }
#ifdef IMAGE_TITLES
        const QString& suggestedTitle() const { return m_suggestedTitle; }
        void setSuggestedTitle( const QString& s ) { m_suggestedTitle = s;  }
#else
        const QString& suggestedTitle() const { return m_suggestedFilename; }
#endif

        void setShowAnimations( KHTMLSettings::KAnimationAdvice );

	virtual void finish();


        khtmlImLoad::Image* image() { return i; }

    protected:
	void clear();

    private:
        /**
         Interface to the image
        */
        virtual void imageHasGeometry(khtmlImLoad::Image* img, int width, int height);
        virtual void imageChange     (khtmlImLoad::Image* img, QRect region);
        virtual void imageError      (khtmlImLoad::Image* img);
        virtual void imageDone       (khtmlImLoad::Image* img);
    private:
        void doNotifyFinished();

        void do_notify(const QRect& r);
        khtmlImLoad::Image* i;

        QString m_suggestedFilename;
#ifdef IMAGE_TITLES
        QString m_suggestedTitle;
#endif
        QPixmap* bg;
        QPixmap* scaled;
        QRgb bgColor;
        QSize bgSize;

        friend class Cache;
        friend class ::KHTMLPart;
    };

    /**
     * a cached sound
     */
    class CachedSound : public CachedObject
    {
    public:
	CachedSound(DocLoader* dl, const DOM::DOMString &url, KIO::CacheControl cachePolicy, const char* accept );

	QByteArray sound() const { return m_sound; }

	virtual void ref(CachedObjectClient *consumer);
	virtual void data( QBuffer &buffer, bool eof );
	virtual void error( int err, const char *text );

	void checkNotify();

        bool isLoaded() const { return !m_loading; }

    protected:
	QByteArray m_sound;
    };

    /**
     * a cached font
     */
    class CachedFont : public CachedObject
    {
    public:
	CachedFont(DocLoader* dl, const DOM::DOMString &url, KIO::CacheControl cachePolicy, const char* accept );

	QByteArray font() const { return m_font; }

	virtual void ref(CachedObjectClient *consumer);
	virtual void data( QBuffer &buffer, bool eof );
	virtual void error( int err, const char *text );

	void checkNotify();

        bool isLoaded() const { return !m_loading; }

    protected:
	QByteArray m_font;
    };

    /**
     * @internal
     *
     * Manages the loading of scripts/images/stylesheets for a particular document
     */
    class DocLoader
    {
    public:
 	DocLoader(KHTMLPart*, DOM::DocumentImpl*);
 	~DocLoader();

	CachedImage *requestImage( const DOM::DOMString &url);
	CachedCSSStyleSheet *requestStyleSheet( const DOM::DOMString &url, const QString& charsetHint,
						const char *accept = "text/css", bool userSheet = false );
        CachedScript *requestScript( const DOM::DOMString &url, const QString& charset);
        CachedSound *requestSound( const DOM::DOMString &url );
        CachedFont *requestFont( const DOM::DOMString &url );
	bool willLoadMediaElement( const DOM::DOMString &url);

	bool autoloadImages() const { return m_bautoloadImages; }
        KIO::CacheControl cachePolicy() const { return m_cachePolicy; }
        KHTMLSettings::KAnimationAdvice showAnimations() const { return m_showAnimations; }
        QDateTime expireDate() const { return m_expireDate; }
        KHTMLPart* part() const { return m_part; }
        DOM::DocumentImpl* doc() const { return m_doc; }

        void setCacheCreationDate(const QDateTime &);
        void setExpireDate(const QDateTime &);
        void setRelativeExpireDate(qint64 seconds);
        void setAutoloadImages( bool );
        void setCachePolicy( KIO::CacheControl cachePolicy ) { m_cachePolicy = cachePolicy; }
        void setShowAnimations( KHTMLSettings::KAnimationAdvice );
        void insertCachedObject( CachedObject* o ) const;
        void removeCachedObject( CachedObject* o) const { m_docObjects.remove( o ); }
        void clearPreloads();
        void registerPreload(CachedObject*);
        void printPreloadStats();

    private:
        bool needReload(CachedObject *existing, const QString &fullUrl);

        friend class Cache;
        friend class DOM::DocumentImpl;
        friend class ::KHTMLPart;

        QStringList m_reloadedURLs;
        mutable QSet<CachedObject*> m_docObjects;
        QSet<CachedObject*> m_preloads;
    QDateTime m_expireDate;
    QDateTime m_creationDate;
    KIO::CacheControl m_cachePolicy;
        bool m_bautoloadImages : 1;
        KHTMLSettings::KAnimationAdvice m_showAnimations : 2;
        KHTMLPart* m_part;
        DOM::DocumentImpl* m_doc;
    };

    /**
     * @internal
     */
    class Request
    {
    public:
	Request(DocLoader* dl, CachedObject *_object, bool _incremental, int priority);
	~Request();
	bool incremental;
	int priority; // -10 to 10, smaller values mean higher priority
	QBuffer m_buffer;
	CachedObject *object;
        DocLoader* m_docLoader;
    };

    /**
     * @internal
     */
    class Loader : public QObject
    {

	Q_OBJECT

    public:
	Loader();
	~Loader();

	void load(DocLoader* dl, CachedObject *object, bool incremental = true, int priority = 0);

        int numRequests( DocLoader* dl ) const;
        void cancelRequests( DocLoader* dl );

        // may return 0L
        KIO::Job *jobForRequest( const DOM::DOMString &url ) const;

    Q_SIGNALS:
        void requestStarted( khtml::DocLoader* dl, khtml::CachedObject* obj );
	void requestDone( khtml::DocLoader* dl, khtml::CachedObject *obj );
	void requestFailed( khtml::DocLoader* dl, khtml::CachedObject *obj );

    protected Q_SLOTS:
	void slotFinished( KJob * );
	void slotMimetype( KIO::Job *, const QString& s);
	void slotData( KIO::Job *, const QByteArray & );

    protected:
        void scheduleRequest(Request* req);
        QHash<KIO::Job*,Request*> m_requestsLoading;
        QStringList m_supportedImageTypes;
    };

        /**
     * @internal
     *
     * Provides a cache/loader for objects needed for displaying the html page.
     * At the moment these are stylesheets, scripts and images
     */
    class Cache
    {
	friend class DocLoader;

        template<typename CachedObjectType, enum CachedObject::Type CachedType>
        static CachedObjectType* requestObject( DocLoader* dl, const QUrl& kurl, const char* accept );

    public:
	/**
	 * init the cache in case it's not already. This needs to get called once
	 * before using it.
	 */
	KHTML_EXPORT static void init();

	/**
	 * Ask the cache for some url. Will return a cachedObject, and
	 * load the requested data in case it's not cached
         * if the DocLoader is zero, the url must be full-qualified.
         * Otherwise, it is automatically base-url expanded
	 */
// 	static CachedImage *requestImage(const QUrl& url)
//         { return Cache::requestObject<CachedImage, CachedObject::Image>( 0, url, 0 ); }

        /**
         * Pre-loads a stylesheet into the cache.
         */
        static void preloadStyleSheet(const QString &url, const QString &stylesheet_data);

        /**
         * Pre-loads a script into the cache.
         */
        static void preloadScript(const QString &url, const QString &script_data);

	static void setSize( int bytes );
	static int size() { return maxSize; }
	static void statistics();
	KHTML_EXPORT static void flush(bool force=false);

	/**
	 * clears the cache
	 * Warning: call this only at the end of your program, to clean
	 * up memory (useful for finding memory holes)
	 */
	KHTML_EXPORT static void clear();

	static Loader *loader() { return m_loader; }

    	static QPixmap *nullPixmap;
        static QPixmap *brokenPixmap;
        static QPixmap *blockedPixmap;
        static int cacheSize;

        static void removeCacheEntry( CachedObject *object );

    private:

        static void checkLRUAndUncacheableListIntegrity();

        friend class CachedObject;

	static QHash<QString,CachedObject*> *cache;
        static QLinkedList<DocLoader*>* docloader;
        static QLinkedList<CachedObject*> *freeList;
        static void insertInLRUList(CachedObject*);
        static void removeFromLRUList(CachedObject*);

        static int totalSizeOfLRU;
	static int maxSize;

	static Loader *m_loader;
    };

} // namespace

#endif
