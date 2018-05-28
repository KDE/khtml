/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 George Staikos (staikos@kde.org)
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

#ifndef _KJS_MOZILLA_H_
#define _KJS_MOZILLA_H_

#include <kjs/object.h>
#include "kjs_binding.h"

class KHTMLPart;

namespace KJS
{

class MozillaSidebarExtension : public JSObject
{
public:
    MozillaSidebarExtension(ExecState *exec, KHTMLPart *p);

    using KJS::JSObject::getOwnPropertySlot;
    bool getOwnPropertySlot(ExecState *exec, const Identifier &propertyName, PropertySlot &slot) override;
    JSValue *getValueProperty(ExecState *exec, int token) const;
    const ClassInfo *classInfo() const override
    {
        return &info;
    }
    static const ClassInfo info;
    enum { addPanel };
    KHTMLPart *part() const
    {
        return m_part;
    }
private:
    KHTMLPart *m_part;
};
} // namespace

#endif
