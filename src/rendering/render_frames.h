/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
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
#ifndef __render_frames_h__
#define __render_frames_h__

#include "rendering/render_replaced.h"
#include "xml/dom_nodeimpl.h"
#include "html/html_baseimpl.h"

namespace DOM
{
class HTMLFrameElementImpl;
class HTMLElementImpl;
class MouseEventImpl;
}

namespace khtml
{
class ChildFrame;

class RenderFrameSet : public RenderBox
{
    friend class DOM::HTMLFrameSetElementImpl;
public:
    RenderFrameSet(DOM::HTMLFrameSetElementImpl *frameSet);

    virtual ~RenderFrameSet();

    const char *renderName() const override
    {
        return "RenderFrameSet";
    }
    bool isFrameSet() const override
    {
        return true;
    }

    void layout() override;

    void positionFrames();
    void paintFrameSetRules(QPainter *paint, const QRect &damageRect);

    bool resizing() const
    {
        return m_resizing;
    }
    bool noResize() const
    {
        return element()->noResize();
    }

    bool userResize(DOM::MouseEventImpl *evt);
    bool canResize(int _x, int _y);
    void setResizing(bool e);

    Qt::CursorShape cursorShape() const
    {
        return m_cursor;
    }

    bool nodeAtPoint(NodeInfo &info, int x, int y, int tx, int ty, HitTestAction hitTestAction, bool inside) override;

    DOM::HTMLFrameSetElementImpl *element() const
    {
        return static_cast<DOM::HTMLFrameSetElementImpl *>(RenderObject::element());
    }

#ifdef ENABLE_DUMP
    void dump(QTextStream &stream, const QString &ind) const override;
#endif

private:
    Qt::CursorShape m_cursor;
    int m_oldpos;
    int m_gridLen[2];
    int *m_gridDelta[2];
    int *m_gridLayout[2];

    bool *m_hSplitVar; // is this split variable?
    bool *m_vSplitVar;

    int m_hSplit;     // the split currently resized
    int m_vSplit;
    int m_hSplitPos;
    int m_vSplitPos;

    bool m_resizing;
    bool m_paint;
    bool m_clientresizing;
};

class RenderPart : public khtml::RenderWidget
{
    Q_OBJECT
public:
    RenderPart(DOM::HTMLElementImpl *node);

    const char *renderName() const override
    {
        return "RenderPart";
    }

    virtual void setWidget(QWidget *widget);

    short intrinsicWidth() const override;
    int intrinsicHeight() const override;

public Q_SLOTS:
    virtual void slotViewCleared();
};

class RenderFrame : public khtml::RenderPart
{
    Q_OBJECT
public:
    RenderFrame(DOM::HTMLFrameElementImpl *frame);

    const char *renderName() const override
    {
        return "RenderFrame";
    }
    bool isFrame() const override
    {
        return true;
    }

    // frames never have padding
    int paddingTop() const override
    {
        return 0;
    }
    int paddingBottom() const override
    {
        return 0;
    }
    int paddingLeft() const override
    {
        return 0;
    }
    int paddingRight() const override
    {
        return 0;
    }

    DOM::HTMLFrameElementImpl *element() const
    {
        return static_cast<DOM::HTMLFrameElementImpl *>(RenderObject::element());
    }

public Q_SLOTS:
    void slotViewCleared() override;
};

// I can hardly call the class RenderObject ;-)
class RenderPartObject : public khtml::RenderPart
{
    Q_OBJECT
public:
    RenderPartObject(DOM::HTMLElementImpl *);

    const char *renderName() const override
    {
        return "RenderPartObject";
    }

    void layout() override;

    bool canHaveBorder() const override
    {
        return true;
    }

public Q_SLOTS:
    void slotViewCleared() override;
};

}

#endif
