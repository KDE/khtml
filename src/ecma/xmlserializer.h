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

#ifndef _XMLSERIALIZER_H_
#define _XMLSERIALIZER_H_

#include "ecma/kjs_binding.h"
#include "ecma/kjs_dom.h"
#include "kio/jobclasses.h"

namespace KJS
{

class XMLSerializerConstructorImp : public JSObject
{
public:
    XMLSerializerConstructorImp(ExecState *);
    bool implementsConstruct() const override;
    using KJS::JSObject::construct;
    JSObject *construct(ExecState *exec, const List &args) override;
};

class XMLSerializer : public DOMObject
{
public:
    XMLSerializer(ExecState *);
    bool toBoolean(ExecState *) const override
    {
        return true;
    }
    const ClassInfo *classInfo() const override
    {
        return &info;
    }
    static const ClassInfo info;
    enum { SerializeToString };

private:
    friend class XMLSerializerProtoFunc;
};

} // namespace

#endif
