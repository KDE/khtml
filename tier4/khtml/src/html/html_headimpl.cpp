/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2003, 2004, 2005, 2006, 2007 Apple Computer, Inc.
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
// -------------------------------------------------------------------------

#include "html/html_headimpl.h"
#include "html/html_documentimpl.h"
#include "xml/dom_textimpl.h"
#include "xml/dom2_eventsimpl.h"

#include "khtmlview.h"
#include "khtml_part.h"

#include "misc/loader.h"
#include "misc/helper.h"

#include "css/cssstyleselector.h"
#include "css/css_stylesheetimpl.h"
#include "css/csshelper.h"
#include "css/css_mediaquery.h"

#include "ecma/kjs_proxy.h"

#include <QUrl>
#include <QDebug>

using namespace khtml;
using namespace DOM;

NodeImpl::Id HTMLBaseElementImpl::id() const
{
    return ID_BASE;
}

void HTMLBaseElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_HREF:
	m_href = khtml::parseURL(attr->value());
	process();
	break;
    case ATTR_TARGET:
	m_target = attr->value();
	process();
	break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLBaseElementImpl::insertedIntoDocument()
{
    HTMLElementImpl::insertedIntoDocument();
    process();
}

void HTMLBaseElementImpl::removedFromDocument()
{
    HTMLElementImpl::removedFromDocument();

    // Since the document doesn't have a base element...
    // (This will break in the case of multiple base elements, but that's not valid anyway (?))
    document()->setBaseURL( QUrl() );
    document()->setBaseTarget( QString() );
}

void HTMLBaseElementImpl::process()
{
    if (!inDocument())
	return;

    if(!m_href.isEmpty() && document()->part())
	document()->setBaseURL( QUrl(document()->part()->url()).resolved(QUrl(m_href.string())) );

    if(!m_target.isEmpty())
	document()->setBaseTarget( m_target.string() );

    // ### should changing a document's base URL dynamically automatically update all images, stylesheets etc?
}

// -------------------------------------------------------------------------


HTMLLinkElementImpl::~HTMLLinkElementImpl()
{
    if(m_sheet) m_sheet->deref();
    if(m_cachedSheet) m_cachedSheet->deref(this);
}

NodeImpl::Id HTMLLinkElementImpl::id() const
{
    return ID_LINK;
}

void HTMLLinkElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch (attr->id())
    {
    case ATTR_HREF:
        m_url = document()->completeURL( khtml::parseURL(attr->value()).string() );
	process();
        break;
    case ATTR_REL:
    case ATTR_TYPE:
	process();
        break;
    case ATTR_TITLE:
        // ### when title changes we have to reconsider our alternative
        // stylesheet choice
        if (m_sheet)
            m_sheet->setTitle(attr->value());
        break;
    case ATTR_MEDIA:
        m_media = attr->value().string().toLower();
        process();
        break;
    case ATTR_DISABLED: {
        bool m_oldisDisabled = m_isDisabled;
        m_isDisabled = attr->val();
        if (m_oldisDisabled != m_isDisabled) {
            if (isLoading()) {
                if (m_oldisDisabled)
                    document()->addPendingSheet();
                else if (!m_alternate)
                    document()->styleSheetLoaded();
            }
            if (m_oldisDisabled) {
                // enabling: if it's an alternate sheet, pretend it's not.
                m_alternate = false;
            } else if (!m_alternate) {
                // disabling: recheck alternate status
                QString rel =  getAttribute(ATTR_REL).string().toLower();
                QString type = getAttribute(ATTR_TYPE).string().toLower();
                m_alternate = (type.contains("text/css") || rel.contains("stylesheet")) && rel.contains("alternate");
            }
            if (isLoading())
                break;
            if (!m_sheet && !m_isDisabled) {
                process();
                if (isLoading() && m_alternate)
                    document()->addPendingSheet();
                m_alternate = false;
            } else
                document()->updateStyleSelector(); // Update the style selector.
        }
        break;
    }
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLLinkElementImpl::process()
{
    if (!inDocument())
        return;

    QString type = getAttribute(ATTR_TYPE).string().toLower();
    QString rel = getAttribute(ATTR_REL).string().toLower();

    KHTMLPart* part = document()->part();

    // IE extension: location of small icon for locationbar / bookmarks
    // Uses both "shortcut icon" and "icon"
    if ( part && rel.contains("icon") && !m_url.isEmpty() && !part->parentPart())
        part->browserExtension()->setIconUrl( QUrl(m_url.string()) );

    // Stylesheet
    else if (!m_isDisabled && rel.contains("stylesheet")) {
        // no need to load style sheets which aren't for the screen output
        // ### there may be in some situations e.g. for an editor or script to manipulate
        khtml::MediaQueryEvaluator allEval(true);
        khtml::MediaQueryEvaluator screenEval("screen", true);
        khtml::MediaQueryEvaluator printEval("print", true);
        MediaListImpl* media = new MediaListImpl((CSSStyleSheetImpl*)0, m_media, true);
        media->ref();
        if (allEval.eval(media) || screenEval.eval(media) || printEval.eval(media)) {
            // Add ourselves as a pending sheet, but only if we aren't an alternate
            // stylesheet.  Alternate stylesheets don't hold up render tree construction.
            m_alternate = rel.contains("alternate");
            if (!isAlternate())
                document()->addPendingSheet();

            QString chset = getAttribute( ATTR_CHARSET ).string();
            // set chset to charset of referring document when attribute CHARSET is absent.
            // http://www.w3.org/TR/CSS21/syndata.html(4.4)
            if (chset.isEmpty() && part) chset = part->encoding();
            if (m_cachedSheet) {
                if (m_loading)
                    document()->styleSheetLoaded();
		m_cachedSheet->deref(this);
            }
            m_loading = true;
            m_cachedSheet = document()->docLoader()->requestStyleSheet(m_url, chset);
            if (m_cachedSheet) {
                m_isCSSSheet = true;
		m_cachedSheet->ref(this);
            }
            else if (!isAlternate()) {
                // Error requesting sheet; decrement pending sheet count
                m_loading = false;
                document()->styleSheetLoaded();
            }
        }
        media->deref();
    }
    else if (m_sheet) {
	// we no longer contain a stylesheet, e.g. perhaps rel or type was changed
	m_sheet->deref();
	m_sheet = 0;
        m_isCSSSheet = false;
	document()->updateStyleSelector();
    }
}

void HTMLLinkElementImpl::insertedIntoDocument()
{
    HTMLElementImpl::insertedIntoDocument();
    process();
}

void HTMLLinkElementImpl::removedFromDocument()
{
    HTMLElementImpl::removedFromDocument();
    document()->updateStyleSelector();
}

void HTMLLinkElementImpl::setStyleSheet(const DOM::DOMString &url, const DOM::DOMString &sheetStr, const DOM::DOMString &charset, const DOM::DOMString &mimetype)
{
    if (m_sheet)
        m_sheet->deref();
    bool strict = !document()->inCompatMode();
    DOMString sheet = sheetStr;
    if (strict && !khtml::isAcceptableCSSMimetype(mimetype))
         sheet = "";
    m_sheet = new CSSStyleSheetImpl(this, url);
    m_sheet->ref();
    m_sheet->setCharset(charset);
    m_sheet->parseString( sheet, strict );
    m_sheet->setTitle(getAttribute(ATTR_TITLE));

    MediaListImpl *media = new MediaListImpl( (CSSStyleSheetImpl*)0, m_media );
    m_sheet->setMedia( media );

    finished();
}

void HTMLLinkElementImpl::finished()
{
    m_loading = false;

    // Tell the doc about the sheet.
    if (!isLoading() && !isDisabled() && !isAlternate())
        document()->styleSheetLoaded();

    // ### major inefficiency, but currently necessary for proper
    // alternate styles support. don't recalc the styleselector
    // when nothing actually changed!
    if ( isAlternate() && m_sheet && !isDisabled())
        document()->updateStyleSelector();
}

void HTMLLinkElementImpl::error( int, const QString& )
{
    finished();
}

bool HTMLLinkElementImpl::isLoading() const
{
//    qDebug() << "link: checking if loading!";
    if(m_loading) return true;
    if(!m_sheet) return false;
    //if(!m_sheet->isCSSStyleSheet()) return false;
    return static_cast<CSSStyleSheetImpl *>(m_sheet)->isLoading();
}

bool HTMLLinkElementImpl::checkAddPendingSheet()
{
    if (!isLoading() && !isDisabled() && !isAlternate()) {
        document()->addPendingSheet();
        return true;
    }
    return false;
}

bool HTMLLinkElementImpl::checkRemovePendingSheet()
{
    if (!isLoading() && !isDisabled() && !isAlternate()) {
        document()->styleSheetLoaded();
	return true;
    }
    return false;
}

// -------------------------------------------------------------------------

NodeImpl::Id HTMLMetaElementImpl::id() const
{
    return ID_META;
}

void HTMLMetaElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_HTTP_EQUIV:
	m_equiv = attr->value();
	process();
	break;
    case ATTR_CONTENT:
	m_content = attr->value();
	process();
	break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLMetaElementImpl::insertedIntoDocument()
{
    HTMLElementImpl::insertedIntoDocument();
    process();
}

void HTMLMetaElementImpl::process()
{
    // Get the document to process the tag, but only if we're actually part of DOM tree (changing a meta tag while
    // it's not in the tree shouldn't have any effect on the document)
    if (inDocument() && !m_equiv.isNull() && !m_content.isNull())
	document()->processHttpEquiv(m_equiv,m_content);
}

// -------------------------------------------------------------------------

HTMLScriptElementImpl::HTMLScriptElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc), m_cachedScript(0), m_createdByParser(false), m_evaluated(false), m_hasNonEmptyForAttribute(false)
{
}

HTMLScriptElementImpl::~HTMLScriptElementImpl()
{
    if (m_cachedScript)
        m_cachedScript->deref(this);
}

NodeImpl::Id HTMLScriptElementImpl::id() const
{
    return ID_SCRIPT;
}

void HTMLScriptElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_ONLOAD:
        setHTMLEventListener(EventImpl::LOAD_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onload", this));
        break;
    case ATTR_SRC: {
        // We want to evaluate scripts on src attr change when a fresh script element
        // is inserted into document, and then has its source changed -after-.
        // If the source is manipulated while we're outside the document,
        // we'll only start doing things once we get insertedIntoDocument()
        if (m_evaluated || m_cachedScript || m_createdByParser || !inDocument())
            return;
        DOMString url = attr->value();
        if (!url.isEmpty())
            loadFromUrl(url);
        break;
    }
    case ATTR_FOR: {
        m_hasNonEmptyForAttribute = !attr->value().isEmpty();
        break;
    }
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

bool HTMLScriptElementImpl::isURLAttribute(AttributeImpl *attr) const
{
    return attr->id() == ATTR_SRC;
}

bool HTMLScriptElementImpl::isValidScript() const
{
    // HTML5 draft 4.3.1 : script elements with non-empty for attribute
    // must not be executed.
    if (m_hasNonEmptyForAttribute)
        return false;

    // Check type before language, since language is deprecated
    /*
        Mozilla 1.5 doesn't accept the text/javascript1.x formats, but WinIE 6 does.
        Mozilla 1.5 doesn't accept text/jscript, text/ecmascript, and text/livescript, but WinIE 6 does.
        Mozilla 1.5 accepts application/x-javascript, WinIE 6 doesn't.
        Mozilla 1.5 allows leading and trailing whitespace, but WinIE 6 doesn't.
        Mozilla 1.5 and WinIE 6 both accept the empty string, but neither accept a whitespace-only string.
        We want to accept all the values that either of these browsers accept, but not other values.
    */
    QString type = getAttribute(ATTR_TYPE).string().toLower();

    // Gecko accepts initial/trailing whitespace around the mimetype.
    // Whitespace only, however, musn't trigger execution.
    int length = type.length();
    type = type.trimmed();
    if (length)
       return !(type.compare("text/javascript") != 0 &&
                type.compare("text/javascript1.0") != 0 &&
                type.compare("text/javascript1.1") != 0 &&
                type.compare("text/javascript1.2") != 0 &&
                type.compare("text/javascript1.3") != 0 &&
                type.compare("text/javascript1.4") != 0 &&
                type.compare("text/javascript1.5") != 0 &&
                type.compare("text/jscript") != 0 &&
                type.compare("text/ecmascript") != 0 &&
                type.compare("text/livescript") != 0 &&
                type.compare("application/x-javascript") != 0 &&
                type.compare("application/x-ecmascript") != 0 &&
                type.compare("application/javascript") != 0 &&
                type.compare("application/ecmascript") != 0 );

    /*
        Mozilla 1.5 doesn't accept jscript or ecmascript, but WinIE 6 does.
        Mozilla 1.5 accepts javascript1.0, javascript1.4, and javascript1.5, but WinIE 6 accepts only 1.1 - 1.3.
        Neither Mozilla 1.5 nor WinIE 6 accept leading or trailing whitespace.
        We want to accept all the values that either of these browsers accept, but not other values.
    */
    QString lang = getAttribute(ATTR_LANGUAGE).string().toLower();
    if (!lang.isEmpty())
       return !(lang.compare("javascript") != 0 &&
                lang.compare("javascript1.0") != 0 &&
                lang.compare("javascript1.1") != 0 &&
                lang.compare("javascript1.2") != 0 &&
                lang.compare("javascript1.3") != 0 &&
                lang.compare("javascript1.4") != 0 &&
                lang.compare("javascript1.5") != 0 &&
                lang.compare("ecmascript") != 0 &&
                lang.compare("livescript") != 0 &&
                lang.compare("jscript") );

    return true;        
}

void HTMLScriptElementImpl::childrenChanged()
{
    // If a node is inserted as a child of the script element
    // and the script element has been inserted in the document
    // we evaluate the script.
    if (!m_createdByParser && inDocument() && firstChild())
        evaluateScript(document()->URL().url(), text());
}

void HTMLScriptElementImpl::loadFromUrl(const DOMString &url)
{
    QString charset = getAttribute(ATTR_CHARSET).string();
    m_cachedScript = document()->docLoader()->requestScript(url, charset);
    if (m_cachedScript)
        m_cachedScript->ref(this);    
}

void HTMLScriptElementImpl::insertedIntoDocument()
{
    HTMLElementImpl::insertedIntoDocument();

    assert(!m_cachedScript);

    if (m_createdByParser)
        return;

    DOMString url = getAttribute(ATTR_SRC).string();
    if (!url.isEmpty()) {
        loadFromUrl(url);
        return;
    }

    // If there's an empty script node, we shouldn't evaluate the script
    // because if a script is inserted afterwards (by setting text or innerText)
    // it should be evaluated, and evaluateScript only evaluates a script once.
    DOMString scriptString = text();
    if (!scriptString.isEmpty())
        evaluateScript(document()->URL().url(), scriptString);
}

void HTMLScriptElementImpl::removedFromDocument()
{
    HTMLElementImpl::removedFromDocument();

    if (m_cachedScript) {
        m_cachedScript->deref(this);
        m_cachedScript = 0;
    }
}

void HTMLScriptElementImpl::notifyFinished(CachedObject* o)
{
    CachedScript *cs = static_cast<CachedScript *>(o);

    assert(cs == m_cachedScript);

    QString   URL    = cs->url().string();
    DOMString script = cs->script();
    cs->deref(this);
    m_cachedScript = 0;

    ref(); // Pin so we don't destroy oursleves.
    if (!cs->hadError()) {
        evaluateScript(URL, script);
        dispatchHTMLEvent(EventImpl::LOAD_EVENT, false, false);
    }
    deref();
}

void HTMLScriptElementImpl::evaluateScript(const QString &URL, const DOMString &script)
{
    if (m_evaluated || !isValidScript())
        return;

    KHTMLPart *part = document()->part();
    if (part) {
        KJSProxy *proxy = KJSProxy::proxy(part);
        if (proxy) {
            m_evaluated = true;
            proxy->evaluate(URL, 0, script.string(), 0);
            DocumentImpl::updateDocumentsRendering();
        }
    }
}

DOMString HTMLScriptElementImpl::text() const
{
    DOMString val = "";

    for (NodeImpl *n = firstChild(); n; n = n->nextSibling()) {
        if (n->isTextNode())
            val += static_cast<TextImpl *>(n)->data();
    }

    return val;
}

void HTMLScriptElementImpl::setText(const DOMString &value)
{
    int exceptioncode = 0;
    int numChildren = childNodeCount();

    if (numChildren == 1 && firstChild()->isTextNode()) {
        static_cast<DOM::TextImpl *>(firstChild())->setData(value, exceptioncode);
        return;
    }

    if (numChildren > 0) {
        removeChildren();
    }

    appendChild(document()->createTextNode(value.implementation()), exceptioncode);
}

DOMString HTMLScriptElementImpl::htmlFor() const
{
    // DOM Level 1 says: reserved for future use.
    return DOMString();
}

void HTMLScriptElementImpl::setHtmlFor(const DOMString &/*value*/)
{
    // DOM Level 1 says: reserved for future use.
}

DOMString HTMLScriptElementImpl::event() const
{
    // DOM Level 1 says: reserved for future use.
    return DOMString();
}

void HTMLScriptElementImpl::setEvent(const DOMString &/*value*/)
{
    // DOM Level 1 says: reserved for future use.
}

DOMString HTMLScriptElementImpl::charset() const
{
    return getAttribute(ATTR_CHARSET);
}

void HTMLScriptElementImpl::setCharset(const DOMString &value)
{
    setAttribute(ATTR_CHARSET, value);
}

bool HTMLScriptElementImpl::defer() const
{
    return !getAttribute(ATTR_DEFER).isNull();
}

void HTMLScriptElementImpl::setDefer(bool defer)
{
    setAttribute(ATTR_DEFER, defer ? "" : 0);
}

DOMString HTMLScriptElementImpl::src() const
{
    return document()->completeURL(getAttribute(ATTR_SRC).string());
}

void HTMLScriptElementImpl::setSrc(const DOMString &value)
{
    setAttribute(ATTR_SRC, value);
}

DOMString HTMLScriptElementImpl::type() const
{
    return getAttribute(ATTR_TYPE);
}

void HTMLScriptElementImpl::setType(const DOMString &value)
{
    setAttribute(ATTR_TYPE, value);
}

// -------------------------------------------------------------------------

HTMLStyleElementImpl::~HTMLStyleElementImpl()
{
    if(m_sheet) m_sheet->deref();
}

NodeImpl::Id HTMLStyleElementImpl::id() const
{
    return ID_STYLE;
}

// other stuff...
void HTMLStyleElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch (attr->id())
    {
    case ATTR_TYPE:
        m_type = attr->value().lower();
        break;
    case ATTR_MEDIA:
        m_media = attr->value().string().toLower();
        break;
    case ATTR_TITLE:
        if (m_sheet)
            m_sheet->setTitle(attr->value());
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLStyleElementImpl::insertedIntoDocument()
{
    HTMLElementImpl::insertedIntoDocument();
    
    // If we're empty, we have to call parseText here, since we won't get childrenChanged();
    // but we still want a CSSOM object
    if (!firstChild())
        parseText();
    
    if (m_sheet)
        document()->updateStyleSelector();
}

void HTMLStyleElementImpl::removedFromDocument()
{
    HTMLElementImpl::removedFromDocument();
    if (m_sheet)
        document()->updateStyleSelector();
}

void HTMLStyleElementImpl::childrenChanged()
{
    HTMLElementImpl::childrenChanged();

    parseText();
}

void HTMLStyleElementImpl::parseText()
{
    DOMString text = "";

    for (NodeImpl *c = firstChild(); c != 0; c = c->nextSibling()) {
	if ((c->nodeType() == Node::TEXT_NODE) ||
	    (c->nodeType() == Node::CDATA_SECTION_NODE) ||
	    (c->nodeType() == Node::COMMENT_NODE))
	    text += c->nodeValue();
    }

    if (m_sheet) {
        m_sheet->deref();
        m_sheet = 0;
    }

    m_loading = false;
    if (m_type.isEmpty() || m_type == "text/css") // Type must be empty or CSS
    {
        MediaListImpl* media = new MediaListImpl((CSSStyleSheetImpl*)0, m_media, true);
        media->ref();
        khtml::MediaQueryEvaluator screenEval("screen", true);
        khtml::MediaQueryEvaluator printEval("print", true);
        if (screenEval.eval(media) || printEval.eval(media)) {
            document()->addPendingSheet();
            m_loading = true;
            m_sheet = new CSSStyleSheetImpl(this);
            m_sheet->ref();
            m_sheet->parseString( text, !document()->inCompatMode() );
            m_sheet->setMedia( media );
            m_sheet->setTitle( getAttribute(ATTR_TITLE) );
            m_loading = false;
        }
        media->deref();
    }

    if (!isLoading() && m_sheet)
        document()->styleSheetLoaded();
}

bool HTMLStyleElementImpl::isLoading() const
{
    if (m_loading) return true;
    if(!m_sheet) return false;
    return static_cast<CSSStyleSheetImpl *>(m_sheet)->isLoading();
}

bool HTMLStyleElementImpl::checkRemovePendingSheet()
{
    if (!isLoading()) {
        document()->styleSheetLoaded();
        return true;
    }
    return false;
}

bool HTMLStyleElementImpl::checkAddPendingSheet()
{
    if (!isLoading()) {
        document()->addPendingSheet();
        return true;
    }
    return false;
}

// -------------------------------------------------------------------------

NodeImpl::Id HTMLTitleElementImpl::id() const
{
    return ID_TITLE;
}

void HTMLTitleElementImpl::childrenChanged()
{
    HTMLElementImpl::childrenChanged();

    m_title = "";
    for (NodeImpl *c = firstChild(); c != 0; c = c->nextSibling()) {
	if ((c->nodeType() == Node::TEXT_NODE) || (c->nodeType() == Node::CDATA_SECTION_NODE))
	    m_title += c->nodeValue();
    }
    if ( !m_title.isEmpty() && inDocument())
        document()->setTitle(m_title);
}

DOMString HTMLTitleElementImpl::text()
{
    if (firstChild() && firstChild()->nodeType() == Node::TEXT_NODE) {
        return firstChild()->nodeValue();
    }
    return "";
}

void HTMLTitleElementImpl::setText( const DOMString& str )
{
    int exceptioncode = 0;
    // Look for an existing text child node
    DOM::NodeListImpl* nl(childNodes());
    if (nl)
    {
        for (unsigned int i = 0; i < nl->length(); i++) {
            if (nl->item(i)->nodeType() == DOM::Node::TEXT_NODE) {
                static_cast<DOM::TextImpl *>(nl->item(i))->setData(str, exceptioncode);
                return;
            }
        }
        delete nl;
    }
    // No child text node found, creating one
    DOM::TextImpl* t = document()->createTextNode(str.implementation());
    appendChild(t, exceptioncode);
}
