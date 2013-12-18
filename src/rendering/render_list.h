/*
 * This file is part of the HTML rendering engine for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
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
#ifndef RENDER_LIST_H
#define RENDER_LIST_H

#include "rendering/render_block.h"

// ### list-style-position, list-style-image is still missing

namespace khtml
{

class RenderListItem;
class RenderListMarker;
class CounterNode;

// -----------------------------------------------------------------------------

class RenderListItem : public RenderBlock
{
    friend class RenderListMarker;
//    friend class CounterListItem;

public:
    RenderListItem(DOM::NodeImpl*);

    virtual const char *renderName() const { return "RenderListItem"; }

    virtual void setStyle(RenderStyle *style);

    virtual bool isListItem() const { return true; }

    void setValue( long v ) { predefVal = v; }

    virtual void layout( );
    virtual void detach( );
    virtual void calcMinMaxWidth();
    //virtual short marginLeft() const;
    //virtual short marginRight() const;

    void setInsideList(bool b ) { m_insideList = b; }

protected:

    void updateMarkerLocation();
    void resetListMarker() { m_marker = 0; }

    RenderListMarker *m_marker;
    CounterNode *m_counter;
    signed long predefVal : 30;
    bool m_insideList  : 1;
    bool m_deleteMarker: 1;
};

// -----------------------------------------------------------------------------

class RenderListMarker : public RenderBox
{
public:
    RenderListMarker(DOM::NodeImpl* node);
    ~RenderListMarker();

    virtual void setStyle(RenderStyle *style);

    virtual const char *renderName() const { return "RenderListMarker"; }
    // so the marker gets to layout itself. Only needed for
    // list-style-position: inside

    virtual void paint(PaintInfo& i, int xoff, int yoff);
    virtual void layout( );
    virtual void calcMinMaxWidth();

    virtual short lineHeight( bool firstLine ) const;
    virtual short baselinePosition( bool firstLine ) const;

    virtual void updatePixmap( const QRect&, CachedImage *);

    virtual void calcWidth();

    virtual bool isListMarker() const { return true; }

    virtual short markerWidth() const { return m_markerWidth; }

    RenderListItem* listItem() const { return m_listItem; }
    void setListItem(RenderListItem* listItem) { m_listItem = listItem; }

    bool listPositionInside() const
    { return !m_listItem->m_insideList || style()->listStylePosition() == INSIDE; }

protected:
    friend class RenderListItem;

    QString m_item;
    CachedImage *m_listImage;
    short m_markerWidth;
    RenderListItem* m_listItem;
};

// Implementation of list-item counter
// ### should replace most list-item specific code in renderObject::getCounter
/*
class CounterListItem : public CounterNode
{
public:
    int count() const;

    virtual void recount( bool first = false );
    virtual void setSelfDirty();

}; */

} //namespace

#endif
