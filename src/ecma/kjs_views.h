// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
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

#ifndef _KJS_VIEWS_H_
#define _KJS_VIEWS_H_

#include "ecma/kjs_dom.h"
#include "xml/dom2_viewsimpl.h"

namespace KJS {


  class DOMAbstractView : public DOMObject {
  public:
    DOMAbstractView(ExecState *, DOM::AbstractViewImpl* av);
    ~DOMAbstractView();

    JSValue *getValueProperty(ExecState *exec, int token);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    virtual DOM::AbstractViewImpl* impl() const { return m_impl.get(); }
    enum { Document, GetComputedStyle };
  protected:
    SharedPtr<DOM::AbstractViewImpl> m_impl;
  };

  JSValue* getDOMAbstractView(ExecState *exec, DOM::AbstractViewImpl* av);

  /**
   * Convert an object to an AbstractView. Returns a null Node if not possible.
   */
  DOM::AbstractViewImpl* toAbstractView(JSValue*);

} // namespace

#endif
