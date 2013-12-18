/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2003 Apple Computer, Inc.
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

// -------------------------------------------------------------------------
#ifndef HTML_BLOCKIMPL_H
#define HTML_BLOCKIMPL_H

#include "html_elementimpl.h"
#include "dtd.h"

namespace DOM {

// -------------------------------------------------------------------------

class HTMLDivElementImpl : public HTMLGenericElementImpl
{
public:
    HTMLDivElementImpl(DocumentImpl *doc, ushort _tagid)
        : HTMLGenericElementImpl(doc, _tagid) {}

    virtual void parseAttribute(AttributeImpl *token);
};

// -------------------------------------------------------------------------

class HTMLHRElementImpl : public HTMLElementImpl
{
public:
    HTMLHRElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    virtual NodeImpl::Id id() const;
    virtual void parseAttribute(AttributeImpl *);
    virtual void attach();
};

// -------------------------------------------------------------------------

class HTMLPreElementImpl : public HTMLGenericElementImpl
{
public:
    HTMLPreElementImpl(DocumentImpl *doc, ushort _tagid)
        : HTMLGenericElementImpl(doc, _tagid) {}

    long width() const;
    void setWidth( long w );
};

// -------------------------------------------------------------------------

class HTMLMarqueeElementImpl : public HTMLElementImpl
{
public:
    HTMLMarqueeElementImpl(DocumentImpl *doc);

    virtual NodeImpl::Id id() const;
    virtual void parseAttribute(AttributeImpl *token);

    int minimumDelay() const { return m_minimumDelay; }

private:
    int m_minimumDelay;
};

// -------------------------------------------------------------------------

class HTMLLayerElementImpl : public HTMLDivElementImpl
{
public:
    HTMLLayerElementImpl( DocumentImpl *doc, ushort _tagid );

    virtual void parseAttribute(AttributeImpl *);
    virtual NodeImpl *addChild(NodeImpl *child);

    virtual void removedFromDocument();
    virtual void insertedIntoDocument();
    virtual void addId(const DOMString& id);
    virtual void removeId(const DOMString& id);
private:
    DOMString m_name;
    bool fixed;
    bool transparent;
};

} //namespace
#endif

