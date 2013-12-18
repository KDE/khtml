// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003 Apple Computer, Inc.
 *  Copyright (C) 2010 Maksim Orlovich (maksim@kde.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "kjs_dom.h"
#include "kjs_dom.lut.h"

#include <khtmlview.h>
#include <xml/dom2_eventsimpl.h>
#include <rendering/render_canvas.h>
#include <rendering/render_layer.h>
#include <xml/dom_nodeimpl.h>
#include <xml/dom_docimpl.h>
#include <xml/dom3_xpathimpl.h>
#include <html/html_baseimpl.h>
#include <html/html_documentimpl.h>
#include <html/html_miscimpl.h>
#include "HTMLAudioElement.h"
#include "HTMLVideoElement.h"
#include "JSHTMLAudioElement.h"
#include "JSHTMLVideoElement.h"
#include <QDebug>
#include <khtml_part.h>
#include <QtCore/QList>

#include "kjs_html.h"
#include "kjs_css.h"
#include "kjs_range.h"
#include "kjs_traversal.h"
#include "kjs_events.h"
#include "kjs_views.h"
#include "kjs_window.h"
#include "kjs_xpath.h"
#include "kjs_clientrect.h"
#include "xmlhttprequest.h"
#include <kjs/PropertyNameArray.h>
#include <dom/dom_exception.h>
#include <dom/html_document.h>
#include <khtmlpart_p.h>

using namespace KJS;
using namespace khtml;
using namespace DOM;

// -------------------------------------------------------------------------
/* Source for DOMNodeConstantsTable.
@begin DOMNodeConstantsTable 21
  ELEMENT_NODE      DOM::Node::ELEMENT_NODE     DontDelete|ReadOnly
  ATTRIBUTE_NODE    DOM::Node::ATTRIBUTE_NODE       DontDelete|ReadOnly
  TEXT_NODE     DOM::Node::TEXT_NODE        DontDelete|ReadOnly
  CDATA_SECTION_NODE    DOM::Node::CDATA_SECTION_NODE   DontDelete|ReadOnly
  ENTITY_REFERENCE_NODE DOM::Node::ENTITY_REFERENCE_NODE    DontDelete|ReadOnly
  ENTITY_NODE       DOM::Node::ENTITY_NODE      DontDelete|ReadOnly
  PROCESSING_INSTRUCTION_NODE DOM::Node::PROCESSING_INSTRUCTION_NODE DontDelete|ReadOnly
  COMMENT_NODE      DOM::Node::COMMENT_NODE     DontDelete|ReadOnly
  DOCUMENT_NODE     DOM::Node::DOCUMENT_NODE        DontDelete|ReadOnly
  DOCUMENT_TYPE_NODE    DOM::Node::DOCUMENT_TYPE_NODE   DontDelete|ReadOnly
  DOCUMENT_FRAGMENT_NODE DOM::Node::DOCUMENT_FRAGMENT_NODE  DontDelete|ReadOnly
  NOTATION_NODE     DOM::Node::NOTATION_NODE        DontDelete|ReadOnly
  XPATH_NAMESPACE_NODE	DOM::Node::XPATH_NAMESPACE_NODE         DontDelete|ReadOnly
  DOCUMENT_POSITION_DISCONNECTED                DOM::Node::DOCUMENT_POSITION_DISCONNECTED  DontDelete|ReadOnly
  DOCUMENT_POSITION_PRECEDING                   DOM::Node::DOCUMENT_POSITION_PRECEDING  DontDelete|ReadOnly
  DOCUMENT_POSITION_FOLLOWING                   DOM::Node::DOCUMENT_POSITION_FOLLOWING   DontDelete|ReadOnly
  DOCUMENT_POSITION_CONTAINS                    DOM::Node::DOCUMENT_POSITION_CONTAINS  DontDelete|ReadOnly
  DOCUMENT_POSITION_CONTAINED_BY                DOM::Node::DOCUMENT_POSITION_CONTAINED_BY  DontDelete|ReadOnly
  DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC     DOM::Node::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC  DontDelete|ReadOnly
@end
*/
IMPLEMENT_CONSTANT_TABLE(DOMNodeConstants,"DOMNodeConstants")

// -------------------------------------------------------------------------
/* Source for DOMNodeProtoTable.
@begin DOMNodeProtoTable 13
  insertBefore	DOMNode::InsertBefore	DontDelete|Function 2
  replaceChild	DOMNode::ReplaceChild	DontDelete|Function 2
  removeChild	DOMNode::RemoveChild	DontDelete|Function 1
  appendChild	DOMNode::AppendChild	DontDelete|Function 1
  hasAttributes	DOMNode::HasAttributes	DontDelete|Function 0
  hasChildNodes	DOMNode::HasChildNodes	DontDelete|Function 0
  cloneNode	DOMNode::CloneNode	DontDelete|Function 1
# DOM2
  normalize	DOMNode::Normalize	DontDelete|Function 0
  isSupported   DOMNode::IsSupported	DontDelete|Function 2
# DOM3
  compareDocumentPosition 	DOMNode::CompareDocumentPosition	DontDelete|Function 1
# from the EventTarget interface
  addEventListener	DOMNode::AddEventListener	DontDelete|Function 3
  removeEventListener	DOMNode::RemoveEventListener	DontDelete|Function 3
  dispatchEvent		DOMNode::DispatchEvent	DontDelete|Function 1
# IE extensions
  contains	DOMNode::Contains		DontDelete|Function 1
  insertAdjacentHTML	DOMNode::InsertAdjacentHTML	DontDelete|Function 2
@end
*/
KJS_IMPLEMENT_PROTOFUNC(DOMNodeProtoFunc)
KJS_IMPLEMENT_PROTOTYPE_IMP("DOMNode", DOMNodeProto, DOMNodeProtoFunc, DOMNodeConstants)
{
    // We need to setup the constructor property to the ctor here, but NodeConstructor::self
    // will try to make us again, since we're in the middle of cacheGlobalObject.
    // To workaround that, register ourselves so the re-entry of cacheGlobalObject
    // will pick us. NodeCtor ctor does the same already, to fix the problem 
    // in the other direction.
    exec->lexicalInterpreter()->globalObject()->put(exec, *name(), this, KJS::Internal | KJS::DontEnum);
    putDirect(exec->propertyNames().constructor, NodeConstructor::self(exec), DontEnum);
}

const ClassInfo DOMNode::info = { "Node", 0, &DOMNodeTable, 0 };

DOMNode::DOMNode(ExecState *exec, DOM::NodeImpl* n)
  : DOMObject(DOMNodeProto::self(exec)), m_impl(n)
{}

DOMNode::DOMNode(JSObject *proto, DOM::NodeImpl* n)
  : DOMObject(proto), m_impl(n)
{}


DOMNode::~DOMNode()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

bool DOMNode::toBoolean(ExecState *) const
{
    return !m_impl.isNull();
}

/* Source for DOMNodeTable.
@begin DOMNodeTable 53
  nodeName	DOMNode::NodeName	DontDelete|ReadOnly
  nodeValue	DOMNode::NodeValue	DontDelete
  nodeType	DOMNode::NodeType	DontDelete|ReadOnly
  parentNode	DOMNode::ParentNode	DontDelete|ReadOnly
  parentElement	DOMNode::ParentElement	DontDelete|ReadOnly
  childNodes	DOMNode::ChildNodes	DontDelete|ReadOnly
  firstChild	DOMNode::FirstChild	DontDelete|ReadOnly
  lastChild	DOMNode::LastChild	DontDelete|ReadOnly
  previousSibling  DOMNode::PreviousSibling DontDelete|ReadOnly
  nextSibling	DOMNode::NextSibling	DontDelete|ReadOnly
  attributes	DOMNode::Attributes	DontDelete|ReadOnly
  namespaceURI	DOMNode::NamespaceURI	DontDelete|ReadOnly
# DOM2
  prefix	DOMNode::Prefix		DontDelete
  localName	DOMNode::LocalName	DontDelete|ReadOnly
  ownerDocument	DOMNode::OwnerDocument	DontDelete|ReadOnly
# DOM3
  textContent   DOMNode::TextContent    DontDelete
# Event handlers
# IE also has: onactivate, onbefore*, oncontextmenu, oncontrolselect, oncut,
# ondeactivate, ondrag*, ondrop, onfocusin, onfocusout, onhelp, onmousewheel,
# onmove*, onpaste, onpropertychange, onreadystatechange, onresizeend/start,
# onselectionchange, onstop
  onabort	DOMNode::OnAbort		DontDelete
  onblur	DOMNode::OnBlur			DontDelete
  onchange	DOMNode::OnChange		DontDelete
  onclick	DOMNode::OnClick		DontDelete
  ondblclick	DOMNode::OnDblClick		DontDelete
  ondragdrop	DOMNode::OnDragDrop		DontDelete
  onerror	DOMNode::OnError		DontDelete
  onfocus	DOMNode::OnFocus       		DontDelete
  onkeydown	DOMNode::OnKeyDown		DontDelete
  onkeypress	DOMNode::OnKeyPress		DontDelete
  onkeyup	DOMNode::OnKeyUp		DontDelete
  onload	DOMNode::OnLoad			DontDelete
  onmousedown	DOMNode::OnMouseDown		DontDelete
  onmousemove	DOMNode::OnMouseMove		DontDelete
  onmouseout	DOMNode::OnMouseOut		DontDelete
  onmouseover	DOMNode::OnMouseOver		DontDelete
  onmouseup	DOMNode::OnMouseUp		DontDelete
  onmove	DOMNode::OnMove			DontDelete
  onreset	DOMNode::OnReset		DontDelete
  onresize	DOMNode::OnResize		DontDelete
  onscroll      DOMNode::OnScroll               DontDelete
  onselect	DOMNode::OnSelect		DontDelete
  onsubmit	DOMNode::OnSubmit		DontDelete
  onunload	DOMNode::OnUnload		DontDelete
# IE extensions
  offsetLeft	DOMNode::OffsetLeft		DontDelete|ReadOnly
  offsetTop	DOMNode::OffsetTop		DontDelete|ReadOnly
  offsetWidth	DOMNode::OffsetWidth		DontDelete|ReadOnly
  offsetHeight	DOMNode::OffsetHeight		DontDelete|ReadOnly
  offsetParent	DOMNode::OffsetParent		DontDelete|ReadOnly
  clientWidth	DOMNode::ClientWidth		DontDelete|ReadOnly
  clientHeight	DOMNode::ClientHeight		DontDelete|ReadOnly
  clientLeft    DOMNode::ClientLeft		DontDelete|ReadOnly
  clientTop	DOMNode::ClientTop		DontDelete|ReadOnly
  scrollLeft	DOMNode::ScrollLeft		DontDelete
  scrollTop	DOMNode::ScrollTop		DontDelete
  scrollWidth   DOMNode::ScrollWidth            DontDelete|ReadOnly
  scrollHeight  DOMNode::ScrollHeight           DontDelete|ReadOnly
  sourceIndex	DOMNode::SourceIndex		DontDelete|ReadOnly
@end
*/

bool DOMNode::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMNode::getOwnPropertySlot " << propertyName.qstring();
#endif
  return getStaticValueSlot<DOMNode, DOMObject>(exec, &DOMNodeTable, this, propertyName, slot);
}

static khtml::RenderObject* handleBodyRootQuirk(const DOM::NodeImpl* node, khtml::RenderObject* rend, int token)
{
  //This emulates the quirks of various height/width properties on the viewport and root. Note that it
  //is (mostly) IE-compatible in quirks, and mozilla-compatible in strict.
  if (!rend) return 0;

  bool quirksMode = rend->style() && rend->style()->htmlHacks();

  //There are a couple quirks here. One is that in quirks mode body is always forwarded to root...
  //This is relevant for even the scrollTop/scrollLeft type properties.
  if (quirksMode && node->id() == ID_BODY) {
    while (rend->parent() && !rend->isRoot())
      rend = rend->parent();
  }

  //Also, some properties of the root are really done in terms of the viewport.
  //These are  {offset/client}{Height/Width}. The offset versions do it only in
  //quirks mode, the client always.
  if (!rend->isRoot()) return rend; //Don't care about non-root things here!
  bool needViewport = false;

  switch (token) {
    case DOMNode::OffsetHeight:
    case DOMNode::OffsetWidth:
      needViewport = quirksMode;
      break;
    case DOMNode::ClientHeight:
    case DOMNode::ClientWidth:
    case DOMNode::ClientLeft:
    case DOMNode::ClientTop:
      needViewport = true;
      break;
  }

  if (needViewport) {
    //Scan up to find the new target
    while (rend->parent())
      rend = rend->parent();
  }
  return rend;
}

JSValue* DOMNode::getValueProperty(ExecState *exec, int token) const
{
  NodeImpl& node = *impl();
  switch (token) {
  case NodeName:
    return jsString(node.nodeName());
  case NodeValue:
    return ::getStringOrNull(node.nodeValue()); // initially null, per domts/level1/core/hc_documentcreateelement.html
  case NodeType:
    return jsNumber((unsigned int)node.nodeType());
  case ParentNode:
    return getDOMNode(exec,node.parentNode());
  case ParentElement: // IE only apparently
    return getDOMNode(exec,node.parentNode());
  case ChildNodes:
    return getDOMNodeList(exec,node.childNodes());
  case FirstChild:
    return getDOMNode(exec,node.firstChild());
  case LastChild:
    return getDOMNode(exec,node.lastChild());
  case PreviousSibling:
    return getDOMNode(exec,node.previousSibling());
  case NextSibling:
    return getDOMNode(exec,node.nextSibling());
  case Attributes: {
    DOM::NamedNodeMapImpl* attrs = 0;
    if (node.isElementNode()) {
      DOM::ElementImpl& el = static_cast<DOM::ElementImpl&>(node);
      attrs = el.attributes();
    }
    return getDOMNamedNodeMap(exec,attrs);
  }
  case NamespaceURI:
    return ::getStringOrNull(node.namespaceURI()); // Moz returns null if not set (dom/namespaces.html)
  case Prefix:
    return ::getStringOrNull(node.prefix());  // Moz returns null if not set (dom/namespaces.html)
  case LocalName:
    return ::getStringOrNull(node.localName());  // Moz returns null if not set (dom/namespaces.html)
  case OwnerDocument:
    return getDOMNode(exec,node.ownerDocument());
  case TextContent:
    return ::getStringOrNull(node.textContent());
  case OnAbort:
    return getListener(DOM::EventImpl::ABORT_EVENT);
  case OnBlur:
    return getListener(DOM::EventImpl::BLUR_EVENT);
  case OnChange:
    return getListener(DOM::EventImpl::CHANGE_EVENT);
  case OnClick:
    return getListener(DOM::EventImpl::KHTML_ECMA_CLICK_EVENT);
  case OnDblClick:
    return getListener(DOM::EventImpl::KHTML_ECMA_DBLCLICK_EVENT);
  case OnDragDrop:
    return getListener(DOM::EventImpl::KHTML_DRAGDROP_EVENT);
  case OnError:
    return getListener(DOM::EventImpl::ERROR_EVENT);
  case OnFocus:
    return getListener(DOM::EventImpl::FOCUS_EVENT);
  case OnKeyDown:
    return getListener(DOM::EventImpl::KEYDOWN_EVENT);
  case OnKeyPress:
    return getListener(DOM::EventImpl::KEYPRESS_EVENT);
  case OnKeyUp:
    return getListener(DOM::EventImpl::KEYUP_EVENT);
  case OnLoad:
    return getListener(DOM::EventImpl::LOAD_EVENT);
  case OnMouseDown:
    return getListener(DOM::EventImpl::MOUSEDOWN_EVENT);
  case OnMouseMove:
    return getListener(DOM::EventImpl::MOUSEMOVE_EVENT);
  case OnMouseOut:
    return getListener(DOM::EventImpl::MOUSEOUT_EVENT);
  case OnMouseOver:
    return getListener(DOM::EventImpl::MOUSEOVER_EVENT);
  case OnMouseUp:
    return getListener(DOM::EventImpl::MOUSEUP_EVENT);
  case OnMove:
    return getListener(DOM::EventImpl::KHTML_MOVE_EVENT);
  case OnReset:
    return getListener(DOM::EventImpl::RESET_EVENT);
  case OnResize:
    return getListener(DOM::EventImpl::RESIZE_EVENT);
  case OnScroll:
    return getListener(DOM::EventImpl::SCROLL_EVENT);
  case OnSelect:
    return getListener(DOM::EventImpl::SELECT_EVENT);
  case OnSubmit:
    return getListener(DOM::EventImpl::SUBMIT_EVENT);
  case OnUnload:
    return getListener(DOM::EventImpl::UNLOAD_EVENT);
  case SourceIndex: {
    // Retrieves the ordinal position of the object, in source order, as the object
    // appears in the document's all collection
    // i.e. document.all[n.sourceIndex] == n
    DOM::DocumentImpl* doc = node.document();
    if (doc->isHTMLDocument()) {
      HTMLCollectionImpl all(doc, HTMLCollectionImpl::DOC_ALL);
      unsigned long i = 0;
      for (DOM::NodeImpl* n = all.firstItem(); n; n = all.nextItem() ) {
        if (n == impl())
            return jsNumber(i);
        ++i;
      }
    }
    return jsUndefined();
  }
  default:
    // no DOM standard, found in IE only

    // Make sure our layout is up to date before we allow a query on these attributes.
    DOM::DocumentImpl* docimpl = node.document();
    if (docimpl) {
      docimpl->updateLayout();
    }

    khtml::RenderObject *rend = node.renderer();

    //In quirks mode, may need to forward if to body.
    rend = handleBodyRootQuirk(impl(), rend, token);

    switch (token) {
    case OffsetLeft:
      return rend ? jsNumber( rend->offsetLeft() ) : jsNumber(0);
    case OffsetTop:
      return rend ? jsNumber( rend->offsetTop() ) : jsNumber(0);
    case OffsetWidth:
      return rend ? jsNumber( rend->offsetWidth() ) : jsNumber(0);
    case OffsetHeight:
      return rend ? jsNumber( rend->offsetHeight() ) : jsNumber(0);
    case OffsetParent:
    {
      khtml::RenderObject* par = rend ? rend->offsetParent() : 0;
      return getDOMNode( exec, par ? par->element() : 0 );
    }
    case ClientWidth:
      return rend ? jsNumber( rend->clientWidth() ) : jsNumber(0);
    case ClientHeight:
      return rend ? jsNumber( rend->clientHeight() ) : jsNumber(0);
    case ClientLeft:
      return rend ? jsNumber( rend->clientLeft() ) : jsNumber(0);
    case ClientTop:
      return rend ? jsNumber( rend->clientTop() ) : jsNumber(0);
    case ScrollWidth:
      return rend ? jsNumber(rend->scrollWidth()) : jsNumber(0);
    case ScrollHeight:
      return rend ? jsNumber(rend->scrollHeight()) : jsNumber(0);
    case ScrollLeft:
      if (rend && rend->layer()) {
          if (rend->isRoot() && !rend->hasOverflowClip())
              return jsNumber( node.document()->view() ? node.document()->view()->contentsX() : 0);
          return jsNumber( rend->hasOverflowClip() ? rend->layer()->scrollXOffset() : 0 );
      }
      return jsNumber( 0 );
    case ScrollTop:
      if (rend && rend->layer()) {
          if (rend->isRoot() && !rend->hasOverflowClip())
              return jsNumber( node.document()->view() ? node.document()->view()->contentsY() : 0);
          return jsNumber( rend->hasOverflowClip() ? rend->layer()->scrollYOffset() : 0 );
      }
      return jsNumber( 0 );
    default:
      // qDebug() << "WARNING: Unhandled token in DOMNode::getValueProperty : " << token;
      break;
    }
  }
  return jsUndefined();
}


void DOMNode::put(ExecState *exec, const Identifier& propertyName, JSValue* value, int attr)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMNode::tryPut " << propertyName.qstring();
#endif
  lookupPut<DOMNode,DOMObject>(exec, propertyName, value, attr,
                                        &DOMNodeTable, this );
}

void DOMNode::putValueProperty(ExecState *exec, int token, JSValue* value, int /*attr*/)
{
  DOMExceptionTranslator exception(exec);
  NodeImpl& node = *impl();

  switch (token) {
  case NodeValue:
    node.setNodeValue(value->toString(exec).domString(), exception);
    break;
  case Prefix:
    node.setPrefix(value->toString(exec).domString(), exception);
    break;
  case TextContent:
    node.setTextContent(valueToStringWithNullCheck(exec, value), exception);
    break;
  case OnAbort:
    setListener(exec,DOM::EventImpl::ABORT_EVENT,value);
    break;
  case OnBlur:
    setListener(exec,DOM::EventImpl::BLUR_EVENT,value);
    break;
  case OnChange:
    setListener(exec,DOM::EventImpl::CHANGE_EVENT,value);
    break;
  case OnClick:
    setListener(exec,DOM::EventImpl::KHTML_ECMA_CLICK_EVENT,value);
    break;
  case OnDblClick:
    setListener(exec,DOM::EventImpl::KHTML_ECMA_DBLCLICK_EVENT,value);
    break;
  case OnDragDrop:
    setListener(exec,DOM::EventImpl::KHTML_DRAGDROP_EVENT,value);
    break;
  case OnError:
    setListener(exec,DOM::EventImpl::ERROR_EVENT,value);
    break;
  case OnFocus:
    setListener(exec,DOM::EventImpl::FOCUS_EVENT,value);
    break;
  case OnKeyDown:
    setListener(exec,DOM::EventImpl::KEYDOWN_EVENT,value);
    break;
  case OnKeyPress:
    setListener(exec,DOM::EventImpl::KEYPRESS_EVENT,value);
    break;
  case OnKeyUp:
    setListener(exec,DOM::EventImpl::KEYUP_EVENT,value);
    break;
  case OnLoad:
    setListener(exec,DOM::EventImpl::LOAD_EVENT,value);
    break;
  case OnMouseDown:
    setListener(exec,DOM::EventImpl::MOUSEDOWN_EVENT,value);
    break;
  case OnMouseMove:
    setListener(exec,DOM::EventImpl::MOUSEMOVE_EVENT,value);
    break;
  case OnMouseOut:
    setListener(exec,DOM::EventImpl::MOUSEOUT_EVENT,value);
    break;
  case OnMouseOver:
    setListener(exec,DOM::EventImpl::MOUSEOVER_EVENT,value);
    break;
  case OnMouseUp:
    setListener(exec,DOM::EventImpl::MOUSEUP_EVENT,value);
    break;
  case OnMove:
    setListener(exec,DOM::EventImpl::KHTML_MOVE_EVENT,value);
    break;
  case OnReset:
    setListener(exec,DOM::EventImpl::RESET_EVENT,value);
    break;
  case OnResize:
    setListener(exec,DOM::EventImpl::RESIZE_EVENT,value);
    break;
  case OnScroll:
    setListener(exec,DOM::EventImpl::SCROLL_EVENT,value);
    break;
  case OnSelect:
    setListener(exec,DOM::EventImpl::SELECT_EVENT,value);
    break;
  case OnSubmit:
    setListener(exec,DOM::EventImpl::SUBMIT_EVENT,value);
    break;
  case OnUnload:
    setListener(exec,DOM::EventImpl::UNLOAD_EVENT,value);
    break;
  default:
    // Make sure our layout is up to date
    DOM::DocumentImpl* docimpl = node.document();
    if (docimpl)
      docimpl->updateLayout();

    khtml::RenderObject *rend = node.renderer();

    //In quirks mode, may need to forward.
    rend = handleBodyRootQuirk(impl(), rend, token);

    switch (token) {
      case ScrollLeft:
        if (rend && rend->layer()) {
          if (rend->hasOverflowClip())
            rend->layer()->scrollToXOffset(value->toInt32(exec));
          else if (rend->isRoot()) {
            KHTMLView* sview = node.document()->view();
            if (sview)
              sview->setContentsPos(value->toInt32(exec), sview->contentsY());
          }
        }
        break;
      case ScrollTop:
        if (rend && rend->layer()) {
          if (rend->hasOverflowClip())
            rend->layer()->scrollToYOffset(value->toInt32(exec));
          else if (rend->isRoot()) {
            KHTMLView* sview = node.document()->view();
            if (sview)
              sview->setContentsPos(sview->contentsX(), value->toInt32(exec));
          }
        }
        break;
      default:
      // qDebug() << "WARNING: DOMNode::putValueProperty unhandled token " << token;
        break;
    }
  }
}

JSValue* DOMNode::toPrimitive(ExecState *exec, JSType /*preferred*/) const
{
  if (m_impl.isNull())
    return jsNull();

  return jsString(toString(exec));
}

UString DOMNode::toString(ExecState *) const
{
  if (m_impl.isNull())
    return "null";
  return "[object " + className() + "]";
}

void DOMNode::setListener(ExecState *exec, int eventId, JSValue* func) const
{
  m_impl->setHTMLEventListener(eventId,Window::retrieveActive(exec)->getJSEventListener(func,true));
}

JSValue* DOMNode::getListener(int eventId) const
{
  DOM::EventListener *listener = m_impl->getHTMLEventListener(eventId);
  JSEventListener *jsListener = static_cast<JSEventListener*>(listener);
  if ( jsListener && jsListener->listenerObj() )
    return jsListener->listenerObj();
  else
    return jsNull();
}

void DOMNode::pushEventHandlerScope(ExecState *, ScopeChain &) const
{
}

JSValue* DOMNodeProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( DOMNode, thisObj );
  DOMExceptionTranslator exception(exec);
  DOM::NodeImpl& node = *static_cast<DOMNode *>( thisObj )->impl();
  switch (id) {
    case DOMNode::HasAttributes:
      return jsBoolean(node.hasAttributes());
    case DOMNode::HasChildNodes:
      return jsBoolean(node.hasChildNodes());
    case DOMNode::CloneNode: {
      RefPtr<NodeImpl> clone = node.cloneNode(args[0]->toBoolean(exec));
      return getDOMNode(exec, clone.get());
    }
    case DOMNode::Normalize:
      node.normalize();
      return jsUndefined();
    case DOMNode::IsSupported:
      return jsBoolean(node.isSupported(args[0]->toString(exec).domString(),args[1]->toString(exec).domString()));
    case DOMNode::AddEventListener: {
        JSEventListener *listener = Window::retrieveActive(exec)->getJSEventListener(args[1]);
        EventName id = EventName::fromString(args[0]->toString(exec).domString());
        node.addEventListener(id,listener,args[2]->toBoolean(exec));
        return jsUndefined();
    }
    case DOMNode::RemoveEventListener: {
        JSEventListener *listener = Window::retrieveActive(exec)->getJSEventListener(args[1]);
        EventName id = EventName::fromString(args[0]->toString(exec).domString());
        node.removeEventListener(id,listener,args[2]->toBoolean(exec));
        return jsUndefined();
    }
    case DOMNode::DispatchEvent: {
      SharedPtr<DOM::EventImpl> evt = toEvent(args[0]);
      if (!evt) {
        setDOMException(exec, DOMException::NOT_FOUND_ERR);
        return jsUndefined();
      }
      node.dispatchEvent(evt.get(), exception);
      return jsBoolean(!evt->defaultPrevented());
    }
    case DOMNode::AppendChild:
      return getDOMNode(exec,node.appendChild(toNode(args[0]), exception));
    case DOMNode::RemoveChild: {
      SharedPtr<DOM::NodeImpl> oldKid = toNode(args[0]);
      node.removeChild(oldKid.get(), exception);
      return getDOMNode(exec, oldKid.get());
    }
    case DOMNode::InsertBefore:
      return getDOMNode(exec,node.insertBefore(toNode(args[0]), toNode(args[1]), exception));
    case DOMNode::ReplaceChild: {
      SharedPtr<DOM::NodeImpl> oldKid = toNode(args[1]);
      node.replaceChild(toNode(args[0]), oldKid.get(), exception);
      return getDOMNode(exec, oldKid.get());
    }
    case DOMNode::Contains:
    {
      DOM::NodeImpl* other = toNode(args[0]);
      if (other && other->isElementNode())
          return jsBoolean(other->isAncestor(&node));
      setDOMException(exec, DOMException::TYPE_MISMATCH_ERR); // ### a guess
      return jsUndefined();
    }
    case DOMNode::InsertAdjacentHTML:
    {
      // see http://www.faqts.com/knowledge_base/view.phtml/aid/5756
      // and http://msdn.microsoft.com/workshop/author/dhtml/reference/methods/insertAdjacentHTML.asp
      SharedPtr<DOM::RangeImpl> range = node.document()->createRange();

      range->setStartBefore(&node, exception);
      if (exception.triggered()) return jsUndefined();

      SharedPtr<DOM::DocumentFragmentImpl> docFrag =
      static_cast<DOM::DocumentFragmentImpl*>(range->createContextualFragment(args[1]->toString(exec).domString(), exception).handle());
      if (exception.triggered()) return jsUndefined();

      DOMString where = args[0]->toString(exec).domString().lower();

      if (where == "beforebegin")
        node.parentNode()->insertBefore(docFrag.get(), &node, exception);
      else if (where == "afterbegin")
        node.insertBefore(docFrag.get(), node.firstChild(), exception);
      else if (where == "beforeend")
        return getDOMNode(exec, node.appendChild(docFrag.get(), exception));
      else if (where == "afterend") {
        if (node.nextSibling()) {
	  node.parentNode()->insertBefore(docFrag.get(), node.nextSibling(),exception);
        } else {
	  node.parentNode()->appendChild(docFrag.get(),exception);
        }
      }

      return jsUndefined();
    }
    case DOMNode::CompareDocumentPosition: {
       DOM::NodeImpl* other = toNode(args[0]);
       if (!other)
          setDOMException(exec, DOMException::TYPE_MISMATCH_ERR);
       else
          return jsNumber(node.compareDocumentPosition(other));
    }
  }

  return jsUndefined();
}

// -------------------------------------------------------------------------

/*
@begin DOMNodeListProtoTable 2
  item		DOMNodeList::Item		DontDelete|Function 1
# IE extension (IE treats DOMNodeList like an HTMLCollection)
  namedItem	DOMNodeList::NamedItem		DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(DOMNodeListProto)
KJS_IMPLEMENT_PROTOFUNC(DOMNodeListProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMNodeList",DOMNodeListProto,DOMNodeListProtoFunc, ObjectPrototype)

IMPLEMENT_PSEUDO_CONSTRUCTOR(NodeListPseudoCtor, "NodeList", DOMNodeListProto)

const ClassInfo DOMNodeList::info = { "NodeList", 0, 0, 0 };

DOMNodeList::DOMNodeList(ExecState *exec, DOM::NodeListImpl* l)
 : DOMObject(DOMNodeListProto::self(exec)), m_impl(l) { }

DOMNodeList::~DOMNodeList()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue* DOMNodeList::indexGetter(ExecState *exec, unsigned index)
{
  return getDOMNode(exec, m_impl->item(index));
}

JSValue *DOMNodeList::nameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot)
{
  DOMNodeList *thisObj = static_cast<DOMNodeList *>(slot.slotBase());
  return getDOMNode(exec, thisObj->getByName(name));
}

JSValue *DOMNodeList::lengthGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot& slot)
{
  DOMNodeList *thisObj = static_cast<DOMNodeList *>(slot.slotBase());
  return jsNumber(thisObj->m_impl->length());
}

DOM::NodeImpl* DOMNodeList::getByName(const Identifier& name)
{
  //### M.O.: I bet IE checks name only for some tags.
  DOMString domName = name.domString();
  unsigned long l   = m_impl->length();
  for ( unsigned long i = 0; i < l; i++ ) {
    DOM::NodeImpl* n = m_impl->item( i );
    if (n->isElementNode()) {
      DOM::ElementImpl* e = static_cast<DOM::ElementImpl*>(n);
      if (e->getAttribute(ATTR_ID) == domName || e->getAttribute(ATTR_NAME) == domName)
        return n;
    }
  }
  return 0;
}

bool DOMNodeList::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  if (propertyName == exec->propertyNames().length) {
    slot.setCustom(this, lengthGetter);
    return true;
  }

  //### this could benefit from further nodelist/collection consolidation,
  //including moving nameditem down to nodelists.

  // Look in the prototype (for functions) before assuming it's an item's name
  JSObject *proto = prototype()->getObject();
  if (proto && proto->hasProperty(exec, propertyName))
    return false;

  //May be it's an index?
  if (getIndexSlot(this, *m_impl, propertyName, slot))
    return true;

  //May be it's a name -- check by ID
  //Check by id
  if (getByName(propertyName)) {
    slot.setCustom(this, nameGetter);
    return true;
  }

  return DOMObject::getOwnPropertySlot(exec, propertyName, slot);
}

void DOMNodeList::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, PropertyMap::PropertyMode mode)
{
  for (unsigned i = 0; i < m_impl->length(); ++i)
      propertyNames.add(Identifier::from(i));

  propertyNames.add(exec->propertyNames().length);

  JSObject::getOwnPropertyNames(exec, propertyNames, mode);
}

// Need to support both get and call, so that list[0] and list(0) work.
JSValue* DOMNodeList::callAsFunction(ExecState *exec, JSObject *, const List &args)
{
  // Do not use thisObj here. See HTMLCollection.
  UString s = args[0]->toString(exec);

  // index-based lookup?
  bool ok;
  unsigned int u = s.qstring().toULong(&ok);
  if (ok)
    return getDOMNode(exec,m_impl->item(u));

  // try lookup by name
  // ### NodeList::namedItem() would be cool to have
  // ### do we need to support the same two arg overload as in HTMLCollection?
  JSValue* result = get(exec, Identifier(s));

  if (result)
    return result;

  return jsUndefined();
}

JSValue* DOMNodeListProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMNodeList, thisObj );
  DOMNodeList* jsList     = static_cast<DOMNodeList *>(thisObj);
  DOM::NodeListImpl& list = *jsList->impl();
  switch (id) {
  case KJS::DOMNodeList::Item:
    return getDOMNode(exec, list.item(args[0]->toInt32(exec)));
  case KJS::DOMNodeList::NamedItem:
    return getDOMNode(exec, jsList->getByName(Identifier(args[0]->toString(exec))));
  default:
    return jsUndefined();
  }
}

// -------------------------------------------------------------------------

//### FIXME: link to the node prototype.
const ClassInfo DOMAttr::info = { "Attr", &DOMNode::info, &DOMAttrTable, 0 };

/* Source for DOMAttrTable.
@begin DOMAttrTable 5
  name		DOMAttr::Name		DontDelete|ReadOnly
  specified	DOMAttr::Specified	DontDelete|ReadOnly
  value		DOMAttr::ValueProperty	DontDelete
  ownerElement	DOMAttr::OwnerElement	DontDelete|ReadOnly
@end
*/
bool DOMAttr::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMAttr::tryGet " << propertyName.qstring();
#endif
  return getStaticValueSlot<DOMAttr, DOMNode>(exec, &DOMAttrTable, this, propertyName, slot);
}

JSValue* DOMAttr::getValueProperty(ExecState *exec, int token) const
{
  AttrImpl *attr = static_cast<AttrImpl *>(impl());
  switch (token) {
  case Name:
    return jsString(attr->name());
  case Specified:
    return jsBoolean(attr->specified());
  case ValueProperty:
    return jsString(attr->nodeValue());
  case OwnerElement: // DOM2
    return getDOMNode(exec,attr->ownerElement());
  }
  return jsNull(); // not reached
}

void DOMAttr::put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMAttr::tryPut " << propertyName.qstring();
#endif
  lookupPut<DOMAttr,DOMNode>(exec, propertyName, value, attr,
                                      &DOMAttrTable, this );
}

void DOMAttr::putValueProperty(ExecState *exec, int token, JSValue* value, int /*attr*/)
{
  DOMExceptionTranslator exception(exec);
  switch (token) {
  case ValueProperty:
    static_cast<AttrImpl *>(impl())->setValue(value->toString(exec).domString(), exception);
    return;
  default:
    qWarning() << "DOMAttr::putValueProperty unhandled token " << token;
  }
}

AttrImpl *toAttr(JSValue *val)
{
    if (!val || !val->isObject(&DOMAttr::info))
        return 0;
    return static_cast<AttrImpl *>(static_cast<DOMNode *>(val)->impl());
}

// -------------------------------------------------------------------------

/* Source for DOMDocumentProtoTable.
@begin DOMDocumentProtoTable 23
  createElement   DOMDocument::CreateElement                   DontDelete|Function 1
  createDocumentFragment DOMDocument::CreateDocumentFragment   DontDelete|Function 1
  createTextNode  DOMDocument::CreateTextNode                  DontDelete|Function 1
  createComment   DOMDocument::CreateComment                   DontDelete|Function 1
  createCDATASection DOMDocument::CreateCDATASection           DontDelete|Function 1
  createProcessingInstruction DOMDocument::CreateProcessingInstruction DontDelete|Function 1
  createAttribute DOMDocument::CreateAttribute                 DontDelete|Function 1
  createEntityReference DOMDocument::CreateEntityReference     DontDelete|Function 1
  getElementsByTagName  DOMDocument::GetElementsByTagName      DontDelete|Function 1
  importNode           DOMDocument::ImportNode                 DontDelete|Function 2
  createElementNS      DOMDocument::CreateElementNS            DontDelete|Function 2
  createAttributeNS    DOMDocument::CreateAttributeNS          DontDelete|Function 2
  getElementsByTagNameNS  DOMDocument::GetElementsByTagNameNS  DontDelete|Function 2
  getElementById     DOMDocument::GetElementById               DontDelete|Function 1
  createRange        DOMDocument::CreateRange                  DontDelete|Function 0
  createNodeIterator DOMDocument::CreateNodeIterator           DontDelete|Function 3
  createTreeWalker   DOMDocument::CreateTreeWalker             DontDelete|Function 4
  createEvent        DOMDocument::CreateEvent                  DontDelete|Function 1
  getOverrideStyle   DOMDocument::GetOverrideStyle             DontDelete|Function 2
  abort              DOMDocument::Abort                        DontDelete|Function 0
  load               DOMDocument::Load                         DontDelete|Function 1
  loadXML            DOMDocument::LoadXML                      DontDelete|Function 2
  execCommand        DOMDocument::ExecCommand                  DontDelete|Function 3
  queryCommandEnabled DOMDocument::QueryCommandEnabled         DontDelete|Function 1
  queryCommandIndeterm DOMDocument::QueryCommandIndeterm       DontDelete|Function 1
  queryCommandState DOMDocument::QueryCommandState             DontDelete|Function 1
  queryCommandSupported DOMDocument::QueryCommandSupported     DontDelete|Function 1
  queryCommandValue DOMDocument::QueryCommandValue             DontDelete|Function 1
  getElementsByClassName DOMDocument::GetElementsByClassName   DontDelete|Function 1
  querySelector          DOMDocument::QuerySelector            DontDelete|Function 1
  querySelectorAll       DOMDocument::QuerySelectorAll         DontDelete|Function 1
  createExpression       DOMDocument::CreateExpression         DontDelete|Function 2
  createNSResolver       DOMDocument::CreateNSResolver         DontDelete|Function 1
  evaluate               DOMDocument::Evaluate                 DontDelete|Function 4
@end
*/

KJS_IMPLEMENT_PROTOFUNC(DOMDocumentProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMDocument",DOMDocumentProto, DOMDocumentProtoFunc, DOMNodeProto)

IMPLEMENT_PSEUDO_CONSTRUCTOR(DocumentPseudoCtor, "Document", DOMDocumentProto)

const ClassInfo DOMDocument::info = { "Document", &DOMNode::info, &DOMDocumentTable, 0 };

/* Source for DOMDocumentTable.
@begin DOMDocumentTable 4
  doctype         DOMDocument::DocType                         DontDelete|ReadOnly
  implementation  DOMDocument::Implementation                  DontDelete|ReadOnly
  characterSet    DOMDocument::CharacterSet                    DontDelete|ReadOnly
  documentElement DOMDocument::DocumentElement                 DontDelete|ReadOnly
  styleSheets     DOMDocument::StyleSheets                     DontDelete|ReadOnly
  preferredStylesheetSet  DOMDocument::PreferredStylesheetSet  DontDelete|ReadOnly
  selectedStylesheetSet  DOMDocument::SelectedStylesheetSet    DontDelete
  readyState      DOMDocument::ReadyState                      DontDelete|ReadOnly
  defaultView     DOMDocument::DefaultView                     DontDelete|ReadOnly
  async           DOMDocument::Async                           DontDelete
  title           DOMDocument::Title                           DontDelete
@end
*/

DOMDocument::DOMDocument(ExecState *exec, DOM::DocumentImpl* d)
  : DOMNode(exec, d)
{
  setPrototype(DOMDocumentProto::self(exec));
}

DOMDocument::DOMDocument(JSObject *proto, DOM::DocumentImpl* d)
  : DOMNode(proto, d)
{}

bool DOMDocument::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMDocument::tryGet " << propertyName.qstring();
#endif
  return getStaticValueSlot<DOMDocument, DOMNode>(
    exec, &DOMDocumentTable, this, propertyName, slot);
}

JSValue* DOMDocument::getValueProperty(ExecState *exec, int token) const
{
  DOM::DocumentImpl& doc = *static_cast<DOM::DocumentImpl*>(m_impl.get());

  switch(token) {
  case DocType:
    return getDOMNode(exec,doc.doctype());
  case Implementation:
    return getDOMDOMImplementation(exec,doc.implementation());
  case DocumentElement:
    return getDOMNode(exec,doc.documentElement());
  case CharacterSet: {
    if (doc.part())
      return jsString(doc.part()->encoding());
    else
      return jsUndefined();
  }
  case StyleSheets:
    //qDebug() << "DOMDocument::StyleSheets, returning " << doc.styleSheets().length() << " stylesheets";
    return getDOMStyleSheetList(exec, doc.styleSheets(), &doc);
  case DOMDocument::DefaultView: // DOM2
    {
    KHTMLPart *part = doc.part();
    if (part)
        return Window::retrieve(part);
    return getDOMAbstractView(exec, doc.defaultView());
    }
  case PreferredStylesheetSet:
    return jsString(doc.preferredStylesheetSet());
  case SelectedStylesheetSet:
    return jsString(doc.selectedStylesheetSet());
  case ReadyState:
    {
    if ( KHTMLPart* part = doc.part() )
    {
	if (part->d->m_bComplete) return jsString("complete");
        if (doc.parsing()) return jsString("loading");
        return jsString("loaded");
        // What does the interactive value mean ?
        // Missing support for "uninitialized"
    }
    return jsUndefined();
    }
  case Async:
    return jsBoolean(doc.async());
  case Title:
    return jsString(doc.title());
  default:
    // qDebug() << "WARNING: DOMDocument::getValueProperty unhandled token " << token;
    return jsNull();
  }
}

void DOMDocument::put(ExecState *exec, const Identifier& propertyName, JSValue* value, int attr)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMDocument::tryPut " << propertyName.qstring();
#endif
  lookupPut<DOMDocument,DOMNode>(exec, propertyName, value, attr, &DOMDocumentTable, this );
}

void DOMDocument::putValueProperty(ExecState *exec, int token, JSValue* value, int /*attr*/)
{
  DOM::DocumentImpl& doc = *static_cast<DOM::DocumentImpl*>(impl());
  switch (token) {
    case SelectedStylesheetSet: {
      doc.setSelectedStylesheetSet(value->toString(exec).domString());
      break;
    }
    case Async: {
      doc.setAsync(value->toBoolean(exec));
      break;
    }
    case Title: {
      DOM::DOMString val = value->toString(exec).domString();
      if (doc.title() != val) doc.setTitle(val);
      break;
    }
  }
}

JSValue* DOMDocumentProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMDocument, thisObj );
  DOMExceptionTranslator exception(exec);
  DOM::NodeImpl&     node = *static_cast<DOMNode *>( thisObj )->impl();
  DOM::DocumentImpl& doc  = static_cast<DOM::DocumentImpl&>(node);

  KJS::UString str = args[0]->toString(exec);

  // we can do it fast, without copying the data
  if (id == DOMDocument::GetElementById) {
#ifdef KJS_VERBOSE
      qDebug() << "DOMDocument::GetElementById looking for " << args[0]->toString(exec).qstring();
#endif
      // create DOMStringImpl without copying
      DOMStringImpl shallowCopy(DOMStringImpl::ShallowCopy, (QChar*)str.data(), str.size());
      return getDOMNode(exec, doc.getElementById(DOMString(&shallowCopy)));
  }

  DOM::DOMString s = str.domString();

  switch(id) {
  case DOMDocument::CreateElement:
    return getDOMNode(exec,doc.createElement(s, exception));
  case DOMDocument::CreateDocumentFragment:
    return getDOMNode(exec,doc.createDocumentFragment());
  case DOMDocument::CreateTextNode:
    return getDOMNode(exec,doc.createTextNode(s.implementation()));
  case DOMDocument::CreateComment:
    return getDOMNode(exec,doc.createComment(s.implementation()));
  case DOMDocument::CreateCDATASection:
    return getDOMNode(exec,doc.createCDATASection(s.implementation()));  /* TODO: okay ? */
  case DOMDocument::CreateProcessingInstruction:
    return getDOMNode(exec,doc.createProcessingInstruction(args[0]->toString(exec).domString(),
                              args[1]->toString(exec).domString().implementation()));
  case DOMDocument::CreateAttribute:
    return getDOMNode(exec,doc.createAttribute(s, exception));
  case DOMDocument::CreateEntityReference:
    return getDOMNode(exec,doc.createEntityReference(args[0]->toString(exec).domString()));
  case DOMDocument::GetElementsByTagName:
    return getDOMNodeList(exec,doc.getElementsByTagName(s));
  case DOMDocument::ImportNode: // DOM2
    return getDOMNode(exec,doc.importNode(toNode(args[0]), args[1]->toBoolean(exec), exception));
  case DOMDocument::CreateElementNS: // DOM2
    return getDOMNode(exec,doc.createElementNS(valueToStringWithNullCheck(exec, args[0]), args[1]->toString(exec).domString(), exception));
  case DOMDocument::CreateAttributeNS: // DOM2
    return getDOMNode(exec,doc.createAttributeNS(valueToStringWithNullCheck(exec, args[0]), args[1]->toString(exec).domString(), exception));
  case DOMDocument::GetElementsByTagNameNS: // DOM2
    return getDOMNodeList(exec,doc.getElementsByTagNameNS(args[0]->toString(exec).domString(),
                                                          args[1]->toString(exec).domString()));
  case DOMDocument::CreateRange:
    return getDOMRange(exec,doc.createRange());
  case DOMDocument::CreateNodeIterator:
        return getDOMNodeIterator(exec,
                                  doc.createNodeIterator(toNode(args[0]),
                                                         (long unsigned int)(args[1]->toNumber(exec)),
                                                         toNodeFilter(args[2]),args[3]->toBoolean(exec), exception));
  case DOMDocument::CreateTreeWalker:
    return getDOMTreeWalker(exec,doc.createTreeWalker(toNode(args[0]),(long unsigned int)(args[1]->toNumber(exec)),
             toNodeFilter(args[2]),args[3]->toBoolean(exec), exception));
  case DOMDocument::CreateEvent:
    return getDOMEvent(exec,doc.createEvent(s, exception));
  case DOMDocument::GetOverrideStyle: {
    DOM::NodeImpl* arg0 = toNode(args[0]);
    if (!arg0 || !arg0->isElementNode())
      return jsUndefined(); // throw exception?
    else
      return getDOMCSSStyleDeclaration(exec,doc.getOverrideStyle(static_cast<DOM::ElementImpl*>(arg0),args[1]->toString(exec).domString().implementation()));
  }
  case DOMDocument::Abort:
    doc.abort();
    break;
  case DOMDocument::Load: {
    Window* active = Window::retrieveActive(exec);
    // Complete the URL using the "active part" (running interpreter). We do this for the security
    // check and to make sure we load exactly the same url as we have verified to be safe
    KHTMLPart *khtmlpart = qobject_cast<KHTMLPart*>(active->part());
    if (khtmlpart) {
      // Security: only allow documents to be loaded from the same host
      QString dstUrl = khtmlpart->htmlDocument().completeURL(s).string();
      KParts::ReadOnlyPart *part = static_cast<KJS::ScriptInterpreter*>(exec->dynamicInterpreter())->part();
      if (part->url().host() == QUrl(dstUrl).host()) {
	// qDebug() << "JavaScript: access granted for document.load() of " << dstUrl;
	doc.load(dstUrl);
      }
      else {
	// qDebug() << "JavaScript: access denied for document.load() of " << dstUrl;
      }
    }
    break;
  }
  case DOMDocument::LoadXML:
    doc.loadXML(s);
    break;
  case DOMDocument::ExecCommand: {
    return jsBoolean(doc.execCommand(args[0]->toString(exec).domString(), args[1]->toBoolean(exec), args[2]->toString(exec).domString()));
  }
  case DOMDocument::QueryCommandEnabled: {
    return jsBoolean(doc.queryCommandEnabled(args[0]->toString(exec).domString()));
  }
  case DOMDocument::QueryCommandIndeterm: {
    return jsBoolean(doc.queryCommandIndeterm(args[0]->toString(exec).domString()));
  }
  case DOMDocument::QueryCommandState: {
    return jsBoolean(doc.queryCommandState(args[0]->toString(exec).domString()));
  }
  case DOMDocument::QueryCommandSupported: {
    return jsBoolean(doc.queryCommandSupported(args[0]->toString(exec).domString()));
  }
  case DOMDocument::QueryCommandValue: {
    DOM::DOMString commandValue(doc.queryCommandValue(args[0]->toString(exec).domString()));
    // Method returns null DOMString to signal command is unsupported.
    // Microsoft documentation for this method says:
    // "If not supported [for a command identifier], this method returns a Boolean set to false."
    if (commandValue.isNull())
      return jsBoolean(false);
    else
      return jsString(commandValue);
  }
  case DOMDocument::GetElementsByClassName:
    return getDOMNodeList(exec, doc.getElementsByClassName(s));
  case DOMDocument::QuerySelector: {
    RefPtr<DOM::ElementImpl> e = doc.querySelector(s, exception);
    return getDOMNode(exec, e.get());
  }
  case DOMDocument::QuerySelectorAll: {
    RefPtr<DOM::NodeListImpl> l = doc.querySelectorAll(s, exception);
    return getDOMNodeList(exec, l.get());
  }
  case DOMDocument::CreateExpression: {
    RefPtr<khtml::XPathNSResolverImpl> res = toResolver(exec, args[1]);
    RefPtr<khtml::XPathExpressionImpl> e = doc.createExpression(s, res.get(), exception);
    JSValue* wrapper = getWrapper<KJS::XPathExpression>(exec, e.get());

    // protect the resolver if needed
    if (!wrapper->isNull() && res && res->type() == khtml::XPathNSResolverImpl::JS) {
      static_cast<XPathExpression*>(wrapper)->setAssociatedResolver(static_cast<JSXPathNSResolver*>(res.get())->resolverObject());
    }
    return wrapper;
  }
  case DOMDocument::CreateNSResolver: {
    DOM::NodeImpl* node = toNode(args[0]);
    return getWrapper<KJS::XPathNSResolver>(exec, doc.createNSResolver(node));
  }
  case DOMDocument::Evaluate: {
    return getWrapper<KJS::XPathResult>(exec,
          doc.evaluate(s, // expression
                      toNode(args[1]), // contextNode,
                      toResolver(exec, args[2]), // resolver
                      args[3]->toInt32(exec), // type
                      0, // result reuse, ignored
                      exception));
  }
  default:
    break;
  }

  return jsUndefined();
}

// -------------------------------------------------------------------------

/* Source for DOMElementProtoTable.
@begin DOMElementProtoTable 17
  getAttribute		DOMElement::GetAttribute	DontDelete|Function 1
  setAttribute		DOMElement::SetAttribute	DontDelete|Function 2
  removeAttribute	DOMElement::RemoveAttribute	DontDelete|Function 1
  getAttributeNode	DOMElement::GetAttributeNode	DontDelete|Function 1
  setAttributeNode	DOMElement::SetAttributeNode	DontDelete|Function 2
  removeAttributeNode	DOMElement::RemoveAttributeNode	DontDelete|Function 1
  getElementsByTagName	DOMElement::GetElementsByTagName	DontDelete|Function 1
  hasAttribute		DOMElement::HasAttribute	DontDelete|Function 1
  getAttributeNS	DOMElement::GetAttributeNS	DontDelete|Function 2
  setAttributeNS	DOMElement::SetAttributeNS	DontDelete|Function 3
  removeAttributeNS	DOMElement::RemoveAttributeNS	DontDelete|Function 2
  getAttributeNodeNS	DOMElement::GetAttributeNodeNS	DontDelete|Function 2
  setAttributeNodeNS	DOMElement::SetAttributeNodeNS	DontDelete|Function 1
  getElementsByTagNameNS DOMElement::GetElementsByTagNameNS	DontDelete|Function 2
  hasAttributeNS	DOMElement::HasAttributeNS	DontDelete|Function 2
  getElementsByClassName DOMElement::GetElementsByClassName   DontDelete|Function 1
# Extensions
  blur          DOMElement::Blur    DontDelete|Function 0
  focus         DOMElement::Focus   DontDelete|Function 0
  querySelector          DOMElement::QuerySelector            DontDelete|Function 1
  querySelectorAll       DOMElement::QuerySelectorAll         DontDelete|Function 1
  getClientRects         DOMElement::GetClientRects           DontDelete|Function 0
  getBoundingClientRect  DOMElement::GetBoundingClientRect    DontDelete|Function 0
@end
*/
KJS_IMPLEMENT_PROTOFUNC(DOMElementProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMElement",DOMElementProto,DOMElementProtoFunc,DOMNodeProto)

IMPLEMENT_PSEUDO_CONSTRUCTOR(ElementPseudoCtor, "Element", DOMElementProto)

const ClassInfo DOMElement::info = { "Element", &DOMNode::info, &DOMElementTable, 0 };
/* Source for DOMElementTable.
@begin DOMElementTable 7
  tagName		DOMElement::TagName                 DontDelete|ReadOnly
  style			DOMElement::Style                   DontDelete|ReadOnly
# DOM 3 - ElementTraversal interface
  firstElementChild	DOMElement::FirstElementChild       DontDelete|ReadOnly
  lastElementChild	DOMElement::LastElementChild        DontDelete|ReadOnly
  previousElementSibling DOMElement::PreviousElementSibling DontDelete|ReadOnly
  nextElementSibling	DOMElement::NextElementSibling      DontDelete|ReadOnly
  childElementCount	DOMElement::ChildElementCount       DontDelete|ReadOnly
@end
*/
DOMElement::DOMElement(ExecState *exec, DOM::ElementImpl* e)
  : DOMNode(exec, e)
{
    setPrototype(DOMElementProto::self(exec));
}

DOMElement::DOMElement(JSObject *proto, DOM::ElementImpl* e)
  : DOMNode(proto, e)
{}

JSValue* DOMElement::getValueProperty(ExecState *exec, int token) const
{
  DOM::ElementImpl& element = static_cast<DOM::ElementImpl&>(*m_impl);
  switch( token ) {
    case TagName:
      return jsString(element.tagName());
    case Style:
      return getDOMCSSStyleDeclaration(exec,element.getInlineStyleDecls());
    case FirstElementChild:
      return getDOMNode(exec,element.firstElementChild());
    case LastElementChild:
      return getDOMNode(exec,element.lastElementChild());
    case PreviousElementSibling:
      return getDOMNode(exec,element.previousElementSibling());
    case NextElementSibling:
      return getDOMNode(exec,element.nextElementSibling());
    case ChildElementCount:
      return jsNumber((unsigned int)element.childElementCount());
    default:
      // qDebug() << "WARNING: Unhandled token in DOMElement::getValueProperty : " << token;
      return jsUndefined();
  }
}

bool DOMElement::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMElement::getOwnPropertySlot " << propertyName.qstring();
#endif

  //DOM::Element element = static_cast<DOM::Element>(node);
  if (getStaticOwnValueSlot(&DOMElementTable, this, propertyName, slot))
    return true;

  return DOMNode::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue* DOMElementProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMNode, thisObj ); // node should be enough here, given the cast
  DOMExceptionTranslator exception(exec);
  DOM::NodeImpl&    node    = *static_cast<DOMNode *>( thisObj )->impl();
  DOM::ElementImpl& element = static_cast<DOM::ElementImpl&>(node);

  switch(id) {
    case DOMElement::GetAttribute:
      /** In theory, we should not return null here, as per DOM. In practice, that
       breaks websites */
      return getStringOrNull(element.getAttribute(args[0]->toString(exec).domString()));
    case DOMElement::SetAttribute:
      element.setAttribute(args[0]->toString(exec).domString(),args[1]->toString(exec).domString(), exception);
      return jsUndefined();
    case DOMElement::RemoveAttribute:
      element.removeAttribute(args[0]->toString(exec).domString(), exception);
      return jsUndefined();
    case DOMElement::GetAttributeNode:
      return getDOMNode(exec,element.getAttributeNode(args[0]->toString(exec).domString()));
    case DOMElement::SetAttributeNode: {
      DOM::Attr ret = element.setAttributeNode(KJS::toAttr(args[0]), exception);
      return getDOMNode(exec, ret.handle());
    }
    case DOMElement::RemoveAttributeNode: {
      DOM::Attr ret = element.removeAttributeNode(KJS::toAttr(args[0]), exception);
      return getDOMNode(exec, ret.handle());
    }
    case DOMElement::GetElementsByTagName:
      return getDOMNodeList(exec,element.getElementsByTagName(args[0]->toString(exec).domString()));
    case DOMElement::HasAttribute: // DOM2
      return jsBoolean(element.hasAttribute(args[0]->toString(exec).domString()));
    case DOMElement::GetAttributeNS: // DOM2
      return jsString(element.getAttributeNS(args[0]->toString(exec).domString(),args[1]->toString(exec).domString(), exception));
    case DOMElement::SetAttributeNS: // DOM2
      element.setAttributeNS(args[0]->toString(exec).domString(),args[1]->toString(exec).domString(),args[2]->toString(exec).domString(),exception);
      return jsUndefined();
    case DOMElement::RemoveAttributeNS: // DOM2
      element.removeAttributeNS(args[0]->toString(exec).domString(),args[1]->toString(exec).domString(), exception);
      return jsUndefined();
    case DOMElement::GetAttributeNodeNS: // DOM2
      return getDOMNode(exec,element.getAttributeNodeNS(args[0]->toString(exec).domString(),args[1]->toString(exec).domString(),exception));
    case DOMElement::SetAttributeNodeNS: {
      DOM::Attr toRet = element.setAttributeNodeNS(KJS::toAttr(args[0]), exception);
      return getDOMNode(exec, toRet.handle());
    }
    case DOMElement::GetElementsByTagNameNS: // DOM2
      return getDOMNodeList(exec,element.getElementsByTagNameNS(args[0]->toString(exec).domString(),args[1]->toString(exec).domString()));
    case DOMElement::HasAttributeNS: // DOM2
      return jsBoolean(element.hasAttributeNS(args[0]->toString(exec).domString(),args[1]->toString(exec).domString()));
    case DOMElement::GetElementsByClassName: // HTML 5
      return getDOMNodeList(exec, element.getElementsByClassName(args[0]->toString(exec).domString()));
    case DOMElement::QuerySelector: { // WA Selectors 1
      RefPtr<DOM::ElementImpl> e = element.querySelector(args[0]->toString(exec).domString(), exception);
      return getDOMNode(exec, e.get());
    }
    case DOMElement::QuerySelectorAll: {  // WA Selectors 1
      RefPtr<DOM::NodeListImpl> l = element.querySelectorAll(args[0]->toString(exec).domString(), exception);
      return getDOMNodeList(exec, l.get());
    }
    case DOMElement::GetClientRects: {
      DOM::DocumentImpl* docimpl = node.document();
      if (docimpl) {
        docimpl->updateLayout();
      }

      khtml::RenderObject *rend = node.renderer();

      if (!rend)
        return new ClientRectList(exec);

      //In quirks mode, may need to forward it to body.
      rend = handleBodyRootQuirk(static_cast<DOMNode *>( thisObj )->impl(), rend, DOMNode::ClientTop);

      return new ClientRectList(exec, rend->getClientRects());
    }
    case DOMElement::GetBoundingClientRect: {
      DOM::DocumentImpl* docimpl = node.document();
      if (docimpl) {
        docimpl->updateLayout();
      }

      khtml::RenderObject *rend = node.renderer();

      //In quirks mode, may need to forward it to body.
      rend = handleBodyRootQuirk(static_cast<DOMNode *>( thisObj )->impl(), rend, DOMNode::ClientTop);

      if (!rend)
        return new ClientRect(exec, 0, 0, 0, 0);

      QList<QRectF> list = rend->getClientRects();
      if (list.isEmpty()) {
        return new ClientRect(exec, 0, 0, 0, 0);
      } else {
        QRectF rect = list.first();
        for (int i = 1; i < list.length(); ++i) {
          rect = rect.united(list.at(i));
        }
        return new ClientRect(exec, rect);
      }
    }

    default:

      // Make sure our layout is up to date before we call these
      DOM::DocumentImpl* docimpl = element.document();
      if (docimpl) {
          docimpl->updateLayout();
      }
      switch(id) {
        case DOMElement::Focus:
          element.focus();
          return jsUndefined();
        case DOMElement::Blur:
          element.blur();
          return jsUndefined();
        default:
          return jsUndefined();
      }
  }
}

DOM::ElementImpl *KJS::toElement(JSValue *v)
{
  DOM::NodeImpl* node = toNode(v);
  if (node && node->isElementNode())
    return static_cast<DOM::ElementImpl*>(node);
  return 0;
}

DOM::AttrImpl *KJS::toAttr(JSValue *v)
{
  DOM::NodeImpl* node = toNode(v);
  if (node && node->isAttributeNode())
    return static_cast<DOM::AttrImpl*>(node);
  return 0;
}


// -------------------------------------------------------------------------

/* Source for DOMDOMImplementationProtoTable.
@begin DOMDOMImplementationProtoTable 5
  hasFeature		DOMDOMImplementation::HasFeature		DontDelete|Function 2
  createCSSStyleSheet	DOMDOMImplementation::CreateCSSStyleSheet	DontDelete|Function 2
# DOM2
  createDocumentType	DOMDOMImplementation::CreateDocumentType	DontDelete|Function 3
  createDocument	DOMDOMImplementation::CreateDocument		DontDelete|Function 3
  createHTMLDocument    DOMDOMImplementation::CreateHTMLDocument        DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(DOMDOMImplementationProto)
KJS_IMPLEMENT_PROTOFUNC(DOMDOMImplementationProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMImplementation",DOMDOMImplementationProto,DOMDOMImplementationProtoFunc,ObjectPrototype)

const ClassInfo DOMDOMImplementation::info = { "DOMImplementation", 0, 0, 0 };

DOMDOMImplementation::DOMDOMImplementation(ExecState *exec, DOM::DOMImplementationImpl* i)
  : DOMObject(DOMDOMImplementationProto::self(exec)), m_impl(i) { }

DOMDOMImplementation::~DOMDOMImplementation()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue* DOMDOMImplementationProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMDOMImplementation, thisObj );
  DOM::DOMImplementationImpl& implementation = *static_cast<DOMDOMImplementation *>( thisObj )->impl();
  DOMExceptionTranslator exception(exec);

  switch(id) {
  case DOMDOMImplementation::HasFeature:
    return jsBoolean(implementation.hasFeature(args[0]->toString(exec).domString(),
                                               valueToStringWithNullCheck(exec, args[1])));
  case DOMDOMImplementation::CreateDocumentType: // DOM2
    return getDOMNode(exec,implementation.createDocumentType(args[0]->toString(exec).domString(),args[1]->toString(exec).domString(),args[2]->toString(exec).domString(),exception));
  case DOMDOMImplementation::CreateDocument: { // DOM2
    // Initially set the URL to document of the creator... this is so that it resides in the same
    // host/domain for security checks. The URL will be updated if Document.load() is called.
    KHTMLPart *part = qobject_cast<KHTMLPart*>(static_cast<KJS::ScriptInterpreter*>(exec->dynamicInterpreter())->part());
    if (part) {
      //### this should probably be pushed to the impl
      DOM::NodeImpl* supposedDocType = toNode(args[2]);
      if (supposedDocType && supposedDocType->nodeType() != DOM::Node::DOCUMENT_TYPE_NODE) {
        setDOMException(exec, DOMException::NOT_FOUND_ERR);
        return jsNull();
      }

      DOM::DocumentTypeImpl* docType = static_cast<DOM::DocumentTypeImpl*>(supposedDocType);

      // ### no real leak but lifetime of part is not tied to document
      KHTMLPart* newPart = new KHTMLPart(part->view(), part);
      DOM::DocumentImpl* doc = implementation.createDocument(valueToStringWithNullCheck(exec, args[0]),
                                                             valueToStringWithNullCheck(exec, args[1]),
                                                             docType,
                                                             newPart->view(),
                                                             exception);
      if (!doc)
        return jsNull();
      QUrl url = static_cast<DocumentImpl*>(part->document().handle())->URL();
      doc->setURL(url.url());
      return getDOMNode(exec,doc);
    }
    break;
  }
  case DOMDOMImplementation::CreateCSSStyleSheet: // DOM2
    return getDOMStyleSheet(exec,implementation.createCSSStyleSheet(
        args[0]->toString(exec).domString().implementation(),
        args[1]->toString(exec).domString().implementation(),exception));
  case DOMDOMImplementation::CreateHTMLDocument: // DOM2-HTML
    return getDOMNode(exec, implementation.createHTMLDocument(args[0]->toString(exec).domString()));
  default:
    break;
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMDocumentType::info = { "DocumentType", &DOMNode::info, &DOMDocumentTypeTable, 0 };

/* Source for DOMDocumentTypeTable.
@begin DOMDocumentTypeTable 6
  name			DOMDocumentType::Name		DontDelete|ReadOnly
  entities		DOMDocumentType::Entities	DontDelete|ReadOnly
  notations		DOMDocumentType::Notations	DontDelete|ReadOnly
# DOM2
  publicId		DOMDocumentType::PublicId	DontDelete|ReadOnly
  systemId		DOMDocumentType::SystemId	DontDelete|ReadOnly
  internalSubset	DOMDocumentType::InternalSubset	DontDelete|ReadOnly
@end
*/
DOMDocumentType::DOMDocumentType(ExecState *exec, DOM::DocumentTypeImpl* dt)
  : DOMNode( /*### no proto yet*/exec, dt ) { }

bool DOMDocumentType::getOwnPropertySlot(ExecState *exec, const Identifier &propertyName, KJS::PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMDocumentType::getOwnPropertySlot " << propertyName.qstring();
#endif
  return getStaticValueSlot<DOMDocumentType, DOMNode>(exec, &DOMDocumentTypeTable, this, propertyName, slot);
}

JSValue* DOMDocumentType::getValueProperty(ExecState *exec, int token) const
{
  DOM::DocumentTypeImpl& type = static_cast<DOM::DocumentTypeImpl&>(*m_impl);
  switch (token) {
  case Name:
    return jsString(type.name());
  case Entities:
    return getDOMNamedNodeMap(exec,type.entities());
  case Notations:
    return getDOMNamedNodeMap(exec,type.notations());
  case PublicId: // DOM2
    return jsString(type.publicId());
  case SystemId: // DOM2
    return jsString(type.systemId());
  case InternalSubset: // DOM2
    return ::getStringOrNull(type.internalSubset()); // can be null, see domts/level2/core/internalSubset01.html
  default:
    // qDebug() << "WARNING: DOMDocumentType::getValueProperty unhandled token " << token;
    return jsNull();
  }
}

// -------------------------------------------------------------------------

/* Source for DOMNamedNodeMapProtoTable.
@begin DOMNamedNodeMapProtoTable 7
  getNamedItem		DOMNamedNodeMap::GetNamedItem		DontDelete|Function 1
  setNamedItem		DOMNamedNodeMap::SetNamedItem		DontDelete|Function 1
  removeNamedItem	DOMNamedNodeMap::RemoveNamedItem	DontDelete|Function 1
  item			DOMNamedNodeMap::Item			DontDelete|Function 1
# DOM2
  getNamedItemNS	DOMNamedNodeMap::GetNamedItemNS		DontDelete|Function 2
  setNamedItemNS	DOMNamedNodeMap::SetNamedItemNS		DontDelete|Function 1
  removeNamedItemNS	DOMNamedNodeMap::RemoveNamedItemNS	DontDelete|Function 2
@end
@begin DOMNamedNodeMapTable 7
  length		DOMNamedNodeMap::Length			DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(DOMNamedNodeMapProto)
KJS_IMPLEMENT_PROTOFUNC(DOMNamedNodeMapProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("NamedNodeMap",DOMNamedNodeMapProto,DOMNamedNodeMapProtoFunc,ObjectPrototype)

const ClassInfo DOMNamedNodeMap::info = { "NamedNodeMap", 0, &DOMNamedNodeMapTable, 0 };

DOMNamedNodeMap::DOMNamedNodeMap(ExecState *exec, DOM::NamedNodeMapImpl* m)
  : DOMObject(DOMNamedNodeMapProto::self(exec)), m_impl(m) { }

DOMNamedNodeMap::~DOMNamedNodeMap()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue* DOMNamedNodeMap::indexGetter(ExecState *exec, unsigned index)
{
  return getDOMNode(exec, m_impl->item(index));
}

JSValue *DOMNamedNodeMap::lengthGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot& slot)
{
  DOMNamedNodeMap *thisObj = static_cast<DOMNamedNodeMap *>(slot.slotBase());
  return jsNumber(thisObj->m_impl->length());
}

bool DOMNamedNodeMap::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot) {
  if (propertyName == exec->propertyNames().length) {
    slot.setCustom(this, lengthGetter);
    return true;
  }

  // See if it's an item name get
  DOM::NodeImpl* attr = impl()->getNamedItem(propertyName.ustring().domString());
  if (attr)
    return getImmediateValueSlot(this, getDOMNode(exec, attr), slot);

  //May be it's an index?
  if (getIndexSlot(this, *m_impl, propertyName, slot))
    return true;

  return DOMObject::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue* DOMNamedNodeMapProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMNamedNodeMap, thisObj );
  DOMExceptionTranslator exception(exec);
  DOM::NamedNodeMapImpl& map = *static_cast<DOMNamedNodeMap *>(thisObj)->impl();

  switch(id) {
    case DOMNamedNodeMap::GetNamedItem:
      return getDOMNode(exec, map.getNamedItem(args[0]->toString(exec).domString()));
    case DOMNamedNodeMap::SetNamedItem: {
      DOM::Node old = map.setNamedItem(toNode(args[0]),exception);
      return getDOMNode(exec, old.handle());
    }
    case DOMNamedNodeMap::RemoveNamedItem: {
      DOM::Attr toRet = map.removeNamedItem(args[0]->toString(exec).domString(), exception);
      return getDOMNode(exec, toRet.handle());
    }
    case DOMNamedNodeMap::Item:
      return getDOMNode(exec, map.item(args[0]->toInt32(exec)));
    case DOMNamedNodeMap::GetNamedItemNS: {// DOM2
      DOM::Node old = map.getNamedItemNS(args[0]->toString(exec).domString(),args[1]->toString(exec).domString());
      return getDOMNode(exec, old.handle());
    }
    case DOMNamedNodeMap::SetNamedItemNS: {// DOM2
      DOM::Node old = map.setNamedItemNS(toNode(args[0]),exception);
      return getDOMNode(exec, old.handle());
    }
    case DOMNamedNodeMap::RemoveNamedItemNS: { // DOM2
      DOM::Node old = map.removeNamedItemNS(args[0]->toString(exec).domString(),args[1]->toString(exec).domString(),exception);
      return getDOMNode(exec, old.handle());
    }
    default:
      break;
  }

  return jsUndefined();
}

// -------------------------------------------------------------------------
//### FIXME: proto
const ClassInfo DOMProcessingInstruction::info = { "ProcessingInstruction", &DOMNode::info, &DOMProcessingInstructionTable, 0 };

/* Source for DOMProcessingInstructionTable.
@begin DOMProcessingInstructionTable 3
  target	DOMProcessingInstruction::Target	DontDelete|ReadOnly
  data		DOMProcessingInstruction::Data		DontDelete
  sheet		DOMProcessingInstruction::Sheet		DontDelete|ReadOnly
@end
*/
bool DOMProcessingInstruction::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMProcessingInstruction, DOMNode>(exec, &DOMProcessingInstructionTable, this, propertyName, slot);
}

JSValue* DOMProcessingInstruction::getValueProperty(ExecState *exec, int token) const
{
  DOM::ProcessingInstructionImpl& pi = *static_cast<DOM::ProcessingInstructionImpl*>(m_impl.get());
  switch (token) {
  case Target:
    return jsString(pi.target());
  case Data:
    return jsString(pi.data());
  case Sheet:
    return getDOMStyleSheet(exec,pi.sheet());
  default:
    // qDebug() << "WARNING: DOMProcessingInstruction::getValueProperty unhandled token " << token;
    return jsNull();
  }
}

void DOMProcessingInstruction::put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr)
{
  DOM::ProcessingInstructionImpl& pi = *static_cast<DOM::ProcessingInstructionImpl*>(m_impl.get());
  // Not worth using the hashtable for this one ;)
  if (propertyName == "data") {
    DOMExceptionTranslator exception(exec);
    pi.setData(value->toString(exec).domString(),exception);
  } else
    DOMNode::put(exec, propertyName,value,attr);
}

// -------------------------------------------------------------------------

const ClassInfo DOMNotation::info = { "Notation", &DOMNode::info, &DOMNotationTable, 0 };

/* Source for DOMNotationTable.
@begin DOMNotationTable 2
  publicId		DOMNotation::PublicId	DontDelete|ReadOnly
  systemId		DOMNotation::SystemId	DontDelete|ReadOnly
@end
*/
bool DOMNotation::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMNotation, DOMNode>(exec, &DOMNotationTable, this, propertyName, slot);
}

JSValue* DOMNotation::getValueProperty(ExecState *, int token) const
{
  DOM::NotationImpl& nota = *static_cast<DOM::NotationImpl*>(m_impl.get());
  switch (token) {
  case PublicId:
    return jsString(nota.publicId());
  case SystemId:
    return jsString(nota.systemId());
  default:
    // qDebug() << "WARNING: DOMNotation::getValueProperty unhandled token " << token;
    return jsNull();
  }
}

// -------------------------------------------------------------------------

const ClassInfo DOMEntity::info = { "Entity", &DOMNode::info, 0, 0 };

/* Source for DOMEntityTable.
@begin DOMEntityTable 2
  publicId		DOMEntity::PublicId		DontDelete|ReadOnly
  systemId		DOMEntity::SystemId		DontDelete|ReadOnly
  notationName		DOMEntity::NotationName	DontDelete|ReadOnly
@end
*/
bool DOMEntity::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMEntity, DOMNode>(exec, &DOMEntityTable, this, propertyName, slot);
}

JSValue* DOMEntity::getValueProperty(ExecState *, int token) const
{
  DOM::EntityImpl& entity = *static_cast<DOM::EntityImpl*>(m_impl.get());
  switch (token) {
  case PublicId:
    return jsString(entity.publicId());
  case SystemId:
    return jsString(entity.systemId());
  case NotationName:
    return jsString(entity.notationName());
  default:
    // qDebug() << "WARNING: DOMEntity::getValueProperty unhandled token " << token;
    return jsNull();
  }
}

// -------------------------------------------------------------------------

bool KJS::checkNodeSecurity(ExecState *exec, const DOM::NodeImpl* n)
{
  // Check to see if the currently executing interpreter is allowed to access the specified node
  if (!n)
    return true;
  KHTMLPart* part = n->document()->part();
  Window* win = part ? Window::retrieveWindow(part) : 0L;
  if ( !win || !win->isSafeScript(exec) )
    return false;
  return true;
}

// adopted from binding/JSHTMLElementFactory.cpp
#define CREATE_WRAPPER_FUNCTION(name) \
static DOMObject* create##name##Wrapper(ExecState* exec, DOM::NodeImpl* n) \
{ \
    return new JSHTML##name##Element(exec, static_cast<HTML##name##Element*>(n)); \
}

CREATE_WRAPPER_FUNCTION(Audio)
CREATE_WRAPPER_FUNCTION(Video)

JSValue* KJS::getEventTarget(ExecState* exec, DOM::EventTargetImpl* t)
{
    if (!t)
        return jsNull();
    if (t->eventTargetType() == EventTargetImpl::DOM_NODE) {
        return getDOMNode(exec, static_cast<DOM::NodeImpl*>(t));
    } else if (t->eventTargetType() == EventTargetImpl::WINDOW) {
        Window* w = static_cast<WindowEventTargetImpl*>(t)->window();
        return w ? w : jsNull();
    } else {
        return static_cast<XMLHttpRequest*>(t);
    }
}

JSValue* KJS::getDOMNode(ExecState *exec, DOM::NodeImpl* n)
{
  DOMObject *ret = 0;
  if (!n)
    return jsNull();
  ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());
  if ((ret = interp->getDOMObject(n)))
    return ret;

  switch (n->nodeType()) {
    case DOM::Node::ELEMENT_NODE:
      switch (n->id()) {
      case ID_AUDIO:
	ret = createAudioWrapper(exec, n);
	break;
      case ID_VIDEO:
	ret = createVideoWrapper(exec, n);
	break;
      default:
	if (static_cast<DOM::ElementImpl*>(n)->isHTMLElement())
	  ret = new HTMLElement(exec, static_cast<DOM::HTMLElementImpl*>(n));
	else
	  ret = new DOMElement(exec, static_cast<DOM::ElementImpl*>(n));
	break;
      }
      break;
    case DOM::Node::ATTRIBUTE_NODE:
      ret = new DOMAttr(exec, static_cast<DOM::AttrImpl*>(n));
      break;
    case DOM::Node::TEXT_NODE:
    case DOM::Node::CDATA_SECTION_NODE:
      ret = new DOMText(exec, static_cast<DOM::TextImpl*>(n));
      break;
    case DOM::Node::ENTITY_REFERENCE_NODE:
      ret = new DOMNode(exec, n);
      break;
    case DOM::Node::ENTITY_NODE:
      ret = new DOMEntity(exec, static_cast<DOM::EntityImpl*>(n));
      break;
    case DOM::Node::PROCESSING_INSTRUCTION_NODE:
      ret = new DOMProcessingInstruction(exec, static_cast<DOM::ProcessingInstructionImpl*>(n));
      break;
    case DOM::Node::COMMENT_NODE:
      ret = new DOMComment(exec, static_cast<DOM::CommentImpl*>(n));
      break;
    case DOM::Node::DOCUMENT_NODE:
      if (static_cast<DOM::DocumentImpl*>(n)->isHTMLDocument())
        ret = new HTMLDocument(exec, static_cast<DOM::HTMLDocumentImpl*>(n));
      else
        ret = new DOMDocument(exec, static_cast<DOM::DocumentImpl*>(n));
      break;
    case DOM::Node::DOCUMENT_TYPE_NODE:
      ret = new DOMDocumentType(exec, static_cast<DOM::DocumentTypeImpl*>(n));
      break;
    case DOM::Node::DOCUMENT_FRAGMENT_NODE:
      ret = new DOMDocumentFragment(exec, static_cast<DOM::DocumentFragmentImpl*>(n));
      break;
    case DOM::Node::NOTATION_NODE:
      ret = new DOMNotation(exec, static_cast<DOM::NotationImpl*>(n));
      break;
    default:
      ret = new DOMNode(exec, n);
  }
  interp->putDOMObject(n,ret);

  return ret;
}

JSValue* KJS::getDOMNamedNodeMap(ExecState *exec, DOM::NamedNodeMapImpl* m)
{
  return cacheDOMObject<DOM::NamedNodeMapImpl, KJS::DOMNamedNodeMap>(exec, m);
}

JSValue* KJS::getDOMNodeList(ExecState *exec, DOM::NodeListImpl* l)
{
  return cacheDOMObject<DOM::NodeListImpl, KJS::DOMNodeList>(exec, l);
}

JSValue* KJS::getDOMDOMImplementation(ExecState *exec, DOM::DOMImplementationImpl* i)
{
  return cacheDOMObject<DOM::DOMImplementationImpl, KJS::DOMDOMImplementation>(exec, i);
}

// -------------------------------------------------------------------------

IMPLEMENT_PSEUDO_CONSTRUCTOR_WITH_PARENT(NodeConstructor, "NodeConstructor", DOMNodeProto, DOMNodeConstants)
// -------------------------------------------------------------------------

/* Source for DOMExceptionProtoTable.
@begin DOMExceptionProtoTable 29
  INDEX_SIZE_ERR		DOM::DOMException::INDEX_SIZE_ERR		DontDelete|ReadOnly
  DOMSTRING_SIZE_ERR		DOM::DOMException::DOMSTRING_SIZE_ERR	DontDelete|ReadOnly
  HIERARCHY_REQUEST_ERR		DOM::DOMException::HIERARCHY_REQUEST_ERR	DontDelete|ReadOnly
  WRONG_DOCUMENT_ERR		DOM::DOMException::WRONG_DOCUMENT_ERR	DontDelete|ReadOnly
  INVALID_CHARACTER_ERR		DOM::DOMException::INVALID_CHARACTER_ERR	DontDelete|ReadOnly
  NO_DATA_ALLOWED_ERR		DOM::DOMException::NO_DATA_ALLOWED_ERR	DontDelete|ReadOnly
  NO_MODIFICATION_ALLOWED_ERR	DOM::DOMException::NO_MODIFICATION_ALLOWED_ERR	DontDelete|ReadOnly
  NOT_FOUND_ERR			DOM::DOMException::NOT_FOUND_ERR		DontDelete|ReadOnly
  NOT_SUPPORTED_ERR		DOM::DOMException::NOT_SUPPORTED_ERR	DontDelete|ReadOnly
  INUSE_ATTRIBUTE_ERR		DOM::DOMException::INUSE_ATTRIBUTE_ERR	DontDelete|ReadOnly
  INVALID_STATE_ERR		DOM::DOMException::INVALID_STATE_ERR	DontDelete|ReadOnly
  SYNTAX_ERR			DOM::DOMException::SYNTAX_ERR		DontDelete|ReadOnly
  INVALID_MODIFICATION_ERR	DOM::DOMException::INVALID_MODIFICATION_ERR	DontDelete|ReadOnly
  NAMESPACE_ERR			DOM::DOMException::NAMESPACE_ERR		DontDelete|ReadOnly
  INVALID_ACCESS_ERR		DOM::DOMException::INVALID_ACCESS_ERR	DontDelete|ReadOnly
  VALIDATION_ERR               DOM::DOMException::VALIDATION_ERR      DontDelete|ReadOnly
  TYPE_MISMATCH_ERR            DOM::DOMException::TYPE_MISMATCH_ERR   DontDelete|ReadOnly
  SECURITY_ERR                 DOM::DOMException::SECURITY_ERR        DontDelete|ReadOnly            
  NETWORK_ERR                  DOM::DOMException::NETWORK_ERR         DontDelete|ReadOnly  
  ABORT_ERR                    DOM::DOMException::ABORT_ERR           DontDelete|ReadOnly
  URL_MISMATCH_ERR             DOM::DOMException::URL_MISMATCH_ERR    DontDelete|ReadOnly
  QUOTA_EXCEEDED_ERR           DOM::DOMException::QUOTA_EXCEEDED_ERR  DontDelete|ReadOnly
  TIMEOUT_ERR                  DOM::DOMException::TIMEOUT_ERR         DontDelete|ReadOnly
  NOT_READABLE_ERR             DOM::DOMException::NOT_READABLE_ERR    DontDelete|ReadOnly
  DATA_CLONE_ERR               DOM::DOMException::DATA_CLONE_ERR      DontDelete|ReadOnly
  ENCODING_ERR                 DOM::DOMException::ENCODING_ERR        DontDelete|ReadOnly
@end
*/

DEFINE_CONSTANT_TABLE(DOMExceptionProto)
IMPLEMENT_CONSTANT_TABLE(DOMExceptionProto, "DOMException")

IMPLEMENT_PSEUDO_CONSTRUCTOR_WITH_PARENT(DOMExceptionPseudoCtor,
                             "DOMException",
                             DOMExceptionProto, DOMExceptionProto)

JSDOMException::JSDOMException(ExecState* exec)
  : DOMObject(DOMExceptionProto::self(exec))
{
}

const ClassInfo JSDOMException::info = { "DOMException", 0, 0, 0 };

JSObject *KJS::getDOMExceptionConstructor(ExecState *exec)
{
  return DOMExceptionPseudoCtor::self(exec);
}

// -------------------------------------------------------------------------

/* Source for DOMNamedNodesCollection.
@begin DOMNamedNodesCollectionTable 1
  length		KJS::DOMNamedNodesCollection::Length		DontDelete|ReadOnly
@end
*/
const ClassInfo KJS::DOMNamedNodesCollection::info = { "DOMNamedNodesCollection", 0, &DOMNamedNodesCollectionTable, 0 };

// Such a collection is usually very short-lived, it only exists
// for constructs like document.forms.<name>[1],
// so it shouldn't be a problem that it's storing all the nodes (with the same name). (David)
DOMNamedNodesCollection::DOMNamedNodesCollection(ExecState *exec, const QList<SharedPtr<DOM::NodeImpl> >& nodes)
  : DOMObject(exec->lexicalInterpreter()->builtinObjectPrototype()),
  m_nodes(nodes)
{
  // Maybe we should ref (and deref in the dtor) the nodes, though ?
}

JSValue* DOMNamedNodesCollection::indexGetter(ExecState *exec, unsigned index)
{
  return getDOMNode(exec, m_nodes[index].get());
}

JSValue *DOMNamedNodesCollection::lengthGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot& slot)
{
  DOMNamedNodesCollection *thisObj = static_cast<DOMNamedNodesCollection *>(slot.slotBase());
  return jsNumber(thisObj->m_nodes.size());
}


bool DOMNamedNodesCollection::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  // qDebug() << propertyName.ascii();

  if (propertyName == exec->propertyNames().length) {
    slot.setCustom(this, lengthGetter);
    return true;
  }

  //May be it's an index?
  if (getIndexSlot(this, m_nodes.size(), propertyName, slot))
    return true;

  return DOMObject::getOwnPropertySlot(exec,propertyName,slot);
}

// -------------------------------------------------------------------------

const ClassInfo DOMCharacterData::info = { "CharacterImp",
					  &DOMNode::info, &DOMCharacterDataTable, 0 };
/*
@begin DOMCharacterDataTable 2
  data		DOMCharacterData::Data		DontDelete
  length	DOMCharacterData::Length	DontDelete|ReadOnly
@end
@begin DOMCharacterDataProtoTable 7
  substringData	DOMCharacterData::SubstringData	DontDelete|Function 2
  appendData	DOMCharacterData::AppendData	DontDelete|Function 1
  insertData	DOMCharacterData::InsertData	DontDelete|Function 2
  deleteData	DOMCharacterData::DeleteData	DontDelete|Function 2
  replaceData	DOMCharacterData::ReplaceData	DontDelete|Function 2
@end
*/
KJS_DEFINE_PROTOTYPE(DOMCharacterDataProto)
KJS_IMPLEMENT_PROTOFUNC(DOMCharacterDataProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMCharacterData",DOMCharacterDataProto,DOMCharacterDataProtoFunc,DOMNodeProto)

DOMCharacterData::DOMCharacterData(ExecState *exec, DOM::CharacterDataImpl* d)
 : DOMNode(exec, d)
 {
    setPrototype(DOMCharacterDataProto::self(exec));
 }

bool DOMCharacterData::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug()<<"DOMCharacterData::tryGet "<<propertyName.qstring();
#endif
  return getStaticValueSlot<DOMCharacterData, DOMNode>(exec, &DOMCharacterDataTable, this, propertyName, slot);
}

JSValue* DOMCharacterData::getValueProperty(ExecState *, int token) const
{
  DOM::CharacterDataImpl& data = *impl();
  switch (token) {
  case Data:
    return jsString(data.data());
  case Length:
    return jsNumber(data.length());
 default:
   // qDebug() << "WARNING: Unhandled token in DOMCharacterData::getValueProperty : " << token;
   return jsNull();
  }
}

void DOMCharacterData::put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr)
{
  if (propertyName == "data") {
    DOMExceptionTranslator exception(exec);
    impl()->setData(value->toString(exec).domString(), exception);
  } else
    DOMNode::put(exec, propertyName,value,attr);
}

JSValue* DOMCharacterDataProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMCharacterData, thisObj );
  DOM::CharacterDataImpl& data = *static_cast<DOMCharacterData *>(thisObj)->impl();
  DOMExceptionTranslator exception(exec);
  switch(id) {
    case DOMCharacterData::SubstringData:
      return jsString(data.substringData(args[0]->toInteger(exec),args[1]->toInteger(exec),exception));
    case DOMCharacterData::AppendData:
      data.appendData(args[0]->toString(exec).domString(),exception);
      return jsUndefined();
      break;
    case DOMCharacterData::InsertData:
      data.insertData(args[0]->toInteger(exec),args[1]->toString(exec).domString(), exception);
      return  jsUndefined();
      break;
    case DOMCharacterData::DeleteData:
      data.deleteData(args[0]->toInteger(exec),args[1]->toInteger(exec),exception);
      return  jsUndefined();
      break;
    case DOMCharacterData::ReplaceData:
      data.replaceData(args[0]->toInteger(exec),args[1]->toInteger(exec),args[2]->toString(exec).domString(),exception);
      return jsUndefined();
    default:
      break;
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMText::info = { "Text", &DOMCharacterData::info,
                                  &DOMTextTable , 0 };

/*
@begin DOMTextTable 2
  wholeText        DOMText::WholeText        DontDelete|ReadOnly
@end
@begin DOMTextProtoTable 2
  splitText	   DOMText::SplitText	     DontDelete|Function 1
  replaceWholeText DOMText::ReplaceWholeText DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(DOMTextProto)
KJS_IMPLEMENT_PROTOFUNC(DOMTextProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMText",DOMTextProto,DOMTextProtoFunc,DOMCharacterDataProto)

DOMText::DOMText(ExecState *exec, DOM::TextImpl* t)
  : DOMCharacterData(exec, t)
{
  setPrototype(DOMTextProto::self(exec));
}

bool DOMText::getOwnPropertySlot(ExecState* exec,
                                 const Identifier& propertyName,
                                 PropertySlot& slot)
{
    return getStaticValueSlot<DOMText, DOMCharacterData>(exec, &DOMTextTable, this, propertyName, slot);
}

JSValue* DOMText::getValueProperty(ExecState*, int token) const
{
    TextImpl* text = static_cast<TextImpl*>(impl());
    switch (token) {
    case WholeText:
        return jsString(text->wholeText());
    }
    return jsNull(); // not reached
}

JSValue* DOMTextProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMText, thisObj );
  DOM::TextImpl& text = *static_cast<DOMText *>(thisObj)->impl();
  DOMExceptionTranslator exception(exec);
  switch(id) {
    case DOMText::SplitText:
      return getDOMNode(exec,text.splitText(args[0]->toInteger(exec), exception));
    case DOMText::ReplaceWholeText:
      return getDOMNode(exec, text.replaceWholeText(args[0]->toString(exec).domString(), exception));
    default:
      break;
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMComment::info = { "Comment",
                                    &DOMCharacterData::info, 0, 0 };
/*
@begin DOMCommentProtoTable 1
@end
*/

KJS_DEFINE_PROTOTYPE(DOMCommentProto)
KJS_IMPLEMENT_PROTOFUNC(DOMCommentProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMComment",DOMCommentProto,DOMCommentProtoFunc,DOMCharacterDataProto)

DOMComment::DOMComment(ExecState *exec, DOM::CommentImpl* d)
 : DOMCharacterData(exec, d)
 {
    setPrototype(DOMCommentProto::self(exec));
 }

JSValue* DOMCommentProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List & /*args*/)
{
  KJS_CHECK_THIS( KJS::DOMComment, thisObj );
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMDocumentFragment::info = { "DocumentFragment",
                                    &DOMNode::info, 0, 0 };
/*
@begin DOMDocumentFragmentProtoTable 2
  querySelector          DOMDocumentFragment::QuerySelector            DontDelete|Function 1
  querySelectorAll       DOMDocumentFragment::QuerySelectorAll         DontDelete|Function 1
@end
*/

KJS_DEFINE_PROTOTYPE(DOMDocumentFragmentProto)
KJS_IMPLEMENT_PROTOFUNC(DOMDocumentFragmentProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DocumentFragment",DOMDocumentFragmentProto,
                        DOMDocumentFragmentProtoFunc,DOMNodeProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(DocumentFragmentPseudoCtor, "DocumentFragment", DOMDocumentFragmentProto)

DOMDocumentFragment::DOMDocumentFragment(ExecState* exec, DOM::DocumentFragmentImpl* i)
 : DOMNode(exec, i)
{
    setPrototype(DOMDocumentFragmentProto::self(exec));
}

JSValue* DOMDocumentFragmentProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMDocumentFragment, thisObj );
  DOMExceptionTranslator exception(exec);  

  DOM::NodeImpl* n = static_cast<DOMDocumentFragment*>(thisObj)->impl();
  DOM::DOMString s = args[0]->toString(exec).domString();
  
  switch (id) {
  case DOMDocumentFragment::QuerySelector: {
    RefPtr<DOM::ElementImpl> e = n->querySelector(s, exception);
    return getDOMNode(exec, e.get());
  }
  case DOMDocumentFragment::QuerySelectorAll: {
    RefPtr<DOM::NodeListImpl> l = n->querySelectorAll(s, exception);
    return getDOMNodeList(exec, l.get());
  }
  }
  return jsUndefined();
}


// kate: indent-width 2; replace-tabs on; tab-width 4; space-indent on;
