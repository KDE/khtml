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

#ifndef KJS_ARRAYBUFFERVIEW_H
#define KJS_ARRAYBUFFERVIEW_H

#include "ecma/kjs_arraybuffer.h"

#include <kjs/object.h>
#include <kjs/operations.h>
#include <kjs/array_instance.h>
#include <dom/dom_exception.h>

namespace KJS {
    class ArrayBuffer;

    // Keep enum outside of template classes, for lut tables
    namespace ArrayBufferViewBase {
        enum {
            Buffer, ByteLength, ByteOffset, Subarray, Length, Set, BytesPerElement
        };
    }

    //type, TypedArrayClass
    template <class T, class U>
    class ArrayBufferViewConstructorImp : public JSObject {
    public:
        ArrayBufferViewConstructorImp(ExecState *exec, DOM::DocumentImpl* d);
        virtual bool implementsConstruct() const;
        using KJS::JSObject::construct;
        virtual JSObject* construct(ExecState *exec, const List &args);

        JSValue *getValueProperty(ExecState *exec, int token) const;
        using KJS::JSObject::getOwnPropertySlot;
        bool getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot);
    private:
        SharedPtr<DOM::DocumentImpl> doc;
    };


    //type, TypedArrayPrototype
    template <class T, class P>
    class ArrayBufferView : public JSObject {
    public:
        explicit ArrayBufferView(ExecState* exec, ArrayBuffer* buffer, size_t byteOffset, size_t byteLength);
        virtual ~ArrayBufferView();

        bool getOwnPropertySlot(ExecState *exec, unsigned i, PropertySlot& slot);
        bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
        JSValue *getValueProperty(ExecState *exec, int token) const;

        void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None);
        void put(ExecState *exec, unsigned propertyName, JSValue *value, int attr = None);

        ArrayBuffer* buffer() const { return m_buffer; }
        size_t byteLength() const { return m_byteLength; }
        size_t byteOffset() const { return m_byteOffset; }
        size_t length() const { return m_length; }
        inline T* bufferStart() const { return m_bufferStart; }

    private:
        // checks if the pos is a valid array index, returns false if not
        inline bool checkIndex(ExecState* exec, unsigned pos);

        ProtectedPtr<ArrayBuffer> m_buffer;
        size_t m_byteOffset;
        size_t m_byteLength;
        size_t m_length;
        T* m_bufferStart;
    };

    //unrolled KJS_DEFINE_PROTOTYPE(ArrayBufferViewProto), for template sake

    //type, TypedArrayClass
    template <class T, class U>
    class ArrayBufferViewProto : public KJS::JSObject
    {
    public:
        bool getOwnPropertySlot(KJS::ExecState *, const KJS::Identifier&, KJS::PropertySlot&);
        using JSObject::getOwnPropertySlot;
    protected:
        ArrayBufferViewProto(KJS::ExecState *exec);
    };

    //unrolled KJS_IMPLEMENT_PROTOFUNC(ArrayBufferViewProtoFunc), for template sake
    template <class T, class U>
    class ArrayBufferViewProtoFunc : public KJS::InternalFunctionImp {
    public:
        ArrayBufferViewProtoFunc<T, U>(KJS::ExecState* exec, int i, int len, const KJS::Identifier& name)
            : InternalFunctionImp(static_cast<KJS::FunctionPrototype*>(exec->lexicalInterpreter()->builtinFunctionPrototype()), name), id(i) {
            put(exec, exec->propertyNames().length, KJS::jsNumber(len), KJS::DontDelete|KJS::ReadOnly|KJS::DontEnum);
        }

        virtual KJS::JSValue *callAsFunction(KJS::ExecState *exec, KJS::JSObject *thisObj, const KJS::List &args);
    private:
        int id;
    };
}

#include "kjs_arraybufferview.lut.h"

namespace KJS {

    // unrolled KJS_IMPLEMENT_PROTOTYPE("ArrayBufferView", ArrayBufferViewProto, ArrayBufferViewProtoFunc, ObjectPrototype)
    // for template sake
    template <class T, class U>
    bool ArrayBufferViewProto<T, U>::getOwnPropertySlot(KJS::ExecState *exec, const KJS::Identifier& propertyName, KJS::PropertySlot& slot) {
        return KJS::getStaticFunctionSlot<ArrayBufferViewProtoFunc<T, U> , KJS::JSObject>(exec, &ArrayBufferViewProtoTable, this, propertyName, slot);
    }

    template <class T, class U>
    ArrayBufferViewProto<T, U>::ArrayBufferViewProto(KJS::ExecState *exec)
        : KJS::JSObject(ObjectPrototype::self(exec))
    {
    }
    // unroll end

    // -------------------- ArrayBufferViewProtoFunc ---------------------

    template <class T, class U>
    JSValue *ArrayBufferViewProtoFunc<T, U>::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
    {
        if (!thisObj->inherits(&U::info)) {
            return jsUndefined();
        }
        U *view = static_cast<U*>(thisObj);

        switch (id) {
            case ArrayBufferViewBase::Subarray: {
                //Subarray takes max long/signed size_t
                // If start or end are negative, it refers to an index from the end of the array
                ssize_t begin = 0;
                ssize_t end = 0;
                double tmp;
                if (args[0]->getNumber(tmp))
                    begin = static_cast<ssize_t>(tmp);
                if (args.size() >= 2 && args[1]->getNumber(tmp))
                    end = static_cast<ssize_t>(tmp);

                // turn negative start/end into a valid positive index
                if (begin < 0 && view->length() > static_cast<size_t>(-begin))
                    begin = view->length() + begin;
                if (end < 0 && view->length() > static_cast<size_t>(-end))
                    end = view->length() + end;

                if (static_cast<size_t>(begin) > view->length())
                    begin = view->length();
                if (static_cast<size_t>(end) > view->length())
                    end = 0;

                size_t length = 0;
                if (begin < end)
                    length = (end - begin) * sizeof(T);

                U *newView = new U(exec, view->buffer(), begin * sizeof(T), length);
                return newView;
            }
            case ArrayBufferViewBase::Set: {
                JSObject* obj = args[0]->getObject();
                if (!obj)
                    return jsUndefined();
                if (obj->inherits(&U::info))
                {
                    U *other = static_cast<U*>(obj);
                    double tmp;
                    size_t offset = 0;
                    if (args.size() >= 2 && args[1]->getNumber(tmp) && tmp > 0)
                        offset = static_cast<size_t>(tmp) * sizeof(T);

                    if (offset > other->byteLength() || other->byteLength() - offset > view->byteLength()) {
                        setDOMException(exec, DOM::DOMException::INDEX_SIZE_ERR);
                        return jsUndefined();
                    }
                    memcpy(view->buffer()->buffer(), other->buffer()->buffer() + offset, std::max<ssize_t>(static_cast<ssize_t>(other->byteLength()) - static_cast<ssize_t>(offset), 0));
                    return jsUndefined();
                } else if (obj->inherits(&ArrayInstance::info)) {
                    ArrayInstance* array = static_cast<ArrayInstance*>(obj);
                    if (array->getLength() > view->length()) {
                        setDOMException(exec, DOM::DOMException::INDEX_SIZE_ERR);
                        return jsUndefined();
                    }
                    for (unsigned i = 0; i < array->getLength(); ++i)
                    {
                        view->put(exec, i, array->getItem(i));
                    }
                    return jsUndefined();
                } else {
                    return jsUndefined();
                }
            }
            default:
                ASSERT_NOT_REACHED();
                return jsUndefined();
        }
    }


    // -------------------- ArrayBufferViewConstructorImp ---------------------

    template <class T, class U>
    ArrayBufferViewConstructorImp<T, U>::ArrayBufferViewConstructorImp(ExecState* exec, DOM::DocumentImpl* d)
        : JSObject(exec->lexicalInterpreter()->builtinFunctionPrototype()),
          doc(d)
    {
    }

    template <class T, class U>
    bool ArrayBufferViewConstructorImp<T, U>::implementsConstruct() const
    {
        return true;
    }

    template <class T, class U>
    JSObject *ArrayBufferViewConstructorImp<T, U>::construct(ExecState *exec, const List &args)
    {
        JSType type = args[0]->type();

        switch (type) {
            case ObjectType: {
                JSObject* obj = args[0]->getObject();
                if (!obj)
                    return throwError(exec, TypeError);
                if (obj->inherits(&ArrayBuffer::info)) {
                    // new ArrayBufferView(ArrayBuffer, [byteOffset[, byteLength]])
                    ArrayBuffer* buf = static_cast<ArrayBuffer*>(obj);

                    size_t byteOffset = 0, byteLength = 0;
                    double tmp;
                    if (args.size() >= 2 && args[1]->getNumber(tmp) && tmp > 0)
                        byteOffset = static_cast<size_t>(tmp);

                    if (args.size() >= 3 && args[2]->getNumber(tmp) && tmp > 0)
                        byteLength = static_cast<size_t>(tmp) * sizeof(T);

                    return new U(exec, buf, byteOffset, byteLength);
                } else if (obj->inherits(&ArrayInstance::info)) {
                    // new ArrayBufferView(Array)
                    ArrayInstance* arr = dynamic_cast<ArrayInstance*>(obj);
                    ArrayBuffer* buf = new ArrayBuffer(arr->getLength()*sizeof(T));
                    U* view = new U(exec, buf, 0, 0);
                    for (unsigned i = 0; i < arr->getLength(); ++i) {
                        view->put(exec, i, arr->getItem(i));
                    }
                    return view;
                } else if (obj->inherits(&U::info)) {
                    // new ArrayBufferView(ArrayBufferView)
                    U* arr = static_cast<U*>(obj);
                    ArrayBuffer* buf = new ArrayBuffer(arr->buffer()->buffer(), arr->byteLength());
                    U* view = new U(exec, buf, 0, 0);
                    return view;
                } else {
                    break;
                }
            }
            case NumberType: {
                // new ArrayBufferView(Length)
                size_t length = 0;
                double lengthF = args[0]->getNumber();
                if (!KJS::isNaN(lengthF) && !KJS::isInf(lengthF) && lengthF > 0)
                    length = static_cast<size_t>(lengthF);
                ArrayBuffer* buf = new ArrayBuffer(length*sizeof(T));
                return new U(exec, buf, 0, 0);
            }
            default:
                break;
        }
        // new ArrayBufferView(), create empty ArrayBuffer
        ArrayBuffer* buf = new ArrayBuffer(0);
        return new U(exec, buf, 0, 0);
    }

    template <class T, class U>
    JSValue* ArrayBufferViewConstructorImp<T, U>::getValueProperty(ExecState *, int) const
    {
        // switch (id)
        // {
             // case ArrayBufferViewFuncImp<T>::BytesPerElement:
                return jsNumber(sizeof(T));
        // }
    }

    template <class T, class U>
    bool ArrayBufferViewConstructorImp<T, U>::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
    {
        return getStaticValueSlot<ArrayBufferViewConstructorImp<T, U>, JSObject>(exec, &ArrayBufferViewConstTable, this, propertyName, slot);
    }


    // -------------------- ArrayBufferView ---------------------

    template <class T, class P>
    ArrayBufferView<T, P>::ArrayBufferView(KJS::ExecState* exec, KJS::ArrayBuffer* buffer, size_t byteOffset, size_t byteLength)
        : JSObject(),
          m_buffer(buffer),
          m_byteOffset(byteOffset)
    {
        if (byteLength == 0) {
            if (byteOffset < buffer->byteLength())
                m_byteLength = buffer->byteLength() - byteOffset;
            else
                m_byteLength = 0;
        } else {
            m_byteLength = byteLength;
        }
        m_length = m_byteLength / sizeof(T);
        setPrototype(P::self(exec));
        m_bufferStart = reinterpret_cast<T*>(m_buffer->buffer() + m_byteOffset);
    }

    template <class T, class P>
    ArrayBufferView<T, P>::~ArrayBufferView()
    {
    }

    template <class T, class P>
    bool ArrayBufferView<T, P>::getOwnPropertySlot(ExecState* exec, unsigned int i, PropertySlot& slot)
    {
        if (!checkIndex(exec, i))
            return false;

        slot.setValue(this, jsNumber(m_bufferStart[i]));
        return true;
    }

    template <class T, class P>
    bool ArrayBufferView<T, P>::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
    {
        bool ok = false;
        unsigned i = propertyName.toArrayIndex(&ok);
        if (ok)
            return ArrayBufferView<T, P>::getOwnPropertySlot(exec, i, slot);

        return getStaticValueSlot< ArrayBufferView<T, P>, JSObject>(exec, &ArrayBufferViewTable, this, propertyName, slot);
    }

    template <class T, class P>
    JSValue *ArrayBufferView<T, P>::getValueProperty(ExecState * /*exec*/, int token) const
    {
        switch (token) {
            case ArrayBufferViewBase::Buffer:
                return m_buffer;
            case ArrayBufferViewBase::ByteLength:
                return jsNumber(m_byteLength);
            case ArrayBufferViewBase::ByteOffset:
                return jsNumber(m_byteOffset);
            case ArrayBufferViewBase::Length:
                return jsNumber(m_length);
            default:
                ASSERT_NOT_REACHED();
                qWarning() << "ArrayBufferView<T>::getValueProperty unhandled token " << token;
                break;
        }
        return 0;
    }

    template <class T, class P>
    void ArrayBufferView<T, P>::put(ExecState* exec, const Identifier& propertyName, JSValue* value, int attr)
    {
        bool ok = false;
        unsigned i = propertyName.toArrayIndex(&ok);
        if (ok)
            ArrayBufferView<T, P>::put(exec, i, value, attr);
        else
            KJS::JSObject::put(exec, propertyName, value, attr);
    }

    template <class T, class P>
    void ArrayBufferView<T, P>::put(ExecState* exec, unsigned int i, JSValue* value, int /*attr*/)
    {
        if (!checkIndex(exec, i))
            return;
        if (value && value->type() != NumberType)
            return;
        m_bufferStart[i] = static_cast<T>(value->getNumber());
    }

    template <class T, class P>
    bool ArrayBufferView<T, P>::checkIndex(KJS::ExecState* /*exec*/, unsigned int pos)
    {
        if (m_byteOffset + (pos+1)*sizeof(T) > m_buffer->byteLength())
            return false;
        if (pos*sizeof(T) >= m_byteLength)
            return false;
        return true;
    }

} // namespace KJS

#endif //KJS_ARRAYBUFFERVIEW_H
