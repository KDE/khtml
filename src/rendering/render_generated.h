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

namespace DOM
{
class CounterImpl;
}

namespace khtml
{
class CounterNode;

// -----------------------------------------------------------------------------

class RenderCounterBase : public RenderText
{
public:
    RenderCounterBase(DOM::NodeImpl *node);

    const char *renderName() const override
    {
        return "RenderCounterBase";
    }

    void layout() override;
    void calcMinMaxWidth() override;
    bool isCounter() const override
    {
        return true;
    }

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
    RenderCounter(DOM::NodeImpl *node, const DOM::CounterImpl *counter);
    virtual ~RenderCounter() {}

    const char *renderName() const override
    {
        return "RenderCounter";
    }

    void generateContent() override;

protected:
    QString toListStyleType(int value, int total, EListStyleType type);

    const DOM::CounterImpl *m_counter;
};

// -----------------------------------------------------------------------------

class RenderQuote : public RenderCounterBase
{
public:
    RenderQuote(DOM::NodeImpl *node, EQuoteContent type);
    virtual ~RenderQuote() {}

    const char *renderName() const override
    {
        return "RenderQuote";
    }

    bool isQuote() const override
    {
        return true;
    }
    virtual int quoteCount() const;

    void generateContent() override;

protected:
    EQuoteContent m_quoteType;
};

// -----------------------------------------------------------------------------

// Is actually a special case of renderCounter for non-counted list-styles
// These have traditionally been drawn rather than use Unicode characters
class RenderGlyph : public RenderBox
{
public:
    RenderGlyph(DOM::NodeImpl *node, EListStyleType type);
    virtual ~RenderGlyph() {}

    const char *renderName() const override
    {
        return "RenderGlyph";
    }

    void paint(PaintInfo &paintInfo, int _tx, int _ty) override;
    void calcMinMaxWidth() override;

    void setStyle(RenderStyle *_style) override;

    short lineHeight(bool firstLine) const override;
    short baselinePosition(bool firstLine) const override;

    bool isGlyph() const override
    {
        return true;
    }

    void position(InlineBox *box, int /*from*/, int /*len*/, bool /*reverse*/) override
    {
        setPos(box->xPos(), box->yPos());
    }

protected:
    EListStyleType m_type;
};

} //namespace

#endif
