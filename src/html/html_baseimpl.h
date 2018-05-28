/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2000-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2002 Apple Computer, Inc.
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

#ifndef HTML_BASEIMPL_H
#define HTML_BASEIMPL_H

#include "html/dtd.h"
#include "html/html_elementimpl.h"
#include "html/html_objectimpl.h"
#include "misc/khtmllayout.h"

class KHTMLPart;

namespace khtml
{
class RenderFrameSet;
class RenderFrame;
class RenderPartObject;
}

namespace DOM
{

class DOMString;
class CSSStyleSheetImpl;
class HTMLFrameElement;

// -------------------------------------------------------------------------

class HTMLBodyElementImpl : public HTMLElementImpl
{
public:
    HTMLBodyElementImpl(DocumentImpl *doc);
    ~HTMLBodyElementImpl();

    Id id() const override;

    DOMString aLink() const;
    void setALink(const DOMString &value);
    DOMString bgColor() const;
    void setBgColor(const DOMString &value);
    DOMString link() const;
    void setLink(const DOMString &value);
    DOMString text() const;
    void setText(const DOMString &value);
    DOMString vLink() const;
    void setVLink(const DOMString &value);

    void parseAttribute(AttributeImpl *) override;
    void attach() override;

    void insertedIntoDocument() override;
    void removedFromDocument() override;

    CSSStyleSheetImpl *sheet() const
    {
        return m_styleSheet;
    }

protected:
    CSSStyleSheetImpl *m_styleSheet;
    bool m_bgSet;
    bool m_fgSet;
};

// -------------------------------------------------------------------------

class HTMLFrameElementImpl : public HTMLPartContainerElementImpl
{
    friend class khtml::RenderFrame;
    friend class khtml::RenderPartObject;

public:
    HTMLFrameElementImpl(DocumentImpl *doc);

    ~HTMLFrameElementImpl();

    Id id() const override;

    void parseAttribute(AttributeImpl *) override;
    void attach() override;
    void defaultEventHandler(EventImpl *evt) override;

    bool noResize()
    {
        return noresize;
    }
    void setLocation(const QString &str);

    bool isFocusableImpl(FocusType ft) const override;
    void setFocus(bool) override;

    DocumentImpl *contentDocument() const;
    KHTMLPart    *contentPart() const;

    QString url;
    DOMString name; //Computed name for the frame map

    int marginWidth;
    int marginHeight;
    Qt::ScrollBarPolicy scrolling;

    bool frameBorder : 1;
    bool frameBorderSet : 1;
    bool noresize : 1;

    void ensureUniqueName();
    void computeContent() override;
    void setWidgetNotify(QWidget *widget) override;
};

// -------------------------------------------------------------------------

class HTMLFrameSetElementImpl : public HTMLElementImpl
{
    friend class khtml::RenderFrameSet;
public:
    HTMLFrameSetElementImpl(DocumentImpl *doc);

    ~HTMLFrameSetElementImpl();

    Id id() const override;

    void parseAttribute(AttributeImpl *) override;
    void attach() override;

    void defaultEventHandler(EventImpl *evt) override;

    bool frameBorder()
    {
        return frameborder;
    }
    bool noResize()
    {
        return noresize;
    }

    int totalRows() const
    {
        return m_totalRows;
    }
    int totalCols() const
    {
        return m_totalCols;
    }
    int border() const
    {
        return frameborder ? m_border : 0;
    }
    void detach() override;

    void recalcStyle(StyleChange ch) override;

protected:
    khtml::Length *m_rows;
    khtml::Length *m_cols;

    int m_totalRows;
    int m_totalCols;
    int m_border;

    bool frameborder : 1;
    bool frameBorderSet : 1;
    bool noresize : 1;
    bool m_resizing : 1;  // is the user resizing currently
};

// -------------------------------------------------------------------------

class HTMLHeadElementImpl : public HTMLElementImpl
{
public:
    HTMLHeadElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    Id id() const override;
};

// -------------------------------------------------------------------------

class HTMLHtmlElementImpl : public HTMLElementImpl
{
public:
    HTMLHtmlElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    Id id() const override;
};

// -------------------------------------------------------------------------

class HTMLIFrameElementImpl : public HTMLFrameElementImpl
{
public:
    HTMLIFrameElementImpl(DocumentImpl *doc);

    ~HTMLIFrameElementImpl();

    Id id() const override;

    void parseAttribute(AttributeImpl *attr) override;
    void attach() override;

    void computeContent() override;
    void setWidgetNotify(QWidget *widget) override;

    void insertedIntoDocument() override;
    void removedFromDocument() override;
protected:

    void updateFrame();
    bool m_frame;
};

} //namespace

#endif

