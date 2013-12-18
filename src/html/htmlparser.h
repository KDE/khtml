/*
    This file is part of the KDE libraries

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
              (C) 1998 Waldo Bastian (bastian@kde.org)
              (C) 1999 Lars Knoll (knoll@kde.org)
              (C) 2003 Apple Computer, Inc.

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
// KDE HTML Widget -- HTML Parser

#ifndef HTMLPARSER_H
#define HTMLPARSER_H

// 0 all
// 1 domtree + rendertree + styleForElement, no layouting
// 2 domtree only
#define SPEED_DEBUG 0

#ifdef SPEED_DEBUG
#include <QDateTime>
#endif


#include "dom/dom_string.h"
#include "xml/dom_nodeimpl.h"
#include "html/html_documentimpl.h"
#include "html/html_headimpl.h"

class KHTMLView;
class HTMLStackElem;

namespace DOM {
    class HTMLDocumentImpl;
    class DocumentImpl;
    class NodeImpl;
    class HTMLFormElementImpl;
    class HTMLMapElementImpl;
    class HTMLHeadElementImpl;
    class DocumentFragmentImpl;
}

namespace khtml {

class Token;
class DoctypeToken;

/**
 * The parser for html. It receives a stream of tokens from the HTMLTokenizer, and
 * builds up the Document structure form it.
 */
class KHTMLParser
{
public:
    KHTMLParser( KHTMLView *w, DOM::DocumentImpl *i );
    KHTMLParser( DOM::DocumentFragmentImpl *frag, DOM::DocumentImpl *doc );
    virtual ~KHTMLParser();

    /**
     * parses one token delivered by the tokenizer
     */
    void parseToken(Token *_t);
    
    /**
     * parses a doctype token delivered by the tokenizer
     */
    void parseDoctypeToken(DoctypeToken *_t);

    /**
     * resets the parser
     */
    void reset();

    bool skipMode() const { return (discard_until != 0); }
    bool noSpaces() const { return (inSelect || !m_inline  || !inBody); }
    bool selectMode() const { return inSelect; }

    DOM::HTMLDocumentImpl *doc() const { return static_cast<DOM::HTMLDocumentImpl *>(document); }
    DOM::DocumentImpl *docPtr() const { return document; }

    DOM::HTMLScriptElementImpl *currentScriptElement() const { return (current && current->id() == ID_SCRIPT) ? static_cast<DOM::HTMLScriptElementImpl*>(current) : 0;}

protected:

    KHTMLView *HTMLWidget;
    DOM::DocumentImpl *document;

    /*
     * generate an element from the token
     */
    DOM::NodeImpl *getElement(Token *);

    void processCloseTag(Token *);

    bool insertNode(DOM::NodeImpl *n, bool flat = false);

    /*
     * The currently active element (the one new elements will be added to)
     */
    void setCurrent( DOM::NodeImpl* newNode )
    {
        if ( newNode ) newNode->ref();
        if ( current ) current->deref();
        current = newNode;
    }

private:
    DOM::NodeImpl *current;

    HTMLStackElem *blockStack;

    void pushBlock( int _id, int _level);

    void generateImpliedEndTags( int _id );
    void popOptionalBlock( int _id );
    void popBlock( int _id );
    void popOneBlock(bool delBlock = true);
    void popInlineBlocks();
    bool isElementInScope( int _id );
    bool isHeadingInScope();

    void freeBlock( void);

    void createHead();

    bool isResidualStyleTag(int _id);
    bool isAffectedByResidualStyle(int _id);
    void handleResidualStyleCloseTagAcrossBlocks(HTMLStackElem* elem);
    void reopenResidualStyleTags(HTMLStackElem* elem, DOM::NodeImpl* malformedTableParent);

    ushort *forbiddenTag;

    /*
     * currently active form
     */
    DOM::HTMLFormElementImpl *form;

    /*
     * current map
     */
    DOM::HTMLMapElementImpl *map;

    /*
     * the head element. Needed for crappy html which defines <base> after </head>
     */
    RefPtr<DOM::HTMLHeadElementImpl> head;

    /*
     * a possible <isindex> element in the head. Compatibility hack for
     * html from the stone age
     */
    DOM::NodeImpl *isindex;
    DOM::NodeImpl *handleIsindex( Token *t );

    /*
     * inserts the stupid isIndex element.
     */
    void startBody();

    bool inBody;
    bool haveContent;
    bool haveBody;
    bool haveFrameSet;
    bool haveTitle;
    bool m_inline;
    bool end;
    bool inSelect;


    /*
     * tells the parser to discard all tags, until it reaches the one specified
     */
    int discard_until;

    bool headLoaded;
    int inStrayTableContent;

#if SPEED_DEBUG > 0
    QTime qt;
#endif
};

} // namespace khtml

#endif // HTMLPARSER_H

