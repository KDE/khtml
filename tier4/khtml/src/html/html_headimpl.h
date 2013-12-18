/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2002-2003 Apple Computer, Inc.
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
#ifndef HTML_HEADIMPL_H
#define HTML_HEADIMPL_H

#include "html/html_elementimpl.h"
#include "misc/loader_client.h"
#include "css/css_stylesheetimpl.h"


namespace khtml {
    class CachedCSSStyleSheet;
    class CachedObject;
}


namespace DOM {

class DOMString;
class StyleSheetImpl;
class CSSStyleSheetImpl;

class HTMLBaseElementImpl : public HTMLElementImpl
{
public:
    HTMLBaseElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    DOMString href() const { return m_href; }
    DOMString target() const { return m_target; }

    virtual Id id() const;
    virtual void parseAttribute(AttributeImpl *attr);
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();

    void process();

protected:
    DOMString m_href;
    DOMString m_target;
};



// -------------------------------------------------------------------------

class HTMLLinkElementImpl : public khtml::CachedObjectClient, public HTMLElementImpl
{
public:
    HTMLLinkElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc), m_cachedSheet(0), m_sheet(0), m_isDisabled(false),
	m_loading(false), m_alternate(false), m_isCSSSheet(false) {}

    ~HTMLLinkElementImpl();

    virtual Id id() const;

    const StyleSheetImpl* sheet() const { return m_sheet; }
    StyleSheetImpl* sheet() { return m_sheet; }

    // overload from HTMLElementImpl
    virtual void parseAttribute(AttributeImpl *attr);

    void process();

    virtual void insertedIntoDocument();
    virtual void removedFromDocument();

    // from CachedObjectClient
    virtual void setStyleSheet(const DOM::DOMString &url, const DOM::DOMString &sheet, const DOM::DOMString &charset, const DOM::DOMString &mimetype);
    virtual void error(int err, const QString &text);
    bool isLoading() const;
    virtual bool checkAddPendingSheet();
    virtual bool checkRemovePendingSheet();

    bool isAlternate() const { return m_alternate; }
    bool isCSSStyleSheet() const { return m_isCSSSheet; }
    bool isDisabled() const { return m_isDisabled; }
    void setDisabled(bool disabled) { m_isDisabled = disabled; }

protected:
    void finished();

    khtml::CachedCSSStyleSheet *m_cachedSheet;
    CSSStyleSheetImpl *m_sheet;
    DOMString m_url;
    QString m_media;
    bool m_isDisabled : 1;
    bool m_loading    : 1;
    bool m_alternate  : 1;
    bool m_isCSSSheet : 1;
};

// -------------------------------------------------------------------------

class HTMLMetaElementImpl : public HTMLElementImpl
{
public:
    HTMLMetaElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    virtual Id id() const;
    virtual void parseAttribute(AttributeImpl *attr);
    virtual void insertedIntoDocument();

    void process();

protected:
    DOMString m_equiv;
    DOMString m_content;
};

// -------------------------------------------------------------------------

class HTMLScriptElementImpl : public HTMLElementImpl, public khtml::CachedObjectClient
{
public:
    HTMLScriptElementImpl(DocumentImpl *doc);
    ~HTMLScriptElementImpl();

    virtual void parseAttribute(AttributeImpl *attr);
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void notifyFinished(khtml::CachedObject *finishedObj);
    virtual void childrenChanged();

    virtual Id id() const;
    virtual bool isURLAttribute(AttributeImpl *attr) const;

    void setCreatedByParser(bool createdByParser) { m_createdByParser = createdByParser; }

    bool isValidScript() const;
    void evaluateScript(const QString &, const DOMString &);

    DOMString text() const;
    void setText( const DOMString& str );

    DOMString htmlFor() const;
    void setHtmlFor(const DOMString &);

    DOMString event() const;
    void setEvent(const DOMString &);

    DOMString charset() const;
    void setCharset(const DOMString &);

    bool defer() const;
    void setDefer(bool);

    DOMString src() const;
    void setSrc(const DOMString &);

    DOMString type() const;
    void setType(const DOMString &);

private:
    void loadFromUrl(const DOMString &url);
    khtml::CachedScript *m_cachedScript;
    bool m_createdByParser;
    bool m_evaluated;
    bool m_hasNonEmptyForAttribute;
};

// -------------------------------------------------------------------------

class HTMLStyleElementImpl : public HTMLElementImpl
{
public:
    HTMLStyleElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc), m_sheet(0), m_loading(false) {}
    ~HTMLStyleElementImpl();

    virtual Id id() const;

    CSSStyleSheetImpl *sheet() { return m_sheet; }

    // overload from HTMLElementImpl
    virtual void parseAttribute(AttributeImpl *attr);
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void childrenChanged();

    bool isLoading() const;
    virtual bool checkAddPendingSheet();
    virtual bool checkRemovePendingSheet();

protected:
    void parseText();
    
    CSSStyleSheetImpl *m_sheet;
    DOMString m_type;
    QString m_media;
    bool m_loading;
};

// -------------------------------------------------------------------------

class HTMLTitleElementImpl : public HTMLElementImpl
{
public:
    HTMLTitleElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc) {}

    DOMString text();
    void setText( const DOMString& str );

    virtual Id id() const;

    virtual void childrenChanged();

protected:
    DOMString m_title;
};

} //namespace

#endif
