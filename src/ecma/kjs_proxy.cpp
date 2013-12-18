// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2001-2003 David Faure (faure@kde.org)
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

#include "kjs_proxy.h"

#include "kjs_window.h"
#include "kjs_events.h"
#ifdef KJS_DEBUGGER
#include "debugger/debugwindow.h"
#endif
#include <xml/dom_nodeimpl.h>
#include <khtmlpart_p.h>
#include <khtml_part.h>
#include <kprotocolmanager.h>
#include <QDebug>
#include <klocalizedstring.h>
#include <unistd.h>
#include <assert.h>
#include <kjs/function.h>
#include <kjs/JSLock.h>

using namespace KJS;
using namespace KJSDebugger;


#ifndef NDEBUG
int KJSProxy::s_count = 0;
#endif

KJSProxy::KJSProxy(khtml::ChildFrame *frame)
{
  m_script = 0;
  m_frame = frame;
  m_debugEnabled = false;
  m_running = 0;
  m_handlerLineno = 0;
#ifndef NDEBUG
  s_count++;
#endif
}

KJSProxy::~KJSProxy()
{
  if ( m_script ) {
    //qDebug() << "KJSProxy::~KJSProxyImpl clearing global object " << m_script->globalObject().imp();
    // This allows to delete the global-object properties, like all the protos
    m_script->globalObject()->clearProperties();
    //qDebug() << "KJSProxy::~KJSProxyImpl garbage collecting";

    JSLock::lock();
    while (Interpreter::collect())
	    ;
    JSLock::unlock();
    //qDebug() << "KJSProxy::~KJSProxyImpl deleting interpreter " << m_script;
    delete m_script;
    //qDebug() << "KJSProxy::~KJSProxyImpl garbage collecting again";
    // Garbage collect - as many times as necessary
    // (we could delete an object which was holding another object, so
    // the deref() will happen too late for deleting the impl of the 2nd object).
    JSLock::lock();
    while (Interpreter::collect())
	    ;
    JSLock::unlock();
  }

#ifndef NDEBUG
  s_count--;
  // If it was the last interpreter, we should have nothing left
#ifdef KJS_DEBUG_MEM
  if ( s_count == 0 )
    Interpreter::finalCheck();
#endif
#endif
}

QVariant KJSProxy::evaluate(QString filename, int baseLine,
                                const QString&str, const DOM::Node &n, Completion *completion) {
  ++m_running;
  // evaluate code. Returns the JS return value or an invalid QVariant
  // if there was none, an error occurred or the type couldn't be converted.

  initScript();
  // inlineCode is true for <a href="javascript:doSomething()">
  // and false for <script>doSomething()</script>. Check if it has the
  // expected value in all cases.
  // See smart window.open policy for where this is used.
  bool inlineCode = filename.isNull();
  //qDebug() << "KJSProxy::evaluate inlineCode=" << inlineCode;

#ifdef KJS_DEBUGGER
    if (inlineCode)
        filename = "(unknown file)";
    if (m_debugWindow)
        m_debugWindow->attach(m_script);
#else
    Q_UNUSED(baseLine);
#endif

  m_script->setInlineCode(inlineCode);
  Window* window = Window::retrieveWindow( m_frame->m_part );
  KJS::JSValue *thisNode = n.isNull() ? Window::retrieve( m_frame->m_part ) : getDOMNode(m_script->globalExec(),n.handle());

  UString code( str );

  m_script->startCPUGuard();
  Completion comp = m_script->evaluate(filename, baseLine, code, thisNode);
  m_script->stopCPUGuard();

  bool success = ( comp.complType() == KJS::Normal ) || ( comp.complType() == ReturnValue );

  if (completion)
    *completion = comp;

#ifdef KJS_DEBUGGER
    //    KJSDebugWin::debugWindow()->setCode(QString());
#endif

  window->afterScriptExecution();

  --m_running;

  // let's try to convert the return value
  if (success && comp.value())
    return ValueToVariant( m_script->globalExec(), comp.value());
  else
  {
    if ( comp.complType() == Throw )
    {
        UString msg = comp.value()->toString(m_script->globalExec());
        // qDebug() << "WARNING: Script threw exception: " << msg.qstring();
    }
    return QVariant();
  }
}

bool KJSProxy::isRunningScript()
{
  return m_running != 0;
}

// Implementation of the debug() function
class TestFunctionImp : public JSObject {
public:
  TestFunctionImp() : JSObject() {}
  virtual bool implementsCall() const { return true; }
  virtual JSValue *callAsFunction(ExecState *exec, JSObject *thisObj, const List &args);
};

JSValue *TestFunctionImp::callAsFunction(ExecState *exec, JSObject * /*thisObj*/, const List &args)
{
  fprintf(stderr,"--> %s\n",args[0]->toString(exec).ascii());
  return jsUndefined();
}

void KJSProxy::clear() {
    // clear resources allocated by the interpreter, and make it ready to be used by another page
    // We have to keep it, so that the Window object for the part remains the same.
    // (we used to delete and re-create it, previously)
    if (m_script) {
#ifdef KJS_DEBUGGER
        if (m_debugWindow)
            m_debugWindow->clearInterpreter(m_script);
#endif
        m_script->clear();

        Window *win = static_cast<Window *>(m_script->globalObject());
        if (win) {
            win->clear( m_script->globalExec() );
            // re-add "debug", clear() removed it
            m_script->globalObject()->put(m_script->globalExec(),
                                        "debug", new TestFunctionImp(), Internal);
            if ( win->part() )
            applyUserAgent();
    }

    // Really delete everything that can be, so that the DOM nodes get deref'ed
    //qDebug() << "all done -> collecting";
    JSLock::lock();
    while (Interpreter::collect())
	    ;
    JSLock::unlock();
  }

#ifdef KJS_DEBUGGER
  // Detach from debugging entirely if it's been turned off.
  if (m_debugWindow && !m_debugEnabled) {
    m_debugWindow->detach(m_script);
    m_debugWindow = 0;
  }
#endif
}

DOM::EventListener *KJSProxy::createHTMLEventHandler(QString sourceUrl, QString name, QString code, DOM::NodeImpl *node, bool svg)
{
    initScript();

#ifdef KJS_DEBUGGER
    if (m_debugWindow)
        m_debugWindow->attach(m_script);
#else
    Q_UNUSED(sourceUrl);
#endif

    return KJS::Window::retrieveWindow(m_frame->m_part)->getJSLazyEventListener(
        code, sourceUrl, m_handlerLineno, name, node, svg);
}

void KJSProxy::finishedWithEvent(const DOM::Event &event)
{
  // This is called when the DOM implementation has finished with a particular event. This
  // is the case in sitations where an event has been created just for temporary usage,
  // e.g. an image load or mouse move. Once the event has been dispatched, it is forgotten
  // by the DOM implementation and so does not need to be cached still by the interpreter
  ScriptInterpreter::forgetDOMObject(event.handle());
}

KJS::Interpreter *KJSProxy::interpreter()
{
  if (!m_script)
    initScript();
  return m_script;
}

void KJSProxy::setDebugEnabled(bool enabled)
{
#ifdef KJS_DEBUGGER
  m_debugEnabled = enabled;

  // Note that we attach to the debugger only before
  // running a script. Detaches/disabling are done between
  // documents, at clear. Both are done so the debugger
  // see the entire session
  if (enabled)
    m_debugWindow = DebugWindow::window();
#else
  Q_UNUSED(enabled)
#endif
}

bool KJSProxy::debugEnabled() const
{
#ifdef KJS_DEBUGGER
  return m_debugEnabled;
#else
  return false;
#endif
}

void KJSProxy::showDebugWindow(bool /*show*/)
{
#ifdef KJS_DEBUGGER
    if (m_debugWindow)
        m_debugWindow->show();
#else
    //Q_UNUSED(show);
#endif
}

bool KJSProxy::paused() const
{
#ifdef KJS_DEBUGGER
    // if (DebugWindow::window())
    //     return DebugWindow::window()->inSession();
#endif
    return false;
}

KJS_QT_UNICODE_IMPL

void KJSProxy::initScript()
{
  if (m_script)
    return;

  // Build the global object - which is a Window instance
  JSGlobalObject *globalObject( new Window(m_frame) );

  // Create a KJS interpreter for this part
  m_script = new KJS::ScriptInterpreter(globalObject, m_frame);
  KJS_QT_UNICODE_SET;
  globalObject->setPrototype(m_script->builtinObjectPrototype());

#ifdef KJS_DEBUGGER
  //m_script->setDebuggingEnabled(m_debugEnabled);
#endif
  //m_script->enableDebug();
  globalObject->put(m_script->globalExec(),
		   "debug", new TestFunctionImp(), Internal);
  applyUserAgent();

#ifdef KJS_DEBUGGER
  // Attach debugger as early as possible as not all scrips have a direct DOM-relation
  // NOTE: attach can be called multiple times
  if (m_debugEnabled)
      m_debugWindow->attach(m_script);
#endif
}

void KJSProxy::applyUserAgent()
{
  assert( m_script );
  QUrl url = m_frame->m_part.data()->url();
  QString host = url.isLocalFile() ? "localhost" : url.host();
  QString userAgent = KProtocolManager::userAgentForHost(host);
  if (userAgent.indexOf(QLatin1String("Microsoft"), 0, Qt::CaseSensitive) >= 0 ||
      userAgent.indexOf(QLatin1String("MSIE"), 0, Qt::CaseSensitive) >= 0)
  {
    m_script->setCompatMode(Interpreter::IECompat);
#ifdef KJS_VERBOSE
    qDebug() << "Setting IE compat mode";
#endif
  }
  else
    // If we find "Mozilla" but not "(compatible, ...)" we are a real Netscape
    if (userAgent.indexOf(QLatin1String("Mozilla"), 0, Qt::CaseSensitive) >= 0 &&
        userAgent.indexOf(QLatin1String("compatible"), 0, Qt::CaseSensitive) == -1 &&
        userAgent.indexOf(QLatin1String("KHTML"), 0, Qt::CaseSensitive) == -1)
    {
      m_script->setCompatMode(Interpreter::NetscapeCompat);
#ifdef KJS_VERBOSE
      qDebug() << "Setting NS compat mode";
#endif
    }
}

// Helper method, so that all classes which need jScript() don't need to be added
// as friend to KHTMLPart
KJSProxy * KJSProxy::proxy( KHTMLPart *part )
{
    return part->jScript();
}
