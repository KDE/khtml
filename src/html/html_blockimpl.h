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

namespace DOM
{

// -------------------------------------------------------------------------

class HTMLDivElementImpl : public HTMLGenericElementImpl
{
public:
    HTMLDivElementImpl(DocumentImpl *doc, ushort _tagid)
        : HTMLGenericElementImpl(doc, _tagid) {}

    void parseAttribute(AttributeImpl *token) override;
};

// -------------------------------------------------------------------------

class HTMLHRElementImpl : public HTMLElementImpl
{
public:
    HTMLHRElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    NodeImpl::Id id() const override;
    void parseAttribute(AttributeImpl *) override;
    void attach() override;
};

// -------------------------------------------------------------------------

class HTMLPreElementImpl : public HTMLGenericElementImpl
{
public:
    HTMLPreElementImpl(DocumentImpl *doc, ushort _tagid)
        : HTMLGenericElementImpl(doc, _tagid) {}

    long width() const;
    void setWidth(long w);
};

// -------------------------------------------------------------------------

class HTMLMarqueeElementImpl : public HTMLElementImpl
{
public:
    HTMLMarqueeElementImpl(DocumentImpl *doc);

    NodeImpl::Id id() const override;
    void parseAttribute(AttributeImpl *token) override;

    int minimumDelay() const
    {
        return m_minimumDelay;
    }

private:
    int m_minimumDelay;
};

// -------------------------------------------------------------------------

class HTMLLayerElementImpl : public HTMLDivElementImpl
{
public:
    HTMLLayerElementImpl(DocumentImpl *doc, ushort _tagid);

    void parseAttribute(AttributeImpl *) override;
    NodeImpl *addChild(NodeImpl *child) override;

    void removedFromDocument() override;
    void insertedIntoDocument() override;
    void addId(const DOMString &id) override;
    void removeId(const DOMString &id) override;
private:
    DOMString m_name;
    bool fixed;
    bool transparent;
};

} //namespace
#endif

