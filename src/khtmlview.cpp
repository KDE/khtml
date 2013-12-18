/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000-2004 Dirk Mueller <mueller@kde.org>
 *                     2003 Leo Savernik <l.savernik@aon.at>
 *                     2003-2008 Apple Computer, Inc.
 *                     2008 Allan Sandfeld Jensen <kde@carewolf.com>
 *                     2006-2008 Germain Garand <germain@ebooksfrance.org>
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


#include "khtmlview.h"


#include "khtml_part.h"
#include "khtml_events.h"
#include <config-khtml.h>
#if HAVE_X11
#include <qx11info_x11.h>
#endif

#include "html/html_documentimpl.h"
#include "html/html_inlineimpl.h"
#include "html/html_formimpl.h"
#include "html/htmltokenizer.h"
#include "editing/editor.h"
#include "rendering/render_arena.h"
#include "rendering/render_canvas.h"
#include "rendering/render_frames.h"
#include "rendering/render_replaced.h"
#include "rendering/render_form.h"
#include "rendering/render_layer.h"
#include "rendering/render_line.h"
#include "rendering/render_table.h"
// removeme
#define protected public
#include "rendering/render_text.h"
#undef protected
#include "xml/dom2_eventsimpl.h"
#include "css/cssstyleselector.h"
#include "css/csshelper.h"
#include "misc/helper.h"
#include "misc/loader.h"
#include "khtml_settings.h"
#include "khtml_printsettings.h"

#include "khtmlpart_p.h"

#include <kcursor.h>
#include <QDebug>
#include <kiconloader.h>
#include <knotification.h>
#include <kconfig.h>
#include <../khtml_version.h>

#include <kstringhandler.h>
#include <kconfiggroup.h>

#include <QBitmap>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QtCore/QObject>
#include <QPainter>
#include <QtCore/QHash>
#include <QToolTip>
#include <QtCore/QString>
#include <QTextDocument>
#include <QtCore/QTimer>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QVector>
#include <QAbstractScrollArea>
#include <QPrinter>
#include <QPrintDialog>
#include <qstandardpaths.h>

//#define DEBUG_FLICKER

#include <limits.h>
#if HAVE_X11
#include <X11/Xlib.h>
#include <fixx11h.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

#if 0
namespace khtml {
    void dumpLineBoxes(RenderFlow *flow);
}
#endif

using namespace DOM;
using namespace khtml;

#ifndef NDEBUG
static const int sFirstLayoutDelay = 520;
static const int sParsingLayoutsInterval = 380;
static const int sLayoutAttemptDelay = 300;
#else
static const int sFirstLayoutDelay = 280;
static const int sParsingLayoutsInterval = 320;
static const int sLayoutAttemptDelay = 200;
#endif
static const int sLayoutAttemptIncrement = 20;
static const int sParsingLayoutsIncrement = 60;

static const int sSmoothScrollTime = 128;
static const int sSmoothScrollTick = 16;
static const int sSmoothScrollMinStaticPixels = 320*200;

static const int sMaxMissedDeadlines = 12;
static const int sWayTooMany = -1;

class KHTMLViewPrivate {
    friend class KHTMLView;
public:

    enum PseudoFocusNodes {
	PFNone,
	PFTop,
	PFBottom
    };

    enum StaticBackgroundState {
         SBNone = 0,
         SBPartial,
         SBFull
    };

    enum CompletedState {
        CSNone = 0,
        CSFull,
        CSActionPending
    };

    KHTMLViewPrivate(KHTMLView* v)
        : underMouse( 0 ), underMouseNonShared( 0 ), oldUnderMouse( 0 )
    {
        postponed_autorepeat = NULL;
        scrollingFromWheelTimerId = 0;
        smoothScrollMode = KHTMLView::SSMWhenEfficient;

        reset();
        vpolicy = Qt::ScrollBarAsNeeded;
 	hpolicy = Qt::ScrollBarAsNeeded;
        formCompletions=0;
        prevScrollbarVisible = true;

        possibleTripleClick = false;
        emitCompletedAfterRepaint = CSNone;
        cursorIconWidget = 0;
        cursorIconType   = KHTMLView::LINK_NORMAL;
        m_mouseScrollTimer = 0;
        m_mouseScrollIndicator = 0;
        contentsX = 0;
        contentsY = 0;
        view = v;
    }
    ~KHTMLViewPrivate()
    {
        delete formCompletions;
        delete postponed_autorepeat;
        if (underMouse)
	    underMouse->deref();
        if (underMouseNonShared)
	    underMouseNonShared->deref();
        if (oldUnderMouse)
            oldUnderMouse->deref();

        delete cursorIconWidget;
        delete m_mouseScrollTimer;
        delete m_mouseScrollIndicator;
    }
    void reset()
    {
        if (underMouse)
	    underMouse->deref();
	underMouse = 0;
        if (underMouseNonShared)
	    underMouseNonShared->deref();
	underMouseNonShared = 0;
	if (oldUnderMouse)
	    oldUnderMouse->deref();
        oldUnderMouse = 0;
        linkPressed = false;
        staticWidget = SBNone;
        fixedObjectsCount = 0;
        staticObjectsCount = 0;
	tabMovePending = false;
	lastTabbingDirection = true;
	pseudoFocusNode = PFNone;
	zoomLevel = 100;
#ifndef KHTML_NO_SCROLLBARS
        //We don't turn off the toolbars here
	//since if the user turns them
	//off, then chances are they want them turned
	//off always - even after a reset.
#else
        vpolicy = ScrollBarAlwaysOff;
        hpolicy = ScrollBarAlwaysOff;
#endif
        scrollBarMoved = false;
        contentsMoving = false;
        ignoreWheelEvents = false;
        scrollingFromWheel = QPoint(-1,-1);
	borderX = 30;
	borderY = 30;
        steps = 0;
	dx = dy = 0;
        paged = false;
	clickX = -1;
	clickY = -1;
	clickCount = 0;
	isDoubleClick = false;
	scrollingSelf = false;
        delete postponed_autorepeat;
        postponed_autorepeat = NULL;
	layoutTimerId = 0;
        repaintTimerId = 0;
        scrollTimerId = 0;
        scrollSuspended = false;
        scrollSuspendPreActivate = false;
        smoothScrolling = false;
        smoothScrollModeIsDefault = true;
        shouldSmoothScroll = false;
        smoothScrollMissedDeadlines = 0;
        hasFrameset = false;
        complete = false;
        firstLayoutPending = true;
#ifdef SPEED_DEBUG
        firstRepaintPending = true;
#endif
        needsFullRepaint = true;
        dirtyLayout = false;
        layoutSchedulingEnabled = true;
        painting = false;
        layoutCounter = 0;
        layoutAttemptCounter = 0;
        scheduledLayoutCounter = 0;
        updateRegion = QRegion();
        m_dialogsAllowed = true;
	accessKeysActivated = false;
	accessKeysPreActivate = false;

        // the view might have been built before the part it will be assigned to,
        // so exceptionally, we need to directly ref/deref KHTMLGlobal to
        // account for this transitory case.
        KHTMLGlobal::ref();
        accessKeysEnabled = KHTMLGlobal::defaultHTMLSettings()->accessKeysEnabled();
        KHTMLGlobal::deref();

        emitCompletedAfterRepaint = CSNone;
        m_mouseEventsTarget = 0;
        m_clipHolder = 0;
    }
    void newScrollTimer(QWidget *view, int tid)
    {
        //qDebug() << "newScrollTimer timer " << tid;
        view->killTimer(scrollTimerId);
        scrollTimerId = tid;
        scrollSuspended = false;
    }
    enum ScrollDirection { ScrollLeft, ScrollRight, ScrollUp, ScrollDown };

    void adjustScroller(QWidget *view, ScrollDirection direction, ScrollDirection oppositedir)
    {
        static const struct { int msec, pixels; } timings [] = {
            {320,1}, {224,1}, {160,1}, {112,1}, {80,1}, {56,1}, {40,1},
            {28,1}, {20,1}, {20,2}, {20,3}, {20,4}, {20,6}, {20,8}, {0,0}
        };
        if (!scrollTimerId ||
            (static_cast<int>(scrollDirection) != direction &&
             (static_cast<int>(scrollDirection) != oppositedir || scrollSuspended))) {
            scrollTiming = 6;
            scrollBy = timings[scrollTiming].pixels;
            scrollDirection = direction;
            newScrollTimer(view, view->startTimer(timings[scrollTiming].msec));
        } else if (scrollDirection == direction &&
                   timings[scrollTiming+1].msec && !scrollSuspended) {
            scrollBy = timings[++scrollTiming].pixels;
            newScrollTimer(view, view->startTimer(timings[scrollTiming].msec));
        } else if (scrollDirection == oppositedir) {
            if (scrollTiming) {
                scrollBy = timings[--scrollTiming].pixels;
                newScrollTimer(view, view->startTimer(timings[scrollTiming].msec));
            }
        }
        scrollSuspended = false;
    }

    bool haveZoom() const { return zoomLevel != 100; }

    void startScrolling()
    {
        smoothScrolling = true;
        smoothScrollTimer.start(sSmoothScrollTick);
        shouldSmoothScroll = false;
    }

    void stopScrolling()
    {
        smoothScrollTimer.stop();
        dx = dy = 0;
        steps = 0;
        updateContentsXY();
        smoothScrolling = false;
        shouldSmoothScroll = false;
    }

    void updateContentsXY()
    {
        contentsX = QApplication::isRightToLeft() ?
                        view->horizontalScrollBar()->maximum()-view->horizontalScrollBar()->value() : view->horizontalScrollBar()->value();
        contentsY = view->verticalScrollBar()->value();
    }
    void scrollAccessKeys(int dx, int dy)
    {
        QList<QLabel*> wl = view->widget()->findChildren<QLabel*>("KHTMLAccessKey");
        foreach(QLabel* w, wl) {
            w->move( w->pos() + QPoint(dx, dy) );
        }
    }
    void scrollExternalWidgets(int dx, int dy)
    {
        if (visibleWidgets.isEmpty())
            return;

        QHashIterator<void*, QWidget*> it(visibleWidgets);
        while (it.hasNext()) {
            it.next();
            it.value()->move( it.value()->pos() + QPoint(dx, dy) );
        }
    }

    NodeImpl *underMouse;
    NodeImpl *underMouseNonShared;
    NodeImpl *oldUnderMouse;

    // Do not adjust bitfield enums sizes.
    // They are oversized because they are signed on some platforms.
    bool tabMovePending:1;
    bool lastTabbingDirection:1;
    PseudoFocusNodes pseudoFocusNode:3;
    bool scrollBarMoved:1;
    bool contentsMoving:1;

    Qt::ScrollBarPolicy vpolicy;
    Qt::ScrollBarPolicy hpolicy;
    bool prevScrollbarVisible:1;
    bool linkPressed:1;
    bool ignoreWheelEvents:1;
    StaticBackgroundState staticWidget: 3;
    int staticObjectsCount;
    int fixedObjectsCount;

    int zoomLevel;
    int borderX, borderY;
    int dx, dy;
    int steps;
    KConfig *formCompletions;

    int clickX, clickY, clickCount;
    bool isDoubleClick;

    bool paged;

    bool scrollingSelf;
    int contentsX, contentsY;
    int layoutTimerId;
    QKeyEvent* postponed_autorepeat;

    int repaintTimerId;
    int scrollTimerId;
    int scrollTiming;
    int scrollBy;
    ScrollDirection scrollDirection		:3;
    bool scrollSuspended			:1;
    bool scrollSuspendPreActivate		:1;
    KHTMLView::SmoothScrollingMode smoothScrollMode :3;
    bool smoothScrolling                          :1;
    bool smoothScrollModeIsDefault                :1;
    bool shouldSmoothScroll                       :1;
    bool hasFrameset                              :1;
    bool complete				:1;
    bool firstLayoutPending			:1;
#ifdef SPEED_DEBUG
    bool firstRepaintPending                    :1;
#endif
    bool layoutSchedulingEnabled		:1;
    bool needsFullRepaint			:1;
    bool painting				:1;
    bool possibleTripleClick			:1;
    bool dirtyLayout                           :1;
    bool m_dialogsAllowed			:1;
    short smoothScrollMissedDeadlines;
    int layoutCounter;
    int layoutAttemptCounter;
    int scheduledLayoutCounter;
    QRegion updateRegion;
    QTimer smoothScrollTimer;
    QTime smoothScrollStopwatch;
    QHash<void*, QWidget*> visibleWidgets;
    bool accessKeysEnabled;
    bool accessKeysActivated;
    bool accessKeysPreActivate;
    CompletedState emitCompletedAfterRepaint;

    QLabel*               cursorIconWidget;
    KHTMLView::LinkCursor cursorIconType;

    // scrolling activated by MMB
    short m_mouseScroll_byX;
    short m_mouseScroll_byY;
    QPoint scrollingFromWheel;
    int scrollingFromWheelTimerId;
    QTimer *m_mouseScrollTimer;
    QWidget *m_mouseScrollIndicator;
    QPointer<QWidget> m_mouseEventsTarget;
    QStack<QRegion>* m_clipHolder;
    KHTMLView* view;
};

#ifndef QT_NO_TOOLTIP

/** calculates the client-side image map rectangle for the given image element
 * @param img image element
 * @param scrollOfs scroll offset of viewport in content coordinates
 * @param p position to be probed in viewport coordinates
 * @param r returns the bounding rectangle in content coordinates
 * @param s returns the title string
 * @return true if an appropriate area was found -- only in this case r and
 *	s are valid, false otherwise
 */
static bool findImageMapRect(HTMLImageElementImpl *img, const QPoint &scrollOfs,
			const QPoint &p, QRect &r, QString &s)
{
    HTMLMapElementImpl* map;
    if (img && img->document()->isHTMLDocument() &&
        (map = static_cast<HTMLDocumentImpl*>(img->document())->getMap(img->imageMap()))) {
        RenderObject::NodeInfo info(true, false);
        RenderObject *rend = img->renderer();
        int ax, ay;
        if (!rend || !rend->absolutePosition(ax, ay))
            return false;
        // we're a client side image map
        bool inside = map->mapMouseEvent(p.x() - ax + scrollOfs.x(),
                p.y() - ay + scrollOfs.y(), rend->contentWidth(),
                rend->contentHeight(), info);
        if (inside && info.URLElement()) {
            HTMLAreaElementImpl *area = static_cast<HTMLAreaElementImpl *>(info.URLElement());
            Q_ASSERT(area->id() == ID_AREA);
            s = area->getAttribute(ATTR_TITLE).string();
            QRegion reg = area->cachedRegion();
            if (!s.isEmpty() && !reg.isEmpty()) {
                r = reg.boundingRect();
                r.translate(ax, ay);
                return true;
            }
        }
    }
    return false;
}

bool KHTMLView::event( QEvent* e )
{
    switch ( e->type() ) {
    case QEvent::ToolTip: {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        QPoint     p   = he->pos();

        DOM::NodeImpl *node = d->underMouseNonShared;
        QRect region;
        while ( node ) {
            if ( node->isElementNode() ) {
                DOM::ElementImpl *e = static_cast<DOM::ElementImpl*>( node );
                QRect r;
                QString s;
                bool found = false;
                // for images, check if it is part of a client-side image map,
                // and query the <area>s' title attributes, too
                if (e->id() == ID_IMG && !e->getAttribute( ATTR_USEMAP ).isEmpty()) {
                    found = findImageMapRect(static_cast<HTMLImageElementImpl *>(e),
                                viewportToContents(QPoint(0, 0)), p, r, s);
                }
                if (!found) {
                    s = e->getAttribute( ATTR_TITLE ).string();
                    r = node->getRect();
                }
                region |= QRect( contentsToViewport( r.topLeft() ), r.size() );
                if ( !s.isEmpty() ) {
                    QToolTip::showText( he->globalPos(),
                        Qt::convertFromPlainText( s, Qt::WhiteSpaceNormal ),
                        widget(), region );
                    break;
                }
            }
            node = node->parentNode();
        }
        // Qt makes tooltip events happen nearly immediately when a preceding one was processed in the past few seconds.
        // We don't want that feature to apply to web tootlips however, as it clashes with dhtml menus.
        // So we'll just pretend we did not process that event.
        return false;
    }

    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::Drop:
      // In Qt4, one needs to both call accept() on the DND event and return
      // true on ::event for the candidate widget for the drop to be possible.
      // Apps hosting us, such as konq, can do the former but not the later.
      // We will do the second bit, as it's a no-op unless someone else explicitly
      // accepts the event. We need to skip the scrollarea to do that,
      // since it will just skip the events, both killing the drop, and
      // not permitting us to forward it up the part hiearchy in our dragEnterEvent,
      // etc. handlers
      return QWidget::event(e);
    case QEvent::StyleChange:
    case QEvent::LayoutRequest: {
         updateScrollBars();
         return QAbstractScrollArea::event(e);
    }
    case QEvent::PaletteChange:
      slotPaletteChanged();
      return QScrollArea::event(e);
    default:
      return QScrollArea::event(e);
    }
}
#endif

KHTMLView::KHTMLView( KHTMLPart *part, QWidget *parent )
    : QScrollArea( parent ), d( new KHTMLViewPrivate( this ) )
{
    m_medium = "screen";

    m_part = part;

    QScrollArea::setVerticalScrollBarPolicy(d->vpolicy);
    QScrollArea::setHorizontalScrollBarPolicy(d->hpolicy);

    initWidget();
    widget()->setMouseTracking(true);
}

KHTMLView::~KHTMLView()
{
    closeChildDialogs();
    if (m_part)
    {
        DOM::DocumentImpl *doc = m_part->xmlDocImpl();
        if (doc)
            doc->detach();
    }
    delete d;
}

void KHTMLView::setPart(KHTMLPart *part)
{
    assert(part && !m_part);
    m_part = part;
}

void KHTMLView::initWidget()
{
    // Do not access the part here. It might not be fully constructed.

    setFrameStyle(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    viewport()->setFocusProxy(this);

    _marginWidth = -1; // undefined
    _marginHeight = -1;
    _width = 0;
    _height = 0;

    installEventFilter(this);

    setAcceptDrops(true);
    if (!widget())
        setWidget( new QWidget(this) );
    widget()->setAttribute( Qt::WA_NoSystemBackground );

    // Do *not* remove this attribute frivolously.
    // You might not notice a change of behaviour in Debug builds
    // but removing opaque events will make QWidget::scroll fail horribly
    // in Release builds.
    widget()->setAttribute( Qt::WA_OpaquePaintEvent );

    verticalScrollBar()->setCursor( Qt::ArrowCursor );
    horizontalScrollBar()->setCursor( Qt::ArrowCursor );

    connect(&d->smoothScrollTimer, SIGNAL(timeout()), this, SLOT(scrollTick()));
}

void KHTMLView::resizeContentsToViewport()
{
    QSize s = viewport()->size();
    resizeContents(s.width(), s.height());
}


// called by KHTMLPart::clear()
void KHTMLView::clear()
{
    if (d->accessKeysEnabled && d->accessKeysActivated)
        accessKeysTimeout();
    viewport()->unsetCursor();
    if ( d->cursorIconWidget )
        d->cursorIconWidget->hide();
    if (d->smoothScrolling)
        d->stopScrolling();
    d->reset();
    QAbstractEventDispatcher::instance()->unregisterTimers(this);
    emit cleared();

    QScrollArea::setHorizontalScrollBarPolicy(d->hpolicy);
    QScrollArea::setVerticalScrollBarPolicy(d->vpolicy);
    verticalScrollBar()->setEnabled( false );
    horizontalScrollBar()->setEnabled( false );

}

void KHTMLView::hideEvent(QHideEvent* e)
{
    QScrollArea::hideEvent(e);
}

void KHTMLView::showEvent(QShowEvent* e)
{
    QScrollArea::showEvent(e);
}

void KHTMLView::setMouseEventsTarget( QWidget* w )
{
    d->m_mouseEventsTarget = w;
}

QWidget* KHTMLView::mouseEventsTarget() const
{
    return d->m_mouseEventsTarget;
}

void KHTMLView::setClipHolder( QStack<QRegion>* ch )
{
    d->m_clipHolder = ch;
}

QStack<QRegion>* KHTMLView::clipHolder() const
{
    return d->m_clipHolder;
}

int KHTMLView::contentsWidth() const
{
    return widget() ? widget()->width() : 0;
}

int KHTMLView::contentsHeight() const
{
    return widget() ? widget()->height() : 0;
}

void KHTMLView::resizeContents(int w, int h)
{
    if (!widget())
        return;
    widget()->resize(w, h);
    if (!widget()->isVisible())
        updateScrollBars();
}

int KHTMLView::contentsX() const
{
    return d->contentsX;
}

int KHTMLView::contentsY() const
{
    return d->contentsY;
}

int KHTMLView::visibleWidth() const
{
    if (m_kwp->isRedirected()) {
        // our RenderWidget knows better
        if (RenderWidget* rw = m_kwp->renderWidget()) {
            int ret = rw->width()-rw->paddingLeft()-rw->paddingRight()-rw->borderLeft()-rw->borderRight();
            if (verticalScrollBar()->isVisible()) {
                ret -= verticalScrollBar()->sizeHint().width();
                ret = qMax(0, ret);
            }
            return ret;
        }
    }
    return viewport()->width();
}

int KHTMLView::visibleHeight() const
{
    if (m_kwp->isRedirected()) {
        // our RenderWidget knows better
        if (RenderWidget* rw = m_kwp->renderWidget()) {
            int ret = rw->height()-rw->paddingBottom()-rw->paddingTop()-rw->borderTop()-rw->borderBottom();
            if (horizontalScrollBar()->isVisible()) {
                ret -= horizontalScrollBar()->sizeHint().height();
                ret = qMax(0, ret);
            }
            return ret;
        }
    }
    return viewport()->height();
}

void KHTMLView::setContentsPos( int x, int y)
{
   horizontalScrollBar()->setValue( QApplication::isRightToLeft() ?
                           horizontalScrollBar()->maximum()-x : x );
   verticalScrollBar()->setValue( y );
}

void KHTMLView::scrollBy(int x, int y)
{
   if (d->scrollTimerId)
       d->newScrollTimer(this, 0);
   horizontalScrollBar()->setValue( horizontalScrollBar()->value()+x );
   verticalScrollBar()->setValue( verticalScrollBar()->value()+y );
}

QPoint KHTMLView::contentsToViewport(const QPoint& p) const
{
    return QPoint(p.x()-contentsX(), p.y()-contentsY());
}

void KHTMLView::contentsToViewport(int x, int y, int& cx, int& cy) const
{
    QPoint p(x,y);
    p = contentsToViewport(p);
    cx = p.x();
    cy = p.y();
}

QPoint KHTMLView::viewportToContents(const QPoint& p) const
{
    return QPoint(p.x()+contentsX(), p.y()+contentsY());
}

void KHTMLView::viewportToContents(int x, int y, int& cx, int& cy) const
{
    QPoint p(x,y);
    p = viewportToContents(p);
    cx = p.x();
    cy = p.y();
}

void KHTMLView::updateContents(int x, int y, int w, int h)
{
    applyTransforms(x, y, w, h);
    if (m_kwp->isRedirected()) {
        QPoint off = m_kwp->absolutePos();
        KHTMLView* pview = m_part->parentPart()->view();
        pview->updateContents(x+off.x(), y+off.y(), w, h);
    } else
        widget()->update(x, y, w, h);
}

void KHTMLView::updateContents( const QRect& r )
{
    updateContents( r.x(), r.y(), r.width(), r.height() );
}

void KHTMLView::repaintContents(int x, int y, int w, int h)
{
    applyTransforms(x, y, w, h);
    if (m_kwp->isRedirected()) {
        QPoint off = m_kwp->absolutePos();
        KHTMLView* pview = m_part->parentPart()->view();
        pview->repaintContents(x+off.x(), y+off.y(), w, h);
    } else
        widget()->repaint(x, y, w, h);
}

void KHTMLView::repaintContents( const QRect& r )
{
    repaintContents( r.x(), r.y(), r.width(), r.height() );
}

void KHTMLView::applyTransforms( int& x, int& y, int& w, int& h) const
{
    if (d->haveZoom()) {
        const int z = d->zoomLevel;
        x = x*z/100;
        y = y*z/100;
        w = w*z/100;
        h = h*z/100;
    }
    x -= contentsX();
    y -= contentsY();
}

void KHTMLView::revertTransforms( int& x, int& y, int& w, int& h) const
{
    x += contentsX();
    y += contentsY();
    if (d->haveZoom()) {
        const int z = d->zoomLevel;
        x = x*100/z;
        y = y*100/z;
        w = w*100/z;
        h = h*100/z;
    }
}

void KHTMLView::revertTransforms( int& x, int& y ) const
{
    int dummy = 0;
    revertTransforms(x, y, dummy, dummy);
}

void KHTMLView::resizeEvent (QResizeEvent* /*e*/)
{
    updateScrollBars();

    // If we didn't load anything, make white area as big as the view
    if (!m_part->xmlDocImpl())
        resizeContentsToViewport();

    // Viewport-dependent media queries may cause us to need completely different style information.
    if (m_part->xmlDocImpl() && m_part->xmlDocImpl()->styleSelector()->affectedByViewportChange()) {
         m_part->xmlDocImpl()->updateStyleSelector();
    }

    if (d->layoutSchedulingEnabled)
        layout();

    QApplication::sendPostedEvents(viewport(), QEvent::Paint);

    if ( m_part && m_part->xmlDocImpl() ) {
        if (m_part->parentPart()) {
            // sub-frame : queue the resize event until our toplevel is done layouting
            khtml::ChildFrame *cf = m_part->parentPart()->frame( m_part );
            if (cf && !cf->m_partContainerElement.isNull())
                cf->m_partContainerElement.data()->postResizeEvent();
        } else {
            // toplevel : dispatch sub-frames'resize events before our own
            HTMLPartContainerElementImpl::sendPostedResizeEvents();
            m_part->xmlDocImpl()->dispatchWindowEvent( EventImpl::RESIZE_EVENT, false, false );
        }
    }
}

void KHTMLView::paintEvent( QPaintEvent *e )
{
    QRect r = e->rect();
    QRect v(contentsX(), contentsY(), visibleWidth(), visibleHeight());
    QPoint off(contentsX(),contentsY());
    r.translate(off);
    r = r.intersect(v);
    if (!r.isValid() || r.isEmpty()) return;

    QPainter p(widget());
    p.translate(-off);

    if (d->haveZoom()) {
        p.scale( d->zoomLevel/100., d->zoomLevel/100.);

        r.setX(r.x()*100/d->zoomLevel);
        r.setY(r.y()*100/d->zoomLevel);
        r.setWidth(r.width()*100/d->zoomLevel);
        r.setHeight(r.height()*100/d->zoomLevel);
        r.adjust(-1,-1,1,1);
    }
    p.setClipRect(r);

    int ex = r.x();
    int ey = r.y();
    int ew = r.width();
    int eh = r.height();

    if(!m_part || !m_part->xmlDocImpl() || !m_part->xmlDocImpl()->renderer()) {
        p.fillRect(ex, ey, ew, eh, palette().brush(QPalette::Active, QPalette::Base));
        return;
    } else if ( d->complete && static_cast<RenderCanvas*>(m_part->xmlDocImpl()->renderer())->needsLayout() ) {
        // an external update request happens while we have a layout scheduled
        unscheduleRelayout();
        layout();
    } else if (m_part->xmlDocImpl()->tokenizer()) {
        m_part->xmlDocImpl()->tokenizer()->setNormalYieldDelay();
    }

    if (d->painting) {
        // qDebug() << "WARNING: paintEvent reentered! ";
        return;
    }
    d->painting = true;

    m_part->xmlDocImpl()->renderer()->layer()->paint(&p, r);

    if (d->hasFrameset) {
        NodeImpl *body = static_cast<HTMLDocumentImpl*>(m_part->xmlDocImpl())->body();
        if(body && body->renderer() && body->id() == ID_FRAMESET)
            static_cast<RenderFrameSet*>(body->renderer())->paintFrameSetRules(&p, r);
        else
            d->hasFrameset = false;
    }

    khtml::DrawContentsEvent event( &p, ex, ey, ew, eh );
    QApplication::sendEvent( m_part, &event );

    if (d->contentsMoving && !d->smoothScrolling && widget()->underMouse()) {
        QMouseEvent *tempEvent = new QMouseEvent( QEvent::MouseMove, widget()->mapFromGlobal( QCursor::pos() ),
                                              Qt::NoButton, Qt::NoButton, Qt::NoModifier );
        QApplication::postEvent(widget(), tempEvent);
    }
#ifdef SPEED_DEBUG
    if (d->firstRepaintPending && !m_part->parentPart()) {
        qDebug() << "FIRST PAINT:" << m_part->d->m_parsetime.elapsed();
    }
    d->firstRepaintPending = false;
#endif
    d->painting = false;
}

void KHTMLView::setMarginWidth(int w)
{
    // make it update the rendering area when set
    _marginWidth = w;
}

void KHTMLView::setMarginHeight(int h)
{
    // make it update the rendering area when set
    _marginHeight = h;
}

void KHTMLView::layout()
{
    if( m_part && m_part->xmlDocImpl() ) {
        DOM::DocumentImpl *document = m_part->xmlDocImpl();

        khtml::RenderCanvas* canvas = static_cast<khtml::RenderCanvas *>(document->renderer());
        if ( !canvas ) return;

        d->layoutSchedulingEnabled=false;
        d->dirtyLayout = true;

        // the reference object for the overflow property on canvas
        RenderObject * ref = 0;
        RenderObject* root = document->documentElement() ? document->documentElement()->renderer() : 0;

        if (document->isHTMLDocument()) {
             NodeImpl *body = static_cast<HTMLDocumentImpl*>(document)->body();
             if(body && body->renderer() && body->id() == ID_FRAMESET) {
                 QScrollArea::setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                 QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                 body->renderer()->setNeedsLayout(true);
                 d->hasFrameset = true;
             }
             else if (root) // only apply body's overflow to canvas if root has a visible overflow
                     ref = (!body || root->style()->hidesOverflow()) ? root : body->renderer();
        } else {
            ref = root;
        }
        if (ref) {
            if( ref->style()->overflowX() == OHIDDEN ) {
                if (d->hpolicy == Qt::ScrollBarAsNeeded) QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            } else if (ref->style()->overflowX() == OSCROLL ) {
                if (d->hpolicy == Qt::ScrollBarAsNeeded) QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            } else if (horizontalScrollBarPolicy() != d->hpolicy) {
                QScrollArea::setHorizontalScrollBarPolicy(d->hpolicy);
            }
            if ( ref->style()->overflowY() == OHIDDEN ) {
                if (d->vpolicy == Qt::ScrollBarAsNeeded) QScrollArea::setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            } else if (ref->style()->overflowY() == OSCROLL ) {
                if (d->vpolicy == Qt::ScrollBarAsNeeded) QScrollArea::setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            } else if (verticalScrollBarPolicy() != d->vpolicy) {
                QScrollArea::setVerticalScrollBarPolicy(d->vpolicy);
            }
        }
        d->needsFullRepaint = d->firstLayoutPending;
        if (_height !=  visibleHeight() || _width != visibleWidth()) {;
            d->needsFullRepaint = true;
            _height = visibleHeight();
            _width = visibleWidth();
        }

        canvas->layout();

        emit finishedLayout();
        if (d->firstLayoutPending) {
            // make sure firstLayoutPending is set to false now in case this layout
            // wasn't scheduled
            d->firstLayoutPending = false;
            verticalScrollBar()->setEnabled( true );
            horizontalScrollBar()->setEnabled( true );
        }
        d->layoutCounter++;

        if (d->accessKeysEnabled && d->accessKeysActivated) {
            emit hideAccessKeys();
            displayAccessKeys();
        }
    }
    else
       _width = visibleWidth();

    if (d->layoutTimerId)
        killTimer(d->layoutTimerId);
    d->layoutTimerId = 0;
    d->layoutSchedulingEnabled=true;
}

void KHTMLView::closeChildDialogs()
{
    QList<QDialog *> dlgs = findChildren<QDialog *>();
    foreach (QDialog *dlg, dlgs)
    {
        if ( dlg->testAttribute( Qt::WA_ShowModal ) ) {
            // qDebug() << "closeChildDialogs: closing dialog " << dlg;
            // close() ends up calling QButton::animateClick, which isn't immediate
            // we need something the exits the event loop immediately (#49068)
            dlg->reject();
        }
    }
    d->m_dialogsAllowed = false;
}

bool KHTMLView::dialogsAllowed() {
    bool allowed = d->m_dialogsAllowed;
    KHTMLPart* p = m_part->parentPart();
    if (p && p->view())
        allowed &= p->view()->dialogsAllowed();
    return allowed;
}

void KHTMLView::closeEvent( QCloseEvent* ev )
{
    closeChildDialogs();
    QScrollArea::closeEvent( ev );
}

void KHTMLView::setZoomLevel(int percent)
{
    percent = percent < 20 ? 20 : (percent > 800 ? 800 : percent);
    int oldpercent = d->zoomLevel;
    d->zoomLevel = percent;
    if (percent != oldpercent) {
        if (d->layoutSchedulingEnabled)
            layout();
        widget()->update();
    }
}

int KHTMLView::zoomLevel() const
{
    return d->zoomLevel;
}

void KHTMLView::setSmoothScrollingMode( SmoothScrollingMode m )
{
    d->smoothScrollMode = m;
    d->smoothScrollModeIsDefault = false;
    if (d->smoothScrolling && !m)
        d->stopScrolling();
}

void KHTMLView::setSmoothScrollingModeDefault( SmoothScrollingMode m )
{
    // check for manual override
    if (!d->smoothScrollModeIsDefault)
        return;
    d->smoothScrollMode = m;
    if (d->smoothScrolling && !m)
        d->stopScrolling();
}

KHTMLView::SmoothScrollingMode KHTMLView::smoothScrollingMode( ) const
{
    return d->smoothScrollMode;
}

//
// Event Handling
//
/////////////////

void KHTMLView::mousePressEvent( QMouseEvent *_mouse )
{
    if (!m_part->xmlDocImpl()) return;
    if (d->possibleTripleClick && ( _mouse->button() & Qt::MouseButtonMask ) == Qt::LeftButton)
    {
        mouseDoubleClickEvent( _mouse ); // it handles triple clicks too
        return;
    }

    int xm = _mouse->x();
    int ym = _mouse->y();
    revertTransforms(xm, ym);

    // qDebug() << "mousePressEvent: viewport=("<<_mouse->x()-contentsX()<<"/"<<_mouse->y()-contentsY()<<"), contents=(" << xm << "/" << ym << ")\n";

    d->isDoubleClick = false;

    DOM::NodeImpl::MouseEvent mev( _mouse->buttons(), DOM::NodeImpl::MousePress );
    m_part->xmlDocImpl()->prepareMouseEvent( false, xm, ym, &mev );

    //qDebug() << "innerNode="<<mev.innerNode.nodeName().string();

    if ( (_mouse->button() == Qt::MidButton) &&
          !m_part->d->m_bOpenMiddleClick && !d->m_mouseScrollTimer &&
          mev.url.isNull() && (mev.innerNode.elementId() != ID_INPUT) ) {
        QPoint point = mapFromGlobal( _mouse->globalPos() );

        d->m_mouseScroll_byX = 0;
        d->m_mouseScroll_byY = 0;

        d->m_mouseScrollTimer = new QTimer( this );
        connect( d->m_mouseScrollTimer, SIGNAL(timeout()), this, SLOT(slotMouseScrollTimer()) );

        if ( !d->m_mouseScrollIndicator ) {
            QPixmap pixmap( 48, 48 ), icon;
            pixmap.fill( QColor( qRgba( 127, 127, 127, 127 ) ) );

            QPainter p( &pixmap );
            QStyleOption option;

            option.rect.setRect( 16, 0, 16, 16 );
            QApplication::style()->drawPrimitive( QStyle::PE_IndicatorArrowUp, &option, &p );
            option.rect.setRect( 0, 16, 16, 16 );
            QApplication::style()->drawPrimitive( QStyle::PE_IndicatorArrowLeft, &option, &p );
            option.rect.setRect( 16, 32, 16, 16 );
            QApplication::style()->drawPrimitive( QStyle::PE_IndicatorArrowDown, &option, &p );
            option.rect.setRect( 32, 16, 16, 16 );
            QApplication::style()->drawPrimitive( QStyle::PE_IndicatorArrowRight, &option, &p );
            p.drawEllipse( 23, 23, 2, 2 );

            d->m_mouseScrollIndicator = new QWidget( this );
            d->m_mouseScrollIndicator->setFixedSize( 48, 48 );
            QPalette palette;
            palette.setBrush( d->m_mouseScrollIndicator->backgroundRole(), QBrush( pixmap ) );
            d->m_mouseScrollIndicator->setPalette( palette );
        }
        d->m_mouseScrollIndicator->move( point.x()-24, point.y()-24 );

        bool hasHorBar = visibleWidth() < contentsWidth();
        bool hasVerBar = visibleHeight() < contentsHeight();

        KConfigGroup cg( KSharedConfig::openConfig(), "HTML Settings" );
        if ( cg.readEntry( "ShowMouseScrollIndicator", true ) ) {
            d->m_mouseScrollIndicator->show();
            d->m_mouseScrollIndicator->unsetCursor();

            QBitmap mask = d->m_mouseScrollIndicator->palette().brush(d->m_mouseScrollIndicator->backgroundRole()).texture().createHeuristicMask( true );

	    if ( hasHorBar && !hasVerBar ) {
                QBitmap bm( 16, 16 );
                bm.clear();
                QPainter painter( &mask );
                painter.drawPixmap( QRectF( 16, 0, bm.width(), bm.height() ), bm, bm.rect() );
                painter.drawPixmap( QRectF( 16, 32, bm.width(), bm.height() ), bm, bm.rect() );
                d->m_mouseScrollIndicator->setCursor( Qt::SizeHorCursor );
            }
            else if ( !hasHorBar && hasVerBar ) {
                QBitmap bm( 16, 16 );
                bm.clear();
                QPainter painter( &mask );
                painter.drawPixmap( QRectF( 0, 16, bm.width(), bm.height() ), bm, bm.rect() );
                painter.drawPixmap( QRectF( 32, 16, bm.width(), bm.height() ), bm, bm.rect() );
                d->m_mouseScrollIndicator->setCursor( Qt::SizeVerCursor );
            }
            else
                d->m_mouseScrollIndicator->setCursor( Qt::SizeAllCursor );

            d->m_mouseScrollIndicator->setMask( mask );
        }
        else {
            if ( hasHorBar && !hasVerBar )
                viewport()->setCursor( Qt::SizeHorCursor );
            else if ( !hasHorBar && hasVerBar )
                viewport()->setCursor( Qt::SizeVerCursor );
            else
                viewport()->setCursor( Qt::SizeAllCursor );
        }

        return;
    }
    else if ( d->m_mouseScrollTimer ) {
        delete d->m_mouseScrollTimer;
        d->m_mouseScrollTimer = 0;

        if ( d->m_mouseScrollIndicator )
            d->m_mouseScrollIndicator->hide();
    }

    if (d->clickCount > 0 &&
        QPoint(d->clickX-xm,d->clickY-ym).manhattanLength() <= QApplication::startDragDistance())
        d->clickCount++;
    else {
        d->clickCount = 1;
        d->clickX = xm;
        d->clickY = ym;
    }

    bool swallowEvent = dispatchMouseEvent(EventImpl::MOUSEDOWN_EVENT,mev.innerNode.handle(),mev.innerNonSharedNode.handle(),true,
                                           d->clickCount,_mouse,true,DOM::NodeImpl::MousePress);

    if (!swallowEvent) {
	emit m_part->nodeActivated(mev.innerNode);

	khtml::MousePressEvent event( _mouse, xm, ym, mev.url, mev.target, mev.innerNode );
        QApplication::sendEvent( m_part, &event );
        // we might be deleted after this
    }
}

void KHTMLView::mouseDoubleClickEvent( QMouseEvent *_mouse )
{
    if(!m_part->xmlDocImpl()) return;

    int xm = _mouse->x();
    int ym = _mouse->y();
    revertTransforms(xm, ym);

    // qDebug() << "mouseDblClickEvent: x=" << xm << ", y=" << ym;

    d->isDoubleClick = true;

    DOM::NodeImpl::MouseEvent mev( _mouse->buttons(), DOM::NodeImpl::MouseDblClick );
    m_part->xmlDocImpl()->prepareMouseEvent( false, xm, ym, &mev );

    // We do the same thing as mousePressEvent() here, since the DOM does not treat
    // single and double-click events as separate (only the detail, i.e. number of clicks differs)
    if (d->clickCount > 0 &&
        QPoint(d->clickX-xm,d->clickY-ym).manhattanLength() <= QApplication::startDragDistance())
	d->clickCount++;
    else { // shouldn't happen, if Qt has the same criterias for double clicks.
	d->clickCount = 1;
	d->clickX = xm;
	d->clickY = ym;
    }
    bool swallowEvent = dispatchMouseEvent(EventImpl::MOUSEDOWN_EVENT,mev.innerNode.handle(),mev.innerNonSharedNode.handle(),true,
                                           d->clickCount,_mouse,true,DOM::NodeImpl::MouseDblClick);

    if (!swallowEvent) {
	khtml::MouseDoubleClickEvent event( _mouse, xm, ym, mev.url, mev.target, mev.innerNode, d->clickCount );
	QApplication::sendEvent( m_part, &event );
    }

    d->possibleTripleClick=true;
    QTimer::singleShot(QApplication::doubleClickInterval(),this,SLOT(tripleClickTimeout()));
}

void KHTMLView::tripleClickTimeout()
{
    d->possibleTripleClick = false;
    d->clickCount = 0;
}

static bool targetOpensNewWindow(KHTMLPart *part, QString target)
{
    if (!target.isEmpty() && (target.toLower() != "_top") &&
       (target.toLower() != "_self") && (target.toLower() != "_parent")) {
        if (target.toLower() == "_blank")
            return true;
        else {
            while (part->parentPart())
                part = part->parentPart();
            if (!part->frameExists(target))
                return true;
        }
    }
    return false;
}

void KHTMLView::mouseMoveEvent( QMouseEvent * _mouse )
{
    if ( d->m_mouseScrollTimer ) {
        QPoint point = mapFromGlobal( _mouse->globalPos() );

        int deltaX = point.x() - d->m_mouseScrollIndicator->x() - 24;
        int deltaY = point.y() - d->m_mouseScrollIndicator->y() - 24;

        (deltaX > 0) ? d->m_mouseScroll_byX = 1 : d->m_mouseScroll_byX = -1;
        (deltaY > 0) ? d->m_mouseScroll_byY = 1 : d->m_mouseScroll_byY = -1;

        double adX = qAbs(deltaX)/30.0;
        double adY = qAbs(deltaY)/30.0;

        d->m_mouseScroll_byX = qMax(qMin(d->m_mouseScroll_byX * int(adX*adX), SHRT_MAX), SHRT_MIN);
        d->m_mouseScroll_byY = qMax(qMin(d->m_mouseScroll_byY * int(adY*adY), SHRT_MAX), SHRT_MIN);

        if (d->m_mouseScroll_byX == 0 && d->m_mouseScroll_byY == 0) {
            d->m_mouseScrollTimer->stop();
        }
        else if (!d->m_mouseScrollTimer->isActive()) {
            d->m_mouseScrollTimer->start( 20 );
        }
    }

    if(!m_part->xmlDocImpl()) return;

    int xm = _mouse->x();
    int ym = _mouse->y();
    revertTransforms(xm, ym);

    DOM::NodeImpl::MouseEvent mev( _mouse->buttons(), DOM::NodeImpl::MouseMove );
    // Do not modify :hover/:active state while mouse is pressed.
    m_part->xmlDocImpl()->prepareMouseEvent( _mouse->buttons() /*readonly ?*/, xm, ym, &mev );

    // qDebug() << "mouse move: " << _mouse->pos()
    //		  << " button " << _mouse->button()
    // 		  << " state " << _mouse->state() << endl;

    DOM::NodeImpl* target = mev.innerNode.handle();
    DOM::NodeImpl* fn = m_part->xmlDocImpl()->focusNode();

    // a widget may be the real target of this event (e.g. if a scrollbar's slider is being moved)
    if (d->m_mouseEventsTarget && fn && fn->renderer() && fn->renderer()->isWidget())
       target = fn;

    bool swallowEvent = dispatchMouseEvent(EventImpl::MOUSEMOVE_EVENT,target,mev.innerNonSharedNode.handle(),false,
                                           0,_mouse,true,DOM::NodeImpl::MouseMove);

    if (d->clickCount > 0 &&
        QPoint(d->clickX-xm,d->clickY-ym).manhattanLength() > QApplication::startDragDistance()) {
	d->clickCount = 0;  // moving the mouse outside the threshold invalidates the click
    }

    khtml::RenderObject* r = target ? target->renderer() : 0;
    bool setCursor = true;
    bool forceDefault = false;
    if (r && r->isWidget()) {
        RenderWidget* rw = static_cast<RenderWidget*>(r);
        KHTMLWidget* kw = qobject_cast<KHTMLView*>(rw->widget())? dynamic_cast<KHTMLWidget*>(rw->widget()) : 0;
        if (kw && kw->m_kwp->isRedirected())
            setCursor = false;
        else if (QLineEdit* le = qobject_cast<QLineEdit*>(rw->widget())) {
            QList<QWidget*> wl = le->findChildren<QWidget *>("KLineEditButton");
            // force arrow cursor above lineedit clear button
            foreach (QWidget*w, wl) {
                if (w->underMouse()) {
                    forceDefault = true;
                    break;
                }
            }
        }
        else if (QTextEdit* te = qobject_cast<QTextEdit*>(rw->widget())) {
            if (te->verticalScrollBar()->underMouse() || te->horizontalScrollBar()->underMouse())
                forceDefault = true;
        }
    }
    khtml::RenderStyle* style = (r && r->style()) ? r->style() : 0;
    QCursor c;
    LinkCursor linkCursor = LINK_NORMAL;
    switch (!forceDefault ? (style ? style->cursor() : CURSOR_AUTO) : CURSOR_DEFAULT) {
    case CURSOR_AUTO:
        if ( r && r->isText() && ((m_part->d->m_bMousePressed && m_part->d->editor_context.m_beganSelectingText) ||
                                   !r->isPointInsideSelection(xm, ym, m_part->caret())) )
            c = QCursor(Qt::IBeamCursor);
        if ( mev.url.length() && m_part->settings()->changeCursor() ) {
            c = m_part->urlCursor();
	    if (mev.url.string().startsWith("mailto:") && mev.url.string().indexOf('@')>0)
              linkCursor = LINK_MAILTO;
            else
              if ( targetOpensNewWindow( m_part, mev.target.string() ) )
	        linkCursor = LINK_NEWWINDOW;
        }

        if (r && r->isFrameSet() && !static_cast<RenderFrameSet*>(r)->noResize())
            c = QCursor(static_cast<RenderFrameSet*>(r)->cursorShape());

        break;
    case CURSOR_CROSS:
        c = QCursor(Qt::CrossCursor);
        break;
    case CURSOR_POINTER:
        c = m_part->urlCursor();
	if (mev.url.string().startsWith("mailto:") && mev.url.string().indexOf('@')>0)
          linkCursor = LINK_MAILTO;
        else
          if ( targetOpensNewWindow( m_part, mev.target.string() ) )
	    linkCursor = LINK_NEWWINDOW;
        break;
    case CURSOR_PROGRESS:
        c = QCursor(Qt::BusyCursor); // working_cursor
        break;
    case CURSOR_MOVE:
    case CURSOR_ALL_SCROLL:
        c = QCursor(Qt::SizeAllCursor);
        break;
    case CURSOR_E_RESIZE:
    case CURSOR_W_RESIZE:
    case CURSOR_EW_RESIZE:
        c = QCursor(Qt::SizeHorCursor);
        break;
    case CURSOR_N_RESIZE:
    case CURSOR_S_RESIZE:
    case CURSOR_NS_RESIZE:
        c = QCursor(Qt::SizeVerCursor);
        break;
    case CURSOR_NE_RESIZE:
    case CURSOR_SW_RESIZE:
    case CURSOR_NESW_RESIZE:
        c = QCursor(Qt::SizeBDiagCursor);
        break;
    case CURSOR_NW_RESIZE:
    case CURSOR_SE_RESIZE:
    case CURSOR_NWSE_RESIZE:
        c = QCursor(Qt::SizeFDiagCursor);
        break;
    case CURSOR_TEXT:
        c = QCursor(Qt::IBeamCursor);
        break;
    case CURSOR_WAIT:
        c = QCursor(Qt::WaitCursor);
        break;
    case CURSOR_HELP:
        c = QCursor(Qt::WhatsThisCursor);
        break;
    case CURSOR_DEFAULT:
        break;
    case CURSOR_NONE:
    case CURSOR_NOT_ALLOWED:
        c = QCursor(Qt::ForbiddenCursor);
        break;
    case CURSOR_ROW_RESIZE:
        c = QCursor(Qt::SplitVCursor);
        break;
    case CURSOR_COL_RESIZE:
        c = QCursor(Qt::SplitHCursor);
        break;
    case CURSOR_VERTICAL_TEXT:
    case CURSOR_CONTEXT_MENU:
    case CURSOR_NO_DROP:
    case CURSOR_CELL:
    case CURSOR_COPY:
    case CURSOR_ALIAS:
        c = QCursor(Qt::ArrowCursor);
        break;
    }

    if (!setCursor && style && style->cursor() != CURSOR_AUTO)
        setCursor = true;

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QWidget* vp = viewport();
    for (KHTMLPart* p = m_part; p; p = p->parentPart())
        if (!p->parentPart())
            vp = p->view()->viewport();
    if ( setCursor && vp->cursor().handle() != c.handle() ) {
        if( c.shape() == Qt::ArrowCursor) {
            for (KHTMLPart* p = m_part; p; p = p->parentPart())
                p->view()->viewport()->unsetCursor();
        }
        else {
            vp->setCursor( c );
        }
    }
#endif

    if ( linkCursor!=LINK_NORMAL && isVisible() && hasFocus() ) {
#if HAVE_X11

        if( !d->cursorIconWidget ) {
#if HAVE_X11
            d->cursorIconWidget = new QLabel( 0, Qt::X11BypassWindowManagerHint );
            XSetWindowAttributes attr;
            attr.save_under = True;
            XChangeWindowAttributes( QX11Info::display(), d->cursorIconWidget->winId(), CWSaveUnder, &attr );
#else
            d->cursorIconWidget = new QLabel( NULL, NULL );
            //TODO
#endif
        }

        // Update the pixmap if need be.
        if (linkCursor != d->cursorIconType) {
            d->cursorIconType = linkCursor;
            QString cursorIcon;
            switch (linkCursor)
            {
              case LINK_MAILTO:     cursorIcon = "mail-message-new"; break;
              case LINK_NEWWINDOW:  cursorIcon = "window-new";       break;
              default:              cursorIcon = "dialog-error";     break;
            }

            QPixmap icon_pixmap = KHTMLGlobal::iconLoader()->loadIcon( cursorIcon, KIconLoader::Small, 0, KIconLoader::DefaultState, QStringList(), 0, true );

            d->cursorIconWidget->resize( icon_pixmap.width(), icon_pixmap.height());
            d->cursorIconWidget->setMask( icon_pixmap.createMaskFromColor(Qt::transparent));
            d->cursorIconWidget->setPixmap( icon_pixmap);
            d->cursorIconWidget->update();
        }

        QPoint c_pos = QCursor::pos();
        d->cursorIconWidget->move( c_pos.x() + 15, c_pos.y() + 15 );
#if HAVE_X11
        XRaiseWindow( QX11Info::display(), d->cursorIconWidget->winId());
        QApplication::flush();
#elif defined(Q_OS_WIN)
        SetWindowPos( d->cursorIconWidget->winId(), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
#else
        //TODO?
#endif
        d->cursorIconWidget->show();
#endif
    }
    else if ( d->cursorIconWidget )
        d->cursorIconWidget->hide();

    if (r && r->isWidget()) {
	_mouse->ignore();
    }

    if (!swallowEvent) {
        khtml::MouseMoveEvent event( _mouse, xm, ym, mev.url, mev.target, mev.innerNode );
        QApplication::sendEvent( m_part, &event );
    }
}

void KHTMLView::mouseReleaseEvent( QMouseEvent * _mouse )
{
    bool swallowEvent = false;

    int xm = _mouse->x();
    int ym = _mouse->y();
    revertTransforms(xm, ym);

    DOM::NodeImpl::MouseEvent mev( _mouse->buttons(), DOM::NodeImpl::MouseRelease );

    if ( m_part->xmlDocImpl() )
    {
        m_part->xmlDocImpl()->prepareMouseEvent( false, xm, ym, &mev );

        DOM::NodeImpl* target = mev.innerNode.handle();
        DOM::NodeImpl* fn = m_part->xmlDocImpl()->focusNode();

        // a widget may be the real target of this event (e.g. if a scrollbar's slider is being moved)
        if (d->m_mouseEventsTarget && fn && fn->renderer() && fn->renderer()->isWidget())
            target = fn;

        swallowEvent = dispatchMouseEvent(EventImpl::MOUSEUP_EVENT,target,mev.innerNonSharedNode.handle(),true,
                                          d->clickCount,_mouse,false,DOM::NodeImpl::MouseRelease);

        // clear our sticky event target on any mouseRelease event
        if (d->m_mouseEventsTarget)
            d->m_mouseEventsTarget = 0;

        if (d->clickCount > 0 &&
            QPoint(d->clickX-xm,d->clickY-ym).manhattanLength() <= QApplication::startDragDistance()) {
            QMouseEvent me(d->isDoubleClick ? QEvent::MouseButtonDblClick : QEvent::MouseButtonRelease,
                           _mouse->pos(), _mouse->button(), _mouse->buttons(), _mouse->modifiers());
            dispatchMouseEvent(EventImpl::CLICK_EVENT, mev.innerNode.handle(),mev.innerNonSharedNode.handle(),true,
                               d->clickCount, &me, true, DOM::NodeImpl::MouseRelease);
        }

        khtml::RenderObject* r = target ? target->renderer() : 0;
        if (r && r->isWidget())
            _mouse->ignore();
    }

    if (!swallowEvent) {
	khtml::MouseReleaseEvent event( _mouse, xm, ym, mev.url, mev.target, mev.innerNode );
	QApplication::sendEvent( m_part, &event );
    }
}

// returns true if event should be swallowed
bool KHTMLView::dispatchKeyEvent( QKeyEvent *_ke )
{
    if (!m_part->xmlDocImpl())
        return false;
    // Pressing and releasing a key should generate keydown, keypress and keyup events
    // Holding it down should generated keydown, keypress (repeatedly) and keyup events
    // The problem here is that Qt generates two autorepeat events (keyrelease+keypress)
    // for autorepeating, while DOM wants only one autorepeat event (keypress), so one
    // of the Qt events shouldn't be passed to DOM, but it should be still filtered
    // out if DOM would filter the autorepeat event. Additional problem is that Qt keyrelease
    // events don't have text() set (Qt bug?), so DOM often would ignore the keypress event
    // if it was created using Qt keyrelease, but Qt autorepeat keyrelease comes
    // before Qt autorepeat keypress (i.e. problem whether to filter it out or not).
    // The solution is to filter out and postpone the Qt autorepeat keyrelease until
    // the following Qt keypress event comes. If DOM accepts the DOM keypress event,
    // the postponed event will be simply discarded. If not, it will be passed to keyPressEvent()
    // again, and here it will be ignored.
    //
    //  Qt:      Press      | Release(autorepeat) Press(autorepeat) etc. |   Release
    //  DOM:   Down + Press |      (nothing)           Press             |     Up

    // It's also possible to get only Releases. E.g. the release of alt-tab,
    // or when the keypresses get captured by an accel.

    if( _ke == d->postponed_autorepeat ) // replayed event
    {
        return false;
    }

    if( _ke->type() == QEvent::KeyPress )
    {
        if( !_ke->isAutoRepeat())
        {
            bool ret = dispatchKeyEventHelper( _ke, false ); // keydown
            // don't send keypress even if keydown was blocked, like IE (and unlike Mozilla)
            if( !ret && dispatchKeyEventHelper( _ke, true )) // keypress
                ret = true;
            return ret;
        }
        else // autorepeat
        {
            bool ret = dispatchKeyEventHelper( _ke, true ); // keypress
            if( !ret && d->postponed_autorepeat )
                keyPressEvent( d->postponed_autorepeat );
            delete d->postponed_autorepeat;
            d->postponed_autorepeat = NULL;
            return ret;
        }
    }
    else // QEvent::KeyRelease
    {
        // Discard postponed "autorepeat key-release" events that didn't see
        // a keypress after them (e.g. due to QAccel)
        delete d->postponed_autorepeat;
        d->postponed_autorepeat = 0;

        if( !_ke->isAutoRepeat()) {
            return dispatchKeyEventHelper( _ke, false ); // keyup
        }
        else
        {
            d->postponed_autorepeat = new QKeyEvent( _ke->type(), _ke->key(), _ke->modifiers(),
                _ke->text(), _ke->isAutoRepeat(), _ke->count());
            if( _ke->isAccepted())
                d->postponed_autorepeat->accept();
            else
                d->postponed_autorepeat->ignore();
            return true;
        }
    }
}

// returns true if event should be swallowed
bool KHTMLView::dispatchKeyEventHelper( QKeyEvent *_ke, bool keypress )
{
    DOM::NodeImpl* keyNode = m_part->xmlDocImpl()->focusNode();
    if (keyNode) {
        return keyNode->dispatchKeyEvent(_ke, keypress);
    } else { // no focused node, send to document
        return m_part->xmlDocImpl()->dispatchKeyEvent(_ke, keypress);
    }
}

void KHTMLView::keyPressEvent( QKeyEvent *_ke )
{
    // If CTRL was hit, be prepared for access keys
    if (d->accessKeysEnabled && _ke->key() == Qt::Key_Control && !(_ke->modifiers() & ~Qt::ControlModifier) && !d->accessKeysActivated)
    {
        d->accessKeysPreActivate=true;
        _ke->accept();
        return;
    }

    if (_ke->key() == Qt::Key_Shift && !(_ke->modifiers() & ~Qt::ShiftModifier))
	    d->scrollSuspendPreActivate=true;

    // accesskey handling needs to be done before dispatching, otherwise e.g. lineedits
    // may eat the event

    if (d->accessKeysEnabled && d->accessKeysActivated)
    {
        int state = ( _ke->modifiers() & ( Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier ));
        if ( state==0 || state==Qt::ShiftModifier ) {
	    if (_ke->key() != Qt::Key_Shift)
	        accessKeysTimeout();
            handleAccessKey( _ke );
            _ke->accept();
            return;
    	}
	accessKeysTimeout();
	_ke->accept();
	return;
    }

    if ( dispatchKeyEvent( _ke )) {
        // If either keydown or keypress was accepted by a widget, or canceled by JS, stop here.
        _ke->accept();
        return;
    }

    int offs = (viewport()->height() < 30) ? viewport()->height() : 30; // ### ??
    if (_ke->modifiers() & Qt::ShiftModifier)
      switch(_ke->key())
        {
        case Qt::Key_Space:
            verticalScrollBar()->setValue( verticalScrollBar()->value() -viewport()->height() + offs );
            if(d->scrollSuspended)
                d->newScrollTimer(this, 0);
            break;

        case Qt::Key_Down:
        case Qt::Key_J:
            d->adjustScroller(this, KHTMLViewPrivate::ScrollDown, KHTMLViewPrivate::ScrollUp);
            break;

        case Qt::Key_Up:
        case Qt::Key_K:
            d->adjustScroller(this, KHTMLViewPrivate::ScrollUp, KHTMLViewPrivate::ScrollDown);
            break;

        case Qt::Key_Left:
        case Qt::Key_H:
            d->adjustScroller(this, KHTMLViewPrivate::ScrollLeft, KHTMLViewPrivate::ScrollRight);
            break;

        case Qt::Key_Right:
        case Qt::Key_L:
            d->adjustScroller(this, KHTMLViewPrivate::ScrollRight, KHTMLViewPrivate::ScrollLeft);
            break;
        }
    else
        switch ( _ke->key() )
        {
        case Qt::Key_Down:
        case Qt::Key_J:
            if (!d->scrollTimerId || d->scrollSuspended)
                verticalScrollBar()->setValue( verticalScrollBar()->value()+10 );
            if (d->scrollTimerId)
                d->newScrollTimer(this, 0);
            break;

        case Qt::Key_Space:
        case Qt::Key_PageDown:
	    d->shouldSmoothScroll = true;
            verticalScrollBar()->setValue( verticalScrollBar()->value() +viewport()->height() - offs );
            if(d->scrollSuspended)
                d->newScrollTimer(this, 0);
            break;

        case Qt::Key_Up:
        case Qt::Key_K:
            if (!d->scrollTimerId || d->scrollSuspended)
                verticalScrollBar()->setValue( verticalScrollBar()->value()-10 );
            if (d->scrollTimerId)
                d->newScrollTimer(this, 0);
            break;

        case Qt::Key_PageUp:
	    d->shouldSmoothScroll = true;
            verticalScrollBar()->setValue( verticalScrollBar()->value() -viewport()->height() + offs );
            if(d->scrollSuspended)
                d->newScrollTimer(this, 0);
            break;
        case Qt::Key_Right:
        case Qt::Key_L:
            if (!d->scrollTimerId || d->scrollSuspended)
                 horizontalScrollBar()->setValue( horizontalScrollBar()->value()+10 );
            if (d->scrollTimerId)
                d->newScrollTimer(this, 0);
            break;

        case Qt::Key_Left:
        case Qt::Key_H:
            if (!d->scrollTimerId || d->scrollSuspended)
                horizontalScrollBar()->setValue( horizontalScrollBar()->value()-10 );
            if (d->scrollTimerId)
                d->newScrollTimer(this, 0);
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
	    // ### FIXME:
	    // or even better to HTMLAnchorElementImpl::event()
            if (m_part->xmlDocImpl()) {
		NodeImpl *n = m_part->xmlDocImpl()->focusNode();
		if (n)
		    n->setActive();
	    }
            break;
        case Qt::Key_Home:
            verticalScrollBar()->setValue( 0 );
            horizontalScrollBar()->setValue( 0 );
            if(d->scrollSuspended)
                d->newScrollTimer(this, 0);
            break;
        case Qt::Key_End:
            verticalScrollBar()->setValue( contentsHeight() - visibleHeight() );
            if(d->scrollSuspended)
                d->newScrollTimer(this, 0);
            break;
        case Qt::Key_Shift:
            // what are you doing here?
	    _ke->ignore();
            return;
        default:
            if (d->scrollTimerId)
                d->newScrollTimer(this, 0);
	    _ke->ignore();
            return;
        }

    _ke->accept();
}

void KHTMLView::keyReleaseEvent(QKeyEvent *_ke)
{
    if( d->scrollSuspendPreActivate && _ke->key() != Qt::Key_Shift )
        d->scrollSuspendPreActivate = false;
    if( _ke->key() == Qt::Key_Shift && d->scrollSuspendPreActivate && !(_ke->modifiers() & Qt::ShiftModifier))
        if (d->scrollTimerId) {
                d->scrollSuspended = !d->scrollSuspended;
                if (d->scrollSuspended)
                    d->stopScrolling();
        }

    if (d->accessKeysEnabled)
    {
        if (d->accessKeysPreActivate && _ke->key() != Qt::Key_Control)
            d->accessKeysPreActivate=false;
        if (d->accessKeysPreActivate && !(_ke->modifiers() & Qt::ControlModifier))
        {
	    displayAccessKeys();
	    m_part->setStatusBarText(i18n("Access Keys activated"),KHTMLPart::BarOverrideText);
	    d->accessKeysActivated = true;
	    d->accessKeysPreActivate = false;
            _ke->accept();
            return;
        }
	else if (d->accessKeysActivated)
        {
            accessKeysTimeout();
            _ke->accept();
            return;
        }
    }

    // Send keyup event
    if ( dispatchKeyEvent( _ke ) )
    {
        _ke->accept();
        return;
    }

    QScrollArea::keyReleaseEvent(_ke);
}

bool KHTMLView::focusNextPrevChild( bool next )
{
    // Now try to find the next child
    if (m_part->xmlDocImpl() && focusNextPrevNode(next))
    {
	//if (m_part->xmlDocImpl()->focusNode())
	    // qDebug() << "focusNode.name: "
		//      << m_part->xmlDocImpl()->focusNode()->nodeName().string() << endl;
	return true; // focus node found
    }

    // If we get here, pass tabbing control up to the next/previous child in our parent
    d->pseudoFocusNode = KHTMLViewPrivate::PFNone;
    if (m_part->parentPart() && m_part->parentPart()->view())
        return m_part->parentPart()->view()->focusNextPrevChild(next);

    return QWidget::focusNextPrevChild(next);
}

void KHTMLView::doAutoScroll()
{
    QPoint pos = QCursor::pos();
    QPoint off;
    KHTMLView* v = m_kwp->isRedirected() ? m_kwp->rootViewPos(off) : this;
    pos = v->viewport()->mapFromGlobal( pos );
    pos -= off;
    int xm, ym;
    viewportToContents(pos.x(), pos.y(), xm, ym); // ###

    pos = QPoint(pos.x() - viewport()->x(), pos.y() - viewport()->y());
    if ( (pos.y() < 0) || (pos.y() > visibleHeight()) ||
         (pos.x() < 0) || (pos.x() > visibleWidth()) )
    {
        ensureVisible( xm, ym, 0, 5 );

#ifndef KHTML_NO_SELECTION
        // extend the selection while scrolling
	DOM::Node innerNode;
	if (m_part->isExtendingSelection()) {
            RenderObject::NodeInfo renderInfo(true/*readonly*/, false/*active*/);
            m_part->xmlDocImpl()->renderer()->layer()
				->nodeAtPoint(renderInfo, xm, ym);
            innerNode = renderInfo.innerNode();
	}/*end if*/

        if (innerNode.handle() && innerNode.handle()->renderer()
             && innerNode.handle()->renderer()->shouldSelect()) {
            m_part->extendSelectionTo(xm, ym, innerNode);
        }/*end if*/
#endif // KHTML_NO_SELECTION
    }
}

// KHTML defines its own stacking order for any object and thus takes
// control of widget painting whenever it can. This is called "redirection".
//
// Redirected widgets are placed off screen. When they are declared as a child of our view (ChildPolished event),
// an event filter is installed, so as to catch any paint event and translate them as update() of the view's main widget.
//
// Painting also happens spontaneously within widgets. In this case, the widget would update() parts of itself.
// While this ordinarily results in a paintEvent being schedduled, it is not the case with off screen widgets.
// Thus update() is monitored by using the mechanism that deffers any update call happening during a paint event,
// transforming it into a posted UpdateLater event. Hence the need to set Qt::WA_WState_InPaintEvent on redirected widgets.
//
// Once the UpdateLater event has been received, Qt::WA_WState_InPaintEvent is removed and the process continues
// with the update of the corresponding rect on the view. That in turn will make our painting subsystem render()
// the widget at the correct stacking position.
//
// For non-redirected (e.g. external) widgets, z-order is honoured through masking. cf.RenderLayer::updateWidgetMasks

static void handleWidget(QWidget* w, KHTMLView* view, bool recurse=true)
{
    if (w->isWindow())
        return;

    if (!qobject_cast<QFrame*>(w))
	w->setAttribute( Qt::WA_NoSystemBackground );

    w->setAttribute(Qt::WA_WState_InPaintEvent);

    if (!(w->objectName() == "KLineEditButton"))
        w->setAttribute(Qt::WA_OpaquePaintEvent);

    w->installEventFilter(view);

    if (!recurse)
        return;
    if (qobject_cast<KHTMLView*>(w)) {
        handleWidget(static_cast<KHTMLView*>(w)->widget(), view, false);
        handleWidget(static_cast<KHTMLView*>(w)->horizontalScrollBar(), view, false);
        handleWidget(static_cast<KHTMLView*>(w)->verticalScrollBar(), view, false);
        return;
    }

    QObjectList children = w->children();
    foreach (QObject* object, children) {
	QWidget *widget = qobject_cast<QWidget*>(object);
	if (widget)
	    handleWidget(widget, view);
    }
}

class KHTMLBackingStoreHackWidget : public QWidget
{
public:
    void publicEvent(QEvent *e)
    {
        QWidget::event(e);
    }
};

bool  KHTMLView::viewportEvent ( QEvent * e )
{
    switch (e->type()) {
      // those must not be dispatched to the specialized handlers
      // as widgetEvent() already took care of that
      case QEvent::MouseButtonPress:
      case QEvent::MouseButtonRelease:
      case QEvent::MouseButtonDblClick:
      case QEvent::MouseMove:
#ifndef QT_NO_WHEELEVENT
      case QEvent::Wheel:
#endif
      case QEvent::ContextMenu:
      case QEvent::DragEnter:
      case QEvent::DragMove:
      case QEvent::DragLeave:
      case QEvent::Drop:
        return false;
      default:
        break;
    }
    return QScrollArea::viewportEvent(e);
}

static void setInPaintEventFlag(QWidget* w, bool b = true, bool recurse=true)
{
      w->setAttribute(Qt::WA_WState_InPaintEvent, b);

      if (!recurse)
          return;
      if (qobject_cast<KHTMLView*>(w)) {
          setInPaintEventFlag(static_cast<KHTMLView*>(w)->widget(), b, false);
          setInPaintEventFlag(static_cast<KHTMLView*>(w)->horizontalScrollBar(), b, false);
          setInPaintEventFlag(static_cast<KHTMLView*>(w)->verticalScrollBar(), b, false);
          return;
      }

      foreach(QObject* cw, w->children()) {
          if (cw->isWidgetType() && ! static_cast<QWidget*>(cw)->isWindow()
                                 && !(static_cast<QWidget*>(cw)->windowModality() & Qt::ApplicationModal)) {
              setInPaintEventFlag(static_cast<QWidget*>(cw), b);
          }
      }
}

bool KHTMLView::eventFilter(QObject *o, QEvent *e)
{
    if ( e->type() == QEvent::ShortcutOverride ) {
	QKeyEvent* ke = (QKeyEvent*) e;
	if (m_part->isEditable() || m_part->isCaretMode()
	    || (m_part->xmlDocImpl() && m_part->xmlDocImpl()->focusNode()
		&& m_part->xmlDocImpl()->focusNode()->isContentEditable())) {
	    if ( (ke->modifiers() & Qt::ControlModifier) || (ke->modifiers() & Qt::ShiftModifier) ) {
		switch ( ke->key() ) {
		case Qt::Key_Left:
		case Qt::Key_Right:
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_Home:
		case Qt::Key_End:
		    ke->accept();
		    return true;
		default:
		    break;
		}
	    }
	}
    }

    if ( e->type() == QEvent::Leave ) {
      if ( d->cursorIconWidget )
        d->cursorIconWidget->hide();
      m_part->resetHoverText();
    }

    QWidget *view = widget();
    if (o == view) {
        if (widgetEvent(e))
            return true;
        else if (e->type() == QEvent::Resize) {
            updateScrollBars();
            return false;
        }
    } else if (o->isWidgetType()) {
	QWidget *v = static_cast<QWidget *>(o);
        QWidget *c = v;
	while (v && v != view) {
            c = v;
	    v = v->parentWidget();
	}
	KHTMLWidget* k = dynamic_cast<KHTMLWidget*>(c);
	if (v && k && k->m_kwp->isRedirected()) {
	    bool block = false;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	    bool isUpdate = false;
#endif
	    QWidget *w = static_cast<QWidget *>(o);
	    switch(e->type()) {
	    case QEvent::UpdateRequest: {
                // implicitly call qt_syncBackingStore(w)
                static_cast<KHTMLBackingStoreHackWidget *>(w)->publicEvent(e);
                block = true;
                break;
            }
            case QEvent::UpdateLater:
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                isUpdate = true;
#endif
                // no break;
	    case QEvent::Paint:
		if (!allowWidgetPaintEvents) {
		    // eat the event. Like this we can control exactly when the widget
		    // gets repainted.
		    block = true;
		    int x = 0, y = 0;
                    QWidget *v = w;
                    while (v && v->parentWidget() != view) {
                        x += v->x();
                        y += v->y();
                        v = v->parentWidget();
                    }

                    QPoint ap = k->m_kwp->absolutePos();
		    x += ap.x();
		    y += ap.y();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
		    QRect pr = isUpdate ? static_cast<QUpdateLaterEvent*>(e)->region().boundingRect() : static_cast<QPaintEvent*>(e)->rect();
                    bool asap = !d->contentsMoving && qobject_cast<QAbstractScrollArea*>(c);

                    if (isUpdate) {
                        setInPaintEventFlag(w, false);
                        if (asap)
                            w->repaint(static_cast<QUpdateLaterEvent*>(e)->region());
                        else
                            w->update(static_cast<QUpdateLaterEvent*>(e)->region());
                        setInPaintEventFlag(w);
                    }

		    // QScrollView needs fast repaints
		    if ( asap && !isUpdate && !d->painting && m_part->xmlDocImpl() && m_part->xmlDocImpl()->renderer() &&
		         !static_cast<khtml::RenderCanvas *>(m_part->xmlDocImpl()->renderer())->needsLayout() ) {
		        repaintContents(x + pr.x(), y + pr.y(),
	                                        pr.width(), pr.height()+1); // ### investigate that +1 (shows up when
	                                                                    // updating e.g a textarea's blinking cursor)
                    } else if (!d->painting) {
 		        scheduleRepaint(x + pr.x(), y + pr.y(),
 				    pr.width(), pr.height()+1, asap);
                    }
#endif
		}
		break;
	    case QEvent::MouseMove:
	    case QEvent::MouseButtonPress:
	    case QEvent::MouseButtonRelease:
	    case QEvent::MouseButtonDblClick: {

		if (0 && w->parentWidget() == view && !qobject_cast<QScrollBar*>(w) && !::qobject_cast<QScrollBar *>(w)) {
		    QMouseEvent *me = static_cast<QMouseEvent *>(e);
		    QPoint pt = w->mapTo( view, me->pos());
		    QMouseEvent me2(me->type(), pt, me->button(), me->buttons(), me->modifiers());

		    if (e->type() == QEvent::MouseMove)
			mouseMoveEvent(&me2);
		    else if(e->type() == QEvent::MouseButtonPress)
			mousePressEvent(&me2);
		    else if(e->type() == QEvent::MouseButtonRelease)
			mouseReleaseEvent(&me2);
		    else
			mouseDoubleClickEvent(&me2);
		    block = true;
                }
		break;
	    }
	    case QEvent::KeyPress:
	    case QEvent::KeyRelease:
		if (w->parentWidget() == view && !qobject_cast<QScrollBar*>(w)) {
		    QKeyEvent *ke = static_cast<QKeyEvent *>(e);
		    if (e->type() == QEvent::KeyPress) {
			keyPressEvent(ke);
			ke->accept();
		    } else{
			keyReleaseEvent(ke);
			ke->accept();
		    }
		    block = true;
		}

                if (qobject_cast<KUrlRequester*>(w->parentWidget()) &&
		    e->type() == QEvent::KeyPress) {
		    // Since keypress events on the upload widget will
		    // be forwarded to the lineedit anyway,
		    // block the original copy at this level to prevent
		    // double-emissions of events it doesn't accept
		    e->ignore();
		    block = true;
		}

		break;
            case QEvent::FocusIn:
            case QEvent::FocusOut: {
                QPoint dummy;
                KHTMLView* root = m_kwp->rootViewPos(dummy);
                if (!root)
                    root = this;
                block = static_cast<QFocusEvent*>(e)->reason() != Qt::MouseFocusReason ||  root->underMouse();
                break;
            }
	    default:
		break;
	    }
	    if (block) {
 		//qDebug("eating event");
		return true;
	    }
	}
    }

//    qDebug() <<"passing event on to sv event filter object=" << o->className() << " event=" << e->type();
    return QScrollArea::eventFilter(o, e);
}

bool KHTMLView::widgetEvent(QEvent* e)
{
    switch (e->type()) {
      case QEvent::MouseButtonPress:
      case QEvent::MouseButtonRelease:
      case QEvent::MouseButtonDblClick:
      case QEvent::MouseMove:
      case QEvent::Paint:
#ifndef QT_NO_WHEELEVENT
      case QEvent::Wheel:
#endif
      case QEvent::ContextMenu:
      case QEvent::DragEnter:
      case QEvent::DragMove:
      case QEvent::DragLeave:
      case QEvent::Drop:
        return QFrame::event(e);
      case QEvent::ChildPolished: {
        // we need to install an event filter on all children of the widget() to
        // be able to get correct stacking of children within the document.
        QObject *c = static_cast<QChildEvent *>(e)->child();
        if (c->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(c);
	    // don't install the event filter on toplevels
	    if (!(w->windowFlags() & Qt::Window) && !(w->windowModality() & Qt::ApplicationModal)) {
	        KHTMLWidget* k = dynamic_cast<KHTMLWidget*>(w);
	        if (k && k->m_kwp->isRedirected()) {
	            w->unsetCursor();
		    handleWidget(w, this);
                }
            }
        }
        break;
      }
      case QEvent::Move: {
          if (static_cast<QMoveEvent*>(e)->pos() != QPoint(0,0)) {
              widget()->move(0,0);
              updateScrollBars();
              return true;
          }
          break;
      }
      default:
        break;
    }
    return false;
}

bool KHTMLView::hasLayoutPending()
{
    return d->layoutTimerId && !d->firstLayoutPending;
}

DOM::NodeImpl *KHTMLView::nodeUnderMouse() const
{
    return d->underMouse;
}

DOM::NodeImpl *KHTMLView::nonSharedNodeUnderMouse() const
{
    return d->underMouseNonShared;
}

bool KHTMLView::scrollTo(const QRect &bounds)
{
    d->scrollingSelf = true; // so scroll events get ignored

    int x, y, xe, ye;
    x = bounds.left();
    y = bounds.top();
    xe = bounds.right();
    ye = bounds.bottom();

    //qDebug()<<"scrolling coords: x="<<x<<" y="<<y<<" width="<<xe-x<<" height="<<ye-y;

    int deltax;
    int deltay;

    int curHeight = visibleHeight();
    int curWidth = visibleWidth();

    if (ye-y>curHeight-d->borderY)
	ye  = y + curHeight - d->borderY;

    if (xe-x>curWidth-d->borderX)
	xe = x + curWidth - d->borderX;

    // is xpos of target left of the view's border?
    if (x < contentsX() + d->borderX )
            deltax = x - contentsX() - d->borderX;
    // is xpos of target right of the view's right border?
    else if (xe + d->borderX > contentsX() + curWidth)
            deltax = xe + d->borderX - ( contentsX() + curWidth );
    else
        deltax = 0;

    // is ypos of target above upper border?
    if (y < contentsY() + d->borderY)
            deltay = y - contentsY() - d->borderY;
    // is ypos of target below lower border?
    else if (ye + d->borderY > contentsY() + curHeight)
            deltay = ye + d->borderY - ( contentsY() + curHeight );
    else
        deltay = 0;

    int maxx = curWidth-d->borderX;
    int maxy = curHeight-d->borderY;

    int scrollX, scrollY;

    scrollX = deltax > 0 ? (deltax > maxx ? maxx : deltax) : deltax == 0 ? 0 : (deltax>-maxx ? deltax : -maxx);
    scrollY = deltay > 0 ? (deltay > maxy ? maxy : deltay) : deltay == 0 ? 0 : (deltay>-maxy ? deltay : -maxy);

    if (contentsX() + scrollX < 0)
	scrollX = -contentsX();
    else if (contentsWidth() - visibleWidth() - contentsX() < scrollX)
	scrollX = contentsWidth() - visibleWidth() - contentsX();

    if (contentsY() + scrollY < 0)
	scrollY = -contentsY();
    else if (contentsHeight() - visibleHeight() - contentsY() < scrollY)
	scrollY = contentsHeight() - visibleHeight() - contentsY();

    horizontalScrollBar()->setValue( horizontalScrollBar()->value()+scrollX );
    verticalScrollBar()->setValue( verticalScrollBar()->value()+scrollY );

    d->scrollingSelf = false;

    if ( (abs(deltax)<=maxx) && (abs(deltay)<=maxy) )
	return true;
    else return false;

}

bool KHTMLView::focusNextPrevNode(bool next)
{
    // Sets the focus node of the document to be the node after (or if
    // next is false, before) the current focus node.  Only nodes that
    // are selectable (i.e. for which isFocusable() returns true) are
    // taken into account, and the order used is that specified in the
    // HTML spec (see DocumentImpl::nextFocusNode() and
    // DocumentImpl::previousFocusNode() for details).

    DocumentImpl *doc = m_part->xmlDocImpl();
    NodeImpl *oldFocusNode = doc->focusNode();

    // See whether we're in the middle of a detach, or hiding of the
    // widget. In this case, we will just clear focus, being careful not to emit events
    // or update rendering. Doing this also prevents the code below from going bonkers with
    // oldFocusNode not actually being focusable, etc.
    if (oldFocusNode) {
	if ((oldFocusNode->renderer() && !oldFocusNode->renderer()->parent())
	      || !oldFocusNode->isTabFocusable()) {
	    doc->quietResetFocus();
	    return true;
	}
    }

#if 1
    // If the user has scrolled the document, then instead of picking
    // the next focusable node in the document, use the first one that
    // is within the visible area (if possible).
    if (d->scrollBarMoved)
    {
	NodeImpl *toFocus;
	if (next)
	    toFocus = doc->nextFocusNode(oldFocusNode);
	else
	    toFocus = doc->previousFocusNode(oldFocusNode);

	if (!toFocus && oldFocusNode) {
	    if (next)
		toFocus = doc->nextFocusNode(NULL);
	    else
		toFocus = doc->previousFocusNode(NULL);
	}

	while (toFocus && toFocus != oldFocusNode)
	{

	    QRect focusNodeRect = toFocus->getRect();
	    if ((focusNodeRect.left() > contentsX()) && (focusNodeRect.right() < contentsX() + visibleWidth()) &&
		(focusNodeRect.top() > contentsY()) && (focusNodeRect.bottom() < contentsY() + visibleHeight())) {
		{
		    QRect r = toFocus->getRect();
		    ensureVisible( r.right(), r.bottom());
		    ensureVisible( r.left(), r.top());
		    d->scrollBarMoved = false;
		    d->tabMovePending = false;
		    d->lastTabbingDirection = next;
		    d->pseudoFocusNode = KHTMLViewPrivate::PFNone;
		    m_part->xmlDocImpl()->setFocusNode(toFocus);
		    Node guard(toFocus);
		    if (!toFocus->hasOneRef() )
		    {
			emit m_part->nodeActivated(Node(toFocus));
		    }
		    return true;
		}
	    }
	    if (next)
		toFocus = doc->nextFocusNode(toFocus);
	    else
		toFocus = doc->previousFocusNode(toFocus);

	    if (!toFocus && oldFocusNode)
	    {
		if (next)
		{
		    toFocus = doc->nextFocusNode(NULL);
		}
		else
		{
		    toFocus = doc->previousFocusNode(NULL);
		}
	    }
	}

	d->scrollBarMoved = false;
    }
#endif

    if (!oldFocusNode && d->pseudoFocusNode == KHTMLViewPrivate::PFNone)
    {
	ensureVisible(contentsX(), next?0:contentsHeight());
	d->scrollBarMoved = false;
	d->pseudoFocusNode = next?KHTMLViewPrivate::PFTop:KHTMLViewPrivate::PFBottom;
	return true;
    }

    NodeImpl *newFocusNode = NULL;

    if (d->tabMovePending && next != d->lastTabbingDirection)
    {
	//qDebug() << " tab move pending and tabbing direction changed!\n";
	newFocusNode = oldFocusNode;
    }
    else if (next)
    {
	if (oldFocusNode || d->pseudoFocusNode == KHTMLViewPrivate::PFTop )
	    newFocusNode = doc->nextFocusNode(oldFocusNode);
    }
    else
    {
	if (oldFocusNode || d->pseudoFocusNode == KHTMLViewPrivate::PFBottom )
	    newFocusNode = doc->previousFocusNode(oldFocusNode);
    }

    bool targetVisible = false;
    if (!newFocusNode)
    {
	if ( next )
	{
	    targetVisible = scrollTo(QRect(contentsX()+visibleWidth()/2,contentsHeight()-d->borderY,0,0));
	}
	else
	{
	    targetVisible = scrollTo(QRect(contentsX()+visibleWidth()/2,d->borderY,0,0));
	}
    }
    else
    {
        // if it's an editable element, activate the caret
        if (!m_part->isCaretMode() && newFocusNode->isContentEditable()) {
            // qDebug() << "show caret! fn: " << newFocusNode->nodeName().string() << endl;
            m_part->clearCaretRectIfNeeded();
            m_part->d->editor_context.m_selection.moveTo(Position(newFocusNode, 0L));
            m_part->setCaretVisible(true);
        } else {
           m_part->setCaretVisible(false);
           // qDebug() << "hide caret! fn: " << newFocusNode->nodeName().string() << endl;
	}
        m_part->notifySelectionChanged();

	targetVisible = scrollTo(newFocusNode->getRect());
    }

    if (targetVisible)
    {
	//qDebug() << " target reached.\n";
	d->tabMovePending = false;

	m_part->xmlDocImpl()->setFocusNode(newFocusNode);
	if (newFocusNode)
	{
	    Node guard(newFocusNode);
	    if (!newFocusNode->hasOneRef() )
	    {
		emit m_part->nodeActivated(Node(newFocusNode));
	    }
	    return true;
	}
	else
	{
	    d->pseudoFocusNode = next?KHTMLViewPrivate::PFBottom:KHTMLViewPrivate::PFTop;
	    return false;
	}
    }
    else
    {
	if (!d->tabMovePending)
	    d->lastTabbingDirection = next;
	d->tabMovePending = true;
	return true;
    }
}

void KHTMLView::displayAccessKeys()
{
    QVector< QChar > taken;
    displayAccessKeys( NULL, this, taken, false );
    displayAccessKeys( NULL, this, taken, true );
}

void KHTMLView::displayAccessKeys( KHTMLView* caller, KHTMLView* origview, QVector< QChar >& taken, bool use_fallbacks )
{
    QMap< ElementImpl*, QChar > fallbacks;
    if( use_fallbacks )
        fallbacks = buildFallbackAccessKeys();
    for( NodeImpl* n = m_part->xmlDocImpl(); n != NULL; n = n->traverseNextNode()) {
        if( n->isElementNode()) {
            ElementImpl* en = static_cast< ElementImpl* >( n );
            DOMString s = en->getAttribute( ATTR_ACCESSKEY );
            QString accesskey;
            if( s.length() == 1 ) {
                QChar a = s.string()[ 0 ].toUpper();
                if( qFind( taken.begin(), taken.end(), a ) == taken.end()) // !contains
                    accesskey = a;
            }
            if( accesskey.isNull() && fallbacks.contains( en )) {
                QChar a = fallbacks[ en ].toUpper();
                if( qFind( taken.begin(), taken.end(), a ) == taken.end()) // !contains
                    accesskey = QString( "<qt><i>" ) + a + "</i></qt>";
            }
            if( !accesskey.isNull()) {
	        QRect rec=en->getRect();
	        QLabel *lab=new QLabel(accesskey,widget());
	        lab->setAttribute(Qt::WA_DeleteOnClose);
	        lab->setObjectName("KHTMLAccessKey");
	        connect( origview, SIGNAL(hideAccessKeys()), lab, SLOT(close()) );
	        connect( this, SIGNAL(repaintAccessKeys()), lab, SLOT(repaint()));
	        lab->setPalette(QToolTip::palette());
	        lab->setLineWidth(2);
	        lab->setFrameStyle(QFrame::Box | QFrame::Plain);
	        lab->setMargin(3);
	        lab->adjustSize();
	        lab->setParent( widget() );
		lab->setAutoFillBackground(true);
	        lab->move(
			qMin(rec.left()+rec.width()/2 - contentsX(), contentsWidth() - lab->width()),
			qMin(rec.top()+rec.height()/2 - contentsY(), contentsHeight() - lab->height()));
	        lab->show();
                taken.append( accesskey[ 0 ] );
	    }
        }
    }
    if( use_fallbacks )
        return;

    QList<KParts::ReadOnlyPart*> frames = m_part->frames();
    foreach( KParts::ReadOnlyPart* cur, frames ) {
        if( !qobject_cast<KHTMLPart*>(cur) )
            continue;
        KHTMLPart* part = static_cast< KHTMLPart* >( cur );
        if( part->view() && part->view() != caller )
            part->view()->displayAccessKeys( this, origview, taken, use_fallbacks );
    }

    // pass up to the parent
    if (m_part->parentPart() && m_part->parentPart()->view()
        && m_part->parentPart()->view() != caller)
        m_part->parentPart()->view()->displayAccessKeys( this, origview, taken, use_fallbacks );
}

bool KHTMLView::isScrollingFromMouseWheel() const
{
    return d->scrollingFromWheel != QPoint(-1,-1);
}

void KHTMLView::accessKeysTimeout()
{
    d->accessKeysActivated=false;
    d->accessKeysPreActivate = false;
    m_part->setStatusBarText(QString(), KHTMLPart::BarOverrideText);
    emit hideAccessKeys();
}

// Handling of the HTML accesskey attribute.
bool KHTMLView::handleAccessKey( const QKeyEvent* ev )
{
// Qt interprets the keyevent also with the modifiers, and ev->text() matches that,
// but this code must act as if the modifiers weren't pressed
    QChar c;
    if( ev->key() >= Qt::Key_A && ev->key() <= Qt::Key_Z )
        c = 'A' + ev->key() - Qt::Key_A;
    else if( ev->key() >= Qt::Key_0 && ev->key() <= Qt::Key_9 )
        c = '0' + ev->key() - Qt::Key_0;
    else {
        // TODO fake XKeyEvent and XLookupString ?
        // This below seems to work e.g. for eacute though.
        if( ev->text().length() == 1 )
            c = ev->text()[ 0 ];
    }
    if( c.isNull())
        return false;
    return focusNodeWithAccessKey( c );
}

bool KHTMLView::focusNodeWithAccessKey( QChar c, KHTMLView* caller )
{
    DocumentImpl *doc = m_part->xmlDocImpl();
    if( !doc )
        return false;
    ElementImpl* node = doc->findAccessKeyElement( c );
    if( !node ) {
        QList<KParts::ReadOnlyPart*> frames = m_part->frames();
        foreach( KParts::ReadOnlyPart* cur, frames ) {
            if( !qobject_cast<KHTMLPart*>(cur) )
                continue;
            KHTMLPart* part = static_cast< KHTMLPart* >( cur );
            if( part->view() && part->view() != caller
                && part->view()->focusNodeWithAccessKey( c, this ))
                return true;
        }
        // pass up to the parent
        if (m_part->parentPart() && m_part->parentPart()->view()
            && m_part->parentPart()->view() != caller
            && m_part->parentPart()->view()->focusNodeWithAccessKey( c, this ))
            return true;
        if( caller == NULL ) { // the active frame (where the accesskey was pressed)
            const QMap< ElementImpl*, QChar > fallbacks = buildFallbackAccessKeys();
            for( QMap< ElementImpl*, QChar >::ConstIterator it = fallbacks.begin();
                 it != fallbacks.end();
                 ++it )
                if( *it == c ) {
                    node = it.key();
                    break;
                }
        }
        if( node == NULL )
            return false;
    }

    // Scroll the view as necessary to ensure that the new focus node is visible

    QRect r = node->getRect();
    ensureVisible( r.right(), r.bottom());
    ensureVisible( r.left(), r.top());

    Node guard( node );
    if( node->isFocusable()) {
	if (node->id()==ID_LABEL) {
	    // if Accesskey is a label, give focus to the label's referrer.
	    node=static_cast<ElementImpl *>(static_cast< HTMLLabelElementImpl* >( node )->getFormElement());
	    if (!node) return true;
            guard = node;
	}
        // Set focus node on the document
        m_part->xmlDocImpl()->setFocusNode(node);

        if( node != NULL && node->hasOneRef()) // deleted, only held by guard
            return true;
        emit m_part->nodeActivated(Node(node));
        if( node != NULL && node->hasOneRef())
            return true;
    }

    switch( node->id()) {
        case ID_A:
            static_cast< HTMLAnchorElementImpl* >( node )->click();
          break;
        case ID_INPUT:
            static_cast< HTMLInputElementImpl* >( node )->click();
          break;
        case ID_BUTTON:
            static_cast< HTMLButtonElementImpl* >( node )->click();
          break;
        case ID_AREA:
            static_cast< HTMLAreaElementImpl* >( node )->click();
          break;
        case ID_TEXTAREA:
	  break; // just focusing it is enough
        case ID_LEGEND:
            // TODO
          break;
    }
    return true;
}

static QString getElementText( NodeImpl* start, bool after )
{
    QString ret;             // nextSibling(), to go after e.g. </select>
    for( NodeImpl* n = after ? start->nextSibling() : start->traversePreviousNode();
         n != NULL;
         n = after ? n->traverseNextNode() : n->traversePreviousNode()) {
        if( n->isTextNode()) {
            if( after )
                ret += static_cast< TextImpl* >( n )->toString().string();
            else
                ret.prepend( static_cast< TextImpl* >( n )->toString().string());
        } else {
            switch( n->id()) {
                case ID_A:
                case ID_FONT:
                case ID_TT:
                case ID_U:
                case ID_B:
                case ID_I:
                case ID_S:
                case ID_STRIKE:
                case ID_BIG:
                case ID_SMALL:
                case ID_EM:
                case ID_STRONG:
                case ID_DFN:
                case ID_CODE:
                case ID_SAMP:
                case ID_KBD:
                case ID_VAR:
                case ID_CITE:
                case ID_ABBR:
                case ID_ACRONYM:
                case ID_SUB:
                case ID_SUP:
                case ID_SPAN:
                case ID_NOBR:
                case ID_WBR:
                    break;
                case ID_TD:
                    if( ret.trimmed().isEmpty())
                        break;
                    // fall through
                default:
                    return ret.simplified();
            }
        }
    }
    return ret.simplified();
}

static QMap< NodeImpl*, QString > buildLabels( NodeImpl* start )
{
    QMap< NodeImpl*, QString > ret;
    for( NodeImpl* n = start;
         n != NULL;
         n = n->traverseNextNode()) {
        if( n->id() == ID_LABEL ) {
            HTMLLabelElementImpl* label = static_cast< HTMLLabelElementImpl* >( n );
            NodeImpl* labelfor = label->getFormElement();
            if( labelfor )
                ret[ labelfor ] = label->innerText().string().simplified();
        }
    }
    return ret;
}

namespace khtml {
struct AccessKeyData {
    ElementImpl* element;
    QString text;
    QString url;
    int priority; // 10(highest) - 0(lowest)
};
}

QMap< ElementImpl*, QChar > KHTMLView::buildFallbackAccessKeys() const
{
    // build a list of all possible candidate elements that could use an accesskey
    QLinkedList< AccessKeyData > data; // Note: this has to be a list type that keep iterators valid
                                       // when other entries are removed
    QMap< NodeImpl*, QString > labels = buildLabels( m_part->xmlDocImpl());
    QMap< QString, QChar > hrefs;

    for( NodeImpl* n = m_part->xmlDocImpl();
         n != NULL;
         n = n->traverseNextNode()) {
        if( n->isElementNode()) {
            ElementImpl* element = static_cast< ElementImpl* >( n );
            if( element->renderer() == NULL )
                continue; // not visible
            QString text;
            QString url;
            int priority = 0;
            bool ignore = false;
            bool text_after = false;
            bool text_before = false;
            switch( element->id()) {
                case ID_A:
                    url = khtml::parseURL(element->getAttribute(ATTR_HREF)).string();
                    if( url.isEmpty()) // doesn't have href, it's only an anchor
                        continue;
                    text = static_cast< HTMLElementImpl* >( element )->innerText().string().simplified();
                    priority = 2;
                    break;
                case ID_INPUT: {
                    HTMLInputElementImpl* in = static_cast< HTMLInputElementImpl* >( element );
                    switch( in->inputType()) {
                        case HTMLInputElementImpl::SUBMIT:
                            text = in->value().string();
                            if( text.isEmpty())
                                text = i18n( "Submit" );
                            priority = 7;
                            break;
                        case HTMLInputElementImpl::IMAGE:
                            text = in->altText().string();
                            priority = 7;
                            break;
                        case HTMLInputElementImpl::BUTTON:
                            text = in->value().string();
                            priority = 5;
                            break;
                        case HTMLInputElementImpl::RESET:
                            text = in->value().string();
                            if( text.isEmpty())
                                text = i18n( "Reset" );
                            priority = 5;
                            break;
                        case HTMLInputElementImpl::HIDDEN:
                            ignore = true;
                            break;
                        case HTMLInputElementImpl::CHECKBOX:
                        case HTMLInputElementImpl::RADIO:
                            text_after = true;
                            priority = 5;
                            break;
                        case HTMLInputElementImpl::TEXT:
                        case HTMLInputElementImpl::PASSWORD:
                        case HTMLInputElementImpl::FILE:
                            text_before = true;
                            priority = 5;
                            break;
                        default:
                            priority = 5;
                            break;
                    }
                    break;
                }
                case ID_BUTTON:
                    text = static_cast< HTMLElementImpl* >( element )->innerText().string().simplified();
                    switch( static_cast< HTMLButtonElementImpl* >( element )->buttonType()) {
                        case HTMLButtonElementImpl::SUBMIT:
                            if( text.isEmpty())
                                text = i18n( "Submit" );
                            priority = 7;
                            break;
                        case HTMLButtonElementImpl::RESET:
                            if( text.isEmpty())
                                text = i18n( "Reset" );
                            priority = 5;
                            break;
                        default:
                            priority = 5;
                            break;
                    }
                    break;
                case ID_SELECT: // these don't have accesskey attribute, but quick access may be handy
                    text_before = true;
                    text_after = true;
                    priority = 5;
                    break;
                case ID_FRAME:
                    ignore = true;
                    break;
                default:
                    ignore = !element->isFocusable();
                    priority = 2;
                    break;
            }
            if( ignore )
                continue;

            // build map of manually assigned accesskeys and their targets
            DOMString akey = element->getAttribute( ATTR_ACCESSKEY );
            if( akey.length() == 1 ) {
                hrefs[url] = akey.string()[ 0 ].toUpper();
                continue; // has accesskey set, ignore
            }
            if( text.isNull() && labels.contains( element ))
                text = labels[ element ];
            if( text.isNull() && text_before )
                text = getElementText( element, false );
            if( text.isNull() && text_after )
                text = getElementText( element, true );
            text = text.trimmed();
            // increase priority of items which have explicitly specified accesskeys in the config
            const QList< QPair< QString, QChar > > priorities
                = m_part->settings()->fallbackAccessKeysAssignments();
            for( QList< QPair< QString, QChar > >::ConstIterator it = priorities.begin();
                 it != priorities.end();
                 ++it ) {
                if( text == (*it).first )
                    priority = 10;
            }
            AccessKeyData tmp = { element, text, url, priority };
            data.append( tmp );
        }
    }

    QList< QChar > keys;
    for( char c = 'A'; c <= 'Z'; ++c )
        keys << c;
    for( char c = '0'; c <= '9'; ++c )
        keys << c;
    for( NodeImpl* n = m_part->xmlDocImpl();
         n != NULL;
         n = n->traverseNextNode()) {
        if( n->isElementNode()) {
            ElementImpl* en = static_cast< ElementImpl* >( n );
            DOMString s = en->getAttribute( ATTR_ACCESSKEY );
            if( s.length() == 1 ) {
                QChar c = s.string()[ 0 ].toUpper();
                keys.removeAll( c ); // remove manually assigned accesskeys
            }
        }
    }

    QMap< ElementImpl*, QChar > ret;
    for( int priority = 10; priority >= 0; --priority ) {
        for( QLinkedList< AccessKeyData >::Iterator it = data.begin();
             it != data.end();
             ) {
            if( (*it).priority != priority ) {
                ++it;
                continue;
            }
            if( keys.isEmpty())
                break;
            QString text = (*it).text;
            QChar key;
            const QString url = (*it).url;
            // an identical link already has an accesskey assigned
            if( hrefs.contains( url ) ) {
                it = data.erase( it );
                continue;
            }
            if( !text.isEmpty()) {
                const QList< QPair< QString, QChar > > priorities
                    = m_part->settings()->fallbackAccessKeysAssignments();
                for( QList< QPair< QString, QChar > >::ConstIterator it = priorities.begin();
                     it != priorities.end();
                     ++it )
                    if( text == (*it).first && keys.contains( (*it).second )) {
                        key = (*it).second;
                        break;
                    }
            }
            // try first to select the first character as the accesskey,
            // then first character of the following words,
            // and then simply the first free character
            if( key.isNull() && !text.isEmpty()) {
                const QStringList words = text.split( ' ' );
                for( QStringList::ConstIterator it = words.begin();
                     it != words.end();
                     ++it ) {
                    if( keys.contains( (*it)[ 0 ].toUpper())) {
                        key = (*it)[ 0 ].toUpper();
                        break;
                    }
                }
            }
            if( key.isNull() && !text.isEmpty()) {
                for( int i = 0; i < text.length(); ++i ) {
                    if( keys.contains( text[ i ].toUpper())) {
                        key = text[ i ].toUpper();
                        break;
                    }
                }
            }
            if( key.isNull())
                key = keys.front();
            ret[ (*it).element ] = key;
            keys.removeAll( key );
            it = data.erase( it );
            // assign the same accesskey also to other elements pointing to the same url
            if( !url.isEmpty() && !url.startsWith( "javascript:", Qt::CaseInsensitive )) {
                for( QLinkedList< AccessKeyData >::Iterator it2 = data.begin();
                     it2 != data.end();
                     ) {
                    if( (*it2).url == url ) {
                        ret[ (*it2).element ] = key;
                        if( it == it2 )
                            ++it;
                        it2 = data.erase( it2 );
                    } else
                        ++it2;
                }
            }
        }
    }
    return ret;
}

void KHTMLView::setMediaType( const QString &medium )
{
    m_medium = medium;
}

QString KHTMLView::mediaType() const
{
    return m_medium;
}

bool KHTMLView::pagedMode() const
{
    return d->paged;
}

void KHTMLView::setWidgetVisible(RenderWidget* w, bool vis)
{
    if (vis) {
        d->visibleWidgets.insert(w, w->widget());
    }
    else
        d->visibleWidgets.remove(w);
}

bool KHTMLView::needsFullRepaint() const
{
    return d->needsFullRepaint;
}

namespace {
   class QPointerDeleter
   {
   public:
       explicit QPointerDeleter(QObject* o) : obj(o) {}
       ~QPointerDeleter() { delete obj; }
   private:
       const QPointer<QObject> obj;
   };
}

void KHTMLView::print(bool quick)
{
    if(!m_part->xmlDocImpl()) return;
    khtml::RenderCanvas *root = static_cast<khtml::RenderCanvas *>(m_part->xmlDocImpl()->renderer());
    if(!root) return;

    QPointer<KHTMLPrintSettings> printSettings(new KHTMLPrintSettings); //XXX: doesn't save settings between prints like this
    const QPointerDeleter settingsDeleter(printSettings); //the printdialog takes ownership of the settings widget, thus this workaround to avoid double deletion
    QPrinter printer;
    QPointer<QPrintDialog> dialog(new QPrintDialog(&printer, this));
    printSettings->setParent(printSettings);
    dialog->setOptionTabs(QList<QWidget*>() << printSettings.data());

    const QPointerDeleter dialogDeleter(dialog);

    QString docname = m_part->xmlDocImpl()->URL().toDisplayString();
    if ( !docname.isEmpty() )
        docname = KStringHandler::csqueeze(docname, 80);

    if(quick || (dialog->exec() && dialog)) { /*'this' and thus dialog might have been deleted while exec()!*/
        viewport()->setCursor( Qt::WaitCursor ); // only viewport(), no QApplication::, otherwise we get the busy cursor in kdeprint's dialogs
        // set up KPrinter
        printer.setFullPage(false);
        printer.setCreator(QString("KDE %1.%2.%3 HTML Library").arg(KHTML_VERSION_MAJOR).arg(KHTML_VERSION_MINOR).arg(KHTML_VERSION_PATCH));
        printer.setDocName(docname);

        QPainter *p = new QPainter;
        p->begin( &printer );
        khtml::setPrintPainter( p );

        m_part->xmlDocImpl()->setPaintDevice( &printer );
        QString oldMediaType = mediaType();
        setMediaType( "print" );
        // We ignore margin settings for html and body when printing
        // and use the default margins from the print-system
        // (In Qt 3.0.x the default margins are hardcoded in Qt)
        m_part->xmlDocImpl()->setPrintStyleSheet( printSettings->printFriendly() ?
                                                  "* { background-image: none !important;"
                                                  "    background-color: white !important;"
                                                  "    color: black !important; }"
						  "body { margin: 0px !important; }"
						  "html { margin: 0px !important; }" :
						  "body { margin: 0px !important; }"
						  "html { margin: 0px !important; }"
						  );

        // qDebug() << "printing: physical page width = " << printer.width()
        //              << " height = " << printer.height() << endl;
        root->setStaticMode(true);
        root->setPagedMode(true);
        root->setWidth(printer.width());
//         root->setHeight(printer.height());
        root->setPageTop(0);
        root->setPageBottom(0);
        d->paged = true;

        m_part->xmlDocImpl()->styleSelector()->computeFontSizes(printer.logicalDpiY(), 100);
        m_part->xmlDocImpl()->updateStyleSelector();
        root->setPrintImages(printSettings->printImages());
        root->makePageBreakAvoidBlocks();

        root->setNeedsLayoutAndMinMaxRecalc();
        root->layout();

        // check sizes ask for action.. (scale or clip)

        bool printHeader = printSettings->printHeader();

        int headerHeight = 0;
        QFont headerFont("Sans Serif", 8);

        QString headerLeft = QDate::currentDate().toString(Qt::DefaultLocaleShortDate);
        QString headerMid = docname;
        QString headerRight;

        if (printHeader)
        {
           p->setFont(headerFont);
           headerHeight = (p->fontMetrics().lineSpacing() * 3) / 2;
        }

        // ok. now print the pages.
        // qDebug() << "printing: html page width = " << root->docWidth()
        //              << " height = " << root->docHeight() << endl;
        // qDebug() << "printing: margins left = " << printer.pageRect().left() - printer.paperRect().left()
        //              << " top = " << printer.pageRect().top() - printer.paperRect().top() << endl;
        // qDebug() << "printing: paper width = " << printer.width()
        //              << " height = " << printer.height() << endl;
        // if the width is too large to fit on the paper we just scale
        // the whole thing.
        int pageWidth = printer.width();
        int pageHeight = printer.height();
        p->setClipRect(0,0, pageWidth, pageHeight);

        pageHeight -= headerHeight;

#ifndef QT_NO_TRANSFORMATIONS
        bool scalePage = false;
        double scale = 0.0;
        if(root->docWidth() > printer.width()) {
            scalePage = true;
            scale = ((double) printer.width())/((double) root->docWidth());
            pageHeight = (int) (pageHeight/scale);
            pageWidth = (int) (pageWidth/scale);
            headerHeight = (int) (headerHeight/scale);
        }
#endif
        // qDebug() << "printing: scaled html width = " << pageWidth
        //              << " height = " << pageHeight << endl;

        root->setHeight(pageHeight);
        root->setPageBottom(pageHeight);
        root->setNeedsLayout(true);
        root->layoutIfNeeded();
//         m_part->slotDebugRenderTree();

        // Squeeze header to make it it on the page.
        if (printHeader)
        {
            int available_width = printer.width() - 10 -
                2 * qMax(p->boundingRect(0, 0, printer.width(), p->fontMetrics().lineSpacing(), Qt::AlignLeft, headerLeft).width(),
                         p->boundingRect(0, 0, printer.width(), p->fontMetrics().lineSpacing(), Qt::AlignLeft, headerRight).width());
            if (available_width < 150)
               available_width = 150;
            int mid_width;
            int squeeze = 120;
            do {
                headerMid = KStringHandler::csqueeze(docname, squeeze);
                mid_width = p->boundingRect(0, 0, printer.width(), p->fontMetrics().lineSpacing(), Qt::AlignLeft, headerMid).width();
                squeeze -= 10;
            } while (mid_width > available_width);
        }

        int top = 0;
        int bottom = 0;
        int page = 1;
        while(top < root->docHeight()) {
            if(top > 0) printer.newPage();
#ifndef QT_NO_TRANSFORMATIONS
            if (scalePage)
                p->scale(scale, scale);
#endif
            p->save();
            p->setClipRect(0, 0, pageWidth, headerHeight);
            if (printHeader)
            {
                int dy = p->fontMetrics().lineSpacing();
                p->setPen(Qt::black);
                p->setFont(headerFont);

                headerRight = QString("#%1").arg(page);

                p->drawText(0, 0, printer.width(), dy, Qt::AlignLeft, headerLeft);
                p->drawText(0, 0, printer.width(), dy, Qt::AlignHCenter, headerMid);
                p->drawText(0, 0, printer.width(), dy, Qt::AlignRight, headerRight);
            }

            p->restore();
            p->translate(0, headerHeight-top);

            bottom = top+pageHeight;

            root->setPageTop(top);
            root->setPageBottom(bottom);
            root->setPageNumber(page);

            root->layer()->paint(p, QRect(0, top, pageWidth, pageHeight));
            // qDebug() << "printed: page " << page <<" bottom At = " << bottom;

            top = bottom;
            p->resetTransform();
            page++;
        }

        p->end();
        delete p;

        // and now reset the layout to the usual one...
        root->setPagedMode(false);
        root->setStaticMode(false);
        d->paged = false;
        khtml::setPrintPainter( 0 );
        setMediaType( oldMediaType );
        m_part->xmlDocImpl()->setPaintDevice( this );
        m_part->xmlDocImpl()->styleSelector()->computeFontSizes(m_part->xmlDocImpl()->logicalDpiY(), m_part->fontScaleFactor());
        m_part->xmlDocImpl()->updateStyleSelector();
        viewport()->unsetCursor();
    }
}

void KHTMLView::slotPaletteChanged()
{
    if(!m_part->xmlDocImpl()) return;
    DOM::DocumentImpl *document = m_part->xmlDocImpl();
    if (!document->isHTMLDocument()) return;
    khtml::RenderCanvas *root = static_cast<khtml::RenderCanvas *>(document->renderer());
    if(!root) return;
    root->style()->resetPalette();
    NodeImpl *body = static_cast<HTMLDocumentImpl*>(document)->body();
    if(!body) return;
    body->setChanged(true);
    body->recalcStyle( NodeImpl::Force );
}

void KHTMLView::paint(QPainter *p, const QRect &rc, int yOff, bool *more)
{
    if(!m_part->xmlDocImpl()) return;
    khtml::RenderCanvas *root = static_cast<khtml::RenderCanvas *>(m_part->xmlDocImpl()->renderer());
    if(!root) return;
#ifdef SPEED_DEBUG
    d->firstRepaintPending = false;
#endif

    QPaintDevice* opd = m_part->xmlDocImpl()->paintDevice();
    m_part->xmlDocImpl()->setPaintDevice(p->device());
    root->setPagedMode(true);
    root->setStaticMode(true);
    root->setWidth(rc.width());

    // save()
    QRegion creg = p->clipRegion();
    QTransform t = p->worldTransform();
    QRect w = p->window();
    QRect v = p->viewport();
    bool vte = p->viewTransformEnabled();
    bool wme = p->worldMatrixEnabled();

    p->setClipRect(rc);
    p->translate(rc.left(), rc.top());
    double scale = ((double) rc.width()/(double) root->docWidth());
    int height = (int) ((double) rc.height() / scale);
#ifndef QT_NO_TRANSFORMATIONS
    p->scale(scale, scale);
#endif
    root->setPageTop(yOff);
    root->setPageBottom(yOff+height);

    root->layer()->paint(p, QRect(0, yOff, root->docWidth(), height));
    if (more)
        *more = yOff + height < root->docHeight();

    // restore()
    p->setWorldTransform(t);
    p->setWindow(w);
    p->setViewport(v);
    p->setViewTransformEnabled( vte );
    p->setWorldMatrixEnabled( wme );
    if (!creg.isEmpty())
        p->setClipRegion( creg );
    else
        p->setClipRegion(QRegion(), Qt::NoClip);

    root->setPagedMode(false);
    root->setStaticMode(false);
    m_part->xmlDocImpl()->setPaintDevice( opd );
}

void KHTMLView::render(QPainter* p, const QRect& r, const QPoint& off)
{
#ifdef SPEED_DEBUG
    d->firstRepaintPending = false;
#endif
    QRect clip(off.x()+r.x(), off.y()+r.y(),r.width(),r.height());
    if(!m_part || !m_part->xmlDocImpl() || !m_part->xmlDocImpl()->renderer()) {
        p->fillRect(clip, palette().brush(QPalette::Active, QPalette::Base));
        return;
    }
    QPaintDevice* opd = m_part->xmlDocImpl()->paintDevice();
    m_part->xmlDocImpl()->setPaintDevice(p->device());

    // save()
    QRegion creg = p->clipRegion();
    QTransform t = p->worldTransform();
    QRect w = p->window();
    QRect v = p->viewport();
    bool vte = p->viewTransformEnabled();
    bool wme = p->worldMatrixEnabled();

    p->setClipRect(clip);
    QRect rect = r.translated(contentsX(),contentsY());
    p->translate(off.x()-contentsX(), off.y()-contentsY());

    m_part->xmlDocImpl()->renderer()->layer()->paint(p, rect);

    // restore()
    p->setWorldTransform(t);
    p->setWindow(w);
    p->setViewport(v);
    p->setViewTransformEnabled( vte );
    p->setWorldMatrixEnabled( wme );
    if (!creg.isEmpty())
        p->setClipRegion( creg );
    else
        p->setClipRegion(QRegion(), Qt::NoClip);

    m_part->xmlDocImpl()->setPaintDevice( opd );
}

void KHTMLView::setHasStaticBackground(bool partial)
{
    // full static iframe is irreversible for now
    if (d->staticWidget == KHTMLViewPrivate::SBFull && m_kwp->isRedirected())
        return;

    d->staticWidget = partial ?
                          KHTMLViewPrivate::SBPartial : KHTMLViewPrivate::SBFull;
}

void KHTMLView::setHasNormalBackground()
{
    // full static iframe is irreversible for now
    if (d->staticWidget == KHTMLViewPrivate::SBFull && m_kwp->isRedirected())
        return;

    d->staticWidget = KHTMLViewPrivate::SBNone;
}

void KHTMLView::addStaticObject(bool fixed)
{
    if (fixed)
        d->fixedObjectsCount++;
    else
        d->staticObjectsCount++;

    setHasStaticBackground( true /*partial*/ );
}

void KHTMLView::removeStaticObject(bool fixed)
{
    if (fixed)
        d->fixedObjectsCount--;
    else
        d->staticObjectsCount--;

    assert( d->fixedObjectsCount >= 0 && d->staticObjectsCount >= 0 );

    if (!d->staticObjectsCount && !d->fixedObjectsCount)
        setHasNormalBackground();
    else
        setHasStaticBackground( true /*partial*/ );
}

void KHTMLView::setVerticalScrollBarPolicy( Qt::ScrollBarPolicy policy )
{
#ifndef KHTML_NO_SCROLLBARS
    d->vpolicy = policy;
    QScrollArea::setVerticalScrollBarPolicy(policy);
#else
    Q_UNUSED( policy );
#endif
}

void KHTMLView::setHorizontalScrollBarPolicy( Qt::ScrollBarPolicy policy )
{
#ifndef KHTML_NO_SCROLLBARS
    d->hpolicy = policy;
    QScrollArea::setHorizontalScrollBarPolicy(policy);
#else
    Q_UNUSED( policy );
#endif
}

void KHTMLView::restoreScrollBar()
{
    int ow = visibleWidth();
    QScrollArea::setVerticalScrollBarPolicy(d->vpolicy);
    if (visibleWidth() != ow)
        layout();
    d->prevScrollbarVisible = verticalScrollBar()->isVisible();
}

QStringList KHTMLView::formCompletionItems(const QString &name) const
{
    if (!m_part->settings()->isFormCompletionEnabled())
        return QStringList();
    if (!d->formCompletions)
        d->formCompletions = new KConfig(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "khtml/formcompletions");
    return d->formCompletions->group("").readEntry(name, QStringList());
}

void KHTMLView::clearCompletionHistory(const QString& name)
{
    if (!d->formCompletions)
    {
        d->formCompletions = new KConfig(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "khtml/formcompletions");
    }
    d->formCompletions->group("").writeEntry(name, "");
    d->formCompletions->sync();
}

void KHTMLView::addFormCompletionItem(const QString &name, const QString &value)
{
    if (!m_part->settings()->isFormCompletionEnabled())
        return;
    // don't store values that are all numbers or just numbers with
    // dashes or spaces as those are likely credit card numbers or
    // something similar
    bool cc_number(true);
    for ( int i = 0; i < value.length(); ++i)
    {
      QChar c(value[i]);
      if (!c.isNumber() && c != '-' && !c.isSpace())
      {
        cc_number = false;
        break;
      }
    }
    if (cc_number)
      return;
    QStringList items = formCompletionItems(name);
    if (!items.contains(value))
        items.prepend(value);
    while ((int)items.count() > m_part->settings()->maxFormCompletionItems())
        items.erase(items.isEmpty() ? items.end() : --items.end());
    d->formCompletions->group("").writeEntry(name, items);
}

void KHTMLView::addNonPasswordStorableSite(const QString& host)
{
    if (!d->formCompletions) {
        d->formCompletions = new KConfig(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "khtml/formcompletions");
    }

    KConfigGroup cg( d->formCompletions, "NonPasswordStorableSites");
    QStringList sites = cg.readEntry("Sites", QStringList());
    sites.append(host);
    cg.writeEntry("Sites", sites);
    cg.sync();
}


void KHTMLView::delNonPasswordStorableSite(const QString& host)
{
    if (!d->formCompletions) {
        d->formCompletions = new KConfig(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "khtml/formcompletions");
    }

    KConfigGroup cg( d->formCompletions, "NonPasswordStorableSites");
    QStringList sites = cg.readEntry("Sites", QStringList());
    sites.removeOne(host);
    cg.writeEntry("Sites", sites);
    cg.sync();
}

bool KHTMLView::nonPasswordStorableSite(const QString& host) const
{
    if (!d->formCompletions) {
        d->formCompletions = new KConfig(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "khtml/formcompletions");
    }
    QStringList sites =  d->formCompletions->group( "NonPasswordStorableSites" ).readEntry("Sites", QStringList());
    return (sites.indexOf(host) != -1);
}

// returns true if event should be swallowed
bool KHTMLView::dispatchMouseEvent(int eventId, DOM::NodeImpl *targetNode,
				   DOM::NodeImpl *targetNodeNonShared, bool cancelable,
				   int detail,QMouseEvent *_mouse, bool setUnder,
				   int mouseEventType, int orient)
{
    // if the target node is a text node, dispatch on the parent node - rdar://4196646 (and #76948)
    if (targetNode && targetNode->isTextNode())
        targetNode = targetNode->parentNode();

    if (d->underMouse)
	d->underMouse->deref();
    d->underMouse = targetNode;
    if (d->underMouse)
	d->underMouse->ref();

    if (d->underMouseNonShared)
	d->underMouseNonShared->deref();
    d->underMouseNonShared = targetNodeNonShared;
    if (d->underMouseNonShared)
	d->underMouseNonShared->ref();

    bool isWheelEvent = (mouseEventType == DOM::NodeImpl::MouseWheel);

    int exceptioncode = 0;
    int pageX = _mouse->x();
    int pageY = _mouse->y();
    revertTransforms(pageX, pageY);
    int clientX = pageX - contentsX();
    int clientY = pageY - contentsY();
    int screenX = _mouse->globalX();
    int screenY = _mouse->globalY();
    int button = -1;
    switch (_mouse->button()) {
	case Qt::LeftButton:
	    button = 0;
	    break;
	case Qt::MidButton:
	    button = 1;
	    break;
	case Qt::RightButton:
	    button = 2;
	    break;
	default:
	    break;
    }
    if (d->accessKeysEnabled && d->accessKeysPreActivate && button!=-1)
    	d->accessKeysPreActivate=false;

    bool ctrlKey = (_mouse->modifiers() & Qt::ControlModifier);
    bool altKey = (_mouse->modifiers() & Qt::AltModifier);
    bool shiftKey = (_mouse->modifiers() & Qt::ShiftModifier);
    bool metaKey = (_mouse->modifiers() & Qt::MetaModifier);

    // mouseout/mouseover
    if (setUnder && d->oldUnderMouse != targetNode) {
        if (d->oldUnderMouse && d->oldUnderMouse->document() != m_part->xmlDocImpl()) {
            d->oldUnderMouse->deref();
            d->oldUnderMouse = 0;
        }
        // send mouseout event to the old node
        if (d->oldUnderMouse) {
        // send mouseout event to the old node
            MouseEventImpl *me = new MouseEventImpl(EventImpl::MOUSEOUT_EVENT,
							true,true,m_part->xmlDocImpl()->defaultView(),
							0,screenX,screenY,clientX,clientY,pageX, pageY,
							ctrlKey,altKey,shiftKey,metaKey,
							button,targetNode);
            me->ref();
            d->oldUnderMouse->dispatchEvent(me,exceptioncode,true);
            me->deref();
        }
        // send mouseover event to the new node
	if (targetNode) {
	    MouseEventImpl *me = new MouseEventImpl(EventImpl::MOUSEOVER_EVENT,
							true,true,m_part->xmlDocImpl()->defaultView(),
							0,screenX,screenY,clientX,clientY,pageX, pageY,
							ctrlKey,altKey,shiftKey,metaKey,
							button,d->oldUnderMouse);

            me->ref();
            targetNode->dispatchEvent(me,exceptioncode,true);
	    me->deref();
	}
	if (d->oldUnderMouse)
	    d->oldUnderMouse->deref();
        d->oldUnderMouse = targetNode;
        if (d->oldUnderMouse)
            d->oldUnderMouse->ref();
    }

    bool swallowEvent = false;

    if (targetNode) {
	// if the target node is a disabled widget, we don't want any full-blown mouse events
	if (targetNode->isGenericFormElement()
	     && static_cast<HTMLGenericFormElementImpl*>(targetNode)->disabled())
	    return true;

        // send the actual event
        bool dblclick = ( eventId == EventImpl::CLICK_EVENT &&
                          _mouse->type() == QEvent::MouseButtonDblClick );
        MouseEventImpl *me = new MouseEventImpl(static_cast<EventImpl::EventId>(eventId),
						true,cancelable,m_part->xmlDocImpl()->defaultView(),
						detail,screenX,screenY,clientX,clientY,pageX, pageY,
						ctrlKey,altKey,shiftKey,metaKey,
						button,0, isWheelEvent ? 0 : _mouse, dblclick,
						isWheelEvent ? static_cast<MouseEventImpl::Orientation>(orient) : MouseEventImpl::ONone );
        me->ref();
        if ( !d->m_mouseEventsTarget && RenderLayer::gScrollBar && eventId == EventImpl::MOUSEDOWN_EVENT )
            // button is pressed inside a layer scrollbar, so make it the target for future mousemove events until released
            d->m_mouseEventsTarget = RenderLayer::gScrollBar;
        if ( d->m_mouseEventsTarget && qobject_cast<QScrollBar*>(d->m_mouseEventsTarget) &&
             dynamic_cast<KHTMLWidget*>(static_cast<QWidget*>(d->m_mouseEventsTarget)) ) {
            // we have a sticky mouse event target and it is a layer's scrollbar. Forward events manually.
            // ### should use the dom
            KHTMLWidget*w = dynamic_cast<KHTMLWidget*>(static_cast<QWidget*>(d->m_mouseEventsTarget));
            QPoint p = w->m_kwp->absolutePos();
            QMouseEvent fw(_mouse->type(), QPoint(pageX, pageY)-p, _mouse->button(), _mouse->buttons(), _mouse->modifiers());
            static_cast<RenderWidget::EventPropagator *>(static_cast<QWidget*>(d->m_mouseEventsTarget))->sendEvent(&fw);
            if (_mouse->type() == QMouseEvent::MouseButtonPress && _mouse->button() == Qt::RightButton) {
                QContextMenuEvent cme(QContextMenuEvent::Mouse, p);
                static_cast<RenderWidget::EventPropagator *>(static_cast<QWidget*>(d->m_mouseEventsTarget))->sendEvent(&cme);
                d->m_mouseEventsTarget = 0;
            }
            swallowEvent = true;
        } else {
            targetNode->dispatchEvent(me,exceptioncode,true);
	    bool defaultHandled = me->defaultHandled();
            if (defaultHandled || me->defaultPrevented())
                swallowEvent = true;
        }
        if (eventId == EventImpl::MOUSEDOWN_EVENT && !me->defaultPrevented()) {
            // Focus should be shifted on mouse down, not on a click.  -dwh
            // Blur current focus node when a link/button is clicked; this
            // is expected by some sites that rely on onChange handlers running
            // from form fields before the button click is processed.
            DOM::NodeImpl* nodeImpl = targetNode;
            for ( ; nodeImpl && !nodeImpl->isFocusable(); nodeImpl = nodeImpl->parentNode())
                {}
            if (nodeImpl && nodeImpl->isMouseFocusable())
                m_part->xmlDocImpl()->setFocusNode(nodeImpl);
            else if (!nodeImpl || !nodeImpl->focused())
                m_part->xmlDocImpl()->setFocusNode(0);
        }
        me->deref();
    }

    return swallowEvent;
}

void KHTMLView::setIgnoreWheelEvents( bool e )
{
    d->ignoreWheelEvents = e;
}

#ifndef QT_NO_WHEELEVENT

void KHTMLView::wheelEvent(QWheelEvent* e)
{
    // check if we should reset the state of the indicator describing if
    // we are currently scrolling the view as a result of wheel events
    if (d->scrollingFromWheel != QPoint(-1,-1) && d->scrollingFromWheel != QCursor::pos())
        d->scrollingFromWheel = d->scrollingFromWheelTimerId ? QCursor::pos() : QPoint(-1,-1);

    if (d->accessKeysEnabled && d->accessKeysPreActivate) d->accessKeysPreActivate=false;

    if ( ( e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier )
    {
        emit zoomView( - e->delta() );
        e->accept();
    }
    else if (d->firstLayoutPending)
    {
        e->accept();
    }
    else if( !m_kwp->isRedirected() &&
             (   (e->orientation() == Qt::Vertical &&
                   ((d->ignoreWheelEvents && !verticalScrollBar()->isVisible())
                     || (e->delta() > 0 && contentsY() <= 0)
                     || (e->delta() < 0 && contentsY() >= contentsHeight() - visibleHeight())))
              ||
                 (e->orientation() == Qt::Horizontal &&
                    ((d->ignoreWheelEvents && !horizontalScrollBar()->isVisible())
                     || (e->delta() > 0 && contentsX() <=0)
                     || (e->delta() < 0 && contentsX() >= contentsWidth() - visibleWidth()))))
            && m_part->parentPart())
    {
        if ( m_part->parentPart()->view() )
            m_part->parentPart()->view()->wheelEvent( e );
        e->ignore();
    }
    else
    {
        int xm = e->x();
        int ym = e->y();
        revertTransforms(xm, ym);

        DOM::NodeImpl::MouseEvent mev( e->buttons(), DOM::NodeImpl::MouseWheel );
        m_part->xmlDocImpl()->prepareMouseEvent( false, xm, ym, &mev );

        MouseEventImpl::Orientation o = MouseEventImpl::OVertical;
        if (e->orientation() == Qt::Horizontal)
            o = MouseEventImpl::OHorizontal;

        QMouseEvent _mouse(QEvent::MouseMove, e->pos(), Qt::NoButton, e->buttons(), e->modifiers());
        bool swallow = dispatchMouseEvent(EventImpl::KHTML_MOUSEWHEEL_EVENT,mev.innerNode.handle(),mev.innerNonSharedNode.handle(),
                                               true,-e->delta()/40,&_mouse,true,DOM::NodeImpl::MouseWheel,o);

        if (swallow)
            return;

        d->scrollBarMoved = true;
        d->scrollingFromWheel = QCursor::pos();
        if (d->smoothScrollMode != SSMDisabled)
            d->shouldSmoothScroll = true;
        if (d->scrollingFromWheelTimerId)
            killTimer(d->scrollingFromWheelTimerId);
        d->scrollingFromWheelTimerId = startTimer(400);

        if (m_part->parentPart()) {
            // don't propagate if we are a sub-frame and our scrollbars are already at end of range
            bool h = (static_cast<QWheelEvent*>(e)->orientation() == Qt::Horizontal);
            bool d = (static_cast<QWheelEvent*>(e)->delta() < 0);
            QScrollBar* hsb = horizontalScrollBar();
            QScrollBar* vsb = verticalScrollBar();
            if ( (h && ((d && hsb->value() == hsb->maximum()) || (!d && hsb->value() == hsb->minimum()))) ||
                (!h && ((d && vsb->value() == vsb->maximum()) || (!d && vsb->value() == vsb->minimum()))) ) {
                e->accept();
                return;
            }
        }
        QScrollArea::wheelEvent( e );
    }

}
#endif

void KHTMLView::dragEnterEvent( QDragEnterEvent* ev )
{
    // Still overridden for BC reasons only...
    QScrollArea::dragEnterEvent( ev );
}

void KHTMLView::dropEvent( QDropEvent *ev )
{
    // Still overridden for BC reasons only...
    QScrollArea::dropEvent( ev );
}

void KHTMLView::focusInEvent( QFocusEvent *e )
{
    DOM::NodeImpl* fn = m_part->xmlDocImpl() ? m_part->xmlDocImpl()->focusNode() : 0;
    if (fn && fn->renderer() && fn->renderer()->isWidget() &&
        (e->reason() != Qt::MouseFocusReason) &&
        static_cast<khtml::RenderWidget*>(fn->renderer())->widget())
        static_cast<khtml::RenderWidget*>(fn->renderer())->widget()->setFocus();
    m_part->setSelectionVisible();
    QScrollArea::focusInEvent( e );
}

void KHTMLView::focusOutEvent( QFocusEvent *e )
{
    if (m_part) {
        m_part->stopAutoScroll();
        m_part->setSelectionVisible(false);
    }

    if ( d->cursorIconWidget )
        d->cursorIconWidget->hide();

    QScrollArea::focusOutEvent( e );
}

void KHTMLView::scrollContentsBy( int dx, int dy )
{
    if (!dx && !dy) return;

    if ( !d->firstLayoutPending && !d->complete && m_part->xmlDocImpl() &&
          d->layoutSchedulingEnabled) {
        // contents scroll while we are not complete: we need to check our layout *now*
        khtml::RenderCanvas* root = static_cast<khtml::RenderCanvas *>( m_part->xmlDocImpl()->renderer() );
        if (root && root->needsLayout()) {
            unscheduleRelayout();
            layout();
        }
    }

    if ( d->shouldSmoothScroll && d->smoothScrollMode != SSMDisabled && m_part->xmlDocImpl() &&
          m_part->xmlDocImpl()->renderer() && (d->smoothScrollMode != SSMWhenEfficient || d->smoothScrollMissedDeadlines != sWayTooMany)) {

        bool doSmoothScroll = (!d->staticWidget || d->smoothScrollMode == SSMEnabled);

        int numStaticPixels = 0;
        QRegion r = static_cast<RenderCanvas*>(m_part->xmlDocImpl()->renderer())->staticRegion();

        // only do smooth scrolling if static region is relatively small
        if (!doSmoothScroll && d->staticWidget == KHTMLViewPrivate::SBPartial && r.rects().size() <= 10) {
            foreach(const QRect &rr, r.rects())
                numStaticPixels += rr.width()*rr.height();
            if ((numStaticPixels < sSmoothScrollMinStaticPixels) || (numStaticPixels*8 < visibleWidth()*visibleHeight()))
                doSmoothScroll = true;
        }
        if (doSmoothScroll) {
            setupSmoothScrolling(dx, dy);
            return;
        }
    }

    if ( underMouse() && QToolTip::isVisible() )
        QToolTip::hideText();

    if (!d->scrollingSelf) {
        d->scrollBarMoved = true;
        d->contentsMoving = true;
        // ensure quick reset of contentsMoving flag
        scheduleRepaint(0, 0, 0, 0);
    }

    if (m_part->xmlDocImpl() && m_part->xmlDocImpl()->documentElement()) {
        m_part->xmlDocImpl()->documentElement()->dispatchHTMLEvent(EventImpl::SCROLL_EVENT, true, false);
    }

    if (QApplication::isRightToLeft())
        dx = -dx;

    if (!d->smoothScrolling) {
        d->updateContentsXY();
    } else {
        d->contentsX -= dx;
        d->contentsY -= dy;
    }
    if (widget()->pos() != QPoint(0,0)) {
         // qDebug() << "Static widget wasn't positioned at (0,0). This should NOT happen. Please report this event to developers.";
         widget()->move(0,0);
    }

    QWidget *w = widget();
    QPoint off;
    if (m_kwp->isRedirected()) {
        // This is a redirected sub frame. Translate to root view context
        KHTMLView* v = m_kwp->rootViewPos( off );
        if (v)
            w = v->widget();
        off = viewport()->mapTo(this, off);
    }

    if ( d->staticWidget ) {

        // now remove from view the external widgets that must have completely
        // disappeared after dx/dy scroll delta is effective
        if (!d->visibleWidgets.isEmpty())
            checkExternalWidgetsPosition();

        if ( d->staticWidget == KHTMLViewPrivate::SBPartial
                                && m_part->xmlDocImpl() && m_part->xmlDocImpl()->renderer() ) {
            // static objects might be selectively repainted, like stones in flowing water
            QRegion r = static_cast<RenderCanvas*>(m_part->xmlDocImpl()->renderer())->staticRegion();
            r.translate( -contentsX(), -contentsY());
            QVector<QRect> ar = r.rects();

            for (int i = 0; i < ar.size() ; ++i) {
                widget()->update( ar[i] );
            }
            r = QRegion(QRect(0, 0, visibleWidth(), visibleHeight())) - r;
            ar = r.rects();
            for (int i = 0; i < ar.size() ; ++i) {
                w->scroll( dx, dy, ar[i].translated(off) );
            }
            d->scrollExternalWidgets(dx, dy);
        } else {
            // we can't avoid a full update
            widget()->update();
        }
        if (d->accessKeysActivated)
            d->scrollAccessKeys(dx, dy);

        return;
    }

    if (m_kwp->isRedirected()) {
        const QRect rect(off.x(), off.y(), visibleWidth() * d->zoomLevel / 100, visibleHeight() * d->zoomLevel / 100);
        w->scroll(dx, dy, rect);
        if (d->zoomLevel != 100) {
            w->update(rect); // without this update we are getting bad rendering when an iframe is zoomed in
        }
    }  else {
        widget()->scroll(dx, dy, widget()->rect() & viewport()->rect());
    }

    d->scrollExternalWidgets(dx, dy);
    if (d->accessKeysActivated)
        d->scrollAccessKeys(dx, dy);
}

void KHTMLView::setupSmoothScrolling(int dx, int dy)
{
    // old or minimum speed
    int ddx = qMax(d->steps ? abs(d->dx)/d->steps : 0,3);
    int ddy = qMax(d->steps ? abs(d->dy)/d->steps : 0,3);

    // full scroll is remaining scroll plus new scroll
    d->dx = d->dx + dx;
    d->dy = d->dy + dy;

    if (d->dx == 0 && d->dy == 0) {
        d->stopScrolling();
        return;
    }

    d->steps = (sSmoothScrollTime-1)/sSmoothScrollTick + 1;

    if (qMax(abs(d->dx), abs(d->dy)) / d->steps < qMax(ddx,ddy)) {
        // Don't move slower than average 4px/step in minimum one direction
        // This means fewer than normal steps
        d->steps = qMax((abs(d->dx)+ddx-1)/ddx, (abs(d->dy)+ddy-1)/ddy);
        if (d->steps < 1) d->steps = 1;
    }

    d->smoothScrollStopwatch.start();
    if (!d->smoothScrolling) {
        d->startScrolling();
        scrollTick();
    }
}

void KHTMLView::scrollTick() {
    if (d->dx == 0 && d->dy == 0) {
        d->stopScrolling();
        return;
    }

    if (d->steps < 1) d->steps = 1;
    int takesteps = d->smoothScrollStopwatch.restart() / sSmoothScrollTick;
    int scroll_x = 0;
    int scroll_y = 0;
    if (takesteps < 1) takesteps = 1;
    if (takesteps > d->steps) takesteps = d->steps;
    for(int i = 0; i < takesteps; i++) {
        int ddx = (d->dx / (d->steps+1)) * 2;
        int ddy = (d->dy / (d->steps+1)) * 2;

        // limit step to requested scrolling distance
        if (abs(ddx) > abs(d->dx)) ddx = d->dx;
        if (abs(ddy) > abs(d->dy)) ddy = d->dy;

        // update remaining scroll
        d->dx -= ddx;
        d->dy -= ddy;
        scroll_x += ddx;
        scroll_y += ddy;
        d->steps--;
    }

    d->shouldSmoothScroll = false;
    scrollContentsBy(scroll_x, scroll_y);

    if (takesteps < 2) {
        d->smoothScrollMissedDeadlines = 0;
    } else {
        if (d->smoothScrollMissedDeadlines != sWayTooMany &&
                (!m_part->xmlDocImpl() || !m_part->xmlDocImpl()->parsing())) {
            d->smoothScrollMissedDeadlines++;
            if (d->smoothScrollMissedDeadlines >= sMaxMissedDeadlines) {
                // we missed many deadlines in a row!
                // time to signal we had enough..
                d->smoothScrollMissedDeadlines = sWayTooMany;
            }
        }
    }
}


void KHTMLView::addChild(QWidget * child, int x, int y)
{
    if (!child)
        return;

    if (child->parent() != widget())
        child->setParent( widget() );

    // ### handle pseudo-zooming of non-redirected widgets (e.g. just resize'em)

    child->move(x-contentsX(), y-contentsY());
}

void KHTMLView::timerEvent ( QTimerEvent *e )
{
//    qDebug() << "timer event " << e->timerId();
    if ( e->timerId() == d->scrollTimerId ) {
        if( d->scrollSuspended )
            return;
        switch (d->scrollDirection) {
            case KHTMLViewPrivate::ScrollDown:
                if (contentsY() + visibleHeight () >= contentsHeight())
                    d->newScrollTimer(this, 0);
                else
                    verticalScrollBar()->setValue( verticalScrollBar()->value() +d->scrollBy );
                break;
            case KHTMLViewPrivate::ScrollUp:
                if (contentsY() <= 0)
                    d->newScrollTimer(this, 0);
                else
                    verticalScrollBar()->setValue( verticalScrollBar()->value() -d->scrollBy );
                break;
            case KHTMLViewPrivate::ScrollRight:
                if (contentsX() + visibleWidth () >= contentsWidth())
                    d->newScrollTimer(this, 0);
                else
                    horizontalScrollBar()->setValue( horizontalScrollBar()->value() +d->scrollBy );
                break;
            case KHTMLViewPrivate::ScrollLeft:
                if (contentsX() <= 0)
                    d->newScrollTimer(this, 0);
                else
                    horizontalScrollBar()->setValue( horizontalScrollBar()->value() -d->scrollBy );
                break;
        }
        return;
    }
    else if ( e->timerId() == d->scrollingFromWheelTimerId ) {
        killTimer( d->scrollingFromWheelTimerId );
        d->scrollingFromWheelTimerId = 0;
    } else if ( e->timerId() == d->layoutTimerId ) {
        if (d->firstLayoutPending && d->layoutAttemptCounter < 4
                           && (!m_part->xmlDocImpl() || !m_part->xmlDocImpl()->readyForLayout())) {
            d->layoutAttemptCounter++;
            killTimer(d->layoutTimerId);
            d->layoutTimerId = 0;
            scheduleRelayout();
            return;
        }
        layout();
        d->scheduledLayoutCounter++;
        if (d->firstLayoutPending) {
            d->firstLayoutPending = false;
            verticalScrollBar()->setEnabled( true );
            horizontalScrollBar()->setEnabled( true );
        }
    }

    d->contentsMoving = false;
    if( m_part->xmlDocImpl() ) {
	DOM::DocumentImpl *document = m_part->xmlDocImpl();
	khtml::RenderCanvas* root = static_cast<khtml::RenderCanvas *>(document->renderer());

	if ( root && root->needsLayout() ) {
	    if (d->repaintTimerId)
	        killTimer(d->repaintTimerId);
	    d->repaintTimerId = 0;
	    scheduleRelayout();
	    return;
	}
    }

    if (d->repaintTimerId)
        killTimer(d->repaintTimerId);
    d->repaintTimerId = 0;

    QRect updateRegion;
    const QVector<QRect> rects = d->updateRegion.rects();

    d->updateRegion = QRegion();

    if ( rects.size() )
        updateRegion = rects[0];

    for ( int i = 1; i < rects.size(); ++i ) {
        QRect newRegion = updateRegion.unite(rects[i]);
        if (2*newRegion.height() > 3*updateRegion.height() )
        {
            repaintContents( updateRegion );
            updateRegion = rects[i];
        }
        else
            updateRegion = newRegion;
    }

    if ( !updateRegion.isNull() )
        repaintContents( updateRegion );

    // As widgets can only be accurately positioned during painting, every layout might
    // dissociate a widget from its RenderWidget. E.g: if a RenderWidget was visible before layout, but the layout
    // pushed it out of the viewport, it will not be repainted, and consequently it's associated widget won't be repositioned.
    // Thus we need to check each supposedly 'visible' widget at the end of layout, and remove it in case it's no more in sight.

    if (d->dirtyLayout && !d->visibleWidgets.isEmpty())
        checkExternalWidgetsPosition();

    d->dirtyLayout = false;

    emit repaintAccessKeys();
    if (d->emitCompletedAfterRepaint) {
        bool full = d->emitCompletedAfterRepaint == KHTMLViewPrivate::CSFull;
        d->emitCompletedAfterRepaint = KHTMLViewPrivate::CSNone;
        if ( full )
            emit m_part->completed();
        else
            emit m_part->completed(true);
    }
}

void KHTMLView::checkExternalWidgetsPosition()
{
    QWidget* w;
    QRect visibleRect(contentsX(), contentsY(), visibleWidth(), visibleHeight());
    QList<RenderWidget*> toRemove;
    QHashIterator<void*, QWidget*> it(d->visibleWidgets);
    while (it.hasNext()) {
        int xp = 0, yp = 0;
        it.next();
        RenderWidget* rw = static_cast<RenderWidget*>( it.key() );
        if (!rw->absolutePosition(xp, yp) ||
            !visibleRect.intersects(QRect(xp, yp, it.value()->width(), it.value()->height())))
            toRemove.append(rw);
    }
    foreach (RenderWidget* r, toRemove)
        if ( (w = d->visibleWidgets.take(r) ) )
            w->move( 0, -500000);
}

void KHTMLView::scheduleRelayout(khtml::RenderObject * /*clippedObj*/)
{
    if (!d->layoutSchedulingEnabled || d->layoutTimerId)
        return;

    int time = 0;
    if (d->firstLayoutPending) {
        // Any repaint happening while we have no content blanks the viewport ("white flash").
        // Hence the need to delay the first layout as much as we can.
        // Only if the document gets stuck for too long in incomplete state will we allow the blanking.
        time = d->layoutAttemptCounter ?
               sLayoutAttemptDelay + sLayoutAttemptIncrement*d->layoutAttemptCounter : sFirstLayoutDelay;
    } else if (m_part->xmlDocImpl() && m_part->xmlDocImpl()->parsing()) {
        // Delay between successive layouts in parsing mode.
        // Increment reflects the decaying importance of visual feedback over time.
        time = qMin(2000, sParsingLayoutsInterval + d->scheduledLayoutCounter*sParsingLayoutsIncrement);
    }
    d->layoutTimerId = startTimer( time );
}

void KHTMLView::unscheduleRelayout()
{
    if (!d->layoutTimerId)
        return;

    killTimer(d->layoutTimerId);
    d->layoutTimerId = 0;
}

void KHTMLView::unscheduleRepaint()
{
    if (!d->repaintTimerId)
        return;

    killTimer(d->repaintTimerId);
    d->repaintTimerId = 0;
}

void KHTMLView::scheduleRepaint(int x, int y, int w, int h, bool asap)
{
    bool parsing = !m_part->xmlDocImpl() || m_part->xmlDocImpl()->parsing();

//     qDebug() << "parsing " << parsing;
//     qDebug() << "complete " << d->complete;

    int time = parsing && !d->firstLayoutPending ? 150 : (!asap ? ( !d->complete ? 80 : 20 ) : 0);

#ifdef DEBUG_FLICKER
    QPainter p;
    p.begin( viewport() );

    int vx, vy;
    contentsToViewport( x, y, vx, vy );
    p.fillRect( vx, vy, w, h, Qt::red );
    p.end();
#endif

    d->updateRegion = d->updateRegion.unite(QRect(x,y,w,h));

    if (asap && !parsing)
        unscheduleRepaint();

    if ( !d->repaintTimerId )
        d->repaintTimerId = startTimer( time );

//     qDebug() << "starting timer " << time;
}

void KHTMLView::complete( bool pendingAction )
{
//     qDebug() << "KHTMLView::complete()";

    d->complete = true;

    // is there a relayout pending?
    if (d->layoutTimerId)
    {
//         qDebug() << "requesting relayout now";
        // do it now
        killTimer(d->layoutTimerId);
        d->layoutTimerId = startTimer( 0 );
        d->emitCompletedAfterRepaint = pendingAction ?
            KHTMLViewPrivate::CSActionPending : KHTMLViewPrivate::CSFull;
    }

    // is there a repaint pending?
    if (d->repaintTimerId)
    {
//         qDebug() << "requesting repaint now";
        // do it now
        killTimer(d->repaintTimerId);
        d->repaintTimerId = startTimer( 0 );
        d->emitCompletedAfterRepaint = pendingAction ?
            KHTMLViewPrivate::CSActionPending : KHTMLViewPrivate::CSFull;
    }

    if (!d->emitCompletedAfterRepaint)
    {
        if (!pendingAction)
	    emit m_part->completed();
        else
            emit m_part->completed(true);
    }

}

void KHTMLView::updateScrollBars()
{
    const QWidget *view = widget();
    if (!view)
        return;

    QSize p = viewport()->size();
    QSize m = maximumViewportSize();

    if (m.expandedTo(view->size()) == m)
        p = m; // no scroll bars needed

    QSize v = view->size();
    horizontalScrollBar()->setRange(0, v.width() - p.width());
    horizontalScrollBar()->setPageStep(p.width());
    verticalScrollBar()->setRange(0, v.height() - p.height());
    verticalScrollBar()->setPageStep(p.height());
    if (!d->smoothScrolling) {
        d->updateContentsXY();
    }
}

void KHTMLView::slotMouseScrollTimer()
{
     horizontalScrollBar()->setValue( horizontalScrollBar()->value() +d->m_mouseScroll_byX );
     verticalScrollBar()->setValue( verticalScrollBar()->value() +d->m_mouseScroll_byY);
}


static DOM::Position positionOfLineBoundary(const DOM::Position &pos, bool toEnd)
{
    Selection sel = pos;
    sel.expandUsingGranularity(Selection::LINE);
    return toEnd ? sel.end() : sel.start();
}

inline static DOM::Position positionOfLineBegin(const DOM::Position &pos)
{
    return positionOfLineBoundary(pos, false);
}

inline static DOM::Position positionOfLineEnd(const DOM::Position &pos)
{
    return positionOfLineBoundary(pos, true);
}

bool KHTMLView::caretKeyPressEvent(QKeyEvent *_ke)
{
  EditorContext *ec = &m_part->d->editor_context;
  Selection &caret = ec->m_selection;
  Position old_pos = caret.caretPos();
  Position pos = old_pos;
  bool recalcXPos = true;
  bool handled = true;

  bool ctrl = _ke->modifiers() & Qt::ControlModifier;
  bool shift = _ke->modifiers() & Qt::ShiftModifier;

  switch(_ke->key()) {

    // -- Navigational keys
    case Qt::Key_Down:
      pos = old_pos.nextLinePosition(caret.xPosForVerticalArrowNavigation(Selection::EXTENT));
      recalcXPos = false;
      break;

    case Qt::Key_Up:
      pos = old_pos.previousLinePosition(caret.xPosForVerticalArrowNavigation(Selection::EXTENT));
      recalcXPos = false;
      break;

    case Qt::Key_Left:
      pos = ctrl ? old_pos.previousWordPosition() : old_pos.previousCharacterPosition();
      break;

    case Qt::Key_Right:
      pos = ctrl ? old_pos.nextWordPosition() : old_pos.nextCharacterPosition();
      break;

    case Qt::Key_PageDown:
//       moveCaretNextPage(); ###
      break;

    case Qt::Key_PageUp:
//       moveCaretPrevPage(); ###
      break;

    case Qt::Key_Home:
      if (ctrl)
        /*moveCaretToDocumentBoundary(false)*/; // ###
      else
        pos = positionOfLineBegin(old_pos);
      break;

    case Qt::Key_End:
      if (ctrl)
        /*moveCaretToDocumentBoundary(true)*/; // ###
      else
        pos = positionOfLineEnd(old_pos);
      break;

    default:
      handled = false;

  }/*end switch*/

  if (pos != old_pos) {
    m_part->clearCaretRectIfNeeded();

    caret.moveTo(shift ? caret.nonCaretPos() : pos, pos);
    int old_x = caret.xPosForVerticalArrowNavigation(Selection::CARETPOS);

    m_part->selectionLayoutChanged();

    // restore old x-position to prevent recalculation
    if (!recalcXPos)
      m_part->d->editor_context.m_xPosForVerticalArrowNavigation = old_x;

    m_part->emitCaretPositionChanged(pos);
    // ### check when to emit it
    m_part->notifySelectionChanged();

  }

  if (handled) _ke->accept();
  return handled;
}

#undef DEBUG_CARETMODE
