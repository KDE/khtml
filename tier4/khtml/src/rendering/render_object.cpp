/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2004-2008 Apple Computer, Inc.
 *           (C) 2006 Germain Garand <germain@ebooksfrance.org>
 *           (C) 2008-2009 Fredrik Höglund <fredrik@kde.org>
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

#include "rendering/render_object.h"
#include "rendering/render_table.h"
#include "rendering/render_list.h"
#include "rendering/render_canvas.h"
#include "rendering/render_block.h"
#include "rendering/render_arena.h"
#include "rendering/render_layer.h"
#include "rendering/render_line.h"
#include "rendering/render_inline.h"
#include "rendering/render_text.h"
#include "rendering/render_replaced.h"
#include "rendering/render_generated.h"
#include "rendering/counter_tree.h"
#include "rendering/render_position.h"

#include "xml/dom_elementimpl.h"
#include "xml/dom_docimpl.h"
#include "dom/dom_doc.h"
#include "misc/loader.h"
#include "misc/borderarcstroker.h"

#include <QDebug>
#include <QPainter>
#include "khtmlview.h"
#include <khtml_part.h>
#include <QPaintEngine>

#include <assert.h>
using namespace DOM;
using namespace khtml;

#define RED_LUMINOSITY        30
#define GREEN_LUMINOSITY      59
#define BLUE_LUMINOSITY       11
#define INTENSITY_FACTOR      25
#define LIGHT_FACTOR          0
#define LUMINOSITY_FACTOR     75

#define MAX_COLOR             255
#define COLOR_DARK_THRESHOLD  51
#define COLOR_LIGHT_THRESHOLD 204

#define COLOR_LITE_BS_FACTOR 45
#define COLOR_LITE_TS_FACTOR 70

#define COLOR_DARK_BS_FACTOR 30
#define COLOR_DARK_TS_FACTOR 50

#define LIGHT_GRAY qRgb(192, 192, 192)
#define DARK_GRAY  qRgb(96, 96, 96)

#ifndef NDEBUG
static void *baseOfRenderObjectBeingDeleted;
#endif

QCache<quint64, QPixmap> *RenderObject::s_dashedLineCache = 0;

void RenderObject::cleanup()
{
    delete s_dashedLineCache;
    s_dashedLineCache = 0;
}

//#define MASK_DEBUG

void* RenderObject::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void RenderObject::operator delete(void* ptr, size_t sz)
{
    assert(baseOfRenderObjectBeingDeleted == ptr);

#ifdef KHTML_USE_ARENA_ALLOCATOR
    // Stash size where detach can find it.
    *(size_t *)ptr = sz;
#endif
}

RenderObject *RenderObject::createObject(DOM::NodeImpl* node,  RenderStyle* style)
{
    RenderObject *o = 0;
    khtml::RenderArena* arena = node->document()->renderArena();
    switch(style->display())
    {
    case NONE:
        break;
    case INLINE:
        o = new (arena) RenderInline(node);
        break;
    case BLOCK:
        o = new (arena) RenderBlock(node);
        break;
    case INLINE_BLOCK:
        o = new (arena) RenderBlock(node);
        break;
    case LIST_ITEM:
        o = new (arena) RenderListItem(node);
        break;
    case RUN_IN:
    case COMPACT:
        o = new (arena) RenderBlock(node);
        break;
    case TABLE:
    case INLINE_TABLE:
        style->setFlowAroundFloats(true);
        o = new (arena) RenderTable(node);
        break;
    case TABLE_ROW_GROUP:
    case TABLE_HEADER_GROUP:
    case TABLE_FOOTER_GROUP:
        o = new (arena) RenderTableSection(node);
        break;
    case TABLE_ROW:
        o = new (arena) RenderTableRow(node);
        break;
    case TABLE_COLUMN_GROUP:
    case TABLE_COLUMN:
        o = new (arena) RenderTableCol(node);
        break;
    case TABLE_CELL:
        o = new (arena) RenderTableCell(node);
        break;
    case TABLE_CAPTION:
        o = new (arena) RenderBlock(node);
        break;
    }
    return o;
}


RenderObject::RenderObject(DOM::NodeImpl* node)
    : CachedObjectClient(),
      m_style( 0 ),
      m_node( node ),
      m_parent( 0 ),
      m_previous( 0 ),
      m_next( 0 ),
      m_verticalPosition( PositionUndefined ),
      m_needsLayout( false ),
      m_normalChildNeedsLayout( false ),
      m_markedForRepaint( false ),
      m_posChildNeedsLayout( false ),
      m_minMaxKnown( false ),
      m_floating( false ),

      m_positioned( false ),
      m_relPositioned( false ),
      m_paintBackground( false ),

      m_isAnonymous( node->isDocumentNode() ),
      m_recalcMinMax( false ),
      m_isText( false ),
      m_inline( true ),
      m_attached( false ),

      m_replaced( false ),
      m_mouseInside( false ),
      m_hasFirstLine( false ),
      m_isSelectionBorder( false ),
      m_isRoot( false ),
      m_afterPageBreak( false ),
      m_needsPageClear( false ),
      m_containsPageBreak( false ),
      m_hasOverflowClip(false),
      m_inPosObjectList(false),
      m_doNotDelete(false)
{
  assert( node );
  if (node->document()->documentElement() == node) setIsRoot(true);
}

RenderObject::~RenderObject()
{
    const BackgroundLayer* bgLayer = m_style->backgroundLayers();
    while (bgLayer) {
        if(bgLayer->backgroundImage())
            bgLayer->backgroundImage()->deref(this);
        bgLayer = bgLayer->next();
    }

    m_style->deref();
}

RenderObject* RenderObject::objectBelow() const
{
    RenderObject* obj = firstChild();
    if ( !obj ) {
        obj = nextSibling();
        if ( !obj )
        {
            obj = parent();
            while (obj && !obj->nextSibling())
                obj = obj->parent();
            if (obj)
                obj = obj->nextSibling();
        }
    }
    return obj;
}

RenderObject* RenderObject::objectAbove() const
{
    RenderObject* obj = previousSibling();
    if ( !obj )
        return parent();

    RenderObject* last = obj->lastChild();
    while ( last )
    {
        obj = last;
        last = last->lastChild();
    }
    return obj;
}
/*
bool RenderObject::isRoot() const
{
    return !isAnonymous() &&
        element()->document()->documentElement() == element();
}*/

bool RenderObject::isHR() const
{
    return element() && element()->id() == ID_HR;
}
bool RenderObject::isWordBreak() const
{
    return element() && element()->id() == ID_WBR;
}
bool RenderObject::isHTMLMarquee() const
{
    return element() && element()->renderer() == this && element()->id() == ID_MARQUEE;
}

void RenderObject::addChild(RenderObject* , RenderObject *)
{
    KHTMLAssert(0);
}

RenderObject* RenderObject::removeChildNode(RenderObject*)
{
    KHTMLAssert(0);
    return 0;
}

void RenderObject::removeChild(RenderObject*)
{
    KHTMLAssert(0);
}

void RenderObject::appendChildNode(RenderObject*)
{
    KHTMLAssert(0);
}

void RenderObject::insertChildNode(RenderObject*, RenderObject*)
{
    KHTMLAssert(0);
}

RenderObject *RenderObject::nextRenderer() const
{
    if (firstChild())
        return firstChild();
    else if (nextSibling())
        return nextSibling();
    else {
        const RenderObject *r = this;
        while (r && !r->nextSibling())
            r = r->parent();
        if (r)
            return r->nextSibling();
    }
    return 0;
}

RenderObject *RenderObject::previousRenderer() const
{
    if (previousSibling()) {
        RenderObject *r = previousSibling();
        while (r->lastChild())
            r = r->lastChild();
        return r;
    }
    else if (parent()) {
        return parent();
    }
    else {
        return 0;
    }
}

bool RenderObject::isEditable() const
{
    RenderText *textRenderer = 0;
    if (isText()) {
        textRenderer = static_cast<RenderText *>(const_cast<RenderObject *>(this));
    }

    return style()->visibility() == VISIBLE &&
        element() && element()->isContentEditable() &&
        ((isBlockFlow() && !firstChild()) ||
        isReplaced() ||
        isBR() ||
        (textRenderer && textRenderer->firstTextBox()));
}

RenderObject *RenderObject::nextEditable() const
{
    RenderObject *r = const_cast<RenderObject *>(this);
    RenderObject *n = firstChild();
    if (n) {
        while (n) {
            r = n;
            n = n->firstChild();
        }
        if (r->isEditable())
            return r;
        else
            return r->nextEditable();
    }
    n = r->nextSibling();
    if (n) {
        r = n;
        while (n) {
            r = n;
            n = n->firstChild();
        }
        if (r->isEditable())
            return r;
        else
            return r->nextEditable();
    }
    n = r->parent();
    while (n) {
        r = n;
        n = r->nextSibling();
        if (n) {
            r = n;
            n = r->firstChild();
            while (n) {
                r = n;
                n = n->firstChild();
            }
            if (r->isEditable())
                return r;
            else
                return r->nextEditable();
        }
        n = r->parent();
    }
    return 0;
}

RenderObject *RenderObject::previousEditable() const
{
    RenderObject *r = const_cast<RenderObject *>(this);
    RenderObject *n = firstChild();
    if (n) {
        while (n) {
            r = n;
            n = n->lastChild();
        }
        if (r->isEditable())
            return r;
        else
            return r->previousEditable();
    }
    n = r->previousSibling();
    if (n) {
        r = n;
        while (n) {
            r = n;
            n = n->lastChild();
        }
        if (r->isEditable())
            return r;
        else
            return r->previousEditable();
    }
    n = r->parent();
    while (n) {
        r = n;
        n = r->previousSibling();
        if (n) {
            r = n;
            n = r->lastChild();
            while (n) {
                r = n;
                n = n->lastChild();
            }
            if (r->isEditable())
                return r;
            else
                return r->previousEditable();
        }
        n = r->parent();
    }
    return 0;
}

RenderObject *RenderObject::firstLeafChild() const
{
    RenderObject *r = firstChild();
    while (r) {
        RenderObject *n = 0;
        n = r->firstChild();
        if (!n)
            break;
        r = n;
    }
    return r;
}

RenderObject *RenderObject::lastLeafChild() const
{
    RenderObject *r = lastChild();
    while (r) {
        RenderObject *n = 0;
        n = r->lastChild();
        if (!n)
            break;
        r = n;
    }
    return r;
}

static void addLayers(RenderObject* obj, RenderLayer* parentLayer, RenderObject*& newObject,
                      RenderLayer*& beforeChild)
{
    if (obj->layer()) {
        if (!beforeChild && newObject) {
            // We need to figure out the layer that follows newObject.  We only do
            // this the first time we find a child layer, and then we update the
            // pointer values for newObject and beforeChild used by everyone else.
            beforeChild = newObject->parent()->findNextLayer(parentLayer, newObject);
            newObject = 0;
        }
        parentLayer->addChild(obj->layer(), beforeChild);
        return;
    }

    for (RenderObject* curr = obj->firstChild(); curr; curr = curr->nextSibling())
        addLayers(curr, parentLayer, newObject, beforeChild);
}

void RenderObject::addLayers(RenderLayer* parentLayer, RenderObject* newObject)
{
    if (!parentLayer)
        return;

    RenderObject* object = newObject;
    RenderLayer* beforeChild = 0;
    ::addLayers(this, parentLayer, object, beforeChild);
}

void RenderObject::removeLayers(RenderLayer* parentLayer)
{
    if (!parentLayer)
        return;

    if (layer()) {
        parentLayer->removeChild(layer());
        return;
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->removeLayers(parentLayer);
}

void RenderObject::moveLayers(RenderLayer* oldParent, RenderLayer* newParent)
{
    if (!newParent)
        return;

    if (layer()) {
        if (oldParent)
            oldParent->removeChild(layer());
        newParent->addChild(layer());
        return;
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->moveLayers(oldParent, newParent);
}

RenderLayer* RenderObject::findNextLayer(RenderLayer* parentLayer, RenderObject* startPoint,
                                         bool checkParent)
{
    // Error check the parent layer passed in.  If it's null, we can't find anything.
    if (!parentLayer)
        return 0;

    // Step 1: If our layer is a child of the desired parent, then return our layer.
    RenderLayer* ourLayer = layer();
    if (ourLayer && ourLayer->parent() == parentLayer)
        return ourLayer;

    // Step 2: If we don't have a layer, or our layer is the desired parent, then descend
    // into our siblings trying to find the next layer whose parent is the desired parent.
    if (!ourLayer || ourLayer == parentLayer) {
        for (RenderObject* curr = startPoint ? startPoint->nextSibling() : firstChild();
             curr; curr = curr->nextSibling()) {
            RenderLayer* nextLayer = curr->findNextLayer(parentLayer, 0, false);
            if (nextLayer)
                return nextLayer;
        }
    }

    // Step 3: If our layer is the desired parent layer, then we're finished.  We didn't
    // find anything.
    if (parentLayer == ourLayer)
        return 0;

    // Step 4: If |checkParent| is set, climb up to our parent and check its siblings that
    // follow us to see if we can locate a layer.
    if (checkParent && parent())
        return parent()->findNextLayer(parentLayer, this, true);

    return 0;
}

RenderLayer* RenderObject::enclosingLayer() const
{
    const RenderObject* curr = this;
    while (curr) {
        RenderLayer *layer = curr->layer();
        if (layer)
            return layer;
        curr = curr->parent();
    }
    return 0;
}

RenderLayer* RenderObject::enclosingStackingContext() const
{
    RenderLayer* l = enclosingLayer();
    while (l && !l->isStackingContext())
        l = l->parent();
    return l;
}

QRectF RenderObject::clientRectToViewport(const QRectF& rect)
{
    int offsetX = document()->part()->view()->contentsX();
    int offsetY = document()->part()->view()->contentsY();

    QRectF newRect(rect.x() - offsetX, rect.y() - offsetY,
                   rect.width(), rect.height());

    return newRect;
}

QList<QRectF> RenderObject::getClientRects()
{
    QList<QRectF> ret;

    int x = 0;
    int y = 0;
    absolutePosition(x, y);

    QRectF rect(x, y, width(), height());
    ret.append(clientRectToViewport(rect));

    return ret;
}

int RenderObject::offsetLeft() const
{
    if (isBody())
        return 0;

    int x, dummy;
    RenderObject* offsetPar = offsetParent();
    if (!offsetPar || offsetPar->isBody()) {
        if (style()->position() == PFIXED)
            return xPos();
        else {
            absolutePosition(x, dummy);
            return x;
        }
    }

    x = xPos() -offsetPar->borderLeft();
    if ( isPositioned() )
        return x;

    if (isRelPositioned()) {
        int y = 0;
        static_cast<const RenderBox*>(this)->relativePositionOffset(x, y);
    }

    for( RenderObject* curr = parent();
         curr && curr != offsetPar;
         curr = curr->parent() )
        x += curr->xPos();

     return x;
}

int RenderObject::offsetTop() const
{
    if (isBody())
        return 0;

    int y, dummy;
    RenderObject* offsetPar = offsetParent();
    if (!offsetPar || offsetPar->isBody()) {
        if (style()->position() == PFIXED)
            return yPos();
        else {
            absolutePosition(dummy, y);
            return y;
        }
    }

    y = yPos() -offsetPar->borderTop();
    if ( isPositioned() )
        return y;

     if (isRelPositioned()) {
        int x = 0;
        static_cast<const RenderBox*>(this)->relativePositionOffset(x, y);
    }
    for( RenderObject* curr = parent();
         curr && curr != offsetPar;
         curr = curr->parent() )
        y += curr->yPos();

     return y;
}

RenderObject* RenderObject::offsetParent() const
{
    if (isBody() || style()->position() == PFIXED)
        return 0;

    // can't really use containing blocks here (#113280)
    bool skipTables = isPositioned() || isRelPositioned();
    bool strict = !style()->htmlHacks();
    RenderObject* curr = parent();
    while (curr && (!curr->element() ||
                    (!curr->isPositioned() && !curr->isRelPositioned() &&
                        !(strict && skipTables ? curr->isRoot() : curr->isBody())))) {
        if (!skipTables && curr->element() && (curr->isTableCell() || curr->isTable()))
            break;
        curr = curr->parent();
    }
    return curr;
}

// IE extensions.
// clientWidth and clientHeight represent the interior of an object
short RenderObject::clientWidth() const
{
    return width() - borderLeft() - borderRight() -
        (layer() ? layer()->verticalScrollbarWidth() : 0);
}

int RenderObject::clientLeft() const
{
    return borderLeft();
}

int RenderObject::clientTop() const
{
    return borderTop();
}

int RenderObject::clientHeight() const
{
    return height() - borderTop() - borderBottom() -
      (layer() ? layer()->horizontalScrollbarHeight() : 0);
}

// scrollWidth/scrollHeight is the size including the overflow area
short RenderObject::scrollWidth() const
{
    return (hasOverflowClip() && layer()) ? layer()->scrollWidth() : overflowWidth() - overflowLeft();
}

int RenderObject::scrollHeight() const
{
    return (hasOverflowClip() && layer()) ? layer()->scrollHeight() : overflowHeight() - overflowTop();
}

void RenderObject::updatePixmap(const QRect& /*r*/, CachedImage* image)
{
#ifdef __GNUC__
#warning "FIXME: Check if complete!"
#endif
    //repaint bg when it finished loading
    if(image && parent() && style() && style()->backgroundLayers()->containsImage(image)) {
        isBody() ? canvas()->repaint() : repaint();
    }
}

void RenderObject::setNeedsLayout(bool b, bool markParents)
{
    bool alreadyNeededLayout = m_needsLayout;
    m_needsLayout = b;
    if (b) {
        if (!alreadyNeededLayout && markParents && m_parent) {
            dirtyFormattingContext( false );
            markContainingBlocksForLayout();
        }
    }
    else {
        m_posChildNeedsLayout = false;
        m_normalChildNeedsLayout = false;
    }
}

void RenderObject::setChildNeedsLayout(bool b, bool markParents)
{
    bool alreadyNeededLayout = m_normalChildNeedsLayout;
    m_normalChildNeedsLayout = b;
    if (b) {
        if (!alreadyNeededLayout && markParents)
            markContainingBlocksForLayout();
    }
    else {
        m_posChildNeedsLayout = false;
        m_normalChildNeedsLayout = false;
    }
}

void RenderObject::markContainingBlocksForLayout()
{
    RenderObject *o = container();
    RenderObject *last = this;

    while (o) {
        if (!last->isText() && (last->style()->position() == PFIXED || last->style()->position() == PABSOLUTE)) {
            if (o->m_posChildNeedsLayout)
                return;
            o->m_posChildNeedsLayout = true;
        }
        else {
            if (o->m_normalChildNeedsLayout)
                return;
            o->m_normalChildNeedsLayout = true;
        }

        last = o;
        o = o->container();
    }

    last->scheduleRelayout();
}

RenderBlock *RenderObject::containingBlock() const
{
    if(isTableCell())
        return static_cast<RenderBlock*>( parent()->parent()->parent() );
    if (isCanvas())
        return const_cast<RenderBlock*>( static_cast<const RenderBlock*>(this) );

    RenderObject *o = parent();
    if(m_style->position() == PFIXED) {
        while ( o && !o->isCanvas() )
            o = o->parent();
    }
    else if(m_style->position() == PABSOLUTE) {
        while (o &&
               ( o->style()->position() == PSTATIC || ( o->isInline() && !o->isReplaced() ) ) && !o->isCanvas()) {
               // for relpos inlines, return the nearest block - it will host the positioned objects list
               if (o->isInline() && !o->isReplaced() && o->style()->position() == PRELATIVE)
                   return o->containingBlock();
            o = o->parent();
        }
    } else {
        while(o && ( ( o->isInline() && !o->isReplaced() ) || o->isTableRow() || o->isTableSection() ||
                       o->isTableCol() || o->isFrameSet() ||
                       o->isSVGContainer() || o->isSVGRoot() ) ) // for svg

            o = o->parent();
    }
    // this is just to make sure we return a valid element.
    // the case below should never happen...
    if(!o || !o->isRenderBlock()) {
        if(!isCanvas()) {
#ifndef NDEBUG
            qDebug() << this << ": " << renderName() << "(RenderObject): No containingBlock!";
            const RenderObject* p = this;
            while (p->parent()) p = p->parent();
            p->printTree();
#endif
        }
        return canvas(); // likely wrong, but better than a crash
    }

    return static_cast<RenderBlock*>( o );
}

short RenderObject::containingBlockWidth(RenderObject*) const
{
    // ###
    return containingBlock()->contentWidth();
}

int RenderObject::containingBlockHeight(RenderObject*) const
{
    // ###
    return containingBlock()->contentHeight();
}

bool RenderObject::sizesToMaxWidth() const
{
    // Marquees in WinIE are like a mixture of blocks and inline-blocks.  They size as though they're blocks,
    // but they allow text to sit on the same line as the marquee.
    if (isFloating() || isCompact() ||
        (isInlineBlockOrInlineTable() && !isHTMLMarquee()) ||
        (element() && (element()->id() == ID_BUTTON || element()->id() == ID_LEGEND)))
        return true;

    // Children of a horizontal marquee do not fill the container by default.
    // FIXME: Need to deal with MAUTO value properly.  It could be vertical.
    if (parent()->style()->overflowX() == OMARQUEE) {
        EMarqueeDirection dir = parent()->style()->marqueeDirection();
        if (dir == MAUTO || dir == MFORWARD || dir == MBACKWARD || dir == MLEFT || dir == MRIGHT)
            return true;
    }

#ifdef APPLE_CHANGES	// ### what the heck is a flexbox?
    // Flexible horizontal boxes lay out children at their maxwidths.  Also vertical boxes
    // that don't stretch their kids lay out their children at their maxwidths.
    if (parent()->isFlexibleBox() &&
        (parent()->style()->boxOrient() == HORIZONTAL || parent()->style()->boxAlign() != BSTRETCH))
        return true;
#endif

    return false;
}

// from Mozilla's nsCSSColorUtils.cpp
static int brightness(int red, int green, int blue)
{

  int intensity = (red + green + blue) / 3;

  int luminosity =
    ((RED_LUMINOSITY * red) / 100) +
    ((GREEN_LUMINOSITY * green) / 100) +
    ((BLUE_LUMINOSITY * blue) / 100);

  return ((intensity * INTENSITY_FACTOR) +
          (luminosity * LUMINOSITY_FACTOR)) / 100;
}

static void calc3DColor(QColor &color, bool darken)
{
  int rb = color.red();
  int gb = color.green();
  int bb = color.blue();
  int a = color.alpha();

  int brightness_ = brightness(rb,gb,bb);

  int f0, f1;
  if (brightness_ < COLOR_DARK_THRESHOLD) {
    f0 = COLOR_DARK_BS_FACTOR;
    f1 = COLOR_DARK_TS_FACTOR;
  } else if (brightness_ > COLOR_LIGHT_THRESHOLD) {
    f0 = COLOR_LITE_BS_FACTOR;
    f1 = COLOR_LITE_TS_FACTOR;
  } else {
    f0 = COLOR_DARK_BS_FACTOR +
      (brightness_ *
       (COLOR_LITE_BS_FACTOR - COLOR_DARK_BS_FACTOR) / MAX_COLOR);
    f1 = COLOR_DARK_TS_FACTOR +
      (brightness_ *
       (COLOR_LITE_TS_FACTOR - COLOR_DARK_TS_FACTOR) / MAX_COLOR);
  }

  if (darken) {
    int r = rb - (f0 * rb / 100);
    int g = gb - (f0 * gb / 100);
    int b = bb - (f0 * bb / 100);
    if ((r == rb) && (g == gb) && (b == bb))
      color = (color == Qt::black) ? QColor(DARK_GRAY) : QColor(Qt::black);
    else
      color.setRgb(r, g, b);
  } else {
    int r = qMin(rb + (f1 * (MAX_COLOR - rb) / 100), 255);
    int g = qMin(gb + (f1 * (MAX_COLOR - gb) / 100), 255);
    int b = qMin(bb + (f1 * (MAX_COLOR - bb) / 100), 255);
    if ((r == rb) && (g == gb) && (b == bb))
      color = (color == Qt::white) ? QColor(LIGHT_GRAY) : QColor(Qt::white);
    else
      color.setRgb(r, g, b);
  }
  color.setAlpha(a);
}

void RenderObject::drawBorder(QPainter *p, int x1, int y1, int x2, int y2,
                              BorderSide s, QColor c, const QColor& textcolor, EBorderStyle style,
                              int adjbw1, int adjbw2, bool invalidisInvert, qreal *nextDashOffset)
{
    if(nextDashOffset && style != DOTTED && style != DASHED)
        *nextDashOffset = 0;

    if (p->hasClipping() && !p->clipRegion().boundingRect().intersects(QRect(x1, y1, x2 - x1, y2 - y1))) {
        if (nextDashOffset && (style == DOTTED || style == DASHED))
            *nextDashOffset += (s == BSTop || s == BSBottom) ? (x2 - x1) : (y2 - y1);
        return;
    }

    int width = (s==BSTop||s==BSBottom?y2-y1:x2-x1);

    if(style == DOUBLE && width < 3)
        style = SOLID;

    if(!c.isValid()) {
        if(invalidisInvert)
        {
            // handle 'outline-color: invert'
            if (p->paintEngine() && p->paintEngine()->hasFeature(QPaintEngine::BlendModes)) {
                p->setCompositionMode(QPainter::CompositionMode_Difference);
                c = Qt::white;
            } else {
                // 'invert' is not supported on this platform (for instance XRender)
                // CSS3 UI 8.4: If the UA does not support the 'invert' value then the initial value of
                // the 'outline-color' property is the 'currentColor' [CSS3COLOR] keyword.
                c = m_style->color();
            }
        }
        else {
            if(style == INSET || style == OUTSET || style == RIDGE || style ==
            GROOVE)
                c = Qt::white;
            else
                c = textcolor;
        }
    }

    switch(style)
    {
    case BNATIVE:
    case BNONE:
    case BHIDDEN:
        // should not happen
        if(invalidisInvert && p->compositionMode() == QPainter::CompositionMode_Difference)
            p->setCompositionMode(QPainter::CompositionMode_SourceOver);

        return;
    case DOTTED:
    case DASHED:
    {
        if (width <= 0)
            break;

        //Figure out on/off spacing
        int onLen  = width;
        int offLen = width;

        if (style == DASHED)
        {
            if (width == 1)
            {
                onLen  = 3;
                offLen = 3;
            }
            else
            {
                onLen  = width  * 3;
                offLen = width;
            }
        }

        // Compute the offset for the dash pattern, taking the direction of
        // the line into account. (The borders are drawn counter-clockwise)
        QPoint offset(0, 0);
        if (nextDashOffset) {
            switch (s)
            {
            // The left border is drawn top to bottom
            case BSLeft:
                offset.ry() = -qRound(*nextDashOffset);
                *nextDashOffset += (y2 - y1);
                break;

            // The bottom border is drawn left to right
            case BSBottom:
                offset.rx() = -qRound(*nextDashOffset);
                *nextDashOffset += (x2 - x1);
                break;

            // The top border is drawn right to left
            case BSTop:
                offset.rx() = (x2 - x1) + offLen + qRound(*nextDashOffset);
                *nextDashOffset += (x2 - x1);
                break;

            // The right border is drawn bottom to top
            case BSRight:
                offset.ry() = (y2 - y1) + offLen + qRound(*nextDashOffset);
                *nextDashOffset += (y2 - y1);
                break;
            }

            offset.rx() = offset.x() % (onLen + offLen);
            offset.ry() = offset.y() % (onLen + offLen);
        }

        if ((onLen + offLen) <= 32 && width < 0x7fff)
        {
            if (!s_dashedLineCache)
                s_dashedLineCache = new QCache<quint64, QPixmap>(30);

            bool horizontal = (s == BSBottom || s == BSTop);
            quint64 key = int(horizontal) << 31 | (onLen & 0xff) << 23 | (offLen & 0xff) << 15 | (width & 0x7fff);
            key = key << 32 | c.rgba();

            QPixmap *tile = s_dashedLineCache->object(key);
            if (!tile)
            {
                QPainterPath path;
                int size = (onLen + offLen) * (64 / (onLen + offLen));
                if (horizontal)
                {
                    tile = new QPixmap(size, width);
                    tile->fill(Qt::transparent);
                    for (int x = 0; x < tile->width(); x += onLen + offLen)
                        path.addRect(x, 0, onLen, tile->height());
                }
                else //Vertical
                {
                    tile = new QPixmap(width, size);
                    tile->fill(Qt::transparent);
                    for (int y = 0; y < tile->height(); y += onLen + offLen)
                        path.addRect(0, y, tile->width(), onLen);
                }
                QPainter p2(tile);
                p2.fillPath(path, c);
                p2.end();
                s_dashedLineCache->insert(key, tile);
            }

            QRect r = QRect(x1, y1, x2 - x1, y2 - y1);
            if (p->hasClipping())
                r &= p->clipRegion().boundingRect();

            // Make sure we're drawing the pattern in the correct phase
            if (horizontal && r.left() > x1)
                offset.rx() += (x1 - r.left());
            else if (!horizontal && r.top() > y1)
                offset.ry() += (y1 - r.top());

            p->drawTiledPixmap(r, *tile, -offset);
        }
        else
        {
            const QRect bounding(x1, y1, x2 - x1, y2 - y1);
            QPainterPath path;
            if (s == BSBottom || s == BSTop) //Horizontal
            {
                if (offset.x() > 0)
                    offset.rx() -= onLen + offLen;
                for (int x = x1 + offset.x(); x < x2; x += onLen + offLen) {
                    const QRect r(x, y1, qMin(onLen, (x2 - x)), width);
                    path.addRect(r & bounding);
                }
            }
            else //Vertical
            {
                if (offset.y() > 0)
                    offset.ry() -= onLen + offLen;
                for (int y = y1 + offset.y(); y < y2; y += onLen + offLen) {
                    const QRect r(x1, y, width, qMin(onLen, (y2 - y)));
                    path.addRect(r & bounding);
                }
            }

            p->fillPath(path, c);
        }
        break;
    }
    case DOUBLE:
    {
        int third = (width+1)/3;

        if (adjbw1 == 0 && adjbw2 == 0)
        {
            p->setPen(Qt::NoPen);
            p->setBrush(c);
            switch(s)
            {
            case BSTop:
            case BSBottom:
                p->drawRect(x1, y1      , x2-x1, third);
                p->drawRect(x1, y2-third, x2-x1, third);
                break;
            case BSLeft:
                p->drawRect(x1      , y1, third, y2-y1);
                p->drawRect(x2-third, y1, third, y2-y1);
                break;
            case BSRight:
                p->drawRect(x1      , y1, third, y2-y1);
                p->drawRect(x2-third, y1, third, y2-y1);
                break;
            }
        }
        else
        {
            int adjbw1bigthird;
            if (adjbw1>0) adjbw1bigthird = adjbw1+1;
            else adjbw1bigthird = adjbw1 - 1;
            adjbw1bigthird /= 3;

            int adjbw2bigthird;
            if (adjbw2>0) adjbw2bigthird = adjbw2 + 1;
            else adjbw2bigthird = adjbw2 - 1;
            adjbw2bigthird /= 3;

          switch(s)
            {
            case BSTop:
              drawBorder(p, x1+qMax((-adjbw1*2+1)/3,0), y1        , x2-qMax((-adjbw2*2+1)/3,0), y1 + third, s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
              drawBorder(p, x1+qMax(( adjbw1*2+1)/3,0), y2 - third, x2-qMax(( adjbw2*2+1)/3,0), y2        , s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
              break;
            case BSLeft:
              drawBorder(p, x1        , y1+qMax((-adjbw1*2+1)/3,0), x1+third, y2-qMax((-adjbw2*2+1)/3,0), s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
              drawBorder(p, x2 - third, y1+qMax(( adjbw1*2+1)/3,0), x2      , y2-qMax(( adjbw2*2+1)/3,0), s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
              break;
            case BSBottom:
              drawBorder(p, x1+qMax(( adjbw1*2+1)/3,0), y1      , x2-qMax(( adjbw2*2+1)/3,0), y1+third, s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
              drawBorder(p, x1+qMax((-adjbw1*2+1)/3,0), y2-third, x2-qMax((-adjbw2*2+1)/3,0), y2      , s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
              break;
            case BSRight:
            drawBorder(p, x1      , y1+qMax(( adjbw1*2+1)/3,0), x1+third, y2-qMax(( adjbw2*2+1)/3,0), s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
            drawBorder(p, x2-third, y1+qMax((-adjbw1*2+1)/3,0), x2      , y2-qMax((-adjbw2*2+1)/3,0), s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
              break;
            default:
              break;
            }
        }
        break;
    }
    case RIDGE:
    case GROOVE:
    {
        EBorderStyle s1;
        EBorderStyle s2;
        if (style==GROOVE)
        {
            s1 = INSET;
            s2 = OUTSET;
        }
        else
        {
            s1 = OUTSET;
            s2 = INSET;
        }

        int adjbw1bighalf;
        int adjbw2bighalf;
        if (adjbw1>0) adjbw1bighalf=adjbw1+1;
        else adjbw1bighalf=adjbw1-1;
        adjbw1bighalf/=2;

        if (adjbw2>0) adjbw2bighalf=adjbw2+1;
        else adjbw2bighalf=adjbw2-1;
        adjbw2bighalf/=2;

        switch (s)
        {
        case BSTop:
            drawBorder(p, x1+qMax(-adjbw1  ,0)/2,  y1        , x2-qMax(-adjbw2,0)/2, (y1+y2+1)/2, s, c, textcolor, s1, adjbw1bighalf, adjbw2bighalf);
            drawBorder(p, x1+qMax( adjbw1+1,0)/2, (y1+y2+1)/2, x2-qMax( adjbw2+1,0)/2,  y2        , s, c, textcolor, s2, adjbw1/2, adjbw2/2);
            break;
        case BSLeft:
            drawBorder(p,  x1        , y1+qMax(-adjbw1  ,0)/2, (x1+x2+1)/2, y2-qMax(-adjbw2,0)/2, s, c, textcolor, s1, adjbw1bighalf, adjbw2bighalf);
            drawBorder(p, (x1+x2+1)/2, y1+qMax( adjbw1+1,0)/2,  x2        , y2-qMax( adjbw2+1,0)/2, s, c, textcolor, s2, adjbw1/2, adjbw2/2);
            break;
        case BSBottom:
            drawBorder(p, x1+qMax( adjbw1  ,0)/2,  y1        , x2-qMax( adjbw2,0)/2, (y1+y2+1)/2, s, c, textcolor, s2,  adjbw1bighalf, adjbw2bighalf);
            drawBorder(p, x1+qMax(-adjbw1+1,0)/2, (y1+y2+1)/2, x2-qMax(-adjbw2+1,0)/2,  y2        , s, c, textcolor, s1, adjbw1/2, adjbw2/2);
            break;
        case BSRight:
            drawBorder(p,  x1        , y1+qMax( adjbw1  ,0)/2, (x1+x2+1)/2, y2-qMax( adjbw2,0)/2, s, c, textcolor, s2, adjbw1bighalf, adjbw2bighalf);
            drawBorder(p, (x1+x2+1)/2, y1+qMax(-adjbw1+1,0)/2,  x2        , y2-qMax(-adjbw2+1,0)/2, s, c, textcolor, s1, adjbw1/2, adjbw2/2);
            break;
        }
        break;
    }
    case INSET:
    case OUTSET:
        calc3DColor(c, (style == OUTSET && (s == BSBottom || s == BSRight)) ||
             (style == INSET && ( s == BSTop || s == BSLeft ) ) );
        /* nobreak; */
    case SOLID:
        p->setPen(Qt::NoPen);
        p->setBrush(c);
        Q_ASSERT(x2>=x1);
        Q_ASSERT(y2>=y1);
        if (adjbw1==0 && adjbw2 == 0) {
            p->drawRect(x1,y1,x2-x1,y2-y1);
            return;
        }
        QPolygon quad(4);
        switch(s) {
        case BSTop:
            quad.setPoints(4,
                           x1+qMax(-adjbw1,0), y1,
                           x1+qMax( adjbw1,0), y2,
                           x2-qMax( adjbw2,0), y2,
                           x2-qMax(-adjbw2,0), y1);
            break;
        case BSBottom:
            quad.setPoints(4,
                           x1+qMax( adjbw1,0), y1,
                           x1+qMax(-adjbw1,0), y2,
                           x2-qMax(-adjbw2,0), y2,
                           x2-qMax( adjbw2,0), y1);
            break;
        case BSLeft:
          quad.setPoints(4,
                         x1, y1+qMax(-adjbw1,0),
                                x1, y2-qMax(-adjbw2,0),
                         x2, y2-qMax( adjbw2,0),
                         x2, y1+qMax( adjbw1,0));
            break;
        case BSRight:
          quad.setPoints(4,
                         x1, y1+qMax( adjbw1,0),
                                x1, y2-qMax( adjbw2,0),
                         x2, y2-qMax(-adjbw2,0),
                         x2, y1+qMax(-adjbw1,0));
            break;
        }
        p->drawConvexPolygon(quad);
        break;
    }

    if(invalidisInvert && p->compositionMode() == QPainter::CompositionMode_Difference)
        p->setCompositionMode(QPainter::CompositionMode_SourceOver);
}

void RenderObject::adjustBorderRadii(BorderRadii &tl, BorderRadii &tr, BorderRadii &bl, BorderRadii &br, int w, int h) const
{
    // CSS Backgrounds and Borders Module Level 3 (WD-css3-background-20080910), chapter 4.5:
    // "  Corners do not overlap: When the sum of two adjacent corner radii exceeds the size of the border box,
    //  UAs must reduce one or more of the radii. The algorithm for reducing radii is as follows:"
    //  The sum of two adjacent radii may not be more than the width or height (whichever is relevant) of the box.
    //  If any sum exceeds that value, all radii are reduced according to the following formula:"
    const int horS = qMax(tl.horizontal + tr.horizontal, bl.horizontal + br.horizontal);
    const int verS = qMax(tl.vertical + bl.vertical, tr.vertical + br.vertical);

    qreal f = 1.0;
    if (horS > 0)
        f = qMin(f, w / qreal(horS));
    if (verS > 0)
        f = qMin(f, h / qreal(verS));

    if (f < 1.0) {
        tl.horizontal *= f;
        tr.horizontal *= f;
        bl.horizontal *= f;
        br.horizontal *= f;
        tl.vertical   *= f;
        tr.vertical   *= f;
        bl.vertical   *= f;
        br.vertical   *= f;
    }
}

static QImage blendCornerImages(const QImage &image1, const QImage &image2)
{
    QImage mask(image1.size(), QImage::Format_ARGB32_Premultiplied);
    QImage composite = image1;
    QImage temp = image2;

    // Construct the mask image
    QConicalGradient gradient(mask.width() / 2, mask.height() / 2, 0);
    gradient.setColorAt(0.00, Qt::transparent);
    gradient.setColorAt(0.25, Qt::black);
    gradient.setColorAt(0.50, Qt::black);
    gradient.setColorAt(0.75, Qt::transparent);
    gradient.setColorAt(1.00, Qt::transparent);

    QBrush gradientBrush = gradient;

    if (mask.width() != mask.height()) {
        int min = qMin(mask.width(), mask.height());
        QTransform xform;
        xform.translate(mask.width() / 2, mask.height() / 2);
        xform.scale(min / mask.width(), min / mask.height());
        gradientBrush.setTransform(xform);
    }

    QPainter p;
    p.begin(&mask);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(mask.rect(), gradientBrush);
    p.end();

    p.begin(&temp);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawImage(0, 0, mask);
    p.end();

    p.begin(&composite);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.drawImage(0, 0, mask);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.drawImage(0, 0, temp);
    p.end();

    return composite;
}

static QBrush cornerGradient(int cx, int cy, const BorderRadii &radius, int angleStart, int angleSpan,
                             const QColor &startColor, const QColor &finalColor)
{
    QConicalGradient g(0, 0, angleStart);
    g.setColorAt(0, startColor);
    g.setColorAt(angleSpan / 360.0, finalColor);

    QBrush brush(g);

    QTransform xform;
    xform.translate(cx, cy);

    if (radius.horizontal < radius.vertical)
        xform.scale(radius.horizontal / radius.vertical, 1);
    else if (radius.vertical < radius.horizontal)
        xform.scale(1, radius.vertical / radius.horizontal);

    brush.setTransform(xform);
    return brush;
}

void RenderObject::drawBorderArc(QPainter *p, int x, int y, float horThickness, float vertThickness,
                                 const BorderRadii &radius, int angleStart, int angleSpan, const QBrush &brush,
                                 const QColor &textColor, EBorderStyle style, qreal *nextDashOffset) const
{
    QColor c = brush.color();
    if (!c.isValid()) {
        if (style == INSET || style == OUTSET || style == RIDGE || style == GROOVE)
            c = Qt::white;
        else
            c = textColor;
    }

    QColor light = c;
    QColor dark = c;
    calc3DColor(light, false);
    calc3DColor(dark, true);

    if (style == DOUBLE && horThickness < 3 && vertThickness < 3)
        style = SOLID;

    if (nextDashOffset && style != DOTTED && style != DASHED)
        *nextDashOffset = 0;

    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    switch (style)
    {
        case BNATIVE:
        case BNONE:
        case BHIDDEN:
        {
            // Should not happen
            break;
        }

        case SOLID:
        {
            const QRect outerRect = QRect(x - radius.horizontal, y - radius.vertical, radius.horizontal * 2, radius.vertical * 2);
            const QRect innerRect = outerRect.adjusted(horThickness, vertThickness, -horThickness, -vertThickness);
            QPainterPath path;
            path.arcMoveTo(outerRect, angleStart);
            path.arcTo(outerRect, angleStart, angleSpan);
            if (innerRect.isValid())
                path.arcTo(innerRect, angleStart + angleSpan, -angleSpan);
            else
                path.lineTo(x, y);
            path.closeSubpath();
            p->fillPath(path, brush);
            break;
        }

        case DOUBLE:
        {
            const qreal hw = (horThickness + 1) / 3;
            const qreal vw = (vertThickness + 1) / 3;

            BorderRadii br;
            br.horizontal = radius.horizontal - hw * 2 + 1;
            br.vertical = radius.vertical - vw * 2 + 1;

            drawBorderArc(p, x, y, hw, vw, radius, angleStart, angleSpan, brush, textColor, SOLID);
            drawBorderArc(p, x, y, hw, vw, br, angleStart, angleSpan, brush, textColor, SOLID);
            break;
        }

        case INSET:
        case OUTSET:
        {
            QImage image1(radius.horizontal * 2, radius.vertical * 2, QImage::Format_ARGB32_Premultiplied);
            image1.fill(0);

            QImage image2 = image1;

            const QColor c1 = style == OUTSET ? dark : light;
            const QColor c2 = style == OUTSET ? light : dark;

            QPainter p2;
            p2.begin(&image1);
            drawBorderArc(&p2, radius.horizontal, radius.vertical, horThickness, vertThickness,
                          radius, angleStart, angleSpan, c1, textColor, SOLID);
            p2.end();

            p2.begin(&image2);
            drawBorderArc(&p2, radius.horizontal, radius.vertical, horThickness, vertThickness,
                          radius, angleStart, angleSpan, c2, textColor, SOLID);
            p2.end();

            p->drawImage(x - radius.horizontal, y - radius.vertical, blendCornerImages(image1, image2));
            break;
        }

        case RIDGE:
        case GROOVE:
        {
            QImage image1(radius.horizontal * 2, radius.vertical * 2, QImage::Format_ARGB32_Premultiplied);
            image1.fill(0);

            QImage image2 = image1;

            const QColor c1 = style == RIDGE ? dark : light;
            const QColor c2 = style == RIDGE ? light : dark;

            const qreal hw = horThickness / 2;
            const qreal vw = vertThickness / 2;
            int cx = radius.horizontal;
            int cy = radius.vertical;

            BorderRadii innerRadius;
            innerRadius.horizontal = radius.horizontal - hw;
            innerRadius.vertical = radius.vertical - vw;

            QPainter p2;
            p2.begin(&image1);
            drawBorderArc(&p2, cx, cy, hw, vw, radius, angleStart, angleSpan, c1, textColor, SOLID);
            drawBorderArc(&p2, cx, cy, hw, vw, innerRadius, angleStart, angleSpan, c2, textColor, SOLID);
            p2.end();

            p2.begin(&image2);
            drawBorderArc(&p2, cx, cy, hw, vw, radius, angleStart, angleSpan, c2, textColor, SOLID);
            drawBorderArc(&p2, cx, cy, hw, vw, innerRadius, angleStart, angleSpan, c1, textColor, SOLID);
            p2.end();

            p->drawImage(x - radius.horizontal, y - radius.vertical, blendCornerImages(image1, image2));
            break;
        }

        case DOTTED:
        case DASHED:
        {
            const QRectF rect = QRectF(x - radius.horizontal, y - radius.vertical, radius.horizontal * 2, radius.vertical * 2);
            int width;

            // Figure out which border we're starting from
            angleStart = angleStart % 360;
            if (angleStart < 0)
                angleStart += 360;

            if ((angleStart > 45 && angleStart <= 135) || (angleStart > 225 && angleStart <= 315))
                width = vertThickness;
            else
                width = horThickness;

            int onLen  = width;
            int offLen = width;

            if (style == DASHED)
            {
                if (width == 1)
                {
                    onLen  = 3;
                    offLen = 3;
                }
                else
                {
                    onLen  = width  * 3;
                    offLen = width;
                }
            }

            BorderArcStroker stroker;
            stroker.setArc(rect, angleStart, angleSpan);
            stroker.setPenWidth(horThickness, vertThickness);
            stroker.setDashPattern(onLen, offLen);
            stroker.setDashOffset(*nextDashOffset);

            const QPainterPath path = stroker.createStroke(nextDashOffset);
            p->fillPath(path, brush);
        }
    }

    p->restore();
}

void RenderObject::paintBorder(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style, bool begin, bool end)
{
    const QColor& tc = style->borderTopColor();
    const QColor& bc = style->borderBottomColor();
    const QColor& lc = style->borderLeftColor();
    const QColor& rc = style->borderRightColor();

    bool tt = style->borderTopIsTransparent();
    bool bt = style->borderBottomIsTransparent();
    bool rt = style->borderRightIsTransparent();
    bool lt = style->borderLeftIsTransparent();

    EBorderStyle ts = style->borderTopStyle();
    EBorderStyle bs = style->borderBottomStyle();
    EBorderStyle ls = style->borderLeftStyle();
    EBorderStyle rs = style->borderRightStyle();

    bool render_t = ts > BHIDDEN && !tt;
    bool render_l = ls > BHIDDEN && begin && !lt;
    bool render_r = rs > BHIDDEN && end && !rt;
    bool render_b = bs > BHIDDEN && !bt;

    BorderRadii topLeft = style->borderTopLeftRadius();
    BorderRadii topRight = style->borderTopRightRadius();
    BorderRadii bottomLeft = style->borderBottomLeftRadius();
    BorderRadii bottomRight = style->borderBottomRightRadius();

    if (style->hasBorderRadius()) {
        // Adjust the border radii so they don't overlap when taking the size of the box
        // into account.
        adjustBorderRadii(topLeft, topRight, bottomLeft, bottomRight, w, h);
    }

    bool upperLeftBorderStylesMatch = render_l && (ts == ls) && (tc == lc);
    bool upperRightBorderStylesMatch = render_r && (ts == rs) && (tc == rc);
    bool lowerLeftBorderStylesMatch = render_l && (bs == ls) && (bc == lc);
    bool lowerRightBorderStylesMatch = render_r && (bs == rs) && (bc == rc);

    // We do a gradient transition for dotted, dashed, solid and double lines
    // when the styles match but the colors differ.
    bool upperLeftGradient = render_t && render_l && ts == ls && tc != lc && ts > OUTSET;
    bool upperRightGradient = render_t && render_r && ts == rs && tc != rc && ts > OUTSET;
    bool lowerLeftGradient = render_b && render_l && bs == ls && bc != lc && bs > OUTSET;
    bool lowerRightGradient = render_b && render_r && bs == rs && bc != rc && bs > OUTSET;

    qreal nextDashOffset = 0;

    // Draw the borders counter-clockwise starting with the upper right corner
    if(render_t) {
        bool ignore_left = (topLeft.horizontal > 0) ||
            ((tc == lc) && (tt == lt) &&
             (ts >= OUTSET) &&
             (ls == DOTTED || ls == DASHED || ls == SOLID || ls == OUTSET));

        bool ignore_right = (topRight.horizontal > 0) ||
            ((tc == rc) && (tt == rt) &&
             (ts >= OUTSET) &&
             (rs == DOTTED || rs == DASHED || rs == SOLID || rs == INSET));

        int x = _tx + topLeft.horizontal;
        int x2 = _tx + w - topRight.horizontal;

        if (topRight.hasBorderRadius()) {
            int x = _tx + w - topRight.horizontal;
            int y = _ty + topRight.vertical;
            int startAngle, span;

            if (upperRightBorderStylesMatch || upperRightGradient) {
                startAngle = 0;
                span = 90;
            } else {
                startAngle = 45;
                span = 45;
            }

            const QBrush brush = upperRightGradient ?
                    cornerGradient(x, y, topRight, startAngle, span, rc, tc) : tc;

            // Draw the upper right arc
            drawBorderArc(p, x, y, style->borderRightWidth(), style->borderTopWidth(),
                          topRight, startAngle, span, brush, style->color(), ts, &nextDashOffset);
        }

        drawBorder(p, x, _ty, x2, _ty + style->borderTopWidth(), BSTop, tc, style->color(), ts,
                   ignore_left?0:style->borderLeftWidth(),
                   ignore_right?0:style->borderRightWidth(), false, &nextDashOffset);

        if (topLeft.hasBorderRadius()) {
            int x = _tx + topLeft.horizontal;
            int y = _ty + topLeft.vertical;
            int startAngle = 90;
            int span = (upperLeftBorderStylesMatch || upperLeftGradient) ? 90 : 45;
            const QBrush brush = upperLeftGradient ?
                    cornerGradient(x, y, topLeft, startAngle, span, tc, lc) : tc;

            // Draw the upper left arc
            drawBorderArc(p, x, y, style->borderLeftWidth(), style->borderTopWidth(),
                          topLeft, startAngle, span, brush, style->color(), ts, &nextDashOffset);
        } else if (ls == DASHED || ls == DOTTED)
            nextDashOffset = 0; // Reset the offset to avoid partially overlapping dashes
    }

    if(render_l)
    {
	bool ignore_top = (topLeft.vertical > 0) ||
	  ((tc == lc) && (tt == lt) &&
	   (ls >= OUTSET) &&
           (ts == DOTTED || ts == DASHED || ts == SOLID || ts == OUTSET));

	bool ignore_bottom = (bottomLeft.vertical > 0) ||
	  ((bc == lc) && (bt == lt) &&
	   (ls >= OUTSET) &&
	   (bs == DOTTED || bs == DASHED || bs == SOLID || bs == INSET));

        int y = _ty + topLeft.vertical;
        int y2 = _ty + h - bottomLeft.vertical;

        if (!upperLeftBorderStylesMatch && !upperLeftGradient && topLeft.hasBorderRadius()) {
            int x = _tx + topLeft.horizontal;
            int y = _ty + topLeft.vertical;
            int startAngle = 135;
            int span = 45;

            // Draw the upper left arc
            drawBorderArc(p, x, y, style->borderLeftWidth(), style->borderTopWidth(),
                          topLeft, startAngle, span, lc, style->color(), ls, &nextDashOffset);
        }

        drawBorder(p, _tx, y, _tx + style->borderLeftWidth(), y2, BSLeft, lc, style->color(), ls,
                   ignore_top?0:style->borderTopWidth(),
                   ignore_bottom?0:style->borderBottomWidth(), false, &nextDashOffset);

       if (!lowerLeftBorderStylesMatch && !lowerLeftGradient && bottomLeft.hasBorderRadius()) {
            int x = _tx + bottomLeft.horizontal;
            int y = _ty + h - bottomLeft.vertical;
            int startAngle = 180;
            int span = 45;

            // Draw the bottom left arc
            drawBorderArc(p, x, y, style->borderLeftWidth(), style->borderBottomWidth(),
                          bottomLeft, startAngle, span, lc, style->color(), ls, &nextDashOffset);
        }

        // Reset the offset to avoid partially overlapping dashes
        if (!bottomLeft.hasBorderRadius() && (bs == DASHED || bs == DOTTED))
            nextDashOffset = 0;
    }

    if(render_b) {
        bool ignore_left = (bottomLeft.horizontal > 0) ||
            ((bc == lc) && (bt == lt) &&
             (bs >= OUTSET) &&
             (ls == DOTTED || ls == DASHED || ls == SOLID || ls == INSET));

        bool ignore_right = (bottomRight.horizontal > 0) ||
            ((bc == rc) && (bt == rt) &&
             (bs >= OUTSET) &&
             (rs == DOTTED || rs == DASHED || rs == SOLID || rs == OUTSET));

        int x = _tx + bottomLeft.horizontal;
        int x2 = _tx + w - bottomRight.horizontal;

        if (bottomLeft.hasBorderRadius()) {
            int x = _tx + bottomLeft.horizontal;
            int y = _ty + h - bottomLeft.vertical;
            int startAngle, span;

            if (lowerLeftBorderStylesMatch || lowerLeftGradient) {
                startAngle = 180;
                span = 90;
            } else {
                startAngle = 225;
                span = 45;
            }

            const QBrush brush = lowerLeftGradient ?
                    cornerGradient(x, y, bottomLeft, startAngle, span, lc, bc) : bc;

            // Draw the bottom left arc
            drawBorderArc(p, x, y, style->borderLeftWidth(), style->borderBottomWidth(),
                          bottomLeft, startAngle, span, brush, style->color(), bs, &nextDashOffset);
        }

        drawBorder(p, x, _ty + h - style->borderBottomWidth(), x2, _ty + h, BSBottom, bc, style->color(), bs,
                   ignore_left?0:style->borderLeftWidth(),
                   ignore_right?0:style->borderRightWidth(), false, &nextDashOffset);

        if (bottomRight.hasBorderRadius()) {
            int x = _tx + w - bottomRight.horizontal;
            int y = _ty + h - bottomRight.vertical;
            int startAngle = 270;
            int span = (lowerRightBorderStylesMatch || lowerRightGradient) ? 90 : 45;
            const QBrush brush = lowerRightGradient ?
                    cornerGradient(x, y, bottomRight, startAngle, span, bc, rc) : bc;

            // Draw the bottom right arc
            drawBorderArc(p, x, y, style->borderRightWidth(), style->borderBottomWidth(),
                          bottomRight, startAngle, span, brush, style->color(), bs, &nextDashOffset);
        }
        else if (rs == DASHED || rs == DOTTED)
            nextDashOffset = 0; // Reset the offset to avoid partially overlapping dashes
    }

    if(render_r)
    {
	bool ignore_top = (topRight.vertical > 0) ||
	  ((tc == rc) && (tt == rt) &&
	   (rs >= DOTTED || rs == INSET) &&
	   (ts == DOTTED || ts == DASHED || ts == SOLID || ts == OUTSET));

	bool ignore_bottom = (bottomRight.vertical > 0) ||
	  ((bc == rc) && (bt == rt) &&
	   (rs >= DOTTED || rs == INSET) &&
	   (bs == DOTTED || bs == DASHED || bs == SOLID || bs == INSET));

        int y = _ty + topRight.vertical;
        int y2 = _ty + h - bottomRight.vertical;

        if (!lowerRightBorderStylesMatch && !lowerRightGradient && bottomRight.hasBorderRadius()) {
            int x = _tx + w - bottomRight.horizontal;
            int y = _ty + h - bottomRight.vertical;
            int startAngle = 315;
            int span = 45;

            // Draw the bottom right arc
            drawBorderArc(p, x, y, style->borderRightWidth(), style->borderBottomWidth(),
                          bottomRight, startAngle, span, rc, style->color(), rs, &nextDashOffset);
        }

        drawBorder(p, _tx + w - style->borderRightWidth(), y, _tx + w, y2, BSRight, rc, style->color(), rs,
                   ignore_top?0:style->borderTopWidth(),
                   ignore_bottom?0:style->borderBottomWidth(), false, &nextDashOffset);

        if (!upperRightBorderStylesMatch && !upperRightGradient && topRight.hasBorderRadius()) {
            int x = _tx + w - topRight.horizontal;
            int y = _ty + topRight.vertical;
            int startAngle = 0;
            int span = 45;

            // Draw the upper right arc
            drawBorderArc(p, x, y, style->borderRightWidth(), style->borderTopWidth(),
                          topRight, startAngle, span, rc, style->color(), rs, &nextDashOffset);
        }
    }
}

void RenderObject::paintOutline(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style)
{
    int ow = style->outlineWidth();
    if(!ow) return;

    const QColor& oc = style->outlineColor();
    EBorderStyle os = style->outlineStyle();
    int offset = style->outlineOffset();

#ifdef APPLE_CHANGES
    if (style->outlineStyleIsAuto()) {
        p->initFocusRing(ow, offset, oc);
        addFocusRingRects(p, _tx, _ty);
        p->drawFocusRing();
        p->clearFocusRing();
        return;
    }
#endif

    _tx -= offset;
    _ty -= offset;
    w += 2*offset;
    h += 2*offset;

    drawBorder(p, _tx-ow, _ty-ow, _tx, _ty+h+ow, BSLeft,
               QColor(oc), style->color(),
               os, ow, ow, true);

    drawBorder(p, _tx-ow, _ty-ow, _tx+w+ow, _ty, BSTop,
               QColor(oc), style->color(),
               os, ow, ow, true);

    drawBorder(p, _tx+w, _ty-ow, _tx+w+ow, _ty+h+ow, BSRight,
               QColor(oc), style->color(),
               os, ow, ow, true);

    drawBorder(p, _tx-ow, _ty+h, _tx+w+ow, _ty+h+ow, BSBottom,
               QColor(oc), style->color(),
               os, ow, ow, true);

}

void RenderObject::paint( PaintInfo&, int /*tx*/, int /*ty*/)
{
}

void RenderObject::repaintRectangle(int x, int y, int w, int h, Priority p, bool f)
{
    if(parent()) parent()->repaintRectangle(x, y, w, h, p, f);
}

#ifdef ENABLE_DUMP

QString RenderObject::information() const
{
    QString str;
    int x; int y;
    absolutePosition(x,y);
    x += inlineXPos();
    y += inlineYPos();
    QTextStream ts( &str, QIODevice::WriteOnly );
    ts << renderName()
        << "(" << (style() ? style()->refCount() : 0) << ")"
       << ": " << (void*)this << "  ";
    ts << "{" << x << " " << y << "} ";
    if (isInline()) ts << "il ";
    if (childrenInline()) ts << "ci ";
    if (isFloating()) ts << "fl ";
    if (isAnonymous()) ts << "an ";
    if (isRelPositioned()) ts << "rp ";
    if (isPositioned()) ts << "ps ";
    if (isReplaced()) ts << "rp ";
    if (needsLayout()) ts << "nl ";
    if (minMaxKnown()) ts << "mmk ";
    if (m_recalcMinMax) ts << "rmm ";
    if (mouseInside()) ts << "mi ";
    if (style() && style()->zIndex()) ts << "zI: " << style()->zIndex();
    if (style() && style()->hasAutoZIndex()) ts << "zI: auto ";
    if (element()) {
        if (element()->active()) ts << "act ";
        if (element()->hasAnchor()) ts << "anchor ";
        if (element()->focused()) ts << "focus ";
        ts << " <" << LocalName::fromId(localNamePart(element()->id())).toString().string() << ">";

    } else if (isPseudoAnonymous() && style() && style()->styleType() != RenderStyle::NOPSEUDO) {
        ts << " <" << LocalName::fromId(localNamePart(node()->id())).toString().string();
        QString pseudo;
        switch (style()->styleType()) {
          case RenderStyle::FIRST_LETTER:
            pseudo = ":first-letter"; break;
          case RenderStyle::BEFORE:
            pseudo = ":before"; break;
          case RenderStyle::AFTER:
            pseudo = ":after"; break;
          default:
            pseudo = ":pseudo-element";
        }
        ts << pseudo;
        ts << ">";
    }
    ts << " (" << xPos() << "," << yPos() << "," << width() << "," << height() << ")"
       << " [" << minWidth() << "-" << maxWidth() << "]"
       << " { mT: " << marginTop() << " qT: " << isTopMarginQuirk()
       << " mB: " << marginBottom() << " qB: " << isBottomMarginQuirk()
       << "}"
        << (isTableCell() ?
            ( QLatin1String(" [r=") +
              QString::number( static_cast<const RenderTableCell *>(this)->row() ) +
              QLatin1String(" c=") +
              QString::number( static_cast<const RenderTableCell *>(this)->col() ) +
              QLatin1String(" rs=") +
              QString::number( static_cast<const RenderTableCell *>(this)->rowSpan() ) +
              QLatin1String(" cs=") +
              QString::number( static_cast<const RenderTableCell *>(this)->colSpan() ) +
              QLatin1String("]") ) : QString() );
    if ( layer() )
        ts << " layer=" << layer();
    if ( continuation() )
        ts << " continuation=" << continuation();
    if (isText())
        ts << " \"" << QString::fromRawData(static_cast<const RenderText *>(this)->text(), qMin(static_cast<const RenderText *>(this)->length(), 10u)) << "\"";
    return str;
}

void RenderObject::printTree(int indent) const
{
    QString ind;
    ind.fill(' ', indent);

    // qDebug() << (ind + information());

    RenderObject *child = firstChild();
    while( child != 0 )
    {
        child->printTree(indent+2);
        child = child->nextSibling();
    }
}

static QTextStream &operator<<(QTextStream &ts, const QRect &r)
{
    return ts << "at (" << r.x() << "," << r.y() << ") size " << r.width() << "x" << r.height();
}

//A bit like getTagName, but handles XML, too.
static QString lookupTagName(NodeImpl* node) {
    return LocalName::fromId(node->id()).toString().string();
}

void RenderObject::dump(QTextStream &ts, const QString &ind) const
{
    if ( !layer() )
        ts << endl;

    ts << ind << renderName();

    if (style() && style()->zIndex()) {
        ts << " zI: " << style()->zIndex();
    }

    if (element()) {
        QString tagName(lookupTagName(element()));
        if (!tagName.isEmpty()) {
            ts << " {" << tagName << "}";
        }
    } else if (isPseudoAnonymous() && style() && style()->styleType() != RenderStyle::NOPSEUDO) {
        QString pseudo;
        QString tagName(lookupTagName(node()));
        switch (style()->styleType()) {
          case RenderStyle::FIRST_LETTER:
            pseudo = ":first-letter"; break;
          case RenderStyle::BEFORE:
            pseudo = ":before"; break;
          case RenderStyle::AFTER:
            pseudo = ":after"; break;
          default:
            pseudo = ":pseudo-element";
        }
        ts << " {" << tagName << pseudo << "}";
    }

    QRect r(xPos(), yPos(), width(), height());
    ts << " " << r;

    if ( parent() )
        ts << style()->createDiff( *parent()->style() );

    if (isAnonymous()) { ts << " anonymousBox"; }
    if (isFloating()) { ts << " floating"; }
    if (isPositioned()) { ts << " positioned"; }
    if (isRelPositioned()) { ts << " relPositioned"; }
    if (isText()) { ts << " text"; }
    if (isInline()) { ts << " inline"; }
    if (isReplaced()) { ts << " replaced"; }
    if (shouldPaintBackgroundOrBorder()) { ts << " paintBackground"; }
    if (needsLayout()) { ts << " needsLayout"; }
    if (minMaxKnown()) { ts << " minMaxKnown"; }
    if (hasFirstLine()) { ts << " hasFirstLine"; }
    if (afterPageBreak()) { ts << " afterPageBreak"; }
}

void RenderObject::printLineBoxTree() const
{
    RenderObject* child = firstChild();
    for (; child; child = child->nextSibling())
        child->printLineBoxTree();
    if (isRenderBlock()) {
        const RenderBlock* block = static_cast<const RenderBlock*>(this);
        RootInlineBox* rootBox = block->firstRootBox();
        for (; rootBox; rootBox = rootBox->nextRootBox())
            rootBox->printTree();
    }
}
#endif

bool RenderObject::shouldSelect() const
{
#if 0 // ### merge
    const RenderObject* curr = this;
    DOM::NodeImpl *node = 0;
    bool forcedOn = false;

    while (curr) {
        if (curr->style()->userSelect() == SELECT_TEXT)
            forcedOn = true;
        if (!forcedOn && curr->style()->userSelect() == SELECT_NONE)
            return false;

        if (!node)
            node = curr->element();
        curr = curr->parent();
    }

    // somewhere up the render tree there must be an element!
    assert(node);

    return node->dispatchHTMLEvent(DOM::EventImpl::SELECTSTART_EVENT, true, true);
#else
    return true;
#endif
}

void RenderObject::selectionStartEnd(int& spos, int& epos)
{
    if (parent())
        parent()->selectionStartEnd(spos, epos);
}

void RenderObject::setStyle(RenderStyle *style)
{
    if (m_style == style)
        return;

    RenderStyle::Diff d = m_style ? m_style->diff( style ) : RenderStyle::Layout;
    //qDebug("m_style: %p new style, diff=%d", m_style,  d);

    Priority pri = NormalPriority;
    if (m_style) {
        pri = HighPriority;
        if ( d >= RenderStyle::Visible && !isText() && m_parent &&
             ( d == RenderStyle::Position ||
               m_style->outlineWidth() > style->outlineWidth() ||
               (!m_style->hidesOverflow() && style->hidesOverflow()) ||
               ( m_style->hasClip() && !(m_style->clip() == style->clip()) ) ) ) {
            // schedule a repaint with the old style
            if (layer() && !isInlineFlow())
                layer()->repaint(pri);
            else
                repaint(pri);
        }

        if ( ( isFloating() && m_style->floating() != style->floating() ) ||
               ( isPositioned() && m_style->position() != style->position() &&
                 style->position() != PABSOLUTE && style->position() != PFIXED ) )
            removeFromObjectLists();

        if ( layer() ) {
            if ( ( m_style->hasAutoZIndex() != style->hasAutoZIndex() ||
                   m_style->zIndex() != style->zIndex() ||
                   m_style->visibility() != style->visibility() ) ) {
                layer()->stackingContext()->dirtyZOrderLists();
                layer()->dirtyZOrderLists();
            }
            // keep layer hierarchy visibility bits up to date if visibility changes
            if (m_style->visibility() != style->visibility()) {
                RenderLayer* l = enclosingLayer(); 
                if (style->visibility() == VISIBLE && l)
                    l->setHasVisibleContent(true);
                else if (l && l->hasVisibleContent() && 
                    (this == l->renderer() || l->renderer()->style()->visibility() != VISIBLE))
                    l->dirtyVisibleContentStatus();
            }            
        }

        // reset style flags
        m_floating = false;
        m_positioned = false;
        m_relPositioned = false;
        m_paintBackground = false;
        m_hasOverflowClip = false;
    }

    // only honor z-index for non-static objects and objects with opacity
    if ( style->position() == PSTATIC && style->opacity() == 1.0f ) {
        style->setHasAutoZIndex();
    }
    // force establishment of a stacking context by transparent objects, as those define
    // the bounds of an atomically painted region.
    if (style->hasAutoZIndex() && (isRoot() || style->opacity() < 1.0f))
        style->setZIndex( 0 );

    if ( d > RenderStyle::Position &&
         (style->hasFixedBackgroundImage() != (m_style && m_style->hasFixedBackgroundImage())
            || (style->position() == PFIXED) != (m_style && (m_style->position() == PFIXED)))
            && canvas() && canvas()->view() ) {
       // some sort of fixed object is added or removed. Let's find out more and report to the canvas,
       // so that it does some bookkeeping and optimizes the view's background display mode accordingly.
       bool fixedBG = style->hasFixedBackgroundImage();
       bool oldFixedBG = m_style && m_style->hasFixedBackgroundImage();
       bool fixedPos = (style->position() == PFIXED);
       bool oldFixedPos = m_style && (m_style->position() == PFIXED);
       if (fixedBG != oldFixedBG) {
           if (fixedBG) {
               canvas()->addStaticObject(this);
           } else {
               canvas()->removeStaticObject(this);
           }
       }
       if (fixedPos != oldFixedPos) {
           if (fixedPos)
               canvas()->addStaticObject( this, true /*positioned*/ );
           else
               canvas()->removeStaticObject( this, true );
       }
    }

    RenderStyle *oldStyle = m_style;
    m_style = style;

    updateBackgroundImages(oldStyle);

        m_style->ref();

    if (oldStyle)
        oldStyle->deref();

    setShouldPaintBackgroundOrBorder(m_style->hasBorder() || m_style->hasBackground());

    m_hasFirstLine = (style->getPseudoStyle(RenderStyle::FIRST_LINE) != 0);
    if (m_parent) {
        if (d == RenderStyle::Position && !attemptDirectLayerTranslation())
            d = RenderStyle::Layout;

        if ( d > RenderStyle::Position) {
            // we must perform a full layout
            if (!isText() && d == RenderStyle::CbLayout) {
                dirtyFormattingContext( true );
            }
            setNeedsLayoutAndMinMaxRecalc();
        } else if (!isText() && d >= RenderStyle::Visible) {
            // a repaint is enough
            if (layer()) {
                if (canvas() && canvas()->needsWidgetMasks()) {
                    // update our widget masks
                    RenderLayer *p, *d = 0;
                    for (p=layer()->parent();p;p=p->parent())
                        if (p->hasOverlaidWidgets()) d=p;
                    if (d) // deepest
                        d->updateWidgetMasks( canvas()->layer() );
                }
            }
            if (layer() && !isInlineFlow())
                layer()->repaint(pri);
            else
                repaint(pri);
        }
    }
}

bool RenderObject::attemptDirectLayerTranslation()
{
    // When the difference between two successive styles is only 'Position'
    // we may attempt to save a layout by directly updating the object position.

    KHTMLAssert( m_style->position() != PSTATIC );
    if (!layer())
        return false;
    setInline(m_style->isDisplayInlineType());
    setPositioned(m_style->position() != PRELATIVE);
    setRelPositioned(m_style->position() == PRELATIVE);
    int oldXPos = xPos();
    int oldYPos = yPos();
    int oldWidth = width();
    int oldHeight = height();
    calcWidth();
    calcHeight();
    if (oldWidth != width() || oldHeight != height()) {
        // implicit size change or overconstrained dimensions:
        // we'll need a layout.
        setWidth(oldWidth);
        setHeight(oldHeight);
        // qDebug() << "Layer translation failed for " << information();
        return false;
    }
    layer()->updateLayerPosition();
    if (m_style->position() != PFIXED) {
        bool needsDocSizeUpdate = true;
        RenderObject *cb = container();
        while (cb) {
            if (cb->hasOverflowClip() && cb->layer()) {
                cb->layer()->checkScrollbarsAfterLayout();
                needsDocSizeUpdate = false;
                break;
            }
            cb = cb->container();
        }
        if (needsDocSizeUpdate && canvas()) {
            bool posXOffset = (xPos()-oldXPos >= 0);
            bool posYOffset = (yPos()-oldYPos >= 0);
            canvas()->updateDocSizeAfterLayerTranslation(this, posXOffset, posYOffset);
        }
    }
    // success
    return true;
}

void RenderObject::dirtyFormattingContext( bool checkContainer )
{
    if (m_markedForRepaint && !checkContainer)
        return;
    m_markedForRepaint = true;
    if (layer() && (style()->position() == PFIXED || style()->position() == PABSOLUTE))
        return;
    if (m_parent && (checkContainer || style()->width().isAuto() || style()->height().isAuto() ||
                    !(isFloating() || flowAroundFloats() || isTableCell())))
        m_parent->dirtyFormattingContext(false);
}

void RenderObject::repaintDuringLayout()
{
    if (canvas()->needsFullRepaint() || isText())
        return;
    if (layer() && !isInlineFlow()) {
        layer()->repaint( NormalPriority, true );
    } else {
       repaint();
       canvas()->deferredRepaint( this );
    }
}

void RenderObject::updateBackgroundImages(RenderStyle* oldStyle)
{
    // FIXME: This will be slow when a large number of images is used.  Fix by using a dict.
    const BackgroundLayer* oldLayers = oldStyle ? oldStyle->backgroundLayers() : 0;
    const BackgroundLayer* newLayers = m_style ? m_style->backgroundLayers() : 0;
    for (const BackgroundLayer* currOld = oldLayers; currOld; currOld = currOld->next()) {
        if (currOld->backgroundImage() && (!newLayers || !newLayers->containsImage(currOld->backgroundImage())))
            currOld->backgroundImage()->deref(this);
    }
    for (const BackgroundLayer* currNew = newLayers; currNew; currNew = currNew->next()) {
        if (currNew->backgroundImage() && (!oldLayers || !oldLayers->containsImage(currNew->backgroundImage())))
            currNew->backgroundImage()->ref(this);
    }
}

QRect RenderObject::viewRect() const
{
    return containingBlock()->viewRect();
}

bool RenderObject::absolutePosition(int &xPos, int &yPos, bool f) const
{
    RenderObject* p = parent();
    if (p) {
        p->absolutePosition(xPos, yPos, f);
        if ( p->hasOverflowClip() )
            p->layer()->subtractScrollOffset( xPos, yPos );
        return true;
    }
    else
    {
        xPos = yPos = 0;
        return false;
    }
}

void RenderObject::caretPos(int /*offset*/, int /*flags*/, int &_x, int &_y, int &width, int &height) const
{
    _x = _y = height = -1;
    width = 1;        // the caret has a default width of one pixel. If you want
                    // to check for validity, only test the x-coordinate for >= 0.
}

int RenderObject::paddingTop() const
{
    int w = 0;
    Length padding = m_style->paddingTop();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.minWidth(w);
    if ( isTableCell() && padding.isAuto() )
        w = static_cast<const RenderTableCell *>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingBottom() const
{
    int w = 0;
    Length padding = style()->paddingBottom();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.minWidth(w);
    if ( isTableCell() && padding.isAuto() )
        w = static_cast<const RenderTableCell *>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingLeft() const
{
    int w = 0;
    Length padding = style()->paddingLeft();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.minWidth(w);
    if ( isTableCell() && padding.isAuto() )
        w = static_cast<const RenderTableCell *>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingRight() const
{
    int w = 0;
    Length padding = style()->paddingRight();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.minWidth(w);
    if ( isTableCell() && padding.isAuto() )
        w = static_cast<const RenderTableCell *>(this)->table()->cellPadding();
    return w;
}

RenderObject *RenderObject::container() const
{
    // This method is extremely similar to containingBlock(), but with a few notable
    // exceptions.
    // (1) It can be used on orphaned subtrees, i.e., it can be called safely even when
    // the object is not part of the primary document subtree yet.
    // (2) For normal flow elements, it just returns the parent.
    // (3) For absolute positioned elements, it will return a relative positioned inline.
    // containingBlock() simply skips relpositioned inlines and lets an enclosing block handle
    // the layout of the positioned object.  This does mean that calcAbsoluteHorizontal and
    // calcAbsoluteVertical have to use container().
    EPosition pos = m_style->position();
    RenderObject *o = 0;
    if( pos == PFIXED ) {
        // container() can be called on an object that is not in the
        // tree yet.  We don't call canvas() since it will assert if it
        // can't get back to the canvas.  Instead we just walk as high up
        // as we can.  If we're in the tree, we'll get the root.  If we
        // aren't we'll get the root of our little subtree (most likely
        // we'll just return 0).
        o = parent();
        while ( o && o->parent() ) o = o->parent();
    }
    else if ( pos == PABSOLUTE ) {
        // Same goes here.  We technically just want our containing block, but
        // we may not have one if we're part of an uninstalled subtree.  We'll
        // climb as high as we can though.
        o = parent();
        while (o && o->style()->position() == PSTATIC && !o->isCanvas())
            o = o->parent();
    }
    else
        o = parent();
    return o;
}

DOM::DocumentImpl* RenderObject::document() const
{
    return m_node->document();
}

void RenderObject::removeFromObjectLists()
{
    // in destruction mode, don't care.
    if ( documentBeingDestroyed() ) return;

    if (isFloating()) {
        RenderBlock* outermostBlock = containingBlock();
        for (RenderBlock* p = outermostBlock; p && !p->isCanvas() && p->containsFloat(this);) {
            outermostBlock = p;
            if (p->isFloatingOrPositioned())
                break;
            p = p->containingBlock();
        }

        if (outermostBlock)
            outermostBlock->markAllDescendantsWithFloatsForLayout(this);

        RenderObject *p;
        for (p = parent(); p; p = p->parent()) {
            if (p->isRenderBlock())
                static_cast<RenderBlock*>(p)->removeFloatingObject(this);
        }

    }

    if (inPosObjectList()) {
        RenderObject *p;
        for (p = parent(); p; p = p->parent()) {
            if (p->isRenderBlock())
                static_cast<RenderBlock*>(p)->removePositionedObject(this);
        }
    }
}

RenderArena* RenderObject::renderArena() const
{
    return m_node->document()->renderArena();
}

void RenderObject::detach()
{
    detachCounters();
    remove();

    // make sure our DOM-node don't think we exist
    if ( node() && node()->renderer() == this)
        node()->setRenderer(0);

    // by default no refcounting
    arenaDelete(renderArena(), this);
}

void RenderObject::remove()
{
    if (m_parent) {
        m_parent->removeChild(this);
        if (isFloating() || inPosObjectList())
            removeFromObjectLists();
    }
}

void RenderObject::arenaDelete(RenderArena *arena, void *base)
{
#ifndef NDEBUG
    void *savedBase = baseOfRenderObjectBeingDeleted;
    baseOfRenderObjectBeingDeleted = base;
#endif
    delete this;
#ifndef NDEBUG
    baseOfRenderObjectBeingDeleted = savedBase;
#endif

    // Recover the size left there for us by operator delete and free the memory.
    arena->free(*(size_t *)base, base);
}

void RenderObject::arenaDelete(RenderArena *arena)
{
    // static_cast unfortunately doesn't work, since we multiple inherit
    // in eg. RenderWidget.
    arenaDelete(arena, dynamic_cast<void *>(this));
}

RenderPosition RenderObject::positionForCoordinates(int /*x*/, int /*y*/)
{
    return RenderPosition(element(), caretMinOffset());
}

bool RenderObject::isPointInsideSelection(int x, int y, const Selection &sel) const
{
    SelectionState selstate = selectionState();
    if (selstate == SelectionInside) return true;
    if (selstate == SelectionNone || !element()) return false;
    return element()->isPointInsideSelection(x, y, sel);
}

#if 0
FindSelectionResult RenderObject::checkSelectionPoint( int _x, int _y, int _tx, int _ty, DOM::NodeImpl*& node, int & offset, SelPointState &state )
{
#if 0
    NodeInfo info(true, false);
    if ( nodeAtPoint( info, _x, _y, _tx, _ty ) && info.innerNode() )
    {
        RenderObject* r = info.innerNode()->renderer();
        if ( r ) {
            if ( r == this ) {
                node = info.innerNode();
                offset = 0; // we have no text...
                return SelectionPointInside;
            }
            else
                return r->checkSelectionPoint( _x, _y, _tx, _ty, node, offset, state );
        }
    }
    //qDebug() << "nodeAtPoint Failed. Fallback - hmm, SelectionPointAfter";
    node = 0;
    offset = 0;
    return SelectionPointAfter;
#endif
    int off = offset;
    DOM::NodeImpl* nod = node;

    for (RenderObject *child = firstChild(); child; child=child->nextSibling()) {
        // ignore empty text boxes, they produce totally bogus information
        // for caret navigation (LS)
        if (child->isText() && !static_cast<RenderText *>(child)->firstTextBox())
            continue;

//        qDebug() << "iterating " << (child ? child->renderName() : "") << "@" << child << (child->isText() ? " contains: \"" + QString::fromRawData(static_cast<RenderText *>(child)->text(), qMin(static_cast<RenderText *>(child)->length(), 10u)) + "\"" : QString());
//        qDebug() << "---------- checkSelectionPoint recursive -----------";
        khtml::FindSelectionResult pos = child->checkSelectionPoint(_x, _y, _tx+xPos(), _ty+yPos(), nod, off, state);
//        qDebug() << "-------- end checkSelectionPoint recursive ---------";
//        qDebug() << this << " child->findSelectionNode returned result=" << pos << " nod=" << nod << " off=" << off;
        switch(pos) {
        case SelectionPointBeforeInLine:
        case SelectionPointInside:
            //qDebug() << "RenderObject::checkSelectionPoint " << this << " returning SelectionPointInside offset=" << offset;
            node = nod;
            offset = off;
            return SelectionPointInside;
        case SelectionPointBefore:
            //x,y is before this element -> stop here
            if ( state.m_lastNode ) {
                node = state.m_lastNode;
                offset = state.m_lastOffset;
                //qDebug() << "RenderObject::checkSelectionPoint " << this << " before this child "
                //              << node << "-> returning SelectionPointInside, offset=" << offset << endl;
                return SelectionPointInside;
            } else {
                node = nod;
                offset = off;
                //qDebug() << "RenderObject::checkSelectionPoint " << this << " before us -> returning SelectionPointBefore " << node << "/" << offset;
                return SelectionPointBefore;
            }
            break;
        case SelectionPointAfter:
            if (state.m_afterInLine) break;
            // fall through
        case SelectionPointAfterInLine:
            if (pos == SelectionPointAfterInLine) state.m_afterInLine = true;
            //qDebug() << "RenderObject::checkSelectionPoint: selection after: " << nod << " offset: " << off << " afterInLine: " << state.m_afterInLine;
            state.m_lastNode = nod;
            state.m_lastOffset = off;
            // No "return" here, obviously. We must keep looking into the children.
            break;
        }
    }
    // If we are after the last child, return lastNode/lastOffset
    // But lastNode can be 0L if there is no child, for instance.
    if ( state.m_lastNode )
    {
        node = state.m_lastNode;
        offset = state.m_lastOffset;
    }
    //qDebug() << "fallback - SelectionPointAfter  node=" << node << " offset=" << offset;
    return SelectionPointAfter;
}
#endif

bool RenderObject::mouseInside() const
{
    if (!m_mouseInside && continuation())
        return continuation()->mouseInside();
    return m_mouseInside;
}

bool RenderObject::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction, bool inside)
{
    int tx = _tx + xPos();
    int ty = _ty + yPos();

    inside |= ( style()->visibility() != HIDDEN &&
                (_y >= ty) && (_y < ty + height()) && (_x >= tx) && (_x < tx + width())) || isRoot() || isBody();
    bool inOverflowRect = inside;
    if ( !inOverflowRect ) {
        int ol = overflowLeft();
        int ot = overflowTop();
        QRect overflowRect( tx+ol, ty+ot, overflowWidth()-ol, overflowHeight()-ot );
        inOverflowRect = overflowRect.contains( _x, _y );
    }

    // ### table should have its own, more performant method
    if (hitTestAction != HitTestSelfOnly &&
        (( !isRenderBlock() ||
           !static_cast<RenderBlock*>( this )->isPointInScrollbar( _x, _y, _tx, _ty )) &&
        (inOverflowRect || isInline() || isRoot() || isCanvas() ||
        isTableRow() || isTableSection() || inside || mouseInside() ))) {
        if ( hitTestAction == HitTestChildrenOnly )
            inside = false;
        if ( hasOverflowClip() && layer() )
            layer()->subtractScrollOffset(tx, ty);
        for (RenderObject* child = lastChild(); child; child = child->previousSibling())
            if (!child->layer() && child->nodeAtPoint(info, _x, _y, tx, ty, HitTestAll))
                inside = true;
    }

    if (inside)
        setInnerNode(info);

    return inside;
}


void RenderObject::setInnerNode(NodeInfo& info)
{
    if (!info.innerNode() && !isInline() && continuation()) {
        // We are in the margins of block elements that are part of a continuation.  In
        // this case we're actually still inside the enclosing inline element that was
        // split.  Go ahead and set our inner node accordingly.
        info.setInnerNode(continuation()->element());
        if (!info.innerNonSharedNode())
            info.setInnerNonSharedNode(continuation()->element());
    }

    if (!info.innerNode() && element())
        info.setInnerNode(element());

    if(!info.innerNonSharedNode() && element())
        info.setInnerNonSharedNode(element());
}


short RenderObject::verticalPositionHint( bool firstLine ) const
{
    short vpos = m_verticalPosition;
    if ( m_verticalPosition == PositionUndefined || firstLine ) {
        vpos = getVerticalPosition( firstLine );
        if ( !firstLine )
            const_cast<RenderObject *>(this)->m_verticalPosition = vpos;
    }
    return vpos;

}

short RenderObject::getVerticalPosition( bool firstLine, RenderObject* ref ) const
{
    // vertical align for table cells has a different meaning
    int vpos = 0;
    if ( !isTableCell() && isInline() ) {
        EVerticalAlign va = style()->verticalAlign();
        if ( va == TOP ) {
            vpos = PositionTop;
        } else if ( va == BOTTOM ) {
            vpos = PositionBottom;
        } else {
            if (!ref) ref = parent();
            bool checkParent = ref->isInline() && !ref->isReplacedBlock() &&
                               !( ref->style()->verticalAlign() == TOP || ref->style()->verticalAlign() == BOTTOM );
            vpos = checkParent ? ref->verticalPositionHint( firstLine ) : 0;
            // don't allow elements nested inside text-top to have a different valignment.
            if ( va == BASELINE )
                return vpos;
            else if ( va == LENGTH )
                return vpos - style()->verticalAlignLength().width( lineHeight( firstLine ) );

            const QFont &f = ref->font( firstLine );
            int fontsize = f.pixelSize();

            if ( va == SUB )
                vpos += fontsize/5 + 1;
            else if ( va == SUPER )
                vpos -= fontsize/3 + 1;
            else if ( va == TEXT_TOP ) {
                vpos += baselinePosition( firstLine ) - (QFontMetrics(f).ascent() + QFontMetrics(f).leading()/2);
            } else if ( va == MIDDLE ) {
                QRect b = QFontMetrics(f).boundingRect('x');
                vpos += -b.height()/2 - lineHeight( firstLine )/2 + baselinePosition( firstLine );
            } else if ( va == TEXT_BOTTOM ) {
                vpos += QFontMetrics(f).descent() + QFontMetrics(f).leading()/2;
                if ( !isReplaced() ) {
                    vpos -= (lineHeight(firstLine) - baselinePosition(firstLine));
                }
            } else if ( va == BASELINE_MIDDLE )
                vpos += - lineHeight( firstLine )/2 + baselinePosition( firstLine );
        }
    }
    return vpos;
}

short RenderObject::lineHeight( bool firstLine ) const
{
    // Inline blocks are replaced elements. Otherwise, just pass off to
    // the base class.  If we're being queried as though we're the root line
    // box, then the fact that we're an inline-block is irrelevant, and we behave
    // just like a block.

    if (isReplaced() && (!isInlineBlockOrInlineTable() || !needsLayout()))
        return height()+marginTop()+marginBottom();

    Length lh;
    if( firstLine && hasFirstLine() ) {
        RenderStyle *pseudoStyle  = style()->getPseudoStyle(RenderStyle::FIRST_LINE);
        if ( pseudoStyle )
            lh = pseudoStyle->lineHeight();
    }
    else
        lh = style()->lineHeight();

    // its "unset", choose nice default
    if ( lh.isNegative() )
        return style()->htmlFont().lineSpacing();

    if ( lh.isPercent() )
        return lh.minWidth( style()->font().pixelSize() );

    // its fixed
    return lh.value();
}

short RenderObject::baselinePosition( bool firstLine ) const
{
    // If we're an inline-block and need layout, it means our replaced boundaries
    // are not yet fully established, so we behave just like a block.
    if (isReplaced() && (!isInlineBlockOrInlineTable() || !needsLayout()))
        return height()+marginTop()+marginBottom();

    const QFontMetrics &fm = fontMetrics( firstLine );
    return fm.ascent() + ( lineHeight( firstLine) - fm.height() ) / 2;
}

void RenderObject::invalidateVerticalPosition()
{
    m_verticalPosition = PositionUndefined;
}

void RenderObject::recalcMinMaxWidths()
{
    KHTMLAssert( m_recalcMinMax );

#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << " recalcMinMaxWidths() this=" << this;
#endif

    RenderObject *child = firstChild();
    int cmin=0;
    int cmax=0;

    while( child ) {
        bool test = false;
        if ( ( m_minMaxKnown && child->m_recalcMinMax ) || !child->m_minMaxKnown ) {
            cmin = child->minWidth();
            cmax = child->maxWidth();
            test = true;
        }
        if ( child->m_recalcMinMax )
            child->recalcMinMaxWidths();
        if ( !child->m_minMaxKnown )
            child->calcMinMaxWidth();
        if ( m_minMaxKnown && test && (cmin != child->minWidth() || cmax != child->maxWidth()) )
            m_minMaxKnown = false;
        child = child->nextSibling();
    }

    // we need to recalculate, if the contains inline children, as the change could have
    // happened somewhere deep inside the child tree
    if ( ( !isInline() || isReplacedBlock() ) && childrenInline() )
        m_minMaxKnown = false;

    if ( !m_minMaxKnown )
        calcMinMaxWidth();
    m_recalcMinMax = false;
}

void RenderObject::scheduleRelayout(RenderObject *clippedObj)
{
    if (!isCanvas()) return;
    KHTMLView *view = static_cast<RenderCanvas *>(this)->view();
    if ( view )
        view->scheduleRelayout(clippedObj);
}

InlineBox* RenderObject::createInlineBox(bool /*makePlaceHolderBox*/, bool /*isRootLineBox*/)
{
    KHTMLAssert(false);
    return 0;
}

void RenderObject::getTextDecorationColors(int decorations, QColor& underline, QColor& overline,
                                           QColor& linethrough, bool quirksMode)
{
    RenderObject* curr = this;
    do {
        RenderStyle *st = curr->style();
        int currDecs = st->textDecoration();
        if (currDecs) {
            if (currDecs & UNDERLINE) {
                decorations &= ~UNDERLINE;
                underline = st->color();
            }
            if (currDecs & OVERLINE) {
                decorations &= ~OVERLINE;
                overline = st->color();
            }
            if (currDecs & LINE_THROUGH) {
                decorations &= ~LINE_THROUGH;
                linethrough = st->color();
            }
        }
        curr = curr->parent();
        if (curr && curr->isRenderBlock() && curr->continuation())
            curr = curr->continuation();
    } while (curr && decorations && (!quirksMode || !curr->element() ||
                                     (curr->element()->id() != ID_A && curr->element()->id() != ID_FONT)));

    // If we bailed out, use the element we bailed out at (typically a <font> or <a> element).
    if (decorations && curr) {
        RenderStyle *st = curr->style();
        if (decorations & UNDERLINE)
            underline = st->color();
        if (decorations & OVERLINE)
            overline = st->color();
        if (decorations & LINE_THROUGH)
            linethrough = st->color();
    }
}

int RenderObject::maximalOutlineSize(PaintAction p) const
{
    if (p != PaintActionOutline)
        return 0;
    return static_cast<RenderCanvas*>(document()->renderer())->maximalOutlineSize();
}

void RenderObject::collectBorders(QList<CollapsedBorderValue>& borderStyles)
{
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->collectBorders(borderStyles);
}

bool RenderObject::flowAroundFloats() const
{
    return isReplaced() || hasOverflowClip() || style()->flowAroundFloats();
}

bool RenderObject::usesLineWidth() const
{
    // All auto-width objects that avoid floats should always use lineWidth
    // unless they are floating or inline. We only care about objects that grow
    // to fill the available space.
    return (!isInline()||isHTMLMarquee()) && flowAroundFloats() && style()->width().isAuto() && !isFloating();
}

long RenderObject::caretMinOffset() const
{
    return 0;
}

long RenderObject::caretMaxOffset() const
{
    return 0;
}

unsigned long RenderObject::caretMaxRenderedOffset() const
{
    return 0;
}

InlineBox *RenderObject::inlineBox(long /*offset*/)
{
    if (isBox())
        return static_cast<RenderBox*>(this)->placeHolderBox();
    return 0;
}

bool RenderObject::hasCounter(const DOMString& counter) const
{
    if (style() && (!isText() || isCounter())) {
        if (lookupCounter(counter)) return true;
        if (style()->hasCounterReset(counter)) {
            return true;
        }
        else if (style()->hasCounterIncrement(counter)) {
            return true;
        }
    }
    if (counter == "list-item") {
        if (isListItem()) return true;
        if (element() && (
                element()->id() == ID_OL ||
                element()->id() == ID_UL ||
                element()->id() == ID_MENU ||
                element()->id() == ID_DIR))
            return true;
    } else
    if (counter == "-khtml-quotes" && isQuote()) {
        return (static_cast<const RenderQuote*>(this)->quoteCount() != 0);
    }
    return false;
}

CounterNode* RenderObject::getCounter(const DOMString& counter, bool view, bool counters)
{
//     qDebug() << renderName() << " getCounter(" << counter << ")";

    if (!style()) return 0;

    if (isText() && !isCounter()) return 0;

    CounterNode *i = lookupCounter(counter);
    if (i) return i;
    int val = 0;

    if (style()->hasCounterReset(counter) || isRoot()) {
        i = new CounterReset(this);
        val = style()->counterReset(counter);
        if (style()->hasCounterIncrement(counter)) {
            val += style()->counterIncrement(counter);
        }
//         qDebug() << renderName() << " counter-reset: " << counter << " " << val;
    }
    else
    if (style()->hasCounterIncrement(counter)) {
        i = new CounterNode(this);
        val = style()->counterIncrement(counter);
//         qDebug() << renderName() << " counter-increment: " << counter << " " << val;
    }
    else if (counter == "list-item") {
        if (isListItem()) {
            if (element() && element()->id() == ID_LI) {
                DOMString v = static_cast<ElementImpl*>(element())->getAttribute(ATTR_VALUE);
                if ( !v.isEmpty() ) {
                    i = new CounterReset(this);
                    val = v.toInt();
//                     qDebug() << renderName() << " counter-reset: " << counter << " " << val;
                }
            }
            if (!i) {
                i = new CounterNode(this);
                val = 1;
//                 qDebug() << renderName() << " counter-increment: " << counter << " " << val;
            }
        }
        else
        if (element() && element()->id() == ID_OL) {
            i = new CounterReset(this);
            DOMString v = static_cast<ElementImpl*>(element())->getAttribute(ATTR_START);
            if ( !v.isEmpty() )
                val = v.toInt()-1;
            else
                val = 0;
//             qDebug() << renderName() << " counter-reset: " << counter << " " << val;
        }
        else
        if (element() &&
            (element()->id() == ID_UL ||
             element()->id() == ID_MENU||
             element()->id() == ID_DIR))
        {
            i = new CounterReset(this);
            val = 0;
//             qDebug() << renderName() << " counter-reset: " << counter << " " << val;
        }
    }
    else if (counter == "-khtml-quotes" && isQuote()) {
        i = new CounterNode(this);
        val = static_cast<RenderQuote*>(this)->quoteCount();
    }

    if (!i) {
        i = new CounterNode(this);
        val = 0;
//         qDebug() << renderName() << " counter-increment: " << counter << " " << val;
    }
    i->setValue(val);
    if (view) i->setIsVisual();
    if (counters) i->setHasCounters();

    insertCounter(counter, i);

    if (!isRoot()) {
        CounterNode *last=0, *current=0;
        RenderObject *n = previousSibling();
        while(n) {
            if (n->hasCounter(counter)) {
                current = n->getCounter(counter);
                break;
            }
            else
                n = n->previousSibling();
        }
        last = current;

        CounterNode *sibling = current;
        // counter-reset on same render-level is our counter-parent
        if (last) {
            // Found render-sibling, now search for later counter-siblings among its render-children
            n = n->lastChild();
            while (n) {
                if (n->hasCounter(counter)) {
                    current = n->getCounter(counter);
                    if (last->parent() == current->parent() || sibling == current->parent()) {
                        last = current;
                        // If the current counter is not the last, search deeper
                        if (current->nextSibling()) {
                            n = n->lastChild();
                            continue;
                        }
                        else
                            break;
                    }
                }
                n = n->previousSibling();
            }
            if (sibling->isReset())
            {
                if (last != sibling)
                    sibling->insertAfter(i, last);
                else
                    sibling->insertAfter(i, 0);
            }
            else if (last->parent())
                last->parent()->insertAfter(i, last);
        }
        else if (parent()) {
            // Nothing found among siblings, let our parent search
            last = parent()->getCounter(counter, false);
            if (last->isReset())
                last->insertAfter(i, 0);
            else if (last->parent())
                last->parent()->insertAfter(i, last);
        }
    }

    return i;
}

CounterNode* RenderObject::lookupCounter(const DOMString& counter) const
{
    QHash<DOMString,khtml::CounterNode*>* counters = document()->counters(this);
    return counters ? counters->value(counter) : 0;
}

void RenderObject::detachCounters()
{
    QHash<DOMString,khtml::CounterNode*>* counters = document()->counters(this);
    if (!counters) return;

    QHashIterator<DOMString,khtml::CounterNode*> i(*counters);

    while (i.hasNext()) {
        i.next();
        i.value()->remove();
        delete i.value();
    }
    document()->removeCounters(this);
}

void RenderObject::insertCounter(const DOMString& counter, CounterNode* val)
{
    QHash<DOMString,khtml::CounterNode*>* counters = document()->counters(this);

    if (!counters) {
        counters = new QHash<DOMString,khtml::CounterNode*>();
        document()->setCounters(this, counters);
    }

    counters->insert(counter, val);
}

void RenderObject::updateWidgetMasks() {
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if ( curr->isWidget() && static_cast<RenderWidget*>(curr)->needsMask() ) {
            QWidget* w = static_cast<RenderWidget*>(curr)->widget();
            if (!w)
                return;
            RenderLayer* l = curr->enclosingStackingContext();
            QRegion r = l ? l->getMask() : QRegion();
            int x,y;
            if (!r.isEmpty() && curr->absolutePosition(x,y)) {
                int pbx = curr->borderLeft()+curr->paddingLeft();
                int pby = curr->borderTop()+curr->paddingTop();
                x+= pbx;
                y+= pby;
                r = r.intersect(QRect(x,y,
                                  curr->width()-pbx-curr->borderRight()-curr->paddingRight(),
                                  curr->height()-pby-curr->borderBottom()-curr->paddingBottom()));
#ifdef MASK_DEBUG
                QVector<QRect> ar = r.rects();
                qDebug() << "|| Setting widget mask for " << curr->information();
                for (int i = 0; i < ar.size() ; ++i) {
                    qDebug() << "		" <<  ar[i];
                }
#endif
                r.translate(-x,-y);

                // ### Scrollarea's widget doesn't update when mask change.
                // Might be a Qt bug. Might be the way we handle updates. Investigate.
                if (::qobject_cast<QScrollArea*>(w)) {
                    QScrollArea* sa = static_cast<QScrollArea*>(w);
                    if (!w->mask().isEmpty()) {
                      QPoint off( sa->horizontalScrollBar()->value(),
                                  sa->verticalScrollBar()->value() );
                      sa->widget()->update(w->mask().translated(off));
                      sa->horizontalScrollBar()->update();
                      sa->verticalScrollBar()->update();
                    }
                }
                w->setMask(r);
            } else {
                w->clearMask();
            }
        }
        else if (!curr->layer() || !curr->layer()->isStackingContext())
            curr->updateWidgetMasks();

    }
}

QRegion RenderObject::visibleFlowRegion(int x, int y) const
{
    QRegion r;
    bool returnSelf = false;
    for (RenderObject* ro=firstChild();ro;ro=ro->nextSibling()) {
        if( !ro->layer() && !ro->isFloating() && ro->style()->visibility() == VISIBLE) {
            const RenderStyle *s = ro->style();
            int ow = s->outlineSize();
            if (ro->isInlineFlow() || ro->isText()) {
                returnSelf = true;
                break;
            }
            if ( s->backgroundImage() || s->backgroundColor().isValid() || s->hasBorder() || ro->isReplaced() || ow ) {
                r += QRect(x -ow +ro->effectiveXPos(),y -ow + ro->effectiveYPos(), 
                                  ro->effectiveWidth()+ow*2, ro->effectiveHeight()+ow*2);
            } else {
                r += ro->visibleFlowRegion(x+ro->xPos(), y+ro->yPos());
            }
        }
    }
    if (hasFloats()) {
        r += static_cast<const RenderBlock*>(this)->visibleFloatingRegion(x, y);
    }
    if (returnSelf) {
        int ow = style()->outlineSize();
        r+= QRect(x-xPos()-ow +effectiveXPos(),y-yPos()-ow + effectiveYPos(),
                               effectiveWidth()+ow*2, effectiveHeight()+ow*2);
    }
    return r;
}

// SVG
FloatRect RenderObject::relativeBBox(bool includeStroke) const
{
    Q_UNUSED(includeStroke);
    return FloatRect();
}

AffineTransform RenderObject::localTransform() const
{
    return AffineTransform(1, 0, 0, 1, xPos(), yPos());
}

AffineTransform RenderObject::absoluteTransform() const
{
    if (parent())
        return localTransform() * parent()->absoluteTransform();
    return localTransform();
}
// END SVG

#undef RED_LUMINOSITY
#undef GREEN_LUMINOSITY
#undef BLUE_LUMINOSITY
#undef INTENSITY_FACTOR
#undef LIGHT_FACTOR
#undef LUMINOSITY_FACTOR

#undef MAX_COLOR
#undef COLOR_DARK_THRESHOLD
#undef COLOR_LIGHT_THRESHOLD

#undef COLOR_LITE_BS_FACTOR
#undef COLOR_LITE_TS_FACTOR

#undef COLOR_DARK_BS_FACTOR
#undef COLOR_DARK_TS_FACTOR

#undef LIGHT_GRAY
#undef DARK_GRAY

