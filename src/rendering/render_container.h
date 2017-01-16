/*
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2001 Antti Koivisto (koivisto@kde.org)
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
#ifndef render_container_h
#define render_container_h

#include "render_object.h"

namespace khtml
{

/**
 * Base class for rendering objects that can have children
 */
class RenderContainer : public RenderObject
{
public:
    RenderContainer(DOM::NodeImpl *node);

    RenderObject *firstChild() const Q_DECL_OVERRIDE
    {
        return m_first;
    }
    RenderObject *lastChild() const Q_DECL_OVERRIDE
    {
        return m_last;
    }

    bool childAllowed() const Q_DECL_OVERRIDE
    {
        // Prevent normal children when we are replaced by generated content
        if (style()) {
            return style()->useNormalContent();
        }
        return true;
    }

    void addChild(RenderObject *newChild, RenderObject *beforeChild = nullptr) Q_DECL_OVERRIDE;

    RenderObject *removeChildNode(RenderObject *child) Q_DECL_OVERRIDE;
    void appendChildNode(RenderObject *child) Q_DECL_OVERRIDE;
    void insertChildNode(RenderObject *child, RenderObject *before) Q_DECL_OVERRIDE;

    void layout() Q_DECL_OVERRIDE;
    void calcMinMaxWidth() Q_DECL_OVERRIDE
    {
        setMinMaxKnown(true);
    }

    void removeSuperfluousAnonymousBlockChild(RenderObject *child) Q_DECL_OVERRIDE;

    void setStyle(RenderStyle *_style) Q_DECL_OVERRIDE;

    RenderPosition positionForCoordinates(int x, int y) Q_DECL_OVERRIDE;

protected:
    // Generate CSS content
    void createGeneratedContent();
    void updateReplacedContent();

    void updatePseudoChildren();
    void updatePseudoChild(RenderStyle::PseudoId type);

    RenderContainer *pseudoContainer(RenderStyle::PseudoId type) const;
    void addPseudoContainer(RenderObject *child);
private:

    void setFirstChild(RenderObject *first)
    {
        m_first = first;
    }
    void setLastChild(RenderObject *last)
    {
        m_last = last;
    }

protected:

    RenderObject *m_first;
    RenderObject *m_last;
};

}
#endif
