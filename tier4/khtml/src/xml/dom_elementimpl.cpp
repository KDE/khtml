/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2005, 2008 Maksim Orlovich (maksim@kde.org)
 *           (C) 2006 Allan Sandfeld Jensen (kde@carewolf.com)
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

//#define EVENT_DEBUG

#include "dom_elementimpl.h"

#include <dom/dom_exception.h>
#include <dom/dom_node.h>
#include <dom/html_image.h>
#include "dom_textimpl.h"
#include "dom_docimpl.h"
#include "dom2_eventsimpl.h"
#include "dom_restyler.h"
#include "dom_xmlimpl.h"

#include <html/dtd.h>
#include <html/htmlparser.h>
#include <html/html_imageimpl.h>

#include <rendering/render_canvas.h>
#include <css/css_valueimpl.h>
#include <css/css_stylesheetimpl.h>
#include <css/cssstyleselector.h>
#include <css/cssvalues.h>
#include <css/cssproperties.h>
#include <khtml_part.h>
#include <khtmlview.h>

#include <editing/editing_p.h>
#include <editing/editor.h>

#include <QtCore/QTextStream>
#include <QTextDocument>
#include <QDebug>
#include <stdlib.h>

#include <wtf/HashMap.h>

// ### support default attributes
// ### dispatch mutation events
// ### check for INVALID_CHARACTER_ERR where appropriate

using namespace khtml;

namespace DOM {

AttrImpl::AttrImpl(ElementImpl* element, DocumentImpl* docPtr, NamespaceName namespacename, LocalName localName, PrefixName prefix, DOMStringImpl* value)
    : NodeBaseImpl(docPtr)
{
    m_value = value;
    m_value->ref();

    m_namespace = namespacename;
    m_localName = localName;
    m_prefix = prefix;

    // When creating the text node initially, we want element = 0,
    // so we don't attempt to update the getElementById cache or
    // call parseAttribute, etc. This is because we're normally lazily,
    // from previous attributes, so there is nothing really changing
    m_element = 0;
    createTextChild();
    m_element = element;
}

AttrImpl::~AttrImpl()
{
    m_value->deref();
}

DOMString AttrImpl::nodeName() const
{
    return name();
}

unsigned short AttrImpl::nodeType() const
{
    return Node::ATTRIBUTE_NODE;
}

DOMString AttrImpl::prefix() const
{
    return m_prefix.toString();
}

void AttrImpl::setPrefix(const DOMString &_prefix, int &exceptioncode )
{
    checkSetPrefix(_prefix, exceptioncode);
    if (exceptioncode)
        return;

    m_prefix = PrefixName::fromString(_prefix);
}

DOMString AttrImpl::namespaceURI() const
{
    if (m_htmlCompat)
        return DOMString();
    return m_namespace.toString();
}

DOMString AttrImpl::localName() const
{
    return m_localName.toString();
}

DOMString AttrImpl::nodeValue() const
{
    return m_value;
}

DOMString AttrImpl::name() const
{
    DOMString n = m_localName.toString();

    // compat mode always return attribute names in lowercase.
    // that's not formally in the specification, but common
    // practice - a w3c erratum to DOM L2 is pending.
    if (m_htmlCompat)
        n = n.lower();

    DOMString p = m_prefix.toString();
    if (!p.isEmpty())
        return p + DOMString(":") + n;

    return n;
}

void AttrImpl::createTextChild()
{
    // add a text node containing the attribute value
    if (m_value->length() > 0) {
	TextImpl* textNode = ownerDocument()->createTextNode(m_value);

	// We want to use addChild and not appendChild here to avoid triggering
	// mutation events. childrenChanged() will still be called.
	addChild(textNode);
    }
}

void AttrImpl::childrenChanged()
{
    NodeBaseImpl::childrenChanged();

    // update value
    DOMStringImpl* oldVal = m_value;
    m_value = new DOMStringImpl((QChar*)0, 0);
    m_value->ref();
    for (NodeImpl* n = firstChild(); n; n = n->nextSibling()) {
	DOMStringImpl* data = static_cast<const TextImpl*>(n)->string();
	m_value->append(data);
    }

    if (m_element) {
        int curr = id();
        if (curr == ATTR_ID)
            m_element->updateId(oldVal, m_value);
        m_element->parseAttribute(this);
        m_element->attributeChanged(curr);
    }

    oldVal->deref();
}

void AttrImpl::setValue( const DOMString &v, int &exceptioncode )
{
    exceptioncode = 0;

    // do not interpret entities in the string, it is literal!

    // NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    // ### what to do on 0 ?
    if (v.isNull()) {
        exceptioncode = DOMException::DOMSTRING_SIZE_ERR;
        return;
    }

    if (m_value == v.implementation())
	return;

    int e = 0;
    removeChildren();
    appendChild(ownerDocument()->createTextNode(v.implementation()), e);
}

void AttrImpl::rewriteValue( const DOMString& newValue )
{
    int ec;

    // We want to avoid any notifications, so temporarily set m_element to 0
    ElementImpl* saveElement = m_element;
    m_element = 0;
    setValue(newValue, ec);
    m_element = saveElement;
}

void AttrImpl::setNodeValue( const DOMString &v, int &exceptioncode )
{
    exceptioncode = 0;
    // NO_MODIFICATION_ALLOWED_ERR: taken care of by setValue()
    setValue(v, exceptioncode);
}

WTF::PassRefPtr<NodeImpl> AttrImpl::cloneNode ( bool /*deep*/)
{
     AttrImpl* attr = new AttrImpl(0, docPtr(), m_namespace, m_localName, m_prefix, m_value);
     attr->setHTMLCompat(m_htmlCompat);
     return attr;
}

// DOM Section 1.1.1
bool AttrImpl::childAllowed( NodeImpl *newChild )
{
    if(!newChild)
        return false;

    return childTypeAllowed(newChild->nodeType());
}

bool AttrImpl::childTypeAllowed( unsigned short type )
{
    switch (type) {
        case Node::TEXT_NODE:
        case Node::ENTITY_REFERENCE_NODE:
            return true;
            break;
        default:
            return false;
    }
}

DOMString AttrImpl::toString() const
{
    DOMString result;

    result += nodeName();

    // FIXME: substitute entities for any instances of " or ' --
    // maybe easier to just use text value and ignore existing
    // entity refs?

    if ( !nodeValue().isEmpty() ) {
        //remove the else once the AttributeImpl changes are merged
        result += "=\"";
        result += nodeValue();
        result += "\"";
    }

    return result;
}

void AttrImpl::setElement(ElementImpl *element)
{
    m_element = element;
}

// -------------------------------------------------------------------------

void AttributeImpl::setValue(DOMStringImpl *value, ElementImpl *element)
{
    assert(value);
    if (m_localName.id()) {
	if (m_data.value == value)
	    return;

	if (element && id() == ATTR_ID)
	   element->updateId(m_data.value, value);

	m_data.value->deref();
	m_data.value = value;
	m_data.value->ref();

	if (element) {
	    element->parseAttribute(this);
	    element->attributeChanged(id());
        }
    }
    else {
	int exceptioncode = 0;
	m_data.attr->setValue(value,exceptioncode);
	// AttrImpl::setValue() calls parseAttribute()
    }
}

void AttributeImpl::rewriteValue( const DOMString& newValue )
{
    if (m_localName.id()) {
	// We may have m_data.value == null if we were given a normalized value
	// off a removeAttribute (which would call parseNullAttribute()).
	// Ignore such requests.
	if (!m_data.value)
	    return;

	DOMStringImpl* value = newValue.implementation();
	if (m_data.value == value)
	    return;

        m_data.value->deref();
        m_data.value = value;
        m_data.value->ref();
    } else {
	m_data.attr->rewriteValue(newValue);
    }
}

AttrImpl *AttributeImpl::createAttr(ElementImpl *element, DocumentImpl *docPtr)
{
    if (m_localName.id()) {
        AttrImpl *attr = new AttrImpl(element, docPtr, m_namespace, m_localName, m_prefix, m_data.value);
        if (!attr) return 0;
        attr->setHTMLCompat(element->htmlCompat());
	m_data.value->deref();
	m_data.attr = attr;
	m_data.attr->ref();
        m_localName = emptyLocalName; /* "has implementation" flag */
    }

    return m_data.attr;
}

void AttributeImpl::free()
{
    if (m_localName.id()) {
	m_data.value->deref();
    }
    else {
	m_data.attr->setElement(0);
	m_data.attr->deref();
    }
}

// -------------------------------------------------------------------------

class ElementRareDataImpl {
public:
    ElementRareDataImpl();
    void resetComputedStyle();
    short tabIndex() const { return m_tabIndex; }
    void setTabIndex(short _tabIndex) { m_tabIndex = _tabIndex; m_hasTabIndex = true; }

    RenderStyle* m_computedStyle;
    signed short m_tabIndex;
    bool m_hasTabIndex;
};

typedef WTF::HashMap<const ElementImpl*, ElementRareDataImpl*> ElementRareDataMap;

static ElementRareDataMap& rareDataMap()
{
    static ElementRareDataMap* dataMap = new ElementRareDataMap;
    return *dataMap;
}

static ElementRareDataImpl* rareDataFromMap(const ElementImpl* element)
{
    return rareDataMap().get(element);
}

inline ElementRareDataImpl::ElementRareDataImpl()
    : m_computedStyle(0), m_tabIndex(0), m_hasTabIndex(false)
{}

void ElementRareDataImpl::resetComputedStyle()
{
    if (!m_computedStyle)
        return;
    m_computedStyle->deref();
    m_computedStyle = 0;
}

// -------------------------------------------------------------------------

ElementImpl::ElementImpl(DocumentImpl *doc)
    : NodeBaseImpl(doc)
{
    namedAttrMap = 0;
    m_style.inlineDecls = 0;
    m_prefix = emptyPrefixName;
}

ElementImpl::~ElementImpl()
{
    if(namedAttrMap) {
        namedAttrMap->detachFromElement();
        namedAttrMap->deref();
    }

    if (m_style.inlineDecls) {
        if (CSSStyleDeclarationImpl * ild = inlineStyleDecls()) {
            // remove inline declarations
            ild->setNode(0);
            ild->setParent(0);
            ild->deref();
        }
        if (CSSStyleDeclarationImpl * ncd = nonCSSStyleDecls()) {
            // remove presentational declarations
            ncd->setNode(0);
            ncd->setParent(0);
            ncd->deref();
            delete m_style.combinedDecls;
        }
    }

    if (!m_elementHasRareData) {
        ASSERT(!rareDataMap().contains(this));
    } else {
        ElementRareDataMap& dataMap = rareDataMap();
        ElementRareDataMap::iterator it = dataMap.find(this);
        ASSERT(it != dataMap.end());
        delete it->second;
        dataMap.remove(it);
    }
}

ElementRareDataImpl* ElementImpl::rareData()
{
    return m_elementHasRareData ? rareDataFromMap(this) : 0;
}

const ElementRareDataImpl* ElementImpl::rareData() const
{
    return m_elementHasRareData ? rareDataFromMap(this) : 0;
}

ElementRareDataImpl* ElementImpl::createRareData()
{
    if (m_elementHasRareData)
        return rareDataMap().get(this);
    ASSERT(!rareDataMap().contains(this));
    ElementRareDataImpl* data = new ElementRareDataImpl();
    rareDataMap().set(this, data);
    m_elementHasRareData = true;
    return data;
}

void ElementImpl::removeAttribute( NodeImpl::Id id, int &exceptioncode )
{
    if (namedAttrMap) {
        namedAttrMap->removeNamedItem(id, emptyPrefixName, false, exceptioncode);
        if (exceptioncode == DOMException::NOT_FOUND_ERR) {
            exceptioncode = 0;
        }
    }
}

unsigned short ElementImpl::nodeType() const
{
    return Node::ELEMENT_NODE;
}


DOMString ElementImpl::localName() const
{
    return LocalName::fromId(id()).toString();
}

DOMString ElementImpl::tagName() const
{
    DOMString tn = LocalName::fromId(id()).toString();

    if ( m_htmlCompat )
        tn = tn.upper();

    DOMString prefix = m_prefix.toString();
    if (!prefix.isEmpty())
        return prefix + DOMString(":") + tn;

    return tn;
}

DOMString ElementImpl::nonCaseFoldedTagName() const
{
    DOMString tn = LocalName::fromId(id()).toString();

    DOMString prefix = m_prefix.toString();
    if (!prefix.isEmpty())
        return prefix + DOMString(":") + tn;

    return tn;
}

/*DOMStringImpl* ElementImpl::getAttributeImpl(NodeImpl::Id id, PrefixName prefix, bool nsAware) const
{
    return namedAttrMap ? namedAttrMap->getValue(id, prefix, nsAware) : 0;
}*/

void ElementImpl::setAttribute(NodeImpl::Id id, const PrefixName& prefix, bool nsAware, const DOMString &value, int &exceptioncode)
{
    // NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    attributes()->setValue(id, value.implementation(), prefix, nsAware);
}

void ElementImpl::setAttributeNS( const DOMString &namespaceURI, const DOMString &qualifiedName,
				const DOMString &value, int &exceptioncode )
{
    // NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    int colonPos;
    if (!DOM::checkQualifiedName(qualifiedName, namespaceURI, &colonPos,
                                 false/*nameCanBeNull*/, false/*nameCanBeEmpty*/,
                                 &exceptioncode))
        return;
    LocalName localname;
    PrefixName prefixname;
    splitPrefixLocalName(qualifiedName, prefixname, localname, m_htmlCompat, colonPos);
    NamespaceName namespacename = NamespaceName::fromString(namespaceURI);
    attributes()->setValue(makeId(namespacename.id(), localname.id()), value.implementation(), prefixname, true /*nsAware*/);
}

void ElementImpl::setAttribute(NodeImpl::Id id, const DOMString &value)
{
    int exceptioncode = 0;
    setAttribute(id, emptyPrefixName, false, value, exceptioncode);
}

void ElementImpl::setBooleanAttribute(NodeImpl::Id id, bool b)
{
    if (b)
        setAttribute(id, "1");
    else {
        int ec;
        removeAttribute(id, ec);
    }
}

void ElementImpl::setAttributeMap( NamedAttrMapImpl* list )
{
    // If setting the whole map changes the id attribute, we need to
    // call updateId.
    DOMStringImpl *oldId = namedAttrMap ? namedAttrMap->getValue(ATTR_ID) : 0;
    DOMStringImpl *newId = list ? list->getValue(ATTR_ID) : 0;

    if (oldId || newId) {
       updateId(oldId, newId);
    }

    if (namedAttrMap) {
	namedAttrMap->detachFromElement();
        namedAttrMap->deref();
    }

    namedAttrMap = list;

    if (namedAttrMap) {
        namedAttrMap->ref();
        assert(namedAttrMap->m_element == 0);
        namedAttrMap->setElement(this);
        unsigned len = namedAttrMap->length();
        for (unsigned i = 0; i < len; i++) {
            parseAttribute(&namedAttrMap->m_attrs[i]);
            attributeChanged(namedAttrMap->m_attrs[i].id());
        }
    }
}

WTF::PassRefPtr<NodeImpl> ElementImpl::cloneNode(bool deep)
{
    WTF::RefPtr<ElementImpl> clone; // Make sure to guard...
    clone = document()->createElementNS( namespaceURI(), nonCaseFoldedTagName() /* includes prefix*/ );
    if (!clone) return 0;
    finishCloneNode( clone.get(), deep );
    return clone;
}

void ElementImpl::finishCloneNode( ElementImpl* clone, bool deep )
{
    // clone attributes
    if (namedAttrMap || m_needsStyleAttributeUpdate)
	clone->attributes()->copyAttributes(attributes(true));

    assert( !m_needsStyleAttributeUpdate ); // ensured by previous line

    // clone individual style rules
    if (m_style.inlineDecls) {
        if (m_hasCombinedStyle) {
            if (!clone->m_hasCombinedStyle)
                clone->createNonCSSDecl();
            if (m_style.combinedDecls->inlineDecls)
                *(clone->getInlineStyleDecls()) = *m_style.combinedDecls->inlineDecls;
            *clone->m_style.combinedDecls->nonCSSDecls = *m_style.combinedDecls->nonCSSDecls;
        } else {
            *(clone->getInlineStyleDecls()) = *m_style.inlineDecls;
        }
    }

    // ### fold above style cloning into this function?
    clone->copyNonAttributeProperties(this);

    if (deep)
        cloneChildNodes(clone);

    // copy over our compatibility mode.
    clone->setHTMLCompat(htmlCompat());
}

bool ElementImpl::hasAttributes() const
{
    return namedAttrMap && namedAttrMap->length() > 0;
}

bool ElementImpl::hasAttribute( const DOMString& name ) const
{
    LocalName localname;
    PrefixName prefixname;
    splitPrefixLocalName(name, prefixname, localname, m_htmlCompat);
    if (!localname.id()) return false;
    if (!namedAttrMap) return false;
    return namedAttrMap->getValue(makeId(emptyNamespace, localname.id()), prefixname, false) != 0;
}

bool ElementImpl::hasAttributeNS( const DOMString &namespaceURI,
                                  const DOMString &localName ) const
{
    NamespaceName namespacename = NamespaceName::fromString(namespaceURI);
    LocalName localname = LocalName::fromString(localName, m_htmlCompat ? IDS_NormalizeLower : IDS_CaseSensitive);
    NodeImpl::Id id = makeId(namespacename.id(), localname.id());
    if (!id) return false;
    if (!namedAttrMap) return false;
    return namedAttrMap->getValue(id, emptyPrefixName, true) != 0;
}

DOMString ElementImpl::getAttribute( const DOMString &name )
{
    LocalName localname;
    PrefixName prefixname;
    splitPrefixLocalName(name, prefixname, localname, m_htmlCompat);
    if (!localname.id()) return DOMString();
    return getAttribute(makeId(emptyNamespace, localname.id()), prefixname, false /*nsAware*/);
}

void ElementImpl::setAttribute( const DOMString &name, const DOMString &value, int& exceptioncode )
{
    int colon;
    if (!DOM::checkQualifiedName(name, "", &colon, false/*nameCanBeNull*/, false/*nameCanBeEmpty*/, &exceptioncode))
        return;
    LocalName localname;
    PrefixName prefixname;
    splitPrefixLocalName(name, prefixname, localname, m_htmlCompat, colon);
    setAttribute(makeId(emptyNamespace, localname.id()), prefixname, false, value.implementation(), exceptioncode);
}

void ElementImpl::removeAttribute( const DOMString &name, int& exceptioncode )
{
    LocalName localname;
    PrefixName prefixname;
    splitPrefixLocalName(name, prefixname, localname, m_htmlCompat);

    // FIXME what if attributes(false) == 0?
    attributes(false)->removeNamedItem(makeId(emptyNamespace, localname.id()), prefixname, false, exceptioncode);

    // it's allowed to remove attributes that don't exist.
    if ( exceptioncode == DOMException::NOT_FOUND_ERR )
        exceptioncode = 0;
}

AttrImpl* ElementImpl::getAttributeNode( const DOMString &name )
{
    LocalName localname;
    PrefixName prefixname;
    splitPrefixLocalName(name, prefixname, localname, m_htmlCompat);

    if (!localname.id()) return 0;
    if (!namedAttrMap) return 0;

    return static_cast<AttrImpl*>(attributes()->getNamedItem(makeId(emptyNamespace, localname.id()), prefixname, false));
}

Attr ElementImpl::setAttributeNode( AttrImpl* newAttr, int& exceptioncode )
{
    if (!newAttr) {
      exceptioncode = DOMException::NOT_FOUND_ERR;
      return 0;
    }
    Attr r = attributes(false)->setNamedItem(newAttr, emptyPrefixName, true, exceptioncode);
    if ( !exceptioncode )
      newAttr->setOwnerElement( this );
    return r;
}

Attr ElementImpl::removeAttributeNode( AttrImpl* oldAttr, int& exceptioncode )
{
  if (!oldAttr || oldAttr->ownerElement() != this) {
    exceptioncode = DOMException::NOT_FOUND_ERR;
    return 0;
  }

  if (isReadOnly()) {
    exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
    return 0;
  }

  if (!namedAttrMap) {
    exceptioncode = DOMException::NOT_FOUND_ERR;
    return 0;
  }

  return attributes(false)->removeAttr(oldAttr);
}


DOMString ElementImpl::getAttributeNS( const DOMString &namespaceURI,
                                       const DOMString &localName,
                                       int& exceptioncode)
{
    if (!localName.implementation()) {
      exceptioncode = DOMException::NOT_FOUND_ERR;
      return DOMString();
    }

    LocalName localname = LocalName::fromString(localName, m_htmlCompat ? IDS_NormalizeLower : IDS_CaseSensitive);
    NamespaceName namespacename = NamespaceName::fromString(namespaceURI);

    NodeImpl::Id id = makeId(namespacename.id(), localname.id());
    return getAttribute(id, emptyPrefixName, true);
}

void ElementImpl::removeAttributeNS( const DOMString &namespaceURI,
                                 const DOMString &localName,
                                 int& exceptioncode)
{
    if (!localName.implementation()) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    NamespaceName namespacename = NamespaceName::fromString(namespaceURI);
    LocalName localname = LocalName::fromString(localName, m_htmlCompat ? IDS_NormalizeLower : IDS_CaseSensitive);

    NodeImpl::Id id = makeId(namespacename.id(), localname.id());
    attributes(false)->removeNamedItem(id, emptyPrefixName, true, exceptioncode);
}

AttrImpl* ElementImpl::getAttributeNodeNS( const DOMString &namespaceURI,
                                  const DOMString &localName,
                                  int& exceptioncode )
{
    if (!localName.implementation()) {
      exceptioncode = DOMException::NOT_FOUND_ERR;
      return 0;
    }

    NamespaceName namespacename = NamespaceName::fromString(namespaceURI);
    LocalName localname = LocalName::fromString(localName, m_htmlCompat ? IDS_NormalizeLower : IDS_CaseSensitive);

    NodeImpl::Id id = makeId(namespacename.id(), localname.id());
    if (!attributes(true)) return 0;
    return static_cast<AttrImpl*>(attributes()->getNamedItem(id, emptyPrefixName, true));
}

Attr ElementImpl::setAttributeNodeNS( AttrImpl* newAttr, int& exceptioncode )
{
    if (!newAttr) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return 0;
    }
    // WRONG_DOCUMENT_ERR and INUSE_ATTRIBUTE_ERR are already tested & thrown by setNamedItem
    Attr r = attributes(false)->setNamedItem(newAttr, emptyPrefixName, true, exceptioncode);
    if ( !exceptioncode )
        newAttr->setOwnerElement( this );
    return r;
}


DOMString ElementImpl::nodeName() const
{
    return tagName();
}

DOMString ElementImpl::namespaceURI() const
{
   return NamespaceName::fromId(namespacePart(id())).toString();
}

void ElementImpl::setPrefix( const DOMString &_prefix, int &exceptioncode )
{
    checkSetPrefix(_prefix, exceptioncode);
    if (exceptioncode)
        return;
    m_prefix = PrefixName::fromString(_prefix);
}

short ElementImpl::tabIndex() const
{
    return m_elementHasRareData ? rareData()->tabIndex() : 0;
}

void ElementImpl::setTabIndex(short _tabIndex)
{
    createRareData()->setTabIndex(_tabIndex);
}

void ElementImpl::setNoTabIndex()
{
    if (!m_elementHasRareData)
        return;
    rareData()->m_hasTabIndex = false;
}

bool ElementImpl::hasTabIndex() const
{
    return m_elementHasRareData ? rareData()->m_hasTabIndex : false;
}

void ElementImpl::defaultEventHandler(EventImpl *e)
{
    if (!e->defaultHandled() && document()->part() && e->id() == EventImpl::KEYPRESS_EVENT && e->isKeyRelatedEvent()) {
        const KHTMLPart* part = document()->part();
        bool isContentEditableElement = part->isEditable() || (focused() && isContentEditable());
        if (isContentEditableElement || part->isCaretMode()) {
            if (document()->view() && document()->view()->caretKeyPressEvent(static_cast<KeyEventBaseImpl*>(e)->qKeyEvent())) {
                e->setDefaultHandled();
                return;
            }
            if (isContentEditableElement && part->editor()->handleKeyEvent(static_cast<KeyEventBaseImpl*>(e)->qKeyEvent())) {
                e->setDefaultHandled();
                return;
            }
        }
    }

    if (m_render && m_render->scrollsOverflow()) {
        switch( e->id() ) {
          case EventImpl::KEYDOWN_EVENT:
          case EventImpl::KEYUP_EVENT:
          case EventImpl::KEYPRESS_EVENT:
            if (!focused() || e->target() != this)
              break;
          // fall through
          case EventImpl::KHTML_MOUSEWHEEL_EVENT:
            if (m_render->handleEvent(*e))
                e->setDefaultHandled();
          default:
            break;
        }
    }
}

void ElementImpl::createAttributeMap() const
{
    namedAttrMap = new NamedAttrMapImpl(const_cast<ElementImpl*>(this));
    namedAttrMap->ref();
}

RenderStyle *ElementImpl::styleForRenderer(RenderObject * /*parentRenderer*/)
{
    return document()->styleSelector()->styleForElement(this);
}

RenderObject *ElementImpl::createRenderer(RenderArena *arena, RenderStyle *style)
{
    if (document()->documentElement() == this && style->display() == NONE) {
        // Ignore display: none on root elements.  Force a display of block in that case.
        RenderBlock* result = new (arena) RenderBlock(this);
        if (result) result->setStyle(style);
        return result;
    }
    return RenderObject::createObject(this, style);
}

void ElementImpl::attach()
{
    assert(!attached());
    assert(!m_render);
    assert(parentNode());

#if SPEED_DEBUG < 1
    createRendererIfNeeded();
#endif

    NodeBaseImpl::attach();
}

void ElementImpl::close()
{
    NodeImpl::close();

    // Trigger all the addChild changes as one large dynamic appendChildren change
    if (attached())
        backwardsStructureChanged();
}

void ElementImpl::detach()
{
    document()->dynamicDomRestyler().resetDependencies(this);

    if (ElementRareDataImpl* rd = rareData())
        rd->resetComputedStyle();

    NodeBaseImpl::detach();
}

void ElementImpl::structureChanged()
{
    NodeBaseImpl::structureChanged();

    if (!document()->renderer())
        return; // the document is about to be destroyed

    document()->dynamicDomRestyler().restyleDependent(this, StructuralDependency);
    // In theory BackwardsStructurualDependencies are indifferent to prepend,
    // but it's too rare to optimize.
    document()->dynamicDomRestyler().restyleDependent(this, BackwardsStructuralDependency);
}

void ElementImpl::backwardsStructureChanged()
{
    NodeBaseImpl::backwardsStructureChanged();

    if (!document()->renderer())
        return; // the document is about to be destroyed

    // Most selectors are not affected by append. Fire the few that are.
    document()->dynamicDomRestyler().restyleDependent(this, BackwardsStructuralDependency);
}

void ElementImpl::attributeChanged(NodeImpl::Id id)
{
    if (!document()->renderer())
        return; // the document is about to be destroyed

#if 0 // one-one dependencies for attributes disabled
    document()->dynamicDomRestyler().restyleDependent(this, AttributeDependency);
#endif
    if (document()->dynamicDomRestyler().checkDependency(id, PersonalDependency))
        setChanged(true);
    if (document()->dynamicDomRestyler().checkDependency(id, AncestorDependency))
        setChangedAscendentAttribute(true);
    if (document()->dynamicDomRestyler().checkDependency(id, PredecessorDependency) && parent())
        // Any element that dependt on a predecessors attribute, also depend structurally on parent
        parent()->structureChanged();
}

void ElementImpl::recalcStyle( StyleChange change )
{
    // ### should go away and be done in renderobject
    RenderStyle* _style = m_render ? m_render->style() : 0;
    bool hasParentRenderer = parent() ? parent()->attached() : false;

    if ((change > NoChange || changed())) {
        if (ElementRareDataImpl* rd = rareData())
            rd->resetComputedStyle();
    }

#if 0
    const char* debug;
    switch(change) {
    case NoChange: debug = "NoChange";
        break;
    case NoInherit: debug= "NoInherit";
        break;
    case Inherit: debug = "Inherit";
        break;
    case Force: debug = "Force";
        break;
    }
    qDebug("recalcStyle(%d: %s, changed: %d)[%p: %s]", change, debug, changed(), this, tagName().string().toLatin1().constData());
#endif
    if ( hasParentRenderer && (change >= Inherit || changed() || (change==NoInherit && affectedByNoInherit())) ) {
        RenderStyle *newStyle = document()->styleSelector()->styleForElement(this);
        newStyle->ref();
        StyleChange ch = diff( _style, newStyle );
        if (ch == Detach) {
            if (attached()) detach();
            // ### Suboptimal. Style gets calculated again.
            attach();
            // attach recalulates the style for all children. No need to do it twice.
            setChanged( false );
            setHasChangedChild( false );
            newStyle->deref();
            return;
        }
        else if (ch != NoChange) {
            if( m_render ) {
                m_render->setStyle(newStyle);
            }
        }
        newStyle->deref();

        if ( change != Force)
            change = ch;
    }
    // If a changed attribute has ancestor dependencies, restyle all children
    if (changedAscendentAttribute()) {
        change = Force;
        setChangedAscendentAttribute(false);
    }

    NodeImpl *n;
    for (n = _first; n; n = n->nextSibling()) {
        if ( change >= Inherit || n->hasChangedChild() || n->changed() ||
             ( change == NoInherit && n->affectedByNoInherit() )
           ) {
	    //qDebug("    (%p) calling recalcStyle on child %p/%s, change=%d", this, n, n->isElementNode() ? ((ElementImpl *)n)->tagName().string().toLatin1().constData() : n->isTextNode() ? "text" : "unknown", change );
            n->recalcStyle( change );
        }
    }

    setChanged( false );
    setHasChangedChild( false );
}

bool ElementImpl::isFocusableImpl(FocusType ft) const
{
    if (m_render && m_render->scrollsOverflow())
        return true;

    // See WAI-ARIA 1.0, UA implementor's guide, 3.1 for the rules this
    // implements.
    if (hasTabIndex()) {
        int ti = tabIndex();

        // Negative things are focusable, but not in taborder
        if (ti < 0)
            return (ft != FT_Tab);
        else // ... while everything else is completely focusable
            return true;
    }

    // Only make editable elements selectable if its parent element
    // is not editable. FIXME: this is not 100% right as non-editable elements
    // within editable elements are focusable too.
    return isContentEditable() && !(parentNode() && parentNode()->isContentEditable());
}

bool ElementImpl::isContentEditable() const
{
    if (document()->part() && document()->part()->isEditable())
        return true;

    // document()->updateRendering();

    if (!renderer()) {
        if (parentNode())
            return parentNode()->isContentEditable();
        else
            return false;
    }

    return renderer()->style()->userInput() == UI_ENABLED;
}

void ElementImpl::setContentEditable(bool enabled) {
    // FIXME: the approach is flawed, better use an enum instead of bool
    int value;
    if (enabled)
        value = CSS_VAL_ENABLED;
    else {
        // Intelligently use "none" or "disabled", depending on the type of
        // element
        // FIXME: intelligence not impl'd yet
        value = CSS_VAL_NONE;

        // FIXME: reset caret if it is in this node or a child
    }/*end if*/
    // FIXME: use addCSSProperty when I get permission to move it here
//    qDebug() << "CSS_PROP__KHTML_USER_INPUT: "<< value << endl;
    getInlineStyleDecls()->setProperty(CSS_PROP__KHTML_USER_INPUT, value, false);
    setChanged();
}

// DOM Section 1.1.1
bool ElementImpl::childAllowed( NodeImpl *newChild )
{
    if (!childTypeAllowed(newChild->nodeType()))
        return false;

    // ### check xml element allowedness according to DTD

    // If either this node or the other node is an XML element node, allow regardless (we don't do DTD checks for XML
    // yet)
    if (isXMLElementNode() || newChild->isXMLElementNode())
	return true;
    else
	return checkChild(id(), newChild->id(), document()->inStrictMode());
}

bool ElementImpl::childTypeAllowed( unsigned short type )
{
    switch (type) {
        case Node::ELEMENT_NODE:
        case Node::TEXT_NODE:
        case Node::COMMENT_NODE:
        case Node::PROCESSING_INSTRUCTION_NODE:
        case Node::CDATA_SECTION_NODE:
        case Node::ENTITY_REFERENCE_NODE:
            return true;
            break;
        default:
            return false;
    }
}

void ElementImpl::scrollIntoView(bool /*alignToTop*/)
{
    // ###
    qWarning() << "non-standard scrollIntoView() not implemented";
}

void ElementImpl::createNonCSSDecl()
{
    assert(!m_hasCombinedStyle);
    CSSInlineStyleDeclarationImpl *ild = m_style.inlineDecls;
    m_style.combinedDecls = new CombinedStyleDecl;
    m_style.combinedDecls->inlineDecls = ild;
    CSSStyleDeclarationImpl *ncd = new CSSStyleDeclarationImpl(0);
    m_style.combinedDecls->nonCSSDecls = ncd;
    ncd->ref();
    ncd->setParent(document()->elementSheet());
    ncd->setNode(this);
    ncd->setStrictParsing( false );
    m_hasCombinedStyle = true;
}

CSSInlineStyleDeclarationImpl *ElementImpl::getInlineStyleDecls()
{
    if (!inlineStyleDecls()) createInlineDecl();
        return inlineStyleDecls();
}

void ElementImpl::createInlineDecl( )
{
    assert( !m_style.inlineDecls || (m_hasCombinedStyle && !m_style.combinedDecls->inlineDecls) );

    CSSInlineStyleDeclarationImpl *dcl = new CSSInlineStyleDeclarationImpl(0);
    dcl->ref();
    dcl->setParent(document()->elementSheet());
    dcl->setNode(this);
    dcl->setStrictParsing( !document()->inCompatMode() );
    if (m_hasCombinedStyle)
        m_style.combinedDecls->inlineDecls = dcl;
    else
        m_style.inlineDecls = dcl;
}

void ElementImpl::dispatchAttrRemovalEvent(NodeImpl::Id /*id*/, DOMStringImpl * /*value*/)
{
    // ### enable this stuff again
    if (!document()->hasListenerType(DocumentImpl::DOMATTRMODIFIED_LISTENER))
	return;
    //int exceptioncode = 0;
    //dispatchEvent(new MutationEventImpl(EventImpl::DOMATTRMODIFIED_EVENT,true,false,attr,attr->value(),
    //attr->value(), document()->attrName(attr->id()),MutationEvent::REMOVAL),exceptioncode);
}

void ElementImpl::dispatchAttrAdditionEvent(NodeImpl::Id /*id*/, DOMStringImpl * /*value*/)
{
    // ### enable this stuff again
    if (!document()->hasListenerType(DocumentImpl::DOMATTRMODIFIED_LISTENER))
	return;
   //int exceptioncode = 0;
   //dispatchEvent(new MutationEventImpl(EventImpl::DOMATTRMODIFIED_EVENT,true,false,attr,attr->value(),
   //attr->value(),document()->attrName(attr->id()),MutationEvent::ADDITION),exceptioncode);
}

void ElementImpl::updateId(DOMStringImpl* oldId, DOMStringImpl* newId)
{
    if (!inDocument())
        return;

    if (oldId && oldId->l)
        removeId(DOMString(oldId));

    if (newId && newId->l)
        addId(DOMString(newId));
}

void ElementImpl::removeId(const DOMString& id)
{
  document()->getElementByIdCache().remove(id, this);
}

void ElementImpl::addId(const DOMString& id)
{
  document()->getElementByIdCache().add(id, this);
}

void ElementImpl::insertedIntoDocument()
{
    // need to do superclass processing first so inDocument() is true
    // by the time we reach updateId
    NodeBaseImpl::insertedIntoDocument();

    if (hasID()) {
        DOMString id = getAttribute(ATTR_ID);
        updateId(0, id.implementation());
    }
}

void ElementImpl::removedFromDocument()
{
    if (hasID()) {
        DOMString id = getAttribute(ATTR_ID);
        updateId(id.implementation(), 0);
    }

    NodeBaseImpl::removedFromDocument();
}

DOMString ElementImpl::openTagStartToString(bool expandurls) const
{
    DOMString result = DOMString("<") + nonCaseFoldedTagName();

    NamedAttrMapImpl *attrMap = attributes(true);

    if (attrMap) {
	unsigned numAttrs = attrMap->length();
	for (unsigned i = 0; i < numAttrs; i++) {
	    result += " ";

        const AttributeImpl& attribute = attrMap->attributeAt(i);
	    AttrImpl *attr = attribute.attr();

	    if (attr) {
		result += attr->toString();
	    } else {
            //FIXME: should use prefix too and depends on html/xhtml case
            PrefixName prefix = attribute.m_prefix;
            DOMString current;
            if (prefix.id())
                current = prefix.toString() + DOMString(":") + attribute.localName();
            else
                current = attribute.localName();
            if (m_htmlCompat)
                current = current.lower();
            result += current;
		if (!attribute.value().isNull()) {
		    result += "=\"";
		    // FIXME: substitute entities for any instances of " or '
		    // Expand out all urls, i.e. the src and href attributes
		    if(expandurls && ( attribute.id() == ATTR_SRC || attribute.id() == ATTR_HREF))
			if(document()) {
                            //We need to sanitize the urls - strip out the passwords.
			    //FIXME:   are src=  and href=  the only places that might have a password and need to be sanitized?
			    QUrl safeURL(document()->completeURL(attribute.value().string()));
			    safeURL.setPassword(QString());
			    result += Qt::escape(safeURL.toDisplayString());
			}
		        else {
		 	    qWarning() << "document() returned false";
			    result += attribute.value();
			}
		    else
		        result += attribute.value();
		    result += "\"";
		}
	    }
	}
    }

    return result;
}
DOMString ElementImpl::selectionToString(NodeImpl *selectionStart, NodeImpl *selectionEnd, int startOffset, int endOffset, bool &found) const
{
    DOMString result = openTagStartToString();

    if (hasChildNodes()) {
	result += ">";

	for (NodeImpl *child = firstChild(); child != NULL; child = child->nextSibling()) {
	    result += child->selectionToString(selectionStart, selectionEnd, startOffset, endOffset, found); // this might set found to true
	    if(child == selectionEnd)
	        found = true;
	    if(found) break;
	}

	result += "</";
	result += nonCaseFoldedTagName();
	result += ">";
    } else {
	result += " />";
    }

    return result;
}

DOMString ElementImpl::toString() const
{
    QString result = openTagStartToString().string(); //Accumulate in QString, since DOMString can't append well.

    if (hasChildNodes()) {
	result += ">";

	for (NodeImpl *child = firstChild(); child != NULL; child = child->nextSibling()) {
	    DOMString kid = child->toString();
	    result += QString::fromRawData(kid.unicode(), kid.length());
	}

	result += "</";
	result += nonCaseFoldedTagName().string();
	result += ">";
    } else if (result.length() == 1) {
	// ensure we do not get results like < /> can happen when serialize document
        result = "";
    } else {
	result += " />";
    }

    return result;
}

RenderStyle* ElementImpl::computedStyle()
{
    if (m_render && m_render->style())
	return m_render->style();

    if (!attached())
        // FIXME: Try to do better than this. Ensure that styleForElement() works for elements that are not in the
        // document tree and figure out when to destroy the computed style for such elements.
        return 0;

    ElementRareDataImpl* rd = createRareData();
    if (!rd->m_computedStyle) {
        rd->m_computedStyle = document()->styleSelector()->styleForElement(this, parent() ? parent()->computedStyle() : 0);
        rd->m_computedStyle->ref();
    }
    return rd->m_computedStyle;
}

// ElementTraversal API
ElementImpl* ElementImpl::firstElementChild() const
{
    NodeImpl* n = firstChild();
    while (n && !n->isElementNode())
        n = n->nextSibling();
    return static_cast<ElementImpl*>(n);
}

ElementImpl* ElementImpl::lastElementChild() const
{
    NodeImpl* n = lastChild();
    while (n && !n->isElementNode())
        n = n->previousSibling();
    return static_cast<ElementImpl*>(n);
}

ElementImpl* ElementImpl::previousElementSibling() const
{
    NodeImpl* n = previousSibling();
    while (n && !n->isElementNode())
        n = n->previousSibling();
    return static_cast<ElementImpl*>(n);
}

ElementImpl* ElementImpl::nextElementSibling() const
{
    NodeImpl* n = nextSibling();
    while (n && !n->isElementNode())
        n = n->nextSibling();
    return static_cast<ElementImpl*>(n);
}

unsigned ElementImpl::childElementCount() const
{
    unsigned count = 0;
    NodeImpl* n = firstChild();
    while (n) {
        count += n->isElementNode();
        n = n->nextSibling();
    }
    return count;
}

void ElementImpl::blur()
{
    if(document()->focusNode() == this)
        document()->setFocusNode(0);
}

void ElementImpl::focus()
{
    document()->setFocusNode(this);
}

void ElementImpl::synchronizeStyleAttribute() const
{
    assert(inlineStyleDecls() && m_needsStyleAttributeUpdate);
    m_needsStyleAttributeUpdate = false;
    DOMString value = inlineStyleDecls()->cssText();
    attributes()->setValueWithoutElementUpdate(ATTR_STYLE, value.implementation());
}


// -------------------------------------------------------------------------

XMLElementImpl::XMLElementImpl(DocumentImpl *doc, NamespaceName namespacename, LocalName localName, PrefixName prefix)
    : ElementImpl(doc)
{
    // Called from createElement(). In this case localName, prefix, and namespaceURI all need to be null.
    m_localName = localName;
    m_namespace = namespacename;
    m_prefix = prefix;
}

XMLElementImpl::~XMLElementImpl()
{
}

WTF::PassRefPtr<NodeImpl> XMLElementImpl::cloneNode ( bool deep )
{
    WTF::RefPtr<ElementImpl> clone = new XMLElementImpl(docPtr(), NamespaceName::fromId(namespacePart(id())), LocalName::fromId(localNamePart(id())), m_prefix);
    finishCloneNode( clone.get(), deep );
    return clone;
}

void XMLElementImpl::parseAttribute(AttributeImpl *attr)
{
    if (attr->id() == ATTR_ID) {
        setHasID();
        document()->incDOMTreeVersion(DocumentImpl::TV_IDNameHref);
    }

    // Note: we do not want to handle ATTR_CLASS here, since the
    // class concept applies only to specific languages, like
    // HTML and SVG, not generic XML.
}

// -------------------------------------------------------------------------

NamedAttrMapImpl::NamedAttrMapImpl(ElementImpl *element)
  : m_element(element)
{
}

NamedAttrMapImpl::~NamedAttrMapImpl()
{
    unsigned len = m_attrs.size();
    for (unsigned i = 0; i < len; i++)
        m_attrs[i].free();
    m_attrs.clear();
}

NodeImpl *NamedAttrMapImpl::getNamedItem(NodeImpl::Id id, const PrefixName& prefix, bool nsAware)
{
    if (!m_element)
        return 0;

    int index = find(id, prefix, nsAware);
    return (index < 0) ? 0 : m_attrs[index].createAttr(m_element, m_element->docPtr());
}

Node NamedAttrMapImpl::removeNamedItem(NodeImpl::Id id, const PrefixName& prefix, bool nsAware, int &exceptioncode )
{
    if (!m_element) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return 0;
    }

    // NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return 0;
    }
    int index = find(id, prefix, nsAware);
    if (index < 0) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return 0;
    }

    id = m_attrs[index].id();
    if (id == ATTR_ID)
       m_element->updateId(m_attrs[index].val(), 0);
    Node removed(m_attrs[index].createAttr(m_element, m_element->docPtr()));
    m_attrs[index].free(); // Also sets the remove'd ownerElement to 0
    m_attrs.remove(index);
    m_element->parseNullAttribute(id, prefix);
    m_element->attributeChanged(id);
    return removed;
}

Node NamedAttrMapImpl::setNamedItem(NodeImpl* arg, const PrefixName& prefix, bool nsAware, int &exceptioncode )
{
    if (!m_element || !arg) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return 0;
    }

    // NO_MODIFICATION_ALLOWED_ERR: Raised if this map is readonly.
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return 0;
    }

    // WRONG_DOCUMENT_ERR: Raised if arg was created from a different document than the one that created this map.
    if (arg->document() != m_element->document()) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return 0;
    }

    // HIERARCHY_REQUEST_ERR: Raised if an attempt is made to add a node doesn't belong in this NamedNodeMap
    if (!arg->isAttributeNode()) {
        exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
        return 0;
    }
    AttrImpl *attr = static_cast<AttrImpl*>(arg);

    // INUSE_ATTRIBUTE_ERR: Raised if arg is an Attr that is already an attribute of another Element object.
    // The DOM user must explicitly clone Attr nodes to re-use them in other elements.
    if (attr->ownerElement() && attr->ownerElement() != m_element) {
        exceptioncode = DOMException::INUSE_ATTRIBUTE_ERR;
        return 0;
    }

    if (attr->ownerElement() == m_element) {
	// Already have this attribute.
	// DOMTS core-1 test "hc_elementreplaceattributewithself" says we should return it.
	return attr;
    }

    int index = find(attr->id(), prefix, nsAware);
    if (index >= 0) {
        if (attr->id() == ATTR_ID)
            m_element->updateId(m_attrs[index].val(), attr->val());

        Node replaced = m_attrs[index].createAttr(m_element, m_element->docPtr());
        m_attrs[index].free();
        m_attrs[index].m_localName = emptyLocalName; /* "has implementation" flag */
        m_attrs[index].m_data.attr = attr;
        m_attrs[index].m_data.attr->ref();
        attr->setElement(m_element);
        m_element->parseAttribute(&m_attrs[index]);
        m_element->attributeChanged(m_attrs[index].id());
        // ### dispatch mutation events
        return replaced;
    }

    // No existing attribute; add to list
    AttributeImpl attribute;
    attribute.m_localName = emptyLocalName; /* "has implementation" flag */
    attribute.m_namespace = NamespaceName::fromId(0);
    attribute.m_prefix = emptyPrefixName;
    attribute.m_data.attr = attr;
    attribute.m_data.attr->ref();
    m_attrs.append(attribute);

    attr->setElement(m_element);
    if (attr->id() == ATTR_ID)
        m_element->updateId(0, attr->val());
    m_element->parseAttribute(&m_attrs.last());
    m_element->attributeChanged(m_attrs.last().id());
    // ### dispatch mutation events

    return 0;
}

NodeImpl *NamedAttrMapImpl::item(unsigned index)
{
    return (index >= m_attrs.size()) ? 0 : m_attrs[index].createAttr(m_element, m_element->docPtr());
}

NodeImpl::Id NamedAttrMapImpl::idAt(unsigned index) const
{
    assert(index < m_attrs.size());
    return m_attrs[index].id();
}

DOMStringImpl *NamedAttrMapImpl::valueAt(unsigned index) const
{
    assert(index < m_attrs.size());
    return m_attrs[index].val();
}

DOMStringImpl *NamedAttrMapImpl::getValue(NodeImpl::Id id, const PrefixName& prefix, bool nsAware) const
{
    int index = find(id, prefix, nsAware);
    return index < 0 ? 0 : m_attrs[index].val();
}

void NamedAttrMapImpl::setValue(NodeImpl::Id id, DOMStringImpl *value, const PrefixName& prefix, bool nsAware)
{
    if (!id) return;
    // Passing in a null value here causes the attribute to be removed. This is a khtml extension
    // (the spec does not specify what to do in this situation).
    int exceptioncode = 0;
    if (!value) {
        removeNamedItem(id, prefix, nsAware, exceptioncode);
	return;
    }
    int index = find(id, prefix, nsAware);
    if (index >= 0) {
        if (m_attrs[index].attr())
            m_attrs[index].attr()->setPrefix(prefix.toString(), exceptioncode);
        else
            m_attrs[index].m_prefix = prefix;
	    m_attrs[index].setValue(value,m_element);
	    // ### dispatch mutation events
	    return;
	}

    AttributeImpl attr;
    attr.m_localName = LocalName::fromId(localNamePart(id));
    attr.m_namespace = NamespaceName::fromId(namespacePart(id));
    attr.m_prefix = prefix;
    attr.m_data.value = value;
    attr.m_data.value->ref();
    m_attrs.append(attr);

    if (m_element) {
        if (id == ATTR_ID)
            m_element->updateId(0, value);
        m_element->parseAttribute(&m_attrs.last());
        m_element->attributeChanged(m_attrs.last().id());
    }
    // ### dispatch mutation events
}

Attr NamedAttrMapImpl::removeAttr(AttrImpl *attr)
{
    unsigned len = m_attrs.size();
    for (unsigned i = 0; i < len; i++) {
	if (m_attrs[i].attr() == attr) {
	    NodeImpl::Id id = m_attrs[i].id();
	    if (id == ATTR_ID)
	        m_element->updateId(attr->val(), 0);
	    Node removed(m_attrs[i].createAttr(m_element,m_element->docPtr()));
	    m_attrs[i].free();
        m_attrs.remove(i);
	    m_element->parseNullAttribute(id, PrefixName::fromString(attr->prefix()));
	    m_element->attributeChanged(id);
	    // ### dispatch mutation events
	    return removed;
	}
    }

    return 0;
}

void NamedAttrMapImpl::copyAttributes(NamedAttrMapImpl *other)
{
    assert(m_element);
    unsigned i;
    unsigned len = m_attrs.size();
    for (i = 0; i < len; i++) {
        if (m_attrs[i].id() == ATTR_ID)
            m_element->updateId(m_attrs[i].val(), 0);
	m_attrs[i].free();
    }
    m_attrs.resize(other->length());
    len = m_attrs.size();
    for (i = 0; i < len; i++) {
        m_attrs[i].m_localName = other->m_attrs[i].m_localName;
        m_attrs[i].m_prefix = other->m_attrs[i].m_prefix;
        m_attrs[i].m_namespace = other->m_attrs[i].m_namespace;
	if (m_attrs[i].m_localName.id()) {
	    m_attrs[i].m_data.value = other->m_attrs[i].m_data.value;
	    m_attrs[i].m_data.value->ref();
	}
	else {
	    WTF::RefPtr<NodeImpl> clonedAttr = other->m_attrs[i].m_data.attr->cloneNode(true);
	    m_attrs[i].m_data.attr = static_cast<AttrImpl*>(clonedAttr.get());
	    m_attrs[i].m_data.attr->ref();
	    m_attrs[i].m_data.attr->setElement(m_element);
	}
	if (m_attrs[i].id() == ATTR_ID)
	   m_element->updateId(0, m_attrs[i].val());
	m_element->parseAttribute(&m_attrs[i]);
	m_element->attributeChanged(m_attrs[i].id());
    }
}

int NamedAttrMapImpl::find(NodeImpl::Id id, const PrefixName& prefix, bool nsAware) const
{
    //qDebug() << "In find:" << getPrintableName(id) << "[" << prefix.toString() << prefix.id() << "]" << nsAware << endl;
    //qDebug() << "m_attrs.size()" << m_attrs.size() << endl;
    unsigned len = m_attrs.size();
    for (unsigned i = 0; i < len; ++i) {
        //qDebug() << "check attr[" << i << "]" << getPrintableName(m_attrs[i].id()) << "prefix:" << m_attrs[i].prefix() << endl;
        if (nsAware && namespacePart(id) == anyNamespace && localNamePart(id) == localNamePart(m_attrs[i].id()))
            return i;
        if ((nsAware && id == m_attrs[i].id()) || (!nsAware && localNamePart(id) == localNamePart(m_attrs[i].id()) && prefix == m_attrs[i].prefixName())) {
            //qDebug() << "attribute matched: exiting..." << endl;
            return i;
        }
    }
    //qDebug() << "attribute doesn't match: exiting... with -1" << endl;
    return -1;
}

void NamedAttrMapImpl::setElement(ElementImpl *element)
{
    assert(!m_element);
    m_element = element;

    unsigned len = m_attrs.size();
    for (unsigned i = 0; i < len; i++)
	if (m_attrs[i].attr())
	    m_attrs[i].attr()->setElement(element);
}

void NamedAttrMapImpl::detachFromElement()
{
    // This makes the map invalid; nothing can really be done with it now since it's not
    // associated with an element. But we have to keep it around in memory in case there
    // are still references to it.
    m_element = 0;
    unsigned len = m_attrs.size();
    for (unsigned i = 0; i < len; i++)
        m_attrs[i].free();
    m_attrs.clear();
}

void NamedAttrMapImpl::setClass(const DOMString& string)
{
    if (!m_element)
        return;

    if (!m_element->hasClass()) {
        m_classNames.clear();
        return;
    }

    m_classNames.parseClassAttribute(string, m_element->document()->htmlMode() != DocumentImpl::XHtml &&
            m_element->document()->inCompatMode());
}

void NamedAttrMapImpl::setValueWithoutElementUpdate(NodeImpl::Id id, DOMStringImpl* value)
{
    // FIXME properly fix case value == 0
    int index = find(id, emptyPrefixName, true);
    if (index >= 0) {
        m_attrs[index].rewriteValue(value ? value : DOMStringImpl::empty());
        return;
    }

    if (!value)
        return;

    AttributeImpl attr;
    attr.m_localName = LocalName::fromId(localNamePart(id));
    attr.m_namespace = NamespaceName::fromId(namespacePart(id));
    attr.m_prefix = emptyPrefixName;
    attr.m_data.value = value;
    attr.m_data.value->ref();
    m_attrs.append(attr);
}

}
