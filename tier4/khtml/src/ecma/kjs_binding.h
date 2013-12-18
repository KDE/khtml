// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003 Apple Computer, Inc.
 *  Copyright (C) 2007, 2008 Maksim Orlovich (maksim@kde.org)
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

#ifndef _KJS_BINDING_H_
#define _KJS_BINDING_H_

#include <khtml_export.h>
#include <kjs/interpreter.h>
#include <kjs/global.h>
#include <wtf/HashMap.h>

#include <dom/dom_node.h>
#include <QtCore/QVariant>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <kjs/lookup.h>
#include <kjs/function.h>
#include <kjs/JSVariableObject.h>
#include <kjs/object_object.h>
#include <misc/shared.h>

#include <stdlib.h> // for abort

#define KJS_CHECK_THIS( ClassName, theObj ) \
       if (!theObj || !theObj->inherits(&ClassName::info)) { \
               KJS::UString errMsg = "Attempt at calling a function that expects a "; \
               errMsg += ClassName::info.className; \
               errMsg += " on a "; \
               errMsg += theObj->className(); \
               KJS::JSObject *err = KJS::Error::create(exec, KJS::TypeError, errMsg.ascii()); \
               exec->setException(err); \
               return err; \
       }

namespace KParts {
  class ReadOnlyPart;
  class LiveConnectExtension;
}

namespace khtml {
  class ChildFrame;
}

namespace KJS {

  // For the ecma debugger we provide our own conversion rather than the
  // use the native one, since using the toString
  // method on object can invoke code
  QString valueToString(KJS::JSValue* value);

  // Serializes an exception for human consumption.
  QString exceptionToString(ExecState* exec, JSValue* exception);

  /**
   * Base class for all objects in this binding. Doesn't manage exceptions any more
   */
  class DOMObject : public JSObject {
  protected:
    DOMObject() : JSObject() {}
    DOMObject(JSObject *proto) : JSObject(proto) {}
  public:
    bool shouldMark() const { return !_prop.isEmpty(); }
    virtual UString toString(ExecState *exec) const;
  };

  /**
   * We inherit from Interpreter, to save a pointer to the HTML part
   * that the interpreter runs for.
   * The interpreter also stores the DOM object - >KJS::DOMObject cache.
   */
  class ScriptInterpreter : public Interpreter
  {
  public:
    ScriptInterpreter( JSGlobalObject *global, khtml::ChildFrame* frame );
    virtual ~ScriptInterpreter();

    // We need to keep track of wrappers in 2 ways:
    //  - we want the same wrapper for the same node (see #145775)
    //  - we want to drop all the references from this interpreter on clear, so
    //  wrappers don't stick around. Hence we have a global set and a per-interpreter one.

    // Reuses an existing wrapper, perhaps also updating the current map
    // to refer to it as well.
    DOMObject* getDOMObject( void* objectHandle ) {
      DOMObject* existing = allDomObjects()->get( objectHandle );
      if (existing)
          m_domObjects.set(objectHandle, existing );
      return existing;
    }

    void putDOMObject( void* objectHandle, DOMObject* obj ) {
      allDomObjects()->set( objectHandle, obj );
      m_domObjects.set( objectHandle, obj );
    }

    static void forgetDOMObject( void* objectHandle );

    void clear() {
      m_domObjects.clear(); // Global set will be cleared at GC time.
    }


    /**
     * Mark objects in the DOMObject cache.
     */
    virtual void mark(bool isMain);
    KParts::ReadOnlyPart* part() const;

    virtual int rtti() { return 1; }

    /**
     * Set the event that is triggering the execution of a script, if any
     */
    void setCurrentEvent( DOM::Event *evt ) { m_evt = evt; }
    void setInlineCode( bool inlineCode ) { m_inlineCode = inlineCode; }
    void setProcessingTimerCallback( bool timerCallback ) { m_timerCallback = timerCallback; }
    /**
     * "Smart" window.open policy
     */
    bool isWindowOpenAllowed() const;

    /**
     * CPU guard API. This should be used instead of Interpreter
     * methods as it manages the timeouts, including VG support
     */
    virtual bool shouldInterruptScript() const;
    void startCPUGuard();
    void stopCPUGuard();

    static void turnOffCPUGuard() {
        s_disableCPUGuard = true;
    }
  private:
    khtml::ChildFrame* m_frame;
    HashMap<void*, DOMObject*> m_domObjects;
    static HashMap<void*, DOMObject*>* s_allDomObjects;
    static HashMap<void*, DOMObject*>* allDomObjects() {
        if (!s_allDomObjects)
            s_allDomObjects = new HashMap<void*, DOMObject*>();
        return s_allDomObjects;
    }

    DOM::Event *m_evt;
    bool m_inlineCode;
    bool m_timerCallback;
    static bool s_disableCPUGuard;
  };

  /** Some templates to help make wrappers. example:
      class Foo: public DOMWrapperObject<DOM::FooImpl> {
      }

      ...

      getWrapper<Foo>(exec, someImpl);
  */
  template<typename Wrapper>
  JSValue* getWrapper(ExecState *exec, typename Wrapper::wrappedType* g)
  {
      DOMObject *ret = 0;
      if (!g)
          return jsNull();

      ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());
      if ((ret = interp->getDOMObject(g)))
          return ret;

      ret = new Wrapper(exec, g);
      interp->putDOMObject(g, ret);
      return ret;
  }

  template<typename Wrapped>
  class DOMWrapperObject : public DOMObject
  {
  public:
    typedef Wrapped wrappedType;
    typedef DOMWrapperObject<Wrapped> WrapperBase;

    DOMWrapperObject(JSObject* proto, Wrapped* wrapee):
      DOMObject(proto), m_impl(wrapee)
    {}

    virtual ~DOMWrapperObject() {
      ScriptInterpreter::forgetDOMObject(m_impl.get());
    }

    virtual bool toBoolean(ExecState *) const { return true; }

    Wrapped* impl() { return m_impl.get(); }
    const Wrapped* impl() const { return m_impl.get(); }
  private:
    SharedPtr<Wrapped> m_impl;
  };


  /**
   A little helper for setting stuff up given an entry
  */
  template<class FuncImp, class ThisImp>
  inline void getSlotFromEntry(const HashEntry* entry, ThisImp* thisObj, PropertySlot& slot)
  {
    if (entry->attr & Function)
      slot.setStaticEntry(thisObj, entry, staticFunctionGetter<FuncImp>);
    else
      slot.setStaticEntry(thisObj, entry, staticValueGetter<ThisImp>);
  }


  /**
    Like getStaticPropertySlot but doesn't check the parent. Handy when there
    are both functions and values
   */
  template <class FuncImp, class ThisImp>
  inline bool getStaticOwnPropertySlot(const HashTable* table,
                                    ThisImp* thisObj, const Identifier& propertyName, PropertySlot& slot)
  {
    const HashEntry* entry = Lookup::findEntry(table, propertyName);

    if (!entry) // not found, forward to parent
      return false;

    if (entry->attr & Function)
      slot.setStaticEntry(thisObj, entry, staticFunctionGetter<FuncImp>);
    else
      slot.setStaticEntry(thisObj, entry, staticValueGetter<ThisImp>);

    return true;
  }

  /**
    Handler for local table-looked up things.
  */
  template<class ThisImp>
  inline bool getStaticOwnValueSlot(const HashTable* table,
        ThisImp* thisObj, const Identifier& propertyName, PropertySlot& slot)
  {
    const HashEntry* entry = Lookup::findEntry(table, propertyName);
    if (!entry)
      return false;

    assert(!(entry->attr & Function));
    slot.setStaticEntry(thisObj, entry, staticValueGetter<ThisImp>);
    return true;
  }

  /* Helper for the below*/
  template<class JSTypeImp>
  JSValue *indexGetterAdapter(ExecState* exec, JSObject*, unsigned, const PropertySlot& slot)
  {
    JSTypeImp *thisObj = static_cast<JSTypeImp*>(slot.slotBase());
    return thisObj->indexGetter(exec, slot.index());
  }

  /**
   Handler for index properties. Will call "length" method on the listObj
   to determine whether it's in range, and arrange to have indexGetter called
  */
  template<class ThisImp, class BaseObj>
  inline bool getIndexSlot(ThisImp* thisObj, const BaseObj& listObj,
      const Identifier& propertyName, PropertySlot& slot)
  {
    bool ok;
    unsigned u = propertyName.toArrayIndex(&ok);
    if (ok && u < listObj.length()) {
      slot.setCustomIndex(thisObj, u, indexGetterAdapter<ThisImp>);
      return true;
    }
    return false;
  }

  /**
   Version that takes an external bound
  */
  template<class ThisImp>
  inline bool getIndexSlot(ThisImp* thisObj, unsigned lengthLimit,
      const Identifier& propertyName, PropertySlot& slot)
  {
    bool ok;
    unsigned u = propertyName.toArrayIndex(&ok);
    if (ok && u < lengthLimit) {
      slot.setCustomIndex(thisObj, u, indexGetterAdapter<ThisImp>);
      return true;
    }
    return false;
  }

  template<class ThisImp>
  inline bool getIndexSlot(ThisImp* thisObj, int lengthLimit,
      const Identifier& propertyName, PropertySlot& slot)
  {
    return getIndexSlot(thisObj, (unsigned)lengthLimit, propertyName, slot);
  }

  /**
   Version w/o the bounds check
  */
  template<class ThisImp>
  inline bool getIndexSlot(ThisImp* thisObj, const Identifier& propertyName, PropertySlot& slot)
  {
    bool ok;
    unsigned u = propertyName.toArrayIndex(&ok);
    if (ok) {
      slot.setCustomIndex(thisObj, u, indexGetterAdapter<ThisImp>);
      return true;
    }
    return false;
  }

  /* Helper for below */
  JSValue* valueGetterAdapter(ExecState* exec, JSObject*, const Identifier& , const PropertySlot& slot);

  /**
   This sets up the slot to return a particular JSValue*; unlike
   setValueSlot, it does not require there to be a location to point at
  */
  inline bool getImmediateValueSlot(JSObject* thisObj, JSValue* value, PropertySlot& slot) {
    slot.setCustomValue(thisObj, value, valueGetterAdapter);
    return true;
  }

  /**
   * Retrieve from cache, or create, a KJS object around a DOM object
   */
  template<class DOMObj, class KJSDOMObj>
  inline JSValue* cacheDOMObject(ExecState *exec, DOMObj* domObj)
  {
    DOMObject *ret;
    if (!domObj)
      return jsNull();
    ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());
    if ((ret = interp->getDOMObject(domObj)))
      return ret;
    else {
      ret = new KJSDOMObj(exec, domObj);
      interp->putDOMObject(domObj,ret);
      return ret;
    }
  }

  /**
   * Convert an object to a Node. Returns 0 if not possible.
   */
  DOM::NodeImpl* toNode(JSValue*);
  /**
   *  Get a String object, or Null() if s is null
   */
  JSValue* getStringOrNull(DOM::DOMString s);

  /**
   * Null string if the value is null
   */
  DOM::DOMString valueToStringWithNullCheck(ExecState* exec, JSValue* v);

  /**
   * Convert a KJS value into a QVariant
   */
  QVariant ValueToVariant(ExecState* exec, JSValue* val);

  // Convert a DOM implementation exception code into a JavaScript exception in the execution state.
  void setDOMException(ExecState *exec, int DOMExceptionCode);

  // Helper class to call setDOMException on exit without adding lots of separate calls to that function.
  class DOMExceptionTranslator {
  public:
    explicit DOMExceptionTranslator(ExecState *exec) : m_exec(exec), m_code(0) { }
    ~DOMExceptionTranslator() { setDOMException(m_exec, m_code); }
    operator int &() { return m_code; }
    operator int *() { return &m_code; }

    bool triggered() {
      return m_code;
    }
  private:
    ExecState *m_exec;
    int m_code;
  };

  // convenience function
  inline JSCell* jsString(const QString& s) { return jsString(UString(s)); }

// This is used to create pseudo-constructor objects, like Mozillaish
// Element, HTMLDocument, etc., which do not act like real constructors,
// but do have the prototype property pointing to prototype of "instances"
#define DEFINE_PSEUDO_CONSTRUCTOR(ClassName) \
  class ClassName : public DOMObject { \
      public: \
          ClassName(ExecState *); \
          virtual const ClassInfo* classInfo() const { return &info; } \
          static const ClassInfo info; \
          static JSObject* self(ExecState *exec); \
          virtual bool implementsHasInstance() const; \
  };

#define IMPLEMENT_PSEUDO_CONSTRUCTOR_IMP(Class,ClassName,ProtoClass,ParentProto) \
    const ClassInfo Class::info = { ClassName, 0, 0, 0 }; \
    Class::Class(ExecState* exec): DOMObject(ParentProto) {\
        /* Since ProtoClass ctor might need us, make sure we're registered */ \
        exec->lexicalInterpreter()->globalObject()->put(exec, "[[" ClassName ".constructor]]", this, KJS::Internal | KJS::DontEnum); \
        JSObject* proto = ProtoClass::self(exec); \
        putDirect(exec->propertyNames().prototype, proto, DontDelete|ReadOnly); \
    }\
    JSObject* Class::self(ExecState *exec) { \
        return cacheGlobalObject<Class>(exec, "[[" ClassName ".constructor]]"); \
    } \
    bool Class::implementsHasInstance() const { \
        return true; \
    }

#define IMPLEMENT_PSEUDO_CONSTRUCTOR(Class,ClassName,ProtoClass) \
    IMPLEMENT_PSEUDO_CONSTRUCTOR_IMP(Class,ClassName,ProtoClass,exec->lexicalInterpreter()->builtinObjectPrototype())

#define IMPLEMENT_PSEUDO_CONSTRUCTOR_WITH_PARENT(Class,ClassName,ProtoClass,ParentProtoClass) \
    IMPLEMENT_PSEUDO_CONSTRUCTOR_IMP(Class,ClassName,ProtoClass,ParentProtoClass::self(exec))

// This declares a constant table, which merely maps everything in its
// table to its token value. Can be used as a prototype
#define DEFINE_CONSTANT_TABLE(Class) \
   class Class : public DOMObject { \
   public: \
     Class(ExecState *exec): DOMObject(exec->lexicalInterpreter()->builtinObjectPrototype()) {} \
     \
     using KJS::JSObject::getOwnPropertySlot;\
     virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);\
     JSValue* getValueProperty(ExecState *exec, int token) const; \
     virtual const ClassInfo* classInfo() const { return &info; } \
     static const ClassInfo info; \
     static JSObject* self(ExecState *exec);\
     static Identifier* s_name; \
     static Identifier* name(); \
   };

// Emits an implementation of a constant table
#define IMPLEMENT_CONSTANT_TABLE(Class,ClassName) \
     bool Class::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot) \
     { \
        return getStaticValueSlot<Class, DOMObject>(exec, &Class##Table, this, propertyName, slot);\
     }\
     JSValue* Class::getValueProperty(ExecState * /*exec*/, int token) const { \
        /* We use the token as the value to return directly*/ \
        return jsNumber((unsigned int)token); \
     }  \
     JSObject* Class::self(ExecState *exec) { \
        return cacheGlobalObject<Class>(exec,  *name()); \
     } \
    Identifier* Class::s_name = 0; \
    Identifier* Class::name() { \
        if (!s_name) s_name = new Identifier("[[" ClassName ".constant_table]]"); \
        return s_name; \
    } \
   const ClassInfo Class::info = { ClassName, 0, &Class##Table, 0 };

// cacheGlobalObject<> is not in namespace KJS - need to use ::cacheGlobalObject<>
#define KJS_EMPTY_PROTOTYPE_IMP(ClassName, ClassProto, ProtoCode) \
      class ClassProto : public JSObject { \
      friend JSObject* KJS_CACHEGLOBALOBJECT_NS cacheGlobalObject<ClassProto>(ExecState *exec, const Identifier &propertyName); \
      public: \
        static JSObject* self(ExecState *exec) { \
          return KJS_CACHEGLOBALOBJECT_NS cacheGlobalObject<ClassProto>(exec, *name()); \
        } \
        virtual const ClassInfo *classInfo() const { return &info; } \
        static const ClassInfo info; \
      protected: \
        ClassProto( ExecState *exec ) \
          : JSObject( ProtoCode ) {} \
        \
        static Identifier* s_name; \
        static Identifier* name() { \
            if (!s_name) s_name = new Identifier("[[" ClassName ".prototype]]"); \
            return s_name; \
        }\
      }; \
      Identifier* ClassProto::s_name = 0; \
      const ClassInfo ClassProto::info = { ClassName, 0, 0, 0 };

#define KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE(ClassName, ClassProto, ClassProtoProto) \
  KJS_EMPTY_PROTOTYPE_IMP(ClassName, ClassProto, ClassProtoProto::self(exec))


} // namespace


#endif
