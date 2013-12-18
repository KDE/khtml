/*
 * This file is part of the render object implementation for KHTML.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999-2003 Antti Koivisto (koivisto@kde.org)
 *           (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2003 Apple Computer, Inc.
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

#ifndef RENDER_BLOCK_H
#define RENDER_BLOCK_H

#include <QList>

#include "render_flow.h"

namespace DOM {
    class Position;
}

namespace khtml {

class RenderBlock : public RenderFlow
{
public:
    RenderBlock(DOM::NodeImpl* node);
    virtual ~RenderBlock();

    virtual const char *renderName() const;

    virtual bool isRenderBlock() const { return true; }
    virtual bool isBlockFlow() const { return (!isInline() || isReplaced()) && !isTable(); }
    virtual bool isInlineFlow() const { return isInline() && !isReplaced(); }
    virtual bool isInlineBlockOrInlineTable() const { return isInline() && isReplaced(); }

    virtual bool childrenInline() const { return m_childrenInline; }
    virtual void setChildrenInline(bool b) { m_childrenInline = b; }
    virtual short baselinePosition( bool firstLine ) const;
    
    int getBaselineOfLastLineBox() const;
    void makeChildrenNonInline(RenderObject* insertionPoint = 0);

    void makePageBreakAvoidBlocks();

    // The height (and width) of a block when you include overflow spillage out of the bottom
    // of the block (e.g., a <div style="height:25px"> that has a 100px tall image inside
    // it would have an overflow height of borderTop() + paddingTop() + 100px.
    virtual int overflowHeight() const  { return m_overflowHeight; }
    virtual int overflowWidth() const   { return m_overflowWidth; }
    virtual int overflowLeft() const { return m_overflowLeft; }
    virtual int overflowTop() const  { return m_overflowTop; }
    virtual void setOverflowHeight(int h) { m_overflowHeight = h; }
    virtual void setOverflowWidth(int w) { m_overflowWidth = w; }
    virtual void setOverflowLeft(int l) { m_overflowLeft = l; }
    virtual void setOverflowTop(int t) { m_overflowTop = t; }

    virtual bool isSelfCollapsingBlock() const;
    virtual bool isTopMarginQuirk() const { return m_topMarginQuirk; }
    virtual bool isBottomMarginQuirk() const { return m_bottomMarginQuirk; }

    virtual short maxTopMargin(bool positive) const {
        if (positive)
            return m_maxTopPosMargin;
        else
            return m_maxTopNegMargin;
    }
    virtual short maxBottomMargin(bool positive) const {
        if (positive)
            return m_maxBottomPosMargin;
        else
            return m_maxBottomNegMargin;
    }

    void initMaxMarginValues() {
        if (m_marginTop >= 0)
            m_maxTopPosMargin = m_marginTop;
        else
            m_maxTopNegMargin = -m_marginTop;
        if (m_marginBottom >= 0)
            m_maxBottomPosMargin = m_marginBottom;
        else
            m_maxBottomNegMargin = -m_marginBottom;
    }

    virtual void addChildToFlow(RenderObject* newChild, RenderObject* beforeChild);
    virtual void removeChild(RenderObject *oldChild);

    virtual void setStyle(RenderStyle* _style);
    virtual void attach();
    void updateFirstLetter();

    virtual void layout();
    void layoutBlock( bool relayoutChildren );
    void layoutBlockChildren( bool relayoutChildren );
    void layoutInlineChildren( bool relayoutChildren, int breakBeforeLine = 0);

    void layoutPositionedObjects( bool relayoutChildren );
    void insertPositionedObject(RenderObject *o);
    void removePositionedObject(RenderObject *o);

    QRegion visibleFloatingRegion(int x, int y) const;
    // Called to lay out the legend for a fieldset.
    virtual RenderObject* layoutLegend(bool /*relayoutChildren*/) { return 0; }

    // the implementation of the following functions is in bidi.cpp
    void bidiReorderLine(const BidiIterator &start, const BidiIterator &end, BidiState &bidi );
    void fitBelowFloats(int widthToFit, int& availableWidth);
    BidiIterator findNextLineBreak(BidiIterator &start, BidiState &info );
    RootInlineBox* constructLine(const BidiIterator& start, const BidiIterator& end);
    InlineFlowBox* createLineBoxes(RenderObject* obj);
    bool inlineChildNeedsLineBox(RenderObject* obj);
    void computeHorizontalPositionsForLine(InlineFlowBox* lineBox, BidiState &bidi);
    void computeVerticalPositionsForLine(RootInlineBox* lineBox);
    bool clearLineOfPageBreaks(InlineFlowBox* lineBox);
    void checkLinesForOverflow();
    void deleteEllipsisLineBoxes();
    void checkLinesForTextOverflow();
    RootInlineBox* determineStartPosition(bool fullLayout, BidiIterator& start, BidiState& bidi);
    RootInlineBox* determineEndPosition(RootInlineBox* startLine, BidiIterator& cleanLineStart, BidiStatus& cleanLineBidiStatus, BidiContext* cleanLineBidiContext, int& yPos);
    bool matchedEndLine(const BidiIterator& start, const BidiStatus& status, BidiContext* context,
                        const BidiIterator& endLineStart, const BidiStatus& endLineStatus, BidiContext* endLineContext,
                        RootInlineBox*& endLine, int& endYPos);
    // end bidi.cpp functions

    virtual void paint(PaintInfo& i, int tx, int ty);
    void paintObject(PaintInfo& i, int tx, int ty, bool paintOutline = true);
    void paintFloats(PaintInfo& i, int _tx, int _ty, bool paintSelection = false);

    void insertFloatingObject(RenderObject *o);
    void removeFloatingObject(RenderObject *o);

    // called from lineWidth, to position the floats added in the last line.
    void positionNewFloats();
    void clearFloats();
    int getClearDelta(RenderObject *child, int yPos);
    virtual void markAllDescendantsWithFloatsForLayout(RenderObject* floatToRemove = 0);

    // FIXME: containsFloats() should not return true if the floating objects list
    // is empty. However, layoutInlineChildren() relies on the current behavior.
    // http://bugzilla.opendarwin.org/show_bug.cgi?id=7395#c3
    virtual bool hasFloats() const { return m_floatingObjects!=0; }
    virtual bool containsFloat(RenderObject* o) const;

    virtual bool hasOverhangingFloats() const { return floatBottom() > m_height; }
    void addOverHangingFloats( RenderBlock *block, int xoffset, int yoffset, bool child );

    int nearestFloatBottom(int height) const;
    int floatBottom() const;
    inline int leftBottom();
    inline int rightBottom();

    virtual unsigned short lineWidth(int y, bool *canClearLine = 0) const;
    virtual int lowestPosition(bool includeOverflowInterior=true, bool includeSelf=true) const;
    virtual int rightmostPosition(bool includeOverflowInterior=true, bool includeSelf=true) const;
    virtual int leftmostPosition(bool includeOverflowInterior=true, bool includeSelf=true) const;
    virtual int highestPosition(bool includeOverflowInterior, bool includeSelf) const;
    int lowestAbsolutePosition() const;
    int leftmostAbsolutePosition() const;
    int rightmostAbsolutePosition() const;
    int highestAbsolutePosition() const;

    int rightOffset() const;
    int rightRelOffset(int y, int fixedOffset, bool applyTextIndent=true, int *heightRemaining = 0, bool *canClearLine = 0) const;
    int rightOffset(int y, bool *canClearLine = 0) const { return rightRelOffset(y, rightOffset(), true, 0, canClearLine); }

    int leftOffset() const;
    int leftRelOffset(int y, int fixedOffset, bool applyTextIndent=true, int *heightRemaining = 0, bool *canClearLine = 0) const;
    int leftOffset(int y, bool *canClearLine = 0) const { return leftRelOffset(y, leftOffset(), true, 0, canClearLine); }

    virtual bool nodeAtPoint(NodeInfo& info, int x, int y, int _tx, int _ty, HitTestAction hitTestAction = HitTestAll, bool inside=false);

    bool isPointInScrollbar(int x, int y, int tx, int ty);

    virtual RenderPosition positionForCoordinates(int x, int y);

    virtual void calcMinMaxWidth();
    void calcInlineMinMaxWidth();
    void calcBlockMinMaxWidth();

    virtual void close();

    virtual int getBaselineOfFirstLineBox();

    RootInlineBox* firstRootBox() const { return static_cast<RootInlineBox*>(m_firstLineBox); }
    RootInlineBox* lastRootBox() const { return static_cast<RootInlineBox*>(m_lastLineBox); }

    virtual InlineFlowBox* getFirstLineBox();

    bool inRootBlockContext() const;
    void deleteLineBoxTree();

#ifdef ENABLE_DUMP
    virtual void printTree(int indent=0) const;
    virtual void dump(QTextStream &stream, const QString &ind) const;
#endif

protected:
    void newLine();

private:
    RenderPosition positionForBox(InlineBox *box, bool start=true) const;
    RenderPosition positionForRenderer(RenderObject *renderer, bool start=true) const;

protected:
    struct FloatingObject {
        enum Type {
            FloatLeft,
            FloatRight
        };

        FloatingObject(Type _type) {
            node = 0;
            startY = 0;
            endY = 0;
            type = _type;
            left = 0;
            width = 0;
            noPaint = false;
            crossedLayer = false;

        }
        RenderObject* node;
        int startY;
        int endY;
        short left;
        short width;
        KDE_BF_ENUM(Type) type : 1; // left or right aligned
        bool noPaint : 1;
        bool crossedLayer : 1; // lock noPaint flag
    };

    // The following helper functions and structs are used by layoutBlockChildren.
    class CompactInfo {
        // A compact child that needs to be collapsed into the margin of the following block.
        RenderObject* m_compact;

        // The block with the open margin that the compact child is going to place itself within.
        RenderObject* m_block;
        bool m_treatAsBlock : 1;

    public:
        RenderObject* compact() const { return m_compact; }
        RenderObject* block() const { return m_block; }
        void setTreatAsBlock(bool b) { m_treatAsBlock = b; }
        bool treatAsBlock() const { return m_treatAsBlock; }
        bool matches(RenderObject* child) const { return m_compact && m_block == child; }

        void clear() { set(0, 0);  }
        void set(RenderObject* c, RenderObject* b) { m_compact = c; m_block = b; }

        CompactInfo() { clear(); }
    };

    class MarginInfo {
        // Collapsing flags for whether we can collapse our margins with our children's margins.
        bool m_canCollapseWithChildren : 1;
        bool m_canCollapseTopWithChildren : 1;
        bool m_canCollapseBottomWithChildren : 1;

        // Whether or not we are a quirky container, i.e., do we collapse away top and bottom
        // margins in our container.  Table cells and the body are the common examples. We
        // also have a custom style property for Safari RSS to deal with TypePad blog articles.
        bool m_quirkContainer : 1;

        // This flag tracks whether we are still looking at child margins that can all collapse together at the beginning of a block.
        // They may or may not collapse with the top margin of the block (|m_canCollapseTopWithChildren| tells us that), but they will
        // always be collapsing with one another.  This variable can remain set to true through multiple iterations
        // as long as we keep encountering self-collapsing blocks.
        bool m_atTopOfBlock : 1;

        // This flag is set when we know we're examining bottom margins and we know we're at the bottom of the block.
        bool m_atBottomOfBlock : 1;

        // If our last normal flow child was a self-collapsing block that cleared a float,
        // we track it in this variable.
        bool m_selfCollapsingBlockClearedFloat : 1;

        // These variables are used to detect quirky margins that we need to collapse away (in table cells
        // and in the body element).
        bool m_topQuirk : 1;
        bool m_bottomQuirk : 1;
        bool m_determinedTopQuirk : 1;

        // These flags track the previous maximal positive and negative margins.
        int m_posMargin;
        int m_negMargin;

    public:
        MarginInfo(RenderBlock* b, int top, int bottom);

        void setAtTopOfBlock(bool b) { m_atTopOfBlock = b; }
        void setAtBottomOfBlock(bool b) { m_atBottomOfBlock = b; }
        void clearMargin() { m_posMargin = m_negMargin = 0; }
        void setSelfCollapsingBlockClearedFloat(bool b) { m_selfCollapsingBlockClearedFloat = b; }
        void setTopQuirk(bool b) { m_topQuirk = b; }
        void setBottomQuirk(bool b) { m_bottomQuirk = b; }
        void setDeterminedTopQuirk(bool b) { m_determinedTopQuirk = b; }
        void setPosMargin(int p) { m_posMargin = p; }
        void setNegMargin(int n) { m_negMargin = n; }
        void setPosMarginIfLarger(int p) { if (p > m_posMargin) m_posMargin = p; }
        void setNegMarginIfLarger(int n) { if (n > m_negMargin) m_negMargin = n; }

        void setMargin(int p, int n) { m_posMargin = p; m_negMargin = n; }

        bool atTopOfBlock() const { return m_atTopOfBlock; }
        bool canCollapseWithTop() const { return m_atTopOfBlock && m_canCollapseTopWithChildren; }
        bool canCollapseWithBottom() const { return m_atBottomOfBlock && m_canCollapseBottomWithChildren; }
        bool canCollapseTopWithChildren() const { return m_canCollapseTopWithChildren; }
        bool canCollapseBottomWithChildren() const { return m_canCollapseBottomWithChildren; }
        bool selfCollapsingBlockClearedFloat() const { return m_selfCollapsingBlockClearedFloat; }
        bool quirkContainer() const { return m_quirkContainer; }
        bool determinedTopQuirk() const { return m_determinedTopQuirk; }
        bool topQuirk() const { return m_topQuirk; }
        bool bottomQuirk() const { return m_bottomQuirk; }
        int posMargin() const { return m_posMargin; }
        int negMargin() const { return m_negMargin; }
        int margin() const { return m_posMargin - m_negMargin; }
    };

    class PageBreakInfo {
        int m_pageBottom; // Next calculated page-break
        bool m_forcePageBreak : 1; // Must break before next block
        // ### to do better "page-break-after/before: avoid" this struct
        // should keep a pagebreakAvoid block and gather children in it
    public:
        PageBreakInfo(int pageBottom) : m_pageBottom(pageBottom), m_forcePageBreak(false) {}
        bool forcePageBreak() { return m_forcePageBreak; }
        void setForcePageBreak(bool b) { m_forcePageBreak = b; }
        int pageBottom() { return m_pageBottom; }
        void setPageBottom(int bottom) { m_pageBottom = bottom; }
    };

    virtual bool canClear(RenderObject *child, PageBreakLevel level);
    void clearPageBreak(RenderObject* child, int pageBottom);

    void adjustPositionedBlock(RenderObject* child, const MarginInfo& marginInfo);
    void adjustFloatingBlock(const MarginInfo& marginInfo);
    RenderObject* handleSpecialChild(RenderObject* child, const MarginInfo& marginInfo, CompactInfo& compactInfo, bool& handled);
    RenderObject* handleFloatingChild(RenderObject* child, const MarginInfo& marginInfo, bool& handled);
    RenderObject* handlePositionedChild(RenderObject* child, const MarginInfo& marginInfo, bool& handled);
    RenderObject* handleCompactChild(RenderObject* child, CompactInfo& compactInfo, const MarginInfo& marginInfo, bool& handled);
    RenderObject* handleRunInChild(RenderObject* child, bool& handled);
    int collapseMargins(RenderObject* child, MarginInfo& marginInfo, int yPos);
    int clearFloatsIfNeeded(RenderObject* child, MarginInfo& marginInfo, int oldTopPosMargin, int oldTopNegMargin, int yPos);
    void adjustSizeForCompactIfNeeded(RenderObject* child, CompactInfo& compactInfo);
    void insertCompactIfNeeded(RenderObject* child, CompactInfo& compactInfo);
    int estimateVerticalPosition(RenderObject* child, const MarginInfo& info);
    void determineHorizontalPosition(RenderObject* child);
    void handleBottomOfBlock(int top, int bottom, MarginInfo& marginInfo);
    void setCollapsedBottomMargin(const MarginInfo& marginInfo);
    void clearChildOfPageBreaks(RenderObject* child, PageBreakInfo &pageBreakInfo, MarginInfo &marginInfo);
    // End helper functions and structs used by layoutBlockChildren.

protected:
    // How much content overflows out of our block vertically or horizontally (all we support
    // for now is spillage out of the bottom and the right, which are the common cases).
    int m_overflowHeight;
    int m_overflowWidth;

    // Left and top overflow.
    int m_overflowTop;
    int m_overflowLeft;

    QList<FloatingObject*>* m_floatingObjects;
    QList<RenderObject*>*   m_positionedObjects;

private:
    bool m_childrenInline : 1;
    bool m_firstLine      : 1; // used in inline layouting
    KDE_BF_ENUM(EClear) m_clearStatus  : 2; // used during layuting of paragraphs
    bool m_avoidPageBreak : 1; // anonymous avoid page-break block
    bool m_topMarginQuirk : 1;
    bool m_bottomMarginQuirk : 1;

    short m_maxTopPosMargin;
    short m_maxTopNegMargin;
    short m_maxBottomPosMargin;
    short m_maxBottomNegMargin;

};

} // namespace

#endif // RENDER_BLOCK_H

