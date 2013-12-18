/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll <knoll@kde.org>
 * Copyright (C) 2000-2003 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2003, 2006 Apple Computer, Inc.
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
#ifndef RENDERTEXT_H
#define RENDERTEXT_H

#include "dom/dom_string.h"
#include "xml/dom_stringimpl.h"
#include "xml/dom_textimpl.h"
#include "rendering/render_object.h"
#include "rendering/render_line.h"

#include <QtCore/QMutableVectorIterator>
#include <assert.h>

class QPainter;
class QFontMetrics;

// Define a constant for soft hyphen's unicode value.
#define SOFT_HYPHEN 173

const int cNoTruncation = -1;
const int cFullTruncation = -2;

namespace khtml
{
    class RenderText;
    class RenderStyle;

class InlineTextBox : public InlineRunBox
{
public:
    InlineTextBox(RenderObject *obj)
    	:InlineRunBox(obj),
    	// ### necessary as some codepaths (<br>) do *not* initialize these (LS)
    	m_start(0), m_len(0), m_truncation(cNoTruncation), m_reversed(false), m_toAdd(0)
    {
    }
 
    uint start() const { return m_start; }
    uint end() const { return m_len ? m_start+m_len-1 : m_start; }
    uint len() const { return m_len; }

    void detach(RenderArena* renderArena, bool noRemove=false);

    InlineTextBox* nextTextBox() const { return static_cast<InlineTextBox*>(nextLineBox()); }
    InlineTextBox* prevTextBox() const { return static_cast<InlineTextBox*>(prevLineBox()); }

    virtual void clearTruncation() { m_truncation = cNoTruncation; }
    virtual int placeEllipsisBox(bool ltr, int blockEdge, int ellipsisWidth, bool& foundBox);

    // Overloaded new operator.  Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void* operator new(size_t sz, RenderArena* renderArena) throw();

    // Overridden to prevent the normal delete from being called.
    void operator delete(void* ptr, size_t sz);

private:
    // The normal operator new is disallowed.
    void* operator new(size_t sz) throw();

public:
    void setSpaceAdd(int add) { m_width -= m_toAdd; m_toAdd = add; m_width += m_toAdd; }
    int spaceAdd() { return m_toAdd; }

    virtual bool isInlineTextBox() const { return true; }

    void paint(RenderObject::PaintInfo& i, int tx, int ty);
    void paintDecoration(QPainter *pt, const Font *f, int _tx, int _ty, int decoration);
    void paintShadow(QPainter *pt, const Font* f, int _tx, int _ty, const ShadowData *shadow );
    void paintSelection(const Font *f, RenderText *text, QPainter *p, RenderStyle* style, int tx, int ty, int startPos, int endPos, int deco);
    
    void selectionStartEnd(int& sPos, int& ePos);
    RenderObject::SelectionState selectionState();

    // Return before, after (offset set to max), or inside the text, at @p offset
    FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty, int & offset);

    bool checkVerticalPoint(int _y, int _ty, int _h, int height)
    { if((_ty + m_y > _y + _h) || (_ty + m_y + m_baseline + height < _y)) return false; return true; }

    /**
     * determines the offset into the DOMString of the character the given
     * coordinate points to.
     * The returned offset is never out of range.
     * @param _x given coordinate (relative to containing block)
     * @param ax returns exact coordinate the offset corresponds to
     *		(relative to containing block)
     * @return the offset (relative to the RenderText object, not to this run)
     */
    int offsetForPoint(int _x, int &ax) const;

    /**
     * calculates the with of the specified chunk in this text box.
     * @param pos zero-based position within the text box up to which
     *	the width is to be determined
     * @return the width in pixels
     */
    int widthFromStart(int pos) const;

    virtual long caretMinOffset() const;
    virtual long caretMaxOffset() const;
    virtual unsigned long caretMaxRenderedOffset() const;

    /** returns the associated render text
     */
    const RenderText *renderText() const;
    RenderText *renderText();
    
    virtual void extractLine();
    virtual void deleteLine(RenderArena* arena);
    virtual void attachLine();
    

    int m_start;
    unsigned short m_len;

    int m_truncation; // Where to truncate when text overflow is applied.
                      // We use special constants to denote no truncation (the whole run paints)
                      // and full truncation (nothing paints at all).

    bool m_reversed : 1;
    unsigned m_toAdd : 14; // for justified text
private:
    friend class RenderText;
};

class RenderText : public RenderObject
{
    friend class InlineTextBox;

public:
    RenderText(DOM::NodeImpl* node, DOM::DOMStringImpl *_str);
    virtual ~RenderText();

    virtual bool isTextFragment() const;
    virtual DOM::DOMStringImpl* originalString() const;

    virtual const char *renderName() const { return "RenderText"; }

    virtual void setStyle(RenderStyle *style);

    virtual void detach( ); 

    virtual void deleteInlineBoxes(RenderArena* arena=0);
    virtual void dirtyInlineBoxes(bool fullLayout, bool);
    virtual void removeInlineBox(InlineBox* _box);

    DOM::DOMString data() const { return str; }
    DOM::DOMStringImpl *string() const { return str; }

    virtual InlineBox* createInlineBox(bool, bool);

    virtual void layout() {assert(false);}

    virtual bool nodeAtPoint(NodeInfo& info, int x, int y, int tx, int ty, HitTestAction hitTestAction, bool inBox);

    virtual RenderPosition positionForCoordinates(int _x, int _y);

    // Return before, after (offset set to max), or inside the text, at @p offset
    virtual FindSelectionResult checkSelectionPoint( int _x, int _y, int _tx, int _ty,
                                                     DOM::NodeImpl*& node, int & offset,
						     SelPointState & );

    unsigned int length() const { if (str) return str->l; else return 0; }
    QChar *text() const { if (str) return str->s; else return 0; }
    unsigned int stringLength() const { return str->l; } // non virtual implementation of length()
    virtual void position(InlineBox* box, int from, int len, bool reverse);

    virtual unsigned int width(unsigned int from, unsigned int len, const Font *f) const;
    virtual unsigned int width(unsigned int from, unsigned int len, bool firstLine = false) const;
    virtual short width() const;
    virtual int height() const;

    // height of the contents (without paddings, margins and borders)
    virtual short lineHeight( bool firstLine ) const;
    virtual short baselinePosition( bool firstLine ) const;

    // overrides
    virtual void calcMinMaxWidth();
    virtual short minWidth() const { return m_minWidth; }
    virtual int maxWidth() const { return m_maxWidth; }

    void trimmedMinMaxWidth(int& beginMinW, bool& beginWS,
                            int& endMinW, bool& endWS,
                            bool& hasBreakableChar, bool& hasBreak,
                            int& beginMaxW, int& endMaxW,
                            int& minW, int& maxW, bool& stripFrontSpaces);

    bool containsOnlyWhitespace(unsigned int from, unsigned int len) const;

    ushort startMin() const { return m_startMin; }
    ushort endMin() const { return m_endMin; }

    // returns the minimum x position of all runs relative to the parent.
    // defaults to 0.
    int minXPos() const;

    virtual int inlineXPos() const;
    virtual int inlineYPos() const;

    bool hasReturn() const { return m_hasReturn; }

    virtual const QFont &font();
    virtual short verticalPositionHint( bool firstLine ) const;

    bool isFixedWidthFont() const;

    void setText(DOM::DOMStringImpl *text, bool force=false);

    virtual SelectionState selectionState() const {return KDE_CAST_BF_ENUM(SelectionState, m_selectionState);}
    virtual void setSelectionState(SelectionState s) {m_selectionState = s; }
    virtual void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const;
    virtual bool absolutePosition(int &/*xPos*/, int &/*yPos*/, bool f = false) const;
    bool posOfChar(int ch, int &x, int &y) const;
    virtual bool isPointInsideSelection(int x, int y, const DOM::Selection &) const;

    virtual short marginLeft() const { return style()->marginLeft().minWidth(0); }
    virtual short marginRight() const { return style()->marginRight().minWidth(0); }

    virtual void repaint(Priority p=NormalPriority);

    InlineTextBox* firstTextBox() const { return m_firstTextBox; }
    InlineTextBox* lastTextBox() const { return m_lastTextBox; }

    virtual QList< QRectF > getClientRects();

    bool hasBreakableChar() const { return m_hasBreakableChar; }
    bool isSimpleText() const { return m_isSimpleText; }
    const QFontMetrics &metrics(bool firstLine) const;
    const Font *htmlFont(bool firstLine) const;

    DOM::TextImpl *element() const
    { return static_cast<DOM::TextImpl*>(RenderObject::element()); }

    virtual InlineBox *inlineBox(long offset);

    void removeTextBox(InlineTextBox* box);
    void attachTextBox(InlineTextBox* box);
    void extractTextBox(InlineTextBox* box);

#ifdef ENABLE_DUMP
    virtual void dump(QTextStream &stream, const QString &ind) const;
#endif

    virtual long caretMinOffset() const;
    virtual long caretMaxOffset() const;
    virtual unsigned long caretMaxRenderedOffset() const;

    /** Find the text box that includes the character at @p offset
     * and return pos, which is the position of the char in the run.
     * @param offset zero-based offset into DOM string
     * @param pos returns relative position within text box
     * @param checkFirstLetter passing @p true will also regard :first-letter
     *		boxes, if available.
     * @return the text box, or 0 if no match has been found
     */
    const InlineTextBox * findInlineTextBox( int offset, int &pos,
    					bool checkFirstLetter = false ) const;

    // helper methods to convert Position from rendered text into DOM position
    // (takes into account space collapsing)
    unsigned convertToDOMPosition(unsigned position) const;
    unsigned convertToRenderedPosition(unsigned position) const;
protected:
    virtual void setTextInternal(DOM::DOMStringImpl *text);

    // members
    InlineTextBox* m_firstTextBox;
    InlineTextBox* m_lastTextBox;

    DOM::DOMStringImpl *str; //

    short m_lineHeight;
    short m_minWidth;
    int   m_maxWidth;
    short m_beginMinWidth;
    short m_endMinWidth;

    KDE_BF_ENUM(SelectionState) m_selectionState : 3 ;
    bool m_hasReturn : 1;
    bool m_hasBreakableChar : 1;
    bool m_hasBreak : 1;
    bool m_hasBeginWS : 1;
    bool m_hasEndWS : 1;
    bool m_isSimpleText : 1;

    ushort m_startMin : 8;
    ushort m_endMin : 8;
};

inline const RenderText* InlineTextBox::renderText() const
{ return static_cast<RenderText*>( object() ); }

inline RenderText* InlineTextBox::renderText()
{ return static_cast<RenderText*>( object() ); }

// Used to represent a text substring of an element, e.g., for text runs that are split because of
// first letter and that must therefore have different styles (and positions in the render tree).
// We cache offsets so that text transformations can be applied in such a way that we can recover
// the original unaltered string from our corresponding DOM node.
class RenderTextFragment : public RenderText
{
public:
    RenderTextFragment(DOM::NodeImpl* _node, DOM::DOMStringImpl* _str,
                       int startOffset, int endOffset);
    RenderTextFragment(DOM::NodeImpl* _node, DOM::DOMStringImpl* _str);
    ~RenderTextFragment();

    virtual bool isTextFragment() const;
    virtual const char *renderName() const { return "RenderTextFragment"; }

    uint start() const { return m_start; }
    uint end() const { return m_end; }

    DOM::DOMStringImpl* contentString() const { return m_generatedContentStr; }
    virtual DOM::DOMStringImpl* originalString() const;

    RenderObject* firstLetter() const { return m_firstLetter; }
    void setFirstLetter(RenderObject* firstLetter) { m_firstLetter = firstLetter; }

    // overrides
    virtual void detach();
private:
    virtual void setTextInternal(DOM::DOMStringImpl *text);

    uint m_start;
    uint m_end;
    DOM::DOMStringImpl* m_generatedContentStr;
    RenderObject* m_firstLetter;
};
} // end namespace
#endif
