// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
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

#include "kjs_traversal.h"
#include "kjs_traversal.lut.h"
#include "kjs_proxy.h"
#include <dom/dom_node.h>
#include <xml/dom_nodeimpl.h>
#include <xml/dom_docimpl.h>
#include <khtmlview.h>
#include <khtml_part.h>
#include <QDebug>

using namespace KJS;

namespace KJS {

class TraversalExceptionForwarder {
public:
  explicit TraversalExceptionForwarder(ExecState *exec) : m_exec(exec), m_code(0) { }
  ~TraversalExceptionForwarder() { if (m_code) m_exec->setException(static_cast<JSValue*>(m_code)); }
  operator void* &() { return m_code; }
private:
  ExecState *m_exec;
  void* m_code;
};

}
// -------------------------------------------------------------------------

const ClassInfo DOMNodeIterator::info = { "NodeIterator", 0, &DOMNodeIteratorTable, 0 };
/*
@begin DOMNodeIteratorTable 5
  root				DOMNodeIterator::Root			DontDelete|ReadOnly
  whatToShow			DOMNodeIterator::WhatToShow		DontDelete|ReadOnly
  filter			DOMNodeIterator::Filter			DontDelete|ReadOnly
  expandEntityReferences	DOMNodeIterator::ExpandEntityReferences	DontDelete|ReadOnly
@end
@begin DOMNodeIteratorProtoTable 3
  nextNode	DOMNodeIterator::NextNode	DontDelete|Function 0
  previousNode	DOMNodeIterator::PreviousNode	DontDelete|Function 0
  detach	DOMNodeIterator::Detach		DontDelete|Function 0
@end
*/
KJS_DEFINE_PROTOTYPE(DOMNodeIteratorProto)
KJS_IMPLEMENT_PROTOFUNC(DOMNodeIteratorProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMNodeIterator",DOMNodeIteratorProto,DOMNodeIteratorProtoFunc,ObjectPrototype)

DOMNodeIterator::DOMNodeIterator(ExecState *exec, DOM::NodeIteratorImpl* ni)
  : DOMObject(DOMNodeIteratorProto::self(exec)), m_impl(ni) {}

DOMNodeIterator::~DOMNodeIterator()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

bool DOMNodeIterator::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMNodeIterator,DOMObject>(exec, &DOMNodeIteratorTable, this, propertyName, slot);
}

JSValue *DOMNodeIterator::getValueProperty(ExecState *exec, int token) const
{
  DOM::NodeIteratorImpl& ni = *impl();
  switch (token) {
  case Root:
    return getDOMNode(exec,ni.root());
  case WhatToShow:
    return jsNumber(ni.whatToShow());
  case Filter:
    return getDOMNodeFilter(exec,ni.filter());
  case ExpandEntityReferences:
    return jsBoolean(ni.expandEntityReferences());
 default:
   // qDebug() << "WARNING: Unhandled token in DOMNodeIterator::getValueProperty : " << token;
   return 0;
  }
}

JSValue* DOMNodeIteratorProtoFunc::callAsFunction(ExecState *exec, JSObject* thisObj, const List &)
{
  KJS_CHECK_THIS( KJS::DOMNodeIterator, thisObj );
  DOMExceptionTranslator      exception(exec);
  TraversalExceptionForwarder filterException(exec);
  DOM::NodeIteratorImpl& nodeIterator = *static_cast<DOMNodeIterator *>(thisObj)->impl();
  switch (id) {
  case DOMNodeIterator::PreviousNode: {
    SharedPtr<DOM::NodeImpl> node = nodeIterator.previousNode(exception, filterException);
    if (!filterException)
      return getDOMNode(exec, node.get());
    break;
  }
  case DOMNodeIterator::NextNode: {
    SharedPtr<DOM::NodeImpl> node = nodeIterator.nextNode(exception, filterException);
    if (!filterException)
      return getDOMNode(exec, node.get());
    break;
  }
  case DOMNodeIterator::Detach:
    nodeIterator.detach(exception);
    return jsUndefined();
  }
  return jsUndefined();
}

JSValue *KJS::getDOMNodeIterator(ExecState *exec, DOM::NodeIteratorImpl* ni)
{
  return cacheDOMObject<DOM::NodeIteratorImpl, DOMNodeIterator>(exec, ni);
}


// -------------------------------------------------------------------------

const ClassInfo NodeFilterConstructor::info = { "NodeFilterConstructor", 0, &NodeFilterConstructorTable, 0 };
/*
@begin NodeFilterConstructorTable 17
  FILTER_ACCEPT		DOM::NodeFilter::FILTER_ACCEPT	DontDelete|ReadOnly
  FILTER_REJECT		DOM::NodeFilter::FILTER_REJECT	DontDelete|ReadOnly
  FILTER_SKIP		DOM::NodeFilter::FILTER_SKIP	DontDelete|ReadOnly
  SHOW_ALL		DOM::NodeFilter::SHOW_ALL	DontDelete|ReadOnly
  SHOW_ELEMENT		DOM::NodeFilter::SHOW_ELEMENT	DontDelete|ReadOnly
  SHOW_ATTRIBUTE	DOM::NodeFilter::SHOW_ATTRIBUTE	DontDelete|ReadOnly
  SHOW_TEXT		DOM::NodeFilter::SHOW_TEXT	DontDelete|ReadOnly
  SHOW_CDATA_SECTION	DOM::NodeFilter::SHOW_CDATA_SECTION	DontDelete|ReadOnly
  SHOW_ENTITY_REFERENCE	DOM::NodeFilter::SHOW_ENTITY_REFERENCE	DontDelete|ReadOnly
  SHOW_ENTITY		DOM::NodeFilter::SHOW_ENTITY	DontDelete|ReadOnly
  SHOW_PROCESSING_INSTRUCTION	DOM::NodeFilter::SHOW_PROCESSING_INSTRUCTION	DontDelete|ReadOnly
  SHOW_COMMENT		DOM::NodeFilter::SHOW_COMMENT	DontDelete|ReadOnly
  SHOW_DOCUMENT		DOM::NodeFilter::SHOW_DOCUMENT	DontDelete|ReadOnly
  SHOW_DOCUMENT_TYPE	DOM::NodeFilter::SHOW_DOCUMENT_TYPE	DontDelete|ReadOnly
  SHOW_DOCUMENT_FRAGMENT	DOM::NodeFilter::SHOW_DOCUMENT_FRAGMENT	DontDelete|ReadOnly
  SHOW_NOTATION		DOM::NodeFilter::SHOW_NOTATION	DontDelete|ReadOnly
@end
*/

NodeFilterConstructor::NodeFilterConstructor(ExecState* exec)
  : DOMObject(exec->lexicalInterpreter()->builtinObjectPrototype())
{
}

bool NodeFilterConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<NodeFilterConstructor,DOMObject>(exec, &NodeFilterConstructorTable, this, propertyName, slot);
}

JSValue *NodeFilterConstructor::getValueProperty(ExecState *, int token) const
{
  // We use the token as the value to return directly
  return jsNumber(token);
}

JSValue *KJS::getNodeFilterConstructor(ExecState *exec)
{
  return cacheGlobalObject<NodeFilterConstructor>(exec, "[[nodeFilter.constructor]]");
}

// -------------------------------------------------------------------------

const ClassInfo DOMTreeWalker::info = { "TreeWalker", 0, &DOMTreeWalkerTable, 0 };
/*
@begin DOMTreeWalkerTable 5
  root			DOMTreeWalker::Root		DontDelete|ReadOnly
  whatToShow		DOMTreeWalker::WhatToShow	DontDelete|ReadOnly
  filter		DOMTreeWalker::Filter		DontDelete|ReadOnly
  expandEntityReferences DOMTreeWalker::ExpandEntityReferences	DontDelete|ReadOnly
  currentNode		DOMTreeWalker::CurrentNode	DontDelete
@end
@begin DOMTreeWalkerProtoTable 7
  parentNode	DOMTreeWalker::ParentNode	DontDelete|Function 0
  firstChild	DOMTreeWalker::FirstChild	DontDelete|Function 0
  lastChild	DOMTreeWalker::LastChild	DontDelete|Function 0
  previousSibling DOMTreeWalker::PreviousSibling	DontDelete|Function 0
  nextSibling	DOMTreeWalker::NextSibling	DontDelete|Function 0
  previousNode	DOMTreeWalker::PreviousNode	DontDelete|Function 0
  nextNode	DOMTreeWalker::NextNode		DontDelete|Function 0
@end
*/
KJS_DEFINE_PROTOTYPE(DOMTreeWalkerProto)
KJS_IMPLEMENT_PROTOFUNC(DOMTreeWalkerProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMTreeWalker",DOMTreeWalkerProto,DOMTreeWalkerProtoFunc,ObjectPrototype)

DOMTreeWalker::DOMTreeWalker(ExecState *exec, DOM::TreeWalkerImpl* tw)
  : m_impl(tw) {
  setPrototype(DOMTreeWalkerProto::self(exec));
}

DOMTreeWalker::~DOMTreeWalker()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

void DOMTreeWalker::mark()
{
  JSObject::mark();
  JSNodeFilter* filt = JSNodeFilter::fromDOMFilter(impl()->getFilter());
  if (filt)
    filt->mark();
}

bool DOMTreeWalker::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMTreeWalker,DOMObject>(exec, &DOMTreeWalkerTable, this, propertyName, slot);
}

JSValue *DOMTreeWalker::getValueProperty(ExecState *exec, int token) const
{
  DOM::TreeWalkerImpl& tw = *impl();
  switch (token) {
  case Root:
    return getDOMNode(exec,tw.getRoot());
  case WhatToShow:
    return jsNumber(tw.getWhatToShow());
  case Filter:
    return getDOMNodeFilter(exec,tw.getFilter());
  case ExpandEntityReferences:
    return jsBoolean(tw.getExpandEntityReferences());
  case CurrentNode:
    return getDOMNode(exec,tw.getCurrentNode());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMTreeWalker::getValueProperty : " << token;
    return 0;
  }
}

void DOMTreeWalker::put(ExecState *exec, const Identifier &propertyName,
                           JSValue *value, int attr)
{
  DOMExceptionTranslator exception(exec);
  if (propertyName == "currentNode") {
    m_impl->setCurrentNode(toNode(value), exception);
  }
  else
    JSObject::put(exec, propertyName, value, attr);
}

JSValue* DOMTreeWalkerProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &)
{
  KJS_CHECK_THIS( KJS::DOMTreeWalker, thisObj );
  DOM::TreeWalkerImpl& treeWalker = *static_cast<DOMTreeWalker *>(thisObj)->impl();
  TraversalExceptionForwarder   filterException(exec);
  switch (id) {
    case DOMTreeWalker::ParentNode:
      return getDOMNode(exec,treeWalker.parentNode(filterException));
    case DOMTreeWalker::FirstChild:
      return getDOMNode(exec,treeWalker.firstChild(filterException));
    case DOMTreeWalker::LastChild:
      return getDOMNode(exec,treeWalker.lastChild(filterException));
    case DOMTreeWalker::PreviousSibling:
      return getDOMNode(exec,treeWalker.previousSibling(filterException));
    case DOMTreeWalker::NextSibling:
      return getDOMNode(exec,treeWalker.nextSibling(filterException));
    case DOMTreeWalker::PreviousNode:
      return getDOMNode(exec,treeWalker.previousNode(filterException));
    case DOMTreeWalker::NextNode:
      return getDOMNode(exec,treeWalker.nextNode(filterException));
  }
  return jsUndefined();
}

JSValue *KJS::getDOMTreeWalker(ExecState *exec, DOM::TreeWalkerImpl* tw)
{
  return cacheDOMObject<DOM::TreeWalkerImpl, DOMTreeWalker>(exec, tw);
}

// -------------------------------------------------------------------------


DOM::NodeFilterImpl* KJS::toNodeFilter(JSValue *val)
{
  JSObject *obj = val->getObject();
  if (!obj)
    return 0;

  return new JSNodeFilter(obj);
}

JSValue *KJS::getDOMNodeFilter(ExecState *exec, DOM::NodeFilterImpl* nf)
{
  Q_UNUSED(exec);
  if (nf && nf->isJSFilter()) {
    return static_cast<JSNodeFilter*>(nf)->filter();
  }

  return jsNull();
}

JSNodeFilter::JSNodeFilter(JSObject *_filter) : m_filter( _filter )
{}

JSNodeFilter::~JSNodeFilter()
{}

void JSNodeFilter::mark()
{
  if (!m_filter->marked())
    m_filter->mark();
}

bool JSNodeFilter::isJSFilter() const
{
  return true;
}

JSNodeFilter* JSNodeFilter::fromDOMFilter(DOM::NodeFilterImpl* nf)
{
  if (!nf || !nf->isJSFilter())
    return 0;

  return static_cast<JSNodeFilter*>(nf);
}

short JSNodeFilter::acceptNode(const DOM::Node &n, void*& bindingsException)
{
  KHTMLPart* part = n.handle()->document()->part();
  if (!part)
      return DOM::NodeFilter::FILTER_REJECT;

  KJSProxy *proxy = part->jScript();
  if (proxy) {
    ExecState *exec = proxy->interpreter()->globalExec();
    JSObject* fn = 0;

    // Use a function given directly, or extract it from the acceptNode field
    if (m_filter->implementsCall()) {
      fn = m_filter;
    } else {
      JSObject *acceptNodeFunc = m_filter->get(exec, "acceptNode")->getObject();
      if (acceptNodeFunc && acceptNodeFunc->implementsCall())
        fn = acceptNodeFunc;
    }

    if (fn) {
      List args;
      args.append(getDOMNode(exec,n.handle()));
      JSValue *result = fn->call(exec, m_filter, args);
      if (exec->hadException()) {
        bindingsException = exec->exception();
        exec->clearException();
        return DOM::NodeFilter::FILTER_REJECT; // throw '1' isn't accept :-)
      }
      return result->toInteger(exec);
    }
  }

  return DOM::NodeFilter::FILTER_REJECT;
}
