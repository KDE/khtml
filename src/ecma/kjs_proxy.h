// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
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

#ifndef _KJS_PROXY_H_
#define _KJS_PROXY_H_

#include <QtCore/QVariant>
#include <QtCore/QString>
#include <sys/time.h>
#include <wtf/RefPtr.h>

class KHTMLPart;

namespace DOM {
  class Node;
  class NodeImpl;
  class EventListener;
  class Event;
}

namespace KJS {
  class List;
  class Interpreter;
  class Completion;
  class ScriptInterpreter;
}

namespace KJSDebugger {
  class DebugWindow;
}

namespace khtml {
  class ChildFrame;
}

/**
 * @internal
 *
 * @short Proxy class serving as interface when being dlopen'ed.
 */
class KJSProxy {
public:
  KJSProxy(khtml::ChildFrame* frame);
  ~KJSProxy();
  
  QVariant evaluate(QString filename, int baseLine, const QString &, const DOM::Node &n,
			    KJS::Completion *completion = 0);
  void clear();
  
  DOM::EventListener *createHTMLEventHandler(QString sourceUrl, QString name, QString code, DOM::NodeImpl* node, bool svg = false);
  void finishedWithEvent(const DOM::Event &event);
  KJS::Interpreter *interpreter();

  bool isRunningScript();

  void setDebugEnabled(bool enabled);
  bool debugEnabled() const;
  void showDebugWindow(bool show = true);
  
  bool paused() const;

  void setEventHandlerLineno(int lineno) { m_handlerLineno = lineno; }

  // Helper method, to access the private KHTMLPart::jScript()
  static KJSProxy *proxy( KHTMLPart *part );
private:
  void initScript();
  void applyUserAgent();

  khtml::ChildFrame *m_frame;
  int m_handlerLineno;

  KJS::ScriptInterpreter* m_script;
#ifdef KJS_DEBUGGER
  WTF::RefPtr<KJSDebugger::DebugWindow> m_debugWindow;
#endif
  bool m_debugEnabled;
  int m_running;
#ifndef NDEBUG
  static int s_count;
#endif
};


#endif
