/*
    This file is part of the KDE libraries

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
              (C) 1998 Waldo Bastian (bastian@kde.org)
              (C) 1999 Lars Knoll (knoll@kde.org)
              (C) 1999 Antti Koivisto (koivisto@kde.org)
              (C) 2001-2003 Dirk Mueller (mueller@kde.org)
              (C) 2004-2008 Apple Computer, Inc.
              (C) 2006-2008 Germain Garand (germain@ebooksfrance.org)

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
//----------------------------------------------------------------------------
//
// KDE HTML Widget - Tokenizers

// #define TOKEN_DEBUG 1
//#define TOKEN_DEBUG 2

#include "htmltokenizer.h"
#include "html_documentimpl.h"
#include "htmlparser.h"
#include "dtd.h"

#include <misc/loader.h>

#include <khtmlview.h>
#include <khtml_part.h>
#include <xml/dom_docimpl.h>
#include <css/csshelper.h>
#include <ecma/kjs_proxy.h>
#include <kcharsets.h>
#include <ctype.h>
#include <assert.h>
#include <QtCore/QVariant>
#include <QDebug>
#include <stdlib.h>

#include "kentities.c"
#include "htmlprospectivetokenizer.h"

#define PROSPECTIVE_TOKENIZER_ENABLED 1

using namespace khtml;

static const QChar commentStart [] = { '<','!','-','-', QChar::Null };
static const char doctypeStart [] = "<!doctype";
static const char publicStart [] = "public";
static const char systemStart [] = "system";

static const char scriptEnd [] = "</script";
static const char xmpEnd [] = "</xmp";
static const char styleEnd [] =  "</style";
static const char textareaEnd [] = "</textarea";
static const char titleEnd [] = "</title";

#ifndef NDEBUG
static const int sTokenizerChunkSize = 2048;
static const int sTokenizerFastYieldDelay = 220;
static const int sTokenizerYieldDelay = 650;
#else
static const int sTokenizerChunkSize = 4096;
static const int sTokenizerFastYieldDelay = 180;
static const int sTokenizerYieldDelay = 450;
#endif

#define KHTML_ALLOC_QCHAR_VEC( N ) (QChar*) malloc( sizeof(QChar)*( N ) )
#define KHTML_REALLOC_QCHAR_VEC(P, N ) (QChar*) realloc(P, sizeof(QChar)*( N ))
#define KHTML_DELETE_QCHAR_VEC( P ) free((char*)( P ))

// Full support for MS Windows extensions to Latin-1.
// Technically these extensions should only be activated for pages
// marked "windows-1252" or "cp1252", but
// in the standard Microsoft way, these extensions infect hundreds of thousands
// of web pages.  Note that people with non-latin-1 Microsoft extensions
// are SOL.
//
// See: http://www.microsoft.com/globaldev/reference/WinCP.asp
//      http://www.bbsinc.com/iso8859.html
//      http://www.obviously.com/
//
// There may be better equivalents
#if 0
#define fixUpChar(x)
#else
#define fixUpChar(x) \
            switch ((x).unicode()) \
            { \
            case 0x80: (x) = 0x20ac; break; \
            case 0x82: (x) = 0x201a;    break; \
            case 0x83: (x) = 0x0192; break; \
            case 0x84: (x) = 0x201e;    break; \
            case 0x85: (x) = 0x2026; break; \
            case 0x86: (x) = 0x2020; break; \
            case 0x87: (x) = 0x2021; break; \
            case 0x88: (x) = 0x02C6; break; \
            case 0x89: (x) = 0x2030; break; \
            case 0x8A: (x) = 0x0160; break; \
            case 0x8b: (x) = 0x2039;    break; \
            case 0x8C: (x) = 0x0152; break; \
            case 0x8E: (x) = 0x017D; break; \
            case 0x91: (x) = 0x2018;   break; \
            case 0x92: (x) = 0x2019;   break; \
            case 0x93: (x) = 0x201C;    break; \
            case 0x94: (x) = 0X201D;    break; \
            case 0x95: (x) = 0x2022;    break; \
            case 0x96: (x) = 0x2013;    break; \
            case 0x97: (x) = 0x2014;    break; \
            case 0x98: (x) = 0x02DC;    break; \
            case 0x99: (x) = 0x2122; break; \
            case 0x9A: (x) = 0x0161; break; \
            case 0x9b: (x) = 0x203A;    break; \
            case 0x9C: (x) = 0x0153; break; \
            case 0x9E: (x) = 0x017E; break; \
            case 0x9F: (x) = 0x0178; break; \
            default: break; \
            }
#endif
// ----------------------------------------------------------------------------

HTMLTokenizer::HTMLTokenizer(DOM::DocumentImpl *_doc, KHTMLView *_view)
{
    view = _view;
    buffer = 0;
    rawContent = 0;
    rawContentSize = rawContentMaxSize = rawContentResync = rawContentSinceLastEntity = 0;
    charsets = KCharsets::charsets();
    parser = new KHTMLParser(_view, _doc);
    m_executingScript = 0;
    m_externalScriptsTimerId = 0;
    m_tokenizerYieldDelay = sTokenizerFastYieldDelay;
    m_yieldTimer = 0;
    m_prospectiveTokenizer = 0;
    onHold = false;
    m_documentTokenizer = true;
    m_hasScriptsWaitingForStylesheets = false;

    reset();
}

HTMLTokenizer::HTMLTokenizer(DOM::DocumentImpl *_doc, DOM::DocumentFragmentImpl *i)
{
    view = 0;
    buffer = 0;
    rawContent = 0;
    rawContentSize = rawContentMaxSize = rawContentResync = rawContentSinceLastEntity = 0;
    charsets = KCharsets::charsets();
    parser = new KHTMLParser( i, _doc );
    m_executingScript = 0;
    m_externalScriptsTimerId = 0;
    m_tokenizerYieldDelay = sTokenizerFastYieldDelay;
    m_yieldTimer = 0;
    m_prospectiveTokenizer = 0;
    onHold = false;
    m_documentTokenizer = false;
    m_hasScriptsWaitingForStylesheets = false;

    reset();
}

void HTMLTokenizer::setNormalYieldDelay()
{
    m_tokenizerYieldDelay = sTokenizerYieldDelay;
}

void HTMLTokenizer::reset()
{
    assert(m_executingScript == 0);
    Q_ASSERT(onHold == false);
    m_abort = false;

    while (!cachedScript.isEmpty())
        cachedScript.dequeue()->deref(this);

    if ( buffer )
        KHTML_DELETE_QCHAR_VEC(buffer);
    buffer = dest = 0;
    size = 0;

    if ( rawContent )
        KHTML_DELETE_QCHAR_VEC(rawContent);
    rawContent = 0;
    rawContentSize = rawContentMaxSize = rawContentResync = 0;

    if (m_yieldTimer > 0) {
        killTimer(m_yieldTimer);
        m_yieldTimer = 0;
    }

    if (m_externalScriptsTimerId > 0) {
        killTimer(m_externalScriptsTimerId);
        m_externalScriptsTimerId = 0;
    }
    currToken.reset();
    doctypeToken.reset();
    javascript = false;
}

void HTMLTokenizer::begin()
{
    m_executingScript = 0;
    onHold = false;
    reset();
    size = 254;
    buffer = KHTML_ALLOC_QCHAR_VEC( 255 );
    dest = buffer;
    tag = NoTag;
    pending = NonePending;
    discard = NoneDiscard;
    pre = false;
    prePos = 0;
    plaintext = false;
    xmp = false;
    processingInstruction = false;
    script = false;
    escaped = false;
    style = false;
    skipLF = false;
    select = false;
    comment = false;
    doctype = false;
    doctypeComment = NoDoctypeComment;
    doctypeAllowComment = false;
    server = false;
    textarea = false;
    title = false;
    startTag = false;
    tquote = NoQuote;
    searchCount = 0;
    doctypeSearchCount = 0;
    doctypeSecondarySearchCount = 0;
    Entity = NoEntity;
    noMoreData = false;
    brokenComments = false;
    brokenServer = false;
    lineno = 0;
    scriptStartLineno = 0;
    tagStartLineno = 0;
}

void HTMLTokenizer::processListing(TokenizerString list)
{
    bool old_pre = pre;

    // This function adds the listing 'list' as
    // preformatted text-tokens to the token-collection
    // thereby converting TABs.
    if(!style) pre = true;
    prePos = 0;

    while ( !list.isEmpty() )
    {
        checkBuffer(3*TAB_SIZE);

        if (skipLF && ( list->unicode() != '\n' ))
        {
            skipLF = false;
        }

        if (skipLF)
        {
            skipLF = false;
            ++list;
        }
        else if (( list->unicode() == '\n' ) || ( list->unicode() == '\r' ))
        {
            if (discard == LFDiscard)
            {
                // Ignore this LF
                discard = NoneDiscard; // We have discarded 1 LF
            }
            else
            {
                // Process this LF
                if (pending)
                    addPending();

                // we used to do it not at all and we want to have
                // it fixed for textarea. So here we are
                if ( textarea ) {
                    prePos++;
                    *dest++ = *list;
                } else
                    pending = LFPending;
            }
            /* Check for MS-DOS CRLF sequence */
            if (list->unicode() == '\r')
            {
                skipLF = true;
            }
            ++list;
        }
        else if (( list->unicode() == ' ' ) || ( list->unicode() == '\t'))
        {
            if (pending)
                addPending();
            if (*list == ' ')
                pending = SpacePending;
            else
                pending = TabPending;

            ++list;
        }
        else
        {
            discard = NoneDiscard;
            if (pending)
                addPending();

            prePos++;
            *dest++ = *list;
            ++list;
        }

    }

    if ((pending == SpacePending) || (pending == TabPending))
        addPending();
    else
        pending = NonePending;

    prePos = 0;
    pre = old_pre;
}

void HTMLTokenizer::parseRawContent(TokenizerString &src)
{
    // The 'raw content' mode is a very lax tokenizing mode
    // that will absorb anything but the exact closing tag
    // that made us enter this mode, *except* if it inside a comment.
    //
    // Any other tag or comment will be passed verbatim to the parser as part
    // of the content. It is used for script, style, and a few others.
    // 
    assert( textarea || title || !Entity );
    assert( !tag );
    assert( xmp+textarea+title+style+script == 1 );
    if (script)
        scriptStartLineno = lineno+src.lineCount();

    if ( comment ) parseComment( src );

    while ( !src.isEmpty() ) {
        checkRawContentBuffer();
        unsigned char ch = src->toLatin1();
        if ( !rawContentResync && !brokenComments && !xmp && ch == '-' && 
              rawContentSize >= 3 && ((!textarea && !title) || rawContentSinceLastEntity >= 3) && !src.escaped() && 
              QString::fromRawData( rawContent+rawContentSize-3, 3 ) == "<!-" ) {
            comment = true;
            rawContent[ rawContentSize++ ] = ch;
            ++src;
            parseComment( src );
            continue;
        }
        if ( rawContentResync && !tquote && ( ch == '>' ) ) {
            ++src;
            rawContentSize = rawContentResync-1;
            rawContentResync = 0;
            rawContent[ rawContentSize ] = rawContent[ rawContentSize + 1 ] = 0;
            if ( script )
                scriptHandler();
            else {
                processListing(TokenizerString(rawContent, rawContentSize));
                processToken();
                if ( style )         { currToken.tid = ID_STYLE + ID_CLOSE_TAG; }
                else if ( textarea ) { currToken.tid = ID_TEXTAREA + ID_CLOSE_TAG; }
                else if ( title ) { currToken.tid = ID_TITLE + ID_CLOSE_TAG; }
                else if ( xmp )  { currToken.tid = ID_XMP + ID_CLOSE_TAG; }
                processToken();
                script = style = textarea = title = xmp = false;
                tquote = NoQuote;
                rawContentSize = rawContentResync = 0;
            }
            return;
        }
        // possible end of tagname, lets check.
        if ( !rawContentResync && !escaped && !src.escaped() && ( ch == '>' || ch == '/' || ch <= ' ' ) && ch &&
              rawContentSize >= searchStopperLen && ((!textarea && !title) || rawContentSinceLastEntity >= searchStopperLen) &&
              QString::compare(QString::fromRawData(rawContent + rawContentSize - searchStopperLen, searchStopperLen),
                  QLatin1String(searchStopper), Qt::CaseInsensitive) == 0) {

            // the purpose of rawContentResync is to look for an end tag that could possibly be of the form:
            // </endtag   junk="more junk>\"><>"     >
            // IOW, once the '</endtag' sequence has been found, the rest of the tag must still be validated,
            // so this micro-tokenizer switches to rawContentResync state until '>' is finally found.
            rawContentResync = rawContentSize-searchStopperLen+1;
            tquote = NoQuote;
            continue;
        }
        if ( rawContentResync && !escaped ) {
            if(ch == '\"')
                tquote = (tquote == NoQuote) ? DoubleQuote : ((tquote == SingleQuote) ? SingleQuote : NoQuote);
            else if(ch == '\'')
                tquote = (tquote == NoQuote) ? SingleQuote : (tquote == DoubleQuote) ? DoubleQuote : NoQuote;
            else if (tquote != NoQuote && (ch == '\r' || ch == '\n'))
                tquote = NoQuote;
        }
        escaped = ( !escaped && ch == '\\' );
        if (!rawContentResync && (textarea||title) && !src.escaped() && ch == '&') {
            QChar *rawContentDest = rawContent+rawContentSize;
            ++src;
            parseEntity(src,rawContentDest,true);
            rawContentSize = rawContentDest-rawContent;
        }
        else {
            rawContent[ rawContentSize++ ] = *src;
            ++src;
            ++rawContentSinceLastEntity;
        }
    }
}

void HTMLTokenizer::scriptHandler()
{
    QString currentScriptSrc = scriptSrc;
    scriptSrc.clear();

    processListing(TokenizerString(rawContent, rawContentSize));
    QString exScript( buffer, dest-buffer );

    processToken();
    currToken.tid = ID_SCRIPT + ID_CLOSE_TAG;
    processToken();

    // Scripts following a frameset element should not be executed or even loaded in the case of extern scripts.
    bool followingFrameset = (parser->doc()->body() && parser->doc()->body()->id() == ID_FRAMESET);
    bool effectiveScript = !parser->skipMode() && !followingFrameset;
    bool deferredScript = false;

    if ( effectiveScript ) {
        CachedScript* cs = 0;

        // forget what we just got, load from src url instead
        if ( !currentScriptSrc.isEmpty() && javascript &&
             (cs = parser->doc()->docLoader()->requestScript(currentScriptSrc, scriptSrcCharset) )) {
            cachedScript.enqueue(cs);
        }

        if (cs) {
            pendingQueue.push(src);
            int scriptCount = cachedScript.count();
            setSrc(TokenizerString());
            rawContentSize = rawContentResync = 0;
            cs->ref(this);
            if (cachedScript.count() == scriptCount)
                deferredScript = true;
        }
        else if (currentScriptSrc.isEmpty() && view && javascript ) {
            pendingQueue.push(src);
            setSrc(TokenizerString());
            rawContentSize = rawContentResync = 0;
            scriptExecution( exScript, QString(), tagStartLineno /*scriptStartLineno*/ );
        } else {
            // script was filtered or disallowed
            effectiveScript = false;
        }
    }

    script = false;
    rawContentSize = rawContentResync = 0;

    if ( !effectiveScript )
        return;

    if ( !m_executingScript && cachedScript.isEmpty() ) {
        src.append(pendingQueue.pop());
    } else if ( cachedScript.isEmpty() ) {
        write( pendingQueue.pop(), false );
    } else if ( !deferredScript && pendingQueue.count() > 1) {
        TokenizerString t = pendingQueue.pop();
        pendingQueue.top().prepend( t );
    }
#if PROSPECTIVE_TOKENIZER_ENABLED
    if (!cachedScript.isEmpty() && !m_executingScript) {
        if (!m_prospectiveTokenizer)
            m_prospectiveTokenizer = new ProspectiveTokenizer(parser->docPtr());
        if (!m_prospectiveTokenizer->inProgress() && !pendingQueue.isEmpty()) {
            m_prospectiveTokenizer->begin();
            m_prospectiveTokenizer->write(pendingQueue.top());
        }
    }
#endif

}

void HTMLTokenizer::scriptExecution( const QString& str, const QString& scriptURL,
                                     int baseLine)
{
    bool oldscript = script;
    m_executingScript++;
    script = false;
    QString url;
    if (scriptURL.isNull() && view)
      url = static_cast<DocumentImpl*>(view->part()->document().handle())->URL().url();
    else
      url = scriptURL;

    if (view)
	view->part()->executeScript(url,baseLine,Node(),str);
    m_executingScript--;
    script = oldscript;
}

void HTMLTokenizer::parseComment(TokenizerString &src)
{
    checkRawContentBuffer(src.length());
    while ( src.length() ) {
        rawContent[ rawContentSize++ ] = *src;

#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 1
        qDebug("comment is now: *%s*", src.toString().left(16).toLatin1().constData());
#endif

        if (src->unicode() == '>')
        {
            bool handleBrokenComments =  brokenComments && !( script || style );
            bool scriptEnd=false;
            if ( rawContentSize > 2 && rawContent[rawContentSize-3] == '-' &&
                  rawContent[rawContentSize-2] == '-' )
            {
                scriptEnd=true;
            }

            if (handleBrokenComments || scriptEnd ){
                ++src;
                if ( !( title || script || xmp || textarea || style) ) {
                    checkRawContentBuffer();
                    rawContent[ rawContentSize ] = 0;
                    rawContent[ rawContentSize + 1 ] = 0;
                    currToken.tid = ID_COMMENT;
                    int size = scriptEnd ? rawContentSize - 3 : rawContentSize - 1;
                    processListing(TokenizerString(rawContent, size));
                    processToken();
                    currToken.tid = ID_COMMENT + ID_CLOSE_TAG;
                    processToken();
                    rawContentSize = 0;
                }
                comment = false;
                return; // Finished parsing comment
            }
        }
        ++src;
    }
}

void HTMLTokenizer::parseDoctypeComment(TokenizerString &src)
{
    while (!src.isEmpty()) {
        QChar c = *src;
        switch (doctypeComment) {
            case DoctypeCommentHalfBegin: {
                if (c != '-') {
                    // Ooops, it's not comment
                    doctypeComment = DoctypeCommentBogus;
                    return;
                } else {
                    // Doctype comment begins
                    doctypeComment = DoctypeComment;
                    ++src;
                }
                break;
            }
            case DoctypeComment: {
                if (c == '-') {
                    // Perhaps this is end of comment
                    doctypeComment = DoctypeCommentHalfEnd;
                    ++src;
                } else {
                    // Keep scanning for '--'
                    ++src;
                }
                break;
            }
            case DoctypeCommentHalfEnd: {
                if (c == '-') {
                    // Doctype comment ends
                    doctypeComment = DoctypeCommentEnd;
                    return;
                } else {
                    // It's not '--'
                    ++src;
                    doctypeComment = DoctypeComment;
                }
                break;
            }
            default: {
                assert(!"Undefined doctype comment state");
                break;
            }
        }
    }
}

void HTMLTokenizer::parseDoctype(TokenizerString &src)
{
    while (!src.isEmpty() && doctype) {
        QChar c;
        bool isWhitespace = false;
        int dontAdvance = 0;
        if (doctypeComment == DoctypeCommentEnd) {
            doctypeComment = NoDoctypeComment;
            isWhitespace = true;
        } else if (doctypeComment == DoctypeCommentBogus) {
            doctypeComment = NoDoctypeComment;
            c = '-';
            dontAdvance++;
        } else {
            c = *src;
            if (doctypeAllowComment) {
                if (!doctypeComment && c == '-') {
                    doctypeComment = DoctypeCommentHalfBegin;
                    ++src;
                }
                if (doctypeComment) {
                    parseDoctypeComment(src);
                    continue;
                }
                isWhitespace = c == '\r' || c == '\n' || c == '\t' || c == ' ';
            }
        }

        switch (doctypeToken.state) {
            case DoctypeBegin: {
                doctypeToken.state = DoctypeBeforeName;
                if (isWhitespace) {
                    // nothing
                }
                break;
            }
            case DoctypeBeforeName: {
                if (c == '>') {
                    // Malformed. Just exit.
                    doctype = false;
                } else if (isWhitespace) {
                    // nothing
                } else {
                    dontAdvance++;
                    doctypeToken.state = DoctypeName;
                }
                break;
            }
            case DoctypeName: {
                if (c == '>') {
                    // Valid doctype. Emit it.
                    doctype = false;
                    processDoctypeToken();
                } else if (isWhitespace) {
                    doctypeSearchCount = 0; // Used now to scan for PUBLIC
                    doctypeSecondarySearchCount = 0; // Used now to scan for SYSTEM
                    doctypeToken.state = DoctypeAfterName;
                } else {
                    doctypeToken.name.append(c);
                }
                break;
            }
            case DoctypeAfterName: {
                if (c == '>') {
                    // Valid doctype. Emit it.
                    doctype = false;
                    processDoctypeToken();
                } else if (c == '[') {
                    if(doctypeSearchCount > 0 || doctypeSecondarySearchCount > 0) { // is there any public/system indicator before?
                        doctypeSearchCount = doctypeSecondarySearchCount = 0;
                        doctypeToken.state = DoctypeBogus;
                    }
                    // Found internal subset
                    doctypeToken.state = DoctypeInternalSubset;
                    doctypeAllowComment = false;
                } else if (!isWhitespace) {
                    if (c.toLower() == publicStart[doctypeSearchCount]) {
                        doctypeSearchCount++;
                        if(doctypeSearchCount == 6)
                            // Found 'PUBLIC' sequence
                            doctypeToken.state = DoctypeBeforePublicID;
                    } else if (doctypeSearchCount > 0) {
                        doctypeSearchCount = 0;
                        doctypeToken.state = DoctypeBogus;
                    } else if (c.toLower() == systemStart[doctypeSecondarySearchCount]) {
                        doctypeSecondarySearchCount++;
                        if(doctypeSecondarySearchCount == 6)
                            // Found 'SYSTEM' sequence
                            doctypeToken.state = DoctypeBeforeSystemID;
                    } else {
                        doctypeSecondarySearchCount = 0;
                        doctypeToken.state = DoctypeBogus;
                    }
                } else {
                    // Whitespace keeps us in the after name state
                }
                break;
            }
            case DoctypeBeforePublicID: {
                if (c == '\"' || c == '\'') {
                    tquote = c == '\"' ? DoubleQuote : SingleQuote;
                    doctypeToken.state = DoctypePublicID;
                    doctypeAllowComment = false;
                } else if (c == '>') {
                    // Considered bogus. Don't process the doctype.
                    doctype = false;
                } else if (isWhitespace) {
                    // nothing
                } else
                    doctypeToken.state = DoctypeBogus;
                break;
            }
            case DoctypePublicID: {
                if ((c == '\"' && tquote == DoubleQuote) || (c == '\'' && tquote == SingleQuote)) {
                    doctypeToken.state = DoctypeAfterPublicID;
                    doctypeAllowComment = true;
                } else if (c == '>') {
                    // Considered bogus. Don't process the doctype.
                    doctype = false;
                } else {
                    doctypeToken.publicID.append(c);
                }
                break;
            }
            case DoctypeAfterPublicID: {
                if (c == '\"' || c == '\'') {
                    tquote = c == '\"' ? DoubleQuote : SingleQuote;
                    doctypeToken.state = DoctypeSystemID;
                } else if (c == '>') {
                    // Valid doctype. Emit it now.
                    doctype = false;
                    processDoctypeToken();
                } else if (isWhitespace) {
                    // nothing
                } else if (c == '[') {
                    // Found internal subset
                    doctypeToken.state = DoctypeInternalSubset;
                    doctypeAllowComment = false;
                } else
                    doctypeToken.state = DoctypeBogus;
                break;
            }
            case DoctypeBeforeSystemID: {
                if (c == '\"' || c == '\'') {
                    tquote = c == '\"' ? DoubleQuote : SingleQuote;
                    doctypeToken.state = DoctypeSystemID;
                    doctypeAllowComment = false;
                } else if (c == '>') {
                    // Considered bogus. Don't process the doctype.
                    doctype = false;
                } else if (isWhitespace) {
                    // nothing
                } else
                    doctypeToken.state = DoctypeBogus;
                break;
            }
            case DoctypeSystemID: {
                if ((c == '\"' && tquote == DoubleQuote) || (c == '\'' && tquote == SingleQuote)) {
                    doctypeToken.state = DoctypeAfterSystemID;
                    doctypeAllowComment = true;
                } else if (c == '>') {
                    // Considered bogus. Don't process the doctype.
                    doctype = false;
                } else {
                    doctypeToken.systemID.append(c);
                }
                break;
            }
            case DoctypeAfterSystemID: {
                if (c == '>') {
                    // Valid doctype. Emit it now.
                    doctype = false;
                    processDoctypeToken();
                } else if (isWhitespace) {
                    // nothing
                } else if (c == '[') {
                    // Found internal subset
                    doctypeToken.state = DoctypeInternalSubset;
                    doctypeAllowComment = false;
                } else {
                    doctypeToken.state = DoctypeBogus;
                }
                break;
            }
            case DoctypeInternalSubset: {
                if(c == ']') {
                    // Done
                    doctypeToken.state = DoctypeAfterInternalSubset;
                    doctypeAllowComment = true;
                } else {
                    doctypeToken.internalSubset.append(c);
                }
                break;
            }
            case DoctypeAfterInternalSubset: {
                if (c == '>') {
                    // Valid doctype. Emit it now.
                    doctype = false;
                    processDoctypeToken();
                } else if (isWhitespace) {
                    // nothing
                } else
                    doctypeToken.state = DoctypeBogus;
                break;
            }
            case DoctypeBogus: {
                if (c == '>') {
                    // Done with the bogus doctype.
                    doctype = false;
                } else {
                    // Just keep scanning for '>'
                }
                break;
            }
            default:
                break;
        }
        if (!dontAdvance)
            ++src;
        else if (dontAdvance == 1)
            continue;
        else // double dontAdvance++, do workaround
            doctypeComment = DoctypeCommentBogus;
    }
}

void HTMLTokenizer::parseServer(TokenizerString &src)
{
    checkRawContentBuffer(src.length());
    while ( !src.isEmpty() ) {
        rawContent[ rawContentSize++ ] = *src;
        if (src->unicode() == '>' &&
            rawContentSize > 1 && rawContent[rawContentSize-2] == '%') {
            ++src;
            server = false;
            rawContentSize = 0;
            return; // Finished parsing server include
        }
        ++src;
    }
}

void HTMLTokenizer::parseProcessingInstruction(TokenizerString &src)
{
    char oldchar = 0;
    while ( !src.isEmpty() )
    {
        unsigned char chbegin = src->toLatin1();
        if(chbegin == '\'') {
            tquote = tquote == SingleQuote ? NoQuote : SingleQuote;
        }
        else if(chbegin == '\"') {
            tquote = tquote == DoubleQuote ? NoQuote : DoubleQuote;
        }
        // Look for '?>'
        // some crappy sites omit the "?" before it, so
        // we look for an unquoted '>' instead. (IE compatible)
        else if ( chbegin == '>' && ( !tquote || oldchar == '?' ) )
        {
            // We got a '?>' sequence
            processingInstruction = false;
            ++src;
            discard=LFDiscard;
            return; // Finished parsing comment!
        }
        ++src;
        oldchar = chbegin;
    }
}

void HTMLTokenizer::parseText(TokenizerString &src)
{
    while ( !src.isEmpty() )
    {
        // do we need to enlarge the buffer?
        checkBuffer();

        // ascii is okay because we only do ascii comparisons
        unsigned char chbegin = src->toLatin1();

        if (skipLF && ( chbegin != '\n' ))
        {
            skipLF = false;
        }

        if (skipLF)
        {
            skipLF = false;
            ++src;
        }
        else if (( chbegin == '\n' ) || ( chbegin == '\r' ))
        {
            if (chbegin == '\r')
                skipLF = true;

            *dest++ = '\n';
            ++src;
        }
        else {
            *dest++ = *src;
            ++src;
        }
    }
}


void HTMLTokenizer::parseEntity(TokenizerString &src, QChar *&dest, bool start)
{
    if( start )
    {
        cBufferPos = 0;
        entityLen = 0;
        Entity = SearchEntity;
    }

    while( !src.isEmpty() )
    {
        ushort cc = src->unicode();
        switch(Entity) {
        case NoEntity:
            return;

            break;
        case SearchEntity:
            if(cc == '#') {
                cBuffer[cBufferPos++] = cc;
                ++src;
                Entity = NumericSearch;
            }
            else
                Entity = EntityName;

            break;

        case NumericSearch:
            if(cc == 'x' || cc == 'X') {
                cBuffer[cBufferPos++] = cc;
                ++src;
                Entity = Hexadecimal;
            }
            else if(cc >= '0' && cc <= '9')
                Entity = Decimal;
            else
                Entity = SearchSemicolon;

            break;

        case Hexadecimal:
        {
            int uc = EntityChar.unicode();
            int ll = qMin<uint>(src.length(), 8);
            while(ll--) {
                QChar csrc(src->toLower());
                cc = csrc.cell();

                if(csrc.row() || !((cc >= '0' && cc <= '9') || (cc >= 'a' && cc <= 'f'))) {
                    break;
                }
                uc = uc*16 + (cc - ( cc < 'a' ? '0' : 'a' - 10));
                cBuffer[cBufferPos++] = cc;
                ++src;
            }
            EntityChar = QChar(uc);
            Entity = SearchSemicolon;
            break;
        }
        case Decimal:
        {
            int uc = EntityChar.unicode();
            int ll = qMin(src.length(), 9-cBufferPos);
            while(ll--) {
                cc = src->cell();

                if(src->row() || !(cc >= '0' && cc <= '9')) {
                    Entity = SearchSemicolon;
                    break;
                }

                uc = uc * 10 + (cc - '0');
                cBuffer[cBufferPos++] = cc;
                ++src;
            }
            EntityChar = QChar(uc);
            if(cBufferPos == 9)  Entity = SearchSemicolon;
            break;
        }
        case EntityName:
        {
            int ll = qMin(src.length(), 9-cBufferPos);
            while(ll--) {
                QChar csrc = *src;
                cc = csrc.cell();

                if(csrc.row() || !((cc >= 'a' && cc <= 'z') ||
                                   (cc >= '0' && cc <= '9') || (cc >= 'A' && cc <= 'Z'))) {
                    Entity = SearchSemicolon;
                    break;
                }

                cBuffer[cBufferPos++] = cc;
                ++src;

                // be IE compatible and interpret even unterminated entities
                // outside tags. like "foo &nbspstuff bla".
                if ( tag == NoTag ) {
                    const entity* e = kde_findEntity(cBuffer, cBufferPos);
                    if ( e && e->code < 256 ) {
                        EntityChar = e->code;
                        entityLen = cBufferPos;
                    }
                }
            }
            if(cBufferPos == 9) Entity = SearchSemicolon;
            if(Entity == SearchSemicolon) {
                if(cBufferPos > 1) {
                    const entity *e = kde_findEntity(cBuffer, cBufferPos);
                    // IE only accepts unterminated entities < 256,
                    // Gecko accepts them all, but only outside tags
                    if(e && ( tag == NoTag || e->code < 256 || *src == ';' )) {
                        EntityChar = e->code;
                        entityLen = cBufferPos;
                    }
                }
            }
            break;
        }
        case SearchSemicolon:
#ifdef TOKEN_DEBUG
            qDebug() << "ENTITY " << EntityChar.unicode();
#endif
            fixUpChar(EntityChar);

            if (*src == ';')
                    ++src;

            if ( !EntityChar.isNull() ) {
                checkBuffer();
                if (entityLen > 0 && entityLen < cBufferPos) {
                    int rem = cBufferPos - entityLen;
                    src.prepend( TokenizerString(QString::fromLatin1(cBuffer+entityLen, rem)) );
                }
                src.push( EntityChar );
                rawContentSinceLastEntity = -1;
            } else {
#ifdef TOKEN_DEBUG
                qDebug() << "unknown entity!";
#endif
                checkBuffer(11);
                // ignore the sequence, add it to the buffer as plaintext
                *dest++ = '&';
                for(unsigned int i = 0; i < cBufferPos; i++)
                    dest[i] = cBuffer[i];
                dest += cBufferPos;
                rawContentSinceLastEntity += cBufferPos+1;
                if (pre)
                    prePos += cBufferPos+1;
            }

            Entity = NoEntity;
            EntityChar = QChar::Null;
            return;
        };
    }
}

void HTMLTokenizer::parseTag(TokenizerString &src)
{
    assert(!Entity );
    checkRawContentBuffer( src.length() );

    while ( !src.isEmpty() )
    {
        checkBuffer();
#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 1
        uint l = 0;
        while(l < src.length() && (src.toString()[l]).toLatin1().constData() != '>')
            l++;
        qDebug("src is now: *%s*, tquote: %d",
               src.toString().left(l).toLatin1().constData(), tquote);
#endif
        switch(tag) {
        case NoTag:
            return;
        case TagName:
        {
#if defined(TOKEN_DEBUG) &&  TOKEN_DEBUG > 1
            qDebug("TagName");
#endif
            if (searchCount > 0)
            {
                if (*src == commentStart[searchCount])
                {
                    searchCount++;
                    if (searchCount == 2)
                        doctypeSearchCount++; // A '!' is also part of doctype, so we are moving through that still as well
                    else
                        doctypeSearchCount = 0;

                    if (searchCount == 4)
                    {
#ifdef TOKEN_DEBUG
                        qDebug() << "Found comment";
#endif
                        // Found '<!--' sequence
                        ++src;
                        dest = buffer; // ignore the previous part of this tag
                        tag = NoTag;

                        comment = true;
                        parseComment(src);
                        return; // Finished parsing tag!
                    }
                    // cuts of high part, is okay
                    cBuffer[cBufferPos++] = src->cell();
                    ++src;
                    break;
                }
                else
                    searchCount = 0; // Stop looking for '<!--' sequence
            }

            if (doctypeSearchCount > 0) {
                if((*src).toLower() == doctypeStart[doctypeSearchCount]) {
                    doctypeSearchCount++;
                    cBuffer[cBufferPos++] = src->cell();
                    ++src;
                    if(doctypeSearchCount == 9) {
                        // Found '<!DOCTYPE' sequence
                        tag = NoTag;
                        doctypeAllowComment = true;
                        doctypeComment = NoDoctypeComment;
                        doctypeToken.reset();
                        doctype = true;

                        parseDoctype(src);
                        return;
                    }
                    break;
                } else
                    doctypeSearchCount = 0; // Stop looking for '<!DOCTYPE' sequence
            }

            bool finish = false;
            unsigned int ll = qMin(src.length(), CBUFLEN-cBufferPos);
            while(ll--) {
                ushort curchar = src->unicode();
                if(curchar <= ' ' || curchar == '>' ) {
                    finish = true;
                    break;
                }
                // this is a nasty performance trick. will work for the A-Z
                // characters, but not for others. if it contains one,
                // we fail anyway
                char cc = curchar;
                cBuffer[cBufferPos++] = cc | 0x20;
                ++src;
            }

            // Disadvantage: we add the possible rest of the tag
            // as attribute names. ### judge if this causes problems
            if(finish || CBUFLEN == cBufferPos) {
                bool beginTag;
                char* ptr = cBuffer;
                unsigned int len = cBufferPos;
                cBuffer[cBufferPos] = '\0';
                if ((cBufferPos > 0) && (*ptr == '/'))
                {
                    // End Tag
                    beginTag = false;
                    ptr++;
                    len--;
                }
                else
                    // Start Tag
                    beginTag = true;
                // Accept empty xml tags like <br/>
                if(len > 1 && ptr[len-1] == '/' ) {
                    ptr[--len] = '\0';
                    // if it is like <br/> and not like <input/ value=foo>, take it as flat
                    if (*src == '>')
                        currToken.flat = true;
                }

                uint tagID = 0;
                if (!tagID) {
                    DOMString tagName(ptr);
                    if (Element::khtmlValidQualifiedName(tagName)) {
                        safeLocalName = LocalName::fromString(tagName, IDS_NormalizeLower);
                        tagID = safeLocalName.id();
                    }
#ifdef TOKEN_DEBUG
                    QByteArray tmp(ptr, len+1);
                    qDebug() << "Unknown tag: \"" << tmp.data() << "\"";
#endif
                }
                if (tagID) {
#ifdef TOKEN_DEBUG
                    QByteArray tmp(ptr, len+1);
                    qDebug() << "found tag id=" << tagID << ": " << tmp.data();
#endif
                    currToken.tid = beginTag ? tagID : tagID + ID_CLOSE_TAG;
                }
                dest = buffer;
                tag = SearchAttribute;
                cBufferPos = 0;
            }
            break;
        }
        case SearchAttribute:
        {
#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 1
                qDebug("SearchAttribute");
#endif
            bool atespace = false;
            ushort curchar;
            while(!src.isEmpty()) {
                curchar = src->unicode();
                if(curchar > ' ') {
                    if(curchar == '<' || curchar == '>')
                        tag = SearchEnd;
                    else if(atespace && (curchar == '\'' || curchar == '"'))
                    {
                        tag = SearchValue;
                        *dest++ = 0;
                        attrName = DOMString("");
                    }
                    else
                        tag = AttributeName;

                    cBufferPos = 0;
                    break;
                }
                atespace = true;
                ++src;
            }
            break;
        }
        case AttributeName:
        {
#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 1
                qDebug("AttributeName");
#endif
            ushort curchar;
            int ll = qMin(src.length(), CBUFLEN-cBufferPos);

            while(ll--) {
                curchar = src->unicode();
                if(curchar <= '>') {
                    if(curchar <= ' ' || curchar == '=' || curchar == '>') {
                        unsigned int a;
                        cBuffer[cBufferPos] = '\0';
                        a = LocalName::fromString(DOMString(cBuffer), IDS_NormalizeLower).id(); // ### still deep copy?
                        if (a > ATTR_LAST_ATTR)
                            a = 0;

                        if ( !a ) {
                            // did we just get /> or e.g checked/>
                            if (curchar == '>' && cBufferPos >=1 && cBuffer[cBufferPos-1] == '/') {
                                currToken.flat = true;
                                cBuffer[cBufferPos - 1] = '\0';
                                if (cBufferPos>1)
                                    a = LocalName::fromString(DOMString(cBuffer), IDS_NormalizeLower).id();
                                if (a > ATTR_LAST_ATTR)
                                    a = 0;
                                cBuffer[cBufferPos - 1] = '/';
                            }
                            if (!a)
                                attrName = DOMString(cBuffer, cBufferPos);
                        }

                        dest = buffer;
                        *dest++ = a;
#ifdef TOKEN_DEBUG
                        if (!a || (cBufferPos && *cBuffer == '!'))
                            qDebug() << "Unknown attribute: *" << QByteArray(cBuffer, cBufferPos+1).data() << "*";
                        else
                            qDebug() << "Known attribute: " << QByteArray(cBuffer, cBufferPos+1).data();
#endif

                        tag = SearchEqual;
                        break;
                    }
                }
                cBuffer[cBufferPos++] =
                     (  curchar >= 'A' && curchar <= 'Z' ) ? curchar | 0x20 : curchar;
                ++src;
            }
            if ( cBufferPos == CBUFLEN ) {
                cBuffer[cBufferPos] = '\0';
                attrName = DOMString(cBuffer, cBufferPos);
                dest = buffer;
                *dest++ = 0;
                tag = SearchEqual;
            }
            break;
        }
        case SearchEqual:
        {
#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 1
                qDebug("SearchEqual");
#endif
            ushort curchar;
            bool atespace = false;
            while(!src.isEmpty()) {
                curchar = src->unicode();
                if(curchar > ' ') {
                    if(curchar == '=') {
#ifdef TOKEN_DEBUG
                        qDebug() << "found equal";
#endif
                        tag = SearchValue;
                        ++src;
                    }
                    else if(atespace && (curchar == '\'' || curchar == '"'))
                    {
                        tag = SearchValue;
                        *dest++ = 0;
                        attrName = DOMString("");
                    }
                    else {
                        DOMString v("");
                        currToken.addAttribute(parser->docPtr(), buffer, attrName, v);
                        dest = buffer;
                        tag = SearchAttribute;
                    }
                    break;
                }
                atespace = true;
                ++src;
            }
            break;
        }
        case SearchValue:
        {
            ushort curchar;
            while(!src.isEmpty()) {
                curchar = src->unicode();
                if(curchar > ' ') {
                    if(( curchar == '\'' || curchar == '\"' )) {
                        tquote = curchar == '\"' ? DoubleQuote : SingleQuote;
                        tag = QuotedValue;
                        ++src;
                    } else
                        tag = Value;

                    break;
                }
                ++src;
            }
            break;
        }
        case QuotedValue:
        {
#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 1
                qDebug("QuotedValue");
#endif
            ushort curchar;
            while(!src.isEmpty()) {
                checkBuffer();

                curchar = src->unicode();
                if(curchar <= '\'' && !src.escaped()) {
                    // ### attributes like '&{blaa....};' are supposed to be treated as jscript.
                    if ( curchar == '&' )
                    {
                        ++src;
                        parseEntity(src, dest, true);
                        break;
                    }
                    else if ( (tquote == SingleQuote && curchar == '\'') ||
                              (tquote == DoubleQuote && curchar == '\"') )
                    {
                        // some <input type=hidden> rely on trailing spaces. argh
                        while(dest > buffer+1 && (*(dest-1) == '\n' || *(dest-1) == '\r'))
                            dest--; // remove trailing newlines
                        DOMString v(buffer+1, dest-buffer-1);
                        currToken.addAttribute(parser->docPtr(), buffer, attrName, v);

                        dest = buffer;
                        tag = SearchAttribute;
                        tquote = NoQuote;
                        ++src;
                        break;
                    }
                }
                *dest++ = *src;
                ++src;
            }
            break;
        }
        case Value:
        {
#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 1
            qDebug("Value");
#endif
            ushort curchar;
            while(!src.isEmpty()) {
                checkBuffer();
                curchar = src->unicode();
                if(curchar <= '>' && !src.escaped()) {
                    // parse Entities
                    if ( curchar == '&' )
                    {
                        ++src;
                        parseEntity(src, dest, true);
                        break;
                    }
                    // no quotes. Every space means end of value
                    // '/' does not delimit in IE!
                    // HTML5: must not contain any literal space characters, any U+0022 QUOTATION MARK (") characters,
                    // U+0027 APOSTROPHE (') characters, U+003D EQUALS SIGN (=) characters, U+003C LESS-THAN SIGN (<) characters,
                    // U+003E GREATER-THAN SIGN (>) characters, or U+0060 GRAVE ACCENT (`) characters, and must not be the empty string.
                    // Real life: images.google.com uses URLs including form arguments (foo=bar)
                    // in unquoted parameters --- with an html5 <!doctype html> DTD.
                    // Real life takes priority, so we accept at least =
                    if ( curchar <= ' ' || curchar == '>' || curchar == '\'' || curchar == '"' || curchar == '<' || /*curchar == '=' ||*/ curchar == '`' )
                    {
                        DOMString v(buffer+1, dest-buffer-1);
                        currToken.addAttribute(parser->docPtr(), buffer, attrName, v);
                        dest = buffer;
                        tag = SearchAttribute;
                        break;
                    }
                }

                *dest++ = *src;
                ++src;
            }
            break;
        }
        case SearchEnd:
        {
#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 1
                qDebug("SearchEnd");
#endif
            while(!src.isEmpty()) {
                if(*src == '<' || *src == '>')
                    break;

                if (*src == '/')
                    currToken.flat = true;

                ++src;
            }
            if(src.isEmpty() && *src != '<' && *src != '>') break;

            searchCount = 0; // Stop looking for '<!--' sequence
            tag = NoTag;
            tquote = NoQuote;
            if ( *src == '>' )
                ++src;

            if ( !currToken.tid ) //stop if tag is unknown
                return;

            uint tagID = currToken.tid;
#if defined(TOKEN_DEBUG) && TOKEN_DEBUG > 0
            qDebug() << "appending Tag: " << tagID;
#endif
            // When parsing HTML flat tags like <div /> should 
	    // be ignored, the only exception is SCRIPT, and 
	    // tags with forbidden end-tags
            if (tagID < ID_CLOSE_TAG && tagID != ID_SCRIPT &&
                DOM::endTagRequirement(tagID) != DOM::FORBIDDEN &&
                parser->doc()->htmlMode() != DocumentImpl::XHtml)
                currToken.flat = false;

            bool beginTag = !currToken.flat && (tagID < ID_CLOSE_TAG);
            HTMLScriptElementImpl* prevScriptElem = 0;

            if(tagID >= ID_CLOSE_TAG)
                tagID -= ID_CLOSE_TAG;
            else if ( tagID == ID_SCRIPT ) {
                prevScriptElem = parser->currentScriptElement();
                DOMStringImpl* a = 0;
                scriptSrc.clear(); scriptSrcCharset.clear();
                if ( currToken.attrs && /* potentially have a ATTR_SRC ? */
                     view &&  /* are we a regular tokenizer or just for innerHTML ? */
                     parser->doc()->view()->part()->jScriptEnabled() /* jscript allowed at all? */
                    ) {
                    if ( ( a = currToken.attrs->getValue( ATTR_SRC ) ) )
                        scriptSrc = parser->doc()->completeURL(khtml::parseURL( DOMString(a) ).string() );
                    if ( ( a = currToken.attrs->getValue( ATTR_CHARSET ) ) )
                        scriptSrcCharset = DOMString(a).string().trimmed();
                    if ( scriptSrcCharset.isEmpty() && view)
                        scriptSrcCharset = parser->doc()->view()->part()->encoding();
                }
                javascript = true;
            }

            processToken();

            if (javascript) {
                HTMLScriptElementImpl* sc = parser->currentScriptElement();
                javascript = (sc && sc != prevScriptElem) ? sc->isValidScript() : false;
            }

            if ( parser->selectMode() && beginTag)
                discard = AllDiscard;

            switch( tagID ) {
            case ID_LISTING:
            case ID_PRE:
                pre = beginTag;
                if (beginTag)
                    discard = LFDiscard;
                prePos = 0;
                break;
            case ID_BR:
                prePos = 0;
                break;
            case ID_SCRIPT:
                if (beginTag) {
                    searchStopper = scriptEnd;
                    searchStopperLen = 8;
                    script = true;
                    parseRawContent(src);
                }
                else if (tagID < ID_CLOSE_TAG) { // Handle <script src="foo"/>
                    script = true;
                    scriptHandler();
                }
                break;
            case ID_STYLE:
                if (beginTag) {
                    searchStopper = styleEnd;
                    searchStopperLen = 7;
                    style = true;
                    parseRawContent(src);
                }
                break;
            case ID_TEXTAREA:
                if(beginTag) {
                    searchStopper = textareaEnd;
                    searchStopperLen = 10;
                    textarea = true;
                    discard = NoneDiscard;
                    rawContentSinceLastEntity = 0;
                    parseRawContent(src);
                }
                break;
            case ID_TITLE:
                if (beginTag) {
                    searchStopper = titleEnd;
                    searchStopperLen = 7;
                    title = true;
                    rawContentSinceLastEntity = 0;
                    parseRawContent(src);
                }
                break;
            case ID_XMP:
                if (beginTag) {
                    searchStopper = xmpEnd;
                    searchStopperLen = 5;
                    xmp = true;
                    parseRawContent(src);
                }
                break;
            case ID_SELECT:
                select = beginTag;
                break;
            case ID_PLAINTEXT:
                plaintext = beginTag;
                break;
            }
            return; // Finished parsing tag!
        }
        } // end switch
    }
    return;
}

void HTMLTokenizer::addPending()
{
    if ( select && !(comment || script))
    {
        *dest++ = ' ';
    }
    else
    {
        switch(pending) {
        case LFPending:  *dest++ = QLatin1Char('\n'); prePos = 0; break;
        case SpacePending: *dest++ = QLatin1Char(' '); ++prePos; break;
        case TabPending: {
            // Don't expand tabs inside <textarea> or script
            int p = TAB_SIZE - ( prePos % TAB_SIZE );
            if (textarea || script) {
                *dest++ = QLatin1Char('\t');
            } else {
                for ( int x = 0; x < p; x++ )
                    *dest++ = QLatin1Char(' ');
            }
            prePos += p;
            break;
        }
        case NonePending:
            assert(0);
        }
    }

    pending = NonePending;
}

inline bool HTMLTokenizer::continueProcessing(int& processedCount)
{
    // We don't want to be checking elapsed time with every character, so we only check after we've
    // processed a certain number of characters. We also do not do suspension if we're
    // parsing something like innerHTML.
    if (!m_executingScript && processedCount > sTokenizerChunkSize && cachedScript.isEmpty()) {
        processedCount = 0;
        if ( m_time.elapsed() > m_tokenizerYieldDelay && m_documentTokenizer) {
            m_yieldTimer = startTimer(0);
            m_tokenizerYieldDelay = sTokenizerFastYieldDelay;
            return false;
        }
    }
    processedCount++;
    return true;
}


#include "khtmlpart_p.h"
void HTMLTokenizer::write( const TokenizerString &str, bool appendData )
{
#ifdef TOKEN_DEBUG
    qDebug() << this << " Tokenizer::write(\"" << str.toString() << "\"," << appendData << ")";
#endif
    if ( !buffer )
        return;

    if ( ( m_executingScript && appendData ) || cachedScript.count() ) {
        // don't parse; we will do this later
        if (pendingQueue.isEmpty())
            pendingQueue.push(str);
        else if (appendData)
            pendingQueue.bottom().append(str);
        else
            pendingQueue.top().append(str);
#if PROSPECTIVE_TOKENIZER_ENABLED
        if (m_prospectiveTokenizer && m_prospectiveTokenizer->inProgress() && appendData)
            m_prospectiveTokenizer->write(str);
#endif
        return;
    }

#if PROSPECTIVE_TOKENIZER_ENABLED
    if (m_prospectiveTokenizer && m_prospectiveTokenizer->inProgress() && appendData)
        m_prospectiveTokenizer->end();
#endif

    if ( onHold ) {
        src.append(str);
        return;
    }

    if (!src.isEmpty())
        src.append(str);
    else
        setSrc(str);

    // Once a timer is set, it has control of when the tokenizer continues.
    if (m_yieldTimer > 0)
        return;

    int processedCount = 0;
    m_time.start();

    while ( !src.isEmpty() )
    {
        if ( m_abort || !continueProcessing(processedCount) )
            break;
        // do we need to enlarge the buffer?
        checkBuffer();

        ushort cc = src->unicode();

        if (skipLF && (cc != '\n'))
            skipLF = false;

        if (skipLF) {
            skipLF = false;
            ++src;
        }
        else if ( Entity )
            parseEntity( src, dest );
        else if ( plaintext )
            parseText( src );
        else if (script)
            parseRawContent(src);
        else if (style)
            parseRawContent(src);
        else if (xmp)
            parseRawContent(src);
        else if (textarea)
            parseRawContent(src);
        else if (title)
            parseRawContent(src);
        else if (comment)
            parseComment(src);
        else if (doctypeComment && doctypeComment != DoctypeCommentEnd && doctypeComment != DoctypeCommentBogus)
            parseDoctypeComment(src);
        else if (doctype)
            parseDoctype(src);
        else if (server)
            parseServer(src);
        else if (processingInstruction)
            parseProcessingInstruction(src);
        else if (tag)
            parseTag(src);
        else if ( startTag )
        {
            startTag = false;

            switch(cc) {
            case '/':
                break;
            case '!':
            {
                // <!-- comment --> or <!DOCTYPE ...>
                searchCount = 1; // Look for '<!--' sequence to start comment...
                doctypeSearchCount = 1; // ... or for '<!DOCTYPE' sequence to start doctype
                break;
            }
            case '?':
            {
                // xml processing instruction
                processingInstruction = true;
                tquote = NoQuote;
                parseProcessingInstruction(src);
                continue;
            }
            case '%':
                if (!brokenServer) {
                    // <% server stuff, handle as comment %>
                    server = true;
                    tquote = NoQuote;
                    parseServer(src);
                    continue;
                }
                // else fall through
            default:
            {
                if( ((cc >= 'a') && (cc <= 'z')) || ((cc >= 'A') && (cc <= 'Z')))
                {
                    // Start of a Start-Tag
                }
                else
                {
                    // Invalid tag
                    // Add as is
                    if (pending)
                        addPending();
                    *dest = '<';
                    dest++;
                    continue;
                }
            }
            }; // end case

            // According to SGML any LF immediately after a starttag, or
            // immediately before an endtag should be ignored.
            // ### Gecko and MSIE though only ignores LF immediately after
            // starttags and only for PRE elements -- asj (28/06-2005)
            if ( pending )
            {
                if (!select)
                {
                    addPending();
                }
                else
                {
                    pending = NonePending;
                }
            }

            // Cancel unused discards
            discard = NoneDiscard;
            // if (!endTag) discard = LFDiscard;

            processToken();

            cBufferPos = 0;
            tag = TagName;
            parseTag(src);
        }
        else if ( cc == '&' && !src.escaped())
        {
            ++src;
            if ( pending )
                addPending();
            discard = NoneDiscard;
            parseEntity(src, dest, true);
        }
        else if ( cc == '<' && !src.escaped())
        {
            tagStartLineno = lineno+src.lineCount();
            ++src;
            discard = NoneDiscard;
            startTag = true;
        }
        else if (( cc == '\n' ) || ( cc == '\r' ))
        {
            if (discard == SpaceDiscard)
                discard = NoneDiscard;

            if (discard == LFDiscard) {
                // Ignore one LF
                discard = NoneDiscard;
            }
            else if (discard == AllDiscard)
            {
                // Ignore
            }
            else
            {
                if (select && !script) {
                    pending = LFPending;
                } else {
                    if (pending)
                        addPending();
                    pending = LFPending;
                }
            }

            /* Check for MS-DOS CRLF sequence */
            if (cc == '\r')
            {
                skipLF = true;
            }
            ++src;
        }
        else if (( cc == ' ' ) || ( cc == '\t' ))
        {
            if(discard == LFDiscard)
                discard = NoneDiscard;

            if(discard == SpaceDiscard) {
                // Ignore one space
                discard = NoneDiscard;
            }
            else if(discard == AllDiscard)
            {
                // Ignore
            }
            else {
                if (select && !script) {
                    if (!pending)
                        pending = SpacePending;
                } else {
                    if (pending)
                        addPending();
                    if (cc == ' ')
                        pending = SpacePending;
                    else
                        pending = TabPending;
                }
            }

            ++src;
        }
        else
        {
            if (pending)
                addPending();

            discard = NoneDiscard;
            if ( pre )
            {
                prePos++;
            }
            *dest = *src;
            fixUpChar( *dest );
            ++dest;
            ++src;
        }
    }

    if (noMoreData && cachedScript.isEmpty() && !m_executingScript && m_yieldTimer<=0)
        end(); // this actually causes us to be deleted
}

void HTMLTokenizer::timerEvent( QTimerEvent *e )
{
    if ( e->timerId() == m_yieldTimer ) {
        killTimer(m_yieldTimer);
        m_yieldTimer = 0;
        write( TokenizerString(), true );
    } else if ( e->timerId() == m_externalScriptsTimerId ) {
        if (view && view->hasLayoutPending()) {
            // all stylesheets are loaded but the style modifications 
            // they triggered have yet to be applied, BBIAB
            return;
        }
        killTimer(m_externalScriptsTimerId);
        m_externalScriptsTimerId = 0;
        notifyFinished(0);
    }
}

void HTMLTokenizer::end()
{
    if ( buffer ) {
        // parseTag is using the buffer for different matters
        if ( !tag )
            processToken();

        if(buffer)
            KHTML_DELETE_QCHAR_VEC(buffer);

        if(rawContent)
            KHTML_DELETE_QCHAR_VEC(rawContent);

        rawContent = 0;
        rawContentSize = rawContentMaxSize = rawContentResync = 0;
        buffer = 0;
    }
    emit finishedParsing();
}

void HTMLTokenizer::finish()
{
    // The purpose of this iteration is to recover from 'raw content' tokenizing mode.
    // In this mode, any error such as the lack of a closing tag (for the considered element) or of a closing comment,
    // would result in the entire document being absorbed in one node.
    // When it happens, we simply put back in the input buffer what this mode's output has accumulated so far,
    // and retokenize after either disabling the 'raw content' mode (by setting the corresponding members to false)
    // or after setting a few flags disabling some lax parsing 'features' (brokenComments/brokenServer).
    while((title || comment || server) && rawContent && rawContentSize)
    {
        // we've found an unmatched comment start
        if (comment)
            brokenComments = true;
        else if (server)
            brokenServer = true;

        checkRawContentBuffer();
        rawContent[ rawContentSize ] = 0;
        rawContent[ rawContentSize + 1 ] = 0;
        int pos;
        QString food;
        if (title || style || script || textarea) {
            rawContentSinceLastEntity = 0;
            food.setUnicode(rawContent, rawContentSize);
        } else if (server) {
            food = "<";
            food += QString(rawContent, rawContentSize);
        }
        else {
            pos = QString::fromRawData(rawContent, rawContentSize).indexOf('>');
            food.setUnicode(rawContent+pos+1, rawContentSize-pos-1); // deep copy
        }
        KHTML_DELETE_QCHAR_VEC(rawContent);
        rawContent = 0;
        rawContentSize = rawContentMaxSize = rawContentResync = 0;

        comment = server = title = false;
        if ( !food.isEmpty() )
            write(food, true);
    }
    // this indicates we will not receive any more data... but if we are waiting on
    // an external script to load, we can't finish parsing until that is done
    noMoreData = true;
    if (cachedScript.isEmpty() && !m_executingScript && !onHold && m_yieldTimer <= 0)
        end(); // this actually causes us to be deleted
}

void HTMLTokenizer::processToken()
{
    KJSProxy *jsProxy = view ? view->part()->jScript() : 0L;
    if (jsProxy)
        jsProxy->setEventHandlerLineno(tagStartLineno);
    if ( dest > buffer )
    {
#if 0
        if(currToken.tid) {
            qDebug( "unexpected token id: %d, str: *%s*", currToken.tid,QString::fromRawData( buffer, dest-buffer ).toLatin1().constData() );
            assert(0);
        }

#endif
        currToken.text = new DOMStringImpl( buffer, dest - buffer );
        currToken.text->ref();
        if (currToken.tid != ID_COMMENT)
            currToken.tid = ID_TEXT;
    }
    else if(!currToken.tid) {
        currToken.reset();
        if (jsProxy)
            jsProxy->setEventHandlerLineno(lineno+src.lineCount());
        return;
    }

    dest = buffer;

#ifdef TOKEN_DEBUG
    QString text;
    bool closing = (currToken.tid > ID_CLOSE_TAG);
    int rid = currToken.tid-(closing ? ID_CLOSE_TAG : 0);
    if(currToken.text)
        text = QString::fromRawData(currToken.text->s, currToken.text->l);
    qDebug() << "Token -->" << LocalName::fromId(localNamePart(rid)).toString()
                   << "id =" << currToken.tid << "closing ="<< closing;
    if (currToken.flat)
        qDebug() << "Token is FLAT!";
    if(!text.isNull())
        qDebug() << "text: \"" << text << "\"";
    unsigned long l = currToken.attrs ? currToken.attrs->length() : 0;
 
    if(l) {
        qDebug() << "Attributes: " << l;
        for (unsigned long i = 0; i < l; ++i) {
            NodeImpl::Id tid = currToken.attrs->idAt(i);
            DOMString value = currToken.attrs->valueAt(i);
            qDebug() << "    " << tid << " " << LocalName::fromId(localNamePart(tid)).toString()
                            << "=\"" << value.string() << "\"" << endl;
        }
    }
#endif

    // In some cases, parseToken() can cause javascript code to be executed
    // (for example, when setting an attribute that causes an event handler
    // to be created). So we need to protect against re-entrancy into the parser
    m_executingScript++;

    // pass the token over to the parser, the parser DOES NOT delete the token
    parser->parseToken(&currToken);

    m_executingScript--;

    if ( currToken.flat && currToken.tid != ID_TEXT && !parser->noSpaces() )
	discard = NoneDiscard;

    currToken.reset();
    if (jsProxy)
        jsProxy->setEventHandlerLineno(0);
}

void HTMLTokenizer::processDoctypeToken()
{
    // qDebug() << "Process DoctypeToken (name: " << doctypeToken.name << ", publicID: " << doctypeToken.publicID << ", systemID: " << doctypeToken.systemID;
    doctypeToken.publicID = doctypeToken.publicID.simplified();
    doctypeToken.systemID = doctypeToken.systemID.simplified();
    parser->parseDoctypeToken(&doctypeToken);
}


HTMLTokenizer::~HTMLTokenizer()
{
    reset();
    delete m_prospectiveTokenizer;
    delete parser;
}


void HTMLTokenizer::enlargeBuffer(int len)
{
    int newsize = qMax(size*2, size+len);
    int oldoffs = (dest - buffer);

    buffer = KHTML_REALLOC_QCHAR_VEC(buffer, newsize);
    dest = buffer + oldoffs;
    size = newsize;
}

void HTMLTokenizer::enlargeRawContentBuffer(int len)
{
    int newsize = qMax(rawContentMaxSize*2, rawContentMaxSize+len);
    rawContent = KHTML_REALLOC_QCHAR_VEC(rawContent, newsize);
    rawContentMaxSize = newsize;
}

void HTMLTokenizer::notifyFinished(CachedObject* finishedObj)
{
    Q_UNUSED(finishedObj);
    assert(!cachedScript.isEmpty());
    // Make external scripts wait for external stylesheets.
    // FIXME: This needs to be done for inline scripts too.
    m_hasScriptsWaitingForStylesheets = !parser->doc()->haveStylesheetsLoaded();
    if (m_hasScriptsWaitingForStylesheets) {
        // qDebug() << "Delaying script execution until stylesheets have loaded.";
        return;
    }
    // qDebug() << (finishedObj ? "Processing an external script"  : "Continuing processing of delayed external scripts");

    bool done = false;
    m_scriptTime.start();
    while (!done && cachedScript.head()->isLoaded()) {
        if (!continueProcessingScripts())
            break;

        CachedScript* cs = cachedScript.dequeue();
        DOMString scriptSource = cs->script();
#ifdef TOKEN_DEBUG
        qDebug() << "External script is:" << endl << scriptSource.string();
#endif
        setSrc(TokenizerString());

        // make sure we forget about the script before we execute the new one
        // infinite recursion might happen otherwise
        QString cachedScriptUrl( cs->url().string() );
        cs->deref(this);

	scriptExecution( scriptSource.string(), cachedScriptUrl );

        done = cachedScript.isEmpty();
        if (done) {
            assert(!m_hasScriptsWaitingForStylesheets);
        } else if (m_hasScriptsWaitingForStylesheets) {
            // flag has changed during the script execution,
            // so we need to wait for stylesheets again.
             done = true;
        }
        // 'script' is true when we are called synchronously from
        // scriptHandler(). In that case scriptHandler() will take care
        // of the pending queue.
        if ( !script ) {
            while (pendingQueue.count() > 1) {
               TokenizerString t = pendingQueue.pop();
               pendingQueue.top().prepend( t );
            }
            if (done) {
                write(pendingQueue.pop(), false);
            }
            // we might be deleted at this point, do not
            // access any members.
        }
    }
}

bool HTMLTokenizer::continueProcessingScripts()
{
    if (m_externalScriptsTimerId)
        return false;
    if (m_scriptTime.elapsed() > m_tokenizerYieldDelay && m_documentTokenizer) {
        if ( (m_externalScriptsTimerId = startTimer(0)) )
            return false;
    }
    return true;
}

void HTMLTokenizer::executeScriptsWaitingForStylesheets()
{
    assert(parser->doc()->haveStylesheetsLoaded());
    if (m_hasScriptsWaitingForStylesheets)
        notifyFinished(0);
}

bool HTMLTokenizer::isWaitingForScripts() const
{
    return cachedScript.count();
}

bool HTMLTokenizer::isExecutingScript() const
{
    return (m_executingScript > 0);
}

void HTMLTokenizer::setSrc(const TokenizerString& source)
{
    lineno += src.lineCount();
    src = source;
    src.resetLineCount();
}

void HTMLTokenizer::setOnHold(bool _onHold)
{
    if (onHold == _onHold) return;
    onHold = _onHold;
}

