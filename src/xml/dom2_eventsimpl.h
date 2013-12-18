/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 *           (C) 2002 Apple Computer, Inc.
 *           (C) 2010 Maksim Orlovich (maksim@kde.org)
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
 *
 */

#ifndef _DOM_EventsImpl_h_
#define _DOM_EventsImpl_h_

#include "dom/dom2_events.h"
#include "xml/dom2_viewsimpl.h"
#include "misc/idstring.h"
#include "misc/enum.h"
#include <QDateTime>
#include <QWeakPointer>

#undef FOCUS_EVENT //for win32

class QMouseEvent;
class QKeyEvent;
class KHTMLPart;

namespace DOM {

class AbstractViewImpl;
class DOMStringImpl;
class NodeImpl;
class DocumentImpl;

class RegisteredEventListener {
public:
    RegisteredEventListener() : useCapture(false), listener(0) {}

    RegisteredEventListener(EventName _id, EventListener *_listener, bool _useCapture)
        : eventName(_id), useCapture(_useCapture), listener(_listener) { if (listener) listener->ref(); }

    ~RegisteredEventListener() { if (listener) listener->deref(); listener = 0; }

    bool operator==(const RegisteredEventListener &other) const
    { return eventName == other.eventName && listener == other.listener
             && useCapture == other.useCapture; }


    EventName eventName;
    bool useCapture;
    EventListener *listener;

    RegisteredEventListener( const RegisteredEventListener &other ) :
                eventName(other.eventName), useCapture(other.useCapture), listener(other.listener)
    { if (listener) listener->ref(); }

    RegisteredEventListener & operator=( const RegisteredEventListener &other ) {
        eventName  = other.eventName;
        useCapture = other.useCapture;
        if (other.listener)
            other.listener->ref();
        if (listener)
            listener->deref();
        listener = other.listener;
        return *this;
    }
};


struct RegisteredListenerList {
    RegisteredListenerList() : listeners(0)
    {}

    ~RegisteredListenerList();

    void addEventListener(EventName id, EventListener *listener, const bool useCapture);
    void removeEventListener(EventName id, EventListener *listener, bool useCapture);

    void setHTMLEventListener(EventName id, EventListener *listener);
    EventListener *getHTMLEventListener(EventName id);

    bool hasEventListener(EventName id);
    void clear();

    //### KDE4: should disappear
    bool stillContainsListener(const RegisteredEventListener& listener);

    QList<RegisteredEventListener>* listeners;//The actual listener list - may be 0
private:
    bool isHTMLEventListener(EventListener* listener);
};

class EventTargetImpl : public khtml::TreeShared<EventTargetImpl>
{
public:
    enum Type {
        DOM_NODE,
        WINDOW,
        XML_HTTP_REQUEST
    };

    virtual Type eventTargetType() const = 0;

    /* Override this to provide access to associated document in order to
     * set appropriate 'has listerner' flags
     */
    virtual DocumentImpl* eventTargetDocument();

    /**
     * Perform the default action for an event e.g. submitting a form
     */
    virtual void defaultEventHandler(EventImpl *evt);

    /*
     * This method fires all the registered handlers for this event
     * (checking the capture flag as well(
     */
    void handleLocalEvents(EventImpl *evt, bool useCapture);

    void addEventListener(EventName id, EventListener *listener, const bool useCapture);
    void removeEventListener(EventName id, EventListener *listener, bool useCapture);
    void setHTMLEventListener(EventName id, EventListener *listener);
    void setHTMLEventListener(unsigned id, EventListener *listener);
    EventListener *getHTMLEventListener(EventName id);
    EventListener *getHTMLEventListener(unsigned id);

    RegisteredListenerList& listenerList() { return m_regdListeners; }
private:
    void setDocListenerFlag(unsigned flag);
    RegisteredListenerList m_regdListeners;
};

class EventImpl : public khtml::Shared<EventImpl>
{
public:
    enum EventId {
       // UI events
        DOMFOCUSIN_EVENT,
        DOMFOCUSOUT_EVENT,
        DOMACTIVATE_EVENT,
        // Mouse events
        CLICK_EVENT,
        MOUSEDOWN_EVENT,
        MOUSEUP_EVENT,
        MOUSEOVER_EVENT,
        MOUSEMOVE_EVENT,
        MOUSEOUT_EVENT,
        // Mutation events
        DOMSUBTREEMODIFIED_EVENT,
        DOMNODEINSERTED_EVENT,
        DOMNODEREMOVED_EVENT,
        DOMNODEREMOVEDFROMDOCUMENT_EVENT,
        DOMNODEINSERTEDINTODOCUMENT_EVENT,
        DOMATTRMODIFIED_EVENT,
        DOMCHARACTERDATAMODIFIED_EVENT,
        // HTML events
        LOAD_EVENT,
        UNLOAD_EVENT,
        ABORT_EVENT,
        ERROR_EVENT,
        SELECT_EVENT,
        CHANGE_EVENT,
        SUBMIT_EVENT,
        RESET_EVENT,
        FOCUS_EVENT,
        BLUR_EVENT,
        RESIZE_EVENT,
        SCROLL_EVENT,
        HASHCHANGE_EVENT,
            // keyboard events
        KEYDOWN_EVENT,
        KEYUP_EVENT,
        KEYPRESS_EVENT, //Mostly corresponds to DOM3 textInput event.
        // khtml events (not part of DOM)
        KHTML_ECMA_DBLCLICK_EVENT, // for html ondblclick
        KHTML_ECMA_CLICK_EVENT, // for html onclick
        KHTML_DRAGDROP_EVENT,
        KHTML_MOVE_EVENT,
        KHTML_MOUSEWHEEL_EVENT,
        KHTML_CONTENTLOADED_EVENT,
        // XMLHttpRequest events
        KHTML_READYSTATECHANGE_EVENT,
        // HTML5 events
        MESSAGE_EVENT
    };

    EventImpl();
    EventImpl(EventId id, bool canBubbleArg, bool cancelableArg);
    virtual ~EventImpl();

    EventId id() const { return EventId(m_eventName.id()); }
    DOMString type() const { return m_eventName.toString(); }
    EventName name() const { return m_eventName; }

    EventTargetImpl *target() const { return m_target; }
    void setTarget(EventTargetImpl *_target);
    EventTargetImpl *currentTarget() const { return m_currentTarget; }
    void setCurrentTarget(EventTargetImpl *_currentTarget) { m_currentTarget = _currentTarget; }
    unsigned short eventPhase() const { return m_eventPhase; }
    void setEventPhase(unsigned short _eventPhase) { m_eventPhase = _eventPhase; }
    bool bubbles() const { return m_canBubble; }
    bool cancelable() const { return m_cancelable; }
    DOMTimeStamp timeStamp();
    void stopPropagation(bool stop) { m_propagationStopped = stop; }
    void preventDefault(bool prevent) { if ( m_cancelable ) m_defaultPrevented = prevent; }

    void initEvent(const DOMString &eventTypeArg, bool canBubbleArg, bool cancelableArg);

    virtual bool isUIEvent() const;
    virtual bool isMouseEvent() const;
    virtual bool isMutationEvent() const;
    virtual bool isTextInputEvent() const;
    virtual bool isKeyboardEvent() const;
    virtual bool isMessageEvent() const;
    virtual bool isHashChangeEvent() const;
    bool isKeyRelatedEvent() const { return isTextInputEvent() || isKeyboardEvent(); }

    bool propagationStopped() const { return m_propagationStopped; }
    bool defaultPrevented() const { return m_defaultPrevented; }

    void setDefaultHandled() { m_defaultHandled = true; }
    bool defaultHandled() const { return m_defaultHandled; }

    DOMString message() const { return m_message; }
    void setMessage(const DOMString &_message) { m_message = _message; }

    static khtml::IDTable<EventImpl>* idTable() {
        if (s_idTable)
            return s_idTable;
        else
            return initIdTable();
    }
protected:
    static khtml::IDTable<EventImpl>* s_idTable;
    static khtml::IDTable<EventImpl>* initIdTable();
    EventName m_eventName;
    bool m_canBubble  : 1;
    bool m_cancelable : 1;

    bool m_propagationStopped : 1;
    bool m_defaultPrevented   : 1;
    bool m_defaultHandled     : 1;
    unsigned short m_eventPhase : 2;
    EventTargetImpl *m_currentTarget; // ref > 0 maintained externally
    EventTargetImpl *m_target;
    QDateTime m_createTime;
    DOMString m_message;
};



class UIEventImpl : public EventImpl
{
public:
    UIEventImpl() : m_view(0), m_detail(0) {}
    UIEventImpl(EventId _id,
		bool canBubbleArg,
		bool cancelableArg,
		AbstractViewImpl *viewArg,
		long detailArg);
    virtual ~UIEventImpl();
    AbstractViewImpl *view() const { return m_view; }
    long detail() const { return m_detail; }
    void initUIEvent(const DOMString &typeArg,
		     bool canBubbleArg,
		     bool cancelableArg,
		     AbstractViewImpl* viewArg,
		     long detailArg);
    virtual bool isUIEvent() const;

    //Compat stuff
    virtual int keyCode() const  { return 0; }
    virtual int charCode() const { return 0; }

    virtual long pageX() const { return 0; }
    virtual long pageY() const { return 0; }
    virtual long layerX() const { return 0; }
    virtual long layerY() const { return 0; }
    virtual int which() const { return 0; }
protected:
    AbstractViewImpl *m_view;
    long m_detail;

};

// Introduced in DOM Level 2: - internal
class MouseEventImpl : public UIEventImpl {
public:
    enum Orientation {
        ONone = 0,
        OHorizontal,
        OVertical
    };

    MouseEventImpl();
    MouseEventImpl(EventId _id,
		   bool canBubbleArg,
		   bool cancelableArg,
		   AbstractViewImpl *viewArg,
		   long detailArg,
		   long screenXArg,
		   long screenYArg,
		   long clientXArg,
		   long clientYArg,
                   long pageXArg,
                   long pageYArg,
		   bool ctrlKeyArg,
		   bool altKeyArg,
		   bool shiftKeyArg,
		   bool metaKeyArg,
		   unsigned short buttonArg,
		   NodeImpl *relatedTargetArg,
		   QMouseEvent *qe = 0,
                   bool isDoubleClick = false,
                   Orientation orient = ONone);
    virtual ~MouseEventImpl();
    long screenX() const { return m_screenX; }
    long screenY() const { return m_screenY; }
    long clientX() const { return m_clientX; }
    long clientY() const { return m_clientY; }
    long layerX() const { return m_layerX; } // non-DOM extension
    long layerY() const { return m_layerY; } // non-DOM extension
    long pageX() const { return m_pageX; } // non-DOM extension
    long pageY() const { return m_pageY; } // non-DOM extension
    virtual int which() const { return button() + 1; } // non-DOM extension
    bool isDoubleClick() const { return m_isDoubleClick; } // non-DOM extension
    Orientation orientation() const { return KDE_CAST_BF_ENUM(Orientation, m_orientation); } // non-DOM extension
    bool ctrlKey() const { return m_ctrlKey; }
    bool shiftKey() const { return m_shiftKey; }
    bool altKey() const { return m_altKey; }
    bool metaKey() const { return m_metaKey; }
    unsigned short button() const { return m_button; }
    NodeImpl *relatedTarget() const { return m_relatedTarget; }

    void computeLayerPos();

    void initMouseEvent(const DOMString &typeArg,
			bool canBubbleArg,
			bool cancelableArg,
			AbstractViewImpl* viewArg,
			long detailArg,
			long screenXArg,
			long screenYArg,
			long clientXArg,
			long clientYArg,
			bool ctrlKeyArg,
			bool altKeyArg,
			bool shiftKeyArg,
			bool metaKeyArg,
			unsigned short buttonArg,
			const Node &relatedTargetArg,
			Orientation orient = ONone);
    virtual bool isMouseEvent() const;

    QMouseEvent *qEvent() const { return m_qevent; }
protected:
    long m_screenX;
    long m_screenY;
    long m_clientX;
    long m_clientY;
    long m_layerX;
    long m_layerY;
    long m_pageX;
    long m_pageY;
    bool m_ctrlKey : 1;
    bool m_altKey  : 1;
    bool m_shiftKey : 1;
    bool m_metaKey : 1;
    bool m_isDoubleClick : 1;
    KDE_BF_ENUM(Orientation) m_orientation : 2;
    unsigned short m_button;
    NodeImpl *m_relatedTarget;
    QMouseEvent *m_qevent;
};

class KeyEventBaseImpl : public UIEventImpl {
public:
  // VirtualKeyCode
  enum KeyCodes  {
      DOM_VK_UNDEFINED		     = 0x0,
      DOM_VK_RIGHT_ALT		     = 0x12,
      DOM_VK_LEFT_ALT		     = 0x12,
      DOM_VK_LEFT_CONTROL	     = 0x11,
      DOM_VK_RIGHT_CONTROL	     = 0x11,
      DOM_VK_LEFT_SHIFT		     = 0x10,
      DOM_VK_RIGHT_SHIFT	     = 0x10,
      DOM_VK_META		     = 0x9D,
      DOM_VK_BACK_SPACE		     = 0x08,
      DOM_VK_CAPS_LOCK		     = 0x14,
      DOM_VK_INSERT		     = 0x2D,
      DOM_VK_DELETE		     = 0x7F,
      DOM_VK_END		     = 0x23,
      DOM_VK_ENTER		     = 0x0D,
      DOM_VK_ESCAPE		     = 0x1B,
      DOM_VK_HOME		     = 0x24,
      DOM_VK_NUM_LOCK		     = 0x90,
      DOM_VK_PAUSE		     = 0x13,
      DOM_VK_PRINTSCREEN	     = 0x9A,
      DOM_VK_SCROLL_LOCK	     = 0x91,
      DOM_VK_SPACE		     = 0x20,
      DOM_VK_TAB		     = 0x09,
      DOM_VK_LEFT		     = 0x25,
      DOM_VK_RIGHT		     = 0x27,
      DOM_VK_UP			     = 0x26,
      DOM_VK_DOWN		     = 0x28,
      DOM_VK_PAGE_DOWN		     = 0x22,
      DOM_VK_PAGE_UP		     = 0x21,
      DOM_VK_F1			     = 0x70,
      DOM_VK_F2			     = 0x71,
      DOM_VK_F3			     = 0x72,
      DOM_VK_F4			     = 0x73,
      DOM_VK_F5			     = 0x74,
      DOM_VK_F6			     = 0x75,
      DOM_VK_F7			     = 0x76,
      DOM_VK_F8			     = 0x77,
      DOM_VK_F9			     = 0x78,
      DOM_VK_F10		     = 0x79,
      DOM_VK_F11		     = 0x7A,
      DOM_VK_F12		     = 0x7B,
      DOM_VK_F13		     = 0xF000,
      DOM_VK_F14		     = 0xF001,
      DOM_VK_F15		     = 0xF002,
      DOM_VK_F16		     = 0xF003,
      DOM_VK_F17		     = 0xF004,
      DOM_VK_F18		     = 0xF005,
      DOM_VK_F19		     = 0xF006,
      DOM_VK_F20		     = 0xF007,
      DOM_VK_F21		     = 0xF008,
      DOM_VK_F22		     = 0xF009,
      DOM_VK_F23		     = 0xF00A,
      DOM_VK_F24		     = 0xF00B
  };

  void initKeyBaseEvent(const DOMString &typeArg,
                         bool canBubbleArg,
                         bool cancelableArg,
                         AbstractViewImpl* viewArg,
                         unsigned long keyVal,
                         unsigned long virtKeyVal,
                         unsigned long modifiers);

  bool ctrlKey()  const { return m_modifier & Qt::ControlModifier; }
  bool shiftKey() const { return m_modifier & Qt::ShiftModifier; }
  bool altKey()   const { return m_modifier & Qt::AltModifier; }
  bool metaKey()  const { return m_modifier & Qt::MetaModifier; }

  bool             inputGenerated() const { return m_virtKeyVal == 0; }
  unsigned long    keyVal() const     { return m_keyVal; }
  unsigned long    virtKeyVal() const { return m_virtKeyVal; }

  QKeyEvent *qKeyEvent() const { if (!m_keyEvent) buildQKeyEvent(); return m_keyEvent; }

  ~KeyEventBaseImpl();

  bool checkModifier(unsigned long modifierArg);

  virtual int which() const { return keyCode(); } // non-DOM extension

  //Returns true if the event was synthesized by client use of DOM
  bool isSynthetic() const { return m_synthetic; }
protected:
  KeyEventBaseImpl(): m_keyEvent(0), m_keyVal(0), m_virtKeyVal(0), m_modifier(0), m_synthetic(false)
  {  m_detail = 0; }

  KeyEventBaseImpl(EventId id,
                   bool canBubbleArg,
                   bool cancelableArg,
                   AbstractViewImpl *viewArg,
                   QKeyEvent *key);


  mutable QKeyEvent *m_keyEvent;
  unsigned long m_keyVal;     //Unicode key value
  unsigned long m_virtKeyVal; //Virtual key value for keys like arrows, Fn, etc.

  // bitfield containing state of modifiers. not part of the dom.
  unsigned long    m_modifier;

  bool             m_synthetic;

  void buildQKeyEvent() const; //Construct a Qt key event from m_keyVal/m_virtKeyVal
};

class TextEventImpl : public KeyEventBaseImpl {
public:
    TextEventImpl();

    TextEventImpl(QKeyEvent* key, DOM::AbstractViewImpl* view);

    void initTextEvent(const DOMString &typeArg,
                       bool canBubbleArg,
                       bool cancelableArg,
                       AbstractViewImpl* viewArg,
                       const DOMString& text);

    virtual bool isTextInputEvent() const;

    //Legacy key stuff...
    virtual int keyCode() const;
    virtual int charCode() const;

    DOMString data() const { return m_outputString; }
private:
    DOMString m_outputString;
};

class KeyboardEventImpl : public KeyEventBaseImpl {
public:
  KeyboardEventImpl();
  KeyboardEventImpl(QKeyEvent* key, DOM::AbstractViewImpl* view);

  virtual bool isKeyboardEvent() const;

  //Legacy key stuff...
  int keyCode() const;
  int charCode() const;

  DOMString     keyIdentifier() const;
  unsigned long keyLocation() const { return m_keyLocation; }

  bool getModifierState(const DOMString& keyIdentifierArg) const;

  void initKeyboardEvent(const DOMString &typeArg,
                         bool canBubbleArg,
                         bool cancelableArg,
                         AbstractViewImpl* viewArg,
                         const DOMString &keyIdentifierArg,
                         unsigned long keyLocationArg,
                         const DOMString& modifiersList);

private:
    unsigned long m_keyLocation;
};


class MutationEventImpl : public EventImpl {
// ### fire these during parsing (if necessary)
public:
    MutationEventImpl();
    MutationEventImpl(EventId _id, /* for convenience */
		      bool canBubbleArg,
		      bool cancelableArg,
		      const Node &relatedNodeArg,
		      const DOMString &prevValueArg,
		      const DOMString &newValueArg,
		      const DOMString &attrNameArg,
		      unsigned short attrChangeArg);
    ~MutationEventImpl();

    Node relatedNode() const { return m_relatedNode; }
    DOMString prevValue() const { return m_prevValue; }
    DOMString newValue() const { return m_newValue; }
    DOMString attrName() const { return m_attrName; }
    unsigned short attrChange() const { return m_attrChange; }
    void initMutationEvent(const DOMString &typeArg,
			   bool canBubbleArg,
			   bool cancelableArg,
			   const Node &relatedNodeArg,
			   const DOMString &prevValueArg,
			   const DOMString &newValueArg,
			   const DOMString &attrNameArg,
			   unsigned short attrChangeArg);
    virtual bool isMutationEvent() const;
protected:
    NodeImpl *m_relatedNode;
    DOMStringImpl *m_prevValue;
    DOMStringImpl *m_newValue;
    DOMStringImpl *m_attrName;
    unsigned short m_attrChange;
};

class MessageEventImpl : public EventImpl {
public:
    enum DataType {
        JS_VALUE
    };

    class Data : public khtml::Shared<Data> {
    public:
        virtual DataType messageDataType() const = 0;
        virtual ~Data() {}
    };

    RefPtr<Data> data() const { return m_data; }
    DOMString  origin() const { return m_origin; }
    KHTMLPart* source() const { return m_source.data(); }
    DOMString  lastEventId() const { return m_lastEventId; }

    MessageEventImpl();

    void initMessageEvent(const DOMString &eventTypeArg,
                          bool  canBubbleArg,
                          bool  cancelableArg,
                          const RefPtr<Data>& dataArg,
                          const DOMString& originArg,
                          const DOMString& lastEventIdArg,
                          KHTMLPart* sourceArg); // no message ports yet.
    virtual bool isMessageEvent() const;
private:
    RefPtr<Data> m_data;
    DOMString    m_origin;
    DOMString    m_lastEventId;
    QWeakPointer<KHTMLPart>  m_source;
};


class HashChangeEventImpl : public EventImpl {
public:
    const DOMString &oldUrl() const { return m_oldUrl; }
    const DOMString &newUrl() const { return m_newUrl; }

    HashChangeEventImpl();

    void initHashChangeEvent(const DOMString &eventTypeArg,
                          bool  canBubbleArg,
                          bool  cancelableArg,
                          const DOMString &oldUrl,
                          const DOMString &newUrl
                         );
    virtual bool isHashChangeEvent() const;
private:
    DOMString    m_oldUrl;
    DOMString    m_newUrl;
};

} //namespace
#endif
