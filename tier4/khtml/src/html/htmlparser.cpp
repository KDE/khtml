/*
    This file is part of the KDE libraries

    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
              (C) 1999,2001 Lars Knoll (knoll@kde.org)
              (C) 2000,2001 Dirk Mueller (mueller@kde.org)
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
// #define PARSER_DEBUG

#include "htmlparser.h"

#include <dom/dom_exception.h>

#include <html/html_baseimpl.h>
#include <html/html_blockimpl.h>
#include <html/html_canvasimpl.h>
#include <html/html_documentimpl.h>
#include <html/html_elementimpl.h>
#include <html/html_formimpl.h>
#include <html/html_headimpl.h>
#include <html/html_imageimpl.h>
#include <html/html_inlineimpl.h>
#include <html/html_listimpl.h>
#include <html/html_miscimpl.h>
#include <html/html_tableimpl.h>
#include <html/html_objectimpl.h>
#include <html/HTMLAudioElement.h>
#include <html/HTMLVideoElement.h>
#include <html/HTMLSourceElement.h>
#include <xml/dom_textimpl.h>
#include <xml/dom_nodeimpl.h>
#include <html/htmltokenizer.h>
#include <khtmlview.h>
#include <khtml_part.h>
#include <khtml_global.h>
#include <css/cssproperties.h>
#include <css/cssvalues.h>
#include <css/csshelper.h>

#include <rendering/render_object.h>

#include <QDebug>
#include <klocalizedstring.h>

// Turn off gnu90 inlining to avoid linker errors
#undef __GNUC_STDC_INLINE__
#undef __GNUC_GNU_INLINE__
#include "doctypes.cpp"

using namespace DOM;
using namespace khtml;

#ifdef PARSER_DEBUG
static QString getParserPrintableName(int id)
{
    if (id >= ID_CLOSE_TAG)
	return "/" + getPrintableName(id - ID_CLOSE_TAG);
    else
	return getPrintableName(id);
}
#endif

//----------------------------------------------------------------------------

/**
 * @internal
 */
class HTMLStackElem
{
public:
    HTMLStackElem( int _id,
                   int _level,
                   DOM::NodeImpl *_node,
                   bool _inline_,
                   HTMLStackElem * _next )
        :
        id(_id),
        level(_level),
        strayTableContent(false),
        m_inline(_inline_),
        node(_node),
        next(_next)
        { node->ref(); }

    ~HTMLStackElem()
        { node->deref(); }

    void setNode(NodeImpl* newNode)
    {
        newNode->ref();
        node->deref();
        node = newNode;
    }

    int       id;
    int       level;
    bool      strayTableContent;
    bool m_inline;
    NodeImpl *node;
    HTMLStackElem *next;
};

/**
 * @internal
 *
 * The parser parses tokenized input into the document, building up the
 * document tree. If the document is wellformed, parsing it is
 * straightforward.
 * Unfortunately, people can't write wellformed HTML documents, so the parser
 * has to be tolerant about errors.
 *
 * We have to take care of the following error conditions:
 * 1. The element being added is explicitly forbidden inside some outer tag.
 *    In this case we should close all tags up to the one, which forbids
 *    the element, and add it afterwards.
 * 2. We are not allowed to add the element directly. It could be, that
 *    the person writing the document forgot some tag inbetween (or that the
 *    tag inbetween is optional...) This could be the case with the following
 *    tags: HTML HEAD BODY TBODY TR TD LI (did I forget any?)
 * 3. We wan't to add a block element inside to an inline element. Close all
 *    inline elements up to the next higher block element.
 * 4. If this doesn't help close elements, until we are allowed to add the
 *    element or ignore the tag.
 *
 */

KHTMLParser::KHTMLParser( KHTMLView *_parent, DocumentImpl *doc)
{
    //qDebug() << "parser constructor";
#if SPEED_DEBUG > 0
    qt.start();
#endif

    HTMLWidget    = _parent;
    document      = doc;

    blockStack = 0;
    current = 0;

    // ID_CLOSE_TAG == Num of tags
    forbiddenTag = new ushort[ID_CLOSE_TAG+1];

    reset();
}

KHTMLParser::KHTMLParser( DOM::DocumentFragmentImpl *i, DocumentImpl *doc )
{
    HTMLWidget = 0;
    document = doc;

    forbiddenTag = new ushort[ID_CLOSE_TAG+1];

    blockStack = 0;
    current = 0;

    reset();

    setCurrent(i);

    inBody = true;
}

KHTMLParser::~KHTMLParser()
{
#if SPEED_DEBUG > 0
    qDebug() << "TIME: parsing time was = " << qt.elapsed();
#endif

    freeBlock();

    if (current) current->deref();

    delete [] forbiddenTag;
    delete isindex;
}

void KHTMLParser::reset()
{
    setCurrent ( document );

    freeBlock();

    // before parsing no tags are forbidden...
    memset(forbiddenTag, 0, (ID_CLOSE_TAG+1)*sizeof(ushort));

    inBody = false;
    haveFrameSet = false;
    haveContent = false;
    haveBody = false;
    haveTitle = false;
    inSelect = false;
    inStrayTableContent = 0;
    m_inline = false;

    form = 0;
    map = 0;
    end = false;
    isindex = 0;

    discard_until = 0;
}

void KHTMLParser::parseToken(Token *t)
{
    if (t->tid > 2*ID_CLOSE_TAG)
    {
      // qDebug() << "Unknown tag!! tagID = " << t->tid;
      return;
    }
    if(discard_until) {
        if(t->tid == discard_until)
            discard_until = 0;

        // do not skip </iframe>
        if ( discard_until || current->id() + ID_CLOSE_TAG != t->tid )
            return;
    }

#ifdef PARSER_DEBUG
    qDebug() << "\n\n==> parser: processing token " << getParserPrintableName(t->tid) << "(" << t->tid << ")"
                    << " current = " << getParserPrintableName(current->id()) << "(" << current->id() << ")" << endl;
    qDebug() << "inline=" << m_inline << " inBody=" << inBody << " haveFrameSet=" << haveFrameSet << " haveContent=" << haveContent;
#endif

    // holy shit. apparently some sites use </br> instead of <br>
    // be compatible with IE and NS
    if(t->tid == ID_BR+ID_CLOSE_TAG && document->inCompatMode())
        t->tid -= ID_CLOSE_TAG;

    if(t->tid > ID_CLOSE_TAG)
    {
        processCloseTag(t);
        return;
    }

    // ignore spaces, if we're not inside a paragraph or other inline code
    if( t->tid == ID_TEXT && t->text ) {
        if(inBody && !skipMode() &&
           current->id() != ID_STYLE && current->id() != ID_TITLE &&
           current->id() != ID_SCRIPT &&
           !t->text->containsOnlyWhitespace()) haveContent = true;
#ifdef PARSER_DEBUG

        qDebug() << "length="<< t->text->l << " text='" << QString::fromRawData(t->text->s, t->text->l) << "'";
#endif
    }

    NodeImpl *n = getElement(t);
    // just to be sure, and to catch currently unimplemented stuff
    if(!n)
        return;

    // set attributes
    if(n->isElementNode() && t->tid != ID_ISINDEX)
    {
        ElementImpl *e = static_cast<ElementImpl *>(n);
        e->setAttributeMap(t->attrs);
    }

    // if this tag is forbidden inside the current context, pop
    // blocks until we are allowed to add it...
    while(blockStack && forbiddenTag[t->tid]) {
#ifdef PARSER_DEBUG
        qDebug() << "t->id: " << t->tid << " is forbidden :-( ";
#endif
        popOneBlock();
    }

    // sometimes flat doesn't make sense
    switch(t->tid) {
    case ID_SELECT:
    case ID_OPTION:
        t->flat = false;
    }

    // the tokenizer needs the feedback for space discarding
    if ( tagPriority(t->tid) == 0 )
	t->flat = true;

    if ( !insertNode(n, t->flat) ) {
        // we couldn't insert the node...
#ifdef PARSER_DEBUG
        qDebug() << "insertNode failed current=" << current->id() << ", new=" << n->id() << "!";
#endif
        if (map == n)
        {
#ifdef PARSER_DEBUG
            qDebug() << "  --> resetting map!";
#endif
            map = 0;
        }
        if (form == n)
        {
#ifdef PARSER_DEBUG
            qDebug() << "   --> resetting form!";
#endif
            form = 0;
        }
        delete n;
    }
}

void KHTMLParser::parseDoctypeToken(DoctypeToken* t)
{
    // Ignore any doctype after the first. TODO It should be also ignored when processing DocumentFragment
    if (current != document || document->doctype())
        return;

    DocumentTypeImpl* doctype = new DocumentTypeImpl(document->implementation(), document, t->name, t->publicID, t->systemID);
    if (!t->internalSubset.isEmpty())
        doctype->setInternalSubset(t->internalSubset);
    document->addChild(doctype);

    // Determine parse mode here
    // This code more or less mimics Mozilla's implementation.
    //
    // There are three possible parse modes:
    // COMPAT - quirks mode emulates WinIE
    // and NS4.  CSS parsing is also relaxed in this mode, e.g., unit types can
    // be omitted from numbers.
    // ALMOST STRICT - This mode is identical to strict mode
    // except for its treatment of line-height in the inline box model.  For
    // now (until the inline box model is re-written), this mode is identical
    // to STANDARDS mode.
    // STRICT - no quirks apply.  Web pages will obey the specifications to
    // the letter.

    if (!document->isHTMLDocument()) // FIXME Could document be non-HTML?
        return;
    DOM::HTMLDocumentImpl* htmldoc = static_cast<DOM::HTMLDocumentImpl*> (document);
    if (t->name.toLower() == "html") {
        if (!t->internalSubset.isEmpty() || t->publicID.isEmpty()) {
            // Internal subsets always denote full standards, as does
            // a doctype without a public ID.
            htmldoc->changeModes(DOM::DocumentImpl::Strict, DOM::DocumentImpl::Html4);
        } else {
            // We have to check a list of public IDs to see what we
            // should do.
            QString lowerPubID = t->publicID.toLower();
            QByteArray pubIDStr = lowerPubID.toLocal8Bit();

            // Look up the entry in our gperf-generated table.
            const PubIDInfo* doctypeEntry = findDoctypeEntry(pubIDStr.constData(), t->publicID.length());
            if (!doctypeEntry) {
                // The DOCTYPE is not in the list.  Assume strict mode.
                // ### Doesn't make any sense, but it's what Mozilla does.
                htmldoc->changeModes(DOM::DocumentImpl::Strict, DOM::DocumentImpl::Html4);
            } else {
                switch ((!t->systemID.isEmpty()) ?
                            doctypeEntry->mode_if_sysid :
                            doctypeEntry->mode_if_no_sysid) {
                    case PubIDInfo::eQuirks3:
                        htmldoc->changeModes(DOM::DocumentImpl::Compat, DOM::DocumentImpl::Html3);
                        break;
                    case PubIDInfo::eQuirks:
                        htmldoc->changeModes(DOM::DocumentImpl::Compat, DOM::DocumentImpl::Html4);
                        break;
                    case PubIDInfo::eAlmostStandards:
                        htmldoc->changeModes(DOM::DocumentImpl::Transitional, DOM::DocumentImpl::Html4);
                        break;
                    default:
                        assert(!"Unknown parse mode");
                }
            }
        }
    } else {
        // Malformed doctype implies quirks mode.
        htmldoc->changeModes(DOM::DocumentImpl::Compat, DOM::DocumentImpl::Html3);
    }
}

static bool isTableRelatedTag(int id)
{
    return (id == ID_TR || id == ID_TD || id == ID_TABLE || id == ID_TBODY || id == ID_TFOOT || id == ID_THEAD ||
            id == ID_TH);
}

bool KHTMLParser::insertNode(NodeImpl *n, bool flat)
{
    int id = n->id();

    // <table> is never allowed inside stray table content.  Always pop out of the stray table content
    // and close up the first table, and then start the second table as a sibling.
    if (inStrayTableContent && id == ID_TABLE)
        popBlock(ID_TABLE);

    // let's be stupid and just try to insert it.
    // this should work if the document is wellformed
#ifdef PARSER_DEBUG
    NodeImpl *tmp = current;
#endif
    NodeImpl *newNode = current->addChild(n);
    if ( newNode ) {
#ifdef PARSER_DEBUG
        qDebug() << "added " << n->nodeName().string() << " to " << tmp->nodeName().string() << ", new current=" << newNode->nodeName().string();
#endif
        // We allow TABLE > FORM in dtd.cpp, but do not allow the form have children in this case
        if (current->id() == ID_TABLE && id == ID_FORM) {
            flat = true;
            static_cast<HTMLFormElementImpl*>(n)->setMalformed(true);
        }

	// don't push elements without end tag on the stack
        if(tagPriority(id) != 0 && !flat) {
#if SPEED_DEBUG < 2
            if(!n->attached() && HTMLWidget )
                n->attach();
#endif
	    if(n->isInline()) m_inline = true;
            pushBlock(id, tagPriority(id));
            setCurrent( newNode );
        } else {
#if SPEED_DEBUG < 2
            if(!n->attached() && HTMLWidget)
                n->attach();
            if (n->maintainsState()) {
                document->registerMaintainsState(n);
                document->attemptRestoreState(n);
            }
            n->close();
#endif
	    if(n->isInline()) m_inline = true;
        }


#if SPEED_DEBUG < 1
        if(tagPriority(id) == 0 && n->renderer())
            n->renderer()->calcMinMaxWidth();
#endif
        return true;
    } else {
#ifdef PARSER_DEBUG
        qDebug() << "ADDING NODE FAILED!!!! current = " << current->nodeName().string() << ", new = " << n->nodeName().string();
#endif
        // error handling...
        HTMLElementImpl *e;
        bool handled = false;

        // first switch on current element for elements with optional end-tag and inline-only content
        switch(current->id())
        {
        case ID_P:
        case ID_DT:
            if(!n->isInline())
            {
                popBlock(current->id());
                return insertNode(n);
            }
            break;
        case ID_TITLE:
            popBlock(current->id());
            return insertNode(n);
        default:
            break;
        }

        // switch according to the element to insert
        switch(id)
        {
        case ID_TR:
        case ID_TH:
        case ID_TD:
            if (inStrayTableContent && !isTableRelatedTag(current->id())) {
                // pop out to the nearest enclosing table-related tag.
                while (blockStack && !isTableRelatedTag(current->id()))
                    popOneBlock();
                return insertNode(n);
            }
            break;
        case ID_HEAD:
            // ### allow not having <HTML> in at all, as per HTML spec
            if (!current->isDocumentNode() && current->id() != ID_HTML )
                return false;
            break;
        case ID_COMMENT:
            if( head )
                break;
        case ID_META:
        case ID_LINK:
        case ID_ISINDEX:
        case ID_BASE:
            if( !head )
                createHead();
            if( head ) {
                if ( head->addChild(n) ) {
#if SPEED_DEBUG < 2
                    if(!n->attached() && HTMLWidget)
                        n->attach();
#endif
                }

                return true;
            }

            break;
        case ID_HTML:
            if (!current->isDocumentNode() ) {
		if ( doc()->documentElement()->id() == ID_HTML) {
		    // we have another <HTML> element.... apply attributes to existing one
		    // make sure we don't overwrite already existing attributes
		    NamedAttrMapImpl *map = static_cast<ElementImpl*>(n)->attributes(true);
		    NamedAttrMapImpl *bmap = static_cast<ElementImpl*>(doc()->documentElement())->attributes(false);
		    bool changed = false;
		    for (unsigned long l = 0; map && l < map->length(); ++l) {
			NodeImpl::Id attrId = map->idAt(l);
			DOMStringImpl *attrValue = map->valueAt(l);
			changed = !bmap->getValue(attrId);
			bmap->setValue(attrId,attrValue);
		    }
		    if ( changed )
			doc()->recalcStyle( NodeImpl::Inherit );
		}
                return false;
	    }
            break;
        case ID_TITLE:
        case ID_STYLE:
            if ( !head )
                createHead();
            if ( head ) {
                DOM::NodeImpl *newNode = head->addChild(n);
                if ( newNode ) {
                    pushBlock(id, tagPriority(id));
                    setCurrent ( newNode );
#if SPEED_DEBUG < 2
		    if(!n->attached() && HTMLWidget)
                        n->attach();
#endif
                } else {
#ifdef PARSER_DEBUG
                    qDebug() << "adding style before to body failed!!!!";
#endif
                    discard_until = ID_STYLE + ID_CLOSE_TAG;
                    return false;
                }
                return true;
            } else if(inBody) {
                discard_until = id + ID_CLOSE_TAG;
                return false;
            }
            break;
        case ID_SCRIPT:
            // if we failed to insert it, go into skip mode
            discard_until = id + ID_CLOSE_TAG;
            break;
        case ID_BODY:
            if(inBody && doc()->body()) {
                // we have another <BODY> element.... apply attributes to existing one
                // make sure we don't overwrite already existing attributes
                // some sites use <body bgcolor=rightcolor>...<body bgcolor=wrongcolor>
                NamedAttrMapImpl *map = static_cast<ElementImpl*>(n)->attributes(true);
                NamedAttrMapImpl *bmap = doc()->body()->attributes(false);
                bool changed = false;
                for (unsigned long l = 0; map && l < map->length(); ++l) {
                    NodeImpl::Id attrId = map->idAt(l);
                    DOMStringImpl *attrValue = map->valueAt(l);
                    if ( !bmap->getValue(attrId) ) {
                        bmap->setValue(attrId,attrValue);
                        changed = true;
                    }
                }
                if ( changed )
                    doc()->recalcStyle( NodeImpl::Inherit );
            } else if ( current->isDocumentNode() )
                break;
            return false;
            break;

            // the following is a hack to move non rendered elements
            // outside of tables.
            // needed for broken constructs like <table><form ...><tr>....
        case ID_INPUT:
        {
            ElementImpl *e = static_cast<ElementImpl *>(n);
            DOMString type = e->getAttribute(ATTR_TYPE);

            if ( strcasecmp( type, "hidden" ) != 0 )
                break;
            // Fall through!
        }
        case ID_TEXT:
        {
            // Don't try to fit random white-space anywhere
            TextImpl *t = static_cast<TextImpl *>(n);
            if (t->containsOnlyWhitespace())
                return false;
            // ignore text inside the following elements.
            switch(current->id())
            {
            case ID_SELECT:
                return false;
            default:
                ;
                // fall through!!
            };
            break;
        }
        case ID_DL:
            popBlock( ID_DT );
            if ( current->id() == ID_DL ) {
                e = new HTMLGenericElementImpl( document, ID_DD );
                insertNode( e );
                handled = true;
            }
            break;
        case ID_DT:
            e = new HTMLDListElementImpl(document);
            if ( insertNode(e) ) {
                insertNode(n);
                return true;
            }
            break;
        case ID_AREA:
        {
            if(map)
            {
                map->addChild(n);
#if SPEED_DEBUG < 2
                if(!n->attached() && HTMLWidget)
		    n->attach();
#endif
                handled = true;
                return true;
            }
            else
                return false;
        }

        case ID_THEAD:
        case ID_TBODY:
        case ID_TFOOT:
        case ID_CAPTION:
        case ID_COLGROUP: {
            if (isTableRelatedTag(current->id())) {
                while (blockStack && current->id() != ID_TABLE && isTableRelatedTag(current->id()))
                    popOneBlock();
                return insertNode(n);
            }
        }
        default:
            break;
        }

        // switch on the currently active element
        switch(current->id())
        {
        case ID_HTML:
            switch(id)
            {
            case ID_SCRIPT:
            case ID_STYLE:
            case ID_META:
            case ID_LINK:
            case ID_OBJECT:
            case ID_EMBED:
            case ID_TITLE:
            case ID_ISINDEX:
            case ID_BASE:
                if(!head) {
                    head = new HTMLHeadElementImpl(document);
                    insertNode(head.get());
                    handled = true;
                }
                break;
            case ID_TEXT: {
                TextImpl *t = static_cast<TextImpl *>(n);
                if (t->containsOnlyWhitespace())
                    return false;
                /* Fall through to default */
            }
            default:
                if ( haveFrameSet ) break;
                e = new HTMLBodyElementImpl(document);
                startBody();
                insertNode(e);
                handled = true;
                break;
            }
            break;
        case ID_HEAD:
            // we can get here only if the element is not allowed in head.
            if (id == ID_HTML)
                return false;
            else {
                // This means the body starts here...
                if ( haveFrameSet ) break;
                popBlock(ID_HEAD);
                e = new HTMLBodyElementImpl(document);
                startBody();
                insertNode(e);
                handled = true;
            }
            break;
        case ID_BODY:
            break;
        case ID_CAPTION:
            // Illegal content in a caption. Close the caption and try again.
            popBlock(ID_CAPTION);
            switch( id ) {
            case ID_THEAD:
            case ID_TFOOT:
            case ID_TBODY:
            case ID_TR:
            case ID_TD:
            case ID_TH:
                return insertNode(n, flat);
            }
            break;
        case ID_TABLE:
        case ID_THEAD:
        case ID_TFOOT:
        case ID_TBODY:
        case ID_TR:
            switch(id)
            {
            case ID_TABLE:
                popBlock(ID_TABLE); // end the table
                handled = checkChild( current->id(), id, doc()->inStrictMode());
                break;
            default:
            {
                NodeImpl *node = current;
                NodeImpl *parent = node->parentNode();
                // A script may have removed the current node's parent from the DOM
                // http://bugzilla.opendarwin.org/show_bug.cgi?id=7137
                // FIXME: we should do real recovery here and re-parent with the correct node.
                if (!parent)
                    return false;
                NodeImpl *parentparent = parent->parentNode();

                if (n->isTextNode() ||
                    ( node->id() == ID_TR &&
                     ( parent->id() == ID_THEAD ||
                       parent->id() == ID_TBODY ||
                       parent->id() == ID_TFOOT ) && parentparent->id() == ID_TABLE ) ||
                   ( !checkChild( ID_TR, id ) && ( node->id() == ID_THEAD || node->id() == ID_TBODY || node->id() == ID_TFOOT ) &&
                     parent->id() == ID_TABLE ) )
                {
                    node = (node->id() == ID_TABLE) ? node :
                           ((node->id() == ID_TR ) ? parentparent : parent);
                    NodeImpl *parent = node->parentNode();
                    if (!parent)
                        return false;
                    int exceptioncode = 0;
#ifdef PARSER_DEBUG
                    qDebug() << "calling insertBefore(" << n->nodeName().string() << "," << node->nodeName().string() << ")";
#endif
                    parent->insertBefore(n, node, exceptioncode);
                    if (exceptioncode) {
#ifndef PARSER_DEBUG
                        if (!n->isTextNode())
#endif
                            // qDebug() << "adding content before table failed..";
                        break;
                    }
                    if ( n->isElementNode() && tagPriority(id) != 0 &&
                         !flat && endTagRequirement(id) != DOM::FORBIDDEN ) {

                        pushBlock(id, tagPriority(id));
                        setCurrent ( n );
                        inStrayTableContent++;
                        blockStack->strayTableContent = true;
                    }
                    return true;
                }

                if ( current->id() == ID_TR )
                    e = new HTMLTableCellElementImpl(document, ID_TD);
                else if ( current->id() == ID_TABLE )
                    e = new HTMLTableSectionElementImpl( document, ID_TBODY, true /* implicit */ );
                else
                    e = new HTMLTableRowElementImpl( document );

                insertNode(e);
                handled = true;
                break;
            } // end default
            } // end switch
            break;
        case ID_OBJECT:
            discard_until = id + ID_CLOSE_TAG;
            return false;
        case ID_UL:
        case ID_OL:
        case ID_DIR:
        case ID_MENU:
            e = new HTMLLIElementImpl(document);
            e->addCSSProperty(CSS_PROP_LIST_STYLE_TYPE, CSS_VAL_NONE);
            insertNode(e);
            handled = true;
            break;
        case ID_FORM:
            popBlock(ID_FORM);
            handled = true;
            break;
        case ID_SELECT:
            if( n->isInline() )
                return false;
            break;
        case ID_P:
        case ID_H1:
        case ID_H2:
        case ID_H3:
        case ID_H4:
        case ID_H5:
        case ID_H6:
            if(!n->isInline())
            {
                popBlock(current->id());
                handled = true;
            }
            break;
        case ID_OPTION:
        case ID_OPTGROUP:
            if (id == ID_OPTGROUP)
            {
                popBlock(current->id());
                handled = true;
            }
            else if(id == ID_SELECT)
            {
                // IE treats a nested select as </select>. Let's do the same
                popBlock( ID_SELECT );
                break;
            }
            break;
            // head elements in the body should be ignored.

        case ID_ADDRESS:
        case ID_COLGROUP:
        case ID_FONT:
            popBlock(current->id());
            handled = true;
            break;
        default:
            if(current->isDocumentNode())
            {
                DocumentImpl* doc = static_cast<DocumentImpl*>(current);
                if (!doc->documentElement()) {
                    e = new HTMLHtmlElementImpl(document);
                    insertNode(e);
                    handled = true;
                }
            }
            else if(current->isInline())
            {
                popInlineBlocks();
                handled = true;
            }
        }

        // if we couldn't handle the error, just rethrow the exception...
        if(!handled)
        {
            //qDebug() << "Exception handler failed in HTMLPArser::insertNode()";
            return false;
        }

        return insertNode(n);
    }
}


NodeImpl *KHTMLParser::getElement(Token* t)
{
    NodeImpl *n = 0;

    switch(t->tid)
    {
    case ID_HTML:
        n = new HTMLHtmlElementImpl(document);
        break;
    case ID_HEAD:
        if(!head && (current->id() == ID_HTML || current->isDocumentNode())) {
            head = new HTMLHeadElementImpl(document);
            n = head.get();
        }
        break;
    case ID_BODY:
        // body no longer allowed if we have a frameset
        if(haveFrameSet) break;
        popBlock(ID_HEAD);
        n = new HTMLBodyElementImpl(document);
        haveBody =  true;
        startBody();
        break;

// head elements
    case ID_BASE:
        n = new HTMLBaseElementImpl(document);
        break;
    case ID_LINK:
        n = new HTMLLinkElementImpl(document);
        break;
    case ID_META:
        n = new HTMLMetaElementImpl(document);
        break;
    case ID_STYLE:
        n = new HTMLStyleElementImpl(document);
        break;
    case ID_TITLE:
        // only one non-empty <title> allowed
        if (haveTitle) {
            discard_until = ID_TITLE+ID_CLOSE_TAG;
            break;
        }
        n = new HTMLTitleElementImpl(document);
        // we'll set haveTitle when closing the tag
        break;

// frames
    case ID_FRAME:
        n = new HTMLFrameElementImpl(document);
        break;
    case ID_FRAMESET:
        popBlock(ID_HEAD);
        if ( inBody && !haveFrameSet && !haveContent && !haveBody) {
            popBlock( ID_BODY );
            // ### actually for IE document.body returns the now hidden "body" element
            // we can't implement that behavior now because it could cause too many
            // regressions and the headaches are not worth the work as long as there is
            // no site actually relying on that detail (Dirk)
            if (static_cast<HTMLDocumentImpl*>(document)->body())
                static_cast<HTMLDocumentImpl*>(document)->body()
                    ->addCSSProperty(CSS_PROP_DISPLAY, CSS_VAL_NONE);
            inBody = false;
        }
        if ( (haveBody || haveContent || haveFrameSet) && current->id() == ID_HTML)
            break;
        n = new HTMLFrameSetElementImpl(document);
        haveFrameSet = true;
        startBody();
        break;
        // a bit a special case, since the frame is inlined...
    case ID_IFRAME:
        n = new HTMLIFrameElementImpl(document);
        break;

// form elements
    case ID_FORM:
        // thou shall not nest <form> - NS/IE quirk
        if (form) break;
        n = form = new HTMLFormElementImpl(document, false);
        break;
    case ID_BUTTON:
        n = new HTMLButtonElementImpl(document, form);
        break;
    case ID_FIELDSET:
        n = new HTMLFieldSetElementImpl(document, form);
        break;
    case ID_INPUT:
        if ( t->attrs &&
             KHTMLGlobal::defaultHTMLSettings()->isAdFilterEnabled() &&
             KHTMLGlobal::defaultHTMLSettings()->isHideAdsEnabled() &&
             !strcasecmp( t->attrs->getValue( ATTR_TYPE ), "image" ) )
        {
            if (KHTMLGlobal::defaultHTMLSettings()->isAdFiltered( doc()->completeURL( khtml::parseURL(t->attrs->getValue(ATTR_SRC)).string() ) ))
                return 0;
        }
        n = new HTMLInputElementImpl(document, form);
        break;
    case ID_ISINDEX:
        n = handleIsindex(t);
        if( !inBody ) {
            isindex = n;
            n = 0;
        } else
            t->flat = true;
        break;
    case ID_KEYGEN:
        n = new HTMLKeygenElementImpl(document, form);
        break;
    case ID_LABEL:
        n = new HTMLLabelElementImpl(document);
        break;
    case ID_LEGEND:
        n = new HTMLLegendElementImpl(document, form);
        break;
    case ID_OPTGROUP:
        n = new HTMLOptGroupElementImpl(document, form);
        break;
    case ID_OPTION:
        popOptionalBlock(ID_OPTION);
        n = new HTMLOptionElementImpl(document, form);
        break;
    case ID_SELECT:
        inSelect = true;
        n = new HTMLSelectElementImpl(document, form);
        break;
    case ID_TEXTAREA:
        n = new HTMLTextAreaElementImpl(document, form);
        break;

// lists
    case ID_DL:
        n = new HTMLDListElementImpl(document);
        break;
    case ID_DD:
        popOptionalBlock(ID_DT);
        popOptionalBlock(ID_DD);
        n = new HTMLGenericElementImpl(document, t->tid);
        break;
    case ID_DT:
        popOptionalBlock(ID_DD);
        popOptionalBlock(ID_DT);
        n = new HTMLGenericElementImpl(document, t->tid);
        break;
    case ID_UL:
    {
        n = new HTMLUListElementImpl(document);
        break;
    }
    case ID_OL:
    {
        n = new HTMLOListElementImpl(document);
        break;
    }
    case ID_DIR:
        n = new HTMLDirectoryElementImpl(document);
        break;
    case ID_MENU:
        n = new HTMLMenuElementImpl(document);
        break;
    case ID_LI:
        popOptionalBlock(ID_LI);
        n = new HTMLLIElementImpl(document);
        break;
// formatting elements (block)
    case ID_BLOCKQUOTE:
        n = new HTMLGenericElementImpl(document, t->tid);
        break;
    case ID_LAYER:
    case ID_ILAYER:
        n = new HTMLLayerElementImpl(document, t->tid);
        break;
    case ID_P:
    case ID_DIV:
        n = new HTMLDivElementImpl(document, t->tid);
        break;
    case ID_H1:
    case ID_H2:
    case ID_H3:
    case ID_H4:
    case ID_H5:
    case ID_H6:
        n = new HTMLGenericElementImpl(document, t->tid);
        break;
    case ID_HR:
        n = new HTMLHRElementImpl(document);
        break;
    case ID_PRE:
    case ID_XMP:
    case ID_PLAINTEXT:
    case ID_LISTING:
        n = new HTMLPreElementImpl(document, t->tid);
        break;

// font stuff
    case ID_BASEFONT:
        n = new HTMLBaseFontElementImpl(document);
        break;
    case ID_FONT:
        n = new HTMLFontElementImpl(document);
        break;

// ins/del
    case ID_DEL:
    case ID_INS:
        n = new HTMLGenericElementImpl(document, t->tid);
        break;

// anchor
    case ID_A:
        popBlock(ID_A);

        n = new HTMLAnchorElementImpl(document);
        break;

// images
    case ID_IMAGE:
    case ID_IMG:
        if (t->attrs&&
            KHTMLGlobal::defaultHTMLSettings()->isAdFilterEnabled()&&
            KHTMLGlobal::defaultHTMLSettings()->isHideAdsEnabled())
        {
            QString url = doc()->completeURL( khtml::parseURL(t->attrs->getValue(ATTR_SRC)).string() );
            if (KHTMLGlobal::defaultHTMLSettings()->isAdFiltered(url))
                return 0;
        }
        n = new HTMLImageElementImpl(document, form);
        break;

    case ID_CANVAS:
        n = new HTMLCanvasElementImpl(document);
        break;

    case ID_MAP:
        map = new HTMLMapElementImpl(document);
        n = map;
        break;
    case ID_AREA:
        n = new HTMLAreaElementImpl(document);
        break;

// objects, applets and scripts
    case ID_APPLET:
        n = new HTMLAppletElementImpl(document);
        break;
    case ID_EMBED:
        n = new HTMLEmbedElementImpl(document);
        break;
    case ID_OBJECT:
        n = new HTMLObjectElementImpl(document);
        break;
    case ID_PARAM:
        n = new HTMLParamElementImpl(document);
        break;
    case ID_SCRIPT:
    {
        HTMLScriptElementImpl *scriptElement = new HTMLScriptElementImpl(document);
        scriptElement->setCreatedByParser(true);
        n = scriptElement;
        break;
    }

// media
    case ID_AUDIO:
        n = new HTMLAudioElement(document);
        break;
    case ID_VIDEO:
        n = new HTMLVideoElement(document);
        break;
    case ID_SOURCE:
        n = new HTMLSourceElement(document);
        break;

// tables
    case ID_TABLE:
        n = new HTMLTableElementImpl(document);
        break;
    case ID_CAPTION:
        n = new HTMLTableCaptionElementImpl(document);
        break;
    case ID_COLGROUP:
    case ID_COL:
        n = new HTMLTableColElementImpl(document, t->tid);
        break;
    case ID_TR:
        popBlock(ID_TR);
        n = new HTMLTableRowElementImpl(document);
        break;
    case ID_TD:
    case ID_TH:
        popBlock(ID_TH);
        popBlock(ID_TD);
        n = new HTMLTableCellElementImpl(document, t->tid);
        break;
    case ID_TBODY:
    case ID_THEAD:
    case ID_TFOOT:
        popBlock( ID_THEAD );
        popBlock( ID_TBODY );
        popBlock( ID_TFOOT );
        n = new HTMLTableSectionElementImpl(document, t->tid, false);
        break;

// inline elements
    case ID_BR:
        n = new HTMLBRElementImpl(document);
        break;
    case ID_Q:
        n = new HTMLGenericElementImpl(document, t->tid);
        break;

// elements with no special representation in the DOM

// block:
    case ID_ADDRESS:
    case ID_CENTER:
        n = new HTMLGenericElementImpl(document, t->tid);
        break;
// inline
        // %fontstyle
    case ID_TT:
    case ID_U:
    case ID_B:
    case ID_I:
    case ID_S:
    case ID_STRIKE:
    case ID_BIG:
    case ID_SMALL:

        // %phrase
    case ID_EM:
    case ID_STRONG:
    case ID_DFN:
    case ID_CODE:
    case ID_SAMP:
    case ID_KBD:
    case ID_VAR:
    case ID_CITE:
    case ID_ABBR:
    case ID_ACRONYM:

        // %special
    case ID_SUB:
    case ID_SUP:
    case ID_SPAN:
    case ID_WBR:
    case ID_NOBR:
        if ( t->tid == ID_NOBR || t->tid == ID_WBR )
            popOptionalBlock( t->tid );
    case ID_BDO:
        n = new HTMLGenericElementImpl(document, t->tid);
        break;

        // these are special, and normally not rendered
    case ID_NOEMBED:
        if (!t->flat) {
            n = new HTMLGenericElementImpl(document, t->tid);
            discard_until = ID_NOEMBED + ID_CLOSE_TAG;
        }
        return n;
    case ID_NOFRAMES:
        if (!t->flat) {
            n = new HTMLGenericElementImpl(document, t->tid);
            discard_until = ID_NOFRAMES + ID_CLOSE_TAG;
        }
        return n;
    case ID_NOSCRIPT:
        if (!t->flat) {
            n = new HTMLGenericElementImpl(document, t->tid);
            if(HTMLWidget && HTMLWidget->part()->jScriptEnabled())
                discard_until = ID_NOSCRIPT + ID_CLOSE_TAG;
        }
        return n;
    case ID_NOLAYER:
//        discard_until = ID_NOLAYER + ID_CLOSE_TAG;
        return 0;
        break;
    case ID_MARQUEE:
        n = new HTMLMarqueeElementImpl(document);
        break;
// text
    case ID_TEXT:
//        qDebug() << "ID_TEXT: \"" << DOMString(t->text).string() << "\"";
        n = new TextImpl(document, t->text);
        break;
    case ID_COMMENT:
        n = new CommentImpl(document, t->text);
        break;
    default:
        n = new HTMLGenericElementImpl(document, t->tid);
        break;
//         qDebug() << "Unknown tag " << t->tid << "!";
    }
    return n;
}

void KHTMLParser::processCloseTag(Token *t)
{
    // FIXME: the below only behaves according to "in body" insertion mode (HTML5 8.2.5.10)
    //    - might need fixing when we have other insertion modes.
    switch(t->tid)
    {
    case ID_HTML+ID_CLOSE_TAG:
    case ID_BODY+ID_CLOSE_TAG:
        // we never trust those close tags, since stupid webpages close
        // them prematurely
        return;
    case ID_FORM+ID_CLOSE_TAG: // needs additional error checking. See spec.
        form = 0;
        if (!isElementInScope(ID_FORM)) {
            // Parse error. Ignore.
            return;
        }
        // this one is to get the right style on the body element
        break;
    case ID_MAP+ID_CLOSE_TAG:
        map = 0;
        break;
    case ID_SELECT+ID_CLOSE_TAG:
        inSelect = false;
        break;
    case ID_TITLE+ID_CLOSE_TAG:
        // Set haveTitle only if <title> isn't empty
        if ( current->firstChild() )
            haveTitle = true;
        break;
    case ID_P+ID_CLOSE_TAG:
        if (!isElementInScope(ID_P)) {
            // Parse error. Handle as if <p> had been seen.
            t->tid = ID_P;
            parseToken(t);
            popBlock(ID_P);
            return;
        }
        break;
    case ID_ADDRESS+ID_CLOSE_TAG:
//    case ID_ARTICLE+ID_CLOSE_TAG:
    case ID_BLOCKQUOTE+ID_CLOSE_TAG:
    case ID_CENTER+ID_CLOSE_TAG:
//    case ID_DATAGRID+ID_CLOSE_TAG:
//    case ID_DETAILS+ID_CLOSE_TAG:
//    case ID_DIALOG+ID_CLOSE_TAG:
    case ID_DIR+ID_CLOSE_TAG:
    case ID_DIV+ID_CLOSE_TAG:
    case ID_DL+ID_CLOSE_TAG:
    case ID_FIELDSET+ID_CLOSE_TAG:
//    case ID_FIGURE+ID_CLOSE_TAG:
//    case ID_FOOTER+ID_CLOSE_TAG:
//    case ID_HEADER+ID_CLOSE_TAG:
    case ID_LISTING+ID_CLOSE_TAG:
    case ID_MENU+ID_CLOSE_TAG:
//    case ID_NAV+ID_CLOSE_TAG:
    case ID_OL+ID_CLOSE_TAG:
    case ID_PRE+ID_CLOSE_TAG:
//    case ID_SECTION+ID_CLOSE_TAG:
    case ID_UL+ID_CLOSE_TAG:

    case ID_DD+ID_CLOSE_TAG:
    case ID_DT+ID_CLOSE_TAG:
    case ID_LI+ID_CLOSE_TAG:

    case ID_APPLET+ID_CLOSE_TAG: // those four should also "Clear the list of active formatting elements
    case ID_BUTTON+ID_CLOSE_TAG: // up to the last marker." whenever we implement adoption agency.
    case ID_MARQUEE+ID_CLOSE_TAG:
    case ID_OBJECT+ID_CLOSE_TAG:  

    case ID_HEAD+ID_CLOSE_TAG: // ### according to HTML5, should be treated as 'Any other end tag'
                               //     We'll do that when proper 'Any other end tag' handling is implemented.
                               //     In the meantime, test scoping at least (#170694)

        if (!isElementInScope(t->tid-ID_CLOSE_TAG)) {
            // Parse error. Ignore token.
            return;
        }
        break;
    case ID_H1:
    case ID_H2:
    case ID_H3:
    case ID_H4:
    case ID_H5:
    case ID_H6:
        if (!isHeadingInScope()) {
            // Parse error. Ignore token.
            return;
        }
        break;
    case ID_A: // Formatting elements - will need special handling - cf. HTML5 "adoption agency algorithm"
    case ID_B: //                       meant to replace the "residual style" handling we have now.
    case ID_BIG:
    case ID_CODE:
    case ID_EM:
    case ID_FONT:
    case ID_I:
    case ID_NOBR:
    case ID_S:
    case ID_SMALL:
    case ID_STRIKE:
    case ID_STRONG:
    case ID_TT:
    case ID_U:
        break;

    default:
//      otherTag = true; // FIXME: implement 'Any other end tag' handling
        break;
    }

#ifdef PARSER_DEBUG
    qDebug() << "added the following children to " << current->nodeName().string();
    NodeImpl *child = current->firstChild();
    while(child != 0)
    {
        qDebug() << "    " << child->nodeName().string();
        child = child->nextSibling();
    }
#endif

    generateImpliedEndTags( t->tid - ID_CLOSE_TAG );
    popBlock( t->tid - ID_CLOSE_TAG );

#ifdef PARSER_DEBUG
    qDebug() << "closeTag --> current = " << current->nodeName().string();
#endif
}

bool KHTMLParser::isResidualStyleTag(int _id)
{
    switch (_id) {
        case ID_A:
        case ID_B:
        case ID_BIG:
        case ID_EM:
        case ID_FONT:
        case ID_I:
        case ID_NOBR:
        case ID_S:
        case ID_SMALL:
        case ID_STRIKE:
        case ID_STRONG:
        case ID_TT:
        case ID_U:
        case ID_DFN:
        case ID_CODE:
        case ID_SAMP:
        case ID_KBD:
        case ID_VAR:
        case ID_DEL:
        case ID_INS:
            return true;
        default:
            return false;
    }
}

bool KHTMLParser::isAffectedByResidualStyle(int _id)
{
    if (isResidualStyleTag(_id))
        return true;

    switch (_id) {
        case ID_P:
        case ID_DIV:
        case ID_BLOCKQUOTE:
        case ID_ADDRESS:
        case ID_H1:
        case ID_H2:
        case ID_H3:
        case ID_H4:
        case ID_H5:
        case ID_H6:
        case ID_CENTER:
        case ID_UL:
        case ID_OL:
        case ID_LI:
        case ID_DL:
        case ID_DT:
        case ID_DD:
        case ID_PRE:
        case ID_LISTING:
            return true;
        default:
            return false;
    }
}

void KHTMLParser::handleResidualStyleCloseTagAcrossBlocks(HTMLStackElem* elem)
{
    // Find the element that crosses over to a higher level.
    // ### For now, if there is more than one, we will only make sure we close the residual style.
    int exceptionCode = 0;
    HTMLStackElem* curr = blockStack;
    HTMLStackElem* maxElem = 0;
    HTMLStackElem* endElem = 0;
    HTMLStackElem* prev = 0;
    HTMLStackElem* prevMaxElem = 0;
    bool advancedResidual = false; // ### if set we only close the residual style
    while (curr && curr != elem) {
        if (curr->level > elem->level) {
            if (!isAffectedByResidualStyle(curr->id)) return;
            if (maxElem) advancedResidual = true;
            else
                endElem = curr;
            maxElem = curr;
            prevMaxElem = prev;
        }

        prev = curr;
        curr = curr->next;
    }

    if (!curr || !maxElem ) return;

    NodeImpl* residualElem = prev->node;
    NodeImpl* blockElem = prevMaxElem ? prevMaxElem->node : current;
    RefPtr<NodeImpl> parentElem = elem->node;

    // Check to see if the reparenting that is going to occur is allowed according to the DOM.
    // FIXME: We should either always allow it or perform an additional fixup instead of
    // just bailing here.
    // Example: <p><font><center>blah</font></center></p> isn't doing a fixup right now.
    if (!parentElem->childAllowed(blockElem))
        return;

    if (maxElem->node->parentNode() != elem->node && !advancedResidual) {
        // Walk the stack and remove any elements that aren't residual style tags.  These
        // are basically just being closed up.  Example:
        // <font><span>Moo<p>Goo</font></p>.
        // In the above example, the <span> doesn't need to be reopened.  It can just close.
        HTMLStackElem* currElem = maxElem->next;
        HTMLStackElem* prevElem = maxElem;
        while (currElem != elem) {
            HTMLStackElem* nextElem = currElem->next;
            if (!isResidualStyleTag(currElem->id)) {
                prevElem->next = nextElem;
                prevElem->setNode(currElem->node);
                delete currElem;
            }
            else
               prevElem = currElem;
            currElem = nextElem;
        }

        // We have to reopen residual tags in between maxElem and elem.  An example of this case s:
        // <font><i>Moo<p>Foo</font>.
        // In this case, we need to transform the part before the <p> into:
        // <font><i>Moo</i></font><i>
        // so that the <i> will remain open.  This involves the modification of elements
        // in the block stack.
        // This will also affect how we ultimately reparent the block, since we want it to end up
        // under the reopened residual tags (e.g., the <i> in the above example.)
        RefPtr<NodeImpl> prevNode = 0;
        RefPtr<NodeImpl> currNode = 0;
        currElem = maxElem;
        while (currElem->node != residualElem) {
            if (isResidualStyleTag(currElem->node->id())) {
                // Create a clone of this element.
                currNode = currElem->node->cloneNode(false);
                currElem->node->close();
                removeForbidden(currElem->id, forbiddenTag);

                // Change the stack element's node to point to the clone.
                currElem->setNode(currNode.get());

                // Attach the previous node as a child of this new node.
                if (prevNode)
                    currNode->appendChild(prevNode.get(), exceptionCode);
                else // The new parent for the block element is going to be the innermost clone.
                    parentElem = currNode;

                prevNode = currNode;
            }

            currElem = currElem->next;
        }

        // Now append the chain of new residual style elements if one exists.
        if (prevNode)
            elem->node->appendChild(prevNode.get(), exceptionCode);
    }

    // We need to make a clone of |residualElem| and place it just inside |blockElem|.
    // All content of |blockElem| is reparented to be under this clone.  We then
    // reparent |blockElem| using real DOM calls so that attachment/detachment will
    // be performed to fix up the rendering tree.
    // So for this example: <b>...<p>Foo</b>Goo</p>
    // The end result will be: <b>...</b><p><b>Foo</b>Goo</p>
    //
    // Step 1: Remove |blockElem| from its parent, doing a batch detach of all the kids.
    SharedPtr<NodeImpl> guard(blockElem);
    blockElem->parentNode()->removeChild(blockElem, exceptionCode);

    if (!advancedResidual) {
    // Step 2: Clone |residualElem|.
    RefPtr<NodeImpl> newNode = residualElem->cloneNode(false); // Shallow clone. We don't pick up the same kids.

    // Step 3: Place |blockElem|'s children under |newNode|.  Remove all of the children of |blockElem|
    // before we've put |newElem| into the document.  That way we'll only do one attachment of all
    // the new content (instead of a bunch of individual attachments).
    NodeImpl* currNode = blockElem->firstChild();
    while (currNode) {
        NodeImpl* nextNode = currNode->nextSibling();
        SharedPtr<NodeImpl> guard(currNode); //Protect from deletion while moving
        blockElem->removeChild(currNode, exceptionCode);
        newNode->appendChild(currNode, exceptionCode);
        currNode = nextNode;

 // TODO - To be replaced.
        // Re-register form elements with currently active form, step 1 will have removed them
        if (form && currNode && currNode->isGenericFormElement())
        {
            HTMLGenericFormElementImpl *e = static_cast<HTMLGenericFormElementImpl *>(currNode);
            form->registerFormElement(e);
        }
    }

    // Step 4: Place |newNode| under |blockElem|.  |blockElem| is still out of the document, so no
    // attachment can occur yet.
    blockElem->appendChild(newNode.get(), exceptionCode);
    }

    // Step 5: Reparent |blockElem|.  Now the full attachment of the fixed up tree takes place.
    parentElem->appendChild(blockElem, exceptionCode);

    // Step 6: Elide |elem|, since it is effectively no longer open.  Also update
    // the node associated with the previous stack element so that when it gets popped,
    // it doesn't make the residual element the next current node.
    HTMLStackElem* currElem = maxElem;
    HTMLStackElem* prevElem = 0;
    while (currElem != elem) {
        prevElem = currElem;
        currElem = currElem->next;
    }
    prevElem->next = elem->next;
    prevElem->setNode(elem->node);
    delete elem;

    // Step 7: Reopen intermediate inlines, e.g., <b><p><i>Foo</b>Goo</p>.
    // In the above example, Goo should stay italic.
    curr = blockStack;
    HTMLStackElem* residualStyleStack = 0;
    while (curr && curr != endElem) {
        // We will actually schedule this tag for reopening
        // after we complete the close of this entire block.
        NodeImpl* currNode = current;
        if (isResidualStyleTag(curr->id)) {
            // We've overloaded the use of stack elements and are just reusing the
            // struct with a slightly different meaning to the variables.  Instead of chaining
            // from innermost to outermost, we build up a list of all the tags we need to reopen
            // from the outermost to the innermost, i.e., residualStyleStack will end up pointing
            // to the outermost tag we need to reopen.
            // We also set curr->node to be the actual element that corresponds to the ID stored in
            // curr->id rather than the node that you should pop to when the element gets pulled off
            // the stack.
            popOneBlock(false);
            curr->setNode(currNode);
            curr->next = residualStyleStack;
            residualStyleStack = curr;
        }
        else
            popOneBlock();

        curr = blockStack;
    }

    reopenResidualStyleTags(residualStyleStack, 0); // FIXME: Deal with stray table content some day
                                                    // if it becomes necessary to do so.
}

void KHTMLParser::reopenResidualStyleTags(HTMLStackElem* elem, DOM::NodeImpl* malformedTableParent)
{
    // Loop for each tag that needs to be reopened.
    while (elem) {
        // Create a shallow clone of the DOM node for this element.
        RefPtr<NodeImpl> newNode = elem->node->cloneNode(false);

        // Append the new node. In the malformed table case, we need to insert before the table,
        // which will be the last child.
        int exceptionCode = 0;
        if (malformedTableParent)
            malformedTableParent->insertBefore(newNode.get(), malformedTableParent->lastChild(), exceptionCode);
        else
            current->appendChild(newNode.get(), exceptionCode);
        // FIXME: Is it really OK to ignore the exceptions here?

        // Now push a new stack element for this node we just created.
        pushBlock(elem->id, elem->level);

        // Set our strayTableContent boolean if needed, so that the reopened tag also knows
        // that it is inside a malformed table.
        blockStack->strayTableContent = malformedTableParent != 0;
        if (blockStack->strayTableContent)
            inStrayTableContent++;

        // Clear our malformed table parent variable.
        malformedTableParent = 0;

        // Update |current| manually to point to the new node.
        setCurrent(newNode.get());

        // Advance to the next tag that needs to be reopened.
        HTMLStackElem* next = elem->next;
        delete elem;
        elem = next;
    }
}

void KHTMLParser::pushBlock(int _id, int _level)
{
    HTMLStackElem *Elem = new HTMLStackElem(_id, _level, current, m_inline, blockStack);

    blockStack = Elem;
    addForbidden(_id, forbiddenTag);
}

void KHTMLParser::generateImpliedEndTags( int _id )
{
    HTMLStackElem *Elem = blockStack;

    int level = tagPriority(_id);
    while( Elem && Elem->id != _id)
    {
        HTMLStackElem *NextElem = Elem->next;
        if (endTagRequirement(Elem->id) == DOM::OPTIONAL && Elem->level <= level) {
            popOneBlock();
        }
        else
            break;
        Elem = NextElem;
    }
}

void KHTMLParser::popOptionalBlock( int _id )
{
    bool found = false;
    HTMLStackElem *Elem = blockStack;

    int level = tagPriority(_id);
    while( Elem )
    {
        if (Elem->id == _id) {
            found = true;
            break;
        }
        if (Elem->level > level || (endTagRequirement(Elem->id) != DOM::OPTIONAL && !isResidualStyleTag(Elem->id)) )
            break;
        Elem = Elem->next;
    }

    if (found) {
        generateImpliedEndTags(_id);
        popBlock(_id);
    }
}

bool KHTMLParser::isElementInScope( int _id )
{
    // HTML5 8.2.3.2
    HTMLStackElem *Elem = blockStack;
    while( Elem && Elem->id != _id ) {
        if (DOM::checkIsScopeBoundary(Elem->id))
            return false;
        Elem = Elem->next;
    }
    return Elem;
}

bool KHTMLParser::isHeadingInScope()
{
    HTMLStackElem *Elem = blockStack;
    while( Elem && (Elem->id < ID_H1 || Elem->id > ID_H6) ) {
        if (DOM::checkIsScopeBoundary(Elem->id))
            return false;
        Elem = Elem->next;
    }
    return Elem;
}

void KHTMLParser::popBlock( int _id )
{
    HTMLStackElem *Elem = blockStack;
    int maxLevel = 0;

#ifdef PARSER_DEBUG
    qDebug() << "popBlock(" << getParserPrintableName(_id) << ")";
    while(Elem) {
        qDebug() << "   > " << getParserPrintableName(Elem->id);
        Elem = Elem->next;
    }
    Elem = blockStack;
#endif

    while( Elem && (Elem->id != _id))
    {
        if (maxLevel < Elem->level)
        {
            maxLevel = Elem->level;
        }
        Elem = Elem->next;
    }
    if (!Elem)
        return;

    if (maxLevel > Elem->level) {
        // We didn't match because the tag is in a different scope, e.g.,
        // <b><p>Foo</b>.  Try to correct the problem.
        if (!isResidualStyleTag(_id))
            return;
        return handleResidualStyleCloseTagAcrossBlocks(Elem);
    }

    bool isAffectedByStyle = isAffectedByResidualStyle(Elem->id);
    HTMLStackElem* residualStyleStack = 0;
    NodeImpl* malformedTableParent = 0;

    Elem = blockStack;

    while (Elem)
    {
        if (Elem->id == _id)
        {
            int strayTable = inStrayTableContent;
            popOneBlock();
            Elem = 0;

            // This element was the root of some malformed content just inside an implicit or
            // explicit <tbody> or <tr>.
            // If we end up needing to reopen residual style tags, the root of the reopened chain
            // must also know that it is the root of malformed content inside a <tbody>/<tr>.
            if (strayTable && (inStrayTableContent < strayTable) && residualStyleStack) {
                NodeImpl* curr = current;
                while (curr && curr->id() != ID_TABLE)
                    curr = curr->parentNode();
                malformedTableParent = curr ? curr->parentNode() : 0;
            }
        }
        else
        {
            // Schedule this tag for reopening
            // after we complete the close of this entire block.
            NodeImpl* currNode = current;
            if (isAffectedByStyle && isResidualStyleTag(Elem->id)) {
                // We've overloaded the use of stack elements and are just reusing the
                // struct with a slightly different meaning to the variables.  Instead of chaining
                // from innermost to outermost, we build up a list of all the tags we need to reopen
                // from the outermost to the innermost, i.e., residualStyleStack will end up pointing
                // to the outermost tag we need to reopen.
                // We also set Elem->node to be the actual element that corresponds to the ID stored in
                // Elem->id rather than the node that you should pop to when the element gets pulled off
                // the stack.
                popOneBlock(false);
                Elem->next = residualStyleStack;
                Elem->setNode(currNode);
                residualStyleStack = Elem;
            }
            else
                popOneBlock();
            Elem = blockStack;
        }
    }

    reopenResidualStyleTags(residualStyleStack, malformedTableParent);
}

void KHTMLParser::popOneBlock(bool delBlock)
{
    HTMLStackElem *Elem = blockStack;

    // we should never get here, but some bad html might cause it.
#ifndef PARSER_DEBUG
    if(!Elem) return;
#else
    qDebug() << "popping block: " << getParserPrintableName(Elem->id) << "(" << Elem->id << ")";
#endif

#if SPEED_DEBUG < 1
    if((Elem->node != current)) {
        if (current->maintainsState() && document){
            document->registerMaintainsState(current);
            document->attemptRestoreState(current);
        }
        current->close();
    }
#endif

    removeForbidden(Elem->id, forbiddenTag);

    blockStack = Elem->next;
    // we only set inline to false, if the element we close is a block level element.
    // This helps getting cases as <p><b>bla</b> <b>bla</b> right.

    m_inline = Elem->m_inline;

    if (current->id() == ID_FORM && form && inStrayTableContent)
        form->setMalformed(true);

    setCurrent( Elem->node );

    if (Elem->strayTableContent)
        inStrayTableContent--;

    if (delBlock)
        delete Elem;
}

void KHTMLParser::popInlineBlocks()
{
    while(blockStack && current->isInline() && current->id() != ID_FONT)
        popOneBlock();
}

void KHTMLParser::freeBlock()
{
    while (blockStack)
        popOneBlock();
    blockStack = 0;
}

void KHTMLParser::createHead()
{
    if(head || !doc()->documentElement())
        return;

    head = new HTMLHeadElementImpl(document);
    HTMLElementImpl *body = doc()->body();
    int exceptioncode = 0;
    doc()->documentElement()->insertBefore(head.get(), body, exceptioncode);
    if ( exceptioncode ) {
#ifdef PARSER_DEBUG
        qDebug() << "creation of head failed!!!!:" << exceptioncode;
#endif
        delete head.get();
        head = 0;
    }
        
    // If the body does not exist yet, then the <head> should be pushed as the current block.
    if (head && !body) {
        pushBlock(head->id(), tagPriority(head->id()));
        setCurrent(head.get());
    }
}

NodeImpl *KHTMLParser::handleIsindex( Token *t )
{
    NodeImpl *n;
    HTMLFormElementImpl *myform = form;
    if ( !myform ) {
        myform = new HTMLFormElementImpl(document, true);
        n = myform;
    } else
        n = new HTMLDivElementImpl( document, ID_DIV );
    NodeImpl *child = new HTMLHRElementImpl( document );
    n->addChild( child );
    DOMStringImpl* a = t->attrs ? t->attrs->getValue(ATTR_PROMPT) : 0;
    DOMString text = i18n("This is a searchable index. Enter search keywords: ");
    if (a)
        text = a;
    child = new TextImpl(document, text.implementation());
    n->addChild( child );
    child = new HTMLIsIndexElementImpl(document, myform);
    static_cast<ElementImpl *>(child)->setAttribute(ATTR_TYPE, "khtml_isindex");
    n->addChild( child );
    child = new HTMLHRElementImpl( document );
    n->addChild( child );

    return n;
}

void KHTMLParser::startBody()
{
    if(inBody) return;

    inBody = true;

    if( isindex ) {
        insertNode( isindex, true /* don't decend into this node */ );
        isindex = 0;
    }
}
