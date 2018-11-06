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

#include <QMutableVectorIterator>
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
        : InlineRunBox(obj),
          // ### necessary as some codepaths (<br>) do *not* initialize these (LS)
          m_start(0), m_len(0), m_truncation(cNoTruncation), m_reversed(false), m_toAdd(0)
    {
    }

    uint start() const
    {
        return m_start;
    }
    uint end() const
    {
        return m_len ? m_start + m_len - 1 : m_start;
    }
    uint len() const
    {
        return m_len;
    }

    void detach(RenderArena *renderArena, bool noRemove = false) override;

    InlineTextBox *nextTextBox() const
    {
        return static_cast<InlineTextBox *>(nextLineBox());
    }
    InlineTextBox *prevTextBox() const
    {
        return static_cast<InlineTextBox *>(prevLineBox());
    }

    void clearTruncation() override
    {
        m_truncation = cNoTruncation;
    }
    int placeEllipsisBox(bool ltr, int blockEdge, int ellipsisWidth, bool &foundBox) override;

    // Overloaded new operator.  Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void *operator new(size_t sz, RenderArena *renderArena) throw();

    // Overridden to prevent the normal delete from being called.
    void operator delete(void *ptr, size_t sz);

private:
    // The normal operator new is disallowed.
    void *operator new(size_t sz) throw();

public:
    void setSpaceAdd(int add)
    {
        m_width -= m_toAdd;
        m_toAdd = add;
        m_width += m_toAdd;
    }
    int spaceAdd()
    {
        return m_toAdd;
    }

    bool isInlineTextBox() const override
    {
        return true;
    }

    void paint(RenderObject::PaintInfo &i, int tx, int ty) override;
    void paintDecoration(QPainter *pt, const Font *f, int _tx, int _ty, int decoration);
    void paintShadow(QPainter *pt, const Font *f, int _tx, int _ty, const ShadowData *shadow);
    void paintSelection(const Font *f, RenderText *text, QPainter *p, RenderStyle *style, int tx, int ty, int startPos, int endPos, int deco);

    void selectionStartEnd(int &sPos, int &ePos);
    RenderObject::SelectionState selectionState();

    // Return before, after (offset set to max), or inside the text, at @p offset
    FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty, int &offset);

    bool checkVerticalPoint(int _y, int _ty, int _h, int height)
    {
        if ((_ty + m_y > _y + _h) || (_ty + m_y + m_baseline + height < _y)) {
            return false;
        } return true;
    }

    /**
     * determines the offset into the DOMString of the character the given
     * coordinate points to.
     * The returned offset is never out of range.
     * @param _x given coordinate (relative to containing block)
     * @param ax returns exact coordinate the offset corresponds to
     *      (relative to containing block)
     * @return the offset (relative to the RenderText object, not to this run)
     */
    int offsetForPoint(int _x, int &ax) const;

    /**
     * calculates the with of the specified chunk in this text box.
     * @param pos zero-based position within the text box up to which
     *  the width is to be determined
     * @return the width in pixels
     */
    int widthFromStart(int pos) const;

    long caretMinOffset() const override;
    long caretMaxOffset() const override;
    unsigned long caretMaxRenderedOffset() const override;

    /** returns the associated render text
     */
    const RenderText *renderText() const;
    RenderText *renderText();

    void extractLine() override;
    void deleteLine(RenderArena *arena) override;
    void attachLine() override;

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
    RenderText(DOM::NodeImpl *node, DOM::DOMStringImpl *_str);
    virtual ~RenderText();

    virtual bool isTextFragment() const;
    virtual DOM::DOMStringImpl *originalString() const;

    const char *renderName() const override
    {
        return "RenderText";
    }

    void setStyle(RenderStyle *style) override;

    void detach() override;

    void deleteInlineBoxes(RenderArena *arena = nullptr) override;
    void dirtyInlineBoxes(bool fullLayout, bool) override;
    void removeInlineBox(InlineBox *_box) override;

    DOM::DOMString data() const
    {
        return str;
    }
    DOM::DOMStringImpl *string() const
    {
        return str;
    }

    InlineBox *createInlineBox(bool, bool) override;

    void layout() override
    {
        assert(false);
    }

    bool nodeAtPoint(NodeInfo &info, int x, int y, int tx, int ty, HitTestAction hitTestAction, bool inBox) override;

    RenderPosition positionForCoordinates(int _x, int _y) override;

    // Return before, after (offset set to max), or inside the text, at @p offset
    virtual FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty,
            DOM::NodeImpl *&node, int &offset,
            SelPointState &);

    unsigned int length() const override
    {
        if (str) {
            return str->l;
        } else {
            return 0;
        }
    }
    QChar *text() const
    {
        if (str) {
            return str->s;
        } else {
            return nullptr;
        }
    }
    unsigned int stringLength() const
    {
        return str->l;    // non virtual implementation of length()
    }
    void position(InlineBox *box, int from, int len, bool reverse) override;

    virtual unsigned int width(unsigned int from, unsigned int len, const Font *f) const;
    virtual unsigned int width(unsigned int from, unsigned int len, bool firstLine = false) const;
    short width() const override;
    int height() const override;

    // height of the contents (without paddings, margins and borders)
    short lineHeight(bool firstLine) const override;
    short baselinePosition(bool firstLine) const override;

    // overrides
    void calcMinMaxWidth() override;
    short minWidth() const override
    {
        return m_minWidth;
    }
    int maxWidth() const override
    {
        return m_maxWidth;
    }

    void trimmedMinMaxWidth(int &beginMinW, bool &beginWS,
                            int &endMinW, bool &endWS,
                            bool &hasBreakableChar, bool &hasBreak,
                            int &beginMaxW, int &endMaxW,
                            int &minW, int &maxW, bool &stripFrontSpaces);

    bool containsOnlyWhitespace(unsigned int from, unsigned int len) const;

    ushort startMin() const
    {
        return m_startMin;
    }
    ushort endMin() const
    {
        return m_endMin;
    }

    // returns the minimum x position of all runs relative to the parent.
    // defaults to 0.
    int minXPos() const;

    int inlineXPos() const override;
    int inlineYPos() const override;

    bool hasReturn() const
    {
        return m_hasReturn;
    }

    virtual const QFont &font();
    short verticalPositionHint(bool firstLine) const override;

    bool isFixedWidthFont() const;

    void setText(DOM::DOMStringImpl *text, bool force = false);

    SelectionState selectionState() const override
    {
        return KDE_CAST_BF_ENUM(SelectionState, m_selectionState);
    }
    void setSelectionState(SelectionState s) override
    {
        m_selectionState = s;
    }
    void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const override;
    bool absolutePosition(int &/*xPos*/, int &/*yPos*/, bool f = false) const override;
    bool posOfChar(int ch, int &x, int &y) const;
    bool isPointInsideSelection(int x, int y, const DOM::Selection &) const override;

    short marginLeft() const override
    {
        return style()->marginLeft().minWidth(0);
    }
    short marginRight() const override
    {
        return style()->marginRight().minWidth(0);
    }

    void repaint(Priority p = NormalPriority) override;

    InlineTextBox *firstTextBox() const
    {
        return m_firstTextBox;
    }
    InlineTextBox *lastTextBox() const
    {
        return m_lastTextBox;
    }

    QList< QRectF > getClientRects() override;

    bool hasBreakableChar() const
    {
        return m_hasBreakableChar;
    }
    bool isSimpleText() const
    {
        return m_isSimpleText;
    }
    const QFontMetrics &metrics(bool firstLine) const;
    const Font *htmlFont(bool firstLine) const;

    DOM::TextImpl *element() const
    {
        return static_cast<DOM::TextImpl *>(RenderObject::element());
    }

    InlineBox *inlineBox(long offset) override;

    void removeTextBox(InlineTextBox *box);
    void attachTextBox(InlineTextBox *box);
    void extractTextBox(InlineTextBox *box);

#ifdef ENABLE_DUMP
    void dump(QTextStream &stream, const QString &ind) const override;
#endif

    long caretMinOffset() const override;
    long caretMaxOffset() const override;
    unsigned long caretMaxRenderedOffset() const override;

    /** Find the text box that includes the character at @p offset
     * and return pos, which is the position of the char in the run.
     * @param offset zero-based offset into DOM string
     * @param pos returns relative position within text box
     * @param checkFirstLetter passing @p true will also regard :first-letter
     *      boxes, if available.
     * @return the text box, or 0 if no match has been found
     */
    const InlineTextBox *findInlineTextBox(int offset, int &pos,
                                           bool checkFirstLetter = false) const;

    // helper methods to convert Position from rendered text into DOM position
    // (takes into account space collapsing)
    unsigned convertToDOMPosition(unsigned position) const;
    unsigned convertToRenderedPosition(unsigned position) const;
protected:
    virtual void setTextInternal(DOM::DOMStringImpl *text);

    // members
    InlineTextBox *m_firstTextBox;
    InlineTextBox *m_lastTextBox;

    DOM::DOMStringImpl *str; //

    short m_lineHeight;
    short m_minWidth;
    int   m_maxWidth;
    short m_beginMinWidth;
    short m_endMinWidth;

    KDE_BF_ENUM(SelectionState) m_selectionState : 3;
    bool m_hasReturn : 1;
    bool m_hasBreakableChar : 1;
    bool m_hasBreak : 1;
    bool m_hasBeginWS : 1;
    bool m_hasEndWS : 1;
    bool m_isSimpleText : 1;

    ushort m_startMin : 8;
    ushort m_endMin : 8;
};

inline const RenderText *InlineTextBox::renderText() const
{
    return static_cast<RenderText *>(object());
}

inline RenderText *InlineTextBox::renderText()
{
    return static_cast<RenderText *>(object());
}

// Used to represent a text substring of an element, e.g., for text runs that are split because of
// first letter and that must therefore have different styles (and positions in the render tree).
// We cache offsets so that text transformations can be applied in such a way that we can recover
// the original unaltered string from our corresponding DOM node.
class RenderTextFragment : public RenderText
{
public:
    RenderTextFragment(DOM::NodeImpl *_node, DOM::DOMStringImpl *_str,
                       int startOffset, int endOffset);
    RenderTextFragment(DOM::NodeImpl *_node, DOM::DOMStringImpl *_str);
    ~RenderTextFragment();

    bool isTextFragment() const override;
    const char *renderName() const override
    {
        return "RenderTextFragment";
    }

    uint start() const
    {
        return m_start;
    }
    uint end() const
    {
        return m_end;
    }

    DOM::DOMStringImpl *contentString() const
    {
        return m_generatedContentStr;
    }
    DOM::DOMStringImpl *originalString() const override;

    RenderObject *firstLetter() const
    {
        return m_firstLetter;
    }
    void setFirstLetter(RenderObject *firstLetter)
    {
        m_firstLetter = firstLetter;
    }

    // overrides
    void detach() override;
private:
    void setTextInternal(DOM::DOMStringImpl *text) override;

    uint m_start;
    uint m_end;
    DOM::DOMStringImpl *m_generatedContentStr;
    RenderObject *m_firstLetter;
};
} // end namespace
#endif
