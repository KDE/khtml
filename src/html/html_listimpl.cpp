/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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
#include "html_listimpl.h"

using namespace DOM;

#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "rendering/render_list.h"
#include "xml/dom_docimpl.h"

using namespace khtml;

NodeImpl::Id HTMLUListElementImpl::id() const
{
    return ID_UL;
}

void HTMLUListElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_TYPE:
        addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, attr->value());
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

// -------------------------------------------------------------------------

NodeImpl::Id HTMLDirectoryElementImpl::id() const
{
    return ID_DIR;
}

// -------------------------------------------------------------------------

NodeImpl::Id HTMLMenuElementImpl::id() const
{
    return ID_MENU;
}

// -------------------------------------------------------------------------

NodeImpl::Id HTMLOListElementImpl::id() const
{
    return ID_OL;
}

void HTMLOListElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_TYPE:
        if ( strcmp( attr->value(), "a" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_LOWER_ALPHA);
        else if ( strcmp( attr->value(), "A" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_UPPER_ALPHA);
        else if ( strcmp( attr->value(), "i" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_LOWER_ROMAN);
        else if ( strcmp( attr->value(), "I" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_UPPER_ROMAN);
        else if ( strcmp( attr->value(), "1" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_DECIMAL);
        break;
    case ATTR_START:
        _start = attr->val() ? attr->val()->toInt() : 1;
        break;
    default:
        HTMLUListElementImpl::parseAttribute(attr);
    }
}

// -------------------------------------------------------------------------

NodeImpl::Id HTMLLIElementImpl::id() const
{
    return ID_LI;
}

void HTMLLIElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_VALUE:
        if(m_render && m_render->isListItem() && m_render->style()->display() == LIST_ITEM)
            static_cast<RenderListItem*>(m_render)->setValue(attr->value().toInt());
        break;
    case ATTR_TYPE:
        if ( strcmp( attr->value(), "a" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_LOWER_ALPHA);
        else if ( strcmp( attr->value(), "A" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_UPPER_ALPHA);
        else if ( strcmp( attr->value(), "i" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_LOWER_ROMAN);
        else if ( strcmp( attr->value(), "I" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_UPPER_ROMAN);
        else if ( strcmp( attr->value(), "1" ) == 0 )
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_DECIMAL);
        else
            addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, attr->value());
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLLIElementImpl::attach()
{
    assert(!attached());

    HTMLElementImpl::attach();

    if ( m_render && m_render->style()->display() == LIST_ITEM ) {
        RenderListItem* render = static_cast<RenderListItem*>( renderer() );
        NodeImpl* listNode = 0;
        NodeImpl* n = parentNode();
        while ( !listNode && n ) {
            switch( n->id() ) {
            case ID_UL:
            case ID_OL:
                listNode = n;
                break;
            }
            n = n->parentNode();
        }

        // if we are not in a list, then position us inside
        // can't use addCSSProperty cause its inherited attribute
        render->setInsideList( listNode );

        DOMString v = getAttribute(ATTR_VALUE);
        if ( !v.isEmpty() )
            render->setValue( v.implementation()->toInt() );
    }
}


// -------------------------------------------------------------------------


NodeImpl::Id HTMLDListElementImpl::id() const
{
    return ID_DL;
}

