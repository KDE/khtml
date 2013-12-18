// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001-2003 David Faure (faure@kde.org)
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

#include "kjs_binding.h"

#include <config-khtml.h>
#if HAVE_VALGRIND_MEMCHECK_H

#include <valgrind/memcheck.h>
#define VALGRIND_SUPPORT

#endif


#include "kjs_dom.h"
#include "kjs_range.h"

#include <dom/css_stylesheet.h>
#include <dom/dom_exception.h>
#include <dom/dom2_range.h>
#include <dom/dom3_xpath.h>
#include <xml/dom2_eventsimpl.h>
#include <khtmlpart_p.h>

#include <QDebug>
#include <kparts/browserextension.h>
#include <kmessagebox.h>
#include <QTextDocument> // Qt::escape

#ifdef KJS_DEBUGGER
#include "debugger/debugwindow.h"
#endif

#include <QtCore/QList>

#include <assert.h>
#include <stdlib.h>

using namespace KJSDebugger;

namespace KJS {

UString DOMObject::toString(ExecState *) const
{
  return "[object " + className() + "]";
}

HashMap<void*, DOMObject*>* ScriptInterpreter::s_allDomObjects;

typedef QList<ScriptInterpreter*> InterpreterList;
static InterpreterList *interpreterList;

ScriptInterpreter::ScriptInterpreter( JSGlobalObject *global, khtml::ChildFrame* frame )
  : Interpreter( global ), m_frame( frame ),
    m_evt( 0L ), m_inlineCode(false), m_timerCallback(false)
{
#ifdef KJS_VERBOSE
  qDebug() << "ScriptInterpreter::ScriptInterpreter " << this << " for part=" << m_frame;
#endif
  if ( !interpreterList )
    interpreterList = new InterpreterList;
  interpreterList->append( this );
}

ScriptInterpreter::~ScriptInterpreter()
{
#ifdef KJS_VERBOSE
  qDebug() << "ScriptInterpreter::~ScriptInterpreter " << this << " for part=" << m_frame;
#endif
  assert( interpreterList && interpreterList->contains( this ) );
  interpreterList->removeAll( this );
  if ( interpreterList->isEmpty() ) {
    delete interpreterList;
    interpreterList = 0;
  }
}

void ScriptInterpreter::forgetDOMObject( void* objectHandle )
{
  if( !interpreterList ) return;

  for (int i = 0; i < interpreterList->size(); ++i)
    interpreterList->at(i)->m_domObjects.remove( objectHandle );
  allDomObjects()->remove( objectHandle );
}

void ScriptInterpreter::mark(bool isMain)
{
  Interpreter::mark(isMain);
#ifdef KJS_VERBOSE
  qDebug() << "ScriptInterpreter::mark " << this << " marking " << m_domObjects.size() << " DOM objects";
#endif
  HashMap<void*, DOMObject*>::iterator it = m_domObjects.begin();
  while (it != m_domObjects.end()) {
    DOMObject* obj = it->second;
    if (obj->shouldMark())
        obj->mark();
    ++it;
  }
}

KParts::ReadOnlyPart* ScriptInterpreter::part() const {
    return m_frame->m_part.data();
}

bool ScriptInterpreter::isWindowOpenAllowed() const
{
  if ( m_evt )
  {
    int id = m_evt->handle()->id();
    bool eventOk = ( // mouse events
      id == DOM::EventImpl::CLICK_EVENT ||
      id == DOM::EventImpl::MOUSEUP_EVENT || id == DOM::EventImpl::MOUSEDOWN_EVENT ||
      id == DOM::EventImpl::KHTML_ECMA_CLICK_EVENT || id == DOM::EventImpl::KHTML_ECMA_DBLCLICK_EVENT ||
      // keyboard events
      id == DOM::EventImpl::KEYDOWN_EVENT || id == DOM::EventImpl::KEYPRESS_EVENT ||
      id == DOM::EventImpl::KEYUP_EVENT ||
      // other accepted events
      id == DOM::EventImpl::SELECT_EVENT || id == DOM::EventImpl::CHANGE_EVENT ||
      id == DOM::EventImpl::SUBMIT_EVENT );
    // qDebug() << "Window.open, smart policy: id=" << id << " eventOk=" << eventOk;
    if (eventOk)
      return true;
  } else // no event
  {
    if ( m_inlineCode && !m_timerCallback )
    {
      // This is the <a href="javascript:window.open('...')> case -> we let it through
      return true;
      // qDebug() << "Window.open, smart policy, no event, inline code -> ok";
    }
    else { // This is the <script>window.open(...)</script> case or a timer callback -> block it
      // qDebug() << "Window.open, smart policy, no event, <script> tag -> refused";
    }
  }
  return false;
}

bool ScriptInterpreter::s_disableCPUGuard = false;

void ScriptInterpreter::startCPUGuard()
{
  if (s_disableCPUGuard) return;

  unsigned time = 5000;
#ifdef VALGRIND_SUPPORT
  if (RUNNING_ON_VALGRIND)
    time *= 50;
#endif

  setTimeoutTime(time);
  startTimeoutCheck();
}

void ScriptInterpreter::stopCPUGuard()
{
   if (s_disableCPUGuard) return;
   stopTimeoutCheck();
}


bool ScriptInterpreter::shouldInterruptScript() const
{
#ifdef KJS_DEBUGGER
  if (DebugWindow::isBlocked())
    return false;
#endif

    // qDebug() << "alarmhandler";
  return KMessageBox::warningYesNo(0L, i18n("A script on this page is causing KHTML to freeze. If it continues to run, other applications may become less responsive.\nDo you want to stop the script?"), i18n("JavaScript"), KGuiItem(i18n("&Stop Script")), KStandardGuiItem::cont(), "kjscupguard_alarmhandler") == KMessageBox::Yes;
}

UString::UString(const QString &d)
{
  unsigned int len = d.length();
  UChar *dat = static_cast<UChar*>(fastMalloc(sizeof(UChar)*len));
  memcpy(dat, d.unicode(), len * sizeof(UChar));
  m_rep = UString::Rep::create(dat, len);
}

UString::UString(const DOM::DOMString &d)
{
  if (d.isNull()) {
    // we do a conversion here as null DOMStrings shouldn't cross
    // the boundary to kjs. They should either be empty strings
    // or explicitly converted to KJS::Null via getString().
    m_rep = &Rep::empty;
    return;
  }

  unsigned int len = d.length();
  UChar *dat = static_cast<UChar*>(fastMalloc(sizeof(UChar)*len));
  memcpy(dat, d.unicode(), len * sizeof(UChar));
  m_rep = UString::Rep::create(dat, len);
}

DOM::DOMString UString::domString() const
{
  return DOM::DOMString((QChar*) data(), size());
}

QString UString::qstring() const
{
  return QString((QChar*) data(), size());
}

DOM::DOMString Identifier::domString() const
{
  return DOM::DOMString((QChar*) data(), size());
}

QString Identifier::qstring() const
{
  return QString((QChar*) data(), size());
}

JSValue* valueGetterAdapter(ExecState* exec, JSObject*, const Identifier& , const PropertySlot& slot)
{
  Q_UNUSED(exec);
  return static_cast<JSValue*>(slot.customValue());
}

DOM::NodeImpl* toNode(JSValue *val)
{
  JSObject *obj = val->getObject();
  if (!obj || !obj->inherits(&DOMNode::info))
    return 0;

  const DOMNode *dobj = static_cast<const DOMNode*>(obj);
  return dobj->impl();
}

JSValue* getStringOrNull(DOM::DOMString s)
{
  if (s.isNull())
    return jsNull();
  else
    return jsString(s);
}

DOM::DOMString valueToStringWithNullCheck(ExecState* exec, JSValue* val)
{
    if (val->isNull())
      return DOM::DOMString();
    return val->toString(exec).domString();
}

QVariant ValueToVariant(ExecState* exec, JSValue *val) {
  QVariant res;
  switch (val->type()) {
  case BooleanType:
    res = QVariant(val->toBoolean(exec));
    break;
  case NumberType:
    res = QVariant(val->toNumber(exec));
    break;
  case StringType:
    res = QVariant(val->toString(exec).qstring());
    break;
  default:
    // everything else will be 'invalid'
    break;
  }
  return res;
}

void setDOMException(ExecState *exec, int internalCode)
{
  if (internalCode == 0 || exec->hadException())
    return;

  const char* type = 0;

  DOMString name;
  DOMString exceptionString;
  JSObject* errorObject = 0;
  int code = -1; // this will get the public exception code,
                 // as opposed to the internal one

  // ### we should probably introduce classes for things other than range + core
  if (DOM::RangeException::isRangeExceptionCode(internalCode)) {
    type = "DOM Range";
    code = internalCode - DOM::RangeException::_EXCEPTION_OFFSET;
    name = DOM::RangeException::codeAsString(code);
    errorObject = new RangeException(exec);
  } else if (DOM::CSSException::isCSSExceptionCode(internalCode)) {
    type = "CSS";
    code = internalCode - DOM::CSSException::_EXCEPTION_OFFSET;
    name = DOM::CSSException::codeAsString(code);
  } else if (DOM::EventException::isEventExceptionCode(internalCode)) {
    type = "DOM Events";
    code = internalCode - DOM::EventException::_EXCEPTION_OFFSET;
    name = DOM::EventException::codeAsString(code);
  } else if (DOM::XPathException::isXPathExceptionCode(internalCode)) {
    type = "XPath";
    code = internalCode - DOM::XPathException::_EXCEPTION_OFFSET;
    name = DOM::XPathException::codeAsString(code);
  } else {
    // Generic DOM.
    type = "DOM";
    code = internalCode;
    name = DOM::DOMException::codeAsString(code);
    errorObject = new JSDOMException(exec);
  }

  if (!errorObject) {
    // 100 characters is a big enough buffer, because there are:
    //   13 characters in the message
    //   10 characters in the longest type, "DOM Events"
    //   27 characters in the longest name, "NO_MODIFICATION_ALLOWED_ERR"
    //   20 or so digits in the longest integer's ASCII form (even if int is 64-bit)
    //   1 byte for a null character
    // That adds up to about 70 bytes.
    char buffer[100];

    if (!name.isEmpty())
      snprintf(buffer, 99, "%s: %s Exception %d", name.string().toLatin1().data(), type, code);
    else
      snprintf(buffer, 99, "%s Exception %d", type, code);
    errorObject = throwError(exec, GeneralError, buffer);
  } else {
    exec->setException(errorObject);
  }

  errorObject->put(exec, exec->propertyNames().name, jsString(UString(type) + " Exception"));
  errorObject->put(exec, exec->propertyNames().message, jsString(name));
  errorObject->put(exec, "code", jsNumber(code));
}

QString valueToString(KJS::JSValue* value)
{
    switch(value->type())
    {
        case KJS::NumberType:
        {
            double v = 0.0;
            value->getNumber(v);
            return QString::number(v);
        }
        case KJS::BooleanType:
            return value->getBoolean() ? "true" : "false";
        case KJS::StringType:
        {
            KJS::UString s;
            value->getString(s);
            return '"' + s.qstring() + '"';
        }
        case KJS::UndefinedType:
            return "undefined";
        case KJS::NullType:
            return "null";
        case KJS::ObjectType:
            return "[object " + static_cast<KJS::JSObject*>(value)->className().qstring() +"]";
        case KJS::GetterSetterType:
        case KJS::UnspecifiedType:
        default:
            return QString();
    }
}

QString exceptionToString(ExecState* exec, JSValue* exceptionObj)
{
  QString exceptionMsg = valueToString(exceptionObj);

    // Since we purposefully bypass toString, we need to figure out
    // string serialization ourselves.
    //### might be easier to export class info for ErrorInstance ---

    JSObject* valueObj = exceptionObj->getObject();
    JSValue*  protoObj = valueObj ? valueObj->prototype() : 0;

    bool exception   = false;
    bool syntaxError = false;
    if (protoObj == exec->lexicalInterpreter()->builtinSyntaxErrorPrototype())
    {
        exception   = true;
        syntaxError = true;
    }

    if (protoObj == exec->lexicalInterpreter()->builtinErrorPrototype()          ||
        protoObj == exec->lexicalInterpreter()->builtinEvalErrorPrototype()      ||
        protoObj == exec->lexicalInterpreter()->builtinReferenceErrorPrototype() ||
        protoObj == exec->lexicalInterpreter()->builtinRangeErrorPrototype()     ||
        protoObj == exec->lexicalInterpreter()->builtinTypeErrorPrototype()      ||
        protoObj == exec->lexicalInterpreter()->builtinURIErrorPrototype())
    {
        exception = true;
    }

    if (!exception)
        return exceptionMsg;

    // Clear exceptions temporarily so we can get/call a few things.
    // We memorize the old exception first, of course. Note that
    // This is not always the same as exceptionObj since we may be
    //  asked to translate a non-active exception
    JSValue* oldExceptionObj = exec->exception();
    exec->clearException();

    // We want to serialize the syntax errors ourselves, to provide the line number.
    // The URL is in "sourceURL" and the line is in "line"
    // ### TODO: Perhaps we want to use 'sourceId' in case of eval contexts.
    if (syntaxError)
    {
        JSValue* lineValue = valueObj->get(exec, "line");
        JSValue* urlValue  = valueObj->get(exec, "sourceURL");

        int      line = lineValue->toNumber(exec);
        QString  url  = urlValue->toString(exec).qstring();
        exceptionMsg = i18n("Parse error at %1 line %2",
                            Qt::escape(url), line + 1);
    }
    else
    {
        // ### it's still not 100% safe to call toString here, even on
        // native exception objects, since someone might have changed the toString property
        // of the exception prototype, but I'll punt on this case for now.
        exceptionMsg = exceptionObj->toString(exec).qstring();
    }
    exec->setException(oldExceptionObj);
    return exceptionMsg;
}

} //namespace KJS
