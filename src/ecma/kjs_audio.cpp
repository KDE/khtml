/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2006 Germain Garand <germain@ebooksfrance.org>
 *  Copyright (C) 2007 Matthias Kretz <kretz@kde.org>
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

#include "ecma/kjs_audio.h"

#include <html/HTMLAudioElement.h>

namespace KJS {

AudioConstructorImp::AudioConstructorImp(ExecState* exec, DOM::DocumentImpl* d)
    : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), doc(d)
{
}

bool AudioConstructorImp::implementsConstruct() const
{
    return true;
}

JSObject *AudioConstructorImp::construct(ExecState *exec, const List &list)
{
    khtml::HTMLAudioElement* audio = static_cast<khtml::HTMLAudioElement*>(doc->createElement("audio"));

    QString url;
    if (list.size() > 0) {
        url = list.at(0)->toString(exec).qstring();
        if (!url.isEmpty())
            audio->setSrc(url);
    }
    return getDOMNode(exec, audio)->getObject();
}

} // namespace KJS

