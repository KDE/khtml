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

#ifndef KJS_ARRAYTYPES_H
#define KJS_ARRAYTYPES_H

#include "kjs_arraybufferview.h"

namespace KJS {

// Declare a TypedArray Class, type == the type, for example int8_t
// and TypeName is a readable name for the Type, for example Int8.
// Use this Macro to declare all typed arrays types.

#define KJS_TYPEDARRAY_DECLARE(type,TypeName) \
class ArrayBufferViewProto##TypeName; \
class ArrayBufferView##TypeName : public ArrayBufferView<type, ArrayBufferViewProto##TypeName> { \
public: \
    explicit ArrayBufferView##TypeName(ExecState* exec, ArrayBuffer* buffer, size_t byteOffset, size_t byteLength); \
    virtual ~ArrayBufferView##TypeName() {}; \
 \
    static const ClassInfo info; \
    virtual const ClassInfo* classInfo() const { return &info; } \
}; \
 \
class ArrayBufferConstructorImp##TypeName : public ArrayBufferViewConstructorImp<type, ArrayBufferView##TypeName> \
{ \
public: \
    ArrayBufferConstructorImp##TypeName(ExecState *exec, DOM::DocumentImpl* d) \
        : ArrayBufferViewConstructorImp<type, ArrayBufferView##TypeName>(exec, d) \
    { } \
    using KJS::JSObject::construct; \
    virtual JSObject* construct(ExecState *exec, const List &args); \
}; \
 \
class ArrayBufferViewProto##TypeName : public ArrayBufferViewProto<type, ArrayBufferView##TypeName> \
{ \
friend KJS::JSObject* KJS_CACHEGLOBALOBJECT_NS cacheGlobalObject<ArrayBufferViewProto##TypeName>(KJS::ExecState *exec, const KJS::Identifier &propertyName); \
public: \
    virtual const KJS::ClassInfo *classInfo() const { return &info; } \
    static const KJS::ClassInfo info; \
    static KJS::JSObject *self(KJS::ExecState *exec); \
protected: \
    ArrayBufferViewProto##TypeName(KJS::ExecState *exec); \
    static KJS::Identifier* s_name; \
    static KJS::Identifier* name(); \
};


KJS_TYPEDARRAY_DECLARE(int8_t, Int8)
KJS_TYPEDARRAY_DECLARE(uint8_t, Uint8)
KJS_TYPEDARRAY_DECLARE(int16_t, Int16)
KJS_TYPEDARRAY_DECLARE(uint16_t, Uint16)
KJS_TYPEDARRAY_DECLARE(int32_t, Int32)
KJS_TYPEDARRAY_DECLARE(uint32_t, Uint32)

KJS_TYPEDARRAY_DECLARE(float, Float32)
KJS_TYPEDARRAY_DECLARE(double, Float64)


#undef KJS_TYPEDARRAY_DECLARE

}

#endif //KJS_ARRAYTYPES_H
