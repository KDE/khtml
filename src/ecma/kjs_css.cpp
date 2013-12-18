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

#include "kjs_css.h"
#include "kjs_css.lut.h"
#include "kjs_binding.h"

#include "html/html_headimpl.h" // for HTMLStyleElementImpl

#include "dom/css_value.h"
#include "dom/css_rule.h"

#include "css/css_base.h"
#include "css/css_ruleimpl.h"
#include "css/css_renderstyledeclarationimpl.h"
#include "css/css_stylesheetimpl.h"
#include "css/css_valueimpl.h"
#include "css/cssproperties.h"

#include "kjs_dom.h"

using DOM::CSSCharsetRuleImpl;
using DOM::CSSFontFaceRuleImpl;
using DOM::CSSImportRuleImpl;
using DOM::CSSMediaRuleImpl;
using DOM::CSSPageRuleImpl;
using DOM::CSSPrimitiveValue;
using DOM::CSSPrimitiveValueImpl;
using DOM::CSSRule;
using DOM::CSSRuleImpl;
using DOM::CSSRuleListImpl;
using DOM::CSSStyleDeclarationImpl;
using DOM::CSSStyleRuleImpl;
using DOM::CSSStyleSheetImpl;
using DOM::CSSValue;
using DOM::CSSValueImpl;
using DOM::CSSValueListImpl;
using DOM::CounterImpl;
using DOM::DocumentImpl;
using DOM::DOMString;
using DOM::ElementImpl;
using DOM::HTMLStyleElementImpl;
using DOM::MediaListImpl;
using DOM::RectImpl;
using DOM::StyleSheetImpl;
using DOM::StyleSheetListImpl;

#include <QDebug>
#include <QtCore/QList>

namespace KJS {

static QString cssPropertyName( const Identifier &p, bool* hadPixelPrefix )
{
    // The point here is to provide compatibility with IE
    // syntax for accessing properties, which camel-cases them
    // and can add prefixes to produce things like pixelFoo
    QString prop = p.qstring();
    for (int i = prop.length() - 1; i >= 0; --i) {
        char c = prop[i].toLatin1();
        if ( c >= 'A' && c <= 'Z' )
            prop.insert( i, '-' );
    }

    prop = prop.toLower();
    *hadPixelPrefix = false;

    if (prop.startsWith(QLatin1String("css-"))) {
        prop = prop.mid(4);
    } else if (prop.startsWith(QLatin1String("pixel-"))) {
        prop = prop.mid(6);
        *hadPixelPrefix = true;
    } else if (prop.startsWith(QLatin1String("pos-"))) {
        prop = prop.mid(4);
        *hadPixelPrefix = true;
    }

    return prop;
}

static int cssPropertyId( const QString& p ) {
  return DOM::getPropertyID(p.toLatin1().constData(), p.length());
}

static bool isCSSPropertyName(const Identifier &JSPropertyName)
{
    bool dummy;
    QString p = cssPropertyName(JSPropertyName, &dummy);
    return cssPropertyId(p) != 0;
}


/*
@begin DOMCSSStyleDeclarationProtoTable 7
  getPropertyValue	DOMCSSStyleDeclaration::GetPropertyValue	DontDelete|Function 1
  getPropertyCSSValue	DOMCSSStyleDeclaration::GetPropertyCSSValue	DontDelete|Function 1
  removeProperty	DOMCSSStyleDeclaration::RemoveProperty		DontDelete|Function 1
  getPropertyPriority	DOMCSSStyleDeclaration::GetPropertyPriority	DontDelete|Function 1
  setProperty		DOMCSSStyleDeclaration::SetProperty		DontDelete|Function 3
  item			DOMCSSStyleDeclaration::Item			DontDelete|Function 1
# IE names for it (#36063)
  getAttribute          DOMCSSStyleDeclaration::GetPropertyValue	DontDelete|Function 1
  removeAttribute       DOMCSSStyleDeclaration::RemoveProperty		DontDelete|Function 1
  setAttribute		DOMCSSStyleDeclaration::SetProperty		DontDelete|Function 3
@end
@begin DOMCSSStyleDeclarationTable 3
  cssText		DOMCSSStyleDeclaration::CssText		DontDelete
  length		DOMCSSStyleDeclaration::Length		DontDelete|ReadOnly
  parentRule		DOMCSSStyleDeclaration::ParentRule	DontDelete|ReadOnly
@end
*/
KJS_DEFINE_PROTOTYPE(DOMCSSStyleDeclarationProto)
KJS_IMPLEMENT_PROTOFUNC(DOMCSSStyleDeclarationProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMCSSStyleDeclaration", DOMCSSStyleDeclarationProto, DOMCSSStyleDeclarationProtoFunc, ObjectPrototype)

IMPLEMENT_PSEUDO_CONSTRUCTOR(CSSStyleDeclarationPseudoCtor, "DOMCSSStyleDeclaration",DOMCSSStyleDeclarationProto)

const ClassInfo DOMCSSStyleDeclaration::info = { "CSSStyleDeclaration", 0, &DOMCSSStyleDeclarationTable, 0 };

DOMCSSStyleDeclaration::DOMCSSStyleDeclaration(ExecState *exec, DOM::CSSStyleDeclarationImpl* s)
  : DOMObject(), m_impl(s)
{
  setPrototype(DOMCSSStyleDeclarationProto::self(exec));
}

DOMCSSStyleDeclaration::~DOMCSSStyleDeclaration()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue* DOMCSSStyleDeclaration::getValueProperty(ExecState *exec, int token)
{
    //### null decls?
    switch (token) {
    case CssText:
        return jsString(m_impl->cssText());
    case Length:
        return jsNumber(m_impl->length());
    case ParentRule:
        return getDOMCSSRule(exec, m_impl->parentRule());
    }

    assert(0);
    return jsUndefined();
}

JSValue *DOMCSSStyleDeclaration::indexGetter(ExecState* , unsigned index)
{
  return jsString(m_impl->item(index));
}

bool DOMCSSStyleDeclaration::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMCSSStyleDeclaration::getOwnPropertySlot " << propertyName.qstring();
#endif

  if (getStaticOwnValueSlot(&DOMCSSStyleDeclarationTable, this, propertyName, slot))
    return true;

  //Check whether it's an index
  if (getIndexSlot(this, propertyName, slot))
    return true;

  if (isCSSPropertyName(propertyName)) {
    // Set up pixelOrPos boolean to handle the fact that
    // pixelTop returns "CSS Top" as number value in unit pixels
    // posTop returns "CSS top" as number value in unit pixels _if_ its a
    // positioned element. if it is not a positioned element, return 0
    // from MSIE documentation ### IMPLEMENT THAT (Dirk)
    bool asNumber;
    DOMString p   = cssPropertyName(propertyName, &asNumber);

    if (asNumber) {
      CSSValueImpl *v = m_impl->getPropertyCSSValue(p);
      if (v && v->cssValueType() == DOM::CSSValue::CSS_PRIMITIVE_VALUE)
         //### FIXME: should this not set exception when type is wrong, or convert?
        return getImmediateValueSlot(this,
                  jsNumber(static_cast<CSSPrimitiveValueImpl*>(v)->floatValue(DOM::CSSPrimitiveValue::CSS_PX)), slot);
    }

    DOM::DOMString str = m_impl->getPropertyValue(p);

    // We want to return at least an empty string here --- see #152791
    return getImmediateValueSlot(this, jsString(str), slot);
  }

  return DOMObject::getOwnPropertySlot(exec, propertyName, slot);
}

void DOMCSSStyleDeclaration::getOwnPropertyNames(ExecState* exec, PropertyNameArray& arr, PropertyMap::PropertyMode mode)
{
    DOMObject::getOwnPropertyNames(exec, arr, mode);

    // Add in all properties we support.
    for (int p = 1; p < CSS_PROP_TOTAL; ++p) {
        QString dashName = getPropertyName(p).string();
        QString camelName;

        bool capitalize = false;
        for (int c = 0; c < dashName.length(); ++c) {
            if (dashName[c] == QLatin1Char('-')) {
                capitalize = true;
            } else {
                camelName += capitalize ? dashName[c].toUpper() : dashName[c];
                capitalize = false;
            }
        } // char

        arr.add(KJS::Identifier(camelName));
    } // prop
}

void DOMCSSStyleDeclaration::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr )
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMCSSStyleDeclaration::put " << propertyName.qstring();
#endif
  DOMExceptionTranslator exception(exec);
  CSSStyleDeclarationImpl &styleDecl = *m_impl;

  if (propertyName == "cssText") {
    styleDecl.setCssText(valueToStringWithNullCheck(exec, value));
  }
  else {
    bool pxSuffix;
    QString prop = cssPropertyName(propertyName, &pxSuffix);
    QString propvalue = valueToStringWithNullCheck(exec, value).string();

    if (pxSuffix)
      propvalue += QLatin1String("px");
#ifdef KJS_VERBOSE
    qDebug() << "DOMCSSStyleDeclaration: prop=" << prop << " propvalue=" << propvalue;
#endif
    // Look whether the property is known. In that case add it as a CSS property.
    if (int pId = cssPropertyId(prop)) {
      if (propvalue.isEmpty())
        styleDecl.removeProperty(pId);
      else {
        int important = propvalue.indexOf("!important", 0, Qt::CaseInsensitive);
        if (important == -1) {
            styleDecl.setProperty(pId, DOM::DOMString(propvalue), false /*important*/, exception);
        } else
            styleDecl.setProperty(pId, DOM::DOMString(propvalue.left(important - 1)), true /*important*/, exception);
      }
    }
    else
      // otherwise add it as a JS property
      DOMObject::put( exec, propertyName, value, attr );
  }
}

JSValue *DOMCSSStyleDeclarationProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMCSSStyleDeclaration, thisObj );
  CSSStyleDeclarationImpl& styleDecl = *static_cast<DOMCSSStyleDeclaration*>(thisObj)->impl();

  DOM::DOMString s = args[0]->toString(exec).domString();

  switch (id) {
    case DOMCSSStyleDeclaration::GetPropertyValue:
      return jsString(styleDecl.getPropertyValue(s));
    case DOMCSSStyleDeclaration::GetPropertyCSSValue:
      return getDOMCSSValue(exec,styleDecl.getPropertyCSSValue(s));
    case DOMCSSStyleDeclaration::RemoveProperty:
      return jsString(styleDecl.removeProperty(s));
    case DOMCSSStyleDeclaration::GetPropertyPriority:
      return jsString(styleDecl.getPropertyPriority(s));
    case DOMCSSStyleDeclaration::SetProperty:
      styleDecl.setProperty(args[0]->toString(exec).domString(),
                            args[1]->toString(exec).domString(),
                            args[2]->toString(exec).domString());
      return jsUndefined();
    case DOMCSSStyleDeclaration::Item:
      return jsString(styleDecl.item(args[0]->toInteger(exec)));
    default:
      return jsUndefined();
  }
}

JSValue *getDOMCSSStyleDeclaration(ExecState *exec, CSSStyleDeclarationImpl *s)
{
  return cacheDOMObject<CSSStyleDeclarationImpl, DOMCSSStyleDeclaration>(exec, s);
}

// -------------------------------------------------------------------------

const ClassInfo DOMStyleSheet::info = { "StyleSheet", 0, &DOMStyleSheetTable, 0 };
/*
@begin DOMStyleSheetTable 7
  type		DOMStyleSheet::Type		DontDelete|ReadOnly
  disabled	DOMStyleSheet::Disabled		DontDelete
  ownerNode	DOMStyleSheet::OwnerNode	DontDelete|ReadOnly
  parentStyleSheet DOMStyleSheet::ParentStyleSheet	DontDelete|ReadOnly
  href		DOMStyleSheet::Href		DontDelete|ReadOnly
  title		DOMStyleSheet::Title		DontDelete|ReadOnly
  media		DOMStyleSheet::Media		DontDelete|ReadOnly
@end
*/
KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("StyleSheet", DOMStyleSheetProto, ObjectPrototype)
IMPLEMENT_PSEUDO_CONSTRUCTOR(DOMStyleSheetPseudoCtor, "StyleSheet", DOMStyleSheetProto)

DOMStyleSheet::DOMStyleSheet(ExecState* exec, DOM::StyleSheetImpl* ss)
  : m_impl(ss)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

DOMStyleSheet::~DOMStyleSheet()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

bool DOMStyleSheet::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMStyleSheet, DOMObject>(exec, &DOMStyleSheetTable, this, propertyName, slot);
}

JSValue *DOMStyleSheet::getValueProperty(ExecState *exec, int token) const
{
  StyleSheetImpl &styleSheet = *m_impl;
  switch (token) {
  case Type:
    return jsString(styleSheet.type());
  case Disabled:
    return jsBoolean(styleSheet.disabled());
  case OwnerNode:
    return getDOMNode(exec,styleSheet.ownerNode());
  case ParentStyleSheet:
    return getDOMStyleSheet(exec,styleSheet.parentStyleSheet());
  case Href:
    return getStringOrNull(styleSheet.href());
  case Title:
    return jsString(styleSheet.title());
  case Media:
    return getDOMMediaList(exec, styleSheet.media());
  }
  return 0;
}

void DOMStyleSheet::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr)
{
  StyleSheetImpl &styleSheet = *m_impl;
  if (propertyName == "disabled") {
    styleSheet.setDisabled(value->toBoolean(exec));
  }
  else
    DOMObject::put(exec, propertyName, value, attr);
}

JSValue *getDOMStyleSheet(ExecState *exec, DOM::StyleSheetImpl* ss)
{
  if (!ss)
    return jsNull();

  DOMObject *ret;
  ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());
  if ((ret = interp->getDOMObject(ss)))
    return ret;
  else {
    if (ss->isCSSStyleSheet()) {
      CSSStyleSheetImpl* cs = static_cast<CSSStyleSheetImpl*>(ss);
      ret = new DOMCSSStyleSheet(exec,cs);
    }
    else
      ret = new DOMStyleSheet(exec,ss);
    interp->putDOMObject(ss,ret);
    return ret;
  }
}

// -------------------------------------------------------------------------

const ClassInfo DOMStyleSheetList::info = { "StyleSheetList", 0, &DOMStyleSheetListTable, 0 };

/*
@begin DOMStyleSheetListTable 2
  length	DOMStyleSheetList::Length	DontDelete|ReadOnly
  item		DOMStyleSheetList::Item		DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(DOMStyleSheetListFunc) // not really a proto, but doesn't matter

DOMStyleSheetList::DOMStyleSheetList(ExecState *exec, DOM::StyleSheetListImpl* ssl, DOM::DocumentImpl* doc)
  : m_impl(ssl), m_doc(doc)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

DOMStyleSheetList::~DOMStyleSheetList()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue *DOMStyleSheetList::getValueProperty(ExecState*, int token) const
{
    switch(token) {
    case Length:
      return jsNumber(m_impl->length());
    default:
      assert(0);
      return jsUndefined();
    }
}

JSValue *DOMStyleSheetList::indexGetter(ExecState *exec, unsigned index)
{
  return getDOMStyleSheet(exec, m_impl->item(index));
}

JSValue *DOMStyleSheetList::nameGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
  DOMStyleSheetList *thisObj = static_cast<DOMStyleSheetList *>(slot.slotBase());
  ElementImpl *element = thisObj->m_doc->getElementById(propertyName.domString());
  assert(element->id() == ID_STYLE); //Should be from existence check
  return getDOMStyleSheet(exec, static_cast<HTMLStyleElementImpl *>(element)->sheet());
}

bool DOMStyleSheetList::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMStyleSheetList::getOwnPropertySlot " << propertyName.qstring();
#endif
  if (getStaticOwnPropertySlot<DOMStyleSheetListFunc, DOMStyleSheetList>(&DOMStyleSheetListTable, this, propertyName, slot))
    return true;

  StyleSheetListImpl &styleSheetList = *m_impl;

  // Retrieve stylesheet by index
  if (getIndexSlot(this, styleSheetList, propertyName, slot))
    return true;

  // IE also supports retrieving a stylesheet by name, using the name/id of the <style> tag
  // (this is consistent with all the other collections)
  // ### Bad implementation because returns a single element (are IDs always unique?)
  // and doesn't look for name attribute (see implementation above).
  // But unicity of stylesheet ids is good practice anyway ;)
  ElementImpl *element = m_doc->getElementById(propertyName.domString());
  if (element && element->id() == ID_STYLE) {
    slot.setCustom(this, nameGetter);
    return true;
  }

  return DOMObject::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue *DOMStyleSheetList::callAsFunction(ExecState *exec, JSObject * /*thisObj*/, const List &args)
{
  if (args.size() == 1) {
    // support for styleSheets(<index>) and styleSheets(<name>)
    return get( exec, Identifier(args[0]->toString(exec)) );
  }
  return jsUndefined();
}

JSValue *getDOMStyleSheetList(ExecState *exec, DOM::StyleSheetListImpl* ssl, DOM::DocumentImpl* doc)
{
  // Can't use the cacheDOMObject macro because of the doc argument
  DOMObject *ret;
  if (!ssl)
    return jsNull();
  ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());
  if ((ret = interp->getDOMObject(ssl)))
    return ret;
  else {
    ret = new DOMStyleSheetList(exec, ssl, doc);
    interp->putDOMObject(ssl,ret);
    return ret;
  }
}

JSValue *DOMStyleSheetListFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMStyleSheetList, thisObj );
  DOM::StyleSheetListImpl* styleSheetList = static_cast<DOMStyleSheetList *>(thisObj)->impl();
  if (id == DOMStyleSheetList::Item)
    return getDOMStyleSheet(exec, styleSheetList->item(args[0]->toInteger(exec)));
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMMediaList::info = { "MediaList", 0, &DOMMediaListTable, 0 };

/*
@begin DOMMediaListTable 2
  mediaText	DOMMediaList::MediaText		DontDelete|ReadOnly
  length	DOMMediaList::Length		DontDelete|ReadOnly
@end
@begin DOMMediaListProtoTable 3
  item		DOMMediaList::Item		DontDelete|Function 1
  deleteMedium	DOMMediaList::DeleteMedium	DontDelete|Function 1
  appendMedium	DOMMediaList::AppendMedium	DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(DOMMediaListProto)
KJS_IMPLEMENT_PROTOFUNC(DOMMediaListProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMMediaList", DOMMediaListProto, DOMMediaListProtoFunc, ObjectPrototype)

DOMMediaList::DOMMediaList(ExecState *exec, DOM::MediaListImpl* ml)
  : m_impl(ml)
{
  setPrototype(DOMMediaListProto::self(exec));
}

DOMMediaList::~DOMMediaList()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue* DOMMediaList::getValueProperty(ExecState*, int token) const
{
  const MediaListImpl& mediaList = *m_impl;
  switch (token)
  {
  case MediaText:
    return jsString(mediaList.mediaText());
  case Length:
    return jsNumber(mediaList.length());
  default:
    assert(0);
    return jsUndefined();
  }
}

JSValue *DOMMediaList::indexGetter(ExecState*, unsigned index)
{
  return jsString(m_impl->item(index));
}

bool DOMMediaList::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  if (getStaticOwnValueSlot(&DOMMediaListTable, this, propertyName, slot))
    return true;

  if (getIndexSlot(this, *m_impl, propertyName, slot))
    return true;

  return DOMObject::getOwnPropertySlot(exec, propertyName, slot);
}

void DOMMediaList::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr)
{
  if (propertyName == "mediaText") {
    DOMExceptionTranslator exception(exec);
    m_impl->setMediaText(value->toString(exec).domString(), exception);
  } else
    DOMObject::put(exec, propertyName, value, attr);
}

JSValue *getDOMMediaList(ExecState *exec, DOM::MediaListImpl* ml)
{
  return cacheDOMObject<DOM::MediaListImpl, KJS::DOMMediaList>(exec, ml);
}

JSValue *DOMMediaListProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMMediaList, thisObj );
  DOM::MediaListImpl& mediaList = *static_cast<DOMMediaList *>(thisObj)->impl();
  DOMExceptionTranslator exception(exec);
  switch (id) {
    case DOMMediaList::Item:
      return jsString(mediaList.item(args[0]->toInteger(exec)));
    case DOMMediaList::DeleteMedium:
      mediaList.deleteMedium(args[0]->toString(exec).domString(), exception);
      return jsUndefined();
    case DOMMediaList::AppendMedium:
      mediaList.appendMedium(args[0]->toString(exec).domString(), exception);
      return jsUndefined();
    default:
      return jsUndefined();
  }
}

// -------------------------------------------------------------------------

const ClassInfo DOMCSSStyleSheet::info = { "CSSStyleSheet", 0, &DOMCSSStyleSheetTable, 0 };

/*
@begin DOMCSSStyleSheetTable 2
  ownerRule	DOMCSSStyleSheet::OwnerRule	DontDelete|ReadOnly
  cssRules	DOMCSSStyleSheet::CssRules	DontDelete|ReadOnly
# MSIE extension
  rules		DOMCSSStyleSheet::Rules		DontDelete|ReadOnly
@end
@begin DOMCSSStyleSheetProtoTable 2
  insertRule	DOMCSSStyleSheet::InsertRule	DontDelete|Function 2
  deleteRule	DOMCSSStyleSheet::DeleteRule	DontDelete|Function 1
# IE extensions
  addRule	DOMCSSStyleSheet::AddRule	DontDelete|Function 3
  removeRule	DOMCSSStyleSheet::RemoveRule	DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(DOMCSSStyleSheetProto)
KJS_IMPLEMENT_PROTOFUNC(DOMCSSStyleSheetProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMCSSStyleSheet",DOMCSSStyleSheetProto,DOMCSSStyleSheetProtoFunc, DOMStyleSheetProto)

DOMCSSStyleSheet::DOMCSSStyleSheet(ExecState *exec, DOM::CSSStyleSheetImpl* ss): DOMStyleSheet(exec, ss)
{
  setPrototype(DOMCSSStyleSheetProto::self(exec));
}

DOMCSSStyleSheet::~DOMCSSStyleSheet()
{}

JSValue* DOMCSSStyleSheet::getValueProperty(ExecState *exec, int token)
{
  CSSStyleSheetImpl& cssStyleSheet = *impl();
  // MSIE does not list the charset rules in its proprietary extension
  bool omitCharsetRules = true;
  switch (token) {
    case OwnerRule:
      return getDOMCSSRule(exec,cssStyleSheet.ownerRule());
    case CssRules:
        omitCharsetRules = false;
        // nobreak
    case Rules: {
        return getDOMCSSRuleList(exec, cssStyleSheet.cssRules(omitCharsetRules));
    }
    default:
      assert(0);
      return jsUndefined();
  }
}


bool DOMCSSStyleSheet::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMCSSStyleSheet, DOMStyleSheet>(exec, &DOMCSSStyleSheetTable, this, propertyName, slot);
}

JSValue *DOMCSSStyleSheetProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMCSSStyleSheet, thisObj );
  DOM::CSSStyleSheetImpl& styleSheet = *static_cast<DOMCSSStyleSheet *>(thisObj)->impl();
  DOMExceptionTranslator exception(exec);

  switch (id) {
    case DOMCSSStyleSheet::InsertRule:
      return jsNumber(styleSheet.insertRule(args[0]->toString(exec).domString(),(long unsigned int)args[1]->toInteger(exec), exception));
    case DOMCSSStyleSheet::DeleteRule:
      styleSheet.deleteRule(args[0]->toInteger(exec), exception);
      return jsUndefined();
    // IE extensions
    case DOMCSSStyleSheet::AddRule: {
      //Unpassed/-1 means append. Since insertRule is picky (throws exceptions)
      //we adjust it to the desired length
      unsigned long index  = args[2]->toInteger(exec);
      unsigned long length = styleSheet.length();
      if (args[2]->type() == UndefinedType) index = length;
      if (index > length)                   index = length;
      DOM::DOMString str = args[0]->toString(exec).domString() + " { " + args[1]->toString(exec).domString() + " } ";
      return jsNumber(styleSheet.insertRule(str, index, exception));
    }
    case DOMCSSStyleSheet::RemoveRule: {
      int index = args.size() > 0 ? args[0]->toInteger(exec) : 0 /*first one*/;
      styleSheet.deleteRule(index, exception);
      return jsUndefined();
    }
    default:
      return jsUndefined();
  }
}

// -------------------------------------------------------------------------

const ClassInfo DOMCSSRuleList::info = { "CSSRuleList", 0, &DOMCSSRuleListTable, 0 };
/*
@begin DOMCSSRuleListTable 3
  length		DOMCSSRuleList::Length		DontDelete|ReadOnly
  item			DOMCSSRuleList::Item		DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(DOMCSSRuleListFunc) // not really a proto, but doesn't matter

DOMCSSRuleList::DOMCSSRuleList(ExecState* exec, DOM::CSSRuleListImpl* rl)
  : m_impl(rl)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

DOMCSSRuleList::~DOMCSSRuleList()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue *DOMCSSRuleList::getValueProperty(ExecState*, int token) const
{
  switch (token) {
  case Length:
    return jsNumber(m_impl->length());
  default:
    assert(0);
    return jsUndefined();
  }
}

JSValue *DOMCSSRuleList::indexGetter(ExecState* exec, unsigned index)
{
  return getDOMCSSRule(exec, m_impl->item(index));
}

bool DOMCSSRuleList::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  if (getStaticOwnPropertySlot<DOMCSSRuleListFunc, DOMCSSRuleList>(&DOMCSSRuleListTable, this, propertyName, slot))
    return true;

  //Check whether it's an index
  //CSSRuleListImpl &cssRuleList = *m_impl;

  if (getIndexSlot(this, *m_impl, propertyName, slot))
    return true;

  return DOMObject::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue *DOMCSSRuleListFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMCSSRuleList, thisObj );
  DOM::CSSRuleListImpl& cssRuleList = *static_cast<DOMCSSRuleList *>(thisObj)->impl();
  switch (id) {
    case DOMCSSRuleList::Item:
      return getDOMCSSRule(exec,cssRuleList.item(args[0]->toInteger(exec)));
    default:
      return jsUndefined();
  }
}

JSValue *getDOMCSSRuleList(ExecState *exec, DOM::CSSRuleListImpl* rl)
{
  return cacheDOMObject<DOM::CSSRuleListImpl, KJS::DOMCSSRuleList>(exec, rl);
}

// -------------------------------------------------------------------------

KJS_IMPLEMENT_PROTOFUNC(DOMCSSRuleFunc) // Not a proto, but doesn't matter

DOMCSSRule::DOMCSSRule(ExecState* exec, DOM::CSSRuleImpl* r)
  : m_impl(r)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

DOMCSSRule::~DOMCSSRule()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

const ClassInfo DOMCSSRule::info = { "CSSRule", 0, &DOMCSSRuleTable, 0 };
const ClassInfo DOMCSSRule::style_info = { "CSSStyleRule", &DOMCSSRule::info, &DOMCSSStyleRuleTable, 0 };
const ClassInfo DOMCSSRule::media_info = { "CSSMediaRule", &DOMCSSRule::info, &DOMCSSMediaRuleTable, 0 };
const ClassInfo DOMCSSRule::fontface_info = { "CSSFontFaceRule", &DOMCSSRule::info, &DOMCSSFontFaceRuleTable, 0 };
const ClassInfo DOMCSSRule::page_info = { "CSSPageRule", &DOMCSSRule::info, &DOMCSSPageRuleTable, 0 };
const ClassInfo DOMCSSRule::import_info = { "CSSImportRule", &DOMCSSRule::info, &DOMCSSImportRuleTable, 0 };
const ClassInfo DOMCSSRule::charset_info = { "CSSCharsetRule", &DOMCSSRule::info, &DOMCSSCharsetRuleTable, 0 };
const ClassInfo DOMCSSRule::namespace_info = { "CSSNamespaceRule", &DOMCSSRule::info, &DOMCSSNamespaceRuleTable, 0 };

const ClassInfo* DOMCSSRule::classInfo() const
{
  switch (m_impl->type()) {
  case DOM::CSSRule::STYLE_RULE:
    return &style_info;
  case DOM::CSSRule::MEDIA_RULE:
    return &media_info;
  case DOM::CSSRule::FONT_FACE_RULE:
    return &fontface_info;
  case DOM::CSSRule::PAGE_RULE:
    return &page_info;
  case DOM::CSSRule::IMPORT_RULE:
    return &import_info;
  case DOM::CSSRule::CHARSET_RULE:
    return &charset_info;
  case DOM::CSSRule::NAMESPACE_RULE:
    return &namespace_info;
  case DOM::CSSRule::UNKNOWN_RULE:
  default:
    return &info;
  }
}
/*
@begin DOMCSSRuleTable 4
  type			DOMCSSRule::Type	DontDelete|ReadOnly
  cssText		DOMCSSRule::CssText	DontDelete|ReadOnly
  parentStyleSheet	DOMCSSRule::ParentStyleSheet	DontDelete|ReadOnly
  parentRule		DOMCSSRule::ParentRule	DontDelete|ReadOnly
@end
@begin DOMCSSStyleRuleTable 2
  selectorText		DOMCSSRule::Style_SelectorText	DontDelete
  style			DOMCSSRule::Style_Style		DontDelete|ReadOnly
@end
@begin DOMCSSMediaRuleTable 4
  media			DOMCSSRule::Media_Media		DontDelete|ReadOnly
  cssRules		DOMCSSRule::Media_CssRules	DontDelete|ReadOnly
  insertRule		DOMCSSRule::Media_InsertRule	DontDelete|Function 2
  deleteRule		DOMCSSRule::Media_DeleteRule	DontDelete|Function 1
@end
@begin DOMCSSFontFaceRuleTable 1
  style			DOMCSSRule::FontFace_Style	DontDelete|ReadOnly
@end
@begin DOMCSSPageRuleTable 2
  selectorText		DOMCSSRule::Page_SelectorText	DontDelete
  style			DOMCSSRule::Page_Style		DontDelete|ReadOnly
@end
@begin DOMCSSImportRuleTable 3
  href			DOMCSSRule::Import_Href		DontDelete|ReadOnly
  media			DOMCSSRule::Import_Media	DontDelete|ReadOnly
  styleSheet		DOMCSSRule::Import_StyleSheet	DontDelete|ReadOnly
@end
@begin DOMCSSCharsetRuleTable 1
  encoding		DOMCSSRule::Charset_Encoding	DontDelete
@end
@begin DOMCSSNamespaceRuleTable 2
  namespaceURI		DOMCSSRule::Namespace_NamespaceURI	DontDelete|ReadOnly
  prefix                DOMCSSRule::Namespace_Prefix            DontDelete|ReadOnly
@end
*/
bool DOMCSSRule::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMCSSRule::tryGet " << propertyName.qstring();
#endif
  //First do the rule-type-specific stuff
  const HashTable* table = classInfo()->propHashTable; // get the right hashtable
  if (getStaticOwnPropertySlot<DOMCSSRuleFunc, DOMCSSRule>(table, this, propertyName, slot))
    return true;

  //Now do generic stuff
  return getStaticPropertySlot<DOMCSSRuleFunc, DOMCSSRule, DOMObject>(exec, &DOMCSSRuleTable, this, propertyName, slot);
}

JSValue *DOMCSSRule::getValueProperty(ExecState *exec, int token) const
{
  CSSRuleImpl &cssRule = *m_impl;
  switch (token) {
  case Type:
    return jsNumber(cssRule.type());
  case CssText:
    return jsString(cssRule.cssText());
  case ParentStyleSheet:
    return getDOMStyleSheet(exec,cssRule.parentStyleSheet());
  case ParentRule:
    return getDOMCSSRule(exec,cssRule.parentRule());

  // for DOM::CSSRule::STYLE_RULE:
  case Style_SelectorText:
    return jsString(static_cast<CSSStyleRuleImpl *>(m_impl.get())->selectorText());
  case Style_Style:
    return getDOMCSSStyleDeclaration(exec, static_cast<CSSStyleRuleImpl *>(m_impl.get())->style());

  // for DOM::CSSRule::MEDIA_RULE:
  case Media_Media:
    return getDOMMediaList(exec, static_cast<CSSMediaRuleImpl *>(m_impl.get())->media());
  case Media_CssRules:
    return getDOMCSSRuleList(exec, static_cast<CSSMediaRuleImpl *>(m_impl.get())->cssRules());

  // for DOM::CSSRule::FONT_FACE_RULE:
  case FontFace_Style:
    return getDOMCSSStyleDeclaration(exec, static_cast<CSSFontFaceRuleImpl *>(m_impl.get())->style());

  // for DOM::CSSRule::PAGE_RULE:
  case Page_SelectorText:
    return jsString(static_cast<CSSPageRuleImpl *>(m_impl.get())->selectorText());
  case Page_Style:
    return getDOMCSSStyleDeclaration(exec, static_cast<CSSPageRuleImpl *>(m_impl.get())->style());

  // for DOM::CSSRule::IMPORT_RULE:
  case Import_Href:
    return jsString(static_cast<CSSImportRuleImpl *>(m_impl.get())->href());
  case Import_Media:
    return getDOMMediaList(exec, static_cast<CSSImportRuleImpl *>(m_impl.get())->media());
  case Import_StyleSheet:
    return getDOMStyleSheet(exec, static_cast<CSSImportRuleImpl *>(m_impl.get())->styleSheet());

  // for DOM::CSSRule::CHARSET_RULE:
  case Charset_Encoding:
    return jsString(static_cast<CSSCharsetRuleImpl *>(m_impl.get())->encoding());
    
  // for DOM::CSSRule::NAMESPACE_RULE:
  case Namespace_Prefix:
    return jsString(static_cast<DOM::CSSNamespaceRuleImpl *>(m_impl.get())->prefix());
  case Namespace_NamespaceURI:
    return jsString(static_cast<DOM::CSSNamespaceRuleImpl *>(m_impl.get())->namespaceURI());
default:
    assert(0);
  }
  return jsUndefined();
}

void DOMCSSRule::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr)
{
  const HashTable* table = classInfo()->propHashTable; // get the right hashtable
  const HashEntry* entry = Lookup::findEntry(table, propertyName);
  if (entry) {
    if (entry->attr & Function) // function: put as override property
    {
      JSObject::put(exec, propertyName, value, attr);
      return;
    }
    else if ((entry->attr & ReadOnly) == 0) // let lookupPut print the warning if not
    {
      putValueProperty(exec, entry->value, value, attr);
      return;
    }
  }
  lookupPut<DOMCSSRule, DOMObject>(exec, propertyName, value, attr, &DOMCSSRuleTable, this);
}

void DOMCSSRule::putValueProperty(ExecState *exec, int token, JSValue *value, int)
{
  switch (token) {
  // for DOM::CSSRule::STYLE_RULE:
  case Style_SelectorText:
    static_cast<CSSStyleRuleImpl *>(m_impl.get())->setSelectorText(value->toString(exec).domString());
    return;

  // for DOM::CSSRule::PAGE_RULE:
  case Page_SelectorText:
    static_cast<CSSPageRuleImpl *>(m_impl.get())->setSelectorText(value->toString(exec).domString());
    return;

  // for DOM::CSSRule::CHARSET_RULE:
  case Charset_Encoding:
    static_cast<CSSCharsetRuleImpl *>(m_impl.get())->setEncoding(value->toString(exec).domString());
    return;

  default:
    // qDebug() << "DOMCSSRule::putValueProperty unhandled token " << token;
    return;
  }
}

JSValue *DOMCSSRuleFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMCSSRule, thisObj );
  DOM::CSSRuleImpl& cssRule = *static_cast<DOMCSSRule *>(thisObj)->impl();

  if (cssRule.type() == DOM::CSSRule::MEDIA_RULE) {
    DOM::CSSMediaRuleImpl& rule = static_cast<DOM::CSSMediaRuleImpl&>(cssRule);
    if (id == DOMCSSRule::Media_InsertRule)
      return jsNumber(rule.insertRule(args[0]->toString(exec).domString(),args[1]->toInteger(exec)));
    else if (id == DOMCSSRule::Media_DeleteRule)
      rule.deleteRule(args[0]->toInteger(exec));
  }

  return jsUndefined();
}

JSValue *getDOMCSSRule(ExecState *exec, DOM::CSSRuleImpl* r)
{
  return cacheDOMObject<DOM::CSSRuleImpl, KJS::DOMCSSRule>(exec, r);
}

// -------------------------------------------------------------------------

const ClassInfo CSSRuleConstructor::info = { "CSSRuleConstructor", 0, &CSSRuleConstructorTable, 0 };
/*
@begin CSSRuleConstructorTable 7
  UNKNOWN_RULE	CSSRuleConstructor::UNKNOWN_RULE	DontDelete|ReadOnly
  STYLE_RULE	CSSRuleConstructor::STYLE_RULE		DontDelete|ReadOnly
  CHARSET_RULE	CSSRuleConstructor::CHARSET_RULE	DontDelete|ReadOnly
  IMPORT_RULE	CSSRuleConstructor::IMPORT_RULE		DontDelete|ReadOnly
  MEDIA_RULE	CSSRuleConstructor::MEDIA_RULE		DontDelete|ReadOnly
  FONT_FACE_RULE CSSRuleConstructor::FONT_FACE_RULE	DontDelete|ReadOnly
  PAGE_RULE	CSSRuleConstructor::PAGE_RULE		DontDelete|ReadOnly
@end
*/

CSSRuleConstructor::CSSRuleConstructor(ExecState *exec)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

bool CSSRuleConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<CSSRuleConstructor, DOMObject>(exec, &CSSRuleConstructorTable, this, propertyName, slot);
}

JSValue *CSSRuleConstructor::getValueProperty(ExecState *, int token) const
{
  switch (token) {
  case UNKNOWN_RULE:
    return jsNumber(DOM::CSSRule::UNKNOWN_RULE);
  case STYLE_RULE:
    return jsNumber(DOM::CSSRule::STYLE_RULE);
  case CHARSET_RULE:
    return jsNumber(DOM::CSSRule::CHARSET_RULE);
  case IMPORT_RULE:
    return jsNumber(DOM::CSSRule::IMPORT_RULE);
  case MEDIA_RULE:
    return jsNumber(DOM::CSSRule::MEDIA_RULE);
  case FONT_FACE_RULE:
    return jsNumber(DOM::CSSRule::FONT_FACE_RULE);
  case PAGE_RULE:
    return jsNumber(DOM::CSSRule::PAGE_RULE);
  }
  return 0;
}

JSValue *getCSSRuleConstructor(ExecState *exec)
{
  return cacheGlobalObject<CSSRuleConstructor>( exec, "[[cssRule.constructor]]" );
}

// -------------------------------------------------------------------------

const ClassInfo DOMCSSValue::info = { "CSSValue", 0, &DOMCSSValueTable, 0 };

/*
@begin DOMCSSValueTable 2
  cssText	DOMCSSValue::CssText		DontDelete|ReadOnly
  cssValueType	DOMCSSValue::CssValueType	DontDelete|ReadOnly
@end
*/

DOMCSSValue::DOMCSSValue(ExecState* exec, DOM::CSSValueImpl* val)
  : m_impl(val)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

DOMCSSValue::~DOMCSSValue()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

JSValue *DOMCSSValue::getValueProperty(ExecState*, int token) const
{
  CSSValueImpl &cssValue = *m_impl;
  switch (token) {
  case CssText:
    return jsString(cssValue.cssText());
  case CssValueType:
    return jsNumber(cssValue.cssValueType());
  default:
    assert(0);
    return jsUndefined();
  }
}

bool DOMCSSValue::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMCSSValue, DOMObject>(exec, &DOMCSSValueTable, this, propertyName, slot);
}

void DOMCSSValue::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr)
{
  CSSValueImpl &cssValue = *m_impl;
  if (propertyName == "cssText") {
    cssValue.setCssText(value->toString(exec).domString());
  } else
    DOMObject::put(exec, propertyName, value, attr);
}

JSValue *getDOMCSSValue(ExecState *exec, DOM::CSSValueImpl* v)
{
  DOMObject *ret;
  if (!v)
    return jsNull();
  ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());
  if ((ret = interp->getDOMObject(v)))
    return ret;
  else {
    if (v->isValueList())
      ret = new DOMCSSValueList(exec, static_cast<CSSValueListImpl *>(v));
    else if (v->isPrimitiveValue())
      ret = new DOMCSSPrimitiveValue(exec, static_cast<CSSPrimitiveValueImpl *>(v));
    else
      ret = new DOMCSSValue(exec,v);
    interp->putDOMObject(v,ret);
    return ret;
  }
}

// -------------------------------------------------------------------------

const ClassInfo CSSValueConstructor::info = { "CSSValueConstructor", 0, &CSSValueConstructorTable, 0 };
/*
@begin CSSValueConstructorTable 5
  CSS_INHERIT		CSSValueConstructor::CSS_INHERIT		DontDelete|ReadOnly
  CSS_PRIMITIVE_VALUE	CSSValueConstructor::CSS_PRIMITIVE_VALUE	DontDelete|ReadOnly
  CSS_VALUE_LIST	CSSValueConstructor::CSS_VALUE_LIST		DontDelete|ReadOnly
  CSS_CUSTOM		CSSValueConstructor::CSS_CUSTOM			DontDelete|ReadOnly
@end
*/

CSSValueConstructor::CSSValueConstructor(ExecState *exec)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

bool CSSValueConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<CSSValueConstructor, DOMObject>(exec, &CSSValueConstructorTable, this, propertyName, slot);
}

JSValue *CSSValueConstructor::getValueProperty(ExecState *, int token) const
{
  switch (token) {
  case CSS_INHERIT:
    return jsNumber(DOM::CSSValue::CSS_INHERIT);
  case CSS_PRIMITIVE_VALUE:
    return jsNumber(DOM::CSSValue::CSS_PRIMITIVE_VALUE);
  case CSS_VALUE_LIST:
    return jsNumber(DOM::CSSValue::CSS_VALUE_LIST);
  case CSS_CUSTOM:
    return jsNumber(DOM::CSSValue::CSS_CUSTOM);
  }
  return 0;
}

JSValue *getCSSValueConstructor(ExecState *exec)
{
  return cacheGlobalObject<CSSValueConstructor>( exec, "[[cssValue.constructor]]" );
}

// -------------------------------------------------------------------------

const ClassInfo DOMCSSPrimitiveValue::info = { "CSSPrimitiveValue", 0, &DOMCSSPrimitiveValueTable, 0 };
/*
@begin DOMCSSPrimitiveValueTable 1
  primitiveType		DOMCSSPrimitiveValue::PrimitiveType	DontDelete|ReadOnly
@end
@begin DOMCSSPrimitiveValueProtoTable 3
  setFloatValue		DOMCSSPrimitiveValue::SetFloatValue	DontDelete|Function 2
  getFloatValue		DOMCSSPrimitiveValue::GetFloatValue	DontDelete|Function 1
  setStringValue	DOMCSSPrimitiveValue::SetStringValue	DontDelete|Function 2
  getStringValue	DOMCSSPrimitiveValue::GetStringValue	DontDelete|Function 0
  getCounterValue	DOMCSSPrimitiveValue::GetCounterValue	DontDelete|Function 0
  getRectValue		DOMCSSPrimitiveValue::GetRectValue	DontDelete|Function 0
  getRGBColorValue	DOMCSSPrimitiveValue::GetRGBColorValue	DontDelete|Function 0
@end
*/
KJS_DEFINE_PROTOTYPE(DOMCSSPrimitiveValueProto)
KJS_IMPLEMENT_PROTOFUNC(DOMCSSPrimitiveValueProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMCSSPrimitiveValue",DOMCSSPrimitiveValueProto,DOMCSSPrimitiveValueProtoFunc,ObjectPrototype)

DOMCSSPrimitiveValue::DOMCSSPrimitiveValue(ExecState *exec, DOM::CSSPrimitiveValueImpl* v)
  : DOMCSSValue(exec, v) {
  setPrototype(DOMCSSPrimitiveValueProto::self(exec));
}

JSValue *DOMCSSPrimitiveValue::getValueProperty(ExecState*, int token)
{
  assert(token == PrimitiveType);
  Q_UNUSED(token);
  return jsNumber(static_cast<CSSPrimitiveValueImpl *>(impl())->primitiveType());
}

bool DOMCSSPrimitiveValue::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMCSSPrimitiveValue, DOMCSSValue>(exec, &DOMCSSPrimitiveValueTable, this, propertyName, slot);
}

JSValue *DOMCSSPrimitiveValueProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMCSSPrimitiveValue, thisObj );
  CSSPrimitiveValueImpl &val = *static_cast<CSSPrimitiveValueImpl *>(static_cast<DOMCSSPrimitiveValue *>(thisObj)->impl());
  DOMExceptionTranslator exception(exec);
  switch (id) {
    case DOMCSSPrimitiveValue::SetFloatValue:
      val.setFloatValue(args[0]->toInteger(exec),args[1]->toNumber(exec), exception);
      return jsUndefined();
    case DOMCSSPrimitiveValue::GetFloatValue:
      //### FIXME: exception?
      return jsNumber(val.floatValue(args[0]->toInteger(exec)));
    case DOMCSSPrimitiveValue::SetStringValue:
      val.setStringValue(args[0]->toInteger(exec),args[1]->toString(exec).domString(), exception);
      return jsUndefined();
    case DOMCSSPrimitiveValue::GetStringValue:
      return jsString(DOM::DOMString(val.getStringValue()));
    case DOMCSSPrimitiveValue::GetCounterValue:
      return getDOMCounter(exec,val.getCounterValue());
    case DOMCSSPrimitiveValue::GetRectValue:
      return getDOMRect(exec,val.getRectValue());
    case DOMCSSPrimitiveValue::GetRGBColorValue:
      return getDOMRGBColor(exec,val.getRGBColorValue());
    default:
      return jsUndefined();
  }
}

// -------------------------------------------------------------------------

const ClassInfo CSSPrimitiveValueConstructor::info = { "CSSPrimitiveValueConstructor", 0, &CSSPrimitiveValueConstructorTable, 0 };

/*
@begin CSSPrimitiveValueConstructorTable 27
  CSS_UNKNOWN   	DOM::CSSPrimitiveValue::CSS_UNKNOWN	DontDelete|ReadOnly
  CSS_NUMBER    	DOM::CSSPrimitiveValue::CSS_NUMBER	DontDelete|ReadOnly
  CSS_PERCENTAGE	DOM::CSSPrimitiveValue::CSS_PERCENTAGE	DontDelete|ReadOnly
  CSS_EMS       	DOM::CSSPrimitiveValue::CSS_EMS		DontDelete|ReadOnly
  CSS_EXS       	DOM::CSSPrimitiveValue::CSS_EXS		DontDelete|ReadOnly
  CSS_PX        	DOM::CSSPrimitiveValue::CSS_PX		DontDelete|ReadOnly
  CSS_CM        	DOM::CSSPrimitiveValue::CSS_CM		DontDelete|ReadOnly
  CSS_MM        	DOM::CSSPrimitiveValue::CSS_MM		DontDelete|ReadOnly
  CSS_IN        	DOM::CSSPrimitiveValue::CSS_IN		DontDelete|ReadOnly
  CSS_PT        	DOM::CSSPrimitiveValue::CSS_PT		DontDelete|ReadOnly
  CSS_PC        	DOM::CSSPrimitiveValue::CSS_PC		DontDelete|ReadOnly
  CSS_DEG       	DOM::CSSPrimitiveValue::CSS_DEG		DontDelete|ReadOnly
  CSS_RAD       	DOM::CSSPrimitiveValue::CSS_RAD		DontDelete|ReadOnly
  CSS_GRAD      	DOM::CSSPrimitiveValue::CSS_GRAD	DontDelete|ReadOnly
  CSS_MS        	DOM::CSSPrimitiveValue::CSS_MS		DontDelete|ReadOnly
  CSS_S			DOM::CSSPrimitiveValue::CSS_S		DontDelete|ReadOnly
  CSS_HZ        	DOM::CSSPrimitiveValue::CSS_HZ		DontDelete|ReadOnly
  CSS_KHZ       	DOM::CSSPrimitiveValue::CSS_KHZ		DontDelete|ReadOnly
  CSS_DIMENSION 	DOM::CSSPrimitiveValue::CSS_DIMENSION	DontDelete|ReadOnly
  CSS_STRING    	DOM::CSSPrimitiveValue::CSS_STRING	DontDelete|ReadOnly
  CSS_URI       	DOM::CSSPrimitiveValue::CSS_URI		DontDelete|ReadOnly
  CSS_IDENT     	DOM::CSSPrimitiveValue::CSS_IDENT	DontDelete|ReadOnly
  CSS_ATTR      	DOM::CSSPrimitiveValue::CSS_ATTR	DontDelete|ReadOnly
  CSS_COUNTER   	DOM::CSSPrimitiveValue::CSS_COUNTER	DontDelete|ReadOnly
  CSS_RECT      	DOM::CSSPrimitiveValue::CSS_RECT	DontDelete|ReadOnly
  CSS_RGBCOLOR  	DOM::CSSPrimitiveValue::CSS_RGBCOLOR	DontDelete|ReadOnly
@end
*/

bool CSSPrimitiveValueConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot) {
  return getStaticValueSlot<CSSPrimitiveValueConstructor, DOMObject>(exec, &CSSPrimitiveValueConstructorTable, this, propertyName, slot);
}

JSValue *CSSPrimitiveValueConstructor::getValueProperty(ExecState *, int token) const
{
  // We use the token as the value to return directly
  return jsNumber(token);
}

JSValue *getCSSPrimitiveValueConstructor(ExecState *exec)
{
  return cacheGlobalObject<CSSPrimitiveValueConstructor>( exec, "[[cssPrimitiveValue.constructor]]" );
}

// -------------------------------------------------------------------------

const ClassInfo DOMCSSValueList::info = { "CSSValueList", 0, &DOMCSSValueListTable, 0 };

/*
@begin DOMCSSValueListTable 3
  length		DOMCSSValueList::Length		DontDelete|ReadOnly
  item			DOMCSSValueList::Item		DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(DOMCSSValueListFunc) // not really a proto, but doesn't matter

DOMCSSValueList::DOMCSSValueList(ExecState *exec, DOM::CSSValueListImpl* v)
  : DOMCSSValue(exec, v) { }


JSValue *DOMCSSValueList::indexGetter(ExecState *exec, unsigned index)
{
  return getDOMCSSValue(exec, impl()->item(index));
}

bool DOMCSSValueList::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  if (getStaticOwnPropertySlot<DOMCSSValueListFunc, DOMCSSValueList>(
        &DOMCSSValueListTable, this, propertyName, slot))
    return true;

  CSSValueListImpl &valueList = *static_cast<CSSValueListImpl *>(impl());
  if (getIndexSlot(this, valueList, propertyName, slot))
    return true;

  return DOMCSSValue::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue *DOMCSSValueListFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMCSSValueList, thisObj );
  CSSValueListImpl &valueList = *static_cast<CSSValueListImpl *>(static_cast<DOMCSSValueList *>(thisObj)->impl());
  switch (id) {
    case DOMCSSValueList::Item:
      return getDOMCSSValue(exec,valueList.item(args[0]->toInteger(exec)));
    default:
      return jsUndefined();
  }
}

// -------------------------------------------------------------------------

const ClassInfo DOMRGBColor::info = { "RGBColor", 0, &DOMRGBColorTable, 0 };

/*
@begin DOMRGBColorTable 3
  red	DOMRGBColor::Red	DontDelete|ReadOnly
  green	DOMRGBColor::Green	DontDelete|ReadOnly
  blue	DOMRGBColor::Blue	DontDelete|ReadOnly
@end
*/

DOMRGBColor::DOMRGBColor(ExecState* exec, QRgb c)
  : m_color(c)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

bool DOMRGBColor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot) {
  return getStaticValueSlot<DOMRGBColor, DOMObject>(exec, &DOMRGBColorTable, this, propertyName, slot);
}

JSValue *DOMRGBColor::getValueProperty(ExecState *exec, int token) const
{
  int color;
  switch (token) {
  case Red:
    color = qRed(m_color); break;
  case Green:
    color = qGreen(m_color); break;
  case Blue:
    color = qBlue(m_color); break;
  default:
    assert(0);
    return jsUndefined();
  }

  return new DOMCSSPrimitiveValue(exec, new CSSPrimitiveValueImpl(color, CSSPrimitiveValue::CSS_NUMBER));
}

JSValue *getDOMRGBColor(ExecState *exec, unsigned color)
{
  // ### implement equals for RGBColor since they're not refcounted objects
  return new DOMRGBColor(exec, color);
}

// -------------------------------------------------------------------------

const ClassInfo DOMRect::info = { "Rect", 0, &DOMRectTable, 0 };
/*
@begin DOMRectTable 4
  top	DOMRect::Top	DontDelete|ReadOnly
  right	DOMRect::Right	DontDelete|ReadOnly
  bottom DOMRect::Bottom DontDelete|ReadOnly
  left	DOMRect::Left	DontDelete|ReadOnly
@end
*/

DOMRect::DOMRect(ExecState *exec, DOM::RectImpl* r)
  : m_impl(r)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

DOMRect::~DOMRect()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

bool DOMRect::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMRect, DOMObject>(exec, &DOMRectTable, this, propertyName, slot);
}

JSValue *DOMRect::getValueProperty(ExecState *exec, int token) const
{
  DOM::RectImpl& rect = *m_impl;
  switch (token) {
  case Top:
    return getDOMCSSValue(exec, rect.top());
  case Right:
    return getDOMCSSValue(exec, rect.right());
  case Bottom:
    return getDOMCSSValue(exec, rect.bottom());
  case Left:
    return getDOMCSSValue(exec, rect.left());
  default:
    return 0;
  }
}

JSValue *getDOMRect(ExecState *exec, DOM::RectImpl* r)
{
  return cacheDOMObject<DOM::RectImpl, KJS::DOMRect>(exec, r);
}

// -------------------------------------------------------------------------

const ClassInfo DOMCounter::info = { "Counter", 0, &DOMCounterTable, 0 };
/*
@begin DOMCounterTable 3
  identifier	DOMCounter::identifier	DontDelete|ReadOnly
  listStyle	DOMCounter::listStyle	DontDelete|ReadOnly
  separator	DOMCounter::separator	DontDelete|ReadOnly
@end
*/
DOMCounter::DOMCounter(ExecState *exec, DOM::CounterImpl* c)
  : m_impl(c)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

DOMCounter::~DOMCounter()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

bool DOMCounter::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMCounter, DOMObject>(exec, &DOMCounterTable, this, propertyName, slot);
}

JSValue *DOMCounter::getValueProperty(ExecState *, int token) const
{
  CounterImpl &counter = *m_impl;
  switch (token) {
  case identifier:
    return jsString(counter.identifier());
  case listStyle:
    return jsString(khtml::stringForListStyleType((khtml::EListStyleType)counter.listStyle()));
  case separator:
    return jsString(counter.separator());
  default:
    return 0;
  }
}

JSValue *getDOMCounter(ExecState *exec, DOM::CounterImpl* c)
{
  return cacheDOMObject<DOM::CounterImpl, KJS::DOMCounter>(exec, c);
}

}

