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

#include "ecma/kjs_arraybuffer.h"

#include <kjs/operations.h>

#include "kjs_arraybuffer.lut.h"

namespace KJS {

ArrayBufferConstructorImp::ArrayBufferConstructorImp(ExecState* exec, DOM::DocumentImpl* d)
    : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), doc(d)
{
}

bool ArrayBufferConstructorImp::implementsConstruct() const
{
    return true;
}

JSObject *ArrayBufferConstructorImp::construct(ExecState* /*exec*/, const List &args)
{
    double sizeF = 0.0;
    size_t size = 0;
    if (args[0]->getNumber(sizeF)) {
        if (!KJS::isNaN(sizeF) && !KJS::isInf(sizeF) && sizeF > 0)
            size = static_cast<size_t>(sizeF);
    }

    return new ArrayBuffer(size);
}


/* Source for ArrayBufferProtoTable.

@begin ArrayBufferProtoTable 1
    splice  ArrayBuffer::Splice     Function|DontDelete     1
@end

*/

KJS_DEFINE_PROTOTYPE(ArrayBufferProto)
KJS_IMPLEMENT_PROTOFUNC(ArrayBufferProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("ArrayBuffer", ArrayBufferProto, ArrayBufferProtoFunc, ObjectPrototype)

const ClassInfo ArrayBuffer::info = { "ArrayBuffer", 0, &ArrayBufferTable, 0 };


/* Source for ArrayBufferTable.

@begin ArrayBufferTable 1
    byteLength  ArrayBuffer::ByteLength     ReadOnly|DontDelete
@end

*/

ArrayBuffer::ArrayBuffer(size_t size)
    : JSObject(),
      m_size(size),
      m_buffer(0)
{
    if (m_size > 0) {
        m_buffer = new uint8_t[m_size];
        memset(m_buffer, 0, m_size);
    }
}

ArrayBuffer::ArrayBuffer(uint8_t* buffer, size_t size)
    : JSObject(),
      m_size(size),
      m_buffer(0)
{
    if (m_size > 0) {
        m_buffer = new uint8_t[m_size];
        memcpy(m_buffer, buffer, m_size);
    }
}


ArrayBuffer::~ArrayBuffer()
{
    delete[] m_buffer;
}

bool ArrayBuffer::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<ArrayBuffer, JSObject>(exec, &ArrayBufferTable, this, propertyName, slot);
}

JSValue* ArrayBuffer::getValueProperty(ExecState* /*exec*/, int token) const
{
    switch(token)
    {
        case ByteLength:
            return jsNumber(m_size);
        default:
            return jsUndefined();
    }
}


// -------------------------- ArrayBufferProtoFunc ----------------------------

JSValue *ArrayBufferProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    if (thisObj->inherits(&ArrayBuffer::info)) {
        return throwError(exec, TypeError);
    }

    ArrayBuffer* arraybuf = static_cast<ArrayBuffer*>(thisObj);
    switch (id)
    {
        case ArrayBuffer::Splice:
        {
            // Slice takes max long/signed size_t
            // If start or end are negative, it refers to an index from the end of the array
            ssize_t start = 0;
            ssize_t end = 0;
            double tmp;
            if (args[0]->getNumber(tmp))
                start = static_cast<ssize_t>(tmp);
            if (args.size() >= 2 && args[1]->getNumber(tmp))
                end = static_cast<ssize_t>(tmp);

            // turn negative start/end into a valid positive index
            if (start < 0 && arraybuf->byteLength() > static_cast<size_t>(-start))
                start = arraybuf->byteLength() + start;
            if (end < 0 && arraybuf->byteLength() > static_cast<size_t>(-end))
                end = arraybuf->byteLength() + end;

            if (static_cast<size_t>(start) > arraybuf->byteLength())
                start = arraybuf->byteLength();
            if (static_cast<size_t>(end) > arraybuf->byteLength())
                end = 0;

            size_t length = 0;
            if (start < end)
                length = end - start;
            else if (args.size() < 2 && start > 0 && arraybuf->byteLength() > static_cast<size_t>(start))
                length = arraybuf->byteLength() - start;

            ArrayBuffer* ret = new ArrayBuffer(length);
            memcpy(ret->buffer(), arraybuf->buffer()+start, length);
            return ret;
        }
        default:
            return jsUndefined();
    }
};

} // namespace KJS
