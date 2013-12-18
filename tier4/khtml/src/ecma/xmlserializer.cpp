// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 Apple Computer, Inc.
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

#include "xmlserializer.h"
#include "xmlserializer.lut.h"

#include "dom/dom_exception.h"
#include "dom/dom_doc.h"
#include "xml/dom_docimpl.h"

using namespace KJS;

////////////////////// XMLSerializer Object ////////////////////////

/* Source for XMLSerializerProtoTable.
@begin XMLSerializerProtoTable 1
  serializeToString XMLSerializer::SerializeToString DontDelete|Function 1
@end
*/
namespace KJS {
KJS_DEFINE_PROTOTYPE(XMLSerializerProto)
KJS_IMPLEMENT_PROTOFUNC(XMLSerializerProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("XMLSerializer", XMLSerializerProto,XMLSerializerProtoFunc, ObjectPrototype)

XMLSerializerConstructorImp::XMLSerializerConstructorImp(ExecState* exec)
    : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype())
{
}

bool XMLSerializerConstructorImp::implementsConstruct() const
{
  return true;
}

JSObject *XMLSerializerConstructorImp::construct(ExecState *exec, const List &)
{
  return new XMLSerializer(exec);
}

const ClassInfo XMLSerializer::info = { "XMLSerializer", 0, 0, 0 };

XMLSerializer::XMLSerializer(ExecState *exec)
{
  setPrototype(XMLSerializerProto::self(exec));
}

JSValue *XMLSerializerProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( XMLSerializer, thisObj );

  switch (id) {
  case XMLSerializer::SerializeToString:
    {
      if (args.size() != 1) {
	return jsUndefined();
      }

      if (!args[0]->toObject(exec)->inherits(&DOMNode::info)) {
	return jsUndefined();
      }

      DOM::NodeImpl* node = static_cast<KJS::DOMNode *>(args[0]->toObject(exec))->impl();

      if (!node) {
	return jsUndefined();
      }

      DOMString body;

      try {
	  body = node->toString();
      } catch(DOM::DOMException&) {
	  JSObject *err = Error::create(exec, GeneralError, "Exception serializing document");
	  exec->setException(err);
	  return err;
      }

      return ::getStringOrNull(body);
    }
  }

  return jsUndefined();
}

} // end namespace
