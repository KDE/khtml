/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 2001 Peter Kelly (pmk@post.com)
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

#ifndef _DOM_Views_h_
#define _DOM_Views_h_

#include <khtml_export.h>
namespace DOM {

class Document;
class AbstractViewImpl;
class CSSStyleDeclaration;
class Element;
class DOMString;

/**
 * Introduced in DOM Level 2
 *
 * A base interface that all views shall derive from.
 *
 */
class KHTML_EXPORT AbstractView {
    friend class Event;
    friend class UIEvent;
    friend class MouseEvent;
    friend class MutationEvent;
    friend class Document;
public:
    AbstractView();
    AbstractView(const AbstractView &other);
    virtual ~AbstractView();

    AbstractView & operator = (const AbstractView &other);

    /**
     * The source DocumentView of which this is an AbstractView.
     */
    Document document() const;

    /**
     * Introduced in DOM Level 2
     * This method is from the ViewCSS interface
     *
     * This method is used to get the computed style as it is defined in
     * [CSS2].
     *
     * @param elt The element whose style is to be computed. This parameter
     * cannot be null.
     *
     * @param pseudoElt The pseudo-element or null if none.
     *
     * @return The computed style. The CSSStyleDeclaration is read-only and
     * contains only absolute values.
     */
    CSSStyleDeclaration getComputedStyle(const Element &elt, const DOMString &pseudoElt);

    /**
     * @internal
     * not part of the DOM
     */
    AbstractViewImpl *handle() const;
    bool isNull() const;

protected:
    AbstractView(AbstractViewImpl *i);
    AbstractViewImpl *impl;
};


} //namespace
#endif
