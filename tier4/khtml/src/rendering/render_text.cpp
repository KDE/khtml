/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2000-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2003, 2006 Apple Computer, Inc.
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2008 Germain Garand (germain@ebooksfrance.org)
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

//#define DEBUG_LAYOUT
//#define BIDI_DEBUG


#include "render_text.h"
#include "render_canvas.h"
#include "break_lines.h"
#include "render_arena.h"
#include "rendering/render_position.h"
#include <xml/dom_nodeimpl.h>

#include <misc/loader.h>
#include <misc/helper.h>

#include <QBitmap>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <assert.h>
#include <limits.h>
#include <math.h>

#include <config-khtml.h>

#if HAVE_ALLOCA_H
// explicitly included for systems that don't provide it in stdlib.h or malloc.h
#  include <alloca.h>
#else
#  if HAVE_MALLOC_H
#    include <malloc.h>
#  else
#    include <stdlib.h>
#  endif
#endif

using namespace khtml;
using namespace DOM;

#ifndef NDEBUG
static bool inInlineTextBoxDetach;
#endif

void InlineTextBox::detach(RenderArena* renderArena, bool noRemove)
{
    if (!noRemove) remove();

#ifndef NDEBUG
    inInlineTextBoxDetach = true;
#endif
    delete this;
#ifndef NDEBUG
    inInlineTextBoxDetach = false;
#endif

    // Recover the size left there for us by operator delete and free the memory.
    renderArena->free(*(size_t *)this, this);
}

void* InlineTextBox::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void InlineTextBox::operator delete(void* ptr, size_t sz)
{
    assert(inInlineTextBoxDetach);

#ifdef KHTML_USE_ARENA_ALLOCATOR
    // Stash size where detach can find it.
    *(size_t *)ptr = sz;
#endif
}

void InlineTextBox::selectionStartEnd(int& sPos, int& ePos)
{
    int startPos, endPos;
    if (object()->selectionState() == RenderObject::SelectionInside) {
        startPos = 0;
        endPos = renderText()->string()->l;
    } else {
        renderText()->selectionStartEnd(startPos, endPos);
        if (object()->selectionState() == RenderObject::SelectionStart)
            endPos = renderText()->string()->l;
        else if (object()->selectionState() == RenderObject::SelectionEnd)
            startPos = 0;
    }

    sPos = qMax(startPos - m_start, 0);
    ePos = qMin(endPos - m_start, (int)m_len);
}

RenderObject::SelectionState InlineTextBox::selectionState()
{
    RenderObject::SelectionState state = object()->selectionState();
    if (state == RenderObject::SelectionStart || state == RenderObject::SelectionEnd ||
        state == RenderObject::SelectionBoth) {
        int startPos, endPos;
        renderText()->selectionStartEnd(startPos, endPos);
        
        bool start = (state != RenderObject::SelectionEnd && startPos >= m_start && startPos < m_start + m_len);
        bool end = (state != RenderObject::SelectionStart && endPos > m_start && endPos <= m_start + m_len);
        if (start && end)
            state = RenderObject::SelectionBoth;
        else if (start)
            state = RenderObject::SelectionStart;
        else if (end)
            state = RenderObject::SelectionEnd;
        else if ((state == RenderObject::SelectionEnd || startPos < m_start) &&
                 (state == RenderObject::SelectionStart || endPos > m_start + m_len))
            state = RenderObject::SelectionInside;
    }
    return state;
}

void InlineTextBox::paint(RenderObject::PaintInfo& i, int tx, int ty)
{
    if (object()->isBR() || object()->style()->visibility() != VISIBLE ||
        m_truncation == cFullTruncation || i.phase == PaintActionOutline)
        return;

    if (i.phase == PaintActionSelection && object()->selectionState() == RenderObject::SelectionNone)
        // When only painting the selection, don't bother to paint if there is none.
        return;

    int xPos = tx + m_x;
    int w = width();
    if ((xPos >= i.r.x() + i.r.width()) || (xPos + w <= i.r.x()))
        return;

    // Set our font.
    RenderStyle* styleToUse = object()->style(m_firstLine);
    int d = styleToUse->textDecorationsInEffect();
    if (styleToUse->font() != i.p->font())
        i.p->setFont(styleToUse->font());
    const Font *font = &styleToUse->htmlFont();
    bool haveSelection = selectionState() != RenderObject::SelectionNone;
    
    // Now calculate startPos and endPos, for painting selection.
    // We paint selection while endPos > 0
    int ePos = 0, sPos = 0;
    if (haveSelection && !object()->canvas()->staticMode()) {
        selectionStartEnd(sPos, ePos);
    }
    i.p->setPen(styleToUse->color());

    if (m_len > 0 && i.phase != PaintActionSelection) {
        int endPoint = m_len;
        if (m_truncation != cNoTruncation)
            endPoint = m_truncation - m_start;
        if (styleToUse->textShadow())
            paintShadow(i.p, font, tx, ty, styleToUse->textShadow());
        if (!haveSelection || sPos != 0 || ePos != m_len) {
            font->drawText(i.p, m_x + tx, m_y + ty + m_baseline, renderText()->string()->s, renderText()->string()->l, m_start, endPoint,
                           m_toAdd, m_reversed ? Qt::RightToLeft : Qt::LeftToRight);
        }
    }

    if (d != TDNONE && i.phase != PaintActionSelection && styleToUse->htmlHacks()) {
        i.p->setPen(styleToUse->color());
        paintDecoration(i.p, font, tx, ty, d);
    }

    if (haveSelection && i.phase == PaintActionSelection) {
        //qDebug() << this << " paintSelection with startPos=" << sPos << " endPos=" << ePos;
        if ( sPos < ePos )
	    paintSelection(font, renderText(), i.p, styleToUse, tx, ty, sPos, ePos, d);
    }
}

/** returns the proper ::selection pseudo style for the given element
 * @return the style or 0 if no ::selection pseudo applies.
 */
inline const RenderStyle *retrieveSelectionPseudoStyle(const RenderObject *obj)
{
  // http://www.w3.org/Style/CSS/Test/CSS3/Selectors/20021129/html/tests/css3-modsel-162.html
  // is of the opinion that ::selection of parent elements is also to be applied
  // to children, so let's do it.
  while (obj) {
    const RenderStyle *style = obj->style()->getPseudoStyle(RenderStyle::SELECTION);
    if (style) return style;

    obj = obj->parent();
  }/*wend*/
  return 0;
}

void InlineTextBox::paintSelection(const Font *f, RenderText *text, QPainter *p, RenderStyle* style, int tx, int ty, int startPos, int endPos, int deco)
{
    if(startPos > m_len) return;
    if(startPos < 0) startPos = 0;

    QColor hc;
    QColor hbg;
    const RenderStyle* pseudoStyle = retrieveSelectionPseudoStyle(text);
    if (pseudoStyle) {
        // ### support outline (mandated by CSS3)
	// ### support background-image? (optional by CSS3)
        if (pseudoStyle->backgroundColor().isValid())
            hbg = pseudoStyle->backgroundColor();
        hc = pseudoStyle->color();
    } else {
        hc = style->palette().color( QPalette::Active, QPalette::HighlightedText );
        hbg = style->palette().color( QPalette::Active, QPalette::Highlight );
	// ### should be at most retrieved once per render text
	QColor bg = khtml::retrieveBackgroundColor(text);
	// It may happen that the contrast is -- well -- virtually non existent.
	// In this case, simply swap the colors, thus in compliance with
	// NN4 (win32 only), IE, and Mozilla.
	if (!khtml::hasSufficientContrast(hbg, bg))
	    qSwap(hc, hbg);
    }

    p->setPen(hc);

    //qDebug() << "textRun::painting(" << QString::fromRawData(text->str->s + m_start, m_len).left(30) << ") at(" << m_x+tx << "/" << m_y+ty << ")";

   const bool needClipping = startPos != 0 || endPos != m_len;

   if (needClipping) {
       p->save();

       int visualSelectionStart = f->width(text->str->s, text->str->l, m_start, startPos, false, m_start, m_start + m_len, m_toAdd);
       int visualSelectionEnd = f->width(text->str->s, text->str->l, m_start, endPos, false, m_start, m_start + m_len, m_toAdd);
       int visualSelectionWidth = visualSelectionEnd - visualSelectionStart;
       if (m_reversed) {
           visualSelectionStart = f->width(text->str->s, text->str->l, m_start, m_len, false) - visualSelectionEnd;
       }

       QRect selectionRect(m_x + tx + visualSelectionStart, m_y + ty, visualSelectionWidth, height());
       QRegion r(selectionRect);
       if (p->hasClipping())
           r &= p->clipRegion();
       p->setClipRegion(r, Qt::IntersectClip);
    }

    f->drawText(p, m_x + tx, m_y + ty + m_baseline, text->str->s, text->str->l,
		m_start, m_len, m_toAdd,
		m_reversed ? Qt::RightToLeft : Qt::LeftToRight,
                needClipping ? 0 : startPos, needClipping ? m_len : endPos,
		hbg, m_y + ty, height(), deco);

    if (needClipping) p->restore();
}

void InlineTextBox::paintDecoration( QPainter *pt, const Font *f, int _tx, int _ty, int deco)
{
    _tx += m_x;
    _ty += m_y;

    if (m_truncation == cFullTruncation)
        return;

    int width = m_width - 1;
    if (m_truncation != cNoTruncation) {
        width = static_cast<RenderText*>(m_object)->width(m_start, m_truncation - m_start, m_firstLine);
    }

    RenderObject *p = object();

    QColor underline, overline, linethrough;
    p->getTextDecorationColors(deco, underline, overline, linethrough, p->style()->htmlHacks());

    if(deco & UNDERLINE){
        pt->setPen(underline);
        f->drawDecoration(pt, _tx, _ty, baseline(), width, height(), Font::UNDERLINE);
    }
    if (deco & OVERLINE) {
        pt->setPen(overline);
        f->drawDecoration(pt, _tx, _ty, baseline(), width, height(), Font::OVERLINE);
    }
    if(deco & LINE_THROUGH) {
        pt->setPen(linethrough);
        f->drawDecoration(pt, _tx, _ty, baseline(), width, height(), Font::LINE_THROUGH);
    }
    // NO! Do NOT add BLINK! It is the most annouing feature of Netscape, and IE has a reason not to
    // support it. Lars
}

void InlineTextBox::paintShadow(QPainter *pt, const Font *f, int _tx, int _ty, const ShadowData *shadow )
{
    int x = m_x + _tx + shadow->x;
    int y = m_y + _ty + shadow->y;
    const RenderText* text = renderText();

    if (shadow->blur <= 0) {
        QColor c = pt->pen().color();
        pt->setPen(shadow->color);
        f->drawText(pt, x, y+m_baseline, text->str->s, text->str->l,
                    m_start, m_len, m_toAdd,
                    m_reversed ? Qt::RightToLeft : Qt::LeftToRight);
        pt->setPen(c);

    }
    else {
        const int thickness = shadow->blur;
        const int w = m_width+2*thickness;
        const int h = m_height+2*thickness;
        const QRgb color = shadow->color.rgba();
        const int gray = qGray(color);
        const bool inverse = (gray < 100);
        const QRgb bgColor = (inverse) ? qRgb(255,255,255) : qRgb(0,0,0);
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(bgColor);
        QPainter p;

        p.begin(&img);
        p.setPen(shadow->color);
        p.setFont(pt->font());
        f->drawText(&p, thickness, thickness+m_baseline, text->str->s, text->str->l,
                    m_start, m_len, m_toAdd,
                    m_reversed ? Qt::RightToLeft : Qt::LeftToRight);

        p.end();

        int md = thickness*thickness; // max-dist^2

        // blur map (division cache)
        float *bmap = (float*)alloca(sizeof(float)*(md+1));
        for(int n=0; n<=md; n++) {
            float f;
            f = n/(float)(md+1);
            f = 1.0 - f*f;
            bmap[n] = f;
        }

        float factor = 0.0; // maximal potential opacity-sum
        for(int n=-thickness; n<=thickness; n++)
            for(int m=-thickness; m<=thickness; m++) {
                int d = n*n+m*m;
                if (d<=md)
                    factor += bmap[d];
            }

        // arbitratry factor adjustment to make shadows solid.
        factor = factor/1.333;

        // alpha map
        float* amap = (float*)alloca(sizeof(float)*(h*w));
        memset(amap, 0, h*w*(sizeof(float)));
        for(int j=thickness; j<h-thickness; j++) {
            const QRgb *line = (QRgb*)img.scanLine(j);
            for(int i=thickness; i<w-thickness; i++) {
                QRgb col= line[i];
                if (col == bgColor) continue;
                float g = qGray(col);
                if (inverse)
                    g = (255-g)/(255-gray);
                else
                    g = g/gray;
                for(int n=-thickness; n<=thickness; n++) {
                    for(int m=-thickness; m<=thickness; m++) {
                        int d = n*n+m*m;
                        if (d>md) continue;
                        float f = bmap[d];
                        amap[(i+m)+(j+n)*w] += (g*f);
                    }
                }
            }
        }

        QImage res(w,h,QImage::Format_ARGB32);
        int r = qRed(color);
        int g = qGreen(color);
        int b = qBlue(color);

        // divide by factor
        factor = 1.0/factor;

        for(int j=0; j<h; j++) {
            QRgb *line = (QRgb*)res.scanLine(j);
            for(int i=0; i<w; i++) {
                int a = (int)(amap[i+j*w] * factor * 255.0);
                if (a > 255) a = 255;
                line[i] = qRgba(r,g,b,a);
            }
        }

        pt->drawImage(x-thickness, y-thickness, res, 0, 0, -1, -1, Qt::DiffuseAlphaDither | Qt::ColorOnly | Qt::PreferDither);
    }
    // Paint next shadow effect
    if (shadow->next) paintShadow(pt, f, _tx, _ty, shadow->next);
}

/**
 * Distributes pixels to justify text.
 * @param numSpaces spaces left, will be decremented by one
 * @param toAdd number of pixels left to be distributed, will have the
 *	amount of pixels distributed during this call subtracted.
 * @return number of pixels to distribute
 */
static inline int justifyWidth(int &numSpaces, int &toAdd) {
  int a = 0;
  if ( numSpaces ) {
    a = toAdd/numSpaces;
    toAdd -= a;
    numSpaces--;
  }/*end if*/
  return a;
}

FindSelectionResult InlineTextBox::checkSelectionPoint(int _x, int _y, int _tx, int _ty, int & offset)
{
//       qDebug() << "InlineTextBox::checkSelectionPoint " << this << " _x=" << _x << " _y=" << _y
//                     << " _tx+m_x=" << _tx+m_x << " _ty+m_y=" << _ty+m_y << endl;
    offset = 0;

    if ( _y < _ty + m_y )
        return SelectionPointBefore; // above -> before

    if ( _y > _ty + m_y + m_height ) {
        // below -> after
        // Set the offset to the max
        offset = m_len;
        return SelectionPointAfter;
    }
    if ( _x > _tx + m_x + m_width ) {
	// to the right
	return  SelectionPointAfterInLine;
    }

    // The Y matches, check if we're on the left
    if ( _x < _tx + m_x ) {
        return SelectionPointBeforeInLine;
    }

    // consider spacing for justified text
    int toAdd = m_toAdd;
    RenderText *text = static_cast<RenderText *>(object());
    Q_ASSERT(text->isText());
    bool justified = text->style()->textAlign() == JUSTIFY && toAdd > 0;
    int numSpaces = 0;
    if (justified) {
        for( int i = 0; i < m_len; i++ )
            if ( text->str->s[m_start+i].category() == QChar::Separator_Space )
	        numSpaces++;

    }/*end if*/

    int delta = _x - (_tx + m_x);
    //qDebug() << "InlineTextBox::checkSelectionPoint delta=" << delta;
    int pos = 0;
    const Font *f = text->htmlFont(m_firstLine);
    if ( m_reversed ) {
	delta -= m_width;
	while(pos < m_len) {
	    int w = f->charWidth( text->str->s, text->str->l, m_start + pos, text->isSimpleText());
	    if (justified && text->str->s[m_start + pos].category() == QChar::Separator_Space)
	        w += justifyWidth(numSpaces, toAdd);
	    int w2 = w/2;
	    w -= w2;
	    delta += w2;
	    if(delta >= 0) break;
	    pos++;
	    delta += w;
	}
    } else {
	while(pos < m_len) {
	    int w = f->charWidth( text->str->s, text->str->l, m_start + pos, text->isSimpleText());
	    if (justified && text->str->s[m_start + pos].category() == QChar::Separator_Space)
	        w += justifyWidth(numSpaces, toAdd);
	    int w2 = w/2;
	    w -= w2;
	    delta -= w2;
	    if(delta <= 0) break;
	    pos++;
	    delta -= w;
	}
    }
//     qDebug() << " Text  --> inside at position " << pos;
    offset = pos;
    return SelectionPointInside;
}

long InlineTextBox::caretMinOffset() const
{
    return m_start;
}

long InlineTextBox::caretMaxOffset() const
{
    return m_start + m_len;
}

unsigned long InlineTextBox::caretMaxRenderedOffset() const
{
    return m_start + m_len;
}

int InlineTextBox::offsetForPoint(int _x, int &ax) const
{
  // Do binary search for finding out offset, saves some time for long
  // runs.
  int start = 0;
  int end = m_len;
  ax = m_x;
  int offset = (start + end) / 2;
  while (end - start > 0) {
    // always snap to the right column. This makes up for "jumpy" vertical
    // navigation.
    if (end - start == 1) start = end;

    offset = (start + end) / 2;
    ax = m_x + widthFromStart(offset);
    if (ax > _x) end = offset;
    else if (ax < _x) start = offset;
    else break;
  }
  return m_start + offset;
}

int InlineTextBox::widthFromStart(int pos) const
{
  // gasp! sometimes pos is i < 0 which crashes Font::width
  // qDebug() << this << pos << endl;
  pos = qMax(pos, 0);

  const RenderText *t = renderText();
  Q_ASSERT(t->isText());
  const Font *f = t->htmlFont(m_firstLine);
  const QFontMetrics &fm = t->fontMetrics(m_firstLine);

  int numSpaces = 0;
  // consider spacing for justified text
  bool justified = t->style()->textAlign() == JUSTIFY;
  //qDebug() << "InlineTextBox::width(int)";
  if (justified && m_toAdd > 0) do {
    //qDebug() << "justify";

//    const QString cstr = QString::fromRawData(t->str->s + m_start, m_len);
    for( int i = 0; i < m_len; i++ )
      if ( t->str->s[m_start+i].category() == QChar::Separator_Space )
	numSpaces++;
    if (numSpaces == 0) break;

    int toAdd = m_toAdd;
    int w = 0;		// accumulated width
    int start = 0;	// start of non-space sequence
    int current = 0;	// current position
    while (current < pos) {
      // add spacing
      while (current < pos && t->str->s[m_start + current].category() == QChar::Separator_Space) {
	w += f->getWordSpacing();
	w += f->getLetterSpacing();
	w += justifyWidth(numSpaces, toAdd);
        w += fm.width(' ');	// ### valid assumption? (LS)
        current++; start++;
      }/*wend*/
      if (current >= pos) break;

      // seek next space
      while (current < pos && t->str->s[m_start + current].category() != QChar::Separator_Space)
        current++;

      // check run without spaces
      if ( current > start ) {
          w += f->width(t->str->s + m_start, m_len, start, current - start, false);
          start = current;
      }
    }

    return w;

  } while(false);/*end if*/

  //qDebug() << "default";
  // else use existing width function
  // qDebug() << "result width:" << f->width(t->str->s + m_start, m_len, 0, pos, false) << endl;
  return f->width(t->str->s + m_start, m_len, 0, pos, false);

}

void InlineTextBox::deleteLine(RenderArena* arena)
{
    static_cast<RenderText*>(m_object)->removeTextBox(this);
    detach(arena, true /*noRemove*/);
}

void InlineTextBox::extractLine()
{
    if (m_extracted)
        return;
    static_cast<RenderText*>(m_object)->extractTextBox(this);
}

void InlineTextBox::attachLine()
{
    if (!m_extracted)
        return;
    static_cast<RenderText*>(m_object)->attachTextBox(this);
}

int InlineTextBox::placeEllipsisBox(bool ltr, int blockEdge, int ellipsisWidth, bool& foundBox)
{
    if (foundBox) {
        m_truncation = cFullTruncation;
        return -1;
    }

    int ellipsisX = ltr ? blockEdge - ellipsisWidth : blockEdge + ellipsisWidth;

    // For LTR, if the left edge of the ellipsis is to the left of our text run, then we are the run that will get truncated.
    if (ltr) {
        if (ellipsisX <= m_x) {
            // Too far.  Just set full truncation, but return -1 and let the ellipsis just be placed at the edge of the box.
            m_truncation = cFullTruncation;
            foundBox = true;
            return -1;
        }

        if (ellipsisX < m_x + m_width) {
            if (m_reversed)
                return -1; // FIXME: Support LTR truncation when the last run is RTL someday.

            foundBox = true;

            int ax;
            int offset = offsetForPoint(ellipsisX, ax) - 1;
            if (offset <= m_start) {
                // No characters should be rendered.  Set ourselves to full truncation and place the ellipsis at the min of our start
                // and the ellipsis edge.
                m_truncation = cFullTruncation;
                return qMin(ellipsisX, (int)m_x);
            }

            // Set the truncation index on the text run.  The ellipsis needs to be placed just after the last visible character.
            m_truncation = offset;
            return widthFromStart(offset - m_start);
        }
    }
    else {
        // FIXME: Support RTL truncation someday, including both modes (when the leftmost run on the line is either RTL or LTR)
    }
    return -1;
}

// -----------------------------------------------------------------------------

RenderText::RenderText(DOM::NodeImpl* node, DOMStringImpl *_str)
    : RenderObject(node)
{
    // init RenderObject attributes
    setRenderText();   // our object inherits from RenderText

    m_minWidth = -1;
    m_maxWidth = -1;
    str = _str;
    if(str) str->ref();
    KHTMLAssert(!str || !str->l || str->s);

    m_selectionState = SelectionNone;
    m_hasReturn = true;
    m_isSimpleText = false;
    m_firstTextBox = m_lastTextBox = 0;

#ifdef DEBUG_LAYOUT
    const QString cstr = QString::fromRawData(str->s, str->l);
    qDebug() << "RenderText ctr( "<< cstr.length() << " )  '" << cstr << "'";
#endif
}

void RenderText::setStyle(RenderStyle *_style)
{
    if ( style() != _style ) {
        bool changedText = ((!style() && ( _style->textTransform() != TTNONE ||
                                           !_style->preserveLF() || !_style->preserveWS() )) ||
                            (style() && (style()->textTransform() != _style->textTransform() ||
                                         style()->whiteSpace() != _style->whiteSpace())));

        RenderObject::setStyle( _style );
        m_lineHeight = RenderObject::lineHeight(false);

        if (!isBR() && changedText) {
            DOM::DOMStringImpl* textToTransform = originalString();
            if (textToTransform)
                setText(textToTransform, true);
        }
    }
}

RenderText::~RenderText()
{
    if(str) str->deref();
    assert(!m_firstTextBox);
    assert(!m_lastTextBox);
}

void RenderText::detach()
{
    if (!documentBeingDestroyed()) {
        if (firstTextBox()) {
            if (isBR()) {
                RootInlineBox* next = firstTextBox()->root()->nextRootBox();
                if (next)
                    next->markDirty();
            }
            for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
                box->remove();
        } else if (parent())
            parent()->dirtyLinesFromChangedChild(this);
    }
    deleteInlineBoxes();
    RenderObject::detach();
}

void RenderText::extractTextBox(InlineTextBox* box)
{
    m_lastTextBox = box->prevTextBox();
    if (box == m_firstTextBox)
        m_firstTextBox = 0;
    if (box->prevTextBox())
        box->prevTextBox()->setNextLineBox(0);
    box->setPreviousLineBox(0);
    for (InlineRunBox* curr = box; curr; curr = curr->nextLineBox())
        curr->setExtracted();
}

void RenderText::attachTextBox(InlineTextBox* box)
{
    if (m_lastTextBox) {
        m_lastTextBox->setNextLineBox(box);
        box->setPreviousLineBox(m_lastTextBox);
    } else
        m_firstTextBox = box;
    InlineTextBox* last = box;
    for (InlineTextBox* curr = box; curr; curr = curr->nextTextBox()) {
        curr->setExtracted(false);
        last = curr;
    }
    m_lastTextBox = last;
}

void RenderText::removeTextBox(InlineTextBox* box)
{
    if (box == m_firstTextBox)
        m_firstTextBox = box->nextTextBox();
    if (box == m_lastTextBox)
        m_lastTextBox = box->prevTextBox();
    if (box->nextTextBox())
        box->nextTextBox()->setPreviousLineBox(box->prevTextBox());
    if (box->prevTextBox())
        box->prevTextBox()->setNextLineBox(box->nextTextBox());
}

void RenderText::removeInlineBox(InlineBox* _box)
{
    KHTMLAssert( _box->isInlineTextBox() );
    removeTextBox( static_cast<InlineTextBox*>(_box) );
}

void RenderText::deleteInlineBoxes(RenderArena* /*arena*/)
{
    if (firstTextBox()) {
        RenderArena* arena = renderArena();
        InlineTextBox* next;
        for (InlineTextBox* curr = firstTextBox(); curr; curr = next) {
            next = curr->nextTextBox();
            curr->detach(arena, true /*noRemove*/);
        }
        m_firstTextBox = m_lastTextBox = 0;
    }
}

void RenderText::dirtyInlineBoxes(bool fullLayout, bool)
{
    if (fullLayout)
        deleteInlineBoxes();
    else {
        for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
            box->dirtyInlineBoxes();
    }
}

bool RenderText::isTextFragment() const
{
    return false;
}

DOM::DOMStringImpl* RenderText::originalString() const
{
    return element() ? element()->string() : 0;
}

const InlineTextBox * RenderText::findInlineTextBox( int offset, int &pos, bool checkFirstLetter ) const
{
  Q_UNUSED( checkFirstLetter );
    // The text boxes point to parts of the rendertext's str string
    // (they don't include '\n')
    // Find the text box that includes the character at @p offset
    // and return pos, which is the position of the char in the run.

    if ( !m_firstTextBox )
        return 0L;

    InlineTextBox* s = m_firstTextBox;
    int off = s->m_len;
    while (offset > off && s->nextTextBox()) {
        s = s->nextTextBox();
        off = s->m_start + s->m_len;
    }
    // we are now in the correct text run
    if (offset >= s->m_start && offset < s->m_start + s->m_len) {
        pos = offset - s->m_start;
    } else {
        pos = (offset > off ? s->m_len : s->m_len - (off - offset) );
    }
    return s;
}

bool RenderText::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty, HitTestAction /*hitTestAction*/, bool /*inBox*/)
{
    assert(parent());

    bool inside = false;
    if (style()->visibility() != HIDDEN) {
        for (InlineTextBox* s = firstTextBox(); s; s = s->nextTextBox()) {
            if((_y >=_ty + s->m_y) && (_y < _ty + s->m_y + s->m_height) &&
               (_x >= _tx + s->m_x) && (_x <_tx + s->m_x + s->m_width) ) {
                inside = true;
                break;
            }
        }
    }

    // #### ported over from Safari. Can this happen at all? (lars)

    if (inside && element()) {
        if (info.innerNode() && info.innerNode()->renderer() &&
            !info.innerNode()->renderer()->isInline()) {
            // Within the same layer, inlines are ALWAYS fully above blocks.  Change inner node.
            info.setInnerNode(element());

            // Clear everything else.
            info.setInnerNonSharedNode(0);
            info.setURLElement(0);
        }

        if (!info.innerNode())
            info.setInnerNode(element());

        if(!info.innerNonSharedNode())
            info.setInnerNonSharedNode(element());
    }

    return inside;
}

FindSelectionResult RenderText::checkSelectionPoint(int _x, int _y, int _tx, int _ty, DOM::NodeImpl*& node, int &offset, SelPointState &)
{
//        qDebug() << "RenderText::checkSelectionPoint " << this << " _x=" << _x << " _y=" << _y
//                     << " _tx=" << _tx << " _ty=" << _ty << endl;
//qDebug() << renderName() << "::checkSelectionPoint x=" << xPos() << " y=" << yPos() << " w=" << width() << " h=" << height();

    NodeImpl *lastNode = 0;
    int lastOffset = 0;
    FindSelectionResult lastResult = SelectionPointAfter;

    for (InlineTextBox* s = firstTextBox(); s; s = s->nextTextBox())
    {
        FindSelectionResult result;
// #### ?
//         result = s->checkSelectionPoint(_x, _y, _tx, _ty, offset);
       if (_y < _ty + s->m_y) result = SelectionPointBefore;
       else if (_y >= _ty + s->m_y + s->height()) result = SelectionPointAfterInLine;
       else if (_x < _tx + s->m_x) result = SelectionPointBeforeInLine;
       else if (_x >= _tx + s->m_x + s->width()) result = SelectionPointAfterInLine;
       else {
           int dummy;
           result = SelectionPointInside;
           // offsetForPoint shifts to the right: correct it
           offset = s->offsetForPoint(_x - _tx, dummy) - 1;
       }

//         qDebug() << "RenderText::checkSelectionPoint " << this << " line " << si << " result=" << result << " offset=" << offset;
        if ( result == SelectionPointInside ) // x,y is inside the textrun
        {
//            offset += s->m_start; // add the offset from the previous lines
//             qDebug() << "RenderText::checkSelectionPoint inside -> " << offset;
            node = element();
            return SelectionPointInside;
        } else if ( result == SelectionPointBefore ) {
	    if (!lastNode) {
                // x,y is before the textrun -> stop here
               offset = 0;
//                qDebug() << "RenderText::checkSelectionPoint " << this << "before us -> returning Before";
               node = element();
               return SelectionPointBefore;
	    }
        } else if ( result == SelectionPointBeforeInLine ) {
	    offset = s->m_start;
	    node = element();
	    return SelectionPointInside;
        } else if ( result == SelectionPointAfterInLine ) {
	    lastOffset = s->m_start + s->m_len;
	    lastNode = element();
	    lastResult = result;
	    // no return here
	}

    }

    if (lastNode) {
        offset = lastOffset;
	node = lastNode;
//         qDebug() << "RenderText::checkSelectionPoint: lastNode " << lastNode << " lastOffset " << lastOffset;
	return lastResult;
    }

    // set offset to max
    offset = str->l;
    //qDebug("setting node to %p", element());
    node = element();
//     qDebug() << "RenderText::checkSelectionPoint: node " << node << " offset " << offset;
    return SelectionPointAfter;
}

unsigned RenderText::convertToDOMPosition(unsigned position) const
{
    if (isBR())
        return 0;
    /*const */DOMStringImpl* domString = originalString();
    /*const */DOMStringImpl* renderedString = string();
    if (domString == renderedString) {
        // qDebug() << "[rendered == dom]" << position << endl;
        return position;
    }
    /* // qDebug() << "[convert]" << position << endl
        << DOMString(domString) << endl
        << DOMString(renderedString) << endl;*/

    if (!domString || !renderedString)
        return position;

    unsigned domLength = domString->length();
    unsigned i = 0, j = 0;
    for (; i < domLength && j < position;) {
        bool isRenderedSpace = renderedString->unicode()[j].isSpace();
        bool isDOMSpace = domString->unicode()[i].isSpace();
        if (isRenderedSpace && isDOMSpace) {
            ++i;
            ++j;
            continue;
        }
        if (isRenderedSpace) {
            ++j;
            continue;
        }
        if (isDOMSpace) {
            ++i;
            continue;
        }
        ++i;
        ++j;
    }
    // qDebug() << "[result]" << i << endl;
    return i;
}

unsigned RenderText::convertToRenderedPosition(unsigned position) const
{
    if (isBR())
        return 0;
    /*const */DOMStringImpl* domString = originalString();
    /*const */DOMStringImpl* renderedString = string();
    if (domString == renderedString) {
        // qDebug() << "[rendered == dom]" << position << endl;
        return position;
    }
    /* // qDebug() << "[convert]" << position << endl
        << DOMString(domString) << endl
        << DOMString(renderedString) << endl;*/

    if (!domString || !renderedString)
        return position;

    unsigned renderedLength = renderedString->length();
    unsigned i = 0, j = 0;
    for (; i < position && j < renderedLength;) {
        bool isRenderedSpace = renderedString->unicode()[j].isSpace();
        bool isDOMSpace = domString->unicode()[i].isSpace();
        if (isRenderedSpace && isDOMSpace) {
            ++i;
            ++j;
            continue;
        }
        if (isRenderedSpace) {
            ++j;
            continue;
        }
        if (isDOMSpace) {
            ++i;
            continue;
        }
        ++i;
        ++j;
    }
    // qDebug() << "[result]" << j << endl;
    return j;
}

RenderPosition RenderText::positionForCoordinates(int _x, int _y)
{
    // qDebug() << this << _x << _y << endl;
    if (!firstTextBox() || stringLength() == 0)
        return Position(element(), 0);

    int absx, absy;
    containingBlock()->absolutePosition(absx, absy);
    // qDebug() << "absolute(" << absx << absy << ")" << endl;

    if (_y < absy + firstTextBox()->root()->bottomOverflow() && _x < absx + firstTextBox()->m_x) {
        // at the y coordinate of the first line or above
        // and the x coordinate is to the left than the first text box left edge
        return RenderPosition(element(), firstTextBox()->m_start);
    }

    if (_y >= absy + lastTextBox()->root()->topOverflow() && _x >= absx + lastTextBox()->m_x + lastTextBox()->m_width) {
        // at the y coordinate of the last line or below
        // and the x coordinate is to the right than the last text box right edge
        return RenderPosition(element(), lastTextBox()->m_start + lastTextBox()->m_len);
    }

    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        // qDebug() << "[check box]" << box << endl;
        if (_y >= absy + box->root()->topOverflow() && _y < absy + box->root()->bottomOverflow()) {
            if (_x < absx + box->m_x + box->m_width) {
                // and the x coordinate is to the left of the right edge of this box
                // check to see if position goes in this box
                int offset;
                box->checkSelectionPoint(_x, absy + box->yPos(), absx, absy, offset);
                // qDebug() << "offset" << offset << endl;
                if (offset != -1) {
                    // qDebug() << "return" << Position(element(), convertToDOMPosition(offset + box->m_start)) << endl;
                    return RenderPosition(element(), offset + box->m_start);
                }
            }
            else if (!box->prevOnLine() && _x < absx + box->m_x)
                // box is first on line
                // and the x coordinate is to the left than the first text box left edge
                return RenderPosition(element(), box->m_start);
            else if (!box->nextOnLine() && _x >= absx + box->m_x + box->m_width)
                // box is last on line
                // and the x coordinate is to the right than the last text box right edge
                return RenderPosition(element(), box->m_start + box->m_len);
        }
    }
    return RenderPosition(element(), 0);
}

void RenderText::caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const
{
    // qDebug() << offset << flags << endl;
  if (!m_firstTextBox) {
    _x = _y = height = -1;
    width = 1;
    return;
  }

  int pos;
  const InlineTextBox * s = findInlineTextBox( offset, pos, true );
  const RenderText *t = s->renderText();
//  qDebug() << "offset="<<offset << " pos="<<pos;

  const QFontMetrics &fm = t->metrics( s->m_firstLine );
  height = fm.height(); // s->m_height;

  _x = s->m_x + s->widthFromStart(pos);
  _y = s->m_y + s->baseline() - fm.ascent();
  // qDebug() << "(" << _x << _y << ")" << endl;
  width = 1;
  if (flags & CFOverride) {
    width = offset < caretMaxOffset() ? fm.width(str->s[offset]) : 1;
    // qDebug() << "CFOverride" << width << endl;
  }/*end if*/
#if 0
  // qDebug() << "_x="<<_x << " s->m_x="<<s->m_x
  		<< " s->m_start"<<s->m_start
		<< " s->m_len" << s->m_len << " _y=" << _y << endl;
#endif

  int absx, absy;

  if (absolutePosition(absx,absy))
  {
    //qDebug() << "absx=" << absx << " absy=" << absy;
    _x += absx;
    _y += absy;
  } else {
    // we don't know our absolute position, and there is no point returning
    // just a relative one
    _x = _y = -1;
  }
}

long RenderText::caretMinOffset() const
{
  if (!m_firstTextBox) return 0;
  // FIXME: it is *not* guaranteed that the first run contains the lowest offset
  // Either make this a linear search (slow),
  // or maintain an index (needs much mem),
  // or calculate and store it in bidi.cpp (needs calculation even if not needed)
  // (LS)
  return m_firstTextBox->m_start;
}

long RenderText::caretMaxOffset() const
{
    InlineTextBox* box = m_lastTextBox;
    if (!box)
        return str->l;
    int maxOffset = box->m_start + box->m_len;
    // ### slow
    for (box = box->prevTextBox(); box; box = box->prevTextBox())
        maxOffset = qMax(maxOffset, box->m_start + box->m_len);
    return maxOffset;
}

unsigned long RenderText::caretMaxRenderedOffset() const
{
    int l = 0;
    // ### no guarantee that the order is ascending
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        l += box->m_len;
    }
    return l;
}

InlineBox *RenderText::inlineBox(long offset)
{
    // ### make educated guess about most likely position
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        if (offset >= box->m_start && offset <= box->m_start + box->m_len) {
            return box;
        }
        else if (offset < box->m_start) {
            // The offset we're looking for is before this node
            // this means the offset must be in content that is
            // not rendered.
            return box->prevTextBox() ? box->prevTextBox() : firstTextBox();
        }
    }

    return 0;
}

bool RenderText::absolutePosition(int &xPos, int &yPos, bool) const
{
    return RenderObject::absolutePosition(xPos, yPos, false);
}

bool RenderText::posOfChar(int chr, int &x, int &y) const
{
    if (!parent())
        return false;
    parent()->absolutePosition( x, y, false );

    int pos;
    const InlineTextBox * s = findInlineTextBox( chr, pos );

    if ( s ) {
        // s is the line containing the character
        x += s->m_x; // this is the x of the beginning of the line, but it's good enough for now
        y += s->m_y;
        return true;
    }

    return false;
}

static bool isSimpleChar( const unsigned short c )
{
    // Exclude ranges with many Mn/Me/Mc and the various combining diacriticals ranges.
    // Unicode version used is 4.1.0

    // General Combining Diacritical Marks
    if (c < 0x300)  return true;
    if (c <= 0x36F) return false;

    // Cyrillic's
    if (c < 0x483) return true;
    if (c <= 0x489) return false;

    // Hebrew's
    if (c < 0x0591) return true;
    if (c <= 0x05C7 && !(c == 0x05BE || c == 0x05C0 || c == 0x05C3 || c == 0x05C6)) return false;

    // Unicode range 6 to 11 (Arabic to Korean Hangul)
    if (c < 0x0600)  return true;
    if (c <= 0x11F9) return false;

    // Unicode range 17 to 1A (Tagalog to Buginese)
    // (also excl. Ethiopic Combining Gemination Mark)
    if (c < 0x1700 && c != 0x135F)  return true;
    if (c <= 0x1A1F) return false;

    // Combining Diacritical Marks Supplement
    if (c < 0x1DC0) return true;
    if (c <= 0x1DFF) return false;

    // Diacritical Marks for Symbols 
    if (c < 0x20D0)  return true;
    if (c <= 0x20EB) return false;

    // Combining Half Marks
    if (c < 0xFE20)  return true;
    if (c <= 0xFE2F) return false;

    return true;
}

void RenderText::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );

    // ### calc Min and Max width...
    m_minWidth = m_beginMinWidth = m_endMinWidth = 0;
    m_maxWidth = 0;

    if (isBR())
        return;

    int currMinWidth = 0;
    int currMaxWidth = 0;
    m_isSimpleText = true;
    m_hasBreakableChar = m_hasBreak = m_hasBeginWS = m_hasEndWS = false;

    // ### not 100% correct for first-line
    const Font *f = htmlFont( false );
    int wordSpacing = style()->wordSpacing();
    int len = str->l;
    bool isSpace = false;
    bool firstWord = true;
    bool firstLine = true;
    for(int i = 0; i < len; i++)
    {
        unsigned short c = str->s[i].unicode();
        bool isNewline = false;

        // If line-breaks survive to here they are preserved
        if ( c == '\n' ) {
            if (style()->preserveLF()) {
                m_hasBreak = true;
                isNewline = true;
                isSpace = false;
            } else
                isSpace = true;
        } else
            isSpace = c == ' ';

        if ((isSpace || isNewline) && i == 0)
            m_hasBeginWS = true;
        if ((isSpace || isNewline) && i == len-1)
            m_hasEndWS = true;
        
        if (i && c == SOFT_HYPHEN)
            continue;

        int wordlen = 0;
        while( i+wordlen < len && (i+wordlen == 0 || str->s[i+wordlen].unicode() != SOFT_HYPHEN) &&
               !(isBreakable( str->s, i+wordlen, str->l )) ) {
            // check if we may use the simpler algorithm for estimating text width
            m_isSimpleText = (m_isSimpleText && isSimpleChar(str->s[i+wordlen].unicode()));
            wordlen++;
         }

        if (wordlen)
        {
            int w = f->width(str->s, str->l, i, wordlen, m_isSimpleText);
            currMinWidth += w;
            currMaxWidth += w;

            // Add in wordspacing to our maxwidth, but not if this is the last word.
            if (wordSpacing && !containsOnlyWhitespace(i+wordlen, len-(i+wordlen)))
                currMaxWidth += wordSpacing;

            if (firstWord) {
                firstWord = false;
                m_beginMinWidth = w;
            }
            m_endMinWidth = w;

            if(currMinWidth > m_minWidth) m_minWidth = currMinWidth;
            currMinWidth = 0;

            i += wordlen-1;
        }
        else {
            // Nowrap can never be broken, so don't bother setting the
            // breakable character boolean. Pre can only be broken if we encounter a newline.
            if (style()->autoWrap() || isNewline)
                m_hasBreakableChar = true;

            if(currMinWidth > m_minWidth) m_minWidth = currMinWidth;
            currMinWidth = 0;

            if (isNewline) // Only set if isPre was true and we saw a newline.
            {
                if ( firstLine ) {
                    firstLine = false;
                    if (!style()->autoWrap())
                        m_beginMinWidth = currMaxWidth;
                }

                if(currMaxWidth > m_maxWidth) m_maxWidth = currMaxWidth;
                currMaxWidth = 0;
            }
            else
            {
                currMaxWidth += f->charWidth( str->s, str->l, i + wordlen, m_isSimpleText);
            }
        }
    }

    if(currMinWidth > m_minWidth) m_minWidth = currMinWidth;
    if(currMaxWidth > m_maxWidth) m_maxWidth = currMaxWidth;

    if (!style()->autoWrap()) {
        m_minWidth = m_maxWidth;
        if (style()->preserveLF()) {
            if (firstLine)
                m_beginMinWidth = m_maxWidth;
            m_endMinWidth = currMaxWidth;
        }
    }

    setMinMaxKnown();
    //qDebug() << "Text::calcMinMaxWidth(): min = " << m_minWidth << " max = " << m_maxWidth;

}

int RenderText::minXPos() const
{
    if (!m_firstTextBox)
	return 0;
    int retval=6666666;
    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
        retval = qMin( retval, static_cast<int>(box->m_x));
    return retval;
}

int RenderText::inlineXPos() const
{
    return minXPos();
}

int RenderText::inlineYPos() const
{
    return m_firstTextBox ? m_firstTextBox->yPos() : 0;
}

const QFont &RenderText::font()
{
    return style()->font();
}

void RenderText::setText(DOMStringImpl *text, bool force)
{
    if (!force && str == text)
        return;

    setTextInternal(text);
}

void RenderText::setTextInternal(DOMStringImpl *text)
{
    DOMStringImpl *oldstr = str;
    if (text && style())
        str = text->collapseWhiteSpace(style()->preserveLF(), style()->preserveWS());
    else
        str = text;

    if (str)
        str->ref();
    if (oldstr)
        oldstr->deref();

    if (str && style()) {
        oldstr = str;
        switch (style()->textTransform()) {
            case CAPITALIZE: {
                RenderObject *o;
                bool runOnString = false;
                // find previous non-empty text renderer if one exists
                for (o = previousRenderer(); o; o = o->previousRenderer()) {
                    if (!o->isInlineFlow()) {
                        if (!o->isText())
                            break;
                        DOMStringImpl *prevStr = static_cast<RenderText*>(o)->string();
                        // !prevStr can happen with css like "content:open-quote;"
                        if (!prevStr)
                            break;
                        if (prevStr->length() == 0)
                            continue;
                        QChar c = (*prevStr)[prevStr->length() - 1];
                        if (!c.isSpace())
                            runOnString = true;
                        break;
                    }
                }
                str = str->capitalize(runOnString);
                break;
            }
            case UPPERCASE:
                str = str->upper();
                break;
            case LOWERCASE:
                str = str->lower();
                break;
            case TTNONE:
            default:
                break;
        }
        str->ref();
        oldstr->deref();
    }

    // ### what should happen if we change the text of a
    // RenderBR object ?
    KHTMLAssert(!isBR() || (str->l == 1 && (*str->s) == '\n'));
    KHTMLAssert(!str->l || str->s);

    if (parent()) setNeedsLayoutAndMinMaxRecalc();
#ifdef BIDI_DEBUG
    QString cstr = QString::fromRawData(str->s, str->l);
    qDebug() << "RenderText::setText( " << cstr.length() << " ) '" << cstr << "'";
#endif
}

int RenderText::height() const
{
    int retval = 0;
    if (firstTextBox())
#ifdef I_LOVE_UPDATING_TESTREGRESSIONS_BASELINES
        retval = lastTextBox()->m_y + lastTextBox()->height() - firstTextBox()->m_y;
#else
        retval = lastTextBox()->m_y + m_lineHeight - firstTextBox()->m_y;
    else
        retval = metrics( false ).height();
#endif
    return retval;
}

short RenderText::lineHeight( bool firstLine ) const
{
    if ( firstLine )
 	return RenderObject::lineHeight( firstLine );

    return m_lineHeight;
}

short RenderText::baselinePosition( bool firstLine ) const
{
    const QFontMetrics &fm = metrics( firstLine );
    return fm.ascent() +
        ( lineHeight( firstLine ) - fm.height() ) / 2;
}

InlineBox* RenderText::createInlineBox(bool, bool isRootLineBox)
{
    KHTMLAssert( !isRootLineBox );
    Q_UNUSED(isRootLineBox);
    InlineTextBox* textBox = new (renderArena()) InlineTextBox(this);
    if (!m_firstTextBox)
        m_firstTextBox = m_lastTextBox = textBox;
    else {
        m_lastTextBox->setNextLineBox(textBox);
        textBox->setPreviousLineBox(m_lastTextBox);
        m_lastTextBox = textBox;
    }
    return textBox;
}

void RenderText::position(InlineBox* box, int from, int len, bool reverse)
{
//qDebug() << "position: from="<<from<<" len="<<len;

    reverse = reverse && !style()->visuallyOrdered();

    KHTMLAssert(box->isInlineTextBox());
    InlineTextBox *s = static_cast<InlineTextBox *>(box);
    s->m_start = from;
    s->m_len = len;
    s->m_reversed = reverse;
}

unsigned int RenderText::width(unsigned int from, unsigned int len, bool firstLine) const
{
    if(!str->s || from > str->l ) return 0;
    if ( from + len > str->l ) len = str->l - from;

    const Font *f = htmlFont( firstLine );
    return width( from, len, f );
}

unsigned int RenderText::width(unsigned int from, unsigned int len, const Font *f) const
{
    if(!str->s || from > str->l ) return 0;
    if ( from + len > str->l ) len = str->l - from;

    if ( f == &style()->htmlFont() && from == 0 && len == str->l )
 	 return m_maxWidth;
    
    int w = f->width(str->s, str->l, from, len, m_isSimpleText );

    //qDebug() << "RenderText::width(" << from << ", " << len << ") = " << w;
    return w;
}

short RenderText::width() const
{
    int w;
    int minx = 100000000;
    int maxx = 0;
    // slooow
    for (InlineTextBox* s = firstTextBox(); s; s = s->nextTextBox()) {
        if(s->m_x < minx)
            minx = s->m_x;
        if(s->m_x + s->m_width > maxx)
            maxx = s->m_x + s->m_width;
    }

    w = qMax(0, maxx-minx);

    return w;
}

void RenderText::repaint(Priority p)
{
    RenderObject *cb = containingBlock();
    if(cb)
        cb->repaint(p);
}

QList< QRectF > RenderText::getClientRects()
{
    QList<QRectF> list;

    int x = 0;
    int y = 0;
    absolutePosition(x,y);

    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox())
    {
        QRectF textBoxRect(box->xPos() + x, box->yPos() + y,
                           box->width(), box->height());

        list.append(clientRectToViewport(textBoxRect));
    }
    return list;
}

bool RenderText::isFixedWidthFont() const
{
    return QFontInfo(style()->font()).fixedPitch();
}

short RenderText::verticalPositionHint( bool firstLine ) const
{
    return parent()->verticalPositionHint( firstLine );
}

const QFontMetrics &RenderText::metrics(bool firstLine) const
{
    if( firstLine && hasFirstLine() ) {
	RenderStyle *pseudoStyle  = style()->getPseudoStyle(RenderStyle::FIRST_LINE);
	if ( pseudoStyle )
	    return pseudoStyle->fontMetrics();
    }
    return style()->fontMetrics();
}

const Font *RenderText::htmlFont(bool firstLine) const
{
    const Font *f = 0;
    if( firstLine && hasFirstLine() ) {
	RenderStyle *pseudoStyle  = style()->getPseudoStyle(RenderStyle::FIRST_LINE);
	if ( pseudoStyle )
	    f = &pseudoStyle->htmlFont();
    } else {
	f = &style()->htmlFont();
    }
    return f;
}

bool RenderText::containsOnlyWhitespace(unsigned int from, unsigned int len) const
{
    unsigned int currPos;
    for (currPos = from;
         currPos < from+len && (str->s[currPos] == '\n' || str->s[currPos].direction() == QChar::DirWS);
         currPos++) {};
    return currPos >= (from+len);
}

void RenderText::trimmedMinMaxWidth(int& beginMinW, bool& beginWS,
                                    int& endMinW, bool& endWS,
                                    bool& hasBreakableChar, bool& hasBreak,
                                    int& beginMaxW, int& endMaxW,
                                    int& minW, int& maxW, bool& stripFrontSpaces)
{
    bool preserveWS = style()->preserveWS();
    bool preserveLF = style()->preserveLF();
    bool autoWrap = style()->autoWrap();
    if (preserveWS)
        stripFrontSpaces = false;

    int len = str->l;
    if (len == 0 || (stripFrontSpaces && str->containsOnlyWhitespace())) {
        maxW = 0;
        hasBreak = false;
        return;
    }

    minW = m_minWidth;
    maxW = m_maxWidth;
    beginWS = stripFrontSpaces ? false : m_hasBeginWS;
    endWS = m_hasEndWS;

    beginMinW = m_beginMinWidth;
    endMinW = m_endMinWidth;

    hasBreakableChar = m_hasBreakableChar;
    hasBreak = m_hasBreak;

    if (stripFrontSpaces && (str->s[0].direction() == QChar::DirWS || (!preserveLF && str->s[0] == '\n'))) {
        const Font *f = htmlFont( false );
        QChar space[1]; space[0] = ' ';
        int spaceWidth = f->charWidth(space, 1, 0, m_isSimpleText);
        maxW -= spaceWidth;
    }

    stripFrontSpaces = !preserveWS && m_hasEndWS;

    if (!autoWrap)
        minW = maxW;
    else if (minW > maxW)
        minW = maxW;

    // Compute our max widths by scanning the string for newlines.
    if (hasBreak) {
        const Font *f = htmlFont( false );
        bool firstLine = true;
        beginMaxW = endMaxW = maxW;
        for(int i = 0; i < len; i++)
        {
            int linelen = 0;
            while( i+linelen < len && str->s[i+linelen] != '\n')
                linelen++;

            if (linelen)
            {
                endMaxW = f->width(str->s, str->l, i, linelen, m_isSimpleText);
                if (firstLine) {
                    firstLine = false;
                    beginMaxW = endMaxW;
                }
                i += linelen;
            }
            else if (firstLine) {
                beginMaxW = 0;
                firstLine = false;
            }
	    if (i == len-1)
	        // A <pre> run that ends with a newline, as in, e.g.,
	        // <pre>Some text\n\n<span>More text</pre>
	        endMaxW = 0;
        }
    }
}
 
bool RenderText::isPointInsideSelection(int x, int y, const Selection &) const
{
    RenderText *rt = const_cast<RenderText *>(this);
    SelectionState selstate = selectionState();
    if (selstate == SelectionInside) return true;
    if (selstate == SelectionNone) return false;
    if (!firstTextBox()) return false;

    int absx, absy;
    if (!rt->absolutePosition(absx, absy)) return false;

    int startPos, endPos, offset;
    rt->selectionStartEnd(startPos, endPos);

    if (selstate == SelectionEnd) startPos = 0;
    if (selstate == SelectionStart) endPos = str->l;

    SelPointState sps;
    DOM::NodeImpl *node;
    /*FindSelectionResult res = */rt->checkSelectionPoint(x, y, absx, absy, node, offset, sps);

    return offset >= startPos && offset < endPos;
}
 
#ifdef ENABLE_DUMP

static QString quoteAndEscapeNonPrintables(const QString &s)
{
    QString result;
    result += '"';
    for (int i = 0; i != s.length(); ++i) {
        QChar c = s.at(i);
        if (c == '\\') {
            result += "\\\\";
        } else if (c == '"') {
            result += "\\\"";
        } else {
            ushort u = c.unicode();
            if (u >= 0x20 && u < 0x7F) {
                result += c;
            } else {
                QString hex;
                hex.sprintf("\\x{%X}", u);
                result += hex;
            }
        }
    }
    result += '"';
    return result;
}

static void writeTextRun(QTextStream &ts, const RenderText &o, const InlineTextBox &run)
{
    ts << "text run at (" << run.m_x << "," << run.m_y << ") width " << run.m_width << ": "
       << quoteAndEscapeNonPrintables(o.data().string().mid(run.m_start, run.m_len));
}

void RenderText::dump(QTextStream &stream, const QString &ind) const
{
    RenderObject::dump( stream, ind );

    for (InlineTextBox* box = firstTextBox(); box; box = box->nextTextBox()) {
        stream << endl << ind << "   ";
        writeTextRun(stream, *this, *box);
    }
}
#endif

RenderTextFragment::RenderTextFragment(DOM::NodeImpl* _node, DOM::DOMStringImpl* _str,
                                       int startOffset, int endOffset)
:RenderText(_node, _str->substring(startOffset, endOffset)),
m_start(startOffset), m_end(endOffset), m_generatedContentStr(0), m_firstLetter(0)
{}

RenderTextFragment::RenderTextFragment(DOM::NodeImpl* _node, DOM::DOMStringImpl* _str)
:RenderText(_node, _str), m_start(0), m_firstLetter(0)
{
    m_generatedContentStr = _str;
    if (_str) {
        _str->ref();
        m_end = _str->l;
    }
    else
        m_end = 0;
}

RenderTextFragment::~RenderTextFragment()
{
    if (m_generatedContentStr)
        m_generatedContentStr->deref();
}

void RenderTextFragment::detach()
{
    if (m_firstLetter)
        m_firstLetter->detach();
    
    RenderText::detach();
}

bool RenderTextFragment::isTextFragment() const
{
    return true;
}

DOM::DOMStringImpl* RenderTextFragment::originalString() const
{
    DOM::DOMStringImpl* result = 0;
    if (element())
        result = element()->string();
    else
        result = contentString();
    if (result && (start() > 0 || start() < result->l))
        result = result->substring(start(), end());
    return result;
}

void RenderTextFragment::setTextInternal(DOM::DOMStringImpl *text)
{
    if (m_firstLetter) {
        m_firstLetter->detach();
        m_firstLetter = 0;
    }
    RenderText::setTextInternal(text);
}

#undef BIDI_DEBUG
#undef DEBUG_LAYOUT
