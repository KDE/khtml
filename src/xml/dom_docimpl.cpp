/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2002-2006 Apple Computer, Inc.
 *           (C) 2006 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2005 Frerich Raabe <raabe@kde.org>
 *           (C) 2010 Maksim Orlovich <maksim@kde.org>
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

#include "dom_docimpl.h"

#include <dom/dom_exception.h>

#include "dom_textimpl.h"
#include "dom_xmlimpl.h"
#include "dom2_rangeimpl.h"
#include "dom2_eventsimpl.h"
#include "xml_tokenizer.h"
#include <html/htmltokenizer.h>
#include "dom_restyler.h"

#include <css/csshelper.h>
#include <css/cssstyleselector.h>
#include <css/css_stylesheetimpl.h>
#include <misc/helper.h>
#include <misc/seed.h>
#include <misc/loader.h>
#include <ecma/kjs_proxy.h>
#include <ecma/kjs_binding.h>

#include <QtCore/QStack>
//Added by qt3to4:
#include <QTimerEvent>
#include <QtCore/QList>
#include <QDebug>
#include <klocalizedstring.h>

#include <rendering/counter_tree.h>
#include <rendering/render_canvas.h>
#include <rendering/render_replaced.h>
#include <rendering/render_arena.h>
#include <rendering/render_layer.h>
#include <rendering/render_frames.h>
#include <rendering/render_image.h>

#include <khtmlview.h>
#include <khtml_part.h>
#include <kurlauthorized.h>
#include <khtml_settings.h>
#include <khtmlpart_p.h>

#include <xml/dom3_xpathimpl.h>
#include <html/html_baseimpl.h>
#include <html/html_blockimpl.h>
#include <html/html_canvasimpl.h>
#include <html/html_documentimpl.h>
#include <html/html_formimpl.h>
#include <html/html_headimpl.h>
#include <html/html_imageimpl.h>
#include <html/html_listimpl.h>
#include <html/html_miscimpl.h>
#include <html/html_tableimpl.h>
#include <html/html_objectimpl.h>
#include <html/HTMLAudioElement.h>
#include <html/HTMLVideoElement.h>
#include <html/HTMLSourceElement.h>
#include <editing/jsediting.h>

#include <ecma/kjs_window.h>

// SVG (WebCore)
#include <svg/SVGElement.h>
#include <svg/SVGSVGElement.h>
#include <svg/SVGNames.h>
#include <svg/SVGDocumentExtensions.h>
#include <svg/SVGRectElement.h>
#include <svg/SVGDocument.h>
#include <svg/SVGCircleElement.h>
#include <svg/SVGStyleElement.h>
#include <svg/SVGEllipseElement.h>
#include <svg/SVGPolygonElement.h>
#include <svg/SVGPolylineElement.h>
#include <svg/SVGPathElement.h>
#include <svg/SVGDefsElement.h>
#include <svg/SVGLinearGradientElement.h>
#include <svg/SVGRadialGradientElement.h>
#include <svg/SVGStopElement.h>
#include <svg/SVGClipPathElement.h>
#include <svg/SVGGElement.h>
#include <svg/SVGUseElement.h>
#include <svg/SVGLineElement.h>
#include <svg/SVGTextElement.h>
#include <svg/SVGAElement.h>
#include <svg/SVGScriptElement.h>
#include <svg/SVGDescElement.h>
#include <svg/SVGTitleElement.h>
#include <svg/SVGTextPathElement.h>
#include <svg/SVGTSpanElement.h>
#include <svg/SVGHKernElement.h>
#include <svg/SVGAltGlyphElement.h>
#include <svg/SVGFontElement.h>

#include <kio/job.h>
#include <QFontDatabase>

#include <stdlib.h>
#include <limits.h>

template class QStack<DOM::NodeImpl*>;

using namespace DOM;
using namespace khtml;

// ------------------------------------------------------------------------

DOMImplementationImpl::DOMImplementationImpl()
{
}

DOMImplementationImpl::~DOMImplementationImpl()
{
}

bool DOMImplementationImpl::hasFeature ( const DOMString &feature, const DOMString &version )
{
    // ### update when we (fully) support the relevant features
    QString lower = feature.string().toLower();
    if ((lower == "html" || lower == "xml") &&
        (version.isEmpty() || version == "1.0" || version == "2.0"))
        return true;

    // ## Do we support Core Level 3 ?
    if ((lower == "core" ) &&
        (version.isEmpty() || version == "2.0"))
        return true;

    if ((lower == "traversal") &&
        (version.isEmpty() || version == "2.0"))
        return true;

    if ((lower == "css") &&
        (version.isEmpty() || version == "2.0"))
        return true;

    if ((lower == "events" || lower == "uievents" ||
         lower == "mouseevents" || lower == "mutationevents" ||
         lower == "htmlevents" || lower == "textevents" ) &&
        (version.isEmpty() || version == "2.0" || version == "3.0"))
        return true;

    if (lower == "selectors-api" && version == "1.0")
        return true;
        
    return false;
}

DocumentTypeImpl *DOMImplementationImpl::createDocumentType( const DOMString &qualifiedName, const DOMString &publicId,
                                                             const DOMString &systemId, int &exceptioncode )
{
    // Not mentioned in spec: throw NAMESPACE_ERR if no qualifiedName supplied
    if (qualifiedName.isNull()) {
        exceptioncode = DOMException::NAMESPACE_ERR;
        return 0;
    }

    // INVALID_CHARACTER_ERR: Raised if the specified qualified name contains an illegal character.
    if (!Element::khtmlValidQualifiedName(qualifiedName)) {
        exceptioncode = DOMException::INVALID_CHARACTER_ERR;
        return 0;
    }

    // NAMESPACE_ERR: Raised if the qualifiedName is malformed.
    // Added special case for the empty string, which seems to be a common pre-DOM2 misuse
    if (!qualifiedName.isEmpty() && Element::khtmlMalformedQualifiedName(qualifiedName)) {
        exceptioncode = DOMException::NAMESPACE_ERR;
        return 0;
    }

    return new DocumentTypeImpl(this, 0, qualifiedName, publicId, systemId);
}

DocumentImpl *DOMImplementationImpl::createDocument( const DOMString &namespaceURI, const DOMString &qualifiedName,
                                                     DocumentTypeImpl* dtype,
                                                     KHTMLView* v,
                                                     int &exceptioncode )
{
    exceptioncode = 0;

    if (!checkQualifiedName(qualifiedName, namespaceURI, 0, true/*nameCanBeNull*/,
                            true /*nameCanBeEmpty, see #61650*/, &exceptioncode) )
        return 0;

    // WRONG_DOCUMENT_ERR: Raised if doctype has already been used with a different document or was
    // created from a different implementation.
    // We elide the "different implementation" case here, since we're not doing interop
    // of different implementations, and different impl objects exist only for
    // isolation  reasons
    if (dtype && dtype->document()) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return 0;
    }

    // ### view can be 0 which can cause problems
    DocumentImpl* doc;
    if (namespaceURI == XHTML_NAMESPACE)
        doc = new HTMLDocumentImpl(v);
    else
        doc = new DocumentImpl(v);

    if (dtype) {
        dtype->setDocument(doc);
        doc->appendChild(dtype,exceptioncode);
    }

    // the document must be created empty if all parameters are null
    // (or empty for qName/nsURI as a tolerance) - see DOM 3 Core.
    if (dtype || !qualifiedName.isEmpty() || !namespaceURI.isEmpty()) {
        ElementImpl *element = doc->createElementNS(namespaceURI,qualifiedName);
        doc->appendChild(element,exceptioncode);
        if (exceptioncode) {
            delete element;
            delete doc;
            return 0;
        }
    }
    return doc;
}

CSSStyleSheetImpl *DOMImplementationImpl::createCSSStyleSheet(DOMStringImpl* title, DOMStringImpl *media,
                                                              int &/*exceptioncode*/)
{
    // ### TODO : media could have wrong syntax, in which case we should
	// generate an exception.
	CSSStyleSheetImpl *parent = 0L;
	CSSStyleSheetImpl *sheet = new CSSStyleSheetImpl(parent, DOMString());
	sheet->setMedia(new MediaListImpl(sheet, media, true /*fallbackToDescriptor*/));
	sheet->setTitle(DOMString(title));
	return sheet;
}

DocumentImpl *DOMImplementationImpl::createDocument( KHTMLView *v )
{
    DocumentImpl* doc = new DocumentImpl(v);

    return doc;
}

XMLDocumentImpl *DOMImplementationImpl::createXMLDocument( KHTMLView *v )
{
    XMLDocumentImpl* doc = new XMLDocumentImpl(v);

    return doc;
}

HTMLDocumentImpl *DOMImplementationImpl::createHTMLDocument( KHTMLView *v )
{
    HTMLDocumentImpl* doc = new HTMLDocumentImpl(v);

    return doc;
}

// create SVG document
WebCore::SVGDocument *DOMImplementationImpl::createSVGDocument( KHTMLView *v )
{
    WebCore::SVGDocument* doc = new WebCore::SVGDocument(v);

    return doc;
}

HTMLDocumentImpl* DOMImplementationImpl::createHTMLDocument( const DOMString& title )
{
    HTMLDocumentImpl* r = createHTMLDocument( 0 /* ### create a view otherwise it doesn't work */);

    r->open();

    r->write(QLatin1String("<HTML><HEAD><TITLE>") + title.string() +
             QLatin1String("</TITLE></HEAD>"));

    return r;
}

// ------------------------------------------------------------------------

ElementMappingCache::ElementMappingCache():m_dict()
{
}

ElementMappingCache::~ElementMappingCache()
{
    qDeleteAll( m_dict );
}

void ElementMappingCache::add(const DOMString& id, ElementImpl* nd)
{
    if (id.isEmpty()) return;

    ItemInfo* info = m_dict.value(id);
    if (info)
    {
        info->ref++;
        info->nd = 0; //Now ambigous
    }
    else
    {
        ItemInfo* info = new ItemInfo();
        info->ref = 1;
        info->nd  = nd;
        m_dict.insert(id, info);
    }
}

void ElementMappingCache::set(const DOMString& id, ElementImpl* nd)
{
    if (id.isEmpty()) return;

    assert(m_dict.contains(id));
    ItemInfo* info = m_dict.value(id);
    info->nd = nd;
}

void ElementMappingCache::remove(const DOMString& id, ElementImpl* nd)
{
    if (id.isEmpty()) return;

    assert(m_dict.contains(id));
    ItemInfo* info = m_dict.value(id);
    info->ref--;
    if (info->ref == 0)
    {
        m_dict.take(id);
        delete info;
    }
    else
    {
        if (info->nd == nd)
            info->nd = 0;
    }
}

bool ElementMappingCache::contains(const DOMString& id)
{
    if (id.isEmpty()) return false;
    return m_dict.contains(id);
}

ElementMappingCache::ItemInfo* ElementMappingCache::get(const DOMString& id)
{
    if (id.isEmpty()) return 0;
    return m_dict.value(id);
}

typedef QList<DocumentImpl*> ChangedDocuments ;
Q_GLOBAL_STATIC(ChangedDocuments, s_changedDocuments)

// KHTMLView might be 0
DocumentImpl::DocumentImpl(KHTMLView *v)
    : NodeBaseImpl( 0 ), m_svgExtensions(0), m_counterDict(),
      m_imageLoadEventTimer(0)
{
    m_document.resetSkippingRef(this); //Make document return us..
    m_selfOnlyRefCount = 0;

    m_paintDevice = 0;
    //m_decoderMibEnum = 0;
    m_textColor = Qt::black;

    m_view = v;
    m_renderArena.reset();

    KHTMLGlobal::registerDocumentImpl(this);

    if ( v ) {
        m_docLoader = new DocLoader(v->part(), this );
        setPaintDevice( m_view );
    }
    else
        m_docLoader = new DocLoader( 0, this );

    visuallyOrdered = false;
    m_bParsing = false;
    m_docChanged = false;
    m_elemSheet = 0;
    m_tokenizer = 0;
    m_decoder = 0;
    m_doctype = 0;
    m_implementation = 0;
    pMode = Strict;
    hMode = XHtml;
    m_htmlCompat = false;
    m_textColor = "#000000";
    m_focusNode = 0;
    m_hoverNode = 0;
    m_activeNode = 0;
    m_defaultView = new AbstractViewImpl(this);
    m_defaultView->ref();
    m_listenerTypes = 0;
    m_styleSheets = new StyleSheetListImpl(this);
    m_styleSheets->ref();
    m_addedStyleSheets = 0;
    m_inDocument = true;
    m_styleSelectorDirty = false;
    m_styleSelector = 0;
    m_styleSheetListDirty = true;

    m_inStyleRecalc = false;
    m_pendingStylesheets = 0;
    m_ignorePendingStylesheets = false;
    m_async = true;
    m_hadLoadError = false;
    m_docLoading = false;
    m_bVariableLength = false;
    m_inSyncLoad = 0;
    m_loadingXMLDoc = 0;
    m_documentElement = 0;
    m_cssTarget = 0;
    m_jsEditor = 0;
    m_dynamicDomRestyler = new khtml::DynamicDomRestyler();
    m_stateRestorePos = 0;
    m_windowEventTarget = new WindowEventTargetImpl(this);
    m_windowEventTarget->ref();
    
    for (int c = 0; c < NumTreeVersions; ++c)
        m_domTreeVersions[c] = 0;
}

void DocumentImpl::removedLastRef()
{
    if (m_selfOnlyRefCount) {
        /* In this case, the only references to us are from children,
           so we have a cycle. We'll try to break it by disconnecting the
           children from us; this sucks/is wrong, but it's pretty much
           the best we can do without tracing.

           Of course, if dumping the children causes the refcount from them to
           drop to 0 we can get killed right here, so better hold
           a temporary reference, too
        */
        DocPtr<DocumentImpl> guard(this);

        // we must make sure not to be retaining any of our children through
        // these extra pointers or we will create a reference cycle
        if (m_doctype) {
            m_doctype->deref();
            m_doctype = 0;
        }

        if (m_cssTarget) {
            m_cssTarget->deref();
            m_cssTarget = 0;
        }

        if (m_focusNode) {
            m_focusNode->deref();
            m_focusNode = 0;
        }

        if (m_hoverNode) {
            m_hoverNode->deref();
            m_hoverNode = 0;
        }

        if (m_activeNode) {
            m_activeNode->deref();
            m_activeNode = 0;
        }

        if (m_documentElement) {
            m_documentElement->deref();
            m_documentElement = 0;
        }

        removeChildren();

        delete m_tokenizer;
        m_tokenizer = 0;
    } else {
        delete this;
    }
}

DocumentImpl::~DocumentImpl()
{
    //Important: if you need to remove stuff here,
    //you may also have to fix removedLastRef() above - M.O.
    assert( !m_render );

    QHashIterator<long,DynamicNodeListImpl::Cache*> it(m_nodeListCache);
    while (it.hasNext())
        it.next().value()->deref();

    if (m_loadingXMLDoc)
	m_loadingXMLDoc->deref(this);
    if (s_changedDocuments() && m_docChanged)
        s_changedDocuments()->removeAll(this);
    delete m_tokenizer;
    m_document.resetSkippingRef(0);
    delete m_styleSelector;
    delete m_docLoader;
    if (m_elemSheet )  m_elemSheet->deref();
    if (m_doctype)
        m_doctype->deref();
    if (m_implementation)
        m_implementation->deref();
    delete m_dynamicDomRestyler;
    delete m_jsEditor;
    m_defaultView->deref();
    m_styleSheets->deref();
    if (m_addedStyleSheets)
        m_addedStyleSheets->deref();
    if (m_cssTarget)
        m_cssTarget->deref();
    if (m_focusNode)
        m_focusNode->deref();
    if ( m_hoverNode )
        m_hoverNode->deref();
    if (m_activeNode)
        m_activeNode->deref();
    if (m_documentElement)
        m_documentElement->deref();
    m_windowEventTarget->deref();
    qDeleteAll(m_counterDict);

    m_renderArena.reset();

    KHTMLGlobal::deregisterDocumentImpl(this);
}


DOMImplementationImpl *DocumentImpl::implementation() const
{
    if (!m_implementation) {
        m_implementation = new DOMImplementationImpl();
        m_implementation->ref();
    }
    return m_implementation;
}

void DocumentImpl::childrenChanged()
{
    // invalidate the document element we have cached in case it was replaced
    if (m_documentElement)
        m_documentElement->deref();
    m_documentElement = 0;

    // same for m_docType
    if (m_doctype)
        m_doctype->deref();
    m_doctype = 0;
}

ElementImpl *DocumentImpl::documentElement() const
{
    if (!m_documentElement) {
        NodeImpl* n = firstChild();
        while (n && n->nodeType() != Node::ELEMENT_NODE)
            n = n->nextSibling();
        m_documentElement = static_cast<ElementImpl*>(n);
        if (m_documentElement)
            m_documentElement->ref();
    }
    return m_documentElement;
}

DocumentTypeImpl *DocumentImpl::doctype() const
{
    if (!m_doctype) {
        NodeImpl* n = firstChild();
        while (n && n->nodeType() != Node::DOCUMENT_TYPE_NODE)
            n = n->nextSibling();
        m_doctype = static_cast<DocumentTypeImpl*>(n);
        if (m_doctype)
            m_doctype->ref();
    }
    return m_doctype;
}


ElementImpl *DocumentImpl::createElement( const DOMString &name, int* pExceptioncode )
{
    if (pExceptioncode && !Element::khtmlValidQualifiedName(name)) {
        *pExceptioncode = DOMException::INVALID_CHARACTER_ERR;
        return 0;
    }

    PrefixName prefix;
    LocalName  localName;
    bool htmlCompat = htmlMode() != XHtml;
    splitPrefixLocalName(name, prefix, localName, htmlCompat);
    XMLElementImpl* e = new XMLElementImpl(document(), emptyNamespaceName, localName, prefix);
    e->setHTMLCompat(htmlCompat); // Not a real HTML element, but inside an html-compat doc all tags are uppercase.
    return e;
}

AttrImpl *DocumentImpl::createAttribute( const DOMString &tagName, int* pExceptioncode )
{
    if (pExceptioncode && !Element::khtmlValidAttrName(tagName)) {
        *pExceptioncode = DOMException::INVALID_CHARACTER_ERR;
        return 0;
    }

    PrefixName prefix;
    LocalName  localName;
    bool htmlCompat = (htmlMode() != XHtml);
    splitPrefixLocalName(tagName, prefix, localName, htmlCompat);

    AttrImpl* attr = new AttrImpl(0, document(), NamespaceName::fromId(emptyNamespace),
            localName, prefix, DOMString("").implementation());
    attr->setHTMLCompat(htmlCompat);
    return attr;
}

DocumentFragmentImpl *DocumentImpl::createDocumentFragment(  )
{
    return new DocumentFragmentImpl( docPtr() );
}

CommentImpl *DocumentImpl::createComment ( DOMStringImpl* data )
{
    return new CommentImpl( docPtr(), data );
}

CDATASectionImpl *DocumentImpl::createCDATASection ( DOMStringImpl* data )
{
    return new CDATASectionImpl( docPtr(), data );
}

ProcessingInstructionImpl *DocumentImpl::createProcessingInstruction ( const DOMString &target, DOMStringImpl* data )
{
    return new ProcessingInstructionImpl( docPtr(),target,data);
}

EntityReferenceImpl *DocumentImpl::createEntityReference ( const DOMString &name )
{
    return new EntityReferenceImpl(docPtr(), name.implementation());
}

EditingTextImpl *DocumentImpl::createEditingTextNode(const DOMString &text)
{
    return new EditingTextImpl(docPtr(), text);
}

NodeImpl *DocumentImpl::importNode(NodeImpl *importedNode, bool deep, int &exceptioncode)
{
    NodeImpl *result = 0;

    // Not mentioned in spec: throw NOT_FOUND_ERR if evt is null
    if (!importedNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return 0;
    }

    if(importedNode->nodeType() == Node::ELEMENT_NODE)
    {
        // Why not use cloneNode?
	ElementImpl *otherElem = static_cast<ElementImpl*>(importedNode);
	NamedAttrMapImpl *otherMap = static_cast<ElementImpl *>(importedNode)->attributes(true);

	ElementImpl *tempElementImpl;
	tempElementImpl = createElementNS(otherElem->namespaceURI(),otherElem->nonCaseFoldedTagName());
	tempElementImpl->setHTMLCompat(htmlMode() != XHtml && otherElem->htmlCompat());
	result = tempElementImpl;

	if(otherMap) {
	    for(unsigned i = 0; i < otherMap->length(); i++)
	    {
		AttrImpl *otherAttr = otherMap->attributeAt(i).createAttr(otherElem,otherElem->docPtr());

		tempElementImpl->setAttributeNS(otherAttr->namespaceURI(),
						    otherAttr->name(),
						    otherAttr->nodeValue(),
						    exceptioncode);

		if(exceptioncode != 0)
		    break; // ### properly cleanup here
	    }
	}
    }
    else if(importedNode->nodeType() == Node::TEXT_NODE)
    {
	result = createTextNode(static_cast<TextImpl*>(importedNode)->string());
	deep = false;
    }
    else if(importedNode->nodeType() == Node::CDATA_SECTION_NODE)
    {
	result = createCDATASection(static_cast<CDATASectionImpl*>(importedNode)->string());
	deep = false;
    }
    else if(importedNode->nodeType() == Node::ENTITY_REFERENCE_NODE)
	result = createEntityReference(importedNode->nodeName());
    else if(importedNode->nodeType() == Node::PROCESSING_INSTRUCTION_NODE)
    {
	result = createProcessingInstruction(importedNode->nodeName(), importedNode->nodeValue().implementation());
	deep = false;
    }
    else if(importedNode->nodeType() == Node::COMMENT_NODE)
    {
	result = createComment(static_cast<CommentImpl*>(importedNode)->string());
	deep = false;
    }
    else if (importedNode->nodeType() == Node::DOCUMENT_FRAGMENT_NODE)
	result = createDocumentFragment();
    else
	exceptioncode = DOMException::NOT_SUPPORTED_ERR;

    //### FIXME: This should handle Attributes, and a few other things

    if(deep && result)
    {
	for(Node n = importedNode->firstChild(); !n.isNull(); n = n.nextSibling())
	    result->appendChild(importNode(n.handle(), true, exceptioncode), exceptioncode);
    }

    return result;
}

ElementImpl *DocumentImpl::createElementNS( const DOMString &_namespaceURI, const DOMString &_qualifiedName, int* pExceptioncode )
{
    ElementImpl *e = 0;
    int colonPos = -2;
    // check NAMESPACE_ERR/INVALID_CHARACTER_ERR
    if (pExceptioncode && !checkQualifiedName(_qualifiedName, _namespaceURI, &colonPos,
                                              false/*nameCanBeNull*/, false/*nameCanBeEmpty*/,
                                              pExceptioncode))
        return 0;
    DOMString prefix, localName;
    splitPrefixLocalName(_qualifiedName.implementation(), prefix, localName, colonPos);

    if (_namespaceURI == SVG_NAMESPACE) {
        e = createSVGElement(QualifiedName(prefix, localName, _namespaceURI));
        if (e) return e;
        if (!e) {
            qWarning() << "svg element" << localName << "either is not supported by khtml or it's not a proper svg element";
        }
    }

    // Regardless of document type (even for HTML), this method will only create HTML
    // elements if given the namespace explicitly. Further, this method is always
    // case sensitive, again, even in HTML; however .tagName will case-normalize
    // in HTML regardless
    if (_namespaceURI == XHTML_NAMESPACE) {
        e = createHTMLElement(localName, false /* case sensitive */);
        int _exceptioncode = 0;
        if (!prefix.isNull())
            e->setPrefix(prefix, _exceptioncode);
        if ( _exceptioncode ) {
                if ( pExceptioncode ) *pExceptioncode = _exceptioncode;
                delete e;
                return 0;
        }
    }
    if (!e) {
        e = new XMLElementImpl(document(), NamespaceName::fromString(_namespaceURI),
                LocalName::fromString(localName), PrefixName::fromString(prefix));
    }

    return e;
}

AttrImpl *DocumentImpl::createAttributeNS( const DOMString &_namespaceURI,
                                           const DOMString &_qualifiedName, int* pExceptioncode)
{
    int colonPos = -2;
    // check NAMESPACE_ERR/INVALID_CHARACTER_ERR
    if (pExceptioncode && !checkQualifiedName(_qualifiedName, _namespaceURI, &colonPos,
                                              false/*nameCanBeNull*/, false/*nameCanBeEmpty*/,
                                              pExceptioncode))
        return 0;
    PrefixName prefix;
    LocalName  localName;
    bool htmlCompat =  _namespaceURI.isNull() && htmlMode() != XHtml;
    splitPrefixLocalName(_qualifiedName, prefix, localName, false, colonPos);
    AttrImpl* attr = new AttrImpl(0, document(), NamespaceName::fromString(_namespaceURI),
            localName, prefix, DOMString("").implementation());
    attr->setHTMLCompat( htmlCompat );
    return attr;
}

ElementImpl *DocumentImpl::getElementById( const DOMString &elementId ) const
{
    ElementMappingCache::ItemInfo* info = m_getElementByIdCache.get(elementId);

    if (!info)
        return 0;

    //See if cache has an unambiguous answer.
    if (info->nd)
        return info->nd;

    //Now we actually have to walk.
    QStack<NodeImpl*> nodeStack;
    NodeImpl *current = _first;

    while(1)
    {
        if(!current)
        {
            if(nodeStack.isEmpty()) break;
            current = nodeStack.pop();
            current = current->nextSibling();
        }
        else
        {
            if(current->isElementNode())
            {
                ElementImpl *e = static_cast<ElementImpl *>(current);
                if(e->getAttribute(ATTR_ID) == elementId) {
                    info->nd = e;
                    return e;
                }
            }

            NodeImpl *child = current->firstChild();
            if(child)
            {
                nodeStack.push(current);
                current = child;
            }
            else
            {
                current = current->nextSibling();
            }
        }
    }

    assert(0); //If there is no item with such an ID, we should never get here

    //qDebug() << "WARNING: *DocumentImpl::getElementById not found " << elementId.string();

    return 0;
}

void DocumentImpl::setTitle(const DOMString& _title)
{
    if (_title == m_title && !m_title.isNull()) return;

    m_title = _title;

    QString titleStr = m_title.string();
    for (int i = 0; i < titleStr.length(); ++i)
        if (titleStr[i] < ' ')
            titleStr[i] = ' ';
    titleStr = titleStr.simplified();
    if ( view() && !view()->part()->parentPart() ) {
	if (titleStr.isEmpty()) {
	    // empty title... set window caption as the URL
	    QUrl url = m_url;
	    url.setFragment(QString());
	    url.setQuery(QString());
	    titleStr = url.toDisplayString();
	}

	emit view()->part()->setWindowCaption( titleStr );
    }
}

DOMString DocumentImpl::nodeName() const
{
    return "#document";
}

unsigned short DocumentImpl::nodeType() const
{
    return Node::DOCUMENT_NODE;
}

ElementImpl *DocumentImpl::createHTMLElement( const DOMString &name, bool caseInsensitive )
{
    LocalName localname = LocalName::fromString(name,
                                  caseInsensitive ? IDS_NormalizeLower : IDS_CaseSensitive);
    uint id = localname.id();

    ElementImpl *n = 0;
    switch(id)
    {
    case ID_HTML:
        n = new HTMLHtmlElementImpl(docPtr());
        break;
    case ID_HEAD:
        n = new HTMLHeadElementImpl(docPtr());
        break;
    case ID_BODY:
        n = new HTMLBodyElementImpl(docPtr());
        break;

// head elements
    case ID_BASE:
        n = new HTMLBaseElementImpl(docPtr());
        break;
    case ID_LINK:
        n = new HTMLLinkElementImpl(docPtr());
        break;
    case ID_META:
        n = new HTMLMetaElementImpl(docPtr());
        break;
    case ID_STYLE:
        n = new HTMLStyleElementImpl(docPtr());
        break;
    case ID_TITLE:
        n = new HTMLTitleElementImpl(docPtr());
        break;

// frames
    case ID_FRAME:
        n = new HTMLFrameElementImpl(docPtr());
        break;
    case ID_FRAMESET:
        n = new HTMLFrameSetElementImpl(docPtr());
        break;
    case ID_IFRAME:
        n = new HTMLIFrameElementImpl(docPtr());
        break;

// form elements
// ### FIXME: we need a way to set form dependency after we have made the form elements
    case ID_FORM:
            n = new HTMLFormElementImpl(docPtr(), false);
        break;
    case ID_BUTTON:
            n = new HTMLButtonElementImpl(docPtr());
        break;
    case ID_FIELDSET:
            n = new HTMLFieldSetElementImpl(docPtr());
        break;
    case ID_INPUT:
            n = new HTMLInputElementImpl(docPtr());
        break;
    case ID_ISINDEX:
            n = new HTMLIsIndexElementImpl(docPtr());
        break;
    case ID_LABEL:
            n = new HTMLLabelElementImpl(docPtr());
        break;
    case ID_LEGEND:
            n = new HTMLLegendElementImpl(docPtr());
        break;
    case ID_OPTGROUP:
            n = new HTMLOptGroupElementImpl(docPtr());
        break;
    case ID_OPTION:
            n = new HTMLOptionElementImpl(docPtr());
        break;
    case ID_SELECT:
            n = new HTMLSelectElementImpl(docPtr());
        break;
    case ID_TEXTAREA:
            n = new HTMLTextAreaElementImpl(docPtr());
        break;

// lists
    case ID_DL:
        n = new HTMLDListElementImpl(docPtr());
        break;
    case ID_DD:
        n = new HTMLGenericElementImpl(docPtr(), id);
        break;
    case ID_DT:
        n = new HTMLGenericElementImpl(docPtr(), id);
        break;
    case ID_UL:
        n = new HTMLUListElementImpl(docPtr());
        break;
    case ID_OL:
        n = new HTMLOListElementImpl(docPtr());
        break;
    case ID_DIR:
        n = new HTMLDirectoryElementImpl(docPtr());
        break;
    case ID_MENU:
        n = new HTMLMenuElementImpl(docPtr());
        break;
    case ID_LI:
        n = new HTMLLIElementImpl(docPtr());
        break;

// formatting elements (block)
    case ID_DIV:
    case ID_P:
        n = new HTMLDivElementImpl( docPtr(), id );
        break;
    case ID_BLOCKQUOTE:
    case ID_H1:
    case ID_H2:
    case ID_H3:
    case ID_H4:
    case ID_H5:
    case ID_H6:
        n = new HTMLGenericElementImpl(docPtr(), id);
        break;
    case ID_HR:
        n = new HTMLHRElementImpl(docPtr());
        break;
    case ID_PLAINTEXT:
    case ID_XMP:
    case ID_PRE:
    case ID_LISTING:
        n = new HTMLPreElementImpl(docPtr(), id);
        break;

// font stuff
    case ID_BASEFONT:
        n = new HTMLBaseFontElementImpl(docPtr());
        break;
    case ID_FONT:
        n = new HTMLFontElementImpl(docPtr());
        break;

// ins/del
    case ID_DEL:
    case ID_INS:
        n = new HTMLGenericElementImpl(docPtr(), id);
        break;

// anchor
    case ID_A:
        n = new HTMLAnchorElementImpl(docPtr());
        break;

// images
    case ID_IMG:
    case ID_IMAGE: // legacy name
        n = new HTMLImageElementImpl(docPtr());
        break;
    case ID_CANVAS:
        n = new HTMLCanvasElementImpl(docPtr());
        break;
    case ID_MAP:
        n = new HTMLMapElementImpl(docPtr());
        /*n = map;*/
        break;
    case ID_AREA:
        n = new HTMLAreaElementImpl(docPtr());
        break;

// objects, applets and scripts
    case ID_APPLET:
        n = new HTMLAppletElementImpl(docPtr());
        break;
    case ID_OBJECT:
        n = new HTMLObjectElementImpl(docPtr());
        break;
    case ID_EMBED:
        n = new HTMLEmbedElementImpl(docPtr());
        break;
    case ID_PARAM:
        n = new HTMLParamElementImpl(docPtr());
        break;
    case ID_SCRIPT:
        n = new HTMLScriptElementImpl(docPtr());
        break;

// media
    case ID_AUDIO:
        n = new HTMLAudioElement(docPtr());
        break;
    case ID_VIDEO:
        n = new HTMLVideoElement(docPtr());
        break;
    case ID_SOURCE:
        n = new HTMLSourceElement(docPtr());
        break;

// tables
    case ID_TABLE:
        n = new HTMLTableElementImpl(docPtr());
        break;
    case ID_CAPTION:
        n = new HTMLTableCaptionElementImpl(docPtr());
        break;
    case ID_COLGROUP:
    case ID_COL:
        n = new HTMLTableColElementImpl(docPtr(), id);
        break;
    case ID_TR:
        n = new HTMLTableRowElementImpl(docPtr());
        break;
    case ID_TD:
    case ID_TH:
        n = new HTMLTableCellElementImpl(docPtr(), id);
        break;
    case ID_THEAD:
    case ID_TBODY:
    case ID_TFOOT:
        n = new HTMLTableSectionElementImpl(docPtr(), id, false);
        break;

// inline elements
    case ID_BR:
        n = new HTMLBRElementImpl(docPtr());
        break;
    case ID_WBR:
        n = new HTMLWBRElementImpl(docPtr());
        break;
    case ID_Q:
        n = new HTMLGenericElementImpl(docPtr(), id);
        break;

// elements with no special representation in the DOM

// block:
    case ID_ADDRESS:
    case ID_CENTER:
        n = new HTMLGenericElementImpl(docPtr(), id);
        break;
// inline
        // %fontstyle
    case ID_TT:
    case ID_U:
    case ID_B:
    case ID_I:
    case ID_S:
    case ID_STRIKE:
    case ID_BIG:
    case ID_SMALL:

        // %phrase
    case ID_EM:
    case ID_STRONG:
    case ID_DFN:
    case ID_CODE:
    case ID_SAMP:
    case ID_KBD:
    case ID_VAR:
    case ID_CITE:
    case ID_ABBR:
    case ID_ACRONYM:

        // %special
    case ID_SUB:
    case ID_SUP:
    case ID_SPAN:
    case ID_NOBR:
    case ID_BDO:
    case ID_NOFRAMES:
    case ID_NOSCRIPT:
    case ID_NOEMBED:
    case ID_NOLAYER:
        n = new HTMLGenericElementImpl(docPtr(), id);
        break;

    case ID_MARQUEE:
        n = new HTMLMarqueeElementImpl(docPtr());
        break;
// text
    case ID_TEXT:
        // qDebug() << "Use document->createTextNode()";
        break;

    default:
        n = new HTMLGenericElementImpl(docPtr(), localname);
        break;
    }
    assert(n);
    return n;
}

// SVG
ElementImpl *DocumentImpl::createSVGElement(const QualifiedName& name)
{
    uint id = name.localNameId().id();
    // qDebug() << getPrintableName(name.id()) << endl;
    // qDebug() << "svg text:   " << getPrintableName(WebCore::SVGNames::textTag.id()) << endl;

    ElementImpl *n = 0;
    switch (id)
    {
    case ID_TEXTPATH:
        n = new WebCore::SVGTextPathElement(name, docPtr());
        break;
    case ID_TSPAN:
        n = new WebCore::SVGTSpanElement(name, docPtr());
        break;
    case ID_HKERN:
        n = new WebCore::SVGHKernElement(name, docPtr());
        break;
    case ID_ALTGLYPH:
        n = new WebCore::SVGAltGlyphElement(name, docPtr());
        break;
    case ID_FONT:
        n = new WebCore::SVGFontElement(name, docPtr());
        break;
    }

    if (id == WebCore::SVGNames::svgTag.localNameId().id()) {
        n = new WebCore::SVGSVGElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::rectTag.localNameId().id()) {
        n = new WebCore::SVGRectElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::circleTag.localNameId().id()) {
        n = new WebCore::SVGCircleElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::ellipseTag.localNameId().id()) {
        n = new WebCore::SVGEllipseElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::polylineTag.localNameId().id()) {
        n = new WebCore::SVGPolylineElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::polygonTag.localNameId().id()) {
        n = new WebCore::SVGPolygonElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::pathTag.localNameId().id()) {
        n = new WebCore::SVGPathElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::defsTag.localNameId().id()) {
        n = new WebCore::SVGDefsElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::linearGradientTag.localNameId().id()) {
        n = new WebCore::SVGLinearGradientElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::radialGradientTag.localNameId().id()) {
        n = new WebCore::SVGRadialGradientElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::stopTag.localNameId().id()) {
        n = new WebCore::SVGStopElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::clipPathTag.localNameId().id()) {
        n = new WebCore::SVGClipPathElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::gTag.localNameId().id()) {
        n = new WebCore::SVGGElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::useTag.localNameId().id()) {
        n = new WebCore::SVGUseElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::lineTag.localNameId().id()) {
        n = new WebCore::SVGLineElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::textTag.localNameId().id()) {
        n = new WebCore::SVGTextElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::aTag.localNameId().id()) {
        n = new WebCore::SVGAElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::scriptTag.localNameId().id()) {
        n = new WebCore::SVGScriptElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::descTag.localNameId().id()) {
        n = new WebCore::SVGDescElement(name, docPtr());
    }

    if (id == WebCore::SVGNames::titleTag.localNameId().id()) {
        n = new WebCore::SVGTitleElement(name, docPtr());
    }

    if (id == makeId(svgNamespace, ID_STYLE)) {
        n = new WebCore::SVGStyleElement(name, docPtr());
    }

    return n;
}

void DocumentImpl::attemptRestoreState(NodeImpl* n)
{
    if (!n->isElementNode())
        return;

    ElementImpl* el = static_cast<ElementImpl*>(n);

    if (m_stateRestorePos >= m_state.size())
        return;

    // Grab the state and element info..
    QString idStr = m_state[m_stateRestorePos];
    QString nmStr = m_state[m_stateRestorePos + 1];
    QString tpStr = m_state[m_stateRestorePos + 2];
    QString stStr = m_state[m_stateRestorePos + 3];

    // Make sure it matches!
    if (idStr.toUInt() != el->id())
        return;
    if (nmStr != el->getAttribute(ATTR_NAME).string())
        return;
    if (tpStr != el->getAttribute(ATTR_TYPE).string())
        return;

    m_stateRestorePos += 4;
    if (!stStr.isNull())
        el->restoreState(stStr);
}

QStringList DocumentImpl::docState()
{
    QStringList s;
    for (QListIterator<NodeImpl*> it(m_maintainsState); it.hasNext();) {
        NodeImpl* n = it.next();
        if (!n->isElementNode())
            continue;

        ElementImpl* el = static_cast<ElementImpl*>(n);
        // Encode the element ID, as well as the name and type attributes
        s.append(QString::number(el->id()));
        s.append(el->getAttribute(ATTR_NAME).string());
        s.append(el->getAttribute(ATTR_TYPE).string());
        s.append(el->state());
    }

    return s;
}

bool DocumentImpl::unsubmittedFormChanges()
{
    for (QListIterator<NodeImpl*> it(m_maintainsState); it.hasNext();) {
        NodeImpl* node = it.next();
        if (node->isGenericFormElement() && static_cast<HTMLGenericFormElementImpl*>(node)->unsubmittedFormChanges())
            return true;
    }

    return false;
}

RangeImpl *DocumentImpl::createRange()
{
    return new RangeImpl( docPtr() );
}

NodeIteratorImpl *DocumentImpl::createNodeIterator(NodeImpl *root, unsigned long whatToShow,
                                                   NodeFilterImpl* filter, bool entityReferenceExpansion,
                                                   int &exceptioncode)
{
    if (!root) {
        exceptioncode = DOMException::NOT_SUPPORTED_ERR;
        return 0;
    }

    return new NodeIteratorImpl(root,whatToShow,filter,entityReferenceExpansion);
}

TreeWalkerImpl *DocumentImpl::createTreeWalker(NodeImpl *root, unsigned long whatToShow, NodeFilterImpl *filter,
                                bool entityReferenceExpansion, int &exceptioncode)
{
    if (!root) {
        exceptioncode = DOMException::NOT_SUPPORTED_ERR;
        return 0;
    }

    return new TreeWalkerImpl( root, whatToShow, filter, entityReferenceExpansion );
}

void DocumentImpl::setDocumentChanged(bool b)
{
    if (b && !m_docChanged)
        s_changedDocuments()->append(this);
    else if (!b && m_docChanged)
        s_changedDocuments()->removeAll(this);
    m_docChanged = b;
}

void DocumentImpl::recalcStyle( StyleChange change )
{
//     qDebug("recalcStyle(%p)", this);
//     QTime qt;
//     qt.start();
    if (m_inStyleRecalc)
        return; // Guard against re-entrancy. -dwh

    m_inStyleRecalc = true;

    if( !m_render ) goto bail_out;

    if ( change == Force ) {
        RenderStyle* oldStyle = m_render->style();
        if ( oldStyle ) oldStyle->ref();
        RenderStyle* _style = new RenderStyle();
        _style->setDisplay(BLOCK);
        _style->setVisuallyOrdered( visuallyOrdered );
        // ### make the font stuff _really_ work!!!!

	khtml::FontDef fontDef;
	QFont f = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
	fontDef.family = f.family();
	fontDef.italic = f.italic();
	fontDef.weight = f.weight();
        if (m_view) {
            const KHTMLSettings *settings = m_view->part()->settings();
	    QString stdfont = settings->stdFontName();
	    if ( !stdfont.isEmpty() )
		fontDef.family = stdfont;

            fontDef.size = m_styleSelector->fontSizes()[3];
        }

        //qDebug() << "DocumentImpl::attach: setting to charset " << settings->charset();
        _style->setFontDef(fontDef);
	_style->htmlFont().update( 0 );
        if ( inCompatMode() )
            _style->setHtmlHacks(true); // enable html specific rendering tricks

        StyleChange ch = diff( _style, oldStyle );
        if(m_render && ch != NoChange)
            m_render->setStyle(_style);
	else
	    delete _style;

        if (oldStyle)
            oldStyle->deref();
    }

    NodeImpl *n;
    for (n = _first; n; n = n->nextSibling())
        if ( change>= Inherit || n->hasChangedChild() || n->changed() )
            n->recalcStyle( change );
    //qDebug() << "TIME: recalcStyle() dt=" << qt.elapsed();

    if (changed() && m_view)
	m_view->layout();

bail_out:
    setChanged( false );
    setHasChangedChild( false );
    setDocumentChanged( false );

    m_inStyleRecalc = false;
}

void DocumentImpl::updateRendering()
{
    if (!hasChangedChild()) return;

//     QTime time;
//     time.start();
//     qDebug() << "UPDATERENDERING: ";

    StyleChange change = NoChange;
#if 0
    if ( m_styleSelectorDirty ) {
	recalcStyleSelector();
	change = Force;
    }
#endif
    recalcStyle( change );

//    qDebug() << "UPDATERENDERING time used="<<time.elapsed();
}

void DocumentImpl::updateDocumentsRendering()
{
    if (!s_changedDocuments())
        return;

    while ( !s_changedDocuments()->isEmpty() ) {
        DocumentImpl* it = s_changedDocuments()->takeFirst();
        if (it->isDocumentChanged())
            it->updateRendering();
    }
}

void DocumentImpl::updateLayout()
{
    if (ElementImpl* oe = ownerElement())
        oe->document()->updateLayout();

    bool oldIgnore = m_ignorePendingStylesheets;

    if (!haveStylesheetsLoaded()) {
        m_ignorePendingStylesheets = true;
        updateStyleSelector();
    }

    updateRendering();

    // Only do a layout if changes have occurred that make it necessary.
    if (m_view && renderer() && renderer()->needsLayout())
	m_view->layout();

    m_ignorePendingStylesheets = oldIgnore;
}

void DocumentImpl::attach()
{
    assert(!attached());

    if ( m_view )
        setPaintDevice( m_view );

    if (!m_renderArena)
	m_renderArena.reset(new RenderArena());

    // Create the rendering tree
    assert(!m_styleSelector);
    m_styleSelector = new CSSStyleSelector( this, m_usersheet, m_styleSheets, m_url,
                                            !inCompatMode() );
    m_render = new (m_renderArena.get()) RenderCanvas(this, m_view);
    m_styleSelector->computeFontSizes(m_paintDevice->logicalDpiY(), m_view ? m_view->part()->fontScaleFactor() : 100);
    recalcStyle( Force );

    RenderObject* render = m_render;
    m_render = 0;

    NodeBaseImpl::attach();
    m_render = render;
}

void DocumentImpl::detach()
{
    RenderObject* render = m_render;

    // indicate destruction mode,  i.e. attached() but m_render == 0
    m_render = 0;

    delete m_tokenizer;
    m_tokenizer = 0;

    // Empty out these lists as a performance optimization
    m_imageLoadEventDispatchSoonList.clear();
    m_imageLoadEventDispatchingList.clear();
    NodeBaseImpl::detach();

    if ( render )
        render->detach();

    m_view = 0;

    m_renderArena.reset();
}

void DocumentImpl::setVisuallyOrdered()
{
    visuallyOrdered = true;
    if (m_render)
        m_render->style()->setVisuallyOrdered(true);
}

void DocumentImpl::setSelection(NodeImpl* s, int sp, NodeImpl* e, int ep)
{
    if ( m_render )
        static_cast<RenderCanvas*>(m_render)->setSelection(s->renderer(),sp,e->renderer(),ep);
}

void DocumentImpl::clearSelection()
{
    if ( m_render )
        static_cast<RenderCanvas*>(m_render)->clearSelection();
}

void DocumentImpl::updateSelection()
{
    if (!m_render)
        return;

    RenderCanvas *canvas = static_cast<RenderCanvas*>(m_render);
    Selection s = part()->caret();
    if (s.isEmpty() || s.state() == Selection::CARET) {
        canvas->clearSelection();
    }
    else {
        RenderObject *startRenderer = s.start().node() ? s.start().node()->renderer() : 0;
        RenderObject *endRenderer = s.end().node() ? s.end().node()->renderer() : 0;
        RenderPosition renderedStart = RenderPosition::fromDOMPosition(s.start());
        RenderPosition renderedEnd   = RenderPosition::fromDOMPosition(s.end());
        static_cast<RenderCanvas*>(m_render)->setSelection(startRenderer, renderedStart.renderedOffset(), endRenderer, renderedEnd.renderedOffset());
    }
}

khtml::Tokenizer *DocumentImpl::createTokenizer()
{
    return new khtml::XMLTokenizer(docPtr(),m_view);
}

int DocumentImpl::logicalDpiY()
{
    return m_paintDevice->logicalDpiY();
}

void DocumentImpl::open( bool clearEventListeners )
{
    if (parsing()) return;

    if (m_tokenizer)
        close();

    delete m_tokenizer;
    m_tokenizer = 0;

    KHTMLView* view = m_view;
    bool was_attached = attached();
    if ( was_attached )
        detach();

    removeChildren();
    childrenChanged(); // Reset m_documentElement, m_doctype
    delete m_styleSelector;
    m_styleSelector = 0;
    m_view = view;
    if ( was_attached )
        attach();

    if (clearEventListeners)
        windowEventTarget()->listenerList().clear();

    m_tokenizer = createTokenizer();
    //m_decoderMibEnum = 0;
    connect(m_tokenizer,SIGNAL(finishedParsing()),this,SIGNAL(finishedParsing()));
    m_tokenizer->begin();
}

HTMLElementImpl* DocumentImpl::body() const
{
    NodeImpl *de = documentElement();
    if (!de)
        return 0;

    // try to prefer a FRAMESET element over BODY
    NodeImpl* body = 0;
    for (NodeImpl* i = de->firstChild(); i; i = i->nextSibling()) {
        if (i->id() == ID_FRAMESET)
            return static_cast<HTMLElementImpl*>(i);

        if (i->id() == ID_BODY)
            body = i;
    }
    return static_cast<HTMLElementImpl *>(body);
}

void DocumentImpl::close(  )
{
    if (parsing() && hasVariableLength() && m_tokenizer) {
        m_tokenizer->finish();
    } else if (parsing() || !m_tokenizer)
        return;

    if ( m_render )
        m_render->close();

    // on an explicit document.close(), the tokenizer might still be waiting on scripts,
    // and in that case we don't want to destroy it because that will prevent the
    // scripts from getting processed.
    if (m_tokenizer && !m_tokenizer->isWaitingForScripts() && !m_tokenizer->isExecutingScript()) {
        delete m_tokenizer;
        m_tokenizer = 0;
    }

    if (m_view)
        m_view->part()->checkEmitLoadEvent();
}

void DocumentImpl::write( const DOMString &text )
{
    write(text.string());
}

void DocumentImpl::write( const QString &text )
{
    if (!m_tokenizer) {
        open();
        if (m_view)
            m_view->part()->resetFromScript();
        setHasVariableLength();
    }
    m_tokenizer->write(text, false);
}

void DocumentImpl::writeln( const DOMString &text )
{
    write(text);
    write(DOMString("\n"));
}

void DocumentImpl::finishParsing (  )
{
    if(m_tokenizer)
        m_tokenizer->finish();
}

QString DocumentImpl::completeURL(const QString& url) const
{
    return baseURL().resolved(QUrl(url)).toString();
}

void DocumentImpl::setUserStyleSheet( const QString& sheet )
{
    if ( m_usersheet != sheet ) {
        m_usersheet = sheet;
        updateStyleSelector();
    }
}

CSSStyleSheetImpl* DocumentImpl::elementSheet()
{
    if (!m_elemSheet) {
        m_elemSheet = new CSSStyleSheetImpl(this, baseURL().url() );
        m_elemSheet->ref();
    }
    return m_elemSheet;
}

void DocumentImpl::determineParseMode()
{
    // For XML documents, use strict parse mode
    pMode = Strict;
    hMode = XHtml;
    m_htmlCompat = false;
    // qDebug() << " using strict parseMode";
}

NodeImpl *DocumentImpl::nextFocusNode(NodeImpl *fromNode)
{
    short fromTabIndex;

    if (!fromNode) {
	// No starting node supplied; begin with the top of the document
	NodeImpl *n;

	int lowestTabIndex = SHRT_MAX + 1;
	for (n = this; n != 0; n = n->traverseNextNode()) {
	    if (n->isTabFocusable()) {
		if ((n->tabIndex() > 0) && (n->tabIndex() < lowestTabIndex))
		    lowestTabIndex = n->tabIndex();
	    }
	}

	if (lowestTabIndex == SHRT_MAX + 1)
	    lowestTabIndex = 0;

	// Go to the first node in the document that has the desired tab index
	for (n = this; n != 0; n = n->traverseNextNode()) {
	    if (n->isTabFocusable() && (n->tabIndex() == lowestTabIndex))
		return n;
	}

	return 0;
    }
    else {
	fromTabIndex = fromNode->tabIndex();
    }

    if (fromTabIndex == 0) {
	// Just need to find the next selectable node after fromNode (in document order) that doesn't have a tab index
	NodeImpl *n = fromNode->traverseNextNode();
	while (n && !(n->isTabFocusable() && n->tabIndex() == 0))
	    n = n->traverseNextNode();
	return n;
    }
    else {
	// Find the lowest tab index out of all the nodes except fromNode, that is greater than or equal to fromNode's
	// tab index. For nodes with the same tab index as fromNode, we are only interested in those that come after
	// fromNode in document order.
	// If we don't find a suitable tab index, the next focus node will be one with a tab index of 0.
	int lowestSuitableTabIndex = SHRT_MAX + 1;
	NodeImpl *n;

	bool reachedFromNode = false;
	for (n = this; n != 0; n = n->traverseNextNode()) {
	    if (n->isTabFocusable() &&
		((reachedFromNode && (n->tabIndex() >= fromTabIndex)) ||
		 (!reachedFromNode && (n->tabIndex() > fromTabIndex))) &&
		(n->tabIndex() < lowestSuitableTabIndex) &&
		(n != fromNode)) {

		// We found a selectable node with a tab index at least as high as fromNode's. Keep searching though,
		// as there may be another node which has a lower tab index but is still suitable for use.
		lowestSuitableTabIndex = n->tabIndex();
	    }

	    if (n == fromNode)
		reachedFromNode = true;
	}

	if (lowestSuitableTabIndex == SHRT_MAX + 1) {
	    // No next node with a tab index -> just take first node with tab index of 0
	    NodeImpl *n = this;
	    while (n && !(n->isTabFocusable() && n->tabIndex() == 0))
		n = n->traverseNextNode();
	    return n;
	}

	// Search forwards from fromNode
	for (n = fromNode->traverseNextNode(); n != 0; n = n->traverseNextNode()) {
	    if (n->isTabFocusable() && (n->tabIndex() == lowestSuitableTabIndex))
		return n;
	}

	// The next node isn't after fromNode, start from the beginning of the document
	for (n = this; n != fromNode; n = n->traverseNextNode()) {
	    if (n->isTabFocusable() && (n->tabIndex() == lowestSuitableTabIndex))
		return n;
	}

	assert(false); // should never get here
	return 0;
    }
}

NodeImpl *DocumentImpl::previousFocusNode(NodeImpl *fromNode)
{
    NodeImpl *lastNode = this;
    while (lastNode->lastChild())
	lastNode = lastNode->lastChild();

    if (!fromNode) {
	// No starting node supplied; begin with the very last node in the document
	NodeImpl *n;

	int highestTabIndex = 0;
	for (n = lastNode; n != 0; n = n->traversePreviousNode()) {
	    if (n->isTabFocusable()) {
		if (n->tabIndex() == 0)
		    return n;
		else if (n->tabIndex() > highestTabIndex)
		    highestTabIndex = n->tabIndex();
	    }
	}

	// No node with a tab index of 0; just go to the last node with the highest tab index
	for (n = lastNode; n != 0; n = n->traversePreviousNode()) {
	    if (n->isTabFocusable() && (n->tabIndex() == highestTabIndex))
		return n;
	}

	return 0;
    }
    else {
	short fromTabIndex = fromNode->tabIndex();

	if (fromTabIndex == 0) {
	    // Find the previous selectable node before fromNode (in document order) that doesn't have a tab index
	    NodeImpl *n = fromNode->traversePreviousNode();
	    while (n && !(n->isTabFocusable() && n->tabIndex() == 0))
		n = n->traversePreviousNode();
	    if (n)
		return n;

	    // No previous nodes with a 0 tab index, go to the last node in the document that has the highest tab index
	    int highestTabIndex = 0;
	    for (n = this; n != 0; n = n->traverseNextNode()) {
		if (n->isTabFocusable() && (n->tabIndex() > highestTabIndex))
		    highestTabIndex = n->tabIndex();
	    }

	    if (highestTabIndex == 0)
		return 0;

	    for (n = lastNode; n != 0; n = n->traversePreviousNode()) {
		if (n->isTabFocusable() && (n->tabIndex() == highestTabIndex))
		    return n;
	    }

	    assert(false); // should never get here
	    return 0;
	}
	else {
	    // Find the lowest tab index out of all the nodes except fromNode, that is less than or equal to fromNode's
	    // tab index. For nodes with the same tab index as fromNode, we are only interested in those before
	    // fromNode.
	    // If we don't find a suitable tab index, then there will be no previous focus node.
	    short highestSuitableTabIndex = 0;
	    NodeImpl *n;

	    bool reachedFromNode = false;
	    for (n = this; n != 0; n = n->traverseNextNode()) {
		if (n->isTabFocusable() &&
		    ((!reachedFromNode && (n->tabIndex() <= fromTabIndex)) ||
		     (reachedFromNode && (n->tabIndex() < fromTabIndex)))  &&
		    (n->tabIndex() > highestSuitableTabIndex) &&
		    (n != fromNode)) {

		    // We found a selectable node with a tab index no higher than fromNode's. Keep searching though, as
		    // there may be another node which has a higher tab index but is still suitable for use.
		    highestSuitableTabIndex = n->tabIndex();
		}

		if (n == fromNode)
		    reachedFromNode = true;
	    }

	    if (highestSuitableTabIndex == 0) {
		// No previous node with a tab index. Since the order specified by HTML is nodes with tab index > 0
		// first, this means that there is no previous node.
		return 0;
	    }

	    // Search backwards from fromNode
	    for (n = fromNode->traversePreviousNode(); n != 0; n = n->traversePreviousNode()) {
		if (n->isTabFocusable() && (n->tabIndex() == highestSuitableTabIndex))
		    return n;
	    }
	    // The previous node isn't before fromNode, start from the end of the document
	    for (n = lastNode; n != fromNode; n = n->traversePreviousNode()) {
		if (n->isTabFocusable() && (n->tabIndex() == highestSuitableTabIndex))
		    return n;
	    }

	    assert(false); // should never get here
	    return 0;
	}
    }
}

ElementImpl* DocumentImpl::findAccessKeyElement(QChar c)
{
    c = c.toUpper();
    for( NodeImpl* n = this;
         n != NULL;
         n = n->traverseNextNode()) {
        if( n->isElementNode()) {
            ElementImpl* en = static_cast< ElementImpl* >( n );
            DOMString s = en->getAttribute( ATTR_ACCESSKEY );
            if( s.length() == 1
                && s[ 0 ].toUpper() == c )
                return en;
        }
    }
    return NULL;
}

int DocumentImpl::nodeAbsIndex(NodeImpl *node)
{
    assert(node->document() == this);

    int absIndex = 0;
    for (NodeImpl *n = node; n && n != this; n = n->traversePreviousNode())
	absIndex++;
    return absIndex;
}

NodeImpl *DocumentImpl::nodeWithAbsIndex(int absIndex)
{
    NodeImpl *n = this;
    for (int i = 0; n && (i < absIndex); i++) {
	n = n->traverseNextNode();
    }
    return n;
}

void DocumentImpl::processHttpEquiv(const DOMString &equiv, const DOMString &content)
{
    assert(!equiv.isNull() && !content.isNull());

    KHTMLView *v = document()->view();

    if(strcasecmp(equiv, "refresh") == 0 && v && v->part()->metaRefreshEnabled())
    {
        // get delay and url
        QString str = content.string().trimmed();
        int pos = str.indexOf(QRegExp("[;,]"));
        if ( pos == -1 )
            pos = str.indexOf(QRegExp("[ \t]"));

        bool ok = false;
	int delay = qMax( 0, content.implementation()->toInt(&ok) );
        if ( !ok && str.length() && str[0] == '.' )
            ok = true;

        if (pos == -1) // There can be no url (David)
        {
            if(ok)
                v->part()->scheduleRedirection(delay, v->part()->url().toString());
        } else {
            pos++;
            while(pos < (int)str.length() && str[pos].isSpace()) pos++;
            str = str.mid(pos);
            if(str.indexOf("url", 0, Qt::CaseInsensitive ) == 0)  str = str.mid(3);
            str = str.trimmed();
            if ( str.length() && str[0] == '=' ) str = str.mid( 1 ).trimmed();
            while(str.length() &&
                  (str[str.length()-1] == ';' || str[str.length()-1] == ','))
                str.resize(str.length()-1);
            str = parseURL( DOMString(str) ).string();
            QString newURL = document()->completeURL( str );
            if ( ok )
                v->part()->scheduleRedirection(delay, newURL,  delay < 2 || newURL == URL().url());
        }
    }
    else if(strcasecmp(equiv, "expires") == 0)
    {
        if (m_docLoader) {
            QString str = content.string().trimmed();
            //QDateTime can't convert from a RFCDate format string
            QDateTime expire_date = QDateTime::fromString(str, Qt::RFC2822Date);

            if (!expire_date.isValid()) {
                qint64 seconds = str.toLongLong();
                if (seconds != 0) {
                    m_docLoader->setRelativeExpireDate(seconds);
                } else {
                    expire_date = QDateTime::currentDateTime(); // expire now
                    m_docLoader->setExpireDate(expire_date);
                }
            }
        }
    }
    else if(v && (strcasecmp(equiv, "pragma") == 0 || strcasecmp(equiv, "cache-control") == 0))
    {
        QString str = content.string().toLower().trimmed();
        QUrl url = v->part()->url();
        if ((str == "no-cache") && url.scheme().startsWith(QLatin1String("http")))
        {
            KIO::http_update_cache(url, true, QDateTime::fromTime_t(0));
        }
    }
    else if( (strcasecmp(equiv, "set-cookie") == 0))
    {
        // ### make setCookie work on XML documents too; e.g. in case of <html:meta .....>
        HTMLDocumentImpl *d = static_cast<HTMLDocumentImpl *>(this);
        d->setCookie(content);
    }
    else if (strcasecmp(equiv, "default-style") == 0) {
        // HTML 4.0 14.3.2
        // http://www.hixie.ch/tests/evil/css/import/main/preferred.html
        m_preferredStylesheetSet = content;
        updateStyleSelector();
    }
    else if (strcasecmp(equiv, "content-language") == 0) {
        m_contentLanguage = content.string();
    }
}

bool DocumentImpl::prepareMouseEvent( bool readonly, int _x, int _y, MouseEvent *ev )
{
    if ( m_render ) {
        assert(m_render->isCanvas());
        RenderObject::NodeInfo renderInfo(readonly, ev->type == MousePress);
        bool isInside = m_render->layer()->nodeAtPoint(renderInfo, _x, _y);
        ev->innerNode = renderInfo.innerNode();
	ev->innerNonSharedNode = renderInfo.innerNonSharedNode();

        if (renderInfo.URLElement()) {
            assert(renderInfo.URLElement()->isElementNode());
            //qDebug("urlnode: %s  (%d)", getTagName(renderInfo.URLElement()->id()).string().toLatin1().constData(), renderInfo.URLElement()->id());

            ElementImpl* e =  static_cast<ElementImpl*>(renderInfo.URLElement());
            DOMString href = khtml::parseURL(e->getAttribute(ATTR_HREF));
            DOMString target = e->getAttribute(ATTR_TARGET);

            if (!target.isNull() && !href.isNull()) {
                ev->target = target;
                ev->url = href;
            }
            else
                ev->url = href;
        }

        if (!readonly)
            updateRendering();

        return isInside;
    }


    return false;
}


// DOM Section 1.1.1
bool DocumentImpl::childTypeAllowed( unsigned short type )
{
    switch (type) {
        case Node::ATTRIBUTE_NODE:
        case Node::CDATA_SECTION_NODE:
        case Node::DOCUMENT_FRAGMENT_NODE:
        case Node::DOCUMENT_NODE:
        case Node::ENTITY_NODE:
        case Node::ENTITY_REFERENCE_NODE:
        case Node::NOTATION_NODE:
        case Node::TEXT_NODE:
//        case Node::XPATH_NAMESPACE_NODE:
            return false;
        case Node::COMMENT_NODE:
        case Node::PROCESSING_INSTRUCTION_NODE:
            return true;
        case Node::DOCUMENT_TYPE_NODE:
        case Node::ELEMENT_NODE:
            // Documents may contain no more than one of each of these.
            // (One Element and one DocumentType.)
            for (NodeImpl* c = firstChild(); c; c = c->nextSibling())
                if (c->nodeType() == type)
                    return false;
            return true;
    }
    return false;
}

WTF::PassRefPtr<NodeImpl> DocumentImpl::cloneNode ( bool deep )
{
#if 0
    NodeImpl *dtn = m_doctype->cloneNode(deep);
    DocumentTypeImpl *dt = static_cast<DocumentTypeImpl*>(dtn);
#endif

    int exceptioncode;
    WTF::RefPtr<NodeImpl> clone = DOMImplementationImpl::createDocument("",
							   "",
                                                           0, 0,
                                                           exceptioncode);
    assert( exceptioncode == 0 );

    // ### attributes, styles, ...

    if (deep)
	cloneChildNodes(clone.get());

    return clone;
}

// This method is called whenever a top-level stylesheet has finished loading.
void DocumentImpl::styleSheetLoaded()
{
  // Make sure we knew this sheet was pending, and that our count isn't out of sync.
  assert(m_pendingStylesheets > 0);

  m_pendingStylesheets--;
  updateStyleSelector();
  if (!m_pendingStylesheets && m_tokenizer)
      m_tokenizer->executeScriptsWaitingForStylesheets();
}

void DocumentImpl::addPendingSheet()
{
     m_pendingStylesheets++;
}

DOMString DocumentImpl::selectedStylesheetSet() const
{
    if (!view()) return DOMString();

    return view()->part()->d->m_sheetUsed;
}

void DocumentImpl::setSelectedStylesheetSet(const DOMString& s)
{
    // this code is evil
    if (view() && view()->part()->d->m_sheetUsed != s.string()) {
        view()->part()->d->m_sheetUsed = s.string();
        updateStyleSelector();
    }
}

void DocumentImpl::addStyleSheet(StyleSheetImpl *sheet, int *exceptioncode)
{
    int excode = 0;

    if (!m_addedStyleSheets) {
        m_addedStyleSheets = new StyleSheetListImpl;
        m_addedStyleSheets->ref();
    }

    m_addedStyleSheets->add(sheet);
    if (sheet->isCSSStyleSheet()) updateStyleSelector();

    if (exceptioncode) *exceptioncode = excode;
}

void DocumentImpl::removeStyleSheet(StyleSheetImpl *sheet, int *exceptioncode)
{
    int excode = 0;
    bool removed = false;
    bool is_css = sheet->isCSSStyleSheet();

    if (m_addedStyleSheets) {
        bool in_main_list = !sheet->hasOneRef();
        removed = m_addedStyleSheets->styleSheets.removeAll(sheet);
        sheet->deref();

        if (m_addedStyleSheets->styleSheets.count() == 0) {
            bool reset = m_addedStyleSheets->hasOneRef();
            m_addedStyleSheets->deref();
            if (reset) m_addedStyleSheets = 0;
        }

        // remove from main list, too
        if (in_main_list) m_styleSheets->remove(sheet);
    }

    if (removed) {
        if (is_css) updateStyleSelector();
    } else
        excode = DOMException::NOT_FOUND_ERR;

    if (exceptioncode) *exceptioncode = excode;
}

void DocumentImpl::updateStyleSelector(bool shallow)
{
//    qDebug() << "PENDING " << m_pendingStylesheets;

    // Don't bother updating, since we haven't loaded all our style info yet.
    if (m_pendingStylesheets > 0) {
        // ... however, if the list of stylesheets changed, mark it as dirty
        // so DOM ops can get an up-to-date version.
        if (!shallow)
            m_styleSheetListDirty = true;
        return;
    }
    
    if (!shallow)
        rebuildStyleSheetList();

    rebuildStyleSelector();
    
    recalcStyle(Force);
#if 0

    m_styleSelectorDirty = true;
#endif
    if ( renderer() )
        renderer()->setNeedsLayoutAndMinMaxRecalc();
}

bool DocumentImpl::readyForLayout() const
{
    return renderer() && haveStylesheetsLoaded() && (!isHTMLDocument() || (body() && body()->renderer()));
}

void DocumentImpl::rebuildStyleSheetList(bool force)
{
    if ( !m_render || !attached() ) {
        // Unless we're forced due to CSS DOM ops, we don't have to compute info
        // when there is nothing to display
        if (!force) {
            m_styleSheetListDirty = true;
            return;
        }
    }

    // Mark us as clean, as we can call add on the list below, forcing us to re-enter
    m_styleSheetListDirty = false;

    QList<StyleSheetImpl*> oldStyleSheets = m_styleSheets->styleSheets;
    m_styleSheets->styleSheets.clear();
    QString sheetUsed = view() ? view()->part()->d->m_sheetUsed.replace("&&", "&") : QString();
    bool autoselect = sheetUsed.isEmpty();
    if (autoselect && !m_preferredStylesheetSet.isEmpty())
        sheetUsed = m_preferredStylesheetSet.string();
    NodeImpl *n;
    for (int i=0 ; i<2 ; i++) {
        m_availableSheets.clear();
        m_availableSheets << i18n("Basic Page Style");
        bool canResetSheet = false;

        QString title;
        for (n = this; n; n = n->traverseNextNode()) {
            StyleSheetImpl *sheet = 0;

            if (n->nodeType() == Node::PROCESSING_INSTRUCTION_NODE)
            {
                // Processing instruction (XML documents only)
                ProcessingInstructionImpl* pi = static_cast<ProcessingInstructionImpl*>(n);
                sheet = pi->sheet();
                if (!sheet && !pi->localHref().isEmpty())
                {
                    // Processing instruction with reference to an element in this document - e.g.
                    // <?xml-stylesheet href="#mystyle">, with the element
                    // <foo id="mystyle">heading { color: red; }</foo> at some location in
                    // the document
                    ElementImpl* elem = getElementById(pi->localHref());
                    if (elem) {
                        DOMString sheetText("");
                        NodeImpl *c;
                        for (c = elem->firstChild(); c; c = c->nextSibling()) {
                            if (c->nodeType() == Node::TEXT_NODE || c->nodeType() == Node::CDATA_SECTION_NODE)
                                sheetText += c->nodeValue();
                        }

                        CSSStyleSheetImpl *cssSheet = new CSSStyleSheetImpl(this);
                        cssSheet->parseString(sheetText);
                        pi->setStyleSheet(cssSheet);
                        sheet = cssSheet;
                    }
                }
                if (sheet) {
                    title = sheet->title().string();
                    if ((autoselect || title != sheetUsed) && sheet->disabled()) {
                        sheet = 0;
                    } else if (!title.isEmpty() && !pi->isAlternate() && sheetUsed.isEmpty()) {
                        sheetUsed = title;
                        sheet->setDisabled(false);
                    }
                }
            }
            else if (n->isHTMLElement() && ( n->id() == ID_LINK || n->id() == ID_STYLE) ) {
                if ( n->id() == ID_LINK ) {
                    HTMLLinkElementImpl* l = static_cast<HTMLLinkElementImpl*>(n);
                    if (l->isCSSStyleSheet()) {
                        sheet = l->sheet();

                        if (sheet || l->isLoading() || l->isAlternate() )
                            title = l->getAttribute(ATTR_TITLE).string();

			if ((autoselect || title != sheetUsed) && l->isDisabled()) {
                            sheet = 0;
                        } else if (!title.isEmpty() && !l->isAlternate() && sheetUsed.isEmpty()) {
                            sheetUsed = title;
                            l->setDisabled(false);
                        }
                    }
                }
                else {
                    // <STYLE> element
                    HTMLStyleElementImpl* s = static_cast<HTMLStyleElementImpl*>(n);
                    if (!s->isLoading()) {
                        sheet = s->sheet();
                        if (sheet) title = s->getAttribute(ATTR_TITLE).string();
                    }
                    if (!title.isEmpty() && sheetUsed.isEmpty())
                        sheetUsed = title;
                }
            }
            else if (n->isHTMLElement() && n->id() == ID_BODY) {
                // <BODY> element (doesn't contain styles as such but vlink="..." and friends
                // are treated as style declarations)
                sheet = static_cast<HTMLBodyElementImpl*>(n)->sheet();
            }

            if ( !title.isEmpty() ) {
                if ( title != sheetUsed )
                    sheet = 0; // don't use it
                title = title.replace('&',  "&&");
                if ( !m_availableSheets.contains( title ) )
                    m_availableSheets.append( title );
                title.clear();
            }

            if (sheet) {
                sheet->ref();
                m_styleSheets->styleSheets.append(sheet);
            }

            // For HTML documents, stylesheets are not allowed within/after the <BODY> tag. So we
            // can stop searching here.
            if (isHTMLDocument() && n->id() == ID_BODY) {
                canResetSheet = !canResetSheet;
                break;
            }
        }

        // we're done if we don't select an alternative sheet
        // or we found the sheet we selected
        if (sheetUsed.isEmpty() ||
            (!canResetSheet && tokenizer()) ||
            m_availableSheets.contains(sheetUsed)) {
            break;
        }

        // the alternative sheet we used doesn't exist anymore
        // so try from scratch again
        if (view())
            view()->part()->d->m_sheetUsed.clear();
        if (!m_preferredStylesheetSet.isEmpty() && !(sheetUsed == m_preferredStylesheetSet))
            sheetUsed = m_preferredStylesheetSet.string();
        else
            sheetUsed.clear();
        autoselect = true;
    }

    // Include programmatically added style sheets
    if (m_addedStyleSheets) {
        foreach (StyleSheetImpl* sh, m_addedStyleSheets->styleSheets) {
            if (sh->isCSSStyleSheet() && !sh->disabled())
                m_styleSheets->add(sh);
        }
    }

    // De-reference all the stylesheets in the old list
    foreach ( StyleSheetImpl* sh, oldStyleSheets)
        sh->deref();
}

void DocumentImpl::rebuildStyleSelector()
{
    if ( !m_render || !attached() )
        return;

    // Create a new style selector
    delete m_styleSelector;
    QString usersheet = m_usersheet;
    if ( m_view && m_view->mediaType() == "print" )
	usersheet += m_printSheet;
    m_styleSelector = new CSSStyleSelector( this, usersheet, m_styleSheets, m_url,
                                            !inCompatMode() );

    m_styleSelectorDirty = false;
}

void DocumentImpl::setBaseURL(const QUrl& _baseURL)
{
    m_baseURL = _baseURL;
    if (m_elemSheet)
        m_elemSheet->setHref( baseURL().toString() );
}

void DocumentImpl::setHoverNode(NodeImpl *newHoverNode)
{
    NodeImpl* oldHoverNode = m_hoverNode;
    if (newHoverNode ) newHoverNode->ref();
    m_hoverNode = newHoverNode;
    if ( oldHoverNode ) oldHoverNode->deref();
}

void DocumentImpl::setActiveNode(NodeImpl* newActiveNode)
{
    NodeImpl* oldActiveNode = m_activeNode;
    if (newActiveNode ) newActiveNode->ref();
    m_activeNode = newActiveNode;
    if ( oldActiveNode ) oldActiveNode->deref();
}

void DocumentImpl::quietResetFocus()
{
    assert(m_focusNode != this);
    if (m_focusNode) {
        if (m_focusNode->active())
            setActiveNode(0);

        m_focusNode->setFocus(false);
        m_focusNode->deref();
    }
    m_focusNode = 0;

    //We're blurring. Better clear the Qt focus/give it to the view...
    if (view())
        view()->setFocus();
}

void DocumentImpl::setFocusNode(NodeImpl *newFocusNode)
{
    // don't process focus changes while detaching
    if( !m_render ) return;

    // See if the new node is really focusable. It might not be
    // if focus() was called explicitly.
    if (newFocusNode && !newFocusNode->isFocusable())
        return;

    // Make sure newFocusNode is actually in this document
    if (newFocusNode && (newFocusNode->document() != this))
        return;

    if (m_focusNode != newFocusNode) {
        NodeImpl *oldFocusNode = m_focusNode;

        // We are blurring, so m_focusNode ATM is 0; this is observable to the
        // event handlers.
        m_focusNode = 0;
        
        // Remove focus from the existing focus node (if any)
        if (oldFocusNode) {
            if (oldFocusNode->active())
                oldFocusNode->setActive(false);

            oldFocusNode->setFocus(false);
            if (oldFocusNode->renderer() && oldFocusNode->renderer()->isWidget()) {
                // Editable widgets may need to dispatch CHANGE_EVENT
                RenderWidget* rw = static_cast<RenderWidget*>(oldFocusNode->renderer());
                if (rw->isRedirectedWidget())
                    rw->handleFocusOut();
            }

            oldFocusNode->dispatchHTMLEvent(EventImpl::BLUR_EVENT,false,false);
            oldFocusNode->dispatchUIEvent(EventImpl::DOMFOCUSOUT_EVENT);

            if ((oldFocusNode == this) && oldFocusNode->hasOneRef()) {
                oldFocusNode->deref(); // may delete this, if there are not kids keeping it alive...
                                       // so we better not add any.
                return;
            }
	    else {
                oldFocusNode->deref();
            }
        }

        // It's possible that one of the blur, etc. handlers has already set focus.
        // in that case, we don't want to override it.
        if (!m_focusNode && newFocusNode) {
            // Set focus on the new node
            m_focusNode = newFocusNode;
            m_focusNode->ref();
            m_focusNode->dispatchHTMLEvent(EventImpl::FOCUS_EVENT,false,false);
            if (m_focusNode != newFocusNode) return;
            m_focusNode->dispatchUIEvent(EventImpl::DOMFOCUSIN_EVENT);
            if (m_focusNode != newFocusNode) return;
            m_focusNode->setFocus();
            if (m_focusNode != newFocusNode) return;

            // eww, I suck. set the qt focus correctly
            // ### find a better place in the code for this
            if (view()) {
                if (!m_focusNode->renderer() || !m_focusNode->renderer()->isWidget())
                    view()->setFocus();
                else if (static_cast<RenderWidget*>(m_focusNode->renderer())->widget())
                {
                    if (view()->isVisible())
                        static_cast<RenderWidget*>(m_focusNode->renderer())->widget()->setFocus();
                }
            }
        } else {
            //We're blurring. Better clear the Qt focus/give it to the view...
            if (view())
                view()->setFocus();
        }

        updateRendering();
    }
}

void DocumentImpl::setCSSTarget(NodeImpl* n)
{
    if (n == m_cssTarget)
        return;

    if (m_cssTarget) {
        m_cssTarget->setChanged();
        m_cssTarget->deref();
    }
    m_cssTarget = n;
    if (n) {
        n->setChanged();
        n->ref();
    }
}

void DocumentImpl::attachNodeIterator(NodeIteratorImpl *ni)
{
    m_nodeIterators.append(ni);
}

void DocumentImpl::detachNodeIterator(NodeIteratorImpl *ni)
{
    int i = m_nodeIterators.indexOf(ni);
    if (i != -1)
        m_nodeIterators.removeAt(i);
}

void DocumentImpl::notifyBeforeNodeRemoval(NodeImpl *n)
{
    QListIterator<NodeIteratorImpl*> it(m_nodeIterators);
    while (it.hasNext())
        it.next()->notifyBeforeNodeRemoval(n);
}

bool DocumentImpl::isURLAllowed(const QString& url) const
{
    KHTMLPart *thisPart = part();

    QUrl newURL(completeURL(url));
    newURL.setFragment(QString());

    if (KHTMLGlobal::defaultHTMLSettings()->isAdFiltered( newURL.url() ))
        return false;

    // Prohibit non-file URLs if we are asked to.
    if (!thisPart || ( thisPart->onlyLocalReferences() && newURL.scheme() != "file" && newURL.scheme() != "data" ))
        return false;

    // do we allow this suburl ?
    if (newURL.scheme() != "javascript" && !KUrlAuthorized::authorizeUrlAction("redirect", thisPart->url(), newURL))
        return false;

    // We allow one level of self-reference because some sites depend on that.
    // But we don't allow more than one.
    bool foundSelfReference = false;
    for (KHTMLPart *part = thisPart; part; part = part->parentPart()) {
        QUrl partURL = part->url();
        partURL.setFragment(QString());
        if (partURL == newURL) {
            if (foundSelfReference)
                return false;
            foundSelfReference = true;
        }
    }

    return true;
}

void DocumentImpl::setDesignMode(bool b)
{
    if (part())
         part()->setEditable(b);
}

bool DocumentImpl::designMode() const
{
    return part() ? part()->isEditable() : false;
}

EventImpl *DocumentImpl::createEvent(const DOMString &eventType, int &exceptioncode)
{
    if (eventType == "UIEvents" || eventType == "UIEvent")
        return new UIEventImpl();
    else if (eventType == "MouseEvents" || eventType == "MouseEvent")
        return new MouseEventImpl();
    else if (eventType == "TextEvent")
        return new TextEventImpl();
    else if (eventType == "KeyboardEvent")
        return new KeyboardEventImpl();
    else if (eventType == "MutationEvents" || eventType == "MutationEvent")
        return new MutationEventImpl();
    else if (eventType == "HTMLEvents" || eventType == "Events" ||
             eventType == "HTMLEvent"  || eventType == "Event")
        return new EventImpl();
    else {
        exceptioncode = DOMException::NOT_SUPPORTED_ERR;
        return 0;
    }
}

CSSStyleDeclarationImpl *DocumentImpl::getOverrideStyle(ElementImpl* /*elt*/, DOMStringImpl* /*pseudoElt*/)
{
    return 0; // ###
}

void DocumentImpl::abort()
{
    if (m_inSyncLoad) {
        assert(m_inSyncLoad->isRunning());
        m_inSyncLoad->exit();
    }

    if (m_loadingXMLDoc)
	m_loadingXMLDoc->deref(this);
    m_loadingXMLDoc = 0;
}

void DocumentImpl::load(const DOMString &uri)
{
    if (m_inSyncLoad) {
        assert(m_inSyncLoad->isRunning());
        m_inSyncLoad->exit();
    }

    m_hadLoadError = false;
    if (m_loadingXMLDoc)
	m_loadingXMLDoc->deref(this);

    // Use the document loader to retrieve the XML file. We use CachedCSSStyleSheet because
    // this is an easy way to retrieve an arbitrary text file... it is not specific to
    // stylesheets.

    // ### Note: By loading the XML document this way we do not get the proper decoding
    // of the data retrieved from the server based on the character set, as happens with
    // HTML files. Need to look into a way of using the decoder in CachedCSSStyleSheet.
    m_docLoading = true;
    m_loadingXMLDoc = m_docLoader->requestStyleSheet(uri.string(),QString(),"text/xml");

    if (!m_loadingXMLDoc) {
	m_docLoading = false;
	return;
    }

    m_loadingXMLDoc->ref(this);

    if (!m_async && m_docLoading) {
        assert(!m_inSyncLoad);
	m_inSyncLoad = new QEventLoop();
	m_inSyncLoad->exec();
	// returning from event loop:
	assert(!m_inSyncLoad->isRunning());
	delete m_inSyncLoad;
	m_inSyncLoad = 0;
    }
}

void DocumentImpl::loadXML(const DOMString &source)
{
    open(false);
    write(source);
    finishParsing();
    close();
    dispatchHTMLEvent(EventImpl::LOAD_EVENT,false,false);
}

void DocumentImpl::setStyleSheet(const DOM::DOMString &url, const DOM::DOMString &sheet, const DOM::DOMString &/*charset*/, const DOM::DOMString &mimetype)
{
    if (!m_hadLoadError) {
	m_url = QUrl(url.string());
	loadXML(khtml::isAcceptableCSSMimetype(mimetype) ? sheet : "");
    }

    m_docLoading = false;
    if (m_inSyncLoad) {
        assert(m_inSyncLoad->isRunning());
        m_inSyncLoad->exit();
    }

    assert(m_loadingXMLDoc != 0);
    m_loadingXMLDoc->deref(this);
    m_loadingXMLDoc = 0;
}

void DocumentImpl::error(int err, const QString &text)
{
    m_docLoading = false;
    if (m_inSyncLoad) {
        assert(m_inSyncLoad->isRunning());
        m_inSyncLoad->exit();
    }

    m_hadLoadError = true;

    int exceptioncode = 0;
    EventImpl *evt = new EventImpl(EventImpl::ERROR_EVENT,false,false);
    if (err != 0)
      evt->setMessage(KIO::buildErrorString(err,text));
    else
      evt->setMessage(text);
    evt->ref();
    dispatchEvent(evt,exceptioncode,true);
    evt->deref();

    assert(m_loadingXMLDoc != 0);
    m_loadingXMLDoc->deref(this);
    m_loadingXMLDoc = 0;
}

void DocumentImpl::defaultEventHandler(EventImpl *evt)
{
    if ( evt->id() == EventImpl::KHTML_CONTENTLOADED_EVENT && !evt->propagationStopped() && !evt->defaultPrevented() )
        contentLoaded();
}

void DocumentImpl::setHTMLWindowEventListener(EventName id, EventListener *listener)
{
    windowEventTarget()->listenerList().setHTMLEventListener(id, listener);
}

void DocumentImpl::setHTMLWindowEventListener(unsigned id, EventListener *listener)
{
    windowEventTarget()->listenerList().setHTMLEventListener(EventName::fromId(id), listener);
}

EventListener *DocumentImpl::getHTMLWindowEventListener(EventName id)
{
    return windowEventTarget()->listenerList().getHTMLEventListener(id);
}

EventListener *DocumentImpl::getHTMLWindowEventListener(unsigned id)
{
    return windowEventTarget()->listenerList().getHTMLEventListener(EventName::fromId(id));
}

void DocumentImpl::addWindowEventListener(EventName id, EventListener *listener, const bool useCapture)
{
    windowEventTarget()->listenerList().addEventListener(id, listener, useCapture);
}

void DocumentImpl::removeWindowEventListener(EventName id, EventListener *listener, bool useCapture)
{
    windowEventTarget()->listenerList().removeEventListener(id, listener, useCapture);
}

bool DocumentImpl::hasWindowEventListener(EventName id)
{
    return windowEventTarget()->listenerList().hasEventListener(id);
}

EventListener *DocumentImpl::createHTMLEventListener(const QString& code, const QString& name, NodeImpl* node)
{
    return part() ? part()->createHTMLEventListener(code, name, node) : 0;
}

void DocumentImpl::dispatchImageLoadEventSoon(HTMLImageElementImpl *image)
{
    m_imageLoadEventDispatchSoonList.append(image);
    if (!m_imageLoadEventTimer) {
        m_imageLoadEventTimer = startTimer(0);
    }
}

void DocumentImpl::removeImage(HTMLImageElementImpl *image)
{
    // Remove instances of this image from both lists.
    m_imageLoadEventDispatchSoonList.removeAll(image);
    m_imageLoadEventDispatchingList.removeAll(image);
    if (m_imageLoadEventDispatchSoonList.isEmpty() && m_imageLoadEventTimer) {
        killTimer(m_imageLoadEventTimer);
        m_imageLoadEventTimer = 0;
    }
}

void DocumentImpl::dispatchImageLoadEventsNow()
{
    // need to avoid re-entering this function; if new dispatches are
    // scheduled before the parent finishes processing the list, they
    // will set a timer and eventually be processed
    if (!m_imageLoadEventDispatchingList.isEmpty()) {
        return;
    }

    if (m_imageLoadEventTimer) {
        killTimer(m_imageLoadEventTimer);
        m_imageLoadEventTimer = 0;
    }

    m_imageLoadEventDispatchingList = m_imageLoadEventDispatchSoonList;
    m_imageLoadEventDispatchSoonList.clear();
    while (!m_imageLoadEventDispatchingList.isEmpty())
        m_imageLoadEventDispatchingList.takeFirst()->dispatchLoadEvent();
    m_imageLoadEventDispatchingList.clear();
}

void DocumentImpl::timerEvent(QTimerEvent *e)
{
    assert(e->timerId() == m_imageLoadEventTimer);
    Q_UNUSED(e);
    dispatchImageLoadEventsNow();
}

/*void DocumentImpl::setDecoderCodec(const QTextCodec *codec)
{
    m_decoderMibEnum = codec->mibEnum();
}*/

HTMLPartContainerElementImpl *DocumentImpl::ownerElement() const
{
    KHTMLPart *childPart = part();
    if (!childPart)
        return 0;
    ChildFrame *childFrame = childPart->d->m_frame;
    if (!childFrame)
        return 0;
    return childFrame->m_partContainerElement.data();
}

khtml::SecurityOrigin* DocumentImpl::origin() const
{
    if (!m_origin)
        m_origin = SecurityOrigin::create(URL());
    return m_origin.get();
}

void DocumentImpl::setOrigin(khtml::SecurityOrigin* newOrigin)
{
    assert(origin()->isEmpty());
    m_origin = newOrigin;
}
    
DOMString DocumentImpl::domain() const
{
    return origin()->domain();
}

void DocumentImpl::setDomain(const DOMString &newDomain)
{
    // ### this test really should move to SecurityOrigin..
    DOMString oldDomain = origin()->domain();

    // Both NS and IE specify that changing the domain is only allowed when
    // the new domain is a suffix of the old domain.
    int oldLength = oldDomain.length();
    int newLength = newDomain.length();
    if ( newLength < oldLength ) { // e.g. newDomain=kde.org (7) and m_domain=www.kde.org (11)
        DOMString test = oldDomain.copy();
        DOMString reference = newDomain.lower();
        if ( test[oldLength - newLength - 1] == '.' ) // Check that it's a subdomain, not e.g. "de.org"
        {
            test.remove( 0, oldLength - newLength ); // now test is "kde.org" from m_domain
            if ( test == reference )                 // and we check that it's the same thing as newDomain
                m_origin->setDomainFromDOM( reference.string() );
        }
    } else if ( oldLength == newLength ) {
        // It's OK and not a no-op to set the domain to the present one:
        // we want to set the 'set from DOM' bit in that case
        DOMString reference = newDomain.lower();
        if ( oldDomain.lower() == reference )
            m_origin->setDomainFromDOM( reference.string() );
    }
}

DOMString DocumentImpl::toString() const
{
    DOMString result;

    for (NodeImpl *child = firstChild(); child != NULL; child = child->nextSibling()) {
        result += child->toString();
    }

    return result;
}

void DOM::DocumentImpl::setRestoreState( const QStringList &s)
{
    m_state = s;
    m_stateRestorePos = 0;
}

KHTMLView* DOM::DocumentImpl::view() const
{
    return m_view;
}

KHTMLPart* DOM::DocumentImpl::part() const
{
    // ### TODO: make this independent from a KHTMLView one day.
    return view() ? view()->part() : 0;
}

DynamicNodeListImpl::Cache* DOM::DocumentImpl::acquireCachedNodeListInfo(
       DynamicNodeListImpl::CacheFactory* factory, NodeImpl* base, int type)
{
    //### might want to flush the dict when the version number
    //changes
    DynamicNodeListImpl::CacheKey key(base, type);

    //Check to see if we have this sort of item cached.
    DynamicNodeListImpl::Cache* cached =
        (type == DynamicNodeListImpl::UNCACHEABLE) ? 0 : m_nodeListCache.value(key.hash());

    if (cached) {
        if (cached->key == key) {
            cached->ref(); //Add the nodelist's reference
            return cached;
        } else {
            //Conflict. Drop our reference to the old item.
            cached->deref();
        }
    }

    //Nothing to reuse, make a new item.
    DynamicNodeListImpl::Cache* newInfo = factory();
    newInfo->key = key;
    newInfo->clear(this);
    newInfo->ref(); //Add the nodelist's reference

    if (type != DynamicNodeListImpl::UNCACHEABLE) {
        newInfo->ref(); //Add the cache's reference
        m_nodeListCache.insert(key.hash(), newInfo);
    }

    return newInfo;
}

void DOM::DocumentImpl::releaseCachedNodeListInfo(DynamicNodeListImpl::Cache* entry)
{
    entry->deref();
}

bool DOM::DocumentImpl::isSVGDocument() const
{
    return (documentElement()->id() == WebCore::SVGNames::svgTag.id()); 
}

const WebCore::SVGDocumentExtensions* DOM::DocumentImpl::svgExtensions()
{
    return m_svgExtensions;
}

WebCore::SVGDocumentExtensions* DOM::DocumentImpl::accessSVGExtensions()
{
    if (!m_svgExtensions) {
        m_svgExtensions = new WebCore::SVGDocumentExtensions(this);
    }
    return m_svgExtensions;
}

// ----------------------------------------------------------------------------
// Support for Javascript execCommand, and related methods

JSEditor *DocumentImpl::jsEditor()
{
    if (!m_jsEditor)
        m_jsEditor = new JSEditor(this);
    return m_jsEditor;
}

bool DocumentImpl::execCommand(const DOMString &command, bool userInterface, const DOMString &value)
{
    // qDebug() << "[execute command]" << command << userInterface << value << endl;
    return jsEditor()->execCommand(jsEditor()->commandImp(command), userInterface, value);
}

bool DocumentImpl::queryCommandEnabled(const DOMString &command)
{
    return jsEditor()->queryCommandEnabled(jsEditor()->commandImp(command));
}

bool DocumentImpl::queryCommandIndeterm(const DOMString &command)
{
    return jsEditor()->queryCommandIndeterm(jsEditor()->commandImp(command));
}

bool DocumentImpl::queryCommandState(const DOMString &command)
{
    return jsEditor()->queryCommandState(jsEditor()->commandImp(command));
}

bool DocumentImpl::queryCommandSupported(const DOMString &command)
{
    // qDebug() << "[query command supported]" << command << endl;
    return jsEditor()->queryCommandSupported(jsEditor()->commandImp(command));
}

DOMString DocumentImpl::queryCommandValue(const DOMString &command)
{
    return jsEditor()->queryCommandValue(jsEditor()->commandImp(command));
}

// ----------------------------------------------------------------------------
// DOM3 XPath, from XPathEvaluator interface

khtml::XPathExpressionImpl* DocumentImpl::createExpression(DOMString &expression,
                                                  khtml::XPathNSResolverImpl *resolver,
                                                  int &exceptioncode )
{
    XPathExpressionImpl* cand = new XPathExpressionImpl( expression, resolver );
    if ((exceptioncode = cand->parseExceptionCode())) {
        delete cand;
        return 0;
    }

    return cand;
}

khtml::XPathNSResolverImpl* DocumentImpl::createNSResolver( NodeImpl *nodeResolver )
{
    return nodeResolver ? new DefaultXPathNSResolverImpl(nodeResolver) : 0;
}

khtml::XPathResultImpl * DocumentImpl::evaluate( DOMString &expression,
                                      NodeImpl *contextNode,
                                      khtml::XPathNSResolverImpl *resolver,
                                      unsigned short type,
                                      khtml::XPathResultImpl * /*result*/,
                                      int &exceptioncode )
{
    XPathExpressionImpl *expr = createExpression( expression, resolver, exceptioncode );
    if ( exceptioncode )
        return 0;

    XPathResultImpl *res = expr->evaluate( contextNode, type, 0, exceptioncode );
    delete expr;  // don't need it anymore.
    
    if ( exceptioncode ) {
        delete res;
        return 0;
    }

    return res;
}
// ----------------------------------------------------------------------------

WindowEventTargetImpl::WindowEventTargetImpl(DOM::DocumentImpl* owner):
    m_owner(owner)
{}

EventTargetImpl::Type WindowEventTargetImpl::eventTargetType() const
{
    return WINDOW;
}

DocumentImpl* WindowEventTargetImpl::eventTargetDocument()
{
    return m_owner;
}

KJS::Window* WindowEventTargetImpl::window()
{
    if (m_owner->part())
        return KJS::Window::retrieveWindow(m_owner->part());
    else
        return 0;
}
// ----------------------------------------------------------------------------

DocumentFragmentImpl::DocumentFragmentImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
}

DOMString DocumentFragmentImpl::nodeName() const
{
  return "#document-fragment";
}

unsigned short DocumentFragmentImpl::nodeType() const
{
    return Node::DOCUMENT_FRAGMENT_NODE;
}

// DOM Section 1.1.1
bool DocumentFragmentImpl::childTypeAllowed( unsigned short type )
{
    switch (type) {
        case Node::ELEMENT_NODE:
        case Node::PROCESSING_INSTRUCTION_NODE:
        case Node::COMMENT_NODE:
        case Node::TEXT_NODE:
        case Node::CDATA_SECTION_NODE:
        case Node::ENTITY_REFERENCE_NODE:
            return true;
            break;
        default:
            return false;
    }
}

DOMString DocumentFragmentImpl::toString() const
{
    DOMString result;

    for (NodeImpl *child = firstChild(); child != NULL; child = child->nextSibling()) {
        if (child->nodeType() == Node::COMMENT_NODE || child->nodeType() == Node::PROCESSING_INSTRUCTION_NODE)
            continue;
	result += child->toString();
    }

    return result;
}

WTF::PassRefPtr<NodeImpl> DocumentFragmentImpl::cloneNode ( bool deep )
{
    WTF::RefPtr<DocumentFragmentImpl> clone = new DocumentFragmentImpl( docPtr() );
    if (deep)
        cloneChildNodes(clone.get());
    return clone;
}


// ----------------------------------------------------------------------------

DocumentTypeImpl::DocumentTypeImpl(DOMImplementationImpl *implementation, DocumentImpl *doc,
                                   const DOMString &qualifiedName, const DOMString &publicId,
                                   const DOMString &systemId)
    : NodeImpl(doc), m_implementation(implementation),
      m_qualifiedName(qualifiedName), m_publicId(publicId), m_systemId(systemId)
{
    m_implementation->ref();

    m_entities = 0;
    m_notations = 0;

    // if doc is 0, it is not attached to a document and / or
    // therefore does not provide entities or notations. (DOM Level 3)
}

DocumentTypeImpl::~DocumentTypeImpl()
{
    m_implementation->deref();
    if (m_entities)
        m_entities->deref();
    if (m_notations)
        m_notations->deref();
}

DOMString DocumentTypeImpl::toString() const
{
    DOMString result = "<!DOCTYPE ";
    result += m_qualifiedName;
    if (!m_publicId.isEmpty()) {
	result += " PUBLIC \"";
	result += m_publicId;
	result += "\" \"";
	result += m_systemId;
	result += "\"";
    } else if (!m_systemId.isEmpty()) {
	result += " SYSTEM \"";
	result += m_systemId;
	result += "\"";
    }

    if (!m_subset.isEmpty()) {
	result += " [";
	result += m_subset;
	result += "]";
    }

    result += ">";

    return result;
}

DOMString DocumentTypeImpl::nodeName() const
{
    return name();
}

unsigned short DocumentTypeImpl::nodeType() const
{
    return Node::DOCUMENT_TYPE_NODE;
}

// DOM Section 1.1.1
bool DocumentTypeImpl::childTypeAllowed( unsigned short /*type*/ )
{
    return false;
}

WTF::PassRefPtr<NodeImpl> DocumentTypeImpl::cloneNode ( bool /*deep*/ )
{
    DocumentTypeImpl *clone = new DocumentTypeImpl(implementation(),
						   0,
						   name(), publicId(),
						   systemId());
    // ### copy entities etc.
    return clone;
}

NamedNodeMapImpl * DocumentTypeImpl::entities() const
{
    if ( !m_entities ) {
        m_entities = new GenericRONamedNodeMapImpl( docPtr() );
        m_entities->ref();
    }
    return m_entities;
}

NamedNodeMapImpl * DocumentTypeImpl::notations() const
{
    if ( !m_notations ) {
        m_notations = new GenericRONamedNodeMapImpl( docPtr() );
        m_notations->ref();
    }
    return m_notations;
}

void XMLDocumentImpl::close()
{
    bool doload = !parsing() && m_tokenizer;

    DocumentImpl::close();

    if (doload) {
        document()->dispatchWindowEvent(EventImpl::LOAD_EVENT, false, false);
    }
}


// kate: indent-width 4; replace-tabs on; tab-width 8; space-indent on;
