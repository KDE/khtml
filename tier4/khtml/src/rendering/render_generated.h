/*
 * This file is part of the HTML rendering engine for KDE.
 *
 * Copyright (C) 2004,2005 Allan Sandfeld Jensen (kde@carewolf.com)
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
#ifndef RENDER_GENERATED_H
#define RENDER_GENERATED_H

#include "rendering/render_text.h"
#include "rendering/render_box.h"

namespace DOM {
    class CounterImpl;
}

namespace khtml {
    class CounterNode;

// -----------------------------------------------------------------------------

class RenderCounterBase : public RenderText
{
public:
    RenderCounterBase(DOM::NodeImpl* node);

    virtual const char *renderName() const { return "RenderCounterBase"; }

    virtual void layout( );
    virtual void calcMinMaxWidth();
    virtual bool isCounter() const { return true; }

    virtual void generateContent() = 0;
    void updateContent();

protected:
    QString m_item;
    CounterNode *m_counterNode; // Cache of the counternode
};

// -----------------------------------------------------------------------------

class RenderCounter : public RenderCounterBase
{
public:
    RenderCounter(DOM::NodeImpl* node, const DOM::CounterImpl* counter);
    virtual ~RenderCounter() {}

    virtual const char *renderName() const { return "RenderCounter"; }

    virtual void generateContent();

protected:
    QString toListStyleType(int value, int total, EListStyleType type);

    const DOM::CounterImpl* m_counter;
};

// -----------------------------------------------------------------------------

class RenderQuote : public RenderCounterBase
{
public:
    RenderQuote(DOM::NodeImpl* node, EQuoteContent type);
    virtual ~RenderQuote() {}

    virtual const char *renderName() const { return "RenderQuote"; }

    virtual bool isQuote() const { return true; }
    virtual int quoteCount() const;

    virtual void generateContent();

protected:
    EQuoteContent m_quoteType;
};

// -----------------------------------------------------------------------------

// Is actually a special case of renderCounter for non-counted list-styles
// These have traditionally been drawn rather than use Unicode characters
class RenderGlyph : public RenderBox
{
public:
    RenderGlyph(DOM::NodeImpl* node, EListStyleType type);
    virtual ~RenderGlyph() {}

    virtual const char *renderName() const { return "RenderGlyph"; }

    virtual void paint(PaintInfo& paintInfo, int _tx, int _ty);
    virtual void calcMinMaxWidth();

    virtual void setStyle(RenderStyle *_style);

    virtual short lineHeight( bool firstLine ) const;
    virtual short baselinePosition( bool firstLine ) const;

    virtual bool isGlyph() const { return true; }

    virtual void position(InlineBox* box, int /*from*/, int /*len*/, bool /*reverse*/) {
        setPos( box->xPos(), box->yPos() );
    }

protected:
    EListStyleType m_type;
};

} //namespace

#endif
