/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2006 Matt Broadstone (mbroadst@gmail.com)
 *  Copyright (C) 2007 Maksim Orlovich (maksim@kde.org)
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

#include "interpreter_ctx.h"

namespace KJSDebugger {

void InterpreterContext::addCall(DebugDocument::Ptr doc, const QString &function, int lineNumber)
{
    CallStackEntry entry;
    entry.doc  = doc;
    entry.name = function;
    entry.lineNumber = lineNumber;

    callStack.append(entry);
}

void InterpreterContext::removeCall()
{
    callStack.pop_back();
}

void InterpreterContext::updateCall(int line)
{
    callStack.last().lineNumber = line;
    // Expects displayStack to be called to redraw.
}

int InterpreterContext::activeLine()
{
    return callStack.top().lineNumber;
}

DebugDocument::Ptr InterpreterContext::activeDocument()
{
    return callStack.top().doc;
}

bool InterpreterContext::hasActiveDocument() const
{
    return !callStack.isEmpty();
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
