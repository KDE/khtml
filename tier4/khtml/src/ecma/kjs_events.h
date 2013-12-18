// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
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

#ifndef _KJS_EVENTS_H_
#define _KJS_EVENTS_H_

#include "ecma/kjs_dom.h"
#include "dom/dom2_events.h"
#include "dom/dom_misc.h"
#include "xml/dom2_eventsimpl.h"

namespace KJS {

  class Window;

  class JSEventListener : public DOM::EventListener {
  public:
    /**
     * @param _listener the function object, that will be called when the event is emitted
     * @param _compareListenerImp Compare Listener implementation.
     * @param _win Window object, for memory management and caching.
     * @param _html \c true if it is HTML. 
     * Never create a JSEventListener directly, use Window::getJSEventListener.
     */
    JSEventListener(JSObject* _listener, JSObject *_compareListenerImp, JSObject* _win, bool _html = false);
    virtual ~JSEventListener();
    virtual void handleEvent(DOM::Event &evt);
    virtual DOM::DOMString eventListenerType();
    // Return the KJS function object executed when this event is emitted
    virtual JSObject* listenerObj() const;
    // for Window::clear(). This is a bad hack though. The JSEventListener might not get deleted
    // if it was added to a DOM node in another frame (#61467). But calling removeEventListener on
    // all nodes we're listening to is quite difficult.
    void clear() { listener = 0; compareListenerImp = 0; }
    bool isHTMLEventListener() const { return html; }

  protected:
    mutable ProtectedPtr<JSObject> listener;
    // Storing a different JSObject ptr is needed to support addEventListener(.. [Object] ..) calls
    // In the real-life case (where a 'function' is passed to addEventListener) we can directly call
    // the 'listener' object and can cache the 'listener.imp()'. If the event listener should be removed
    // the implementation will call removeEventListener(.. [Function] ..), and we can lookup the event
    // listener by the passed function's imp() ptr.
    // In the only dom-approved way (passing an Object to add/removeEventListener), the 'listener'
    // variable stores the function object 'passedListener.handleEvent'. But we need to cache
    // the imp() ptr of the 'passedListener' function _object_, as the implementation will
    // call removeEventListener(.. [Object ..] on removal, and now we can successfully lookup
    // the correct event listener, as well as the 'listener.handleEvent' function, we need to call.
    mutable ProtectedPtr<JSObject> compareListenerImp;
    bool html;
    mutable ProtectedPtr<JSObject> win;
  };

  class JSLazyEventListener : public JSEventListener {
  public:
    JSLazyEventListener(const QString &_code, const QString &_url, int _lineNum,
                        const QString &_name, JSObject* _win, DOM::NodeImpl* node, bool _svg = false);
    ~JSLazyEventListener();
    virtual void handleEvent(DOM::Event &evt);
    JSObject* listenerObj() const;
  private:
    void parseCode() const;

    mutable QString code;
    mutable QString url;
    int     lineNum;

    mutable QString name;
    mutable bool parsed;
    DOM::NodeImpl *originalNode;
    bool svg;
  };

  // Constructor for Event - currently only used for some global vars
  DEFINE_PSEUDO_CONSTRUCTOR(EventConstructor)

  class DOMEvent : public DOMObject {
  public:
    // Build a DOMEvent
    DOMEvent(ExecState *exec, DOM::EventImpl* e);
    DOMEvent(JSObject *proto, DOM::EventImpl* e);
    ~DOMEvent();

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName,
			JSValue* value, int attr = None);
    virtual JSValue* defaultValue(ExecState *exec, KJS::JSType hint) const;
    void putValueProperty(ExecState *exec, int token, JSValue* value, int);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Type, Target, CurrentTarget, EventPhase, Bubbles,
           Cancelable, TimeStamp, StopPropagation, PreventDefault, InitEvent,
	   // MS IE equivalents
	   SrcElement, ReturnValue, CancelBubble };
    DOM::EventImpl* impl() const { return m_impl.get(); }
  protected:
    SharedPtr<DOM::EventImpl> m_impl;
  };

  JSValue* getDOMEvent(ExecState *exec, DOM::EventImpl* e);

  /**
   * Convert an object to an Event. Returns a null Event if not possible.
   */
  DOM::EventImpl* toEvent(JSValue*);

  // Constructor object EventException
  class EventExceptionConstructor : public DOMObject {
  public:
    EventExceptionConstructor(ExecState *);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  JSValue* getEventExceptionConstructor(ExecState *exec);

  class DOMUIEvent : public DOMEvent {
  public:
    // Build a DOMUIEvent
    DOMUIEvent(ExecState *exec, DOM::UIEventImpl* ue);
    DOMUIEvent(JSObject *proto, DOM::UIEventImpl* ue);
    ~DOMUIEvent();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { View, Detail, KeyCode, CharCode, LayerX, LayerY, PageX, PageY, Which, InitUIEvent };
    DOM::UIEventImpl* impl() const { return static_cast<DOM::UIEventImpl*>(m_impl.get()); }
  };

  class DOMMouseEvent : public DOMUIEvent {
  public:
    DOMMouseEvent(ExecState *exec, DOM::MouseEventImpl* me);
    ~DOMMouseEvent();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { ScreenX, ScreenY, ClientX, X, ClientY, Y, OffsetX, OffsetY,
           CtrlKey, ShiftKey, AltKey,
           MetaKey, Button, RelatedTarget, FromElement, ToElement,
           InitMouseEvent
    };
    DOM::MouseEventImpl* impl() const { return static_cast<DOM::MouseEventImpl*>(m_impl.get()); }
  };

  class DOMKeyEventBase : public DOMUIEvent {
  public:
    DOMKeyEventBase(JSObject* proto, DOM::KeyEventBaseImpl* ke);
    ~DOMKeyEventBase();

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Key, VirtKey, CtrlKey, ShiftKey, AltKey, MetaKey };
    DOM::KeyEventBaseImpl* impl() const { return static_cast<DOM::KeyEventBaseImpl*>(m_impl.get()); }
  };

  class DOMTextEvent : public DOMKeyEventBase {
  public:
    DOMTextEvent(ExecState *exec, DOM::TextEventImpl* ke);
    ~DOMTextEvent();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum {Data, InitTextEvent};
    DOM::TextEventImpl* impl() const { return static_cast<DOM::TextEventImpl*>(m_impl.get()); }
  };

  class DOMKeyboardEvent : public DOMKeyEventBase {
  public:
    DOMKeyboardEvent(ExecState *exec, DOM::KeyboardEventImpl* ke);
    ~DOMKeyboardEvent();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum {KeyIdentifier, KeyLocation, GetModifierState, InitKeyboardEvent};
    DOM::KeyboardEventImpl* impl() const { return static_cast<DOM::KeyboardEventImpl*>(m_impl.get()); }
  };

  // Constructor object KeyboardEvent
  class KeyboardEventConstructor : public DOMObject {
  public:
    KeyboardEventConstructor(ExecState *);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  JSValue* getKeyboardEventConstructor(ExecState *exec);

  // Constructor object MutationEvent
  class MutationEventConstructor : public DOMObject {
  public:
    MutationEventConstructor(ExecState *);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  JSValue* getMutationEventConstructor(ExecState *exec);

  class DOMMutationEvent : public DOMEvent {
  public:
    DOMMutationEvent(ExecState *exec, DOM::MutationEventImpl* me);
    ~DOMMutationEvent();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { AttrChange, RelatedNode, AttrName, PrevValue, NewValue,
           InitMutationEvent };
    DOM::MutationEventImpl* impl() const { return static_cast<DOM::MutationEventImpl*>(m_impl.get()); }
  };

  class DOMMessageEvent: public DOMEvent {
  public:
    DOMMessageEvent(ExecState* exec, DOM::MessageEventImpl* me);
    
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Data, Origin, Source, LastEventId, InitMessageEvent };
    DOM::MessageEventImpl* impl() const { return static_cast<DOM::MessageEventImpl*>(m_impl.get()); }
  };

  DEFINE_PSEUDO_CONSTRUCTOR(MessageEventPseudoCtor)

  class DOMHashChangeEvent : public DOMEvent {
  public:
    DOMHashChangeEvent(ExecState* exec, DOM::HashChangeEventImpl* me);

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { NewUrl, OldUrl, InitHashChangeEvent };
    DOM::HashChangeEventImpl* impl() const { return static_cast<DOM::HashChangeEventImpl*>(m_impl.get()); }
  };
  DEFINE_PSEUDO_CONSTRUCTOR(HashChangeEventPseudoCtor)

} // namespace

#endif
