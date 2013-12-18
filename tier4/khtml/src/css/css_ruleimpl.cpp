/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright 2002-2008 Apple Computer, Inc.
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

#include "css_ruleimpl.h"
#include "css_stylesheetimpl.h"
#include "css_valueimpl.h"
#include "cssparser.h"

#include <dom/css_rule.h>
#include <dom/css_stylesheet.h>
#include <dom/dom_exception.h>
#include <dom/dom_string.h>

#include <xml/dom_docimpl.h>

using namespace DOM;

CSSStyleSheetImpl *CSSRuleImpl::parentStyleSheet() const
{
    return ( m_parent && m_parent->isCSSStyleSheet() )  ?
	static_cast<CSSStyleSheetImpl *>(m_parent) : 0;
}

CSSRuleImpl *CSSRuleImpl::parentRule() const
{
    return ( m_parent && m_parent->isRule() )  ?
	static_cast<CSSRuleImpl *>(m_parent) : 0;
}

DOM::DOMString CSSRuleImpl::cssText() const
{
    // ###
    return DOMString();
}

void CSSRuleImpl::setCssText(DOM::DOMString /*str*/)
{
    // ###
}

// ---------------------------------------------------------------------------

CSSFontFaceRuleImpl::CSSFontFaceRuleImpl(StyleBaseImpl *parent)
    : CSSRuleImpl(parent)
{
    m_type = CSSRule::FONT_FACE_RULE;
    m_style = 0;
}

CSSFontFaceRuleImpl::~CSSFontFaceRuleImpl()
{
    if (m_style)
        m_style->deref();
}

void CSSFontFaceRuleImpl::setDeclaration( CSSStyleDeclarationImpl* decl)
{
    assert(!m_style);
    m_style = decl;
    if (m_style)
        m_style->ref();
}

DOMString CSSFontFaceRuleImpl::cssText() const
{
    DOMString result("@font-face");

    result += " { ";
    result += m_style->cssText();
    result += "}";

    return result;
}

// --------------------------------------------------------------------------

CSSImportRuleImpl::CSSImportRuleImpl( StyleBaseImpl *parent,
                                      const DOM::DOMString &href,
                                      MediaListImpl *media )
    : CSSRuleImpl(parent)
{
    m_type = CSSRule::IMPORT_RULE;

    m_lstMedia = media;
    if ( !m_lstMedia )
	m_lstMedia = new MediaListImpl( this, DOMString() );
    m_lstMedia->setParent( this );
    m_lstMedia->ref();

    m_strHref = href;
    m_styleSheet = 0;

    m_cachedSheet = 0;

    init();
}
CSSImportRuleImpl::CSSImportRuleImpl( StyleBaseImpl *parent,
                                      const DOM::DOMString &href,
                                      const DOM::DOMString &media )
    : CSSRuleImpl(parent)
{
    m_type = CSSRule::IMPORT_RULE;

    m_lstMedia = new MediaListImpl( this, media );
    m_lstMedia->ref();

    m_strHref = href;
    m_styleSheet = 0;

    m_cachedSheet = 0;

    init();
}

CSSImportRuleImpl::~CSSImportRuleImpl()
{
    if( m_lstMedia ) {
 	m_lstMedia->setParent( 0 );
	m_lstMedia->deref();
    }
    if(m_styleSheet) {
        m_styleSheet->setParent(0);
        m_styleSheet->deref();
    }

    if(m_cachedSheet) m_cachedSheet->deref(this);
}

void CSSImportRuleImpl::checkLoaded() const
{
    if (isLoading())
        return;
    CSSRuleImpl::checkLoaded();
}

void CSSImportRuleImpl::setStyleSheet(const DOM::DOMString &url, const DOM::DOMString &sheetStr, const DOM::DOMString &charset, const DOM::DOMString &mimetype)
{
    if ( m_styleSheet ) {
        m_styleSheet->setParent(0);
        m_styleSheet->deref();
    }
    m_styleSheet = new CSSStyleSheetImpl(this, url);
    m_styleSheet->setCharset(charset);
    m_styleSheet->ref();

    CSSStyleSheetImpl *parent = parentStyleSheet();
    bool strict = parent ? parent->useStrictParsing() : true;
    DOMString sheet = sheetStr;
    if (strict && !khtml::isAcceptableCSSMimetype(mimetype))
        sheet = "";
    m_styleSheet->parseString( sheet, strict );
    m_loading = false;
    m_done = true;

    checkLoaded();
}

void CSSImportRuleImpl::error(int /*err*/, const QString &/*text*/)
{
    if ( m_styleSheet ) {
        m_styleSheet->setParent(0);
        m_styleSheet->deref();
    }
    m_styleSheet = 0;

    m_loading = false;
    m_done = true;

    checkLoaded();
}

bool CSSImportRuleImpl::isLoading() const
{
    return ( m_loading || (m_styleSheet && m_styleSheet->isLoading()) );
}

void CSSImportRuleImpl::init()
{
    m_loading = 0;
    m_done = false;
    khtml::DocLoader *docLoader = 0;
    StyleBaseImpl *root = this;
    StyleBaseImpl *parent;
    while ( ( parent = root->parent()) )
	root = parent;
    if (root->isCSSStyleSheet())
	docLoader = static_cast<CSSStyleSheetImpl*>(root)->docLoader();

    DOMString absHref = m_strHref;
    CSSStyleSheetImpl *parentSheet = parentStyleSheet();
    if (!parentSheet->href().isNull()) {
      // use parent styleheet's URL as the base URL
      absHref = QUrl(parentSheet->href().string()).resolved(QUrl(m_strHref.string())).toString();
    }
/*
    else {
      // use documents's URL as the base URL
      DocumentImpl *doc = static_cast<CSSStyleSheetImpl*>(root)->doc();
      absHref = QUrl(doc->URL()).resolved(QUrl(m_strHref.string())).toString();
    }
*/
    // Check for a cycle in our import chain.  If we encounter a stylesheet
    // in our parent chain with the same URL, then just bail.
    for ( parent = static_cast<StyleBaseImpl*>( this )->parent();
         parent;
          parent = parent->parent() )
        if ( absHref == parent->baseURL().url() )
            return;

    m_cachedSheet = docLoader->requestStyleSheet(absHref, parentStyleSheet()->charset().string());

    if (m_cachedSheet)
    {
      // if the import rule is issued dynamically, the sheet may have already been
      // removed from the pending sheet count, so let the doc know
      // the sheet being imported is pending.
      checkPending();

      m_loading = true;
      m_cachedSheet->ref(this);
    }
}

DOMString CSSImportRuleImpl::cssText() const
{
    DOMString result = "@import url(\"";
    result += m_strHref;
    result += "\")";

    if (m_lstMedia) {
        result += " ";
        result += m_lstMedia->mediaText();
    }
    result += ";";

    return result;
}

// --------------------------------------------------------------------------
CSSMediaRuleImpl::CSSMediaRuleImpl( StyleBaseImpl *parent, MediaListImpl *mediaList, CSSRuleListImpl *ruleList )
    :   CSSRuleImpl( parent )
{
    m_type = CSSRule::MEDIA_RULE;
    m_lstMedia = mediaList;
    if (m_lstMedia)
        m_lstMedia->ref();
    m_lstCSSRules = ruleList;
    m_lstCSSRules->ref();
}

CSSMediaRuleImpl::CSSMediaRuleImpl(StyleBaseImpl *parent)
    :   CSSRuleImpl( parent )
{
    m_type = CSSRule::MEDIA_RULE;
    m_lstMedia = 0;
    m_lstCSSRules = new CSSRuleListImpl();
    m_lstCSSRules->ref();
}

CSSMediaRuleImpl::CSSMediaRuleImpl( StyleBaseImpl *parent, const DOM::DOMString &media )
:   CSSRuleImpl( parent )
{
    m_type = CSSRule::MEDIA_RULE;
    m_lstMedia = new MediaListImpl( this, media );
    m_lstMedia->ref();
    m_lstCSSRules = new CSSRuleListImpl();
    m_lstCSSRules->ref();
}

CSSMediaRuleImpl::~CSSMediaRuleImpl()
{
    if( m_lstMedia ) {
	m_lstMedia->setParent( 0 );
        m_lstMedia->deref();
    }
    for ( unsigned int i = 0; i < m_lstCSSRules->length(); ++i )
        m_lstCSSRules->item(  i )->setParent(  0 );
    m_lstCSSRules->deref();
}

unsigned long CSSMediaRuleImpl::append( CSSRuleImpl *rule )
{
    return rule ? m_lstCSSRules->insertRule( rule, m_lstCSSRules->length() ) : 0;
}

unsigned long CSSMediaRuleImpl::insertRule( const DOMString &rule,
                                            unsigned long index )
{
    CSSParser p( strictParsing );
    CSSRuleImpl *newRule = p.parseRule( parentStyleSheet(), rule );

    return newRule ? m_lstCSSRules->insertRule( newRule, index ) : 0;
}

DOM::DOMString CSSMediaRuleImpl::cssText() const
{
    DOMString result("@media ");
    if (m_lstMedia) {
        result += m_lstMedia->mediaText();
        result += " ";
    }
    result += "{ \n";

    if (m_lstCSSRules) {
        unsigned len = m_lstCSSRules->length();
        for (unsigned i = 0; i < len; i++) {
            result += "  ";
            result += m_lstCSSRules->item(i)->cssText();
            result += "\n";
        }
    }

    result += "}";
    return result;
}

// ---------------------------------------------------------------------------

CSSPageRuleImpl::CSSPageRuleImpl(StyleBaseImpl *parent)
    : CSSRuleImpl(parent)
{
    m_type = CSSRule::PAGE_RULE;
    m_style = 0;
}

CSSPageRuleImpl::~CSSPageRuleImpl()
{
    if(m_style) m_style->deref();
}

DOM::DOMString CSSPageRuleImpl::selectorText() const
{
    // ###
    return DOMString();
}

void CSSPageRuleImpl::setSelectorText(DOM::DOMString /*str*/)
{
    // ###
}

// --------------------------------------------------------------------------

CSSStyleRuleImpl::CSSStyleRuleImpl(StyleBaseImpl *parent)
    : CSSRuleImpl(parent)
{
    m_type = CSSRule::STYLE_RULE;
    m_style = 0;
    m_selector = 0;
}

CSSStyleRuleImpl::~CSSStyleRuleImpl()
{
    if(m_style) {
	m_style->setParent( 0 );
	m_style->deref();
    }
    qDeleteAll(*m_selector);
    delete m_selector;
}

DOMString CSSStyleRuleImpl::cssText() const
{
    DOMString result(selectorText());

    result += " { ";
    result += m_style->cssText();
    result += "}";

    return result;
}

DOM::DOMString CSSStyleRuleImpl::selectorText() const
{
    if (m_selector) {
        DOMString str;
        foreach (CSSSelector *s, *m_selector) {
            if (s != m_selector->at(0))
                str += ", ";
            str += s->selectorText();
        }
        return str;
    }
    return DOMString();
}

void CSSStyleRuleImpl::setSelectorText(DOM::DOMString /*str*/)
{
    // ###
}

bool CSSStyleRuleImpl::parseString( const DOMString &/*string*/, bool )
{
    // ###
    return false;
}

void CSSStyleRuleImpl::setDeclaration( CSSStyleDeclarationImpl *style)
{
    if ( m_style != style ) {
        if(m_style) m_style->deref();
        m_style = style;
        if(m_style) m_style->ref();
    }
}

// --------------------------------------------------------------------

CSSNamespaceRuleImpl::CSSNamespaceRuleImpl(StyleBaseImpl *parent, const DOMString& prefix, const DOMString& ns)
    : CSSRuleImpl(parent)
{
    m_type      = CSSRule::NAMESPACE_RULE;
    m_prefix    = prefix;
    m_namespace = ns;
}

// --------------------------------------------------------------------

CSSRuleListImpl::CSSRuleListImpl(StyleListImpl* const list, bool omitCharsetRules)
{
     m_list = list;
     if (list && omitCharsetRules) {
         m_list = 0;
         unsigned len = list->length();
         for (unsigned i = 0; i < len; ++i) {
             StyleBaseImpl* rule = list->item(i);
             if (rule->isRule() && !rule->isCharsetRule())
                 append(static_cast<CSSRuleImpl*>(rule));
         }
     } else if (m_list) {
         m_list->ref();
     }
}

CSSRuleListImpl::~CSSRuleListImpl()
{
    CSSRuleImpl* rule;
    while ( !m_lstCSSRules.isEmpty() && ( rule = m_lstCSSRules.takeFirst() ) )
        rule->deref();
    if (m_list)
        m_list->deref();
}

unsigned long CSSRuleListImpl::length() const
{
    return m_list ? m_list->length() : m_lstCSSRules.count();
}

CSSRuleImpl* CSSRuleListImpl::item(unsigned long index)
{
    if (m_list) {
        StyleBaseImpl* rule = m_list->item(index);
        assert(!rule || rule->isRule());
        return static_cast<CSSRuleImpl*>(rule);
    }
    return index < length() ? m_lstCSSRules.at(index) : 0;
}

void CSSRuleListImpl::deleteRule ( unsigned long index )
{
    assert(!m_list);
    if (index+1 > (unsigned) m_lstCSSRules.size()) {
        return;
        // ### Throw INDEX_SIZE_ERR exception here (TODO)
    }
    CSSRuleImpl *rule = m_lstCSSRules.takeAt( index );
    rule->deref();
}

void CSSRuleListImpl::append(CSSRuleImpl* rule)
{
    assert(!m_list);
    rule->ref();
    m_lstCSSRules.append( rule );
}

unsigned long CSSRuleListImpl::insertRule( CSSRuleImpl *rule,
                                           unsigned long index )
{
    assert(!m_list);
    if (index > (unsigned) m_lstCSSRules.size()) {
        return 0;
        // ### Throw INDEX_SIZE_ERR exception here (TODO)
    }
    
    if( rule )
    {
        m_lstCSSRules.insert( index, rule );
        rule->ref();
        return index;
    }

    return 0;
}

