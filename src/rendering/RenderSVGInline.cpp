/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 *           (C) 2006 Apple Computer Inc.
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

#include "wtf/Platform.h"

#if ENABLE(SVG)
#include "RenderSVGInline.h"

#include "SVGInlineFlowBox.h"
#include "xml/Document.h"

namespace WebCore {
    
RenderSVGInline::RenderSVGInline(DOM::NodeImpl* n)
    : RenderInline(n)
{
}

InlineBox* RenderSVGInline::createInlineBox(bool makePlaceHolderBox, bool isRootLineBox, bool isOnlyRun)
{
    Q_UNUSED(isOnlyRun);
    ASSERT(!(!isRootLineBox && (isReplaced() || makePlaceHolderBox)));
    Q_UNUSED(makePlaceHolderBox);
    Q_UNUSED(isRootLineBox);
    ASSERT(isInlineFlow());

    InlineFlowBox* flowBox = new (renderArena()) SVGInlineFlowBox(this);

    if (!m_firstLineBox)
        m_firstLineBox = m_lastLineBox = flowBox;
    else {
        m_lastLineBox->setNextLineBox(flowBox);
        flowBox->setPreviousLineBox(m_lastLineBox);
        m_lastLineBox = flowBox;
    }
        
    return flowBox;
}

}

#endif // ENABLE(SVG)
