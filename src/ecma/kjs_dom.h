// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003 Apple Computer, Inc.
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

#ifndef _KJS_DOM_H_
#define _KJS_DOM_H_

#include "xml/dom_nodeimpl.h"
#include "xml/dom_docimpl.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_xmlimpl.h"

#include "ecma/kjs_binding.h"


namespace KJS {

  class DOMNode : public DOMObject {
  public:
    // Build a DOMNode
    DOMNode(ExecState *exec,  DOM::NodeImpl* n);
    DOMNode(JSObject *proto, DOM::NodeImpl* n);
    ~DOMNode();
    virtual bool toBoolean(ExecState *) const;
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;

    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue* value, int attr);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    virtual JSValue* toPrimitive(ExecState *exec, JSType preferred = UndefinedType) const;
    virtual UString toString(ExecState *exec) const;
    void setListener(ExecState *exec, int eventId, JSValue* func) const;
    JSValue* getListener(int eventId) const;
    virtual void pushEventHandlerScope(ExecState *exec, ScopeChain &scope) const;

    enum { NodeName, NodeValue, NodeType, ParentNode, ParentElement,
           ChildNodes, FirstChild, LastChild, PreviousSibling, NextSibling, 
           Attributes, NamespaceURI, Prefix, LocalName, OwnerDocument, InsertBefore,
           ReplaceChild, RemoveChild, AppendChild, HasAttributes, HasChildNodes,
           CloneNode, Normalize, IsSupported, AddEventListener, RemoveEventListener,
           DispatchEvent, Contains, InsertAdjacentHTML,
           OnAbort, OnBlur, OnChange, OnClick, OnDblClick, OnDragDrop, OnError,
           OnFocus, OnKeyDown, OnKeyPress, OnKeyUp, OnLoad, OnMouseDown,
           OnMouseMove, OnMouseOut, OnMouseOver, OnMouseUp, OnMove, OnReset,
           OnResize, OnScroll, OnSelect, OnSubmit, OnUnload,
           OffsetLeft, OffsetTop, OffsetWidth, OffsetHeight, OffsetParent,
           ClientLeft, ClientTop, ClientWidth, ClientHeight, ScrollLeft, ScrollTop,
           ScrollWidth, ScrollHeight, SourceIndex, TextContent, CompareDocumentPosition };

    //### toNode? virtual
    DOM::NodeImpl* impl() const { return m_impl.get(); }
  protected:
    SharedPtr<DOM::NodeImpl> m_impl;
  };

  DEFINE_CONSTANT_TABLE(DOMNodeConstants)
  KJS_DEFINE_PROTOTYPE(DOMNodeProto)
  DEFINE_PSEUDO_CONSTRUCTOR(NodeConstructor)

  class DOMNodeList : public DOMObject {
  public:
    DOMNodeList(ExecState *, DOM::NodeListImpl* l);
    ~DOMNodeList();

    JSValue* indexGetter(ExecState *exec, unsigned index);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    virtual JSValue* callAsFunction(ExecState *exec, JSObject* thisObj, const List& args);
    virtual bool implementsCall() const { return true; }
    virtual bool isFunctionType() const { return false; }
    virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, PropertyMap::PropertyMode mode);

    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    virtual bool toBoolean(ExecState *) const { return true; }
    static const ClassInfo info;
    
    enum { Length, Item, NamedItem };

    DOM::NodeListImpl* impl() const { return m_impl.get(); }

    DOM::NodeImpl* getByName(const Identifier& name);
  private:
    SharedPtr<DOM::NodeListImpl> m_impl;

    static JSValue *nameGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
    static JSValue *lengthGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
  };

  DEFINE_PSEUDO_CONSTRUCTOR(NodeListPseudoCtor)

  class DOMDocument : public DOMNode {
  public:
    // Build a DOMDocument
    DOMDocument(ExecState *exec,  DOM::DocumentImpl* d);
    DOMDocument(JSObject *proto, DOM::DocumentImpl* d);

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue* value, int attr);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { DocType, Implementation, DocumentElement, CharacterSet, 
           // Functions
           CreateElement, CreateDocumentFragment, CreateTextNode, CreateComment,
           CreateCDATASection, CreateProcessingInstruction, CreateAttribute,
           CreateEntityReference, GetElementsByTagName, ImportNode, CreateElementNS,
           CreateAttributeNS, GetElementsByTagNameNS, GetElementById,
           CreateRange, CreateNodeIterator, CreateTreeWalker, DefaultView,
           CreateEvent, StyleSheets, GetOverrideStyle, Abort, Load, LoadXML,
           PreferredStylesheetSet, SelectedStylesheetSet, ReadyState, Async,
           GetElementsByClassName, Title, ExecCommand, QueryCommandEnabled,
           QueryCommandIndeterm, QueryCommandState, QueryCommandSupported,
           QueryCommandValue, QuerySelector, QuerySelectorAll,
           CreateExpression, CreateNSResolver, Evaluate
           };
    DOM::DocumentImpl* impl() { return static_cast<DOM::DocumentImpl*>(m_impl.get()); }
  };
  
  KJS_DEFINE_PROTOTYPE(DOMDocumentProto)

  DEFINE_PSEUDO_CONSTRUCTOR(DocumentPseudoCtor)

  class DOMDocumentFragment : public DOMNode {
  public:
    DOMDocumentFragment(ExecState* exec, DOM::DocumentFragmentImpl* i);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    enum { QuerySelector, QuerySelectorAll };
  };
  DEFINE_PSEUDO_CONSTRUCTOR(DocumentFragmentPseudoCtor)

  class DOMAttr : public DOMNode {
  public:
    DOMAttr(ExecState *exec, DOM::AttrImpl* a) : DOMNode(exec, a) { }
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue* value, int attr);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Name, Specified, ValueProperty, OwnerElement };
  };

  class DOMElement : public DOMNode {
  public:
    // Build a DOMElement
    DOMElement(ExecState *exec, DOM::ElementImpl* e);
    DOMElement(JSObject *proto, DOM::ElementImpl* e);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;

    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { TagName, Style, FirstElementChild, LastElementChild,
           PreviousElementSibling, NextElementSibling, ChildElementCount,
           GetAttribute, SetAttribute, RemoveAttribute, GetAttributeNode,
           SetAttributeNode, RemoveAttributeNode, GetElementsByTagName,
           GetAttributeNS, SetAttributeNS, RemoveAttributeNS, GetAttributeNodeNS,
           SetAttributeNodeNS, GetElementsByTagNameNS, HasAttribute, HasAttributeNS,
           GetElementsByClassName, Blur, Focus, QuerySelector, QuerySelectorAll,
           GetClientRects, GetBoundingClientRect };
  private:
#if 0
    static JSValue *attributeGetter(ExecState *exec, JSObject*, const Identifier&, const PropertySlot& slot);
#endif
  };

  DOM::AttrImpl    *toAttr   (JSValue *); // returns 0 if passed-in value is not a DOMAtt object
  DOM::ElementImpl *toElement(JSValue *); // returns 0 if passed-in value is not a DOMElement object

  KJS_DEFINE_PROTOTYPE(DOMElementProto)
  DEFINE_PSEUDO_CONSTRUCTOR(ElementPseudoCtor)

  class DOMDOMImplementation : public DOMObject {
  public:
    // Build a DOMDOMImplementation
    DOMDOMImplementation(ExecState *, DOM::DOMImplementationImpl* i);
    ~DOMDOMImplementation();
    // no put - all functions
    virtual const ClassInfo* classInfo() const { return &info; }
    virtual bool toBoolean(ExecState *) const { return true; }
    static const ClassInfo info;
    enum { HasFeature, CreateDocumentType, CreateDocument, CreateCSSStyleSheet, CreateHTMLDocument };
    
    DOM::DOMImplementationImpl* impl() const { return m_impl.get(); }
  private:
    SharedPtr<DOM::DOMImplementationImpl> m_impl;
  };

  class DOMDocumentType : public DOMNode {
  public:
    // Build a DOMDocumentType
    DOMDocumentType(ExecState *exec, DOM::DocumentTypeImpl* dt);

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Name, Entities, Notations, PublicId, SystemId, InternalSubset };
  };

  class DOMNamedNodeMap : public DOMObject {
  public:
    DOMNamedNodeMap(ExecState *, DOM::NamedNodeMapImpl* m);
    ~DOMNamedNodeMap();

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    virtual bool toBoolean(ExecState *) const { return true; }
    static const ClassInfo info;
    enum { GetNamedItem, SetNamedItem, RemoveNamedItem, Item, Length,
           GetNamedItemNS, SetNamedItemNS, RemoveNamedItemNS };
           
    DOM::NamedNodeMapImpl* impl() const { return m_impl.get(); }

    JSValue* indexGetter(ExecState *exec, unsigned index);
  private:
    static JSValue *lengthGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot& slot);
    SharedPtr<DOM::NamedNodeMapImpl> m_impl;
  };

  class DOMProcessingInstruction : public DOMNode {
  public:
    DOMProcessingInstruction(ExecState *exec, DOM::ProcessingInstructionImpl* pi) : DOMNode(exec, pi) { }

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Target, Data, Sheet };
  };

  class DOMNotation : public DOMNode {
  public:
    DOMNotation(ExecState *exec, DOM::NotationImpl* n) : DOMNode(exec, n) { }

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { PublicId, SystemId };
  };

  class DOMEntity : public DOMNode {
  public:
    DOMEntity(ExecState *exec, DOM::EntityImpl* e) : DOMNode(exec, e) { }
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { PublicId, SystemId, NotationName };
  };

  DEFINE_PSEUDO_CONSTRUCTOR(DOMExceptionPseudoCtor)

  class JSDOMException : public DOMObject {
  public:
    JSDOMException(ExecState* exec);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  bool checkNodeSecurity(ExecState *exec, const DOM::NodeImpl* n);
  JSValue* getEventTarget(ExecState* exec, DOM::EventTargetImpl* t);
  JSValue* getDOMNode(ExecState *exec, DOM::NodeImpl* n);
  JSValue* getDOMNamedNodeMap(ExecState *exec, DOM::NamedNodeMapImpl* m);
  JSValue* getDOMNodeList(ExecState *exec, DOM::NodeListImpl* l);
  JSValue* getDOMDOMImplementation(ExecState *exec, DOM::DOMImplementationImpl* i);
  JSObject *getDOMExceptionConstructor(ExecState *exec);

  // Internal class, used for the collection return by e.g. document.forms.myinput
  // when multiple nodes have the same name.
  class DOMNamedNodesCollection : public DOMObject {
  public:
    DOMNamedNodesCollection(ExecState *exec, const QList<SharedPtr<DOM::NodeImpl> >& nodes );
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    const QList<SharedPtr<DOM::NodeImpl> > nodes() const { return m_nodes; }
    enum { Length };

    JSValue* indexGetter(ExecState *exec, unsigned index);
  private:
    static JSValue *lengthGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot& slot);
    QList<SharedPtr<DOM::NodeImpl> > m_nodes;
  };

  class DOMCharacterData : public DOMNode {
  public:
    // Build a DOMCharacterData
    DOMCharacterData(ExecState *exec, DOM::CharacterDataImpl* d);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    DOM::CharacterDataImpl* impl() const { return static_cast<DOM::CharacterDataImpl*>(m_impl.get()); }
    enum { Data, Length,
           SubstringData, AppendData, InsertData, DeleteData, ReplaceData };
  };

  class DOMText : public DOMCharacterData {
  public:
    DOMText(ExecState *exec, DOM::TextImpl* t);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState* exec, int token) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    DOM::TextImpl* impl() const { return static_cast<DOM::TextImpl*>(m_impl.get()); }
    enum { SplitText, WholeText, ReplaceWholeText };
  };

  class DOMComment : public DOMCharacterData {
  public:
    DOMComment(ExecState *exec, DOM::CommentImpl* t);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    DOM::CommentImpl* impl() const { return static_cast<DOM::CommentImpl*>(m_impl.get()); }
  };

} // namespace

#endif
