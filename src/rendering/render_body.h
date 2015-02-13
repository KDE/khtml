/*
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2000-2003 Lars Knoll (knoll@kde.org)
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
#ifndef RENDER_BODY
#define RENDER_BODY

#include "rendering/render_block.h"

namespace DOM
{
class HTMLBodyElementImpl;
}

namespace khtml
{

class RenderBody : public RenderBlock
{
public:
    RenderBody(DOM::HTMLBodyElementImpl *node);
    virtual ~RenderBody();

    bool isBody() const Q_DECL_OVERRIDE
    {
        return true;
    }

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderBody";
    }
    void repaint(Priority p = NormalPriority) Q_DECL_OVERRIDE;

    void layout() Q_DECL_OVERRIDE;
    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;

    int availableHeight() const Q_DECL_OVERRIDE;

protected:
    void paintBoxDecorations(PaintInfo &, int _tx, int _ty) Q_DECL_OVERRIDE;
    bool scrollbarsStyled;
};

} // end namespace
#endif
