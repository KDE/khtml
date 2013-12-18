/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001-2003 Dirk Mueller (mueller@kde.org)
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
 */
// -------------------------------------------------------------------------

#include "html/html_baseimpl.h"
#include "html/html_documentimpl.h"

#include "khtmlview.h"
#include "khtml_part.h"
#include "khtmlpart_p.h"

#include "rendering/render_frames.h"
#include "rendering/render_body.h"
#include "css/cssstyleselector.h"
#include "css/css_stylesheetimpl.h"
#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "css/csshelper.h"
#include "misc/loader.h"
#include "dom/dom_string.h"
#include "dom/dom_doc.h"
#include "xml/dom2_eventsimpl.h"

#include <QUrl>
#include <QDebug>

using namespace DOM;
using namespace khtml;

HTMLBodyElementImpl::HTMLBodyElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc),
    m_bgSet( false ), m_fgSet( false )
{
    m_styleSheet = 0;
}

HTMLBodyElementImpl::~HTMLBodyElementImpl()
{
    if(m_styleSheet) m_styleSheet->deref();
}

NodeImpl::Id HTMLBodyElementImpl::id() const
{
    return ID_BODY;
}

void HTMLBodyElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {

    case ATTR_BACKGROUND:
    {
        QString url = khtml::parseURL( attr->value() ).string();
        if (!url.isEmpty()) {
            url = document()->completeURL( url );
            addCSSProperty(CSS_PROP_BACKGROUND_IMAGE, DOMString("url('"+url+"')") );
            m_bgSet = true;
        }
        else {
             removeCSSProperty(CSS_PROP_BACKGROUND_IMAGE);
             m_bgSet = false;
         }
        break;
    }
    case ATTR_MARGINWIDTH: {
	KHTMLView* w = document()->view();
	if (w)
	    w->setMarginWidth( -1 ); // unset this, so it doesn't override the setting here
        addCSSLength(CSS_PROP_MARGIN_RIGHT, attr->value() );
    }
        /* nobreak; */
    case ATTR_LEFTMARGIN:
        addCSSLength(CSS_PROP_MARGIN_LEFT, attr->value() );
        break;
    case ATTR_MARGINHEIGHT: {
	KHTMLView* w = document()->view();
	if (w)
	    w->setMarginHeight( -1 ); // unset this, so it doesn't override the setting here
        addCSSLength(CSS_PROP_MARGIN_BOTTOM, attr->value());
    }
        /* nobreak */
    case ATTR_TOPMARGIN:
        addCSSLength(CSS_PROP_MARGIN_TOP, attr->value());
        break;
    case ATTR_BGCOLOR:
        addHTMLColor(CSS_PROP_BACKGROUND_COLOR, attr->value());
        m_bgSet = !attr->value().isNull();
        break;
    case ATTR_TEXT:
        addHTMLColor(CSS_PROP_COLOR, attr->value());
        m_fgSet = !attr->value().isNull();
        break;
    case ATTR_BGPROPERTIES:
        if ( strcasecmp( attr->value(), "fixed" ) == 0)
            addCSSProperty(CSS_PROP_BACKGROUND_ATTACHMENT, CSS_VAL_FIXED);
        break;
    case ATTR_VLINK:
    case ATTR_ALINK:
    case ATTR_LINK:
    {
        if(!m_styleSheet) {
            m_styleSheet = new CSSStyleSheetImpl(this,DOMString(),true /*implicit*/);
            m_styleSheet->ref();
        }
        QString aStr;
	if ( attr->id() == ATTR_LINK )
	    aStr = "a:link";
	else if ( attr->id() == ATTR_VLINK )
	    aStr = "a:visited";
	else if ( attr->id() == ATTR_ALINK )
	    aStr = "a:active";
	aStr += " { color: " + attr->value().string() + "; }";
        m_styleSheet->parseString(aStr, false);
        if (attached())
            document()->updateStyleSelector();
        break;
    }
    case ATTR_ONLOAD:
        document()->setHTMLWindowEventListener(EventImpl::LOAD_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onload", NULL));
        break;
    case ATTR_ONUNLOAD:
        document()->setHTMLWindowEventListener(EventImpl::UNLOAD_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onunload", NULL));
        break;
    case ATTR_ONBLUR:
        document()->setHTMLWindowEventListener(EventImpl::BLUR_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onblur", NULL));
        break;
    case ATTR_ONFOCUS:
        document()->setHTMLWindowEventListener(EventImpl::FOCUS_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onfocus", NULL));
        break;
    case ATTR_ONRESIZE:
        document()->setHTMLWindowEventListener(EventImpl::RESIZE_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onresize", NULL));
        break;
    case ATTR_ONKEYUP:
        document()->setHTMLWindowEventListener(EventImpl::KEYUP_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onkeyup", NULL));
        break;
    case ATTR_ONKEYDOWN:
        document()->setHTMLWindowEventListener(EventImpl::KEYDOWN_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onkeydown", NULL));
        break;
    case ATTR_ONKEYPRESS:
        document()->setHTMLWindowEventListener(EventImpl::KEYPRESS_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onkeypress", NULL));
        break;
    case ATTR_ONSCROLL:
        document()->setHTMLWindowEventListener(EventImpl::SCROLL_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onscroll", NULL));
        break;
    case ATTR_ONMESSAGE:
        document()->setHTMLWindowEventListener(EventImpl::MESSAGE_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onmessage", NULL));
        break;        
    case ATTR_ONHASHCHANGE:
        document()->setHTMLWindowEventListener(EventImpl::HASHCHANGE_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onhashchange", NULL));
        break;
    case ATTR_NOSAVE:
	break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLBodyElementImpl::insertedIntoDocument()
{
    HTMLElementImpl::insertedIntoDocument();

    KHTMLView* w = document()->view();
    if(w && w->marginWidth() != -1) {
        QString s;
        s.sprintf( "%d", w->marginWidth() );
        addCSSLength(CSS_PROP_MARGIN_LEFT, s);
        addCSSLength(CSS_PROP_MARGIN_RIGHT, s);
    }
    if(w && w->marginHeight() != -1) {
        QString s;
        s.sprintf( "%d", w->marginHeight() );
        addCSSLength(CSS_PROP_MARGIN_TOP, s);
        addCSSLength(CSS_PROP_MARGIN_BOTTOM, s);
    }

    if ( m_bgSet && !m_fgSet )
        addCSSProperty(CSS_PROP_COLOR, CSS_VAL_BLACK);

    if (m_styleSheet)
        document()->updateStyleSelector();
}

void HTMLBodyElementImpl::removedFromDocument()
{
    HTMLElementImpl::removedFromDocument();

    if (m_styleSheet)
        document()->updateStyleSelector();
}

void HTMLBodyElementImpl::attach()
{
    assert(!m_render);
    assert(parentNode());

    RenderStyle* style = document()->styleSelector()->styleForElement(this);
    style->ref();
    if (parentNode()->renderer() && parentNode()->renderer()->childAllowed()
                                 && style->display() != NONE) {
        if (style->display() == BLOCK)
            // only use the quirky class for block display
            m_render = new (document()->renderArena()) RenderBody(this);
        else
            m_render = RenderObject::createObject(this, style);
        m_render->setStyle(style);
        parentNode()->renderer()->addChild(m_render, nextRenderer());
    }
    style->deref();

    NodeBaseImpl::attach();
}

// -------------------------------------------------------------------------

HTMLFrameElementImpl::HTMLFrameElementImpl(DocumentImpl *doc)
    : HTMLPartContainerElementImpl(doc)
{
    frameBorder = true;
    frameBorderSet = false;
    marginWidth = -1;
    marginHeight = -1;
    scrolling = Qt::ScrollBarAsNeeded;
    noresize = false;
    url = "about:blank";
}

HTMLFrameElementImpl::~HTMLFrameElementImpl()
{
}

NodeImpl::Id HTMLFrameElementImpl::id() const
{
    return ID_FRAME;
}

void HTMLFrameElementImpl::ensureUniqueName()
{
    // If we already have a name, don't do anything.
    if (!name.isEmpty())
        return;

    // Use the specified name first..
    name = getAttribute(ATTR_NAME);
    if (name.isNull())
        name = getAttribute(ATTR_ID);

    // Generate synthetic name if there isn't a natural one or
    // if the natural one conflicts
    KHTMLPart* parentPart = document()->part();
    
    // If there is no part, we will not load anything, so no 
    // worry about names being unique or not.
    if (!parentPart)
        return;

    KHTMLPart* otherFrame = parentPart->findFrame( name.string() );
    if (name.isEmpty() || (otherFrame && otherFrame != contentPart()))
        name = DOMString(parentPart->requestFrameName());

    // Make sure we're registered properly.
    parentPart->d->renameFrameForContainer(this, name.string());
}

void HTMLFrameElementImpl::defaultEventHandler(EventImpl *e)
{
     // ### duplicated in HTMLObjectBaseElementImpl
     if ( e->target() == this && m_render && m_render->isWidget() 
                                    && static_cast<RenderWidget*>(m_render)->isRedirectedWidget() 
                                    && qobject_cast<KHTMLView*>(static_cast<RenderWidget*>(m_render)->widget())) {
        switch(e->id())  {
        case EventImpl::MOUSEDOWN_EVENT:
        case EventImpl::MOUSEUP_EVENT:
        case EventImpl::MOUSEMOVE_EVENT:
        case EventImpl::MOUSEOUT_EVENT:
        case EventImpl::MOUSEOVER_EVENT:
        case EventImpl::KHTML_MOUSEWHEEL_EVENT:
        case EventImpl::KEYDOWN_EVENT:
        case EventImpl::KEYUP_EVENT:
        case EventImpl::KEYPRESS_EVENT:
        case EventImpl::DOMFOCUSIN_EVENT:
        case EventImpl::DOMFOCUSOUT_EVENT:
            if (static_cast<RenderWidget*>(m_render)->handleEvent(*e))
                e->setDefaultHandled();
        default:
            break;
        }
    }
    HTMLPartContainerElementImpl::defaultEventHandler(e);
}

void HTMLFrameElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_SRC:
        setLocation(khtml::parseURL(attr->val()));
        break;
    case ATTR_FRAMEBORDER:
    {
        frameBorder = attr->value().toInt();
        frameBorderSet = ( attr->val() != 0 );
        // FIXME: when attached, has no effect
    }
    break;
    case ATTR_MARGINWIDTH:
        marginWidth = attr->val()->toInt();
        // FIXME: when attached, has no effect
        break;
    case ATTR_MARGINHEIGHT:
        marginHeight = attr->val()->toInt();
        // FIXME: when attached, has no effect
        break;
    case ATTR_NORESIZE:
        noresize = true;
        // FIXME: when attached, has no effect
        break;
    case ATTR_SCROLLING:
        if( strcasecmp( attr->value(), "auto" ) == 0 )
            scrolling = Qt::ScrollBarAsNeeded;
        else if( strcasecmp( attr->value(), "yes" ) == 0 )
            scrolling = Qt::ScrollBarAlwaysOn;
        else if( strcasecmp( attr->value(), "no" ) == 0 )
            scrolling = Qt::ScrollBarAlwaysOff;
        // when attached, has no effect
        break;
    case ATTR_ONLOAD:
        setHTMLEventListener(EventImpl::LOAD_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onload", this));
        break;
    case ATTR_ONUNLOAD:
        setHTMLEventListener(EventImpl::UNLOAD_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onunload", this));
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLFrameElementImpl::attach()
{
    assert(!attached());
    assert(parentNode());

    computeContentIfNeeded();

    // inherit default settings from parent frameset
    HTMLElementImpl* node = static_cast<HTMLElementImpl*>(parentNode());
    while(node)
    {
        if(node->id() == ID_FRAMESET)
        {
            HTMLFrameSetElementImpl* frameset = static_cast<HTMLFrameSetElementImpl*>(node);
            if(!frameBorderSet)  frameBorder = frameset->frameBorder();
            if(!noresize)  noresize = frameset->noResize();
            break;
        }
        node = static_cast<HTMLElementImpl*>(node->parentNode());
    }

    if (parentNode()->renderer() && parentNode()->renderer()->childAllowed()
                                 && document()->isURLAllowed(url.string()))  {
        RenderStyle* _style = document()->styleSelector()->styleForElement(this);
        _style->ref();
        if ( _style->display() != NONE ) {
            m_render = new (document()->renderArena()) RenderFrame(this);
            m_render->setStyle(_style);
            parentNode()->renderer()->addChild(m_render, nextRenderer());
        }
        _style->deref();
    }

    // If we already have a widget, set it.
    if (m_render && childWidget())
        static_cast<RenderFrame*>(m_render)->setWidget(childWidget());

    NodeBaseImpl::attach();
}

void HTMLFrameElementImpl::setWidgetNotify(QWidget* widget)
{
    if (m_render)
        static_cast<RenderFrame*>(m_render)->setWidget(widget);
}

void HTMLFrameElementImpl::computeContent()
{
    KHTMLPart* parentPart = document()->part();

    if (!parentPart)
        return;

    // Bail out on any disallowed URLs
    if( !document()->isURLAllowed(url.string()) )
        return;

    // If we have a part already, make sure it's in the right spot...
    // (can happen if someone asks to change location while we're in process 
    // of loading the original one)
    if ( contentPart() ) {
        setLocation(url);
        return;
    }

    ensureUniqueName();    

    // Go ahead and load a part... We don't need to clear the widget here,
    // since the -frames- have their lifetime managed, using the name uniqueness.
    parentPart->loadFrameElement( this, url.string(), name.string() );
}

void HTMLFrameElementImpl::setLocation( const DOMString& str )
{
    url = str;

    if( !document()->isURLAllowed(url.string()) )
        return;

    // if we already have a child part, ask it to go there..
    KHTMLPart* childPart = contentPart();
    if ( childPart )
        childPart->openUrl( QUrl( document()->completeURL( url.string() ) ) );
    else
        setNeedComputeContent(); // otherwise, request it.. 
}

bool HTMLFrameElementImpl::isFocusableImpl(FocusType ft) const
{
    if (m_render!=0)
        return true;
    return HTMLPartContainerElementImpl::isFocusableImpl(ft);
}

void HTMLFrameElementImpl::setFocus(bool received)
{
    HTMLElementImpl::setFocus(received);
    khtml::RenderFrame *renderFrame = static_cast<khtml::RenderFrame *>(m_render);
    if (!renderFrame || !renderFrame->widget())
	return;
    if (received)
	renderFrame->widget()->setFocus();
    else
	renderFrame->widget()->clearFocus();
}

DocumentImpl* HTMLFrameElementImpl::contentDocument() const
{
    if (!childWidget()) return 0;

    if (::qobject_cast<KHTMLView*>( childWidget() ) )
        return static_cast<KHTMLView*>( childWidget() )->part()->xmlDocImpl();

    return 0;
}

KHTMLPart*   HTMLFrameElementImpl::contentPart() const
{
    if (!childWidget()) return 0;

    if (::qobject_cast<KHTMLView*>( childWidget() ) )
        return static_cast<KHTMLView*>( childWidget() )->part();

    return 0;
}

// -------------------------------------------------------------------------

DOMString HTMLBodyElementImpl::aLink() const
{
    return getAttribute(ATTR_ALINK);
}

void HTMLBodyElementImpl::setALink( const DOMString &value )
{
    setAttribute(ATTR_ALINK, value);
}

DOMString HTMLBodyElementImpl::bgColor() const
{
    return getAttribute(ATTR_BGCOLOR);
}

void HTMLBodyElementImpl::setBgColor( const DOMString &value )
{
    setAttribute(ATTR_BGCOLOR, value);
}

DOMString HTMLBodyElementImpl::link() const
{
    return getAttribute(ATTR_LINK);
}

void HTMLBodyElementImpl::setLink( const DOMString &value )
{
    setAttribute(ATTR_LINK, value);
}

DOMString HTMLBodyElementImpl::text() const
{
    return getAttribute(ATTR_TEXT);
}

void HTMLBodyElementImpl::setText( const DOMString &value )
{
    setAttribute(ATTR_TEXT, value);
}

DOMString HTMLBodyElementImpl::vLink() const
{
    return getAttribute(ATTR_VLINK);
}

void HTMLBodyElementImpl::setVLink( const DOMString &value )
{
    setAttribute(ATTR_VLINK, value);
}

// -------------------------------------------------------------------------

HTMLFrameSetElementImpl::HTMLFrameSetElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc)
{
    // default value for rows and cols...
    m_totalRows = 1;
    m_totalCols = 1;

    m_rows = m_cols = 0;

    frameborder = true;
    frameBorderSet = false;
    m_border = 4;
    noresize = false;

    m_resizing = false;
}

HTMLFrameSetElementImpl::~HTMLFrameSetElementImpl()
{
    delete [] m_rows;
    delete [] m_cols;
}

NodeImpl::Id HTMLFrameSetElementImpl::id() const
{
    return ID_FRAMESET;
}

void HTMLFrameSetElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_ROWS:
        if (!attr->val()) break;
        delete [] m_rows;
        m_rows = attr->val()->toLengthArray(m_totalRows);
        setChanged();
    break;
    case ATTR_COLS:
        if (!attr->val()) break;
        delete [] m_cols;
        m_cols = attr->val()->toLengthArray(m_totalCols);
        setChanged();
    break;
    case ATTR_FRAMEBORDER:
        // false or "no" or "0"..
        if ( attr->value().toInt() == 0 ) {
            frameborder = false;
            m_border = 0;
        }
        frameBorderSet = true;
        break;
    case ATTR_NORESIZE:
        noresize = true;
        break;
    case ATTR_BORDER:
        m_border = attr->val()->toInt();
        if(!m_border)
            frameborder = false;
        break;
    case ATTR_ONLOAD:
        document()->setHTMLWindowEventListener(EventImpl::LOAD_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onload", NULL));
        break;
    case ATTR_ONUNLOAD:
        document()->setHTMLWindowEventListener(EventImpl::UNLOAD_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onunload", NULL));
        break;
    case ATTR_ONMESSAGE:
        document()->setHTMLWindowEventListener(EventImpl::MESSAGE_EVENT,
            document()->createHTMLEventListener(attr->value().string(), "onmessage", NULL));
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLFrameSetElementImpl::attach()
{
    assert(!attached() );
    assert(parentNode());

    // inherit default settings from parent frameset
    HTMLElementImpl* node = static_cast<HTMLElementImpl*>(parentNode());
    while(node)
    {
        if(node->id() == ID_FRAMESET)
        {
            HTMLFrameSetElementImpl* frameset = static_cast<HTMLFrameSetElementImpl*>(node);
            if(!frameBorderSet)  frameborder = frameset->frameBorder();
            if(!noresize)  noresize = frameset->noResize();
            break;
        }
        node = static_cast<HTMLElementImpl*>(node->parentNode());
    }

    RenderStyle* _style = document()->styleSelector()->styleForElement(this);
    _style->ref();
    // ignore display: none
    if ( parentNode()->renderer() && parentNode()->renderer()->childAllowed() ) {
        m_render = new (document()->renderArena()) RenderFrameSet(this);
        m_render->setStyle(_style);
        parentNode()->renderer()->addChild(m_render, nextRenderer());
    }
    _style->deref();

    NodeBaseImpl::attach();
}

void HTMLFrameSetElementImpl::defaultEventHandler(EventImpl *evt)
{
    if (evt->isMouseEvent() && !noresize && m_render)
        static_cast<khtml::RenderFrameSet *>(m_render)->userResize(static_cast<MouseEventImpl*>(evt));

    evt->setDefaultHandled();
    HTMLElementImpl::defaultEventHandler(evt);
}

void HTMLFrameSetElementImpl::detach()
{
    if(attached())
        // ### send the event when we actually get removed from the doc instead of here
        document()->dispatchHTMLEvent(EventImpl::UNLOAD_EVENT,false,false);

    HTMLElementImpl::detach();
}

void HTMLFrameSetElementImpl::recalcStyle( StyleChange ch )
{
    if (changed() && m_render) {
        m_render->setNeedsLayout(true);
//         m_render->layout();
        setChanged(false);
    }
    HTMLElementImpl::recalcStyle( ch );
}


// -------------------------------------------------------------------------

NodeImpl::Id HTMLHeadElementImpl::id() const
{
    return ID_HEAD;
}

// -------------------------------------------------------------------------

NodeImpl::Id HTMLHtmlElementImpl::id() const
{
    return ID_HTML;
}


// -------------------------------------------------------------------------

HTMLIFrameElementImpl::HTMLIFrameElementImpl(DocumentImpl *doc) : HTMLFrameElementImpl(doc)
{
    frameBorder = false;
    marginWidth = 0;
    marginHeight = 0;
    m_frame = true;
}

void HTMLIFrameElementImpl::insertedIntoDocument()
{
    HTMLFrameElementImpl::insertedIntoDocument();
    
    assert(!contentPart());
    setNeedComputeContent();
    computeContentIfNeeded(); // also clears
}

void HTMLIFrameElementImpl::removedFromDocument()
{
    HTMLFrameElementImpl::removedFromDocument();
    clearChildWidget();
}

HTMLIFrameElementImpl::~HTMLIFrameElementImpl()
{
}

NodeImpl::Id HTMLIFrameElementImpl::id() const
{
    return ID_IFRAME;
}

void HTMLIFrameElementImpl::parseAttribute(AttributeImpl *attr )
{
    switch (  attr->id() )
    {
    case ATTR_WIDTH:
        if (!attr->value().isEmpty())
            addCSSLength(CSS_PROP_WIDTH, attr->value());
        else
            removeCSSProperty(CSS_PROP_WIDTH);
        break;
    case ATTR_HEIGHT:
        if (!attr->value().isEmpty())
            addCSSLength(CSS_PROP_HEIGHT, attr->value());
        else
            removeCSSProperty(CSS_PROP_HEIGHT);
        break;
    case ATTR_ALIGN:
        addHTMLAlignment( attr->value() );
        break;
    case ATTR_SRC:
        url = khtml::parseURL(attr->val());
        setNeedComputeContent();
        // ### synchronously start the process?
        break;
    case ATTR_NAME:
        ensureUniqueName();
        break;
    case ATTR_ID:
        HTMLFrameElementImpl::parseAttribute( attr ); // want default ID handling
        ensureUniqueName();
        break;
    case ATTR_FRAMEBORDER:
    {
        m_frame = (!attr->val() || attr->value().toInt() > 0);
        if (attached()) updateFrame();
        break;
    }
    default:
        HTMLFrameElementImpl::parseAttribute( attr );
    }
}

void HTMLIFrameElementImpl::updateFrame()
{
    if (m_frame) {
        addCSSProperty(CSS_PROP_BORDER_TOP_STYLE, CSS_VAL_OUTSET);
        addCSSProperty(CSS_PROP_BORDER_BOTTOM_STYLE, CSS_VAL_OUTSET);
        addCSSProperty(CSS_PROP_BORDER_LEFT_STYLE, CSS_VAL_OUTSET);
        addCSSProperty(CSS_PROP_BORDER_RIGHT_STYLE, CSS_VAL_OUTSET);
        addCSSLength(CSS_PROP_BORDER_WIDTH, "2");
    } else {
        addCSSProperty(CSS_PROP_BORDER_TOP_STYLE, CSS_VAL_NONE);
        addCSSProperty(CSS_PROP_BORDER_BOTTOM_STYLE, CSS_VAL_NONE);
        addCSSProperty(CSS_PROP_BORDER_LEFT_STYLE, CSS_VAL_NONE);
        addCSSProperty(CSS_PROP_BORDER_RIGHT_STYLE, CSS_VAL_NONE);
        removeCSSProperty(CSS_PROP_BORDER_WIDTH);
    }

}

void HTMLIFrameElementImpl::attach()
{
    assert(!attached());
    assert(!m_render);
    assert(parentNode());

    updateFrame();

    RenderStyle* style = document()->styleSelector()->styleForElement(this);
    style->ref();
    if (document()->isURLAllowed(url.string()) && parentNode()->renderer()
        && parentNode()->renderer()->childAllowed() && style->display() != NONE) {
        m_render = new (document()->renderArena()) RenderPartObject(this);
        m_render->setStyle(style);
        parentNode()->renderer()->addChild(m_render, nextRenderer());
    }
    style->deref();

    NodeBaseImpl::attach();

    if (m_render && childWidget())
        static_cast<RenderPartObject*>(m_render)->setWidget(childWidget());
}

void HTMLIFrameElementImpl::computeContent()
{
    KHTMLPart* parentPart = document()->part();

    if (!parentPart)
        return;

    if (!document()->isURLAllowed(url.string()))
        return;

    if (!inDocument()) {
        clearChildWidget();
        return;
    }

    // get our name in order..
    ensureUniqueName();

    // make sure "" is handled as about: blank
    const QString aboutBlank = QLatin1String("about:blank");
    QString effectiveURL = url.string();
    if (effectiveURL.isEmpty())
        effectiveURL = aboutBlank;

    // qDebug() << "-> requesting:" << name.string() << effectiveURL << contentPart();
    
    parentPart->loadFrameElement(this, effectiveURL, name.string(), QStringList(), true);
}

void HTMLIFrameElementImpl::setWidgetNotify(QWidget* widget)
{
    if (m_render)
        static_cast<RenderPartObject*>(m_render)->setWidget(widget);
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
