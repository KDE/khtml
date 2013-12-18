/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright 2004 Apple Computer, Inc.
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
#ifndef _CSS_css_stylesheetimpl_h_
#define _CSS_css_stylesheetimpl_h_

#include "dom/dom_string.h"
#include "css/css_base.h"
#include "misc/loader_client.h"
#include "xml/dom_docimpl.h"
#include <QPointer>

namespace khtml {
    class CachedCSSStyleSheet;
    class DocLoader;
    class MediaQuery;
}

namespace DOM {

class StyleSheet;
class CSSStyleSheet;
class CSSParser;
class MediaListImpl;
class CSSRuleImpl;
class CSSNamespaceRuleImpl;
class CSSRuleListImpl;
class NodeImpl;
class DocumentImpl;

class StyleSheetImpl : public StyleListImpl
{
public:
    StyleSheetImpl(DOM::NodeImpl *ownerNode, DOM::DOMString href = DOMString());
    StyleSheetImpl(StyleSheetImpl *parentSheet, DOM::DOMString href = DOMString());
    StyleSheetImpl(StyleBaseImpl *owner, DOM::DOMString href  = DOMString());
    StyleSheetImpl(khtml::CachedCSSStyleSheet *cached, DOM::DOMString href  = DOMString());
    virtual ~StyleSheetImpl();

    virtual bool isStyleSheet() const { return true; }

    virtual DOM::DOMString type() const { return DOMString(); }

    bool disabled() const { return m_disabled; }
    void setDisabled( bool disabled );
    DOM::NodeImpl *ownerNode() const { return m_parentNode; }
    StyleSheetImpl *parentStyleSheet() const;
    DOM::DOMString href() const { return m_strHref; }
    void setHref(const DOM::DOMString& href) { m_strHref = href; }
    DOM::DOMString title() const { return m_strTitle; }
    MediaListImpl *media() const { return m_media; }
    void setMedia( MediaListImpl *media );
    void setTitle( const DOM::DOMString& title ) { m_strTitle = title; }

protected:
    DOM::NodeImpl *m_parentNode;
    DOM::DOMString m_strHref;
    DOM::DOMString m_strTitle;
    MediaListImpl *m_media;
    bool m_disabled;
};

class CSSStyleSheetImpl : public StyleSheetImpl
{
public:
    CSSStyleSheetImpl(DOM::NodeImpl *parentNode, DOM::DOMString href = DOMString(), bool _implicit = false);
    CSSStyleSheetImpl(CSSStyleSheetImpl *parentSheet, DOM::DOMString href = DOMString());
    CSSStyleSheetImpl(CSSRuleImpl *ownerRule, DOM::DOMString href = DOMString());
    // clone from a cached version of the sheet
    CSSStyleSheetImpl(DOM::NodeImpl *parentNode, CSSStyleSheetImpl *orig);
    CSSStyleSheetImpl(CSSRuleImpl *ownerRule, CSSStyleSheetImpl *orig);

    ~CSSStyleSheetImpl() { delete m_namespaces; }

    virtual bool isCSSStyleSheet() const { return true; }

    virtual DOM::DOMString type() const { return "text/css"; }

    CSSRuleImpl *ownerRule() const;
    CSSRuleListImpl *cssRules(bool omitCharsetRule = false);
    unsigned long insertRule ( const DOM::DOMString &rule, unsigned long index, int &exceptioncode );
    void deleteRule ( unsigned long index, int &exceptioncode );

    void determineNamespace(NamespaceName& namespacename, const DOM::DOMString& prefix);
    quint32 defaultNamespace() { return m_defaultNamespace.id(); }
    void appendNamespaceRule(CSSNamespaceRuleImpl* ns);

    void setCharset(const DOMString &charset) { m_charset = charset; }
    const DOMString& charset() const { return m_charset; }

    virtual bool parseString( const DOMString &string, bool strict = true );

    bool isLoading() const;

    virtual void checkLoaded() const;
    virtual void checkPending() const;
    bool loadedHint() const { return m_loadedHint; }

    // ### remove? (clients should use sheet->doc()->docLoader())
    khtml::DocLoader *docLoader() const
    { return m_doc ? m_doc->docLoader() : 0; }

    DocumentImpl *doc() const { return m_doc; }
    bool implicit() const { return m_implicit; }
protected:
    void recomputeNamespaceInfo(); // updates m_defaultNamespace and m_namespaces
                                   // we update m_namespaces lazily, but 
                                   // m_defaulNamespace eagerly.
    void dirtyNamespaceInfo() { delete m_namespaces; m_namespaces = 0; }

    DocumentImpl *m_doc;
    bool m_implicit;
    mutable bool m_loadedHint;
    NamespaceName m_defaultNamespace;
    QList<CSSNamespaceRuleImpl*>* m_namespaces;
    DOMString m_charset;
};

// ----------------------------------------------------------------------------

class StyleSheetListImpl : public khtml::Shared<StyleSheetListImpl>
{
public:
    // the manager argument should be passed only when this is 
    // document.styleSheets.
    StyleSheetListImpl(DocumentImpl* manager = 0): managerDocument(manager) {}
    ~StyleSheetListImpl();

    // the following two ignore implicit stylesheets
    unsigned long length() const;
    StyleSheetImpl *item ( unsigned long index );

    void add(StyleSheetImpl* s);
    void remove(StyleSheetImpl* s);

    QList<StyleSheetImpl*> styleSheets;
    
    // we need the document pointer to make it update the stylesheet list
    // if needed for the global list. Luckily, we don't care about that if the 
    // document dies, so we use QPointer to break the cycle
    QPointer<DocumentImpl> managerDocument;
};

// ----------------------------------------------------------------------------

class MediaListImpl : public StyleBaseImpl
{
public:
    MediaListImpl(bool fallbackToDescription = false)
	: StyleBaseImpl( 0 ), m_fallback(fallbackToDescription) {}
    MediaListImpl( CSSStyleSheetImpl *parentSheet, bool fallbackToDescription = false)
        : StyleBaseImpl(parentSheet), m_fallback(fallbackToDescription) {}
    MediaListImpl( CSSStyleSheetImpl *parentSheet,
                   const DOM::DOMString &media, bool fallbackToDescription = false);
    MediaListImpl( CSSRuleImpl *parentRule, const DOM::DOMString &media, bool fallbackToDescription = false);
    ~MediaListImpl();

    virtual bool isMediaList() const { return true; }

    CSSStyleSheetImpl *parentStyleSheet() const;
    CSSRuleImpl *parentRule() const;
    unsigned long length() const { return  m_queries.size(); }
    DOM::DOMString item ( unsigned long index ) const;
    void deleteMedium ( const DOM::DOMString &oldMedium, int& ec);
    void appendMedium ( const DOM::DOMString &newMedium, int& ec);

    DOM::DOMString mediaText() const;
    void setMediaText(const DOM::DOMString &value, int& ec);

    void appendMediaQuery(khtml::MediaQuery* mediaQuery);
    const QList<khtml::MediaQuery*>* mediaQueries() const { return &m_queries; }

protected:
    QList<khtml::MediaQuery*> m_queries;
    bool m_fallback; // true if failed media query parsing should fallback to media description parsing
};


} // namespace

#endif

