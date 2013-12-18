/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2006 Matt Broadstone (mbroadst@gmail.com)
 *            (C) 2007 Maks Orlovich <maksim@kde.org>
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

#include "localvariabledock.h"

#include <QVBoxLayout>
#include <QTreeWidget>
#include <QEventLoop>
#include <QStringList>

#include <kjs/interpreter.h>
#include <kjs/PropertyNameArray.h>
#include <kjs/context.h>
#include <kjs/scope_chain.h>
#include <kjs/JSVariableObject.h>
#include <kjs/object.h>
#include <QDebug>
#include <klocalizedstring.h>

#include "value2string.h"

namespace KJSDebugger {

LocalVariablesDock::LocalVariablesDock(QWidget *parent)
    : QDockWidget(i18n("Local Variables"), parent), m_execState(0)
{
    setFeatures(DockWidgetMovable | DockWidgetFloatable);
    m_view = new QTreeWidget(this);

    m_view->setColumnCount(2);

    QStringList headers;
    headers << i18n("Reference");
    headers << i18n("Value");
    m_view->setHeaderLabels(headers);

    connect(m_view, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this,   SLOT  (slotItemExpanded(QTreeWidgetItem*)));

    setWidget(m_view);
}

LocalVariablesDock::~LocalVariablesDock()
{
}

void LocalVariablesDock::updateValue(KJS::ExecState* exec, KJS::JSValue* val,
                                     QTreeWidgetItem* item, bool recurse)
{
    // Note: parent is responsible for setting our name..
    item->setText(1, valueToString(val));
    if (recurse) {
        if (val->isObject())
        {
            updateObjectProperties(exec, val, item);
        }
        else
        {
            // It has no kids, so treeview item better not, either
            QList<QTreeWidgetItem*> kids = item->takeChildren();
            qDeleteAll(kids);
        }
    }   
}

void LocalVariablesDock::updateObjectProperties(KJS::ExecState* exec, KJS::JSValue* val,
                                                QTreeWidgetItem* item, bool globalObject)
{
    bool root = (item == m_view->invisibleRootItem());

    // We have to be careful here -- we don't want to recurse if we're not open;
    // except for root since we may want the + there
    bool recurse = item->isExpanded() || root;

    QStringList props;
    KJS::JSObject* obj = 0;

    // Get the list of all relevant properties..
    // Note: val may be null for root case..
    if (val)
    {
        assert (val->isObject());

        obj = val->getObject();
        KJS::PropertyNameArray jsProps;

        // We use getOwnPropertyNames and not getPropertyNames here
        // to not include things from the prototype
        obj->getOwnPropertyNames(exec, jsProps, KJS::PropertyMap::ExcludeDontEnumProperties);

        for (int pos = 0; pos < jsProps.size(); ++pos) 
        {
            // For global (window) objects, we only show hash table properties, 
            // for less cluttered display.
            if (globalObject && !obj->getDirect(jsProps[pos]))
                continue;

            props.append(jsProps[pos].ustring().qstring());
        }
    }

    // If we're not an activation, also list the prototype, as __proto__
    if (val && !val->getObject()->isActivation())
        props << QLatin1String("__proto__");

    // If we're the root, also pretend 'this' is there.
    if (root && exec)
        props << QLatin1String("this");

    // Sort them, to make updates easier.
    props.sort();

    // Do we need more or less nodes?
    while (props.size() < item->childCount())
        delete item->takeChild(item->childCount() - 1);

    while (props.size() > item->childCount())
        item->addChild(new QTreeWidgetItem);

    // Update names and values.
    for (int pos = 0; pos < props.size(); ++pos) 
    {
        QString propName = props[pos];
        QTreeWidgetItem* kid = item->child(pos);
        kid->setText(0, propName);

        if (root && propName == "this")
            updateValue(exec, exec->thisValue(), kid, true);
        else
            updateValue(exec, obj->get(exec, KJS::Identifier(KJS::UString(props[pos]))), kid, recurse);
    }
}

void LocalVariablesDock::slotItemExpanded(QTreeWidgetItem* item)
{
    Q_UNUSED(item);
    updateDisplay(m_execState);
}

void LocalVariablesDock::updateDisplay(KJS::ExecState *exec)
{
    m_execState = exec;

    // Find out out scope object...
    KJS::JSObject* scopeObject = 0;
    KJS::Context*  context =0;

    if (exec)
        context = exec->context();

    // Find the nearest local scope, or 
    // failing that the topmost scope
    if (context)
    {
        KJS::ScopeChain chain = context->scopeChain();
        for( KJS::ScopeChainIterator iter = chain.begin();
            iter != chain.end(); ++iter)
        {
            scopeObject = *iter;
            if (scopeObject->isActivation())
                break;
        }
    }

    updateObjectProperties(exec, scopeObject, m_view->invisibleRootItem(),
                           scopeObject && !scopeObject->isActivation());
}

}
