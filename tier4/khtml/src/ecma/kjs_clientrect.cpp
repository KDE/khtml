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

#include "kjs_clientrect.h"

#include "kjs_clientrect.lut.h"

namespace KJS {

// table for ClientRect object
/*
@begin ClientRectTable 6
  height        ClientRect::Height      DontEnum|ReadOnly
  width         ClientRect::Width       DontEnum|ReadOnly
  top           ClientRect::Top         DontEnum|ReadOnly
  left          ClientRect::Left        DontEnum|ReadOnly
  bottom        ClientRect::Bottom      DontEnum|ReadOnly
  right         ClientRect::Right       DontEnum|ReadOnly
@end
*/

const ClassInfo ClientRect::info = { "ClientRect", 0, &ClientRectTable, 0 };

ClientRect::ClientRect(ExecState* /*exec*/, float left, float top, float width, float height)
    : JSObject(),
      m_rect(left, top, width, height)
{
}

ClientRect::ClientRect(ExecState* /*exec*/, const QRectF& rect)
    : JSObject(),
      m_rect(rect)
{
}

JSValue* ClientRect::getValueProperty(ExecState* /*exec*/, int token) const
{
    switch (token)
    {
        case Top:
            return jsNumber(top());
        case Right:
            return jsNumber(right());
        case Left:
            return jsNumber(left());
        case Bottom:
            return jsNumber(bottom());
        case Height:
            return jsNumber(height());
        case Width:
            return jsNumber(width());
    }
    return jsUndefined();
}

bool ClientRect::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<ClientRect, JSObject>(exec, &ClientRectTable, this, propertyName, slot);
}

float ClientRect::bottom() const
{
    return static_cast<float>(m_rect.bottom());
}

float ClientRect::height() const
{
    return static_cast<float>(m_rect.height());
}

float ClientRect::left() const
{
    return static_cast<float>(m_rect.left());
}

float ClientRect::right() const
{
    return static_cast<float>(m_rect.right());
}

float ClientRect::top() const
{
    return static_cast<float>(m_rect.top());
}

float ClientRect::width() const
{
    return static_cast<float>(m_rect.width());
}

void ClientRect::setHeight(float height)
{
    m_rect.setHeight(height);
}

void ClientRect::setLeft(float left)
{
    m_rect.setLeft(left);
}

void ClientRect::setTop(float top)
{
    m_rect.setTop(top);
}

void ClientRect::setWidth(float width)
{
    m_rect.setWidth(width);
}


// ---------------------- ClientRectList --------------------------

// table for ClientRectList object
/*
@begin ClientRectListTable 1
  length        ClientRectList::Length      DontEnum|ReadOnly
@end
*/

const ClassInfo ClientRectList::info = { "ClientRectList", 0, &ClientRectListTable, 0 };

ClientRectList::ClientRectList(ExecState* /*exec*/)
    : JSObject()
{
}

ClientRectList::ClientRectList(ExecState* exec, const QList< QRectF >& list)
    : JSObject()
{
    foreach(const QRectF& rect, list)
        m_list.append(new ClientRect(exec, rect));
}

unsigned int ClientRectList::length() const
{
    return m_list.size();
}

JSValue* ClientRectList::getValueProperty(ExecState* /*exec*/, int token) const
{
    switch(token)
    {
        case Length:
            return jsNumber(length());
    }
    return jsUndefined();
}

bool ClientRectList::getOwnPropertySlot(ExecState* exec, unsigned int index, PropertySlot& slot)
{
    if (index >= length())
    {
        setDOMException(exec, DOM::DOMException::INDEX_SIZE_ERR);
        return false;
    }
    slot.setValue(this, m_list.at(index));
    return true;
}

bool ClientRectList::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    bool ok;
    unsigned int index = propertyName.toArrayIndex(&ok);
    if (ok)
        return ClientRectList::getOwnPropertySlot(exec, index, slot);

    bool ret = getStaticValueSlot<ClientRectList, JSObject>(exec, &ClientRectListTable, this, propertyName, slot);

    if (ret)
        return ret;

    setDOMException(exec, DOM::DOMException::INDEX_SIZE_ERR);
    return false;
}

ClientRect* ClientRectList::item(unsigned int index)
{
    ASSERT(index < length());
    return m_list.at(index);
}

void ClientRectList::append(ClientRect* item)
{
    m_list.append(item);
}


} //namespace KJS

