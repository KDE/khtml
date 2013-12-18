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

#include "domparser.h"
#include "domparser.lut.h"

#include "kjs_dom.h"
#include "kjs_window.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_docimpl.h"

#include "html/html_documentimpl.h"

using DOM::DocumentImpl;

////////////////////// DOMParser Object ////////////////////////

/* Source for DOMParserProtoTable.
@begin DOMParserProtoTable 1
  parseFromString DOMParser::ParseFromString DontDelete|Function 2
@end
*/

using namespace KJS;

namespace KJS {

KJS_DEFINE_PROTOTYPE(DOMParserProto)
KJS_IMPLEMENT_PROTOFUNC(DOMParserProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMParser",DOMParserProto,DOMParserProtoFunc,ObjectPrototype)


DOMParserConstructorImp::DOMParserConstructorImp(ExecState* exec, DOM::DocumentImpl *d)
    : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), doc(d)
{
}

bool DOMParserConstructorImp::implementsConstruct() const
{
  return true;
}

JSObject *DOMParserConstructorImp::construct(ExecState *exec, const List &)
{
  return new DOMParser(exec, doc.get());
}

const ClassInfo DOMParser::info = { "DOMParser", 0, 0 /* &DOMParserTable*/, 0 };


DOMParser::DOMParser(ExecState *exec, DOM::DocumentImpl *d)
  : doc(d)
{
   setPrototype(DOMParserProto::self(exec));
}


JSValue *DOMParserProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( DOMParser, thisObj );

  DOMParser *parser = static_cast<DOMParser *>(thisObj);

  switch (id) {
  case DOMParser::ParseFromString:
    {
      if (args.size() != 2) {
	return jsUndefined();
      }

      QString str = args[0]->toString(exec).qstring();
      QString contentType = args[1]->toString(exec).qstring().trimmed();

      if (contentType == "text/xml" || contentType == "application/xml" || contentType == "application/xhtml+xml") {
        SharedPtr<DocumentImpl> docImpl = parser->doc->implementation()->createDocument();

        docImpl->open();
        docImpl->write(str);
        docImpl->finishParsing();
        docImpl->close();

        return getDOMNode(exec, docImpl.get());
      }
    }
  }

  return jsUndefined();
}

} // end namespace


