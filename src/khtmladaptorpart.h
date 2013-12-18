/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2007 Harri Porten (porten@kde.org)
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

#ifndef KHTMLADAPTORPART_H
#define KHTMLADAPTORPART_H

#include <khtml_export.h>
#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kpluginfactory.h>

namespace KJS
{
    class ExecState;
    class JSObject;
}

class ScriptingInterface
{
public:
    virtual ~ScriptingInterface() { }

    virtual void initScripting(KJS::ExecState *exec) = 0;
    virtual void stopScripting() = 0;

    virtual KJS::JSObject* scriptObject() = 0;
};

Q_DECLARE_INTERFACE(ScriptingInterface, "org.kde.khtml.ScriptingInterface")

class KHTMLAdaptorPartFactory : public KPluginFactory {
    Q_OBJECT
public:
    KHTMLAdaptorPartFactory ();
    virtual QObject *create(const char *iface,
                            QWidget *wparent,
                            QObject *parent,
                            const QVariantList &args,
                            const QString &keyword);
};

class AdaptorView : public KParts::ReadOnlyPart,
                    public ScriptingInterface {
    Q_OBJECT
    Q_INTERFACES(ScriptingInterface)
public:
    AdaptorView(QWidget* wparent, QObject* parent, const QStringList& args);

    void initScripting(KJS::ExecState *exec);
    void stopScripting() { }
    KJS::JSObject* scriptObject();

protected:
    bool openFile();
};

#endif
