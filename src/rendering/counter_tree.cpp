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

#include "rendering/counter_tree.h"
#include "rendering/render_object.h"

namespace khtml {

CounterNode::CounterNode(RenderObject *o)
    : m_hasCounters(false), m_isVisual(false),
      m_value(0), m_count(0), m_parent(0), m_previous(0), m_next(0),
      m_renderer(o) {}

CounterNode::~CounterNode()
{
    if (m_parent) m_parent->removeChild(this);
}

void CounterNode::insertAfter ( CounterNode *, CounterNode *)
{
    Q_ASSERT( false);
}

void CounterNode::removeChild ( CounterNode *)
{
    Q_ASSERT( false);
}

void CounterNode::remove ()
{
    if (m_parent) m_parent->removeChild(this);
    else {
        Q_ASSERT(isReset());
        // abandon our children
        CounterNode *n = firstChild();
        for(; n; n = n->m_next)
            n->m_parent = 0;
    }
}

void CounterNode::setHasCounters ()
{
    m_hasCounters = true;
    if (parent())
        parent()->setHasCounters();
}

void CounterNode::recount (bool first)
{
    int old_count = m_count;
    if (m_previous)
        m_count = m_previous->count() + m_value;
    else {
        assert(m_parent->firstChild() == this);
        m_count = m_parent->value() + m_value;
    }
    if (old_count != m_count && !first)
        setSelfDirty();
    if (old_count != m_count || first) {
        if (m_parent) m_parent->updateTotal(m_count);
        if (m_next) m_next->recount();
    }
}

void CounterNode::setSelfDirty ()
{
    if (m_renderer && m_isVisual)
        m_renderer->setNeedsLayoutAndMinMaxRecalc();
}

void CounterNode::setParentDirty ()
{
    if (m_renderer && m_isVisual && m_hasCounters)
        m_renderer->setNeedsLayoutAndMinMaxRecalc();
}

CounterReset::CounterReset(RenderObject *o) : CounterNode(o), m_total(0), m_first(0), m_last(0) {}
CounterReset::~CounterReset() {}

void CounterReset::insertAfter ( CounterNode *newChild, CounterNode *refChild )
{
    Q_ASSERT( newChild );
    Q_ASSERT( !refChild || refChild->parent() == this );

    newChild->m_parent = this;
    newChild->m_previous = refChild;

    if (refChild) {
        newChild->m_next = refChild->m_next;
        refChild->m_next = newChild;
    } else {
        newChild->m_next = m_first;
        m_first = newChild;
    }

    if (newChild->m_next) {
        assert(newChild->m_next->m_previous == refChild);
        newChild->m_next->m_previous = newChild;
    }
    else {
        assert (m_last == refChild);
        m_last = newChild;
    }

    newChild->recount(true);
}

void CounterReset::removeChild ( CounterNode *oldChild )
{
    Q_ASSERT( oldChild );

    CounterNode* next = oldChild->m_next;
    CounterNode* prev = oldChild->m_previous;

    if (oldChild->firstChild()) {
        CounterNode* first = oldChild->firstChild();
        CounterNode* last = oldChild->lastChild();
        if (prev) {
            prev->m_next = first;
            first->m_previous = prev;
        }
        else {
            assert ( m_first == oldChild );
            m_first = first;
         }

        if (next) {
            next->m_previous = last;
            last->m_next = next;
        }
        else {
            assert ( m_last == oldChild );
            m_last = last;
        }

        next = first;
        while (next) {
            next->m_parent = this;
            if (next == last) break;
            next = next->m_next;
        }

        first->recount(true);
    }
    else {
        if (prev) prev->m_next = next;
        else {
            assert ( m_first == oldChild );
            m_first = next;
        }
        if (next) next->m_previous = prev;
        else {
            assert ( m_last == oldChild );
            m_last = prev;
        }
        if (next)
            next->recount();
    }


    oldChild->m_next = 0;
    oldChild->m_previous = 0;
    oldChild->m_parent = 0;
}

void CounterReset::recount (bool first)
{
    int old_count = m_count;
    if (m_previous)
        m_count = m_previous->count();
    else if (m_parent)
        m_count = m_parent->value();
    else
        m_count = 0;

    updateTotal(m_value);
    if (!first) setSelfDirty();
    if (first || m_count != old_count) {
        if (m_next) m_next->recount();
    }
}

void CounterReset::setSelfDirty ()
{
    setParentDirty();
}

void CounterReset::setParentDirty ()
{
    if (hasCounters()) {
        if (m_renderer && m_isVisual) m_renderer->setNeedsLayoutAndMinMaxRecalc();
        CounterNode* n = firstChild();
        for(; n; n = n->nextSibling())
        {
            n->setParentDirty();
        }
    }
}

void CounterReset::updateTotal (int value)
{
    if (value > m_total) m_total = value;
}

} // namespace
