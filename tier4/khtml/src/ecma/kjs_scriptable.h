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


#ifndef KJS_SCRIPTABLE_H
#define KJS_SCRIPTABLE_H

#include <khtml_part.h>
#include <kparts/scriptableextension.h>
#include <kjs/object.h>
#include <kjs/list.h>
#include <QHash>

using namespace KParts;

namespace KJS {

// First, a couple of helpers: these let clients perform basic getOwnPropertySlot or
// put (with a bool effect return) on the root of given extension.
bool pluginRootGet(ExecState* exec, ScriptableExtension* ext, const KJS::Identifier& i, PropertySlot& slot);
bool pluginRootPut(ExecState* exec, ScriptableExtension* ext, const KJS::Identifier& i, JSValue* v);

class WrapScriptableObject;

// We have two ScriptableExtension subclasses.
// ScriptableOperations is a singleton used to perform operations on objects.
// This is because objects in child  parts (and other windows) can survive their
// parent, and we still need to perform operations on them.
class ScriptableOperations: public ScriptableExtension
{
    Q_OBJECT
public:
    // ScriptableExtension API
    virtual QVariant callAsFunction(ScriptableExtension* callerPrincipal, quint64 objId, const ArgList& args);
    virtual QVariant callFunctionReference(ScriptableExtension* callerPrincipal, quint64 objId,
                                           const QString& f, const ArgList& args);
    virtual QVariant callAsConstructor(ScriptableExtension* callerPrincipal, quint64 objId, const ArgList& args);
    virtual bool hasProperty(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName);
    virtual QVariant get(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName);
    virtual bool put(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName, const QVariant& value);
    virtual bool removeProperty(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName);
    virtual bool enumerateProperties(ScriptableExtension* callerPrincipal, quint64 objId, QStringList* result);

    virtual void acquire(quint64 objid);
    virtual void release(quint64 objid);

    // May return null.
    static JSObject* objectForId(quint64 objId);    

    static ScriptableOperations* self();

    void mark();
public:
    // We keep a weak table of our wrappers for external objects, so that
    // the same object gets the same wrapper all the time.
    static QHash<Object,      WrapScriptableObject*>* importedObjects();
    static QHash<FunctionRef, WrapScriptableObject*>* importedFunctions();

    // For exported objects, we keep refcounts, and mark them
    static QHash<JSObject*, int>* exportedObjects();

    static JSValue* importValue(ExecState* exec, const QVariant& v, bool alreadyRefd);
    static JSValue* importFunctionRef(ExecState* exec, const QVariant& v, bool alreadyRefd);
    static JSObject* importObject(ExecState* exec, const QVariant& v, bool alreadyRefd);
    static List importArgs(ExecState* exec, const ArgList& args);

    static QVariant exportValue (JSValue* v, bool preRef);
    static QVariant exportObject(JSObject* o, bool preRef);
    static QVariant exportFuncRef(JSObject* base, const QString& field, bool preRef);

    // Both methods may return 0. Used for security checks!
    static KHTMLPart* partForPrincipal(ScriptableExtension* callerPrincipal);
    static ExecState* execStateForPrincipal(ScriptableExtension* callerPrincipal);    
private:
    // input should not be a WrapScriptableObject.
    static ScriptableExtension::Object exportNativeObject(JSObject* o, bool preRef);

    // Checks exception state before handling conversion
    QVariant handleReturn(ExecState* exec, JSValue* v);

    // If the given object is owned by a KHTMLScriptable, return the
    // JS object for it. If not, return 0.
    static JSObject* tryGetNativeObject(const Object& sObj);

    static QHash<JSObject*, int>* s_exportedObjects;
    static QHash<Object,      WrapScriptableObject*>* s_importedObjects;
    static QHash<FunctionRef, WrapScriptableObject*>* s_importedFunctions;

    ScriptableOperations();
    ~ScriptableOperations();

    // copy ctor, etc., disabled already, being a QObject
    static ScriptableOperations* s_instance;
};

// KHTMLPartScriptable, on other hands, is tied to a part, and
// is used for part-specific operations such as looking up
// root objects and script execution.
class KHTMLPartScriptable: public ScriptableExtension {
    Q_OBJECT
    friend class ScriptableOperations;
public:
    KHTMLPartScriptable(KHTMLPart* part);

    virtual QVariant rootObject();
    virtual QVariant encloserForKid(KParts::ScriptableExtension* kid);

    virtual bool setException(ScriptableExtension* callerPrincipal, const QString& message);

    virtual QVariant evaluateScript(ScriptableExtension* callerPrincipal,
                                    quint64 contextObjectId,
                                    const QString& code,
                                    ScriptLanguage language = ECMAScript);
    virtual bool isScriptLanguageSupported(ScriptLanguage lang) const;

    // For paranoia: forward to ScriptOperations
    virtual void acquire(quint64 objid);
    virtual void release(quint64 objid);
private:
    KJS::Interpreter* interpreter();
    KHTMLPart* m_part;
};
                                    


// This represents an object we imported from a foreign ScriptableExtension
class WrapScriptableObject: public JSObject {
public:
    friend class ScriptableOperations;

    enum Type {
        Object,
        FunctionRef
    };

    WrapScriptableObject(ExecState* exec, Type t,
                         ScriptableExtension* owner, quint64 objId,
                         const QString& field = QString());

   ~WrapScriptableObject();

    virtual const ClassInfo *classInfo() const { return &info; }
    static const ClassInfo info;

    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    using JSObject::getOwnPropertySlot;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int);
    using JSObject::put;
    virtual bool deleteProperty(ExecState* exec, const Identifier& i);
    using JSObject::deleteProperty;

    virtual bool isFunctionType() const { return false; }
    virtual bool implementsCall() const { return true; }
    virtual JSValue *callAsFunction(ExecState *exec, JSObject *thisObj, const List &args);

    // We claim true, since may be calleable
    virtual bool implementsConstruct() const { return true; }
    virtual JSObject* construct(ExecState* exec, const List& args);
    using JSObject::construct;

    virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, PropertyMap::PropertyMode mode);

    virtual UString toString(ExecState *exec) const;

    // This method is used to note that the object has been ref'd on our
    // behalf by an external producer.
    void reportRef(); 
private:
    // If we're a function reference type, before we perform non-call operations we need
    // to actually lookup the field. This takes care of that.
    ScriptableExtension::Object resolveAnyReferences(ExecState* exec, bool* ok);


    // resolves all function references to get an ref, if any.
    ScriptableExtension::Object resolveReferences(ExecState* exec,
                                                  const ScriptableExtension::FunctionRef& f,
                                                  bool* ok);

    // gets a field of the given object id, base
    QVariant doGet(ExecState* exec, const ScriptableExtension::Object& o,
                   const QString& field, bool* ok);

    ScriptableExtension::ArgList exportArgs(const List& l);
    void releaseArgs(ScriptableExtension::ArgList& a);

    // Looks up the principal we're running as
    ScriptableExtension* principal(ExecState* exec);

    // what we wrap
    QWeakPointer<ScriptableExtension> objExtension;
    quint64 objId;
    QString field;
    Type    type;
    int     refsByUs; // how many references we hold

    // this is an unguarded copy of objExtension. We need it in order to
    // clean ourselves up from the imports tables properly even if the peer
    // was destroyed.
    ScriptableExtension* tableKey;
};

}

#endif
// kate: space-indent on; indent-width 4; replace-tabs on;
