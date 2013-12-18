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
#ifndef KJSDBG_ICTX_H
#define KJSDBG_ICTX_H

#include <QStack>
#include <QString>
#include "debugdocument.h"

namespace KJS {
    class ExecState;
}

namespace KTextEditor {
    class MarkInterface;
}

namespace KJSDebugger {

enum Mode
{
    Normal   = 0, // Only stop at breakpoints
    StepOver = 1, // Will break on next statement in current context
    StepOut  = 2, // Will break one or more contexts above.
    Step     = 3, // Will break on next statement in current or deeper context
    Abort    = 4  // The script will stop execution completely,
                    // as soon as possible
};

struct CallStackEntry
{
    QString name;
    int lineNumber;
    DebugDocument::Ptr doc;

    bool operator==(const CallStackEntry& other) const
    {
        return ((other.name == name) &&
                (other.lineNumber == lineNumber) &&
                (other.doc        == doc));
    }
};

// This contains information we have to keep track of per-interpreter,
// such as the stack information. 
struct InterpreterContext
{
    Mode mode;
    QStack<KJS::ExecState*> execContexts;
    int  depthAtSkip; // How far we were in on stepOut
                      // our stepOver.
    QStack<CallStackEntry> callStack;

    // Document and line we're currently in
    DebugDocument::Ptr activeDocument();
    int                activeLine();

    bool hasActiveDocument() const;

    InterpreterContext() : mode(Normal), depthAtSkip(0)
    {}

    /**
     * add a new entry to the call stack
     *
     * @param doc the document the function is in
     * @param function the function called
     * @param line the line number of the function
     */
    void addCall(DebugDocument::Ptr doc, const QString &function, int line);
    /**
     * update the line number of the current context
     *
     * @param line the new line number within the current function
     */
    void updateCall(int line);
    void removeCall();

};

}

#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;

