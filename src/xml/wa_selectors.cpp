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

#include "wa_selectors.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_nodelistimpl.h"
#include <wtf/RefPtr.h>

#include "css/cssparser.h"
#include "css/cssstyleselector.h"

using namespace DOM;

namespace khtml {

namespace SelectorQuery {

static WTF::PassRefPtr<DOM::NodeListImpl>  querySelectorImp(bool justOne, DOM::NodeImpl* root, const DOM::DOMString& query, int& ec)
{
    // Parse the query.
    CSSParser p;
    QList<CSSSelector*> selectors = p.parseSelectorList(root->document(), query);

    if (selectors.isEmpty()) {
        ec = DOMException::SYNTAX_ERR;
        return 0;
    }

    khtml::CSSStyleSelector* styleSelector = root->document()->styleSelector();

    // ### not in the spec.
    if (!styleSelector) {
        ec = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    // Check for matches. We specialize some paths for common selectors.
    DOM::StaticNodeListImpl* matches = new DOM::StaticNodeListImpl;

    bool requiresClass = true;
    bool requiresId    = true;
    for (int i = 0; i < selectors.size(); ++i) {
        if (selectors[i]->match != CSSSelector::Class)
            requiresClass = false;
        if (selectors[i]->match != CSSSelector::Id)
            requiresId    = false;
    }

    for (DOM::NodeImpl* cur = root; cur; cur = cur->traverseNextNode(root)) {
        if (requiresClass && !cur->hasClass())
            continue;

        if (requiresId && !cur->hasID())
            continue;            

        DOM::ElementImpl* e = 0;
        if (cur->isElementNode())
            e = static_cast<DOM::ElementImpl*>(cur);
    
        if (e && styleSelector->isMatchedByAnySelector(e, selectors)) {
            matches->append(e);
            if (justOne)
                break;
        }
    }

    // Cleanup the selectors.
    qDeleteAll(selectors);

    // all done
    return matches;
}

WTF::PassRefPtr<DOM::ElementImpl> querySelector(DOM::NodeImpl* root, const DOM::DOMString& query, int& ec)
{
    WTF::RefPtr<DOM::NodeListImpl> nl = querySelectorImp(true, root, query, ec);

    if (nl && nl->length()) {
        return static_cast<DOM::ElementImpl*>(nl->item(0));
    }

    return 0;
}

WTF::PassRefPtr<DOM::NodeListImpl> querySelectorAll(DOM::NodeImpl* root, const DOM::DOMString& query, int& ec)
{
    return querySelectorImp(false, root, query, ec);
}

} // namespace SelectorQuery
    
} // namespace khtml


// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
