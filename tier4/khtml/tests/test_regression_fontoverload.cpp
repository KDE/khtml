/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2003 Stephan Kulow (coolo@kde.org)
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
 *
 */

#include "ecma/kjs_proxy.h"

#define QT_NO_FONTCONFIG 1
#define QT_NO_FREETYPE   1
#include <private/qfontengine_p.h>
#pragma message ("Port to Qt5")
#if 0
#  include <private/qfontengine_x11_p.h>
#endif
#include <QFontDatabase>
#include <QFont>
#include "khtml_settings.h"
#include <QImage>
#include <assert.h>
#include <QX11Info>
#include <QPainter>
#if HAVE_X11
#include <fixx11h.h>
#endif
#include <QtGlobal>

struct MetricsInfo {
    const char* name;
    int ascent;
    int descent;
    int leading;
};


typedef QFixed QtFontDim;

static int dimToInt(QtFontDim dim) {
    return dim.toInt();
}


static const MetricsInfo compatMetrics[] = {
    {"-Adobe-Courier-Medium-R-Normal--10-100-75-75-M-60-ISO10646-1", 8, 1, 2},
    {"-Adobe-Courier-Medium-O-Normal--10-100-75-75-M-60-ISO10646-1", 8, 1, 2},
    {"-Adobe-Courier-Bold-R-Normal--10-100-75-75-M-60-ISO10646-1", 8, 1, 2},
    {"-Adobe-Courier-Bold-O-Normal--10-100-75-75-M-60-ISO10646-1", 8, 1, 2},
    {"-Adobe-Courier-Medium-R-Normal--12-120-75-75-M-70-ISO10646-1", 10, 2, 2},
    {"-Adobe-Courier-Medium-O-Normal--12-120-75-75-M-70-ISO10646-1", 10, 2, 2},
    {"-Adobe-Courier-Bold-R-Normal--12-120-75-75-M-70-ISO10646-1", 10, 2, 2},
    {"-Adobe-Courier-Bold-O-Normal--12-120-75-75-M-70-ISO10646-1", 10, 2, 2},
    {"-Adobe-Courier-Medium-R-Normal--18-180-75-75-M-110-ISO10646-1", 14, 3, 3},
    {"-Adobe-Courier-Medium-O-Normal--18-180-75-75-M-110-ISO10646-1", 14, 3, 3},
    {"-Adobe-Courier-Bold-R-Normal--18-180-75-75-M-110-ISO10646-1", 14, 3, 3},
    {"-Adobe-Courier-Bold-O-Normal--18-180-75-75-M-110-ISO10646-1", 14, 3, 3},
    {"-Adobe-Courier-Medium-R-Normal--24-240-75-75-M-150-ISO10646-1", 19, 4, 4},
    {"-Adobe-Courier-Medium-O-Normal--24-240-75-75-M-150-ISO10646-1", 19, 4, 4},
    {"-Adobe-Courier-Bold-R-Normal--24-240-75-75-M-150-ISO10646-1", 19, 4, 4},
    {"-Adobe-Courier-Bold-O-Normal--24-240-75-75-M-150-ISO10646-1", 19, 4, 4},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Helvetica-Medium-R-Normal--10-100-75-75-P-56-ISO10646-1", 10, 1, 2},
    {"-Adobe-Helvetica-Medium-O-Normal--10-100-75-75-P-57-ISO10646-1", 10, 1, 2},
    {"-Adobe-Helvetica-Bold-R-Normal--10-100-75-75-P-60-ISO10646-1", 10, 1, 2},
    {"-Adobe-Helvetica-Bold-O-Normal--10-100-75-75-P-60-ISO10646-1", 10, 1, 2},
    {"-Adobe-Helvetica-Medium-R-Normal--12-120-75-75-P-67-ISO10646-1", 11, 2, 2},
    {"-Adobe-Helvetica-Medium-O-Normal--12-120-75-75-P-67-ISO10646-1", 11, 2, 2},
    {"-Adobe-Helvetica-Bold-R-Normal--12-120-75-75-P-70-ISO10646-1", 11, 2, 2},
    {"-Adobe-Helvetica-Bold-O-Normal--12-120-75-75-P-69-ISO10646-1", 11, 2, 2},
    {"-Adobe-Helvetica-Medium-R-Normal--18-180-75-75-P-98-ISO10646-1", 16, 4, 3},
    {"-Adobe-Helvetica-Medium-O-Normal--18-180-75-75-P-98-ISO10646-1", 16, 4, 3},
    {"-Adobe-Helvetica-Bold-R-Normal--18-180-75-75-P-103-ISO10646-1", 16, 4, 3},
    {"-Adobe-Helvetica-Bold-O-Normal--18-180-75-75-P-104-ISO10646-1", 16, 4, 3},
    {"-Adobe-Helvetica-Medium-R-Normal--24-240-75-75-P-130-ISO10646-1", 22, 4, 4},
    {"-Adobe-Helvetica-Medium-O-Normal--24-240-75-75-P-130-ISO10646-1", 22, 4, 4},
    {"-Adobe-Helvetica-Bold-R-Normal--24-240-75-75-P-138-ISO10646-1", 22, 4, 4},
    {"-Adobe-Helvetica-Bold-O-Normal--24-240-75-75-P-138-ISO10646-1", 22, 4, 4},
    {"-Adobe-Courier-Medium-R-Normal--10-100-75-75-M-60-ISO10646-1", 8, 1, 2},
    {"-Adobe-Courier-Medium-O-Normal--10-100-75-75-M-60-ISO10646-1", 8, 1, 2},
    {"-Adobe-Courier-Bold-R-Normal--10-100-75-75-M-60-ISO10646-1", 8, 1, 2},
    {"-Adobe-Courier-Bold-O-Normal--10-100-75-75-M-60-ISO10646-1", 8, 1, 2},
    {"-Adobe-Courier-Medium-R-Normal--12-120-75-75-M-70-ISO10646-1", 10, 2, 2},
    {"-Adobe-Courier-Medium-O-Normal--12-120-75-75-M-70-ISO10646-1", 10, 2, 2},
    {"-Adobe-Courier-Bold-R-Normal--12-120-75-75-M-70-ISO10646-1", 10, 2, 2},
    {"-Adobe-Courier-Bold-O-Normal--12-120-75-75-M-70-ISO10646-1", 10, 2, 2},
    {"-Adobe-Courier-Medium-R-Normal--18-180-75-75-M-110-ISO10646-1", 14, 3, 3},
    {"-Adobe-Courier-Medium-O-Normal--18-180-75-75-M-110-ISO10646-1", 14, 3, 3},
    {"-Adobe-Courier-Bold-R-Normal--18-180-75-75-M-110-ISO10646-1", 14, 3, 3},
    {"-Adobe-Courier-Bold-O-Normal--18-180-75-75-M-110-ISO10646-1", 14, 3, 3},
    {"-Adobe-Courier-Medium-R-Normal--24-240-75-75-M-150-ISO10646-1", 19, 4, 4},
    {"-Adobe-Courier-Medium-O-Normal--24-240-75-75-M-150-ISO10646-1", 19, 4, 4},
    {"-Adobe-Courier-Bold-R-Normal--24-240-75-75-M-150-ISO10646-1", 19, 4, 4},
    {"-Adobe-Courier-Bold-O-Normal--24-240-75-75-M-150-ISO10646-1", 19, 4, 4},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Times-Medium-R-Normal--8-80-75-75-P-44-ISO10646-1", 7, 1, 1},
    {"-Adobe-Helvetica-Medium-R-Normal--10-100-75-75-P-56-ISO10646-1", 10, 1, 2},
    {"-Adobe-Helvetica-Medium-O-Normal--10-100-75-75-P-57-ISO10646-1", 10, 1, 2},
    {"-Adobe-Helvetica-Bold-R-Normal--10-100-75-75-P-60-ISO10646-1", 10, 1, 2},
    {"-Adobe-Helvetica-Bold-O-Normal--10-100-75-75-P-60-ISO10646-1", 10, 1, 2},
    {"-Adobe-Helvetica-Medium-R-Normal--12-120-75-75-P-67-ISO10646-1", 11, 2, 2},
    {"-Adobe-Helvetica-Medium-O-Normal--12-120-75-75-P-67-ISO10646-1", 11, 2, 2},
    {"-Adobe-Helvetica-Bold-R-Normal--12-120-75-75-P-70-ISO10646-1", 11, 2, 2},
    {"-Adobe-Helvetica-Bold-O-Normal--12-120-75-75-P-69-ISO10646-1", 11, 2, 2},
    {"-Adobe-Helvetica-Medium-R-Normal--18-180-75-75-P-98-ISO10646-1", 16, 4, 3},
    {"-Adobe-Helvetica-Medium-O-Normal--18-180-75-75-P-98-ISO10646-1", 16, 4, 3},
    {"-Adobe-Helvetica-Bold-R-Normal--18-180-75-75-P-103-ISO10646-1", 16, 4, 3},
    {"-Adobe-Helvetica-Bold-O-Normal--18-180-75-75-P-104-ISO10646-1", 16, 4, 3},
    {"-Adobe-Helvetica-Medium-R-Normal--24-240-75-75-P-130-ISO10646-1", 22, 4, 4},
    {"-Adobe-Helvetica-Medium-O-Normal--24-240-75-75-P-130-ISO10646-1", 22, 4, 4},
    {"-Adobe-Helvetica-Bold-R-Normal--24-240-75-75-P-138-ISO10646-1", 22, 4, 4},
    {"-Adobe-Helvetica-Bold-O-Normal--24-240-75-75-P-138-ISO10646-1", 22, 4, 4},
    {0, 0, 0, 0}
};

static const MetricsInfo* grabMetrics(const QString &name)
{
    for (int pos = 0; compatMetrics[pos].name; ++pos)
        if (name == QLatin1String(compatMetrics[pos].name))
            return &compatMetrics[pos];
    return 0;
}

class QFakeFontEngine : public QFontEngineXLFD
{
public:
    QString name;

    QFakeFontEngine( XFontStruct *fs, const char *name, int size );
    ~QFakeFontEngine();

    bool  haveMetrics;
    QtFontDim m_ascent, m_descent, m_leading;
    bool  ahem;
    int   pixS;
    XFontStruct* xfs;

    QtFontDim fallBackWidth() const {
        QtFontDim fbw = xfs->min_bounds.width;
        if (xfs->per_char) {
            if (haveMetrics)
                fbw = m_ascent; //### we really should get rid of these and regen..
            else
                fbw = xfs->ascent;
        }
        return fbw;
    }

    void recalcAdvances(QGlyphLayout *glyphs, QTextEngine::ShaperFlags flags) const
    {
        QFontEngineXLFD::recalcAdvances(glyphs, flags);

        // Go through the glyhs with glyph 0 and fix up their x advance
        // to make sense (or at least match Qt3)
        QtFontDim fbw = fallBackWidth();

        for (int c = 0; c < glyphs->numGlyphs; ++c) {
            if (glyphs->glyphs[c] == 0x200B) // ZERO WIDTH SPACE
                glyphs->advances_x[c] = 0;
            else if (!glyphs->glyphs[c])
                glyphs->advances_x[c] = fbw;
        }
    }

    Type type() const
    {
        if (ahem) {
            return QFontEngine::Freetype;
        } else
            return QFontEngine::XLFD;
    }


    QtFontDim ascent() const
    {
      if (haveMetrics)
        return m_ascent;
      else
        return QFontEngineXLFD::ascent();
    }

    QtFontDim descent() const
    {
      if (haveMetrics)
        return m_descent;
      else
        return QFontEngineXLFD::descent();
    }

    QtFontDim leading() const
    {
      if (ahem)
        return 0;
      else if (haveMetrics)
        return m_leading;
      else
        return QFontEngineXLFD::leading();
    }

    bool canRender( const QChar *string,  int len );
};

//OK. This is evil. Since we don't use Xft, we hijack the FreeType painting hook in the X11 engine
//for ahem, as unfortunately the drawing is in the paint engine, and not the font engine in Qt4
class Q_DECL_EXPORT QX11PaintEngine: public QPaintEngine
{
    void drawFreetype(const QPointF &p, const QTextItemInt &si);
};

void QX11PaintEngine::drawFreetype(const QPointF &p, const QTextItemInt &si)
{
    int cnt = si.glyphs.numGlyphs;

    if (!cnt) return;

    QFakeFontEngine *eng = static_cast<QFakeFontEngine*>(si.fontEngine);


    int x       = int(p.x());
    int y       = int(p.y());
    int pixS    = int(eng->pixS);
    int advance = pixS;
    int ascent  = dimToInt(eng->ascent());
    int descent = dimToInt(eng->descent());

    if (si.flags & QTextItem::RightToLeft)
    {
        x       = x + advance * (cnt - 1);
        advance = -advance;
    }

    for (int pos = 0; pos < cnt; ++pos)
    {
        QRect rect;

        switch (si.chars[pos].unicode())
        {
        case ' ':
        case 0x00A0: // NON-BREAKING SPACE
            rect = QRect();
            break;
        case 0x200B: // ZERO WIDTH SPACE
            continue;
        case 'p':
            //Below the baseline, including it
            rect = QRect(x, y, pixS, descent + 1);
            break;
        case 0xC9:
            //Above the baseline
            rect = QRect(x, y - ascent, pixS, ascent);
            break;
        default:
            //Whole block
            rect = QRect(x, y - ascent, pixS, pixS);
        }

        QPainter* p = painter();
        p->fillRect(rect, p->pen().color());

        x += advance;
    }
}


QFakeFontEngine::QFakeFontEngine( XFontStruct *fs, const char *name, int size )
    : QFontEngineXLFD( fs,  name,  0)
{
    xfs = fs;
    pixS = size;
    this->name = QLatin1String(name);
    ahem = this->name.contains("ahem");

    const MetricsInfo* metrics = grabMetrics(name);
    if (metrics)
    {
        haveMetrics = true;
        m_ascent  = metrics->ascent;
        m_descent = metrics->descent;
        m_leading = metrics->leading;
    }
    else
        haveMetrics = false;
}

QFakeFontEngine::~QFakeFontEngine()
{
}


bool QFakeFontEngine::canRender( const QChar *, int )
{
    return true;
}

static QString courier_pickxlfd( int pixelsize, bool italic, bool bold )
{
    if ( pixelsize >= 24 )
        pixelsize = 24;
    else if ( pixelsize >= 18 )
        pixelsize = 18;
    else if ( pixelsize >= 12 )
        pixelsize = 12;
    else
        pixelsize = 10;

    return QString( "-adobe-courier-%1-%2-normal--%3-*-75-75-m-*-iso10646-1" ).arg( bold ? "bold" : "medium" ).arg( italic ? "o" : "r" ).arg( pixelsize );
}

static QString ahem_pickxlfd( int pixelsize )
{
    return QString( "-misc-ahem-medium-r-normal--%1-*-100-100-c-*-iso10646-1" ).arg( pixelsize );
}

static QString helv_pickxlfd( int pixelsize, bool italic, bool bold )
{
    if ( pixelsize >= 24 )
        pixelsize = 24;
    else if ( pixelsize >= 18 )
        pixelsize = 18;
    else if ( pixelsize >= 12 )
        pixelsize = 12;
    else
        pixelsize = 10;

    return QString( "-adobe-helvetica-%1-%2-normal--%3-*-75-75-p-*-iso10646-1" ).arg( bold ? "bold" : "medium" ).arg( italic ? "o" : "r" ).arg( pixelsize );

}

static QFontEngine* loadFont(const QFontDef& request)
{
    QString xlfd;
    QStringList flist = request.family.toLower().split(",");
    foreach (QString family, flist) {
        if (!KHTMLSettings::availableFamilies().contains( ","+ family + ",", Qt::CaseInsensitive ))
            continue;

        if ( family == "adobe courier" || family == "courier" || family == "fixed" ) {
            xlfd = courier_pickxlfd( request.pixelSize, request.style == QFont::StyleItalic, request.weight > 50 );
        } else if ( family == "times new roman" || family == "times" ) {
            xlfd = "-adobe-times-medium-r-normal--8-80-75-75-p-44-iso10646-1";
        } else if ( family == "ahem" ) {
            xlfd = ahem_pickxlfd( request.pixelSize );
        } else {
            xlfd = helv_pickxlfd( request.pixelSize,  request.style == QFont::StyleItalic, request.weight > 50 );
        }
        break;
    }
    if (xlfd.isEmpty()) {
        xlfd = helv_pickxlfd( request.pixelSize,  request.style == QFont::StyleItalic, request.weight > 50 );
    }

    QFontEngine *fe = 0;

    XFontStruct *xfs;
	xfs = XLoadQueryFont( QX11Info::display(), xlfd.toLatin1() );
    if (!xfs) // as long as you don't do screenshots, it's maybe fine
	qFatal("we need some fonts. So make sure you have %s installed.", qPrintable(xlfd));

    unsigned long value;
    if ( !XGetFontProperty( xfs, XA_FONT, &value ) )
        return 0;

	char *n = XGetAtomName( QX11Info::display(), value );
    xlfd = n;
    if ( n )
        XFree( n );

    fe = new QFakeFontEngine( xfs, xlfd.toLatin1(),request.pixelSize );
    return fe;
}

Q_DECL_EXPORT
QFontEngine *QFontDatabase::loadXlfd(int /* screen */, int /* script */,
            const QFontDef &request, int /* force_encoding_id */)
{
    return loadFont(request);
}

extern "C" Q_DECL_EXPORT int FcInit() {
    /* Make sure Qt uses the Xlfd path, which we intercept */
    return 0;
}

Q_DECL_EXPORT bool QFontDatabase::isBitmapScalable( const QString &,
				      const QString &) const
{
    return true;
}

Q_DECL_EXPORT bool  QFontDatabase::isSmoothlyScalable( const QString &,
                                         const QString &) const
{
    return true;
}

const QString &KHTMLSettings::availableFamilies()
{
    if ( !avFamilies ) {
        avFamilies = new QString;
        *avFamilies = ",Adobe Courier,Arial,Comic Sans MS,Courier,Helvetica,Times,Times New Roman,Utopia,Fixed,Ahem,";
    }

  return *avFamilies;
}


bool KHTMLSettings::unfinishedImageFrame() const
{
  return false;
}

Q_DECL_EXPORT int QX11Info::appDpiY( int )
{
    return 100;
}

Q_DECL_EXPORT int QX11Info::appDpiX( int )
{
    return 100;
}

Q_DECL_EXPORT void QFont::insertSubstitution(const QString &,
                               const QString &)
{
}

Q_DECL_EXPORT void QFont::insertSubstitutions(const QString &,
                                const QStringList &)
{
}

#include <kprotocolinfo.h>
bool KProtocolInfo::isKnownProtocol( const QString& _protocol )
{
    return ( _protocol == "file" );
}

#include <kprotocolinfofactory.h>

QString KProtocolInfo::exec( const QString& _protocol )
{
    if ( _protocol != "file" )
        return QString();

    KProtocolInfo::Ptr prot = KProtocolInfoFactory::self()->findProtocol(_protocol);
    if ( !prot )
        return QString();

    return prot->m_exec;
}
/*
#include <dcopclient.h>

bool DCOPClient::attach()
{
    return false;
}

bool DCOPClient::isAttached() const
{
    return false;
}

void DCOPClient::processSocketData( int )
{
}*/

#include <QApplication>
#include <QPalette>

#if 0
Q_DECL_EXPORT void QApplication::setPalette( const QPalette &, bool ,
                               const char*  )
{
    static bool done = false;
    if (done) return;
    QString xlfd = AHEM;
    XFontStruct *xfs;
    xfs = XLoadQueryFont(QPaintDevice::x11AppDisplay(), xlfd.toLatin1().constData() );
    if (!xfs) // as long as you don't do screenshots, it's maybe fine
	qFatal("We will need some fonts. So make sure you have %s installed.", xlfd.toLatin1().constData());
    XFreeFont(QPaintDevice::x11AppDisplay(), xfs);
    done = true;
}
#endif

#include <kparts/historyprovider.h>

bool KParts::HistoryProvider::contains( const QString& t ) const
{
    return ( t == "http://www.kde.org/" || t == "http://www.google.com/");
}


#include <ksslsettings.h>

bool KSSLSettings::warnOnEnter() const       { return false; }
bool KSSLSettings::warnOnUnencrypted() const { return false; }
bool KSSLSettings::warnOnLeave() const       { return false; }

#include <kparts/plugin.h>

KParts::Plugin* KParts::Plugin::loadPlugin( QObject * /* parent */, const char* /* libname */) { return 0; }

#include <klineedit.h>

void KLineEdit::setClearButtonShown(bool /*show*/)
{}
