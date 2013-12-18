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

#ifndef _KJS_WINDOW_H_
#define _KJS_WINDOW_H_

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QDate>

#include "khtmlpart_p.h"
#include "kjs_binding.h"
#include "kjs_views.h"

class QTimer;
class KHTMLView;
class KHTMLPart;

namespace KParts {
  class ReadOnlyPart;
}

namespace khtml {
  class ChildFrame;
}

namespace DOM {
  class EventImpl;
}

namespace KJS {

  class WindowFunc;
  class WindowQObject;
  class Location;
  class History;
  class External;
  class ScheduledAction;
  class JSEventListener;
  class JSLazyEventListener;

  class Screen : public JSObject {
  public:
    Screen(ExecState *exec);
    enum {
      Height, Width, ColorDepth, PixelDepth, AvailLeft, AvailTop, AvailHeight,
      AvailWidth
    };
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
  private:
    KHTMLView *view;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  class Console : public JSObject {
  public:
    Console(ExecState *exec);
    enum {
      Assert, Log, Debug, Info, Warn, Error, Clear
    };

    enum MessageType {
        LogType     = 1,
        DebugType   = 1 << 1,
        InfoType    = 1 << 2,
        WarnType    = 1 << 3,
        ErrorType   = 1 << 4
    };
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
  private:
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  class Window : public JSGlobalObject {
    friend QPointer<KHTMLPart> getInstance();
    friend class KJS::Location;
    friend class KJS::WindowFunc;
    friend class KJS::WindowQObject;
    friend class KJS::ScheduledAction;
  public:
    Window(khtml::ChildFrame *p);
  public:
    ~Window();
    /**
     * Returns and registers a window object. In case there's already a Window
     * for the specified part p this will be returned in order to have unique
     * bindings.
     */
    static JSValue* retrieve(KParts::ReadOnlyPart *p);
    /**
     * Returns the Window object for a given part
     */
    static Window *retrieveWindow(KParts::ReadOnlyPart *p);
    /**
     * returns a pointer to the Window object this javascript interpreting instance
     * was called from.
     */
    static Window *retrieveActive(ExecState *exec);
    KParts::ReadOnlyPart *part() const {
        return m_frame.isNull() ? 0L : m_frame->m_part.data();
    }

    virtual void mark();
    JSValue* getValueProperty(ExecState *exec, int token);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    virtual bool toBoolean(ExecState *exec) const;
    virtual DOM::AbstractViewImpl* toAbstractView() const;
    void scheduleClose();
    void closeNow();
    void delayedGoHistory(int steps);
    void goHistory(int steps);
    void goURL(ExecState* exec, const QString& url, bool lockHistory);

    static bool targetIsExistingWindow(KHTMLPart* part, const QString& frameName);
    JSValue* openWindow(ExecState *exec, const List &args);
    JSValue* executeOpenWindow(ExecState *exec, const QUrl& url, const QString& frameName, const QString& features);
    void resizeTo(QWidget* tl, int width, int height);
    void afterScriptExecution();
    bool isSafeScript(ExecState *exec) const {
        KParts::ReadOnlyPart *activePart = static_cast<KJS::ScriptInterpreter *>(  exec->dynamicInterpreter() )->part();
      if ( activePart == part() ) return true;
      return checkIsSafeScript( activePart );
    }
    Location *location() const;
    JSEventListener *getJSEventListener(JSValue* val, bool html = false);
    JSLazyEventListener *getJSLazyEventListener(const QString &code, const QString& sourceUrl, int lineNo,
                                                const QString &name, DOM::NodeImpl* node, bool svg = false);
    void clear( ExecState *exec );
    virtual UString toString(ExecState *exec) const;

    // Set the current "event" object
    void setCurrentEvent( DOM::EventImpl *evt );

    QHash<const QPair<void*, bool>, JSEventListener*> jsEventListeners;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Closed, Crypto, DefaultStatus, Status, Document, Node, EventCtor, Range,
           NodeFilter, NodeList, DOMException, RangeException, CSSRule, Frames, _History, _External, Event, InnerHeight,
           InnerWidth, Length, _Location, Navigate, Name, _Navigator, _Konqueror, ClientInformation,
           OffscreenBuffering, Opener, OuterHeight, OuterWidth, PageXOffset, PageYOffset,
           Parent, Personalbar, ScreenX, ScreenY, Scrollbars, Scroll, ScrollBy,
           ScreenTop, ScreenLeft, AToB, BToA, FrameElement, GetComputedStyle,
           ScrollTo, ScrollX, ScrollY, MoveBy, MoveTo, PostMessage, ResizeBy, ResizeTo, Self, _Window, Top, _Screen,
           _Console, Audio, Image, Option, Alert, Confirm, Prompt, Open, SetTimeout, ClearTimeout,
           XMLHttpRequest, XMLSerializer, DOMParser, ArrayBuffer, Int8Array, Uint8Array,
           Int16Array, Uint16Array, Int32Array, Uint32Array, Float32Array, Float64Array,
           Focus, Blur, Close, SetInterval, ClearInterval, CaptureEvents, ReleaseEvents,
           Print, AddEventListener, RemoveEventListener, SideBar, GetSelection,
           ValueOf, ToString,
           Onabort, Onblur,
           Onchange, Onclick, Ondblclick, Ondragdrop, Onerror, Onfocus,
           Onkeydown, Onkeypress, Onkeyup, Onload,  Onmessage, Onmousedown, Onmousemove,
           Onmouseout, Onmouseover, Onmouseup, Onmove, Onreset, Onresize,
           Onscroll, Onselect, Onsubmit, Onunload, Onhashchange,
           MutationEventCtor, MessageEventCtor, KeyboardEventCtor, EventExceptionCtor, HashChangeEventCtor,
           ElementCtor, DocumentCtor, DocumentFragmentCtor, HTMLDocumentCtor,
           HTMLElementCtor, HTMLHtmlElementCtor, HTMLHeadElementCtor, HTMLLinkElementCtor,
           HTMLTitleElementCtor, HTMLMetaElementCtor, HTMLBaseElementCtor, HTMLIsIndexElementCtor,
           HTMLStyleElementCtor, HTMLBodyElementCtor, HTMLFormElementCtor, HTMLSelectElementCtor,
           HTMLOptGroupElementCtor, HTMLOptionElementCtor, HTMLInputElementCtor, HTMLTextAreaElementCtor,
           HTMLButtonElementCtor, HTMLLabelElementCtor, HTMLFieldSetElementCtor, HTMLLegendElementCtor,
           HTMLUListElementCtor, HTMLOListElementCtor, HTMLDListElementCtor, HTMLDirectoryElementCtor,
           HTMLMenuElementCtor, HTMLLIElementCtor, HTMLDivElementCtor, HTMLParagraphElementCtor,
           HTMLHeadingElementCtor, HTMLBlockQuoteElementCtor, HTMLQuoteElementCtor, HTMLPreElementCtor,
           HTMLBRElementCtor, HTMLBaseFontElementCtor, HTMLFontElementCtor, HTMLHRElementCtor, HTMLModElementCtor,
           HTMLAnchorElementCtor, HTMLImageElementCtor, HTMLObjectElementCtor, HTMLParamElementCtor,
           HTMLAppletElementCtor, HTMLMapElementCtor, HTMLAreaElementCtor, HTMLScriptElementCtor,
           HTMLTableElementCtor, HTMLTableCaptionElementCtor, HTMLTableColElementCtor,
           HTMLTableSectionElementCtor, HTMLTableRowElementCtor, HTMLTableCellElementCtor,
           HTMLFrameSetElementCtor, HTMLLayerElementCtor, HTMLFrameElementCtor, HTMLIFrameElementCtor,
           HTMLCollectionCtor, StyleSheetCtor, 
           CSSStyleDeclarationCtor, HTMLCanvasElementCtor, Context2DCtor, SVGAngleCtor,
           XPathResultCtor, XPathExpressionCtor, XPathNSResolverCtor};
    WindowQObject *winq;

    void forgetSuppressedWindows();
    void showSuppressedWindows();

    JSValue* indexGetter(ExecState *exec, unsigned index);

    // updates window listeners.
    JSValue* getListener(ExecState *exec, int eventId) const;
    void setListener(ExecState *exec, int eventId, JSValue* func);
    
    struct DelayedAction {
      virtual void mark() {}; // mark any JS objects we use
      virtual bool execute(Window*) = 0; // returns whether to continue or not
      virtual ~DelayedAction() {};
    };
    
  private:
    // Returns true if the particular method or property should be permitted
    // to be accessed even across frames.
    bool isCrossFrameAccessible(int token) const;
  
    KParts::ReadOnlyPart* frameByIndex(unsigned index);
    static JSValue *framePartGetter(ExecState *exec, JSObject*, const Identifier&, const PropertySlot& slot);
    static JSValue *namedItemGetter(ExecState *exec, JSObject*, const Identifier&, const PropertySlot& slot);

    bool checkIsSafeScript( KParts::ReadOnlyPart* activePart ) const;

    QPointer<khtml::ChildFrame> m_frame;
    Screen *screen;
    Console *console;
    History *history;
    External *external;
    Location *loc;
    DOM::EventImpl *m_evt;

    QList<DelayedAction*> m_delayed;

    struct SuppressedWindowInfo {
       SuppressedWindowInfo() {}  // for QValueList
       SuppressedWindowInfo( QUrl u, QString fr, QString fe ) : url(u), frameName(fr), features(fe) {}
       QUrl url;
       QString frameName;
       QString features;
     };
     QList<SuppressedWindowInfo> m_suppressedWindowInfo;
  };

  /**
   * like QDateTime, but properly handles milliseconds
   */
  class DateTimeMS
  {
  	QDate mDate;
  	QTime mTime;
  public:
    DateTimeMS addMSecs(int s) const;
    bool operator >(const DateTimeMS &other) const;
    bool operator >=(const DateTimeMS &other) const;

    int msecsTo(const DateTimeMS &other) const;

    static DateTimeMS now();
  };

  /**
   * An action (either function or string) to be executed after a specified
   * time interval, either once or repeatedly. Used for window.setTimeout()
   * and window.setInterval()
   */
  class ScheduledAction {
  public:
    ScheduledAction(JSObject* _func, List _args, DateTimeMS _nextTime, int _interval, bool _singleShot, int _timerId);
    ScheduledAction(QString _code, DateTimeMS _nextTime, int _interval, bool _singleShot, int _timerId);
    ~ScheduledAction();
    bool execute(Window *window);
    void mark();

    JSObject *func;
    List args;
    QString code;
    bool isFunction;
    bool singleShot;

    DateTimeMS nextTime;
    int interval;
    bool executing;
    int timerId;
  };

  class WindowQObject : public QObject {
    Q_OBJECT
  public:
    WindowQObject(Window *w);
    ~WindowQObject();
    int installTimeout(const Identifier &handler, int t, bool singleShot);
    int installTimeout(JSValue* func, List args, int t, bool singleShot);
    void clearTimeout(int timerId);
    void mark();
    bool hasTimers() const;

    void pauseTimers();
    void resumeTimers();
  public Q_SLOTS:
    void timeoutClose();
  protected Q_SLOTS:
    void parentDestroyed();
  protected:
    void timerEvent(QTimerEvent *e);
    void setNextTimer();
    void killTimers();
  private:
    Window *parent;
    QList<ScheduledAction*> scheduledActions;

    /**
     We need to pause timers when alerts are up; so we keep track
     of recursion of that and the delay at topmost level.
     */
    int pauseLevel;
    DateTimeMS pauseStart;

    int lastTimerId;
    QList<int> timerIds;
    bool currentlyDispatching;
  };

  /**
   * Helper for pausing/resuming timers
   */
  class TimerPauser
  {
  public:
    TimerPauser(ExecState* exec) {
      win = Window::retrieveActive(exec);
      win->winq->pauseTimers();
    }

    ~TimerPauser() {
      win->winq->resumeTimers();
    }
  private:
    Window* win;
  };

  class Location : public JSObject {
  public:
    ~Location();

    JSValue* getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    virtual JSValue* toPrimitive(ExecState *exec, JSType preferred) const;
    virtual UString toString(ExecState *exec) const;
    enum { Hash, Href, Hostname, Host, Pathname, Port, Protocol, Search, EqualEqual,
           Assign, Replace, Reload, ToString };
    KParts::ReadOnlyPart *part() const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  private:
    friend class Window;
    Location(khtml::ChildFrame *f);
    QPointer<khtml::ChildFrame> m_frame;
  };

} // namespace

#endif
