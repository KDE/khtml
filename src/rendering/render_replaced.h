/*
 * This file is part of the HTML widget for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2006 Germain Garand (germain@ebooksfrance.org)
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
#ifndef render_replaced_h
#define render_replaced_h

#include "rendering/render_block.h"
#include <QtCore/QObject>
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QPointer>

class KHTMLView;
class QWidget;

namespace khtml
{

class RenderReplaced : public RenderBox
{
public:
    RenderReplaced(DOM::NodeImpl *node);

    const char *renderName() const override
    {
        return "RenderReplaced";
    }
    bool isRenderReplaced() const override
    {
        return true;
    }

    bool childAllowed() const override
    {
        return false;
    }

    void calcMinMaxWidth() override;

    short intrinsicWidth() const override
    {
        return m_intrinsicWidth;
    }
    int intrinsicHeight() const override
    {
        return m_intrinsicHeight;
    }

    void setIntrinsicWidth(int w)
    {
        m_intrinsicWidth = w;
    }
    void setIntrinsicHeight(int h)
    {
        m_intrinsicHeight = h;
    }

    // Return before, after (offset set to max), or inside the replaced element,
    // at @p offset
    virtual FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty,
            DOM::NodeImpl *&node, int &offset,
            SelPointState &);

    long caretMinOffset() const override;
    long caretMaxOffset() const override;
    unsigned long caretMaxRenderedOffset() const override;
    RenderPosition positionForCoordinates(int x, int y) override;
    virtual bool forceTransparentText() const
    {
        return false;
    }

protected:
    short m_intrinsicWidth;
    short m_intrinsicHeight;
};

class RenderWidget : public QObject, public RenderReplaced, public khtml::Shared<RenderWidget>
{
    Q_OBJECT
public:
    RenderWidget(DOM::NodeImpl *node);
    virtual ~RenderWidget();

    void setStyle(RenderStyle *style) override;
    void paint(PaintInfo &i, int tx, int ty) override;
    bool isWidget() const override
    {
        return true;
    }

    virtual bool isFrame() const
    {
        return false;
    }

    void detach() override;
    void layout() override;

    void updateFromElement() override;
    virtual void handleFocusOut() {}

    QWidget *widget() const
    {
        return m_widget;
    }
    KHTMLView *view() const
    {
        return m_view;
    }

    void deref();

    bool needsMask() const
    {
        return m_needsMask;
    }

    static void paintWidget(PaintInfo &pI, QWidget *widget, int tx, int ty, QPixmap *buffer[] = nullptr);
    bool handleEvent(const DOM::EventImpl &ev) override;
    bool isRedirectedWidget() const;
    bool isDisabled() const
    {
        return m_widget && !m_widget->isEnabled();
    }

#ifdef ENABLE_DUMP
    void dump(QTextStream &stream, const QString &ind) const override;
#endif

public Q_SLOTS:
    void slotWidgetDestructed();

protected:
    // Should be called by subclasses to ensure we don't memory-manage this..
    void setDoesNotOwnWidget()
    {
        m_ownsWidget = false;
    }

    void paintBoxDecorations(PaintInfo &paintInfo, int _tx, int _ty) override;

    virtual bool canHaveBorder() const
    {
        return false;
    }
    virtual bool includesPadding() const
    {
        return false;
    }

    bool shouldPaintCSSBorders() const
    {
        // Don't paint borders if the border-style is native
        // or borders are not supported on this widget
        return shouldPaintBackgroundOrBorder() && canHaveBorder() &&
               (style()->borderLeftStyle() != BNATIVE ||
                style()->borderRightStyle()  != BNATIVE ||
                style()->borderTopStyle()    != BNATIVE ||
                style()->borderBottomStyle() != BNATIVE);
    }

    bool shouldDisableNativeBorders() const
    {
        return (shouldPaintCSSBorders() || (!shouldPaintBackgroundOrBorder() && canHaveBorder()));
    }

    virtual bool acceptsSyntheticEvents() const
    {
        return true;
    }

    bool event(QEvent *e) override;

    bool eventFilter(QObject * /*o*/, QEvent *e) override;
    void setQWidget(QWidget *widget);
    void resizeWidget(int w, int h);

    QWidget *m_widget;
    KHTMLView *m_view;
    QPointer<QWidget> m_underMouse;
    QPixmap *m_buffer[2];

    QFrame::Shape m_nativeFrameShape;

    //Because we mess with normal detach due to ref/deref,
    //we need to keep track of the arena ourselves
    //so it doesn't get yanked from us, etc.
    SharedPtr<RenderArena> m_arena;

    bool m_needsMask;
    bool m_ownsWidget;

public:
    int borderTop() const override
    {
        return canHaveBorder() ? RenderReplaced::borderTop() : 0;
    }
    int borderBottom() const override
    {
        return canHaveBorder() ? RenderReplaced::borderBottom() : 0;
    }
    int borderLeft() const override
    {
        return canHaveBorder() ? RenderReplaced::borderLeft() : 0;
    }
    int borderRight() const override
    {
        return canHaveBorder() ? RenderReplaced::borderRight() : 0;
    }

    class EventPropagator : public QWidget
    {
    public:
        void sendEvent(QEvent *e);
    };
};

class KHTMLWidgetPrivate
{
public:
    KHTMLWidgetPrivate(): m_rw(nullptr), m_redirected(false) {}
    QPoint absolutePos();
    KHTMLView *rootViewPos(QPoint &pos);
    RenderWidget *renderWidget() const
    {
        return m_rw;
    }
    void setRenderWidget(RenderWidget *rw)
    {
        m_rw = rw;
    }
    bool isRedirected() const
    {
        return m_redirected;
    }
    void setIsRedirected(bool b);
    void setPos(const QPoint &p)
    {
        m_pos = p;
    }
private:
    QPoint m_pos;
    RenderWidget *m_rw;
    bool m_redirected;
};

extern bool allowWidgetPaintEvents;

}

#endif
