// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2003 George Staikos (staikos@kde.org)
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

#include "kjs_mozilla.h"
#include "kjs_mozilla.lut.h"

#include "kjs_binding.h"

#include <klocalizedstring.h>
#include <QDebug>

#include <kjs/lookup.h>
#include <khtml_part.h>

using namespace KJS;

namespace KJS {

const ClassInfo MozillaSidebarExtension::info = { "sidebar", 0, &MozillaSidebarExtensionTable, 0 };
/*
@begin MozillaSidebarExtensionTable 1
  addPanel	MozillaSidebarExtension::addPanel	DontDelete|Function 0
@end
*/
}
KJS_IMPLEMENT_PROTOFUNC(MozillaSidebarExtensionFunc)

MozillaSidebarExtension::MozillaSidebarExtension(ExecState *exec, KHTMLPart *p)
  : m_part(p) {
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

bool MozillaSidebarExtension::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "MozillaSidebarExtension::get " << propertyName.ascii();
#endif
  return getStaticPropertySlot<MozillaSidebarExtensionFunc,MozillaSidebarExtension,JSObject>
            (exec,&MozillaSidebarExtensionTable,this, propertyName, slot);
}

JSValue *MozillaSidebarExtension::getValueProperty(ExecState *exec, int token) const
{
  Q_UNUSED(exec);
  switch (token) {
  default:
    // qDebug() << "WARNING: Unhandled token in MozillaSidebarExtension::getValueProperty : " << token;
    return jsNull();
  }
}

JSValue *MozillaSidebarExtensionFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::MozillaSidebarExtension, thisObj );
  MozillaSidebarExtension *mse = static_cast<MozillaSidebarExtension*>(thisObj);

  KHTMLPart *part = mse->part();
  if (!part)
    return jsUndefined();

  // addPanel()  id == 0
  KParts::BrowserExtension *ext = part->browserExtension();
  if (ext) {
    QString url, name;
    if (args.size() == 1) {  // I've seen this, don't know if it's legal.
      name.clear();
      url = args[0]->toString(exec).qstring();
    } else if (args.size() == 2 || args.size() == 3) {
      name = args[0]->toString(exec).qstring();
      url = args[1]->toString(exec).qstring();
      // 2 is the "CURL" which I don't understand and don't think we need.
    } else {
      return jsBoolean(false);
    }
    emit ext->addWebSideBar(QUrl( url ), name);
    return jsBoolean(true);
  }

  return jsUndefined();
}


