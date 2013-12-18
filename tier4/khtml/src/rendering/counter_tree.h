/*
 * This file is part of the HTML rendering engine for KDE.
 *
 * Copyright (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
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
#ifndef _Counter_Tree_h_
#define _Counter_Tree_h_

#include "misc/shared.h"
#include "rendering/render_object.h"

namespace khtml {

class CounterReset;

// This file implements a counter-tree that is used for finding all parents in counters() lookup,
// and for propagating count-changes when nodes are added or removed.
// Please note that only counter-reset and root can be parents here, and that render-tree parents
// are just counter-tree siblings

// Implementation of counter-increment and counter-content
class CounterNode
{
public:
    CounterNode(RenderObject *o);
    virtual ~CounterNode();

    CounterReset* parent() const { return m_parent; }
    CounterNode* previousSibling() const { return m_previous; }
    CounterNode* nextSibling() const { return m_next; }
    virtual CounterNode* firstChild() const { return 0; } 
    virtual CounterNode* lastChild() const { return 0; }
    virtual void insertAfter ( CounterNode *newChild, CounterNode *refChild );
    virtual void removeChild ( CounterNode *oldChild );
    // Convenient self-referring version of the above
    void remove();

    int value() const { return m_value; }
    void setValue(short v) { m_value = v; }
    int count() const { return m_count; }

    virtual bool isReset() { return false; }
    virtual void recount( bool first = false );
    virtual void setSelfDirty();
    virtual void setParentDirty();

    bool hasCounters() const { return m_hasCounters; }
    bool isVisual() const { return m_isVisual; }
    void setHasCounters();
    void setIsVisual() { m_isVisual = true; }
    bool isRoot() { return m_renderer && m_renderer->isRoot(); }

    void setRenderer(RenderObject *o) { m_renderer = o; }
    RenderObject* renderer() const { return m_renderer; }

    friend class CounterReset;
protected:
    bool m_hasCounters : 1;
    bool m_isVisual : 1;
    short m_value;
    short m_count;
    CounterReset *m_parent;
    CounterNode *m_previous;
    CounterNode *m_next;
    RenderObject *m_renderer;
};

// Implementation of counter-reset and root
class CounterReset : public CounterNode
{
public:
    CounterReset(RenderObject *o);
    virtual ~CounterReset();

    virtual CounterNode *firstChild() const { return m_first; }
    virtual CounterNode *lastChild() const { return m_last; }
    virtual void insertAfter ( CounterNode *newChild, CounterNode *refChild );
    virtual void removeChild ( CounterNode *oldChild );

    virtual bool isReset() { return true; }
    virtual void recount( bool first = false );
    virtual void setSelfDirty();
    virtual void setParentDirty();

    void updateTotal(int value);
    // The highest value among children
    int total() const { return m_total; }

protected:
    int m_total;
    CounterNode *m_first;
    CounterNode *m_last;
};

} // namespace

#endif

