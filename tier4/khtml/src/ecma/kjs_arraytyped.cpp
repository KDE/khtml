/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2013 Bernd Buschinski <b.buschinski@googlemail.com>
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

#include "kjs_arraytyped.h"

namespace KJS {

// Define TypedArrayTypes
// type == the type, like int8_t
// TypeName == the readable Type, like Int8
// this will define 
// ArrayBufferConstructorImpInt8
// ArrayBufferViewProtoInt8
// ArrayBufferViewInt8

#define KJS_TYPEDARRAY_DEFINE(type,TypeName) \
JSObject* ArrayBufferConstructorImp##TypeName::construct(ExecState *exec, const List &args) \
{ \
    return ArrayBufferViewConstructorImp<type, ArrayBufferView##TypeName>::construct(exec, args); \
} \
 \
Identifier* ArrayBufferViewProto##TypeName::name() \
{ \
    if (!s_name) \
        s_name = new KJS::Identifier("[[" "ArrayBuffer"#TypeName  ".prototype]]"); \
    return s_name; \
} \
 \
KJS::Identifier* ArrayBufferViewProto##TypeName::s_name = 0; \
const KJS::ClassInfo ArrayBufferViewProto##TypeName::info = { "ArrayBuffer"#TypeName , 0, &ArrayBufferViewProtoTable, 0 }; \
 \
KJS::JSObject *ArrayBufferViewProto##TypeName::self(KJS::ExecState *exec) { \
    return KJS_CACHEGLOBALOBJECT_NS cacheGlobalObject<ArrayBufferViewProto##TypeName >(exec, *name()); \
} \
ArrayBufferViewProto##TypeName::ArrayBufferViewProto##TypeName(KJS::ExecState *exec) \
    : ArrayBufferViewProto<type, ArrayBufferView##TypeName>(exec) \
{} \
 \
const KJS::ClassInfo ArrayBufferView##TypeName::info = { "ArrayBuffer"#TypeName , 0, &ArrayBufferViewProtoTable, 0 }; \
 \
ArrayBufferView##TypeName::ArrayBufferView##TypeName(ExecState* exec, ArrayBuffer* buffer, size_t byteOffset, size_t byteLength) \
    : ArrayBufferView<type, ArrayBufferViewProto##TypeName>(exec,buffer, byteOffset, byteLength) \
{}


KJS_TYPEDARRAY_DEFINE(int8_t, Int8)
KJS_TYPEDARRAY_DEFINE(uint8_t, Uint8)
KJS_TYPEDARRAY_DEFINE(int16_t, Int16)
KJS_TYPEDARRAY_DEFINE(uint16_t, Uint16)
KJS_TYPEDARRAY_DEFINE(int32_t, Int32)
KJS_TYPEDARRAY_DEFINE(uint32_t, Uint32)

KJS_TYPEDARRAY_DEFINE(float, Float32)
KJS_TYPEDARRAY_DEFINE(double, Float64)

}
