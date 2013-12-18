/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
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
#ifndef HTML_INLINEIMPL_H
#define HTML_INLINEIMPL_H

#include "html_elementimpl.h"

namespace DOM {


class HTMLAnchorElementImpl : public HTMLElementImpl
{
public:
    HTMLAnchorElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc), m_hasTarget(false) {}

    virtual bool isFocusableImpl(FocusType ft) const;
    virtual Id id() const;
    virtual void parseAttribute(AttributeImpl *attr);
    virtual void defaultEventHandler(EventImpl *evt);
    void click();
protected:
    bool m_hasTarget : 1;
};

// -------------------------------------------------------------------------

class HTMLBRElementImpl : public HTMLElementImpl
{
public:
    HTMLBRElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    virtual Id id() const;
    virtual void parseAttribute(AttributeImpl *attr);
    virtual void attach();
};

// -------------------------------------------------------------------------

class HTMLWBRElementImpl : public HTMLElementImpl
{
public:
    HTMLWBRElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    virtual Id id() const;
    virtual void attach();
};

// -------------------------------------------------------------------------

class HTMLFontElementImpl : public HTMLElementImpl
{
public:
    HTMLFontElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    virtual Id id() const;
    virtual void parseAttribute(AttributeImpl *attr);
};

} //namespace

#endif
