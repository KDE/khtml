/*
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2000-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2002-2003 Apple Computer, Inc.
 *           (C) 2004 Allan Sandfeld Jensen
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
#ifndef render_object_h
#define render_object_h

#include <QColor>
#include <QtCore/QRect>
#include <assert.h>
#include <QList>

#include <QDebug>

#include "xml/dom_docimpl.h"
#include "misc/khtmllayout.h"
#include "misc/loader_client.h"
#include "misc/helper.h"
#include "rendering/render_style.h"
#include "rendering/render_position.h"
#include <QtCore/QTextStream>

// svg
#include "FloatRect.h"
#include "AffineTransform.h"

using WebCore::FloatRect;
using WebCore::AffineTransform;

class QPainter;
class QTextStream;
template<class Key, class T> class QCache;

#ifndef NDEBUG
#define KHTMLAssert( x ) if( !(x) ) { \
    const RenderObject *o = this; while( o->parent() ) o = o->parent(); \
    o->printTree(); \
    qDebug(" this object = %p", (void*) this); \
    assert( x ); \
}
#else
#define KHTMLAssert( x )
#endif

/*
 *	The painting of a layer occurs in three distinct phases.  Each phase involves
 *	a recursive descent into the layer's render objects. The first phase is the background phase.
 *	The backgrounds and borders of all blocks are painted.  Inlines are not painted at all.
 *	Floats must paint above block backgrounds but entirely below inline content that can overlap them.
 *	In the foreground phase, all inlines are fully painted.  Inline replaced elements will get all
 *	three phases invoked on them during this phase.
 */

typedef enum {
    PaintActionElementBackground = 0,
    PaintActionChildBackground,
    PaintActionChildBackgrounds,
    PaintActionFloat,
    PaintActionForeground,
    PaintActionOutline,
    PaintActionSelection,
    PaintActionCollapsedTableBorders
} PaintAction;

typedef enum {
    HitTestAll = 0,
    HitTestSelfOnly = 1,
    HitTestChildrenOnly = 2
} HitTestAction;

typedef enum {
    PageBreakNormal = 0, // all rules apply
    PageBreakHarder = 1, // page-break-inside: avoid is ignored
    PageBreakForced = 2  // page-break-after/before: avoid, orphans and widows ignored
} PageBreakLevel;

typedef enum {
    LowPriority = 0,
    NormalPriority = 1,
    HighPriority = 2,
    RealtimePriority = 3
} Priority;

inline PageBreakLevel operator| (PageBreakLevel a, PageBreakLevel b) {
    if (a == PageBreakForced || b == PageBreakForced)
        return PageBreakForced;
    if (a == PageBreakHarder || b == PageBreakHarder)
        return PageBreakHarder;
    return PageBreakNormal;
}

namespace DOM {
    class HTMLAreaElementImpl;
    class NodeImpl;
    class DocumentImpl;
    class ElementImpl;
    class EventImpl;
    class Selection;
}

namespace khtml {
    class RenderFlow;
    class RenderStyle;
    class CachedObject;
    class RenderObject;
    class RenderCanvas;
    class RenderText;
    class RenderFrameSet;
    class RenderArena;
    class RenderLayer;
    class RenderBlock;
    class InlineBox;
    class InlineFlowBox;
    class CounterNode;
    class RenderPosition;

/**
 * Base Class for all rendering tree objects.
 */
class RenderObject : public CachedObjectClient
{
    RenderObject(const RenderObject&);
    RenderObject& operator=(const RenderObject&);
public:
    KHTML_EXPORT static void cleanup();

    RenderObject(DOM::NodeImpl* node);
    virtual ~RenderObject();

    RenderObject *parent() const { return m_parent; }

    RenderObject *previousSibling() const { return m_previous; }
    RenderObject *nextSibling() const { return m_next; }

    virtual RenderObject *firstChild() const { return 0; }
    virtual RenderObject *lastChild() const { return 0; }

    RenderObject *nextRenderer() const;
    RenderObject *previousRenderer() const;

    RenderObject *nextEditable() const;
    RenderObject *previousEditable() const;

    RenderObject *firstLeafChild() const;
    RenderObject *lastLeafChild() const;

    virtual bool childAllowed() const { return false; }
    virtual int borderTopExtra() const { return 0; }
    virtual int borderBottomExtra() const { return 0; }

    virtual RenderLayer* layer() const { return 0; }
    RenderLayer* enclosingLayer() const;
    RenderLayer* enclosingStackingContext() const;
    void addLayers(RenderLayer* parentLayer, RenderObject* newObject);
    void removeLayers(RenderLayer* parentLayer);
    void moveLayers(RenderLayer* oldParent, RenderLayer* newParent);
    RenderLayer* findNextLayer(RenderLayer* parentLayer, RenderObject* startPoint,
                               bool checkParent=true);
    virtual void positionChildLayers() { }
    virtual bool requiresLayer() const {
        return isRoot()/* ### */ || isPositioned() || isRelPositioned() || hasOverflowClip() || style()->opacity() < 1.0f;
    }

    virtual QRect overflowClipRect(int /*tx*/, int /*ty*/)
	{ return QRect(0,0,0,0); }
    virtual QRect clipRect(int /*tx*/, int /*ty*/) { return QRect(0,0,0,0); }
    bool hasClip() const { return isPositioned() &&  style()->hasClip(); }
    bool hasOverflowClip() const { return m_hasOverflowClip; }

    bool scrollsOverflow() const { return scrollsOverflowX() || scrollsOverflowY(); }
    bool scrollsOverflowX() const { return  hasOverflowClip() && (style()->overflowX() == OSCROLL || style()->overflowX() == OAUTO); }
    bool scrollsOverflowY() const { return  hasOverflowClip() && (style()->overflowY() == OSCROLL || style()->overflowY() == OAUTO); }

    virtual int getBaselineOfFirstLineBox() { return -1; } // Tables and blocks implement this.
    virtual InlineFlowBox* getFirstLineBox() { return 0; } // Tables and blocks implement this.

    // Whether or not a positioned element requires normal flow x/y to be computed
    // to determine its position.
    bool hasStaticX() const { return style()->left().isAuto() && style()->right().isAuto(); }
    bool hasStaticY() const { return style()->top().isAuto() && style()->bottom().isAuto(); }
    bool isPosWithStaticDim()   const { return isPositioned() && (hasStaticX() || hasStaticY()); }

    // Linear tree traversal
    RenderObject *objectBelow() const;
    RenderObject *objectAbove() const;

    // Returns if an object has counter-increment or counter-reset
    bool hasCounter(const DOMString& counter) const;
    // Calculates the value of the counter
    CounterNode* getCounter(const DOMString& counter, bool view = false, bool counters = false);
    // Detaches all counterNodes
    void detachCounters();


protected:
    // Helper functions for counter-cache
    void insertCounter(const DOMString& counter, CounterNode* value);
    CounterNode* lookupCounter(const DOMString& counter) const;

public:
    //////////////////////////////////////////
    // RenderObject tree manipulation
    virtual void addChild(RenderObject *newChild, RenderObject *beforeChild = 0);
    virtual void removeChild(RenderObject *oldChild);

    // raw tree manipulation
    virtual RenderObject* removeChildNode(RenderObject* child);
    virtual void appendChildNode(RenderObject* child);
    virtual void insertChildNode(RenderObject* child, RenderObject* before);
    //////////////////////////////////////////

    //////////////////////////////////////////
    // Helper functions. Dangerous to use!
    void setPreviousSibling(RenderObject *previous) { m_previous = previous; }
    void setNextSibling(RenderObject *next) { m_next = next; }
    void setParent(RenderObject *parent) { m_parent = parent; }
    //////////////////////////////////////////

public:
    virtual const char *renderName() const { return "RenderObject"; }
#ifdef ENABLE_DUMP
    QString information() const;
    virtual void printTree(int indent=0) const;
    virtual void dump(QTextStream &stream, const QString &ind = QString()) const;
    void printLineBoxTree() const;
#endif

    static RenderObject *createObject(DOM::NodeImpl* node, RenderStyle* style);

    // Overloaded new operator.  Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void* operator new(size_t sz, RenderArena* renderArena) throw();

    // Overridden to prevent the normal delete from being called.
    void operator delete(void* ptr, size_t sz);

private:
    // The normal operator new is disallowed on all render objects.
    void* operator new(size_t sz);

public:
    RenderArena* renderArena() const;
    virtual RenderFlow* continuation() const { return 0; }
    virtual bool isInlineContinuation() const { return false; }


    bool isRoot() const { return m_isRoot && !m_isAnonymous; }
    void setIsRoot(bool b) { m_isRoot = b; }
    bool isHR() const;
    // some helper functions...
    virtual bool isRenderBlock() const { return false; }
    virtual bool isRenderInline() const { return false; }
    virtual bool isInlineFlow() const { return false; }
    virtual bool isBlockFlow() const { return false; }
    virtual bool isInlineBlockOrInlineTable() const { return false; }
    virtual bool childrenInline() const { return false; }
    virtual bool isBox() const { return false; }
    virtual bool isRenderReplaced() const { return false; }

    virtual bool isGlyph() const { return false; }
    virtual bool isCounter() const { return false; }
    virtual bool isQuote() const { return false; }
    virtual bool isListItem() const { return false; }
    virtual bool isListMarker() const { return false; }
    virtual bool isCanvas() const { return false; }
    virtual bool isBR() const { return false; }
    virtual bool isTableCell() const { return false; }
    virtual bool isTableRow() const { return false; }
    virtual bool isTableSection() const { return false; }
    virtual bool isTableCol() const { return false; }
    virtual bool isTable() const { return false; }
    virtual bool isWidget() const { return false; }
    virtual bool isBody() const { return false; }
    virtual bool isFormElement() const { return false; }
    virtual bool isFrameSet() const { return false; }
    virtual bool isApplet() const { return false; }
    virtual bool isMedia() const { return false; }

    virtual bool isEditable() const;

    // svg
    virtual bool isSVGRoot() const { return false; }
    virtual bool isRenderPath() const { return false; }
    virtual bool isSVGContainer() const { return false; }
    virtual bool isSVGText() const { return false; }
    virtual bool isSVGHiddenContainer() const { return false; }

    virtual FloatRect relativeBBox(bool includeStroke = false) const;

    virtual AffineTransform localTransform() const;
    virtual AffineTransform absoluteTransform() const;
    // end svg

    bool isHTMLMarquee() const;
    bool isWordBreak() const;

    bool isAnonymous() const { return m_isAnonymous; }
    void setIsAnonymous(bool b) { m_isAnonymous = b; }
    bool isAnonymousBlock() const { return isAnonymous() && style()->display() == BLOCK && node()->isDocumentNode(); }
    bool isPseudoAnonymous() const { return isAnonymous() && !node()->isDocumentNode(); }

    bool isFloating() const { return m_floating; }
    bool isPositioned() const { return m_positioned; }
    bool isRelPositioned() const { return m_relPositioned; }
    bool isText() const { return m_isText; }
    bool isInline() const { return m_inline; }
    bool isCompact() const { return style()->display() == COMPACT; } // compact
    bool isRunIn() const { return style()->display() == RUN_IN; } // run-in object
    bool mouseInside() const;
    bool isDragging() const;
    bool isReplaced() const { return m_replaced; }
    bool isReplacedBlock() const { return isInline() && isReplaced() && isRenderBlock(); }
    bool shouldPaintBackgroundOrBorder() const { return m_paintBackground; }
    bool needsLayout() const   { return m_needsLayout || m_normalChildNeedsLayout || m_posChildNeedsLayout; }
    bool markedForRepaint() const { return m_markedForRepaint; }
    void setMarkedForRepaint(bool m) { m_markedForRepaint = m; }
    bool selfNeedsLayout() const { return m_needsLayout; }
    bool posChildNeedsLayout() const { return m_posChildNeedsLayout; }
    bool normalChildNeedsLayout() const { return m_normalChildNeedsLayout; }
    bool minMaxKnown() const{ return m_minMaxKnown; }
    bool hasFirstLine() const { return m_hasFirstLine; }
    bool isSelectionBorder() const { return m_isSelectionBorder; }
    bool recalcMinMax() const { return m_recalcMinMax; }

    RenderCanvas* canvas() const;
    // don't even think about making this method virtual!
    DOM::DocumentImpl* document() const;
    DOM::NodeImpl* element() const { return isAnonymous() ? 0L : m_node; }
    DOM::NodeImpl* node() const { return m_node; }

    virtual bool handleEvent(const DOM::EventImpl&) { return false; }

   /**
     * returns the object containing this one. can be different from parent for
     * positioned elements
     */
    RenderObject *container() const;

    void markContainingBlocksForLayout();
    void dirtyFormattingContext( bool checkContainer );
    void repaintDuringLayout();
    void setNeedsLayout(bool b, bool markParents = true);
    void setChildNeedsLayout(bool b, bool markParents = true);
    void setMinMaxKnown(bool b=true) {
	m_minMaxKnown = b;
	if ( !b ) {
	    RenderObject *o = this;
	    while( o ) { // ### && !o->m_recalcMinMax ) {
		o->m_recalcMinMax = true;
		o = o->m_parent;
	    }
	}
    }
    void setNeedsLayoutAndMinMaxRecalc() {
        setMinMaxKnown(false);
        setNeedsLayout(true);
    }
    void setPositioned(bool b=true)  { m_positioned = b;  }
    void setRelPositioned(bool b=true) { m_relPositioned = b; }
    void setFloating(bool b=true) { m_floating = b; }
    void setInline(bool b=true) { m_inline = b; }
    void setMouseInside(bool b=true) { m_mouseInside = b; }
    void setShouldPaintBackgroundOrBorder(bool b=true) { m_paintBackground = b; }
    void setRenderText() { m_isText = true; }
    void setReplaced(bool b=true) { m_replaced = b; }
    void setHasOverflowClip(bool b = true) { m_hasOverflowClip = b; }
    void setIsSelectionBorder(bool b=true) { m_isSelectionBorder = b; }

    void scheduleRelayout(RenderObject *clippedObj = 0);

    void updateBackgroundImages(RenderStyle* oldStyle);

    virtual InlineBox* createInlineBox(bool makePlaceHolderBox, bool isRootLineBox);
    virtual void removeInlineBox(InlineBox* /*box*/) {}

    virtual InlineBox *inlineBox(long offset=0);
    virtual short lineHeight( bool firstLine ) const;
    virtual short verticalPositionHint( bool firstLine ) const;
    virtual short baselinePosition( bool firstLine ) const;
    short getVerticalPosition( bool firstLine, RenderObject* ref=0 ) const;

    /*
     * Print the object and its children, clipped by (x|y|w|h).
     * (tx|ty) is the calculated position of the parent
     */
    struct PaintInfo {
       PaintInfo(QPainter* _p, const QRect& _r, PaintAction _phase)
           : p(_p), r(_r), phase(_phase), outlineObjects(0) {}
       ~PaintInfo() { delete outlineObjects; }
       QPainter* p;
       QRect     r;
       PaintAction phase;
       QList<RenderFlow *>* outlineObjects; // used to list which outlines should be painted by a block with inline children
    };
    virtual void paint( PaintInfo& i, int tx, int ty);

    void adjustBorderRadii(BorderRadii &tl, BorderRadii &tr, BorderRadii &bl, BorderRadii &br, int w, int h) const;

    void drawBorderArc(QPainter *p, int x, int y, float horThickness, float vertThickness,
                       const BorderRadii &radius, int angleStart, int angleSpan, const QBrush &brush,
                       const QColor &textColor, EBorderStyle style, qreal *dashOffset = 0) const;

    void paintBorder(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style, bool begin=true, bool end=true);
    void paintOutline(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style);

    virtual void paintBoxDecorations(PaintInfo&, int /*_tx*/, int /*_ty*/) {}

    virtual void paintBackgroundExtended(QPainter* /*p*/, const QColor& /*c*/, const BackgroundLayer * /*bgLayer*/,
                                         QRect /*clipr*/, int /*_tx*/, int /*_ty*/,
                                         int /*w*/, int /*height*/, int /*bleft*/, int /*bright*/, int /*pleft*/, int /*pright*/,
                                         int /*btop*/, int /*bbottom*/, int /*ptop*/, int /*pbottom*/) {}

    /*
     * This function calculates the minimum & maximum width that the object
     * can be set to.
     *
     * when the Element calls setMinMaxKnown(true), calcMinMaxWidth() will
     * be no longer called.
     *
     * when a element has a fixed size, m_minWidth and m_maxWidth should be
     * set to the same value. This has the special meaning that m_width,
     * contains the actual value.
     *
     * assumes calcMinMaxWidth has already been called for all children.
     */
    virtual void calcMinMaxWidth() { }

    /*
     * Does the min max width recalculations after changes.
     */
    void recalcMinMaxWidths();

    /*
     * Calculates the actual width of the object (only for non inline
     * objects)
     */
    virtual void calcWidth() {}

    /*
     * Calculates the actual width of the object (only for non inline
     * objects)
     */
    virtual void calcHeight() {}

    /*
     * This function should cause the Element to calculate its
     * width and height and the layout of its content
     *
     * when the Element calls setNeedsLayout(false), layout() is no
     * longer called during relayouts, as long as there is no
     * style sheet change. When that occurs, m_needsLayout will be
     * set to true and the Element receives layout() calls
     * again.
     */
    virtual void layout() = 0;

    /* This function performs a layout only if one is needed. */
    void layoutIfNeeded() { if (needsLayout()) layout(); }

    // used for element state updates that can not be fixed with a
    // repaint and do not need a relayout
    virtual void updateFromElement() {}

    // Called immediately after render-object is inserted
    virtual void attach() { m_attached = true; }
    bool attached() { return m_attached; }
    // The corresponding closing element has been parsed. ### remove me
    virtual void close() { }

    virtual int availableHeight() const { return 0; }

    // Whether or not the element shrinks to its max width (rather than filling the width
    // of a containing block).  HTML4 buttons, legends, and floating/compact elements do this.
    bool sizesToMaxWidth() const;

    /*
     * NeesPageClear indicates the object crossed a page-break but could not break itself and now
     * needs to be moved clear by its parent.
     */
    void setNeedsPageClear(bool b = true) { m_needsPageClear = b; }
    virtual bool needsPageClear() const { return m_needsPageClear; }

    /*
     * ContainsPageBreak indicates the object contains a clean page-break.
     * ### should be removed and replaced with (crossesPageBreak && !needsPageClear)
     */
    void setContainsPageBreak(bool b = true) { m_containsPageBreak = b; }
    virtual bool containsPageBreak() const { return m_containsPageBreak; }

    virtual int pageTopAfter(int y) const { if (parent()) return parent()->pageTopAfter(y); else return 0; }

    virtual int crossesPageBreak(int top, int bottom) const
    { if (parent()) return parent()->crossesPageBreak(top, bottom); else return 0; }

    // Checks if a page-break before child is possible at the given page-break level
    // false means the child should attempt the break self.
    virtual bool canClear(RenderObject * /*child*/, PageBreakLevel level)
    { if (parent()) return parent()->canClear(this, level); else return false; }

    void setAfterPageBreak(bool b = true)  { m_afterPageBreak = b; }
    void setBeforePageBreak(bool b = true) { m_beforePageBreak = b; }
    virtual bool afterPageBreak() const  { return m_afterPageBreak; }
    virtual bool beforePageBreak() const { return m_beforePageBreak; }

    // does a query on the rendertree and finds the innernode
    // and overURL for the given position
    // if readonly == false, it will recalc hover styles accordingly
   class NodeInfo
    {
        friend class RenderImage;
        friend class RenderFlow;
        friend class RenderInline;
        friend class RenderText;
        friend class RenderWidget;
        friend class RenderObject;
        friend class RenderFrameSet;
	friend class RenderLayer;
        friend class DOM::HTMLAreaElementImpl;
    public:
        NodeInfo(bool readonly, bool active)
            : m_innerNode(0), m_innerNonSharedNode(0), m_innerURLElement(0), m_readonly(readonly), m_active(active)
            { }

        DOM::NodeImpl* innerNode() const { return m_innerNode; }
        DOM::NodeImpl* innerNonSharedNode() const { return m_innerNonSharedNode; }
        DOM::NodeImpl* URLElement() const { return m_innerURLElement; }
        bool readonly() const { return m_readonly; }
        bool active() const { return m_active; }

    private:
        void setInnerNode(DOM::NodeImpl* n) { m_innerNode = n; }
        void setInnerNonSharedNode(DOM::NodeImpl* n) { m_innerNonSharedNode = n; }
        void setURLElement(DOM::NodeImpl* n) { m_innerURLElement = n; }

        DOM::NodeImpl* m_innerNode;
        DOM::NodeImpl* m_innerNonSharedNode;
        DOM::NodeImpl* m_innerURLElement;
        bool m_readonly;
        bool m_active;
    };

    /** contains stateful information for a checkSelectionPoint call
     */
    struct SelPointState {
        /** last node that was before the current position */
        DOM::NodeImpl *m_lastNode;
	/** offset of last node */
	long m_lastOffset;
	/** true when the last node had the result SelectionAfterInLine */
	bool m_afterInLine;

	SelPointState() : m_lastNode(0), m_lastOffset(0), m_afterInLine(false)
	{}
    };

#if 0
    virtual FindSelectionResult checkSelectionPoint( int _x, int _y, int _tx, int _ty,
                                                     DOM::NodeImpl*&, int & offset,
						     SelPointState & );
#endif
    virtual bool nodeAtPoint(NodeInfo& info, int x, int y, int tx, int ty, HitTestAction, bool inside = false);
    void setInnerNode(NodeInfo& info);

    // Position/Selection stuff
    virtual RenderPosition positionForCoordinates(int x, int y);

    // set the style of the object.
    virtual void setStyle(RenderStyle *style);

    // returns the containing block level element for this element.
    RenderBlock *containingBlock() const;

    // return just the width of the containing block
    virtual short containingBlockWidth(RenderObject* providedCB=0) const;
    // return just the height of the containing block
    virtual int containingBlockHeight(RenderObject* providedCB=0) const;

    // size of the content area (box size minus padding/border)
    virtual short contentWidth() const { return 0; }
    virtual int contentHeight() const { return 0; }

    // intrinsic extend of replaced elements. undefined otherwise
    virtual short intrinsicWidth() const { return 0; }
    virtual int intrinsicHeight() const { return 0; }

    // relative to parent node
    virtual void setPos( int /*xPos*/, int /*yPos*/ ) { }
    virtual void setWidth( int /*width*/ ) { }
    virtual void setHeight( int /*height*/ ) { }

    virtual int xPos() const { return 0; }
    virtual int yPos() const { return 0; }

    /** the position of the object from where it begins drawing, including
     * its negative overflow
     */
    int effectiveXPos() const { return xPos() + (hasOverflowClip() ? 0 : overflowLeft()); }

    /** the position of the object from where it begins drawing, including
     * its negative overflow
     */
    int effectiveYPos() const { return yPos() + (hasOverflowClip() ? -borderTopExtra() : qMin(overflowTop(), -borderTopExtra())); }

    /** Leftmost coordinate of this inline element relative to containing
     * block. Always zero for non-inline elements.
     */
    virtual int inlineXPos() const { return 0; }
    /** Topmost coordinate of this inline element relative to containing
     * block. Always zero for non-inline elements.
     */
    virtual int inlineYPos() const { return 0; }

    // calculate client position of box
    virtual bool absolutePosition(int &/*xPos*/, int &/*yPos*/, bool fixed = false) const;

    // width and height are without margins but include paddings and borders
    virtual short width() const { return 0; }
    virtual int height() const { return 0; }

    // The height of a block when you include overflow spillage out of
    // the bottom of the block (e.g., a <div style="height:25px"> that
    // has a 100px tall image inside it would have an overflow height
    // of borderTop() + paddingTop() + 100px.
    virtual int overflowHeight() const { return height(); }
    virtual int overflowWidth() const { return width(); }
    // how much goes over the left hand side (0 or a negative number)
    virtual int overflowTop() const { return 0; }
    virtual int overflowLeft() const { return 0; }

    /**
     * Returns the height that is effectively considered when contemplating the
     * object as a whole -- usually the overflow height, or the height if clipped.
     */
    int effectiveHeight() const { return hasOverflowClip() ? height() + borderTopExtra() + borderBottomExtra() :
                                         qMax(overflowHeight() - overflowTop(),  height() + borderTopExtra() + borderBottomExtra()); }
    /**
     * Returns the width that is effectively considered when contemplating the
     * object as a whole -- usually the overflow width, or the width if clipped.
     */
    int effectiveWidth() const { return hasOverflowClip() ? width() : overflowWidth() - overflowLeft(); }

    QRectF clientRectToViewport(const QRectF& rect);
    virtual QList< QRectF > getClientRects();

    // IE extensions, heavily used in ECMA
    virtual short offsetWidth() const { return width(); }
    virtual int offsetHeight() const { return height() + borderTopExtra() + borderBottomExtra(); }
    virtual int offsetLeft() const;
    virtual int offsetTop() const;
    virtual RenderObject* offsetParent() const;
    int clientLeft() const;
    int clientTop() const;
    short clientWidth() const;
    int clientHeight() const;
    virtual short scrollWidth() const;
    virtual int scrollHeight() const;

    virtual bool isSelfCollapsingBlock() const { return false; }
    short collapsedMarginTop() const { return maxTopMargin(true)-maxTopMargin(false);  }
    short collapsedMarginBottom() const { return maxBottomMargin(true)-maxBottomMargin(false); }

    virtual bool isTopMarginQuirk() const { return false; }
    virtual bool isBottomMarginQuirk() const { return false; }
    virtual short maxTopMargin(bool positive) const
    { return positive ? qMax( int( marginTop() ), 0 ) : - qMin( int( marginTop() ), 0 ); }
    virtual short maxBottomMargin(bool positive) const
    { return positive ? qMax( int( marginBottom() ), 0 ) : - qMin( int( marginBottom() ), 0 ); }

    virtual short marginTop() const { return 0; }
    virtual short marginBottom() const { return 0; }
    virtual short marginLeft() const { return 0; }
    virtual short marginRight() const { return 0; }

    virtual int paddingTop() const;
    virtual int paddingBottom() const;
    virtual int paddingLeft() const;
    virtual int paddingRight() const;

    virtual int borderTop() const { return style()->borderTopWidth(); }
    virtual int borderBottom() const { return style()->borderBottomWidth(); }
    virtual int borderLeft() const { return style()->borderLeftWidth(); }
    virtual int borderRight() const { return style()->borderRightWidth(); }

    virtual short minWidth() const { return 0; }
    virtual int maxWidth() const { return 0; }

    RenderStyle* style() const { return m_style; }
    RenderStyle* style( bool firstLine ) const {
	RenderStyle *s = m_style;
	if( firstLine && hasFirstLine() ) {
	    RenderStyle *pseudoStyle  = style()->getPseudoStyle(RenderStyle::FIRST_LINE);
	    if ( pseudoStyle )
		s = pseudoStyle;
	}
	return s;
    }

    void getTextDecorationColors(int decorations, QColor& underline, QColor& overline,
                                 QColor& linethrough, bool quirksMode=false);

    enum BorderSide {
        BSTop, BSBottom, BSLeft, BSRight
    };
    void drawBorder(QPainter *p, int x1, int y1, int x2, int y2, BorderSide s,
                    QColor c, const QColor& textcolor, EBorderStyle style,
                    int adjbw1, int adjbw2, bool invalidisInvert = false, qreal *dashOffset = 0);

    // Used by collapsed border tables.
    virtual void collectBorders(QList<CollapsedBorderValue>& borderStyles);

    // force a complete repaint
    virtual void repaint(Priority p = NormalPriority) { if(m_parent) m_parent->repaint(p); }
    virtual void repaintRectangle(int x, int y, int w, int h, Priority p=NormalPriority, bool f=false);

    virtual unsigned int length() const { return 1; }

    virtual bool isHidden() const { return isFloating() || isPositioned(); }

    // Special objects are objects that are neither really inline nor blocklevel
    bool isFloatingOrPositioned() const { return (isFloating() || isPositioned()); }
    virtual bool hasOverhangingFloats() const { return false; }
    virtual bool hasFloats() const { return false; }
    virtual bool containsFloat(RenderObject* /*o*/) const { return false; }
    virtual void markAllDescendantsWithFloatsForLayout(RenderObject* /*floatToRemove*/ = 0) {}

    bool flowAroundFloats() const;
    bool usesLineWidth() const;

    // positioning of inline children (bidi)
    virtual void position(InlineBox*, int, int, bool) {}
//    virtual void position(int, int, int, int, int, bool, bool, int) {}

    // Applied as a "slop" to dirty rect checks during the outline painting phase's dirty-rect checks.
    int maximalOutlineSize(PaintAction p) const;

    enum SelectionState {
        SelectionNone,
        SelectionStart,
        SelectionInside,
        SelectionEnd,
        SelectionBoth
    };

    virtual SelectionState selectionState() const { return SelectionNone;}
    virtual void setSelectionState(SelectionState) {}
    bool shouldSelect() const;
    virtual bool isPointInsideSelection(int x, int y, const DOM::Selection &) const;

    DOM::NodeImpl* draggableNode(bool dhtmlOK, bool uaOK, bool& dhtmlWillDrag) const;

    /**
     * Flags which influence the appearance and position
     * @param CFOverride input overrides existing character, caret should be
     *		cover the whole character
     * @param CFOutside coordinates are to be interpreted outside of the
     *		render object
     * @param CFOutsideEnd coordinates are to be interpreted at the outside
     *		end of the render object (only valid if CFOutside is also set)
     */
    enum CaretFlags { CFOverride = 0x01, CFOutside = 0x02, CFOutsideEnd = 0x04 };

    /**
     * Returns the content coordinates of the caret within this render object.
     * @param offset zero-based offset determining position within the render object.
     * @param flags combination of enum CaretFlags
     * @param _x returns the left coordinate
     * @param _y returns the top coordinate
     * @param width returns the caret's width
     * @param height returns the caret's height
     */
    virtual void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const;

    // returns the lowest position of the lowest object in that particular object.
    // This 'height' is relative to the topleft of the margin box of the object.
    // Implemented in RenderFlow.
    virtual int lowestPosition(bool /*includeOverflowInterior*/=true, bool /*includeSelf*/=true) const { return 0; }
    virtual int rightmostPosition(bool /*includeOverflowInterior*/=true, bool /*includeSelf*/=true) const { return 0; }
    virtual int leftmostPosition(bool /*includeOverflowInterior*/=true, bool /*includeSelf*/=true) const { return 0; }
    virtual int highestPosition(bool /*includeOverflowInterior*/=true, bool /*includeSelf*/=true) const { return 0; }

    // recursively invalidate current layout
    // unused: void invalidateLayout();

    virtual void calcVerticalMargins() {}
    void removeFromObjectLists();
    void setInPosObjectList(bool b = true) { m_inPosObjectList = b; }
    bool inPosObjectList() const { return m_inPosObjectList; }

    virtual void deleteInlineBoxes(RenderArena* arena=0) {(void)arena;}
    virtual void dirtyInlineBoxes(bool /*fullLayout*/, bool /*isRootLineBox*/ = false) {}
    virtual void dirtyLinesFromChangedChild(RenderObject*) {}
    virtual void detach( );

    bool documentBeingDestroyed() const { return !document()->renderer(); }

    void setDoNotDelete(bool b) { m_doNotDelete = b; }
    bool doNotDelete() const { return m_doNotDelete; }

    const QFont &font(bool firstLine) const {
	return style( firstLine )->font();
    }

    const QFontMetrics &fontMetrics(bool firstLine) const {
	return style( firstLine )->fontMetrics();
    }

    /** returns the lowest possible value the caret offset may have to
     * still point to a valid position.
     *
     * Returns 0 by default.
     */
    virtual long caretMinOffset() const;
    /** returns the highest possible value the caret offset may have to
     * still point to a valid position.
     *
     * Returns 0 by default, as generic elements are considered to have no
     * width.
     */
    virtual long caretMaxOffset() const;
    virtual unsigned long caretMaxRenderedOffset() const;

    virtual void updatePixmap(const QRect&, CachedImage *);

    QRegion visibleFlowRegion(int x, int y) const;

    virtual void removeSuperfluousAnonymousBlockChild( RenderObject* ) {}

    // Unregisters from parent but does not destroy
    void remove();
protected:
    virtual void selectionStartEnd(int& spos, int& epos);

    virtual QRect viewRect() const;
    void setDetached() { m_attached = false; }
    void invalidateVerticalPosition();
    bool attemptDirectLayerTranslation();
    void updateWidgetMasks();

    void arenaDelete(RenderArena *arena);

private:
    RenderStyle* m_style;
    DOM::NodeImpl* m_node;
    RenderObject *m_parent;
    RenderObject *m_previous;
    RenderObject *m_next;

    short m_verticalPosition;

    bool m_needsLayout               : 1;
    bool m_normalChildNeedsLayout    : 1;
    bool m_markedForRepaint          : 1;
    bool m_posChildNeedsLayout       : 1;

    bool m_minMaxKnown               : 1;
    bool m_floating                  : 1;

    bool m_positioned                : 1;
    bool m_relPositioned             : 1;
    bool m_paintBackground           : 1; // if the box has something to paint in the
                                          // background painting phase (background, border, etc)

    bool m_isAnonymous               : 1;
    bool m_recalcMinMax 	     : 1;
    bool m_isText                    : 1;
    bool m_inline                    : 1;
    bool m_attached                  : 1;

    bool m_replaced                  : 1;
    bool m_mouseInside               : 1;
    bool m_hasFirstLine              : 1;
    bool m_isSelectionBorder         : 1;

    bool m_isRoot                    : 1;

    bool m_beforePageBreak           : 1;
    bool m_afterPageBreak            : 1;

    bool m_needsPageClear            : 1;
    bool m_containsPageBreak         : 1;

    bool m_hasOverflowClip           : 1;
    bool m_inPosObjectList           : 1;

    bool m_doNotDelete 	             : 1; // This object should not be auto-deleted

    // ### we have 16 + 26 bits.

    static QCache<quint64, QPixmap> *s_dashedLineCache;

    void arenaDelete(RenderArena *arena, void *objectBase);

    friend class RenderLayer;
    friend class RenderListItem;
    friend class RenderContainer;
    friend class RenderCanvas;
};


enum VerticalPositionHint {
    PositionTop = -0x4000,
    PositionBottom = 0x4000,
    PositionUndefined = 0x3fff
};

} //namespace
#endif
