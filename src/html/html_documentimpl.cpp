/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2010 Maksim Orlovich (maksim@kde.org)
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

#include "html/html_documentimpl.h"
#include "html/html_imageimpl.h"
#include "html/html_headimpl.h"
#include "html/html_baseimpl.h"
#include "html/htmltokenizer.h"
#include "html/html_miscimpl.h"
#include "html/html_formimpl.h"

#include "khtmlview.h"
#include "khtml_part.h"
#include "khtmlpart_p.h"
#include "khtml_settings.h"

#include "xml/xml_tokenizer.h"
#include "xml/dom2_eventsimpl.h"

#include <khtml_global.h>
#include "rendering/render_object.h"
#include "dom/dom_exception.h"

#include <QDebug>
#include <QDBusConnection>
#include <kcookiejar_interface.h>

#include "css/cssproperties.h"
#include "css/cssstyleselector.h"
#include "css/css_stylesheetimpl.h"
#include <stdlib.h>

using namespace DOM;
using namespace khtml;


HTMLDocumentImpl::HTMLDocumentImpl(KHTMLView *v)
  : DocumentImpl(v)
{
//    qDebug() << "HTMLDocumentImpl constructor this = " << this;
    htmlElement = 0;

    m_doAutoFill = false;
    m_determineParseMode = false;

/* dynamic history stuff to be fixed later (pfeiffer)
    connect( KHTMLGlobal::vLinks(), SIGNAL(removed(QString)),
             SLOT(slotHistoryChanged()));
*/
    connect( KHTMLGlobal::vLinks(), SIGNAL(inserted(QString)),
             SLOT(slotHistoryChanged()));
    connect( KHTMLGlobal::vLinks(), SIGNAL(cleared()),
             SLOT(slotHistoryChanged()));
}

HTMLDocumentImpl::~HTMLDocumentImpl()
{
}

DOMString HTMLDocumentImpl::referrer() const
{
    if ( part() )
        return part()->pageReferrer();
    return DOMString();
}

DOMString HTMLDocumentImpl::lastModified() const
{
    if ( part() )
        return part()->lastModified();
    return DOMString();
}

DOMString HTMLDocumentImpl::cookie() const
{
    WId windowId = 0;
    KHTMLView *v = view ();

    if ( v && v->topLevelWidget() )
      windowId = v->topLevelWidget()->winId();

    org::kde::KCookieServer kcookiejar("org.kde.kded5", "/modules/kcookiejar", QDBusConnection::sessionBus());
    QDBusReply<QString> reply = kcookiejar.findDOMCookies(URL().url(), qlonglong(windowId));

    if ( !reply.isValid() )
    {
       qWarning() << "Can't communicate with cookiejar!";
       return DOMString();
    }

    return DOMString(reply.value());
}

void HTMLDocumentImpl::setCookie( const DOMString & value )
{
    WId windowId = 0;
    KHTMLView *v = view ();

    if ( v && v->topLevelWidget() )
      windowId = v->topLevelWidget()->winId();

    QByteArray fake_header("Set-Cookie: ");
    fake_header.append(value.string().toLatin1().constData());
    fake_header.append("\n");
    // Note that kded modules are autoloaded so we don't need to call loadModule ourselves.
    org::kde::KCookieServer kcookiejar("org.kde.kded5", "/modules/kcookiejar", QDBusConnection::sessionBus());
    // Can't use kcookiejar.addCookies because then we can't pass NoBlock...
    kcookiejar.call(QDBus::NoBlock, "addCookies",
                     URL().url(), fake_header, qlonglong(windowId));
}

void HTMLDocumentImpl::setBody(HTMLElementImpl *_body, int& exceptioncode)
{
    HTMLElementImpl *b = body();
    if ( !_body ) {
        exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
        return;
    }
    if ( !b )
        documentElement()->appendChild( _body, exceptioncode );
    else
        documentElement()->replaceChild( _body, b, exceptioncode );
}

Tokenizer *HTMLDocumentImpl::createTokenizer()
{
    return new HTMLTokenizer(docPtr(),m_view);
}

// --------------------------------------------------------------------------
// not part of the DOM
// --------------------------------------------------------------------------

bool HTMLDocumentImpl::childAllowed( NodeImpl *newChild )
{
    // ### support comments. etc as a child
    return (newChild->id() == ID_HTML || newChild->id() == ID_COMMENT || newChild->nodeType() == Node::DOCUMENT_TYPE_NODE);
}

ElementImpl *HTMLDocumentImpl::createElement( const DOMString &name, int* pExceptioncode )
{
    if (pExceptioncode && !Element::khtmlValidQualifiedName(name)) {
        *pExceptioncode = DOMException::INVALID_CHARACTER_ERR;
        return 0;
    }

    return createHTMLElement(name, hMode != XHtml);
}

ElementImpl* HTMLDocumentImpl::activeElement() const
{
    NodeImpl* fn = focusNode();
    if (!fn || !fn->isElementNode())
	return body();
    else
	return static_cast<ElementImpl*>(fn);
}

void HTMLDocumentImpl::slotHistoryChanged()
{
    if ( true || !m_render )
        return;

    recalcStyle( Force );
    m_render->repaint();
}

HTMLMapElementImpl* HTMLDocumentImpl::getMap(const DOMString& _url)
{
    QString url = _url.string();
    QString s;
    int pos = url.indexOf('#');
    //qDebug() << "map pos of #:" << pos;
    s = QString(_url.unicode() + pos + 1, _url.length() - pos - 1);

    QMap<QString,HTMLMapElementImpl*>::const_iterator it = mapMap.constFind(s);

    if (it != mapMap.constEnd())
        return *it;
    else
        return 0;
}

void HTMLDocumentImpl::contentLoaded()
{
    // auto fill: walk the tree and try to fill in login credentials
    if (view() && m_doAutoFill) {
        for (NodeImpl* n = this; n; n = n->traverseNextNode())
            if (n->id() == ID_FORM)
                static_cast<HTMLFormElementImpl*>(n)->doAutoFill();
        m_doAutoFill = false;
    }
}

void HTMLDocumentImpl::close()
{
    bool doload = !parsing() && m_tokenizer;

    DocumentImpl::close();

    if (doload) {

        if (title().isEmpty()) // ensure setTitle is called at least once
            setTitle( DOMString() );

        // According to dom the load event must not bubble
        // but other browsers execute in a frameset document
        // the first(IE)/last(Moz/Konq) registered onload on a <frame> and the
        // first(IE)/last(Moz/Konq) registered onload on a <frameset>.

        //qDebug() << "dispatching LOAD_EVENT on document " << document() << " " << (view()?view()->part()->name():0);

        //Make sure to flush any pending image events now, as we want them out before the document's load event
        dispatchImageLoadEventsNow();
        document()->dispatchWindowEvent(EventImpl::LOAD_EVENT, false, false);

        // don't update rendering if we're going to redirect anyway
        if ( part() && (part()->d->m_redirectURL.isNull() ||
                        part()->d->m_delayRedirect > 1) )
            updateRendering();
    }
}

void HTMLDocumentImpl::determineParseMode()
{
    m_determineParseMode = true;
    if (m_doctype == 0) {
        // Currently we haven't got any doctype, so default to quirks mode and Html4
        changeModes(Compat, Html4);
    }
}

void HTMLDocumentImpl::changeModes(ParseMode newPMode, HTMLMode newHMode)
{
    if (!m_determineParseMode) // change mode only when KHTMLPart called determineParseMode
        return;
    ParseMode oldPMode = pMode;
    pMode = newPMode;
    hMode = newHMode;
    // This needs to be done last, see tests/parser/compatmode_xhtml_mixed.html
    if ( hMode == Html4 && !m_htmlRequested ) {
        // this part is still debatable and possibly UA dependent
        hMode = XHtml;
        pMode = Transitional;
    }
    m_htmlCompat = (hMode != XHtml);

    m_styleSelector->strictParsing = !inCompatMode();

#if 0
    qDebug() << "DocumentImpl::determineParseMode: publicId =" << publicID << " systemId = " << systemID;
    qDebug() << "DocumentImpl::determineParseMode: htmlMode = " << hMode;
    if( pMode == Strict )
        qDebug() << " using strict parseMode";
    else if (pMode == Compat )
        qDebug() << " using compatibility parseMode";
    else
        qDebug() << " using transitional parseMode";
#endif

    if ( pMode != oldPMode && styleSelector() )
        updateStyleSelector(true/*shallow*/);
}

NodeListImpl* HTMLDocumentImpl::getElementsByName( const DOMString &elementName )
{
    return new NameNodeListImpl(this, elementName);
}

HTMLCollectionImpl* HTMLDocumentImpl::images()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::DOC_IMAGES);
}

HTMLCollectionImpl* HTMLDocumentImpl::applets()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::DOC_APPLETS);
}

HTMLCollectionImpl* HTMLDocumentImpl::links()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::DOC_LINKS);
}

HTMLCollectionImpl* HTMLDocumentImpl::forms()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::DOC_FORMS);
}

HTMLCollectionImpl* HTMLDocumentImpl::layers()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::DOC_LAYERS);
}

HTMLCollectionImpl* HTMLDocumentImpl::anchors()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::DOC_ANCHORS);
}

HTMLCollectionImpl* HTMLDocumentImpl::all()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::DOC_ALL);
}

HTMLCollectionImpl* HTMLDocumentImpl::scripts()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::DOC_SCRIPTS);
}

// --------------------------------------------------------------------------
// Support for displaying plaintext
// --------------------------------------------------------------------------

class HTMLTextTokenizer: public khtml::Tokenizer
{
public:
    HTMLTextTokenizer(DOM::HTMLDocumentImpl* doc): m_doc(doc)
    {}

    virtual void begin();
    virtual void write(const TokenizerString &str, bool appendData);

    virtual void end()    { emit finishedParsing(); };
    virtual void finish() { end(); };

    // We don't support any inline scripts here
    virtual bool isWaitingForScripts() const { return false; }
    virtual bool isExecutingScript() const   { return false; }
    virtual void executeScriptsWaitingForStylesheets() {};
private:
    DOM::HTMLDocumentImpl* m_doc;
};

void HTMLTextTokenizer::begin()
{
    int dummy;
    DOM::ElementImpl* html = m_doc->createElement("html", &dummy);
    DOM::ElementImpl* head = m_doc->createElement("head", &dummy);
    DOM::ElementImpl* body = m_doc->createElement("body", &dummy);
    DOM::ElementImpl* pre  = m_doc->createElement("pre", &dummy);

    m_doc->appendChild(html, dummy);
    html->appendChild(head, dummy);
    html->appendChild(body, dummy);
    body->appendChild(pre, dummy);
}

void HTMLTextTokenizer::write(const TokenizerString &str, bool /*appendData*/)
{
    // A potential worry here is the document being modified by
    // a script while we're still loading. To handle that, we always look up
    // the pre again, and append to it, even for document.write mess
    // and the like.
    RefPtr<NodeListImpl> coll = m_doc->getElementsByTagName("pre");
    if (coll->length() >= 1ul) {
        int dummy;
        coll->item(0)->appendChild(m_doc->createTextNode(str.toString()), dummy);
    }
}

HTMLTextDocumentImpl::HTMLTextDocumentImpl(KHTMLView *v): HTMLDocumentImpl(v)
{}

khtml::Tokenizer* HTMLTextDocumentImpl::createTokenizer()
{
    return new HTMLTextTokenizer(this);
}

