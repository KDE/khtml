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

#ifndef KJS_CLIENTRECT_H
#define KJS_CLIENTRECT_H

#include "ecma/kjs_dom.h"

#include <kjs/object.h>

#include <QRect>
#include <QList>

namespace KJS {

class ClientRect : public JSObject {
public:
    ClientRect(ExecState *exec, float left, float top, float width, float height);
    ClientRect(ExecState *exec, const QRectF& rect);
    enum {
        Top, Right, Bottom, Left, Width, Height
    };
    JSValue* getValueProperty(ExecState *exec, int token) const;

    using KJS::JSObject::getOwnPropertySlot;
    bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);

    float top() const;
    float left() const;
    float width() const;
    float height() const;
    float right() const;
    float bottom() const;

    void setTop(float top);
    void setLeft(float left);
    void setWidth(float width);
    void setHeight(float height);

private:
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    QRectF m_rect;
};


class ClientRectList : public JSObject {
public:
    ClientRectList(ExecState* exec);
    ClientRectList(ExecState* exec, const QList<QRectF>& list);
    enum {
        Length
    };

    JSValue* getValueProperty(ExecState *exec, int token) const;
    bool getOwnPropertySlot(ExecState *exec, unsigned int index, PropertySlot& slot);
    bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);

    unsigned length() const;
    ClientRect* item(unsigned index);
    void append(ClientRect* item);

private:
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    WTF::Vector< ProtectedPtr<ClientRect> > m_list;
};

}

#endif //KJS_CLIENTRECT_H
