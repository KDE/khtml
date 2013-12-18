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

#ifndef KJS_ARRAYBUFFER_H
#define KJS_ARRAYBUFFER_H

#include "ecma/kjs_dom.h"

#include <kjs/object.h>

namespace KJS {

    class ArrayBufferConstructorImp : public JSObject {
    public:
        ArrayBufferConstructorImp(ExecState *exec, DOM::DocumentImpl* d);
        virtual bool implementsConstruct() const;
        using KJS::JSObject::construct;
        virtual JSObject* construct(ExecState *exec, const List &args);
    private:
        SharedPtr<DOM::DocumentImpl> doc;
    };

    class ArrayBuffer : public JSObject {
    public:
        explicit ArrayBuffer(size_t size);
        //copy the buffer
        explicit ArrayBuffer(uint8_t* buffer, size_t size);
        virtual ~ArrayBuffer();

        enum {
            ByteLength, Splice
        };

        using KJS::JSObject::getOwnPropertySlot;
        bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
        JSValue *getValueProperty(ExecState *exec, int token) const;

        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;

        size_t byteLength() const { return m_size; }
        uint8_t* buffer() const { return m_buffer; }

    private:
        size_t m_size;
        uint8_t* m_buffer;
    };

} // namespace KJS

#endif
