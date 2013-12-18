/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999 Harri Porten (porten@kde.org)
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
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef JSHTMLElement_h
#define JSHTMLElement_h

#include <kjs/object.h>
#include <kjs/ExecState.h>
#include <kjs/lookup.h>
#include "kjs_dom.h"
#include <html/html_elementimpl.h>

//
// Temporary replacement for a file that should actually be
// automatically generated from HTMLElement.idl
//

namespace khtml {
  
  class JSHTMLElement : public KJS::DOMElement
  {
  public:
      JSHTMLElement(KJS::ExecState* exec, DOM::HTMLElementImpl* impl)
         : KJS::DOMElement(KJS::DOMElementProto::self(exec), impl) { }

      virtual ~JSHTMLElement() { }

      static const KJS::ClassInfo s_info;
  };

  class JSHTMLElementPrototype : public KJS::JSObject {
  public:
      static KJS::JSObject* self(KJS::ExecState* exec)
      {
	  // ### should actually be DOMHTMLElementProto
	  return KJS::DOMElementProto::self(exec);
      }
  };

}

#endif

