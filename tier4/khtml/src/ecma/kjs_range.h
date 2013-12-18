// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2009 Maksim Orlovich (maksim@kde.org)
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

#ifndef _KJS_RANGE_H_
#define _KJS_RANGE_H_

#include <QPointer>

#include "ecma/kjs_dom.h"
#include "xml/dom_docimpl.h"
#include "xml/dom2_rangeimpl.h"
#include "xml/dom_selection.h"

namespace KJS {

  class DOMRange : public DOMObject {
  public:
    DOMRange(ExecState *exec, DOM::RangeImpl* r);
    ~DOMRange();
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { StartContainer, StartOffset, EndContainer, EndOffset, Collapsed,
           CommonAncestorContainer,
           SetStart, SetEnd, SetStartBefore, SetStartAfter, SetEndBefore,
           SetEndAfter, Collapse, SelectNode, SelectNodeContents,
           CompareBoundaryPoints, DeleteContents, ExtractContents,
           CloneContents, InsertNode, SurroundContents, CloneRange, ToString,
           Detach, CreateContextualFragment };
    DOM::RangeImpl *impl() const { return m_impl.get(); }
  protected:
    SharedPtr<DOM::RangeImpl> m_impl;
  };

  class DOMSelection: public JSObject {
  public:
    DOMSelection(ExecState* exec, DOM::DocumentImpl* parentDocument);

    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only

    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    enum { AnchorNode, AnchorOffset, FocusNode, FocusOffset, IsCollapsed,
           Collapsed, CollapseToStart, CollapseToEnd, SelectAllChildren,
           DeleteFromDocument, RangeCount, GetRangeAt, AddRange, RemoveRange, RemoveAllRanges, ToString };

    DOM::Selection currentSelection() const;
    bool           attached() const; // if document & part are still alive..
    QPointer<DOM::DocumentImpl> m_document;  
  };

  // Constructor object Range
  class RangeConstructor : public DOMObject {
  public:
    RangeConstructor(ExecState *);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  JSValue* getDOMRange(ExecState *exec, DOM::RangeImpl* r);
  JSValue* getRangeConstructor(ExecState *exec);
  JSObject* getRangeExceptionConstructor(ExecState *exec);

  /**
   * Convert an object to a RangeImpl. Returns 0 if not possible
   */
  DOM::RangeImpl* toRange(JSValue*);

  DEFINE_PSEUDO_CONSTRUCTOR(RangeExceptionPseudoCtor)

  class RangeException : public DOMObject {
  public:
    RangeException(ExecState* exec);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

} // namespace

#endif
// kate: indent-width 2; replace-tabs on; tab-width 4; space-indent on;
