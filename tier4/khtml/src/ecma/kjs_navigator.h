// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
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

#ifndef _KJS_NAVIGATOR_H_
#define _KJS_NAVIGATOR_H_

#include <kjs/object.h>
#include "kjs_binding.h"

class KHTMLPart;

namespace KJS {

  class Navigator : public JSObject {
  public:
    Navigator(ExecState *exec, KHTMLPart *p);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { AppCodeName, AppName, AppVersion, Language, UserAgent, UserLanguage, Platform,
           _Plugins, _MimeTypes, Product,  ProductSub, Vendor, VendorSub, CookieEnabled, JavaEnabled,
           BrowserLanguage, CpuClass };
    KHTMLPart *part() const { return m_part; }
  private:
    KHTMLPart *m_part;
  };

  // Hashtable enums
  enum { Plugins_Refresh, Plugins_Length, Plugins_Item, Plugins_NamedItem };
  enum { MimeTypes_Length, MimeTypes_Item, MimeTypes_NamedItem };
  enum { Plugin_Name, Plugin_FileName, Plugin_Description, Plugin_Length, Plugin_Item, Plugin_NamedItem };
  enum { MimeType_Type, MimeType_Description, MimeType_EnabledPlugin, MimeType_Suffixes };

} // namespace

#endif
