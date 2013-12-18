// -*- c-basic-offset: 4; -*-
/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
// -------------------------------------------------------------------------
//#define DEBUG
//#define DEBUG_LAYOUT
//#define PAR_DEBUG
//#define EVENT_DEBUG
//#define UNSUPPORTED_ATTR

#include "html_elementimpl.h"
#include "dtd.h"
#include "html_documentimpl.h"
#include "htmltokenizer.h"

#include <khtmlview.h>
#include <khtml_part.h>

#include <rendering/render_object.h>
#include <rendering/render_replaced.h>
#include <css/css_valueimpl.h>
#include <css/css_stylesheetimpl.h>
#include <css/cssproperties.h>
#include <css/cssvalues.h>
#include <xml/dom_textimpl.h>
#include <xml/dom2_eventsimpl.h>
#include <dom/dom_doc.h>

#include <QDebug>

using namespace DOM;
using namespace khtml;

HTMLElementImpl::HTMLElementImpl(DocumentImpl *doc)
    : ElementImpl( doc )
{
    m_htmlCompat = doc && doc->htmlMode() != DocumentImpl::XHtml;
}

HTMLElementImpl::~HTMLElementImpl()
{
}

bool HTMLElementImpl::isInline() const
{
    if (renderer())
        return ElementImpl::isInline();

    switch(id()) {
        case ID_A:
        case ID_FONT:
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
        case ID_NOBR:
        case ID_WBR:
            return true;

        default:
            return ElementImpl::isInline();
    }
}

DOMString HTMLElementImpl::namespaceURI() const
{
    return DOMString(XHTML_NAMESPACE);
}

void HTMLElementImpl::parseAttribute(AttributeImpl *attr)
{
    DOMString indexstring;
    switch( attr->id() )
    {
    case ATTR_ALIGN:
        if (attr->val()) {
            if ( strcasecmp(attr->value(), "middle" ) == 0 )
                addCSSProperty( CSS_PROP_TEXT_ALIGN, CSS_VAL_CENTER );
            else
                addCSSProperty(CSS_PROP_TEXT_ALIGN, attr->value().lower());
        }
        else
            removeCSSProperty(CSS_PROP_TEXT_ALIGN);
        break;
// the core attributes...
    case ATTR_ID:
        // unique id
        setHasID();
        document()->incDOMTreeVersion(DocumentImpl::TV_IDNameHref);
        break;
    case ATTR_CLASS:
        if (attr->val()) {
            DOMString v = attr->value();
            const QChar* characters = v.unicode();
            unsigned length = v.length();
            unsigned i;
            for (i = 0; i < length && characters[i].isSpace(); ++i) { }
            setHasClass(i < length);
            attributes()->setClass(v);
        } else {
            setHasClass(false);
        }
        document()->incDOMTreeVersion(DocumentImpl::TV_Class);
        break;
    case ATTR_NAME:
        document()->incDOMTreeVersion(DocumentImpl::TV_IDNameHref);
        break;
    case ATTR_CONTENTEDITABLE:
        setContentEditable(attr);
        break;
    case ATTR_STYLE:
        getInlineStyleDecls()->updateFromAttribute(attr->value());
        setChanged();
        break;
    case ATTR_TABINDEX:
        indexstring=getAttribute(ATTR_TABINDEX);
        if (attr->val())
            setTabIndex(attr->value().toInt());
        else
            setNoTabIndex();
        break;
// i18n attributes
    case ATTR_LANG:
        break;
    case ATTR_DIR:
        addCSSProperty(CSS_PROP_DIRECTION, attr->value().lower());
        break;
// standard events
    case ATTR_ONCLICK:
	setHTMLEventListener(EventImpl::KHTML_ECMA_CLICK_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onclick", this));
        break;
    case ATTR_ONDBLCLICK:
	setHTMLEventListener(EventImpl::KHTML_ECMA_DBLCLICK_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "ondblclick", this));
        break;
    case ATTR_ONMOUSEDOWN:
        setHTMLEventListener(EventImpl::MOUSEDOWN_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onmousedown", this));
        break;
    case ATTR_ONMOUSEMOVE:
        setHTMLEventListener(EventImpl::MOUSEMOVE_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onmousemove", this));
        break;
    case ATTR_ONMOUSEOUT:
        setHTMLEventListener(EventImpl::MOUSEOUT_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onmouseout", this));
        break;
    case ATTR_ONMOUSEOVER:
        setHTMLEventListener(EventImpl::MOUSEOVER_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onmouseover", this));
        break;
    case ATTR_ONMOUSEUP:
        setHTMLEventListener(EventImpl::MOUSEUP_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onmouseup", this));
        break;
    case ATTR_ONKEYDOWN:
        setHTMLEventListener(EventImpl::KEYDOWN_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onkeydown", this));
	break;
    case ATTR_ONKEYPRESS:
        setHTMLEventListener(EventImpl::KEYPRESS_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onkeypress", this));
	break;
    case ATTR_ONKEYUP:
        setHTMLEventListener(EventImpl::KEYUP_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onkeyup", this));
        break;
    case ATTR_ONFOCUS:
        setHTMLEventListener(EventImpl::FOCUS_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onfocus", this));
        break;
    case ATTR_ONBLUR:
        setHTMLEventListener(EventImpl::BLUR_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onblur", this));
        break;
    case ATTR_ONSCROLL:
        setHTMLEventListener(EventImpl::SCROLL_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onscroll", this));
        break;
// other misc attributes
    default:
#ifdef UNSUPPORTED_ATTR
	qDebug() << "UATTR: <" << this->nodeName().string() << "> ["
		      << attr->name().string() << "]=[" << attr->value().string() << "]" << endl;
#endif
        break;
    }
}

void HTMLElementImpl::recalcStyle( StyleChange ch )
{
    ElementImpl::recalcStyle( ch );

    if (m_render /*&& changed*/)
        m_render->updateFromElement();
}

void HTMLElementImpl::addCSSProperty(int id, const DOMString &value)
{
    if (!m_hasCombinedStyle) createNonCSSDecl();
    nonCSSStyleDecls()->setProperty(id, value, false);
    setChanged();
}

void HTMLElementImpl::addCSSProperty(int id, int value)
{
    if (!m_hasCombinedStyle) createNonCSSDecl();
    nonCSSStyleDecls()->setProperty(id, value, false);
    setChanged();
}

void HTMLElementImpl::addCSSLength(int id, const DOMString &value, bool numOnly, bool multiLength)
{
    if (!m_hasCombinedStyle) createNonCSSDecl();

    // strip attribute garbage to avoid CSS parsing errors
    // ### create specialized hook that avoids parsing every
    // value twice!
    if ( value.implementation() ) {
        // match \s*[+-]?\d*(\.\d*)?[%\*]?
        unsigned i = 0, j = 0;
        QChar* s = value.implementation()->s;
        unsigned l = value.implementation()->l;

        while (i < l && s[i].isSpace())
            ++i;
        if (i < l && (s[i] == '+' || s[i] == '-'))
            ++i;
        while (i < l && s[i].isDigit())
            ++i,++j;

        // no digits!
        if (j == 0) return;

        int v = qBound( -8192, QString::fromRawData(s, i).toInt(), 8191 ) ;
        const char* suffix = "px";
        if (!numOnly || multiLength) {
            // look if we find a % or *
            while (i < l) {
                if (multiLength && s[i] == '*') {
                    suffix = "";
                    break;
                }
                if (s[i] == '%') {
                    suffix = "%";
                    break;
                }
                ++i;
            }
        }
	if (numOnly) suffix = "";

        QString ns = QString::number(v) + suffix;
        nonCSSStyleDecls()->setLengthProperty( id, DOMString( ns ), false, multiLength );
        setChanged();
        return;
    }

    nonCSSStyleDecls()->setLengthProperty(id, value, false, multiLength);
    setChanged();
}

static inline bool isHexDigit( const QChar &c ) {
    return ( c >= '0' && c <= '9' ) ||
	   ( c >= 'a' && c <= 'f' ) ||
	   ( c >= 'A' && c <= 'F' );
}

static inline int toHex( const QChar &c ) {
    return ( (c >= '0' && c <= '9')
	     ? (c.unicode() - '0')
	     : ( ( c >= 'a' && c <= 'f' )
		 ? (c.unicode() - 'a' + 10)
		 : ( ( c >= 'A' && c <= 'F' )
		     ? (c.unicode() - 'A' + 10)
		     : -1 ) ) );
}

/* color parsing that tries to match as close as possible IE 6. */
void HTMLElementImpl::addHTMLColor( int id, const DOMString &c )
{
    if (!m_hasCombinedStyle) createNonCSSDecl();

    // this is the only case no color gets applied in IE.
    if ( !c.length() ) {
	removeCSSProperty(id);
	return;
    }

    if ( nonCSSStyleDecls()->setProperty(id, c, false) )
	return;

    QString color = c.string();
    // not something that fits the specs.

    // we're emulating IEs color parser here. It maps transparent to black, otherwise it tries to build a rgb value
    // out of everyhting you put in. The algorithm is experimentally determined, but seems to work for all test cases I have.

    // the length of the color value is rounded up to the next
    // multiple of 3. each part of the rgb triple then gets one third
    // of the length.
    //
    // Each triplet is parsed byte by byte, mapping
    // each number to a hex value (0-9a-fA-F to their values
    // everything else to 0).
    //
    // The highest non zero digit in all triplets is remembered, and
    // used as a normalization point to normalize to values between 0
    // and 255.

    if ( color.toLower() != "transparent" ) {
	if ( color[0] == '#' )
	    color.remove( 0,  1 );
	int basicLength = (color.length() + 2) / 3;
	if ( basicLength > 1 ) {
	    // IE ignores colors with three digits or less
// 	    qDebug("trying to fix up color '%s'. basicLength=%d, length=%d",
// 		   color.toLatin1().constData(), basicLength, color.length() );
	    int colors[3] = { 0, 0, 0 };
	    int component = 0;
	    int pos = 0;
	    int maxDigit = basicLength-1;
	    while ( component < 3 ) {
		// search forward for digits in the string
		int numDigits = 0;
		while ( pos < (int)color.length() && numDigits < basicLength ) {
		    int hex = toHex( color[pos] );
		    colors[component] = (colors[component] << 4);
		    if ( hex > 0 ) {
			colors[component] += hex;
			maxDigit = qMin( maxDigit, numDigits );
		    }
		    numDigits++;
		    pos++;
		}
		while ( numDigits++ < basicLength )
		    colors[component] <<= 4;
		component++;
	    }
	    maxDigit = basicLength - maxDigit;
// 	    qDebug("color is %x %x %x, maxDigit=%d",  colors[0], colors[1], colors[2], maxDigit );

	    // normalize to 00-ff. The highest filled digit counts, minimum is 2 digits
	    maxDigit -= 2;
	    colors[0] >>= 4*maxDigit;
	    colors[1] >>= 4*maxDigit;
	    colors[2] >>= 4*maxDigit;
// 	    qDebug("normalized color is %x %x %x",  colors[0], colors[1], colors[2] );
	    // 	assert( colors[0] < 0x100 && colors[1] < 0x100 && colors[2] < 0x100 );

	    color.sprintf("#%02x%02x%02x", colors[0], colors[1], colors[2] );
// 	    qDebug( "trying to add fixed color string '%s'", color.toLatin1().constData() );
	    if ( nonCSSStyleDecls()->setProperty(id, DOMString(color), false) )
		return;
	}
    }
    nonCSSStyleDecls()->setProperty(id, CSS_VAL_BLACK, false);
}

void HTMLElementImpl::removeCSSProperty(int id)
{
    if (!m_hasCombinedStyle)
        return;
    nonCSSStyleDecls()->setParent(document()->elementSheet());
    nonCSSStyleDecls()->removeProperty(id);
    setChanged();
}

DOMString HTMLElementImpl::innerHTML() const
{
    QString result = ""; //Use QString to accumulate since DOMString is poor for appends
    for (NodeImpl *child = firstChild(); child != NULL; child = child->nextSibling()) {
        DOMString kid = child->toString();
        result += QString::fromRawData(kid.unicode(), kid.length());
    }
    return result;
}

DOMString HTMLElementImpl::innerText() const
{
    QString text = "";
    if(!firstChild())
        return text;

    const NodeImpl *n = this;
    // find the next text/image after the anchor, to get a position
    while(n) {
        if(n->firstChild())
            n = n->firstChild();
        else if(n->nextSibling())
            n = n->nextSibling();
        else {
            NodeImpl *next = 0;
            while(!next) {
                n = n->parentNode();
                if(!n || n == (NodeImpl *)this ) goto end;
                next = n->nextSibling();
            }
            n = next;
        }
        if(n->isTextNode() ) {
            DOMStringImpl* data = static_cast<const TextImpl *>(n)->string();
            text += QString::fromRawData(data->s, data->l);
        }
    }
 end:
    return text;
}

DocumentFragment HTMLElementImpl::createContextualFragment( const DOMString &html )
{
    // IE originally restricted innerHTML to a small subset of elements; 
    // and we largely matched that. Mozilla's embrace of innerHTML, however, extended 
    // it to pretty much everything, and so the web (and HTML5) requires it now.
    // For now, we accept everything, but do not do context-based recovery in the parser.
    if ( !document()->isHTMLDocument() )
        return DocumentFragment();

    DocumentFragmentImpl* fragment = new DocumentFragmentImpl( docPtr() );
    DocumentFragment f( fragment );
    {
        HTMLTokenizer tok( docPtr(), fragment );
        tok.begin();
        tok.write( html.string(), true );
        tok.end();
    }

    // Exceptions are ignored because none ought to happen here.
    int ignoredExceptionCode;

    // we need to pop <html> and <body> elements and remove <head> to
    // accomadate folks passing complete HTML documents to make the
    // child of an element.
    for ( NodeImpl* node = fragment->firstChild(); node; ) {
        if (node->id() == ID_HTML || node->id() == ID_BODY) {
            NodeImpl* firstChild = node->firstChild();
            NodeImpl* child = firstChild;
            while ( child ) {
                NodeImpl *nextChild = child->nextSibling();
                fragment->insertBefore(child, node, ignoredExceptionCode);
                child = nextChild;
            }
            if ( !firstChild ) {
                NodeImpl *nextNode = node->nextSibling();
                fragment->removeChild(node, ignoredExceptionCode);
                node = nextNode;
            } else {
                fragment->removeChild(node, ignoredExceptionCode);
                node = firstChild;
            }
        } else if (node->id() == ID_HEAD) {
            NodeImpl *nextNode = node->nextSibling();
            fragment->removeChild(node, ignoredExceptionCode);
            node = nextNode;
        } else {
            node = node->nextSibling();
        }
    }

    return f;
}

void HTMLElementImpl::setInnerHTML( const DOMString &html, int &exceptioncode )
{
    if (id() == ID_SCRIPT || id() == ID_STYLE) {
        // Script and CSS source shouldn't be parsed as HTML.
        removeChildren();
        appendChild(document()->createTextNode(html), exceptioncode);
        return;
    }

    DocumentFragment fragment = createContextualFragment( html );
    if ( fragment.isNull() ) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    // Make sure adding the new child is ok, before removing all children (#96187)
    checkAddChild( fragment.handle(), exceptioncode );
    if ( exceptioncode )
        return;

    removeChildren();
    appendChild( fragment.handle(), exceptioncode );
}

void HTMLElementImpl::setInnerText( const DOMString &text, int& exceptioncode )
{
    // following the IE specs.
    if( endTagRequirement(id()) == FORBIDDEN ) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    // IE disallows innerHTML on inline elements. I don't see why we should have this restriction, as our
    // dhtml engine can cope with it. Lars
    //if ( isInline() ) return false;
    switch( id() ) {
        case ID_COL:
        case ID_COLGROUP:
        case ID_FRAMESET:
        case ID_HEAD:
        case ID_HTML:
        case ID_TABLE:
        case ID_TBODY:
        case ID_TFOOT:
        case ID_THEAD:
        case ID_TR:
            exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
            return;
        default:
            break;
    }

    removeChildren();

    TextImpl *t = new TextImpl( docPtr(), text.implementation() );
    appendChild( t, exceptioncode );
}

void HTMLElementImpl::addHTMLAlignment( DOMString alignment )
{
    //qDebug("alignment is %s", alignment.string().toLatin1().constData() );
    // vertical alignment with respect to the current baseline of the text
    // right or left means floating images
    int propfloat = -1;
    int propvalign = -1;
    if ( strcasecmp( alignment, "absmiddle" ) == 0 ) {
        propvalign = CSS_VAL_MIDDLE;
    } else if ( strcasecmp( alignment, "absbottom" ) == 0 ) {
        propvalign = CSS_VAL_BOTTOM;
    } else if ( strcasecmp( alignment, "left" ) == 0 ) {
	propfloat = CSS_VAL_LEFT;
	propvalign = CSS_VAL_TOP;
    } else if ( strcasecmp( alignment, "right" ) == 0 ) {
	propfloat = CSS_VAL_RIGHT;
	propvalign = CSS_VAL_TOP;
    } else if ( strcasecmp( alignment, "top" ) == 0 ) {
	propvalign = CSS_VAL_TOP;
    } else if ( strcasecmp( alignment, "middle" ) == 0 ) {
	propvalign = CSS_VAL__KHTML_BASELINE_MIDDLE;
    } else if ( strcasecmp( alignment, "center" ) == 0 ) {
	propvalign = CSS_VAL_MIDDLE;
    } else if ( strcasecmp( alignment, "bottom" ) == 0 ) {
	propvalign = CSS_VAL_BASELINE;
    } else if ( strcasecmp ( alignment, "texttop") == 0 ) {
	propvalign = CSS_VAL_TEXT_TOP;
    }

    if ( propfloat != -1 )
	addCSSProperty( CSS_PROP_FLOAT, propfloat );
    if ( propvalign != -1 )
	addCSSProperty( CSS_PROP_VERTICAL_ALIGN, propvalign );
}

DOMString HTMLElementImpl::contentEditable() const
{
    document()->updateRendering();

    if (!renderer())
        return "false";

    switch (renderer()->style()->userInput()) {
        case UI_ENABLED:
            return "true";
        case UI_DISABLED:
        case UI_NONE:
            return "false";
        default: ;
    }
    return "inherit";
}

void HTMLElementImpl::setContentEditable(AttributeImpl* attr)
{
    const DOMString &enabled = attr->value();
    if (enabled.isEmpty() || strcasecmp(enabled, "true") == 0)
        addCSSProperty(CSS_PROP__KHTML_USER_INPUT, CSS_VAL_ENABLED);
    else if (strcasecmp(enabled, "false") == 0)
        addCSSProperty(CSS_PROP__KHTML_USER_INPUT, CSS_VAL_NONE);
    else if (strcasecmp(enabled, "inherit") == 0)
        addCSSProperty(CSS_PROP__KHTML_USER_INPUT, CSS_VAL_INHERIT);
}

void HTMLElementImpl::setContentEditable(const DOMString &enabled) {
    if (enabled == "inherit") {
        int exceptionCode;
        removeAttribute(ATTR_CONTENTEDITABLE, exceptionCode);
    }
    else
        setAttribute(ATTR_CONTENTEDITABLE, enabled.isEmpty() ? "true" : enabled);
}

DOMString HTMLElementImpl::toString() const
{
    if (!hasChildNodes()) {
	DOMString result = openTagStartToString();
	result += ">";

	if (endTagRequirement(id()) == REQUIRED) {
	    result += "</";
	    result += nonCaseFoldedTagName();
	    result += ">";
	}

	return result;
    }

    return ElementImpl::toString();
}

// -------------------------------------------------------------------------
HTMLGenericElementImpl::HTMLGenericElementImpl(DocumentImpl *doc, ushort i)
    : HTMLElementImpl(doc)
{
    m_localName = LocalName::fromId(i);
}

HTMLGenericElementImpl::HTMLGenericElementImpl(DocumentImpl *doc, LocalName l)
    : HTMLElementImpl(doc),
      m_localName(l)
{}


HTMLGenericElementImpl::~HTMLGenericElementImpl()
{
}
