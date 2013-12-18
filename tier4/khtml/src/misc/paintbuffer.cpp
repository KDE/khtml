/*
 * This file is part of the KDE libraries
 *
 * Copyright (C) 2007 Germain Garand <germain@ebooksfrance.org>
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

#include "paintbuffer.h"
#include <QPixmap>
#include <QTimerEvent>

using namespace khtml;

const int PaintBuffer::maxPixelBuffering = 200*200;
const int PaintBuffer::leaseTime = 2*1000;
const int PaintBuffer::cleanupTime = 10*1000;
const int PaintBuffer::maxBuffers = 10;

namespace khtml {

class BufferSweeper: public QObject
{
public:
    BufferSweeper(): QObject() { m_timer = 0; m_reset = false; }
    
    void timerEvent(QTimerEvent* e) {
        assert( m_timer == e->timerId() );
        Q_UNUSED(e);
        if (m_reset) {
            m_reset = false;
            return;
        }
        if (PaintBuffer::s_avail) {
            while (PaintBuffer::s_avail->count()>1)
                delete PaintBuffer::s_avail->pop();
            if (PaintBuffer::s_avail->count())
                PaintBuffer::s_avail->top()->reset();
        }
        if (!PaintBuffer::s_grabbed)
            stop();
    }
    void start() {
        if (m_timer) return;
        m_timer = startTimer( PaintBuffer::cleanupTime );
    }
    void stop() {
        if (m_timer)
            killTimer( m_timer );
        m_timer = 0;
    }
    void restart() {
        stop();
        start();
    }
    void reset() {
        m_reset = true;
    }
    bool stopped() const { return !m_timer; }
    int m_timer;
    bool m_reset;
};

}

PaintBuffer::PaintBuffer()
:   m_overflow(false), 
    m_grabbed(false),
    m_renewTimer(false),
    m_timer(0),
    m_resetWidth(0), 
    m_resetHeight(0)
{

}

// static
void PaintBuffer::cleanup()
{
    if (s_avail) {
        qDeleteAll( *s_avail );
        delete s_avail;
        s_avail = 0;
    }
    if (s_grabbed) {
        qDeleteAll( *s_grabbed );
        delete s_grabbed;
        s_grabbed = 0;
    }
    if (s_full) {
        qDeleteAll( *s_full );
        delete s_full;
        s_full = 0;
    }
    if (s_sweeper) {
        s_sweeper->deleteLater();
        s_sweeper = 0;
    }
}

// static
QPixmap *PaintBuffer::grab( QSize s ) 
{
    if (!s_avail) {
        s_avail = new QStack<PaintBuffer*>;
        s_grabbed = new QStack<PaintBuffer*>;
        s_sweeper = new BufferSweeper;
    }

    if (s_sweeper->stopped())
        s_sweeper->start();
    else
        s_sweeper->reset();

    if ( s_grabbed->count()+s_avail->count() >= maxBuffers ) {
        if (!s_full)
            s_full = new QStack<QPixmap*>;
        s_full->push( new QPixmap(s.width(), s.height()) );
        return s_full->top();
    }

    s_grabbed->push( s_avail->count() ? s_avail->pop() : new PaintBuffer );
    QPixmap *ret = s_grabbed->top()->getBuf( s );
    //qDebug() << "requested size:" << s << "real size:" << ret->size();
    return ret;
}

// static
void PaintBuffer::release( QPixmap *px )
{
    if (s_full && s_full->count()) {
        assert( px == s_full->top() );
        Q_UNUSED(px);
        delete s_full->top();
        s_full->pop();
        return;
    }
    assert(px == &s_grabbed->top()->m_buf);
    s_grabbed->top()->m_grabbed = false;
    s_avail->push( s_grabbed->pop() );
}

void PaintBuffer::timerEvent(QTimerEvent* e)
{
    assert( m_timer == e->timerId() );
    Q_UNUSED(e);
    if (m_grabbed)
        return;
    if (m_renewTimer) {
        m_renewTimer = false;
        return;
    }
    m_buf = QPixmap(m_resetWidth, m_resetHeight);
    m_resetWidth = m_resetHeight = 0;
    m_overflow = false;
    killTimer( m_timer );
    m_timer = 0;
}

void PaintBuffer::reset()
{
    if (m_grabbed|m_renewTimer)
        return;
    m_resetWidth = m_resetHeight = 0;
    m_buf = QPixmap();
    m_overflow = false;
    if( m_timer )
      killTimer( m_timer );
    m_timer = 0;
}

QPixmap *PaintBuffer::getBuf( QSize s )
{
    assert( !m_grabbed );
    if (s.isEmpty())
        return 0;

    m_grabbed = true;
    bool cur_overflow = false;
    int nw = qMax(m_buf.width(), s.width());
    int nh = qMax(m_buf.height(), s.height());

    if (!m_overflow && (nw*nh > maxPixelBuffering))
        cur_overflow = true;

    if (nw != m_buf.width() || nh != m_buf.height())
        m_buf = QPixmap(nw, nh);

    if (cur_overflow) {
        m_overflow = true;
        m_timer = startTimer( leaseTime );
    } else if (m_overflow) {
        int numPx = s.width()*s.height();
        if( numPx > maxPixelBuffering ) {
            m_renewTimer = true;
        } else if (numPx > m_resetWidth*m_resetHeight) {
             m_resetWidth = s.width();
             m_resetHeight = s.height();
        }
    }
    return &m_buf;
}

QStack<PaintBuffer*> *PaintBuffer::s_avail = 0;
QStack<PaintBuffer*> *PaintBuffer::s_grabbed = 0;
QStack<QPixmap*> *    PaintBuffer::s_full = 0;
BufferSweeper*        PaintBuffer::s_sweeper = 0;

// ### benchark me in release mode
#define USE_PIXMAP_CACHE

// static
BufferedPainter* BufferedPainter::start(QPainter*&p, const QRegion&rr)
{
    if (rr.isEmpty())
        return 0;
#ifdef USE_PIXMAP_CACHE
    QPixmap *pm = PaintBuffer::grab(rr.boundingRect().size());
#else
    QPixmap *pm =  new QPixmap( rr.boundingRect().size() );
#endif
    if (!pm || pm->isNull())
        return 0;
    return new BufferedPainter(pm, p, rr, true /*replacePainter*/);
}

// static
void BufferedPainter::end(QPainter*&p, BufferedPainter* bp, float opacity)
{
    bp->transfer( opacity );
    p = bp->originalPainter();
#ifdef USE_PIXMAP_CACHE
    PaintBuffer::release( bp->buffer() );
#else
    delete bp->buffer();
#endif
    delete bp;
}
