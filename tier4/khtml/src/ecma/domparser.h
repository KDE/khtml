// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2005 Anders Carlsson (andersca@mac.com)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DOMPARSER_H
#define DOMPARSER_H

#include <khtml_export.h>
#include <kjs/object.h>
#include <kjs/interpreter.h>
#include <misc/shared.h>
#include <QtCore/QPointer>

#include "kjs_dom.h"

namespace KJS {

  class DOMParserConstructorImp : public JSObject {
  public:
    DOMParserConstructorImp(ExecState *, DOM::DocumentImpl *d);
    virtual bool implementsConstruct() const;
    using KJS::JSObject::construct;
    virtual JSObject *construct(ExecState *exec, const List &args);
private:
    SharedPtr<DOM::DocumentImpl> doc;
  };

  class DOMParser : public DOMObject {
  public:
    DOMParser(ExecState *, DOM::DocumentImpl *d);
    virtual bool toBoolean(ExecState *) const { return true; }
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { ParseFromString };

  private:
    QPointer<DOM::DocumentImpl> doc;

    friend class DOMParserProtoFunc;
  };

} // namespace

#endif
