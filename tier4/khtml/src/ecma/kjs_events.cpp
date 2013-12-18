// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003 Apple Computer, Inc.
 *  Copyright (C) 2006, 2009, 2010 Maksim Orlovich (maksim@kde.org)
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

#include "kjs_events.h"
#include "kjs_events.lut.h"

#include "kjs_data.h"
#include "kjs_window.h"
#include "kjs_views.h"
#include "kjs_proxy.h"
#include <xml/dom_nodeimpl.h>
#include <xml/dom_docimpl.h>
#include <xml/dom2_eventsimpl.h>
#include <rendering/render_object.h>
#include <rendering/render_canvas.h>
#include <khtml_part.h>
#ifdef KJS_DEBUGGER
#include "debugger/debugwindow.h"
#endif
#include <QDebug>
#include <kjs/scriptfunction.h>
#include <kjs/function_object.h>

using namespace KJS;
using namespace KJSDebugger;
using namespace DOM;

// -------------------------------------------------------------------------

JSEventListener::JSEventListener(JSObject *_listener, JSObject *_compareListenerImp, JSObject *_win, bool _html)
  : listener( _listener ), compareListenerImp( _compareListenerImp ), html( _html ), win( _win )
{
    //fprintf(stderr,"JSEventListener::JSEventListener this=%p listener=%p\n",this,listener.imp());
  if (compareListenerImp) {
    static_cast<Window*>(win.get())->jsEventListeners.insert(QPair<void*, bool>(compareListenerImp.get(), html), this);
  }
}

JSEventListener::~JSEventListener()
{
  if (compareListenerImp) {
    static_cast<Window*>(win.get())->jsEventListeners.remove(QPair<void*, bool>(compareListenerImp.get(), html));
  }
  //fprintf(stderr,"JSEventListener::~JSEventListener this=%p listener=%p\n",this,listener.imp());
}

void JSEventListener::handleEvent(DOM::Event &evt)
{
  KHTMLPart *part = qobject_cast<KHTMLPart*>(static_cast<Window*>(win.get())->part());
  KJSProxy *proxy = 0L;
  if (part)
    proxy = part->jScript();

  if (proxy && listener && listener->implementsCall()) {
#ifdef KJS_DEBUGGER
  //### This is the wrong place to do this --- we need 
  // a more global/general stategy to prevent unwanted event loop recursion issues.
    if (proxy->debugEnabled() && DebugWindow::window()->inSession())
      return;
#endif
    ref();

    KJS::ScriptInterpreter *interpreter = static_cast<KJS::ScriptInterpreter *>(proxy->interpreter());
    ExecState *exec = interpreter->globalExec();

    List args;
    args.append(getDOMEvent(exec,evt.handle()));

    JSObject *thisObj = 0;
    // Check whether handler is a function or an object with handleEvent method
    if (listener == compareListenerImp) {
      // Set "this" to the event's current target
      thisObj = getEventTarget(exec,evt.handle()->currentTarget())->getObject();
    } else {
      thisObj = compareListenerImp;
    }

    if ( !thisObj ) {
      // ### can this still happen? eventTarget should be window on Window events now.
      thisObj = win;
    }

    Window *window = static_cast<Window*>(win.get());
    // Set the event we're handling in the Window object
    window->setCurrentEvent( evt.handle() );
    // ... and in the interpreter
    interpreter->setCurrentEvent( &evt );

    interpreter->startCPUGuard();
    JSValue *retval = listener->call(exec, thisObj, args);
    interpreter->stopCPUGuard();

    window->setCurrentEvent( 0 );
    interpreter->setCurrentEvent( 0 );
    if ( exec->hadException() )
      exec->clearException();
    else if (html)
    {
      QVariant ret = ValueToVariant(exec, retval);
      if (ret.type() == QVariant::Bool && ret.toBool() == false)
        evt.preventDefault();
    }
    window->afterScriptExecution();
    deref();
  }
}

DOM::DOMString JSEventListener::eventListenerType()
{
    if (html)
	return "_khtml_HTMLEventListener";
    else
	return "_khtml_JSEventListener";
}

JSObject *JSEventListener::listenerObj() const
{
  return listener;
}

JSLazyEventListener::JSLazyEventListener(const QString &_code, const QString &_url, int _lineNum,
                               const QString &_name, JSObject *_win, DOM::NodeImpl* _originalNode, bool _svg)
  : JSEventListener(0, 0, _win, true), code(_code), url(_url), lineNum(_lineNum), 
    name(_name), parsed(false), svg(_svg)
{
  // We don't retain the original node, because we assume it
  // will stay alive as long as this handler object is around
  // and we need to avoid a reference cycle. If JS transfers
  // this handler to another node, parseCode will be called and
  // then originalNode is no longer needed.

  originalNode = _originalNode;
}

JSLazyEventListener::~JSLazyEventListener()
{
}

void JSLazyEventListener::handleEvent(DOM::Event &evt)
{
  parseCode();
  if (listener) {
    JSEventListener::handleEvent(evt);
  }
}


JSObject *JSLazyEventListener::listenerObj() const
{
  parseCode();
  return listener;
}

void JSLazyEventListener::parseCode() const
{
  if (!parsed) {
    KHTMLPart *part = qobject_cast<KHTMLPart*>(static_cast<Window*>(win.get())->part());
    KJSProxy *proxy = 0L;
    if (part)
      proxy = part->jScript();

    if (proxy) {
      KJS::ScriptInterpreter *interpreter = static_cast<KJS::ScriptInterpreter *>(proxy->interpreter());
      ExecState *exec = interpreter->globalExec();

      //KJS::Constructor constr(KJS::Global::current().get("Function").imp());
      KJS::FunctionObjectImp *constr = static_cast<KJS::FunctionObjectImp*>(interpreter->builtinFunction());
      KJS::List args;

      if (svg)
          args.append(jsString("evt"));
      else
          args.append(jsString("event"));
      
      args.append(jsString(code));
      listener = constr->construct(exec, args, 
            Identifier(UString(name)), url, lineNum); // ### is globalExec ok ?
      compareListenerImp = listener;

      if (exec->hadException()) {
        exec->clearException();

        // failed to parse, so let's just make this listener a no-op
        listener = 0;
      } else if (!listener->inherits(&DeclaredFunctionImp::info)) {
        listener = 0;// Error creating function
      } else {
        DeclaredFunctionImp *declFunc = static_cast<DeclaredFunctionImp*>(listener.get());

        if (originalNode)
        {
          // Add the event's home element to the scope
          // (and the document, and the form - see KJS::HTMLElement::eventHandlerScope)
          ScopeChain scope = declFunc->scope();

          JSObject *thisObj = getDOMNode(exec, originalNode)->getObject();

          if (thisObj) {
            static_cast<DOMNode*>(thisObj)->pushEventHandlerScope(exec, scope);
            declFunc->setScope(scope);
          }
        }
      }
    }

    // no more need to keep the unparsed code around
    code.clear();

    if (listener) {
      static_cast<Window*>(win.get())->jsEventListeners.insert(QPair<void*, bool>(compareListenerImp.get(), true),
                                              (KJS::JSEventListener *)(this));
    }

    parsed = true;
  }
}

// -------------------------------------------------------------------------

const ClassInfo DOMEvent::info = { "Event", 0, &DOMEventTable, 0 };
/*
@begin DOMEventTable 7
  type		DOMEvent::Type		DontDelete|ReadOnly
  target	DOMEvent::Target	DontDelete|ReadOnly
  currentTarget	DOMEvent::CurrentTarget	DontDelete|ReadOnly
  srcElement	DOMEvent::SrcElement	DontDelete|ReadOnly
  eventPhase	DOMEvent::EventPhase	DontDelete|ReadOnly
  bubbles	DOMEvent::Bubbles	DontDelete|ReadOnly
  cancelable	DOMEvent::Cancelable	DontDelete|ReadOnly
  timeStamp	DOMEvent::TimeStamp	DontDelete|ReadOnly
  returnValue   DOMEvent::ReturnValue   DontDelete
  cancelBubble  DOMEvent::CancelBubble  DontDelete
@end
@begin DOMEventProtoTable 3
  stopPropagation 	DOMEvent::StopPropagation	DontDelete|Function 0
  preventDefault 	DOMEvent::PreventDefault	DontDelete|Function 0
  initEvent		DOMEvent::InitEvent		DontDelete|Function 3
@end
*/
KJS_DEFINE_PROTOTYPE(DOMEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMEvent", DOMEventProto, DOMEventProtoFunc,ObjectPrototype)

DOMEvent::DOMEvent(ExecState *exec, DOM::EventImpl* e)
  : m_impl(e) {
  setPrototype(DOMEventProto::self(exec));
}

DOMEvent::DOMEvent(JSObject *proto, DOM::EventImpl* e):
  m_impl(e) {
  setPrototype(proto);
}

DOMEvent::~DOMEvent()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}


bool DOMEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "KJS::DOMEvent::getOwnPropertySlot " << propertyName.qstring();
#endif

  return getStaticValueSlot<DOMEvent, DOMObject>(exec,&DOMEventTable,this,propertyName,slot);
}

JSValue *DOMEvent::getValueProperty(ExecState *exec, int token) const
{
  DOM::EventImpl& event = *impl();
  switch (token) {
  case Type:
    return jsString(event.type());
  case Target:
  case SrcElement: /*MSIE extension - "the object that fired the event"*/
    return getEventTarget(exec,event.target());
  case CurrentTarget:
    return getEventTarget(exec,event.currentTarget());
  case EventPhase:
    return jsNumber((unsigned int)event.eventPhase());
  case Bubbles:
    return jsBoolean(event.bubbles());
  case Cancelable:
    return jsBoolean(event.cancelable());
  case TimeStamp:
    return jsNumber((long unsigned int)event.timeStamp()); // ### long long ?
  case ReturnValue: // MSIE extension
    // return false == cancel, so this returns the -opposite- of defaultPrevented
    return jsBoolean(!event.defaultPrevented());
  case CancelBubble: // MSIE extension
    return jsBoolean(event.propagationStopped());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMEvent::getValueProperty : " << token;
    return 0;
  }
}

JSValue *DOMEvent::defaultValue(ExecState *exec, KJS::JSType hint) const
{
  if (m_impl->id() == EventImpl::ERROR_EVENT && !m_impl->message().isNull()) {
    return jsString(m_impl->message());
  }
  else
    return DOMObject::defaultValue(exec,hint);
}

void DOMEvent::put(ExecState *exec, const Identifier &propertyName,
                      JSValue *value, int attr)
{
  lookupPut<DOMEvent, DOMObject>(exec, propertyName, value, attr,
                                          &DOMEventTable, this);
}

void DOMEvent::putValueProperty(ExecState *exec, int token, JSValue *value, int)
{
  switch (token) {
  case ReturnValue: // MSIE equivalent for "preventDefault" (but with a way to reset it)
    // returnValue=false means "default action of the event on the source object is canceled",
    // which means preventDefault(true). Hence the '!'.
    m_impl->preventDefault(!value->toBoolean(exec));
    break;
  case CancelBubble: // MSIE equivalent for "stopPropagation" (but with a way to reset it)
    m_impl->stopPropagation(value->toBoolean(exec));
    break;
  default:
    break;
  }
}

JSValue *DOMEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMEvent, thisObj );
  DOM::EventImpl& event = *static_cast<DOMEvent *>( thisObj )->impl();
  switch (id) {
    case DOMEvent::StopPropagation:
      event.stopPropagation(true);
      return jsUndefined();
    case DOMEvent::PreventDefault:
      event.preventDefault(true);
      return jsUndefined();
    case DOMEvent::InitEvent:
      event.initEvent(args[0]->toString(exec).domString(),args[1]->toBoolean(exec),args[2]->toBoolean(exec));
      return jsUndefined();
  };
  return jsUndefined();
}

JSValue *KJS::getDOMEvent(ExecState *exec, DOM::EventImpl* ei)
{
  if (!ei)
    return jsNull();
  ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());
  DOMObject *ret = interp->getDOMObject(ei);
  if (!ret) {
    if (ei->isTextInputEvent())
      ret = new DOMTextEvent(exec, static_cast<DOM::TextEventImpl*>(ei));
    else if (ei->isKeyboardEvent())
      ret = new DOMKeyboardEvent(exec, static_cast<DOM::KeyboardEventImpl*>(ei));
    else if (ei->isMouseEvent())
      ret = new DOMMouseEvent(exec, static_cast<DOM::MouseEventImpl*>(ei));
    else if (ei->isUIEvent())
      ret = new DOMUIEvent(exec, static_cast<DOM::UIEventImpl*>(ei));
    else if (ei->isMutationEvent())
      ret = new DOMMutationEvent(exec, static_cast<DOM::MutationEventImpl*>(ei));
    else if (ei->isMessageEvent())
      ret = new DOMMessageEvent(exec, static_cast<DOM::MessageEventImpl*>(ei));
    else if (ei->isHashChangeEvent())
      ret = new DOMHashChangeEvent(exec, static_cast<DOM::HashChangeEventImpl*>(ei));
    else
      ret = new DOMEvent(exec, ei);

    interp->putDOMObject(ei, ret);
  }

  return ret;
}

DOM::EventImpl* KJS::toEvent(JSValue *val)
{
  JSObject *obj = val->getObject();
  if (!obj || !obj->inherits(&DOMEvent::info))
    return 0;

  const DOMEvent *dobj = static_cast<const DOMEvent*>(obj);
  return dobj->impl();
}

// -------------------------------------------------------------------------
/*
@begin EventConstantsTable 23
  CAPTURING_PHASE   DOM::Event::CAPTURING_PHASE DontDelete|ReadOnly
  AT_TARGET     DOM::Event::AT_TARGET       DontDelete|ReadOnly
  BUBBLING_PHASE    DOM::Event::BUBBLING_PHASE  DontDelete|ReadOnly
# Reverse-engineered from Netscape
  MOUSEDOWN     1               DontDelete|ReadOnly
  MOUSEUP       2               DontDelete|ReadOnly
  MOUSEOVER     4               DontDelete|ReadOnly
  MOUSEOUT      8               DontDelete|ReadOnly
  MOUSEMOVE     16              DontDelete|ReadOnly
  MOUSEDRAG     32              DontDelete|ReadOnly
  CLICK         64              DontDelete|ReadOnly
  DBLCLICK      128             DontDelete|ReadOnly
  KEYDOWN       256             DontDelete|ReadOnly
  KEYUP         512             DontDelete|ReadOnly
  KEYPRESS      1024                DontDelete|ReadOnly
  DRAGDROP      2048                DontDelete|ReadOnly
  FOCUS         4096                DontDelete|ReadOnly
  BLUR          8192                DontDelete|ReadOnly
  SELECT        16384               DontDelete|ReadOnly
  CHANGE        32768               DontDelete|ReadOnly
@end
*/
DEFINE_CONSTANT_TABLE(EventConstants)
IMPLEMENT_CONSTANT_TABLE(EventConstants, "EventConstants")

IMPLEMENT_PSEUDO_CONSTRUCTOR_WITH_PARENT(EventConstructor, "EventConstructor", DOMEventProto, EventConstants)
// -------------------------------------------------------------------------


const ClassInfo EventExceptionConstructor::info = { "EventExceptionConstructor", 0, &EventExceptionConstructorTable, 0 };
/*
@begin EventExceptionConstructorTable 1
  UNSPECIFIED_EVENT_TYPE_ERR    DOM::EventException::UNSPECIFIED_EVENT_TYPE_ERR DontDelete|ReadOnly
@end
*/
EventExceptionConstructor::EventExceptionConstructor(ExecState *exec)
  : DOMObject(exec->lexicalInterpreter()->builtinObjectPrototype())
{
}

bool EventExceptionConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<EventExceptionConstructor, DOMObject>(exec,&EventExceptionConstructorTable,this,propertyName,slot);
}

JSValue *EventExceptionConstructor::getValueProperty(ExecState *, int token) const
{
  // We use the token as the value to return directly
  return jsNumber(token);
}

JSValue *KJS::getEventExceptionConstructor(ExecState *exec)
{
  return cacheGlobalObject<EventExceptionConstructor>(exec, "[[eventException.constructor]]");
}

// -------------------------------------------------------------------------

const ClassInfo DOMUIEvent::info = { "UIEvent", &DOMEvent::info, &DOMUIEventTable, 0 };
/*
@begin DOMUIEventTable 7
  view		DOMUIEvent::View	DontDelete|ReadOnly
  detail	DOMUIEvent::Detail	DontDelete|ReadOnly
  keyCode	DOMUIEvent::KeyCode	DontDelete|ReadOnly
  charCode	DOMUIEvent::CharCode	DontDelete|ReadOnly
  layerX	DOMUIEvent::LayerX	DontDelete|ReadOnly
  layerY	DOMUIEvent::LayerY	DontDelete|ReadOnly
  pageX		DOMUIEvent::PageX	DontDelete|ReadOnly
  pageY		DOMUIEvent::PageY	DontDelete|ReadOnly
  which		DOMUIEvent::Which	DontDelete|ReadOnly
@end
@begin DOMUIEventProtoTable 1
  initUIEvent	DOMUIEvent::InitUIEvent	DontDelete|Function 5
@end
*/
KJS_DEFINE_PROTOTYPE(DOMUIEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMUIEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMUIEvent",DOMUIEventProto,DOMUIEventProtoFunc,DOMEventProto)

DOMUIEvent::DOMUIEvent(ExecState *exec, DOM::UIEventImpl* ue) :
  DOMEvent(DOMUIEventProto::self(exec), ue) {}

DOMUIEvent::DOMUIEvent(JSObject *proto, DOM::UIEventImpl* ue) :
  DOMEvent(proto, ue) {}

DOMUIEvent::~DOMUIEvent()
{
}

bool DOMUIEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMUIEvent, DOMEvent>(exec,&DOMUIEventTable,this,propertyName,slot);
}

JSValue *DOMUIEvent::getValueProperty(ExecState *exec, int token) const
{
  DOM::UIEventImpl& event = *impl();
  switch (token) {
  case View:
    return getDOMAbstractView(exec,event.view());
  case Detail:
    return jsNumber(event.detail());
  case KeyCode:
    // IE-compatibility
    return jsNumber(event.keyCode());
  case CharCode:
    // IE-compatibility
    return jsNumber(event.charCode());
  case LayerX:
    // NS-compatibility
    return jsNumber(event.layerX());
  case LayerY:
    // NS-compatibility
    return jsNumber(event.layerY());
  case PageX:
    // NS-compatibility
    return jsNumber(event.pageX());
  case PageY:
    // NS-compatibility
    return jsNumber(event.pageY());
  case Which:
    // NS-compatibility
    return jsNumber(event.which());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMUIEvent::getValueProperty : " << token;
    return jsUndefined();
  }
}

JSValue *DOMUIEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMUIEvent, thisObj );
  DOM::UIEventImpl& uiEvent = *static_cast<DOMUIEvent *>(thisObj)->impl();
  switch (id) {
    case DOMUIEvent::InitUIEvent: {
      DOM::AbstractViewImpl* v = toAbstractView(args[3]);
      uiEvent.initUIEvent(args[0]->toString(exec).domString(),
                                                     args[1]->toBoolean(exec),
                                                     args[2]->toBoolean(exec),
                                                     v,
                                                     args[4]->toInteger(exec));
      }
      return jsUndefined();
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMMouseEvent::info = { "MouseEvent", &DOMUIEvent::info, &DOMMouseEventTable, 0 };

/*
@begin DOMMouseEventTable 2
  screenX	DOMMouseEvent::ScreenX	DontDelete|ReadOnly
  screenY	DOMMouseEvent::ScreenY	DontDelete|ReadOnly
  clientX	DOMMouseEvent::ClientX	DontDelete|ReadOnly
  x		DOMMouseEvent::X	DontDelete|ReadOnly
  clientY	DOMMouseEvent::ClientY	DontDelete|ReadOnly
  y		DOMMouseEvent::Y	DontDelete|ReadOnly
  offsetX	DOMMouseEvent::OffsetX	DontDelete|ReadOnly
  offsetY	DOMMouseEvent::OffsetY	DontDelete|ReadOnly
  ctrlKey	DOMMouseEvent::CtrlKey	DontDelete|ReadOnly
  shiftKey	DOMMouseEvent::ShiftKey	DontDelete|ReadOnly
  altKey	DOMMouseEvent::AltKey	DontDelete|ReadOnly
  metaKey	DOMMouseEvent::MetaKey	DontDelete|ReadOnly
  button	DOMMouseEvent::Button	DontDelete|ReadOnly
  relatedTarget	DOMMouseEvent::RelatedTarget DontDelete|ReadOnly
  fromElement	DOMMouseEvent::FromElement DontDelete|ReadOnly
  toElement	DOMMouseEvent::ToElement	DontDelete|ReadOnly
@end
@begin DOMMouseEventProtoTable 1
  initMouseEvent	DOMMouseEvent::InitMouseEvent	DontDelete|Function 15
@end
*/
KJS_DEFINE_PROTOTYPE(DOMMouseEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMMouseEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMMouseEvent",DOMMouseEventProto,DOMMouseEventProtoFunc,DOMUIEventProto)

DOMMouseEvent::DOMMouseEvent(ExecState *exec, DOM::MouseEventImpl* me) :
  DOMUIEvent(DOMMouseEventProto::self(exec), me) {}

DOMMouseEvent::~DOMMouseEvent()
{
}

bool DOMMouseEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMMouseEvent::getOwnPropertySlot " << propertyName.qstring();
#endif

  return getStaticValueSlot<DOMMouseEvent, DOMUIEvent>(exec,&DOMMouseEventTable,this,propertyName,slot);
}

JSValue *DOMMouseEvent::getValueProperty(ExecState *exec, int token) const
{
  DOM::MouseEventImpl& event = *impl();
  switch (token) {
  case ScreenX:
    return jsNumber(event.screenX());
  case ScreenY:
    return jsNumber(event.screenY());
  case ClientX:
  case X:
    return jsNumber(event.clientX());
  case ClientY:
  case Y:
    return jsNumber(event.clientY());
  case OffsetX:
  case OffsetY: // MSIE extension
  {
    if (event.target()->eventTargetType() != EventTargetImpl::DOM_NODE)
        return jsUndefined();
    
    DOM::Node node = static_cast<NodeImpl*>(event.target());
    khtml::RenderObject *rend = 0;
    if (node.handle()) {
        node.handle()->document()->updateRendering();
        rend = node.handle()->renderer();
    }
    int x = event.clientX();
    int y = event.clientY();
    if ( rend ) {
      int xPos, yPos;
      if ( rend->absolutePosition( xPos, yPos ) ) {
        //qDebug() << "DOMMouseEvent::getValueProperty rend=" << rend << "  xPos=" << xPos << "  yPos=" << yPos;
        x -= xPos;
        y -= yPos;
      }
      if ( rend->canvas() ) {
        int cYPos, cXPos;
        rend->canvas()->absolutePosition( cXPos,  cYPos,  true );
        x += cXPos;
        y += cYPos;
      }
    }
    return jsNumber( token == OffsetX ? x : y );
  }
  case CtrlKey:
    return jsBoolean(event.ctrlKey());
  case ShiftKey:
    return jsBoolean(event.shiftKey());
  case AltKey:
    return jsBoolean(event.altKey());
  case MetaKey:
    return jsBoolean(event.metaKey());
  case Button:
  {
    if ( exec->dynamicInterpreter()->compatMode() == Interpreter::IECompat ) {
      // Tricky. The DOM (and khtml) use 0 for LMB, 1 for MMB and 2 for RMB
      // but MSIE uses 1=LMB, 2=RMB, 4=MMB, as a bitfield
      int domButton = event.button();
      int button = domButton==0 ? 1 : domButton==1 ? 4 : domButton==2 ? 2 : 0;
      return jsNumber( (unsigned int)button );
    }
    return jsNumber(event.button());
  }
  case ToElement:
    // MSIE extension - "the object toward which the user is moving the mouse pointer"
    if (event.id() == DOM::EventImpl::MOUSEOUT_EVENT)
      return getEventTarget(exec,event.relatedTarget());
    return getEventTarget(exec,event.target());
  case FromElement:
    // MSIE extension - "object from which activation
    // or the mouse pointer is exiting during the event" (huh?)
    if (event.id() == DOM::EventImpl::MOUSEOUT_EVENT)
      return getEventTarget(exec,event.target());
    /* fall through */
  case RelatedTarget:
    return getEventTarget(exec,event.relatedTarget());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMMouseEvent::getValueProperty : " << token;
    return 0;
  }
}

JSValue *DOMMouseEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMMouseEvent, thisObj );
  DOM::MouseEventImpl& mouseEvent = *static_cast<DOMMouseEvent *>(thisObj)->impl();
  switch (id) {
    case DOMMouseEvent::InitMouseEvent:
      mouseEvent.initMouseEvent(args[0]->toString(exec).domString(), // typeArg
                                args[1]->toBoolean(exec), // canBubbleArg
                                args[2]->toBoolean(exec), // cancelableArg
                                toAbstractView(args[3]), // viewArg
                                args[4]->toInteger(exec), // detailArg
                                args[5]->toInteger(exec), // screenXArg
                                args[6]->toInteger(exec), // screenYArg
                                args[7]->toInteger(exec), // clientXArg
                                args[8]->toInteger(exec), // clientYArg
                                args[9]->toBoolean(exec), // ctrlKeyArg
                                args[10]->toBoolean(exec), // altKeyArg
                                args[11]->toBoolean(exec), // shiftKeyArg
                                args[12]->toBoolean(exec), // metaKeyArg
                                args[13]->toInteger(exec), // buttonArg
                                toNode(args[14])); // relatedTargetArg
      return jsUndefined();
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMKeyEventBase::info = { "KeyEventBase", &DOMUIEvent::info, &DOMKeyEventBaseTable, 0 };

/*
@begin DOMKeyEventBaseTable 5
  keyVal   	 DOMKeyEventBase::Key	      DontDelete|ReadOnly
  virtKeyVal	 DOMKeyEventBase::VirtKey     DontDelete|ReadOnly
  ctrlKey        DOMKeyEventBase::CtrlKey     DontDelete|ReadOnly
  altKey         DOMKeyEventBase::AltKey      DontDelete|ReadOnly
  shiftKey       DOMKeyEventBase::ShiftKey    DontDelete|ReadOnly
  metaKey        DOMKeyEventBase::MetaKey     DontDelete|ReadOnly
@end
*/

DOMKeyEventBase::DOMKeyEventBase(JSObject* proto, DOM::KeyEventBaseImpl* ke) :
  DOMUIEvent(proto, ke) {}

DOMKeyEventBase::~DOMKeyEventBase()
{}

bool DOMKeyEventBase::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMKeyEventBase::getOwnPropertySlot " << propertyName.qstring();
#endif
  return getStaticValueSlot<DOMKeyEventBase, DOMUIEvent>(exec,&DOMKeyEventBaseTable,this,propertyName,slot);
}

JSValue* DOMKeyEventBase::getValueProperty(ExecState *, int token) const
{
  DOM::KeyEventBaseImpl* tevent = impl();
  switch (token) {
  case Key:
    return jsNumber(tevent->keyVal());
  case VirtKey:
    return jsNumber(tevent->virtKeyVal());
  // these modifier attributes actually belong into a KeyboardEvent interface,
  // but we want them on "keypress" as well.
  case CtrlKey:
    return jsBoolean(tevent->ctrlKey());
  case ShiftKey:
    return jsBoolean(tevent->shiftKey());
  case AltKey:
    return jsBoolean(tevent->altKey());
  case MetaKey:
    return jsBoolean(tevent->metaKey());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMKeyEventBase::getValueProperty : " << token;
    return jsUndefined();
  }
}

// -------------------------------------------------------------------------
const ClassInfo DOMTextEvent::info = { "TextEvent", &DOMKeyEventBase::info, &DOMTextEventTable, 0 };

/*
@begin DOMTextEventTable 1
  data           DOMTextEvent::Data          DontDelete|ReadOnly
@end
@begin DOMTextEventProtoTable 1
  initTextEvent	DOMTextEvent::InitTextEvent	DontDelete|Function 5
  # Missing: initTextEventNS
@end
*/
KJS_DEFINE_PROTOTYPE(DOMTextEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMTextEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMTextEvent", DOMTextEventProto,DOMTextEventProtoFunc,DOMUIEventProto)//Note: no proto in KeyBase

DOMTextEvent::DOMTextEvent(ExecState *exec, DOM::TextEventImpl* ke) :
  DOMKeyEventBase(DOMTextEventProto::self(exec), ke) {}

DOMTextEvent::~DOMTextEvent()
{
}

bool DOMTextEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMTextEvent::getOwnPropertySlot " << propertyName.qstring();
#endif
  return getStaticValueSlot<DOMTextEvent, DOMKeyEventBase>(exec,&DOMTextEventTable,this,propertyName,slot);
}


JSValue *DOMTextEvent::getValueProperty(ExecState *, int token) const
{
  DOM::TextEventImpl& tevent = *impl();
  switch (token) {
  case Data:
    return jsString(tevent.data());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMTextEvent::getValueProperty : " << token;
    return jsUndefined();
  }
}

JSValue *DOMTextEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMTextEvent, thisObj );
  DOM::TextEventImpl& keyEvent = *static_cast<DOMTextEvent *>(thisObj)->impl();
  switch (id) {
    case DOMTextEvent::InitTextEvent:

      keyEvent.initTextEvent(args[0]->toString(exec).domString(), // typeArg
                            args[1]->toBoolean(exec), // canBubbleArg
                            args[2]->toBoolean(exec), // cancelableArg
                            toAbstractView(args[3]), // viewArg
                            args[4]->toString(exec).domString()); // dataArg
      return jsUndefined();
  }
  return jsUndefined();
}
// -------------------------------------------------------------------------
const ClassInfo DOMKeyboardEvent::info = { "KeyboardEvent", &DOMKeyEventBase::info, &DOMKeyboardEventTable, 0 };

/*
@begin DOMKeyboardEventTable 2
  keyIdentifier  DOMKeyboardEvent::KeyIdentifier  DontDelete|ReadOnly
  keyLocation    DOMKeyboardEvent::KeyLocation    DontDelete|ReadOnly
@end
@begin DOMKeyboardEventProtoTable 2
  initKeyboardEvent	DOMKeyboardEvent::InitKeyboardEvent	DontDelete|Function 7
  getModifierState      DOMKeyboardEvent::GetModifierState      DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(DOMKeyboardEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMKeyboardEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMKeyboardEvent",DOMKeyboardEventProto,DOMKeyboardEventProtoFunc, DOMUIEventProto) //Note: no proto in KeyBase

DOMKeyboardEvent::DOMKeyboardEvent(ExecState *exec, DOM::KeyboardEventImpl* ke) :
  DOMKeyEventBase(DOMKeyboardEventProto::self(exec), ke) {}

DOMKeyboardEvent::~DOMKeyboardEvent()
{
}

bool DOMKeyboardEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMKeyboardEvent::getOwnPropertySlot " << propertyName.qstring();
#endif
  return getStaticValueSlot<DOMKeyboardEvent, DOMKeyEventBase>(exec,&DOMKeyboardEventTable,this,propertyName,slot);
}

JSValue* DOMKeyboardEvent::getValueProperty(ExecState *, int token) const
{
  DOM::KeyboardEventImpl* tevent = impl();
  switch (token) {
  case KeyIdentifier:
    return jsString(tevent->keyIdentifier());
  case KeyLocation:
    return jsNumber(tevent->keyLocation());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMKeyboardEvent::getValueProperty : " << token;
    return jsUndefined();
  }
}

JSValue* DOMKeyboardEventProtoFunc::callAsFunction(ExecState *exec, JSObject* thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMKeyboardEvent, thisObj );
  DOM::KeyboardEventImpl* keyEvent = static_cast<DOMKeyboardEvent *>(thisObj)->impl();
  switch (id) {
    case DOMKeyboardEvent::InitKeyboardEvent:
      keyEvent->initKeyboardEvent(args[0]->toString(exec).domString(), // typeArg
                            args[1]->toBoolean(exec), // canBubbleArg
                            args[2]->toBoolean(exec), // cancelableArg
                            toAbstractView(args[3]), // viewArg
                            args[4]->toString(exec).domString(), // keyIdentifierArg
                            args[5]->toInteger(exec),         // keyLocationArg
                            args[6]->toString(exec).domString()); //modifiersList
      break;
    case DOMKeyboardEvent::GetModifierState:
      return jsBoolean(keyEvent->getModifierState(args[0]->toString(exec).domString()));
  }
  return jsUndefined();
}

// -------------------------------------------------------------------------
const ClassInfo KeyboardEventConstructor::info = { "KeyboardEventConstructor", 0, &KeyboardEventConstructorTable, 0 };
/*
@begin KeyboardEventConstructorTable 4
  DOM_KEY_LOCATION_STANDARD  DOM::KeyboardEvent::DOM_KEY_LOCATION_STANDARD DontDelete|ReadOnly
  DOM_KEY_LOCATION_LEFT      DOM::KeyboardEvent::DOM_KEY_LOCATION_LEFT     DontDelete|ReadOnly
  DOM_KEY_LOCATION_RIGHT     DOM::KeyboardEvent::DOM_KEY_LOCATION_RIGHT    DontDelete|ReadOnly
  DOM_KEY_LOCATION_NUMPAD    DOM::KeyboardEvent::DOM_KEY_LOCATION_NUMPAD   DontDelete|ReadOnly
@end
*/
KeyboardEventConstructor::KeyboardEventConstructor(ExecState* exec)
  : DOMObject(exec->lexicalInterpreter()->builtinObjectPrototype())
{}


bool KeyboardEventConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "DOMKeyboardEvent::getOwnPropertySlot " << propertyName.qstring();
#endif
  return getStaticValueSlot<KeyboardEventConstructor, DOMObject>(exec,&KeyboardEventConstructorTable,this,propertyName,slot);
}

JSValue* KeyboardEventConstructor::getValueProperty(ExecState *, int token) const
{
  // We use the token as the value to return directly
  return jsNumber(token);
}

JSValue* KJS::getKeyboardEventConstructor(ExecState *exec)
{
  return cacheGlobalObject<KeyboardEventConstructor>(exec, "[[keyboardEvent.constructor]]");
}


// -------------------------------------------------------------------------
const ClassInfo MutationEventConstructor::info = { "MutationEventConstructor", 0, &MutationEventConstructorTable, 0 };
/*
@begin MutationEventConstructorTable 3
  MODIFICATION	DOM::MutationEvent::MODIFICATION	DontDelete|ReadOnly
  ADDITION	DOM::MutationEvent::ADDITION		DontDelete|ReadOnly
  REMOVAL	DOM::MutationEvent::REMOVAL		DontDelete|ReadOnly
@end
*/
MutationEventConstructor::MutationEventConstructor(ExecState* exec)
  : DOMObject(exec->lexicalInterpreter()->builtinObjectPrototype())
{
}

bool MutationEventConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<MutationEventConstructor, DOMObject>(exec,&MutationEventConstructorTable,this,propertyName,slot);
}

JSValue *MutationEventConstructor::getValueProperty(ExecState *, int token) const
{
  // We use the token as the value to return directly
  return jsNumber(token);
}

JSValue *KJS::getMutationEventConstructor(ExecState *exec)
{
  return cacheGlobalObject<MutationEventConstructor>(exec, "[[mutationEvent.constructor]]");
}

// -------------------------------------------------------------------------

const ClassInfo DOMMutationEvent::info = { "MutationEvent", &DOMEvent::info, &DOMMutationEventTable, 0 };
/*
@begin DOMMutationEventTable 5
  relatedNode	DOMMutationEvent::RelatedNode	DontDelete|ReadOnly
  prevValue	DOMMutationEvent::PrevValue	DontDelete|ReadOnly
  newValue	DOMMutationEvent::NewValue	DontDelete|ReadOnly
  attrName	DOMMutationEvent::AttrName	DontDelete|ReadOnly
  attrChange	DOMMutationEvent::AttrChange	DontDelete|ReadOnly
@end
@begin DOMMutationEventProtoTable 1
  initMutationEvent	DOMMutationEvent::InitMutationEvent	DontDelete|Function 8
@end
*/
KJS_DEFINE_PROTOTYPE(DOMMutationEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMMutationEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMMutationEvent",DOMMutationEventProto,DOMMutationEventProtoFunc,DOMEventProto)

DOMMutationEvent::DOMMutationEvent(ExecState *exec, DOM::MutationEventImpl* me) :
  DOMEvent(DOMMutationEventProto::self(exec), me) {}

DOMMutationEvent::~DOMMutationEvent()
{
}

bool DOMMutationEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMMutationEvent, DOMEvent>(exec,&DOMMutationEventTable,this,propertyName,slot);
}

JSValue *DOMMutationEvent::getValueProperty(ExecState *exec, int token) const
{
  DOM::MutationEventImpl& event = *impl();
  switch (token) {
  case RelatedNode: {
    DOM::Node relatedNode = event.relatedNode();
    return getDOMNode(exec,relatedNode.handle());
  }
  case PrevValue:
    return jsString(event.prevValue());
  case NewValue:
    return jsString(event.newValue());
  case AttrName:
    return jsString(event.attrName());
  case AttrChange:
    return jsNumber((unsigned int)event.attrChange());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMMutationEvent::getValueProperty : " << token;
    return 0;
  }
}

JSValue *DOMMutationEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMMutationEvent, thisObj );
  DOM::MutationEventImpl& mutationEvent = *static_cast<DOMMutationEvent *>(thisObj)->impl();
  switch (id) {
    case DOMMutationEvent::InitMutationEvent:
      mutationEvent.initMutationEvent(args[0]->toString(exec).domString(), // typeArg,
                                      args[1]->toBoolean(exec), // canBubbleArg
                                      args[2]->toBoolean(exec), // cancelableArg
                                      toNode(args[3]), // relatedNodeArg
                                      args[4]->toString(exec).domString(), // prevValueArg
                                      args[5]->toString(exec).domString(), // newValueArg
                                      args[6]->toString(exec).domString(), // attrNameArg
                                      args[7]->toInteger(exec)); // attrChangeArg
      return jsUndefined();
  }
  return jsUndefined();
}
// -------------------------------------------------------------------------

const ClassInfo DOMMessageEvent::info = { "MessageEvent", &DOMEvent::info, &DOMMessageEventTable, 0 };
/*
@begin DOMMessageEventTable 5
  data     DOMMessageEvent::Data     DontDelete|ReadOnly
  origin   DOMMessageEvent::Origin   DontDelete|ReadOnly
  source   DOMMessageEvent::Source   DontDelete|ReadOnly
  lastEventId  DOMMessageEvent::LastEventId   DontDelete|ReadOnly
@end
@begin DOMMessageEventProtoTable 1
  initMessageEvent     DOMMessageEvent::InitMessageEvent     DontDelete|Function 7
@end
*/
KJS_DEFINE_PROTOTYPE(DOMMessageEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMMessageEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMMessageEvent",DOMMessageEventProto,DOMMessageEventProtoFunc,DOMEventProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(MessageEventPseudoCtor, "DOMMessageEvent", DOMMessageEventProto)

DOMMessageEvent::DOMMessageEvent(ExecState *exec, DOM::MessageEventImpl* me) :
  DOMEvent(DOMMessageEventProto::self(exec), me) {}

bool DOMMessageEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMMessageEvent, DOMEvent>(exec,&DOMMessageEventTable,this,propertyName,slot);
}

JSValue *DOMMessageEvent::getValueProperty(ExecState *exec, int token) const
{
  DOM::MessageEventImpl& event = *impl();
  switch (token) {
  case Data:
    return getMessageEventData(exec, event.data().get());
  case Origin:
    return jsString(event.origin());
  case LastEventId:
    return jsString(event.lastEventId());
  case Source: 
    if (KHTMLPart* p = event.source())
	return Window::retrieve(p);
    else
	return jsNull();
  default:
    // qDebug() << "WARNING: Unhandled token in DOMMessageEvent::getValueProperty : " << token;
    return 0;
  }
}

JSValue *DOMMessageEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    KJS_CHECK_THIS( KJS::DOMMessageEvent, thisObj );
    DOM::MessageEventImpl& messageEvent = *static_cast<DOMMessageEvent *>(thisObj)->impl();
    switch (id) {
        case DOMMessageEvent::InitMessageEvent: {
            JSObject* sourceObj = args[3]->getObject();

            Window* sourceWin = 0;
            if (sourceObj && sourceObj->inherits(&Window::info))
                sourceWin = static_cast<Window *>(sourceObj);

            KHTMLPart* part = 0;
            if (sourceWin)
                part = qobject_cast<KHTMLPart*>(sourceWin->part());

            if (!part) {
                setDOMException(exec, DOM::DOMException::TYPE_MISMATCH_ERR);
                return jsUndefined();
            }
        
            messageEvent.initMessageEvent(args[0]->toString(exec).domString(), // typeArg,
                                          args[1]->toBoolean(exec), // canBubbleArg
                                          args[2]->toBoolean(exec), // cancelableArg
                                          encapsulateMessageEventData(
                                               exec, exec->dynamicInterpreter(), args[3]), // dataArg
                                          args[4]->toString(exec).domString(), // originArg
                                          args[5]->toString(exec).domString(), // lastEventIdArg
                                          part); // sourceArg
            return jsUndefined();
        }
    }
    return jsUndefined();
}

// -------------------------------------------------------------------------

const ClassInfo DOMHashChangeEvent::info = { "HashChangeEvent", &DOMEvent::info, &DOMHashChangeEventTable, 0 };
/*
@begin DOMHashChangeEventTable 2
  oldURL   DOMHashChangeEvent::OldUrl DontDelete|ReadOnly
  newURL   DOMHashChangeEvent::NewUrl DontDelete|ReadOnly
@end
@begin DOMHashChangeEventProtoTable 1
  initHashChangeEvent     DOMHashChangeEvent::InitHashChangeEvent     DontDelete|Function 5
@end
*/
KJS_DEFINE_PROTOTYPE(DOMHashChangeEventProto)
KJS_IMPLEMENT_PROTOFUNC(DOMHashChangeEventProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMHashChangeEvent", DOMHashChangeEventProto, DOMHashChangeEventProtoFunc, DOMEventProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HashChangeEventPseudoCtor, "DOMHashChangeEvent", DOMHashChangeEventProto)

DOMHashChangeEvent::DOMHashChangeEvent(ExecState* exec, HashChangeEventImpl* me) :
  DOMEvent(DOMHashChangeEventProto::self(exec), me)
{}

bool DOMHashChangeEvent::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMHashChangeEvent, DOMEvent>(exec,&DOMHashChangeEventTable,this,propertyName,slot);
}

JSValue *DOMHashChangeEvent::getValueProperty(ExecState * /*exec*/, int token) const
{
  DOM::HashChangeEventImpl& event = *impl();
  switch (token) {
  case NewUrl:
    return jsString(event.newUrl());
  case OldUrl:
    return jsString(event.oldUrl());
  default:
    // qDebug() << "WARNING: Unhandled token in DOMHashChangeEvent::getValueProperty : " << token;
    return jsUndefined();
  }
}

JSValue *DOMHashChangeEventProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    KJS_CHECK_THIS( KJS::DOMHashChangeEvent, thisObj );
    DOM::HashChangeEventImpl& hashChangeEvent = *static_cast<DOMHashChangeEvent *>(thisObj)->impl();
    switch (id) {
        case DOMHashChangeEvent::InitHashChangeEvent: {
            hashChangeEvent.initHashChangeEvent(args[0]->toString(exec).domString(), // typeArg,
                                          args[1]->toBoolean(exec), // canBubbleArg
                                          args[2]->toBoolean(exec), // cancelableArg
                                          args[3]->toString(exec).domString(), // oldURL
                                          args[4]->toString(exec).domString()); // newURL
            return jsUndefined();
        }
    }
    return jsUndefined();
}
