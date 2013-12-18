/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright  (C) 2010 Maksim Orlovich (maksim@kde.org)
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
 */
#ifndef khtml_WA_Selectors_H
#define khtml_WA_Selectors_H

#include "dom/dom_string.h"
#include <wtf/PassRefPtr.h>

namespace DOM {
    class NodeImpl;
    class ElementImpl;
    class NodeListImpl;
}

namespace khtml {

/**
 Implementation of Web Applications Selectors API (level 1)
*/
namespace SelectorQuery {
    WTF::PassRefPtr<DOM::ElementImpl>  querySelector(DOM::NodeImpl* root, const DOM::DOMString& query, int& ec);
    WTF::PassRefPtr<DOM::NodeListImpl> querySelectorAll(DOM::NodeImpl* root, const DOM::DOMString& query, int& ec);
} // namespace SelectorQuery
 
} // namespace khtml

#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
