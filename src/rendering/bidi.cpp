/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2000-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2003-2007 Apple Computer, Inc.
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2007-2009 Germain Garand (germain@ebooksfrance.org)
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
#include "rendering/bidi.h"
#include "rendering/break_lines.h"
#include "rendering/render_block.h"
#include "rendering/render_text.h"
#include "rendering/render_arena.h"
#include "rendering/render_layer.h"
#include "rendering/render_canvas.h"
#include "xml/dom_docimpl.h"
#include <QVector>

#include "QDebug"

#include <limits.h>

// SVG
#include "rendering/SVGRootInlineBox.h"
#include "rendering/SVGInlineTextBox.h"

#define BIDI_DEBUG 0
//#define DEBUG_LINEBREAKS
//#define PAGE_DEBUG

namespace khtml {


// an iterator which goes through a BidiParagraph
struct BidiIterator
{
    BidiIterator() : par(0), obj(0), pos(0), endOfInline(false) {}
    BidiIterator(RenderBlock *_par, RenderObject *_obj, unsigned int _pos, bool eoi=false) : par(_par), obj(_obj), pos(_pos), endOfInline(eoi) {}

    void increment( BidiState *bidi=0, bool skipInlines=true );

    bool atEnd() const;

    const QChar &current() const;
    QChar::Direction direction() const;

    RenderBlock *par;
    RenderObject *obj;
    unsigned int pos;
    bool endOfInline;
};

struct BidiState {
    BidiState() : context(0) {}

    BidiIterator sor;
    BidiIterator eor;
    BidiIterator last;
    BidiIterator current;
    BidiContext *context;
    BidiStatus status;
};

// Used to track a list of chained bidi runs.
static BidiRun* sFirstBidiRun;
static BidiRun* sLastBidiRun;
static int sBidiRunCount;
static BidiRun* sCompactFirstBidiRun;
static BidiRun* sCompactLastBidiRun;
static int sCompactBidiRunCount;
static bool sBuildingCompactRuns;

// Midpoint globals.  The goal is not to do any allocation when dealing with
// these midpoints, so we just keep an array around and never clear it.  We track
// the number of items and position using the two other variables.
static QVector<BidiIterator> *smidpoints;
static uint sNumMidpoints;
static uint sCurrMidpoint;
static bool betweenMidpoints;

static bool isLineEmpty = true;
static bool previousLineBrokeAtBR = false;
static QChar::Direction dir = QChar::DirON;
static bool emptyRun = true;
static int numSpaces;

static void embed( QChar::Direction d, BidiState &bidi );
static void appendRun( BidiState &bidi );

static int getBPMWidth(int childValue, Length cssUnit)
{
    if (!cssUnit.isAuto())
        return (cssUnit.isFixed() ? cssUnit.value() : childValue);
    return 0;
}

static int getBorderPaddingMargin(RenderObject* child, bool endOfInline)
{
    RenderStyle* cstyle = child->style();
    int result = 0;
    bool leftSide = (cstyle->direction() == LTR) ? !endOfInline : endOfInline;
    result += getBPMWidth((leftSide ? child->marginLeft() : child->marginRight()),
                          (leftSide ? cstyle->marginLeft() :
                           cstyle->marginRight()));
    result += getBPMWidth((leftSide ? child->paddingLeft() : child->paddingRight()),
                          (leftSide ? cstyle->paddingLeft() :
                           cstyle->paddingRight()));
    result += leftSide ? child->borderLeft() : child->borderRight();
    return result;
}

#ifndef NDEBUG
static bool inBidiRunDetach;
#endif

void BidiRun::detach(RenderArena* renderArena)
{
#ifndef NDEBUG
    inBidiRunDetach = true;
#endif
    delete this;
#ifndef NDEBUG
    inBidiRunDetach = false;
#endif

    // Recover the size left there for us by operator delete and free the memory.
    renderArena->free(*(size_t *)this, this);
}

void* BidiRun::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void BidiRun::operator delete(void* ptr, size_t sz)
{
    assert(inBidiRunDetach);

    // Stash size where detach can find it.
    *(size_t*)ptr = sz;
}

static void deleteBidiRuns(RenderArena* arena)
{
    if (!sFirstBidiRun)
        return;

    BidiRun* curr = sFirstBidiRun;
    while (curr) {
        BidiRun* s = curr->nextRun;
        curr->detach(arena);
        curr = s;
    }

    sFirstBidiRun = 0;
    sLastBidiRun = 0;
    sBidiRunCount = 0;
}

// ---------------------------------------------------------------------

/* a small helper class used internally to resolve Bidi embedding levels.
   Each line of text caches the embedding level at the start of the line for faster
   relayouting
*/
BidiContext::BidiContext(unsigned char l, QChar::Direction e, BidiContext *p, bool o)
    : level(l) , override(o), dir(e)
{
    parent = p;
    if(p) {
        p->ref();
        basicDir = p->basicDir;
    } else
        basicDir = e;
    count = 0;
}

BidiContext::~BidiContext()
{
    if(parent) parent->deref();
}

void BidiContext::ref() const
{
    count++;
}

void BidiContext::deref() const
{
    count--;
    if(count <= 0) delete this;
}

// ---------------------------------------------------------------------


inline bool operator==(const BidiContext& c1, const BidiContext& c2)
{
    if (&c1 == &c2)
        return true;
    if (c1.level != c2.level || c1.override != c2.override || c1.dir != c2.dir || c1.basicDir != c2.basicDir)
        return false;
    if (!c1.parent)
        return !c2.parent;
    return c2.parent && *c1.parent == *c2.parent;
}

inline bool operator==( const BidiIterator &it1, const BidiIterator &it2 )
{
    if(it1.pos != it2.pos) return false;
    if(it1.obj != it2.obj) return false;
    return true;
}

inline bool operator!=( const BidiIterator &it1, const BidiIterator &it2 )
{
    if(it1.pos != it2.pos) return true;
    if(it1.obj != it2.obj) return true;
    return false;
}

inline bool operator==(const BidiStatus& status1, const BidiStatus& status2)
{
    return status1.eor == status2.eor && status1.last == status2.last && status1.lastStrong == status2.lastStrong;
}
    
inline bool operator!=(const BidiStatus& status1, const BidiStatus& status2)
{
    return !(status1 == status2);
}

// when modifying this function, make sure you check InlineMinMaxIterator::next() as well.
static inline RenderObject *Bidinext(RenderObject *par, RenderObject *current, BidiState *bidi=0,
                                     bool skipInlines = true, bool* endOfInline = 0)
{
    RenderObject *next = 0;
    bool oldEndOfInline = endOfInline ? *endOfInline : false;
    if (oldEndOfInline)
        *endOfInline = false;
    while(current != 0)
    {
        //qDebug() << "current = " << current;
        if (!oldEndOfInline && !current->isFloating() && !current->isReplaced() && !current->isPositioned()) {
            next = current->firstChild();
            if ( next && bidi) {
                EUnicodeBidi ub = next->style()->unicodeBidi();
                if ( ub != UBNormal && !emptyRun ) {
                    EDirection dir = next->style()->direction();
                    QChar::Direction d = ( ub == Embed ? ( dir == RTL ? QChar::DirRLE : QChar::DirLRE )
                                        : ( dir == RTL ? QChar::DirRLO : QChar::DirLRO ) );
                    embed( d, *bidi );
                }
            }
        }
        if (!next) {
            if (!skipInlines && !oldEndOfInline && current->isInlineFlow() && endOfInline) {
                next = current;
                *endOfInline = true;
                break;
            }

            while (current && current != par) {
                next = current->nextSibling();
                if (next) break;
                if ( bidi && current->style()->unicodeBidi() != UBNormal && !emptyRun ) {
                    embed( QChar::DirPDF, *bidi );
                }
                current = current->parent();
                if (!skipInlines && current && current != par && current->isInlineFlow() && endOfInline) {
                    next = current;
                    *endOfInline = true;
                    break;
                }
            }
        }

        if (!next) break;

        if (next->isText() || next->isBR() || next->isFloating() || next->isReplaced() || next->isPositioned() || next->isGlyph()
            || ((!skipInlines || !next->firstChild()) // Always return EMPTY inlines.
                && next->isInlineFlow()))
            break;
        current = next;
        next    = 0;
    }
    return next;
}

static RenderObject *first( RenderObject *par, BidiState *bidi, bool skipInlines = true )
{
    if(!par->firstChild()) return 0;
    RenderObject *o = par->firstChild();

    if (o->isInlineFlow()) {
        if (skipInlines && o->firstChild())
            o = Bidinext( par, o, bidi, skipInlines );
        else
            return o; // Never skip empty inlines.
    }

    if (o && !o->isText() && !o->isBR() && !o->isReplaced() && !o->isFloating() && !o->isPositioned() && !o->isGlyph())
        o = Bidinext( par, o, bidi, skipInlines );
    return o;
}

inline void BidiIterator::increment(BidiState *bidi, bool skipInlines)
{
    if(!obj) return;
    if(obj->isText()) {
        pos++;
        if(pos >= static_cast<RenderText *>(obj)->stringLength()) {
            obj = Bidinext( par, obj, bidi, skipInlines );
            pos = 0;
        }
    } else {
        obj = Bidinext( par, obj, bidi, skipInlines, &endOfInline );
        pos = 0;
    }
}

inline bool BidiIterator::atEnd() const
{
    if(!obj) return true;
    return false;
}

const QChar &BidiIterator::current() const
{
    static QChar nonBreakingSpace(0xA0);

    if (!obj || !obj->isText())
      return nonBreakingSpace;

    RenderText* text = static_cast<RenderText*>(obj);
    if (!text->text())
        return nonBreakingSpace;

    return text->text()[pos];
}

inline QChar::Direction BidiIterator::direction() const
{
    if(!obj || !obj->isText() ) return QChar::DirON;

    RenderText *renderTxt = static_cast<RenderText *>( obj );
    if ( pos >= renderTxt->stringLength() )
        return QChar::DirON;

    return renderTxt->text()[pos].direction();
}

// -------------------------------------------------------------------------------------------------

static void addRun(BidiRun* bidiRun)
{
    if (!sFirstBidiRun)
        sFirstBidiRun = sLastBidiRun = bidiRun;
    else {
        sLastBidiRun->nextRun = bidiRun;
        sLastBidiRun = bidiRun;
    }
    sBidiRunCount++;
    bidiRun->compact = sBuildingCompactRuns;

    // Compute the number of spaces in this run,
    if (bidiRun->obj && bidiRun->obj->isText()) {
        RenderText* text = static_cast<RenderText*>(bidiRun->obj);
        if (text->text()) {
            for (int i = bidiRun->start; i < bidiRun->stop; i++) {
                const QChar c = text->text()[i];
                if (c.unicode() == '\n' || c.category() == QChar::Separator_Space)
                    numSpaces++;
            }
        }
    }
}

static void reverseRuns(int start, int end)
{
    if (start >= end)
        return;

    assert(start >= 0 && end < sBidiRunCount);

    // Get the item before the start of the runs to reverse and put it in
    // |beforeStart|.  |curr| should point to the first run to reverse.
    BidiRun* curr = sFirstBidiRun;
    BidiRun* beforeStart = 0;
    int i = 0;
    while (i < start) {
        i++;
        beforeStart = curr;
        curr = curr->nextRun;
    }

    BidiRun* startRun = curr;
    while (i < end) {
        i++;
        curr = curr->nextRun;
    }
    BidiRun* endRun = curr;
    BidiRun* afterEnd = curr->nextRun;

    i = start;
    curr = startRun;
    BidiRun* newNext = afterEnd;
    while (i <= end) {
        // Do the reversal.
        BidiRun* next = curr->nextRun;
        curr->nextRun = newNext;
        newNext = curr;
        curr = next;
        i++;
    }

    // Now hook up beforeStart and afterEnd to the newStart and newEnd.
    if (beforeStart)
        beforeStart->nextRun = endRun;
    else
        sFirstBidiRun = endRun;

    startRun->nextRun = afterEnd;
    if (!afterEnd)
        sLastBidiRun = startRun;
}

static void chopMidpointsAt(RenderObject* obj, uint pos)
{
    if (!sNumMidpoints) return;
    BidiIterator* midpoints = smidpoints->data();
    for (uint i = 0; i < sNumMidpoints; i++) {
        const BidiIterator& point = midpoints[i];
        if (point.obj == obj && point.pos == pos) {
            sNumMidpoints = i;
            break;
        }
    }
}

static void checkMidpoints(BidiIterator& lBreak)
{
    // Check to see if our last midpoint is a start point beyond the line break.  If so,
    // shave it off the list, and shave off a trailing space if the previous end point isn't
    // white-space: pre.
    if (lBreak.obj && sNumMidpoints && sNumMidpoints%2 == 0) {
        BidiIterator* midpoints = smidpoints->data();
        BidiIterator& endpoint = midpoints[sNumMidpoints-2];
        const BidiIterator& startpoint = midpoints[sNumMidpoints-1];
        BidiIterator currpoint = endpoint;
        while (!currpoint.atEnd() && currpoint != startpoint && currpoint != lBreak)
            currpoint.increment();
        if (currpoint == lBreak) {
            // We hit the line break before the start point.  Shave off the start point.
            sNumMidpoints--;
            if (!endpoint.obj->style()->preserveWS()) {
                if (endpoint.obj->isText()) {
                    // Don't shave a character off the endpoint if it was from a soft hyphen.
                    RenderText* textObj = static_cast<RenderText*>(endpoint.obj);
                    if (endpoint.pos+1 < textObj->length() &&
                        textObj->text()[endpoint.pos+1].unicode() == SOFT_HYPHEN)
                        return;
                }
                endpoint.pos--;
            }
        }
    }
}

static void addMidpoint(const BidiIterator& midpoint)
{
    if (!smidpoints)
        return;

    if (smidpoints->size() <= (int)sNumMidpoints)
        smidpoints->resize(sNumMidpoints+10);

    BidiIterator* midpoints = smidpoints->data();

    // do not place midpoints in inline flows that are going to be skipped by the bidi iteration process.
    // Place them at the next non-skippable object instead.
    // #### eventually, we may want to have the same iteration in bidi and in findNextLineBreak,
    //      then this extra complexity can go away.
    if (midpoint.obj && midpoint.obj->isInlineFlow() && (midpoint.obj->firstChild() || midpoint.endOfInline)) {
        BidiIterator n = midpoint;
        n.increment();
        assert(!n.endOfInline);
        // we'll recycle the endOfInline flag to mean : don't include this stop point, stop right before it.
        // this is necessary because we just advanced our position to skip an inline, so we passed the real stop point
        n.endOfInline = true;
        if (!n.atEnd())
            midpoints[sNumMidpoints++] = n;
    } else {
        assert(!midpoint.endOfInline);
        midpoints[sNumMidpoints++] = midpoint;
    }
}

static void appendRunsForObject(int start, int end, RenderObject* obj, BidiState &bidi)
{
    if (start > end || obj->isFloating() ||
        (obj->isPositioned() && !obj->hasStaticX() && !obj->hasStaticY()))
        return;

    bool haveNextMidpoint = (smidpoints && sCurrMidpoint < sNumMidpoints);
    BidiIterator nextMidpoint;
    if (haveNextMidpoint)
        nextMidpoint = smidpoints->at(sCurrMidpoint);
    if (betweenMidpoints) {
        if (!(haveNextMidpoint && nextMidpoint.obj == obj))
            return;
        // This is a new start point. Stop ignoring objects and
        // adjust our start.
        betweenMidpoints = false;
        start = nextMidpoint.pos;
        sCurrMidpoint++;
        if (start < end)
            return appendRunsForObject(start, end, obj, bidi);
    }
    else {
        if (!smidpoints || !haveNextMidpoint || (obj != nextMidpoint.obj)) {
            addRun(new (obj->renderArena()) BidiRun(start, end, obj, bidi.context, dir));
            return;
        }

        // An end midpoint has been encountered within our object.  We
        // need to go ahead and append a run with our endpoint.
        if (int(nextMidpoint.pos+1) <= end) {
            betweenMidpoints = true;
            sCurrMidpoint++;
            if (nextMidpoint.pos != UINT_MAX) { // UINT_MAX means stop at the object and don't include any of it.
               if (!nextMidpoint.endOfInline) // In this context, this flag means the stop point is exclusive, not inclusive (see addMidpoint).
                   addRun(new (obj->renderArena())
                       BidiRun(start, nextMidpoint.pos+1, obj, bidi.context, dir));
                return appendRunsForObject(nextMidpoint.pos+1, end, obj, bidi);
            }
        }
        else
           addRun(new (obj->renderArena()) BidiRun(start, end, obj, bidi.context, dir));
    }
}

static void appendRun( BidiState &bidi )
{
    if ( emptyRun ) return;
#if BIDI_DEBUG > 1
    qDebug() << "appendRun: dir="<<(int)dir;
#endif

    int start = bidi.sor.pos;
    RenderObject *obj = bidi.sor.obj;
    while( obj && obj != bidi.eor.obj ) {
        appendRunsForObject(start, obj->length(), obj, bidi);
        start = 0;
        obj = Bidinext( bidi.sor.par, obj);
    }
    if (obj)
        appendRunsForObject(start, bidi.eor.pos+1, obj, bidi);

    bidi.eor.increment();
    bidi.sor = bidi.eor;
    dir = QChar::DirON;
    bidi.status.eor = QChar::DirON;
}

static void embed( QChar::Direction d, BidiState &bidi )
{
#if BIDI_DEBUG > 1
    qDebug("*** embed dir=%d emptyrun=%d", d, emptyRun );
#endif
    if ( d == QChar::DirPDF ) {
	BidiContext *c = bidi.context->parent;
	if (c) {
	    if ( bidi.eor != bidi.last ) {
		appendRun( bidi );
		bidi.eor = bidi.last;
	    }
	    appendRun( bidi );
	    emptyRun = true;
	    bidi.status.last = bidi.context->dir;
	    bidi.context->deref();
	    bidi.context = c;
	    if(bidi.context->override)
		dir = bidi.context->dir;
	    else
		dir = QChar::DirON;
	    bidi.status.lastStrong = bidi.context->dir;
	}
    } else {
	QChar::Direction runDir;
	if( d == QChar::DirRLE || d == QChar::DirRLO )
	    runDir = QChar::DirR;
	else
	    runDir = QChar::DirL;
	bool override;
	if( d == QChar::DirLRO || d == QChar::DirRLO )
	    override = true;
	else
	    override = false;

	unsigned char level = bidi.context->level;
	if ( runDir == QChar::DirR ) {
	    if(level%2) // we have an odd level
		level += 2;
	    else
		level++;
	} else {
	    if(level%2) // we have an odd level
		level++;
	    else
		level += 2;
	}

	if(level < 61) {
	    if ( bidi.eor != bidi.last ) {
                appendRun( bidi );
                bidi.eor = bidi.last;
            }
            appendRun( bidi );
            emptyRun = true;

	    bidi.context = new BidiContext(level, runDir, bidi.context, override);
	    bidi.context->ref();
	    dir = runDir;
	    bidi.status.last = runDir;
	    bidi.status.lastStrong = runDir;
	    bidi.status.eor = runDir;
	}
    }
}

InlineFlowBox* RenderBlock::createLineBoxes(RenderObject* obj)
{
    // See if we have an unconstructed line box for this object that is also
    // the last item on the line.
    KHTMLAssert(obj->isInlineFlow() || obj == this);
    RenderFlow* flow = static_cast<RenderFlow*>(obj);

    // Get the last box we made for this render object.
    InlineFlowBox* box = flow->lastLineBox();

    // If this box is constructed then it is from a previous line, and we need
    // to make a new box for our line.  If this box is unconstructed but it has
    // something following it on the line, then we know we have to make a new box
    // as well.  In this situation our inline has actually been split in two on
    // the same line (this can happen with very fancy language mixtures).
    if (!box || box->isConstructed() || box->nextOnLine()) {
        // We need to make a new box for this render object.  Once
        // made, we need to place it at the end of the current line.
        InlineBox* newBox = obj->createInlineBox(false, obj == this);
        KHTMLAssert(newBox->isInlineFlowBox());
        box = static_cast<InlineFlowBox*>(newBox);
        box->setFirstLineStyleBit(m_firstLine);

        // We have a new box. Append it to the inline box we get by constructing our
        // parent.  If we have hit the block itself, then |box| represents the root
        // inline box for the line, and it doesn't have to be appended to any parent
        // inline.
        if (obj != this) {
            InlineFlowBox* parentBox = createLineBoxes(obj->parent());
            parentBox->addToLine(box);
        }
    }

    return box;
}

RootInlineBox* RenderBlock::constructLine(const BidiIterator &/*start*/, const BidiIterator &end)
{
    if (!sFirstBidiRun)
        return 0; // We had no runs. Don't make a root inline box at all. The line is empty.

    InlineFlowBox* parentBox = 0;
    for (BidiRun* r = sFirstBidiRun; r; r = r->nextRun) {
        // Create a box for our object.
        r->box = r->obj->createInlineBox(r->obj->isPositioned(), false);

        // If we have no parent box yet, or if the run is not simply a sibling,
        // then we need to construct inline boxes as necessary to properly enclose the
        // run's inline box.
        if (!parentBox || (parentBox->object() != r->obj->parent()))
            // Create new inline boxes all the way back to the appropriate insertion point.
            parentBox = createLineBoxes(r->obj->parent());

        // Append the inline box to this line.
        parentBox->addToLine(r->box);
    }

    // We should have a root inline box.  It should be unconstructed and
    // be the last continuation of our line list.
    KHTMLAssert(lastLineBox() && !lastLineBox()->isConstructed());

    // Set bits on our inline flow boxes that indicate which sides should
    // paint borders/margins/padding.  This knowledge will ultimately be used when
    // we determine the horizontal positions and widths of all the inline boxes on
    // the line.
    RenderObject* endObject = 0;
    bool lastLine = !end.obj;
    if (end.obj && end.pos == 0)
        endObject = end.obj;
    lastLineBox()->determineSpacingForFlowBoxes(lastLine, endObject);

    // Now mark the line boxes as being constructed.
    lastLineBox()->setConstructed();

    // Return the last line.
    return lastRootBox();
}

void RenderBlock::computeHorizontalPositionsForLine(InlineFlowBox* lineBox, BidiState &bidi)
{
    // First determine our total width.
    int totWidth = lineBox->getFlowSpacingWidth();
    BidiRun* r = 0;
    for (r = sFirstBidiRun; r; r = r->nextRun) {
        if (r->obj->isPositioned())
            continue; // Positioned objects are only participating to figure out their
                      // correct static x position.  They have no effect on the width.
        if (r->obj->isText())
            r->box->setWidth(static_cast<RenderText *>(r->obj)->width(r->start, r->stop-r->start, m_firstLine));
        else if (!r->obj->isInlineFlow()) {
            r->obj->calcWidth(); // Is this really needed or the object width is already correct here ?
            r->box->setWidth(r->obj->width());
            totWidth += r->obj->marginLeft() + r->obj->marginRight();
        }
        totWidth += r->box->width();
    }

    // Armed with the total width of the line (without justification),
    // we now examine our text-align property in order to determine where to position the
    // objects horizontally.  The total width of the line can be increased if we end up
    // justifying text.
    int x = leftOffset(m_height);
    int availableWidth = lineWidth(m_height);
    switch(style()->textAlign()) {
        case LEFT:
        case KHTML_LEFT:
            if (style()->direction() == RTL && totWidth > availableWidth)
                x -= (totWidth - availableWidth);
            numSpaces = 0;
            break;
        case JUSTIFY:
            if (numSpaces != 0 && !bidi.current.atEnd() && !bidi.current.obj->isBR() )
                break;
            // fall through
        case TAAUTO:
            numSpaces = 0;
            // for right to left fall through to right aligned
            if (bidi.context->basicDir == QChar::DirL)
                break;
        case RIGHT:
        case KHTML_RIGHT:
            if (style()->direction() == RTL || totWidth < availableWidth)
                x += availableWidth - totWidth;
            numSpaces = 0;
            break;
        case CENTER:
        case KHTML_CENTER:
            int xd = (availableWidth - totWidth)/2;
            x += xd >0 ? xd : 0;
            numSpaces = 0;
            break;
    }

    if (numSpaces > 0) {
        for (r = sFirstBidiRun; r; r = r->nextRun) {
            int spaceAdd = 0;
            if (numSpaces > 0 && r->obj->isText()) {
                // get the number of spaces in the run
                int spaces = 0;
                for ( int i = r->start; i < r->stop; i++ ) {
                    const QChar c = static_cast<RenderText *>(r->obj)->text()[i];
                    if (c.category() == QChar::Separator_Space || c == '\n')
                        spaces++;
                }

                KHTMLAssert(spaces <= numSpaces);

                // Only justify text with white-space: normal.
                if (r->obj->style()->whiteSpace() == NORMAL) {
                    spaceAdd = (availableWidth - totWidth)*spaces/numSpaces;
                    spaceAdd = qMax(0, spaceAdd);
                    static_cast<InlineTextBox*>(r->box)->setSpaceAdd(spaceAdd);
                    totWidth += spaceAdd;
                }
                numSpaces -= spaces;
            }
        }
    }

    // The widths of all runs are now known.  We can now place every inline box (and
    // compute accurate widths for the inline flow boxes).
    int rightPos = lineBox->placeBoxesHorizontally(x);
    if (rightPos > m_overflowWidth)
        m_overflowWidth = rightPos; // FIXME: Work for rtl overflow also.
    if (x < 0)
        m_overflowLeft = qMin(m_overflowLeft, x);
}

void RenderBlock::computeVerticalPositionsForLine(RootInlineBox* lineBox)
{
    lineBox->verticallyAlignBoxes(m_height);
    lineBox->setBlockHeight(m_height);

    // Check for page-breaks
    if (canvas()->pagedMode() && !lineBox->afterPageBreak())
        // If we get a page-break we might need to redo the line-break
        if (clearLineOfPageBreaks(lineBox) && hasFloats()) return;

    // See if the line spilled out.  If so set overflow height accordingly.
    int bottomOfLine = lineBox->bottomOverflow();
    if (bottomOfLine > m_height && bottomOfLine > m_overflowHeight)
        m_overflowHeight = bottomOfLine;

    bool beforeContent = true;

    // Now make sure we place replaced render objects correctly.
    for (BidiRun* r = sFirstBidiRun; r; r = r->nextRun) {

        // For positioned placeholders, cache the static Y position an object with non-inline display would have.
        // Either it is unchanged if it comes before any real linebox, or it must clear the current line (already accounted in m_height).
        // This value will be picked up by position() if relevant.
        if (r->obj->isPositioned())
            r->box->setYPos( beforeContent && r->obj->isBox() ? static_cast<RenderBox*>(r->obj)->staticY() : m_height );
        else if (beforeContent)
           beforeContent = false;

        // Position is used to properly position both replaced elements and
        // to update the static normal flow x/y of positioned elements.
        r->obj->position(r->box, r->start, r->stop - r->start, r->level%2);
    }
}

bool RenderBlock::clearLineOfPageBreaks(InlineFlowBox* lineBox)
{
    bool doPageBreak = false;
    // Check for physical page-breaks
    int xpage = crossesPageBreak(lineBox->topOverflow(), lineBox->bottomOverflow());
    if (xpage) {
#ifdef PAGE_DEBUG
        qDebug() << renderName() << " Line crosses to page " << xpage;
        qDebug() << renderName() << " at pos " << lineBox->yPos() << " height " << lineBox->height();
#endif

        doPageBreak = true;
        // check page-break-inside
        if (!style()->pageBreakInside()) {
            if (parent()->canClear(this, PageBreakNormal)) {
                setNeedsPageClear(true);
                doPageBreak = false;
            }
#ifdef PAGE_DEBUG
            else
                qDebug() << "Ignoring page-break-inside: avoid";
#endif
        }
        // check orphans
        int orphans = 0;
        InlineRunBox* box = lineBox->prevLineBox();
        while (box && orphans < style()->orphans()) {
            orphans++;
            box = box->prevLineBox();
        }

        if (orphans == 0) {
            setNeedsPageClear(true);
            doPageBreak = false;
        } else
        if (orphans < style()->orphans() ) {
#ifdef PAGE_DEBUG
            qDebug() << "Orphans: " << orphans;
#endif
            // Orphans is a level 2 page-break rule and can be broken only
            // if the break is physically required.
            if (parent()->canClear(this, PageBreakHarder)) {
                // move block instead
                setNeedsPageClear(true);
                doPageBreak = false;
            }
#ifdef PAGE_DEBUG
            else
                qDebug() << "Ignoring violated orphans";
#endif
        }
        if (doPageBreak) {
#ifdef PAGE_DEBUG
            int oldYPos = lineBox->yPos();
#endif
            int pTop = pageTopAfter(lineBox->yPos());
            m_height = pTop;
            lineBox->setAfterPageBreak(true);
            lineBox->verticallyAlignBoxes(m_height);
            if (lineBox->yPos() < pTop) {
                // ### serious crap. render_line is sometimes placing lines too high
                // qDebug() << "page top overflow by repositioned line";
                int heightIncrease = pTop - lineBox->yPos();
                m_height = pTop + heightIncrease;
                lineBox->verticallyAlignBoxes(m_height);
            }
#ifdef PAGE_DEBUG
            qDebug() << "Cleared line " << lineBox->yPos() - oldYPos << "px";
#endif
            setContainsPageBreak(true);
        }
    }
    return doPageBreak;
}

// collects one line of the paragraph and transforms it to visual order
void RenderBlock::bidiReorderLine(const BidiIterator &start, const BidiIterator &end, BidiState &bidi)
{
    if ( start == end ) {
        if ( start.current() == '\n' ) {
            m_height += lineHeight( m_firstLine );
        }
        return;
    }

#if BIDI_DEBUG > 1
    qDebug() << "reordering Line from " << start.obj << "/" << start.pos << " to " << end.obj << "/" << end.pos;
#endif

    sFirstBidiRun = 0;
    sLastBidiRun = 0;
    sBidiRunCount = 0;

    //    context->ref();

    dir = QChar::DirON;
    emptyRun = true;

    numSpaces = 0;

    bidi.current = start;
    bidi.last = bidi.current;
    bool atEnd = false;
    while( 1 ) {
        QChar::Direction dirCurrent;
        if (atEnd) {
            //qDebug() << "atEnd";
            BidiContext *c = bidi.context;
            if ( bidi.current.atEnd())
                while ( c->parent )
                    c = c->parent;
            dirCurrent = c->dir;
        }
        else if (bidi.context->override) {
            dirCurrent = bidi.context->dir;
        }
        else {
            dirCurrent = bidi.current.direction();
        }

#ifndef QT_NO_UNICODETABLES

#if BIDI_DEBUG > 1
        qDebug() << "directions: dir=" << (int)dir << " current=" << (int)dirCurrent << " last=" << bidi.status.last << " eor=" << bidi.status.eor << " lastStrong=" << bidi.status.lastStrong << " embedding=" << (int)bidi.context->dir << " level =" << (int)bidi.context->level;
#endif

        switch(dirCurrent) {

            // embedding and overrides (X1-X9 in the Bidi specs)
        case QChar::DirRLE:
        case QChar::DirLRE:
        case QChar::DirRLO:
        case QChar::DirLRO:
        case QChar::DirPDF:
            embed( dirCurrent, bidi );
            break;

            // strong types
        case QChar::DirL:
            if(dir == QChar::DirON)
                dir = QChar::DirL;
            switch(bidi.status.last)
                {
                case QChar::DirL:
                    bidi.eor = bidi.current; bidi.status.eor = QChar::DirL; break;
                case QChar::DirR:
                case QChar::DirAL:
                case QChar::DirEN:
                case QChar::DirAN:
                    appendRun( bidi );
                    break;
                case QChar::DirES:
                case QChar::DirET:
                case QChar::DirCS:
                case QChar::DirBN:
                case QChar::DirB:
                case QChar::DirS:
                case QChar::DirWS:
                case QChar::DirON:
                     if( bidi.status.eor != QChar::DirL ) {
                        //last stuff takes embedding dir
                        if(bidi.context->dir == QChar::DirL || bidi.status.lastStrong == QChar::DirL) {
                            if ( bidi.status.eor != QChar::DirEN && bidi.status.eor != QChar::DirAN && bidi.status.eor != QChar::DirON )
                                appendRun( bidi );
                            dir = QChar::DirL;
                            bidi.eor = bidi.current;
                            bidi.status.eor = QChar::DirL;
                        } else {
                            if ( bidi.status.eor == QChar::DirEN || bidi.status.eor == QChar::DirAN )
                            {
                                dir = bidi.status.eor;
                                appendRun( bidi );
                            }
                            dir = QChar::DirR;
                            bidi.eor = bidi.last;
                            appendRun( bidi );
                            dir = QChar::DirL;
                            bidi.status.eor = QChar::DirL;
                        }
                    } else {
                        bidi.eor = bidi.current; bidi.status.eor = QChar::DirL;
                    }
                default:
                    break;
                }
            bidi.status.lastStrong = QChar::DirL;
            break;
        case QChar::DirAL:
        case QChar::DirR:
            if(dir == QChar::DirON) dir = QChar::DirR;
            switch(bidi.status.last)
                {
                case QChar::DirR:
                case QChar::DirAL:
                    bidi.eor = bidi.current; bidi.status.eor = QChar::DirR; break;
                case QChar::DirL:
                case QChar::DirEN:
                case QChar::DirAN:
                    appendRun( bidi );
		    dir = QChar::DirR;
		    bidi.eor = bidi.current;
		    bidi.status.eor = QChar::DirR;
                    break;
                case QChar::DirES:
                case QChar::DirET:
                case QChar::DirCS:
                case QChar::DirBN:
                case QChar::DirB:
                case QChar::DirS:
                case QChar::DirWS:
                case QChar::DirON:
                    if( !(bidi.status.eor == QChar::DirR) && !(bidi.status.eor == QChar::DirAL) ) {
                        //last stuff takes embedding dir
                        if(bidi.context->dir == QChar::DirR || bidi.status.lastStrong == QChar::DirR
                            || bidi.status.lastStrong == QChar::DirAL) {
                            appendRun( bidi );
                            dir = QChar::DirR;
                            bidi.eor = bidi.current;
                            bidi.status.eor = QChar::DirR;
                        } else {
                            dir = QChar::DirL;
                            bidi.eor = bidi.last;
                            appendRun( bidi );
                            dir = QChar::DirR;
                            bidi.status.eor = QChar::DirR;
                        }
                    } else {
                        bidi.eor = bidi.current; bidi.status.eor = QChar::DirR;
                    }
                default:
                    break;
                }
            bidi.status.lastStrong = dirCurrent;
            break;

            // weak types:

        case QChar::DirNSM:
            // ### if @sor, set dir to dirSor
            break;
        case QChar::DirEN:
            if(!(bidi.status.lastStrong == QChar::DirAL)) {
                // if last strong was AL change EN to AN
                if(dir == QChar::DirON) {
                    dir = QChar::DirL;
                }
                switch(bidi.status.last)
                    {
                    case QChar::DirET:
			if ( bidi.status.lastStrong == QChar::DirR || bidi.status.lastStrong == QChar::DirAL ) {
			    appendRun( bidi );
                            dir = QChar::DirEN;
                            bidi.status.eor = QChar::DirEN;
			}
			// fall through
                    case QChar::DirEN:
                    case QChar::DirL:
                        bidi.eor = bidi.current;
                        bidi.status.eor = dirCurrent;
                        break;
                    case QChar::DirR:
                    case QChar::DirAL:
                    case QChar::DirAN:
                        appendRun( bidi );
			bidi.status.eor = QChar::DirEN;
                        dir = QChar::DirEN; break;
                    case QChar::DirES:
                    case QChar::DirCS:
                        if(bidi.status.eor == QChar::DirEN) {
                            bidi.eor = bidi.current; break;
                        }
                    case QChar::DirBN:
                    case QChar::DirB:
                    case QChar::DirS:
                    case QChar::DirWS:
                    case QChar::DirON:
                        if(bidi.status.eor == QChar::DirR) {
                            // neutrals go to R
                            bidi.eor = bidi.last;
                            appendRun( bidi );
                            dir = QChar::DirEN;
                            bidi.status.eor = QChar::DirEN;
                        }
                        else if( bidi.status.eor == QChar::DirL ||
                                 (bidi.status.eor == QChar::DirEN && bidi.status.lastStrong == QChar::DirL)) {
                            bidi.eor = bidi.current; bidi.status.eor = dirCurrent;
                        } else {
                            // numbers on both sides, neutrals get right to left direction
                            if(dir != QChar::DirL) {
                                appendRun( bidi );
                                bidi.eor = bidi.last;
                                dir = QChar::DirR;
                                appendRun( bidi );
                                dir = QChar::DirEN;
                                bidi.status.eor = QChar::DirEN;
                            } else {
                                bidi.eor = bidi.current; bidi.status.eor = dirCurrent;
                            }
                        }
                    default:
                        break;
                    }
                break;
            }
        case QChar::DirAN:
            dirCurrent = QChar::DirAN;
            if(dir == QChar::DirON) dir = QChar::DirAN;
            switch(bidi.status.last)
                {
                case QChar::DirL:
                case QChar::DirAN:
                    bidi.eor = bidi.current; bidi.status.eor = QChar::DirAN; break;
                case QChar::DirR:
                case QChar::DirAL:
                case QChar::DirEN:
                    appendRun( bidi );
                    dir = QChar::DirAN; bidi.status.eor = QChar::DirAN;
                    break;
                case QChar::DirCS:
                    if(bidi.status.eor == QChar::DirAN) {
                        bidi.eor = bidi.current; break;
                    }
                case QChar::DirES:
                case QChar::DirET:
                case QChar::DirBN:
                case QChar::DirB:
                case QChar::DirS:
                case QChar::DirWS:
                case QChar::DirON:
                    if(bidi.status.eor == QChar::DirR) {
                        // neutrals go to R
                        bidi.eor = bidi.last;
                        appendRun( bidi );
                        dir = QChar::DirAN;
                        bidi.status.eor = QChar::DirAN;
                    } else if( bidi.status.eor == QChar::DirL ||
                               (bidi.status.eor == QChar::DirEN && bidi.status.lastStrong == QChar::DirL)) {
                        bidi.eor = bidi.current; bidi.status.eor = dirCurrent;
                    } else {
                        // numbers on both sides, neutrals get right to left direction
                        if(dir != QChar::DirL) {
                            appendRun( bidi );
                            bidi.eor = bidi.last;
                            dir = QChar::DirR;
                            appendRun( bidi );
                            dir = QChar::DirAN;
                            bidi.status.eor = QChar::DirAN;
                        } else {
                            bidi.eor = bidi.current; bidi.status.eor = dirCurrent;
                        }
                    }
                default:
                    break;
                }
            break;
        case QChar::DirES:
        case QChar::DirCS:
            break;
        case QChar::DirET:
            if(bidi.status.last == QChar::DirEN) {
                dirCurrent = QChar::DirEN;
                bidi.eor = bidi.current; bidi.status.eor = dirCurrent;
                break;
            }
            break;

        // boundary neutrals should be ignored
        case QChar::DirBN:
            break;
            // neutrals
        case QChar::DirB:
            // ### what do we do with newline and paragraph separators that come to here?
            break;
        case QChar::DirS:
            // ### implement rule L1
            break;
        case QChar::DirWS:
            break;
        case QChar::DirON:
            break;
        default:
            break;
        }

        //cout << "     after: dir=" << //        dir << " current=" << dirCurrent << " last=" << status.last << " eor=" << status.eor << " lastStrong=" << status.lastStrong << " embedding=" << context->dir << endl;

        if(bidi.current.atEnd()) break;

        // set status.last as needed.
        switch(dirCurrent)
            {
            case QChar::DirET:
            case QChar::DirES:
            case QChar::DirCS:
            case QChar::DirS:
            case QChar::DirWS:
            case QChar::DirON:
                switch(bidi.status.last)
                    {
                    case QChar::DirL:
                    case QChar::DirR:
                    case QChar::DirAL:
                    case QChar::DirEN:
                    case QChar::DirAN:
                        bidi.status.last = dirCurrent;
                        break;
                    default:
                        bidi.status.last = QChar::DirON;
                    }
                break;
            case QChar::DirNSM:
            case QChar::DirBN:
                // ignore these
                break;
            case QChar::DirEN:
                if ( bidi.status.last == QChar::DirL ) {
                    break;
                }
                // fall through
            default:
                bidi.status.last = dirCurrent;
            }
#endif

	if ( atEnd ) break;
        bidi.last = bidi.current;

	if ( emptyRun ) {
	    bidi.sor = bidi.current;
	    bidi.eor = bidi.current;
	    emptyRun = false;
	}

	// this causes the operator ++ to open and close embedding levels as needed
	// for the CSS unicode-bidi property
        bidi.current.increment( &bidi );

	if ( bidi.current == end ) {
	    if ( emptyRun )
		break;
	    atEnd = true;
	}
    }

#if BIDI_DEBUG > 0
    qDebug() << "reached end of line current=" << bidi.current.obj << "/" << bidi.current.pos
		  << ", eor=" << bidi.eor.obj << "/" << bidi.eor.pos << endl;
#endif
    if ( !emptyRun && bidi.sor != bidi.current ) {
	    bidi.eor = bidi.last;
	    appendRun( bidi );
    }

    // reorder line according to run structure...

    // first find highest and lowest levels
    uchar levelLow = 128;
    uchar levelHigh = 0;
    BidiRun *r = sFirstBidiRun;
    while ( r ) {
        if ( r->level > levelHigh )
            levelHigh = r->level;
        if ( r->level < levelLow )
            levelLow = r->level;
        r = r->nextRun;
    }

    // implements reordering of the line (L2 according to Bidi spec):
    // L2. From the highest level found in the text to the lowest odd level on each line,
    // reverse any contiguous sequence of characters that are at that level or higher.

    // reversing is only done up to the lowest odd level
    if( !(levelLow%2) ) levelLow++;

    int count = sBidiRunCount - 1;

    // do not reverse for visually ordered web sites
    if(!style()->visuallyOrdered()) {
        while(levelHigh >= levelLow) {
            int i = 0;
            BidiRun* currRun = sFirstBidiRun;
            while ( i < count ) {
                while(i < count && currRun && currRun->level < levelHigh) {
                    i++;
                    currRun = currRun->nextRun;
                }
                int start = i;
                while(i <= count && currRun && currRun->level >= levelHigh) {
                    i++;
                    currRun = currRun->nextRun;
                }
                int end = i-1;
                reverseRuns(start, end);
            }
            levelHigh--;
        }
    }

#if BIDI_DEBUG > 0
    qDebug() << "visual order is:";
    for (BidiRun* curr = sFirstBidiRun; curr; curr = curr->nextRun)
        qDebug() << "    " << curr;
#endif
}

void RenderBlock::layoutInlineChildren(bool relayoutChildren, int breakBeforeLine)
{
    BidiState bidi;

    m_overflowHeight = 0;

    invalidateVerticalPosition();
#ifdef DEBUG_LAYOUT
    QTime qt;
    qt.start();
    qDebug() << renderName() << " layoutInlineChildren( " << this <<" )";
#endif
#if BIDI_DEBUG > 1 || defined( DEBUG_LINEBREAKS )
    qDebug() << " ------- bidi start " << this << " -------";
#endif

    m_height = borderTop() + paddingTop();
    int toAdd = borderBottom() + paddingBottom();
    if (m_layer && scrollsOverflowX() && style()->height().isAuto())
        toAdd += m_layer->horizontalScrollbarHeight();

    // Figure out if we should clear our line boxes.
    bool fullLayout = !firstLineBox() || !firstChild() || selfNeedsLayout() || relayoutChildren || hasFloats();

    if (fullLayout)
        deleteInlineBoxes();

    // Text truncation only kicks in if your overflow isn't visible and your
    // text-overflow-mode isn't clip.
    bool hasTextOverflow = style()->textOverflow() && hasOverflowClip();

    // Walk all the lines and delete our ellipsis line boxes if they exist.
    if (hasTextOverflow)
         deleteEllipsisLineBoxes();

    if (firstChild()) {
        // layout replaced elements
        RenderObject *o = first( this, 0, false );
        while ( o ) {
            invalidateVerticalPosition();
            if (!fullLayout && o->markedForRepaint()) {
                o->repaintDuringLayout();
                o->setMarkedForRepaint(false);
            }
            if (o->isReplaced() || o->isFloating() || o->isPositioned()) {

                if ((!o->isPositioned() || o->isPosWithStaticDim()) && 
                        (relayoutChildren || o->style()->width().isPercent() || o->style()->height().isPercent()))
                    o->setChildNeedsLayout(true, false);

                if (o->isPositioned()) {
                    if (!o->inPosObjectList())
                        o->containingBlock()->insertPositionedObject(o);
                    if (fullLayout)
                        static_cast<RenderBox*>(o)->RenderBox::deleteInlineBoxes();
                } else {
                    if (fullLayout || o->needsLayout())
                        static_cast<RenderBox*>(o)->RenderBox::dirtyInlineBoxes(fullLayout);
                    o->layoutIfNeeded();
                }
            }
            else {
                if (fullLayout || o->selfNeedsLayout()) {
                    o->dirtyInlineBoxes(fullLayout);
                    o->setMarkedForRepaint(false);
                }
                o->setNeedsLayout(false);
            }
            o = Bidinext( this, o, 0, false );
        }

        BidiContext *startEmbed;
        if( style()->direction() == LTR ) {
            startEmbed = new BidiContext( 0, QChar::DirL );
            bidi.status.eor = QChar::DirL;
        } else {
            startEmbed = new BidiContext( 1, QChar::DirR );
            bidi.status.eor = QChar::DirR;
        }
        startEmbed->ref();

	bidi.status.lastStrong = QChar::DirON;
	bidi.status.last = QChar::DirON;

        bidi.context = startEmbed;

        // We want to skip ahead to the first dirty line
        BidiIterator start;
        RootInlineBox* startLine = determineStartPosition(fullLayout, start, bidi);

        // Then look forward to see if we can find a clean area that is clean up to the end.
        BidiIterator cleanLineStart;
        BidiStatus cleanLineBidiStatus;
        BidiContext* cleanLineBidiContext = 0;
        int endLineYPos = 0;
        RootInlineBox* endLine = (fullLayout || !startLine) ? 
                                 0 : determineEndPosition(startLine, cleanLineStart, cleanLineBidiStatus, cleanLineBidiContext, endLineYPos);

        // Extract the clean area. We will add it back if we determine that we're able to
        // synchronize after relayouting the dirty area.
        if (endLine)
            for (RootInlineBox* line = endLine; line; line = line->nextRootBox())
                line->extractLine();

        // Delete the dirty area.
        if (startLine) {
            RenderArena* arena = renderArena();
            RootInlineBox* box = startLine;
            while (box) {
                RootInlineBox* next = box->nextRootBox();
                box->deleteLine(arena);
                box = next;
            }
            startLine = 0;
        }
        BidiIterator end = start;
        bool endLineMatched = false;
        m_firstLine = true;

        if (!smidpoints)
            smidpoints = new QVector<BidiIterator>;

        sNumMidpoints = 0;
        sCurrMidpoint = 0;
        sCompactFirstBidiRun = sCompactLastBidiRun = 0;
        sCompactBidiRunCount = 0;

        previousLineBrokeAtBR = true;

        int lineCount = 0;
        bool pagebreakHint = false;
        int oldPos = 0;
        BidiIterator oldStart;
        BidiState oldBidi;
        const bool pagedMode = canvas()->pagedMode();

        while( !end.atEnd() ) {
            start = end;
            if (endLine && (endLineMatched = matchedEndLine(start, bidi.status, bidi.context, cleanLineStart, cleanLineBidiStatus, cleanLineBidiContext, endLine, endLineYPos)))
                break;
            lineCount++;
            betweenMidpoints = false;
            isLineEmpty = true;
            pagebreakHint = false;
            if (pagedMode) {
                oldPos = m_height;
                oldStart = start;
                oldBidi = bidi;
            }
            if (lineCount == breakBeforeLine) {
                m_height = pageTopAfter(oldPos);
                pagebreakHint = true;
            }
redo_linebreak:
            end = findNextLineBreak(start, bidi);
            if( start.atEnd() ) {
                deleteBidiRuns(renderArena());
                break;
            }
            if (!isLineEmpty) {
                bidiReorderLine(start, end, bidi);

                // Now that the runs have been ordered, we create the line boxes.
                // At the same time we figure out where border/padding/margin should be applied for
                // inline flow boxes.

                RootInlineBox* lineBox = 0;
                if (sBidiRunCount) {
                    lineBox = constructLine(start, end);
                    if (lineBox) {
                        lineBox->setEndsWithBreak(previousLineBrokeAtBR);
                        if (pagebreakHint) lineBox->setAfterPageBreak(true);

                        // Now we position all of our text runs horizontally.
                        computeHorizontalPositionsForLine(lineBox, bidi);

                        // Now position our text runs vertically.
                        computeVerticalPositionsForLine(lineBox);

                        // SVG
                        if (lineBox->isSVGRootInlineBox()) {
                            //qDebug() << "svgrootinline box:" << endl;
                            WebCore::SVGRootInlineBox* svgLineBox = static_cast<WebCore::SVGRootInlineBox*>(lineBox);
                            svgLineBox->computePerCharacterLayoutInformation();
                        }

                        deleteBidiRuns(renderArena());

                        if (lineBox->afterPageBreak() && hasFloats() && !pagebreakHint) {
                            start = end = oldStart;
                            bidi = oldBidi;
                            m_height = pageTopAfter(oldPos);
                            deleteLastLineBox(renderArena());
                            pagebreakHint = true;
                            goto redo_linebreak;
                        }
                    }
                }

                if( end == start || (end.obj && end.obj->isBR() && !start.obj->isBR() ) ) {
                    end.increment(&bidi);
                } else if (end.obj && end.obj->style()->preserveLF() && end.current() == QChar('\n')) {
                    end.increment(&bidi);
                }

                if (lineBox)
                    lineBox->setLineBreakInfo(end.obj, end.pos, bidi.status, bidi.context);

                m_firstLine = false;
                newLine();
            }

            sNumMidpoints = 0;
            sCurrMidpoint = 0;
            sCompactFirstBidiRun = sCompactLastBidiRun = 0;
            sCompactBidiRunCount = 0;
        }
        startEmbed->deref();
        //embed->deref();

        if (endLine) {
            if (endLineMatched) {
                // Attach all the remaining lines, and then adjust their y-positions as needed.
                for (RootInlineBox* line = endLine; line; line = line->nextRootBox())
                    line->attachLine();

                // Now apply the offset to each line if needed.
                int delta = m_height - endLineYPos;
                if (delta) {
                    for (RootInlineBox* line = endLine; line; line = line->nextRootBox())
                        line->adjustPosition(0, delta);
                }
                m_height = lastRootBox()->blockHeight();
            } else {
                // Delete all the remaining lines.
                InlineRunBox* line = endLine;
                RenderArena* arena = renderArena();
                while (line) {
                    InlineRunBox* next = line->nextLineBox();
                    line->deleteLine(arena);
                    line = next;
                }
            }
        }
   }

    sNumMidpoints = 0;
    sCurrMidpoint = 0;


    // If we violate widows page-breaking rules, we set a hint and relayout.
    // Note that the widows rule might still be violated afterwards if the lines have become wider
    if (canvas()->pagedMode() && containsPageBreak() && breakBeforeLine == 0)
    {
        int orphans = 0;
        int widows = 0;
        // find breaking line
        InlineRunBox* lineBox = firstLineBox();
        while (lineBox) {
            if (lineBox->isInlineFlowBox()) {
                InlineFlowBox* flowBox = static_cast<InlineFlowBox*>(lineBox);
                if (flowBox->afterPageBreak()) break;
            }
            orphans++;
            lineBox = lineBox->nextLineBox();
        }
        InlineFlowBox* pageBreaker = static_cast<InlineFlowBox*>(lineBox);
        if (!pageBreaker) goto no_break;
        // count widows
        while (lineBox && widows < style()->widows()) {
            if (lineBox->hasTextChildren())
                widows++;
            lineBox = lineBox->nextLineBox();
        }
        // Widows rule broken and more orphans left to use
        if (widows < style()->widows() && orphans > 0) {
            // qDebug() << "Widows: " << widows;
            // Check if we have enough orphans after respecting widows count
            int newOrphans = orphans - (style()->widows() - widows);
            if (newOrphans < style()->orphans()) {
                if (parent()->canClear(this,PageBreakHarder)) {
                    // Relayout to remove incorrect page-break
                    setNeedsPageClear(true);
                    setContainsPageBreak(false);
                    layoutInlineChildren(relayoutChildren, -1);
                    return;
                }
            } else {
                // Set hint and try again
                layoutInlineChildren(relayoutChildren, newOrphans+1);
                return;
            }
        }
    }
    no_break:

    // in case we have a float on the last line, it might not be positioned up to now.
    // This has to be done before adding in the bottom border/padding, or the float will
    // include the padding incorrectly. -dwh
    positionNewFloats();

    // Now add in the bottom border/padding.
    m_height += toAdd;

    // Always make sure this is at least our height.
    m_overflowHeight = qMax(m_height, m_overflowHeight);

    // See if any lines spill out of the block.  If so, we need to update our overflow width.
    checkLinesForOverflow();

    // See if we have any lines that spill out of our block.  If we do, then we will
    // possibly need to truncate text.
    if (hasTextOverflow)
        checkLinesForTextOverflow();

#if BIDI_DEBUG > 1
    qDebug() << " ------- bidi end " << this << " -------";
#endif
    //qDebug() << "RenderBlock::layoutInlineChildren time used " << qt.elapsed();
    //qDebug() << "height = " << m_height;
}

RootInlineBox* RenderBlock::determineStartPosition(bool fullLayout, BidiIterator& start, BidiState& bidi)
{
    RootInlineBox* curr = 0;
    RootInlineBox* last = 0;
    RenderObject* startObj = 0;
    int pos = 0;
    
    if (fullLayout) {
        // Nuke all our lines.
        // ### should be done already at this point... assert( !firstRootBox() )
        if (firstRootBox()) {
            RenderArena* arena = renderArena();
            curr = firstRootBox(); 
            while (curr) {
                RootInlineBox* next = curr->nextRootBox();
                curr->deleteLine(arena);
                curr = next;
            }
            assert(!firstLineBox() && !lastLineBox());
        }
    } else {
        int cnt = 0;
        for (curr = firstRootBox(); curr && !curr->isDirty(); curr = curr->nextRootBox()) cnt++;
        if (curr) {
            // qDebug() << "found dirty line at " << cnt;
            // We have a dirty line.
            if (RootInlineBox* prevRootBox = curr->prevRootBox()) {
                // We have a previous line.
                if (!prevRootBox->endsWithBreak() || (prevRootBox->lineBreakObj()->isText() && prevRootBox->lineBreakPos() >= static_cast<RenderText*>(prevRootBox->lineBreakObj())->stringLength()))
                    // The previous line didn't break cleanly or broke at a newline
                    // that has been deleted, so treat it as dirty too.
                    curr = prevRootBox;
            }
        } else {
            // qDebug() << "No dirty line found";
            // No dirty lines were found.
            // If the last line didn't break cleanly, treat it as dirty.
            if (lastRootBox() && !lastRootBox()->endsWithBreak())
                curr = lastRootBox();
        }
        
        // If we have no dirty lines, then last is just the last root box.
        last = curr ? curr->prevRootBox() : lastRootBox();
    }
    
    m_firstLine = !last;
    previousLineBrokeAtBR = !last || last->endsWithBreak();
    if (last) {
        m_height = last->blockHeight();
        startObj = last->lineBreakObj();
        pos = last->lineBreakPos();
        bidi.status = last->lineBreakBidiStatus();
    } else {
        startObj = first(this, &bidi, false);
    }
        
    start = BidiIterator(this, startObj, pos);
    
    return curr;
}

RootInlineBox* RenderBlock::determineEndPosition(RootInlineBox* startLine, BidiIterator& cleanLineStart, BidiStatus& cleanLineBidiStatus, BidiContext* cleanLineBidiContext, int& yPos)
{
    RootInlineBox* last = 0;
    if (!startLine)
        last = 0;
    else {
        for (RootInlineBox* curr = startLine->nextRootBox(); curr; curr = curr->nextRootBox()) {
            if (curr->isDirty())
                last = 0;
            else if (!last)
                last = curr;
        }
    }
    
    if (!last)
        return 0;
    
    RootInlineBox* prev = last->prevRootBox();
    cleanLineStart = BidiIterator(this, prev->lineBreakObj(), prev->lineBreakPos());
    cleanLineBidiStatus = prev->lineBreakBidiStatus();
    cleanLineBidiContext = prev->lineBreakBidiContext();
    yPos = prev->blockHeight();
    
    return last;
}

bool RenderBlock::matchedEndLine(const BidiIterator& start, const BidiStatus& status, BidiContext* context, 
                                 const BidiIterator& endLineStart, const BidiStatus& endLineStatus, BidiContext* endLineContext,
                                 RootInlineBox*& endLine, int& endYPos)
{
    if (start == endLineStart)
        return status == endLineStatus && endLineContext && (*context == *endLineContext);
    else {
        // The first clean line doesn't match, but we can check a handful of following lines to try
        // to match back up.
        static int numLines = 8; // The # of lines we're willing to match against.
        RootInlineBox* line = endLine;
        for (int i = 0; i < numLines && line; i++, line = line->nextRootBox()) {
            if (line->lineBreakObj() == start.obj && line->lineBreakPos() == start.pos) {
                // We have a match.
                if ((line->lineBreakBidiStatus() != status) || (line->lineBreakBidiContext() != context))
                    return false; // ...but the bidi state doesn't match.
                RootInlineBox* result = line->nextRootBox();
                                
                // Set our yPos to be the block height of endLine.
                if (result)
                    endYPos = line->blockHeight();
                
                // Now delete the lines that we failed to sync.
                RootInlineBox* boxToDelete = endLine;
                RenderArena* arena = renderArena();
                while (boxToDelete && boxToDelete != result) {
                    RootInlineBox* next = boxToDelete->nextRootBox();
                    boxToDelete->deleteLine(arena);
                    boxToDelete = next;
                }

                endLine = result;
                return result;
            }
        }
    }
    return false;
}

static void setStaticPosition( RenderBlock* p, RenderObject *o, bool *needToSetStaticX = 0, bool *needToSetStaticY = 0 )
{
      // If our original display wasn't an inline type, then we can
      // determine our static x position now.
      bool nssx, nssy;
      bool isInlineType = o->style()->isOriginalDisplayInlineType();
      nssx = o->hasStaticX();
      if (nssx && o->isBox()) {
          static_cast<RenderBox*>(o)->setStaticX(o->parent()->style()->direction() == LTR ?
                                  p->borderLeft()+p->paddingLeft() :
                                  p->borderRight()+p->paddingRight());
          nssx = isInlineType;
      }

      // If our original display was an INLINE type, then we can
      // determine our static y position now.
      nssy = o->hasStaticY();
      if (nssy && o->isBox()) {
          static_cast<RenderBox*>(o)->setStaticY(p->height());
          nssy = !isInlineType;
      }
      if (needToSetStaticX) *needToSetStaticX = nssx;
      if (needToSetStaticY) *needToSetStaticY = nssy;
}

static inline bool requiresLineBox(BidiIterator& it)
{
    if (it.obj->isFloatingOrPositioned())
        return false;
    if (it.obj->isInlineFlow())
        return (getBorderPaddingMargin(it.obj, it.endOfInline) != 0);
    if (it.obj->isText() && !static_cast<RenderText*>(it.obj)->length())
        return false;
    if (it.obj->style()->preserveWS() || it.obj->isBR())
        return true;

    switch (it.current().unicode()) {
        case 0x0009: // ASCII tab
        case 0x000A: // ASCII line feed
        case 0x000C: // ASCII form feed
        case 0x0020: // ASCII space
        case 0x200B: // Zero-width space
            return false;
    }
    return true;
}

bool RenderBlock::inlineChildNeedsLineBox(RenderObject* inlineObj) // WC: generatesLineBoxesForInlineChild
{
    assert(inlineObj->parent() == this);

    BidiIterator it(this, inlineObj, 0);
    while (!it.atEnd() && !requiresLineBox(it))
        it.increment(0, false /*skipInlines*/);

    return !it.atEnd();
}
 
void RenderBlock::fitBelowFloats(int widthToFit, int& availableWidth)
{
    assert(widthToFit > availableWidth);

    int floatBottom;
    int lastFloatBottom = m_height;
    int newLineWidth = availableWidth;
    while (true) {
        floatBottom = nearestFloatBottom(lastFloatBottom);
        if (!floatBottom)
            break;

        newLineWidth = lineWidth(floatBottom);
        lastFloatBottom = floatBottom;
        if (newLineWidth >= widthToFit)
            break;
    }
    if (newLineWidth > availableWidth) {
        m_height = lastFloatBottom;
        availableWidth = newLineWidth;
#ifdef DEBUG_LINEBREAKS
    qDebug() << " new position at " << m_height << " newWidth " << availableWidth;
#endif
    }
}

BidiIterator RenderBlock::findNextLineBreak(BidiIterator &start, BidiState &bidi)
{
    int width = lineWidth(m_height);
    int w = 0; // the width from the start of the line up to the currently chosen breaking opportunity
    int tmpW = 0; // the accumulated width since the last chosen breaking opportunity
#ifdef DEBUG_LINEBREAKS
    qDebug() << "findNextLineBreak: line at " << m_height << " line width " << width;
    qDebug() << "sol: " << start.obj << " " << start.pos;
#endif

    BidiIterator posStart = start;
    bool hadPosStart = false;

    // Skip initial whitespace
    while (!start.atEnd() && !requiresLineBox(start)) {
        if( start.obj->isFloating() || start.obj->isPosWithStaticDim()) {
            RenderObject *o = start.obj;
            // add to special objects...
            if (o->isFloating()) {
                insertFloatingObject(o);
                positionNewFloats();
                width = lineWidth(m_height);
            }
            else if (o->isPositioned()) {
                // add midpoints to have positioned objects at the correct static location
                // while still skipping initial whitespace.
                if (!hadPosStart) {
                    hadPosStart = true;
                    posStart = start;
                    // include this object then stop
                    addMidpoint(BidiIterator(0, o, 0));
                } else {
                    // start/stop
                    addMidpoint(BidiIterator(0, o, 0));
                    addMidpoint(BidiIterator(0, o, 0));
                }
                setStaticPosition(this, o);
            }
        }
        start.increment(&bidi, false /*skipInlines*/);
    }

    if (hadPosStart && !start.atEnd())
        addMidpoint(start);

    if ( start.atEnd() ){
        if (hadPosStart) {
            start = posStart;
            posStart.increment();
            return posStart;
        }
        return start;
    }

    // This variable says we have encountered an object after which initial whitespace should be ignored (e.g. InlineFlows at the beginning of a line).
    // Either we have nothing to do, if there is no whitespace after the object... or we have to enter the ignoringSpaces state.
    // This dilemma will be resolved when we have a peek at the next object.
    bool checkShouldIgnoreInitialWhitespace = false;

    // This variable is used only if whitespace isn't set to PRE, and it tells us whether
    // or not we are currently ignoring whitespace.
    bool ignoringSpaces = false;
    BidiIterator ignoreStart;

    // This variable tracks whether the very last character we saw was a space.  We use
    // this to detect when we encounter a second space so we know we have to terminate
    // a run.
    bool currentCharacterIsSpace = false;

    // This variable tracks whether there is space still available on the line for floating objects.
    // Once a floating object does not fit, we wait till next linebreak before positioning more floats.
    bool floatsFitOnLine = true;

    RenderObject* trailingSpaceObject = 0;

    BidiIterator lBreak = start;
    InlineMinMaxIterator it(start.par, start.obj, start.endOfInline, false /*skipPositioned*/);
    InlineMinMaxIterator lastIt = it;
    int pos = start.pos;

    bool prevLineBrokeCleanly = previousLineBrokeAtBR;
    previousLineBrokeAtBR = false;

    RenderObject* o = it.current;
    while( o ) {
#ifdef DEBUG_LINEBREAKS
        qDebug() << "new object "<< o <<" width = " << w <<" tmpw = " << tmpW;
#endif
        if(o->isBR()) {
            if( w + tmpW <= width ) {
                lBreak.obj = o;
                lBreak.pos = 0;
                lBreak.endOfInline = it.endOfInline;

                // A <br> always breaks a line, so don't let the line be collapsed
                // away. Also, the space at the end of a line with a <br> does not
                // get collapsed away.  It only does this if the previous line broke
                // cleanly.  Otherwise the <br> has no effect on whether the line is
                // empty or not.
                if (prevLineBrokeCleanly)
                    isLineEmpty = false;
                trailingSpaceObject = 0;
                previousLineBrokeAtBR = true;

                if (!isLineEmpty) {
                    // only check the clear status for non-empty lines.
                    EClear clear = o->style()->clear();
                    if(clear != CNONE)
                        m_clearStatus = (EClear) (m_clearStatus | clear);
                }
            }
            goto end;
        }
        if( o->isFloatingOrPositioned() ) {
            // add to special objects...
            if(o->isFloating()) {
                insertFloatingObject(o);
                // check if it fits in the current line.
                // If it does, position it now, otherwise, position
                // it after moving to next line (in newLine() func)
                if (floatsFitOnLine && o->width()+o->marginLeft()+o->marginRight()+w+tmpW <= width) {
                    positionNewFloats();
                    width = lineWidth(m_height);
                } else
                    floatsFitOnLine = false;
            }
            else if (o->isPositioned() && o->isPosWithStaticDim()) {
                bool needToSetStaticX;
                bool needToSetStaticY;
                setStaticPosition(this, o, &needToSetStaticX, &needToSetStaticY);

                // If we're ignoring spaces, we have to stop and include this object and
                // then start ignoring spaces again.
                if (needToSetStaticX || needToSetStaticY) {
                    trailingSpaceObject = 0;
                    ignoreStart.obj = o;
                    ignoreStart.pos = 0;
                    if (ignoringSpaces) {
                        addMidpoint(ignoreStart); // Stop ignoring spaces.
                        addMidpoint(ignoreStart); // Start ignoring again.
                    }
                }
            }
        } else if (o->isInlineFlow()) {
            tmpW += getBorderPaddingMargin(o, it.endOfInline);
            if (isLineEmpty) isLineEmpty = !tmpW;
            if (o->isWordBreak()) { // #### shouldn't be an InlineFlow!
                w += tmpW;
                tmpW = 0;
                lBreak.obj = o;
                lBreak.pos = 0;
                lBreak.endOfInline = it.endOfInline;
            } else if (!it.endOfInline) {
                 // this is the beginning of the line (other non-initial inline flows are handled directly when
                 // incrementing the iterator below). We want to skip initial whitespace as much as possible.
                 checkShouldIgnoreInitialWhitespace = true;
            }
        } else if ( o->isReplaced() || o->isGlyph() ) {
            EWhiteSpace currWS = o->style()->whiteSpace();
            EWhiteSpace lastWS = lastIt.current->style()->whiteSpace();

            // WinIE marquees have different whitespace characteristics by default when viewed from
            // the outside vs. the inside.  Text inside is NOWRAP, and so we altered the marquee's
            // style to reflect this, but we now have to get back to the original whitespace value
            // for the marquee when checking for line breaking.
            if (o->isHTMLMarquee() && o->layer() && o->layer()->marquee())
                currWS = o->layer()->marquee()->whiteSpace();
            if (lastIt.current->isHTMLMarquee() && lastIt.current->layer() && lastIt.current->layer()->marquee())
                lastWS = lastIt.current->layer()->marquee()->whiteSpace();

            // Break on replaced elements if either has normal white-space.
            if (currWS == NORMAL || lastWS == NORMAL) {
                w += tmpW;
                tmpW = 0;
                lBreak.obj = o;
                lBreak.pos = 0;
                lBreak.endOfInline = false;
            }

            tmpW += o->width()+o->marginLeft()+o->marginRight();
            if (ignoringSpaces) {
                BidiIterator startMid( 0, o, 0 );
                addMidpoint(startMid);
            }
            isLineEmpty = false;
            ignoringSpaces = false;
            currentCharacterIsSpace = false;
            trailingSpaceObject = 0;

            if (o->isListMarker()) {
                checkShouldIgnoreInitialWhitespace = true;
            }
        } else if ( o->isText() ) {
            RenderText *t = static_cast<RenderText *>(o);
            int strlen = t->stringLength();
            int len = strlen - pos;
            QChar *str = t->text();

            const Font *f = t->htmlFont( m_firstLine );
            // proportional font, needs a bit more work.
            int lastSpace = pos;
            bool autoWrap = o->style()->autoWrap();
            bool preserveWS = o->style()->preserveWS();
            bool preserveLF = o->style()->preserveLF();
#ifdef APPLE_CHANGES
            int wordSpacing = o->style()->wordSpacing();
#endif
            bool nextIsSoftBreakable = false;
            bool checkBreakWord = autoWrap && (o->style()->wordWrap() == WWBREAKWORD);

            while(len) {
                bool previousCharacterIsSpace = currentCharacterIsSpace;
                bool isSoftBreakable = nextIsSoftBreakable;
                nextIsSoftBreakable = false;
                const QChar c = str[pos];
                currentCharacterIsSpace = c.unicode() == ' ';
                checkBreakWord &= !w; // only break words when no other breaking opportunity exists earlier
                                      // on the line (even within the text object we are currently processing)

                if (preserveWS || !currentCharacterIsSpace)
                    isLineEmpty = false;

                // Check for soft hyphens.  Go ahead and ignore them.
                if (c.unicode() == SOFT_HYPHEN && pos > 0) {
                    nextIsSoftBreakable = true;
                    if (!ignoringSpaces) {
                        // Ignore soft hyphens
                        BidiIterator endMid(0, o, pos-1);
                        addMidpoint(endMid);

                        // Add the width up to but not including the hyphen.
                        tmpW += t->width(lastSpace, pos - lastSpace, f);

                        // For wrapping text only, include the hyphen.  We need to ensure it will fit
                        // on the line if it shows when we break.
                        if (o->style()->autoWrap()) {
                            const QChar softHyphen(0x00ad);
                            tmpW += f->charWidth(&softHyphen, 1, 0, true);
                        }

                        BidiIterator startMid(0, o, pos+1);
                        addMidpoint(startMid);
                    }

                    pos++;
                    len--;
                    lastSpace = pos; // Cheesy hack to prevent adding in widths of the run twice.
                    continue;
                }
#ifdef APPLE_CHANGES    // KDE applies wordspacing differently
                bool applyWordSpacing = false;
#endif
                if (ignoringSpaces) {
                    // We need to stop ignoring spaces, if we encounter a non-space or
                    // a run that doesn't collapse spaces.
                    if (!currentCharacterIsSpace || preserveWS) {
                        // Stop ignoring spaces and begin at this
                        // new point.
                        ignoringSpaces = false;
                        lastSpace = pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
                        BidiIterator startMid ( 0, o, pos );
                        addMidpoint(startMid);
                    }
                    else {
                        // Just keep ignoring these spaces.
                        pos++;
                        len--;
                        continue;
                    }
                }
                bool isbreakablePosition = (preserveLF && c.unicode() == '\n') || (autoWrap && (isBreakable( str, pos, strlen ) || isSoftBreakable));
                if ( isbreakablePosition || checkBreakWord) {
                    tmpW += t->width(lastSpace, pos - lastSpace, f);
#ifdef APPLE_CHANGES
                    applyWordSpacing = (wordSpacing && currentCharacterIsSpace && !previousCharacterIsSpace &&
                        !t->containsOnlyWhitespace(pos+1, strlen-(pos+1)));
#endif
#ifdef DEBUG_LINEBREAKS
                    qDebug() << "found space at " << pos << " in string '" << QString( str, strlen ).toLatin1().constData() << "' adding " << tmpW << " new width = " << w;
#endif
                    if (!w && autoWrap && tmpW > width)
                        fitBelowFloats(tmpW, width);

                    if (autoWrap) {
                        if (w+tmpW > width) {
                            if (checkBreakWord && pos) {
                                lBreak.obj = o;
                                lBreak.pos = pos-1;
                                lBreak.endOfInline = false;
                            }
                            goto end;
                        } else if ( (pos > 1 && str[pos-1].unicode() == SOFT_HYPHEN) )
                            // Subtract the width of the soft hyphen out since we fit on a line.
                            tmpW -= t->width(pos-1, 1, f);
                    }

                    if( preserveLF && (str+pos)->unicode() == '\n' ) {
                        lBreak.obj = o;
                        lBreak.pos = pos;
                        lBreak.endOfInline = false;

#ifdef DEBUG_LINEBREAKS
                        qDebug() << "forced break sol: " << start.obj << " " << start.pos << "   end: " << lBreak.obj << " " << lBreak.pos << "   width=" << w;
#endif
                        return lBreak;
                    }

                    if ( autoWrap && isbreakablePosition ) {
                        w += tmpW;
                        tmpW = 0;
                        lBreak.obj = o;
                        lBreak.pos = pos;
                        lBreak.endOfInline = false;
                    }

                    lastSpace = pos;
#ifdef APPLE_CHANGES
                    if (applyWordSpacing)
                        w += wordSpacing;
#endif
                }

                if (!ignoringSpaces && !preserveWS) {
                    // If we encounter a second space, we need to go ahead and break up this run
                    // and enter a mode where we start collapsing spaces.
                    if (currentCharacterIsSpace && previousCharacterIsSpace) {
                        ignoringSpaces = true;

                        // We just entered a mode where we are ignoring
                        // spaces. Create a midpoint to terminate the run
                        // before the second space.
                        addMidpoint(ignoreStart);
                        lastSpace = pos;
                    }
                }

                if (currentCharacterIsSpace && !previousCharacterIsSpace) {
                    ignoreStart.obj = o;
                    ignoreStart.pos = pos;
                }

                if (!preserveWS && currentCharacterIsSpace && !ignoringSpaces)
                    trailingSpaceObject = o;
                else if (preserveWS || !currentCharacterIsSpace)
                    trailingSpaceObject = 0;

                pos++;
                len--;
            }

            if (!ignoringSpaces) {
                // We didn't find any space that would be beyond the line |width|.
                // Lets add to |tmpW| the remaining width since the last space we found.
                // Before we test this new |tmpW| however, we will have to look ahead to check
                // if the next object/position can serve as a line breaking opportunity.
                tmpW += t->width(lastSpace, pos - lastSpace, f);
                if (checkBreakWord && !w && pos && tmpW > width) {
                    // Avoid doing the costly lookahead for break-word,
                    // since we know we are allowed to break.
                    lBreak.obj = o;
                    lBreak.pos = pos-1;
                    lBreak.endOfInline = false;
                    goto end;
                }
            }
        }
        else {
            KHTMLAssert( false );
        }

        InlineMinMaxIterator savedIt = lastIt;
        lastIt = it;
        o = it.next();

        // Advance the iterator to the next non-inline-flow
        while (o && o->isInlineFlow() && !o->isWordBreak()) {
            tmpW += getBorderPaddingMargin(o, it.endOfInline);
            if (isLineEmpty) isLineEmpty = !tmpW;
            o = it.next();
        }

        // All code below, until the end of the loop, is looking ahead the |it| object we just
        // advanced to, comparing it to the previous object |lastIt|.

        if (checkShouldIgnoreInitialWhitespace) {
            // Check if we should switch to ignoringSpaces state
            if (!style()->preserveWS() && it.current && it.current->isText()) {
                const RenderText* rt = static_cast<RenderText*>(it.current);
                if (rt->stringLength() > 0 && (rt->text()[0].category() == QChar::Separator_Space || rt->text()[0] == '\n')) {
                    currentCharacterIsSpace = true;
                    ignoringSpaces = true;
                    BidiIterator endMid( 0, lastIt.current, 0 );
                    addMidpoint(endMid);
                }
            }
            checkShouldIgnoreInitialWhitespace = false;
        }

        bool autoWrap = lastIt.current->style()->autoWrap();
        bool canBreak = !lBreak.obj || !lBreak.obj->isInlineFlow() || !lBreak.obj->firstChild();

        bool checkForBreak = autoWrap;
        if (canBreak) {
            if (!autoWrap && w && w + tmpW > width && lBreak.obj && !lastIt.current->style()->preserveLF())
                // ### needs explanation
                checkForBreak = true;
            else if (it.current && lastIt.current->isText() && it.current->isText() && !it.current->isBR()) {
                // We are looking ahead the next text object to see if it continues a word started previously,
                // or is a line-breaking opportunity.
                if (autoWrap || it.current->style()->autoWrap()) {
                    if (currentCharacterIsSpace)
                        // "<i>s </i>top"
                        //      _    ^
                        checkForBreak = true;
                    else {
                        // either "<i>c</i>ontinue" or "<i>s</i> top"
                        //            _    ^               _    ^
                        checkForBreak = false;
                        RenderText* nextText = static_cast<RenderText*>(it.current);
                        if (nextText->stringLength() != 0) {
                            QChar c = nextText->text()[0];
                            // If the next item is a space, then we may try to break.
                            // Otherwise the next text run continues our word (and so it needs to
                            // keep adding to |tmpW|).
                            if (c == ' ' || c == '\t' || (c == '\n' && !it.current->style()->preserveLF())) {
                                checkForBreak = true;
                            }
                        }

                        bool willFitOnLine = (w + tmpW <= width);
                        if (!willFitOnLine && !w) {
                            fitBelowFloats(tmpW, width);
                            willFitOnLine = tmpW <= width;
                        }        
                        bool canPlaceOnLine = willFitOnLine || !autoWrap;
                        if (canPlaceOnLine && checkForBreak) {
                            w += tmpW;
                            tmpW = 0;
                            lBreak.obj = it.current;
                            lBreak.pos = 0;
                            lBreak.endOfInline = it.endOfInline;
                        }
                    }
                }
            }

            if (checkForBreak && (w + tmpW > width)) {
                // if we have floats, try to get below them.
                if (currentCharacterIsSpace && !ignoringSpaces && !lastIt.current->style()->preserveWS())
                    trailingSpaceObject = 0;

                if (w)
                    goto end;
                fitBelowFloats(tmpW, width);

                // |width| may have been adjusted because we got shoved down past a float (thus
                // giving us more room), so we need to retest, and only jump to
                // the end label if we still don't fit on the line. -dwh
                if (w + tmpW > width) {
                    it = lastIt;
                    lastIt = savedIt;
                    o = it.current;
                    goto end;
                }
            }
        }
        if (!lastIt.current->isFloatingOrPositioned() && lastIt.current->isReplaced() && lastIt.current->style()->autoWrap()) {
            // Go ahead and add in tmpW.
            w += tmpW;
            tmpW = 0;
            lBreak.obj = o;
            lBreak.pos = 0;
            lBreak.endOfInline = it.endOfInline;
        }

        // Clear out our character space bool, since inline <pre>s don't collapse whitespace
        // with adjacent inline normal/nowrap spans.
        if (lastIt.current->style()->preserveWS())
            currentCharacterIsSpace = false;

        pos = 0;
    }

#ifdef DEBUG_LINEBREAKS
    qDebug() << "end of par, width = " << width << " linewidth = " << w + tmpW;
#endif
    if( w + tmpW <= width || (lastIt.current && !lastIt.current->style()->autoWrap())) {
        lBreak.obj = 0;
        lBreak.pos = 0;
        lBreak.endOfInline = false;
    }

 end:
    if ( lBreak == start && !lBreak.obj->isBR() ) {
        // Having an |lBreak| identical to our |start| at this point means the first suitable
        // break point |it.current| that we found was past |width|, so we jumped to the |end| label
        // before we could set this (overflowing) breaking opportunity. Let's set it now. 
        if ( style()->whiteSpace() == PRE ) {
            // FIXME: Don't really understand this case.
            if(pos != 0) {
                lBreak.obj = o;
                lBreak.pos = pos - 1;
                lBreak.endOfInline = it.endOfInline;
            } else {
                lBreak.obj = lastIt.current;
                lBreak.pos = lastIt.current->isText() ? lastIt.current->length() : 0;
                lBreak.endOfInline = lastIt.endOfInline;
            }
        } else if( lBreak.obj ) {
                lBreak.obj = o;
                lBreak.pos = (o && o->isText() ? pos : 0);
                lBreak.endOfInline = it.endOfInline;
        }
    }

    if (hadPosStart)
        start = posStart;

    if( lBreak == start) {
        // make sure we consume at least one char/object.
        lBreak.increment();
    }

#ifdef DEBUG_LINEBREAKS
    qDebug() << "regular break sol: " << start.obj << " " << start.pos << "   end: " << lBreak.obj << " " << lBreak.pos << "   width=" << w;
#endif

    // Sanity check our midpoints.
    checkMidpoints(lBreak);

    if (trailingSpaceObject) {
        // This object is either going to be part of the last midpoint, or it is going
        // to be the actual endpoint.  In both cases we just decrease our pos by 1 level to
        // exclude the space, allowing it to - in effect - collapse into the newline.
        if (sNumMidpoints%2==1) {
            BidiIterator* midpoints = smidpoints->data();
            midpoints[sNumMidpoints-1].pos--;
        }
        //else if (lBreak.pos > 0)
        //    lBreak.pos--;
        else if (lBreak.obj == 0 && trailingSpaceObject->isText()) {
            // Add a new end midpoint that stops right at the very end.
            RenderText* text = static_cast<RenderText *>(trailingSpaceObject);
            unsigned pos = text->length() >=2 ? text->length() - 2 : UINT_MAX;
            BidiIterator endMid ( 0, trailingSpaceObject, pos );
            addMidpoint(endMid);
        }
    }

    // We might have made lBreak an iterator that points past the end
    // of the object. Do this adjustment to make it point to the start
    // of the next object instead to avoid confusing the rest of the
    // code.
    if (lBreak.pos > 0) {
        lBreak.pos--;
        lBreak.increment();
    }

    if (lBreak.obj && lBreak.pos >= 2 && lBreak.obj->isText()) {
        // For soft hyphens on line breaks, we have to chop out the midpoints that made us
        // ignore the hyphen so that it will render at the end of the line.
        QChar c = static_cast<RenderText*>(lBreak.obj)->text()[lBreak.pos-1];
        if (c.unicode() == SOFT_HYPHEN)
            chopMidpointsAt(lBreak.obj, lBreak.pos-2);
    }

    return lBreak;
}

void RenderBlock::checkLinesForOverflow()
{
    for (RootInlineBox* curr = static_cast<khtml::RootInlineBox*>(firstLineBox()); curr; curr = static_cast<khtml::RootInlineBox*>(curr->nextLineBox())) {
//         m_overflowLeft = min(curr->leftOverflow(), m_overflowLeft);
        m_overflowTop = qMin(curr->topOverflow(), m_overflowTop);
//         m_overflowWidth = max(curr->rightOverflow(), m_overflowWidth);
        m_overflowHeight = qMax(curr->bottomOverflow(), m_overflowHeight);
    }
}

void RenderBlock::deleteEllipsisLineBoxes()
{
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox())
        curr->clearTruncation();
}

void RenderBlock::checkLinesForTextOverflow()
{
    // Determine the width of the ellipsis using the current font.
    QChar ellipsis = 0x2026; // FIXME: CSS3 says this is configurable, also need to use 0x002E (FULL STOP) if 0x2026 not renderable
    static QString ellipsisStr(ellipsis);
    const Font& firstLineFont = style(true)->htmlFont();
    const Font& font = style()->htmlFont();
    int firstLineEllipsisWidth = firstLineFont.charWidth(&ellipsis, 1, 0, true /*fast algo*/);
    int ellipsisWidth = (font == firstLineFont) ? firstLineEllipsisWidth : font.charWidth(&ellipsis, 1, 0, true /*fast algo*/);

    // For LTR text truncation, we want to get the right edge of our padding box, and then we want to see
    // if the right edge of a line box exceeds that.  For RTL, we use the left edge of the padding box and
    // check the left edge of the line box to see if it is less
    // Include the scrollbar for overflow blocks, which means we want to use "contentWidth()"
    bool ltr = style()->direction() == LTR;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        int blockEdge = ltr ? rightOffset(curr->yPos()) : leftOffset(curr->yPos());
        int lineBoxEdge = ltr ? curr->xPos() + curr->width() : curr->xPos();
        if ((ltr && lineBoxEdge > blockEdge) || (!ltr && lineBoxEdge < blockEdge)) {
            // This line spills out of our box in the appropriate direction.  Now we need to see if the line
            // can be truncated.  In order for truncation to be possible, the line must have sufficient space to
            // accommodate our truncation string, and no replaced elements (images, tables) can overlap the ellipsis
            // space.
            int width = curr == firstRootBox() ? firstLineEllipsisWidth : ellipsisWidth;
            if (curr->canAccommodateEllipsis(ltr, blockEdge, lineBoxEdge, width))
                curr->placeEllipsis(ellipsisStr, ltr, blockEdge, width);
        }
    }
}

// For --enable-final
#undef BIDI_DEBUG
#undef DEBUG_LINEBREAKS
#undef DEBUG_LAYOUT

}
// kate: space-indent on; indent-width 4; tab-width 8;
