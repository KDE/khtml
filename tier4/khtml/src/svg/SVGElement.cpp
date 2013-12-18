/*
    Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "wtf/Platform.h"

#if ENABLE(SVG)
#include "SVGElement.h"

/*#include "DOMImplementation.h"*/
#include "Document.h"
/*#include "Event.h"
#include "EventListener.h"
#include "EventNames.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "PlatformString.h"
#include "RenderObject.h"
#include "SVGDocumentExtensions.h"
#include "SVGElementInstance.h"*/
#include "SVGNames.h"
#include "SVGSVGElement.h"
/*#include "SVGURIReference.h"
#include "SVGUseElement.h"
#include "RegisteredEventListener.h"*/

namespace WebCore {

using namespace DOM;

//using namespace HTMLNames;
//using namespace EventNames;

SVGElement::SVGElement(const QualifiedName& tagName, Document* doc)
    : StyledElement(doc) // it's wrong, remove it!!! FIXME: vtokarev
//    : StyledElement(tagName, doc)
//    , m_shadowParent(0)
{
    m_prefix = tagName.prefixId();
}

SVGElement::~SVGElement()
{
}

bool SVGElement::isSupported(StringImpl* feature, StringImpl* version) const
{
    return DOMImplementation::hasFeature(feature, version);
}


String SVGElement::attrid() const
{
    return getAttribute(idAttr);
}


void SVGElement::setId(const String& value, ExceptionCode&)
{
    setAttribute(idAttr, value);
}


String SVGElement::xmlbase() const
{
    return getAttribute(ATTR_XML_BASE);
}

void SVGElement::setXmlbase(const String& value, ExceptionCode&)
{
    setAttribute(ATTR_XML_BASE, value);
}

SVGSVGElement* SVGElement::ownerSVGElement() const
{
    Node* n = isShadowNode() ? const_cast<SVGElement*>(this)->shadowParentNode() : parentNode();
    while (n) {
        if (/*n->hasTagName(SVGNames::svgTag)*/n->id() == SVGNames::svgTag.id())
            return static_cast<SVGSVGElement*>(n);

        n = n->isShadowNode() ? n->shadowParentNode() : n->parentNode();
    }

    return 0;
}

SVGElement* SVGElement::viewportElement() const
{
    // This function needs shadow tree support - as RenderSVGContainer uses this function
    // to determine the "overflow" property. <use> on <symbol> wouldn't work otherwhise.
    /*Node* n = isShadowNode() ? const_cast<SVGElement*>(this)->shadowParentNode() : parentNode();
    while (n) {
        if (n->hasTagName(SVGNames::svgTag) || n->hasTagName(SVGNames::imageTag) || n->hasTagName(SVGNames::symbolTag))
            return static_cast<SVGElement*>(n);

        n = n->isShadowNode() ? n->shadowParentNode() : n->parentNode();
    }*/

    return 0;
}

void SVGElement::addSVGEventListener(/*const AtomicString& eventType*/const EventImpl::EventId& eventType, const Attribute* attr)
{
    // qDebug() << "add listener for: " << EventName::fromId(eventType).toString() << endl;
    Element::setHTMLEventListener(EventName::fromId(eventType), document()->accessSVGExtensions()->
        createSVGEventListener(attr->localName().string(), attr->value(), this));
}

void SVGElement::parseMappedAttribute(MappedAttribute* attr)
{
    // standard events
    if (/*attr->name() == onloadAttr*/attr->id() == ATTR_ONLOAD)
        addSVGEventListener(/*loadEvent*/EventImpl::LOAD_EVENT, attr);
    else if (/*attr->name() == onclickAttr*/attr->id() == ATTR_ONCLICK)
        addSVGEventListener(/*clickEvent*/EventImpl::CLICK_EVENT, attr);
    else /*if (attr->name() == onmousedownAttr)
        addSVGEventListener(mousedownEvent, attr);
    else if (attr->name() == onmousemoveAttr)
        addSVGEventListener(mousemoveEvent, attr);
    else if (attr->name() == onmouseoutAttr)
        addSVGEventListener(mouseoutEvent, attr);
    else if (attr->name() == onmouseoverAttr)
        addSVGEventListener(mouseoverEvent, attr);
    else if (attr->name() == onmouseupAttr)
        addSVGEventListener(mouseupEvent, attr);
    else if (attr->name() == SVGNames::onfocusinAttr)
        addSVGEventListener(DOMFocusInEvent, attr);
    else if (attr->name() == SVGNames::onfocusoutAttr)
        addSVGEventListener(DOMFocusOutEvent, attr);
    else if (attr->name() == SVGNames::onactivateAttr)
        addSVGEventListener(DOMActivateEvent, attr);
    else*/
    if (attr->id() == ATTR_ID) {
        setHasID();
        document()->incDOMTreeVersion(DocumentImpl::TV_IDNameHref);
    } else
        StyledElement::parseAttribute(attr);
}

bool SVGElement::haveLoadedRequiredResources()
{
    Node* child = firstChild();
    while (child) {
        if (child->isSVGElement() && !static_cast<SVGElement*>(child)->haveLoadedRequiredResources())
            return false;
        child = child->nextSibling();
    }
    return true;
}

static bool hasLoadListener(SVGElement* node)
{
    Node* currentNode = node;
    while (currentNode && currentNode->isElementNode()) {
        QList<RegisteredEventListener> *list = static_cast<Element*>(currentNode)->localEventListeners();
        if (list) {
            QList<RegisteredEventListener>::Iterator end = list->end();
            for (QList<RegisteredEventListener>::Iterator it = list->begin(); it != end; ++it)
                if ((*it).useCapture || (*it).eventName.id() == EventImpl::LOAD_EVENT/* || currentNode == node*/)
                    return true;
                /*if ((*it)->eventType() == loadEvent &&
                    (*it)->useCapture() == true || currentNode == node)
                    return true;*/
        }
        currentNode = currentNode->parentNode();
    }

    return false;
}

void SVGElement::sendSVGLoadEventIfPossible(bool sendParentLoadEvents)
{
    // qDebug() << "send svg load event" << endl;
    RefPtr<SVGElement> currentTarget = this;
    // qDebug() << currentTarget << currentTarget->haveLoadedRequiredResources() << endl;
    while (currentTarget && currentTarget->haveLoadedRequiredResources()) {
        RefPtr<Node> parent;
        if (sendParentLoadEvents)
            parent = currentTarget->parentNode(); // save the next parent to dispatch too incase dispatching the event changes the tree
        // qDebug() << hasLoadListener(currentTarget.get()) << endl;
        if (hasLoadListener(currentTarget.get())) {
            //Event* event = new Event(EventImpl::LOAD_EVENT, true/*false*/, false);
            //event->setTarget(currentTarget.get());
            //ExceptionCode ignored = 0;
            //dispatchGenericEvent(/*currentTarget.get(), */event, ignored/*, false*/);
            dispatchHTMLEvent(EventImpl::LOAD_EVENT, false, false);
        }
        currentTarget = (parent && parent->isSVGElement()) ? static_pointer_cast<SVGElement>(parent) : RefPtr<SVGElement>();
    }
}

void SVGElement::finishParsingChildren()
{
    // finishParsingChildren() is called when the close tag is reached for an element (e.g. </svg>)
    // we send SVGLoad events here if we can, otherwise they'll be sent when any required loads finish
    sendSVGLoadEventIfPossible();
}

bool SVGElement::childShouldCreateRenderer(Node* child) const
{
    if (child->isSVGElement())
        return static_cast<SVGElement*>(child)->isValid();
    return false;
}

void SVGElement::insertedIntoDocument()
{
    StyledElement::insertedIntoDocument();
    /*SVGDocumentExtensions* extensions = document()->accessSVGExtensions();

    String resourceId = SVGURIReference::getTarget(id());
    if (extensions->isPendingResource(resourceId)) {
        std::auto_ptr<HashSet<SVGStyledElement*> > clients(extensions->removePendingResource(resourceId));
        if (clients->isEmpty())
            return;

        HashSet<SVGStyledElement*>::const_iterator it = clients->begin();
        const HashSet<SVGStyledElement*>::const_iterator end = clients->end();

        for (; it != end; ++it)
            (*it)->buildPendingResource();

        SVGResource::invalidateClients(*clients);
    }*/
}

static Node* shadowTreeParentElementForShadowTreeElement(Node* node)
{
    for (Node* n = node; n; n = n->parentNode()) {
        /*if (n->isShadowNode())
            return n->shadowParentNode();*/
    }

    return 0;
}

bool SVGElement::dispatchEvent(Event* e, ExceptionCode& ec, bool tempEvent)
{
    Q_UNUSED(e);
    Q_UNUSED(ec);
    Q_UNUSED(tempEvent);
    // qDebug() << "dispatch event" << endl;
    // TODO: This function will be removed in a follow-up patch!

    /*EventTarget* target = this;
    Node* useNode = shadowTreeParentElementForShadowTreeElement(this);

    // If we are a hidden shadow tree element, the target must
    // point to our corresponding SVGElementInstance object
    if (useNode) {
        ASSERT(useNode->hasTagName(SVGNames::useTag));
        SVGUseElement* use = static_cast<SVGUseElement*>(useNode);

        SVGElementInstance* instance = use->instanceForShadowTreeElement(this);

        if (instance)
            target = instance;
    }

    e->setTarget(target);

    RefPtr<FrameView> view = document()->view();
    return EventTargetNode::dispatchGenericEvent(this, e, ec, tempEvent);*/
	ASSERT(false);
	return false;
}

void SVGElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    ASSERT(attr);
    if (!attr)
        return;

    StyledElement::attributeChanged(attr, preserveDecls);
    svgAttributeChanged(attr->name());
}

// for KHTML compatibility
void SVGElement::addCSSProperty(Attribute* attr, int id, const String& value)
{
    Q_UNUSED(attr);
    // qDebug() << "called with: " << id << " " << value << endl;
    /* WARNING: copy&past'ed from HTMLElementImpl class */
    if (!m_hasCombinedStyle) createNonCSSDecl();
    nonCSSStyleDecls()->setProperty(id, value, false);
    setChanged();
}

void SVGElement::addCSSProperty(Attribute* attr, int id, int value)
{
    Q_UNUSED(attr);
    // qDebug() << "called with: " << id << " " << value << endl;
    /* WARNING: copy&past'ed from HTMLElementImpl class */
    if (!m_hasCombinedStyle) createNonCSSDecl();
    nonCSSStyleDecls()->setProperty(id, value, false);
    setChanged();
}


}

#endif // ENABLE(SVG)
