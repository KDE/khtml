/**
 * This file is part of the DOM implementation for KDE.
 *
 * (C) 2001 Peter Kelly (pmk@post.com)
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

#include "dom2_viewsimpl.h"
#include "dom_elementimpl.h"
#include "dom_docimpl.h"
#include "css/css_renderstyledeclarationimpl.h"
#include "css/cssproperties.h"
#include "css/css_stylesheetimpl.h"

using namespace khtml;
using namespace DOM;

AbstractViewImpl::AbstractViewImpl(DocumentImpl *_document)
{
    m_document = _document;
}

AbstractViewImpl::~AbstractViewImpl()
{
}

CSSStyleDeclarationImpl *AbstractViewImpl::getComputedStyle(ElementImpl* elt, DOMStringImpl* /*pseudoElt*/)
{
    if (!elt)
        return 0;

    CSSStyleDeclarationImpl* style = new RenderStyleDeclarationImpl( elt );
    return style;
}

