/*
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
#ifndef HTML_LISTIMPL_H
#define HTML_LISTIMPL_H

/*
 * we ignore the deprecated compact attribute. Netscape does so too...
 */

#include "html_elementimpl.h"

namespace DOM
{

class HTMLUListElementImpl : public HTMLElementImpl
{
public:
    HTMLUListElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}

    Id id() const override;

    void parseAttribute(AttributeImpl *) override;

    virtual int start() const
    {
        return 1;
    }
};

// -------------------------------------------------------------------------

class HTMLDirectoryElementImpl : public HTMLElementImpl
{
public:
    HTMLDirectoryElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}

    Id id() const override;
};

// -------------------------------------------------------------------------

class HTMLMenuElementImpl : public HTMLElementImpl
{
public:
    HTMLMenuElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}
    virtual ~HTMLMenuElementImpl() {}

    Id id() const override;
};

// -------------------------------------------------------------------------

class HTMLOListElementImpl : public HTMLUListElementImpl
{
public:
    HTMLOListElementImpl(DocumentImpl *doc)
        : HTMLUListElementImpl(doc), _start(1) {}

    Id id() const override;
    void parseAttribute(AttributeImpl *) override;

    int start() const override
    {
        return _start;
    }
private:
    int _start;
};

// -------------------------------------------------------------------------

class HTMLLIElementImpl : public HTMLElementImpl
{
public:
    HTMLLIElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    Id id() const override;
    void parseAttribute(AttributeImpl *attr) override;
    void attach() override;
};

// -------------------------------------------------------------------------

class HTMLDListElementImpl : public HTMLElementImpl
{
public:
    HTMLDListElementImpl(DocumentImpl *doc) : HTMLElementImpl(doc) {}
    virtual ~HTMLDListElementImpl() {}

    Id id() const override;
};

} //namespace

#endif
