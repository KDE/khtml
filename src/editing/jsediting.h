/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef __jsediting_h__
#define __jsediting_h__

class KHTMLPart;
class QString;

namespace DOM {

class DocumentImpl;
class DOMString;
struct CommandImp;

class JSEditor {

public:
    JSEditor(DocumentImpl *doc) : m_doc(doc) { }

    bool execCommand(const CommandImp *, bool userInterface, const DOMString &value);
    bool queryCommandEnabled(const CommandImp *);
    bool queryCommandIndeterm(const CommandImp *);
    bool queryCommandState(const CommandImp *);
    bool queryCommandSupported(const CommandImp *);
    DOMString queryCommandValue(const CommandImp *);

    const CommandImp *commandImp(const DOMString &command);
    const CommandImp *commandImp(int command);

private:
    JSEditor(const JSEditor &);
    JSEditor &operator=(const JSEditor &);

    DocumentImpl *m_doc;
};

} // end namespace DOM

#endif
