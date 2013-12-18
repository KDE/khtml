// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
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

#ifndef _KJS_CSS_H_
#define _KJS_CSS_H_

#include <dom/dom_node.h>
#include <dom/dom_doc.h>
#include <kjs/object.h>
#include "css/css_base.h"
#include "css/css_ruleimpl.h"
#include "css/css_stylesheetimpl.h"
#include "css/css_valueimpl.h"
#include "kjs_binding.h"

namespace KJS {

  class DOMCSSStyleDeclaration : public DOMObject {
  public:
    DOMCSSStyleDeclaration(ExecState *exec, DOM::CSSStyleDeclarationImpl* s);
    virtual ~DOMCSSStyleDeclaration();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, PropertyMap::PropertyMode mode);
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None);
    JSValue *getValueProperty(ExecState *exec, int token);

    virtual const ClassInfo *classInfo() const { return &info; }
    static const ClassInfo info;
    enum { CssText, Length, ParentRule,
           GetPropertyValue, GetPropertyCSSValue, RemoveProperty, GetPropertyPriority,
           SetProperty, Item };

    DOM::CSSStyleDeclarationImpl *impl() const { return m_impl.get(); }

    JSValue *indexGetter(ExecState* exec, unsigned index);
  private:
    SharedPtr<DOM::CSSStyleDeclarationImpl> m_impl;
  };

  DEFINE_PSEUDO_CONSTRUCTOR(CSSStyleDeclarationPseudoCtor)
  JSValue* getDOMCSSStyleDeclaration(ExecState *exec, DOM::CSSStyleDeclarationImpl* n);

  class DOMStyleSheet : public DOMObject {
  public:
    // Build a DOMStyleSheet
    DOMStyleSheet(ExecState *, DOM::StyleSheetImpl* ss);
    virtual ~DOMStyleSheet();

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    virtual bool toBoolean(ExecState *) const { return true; }
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Type, Disabled, OwnerNode, ParentStyleSheet, Href, Title, Media };
  protected:
    SharedPtr<DOM::StyleSheetImpl> m_impl;
  };
  DEFINE_PSEUDO_CONSTRUCTOR(DOMStyleSheetPseudoCtor)

  JSValue* getDOMStyleSheet(ExecState *exec, DOM::StyleSheetImpl* ss);

  class DOMStyleSheetList : public DOMObject {
  public:
    DOMStyleSheetList(ExecState *, DOM::StyleSheetListImpl* ssl, DOM::DocumentImpl* doc);
    virtual ~DOMStyleSheetList();

    JSValue *getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    virtual JSValue* callAsFunction(ExecState *exec, JSObject* thisObj, const List &args);
    virtual bool implementsCall() const { return true; }
    virtual bool isFunctionType() const { return false; }
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    virtual bool toBoolean(ExecState* ) const { return true; }
    static const ClassInfo info;

    DOM::StyleSheetListImpl* impl() const { return m_impl.get(); }
    enum { Item, Length };
    JSValue *indexGetter(ExecState* exec, unsigned index);
  private:
    static JSValue *nameGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);

    SharedPtr<DOM::StyleSheetListImpl> m_impl;
    SharedPtr<DOM::DocumentImpl>       m_doc;
  };

  // The document is only used for get-stylesheet-by-name (make optional if necessary)
  JSValue* getDOMStyleSheetList(ExecState *exec, DOM::StyleSheetListImpl* ss, DOM::DocumentImpl* doc);

  class DOMMediaList : public DOMObject {
  public:
    DOMMediaList(ExecState *, DOM::MediaListImpl* ml);
    virtual ~DOMMediaList();

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    virtual const ClassInfo* classInfo() const { return &info; }
    virtual bool toBoolean(ExecState* ) const { return true; }
    static const ClassInfo info;
    enum { MediaText, Length,
           Item, DeleteMedium, AppendMedium };
    DOM::MediaListImpl* impl() const { return m_impl.get(); }
    JSValue *indexGetter(ExecState* exec, unsigned index);
  private:
    SharedPtr<DOM::MediaListImpl> m_impl;
  };

  JSValue* getDOMMediaList(ExecState *exec, DOM::MediaListImpl* ss);

  class DOMCSSStyleSheet : public DOMStyleSheet {
  public:
    DOMCSSStyleSheet(ExecState *exec, DOM::CSSStyleSheetImpl* ss);
    virtual ~DOMCSSStyleSheet();
    JSValue* getValueProperty(ExecState *exec, int token);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { OwnerRule, CssRules, Rules,
           InsertRule, DeleteRule, AddRule, RemoveRule };
    DOM::CSSStyleSheetImpl* impl() const { return static_cast<DOM::CSSStyleSheetImpl*>(m_impl.get()); }
  };

  class DOMCSSRuleList : public DOMObject {
  public:
    DOMCSSRuleList(ExecState *, DOM::CSSRuleListImpl* rl);
    virtual ~DOMCSSRuleList();
    JSValue *getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Item, Length };
    DOM::CSSRuleListImpl* impl() const { return m_impl.get(); }
    JSValue *indexGetter(ExecState* exec, unsigned index);
  protected:
    SharedPtr<DOM::CSSRuleListImpl> m_impl;
  private:
  };

  JSValue* getDOMCSSRuleList(ExecState *exec, DOM::CSSRuleListImpl* rl);

  class DOMCSSRule : public DOMObject {
  public:
    DOMCSSRule(ExecState *, DOM::CSSRuleImpl* r);
    virtual ~DOMCSSRule();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue* value, int attr);
    virtual const ClassInfo* classInfo() const;
    static const ClassInfo info;
    static const ClassInfo style_info, media_info, fontface_info, page_info, import_info, charset_info, namespace_info;
    enum { ParentStyleSheet, Type, CssText, ParentRule,
           Style_SelectorText, Style_Style,
           Media_Media, Media_InsertRule, Media_DeleteRule, Media_CssRules,
           FontFace_Style, Page_SelectorText, Page_Style,
           Import_Href, Import_Media, Import_StyleSheet, Charset_Encoding, 
           Namespace_NamespaceURI, Namespace_Prefix };
    DOM::CSSRuleImpl* impl() const { return m_impl.get(); }
    JSValue *indexGetter(ExecState* exec, unsigned index);
  protected:
    SharedPtr<DOM::CSSRuleImpl> m_impl;
  };

  JSValue* getDOMCSSRule(ExecState *exec, DOM::CSSRuleImpl* r);

  // Constructor for CSSRule - currently only used for some global values
  class CSSRuleConstructor : public DOMObject {
  public:
    CSSRuleConstructor(ExecState *);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { UNKNOWN_RULE, STYLE_RULE, CHARSET_RULE, IMPORT_RULE, MEDIA_RULE, FONT_FACE_RULE, PAGE_RULE };
  };

  JSValue* getCSSRuleConstructor(ExecState *exec);

  class DOMCSSValue : public DOMObject {
  public:
    DOMCSSValue(ExecState *, DOM::CSSValueImpl* v);
    virtual ~DOMCSSValue();
    JSValue* getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { CssText, CssValueType };
  protected:
    SharedPtr<DOM::CSSValueImpl> m_impl;
  };

  JSValue* getDOMCSSValue(ExecState *exec, DOM::CSSValueImpl* v);

  // Constructor for CSSValue - currently only used for some global values
  class CSSValueConstructor : public DOMObject {
  public:
    CSSValueConstructor(ExecState *exec);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { CSS_VALUE_LIST, CSS_PRIMITIVE_VALUE, CSS_CUSTOM, CSS_INHERIT };
  };

  JSValue* getCSSValueConstructor(ExecState *exec);

  class DOMCSSPrimitiveValue : public DOMCSSValue {
  public:
    DOMCSSPrimitiveValue(ExecState *exec, DOM::CSSPrimitiveValueImpl* v);
    JSValue *getValueProperty(ExecState *exec, int token);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    DOM::CSSPrimitiveValueImpl* impl() const { return static_cast<DOM::CSSPrimitiveValueImpl*>(m_impl.get()); }
    enum { PrimitiveType, SetFloatValue, GetFloatValue, SetStringValue, GetStringValue,
           GetCounterValue, GetRectValue, GetRGBColorValue };
  };

  // Constructor for CSSPrimitiveValue - currently only used for some global values
  class CSSPrimitiveValueConstructor : public CSSValueConstructor {
  public:
    CSSPrimitiveValueConstructor(ExecState *exec) : CSSValueConstructor(exec) { }
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  JSValue* getCSSPrimitiveValueConstructor(ExecState *exec);

  class DOMCSSValueList : public DOMCSSValue {
  public:
    DOMCSSValueList(ExecState *exec, DOM::CSSValueListImpl* v);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Item, Length };
    DOM::CSSValueListImpl* impl() const { return static_cast<DOM::CSSValueListImpl*>(m_impl.get()); }
    JSValue *indexGetter(ExecState* exec, unsigned index);
  };

  class DOMRGBColor : public DOMObject {
  public:
    DOMRGBColor(ExecState* exec, QRgb color);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Red, Green, Blue };
  private:
    QRgb m_color;
  };

  JSValue *getDOMRGBColor(ExecState *exec, unsigned color);

  class DOMRect : public DOMObject {
  public:
    DOMRect(ExecState *, DOM::RectImpl *r);
    ~DOMRect();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Top, Right, Bottom, Left };
  private:
    SharedPtr<DOM::RectImpl> m_impl;
  };

  JSValue *getDOMRect(ExecState *exec, DOM::RectImpl *r);

  class DOMCounter : public DOMObject {
  public:
    DOMCounter(ExecState *, DOM::CounterImpl *c);
    ~DOMCounter();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { identifier, listStyle, separator };
  protected:
    SharedPtr<DOM::CounterImpl> m_impl;
  };

  JSValue *getDOMCounter(ExecState *exec, DOM::CounterImpl *c);
} // namespace

#endif
