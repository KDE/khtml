/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 *           (C) 2003 Apple Computer, Inc.
 *           (C) 2006, 2010 Maksim Orlovich (maksim@kde.org)
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

 */

#include "dom2_eventsimpl.h"

#include <dom/dom2_views.h>

#include "dom_stringimpl.h"
#include "dom_nodeimpl.h"
#include "dom_docimpl.h"
#include "misc/translator.h"
#include <rendering/render_layer.h>
#include <khtmlview.h>
#include <khtml_part.h>

#include <QActionEvent>
#include <QDebug>

using namespace DOM;
using namespace khtml;

void EventTargetImpl::handleLocalEvents(EventImpl *evt, bool useCapture)
{
    if (!m_regdListeners.listeners)
        return;

    Event ev = evt;
    // removeEventListener (e.g. called from a JS event listener) might
    // invalidate the item after the current iterator (which "it" is pointing to).
    // So we make a copy of the list.
    QList<RegisteredEventListener> listeners = *m_regdListeners.listeners;
    QList<RegisteredEventListener>::iterator it;
    for (it = listeners.begin(); it != listeners.end(); ++it) {
        //Check whether this got removed...KDE4: use Java-style iterators
        if (!m_regdListeners.stillContainsListener(*it))
            continue;

        RegisteredEventListener& current = (*it);
        if (current.eventName == evt->name() && current.useCapture == useCapture)
            current.listener->handleEvent(ev);

        // ECMA legacy hack
        if (current.useCapture == useCapture && evt->id() == EventImpl::CLICK_EVENT) {
            MouseEventImpl* me = static_cast<MouseEventImpl*>(evt);
            if (me->button() == 0) {
                // To find whether to call onclick or ondblclick, we can't
                // * use me->detail(), it's 2 when clicking twice w/o moving, even very slowly
                // * use me->qEvent(), it's not available when using initMouseEvent/dispatchEvent
                // So we currently store a bool in MouseEventImpl. If anyone needs to trigger
                // dblclicks from the DOM API, we'll need a timer here (well in the doc).
                if ( ( !me->isDoubleClick() && current.eventName.id() == EventImpl::KHTML_ECMA_CLICK_EVENT) ||
                  ( me->isDoubleClick() && current.eventName.id() == EventImpl::KHTML_ECMA_DBLCLICK_EVENT) )
                    current.listener->handleEvent(ev);
            }
        }
    }
}

void EventTargetImpl::defaultEventHandler(EventImpl*)
{}

DocumentImpl* EventTargetImpl::eventTargetDocument()
{
    return 0;
}

void EventTargetImpl::setDocListenerFlag(unsigned flag)
{
    DocumentImpl* doc = eventTargetDocument();
    if (doc)
        doc->addListenerType(DocumentImpl::ListenerType(flag));
}

void EventTargetImpl::addEventListener(EventName id, EventListener *listener, const bool useCapture)
{
    switch (id.id()) {
    case EventImpl::DOMSUBTREEMODIFIED_EVENT:
        setDocListenerFlag(DocumentImpl::DOMSUBTREEMODIFIED_LISTENER);
        break;
    case EventImpl::DOMNODEINSERTED_EVENT:
        setDocListenerFlag(DocumentImpl::DOMNODEINSERTED_LISTENER);
        break;
    case EventImpl::DOMNODEREMOVED_EVENT:
        setDocListenerFlag(DocumentImpl::DOMNODEREMOVED_LISTENER);
        break;
    case EventImpl::DOMNODEREMOVEDFROMDOCUMENT_EVENT:
        setDocListenerFlag(DocumentImpl::DOMNODEREMOVEDFROMDOCUMENT_LISTENER);
        break;
    case EventImpl::DOMNODEINSERTEDINTODOCUMENT_EVENT:
        setDocListenerFlag(DocumentImpl::DOMNODEINSERTEDINTODOCUMENT_LISTENER);
        break;
    case EventImpl::DOMATTRMODIFIED_EVENT:
        setDocListenerFlag(DocumentImpl::DOMATTRMODIFIED_LISTENER);
        break;
    case EventImpl::DOMCHARACTERDATAMODIFIED_EVENT:
        setDocListenerFlag(DocumentImpl::DOMCHARACTERDATAMODIFIED_LISTENER);
        break;
    default:
        break;
    }

    m_regdListeners.addEventListener(id, listener, useCapture);
}

void EventTargetImpl::removeEventListener(EventName id, EventListener *listener, bool useCapture)
{
    m_regdListeners.removeEventListener(id, listener, useCapture);
}

void EventTargetImpl::setHTMLEventListener(EventName id, EventListener *listener)
{
    m_regdListeners.setHTMLEventListener(id, listener);
}

void EventTargetImpl::setHTMLEventListener(unsigned id, EventListener *listener)
{
    m_regdListeners.setHTMLEventListener(EventName::fromId(id), listener);
}

EventListener* EventTargetImpl::getHTMLEventListener(EventName id)
{
    return m_regdListeners.getHTMLEventListener(id);
}

EventListener* EventTargetImpl::getHTMLEventListener(unsigned id)
{
    return m_regdListeners.getHTMLEventListener(EventName::fromId(id));
}
// -----------------------------------------------------------------------------

void RegisteredListenerList::addEventListener(EventName id, EventListener *listener, const bool useCapture)
{
    if (!listener)
    return;
    RegisteredEventListener rl(id,listener,useCapture);
    if (!listeners)
        listeners = new QList<RegisteredEventListener>;

    // if this id/listener/useCapture combination is already registered, do nothing.
    // the DOM2 spec says that "duplicate instances are discarded", and this keeps
    // the listener order intact.
    QList<RegisteredEventListener>::iterator it;
    for (it = listeners->begin(); it != listeners->end(); ++it)
        if (*it == rl)
            return;

    listeners->append(rl);
}

void RegisteredListenerList::removeEventListener(EventName id, EventListener *listener, bool useCapture)
{
    if (!listeners) // nothing to remove
        return;

    RegisteredEventListener rl(id,listener,useCapture);

    QList<RegisteredEventListener>::iterator it;
    for (it = listeners->begin(); it != listeners->end(); ++it)
        if (*it == rl) {
            listeners->erase(it);
            return;
        }
}

bool RegisteredListenerList::isHTMLEventListener(EventListener* listener)
{
    return (listener->eventListenerType() == "_khtml_HTMLEventListener");
}

void RegisteredListenerList::setHTMLEventListener(EventName name, EventListener *listener)
{
    if (!listeners)
        listeners = new QList<RegisteredEventListener>;

    QList<RegisteredEventListener>::iterator it;
    if (!listener) {
        for (it = listeners->begin(); it != listeners->end(); ++it) {
            if ((*it).eventName == name && isHTMLEventListener((*it).listener)) {
                listeners->erase(it);
                break;
            }
        }
        return;
    }

    // if this event already has a registered handler, insert the new one in
    // place of the old one, to preserve the order.
    RegisteredEventListener rl(name,listener,false);

    for (int i = 0; i < listeners->size(); ++i) {
        const RegisteredEventListener& listener = listeners->at(i);
        if (listener.eventName == name && isHTMLEventListener(listener.listener)) {
            listeners->replace(i, rl);
            return;
        }
    }

    listeners->append(rl);
}

EventListener *RegisteredListenerList::getHTMLEventListener(EventName name)
{
    if (!listeners)
        return 0;

    QList<RegisteredEventListener>::iterator it;
    for (it = listeners->begin(); it != listeners->end(); ++it)
        if ((*it).eventName == name && isHTMLEventListener((*it).listener)) {
            return (*it).listener;
        }
    return 0;
}

bool RegisteredListenerList::hasEventListener(EventName name)
{
    if (!listeners)
        return false;

    QList<RegisteredEventListener>::iterator it;
    for (it = listeners->begin(); it != listeners->end(); ++it)
        if ((*it).eventName == name)
            return true;

    return false;
}

void RegisteredListenerList::clear()
{
    delete listeners;
    listeners = 0;
}

bool RegisteredListenerList::stillContainsListener(const RegisteredEventListener& listener)
{
    if (!listeners)
        return false;
    return listeners->contains(listener);
}

RegisteredListenerList::~RegisteredListenerList() {
    delete listeners; listeners = 0;
}

// -----------------------------------------------------------------------------

EventImpl::EventImpl()
{
    m_canBubble = false;
    m_cancelable = false;

    m_propagationStopped = false;
    m_defaultPrevented = false;
    m_currentTarget = 0;
    m_eventPhase = 0;
    m_target = 0;
    m_createTime = QDateTime::currentDateTime();
    m_defaultHandled = false;
}

EventImpl::EventImpl(EventId _id, bool canBubbleArg, bool cancelableArg)
{
    m_eventName = EventName::fromId(_id);
    m_canBubble = canBubbleArg;
    m_cancelable = cancelableArg;

    m_propagationStopped = false;
    m_defaultPrevented = false;
    m_currentTarget = 0;
    m_eventPhase = 0;
    m_target = 0;
    m_createTime = QDateTime::currentDateTime();
    m_defaultHandled = false;
}

EventImpl::~EventImpl()
{
    if (m_target)
        m_target->deref();
}

void EventImpl::setTarget(EventTargetImpl *_target)
{
    if (m_target)
        m_target->deref();
    m_target = _target;
    if (m_target)
        m_target->ref();
}

DOMTimeStamp EventImpl::timeStamp()
{
    QDateTime epoch(QDate(1970,1,1),QTime(0,0));
    // ### kjs does not yet support long long (?) so the value wraps around
    return epoch.secsTo(m_createTime)*1000+m_createTime.time().msec();
}

void EventImpl::initEvent(const DOMString &eventTypeArg, bool canBubbleArg, bool cancelableArg)
{
    // ### ensure this is not called after we have been dispatched (also for subclasses)

    m_eventName = EventName::fromString(eventTypeArg);

    m_canBubble = canBubbleArg;
    m_cancelable = cancelableArg;
}

khtml::IDTable<EventImpl>* EventImpl::s_idTable;

khtml::IDTable<EventImpl>* EventImpl::initIdTable()
{
    s_idTable = new khtml::IDTable<EventImpl>();
    s_idTable->addStaticMapping(DOMFOCUSIN_EVENT, "DOMFocusIn");
    s_idTable->addStaticMapping(DOMFOCUSOUT_EVENT, "DOMFocusOut");
    s_idTable->addStaticMapping(DOMACTIVATE_EVENT, "DOMActivate");
    s_idTable->addStaticMapping(CLICK_EVENT, "click");
    s_idTable->addStaticMapping(MOUSEDOWN_EVENT, "mousedown");
    s_idTable->addStaticMapping(MOUSEUP_EVENT, "mouseup");
    s_idTable->addStaticMapping(MOUSEOVER_EVENT, "mouseover");
    s_idTable->addStaticMapping(MOUSEMOVE_EVENT, "mousemove");
    s_idTable->addStaticMapping(MOUSEOUT_EVENT, "mouseout");
    s_idTable->addStaticMapping(DOMSUBTREEMODIFIED_EVENT, "DOMSubtreeModified");
    s_idTable->addStaticMapping(DOMNODEINSERTED_EVENT, "DOMNodeInserted");
    s_idTable->addStaticMapping(DOMNODEREMOVED_EVENT, "DOMNodeRemoved");
    s_idTable->addStaticMapping(DOMNODEREMOVEDFROMDOCUMENT_EVENT, "DOMNodeRemovedFromDocument");
    s_idTable->addStaticMapping(DOMNODEINSERTEDINTODOCUMENT_EVENT,"DOMNodeInsertedIntoDocument");
    s_idTable->addStaticMapping(DOMATTRMODIFIED_EVENT, "DOMAttrModified");
    s_idTable->addStaticMapping(DOMCHARACTERDATAMODIFIED_EVENT, "DOMCharacterDataModified");
    s_idTable->addStaticMapping(LOAD_EVENT, "load");
    s_idTable->addStaticMapping(UNLOAD_EVENT, "unload");
    s_idTable->addStaticMapping(ABORT_EVENT, "abort");
    s_idTable->addStaticMapping(ERROR_EVENT, "error");
    s_idTable->addStaticMapping(SELECT_EVENT, "select");
    s_idTable->addStaticMapping(CHANGE_EVENT, "change");
    s_idTable->addStaticMapping(SUBMIT_EVENT, "submit");
    s_idTable->addStaticMapping(RESET_EVENT, "reset");
    s_idTable->addStaticMapping(FOCUS_EVENT, "focus");
    s_idTable->addStaticMapping(BLUR_EVENT, "blur");
    s_idTable->addStaticMapping(RESIZE_EVENT, "resize");
    s_idTable->addStaticMapping(SCROLL_EVENT, "scroll");
    s_idTable->addStaticMapping(KEYDOWN_EVENT, "keydown");
    s_idTable->addStaticMapping(KEYUP_EVENT, "keyup");
    s_idTable->addStaticMapping(KEYPRESS_EVENT, "keypress");
        //DOM3 ev. suggests textInput, but it's better for compat this way
    s_idTable->addStaticMapping(HASHCHANGE_EVENT, "hashchange");

    //khtml extensions
    s_idTable->addStaticMapping(KHTML_ECMA_DBLCLICK_EVENT, "dblclick");
    s_idTable->addHiddenMapping(KHTML_ECMA_CLICK_EVENT, "click");
    s_idTable->addStaticMapping(KHTML_DRAGDROP_EVENT, "khtml_dragdrop");
    s_idTable->addStaticMapping(KHTML_MOVE_EVENT, "khtml_move");
    s_idTable->addStaticMapping(KHTML_MOUSEWHEEL_EVENT, "DOMMouseScroll");
        // adopt the mozilla name for compatibility
    s_idTable->addStaticMapping(KHTML_CONTENTLOADED_EVENT, "DOMContentLoaded");
        // idem
    s_idTable->addStaticMapping(KHTML_READYSTATECHANGE_EVENT, "readystatechange");

    s_idTable->addStaticMapping(MESSAGE_EVENT, "message");

    return s_idTable;
}

bool EventImpl::isUIEvent() const
{
    return false;
}

bool EventImpl::isMouseEvent() const
{
    return false;
}

bool EventImpl::isMutationEvent() const
{
    return false;
}

bool EventImpl::isTextInputEvent() const
{
    return false;
}

bool EventImpl::isKeyboardEvent() const
{
    return false;
}

bool EventImpl::isMessageEvent() const
{
    return false;
}

bool EventImpl::isHashChangeEvent() const
{
    return false;
}

// -----------------------------------------------------------------------------

UIEventImpl::UIEventImpl(EventId _id, bool canBubbleArg, bool cancelableArg,
		AbstractViewImpl *viewArg, long detailArg)
		: EventImpl(_id,canBubbleArg,cancelableArg)
{
    m_view = viewArg;
    if (m_view)
        m_view->ref();
    m_detail = detailArg;
}

UIEventImpl::~UIEventImpl()
{
    if (m_view)
        m_view->deref();
}

void UIEventImpl::initUIEvent(const DOMString &typeArg,
			      bool canBubbleArg,
			      bool cancelableArg,
			      AbstractViewImpl* viewArg,
			      long detailArg)
{
    EventImpl::initEvent(typeArg,canBubbleArg,cancelableArg);

    if (viewArg)
      viewArg->ref();

    if (m_view)
      m_view->deref();

    m_view = viewArg;

    m_detail = detailArg;
}

bool UIEventImpl::isUIEvent() const
{
    return true;
}

// -----------------------------------------------------------------------------

MouseEventImpl::MouseEventImpl()
{
    m_screenX = 0;
    m_screenY = 0;
    m_clientX = 0;
    m_clientY = 0;
    m_pageX = 0;
    m_pageY = 0;
    m_ctrlKey = false;
    m_altKey = false;
    m_shiftKey = false;
    m_metaKey = false;
    m_button = 0;
    m_relatedTarget = 0;
    m_qevent = 0;
    m_isDoubleClick = false;
}

MouseEventImpl::MouseEventImpl(EventId _id,
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
			       QMouseEvent *qe,
                               bool isDoubleClick,
                               Orientation orient)
		   : UIEventImpl(_id,canBubbleArg,cancelableArg,viewArg,detailArg)
{
    m_screenX = screenXArg;
    m_screenY = screenYArg;
    m_clientX = clientXArg;
    m_clientY = clientYArg;
    m_pageX = pageXArg;
    m_pageY = pageYArg;
    m_ctrlKey = ctrlKeyArg;
    m_altKey = altKeyArg;
    m_shiftKey = shiftKeyArg;
    m_metaKey = metaKeyArg;
    m_button = buttonArg;
    m_relatedTarget = relatedTargetArg;
    if (m_relatedTarget)
	m_relatedTarget->ref();
    computeLayerPos();
    m_qevent = qe;
    m_isDoubleClick = isDoubleClick;
    m_orientation = orient;
}

MouseEventImpl::~MouseEventImpl()
{
    if (m_relatedTarget)
	m_relatedTarget->deref();
}

void MouseEventImpl::computeLayerPos()
{
    m_layerX = m_pageX;
    m_layerY = m_pageY;

    DocumentImpl* doc = view() ? view()->document() : 0;
    if (doc && doc->renderer()) {
        khtml::RenderObject::NodeInfo renderInfo(true, false);
        doc->renderer()->layer()->nodeAtPoint(renderInfo, m_pageX, m_pageY);

        NodeImpl *node = renderInfo.innerNonSharedNode();
        while (node && !node->renderer()) {
            node = node->parent();
        }

        if (node) {
            RenderLayer* layer = node->renderer()->enclosingLayer();
            if (layer) {
                layer->updateLayerPosition();
            }

            while (layer) {
                m_layerX -= layer->xPos();
                m_layerY -= layer->yPos();
                layer = layer->parent();
            }
        }
    }
}

void MouseEventImpl::initMouseEvent(const DOMString &typeArg,
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
                                    Orientation orient)
{
    UIEventImpl::initUIEvent(typeArg,canBubbleArg,cancelableArg,viewArg,detailArg);

    if (m_relatedTarget)
	m_relatedTarget->deref();

    m_screenX = screenXArg;
    m_screenY = screenYArg;
    m_clientX = clientXArg;
    m_clientY = clientYArg;
    m_pageX   = clientXArg;
    m_pageY   = clientYArg;
    KHTMLView* v;
    if ( view() && view()->document() && ( v = view()->document()->view() ) ) {
        m_pageX += v->contentsX();
        m_pageY += v->contentsY();
    }
    m_ctrlKey = ctrlKeyArg;
    m_altKey = altKeyArg;
    m_shiftKey = shiftKeyArg;
    m_metaKey = metaKeyArg;
    m_button = buttonArg;
    m_relatedTarget = relatedTargetArg.handle();
    if (m_relatedTarget)
	m_relatedTarget->ref();
    m_orientation = orient;


    // ### make this on-demand. it is soo sloooow
    computeLayerPos();
    m_qevent = 0;
}

bool MouseEventImpl::isMouseEvent() const
{
    return true;
}

//---------------------------------------------------------------------------------------------

/* Mapping between special Qt keycodes and virtual DOM codes */
IDTranslator<unsigned, unsigned, unsigned>::Info virtKeyToQtKeyTable[] =
{
    {KeyEventBaseImpl::DOM_VK_BACK_SPACE, Qt::Key_Backspace},
    {KeyEventBaseImpl::DOM_VK_ENTER, Qt::Key_Enter},
    {KeyEventBaseImpl::DOM_VK_ENTER, Qt::Key_Return},
    {KeyEventBaseImpl::DOM_VK_NUM_LOCK,  Qt::Key_NumLock},
    {KeyEventBaseImpl::DOM_VK_RIGHT_ALT,    Qt::Key_Alt},
    {KeyEventBaseImpl::DOM_VK_LEFT_CONTROL, Qt::Key_Control},
    {KeyEventBaseImpl::DOM_VK_LEFT_SHIFT,   Qt::Key_Shift},
    {KeyEventBaseImpl::DOM_VK_META,         Qt::Key_Meta},
    {KeyEventBaseImpl::DOM_VK_CAPS_LOCK,    Qt::Key_CapsLock},
    {KeyEventBaseImpl::DOM_VK_DELETE,       Qt::Key_Delete},
    {KeyEventBaseImpl::DOM_VK_INSERT,       Qt::Key_Insert},    
    {KeyEventBaseImpl::DOM_VK_END,          Qt::Key_End},
    {KeyEventBaseImpl::DOM_VK_ESCAPE,       Qt::Key_Escape},
    {KeyEventBaseImpl::DOM_VK_HOME,         Qt::Key_Home},
    {KeyEventBaseImpl::DOM_VK_PAUSE,        Qt::Key_Pause},
    {KeyEventBaseImpl::DOM_VK_PRINTSCREEN,  Qt::Key_Print},
    {KeyEventBaseImpl::DOM_VK_SCROLL_LOCK,  Qt::Key_ScrollLock},
    {KeyEventBaseImpl::DOM_VK_LEFT,         Qt::Key_Left},
    {KeyEventBaseImpl::DOM_VK_RIGHT,        Qt::Key_Right},
    {KeyEventBaseImpl::DOM_VK_UP,           Qt::Key_Up},
    {KeyEventBaseImpl::DOM_VK_DOWN,         Qt::Key_Down},
    {KeyEventBaseImpl::DOM_VK_PAGE_DOWN,    Qt::Key_PageDown},
    {KeyEventBaseImpl::DOM_VK_PAGE_UP,      Qt::Key_PageUp},
    {KeyEventBaseImpl::DOM_VK_F1,           Qt::Key_F1},
    {KeyEventBaseImpl::DOM_VK_F2,           Qt::Key_F2},
    {KeyEventBaseImpl::DOM_VK_F3,           Qt::Key_F3},
    {KeyEventBaseImpl::DOM_VK_F4,           Qt::Key_F4},
    {KeyEventBaseImpl::DOM_VK_F5,           Qt::Key_F5},
    {KeyEventBaseImpl::DOM_VK_F6,           Qt::Key_F6},
    {KeyEventBaseImpl::DOM_VK_F7,           Qt::Key_F7},
    {KeyEventBaseImpl::DOM_VK_F8,           Qt::Key_F8},
    {KeyEventBaseImpl::DOM_VK_F9,           Qt::Key_F9},
    {KeyEventBaseImpl::DOM_VK_F10,          Qt::Key_F10},
    {KeyEventBaseImpl::DOM_VK_F11,          Qt::Key_F11},
    {KeyEventBaseImpl::DOM_VK_F12,          Qt::Key_F12},
    {KeyEventBaseImpl::DOM_VK_F13,          Qt::Key_F13},
    {KeyEventBaseImpl::DOM_VK_F14,          Qt::Key_F14},
    {KeyEventBaseImpl::DOM_VK_F15,          Qt::Key_F15},
    {KeyEventBaseImpl::DOM_VK_F16,          Qt::Key_F16},
    {KeyEventBaseImpl::DOM_VK_F17,          Qt::Key_F17},
    {KeyEventBaseImpl::DOM_VK_F18,          Qt::Key_F18},
    {KeyEventBaseImpl::DOM_VK_F19,          Qt::Key_F19},
    {KeyEventBaseImpl::DOM_VK_F20,          Qt::Key_F20},
    {KeyEventBaseImpl::DOM_VK_F21,          Qt::Key_F21},
    {KeyEventBaseImpl::DOM_VK_F22,          Qt::Key_F22},
    {KeyEventBaseImpl::DOM_VK_F23,          Qt::Key_F23},
    {KeyEventBaseImpl::DOM_VK_F24,          Qt::Key_F24},
    {0,                   0}
};

MAKE_TRANSLATOR(virtKeyToQtKey, unsigned, unsigned, unsigned, virtKeyToQtKeyTable)

KeyEventBaseImpl::KeyEventBaseImpl(EventId id, bool canBubbleArg, bool cancelableArg, AbstractViewImpl *viewArg,
                                        QKeyEvent *key) :
     UIEventImpl(id, canBubbleArg, cancelableArg, viewArg, 0)
{
    m_synthetic = false;

    m_keyEvent = new QKeyEvent(key->type(), key->key(), key->modifiers(), key->text(), key->isAutoRepeat(), key->count() );

    //Here, we need to map Qt's internal info to browser-style info.
    m_detail = key->count();
    m_keyVal = 0; // Set below unless virtual...
    m_virtKeyVal = virtKeyToQtKey()->toLeft(key->key());

    // m_keyVal should contain the unicode value
    // of the pressed key if available, including case distinction
    if (m_virtKeyVal == DOM_VK_UNDEFINED && !key->text().isEmpty()) {
        // ... unfortunately, this value is useless if ctrl+ or alt+ are used.
        if (key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
            // Try to recover the case... Not quite right with caps lock involved, hence proving again its evilness
            if (key->modifiers() & (Qt::ShiftModifier))
                m_keyVal = key->key(); // The original is upper case anyway
            else
                m_keyVal = QChar(key->key()).toLower().unicode();
        } else {
            m_keyVal = key->text().unicode()[0].unicode();
        }
    }

    // key->state returns enum ButtonState, which is ShiftButton, ControlButton and AltButton or'ed together.
    m_modifier = key->modifiers();
}

KeyEventBaseImpl::~KeyEventBaseImpl()
{
    delete m_keyEvent;
}

void KeyEventBaseImpl::initKeyBaseEvent(const DOMString &typeArg,
                                        bool canBubbleArg,
                                        bool cancelableArg,
                                        AbstractViewImpl* viewArg,
                                        unsigned long keyValArg,
                                        unsigned long virtKeyValArg,
                                        unsigned long modifiersArg)
{
    m_synthetic = true;
    delete m_keyEvent;
    m_keyEvent = 0;
    initUIEvent(typeArg, canBubbleArg, cancelableArg, viewArg, 1);
    m_virtKeyVal = virtKeyValArg;
    m_keyVal     = keyValArg;
    m_modifier   = modifiersArg;
}

bool KeyEventBaseImpl::checkModifier(unsigned long modifierArg)
{
  return ((m_modifier & modifierArg) == modifierArg);
}

void KeyEventBaseImpl::buildQKeyEvent() const
{
    delete m_keyEvent;

    assert(m_synthetic);
    //IMPORTANT: we ignore modifers on purpose.
    //this is to prevent a website from synthesizing something
    //like Ctrl-V or Shift-Insert and stealing contents of the user's clipboard.
    Qt::KeyboardModifiers modifiers = 0;

    if (m_modifier & Qt::KeypadModifier)
        modifiers |= Qt::KeypadModifier;

    int key   = 0;
    QString text;
    if (m_virtKeyVal)
        key = virtKeyToQtKey()->toRight(m_virtKeyVal);
    if (!key) {
        key   = m_keyVal;
        text  = QChar(key);
    }

    //Neuter F keys as well.
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
        key = Qt::Key_ScrollLock;

    m_keyEvent = new QKeyEvent(id() == KEYUP_EVENT ? QEvent::KeyRelease : QEvent::KeyPress,
                        key, modifiers, text);
}

//------------------------------------------------------------------------------


static const IDTranslator<QByteArray, unsigned, const char*>::Info keyIdentifiersToVirtKeysTable[] = {
    {"Alt",         KeyEventBaseImpl::DOM_VK_LEFT_ALT},
    {"Control",     KeyEventBaseImpl::DOM_VK_LEFT_CONTROL},
    {"Shift",       KeyEventBaseImpl::DOM_VK_LEFT_SHIFT},
    {"Meta",        KeyEventBaseImpl::DOM_VK_META},
    {"\0x08",       KeyEventBaseImpl::DOM_VK_SPACE},           //1-char virt!
    {"CapsLock",    KeyEventBaseImpl::DOM_VK_CAPS_LOCK},
    {"\x7F",        KeyEventBaseImpl::DOM_VK_DELETE},          //1-char virt!
    {"End",         KeyEventBaseImpl::DOM_VK_END},
    {"Enter",       KeyEventBaseImpl::DOM_VK_ENTER},
    {"\x1b",        KeyEventBaseImpl::DOM_VK_ESCAPE},          //1-char virt!
    {"Home",        KeyEventBaseImpl::DOM_VK_HOME},
    {"NumLock",     KeyEventBaseImpl::DOM_VK_NUM_LOCK},
    {"Pause",       KeyEventBaseImpl::DOM_VK_PAUSE},
    {"PrintScreen", KeyEventBaseImpl::DOM_VK_PRINTSCREEN},
    {"Scroll",   KeyEventBaseImpl::DOM_VK_SCROLL_LOCK},
    {" ",        KeyEventBaseImpl::DOM_VK_SPACE},               //1-char virt!
    {"\t",       KeyEventBaseImpl::DOM_VK_TAB},                 //1-char virt!
    {"Left",     KeyEventBaseImpl::DOM_VK_LEFT},
    {"Left",     KeyEventBaseImpl::DOM_VK_LEFT},
    {"Right",    KeyEventBaseImpl::DOM_VK_RIGHT},
    {"Up",       KeyEventBaseImpl::DOM_VK_UP},
    {"Down",     KeyEventBaseImpl::DOM_VK_DOWN},
    {"PageDown", KeyEventBaseImpl::DOM_VK_PAGE_DOWN},
    {"PageUp", KeyEventBaseImpl::DOM_VK_PAGE_UP},
    {"F1", KeyEventBaseImpl::DOM_VK_F1},
    {"F2", KeyEventBaseImpl::DOM_VK_F2},
    {"F3", KeyEventBaseImpl::DOM_VK_F3},
    {"F4", KeyEventBaseImpl::DOM_VK_F4},
    {"F5", KeyEventBaseImpl::DOM_VK_F5},
    {"F6", KeyEventBaseImpl::DOM_VK_F6},
    {"F7", KeyEventBaseImpl::DOM_VK_F7},
    {"F8", KeyEventBaseImpl::DOM_VK_F8},
    {"F9", KeyEventBaseImpl::DOM_VK_F9},
    {"F10", KeyEventBaseImpl::DOM_VK_F10},
    {"F11", KeyEventBaseImpl::DOM_VK_F11},
    {"F12", KeyEventBaseImpl::DOM_VK_F12},
    {"F13", KeyEventBaseImpl::DOM_VK_F13},
    {"F14", KeyEventBaseImpl::DOM_VK_F14},
    {"F15", KeyEventBaseImpl::DOM_VK_F15},
    {"F16", KeyEventBaseImpl::DOM_VK_F16},
    {"F17", KeyEventBaseImpl::DOM_VK_F17},
    {"F18", KeyEventBaseImpl::DOM_VK_F18},
    {"F19", KeyEventBaseImpl::DOM_VK_F19},
    {"F20", KeyEventBaseImpl::DOM_VK_F20},
    {"F21", KeyEventBaseImpl::DOM_VK_F21},
    {"F22", KeyEventBaseImpl::DOM_VK_F22},
    {"F23", KeyEventBaseImpl::DOM_VK_F23},
    {"F24", KeyEventBaseImpl::DOM_VK_F24},
    {0, 0}
};

MAKE_TRANSLATOR(keyIdentifiersToVirtKeys, QByteArray, unsigned, const char*, keyIdentifiersToVirtKeysTable)

/** These are the modifiers we currently support */
static const IDTranslator<QByteArray, unsigned, const char*>::Info keyModifiersToCodeTable[] = {
    {"Alt",         Qt::AltModifier},
    {"Control",     Qt::ControlModifier},
    {"Shift",       Qt::ShiftModifier},
    {"Meta",        Qt::MetaModifier},
    {0,             0}
};

MAKE_TRANSLATOR(keyModifiersToCode, QByteArray, unsigned, const char*, keyModifiersToCodeTable)

KeyboardEventImpl::KeyboardEventImpl() : m_keyLocation(KeyboardEvent::DOM_KEY_LOCATION_STANDARD)
{}

DOMString KeyboardEventImpl::keyIdentifier() const
{
    if (unsigned special = virtKeyVal())
        if (const char* id = keyIdentifiersToVirtKeys()->toLeft(special))
            return QString::fromLatin1(id);

    if (unsigned unicode = keyVal())
        return QString(QChar(unicode));

    return "Unidentified";
}

bool KeyboardEventImpl::getModifierState (const DOMString& keyIdentifierArg) const
{
    unsigned mask = keyModifiersToCode()->toRight(keyIdentifierArg.string().toLatin1());
    return m_modifier & mask;
}

bool KeyboardEventImpl::isKeyboardEvent() const
{
    return true;
}

void KeyboardEventImpl::initKeyboardEvent(const DOMString &typeArg,
                                          bool canBubbleArg,
                                          bool cancelableArg,
                                          AbstractViewImpl* viewArg,
                                          const DOMString &keyIdentifierArg,
                                          unsigned long keyLocationArg,
                                          const DOMString& modifiersList)
{
    unsigned keyVal     = 0;
    unsigned virtKeyVal = 0;

    m_keyLocation = keyLocationArg;

    //Figure out the code information from the key identifier.
    if (keyIdentifierArg.length() == 1) {
        //Likely to be normal unicode id, unless it's one of the few
        //special values.
        unsigned short code = keyIdentifierArg.unicode()[0].unicode();
        if (code > 0x20 && code != 0x7F)
            keyVal = code;
    }

    if (!keyVal) //One of special keys, likely.
        virtKeyVal = keyIdentifiersToVirtKeys()->toRight(keyIdentifierArg.string().toLatin1());

    //Process modifier list.
    const QStringList mods = modifiersList.string().trimmed().simplified().split( ' ' );

    unsigned modifiers = 0;
    for (QStringList::ConstIterator i = mods.begin(); i != mods.end(); ++i)
        if (unsigned mask = keyModifiersToCode()->toRight((*i).toLatin1()))
            modifiers |= mask;

    initKeyBaseEvent(typeArg, canBubbleArg, cancelableArg, viewArg,
            keyVal, virtKeyVal, modifiers);
}

KeyboardEventImpl::KeyboardEventImpl(QKeyEvent* key, DOM::AbstractViewImpl* view) :
    KeyEventBaseImpl(key->type() == QEvent::KeyRelease ? KEYUP_EVENT : KEYDOWN_EVENT, true, true, view, key)
{
    if (key->modifiers() & Qt::KeypadModifier)
        m_keyLocation = KeyboardEvent::DOM_KEY_LOCATION_NUMPAD;
    else {
        //It's generally standard, but for the modifiers,
        //it should be left/right, so guess left.
        m_keyLocation = KeyboardEvent::DOM_KEY_LOCATION_STANDARD;
        switch (m_virtKeyVal) {
            case DOM_VK_LEFT_ALT:
            case DOM_VK_LEFT_SHIFT:
            case DOM_VK_LEFT_CONTROL:
            case DOM_VK_META:
                m_keyLocation = KeyboardEvent::DOM_KEY_LOCATION_LEFT;
        }
    }
}

int KeyboardEventImpl::keyCode() const
{
    //Keycode on key events always identifies the -key- and not the input,
    //so e.g. 'a' will get 'A'
    if (m_virtKeyVal != DOM_VK_UNDEFINED) {
        return m_virtKeyVal;
    } else {
        unsigned char code = QChar((unsigned short)m_keyVal).toUpper().unicode();
        // Some codes we get from Qt are things like ( which conflict with
        // browser scancodes. Remap them to what's on their keycaps in a US
        // layout.
        if (virtKeyToQtKey()->hasLeft(code)) {
            switch (code) {
                case '!': return '1';
                case '@': return '2';
                case '#': return '3';
                case '$': return '4';
                case '%': return '5';
                case '^': return '6';
                case '&': return '7';
                case '*': return '8';
                case '(': return '9';
                case ')': return '0';
                default:
                    qWarning() << "Don't know how resolve conflict of code:" << code
                                   << " with a virtKey";
            }
        }
        return code;
    }
}

int KeyboardEventImpl::charCode() const
{
    //IE doesn't support charCode at all, and mozilla returns 0
    //on key events. So return 0 here
    return 0;
}


// -----------------------------------------------------------------------------
TextEventImpl::TextEventImpl()
{}

bool TextEventImpl::isTextInputEvent() const
{
    return true;
}

TextEventImpl::TextEventImpl(QKeyEvent* key, DOM::AbstractViewImpl* view) :
    KeyEventBaseImpl(KEYPRESS_EVENT, true, true, view, key)
{
    m_outputString = key->text();
}

void TextEventImpl::initTextEvent(const DOMString &typeArg,
                       bool canBubbleArg,
                       bool cancelableArg,
                       AbstractViewImpl* viewArg,
                       const DOMString& text)
{
    m_outputString = text;

    //See whether we can get a key out of this.
    unsigned keyCode = 0;
    if (text.length() == 1)
        keyCode = text.unicode()[0].unicode();
    initKeyBaseEvent(typeArg, canBubbleArg, cancelableArg, viewArg,
        keyCode, 0, 0);
}

int TextEventImpl::keyCode() const
{
    //Mozilla returns 0 here unless this is a non-unicode key.
    //IE stuffs everything here, and so we try to match it..
    if (m_keyVal)
        return m_keyVal;
    return m_virtKeyVal;
}

int TextEventImpl::charCode() const
{
    //On text events, in Mozilla charCode is 0 for non-unicode keys,
    //and the unicode key otherwise... IE doesn't support this.
    if (m_virtKeyVal)
        return 0;
    return m_keyVal;
}


// -----------------------------------------------------------------------------
MutationEventImpl::MutationEventImpl()
{
    m_relatedNode = 0;
    m_prevValue = 0;
    m_newValue = 0;
    m_attrName = 0;
    m_attrChange = 0;
}

MutationEventImpl::MutationEventImpl(EventId _id,
				     bool canBubbleArg,
				     bool cancelableArg,
				     const Node &relatedNodeArg,
				     const DOMString &prevValueArg,
				     const DOMString &newValueArg,
				     const DOMString &attrNameArg,
				     unsigned short attrChangeArg)
		      : EventImpl(_id,canBubbleArg,cancelableArg)
{
    m_relatedNode = relatedNodeArg.handle();
    if (m_relatedNode)
	m_relatedNode->ref();
    m_prevValue = prevValueArg.implementation();
    if (m_prevValue)
	m_prevValue->ref();
    m_newValue = newValueArg.implementation();
    if (m_newValue)
	m_newValue->ref();
    m_attrName = attrNameArg.implementation();
    if (m_attrName)
	m_attrName->ref();
    m_attrChange = attrChangeArg;
}

MutationEventImpl::~MutationEventImpl()
{
    if (m_relatedNode)
	m_relatedNode->deref();
    if (m_prevValue)
	m_prevValue->deref();
    if (m_newValue)
	m_newValue->deref();
    if (m_attrName)
	m_attrName->deref();
}

void MutationEventImpl::initMutationEvent(const DOMString &typeArg,
					  bool canBubbleArg,
					  bool cancelableArg,
					  const Node &relatedNodeArg,
					  const DOMString &prevValueArg,
					  const DOMString &newValueArg,
					  const DOMString &attrNameArg,
					  unsigned short attrChangeArg)
{
    EventImpl::initEvent(typeArg,canBubbleArg,cancelableArg);

    if (m_relatedNode)
	m_relatedNode->deref();
    if (m_prevValue)
	m_prevValue->deref();
    if (m_newValue)
	m_newValue->deref();
    if (m_attrName)
	m_attrName->deref();

    m_relatedNode = relatedNodeArg.handle();
    if (m_relatedNode)
	m_relatedNode->ref();
    m_prevValue = prevValueArg.implementation();
    if (m_prevValue)
	m_prevValue->ref();
    m_newValue = newValueArg.implementation();
    if (m_newValue)
	m_newValue->ref();
    m_attrName = attrNameArg.implementation();
    if (m_newValue)
	m_newValue->ref();
    m_attrChange = attrChangeArg;
}

bool MutationEventImpl::isMutationEvent() const
{
    return true;
}

// -----------------------------------------------------------------------------

MessageEventImpl::MessageEventImpl()
{}


bool MessageEventImpl::isMessageEvent() const
{
    return true;
}

void MessageEventImpl::initMessageEvent(const DOMString& typeArg,
                                        bool  canBubbleArg,
                                        bool  cancelableArg,
                                        const RefPtr<Data>& dataArg,
                                        const DOMString& originArg,
                                        const DOMString& lastEventIdArg,
                                        KHTMLPart* sourceArg)
{
    EventImpl::initEvent(typeArg, canBubbleArg, cancelableArg);
    m_data   = dataArg;
    m_origin = originArg;
    m_lastEventId = lastEventIdArg;
    m_source      = sourceArg;
}

// -----------------------------------------------------------------------------

HashChangeEventImpl::HashChangeEventImpl()
{
}

bool HashChangeEventImpl::isHashChangeEvent() const
{
  return true;
}

void HashChangeEventImpl::initHashChangeEvent(const DOMString& eventTypeArg, bool canBubbleArg, bool cancelableArg, const DOMString& oldUrl, const DOMString& newUrl)
{
    EventImpl::initEvent(eventTypeArg, canBubbleArg, cancelableArg);
    m_oldUrl = oldUrl;
    m_newUrl = newUrl;
}
