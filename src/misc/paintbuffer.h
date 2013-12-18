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

// The PaintBuffer class provides a shared buffer for efficient repetitive painting.
//
// It will grow to encompass the biggest size encountered, in order to avoid
// constantly resizing.
// When it grows over maxPixelBuffering, it periodically checks if such a size
// is still needed.  If not, it shrinks down to the biggest size < maxPixelBuffering
// that was requested during the overflow lapse.

#ifndef html_paintbuffer_h
#define html_paintbuffer_h

#include <QPixmap>
#include <QStack>
#include <QPainter>
#include <QPaintEngine>
#include <QDebug>
#include <assert.h>

namespace khtml {

class BufferedPainter;
class BufferSweeper;

class PaintBuffer: public QObject
{
   friend class khtml::BufferSweeper;

   Q_OBJECT

public:
    static const int maxPixelBuffering;
    static const int leaseTime;
    static const int cleanupTime;
    static const int maxBuffers;
    
    static QPixmap* grab( QSize s = QSize() );
    static void release(QPixmap*p);
    
    static void cleanup();
    
    PaintBuffer();
    void reset();
    virtual void timerEvent(QTimerEvent* e);
    
private:
    QPixmap* getBuf( QSize S );

    static QStack<PaintBuffer*> *s_avail;
    static QStack<PaintBuffer*> *s_grabbed;
    static QStack<QPixmap*>     *s_full;
    static BufferSweeper        *s_sweeper;

    QPixmap m_buf;
    bool m_overflow;
    bool m_grabbed;
    bool m_renewTimer;
    int m_timer;
    int m_resetWidth;
    int m_resetHeight;
};

class BufferedPainter
{
public: 
    BufferedPainter(QPixmap* px, QPainter*& p, const QRegion &rr, bool replacePainter) {
         QRect br = rr.boundingRect();
         m_rect = br;
         bool doFill = 1 || !px->hasAlphaChannel(); // shared pixmaps aren't properly cleared
                                                    // with Qt 4 + NVidia proprietary driver.
                                                    // So we can't use this optimization until
                                                    // we can detect this defect. 
         if (doFill)
             px->fill(Qt::transparent);
         m_painter.begin(px);
         
         m_off = br.topLeft() + QPoint( static_cast<int>(p->worldTransform().dx()),
                                        static_cast<int>(p->worldTransform().dy()));
         m_painter.setWorldTransform(p->worldTransform());
         m_painter.translate( -m_off );
         m_painter.setClipRegion(m_region = rr);
         //foreach(QRect rrr, m_region.rects())
         //    qDebug() << rrr;

         if (!doFill) {
             m_painter.setCompositionMode( QPainter::CompositionMode_Source );
             m_painter.fillRect(br, Qt::transparent);
         }

         m_painter.setCompositionMode(p->compositionMode());
         m_buf = px;

         m_painter.setFont( p->font() );
         m_painter.setBrush( p->brush() );
         m_painter.setPen( p->pen() );
         m_painter.setBackground( p->background() );
         m_painter.setRenderHints( p->renderHints() );
         // ### what else?

         m_origPainter = p;
         if (replacePainter)
             p = &m_painter;
         m_paused = false;
    }
    
    void transfer( float opacity ) {
        // ### when using DestinationIn with an alpha above 0.99, the resulting buffer ends up black in Qt 4.5.1
        bool constantOpacity = (opacity > 0.99) || (m_origPainter->paintEngine() && m_origPainter->paintEngine()->hasFeature(QPaintEngine::ConstantOpacity));
        if (!constantOpacity) {
            QColor color;
            color.setAlphaF(opacity);
            m_painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            m_painter.fillRect(m_rect, color);
        }
        m_painter.end();
        QTransform t = m_origPainter->worldTransform();
        QPoint trans(static_cast<int>(t.dx()),static_cast<int>(t.dy()));
        m_origPainter->save();
        m_origPainter->resetTransform();
        m_origPainter->setClipRegion(trans.isNull() ? m_region : m_region.translated(trans));
        m_origPainter->setWorldTransform(t);
        if (constantOpacity)
            m_origPainter->setOpacity(opacity);
        m_origPainter->drawPixmap(m_off-trans, *m_buf, QRect(0,0,m_rect.width(),m_rect.height()));
        m_origPainter->restore();   
    }
    
    static BufferedPainter* start(QPainter*&, const QRegion&);
    static void end(QPainter*&, BufferedPainter* bp, float opacity=1.0);
    
    const QPainter* painter() const { return &m_painter; }
    QPainter* originalPainter() const { return m_origPainter; }
    QPixmap* buffer() const { return m_buf; }
private:
    bool m_paused;
    QRect m_rect;
    QRegion m_region;
    QPoint m_off;
    QPainter m_painter;
    QPixmap* m_buf;
    QPainter* m_origPainter;
};

}
#endif
