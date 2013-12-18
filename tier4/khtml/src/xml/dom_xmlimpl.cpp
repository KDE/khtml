/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2000 Peter Kelly (pmk@post.com)
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

#include "dom_xmlimpl.h"

#include <dom/dom_exception.h>

#include "dom_docimpl.h"
#include "dom_stringimpl.h"
#include <css/css_stylesheetimpl.h>
#include <css/css_mediaquery.h>
#include <misc/loader.h>

using namespace DOM;

EntityImpl::EntityImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    m_publicId = 0;
    m_systemId = 0;
    m_notationName = 0;
    m_name = 0;
}

EntityImpl::EntityImpl(DocumentImpl *doc, DOMString _name) : NodeBaseImpl(doc)
{
    m_publicId = 0;
    m_systemId = 0;
    m_notationName = 0;
    m_name = _name.implementation();
    if (m_name)
        m_name->ref();
}

EntityImpl::EntityImpl(DocumentImpl *doc, DOMString _publicId, DOMString _systemId, DOMString _notationName) : NodeBaseImpl(doc)
{
    m_publicId = _publicId.implementation();
    if (m_publicId)
        m_publicId->ref();
    m_systemId = _systemId.implementation();
    if (m_systemId)
        m_systemId->ref();
    m_notationName = _notationName.implementation();
    if (m_notationName)
        m_notationName->ref();
    m_name = 0;
}


EntityImpl::~EntityImpl()
{
    if (m_publicId)
        m_publicId->deref();
    if (m_systemId)
        m_systemId->deref();
    if (m_notationName)
        m_notationName->deref();
    if (m_name)
        m_name->deref();
}

DOMString EntityImpl::publicId() const
{
    return m_publicId;
}

DOMString EntityImpl::systemId() const
{
    return m_systemId;
}

DOMString EntityImpl::notationName() const
{
    return m_notationName;
}

DOMString EntityImpl::nodeName() const
{
    return m_name;
}

unsigned short EntityImpl::nodeType() const
{
    return Node::ENTITY_NODE;
}

WTF::PassRefPtr<NodeImpl> EntityImpl::cloneNode ( bool /*deep*/)
{
    // Spec says cloning Document nodes is "implementation dependent"
    // so we do not support it...
    return 0;
}

// DOM Section 1.1.1
bool EntityImpl::childTypeAllowed( unsigned short type )
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

DOMString EntityImpl::toString() const
{
    DOMString result = "<!ENTITY' ";

    if (m_name && m_name->l != 0) {
	result += " ";
	result += m_name;
    }

    if (m_publicId && m_publicId->l != 0) {
	result += " PUBLIC \"";
	result += m_publicId;
	result += "\" \"";
	result += m_systemId;
	result += "\"";
    } else if (m_systemId && m_systemId->l != 0) {
	result += " SYSTEM \"";
	result += m_systemId;
	result += "\"";
    }

    if (m_notationName && m_notationName->l != 0) {
	result += " NDATA ";
	result += m_notationName;
    }

    result += ">";

    return result;
}

// -------------------------------------------------------------------------

EntityReferenceImpl::EntityReferenceImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    m_entityName = 0;
}

EntityReferenceImpl::EntityReferenceImpl(DocumentImpl *doc, DOMStringImpl *_entityName) : NodeBaseImpl(doc)
{
    m_entityName = _entityName;
    if (m_entityName)
        m_entityName->ref();
}

EntityReferenceImpl::~EntityReferenceImpl()
{
    if (m_entityName)
        m_entityName->deref();
}

DOMString EntityReferenceImpl::nodeName() const
{
    return m_entityName;
}

unsigned short EntityReferenceImpl::nodeType() const
{
    return Node::ENTITY_REFERENCE_NODE;
}

WTF::PassRefPtr<NodeImpl> EntityReferenceImpl::cloneNode ( bool deep )
{
    EntityReferenceImpl *clone = new EntityReferenceImpl(docPtr(),m_entityName);
    // ### make sure children are readonly
    // ### since we are a reference, should we clone children anyway (even if not deep?)
    if (deep)
        cloneChildNodes(clone);
    return clone;
}

// DOM Section 1.1.1
bool EntityReferenceImpl::childTypeAllowed( unsigned short type )
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

DOMString EntityReferenceImpl::toString() const
{
    DOMString result = "&";
    result += m_entityName;
    result += ";";

    return result;
}

// -------------------------------------------------------------------------

NotationImpl::NotationImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    m_publicId = 0;
    m_systemId = 0;
    m_name = 0;
}

NotationImpl::NotationImpl(DocumentImpl *doc, DOMString _name, DOMString _publicId, DOMString _systemId) : NodeBaseImpl(doc)
{
    m_name = _name.implementation();
    if (m_name)
        m_name->ref();
    m_publicId = _publicId.implementation();
    if (m_publicId)
        m_publicId->ref();
    m_systemId = _systemId.implementation();
    if (m_systemId)
        m_systemId->ref();
}

NotationImpl::~NotationImpl()
{
    if (m_name)
        m_name->deref();
    if (m_publicId)
        m_publicId->deref();
    if (m_systemId)
        m_systemId->deref();
}

DOMString NotationImpl::publicId() const
{
    return m_publicId;
}

DOMString NotationImpl::systemId() const
{
    return m_systemId;
}

DOMString NotationImpl::nodeName() const
{
    return m_name;
}

unsigned short NotationImpl::nodeType() const
{
    return Node::NOTATION_NODE;
}

WTF::PassRefPtr<NodeImpl> NotationImpl::cloneNode ( bool /*deep*/)
{
    // Spec says cloning Document nodes is "implementation dependent"
    // so we do not support it...
    return 0;
}

// DOM Section 1.1.1
bool NotationImpl::childTypeAllowed( unsigned short /*type*/ )
{
    return false;
}

// -------------------------------------------------------------------------

// ### need a way of updating these properly whenever child nodes of the processing instruction
// change or are added/removed

ProcessingInstructionImpl::ProcessingInstructionImpl(DocumentImpl *doc) : NodeBaseImpl(doc)
{
    m_target = 0;
    m_data = 0;
    m_localHref = 0;
    m_alternate = false;
    m_title = 0;
    m_media = 0;
    m_sheet = 0;
    m_cachedSheet = 0;
}

ProcessingInstructionImpl::ProcessingInstructionImpl(DocumentImpl *doc, DOMString _target, DOMString _data) : NodeBaseImpl(doc)
{
    m_target = _target.implementation();
    if (m_target)
        m_target->ref();
    m_data = _data.implementation();
    if (m_data)
        m_data->ref();
    m_sheet = 0;
    m_cachedSheet = 0;
    m_localHref = 0;
    m_title = 0;
    m_media = 0;
    m_alternate = false;
}

ProcessingInstructionImpl::~ProcessingInstructionImpl()
{
    if (m_target)
        m_target->deref();
    if (m_title)
        m_title->deref();
    if (m_media)
        m_media->deref();
    if (m_data)
        m_data->deref();
    if (m_cachedSheet)
	m_cachedSheet->deref(this);
    if (m_sheet)
	m_sheet->deref();
}

DOMString ProcessingInstructionImpl::target() const
{
    return m_target;
}

void ProcessingInstructionImpl::setData( const DOMString &_data, int &exceptioncode )
{
    // NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    if (m_data)
        m_data->deref();
    m_data = _data.implementation();
    if (m_data)
        m_data->ref();
}

DOMString ProcessingInstructionImpl::nodeName() const
{
    return m_target;
}

unsigned short ProcessingInstructionImpl::nodeType() const
{
    return Node::PROCESSING_INSTRUCTION_NODE;
}

DOMString ProcessingInstructionImpl::nodeValue() const
{
    return m_data;
}

void ProcessingInstructionImpl::setNodeValue( const DOMString &_nodeValue, int &exceptioncode )
{
    // NO_MODIFICATION_ALLOWED_ERR: taken care of by setData()
    setData(_nodeValue, exceptioncode);
}

WTF::PassRefPtr<NodeImpl> ProcessingInstructionImpl::cloneNode ( bool /*deep*/)
{
    // ### copy m_localHref
    return new ProcessingInstructionImpl(docPtr(),m_target,m_data);
}

DOMString ProcessingInstructionImpl::localHref() const
{
    return m_localHref;
}

// DOM Section 1.1.1
bool ProcessingInstructionImpl::childTypeAllowed( unsigned short /*type*/ )
{
    return false;
}

void ProcessingInstructionImpl::checkStyleSheet()
{
    if (m_target && DOMString(m_target) == "xml-stylesheet") {
        // see http://www.w3.org/TR/xml-stylesheet/
        // ### check that this occurs only in the prolog
        // ### support stylesheet included in a fragment of this (or another) document
        // ### make sure this gets called when adding from javascript
        XMLAttributeReader attrReader(DOMString(m_data).string());
        bool attrsOk;
        QXmlAttributes attrs = attrReader.readAttrs(attrsOk);
        if (!attrsOk)
            return;
        if (attrs.value("type") != "text/css" && !attrs.value("type").isEmpty())
            return;

        DOMString href = attrs.value("href");
        DOMString alternate = attrs.value("alternate");
        m_alternate = alternate == "yes";
        DOMString title = attrs.value("title");
        DOMString media = attrs.value("media");
        if (m_title)
            m_title->deref();
        m_title = title.implementation();
        if (m_title)
            m_title->ref();
        if (m_media)
            m_media->deref();
        m_media = media.implementation();
        if (m_media)
            m_media->ref();

        if (href.length()>1)
        {
            if (href[0]=='#')
            {
                if (m_localHref)
                    m_localHref->deref();
                m_localHref=href.implementation()->split(1);
                if (m_localHref)
                    m_localHref->ref();
            }
            else
            {
                // ### some validation on the URL?
                // ### FIXME charset
                if (m_cachedSheet) m_cachedSheet->deref(this);
                m_cachedSheet = document()->docLoader()->requestStyleSheet(document()->completeURL(href.string()), QString());
                if (m_cachedSheet) {
                    document()->addPendingSheet(); //before ref, because during the ref it might load!
                    m_cachedSheet->ref( this );
                }
            }

        }
    }
}

StyleSheetImpl *ProcessingInstructionImpl::sheet() const
{
    return m_sheet;
}

void ProcessingInstructionImpl::setStyleSheet(const DOM::DOMString &url, const DOM::DOMString &sheet, const DOM::DOMString &charset, const DOM::DOMString &mimetype)
{
    if (m_sheet)
	m_sheet->deref();
    m_sheet = new CSSStyleSheetImpl(document(), url);
    m_sheet->ref();
    m_sheet->setCharset(charset);
    m_sheet->setTitle(m_title);
    m_sheet->setMedia(new MediaListImpl((CSSStyleSheetImpl*)m_sheet, m_media, false));
    m_sheet->parseString(khtml::isAcceptableCSSMimetype(mimetype) ? sheet : "");
    if (m_cachedSheet)
	m_cachedSheet->deref(this);
    m_cachedSheet = 0;

    document()->styleSheetLoaded();
}

void ProcessingInstructionImpl::setStyleSheet(CSSStyleSheetImpl* sheet)
{
    if (m_sheet)
        m_sheet->deref();
    m_sheet = sheet;
    if (m_sheet) {
        m_sheet->ref();
        m_sheet->setTitle(m_title);
    }
}

DOMString ProcessingInstructionImpl::toString() const
{
    DOMString result = "<?";
    result += m_target;
    result += " ";
    result += m_data;
    result += ">";
    return result;
}

int ProcessingInstructionImpl::maxCharacterOffset() const
{ 
    return m_data ? static_cast<int>(m_data->length()) : 0;
}

// -------------------------------------------------------------------------

XMLAttributeReader::XMLAttributeReader(const QString& _attrString)
{
    m_attrString = _attrString;
}

XMLAttributeReader::~XMLAttributeReader()
{
}

QXmlAttributes XMLAttributeReader::readAttrs(bool &ok)
{
    // parse xml file
    QXmlInputSource source;
    source.setData("<?xml version=\"1.0\"?><attrs "+m_attrString+" />");
    QXmlSimpleReader reader;
    reader.setContentHandler( this );
    ok = reader.parse( source );
    return attrs;
}

bool XMLAttributeReader::startElement(const QString& /*namespaceURI*/, const QString& localName,
                                      const QString& /*qName*/, const QXmlAttributes& atts)
{
    if (localName == "attrs") {
        attrs = atts;
        return true;
    }
    else
        return false; // we shouldn't have any other elements
}
