/* This file is part of the KDE project
   Copyright (C) 2010 Maksim Orlovich <maksim@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include "kjs_scriptable.h"
#include "kjs_binding.h"
#include "kjs_proxy.h"
#include "kjs_dom.h"
#include "khtmlpart_p.h"

namespace KJS {

static QVariant scriptableNull()
{
    return QVariant::fromValue(ScriptableExtension::Null());
}

static bool isException(const QVariant& v)
{
    return v.canConvert<ScriptableExtension::Exception>();
}

static bool isFuncRef(const QVariant& v)
{
    return v.canConvert<ScriptableExtension::FunctionRef>();
}

static bool isForeignObject(const QVariant& v)
{
    return v.canConvert<ScriptableExtension::Object>();
}

QVariant exception(const char* msg)
{
    qWarning() << msg;
    return QVariant::fromValue(ScriptableExtension::Exception(QString::fromLatin1(msg)));
}

//------------------------------------------------------------------------------

// will return with owner = 0 on failure.
static ScriptableExtension::Object grabRoot(ScriptableExtension* ext)
{
    ScriptableExtension::Object o;

    if (!ext)
        return o;

    // Grab root, make sure it's an actual object.
    QVariant root = ext->rootObject();
    if (!isForeignObject(root)) {
        // Might still need to release it if it's a function
        ScriptableExtension::releaseValue(root);
        
        return o;
    }

    o = root.value<ScriptableExtension::Object>();
    return o;
}

// a couple helpers for client code.
bool pluginRootGet(ExecState* exec, ScriptableExtension* ext, const KJS::Identifier& i, PropertySlot& slot)
{
    ScriptableExtension::Object rootObj = grabRoot(ext);
    if (!rootObj.owner)
        return false;

    QVariant v = rootObj.owner->get(0 /* ### we don't expect leaves to check credentials*/,
                                       rootObj.objId, i.qstring());

    bool ok = false;
    if (!isException(v)) {
        getImmediateValueSlot(0, ScriptableOperations::importValue(exec, v, true), slot);
        ok = true;
    }

    rootObj.owner->release(rootObj.objId);
    return ok;
}

bool pluginRootPut(ExecState* /*exec*/, ScriptableExtension* ext, const KJS::Identifier& i, JSValue* v)
{
    ScriptableExtension::Object rootObj = grabRoot(ext);
    if (!rootObj.owner)
        return false;

    QVariant qv = ScriptableOperations::exportValue(v, true);
    bool ok = rootObj.owner->put(0, rootObj.objId, i.qstring(), qv);
    ScriptableExtension::releaseValue(qv);
    
    rootObj.owner->release(rootObj.objId);
    return ok;
}

//------------------------------------------------------------------------------
// KJS peer wrapping external objects

const ClassInfo WrapScriptableObject::info = { " WrapScriptableObject", 0, 0, 0 };

WrapScriptableObject::WrapScriptableObject(ExecState* /*exec*/, Type t,
                                           ScriptableExtension* owner, quint64 objId,
                                           const QString& field):
    objExtension(owner), objId(objId), field(field), type(t), refsByUs(1), tableKey(owner)
{
    owner->acquire(objId);
}

WrapScriptableObject::~WrapScriptableObject()
{
    if (ScriptableExtension* o = objExtension.data()) {
        for (int r = 0; r < refsByUs; ++r)
            o->release(objId);
    }

    ScriptableExtension::Object obj(tableKey, objId);
    if (type == Object) {
        ScriptableOperations::importedObjects()->remove(obj);
    } else {
        ScriptableOperations::importedFunctions()->remove(ScriptableExtension::FunctionRef(obj, field));
    }
}

void WrapScriptableObject::reportRef()
{
    ++refsByUs;
}

QVariant WrapScriptableObject::doGet(ExecState* exec, const ScriptableExtension::Object& o,
                                     const QString& field, bool* ok)
{
    *ok = false;

    if (!o.owner) // such as when was constructed from null .data();
        return QVariant();

    QVariant v = o.owner->get(principal(exec), o.objId, field);

    if (!isException(v))
        *ok = true;
    return v;
}

ScriptableExtension::Object WrapScriptableObject::resolveReferences(
                                                    ExecState* exec,
                                                    const ScriptableExtension::FunctionRef& f,
                                                    bool* ok)
{
    QVariant v = doGet(exec, f.base, f.field, ok);
    if (*ok) {
        // See what sort of type we got.
        if (isForeignObject(v)) {
            return v.value<ScriptableExtension::Object>();
        } else if (isFuncRef(v)) {
            return resolveReferences(exec, v.value<ScriptableExtension::FunctionRef>(), ok);
        } else {
            // We got a primitive. We don't care for those.
            *ok = false;
        }
    }
    return ScriptableExtension::Object();
}

ScriptableExtension::Object WrapScriptableObject::resolveAnyReferences(ExecState* exec, bool* ok)
{
    ScriptableExtension::Object obj(objExtension.data(), objId);

    if (type == FunctionRef)
        obj = resolveReferences(exec, ScriptableExtension::FunctionRef(obj, field), ok);

    if (!obj.owner)
        *ok = false;

    return obj;
}

bool WrapScriptableObject::getOwnPropertySlot(ExecState* exec, const Identifier& i,
                                             PropertySlot& slot)
{
    bool ok;
    ScriptableExtension::Object actualObj = resolveAnyReferences(exec, &ok);

    if (!ok)
        return false;

    QVariant v = doGet(exec, actualObj, i.qstring(), &ok);

    if (!ok)
        return false;

    return getImmediateValueSlot(this, ScriptableOperations::importValue(exec, v, true), slot);
}

void WrapScriptableObject::put(ExecState* exec, const Identifier& i, JSValue* value, int)
{
    // ### Do we swallow failure, or what?
    bool ok;
    ScriptableExtension::Object actualObj = resolveAnyReferences(exec, &ok);

    if (!ok)
        return;

    QVariant sv = ScriptableOperations::exportValue(value, true);
    actualObj.owner->put(principal(exec), actualObj.objId, i.qstring(), sv);
    ScriptableExtension::releaseValue(sv);
}

bool WrapScriptableObject::deleteProperty(ExecState* exec, const Identifier& i)
{
    bool ok;
    ScriptableExtension::Object actualObj = resolveAnyReferences(exec, &ok);

    if (!ok)
        return false;

    return actualObj.owner->removeProperty(principal(exec),
                                           actualObj.objId, i.qstring());
}

ScriptableExtension::ArgList WrapScriptableObject::exportArgs(const List& l)
{
    ScriptableExtension::ArgList ol;
    for (int p = 0; p < l.size(); ++p)
        ol.append(ScriptableOperations::exportValue(l.at(p), true));
    return ol;
}

void WrapScriptableObject::releaseArgs(ScriptableExtension::ArgList& a)
{
    for (int p = 0; p < a.size(); ++p)
        ScriptableOperations::releaseValue(a[p]);
}

JSValue* WrapScriptableObject::callAsFunction(ExecState *exec, JSObject */*thisObj*/, const List &args)
{
    QVariant res;

    if (ScriptableExtension* base = objExtension.data()) {
        ScriptableExtension::ArgList sargs = exportArgs(args);
        if (type == Object)
            res = base->callAsFunction(principal(exec), objId, sargs);
        else
            res = base->callFunctionReference(principal(exec), objId, field, sargs);
        releaseArgs(sargs);
    }

    // Problem. Throw an exception.
    if (!res.isValid() || isException(res))
        return throwError(exec, GeneralError, "Call to plugin function failed");
    else
        return ScriptableOperations::importValue(exec, res, true);
}

JSObject* WrapScriptableObject::construct(ExecState* exec, const List& args)
{
    QVariant res;

    bool ok;
    ScriptableExtension::Object actualObj = resolveAnyReferences(exec, &ok);
    if (ok) {
        ScriptableExtension::ArgList sargs = exportArgs(args);
        res = actualObj.owner->callAsConstructor(principal(exec), actualObj.objId,
                                                 sargs);
        releaseArgs(sargs);
    }

    if (!res.isValid() || isException(res)) {
        return throwError(exec, GeneralError, "Call to plugin ctor failed");
    } else {
        JSValue* v = ScriptableOperations::importValue(exec, res, true);
        return v->toObject(exec);
    }
}

void WrapScriptableObject::getOwnPropertyNames(ExecState* exec, PropertyNameArray& a, PropertyMap::PropertyMode mode)
{
    JSObject::getOwnPropertyNames(exec, a, mode);

    bool ok;
    ScriptableExtension::Object actualObj = resolveAnyReferences(exec, &ok);
    if (ok) {
        QStringList out;
        if (actualObj.owner->enumerateProperties(principal(exec), actualObj.objId, &out)) {
            foreach (const QString& s, out)
                a.add(Identifier(s));
        }
    }
}

UString WrapScriptableObject::toString(ExecState*) const
{
    QString iface;
    if (ScriptableExtension* se = objExtension.data()) {
        iface = QString::fromLatin1(se->metaObject()->className());
    } else {
        iface = QString::fromLatin1("detached");
    }

    if (type == FunctionRef) {
        return QString(QLatin1String("[function ImportedScriptable:") + iface + QLatin1Char('/') + field + QLatin1Char(']'));
    } else {
        return QString(QLatin1String("[object ImportedScriptable:") + iface + QLatin1Char(']'));
    }
}

ScriptableExtension* WrapScriptableObject::principal(ExecState* exec)
{
    KJS::ScriptInterpreter* si = static_cast<KJS::ScriptInterpreter*>(exec->dynamicInterpreter());
    KParts::ReadOnlyPart*   part = si->part();
    if (!part)
        return 0;

    return ScriptableExtension::childObject(part);
}

//-----------------------------------------------------------------------------
// conversion stuff

/**
   SECURITY: For the conversion helpers, it is assumed that 'exec' corresponds
   to the appropriate principal.
*/

JSObject* ScriptableOperations::importObject(ExecState* exec, const QVariant& v, bool alreadyRefd)
{
    ScriptableExtension::Object obj = v.value<ScriptableExtension::Object>();
    if (JSObject* our = tryGetNativeObject(obj)) {
        return our;
    } else {
        // Create a wrapper, and register the import, this is just for
        // hashconsing.
        if (WrapScriptableObject* old = importedObjects()->value(obj)) {
            if (alreadyRefd)
                old->reportRef();
            return old;
        } else {
            WrapScriptableObject* wrap =
                new WrapScriptableObject(exec, WrapScriptableObject::Object,
                                         obj.owner, obj.objId);
            importedObjects()->insert(obj, wrap);
            if (alreadyRefd)
                wrap->reportRef();
            return wrap;
        }
    }
}

// Note: this is used to convert a full function ref to a value,
// which loses the this-binding in the native case.  We do keep the pair in the
// external case, inside the wrapper,  since the method might not have an own
// name, and we'd like to be able to refer to it.
JSValue* ScriptableOperations::importFunctionRef(ExecState* exec, const QVariant& v, bool /*alreadyRefd*/)
{
    ScriptableExtension::FunctionRef fr = v.value<ScriptableExtension::FunctionRef>();

    if (JSObject* base = tryGetNativeObject(fr.base)) {
        return base->get(exec, Identifier(fr.field));
    } else {
        if (WrapScriptableObject* old = importedFunctions()->value(fr)) {
            return old;
        } else {
            WrapScriptableObject* wrap =
                new WrapScriptableObject(exec, WrapScriptableObject::FunctionRef,
                                         fr.base.owner, fr.base.objId, fr.field);
            importedFunctions()->insert(fr, wrap);
            return wrap;
        }
    }
}

JSValue* ScriptableOperations::importValue(ExecState* exec, const QVariant& v, bool alreadyRefd)
{
    if (v.canConvert<ScriptableExtension::FunctionRef>())
        return importFunctionRef(exec, v, alreadyRefd);
    if (v.canConvert<ScriptableExtension::Object>())
        return importObject(exec, v, alreadyRefd);
    if (v.canConvert<ScriptableExtension::Null>())
        return jsNull();
    if (v.canConvert<ScriptableExtension::Undefined>())
        return jsUndefined();
    if (v.type() == QVariant::Bool)
        return jsBoolean(v.toBool());
    if (v.type() == QVariant::String)
        return jsString(v.toString());
    if (v.canConvert<double>())
        return jsNumber(v.toDouble());
    qWarning() << "conversion from " << v << "failed";
    return jsNull();
}

List ScriptableOperations::importArgs(ExecState* exec, const ArgList& args)
{
    // Args are not pre-ref'd for us..
    List out;
    for (int i = 0; i < args.size(); ++i)
        out.append(importValue(exec, args[i], false));
    return out;
}

ScriptableExtension::Object ScriptableOperations::exportNativeObject(JSObject* o, bool preRef)
{
    assert (!o->inherits(&WrapScriptableObject::info));
    // We're exporting our own. Add to export table if needed.
    // Use the pointer as an ID.
    if (!exportedObjects()->contains(o))
        exportedObjects()->insert(o, 0);
    if (preRef)
        ++(*exportedObjects())[o];

    return ScriptableExtension::Object(ScriptableOperations::self(),
                                       reinterpret_cast<quint64>(o));
}

QVariant ScriptableOperations::exportObject(JSObject* o, bool preRef)
{
    // XSS checks are done at get time, so if we have a value here, we can
    // export it.
    if (o->inherits(&WrapScriptableObject::info)) {
        // Re-exporting external one. That's easy.
        WrapScriptableObject* wo = static_cast<WrapScriptableObject*>(o);
        if (ScriptableExtension* owner = wo->objExtension.data()) {
            QVariant v = QVariant::fromValue(ScriptableExtension::Object(owner, wo->objId));
            if (preRef)
                acquireValue(v);
            return v;
        } else {
            qWarning() << "export of an object of a destroyed extension. Returning null";
            return scriptableNull();
        }   
    } else {
        return QVariant::fromValue(exportNativeObject(o, preRef));
    }
}

QVariant ScriptableOperations::exportFuncRef(JSObject* base, const QString& field, bool preRef)
{
    ScriptableExtension::Object exportBase = exportNativeObject(base, preRef);
    return QVariant::fromValue(ScriptableExtension::FunctionRef(exportBase, field));
}

QVariant ScriptableOperations::exportValue(JSValue* v, bool preRef)
{
    switch (v->type()) {
    case NumberType:
        return QVariant::fromValue(v->getNumber());
    case BooleanType:
        return QVariant::fromValue(v->getBoolean());
    case NullType:
        return QVariant::fromValue(ScriptableExtension::Null());
    case StringType:
        return QVariant::fromValue(v->getString().qstring());
    case ObjectType:
        return exportObject(v->getObject(), preRef);
    case UndefinedType:
    default:
        return QVariant::fromValue(ScriptableExtension::Undefined());
    }
}

//-----------------------------------------------------------------------------
// operations
QHash<JSObject*, int>* ScriptableOperations::s_exportedObjects = 0;
QHash<ScriptableExtension::Object, WrapScriptableObject*>* ScriptableOperations::s_importedObjects   = 0;
QHash<ScriptableExtension::FunctionRef, WrapScriptableObject*>* ScriptableOperations::s_importedFunctions = 0;
ScriptableOperations* ScriptableOperations::s_instance = 0;

QHash<ScriptableExtension::Object, WrapScriptableObject*>* ScriptableOperations::importedObjects()
{
    if (!s_importedObjects)
        s_importedObjects = new QHash<Object, WrapScriptableObject*>;
    return s_importedObjects;
}

QHash<ScriptableExtension::FunctionRef, WrapScriptableObject*>* ScriptableOperations::importedFunctions()
{
    if (!s_importedFunctions)
        s_importedFunctions = new QHash<FunctionRef, WrapScriptableObject*>();
    return s_importedFunctions;
}

// A little helper for marking exportedObjects. We need this since we have
// an unclear runtime with respect to interpreter.
class ScriptableOperationsMarker: public JSObject
{
public:
    virtual void mark() {
        JSObject::mark();
        ScriptableOperations::self()->mark();
    }
};

QHash<JSObject*, int>* ScriptableOperations::exportedObjects()
{
    if (!s_exportedObjects) {
        s_exportedObjects = new QHash<JSObject*, int>;

        gcProtect(new ScriptableOperationsMarker());
    }
    return s_exportedObjects;
}

ScriptableOperations* ScriptableOperations::self()
{
    if (!s_instance)
        s_instance = new ScriptableOperations;
    return s_instance;
}

ScriptableOperations::ScriptableOperations(): ScriptableExtension(0)
{}

ScriptableOperations::~ScriptableOperations()
{
    assert(false);
}

JSObject* ScriptableOperations::tryGetNativeObject(const Object& sObj)
{
    if (ScriptableOperations* o = qobject_cast<ScriptableOperations*>(sObj.owner)) {
        return o->objectForId(sObj.objId);
    } else {
        return 0;
    }
}

QVariant ScriptableOperations::handleReturn(ExecState* exec, JSValue* v)
{
    if (exec->hadException()) {
        JSValue* e = exec->exception();
        exec->clearException();
        
        QString msg = QLatin1String("KJS exception");
        
        if (JSObject* eo = e->getObject()) {
            JSValue* msgVal = eo->get(exec, exec->propertyNames().message);
            if (!msgVal->isUndefined())
                msg = msgVal->toString(exec).qstring();
            
            // in case the get failed too.
            exec->clearException();
        }

        return QVariant::fromValue(ScriptableExtension::Exception(msg));
    }

    return exportValue(v, true);
}

QVariant ScriptableOperations::callAsFunction(ScriptableExtension* caller,
                                         quint64 objId, const ArgList& args)
{
    ExecState* exec = execStateForPrincipal(caller);
    if (!exec)
        return exception("No scripting context or frame");

    JSObject* fn = objectForId(objId);
    if (!fn || !fn->implementsCall())
        return exception("Call on a non-object or non-calleable");

    JSValue* res = fn->callAsFunction(exec, exec->dynamicInterpreter()->globalObject(),
                                      importArgs(exec, args));
    return handleReturn(exec, res);
}

QVariant ScriptableOperations::callAsConstructor(ScriptableExtension* caller, quint64 objId, const ArgList& args)
{
    ExecState* exec = execStateForPrincipal(caller);
    if (!exec)
        return exception("No scripting context or frame");

    JSObject* fn = objectForId(objId);
    if (!fn || !fn->implementsConstruct())
        return exception("new on a non-constructor");

    JSValue* res = fn->construct(exec,importArgs(exec, args));
    return handleReturn(exec, res);
}

QVariant ScriptableOperations::callFunctionReference(ScriptableExtension* caller,
                                           quint64 objId, const QString& f, const ArgList& args)
{
    ExecState* exec = execStateForPrincipal(caller);

    if (!exec)
        return exception("No scripting context or frame");

    JSObject* base = objectForId(objId);
    if (!base)
        return exception("Call with an invalid base");

    JSValue* kid = base->get(exec, Identifier(f));
    if (!kid->isObject() || exec->hadException() || !kid->getObject()->implementsCall()) {
        exec->clearException();
        return exception("Reference did not resolve to a function");
    }

    // Whee..
    JSValue* res = kid->getObject()->callAsFunction(exec, base, importArgs(exec, args));
    return handleReturn(exec, res);
}

bool ScriptableOperations::hasProperty(ScriptableExtension* caller, quint64 objId, const QString& propName)
{
    ExecState* exec = execStateForPrincipal(caller);
    if (!exec) {
        exception("No scripting context or frame");
        return false;
    }

    JSObject* o = objectForId(objId);
    if (!o) {
        exception("hasProperty on a non-object");
        return false;
    }

    return o->hasProperty(exec, Identifier(propName));
}

QVariant ScriptableOperations::get(ScriptableExtension* caller, quint64 objId, const QString& propName)
{
    ExecState* exec = execStateForPrincipal(caller);
    if (!exec)
        return exception("No scripting context or frame");

    JSObject* o = objectForId(objId);
    if (!o)
        return exception("get on a non-object");

    JSValue* v = o->get(exec, Identifier(propName));
    if (!exec->hadException() && v->isObject() && v->getObject()->implementsCall()) {
        // For a function we got OK, return a reference.
        return exportFuncRef(o, propName, true);
    } else {
        // straight return for other stuff or failure
        return handleReturn(exec, v);
    }
}

bool ScriptableOperations::put(ScriptableExtension* caller, quint64 objId,
                               const QString& propName, const QVariant& value)
{
    ExecState* exec = execStateForPrincipal(caller);
    if (!exec)
        return false; // ### debug warn?

    JSObject* o = objectForId(objId);
    if (!o)
        return false;

    o->put(exec, Identifier(propName), importValue(exec, value, false));
    if (!exec->hadException()) {
        return true;
    } else {
        exec->clearException();
        return false;
    }
}

bool ScriptableOperations::removeProperty(ScriptableExtension* caller,
                                          quint64 objId, const QString& propName)
{
    ExecState* exec = execStateForPrincipal(caller);
    if (!exec)
        return false; // ### debug warn?

    JSObject* o = objectForId(objId);
    if (!o)
        return false;

    bool ok = o->deleteProperty(exec, Identifier(propName));
    if (exec->hadException()) {
        exec->clearException();
        ok = false;
    }
    return ok;
}

bool ScriptableOperations::enumerateProperties(ScriptableExtension* caller,
                                               quint64 objId, QStringList* result)
{
    ExecState* exec = execStateForPrincipal(caller);
    if (!exec)
        return false; // ### debug warn?

    JSObject* o = objectForId(objId);
    if (!o)
        return false;

    PropertyNameArray pa;
    o->getPropertyNames(exec, pa);

    for (int i = 0; i < pa.size(); ++i)
        result->append(pa[i].qstring());
    return true;
}


KHTMLPart* ScriptableOperations::partForPrincipal(ScriptableExtension* caller)
{
    // We implement our security checks by delegating to the KHTMLPart corresponding
    // to the given plugin's principal (which is the KHTMLPart owning it), and letting
    // the underlying implementation perform them (which it has to anyway)

    if (KHTMLPartScriptable* o = qobject_cast<KHTMLPartScriptable*>(caller)) {
        return o->m_part;
    } else {
        // We always set the host on child extensions.
        return partForPrincipal(caller->host());
    }
}

ExecState* ScriptableOperations::execStateForPrincipal(ScriptableExtension* caller)
{
    KHTMLPart* part = partForPrincipal(caller);

    if (!part)
        return 0;

    KJSProxy* proxy = KJSProxy::proxy(part);
    if (!proxy)
        return 0;

    KJS::Interpreter* i = proxy->interpreter();
    if (!i)
        return 0;

    return i->globalExec();
}

void ScriptableOperations::acquire(quint64 objId)
{
    JSObject* ptr = objectForId(objId);

    if (ptr)
        ++(*exportedObjects())[ptr];
    else
        assert(false);
}

void ScriptableOperations::release(quint64 objId)
{
    JSObject* ptr = objectForId(objId);

    if (ptr) {
        int newRC = --(*exportedObjects())[ptr];
        if (newRC == 0)
            exportedObjects()->remove(ptr);
    } else {
        assert(false);
    }
}

JSObject* ScriptableOperations::objectForId(quint64 objId)
{
    JSObject* ptr = reinterpret_cast<JSObject*>(objId);

    // Verify the pointer against the exports table for paranoia.
    if (exportedObjects()->contains(ptr)) {
        return ptr;
    } else {
        assert(false);
        return 0; 
    }
}

void ScriptableOperations::mark()
{
    QHash<JSObject*, int>* exp = exportedObjects();

    for (QHash<JSObject*, int>::iterator i = exp->begin(); i != exp->end(); ++i) {
        JSObject* o = i.key();
        if (i.value() && !o->marked())
            o->mark();
    }
}

//-----------------------------------------------------------------------------
// per-part stuff.

KHTMLPartScriptable::KHTMLPartScriptable(KHTMLPart* part):
    ScriptableExtension(part), m_part(part)
{
}

KJS::Interpreter* KHTMLPartScriptable::interpreter()
{
    KJSProxy* proxy = KJSProxy::proxy(m_part);
    if (!proxy)
        return 0;

    return proxy->interpreter();
}

QVariant KHTMLPartScriptable::rootObject()
{
    if (KJS::Interpreter* i = interpreter())
        return ScriptableOperations::exportObject(i->globalObject(), true);

    return scriptableNull();
}

QVariant KHTMLPartScriptable::encloserForKid(KParts::ScriptableExtension* kid)
{
    ReadOnlyPart* childPart = qobject_cast<ReadOnlyPart*>(kid->parent());

    KJS::Interpreter* i = interpreter();
    if (!childPart || !i)
        return scriptableNull();

    khtml::ChildFrame* f = m_part->frame(childPart);

    if (!f) {
        qWarning() << "unable to find frame. Huh?";
        return scriptableNull();
    }

    // ### should this deal with fake window objects for iframes?
    // ### this should never actually get an iframe once iframes are fixed
    if (!f->m_partContainerElement.isNull()) {
        return ScriptableOperations::exportValue(
                getDOMNode(i->globalExec(), f->m_partContainerElement.data()), true);
    }

    qWarning() << "could not find the part container";
    return scriptableNull();
}

// For paranoia: forward to ScriptOperations
void KHTMLPartScriptable::acquire(quint64 objid)
{
    ScriptableOperations::self()->acquire(objid);
}

void KHTMLPartScriptable::release(quint64 objid)
{
    ScriptableOperations::self()->release(objid);
}

QVariant KHTMLPartScriptable::evaluateScript(ScriptableExtension* caller,
                                             quint64 contextObjectId,
                                             const QString& code,
                                             ScriptLanguage lang)
{
    // qDebug() << code;
    
    if (lang != ECMAScript)
        return exception("unsupported language");

    
    KHTMLPart* callingHtmlPart = ScriptableOperations::partForPrincipal(caller);
    if (!callingHtmlPart)
        return exception("failed to resolve principal");

    // Figure out the object we want to access, and its corresponding part.
    JSObject* o = ScriptableOperations::objectForId(contextObjectId);
    if (!o)
        return exception("invalid object");

    DOM::NodeImpl* node = toNode(o);

    // Presently, we only permit node contexts here.
    // ### TODO: window contexts?
    if (!node)
        return exception("non-Node context");

    KHTMLPart* targetPart = node->document()->part();

    if (!targetPart)
        return exception("failed to resolve destination principal");

    if (!targetPart->checkFrameAccess(callingHtmlPart))
        return exception("XSS check failed");

    // wheee..
    targetPart->executeScript(DOM::Node(node), code);

    // ### TODO: Return value. Which is a completely different kind of QVariant.
    // might just want to go to kJSProxy directly    
    
    return scriptableNull();
}

bool KHTMLPartScriptable::isScriptLanguageSupported(ScriptLanguage lang) const
{
    return lang == ECMAScript;
}

bool KHTMLPartScriptable::setException(ScriptableExtension* /*caller*/,
                                       const QString& message)
{
    qWarning() << "ignoring:" << message;
    return false;
}

} // namespace KJS

// kate: space-indent on; indent-width 4; replace-tabs on;
