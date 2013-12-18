/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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

#include "html/html_imageimpl.h"
#include "html/html_formimpl.h"
#include "html/html_documentimpl.h"

#include "khtmlview.h"
#include "khtml_part.h"

#include <kstringhandler.h>

#include "rendering/render_image.h"
#include "rendering/render_flow.h"
#include "css/cssstyleselector.h"
#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "css/csshelper.h"
#include "xml/dom2_eventsimpl.h"

#include <QtCore/QCharRef>
#include <QtCore/QPoint>
#include <QtCore/QStack>
#include <QRegion>
#include <QImage>

using namespace DOM;
using namespace khtml;

// -------------------------------------------------------------------------

HTMLImageElementImpl::HTMLImageElementImpl(DocumentImpl *doc, HTMLFormElementImpl *f)
    : HTMLElementImpl(doc), ismap(false), loadEventSent(true), unsafe(false), m_image(0), m_form(f)
{
    if (m_form)
        m_form->registerImgElement(this);
}

HTMLImageElementImpl::~HTMLImageElementImpl()
{
    if (document())
        document()->removeImage(this);

    if (m_image)
        m_image->deref(this);

    if (m_form)
        m_form->removeImgElement(this);
}

NodeImpl::Id HTMLImageElementImpl::id() const
{
    return ID_IMG;
}

void HTMLImageElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch (attr->id())
    {
    case ATTR_ALT:
        setChanged();
        break;
    case ATTR_SRC: {
        setChanged();

        //Start loading the image already, to generate events
        DOMString url = attr->value();
        if (!url.isEmpty()) { //### why do we not hide or something when setting this?
            DOMString parsedURL = khtml::parseURL(url);
            CachedImage* newImage = document()->docLoader()->requestImage(parsedURL);
            if (newImage && newImage != m_image) {
                CachedImage* oldImage = m_image;
                loadEventSent = false;
                m_image = newImage;
                m_image->ref(this);
                if (oldImage)
                    oldImage->deref(this);
            }

            QUrl fullURL(document()->completeURL(parsedURL.string()));
            if (document()->origin()->taintsCanvas(fullURL))
                unsafe = true;
        }
    }
    break;
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
    case ATTR_BORDER:
        // border="noborder" -> border="0"
        if(attr->value().toInt()) {
            addCSSLength(CSS_PROP_BORDER_WIDTH, attr->value());
            addCSSProperty( CSS_PROP_BORDER_TOP_STYLE, CSS_VAL_SOLID );
            addCSSProperty( CSS_PROP_BORDER_RIGHT_STYLE, CSS_VAL_SOLID );
            addCSSProperty( CSS_PROP_BORDER_BOTTOM_STYLE, CSS_VAL_SOLID );
            addCSSProperty( CSS_PROP_BORDER_LEFT_STYLE, CSS_VAL_SOLID );
        } else {
             removeCSSProperty( CSS_PROP_BORDER_WIDTH );
             removeCSSProperty( CSS_PROP_BORDER_TOP_STYLE );
             removeCSSProperty( CSS_PROP_BORDER_RIGHT_STYLE );
             removeCSSProperty( CSS_PROP_BORDER_BOTTOM_STYLE );
             removeCSSProperty( CSS_PROP_BORDER_LEFT_STYLE );
        }
        break;
    case ATTR_VSPACE:
        addCSSLength(CSS_PROP_MARGIN_TOP, attr->value());
        addCSSLength(CSS_PROP_MARGIN_BOTTOM, attr->value());
        break;
    case ATTR_HSPACE:
        addCSSLength(CSS_PROP_MARGIN_LEFT, attr->value());
        addCSSLength(CSS_PROP_MARGIN_RIGHT, attr->value());
        break;
    case ATTR_ALIGN:
	addHTMLAlignment( attr->value() );
	break;
    case ATTR_VALIGN:
	addCSSProperty(CSS_PROP_VERTICAL_ALIGN, attr->value().lower());
        break;
    case ATTR_USEMAP:
        if ( attr->value()[0] == '#' )
            usemap = attr->value().lower();
        else {
            QString url = document()->completeURL( khtml::parseURL( attr->value() ).string() );
            // ### we remove the part before the anchor and hope
            // the map is on the same html page....
            usemap = url;
        }
        m_hasAnchor = attr->val() != 0;
        break;
    case ATTR_ISMAP:
        ismap = true;
        break;
    case ATTR_ONABORT: // ### add support for this
        setHTMLEventListener(EventImpl::ABORT_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onabort", this));
        break;
    case ATTR_ONERROR:
        setHTMLEventListener(EventImpl::ERROR_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onerror", this));
        break;
    case ATTR_ONLOAD:
        setHTMLEventListener(EventImpl::LOAD_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onload", this));
        break;
    case ATTR_NOSAVE:
	break;
    case ATTR_NAME:
        if (inDocument() && m_name != attr->value()) {
            document()->underDocNamedCache().remove(m_name,        this);
            document()->underDocNamedCache().add   (attr->value(), this);
        }
        m_name = attr->value();
        //fallthrough
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLImageElementImpl::notifyFinished(CachedObject *finishedObj)
{
    if (m_image == finishedObj) {
        document()->dispatchImageLoadEventSoon(this);
    }
}

void HTMLImageElementImpl::dispatchLoadEvent()
{
    if (!loadEventSent) {
        loadEventSent = true;
        if (m_image->isErrorImage()) {
            dispatchHTMLEvent(EventImpl::ERROR_EVENT, false, false);
        } else {
            dispatchHTMLEvent(EventImpl::LOAD_EVENT, false, false);
        }
    }
}

DOMString HTMLImageElementImpl::altText() const
{
    // lets figure out the alt text.. magic stuff
    // http://www.w3.org/TR/1998/REC-html40-19980424/appendix/notes.html#altgen
    // also heavily discussed by Hixie on bugzilla
    DOMString alt( getAttribute( ATTR_ALT ) );
    // fall back to title attribute
    if ( alt.isNull() )
        alt = getAttribute( ATTR_TITLE );
#if 0
    if ( alt.isNull() ) {
        QString p = QUrl( document()->completeURL( getAttribute(ATTR_SRC).string() ) ).toDisplayString();
        int pos;
        if ( ( pos = p.lastIndexOf( '.' ) ) > 0 )
            p.truncate( pos );
        alt = DOMString( KStringHandler::csqueeze( p ) );
    }
#endif

    return alt;
}

void HTMLImageElementImpl::attach()
{
    assert(!attached());
    assert(!m_render);
    assert(parentNode());

    RenderStyle* _style = document()->styleSelector()->styleForElement(this);
    _style->ref();
    if (parentNode()->renderer() && parentNode()->renderer()->childAllowed() &&
        _style->display() != NONE)
    {
        m_render = new (document()->renderArena()) RenderImage(this);
        m_render->setStyle(_style);
        parentNode()->renderer()->addChild(m_render, nextRenderer());
    }
    _style->deref();

    NodeBaseImpl::attach();
    if (m_render)
        m_render->updateFromElement();
}

void HTMLImageElementImpl::removedFromDocument()
{
    document()->underDocNamedCache().remove(m_name, this);
    HTMLElementImpl::removedFromDocument();
}

void HTMLImageElementImpl::insertedIntoDocument()
{
    document()->underDocNamedCache().add(m_name, this);
    HTMLElementImpl::insertedIntoDocument();
}

void HTMLImageElementImpl::removeId(const DOMString& id)
{
    document()->underDocNamedCache().remove(id, this);
    HTMLElementImpl::removeId(id);
}

void HTMLImageElementImpl::addId   (const DOMString& id)
{
    document()->underDocNamedCache().add(id, this);
    HTMLElementImpl::addId(id);
}


long HTMLImageElementImpl::width() const
{
    if (!m_render) {
        DOMString widthAttr = getAttribute(ATTR_WIDTH);
        if (!widthAttr.isNull())
            return widthAttr.toInt();
        else if (m_image && m_image->pixmap_size().isValid())
            return m_image->pixmap_size().width();
        else
            return 0;
    }

    document()->updateLayout();

    return m_render ? m_render->contentWidth() :
                      getAttribute(ATTR_WIDTH).toInt();
}

long HTMLImageElementImpl::height() const
{
    if (!m_render) {
        DOMString heightAttr = getAttribute(ATTR_HEIGHT);
        if (!heightAttr.isNull())
            return heightAttr.toInt();
        else if (m_image && m_image->pixmap_size().isValid())
            return m_image->pixmap_size().height();
        else
            return 0;
    }

    document()->updateLayout();

    return m_render ? m_render->contentHeight() :
                      getAttribute(ATTR_HEIGHT).toInt();
}

void HTMLImageElementImpl::setWidth(long width)
{
    setAttribute(ATTR_WIDTH, QString::number(width));
}

void HTMLImageElementImpl::setHeight(long height)
{
    setAttribute(ATTR_HEIGHT, QString::number(height));
}

QImage HTMLImageElementImpl::currentImage() const
{
    if (!complete() || !m_image || !m_image->image())
        return QImage();

    QImage* im = m_image->image()->qimage();
    if (im)
        return *im;
    else
        return QImage();
}

long HTMLImageElementImpl::x() const
{
    if (renderer()) {
        int x = 0;
        int y = 0;
        renderer()->absolutePosition(x,y);
        return x;
    }
    return 0;
}

long HTMLImageElementImpl::y() const
{
    if (renderer()) {
        int x = 0;
        int y = 0;
        renderer()->absolutePosition(x,y);
        return y;
    }
    return 0;
}

QPixmap HTMLImageElementImpl::currentPixmap() const
{
    if (m_image) {
        return m_image->pixmap();
    }

    return QPixmap();
}

bool HTMLImageElementImpl::complete() const
{
    return m_image && m_image->isComplete();
}

// -------------------------------------------------------------------------

HTMLMapElementImpl::HTMLMapElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc)
{
}

HTMLMapElementImpl::~HTMLMapElementImpl()
{
    if(document() && document()->isHTMLDocument())
        static_cast<HTMLDocumentImpl*>(document())->mapMap.remove(name);
}

NodeImpl::Id HTMLMapElementImpl::id() const
{
    return ID_MAP;
}

bool
HTMLMapElementImpl::mapMouseEvent(int x_, int y_, int width_, int height_,
                                  RenderObject::NodeInfo& info)
{
    //cout << "map:mapMouseEvent " << endl;
    //cout << x_ << " " << y_ <<" "<< width_ <<" "<< height_ << endl;
    QStack<NodeImpl*> nodeStack;

    NodeImpl *current = firstChild();
    while(1)
    {
        if(!current)
        {
            if(nodeStack.isEmpty()) break;
            current = nodeStack.pop();
            current = current->nextSibling();
            continue;
        }
        if(current->id()==ID_AREA)
        {
            //cout << "area found " << endl;
            HTMLAreaElementImpl* area=static_cast<HTMLAreaElementImpl*>(current);
            if (area->mapMouseEvent(x_,y_,width_,height_, info))
                return true;
        }
        NodeImpl *child = current->firstChild();
        if(child)
        {
            nodeStack.push(current);
            current = child;
        }
        else
        {
            current = current->nextSibling();
        }
    }

    return false;
}

void HTMLMapElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch (attr->id())
    {
    case ATTR_ID:
        if (document()->htmlMode() != DocumentImpl::XHtml) {
            HTMLElementImpl::parseAttribute(attr);
            break;
        }
        else {
            // add name with full url:
            QString url = document()->completeURL( khtml::parseURL( attr->value() ).string() );
            if(document()->isHTMLDocument())
                static_cast<HTMLDocumentImpl*>(document())->mapMap[url] = this;
        }
        // fall through
    case ATTR_NAME:
    {
        DOMString s = attr->value();
        if(*s.unicode() == '#')
            name = QString(s.unicode()+1, s.length()-1).toLower();
        else
            name = s.string().toLower();
	// ### make this work for XML documents, e.g. in case of <html:map...>
        if(document()->isHTMLDocument())
            static_cast<HTMLDocumentImpl*>(document())->mapMap[name] = this;

        //fallthrough
    }
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

HTMLCollectionImpl* HTMLMapElementImpl::areas()
{
    return new HTMLCollectionImpl(this, HTMLCollectionImpl::MAP_AREAS);
}

// -------------------------------------------------------------------------

HTMLAreaElementImpl::HTMLAreaElementImpl(DocumentImpl *doc)
    : HTMLAnchorElementImpl(doc)
{
    m_coords=0;
    m_coordsLen = 0;
    nohref = false;
    shape = Unknown;
    lasth = lastw = -1;
}

HTMLAreaElementImpl::~HTMLAreaElementImpl()
{
    delete [] m_coords;
}

NodeImpl::Id HTMLAreaElementImpl::id() const
{
    return ID_AREA;
}

void HTMLAreaElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch (attr->id())
    {
    case ATTR_SHAPE:
        if ( strcasecmp( attr->value(), "default" ) == 0 )
            shape = Default;
        else if ( strcasecmp( attr->value(), "circle" ) == 0 )
            shape = Circle;
        else if ( strcasecmp( attr->value(), "poly" ) == 0 || strcasecmp( attr->value(),  "polygon" ) == 0 )
            shape = Poly;
        else if ( strcasecmp( attr->value(), "rect" ) == 0 )
            shape = Rect;
        break;
    case ATTR_COORDS:
        delete [] m_coords;
        m_coords = attr->val()->toCoordsArray(m_coordsLen);
        break;
    case ATTR_NOHREF:
        nohref = attr->val() != 0;
        break;
    case ATTR_TARGET:
        m_hasTarget = attr->val() != 0;
        break;
    case ATTR_ALT:
        break;
    case ATTR_ACCESSKEY:
        break;
    default:
        HTMLAnchorElementImpl::parseAttribute(attr);
    }
}

bool HTMLAreaElementImpl::mapMouseEvent(int x_, int y_, int width_, int height_,
                                   RenderObject::NodeInfo& info)
{
    bool inside = false;
    if (width_ != lastw || height_ != lasth)
    {
        region=getRegion(width_, height_);
        lastw=width_; lasth=height_;
    }
    if (region.contains(QPoint(x_,y_)))
    {
        inside = true;
        info.setInnerNode(this);
        info.setURLElement(this);
    }

    return inside;
}

QRect HTMLAreaElementImpl::getRect() const
{
    if (parentNode()->renderer()==0)
        return QRect();
    int dx, dy;
    if (!parentNode()->renderer()->absolutePosition(dx, dy))
        return QRect();
    QRegion region = getRegion(lastw,lasth);
    region.translate(dx, dy);
    return region.boundingRect();
}

QRegion HTMLAreaElementImpl::getRegion(int width_, int height_) const
{
    QRegion region;
    if (!m_coords)
        return region;

    // added broken HTML support (Dirk): some pages omit the SHAPE
    // attribute, so we try to guess by looking at the coords count
    // what the HTML author tried to tell us.

    // a Poly needs at least 3 points (6 coords), so this is correct
    if ((shape==Poly || shape==Unknown) && m_coordsLen > 5) {
        // make sure it is even
        int len = m_coordsLen >> 1;
        QPolygon points(len);
        for (int i = 0; i < len; ++i)
            points.setPoint(i, m_coords[(i<<1)].minWidth(width_),
                            m_coords[(i<<1)+1].minWidth(height_));
        region = QRegion(points);
    }
    else if ((shape==Circle && m_coordsLen>=3) || (shape==Unknown && m_coordsLen == 3)) {
        int r = qMin(m_coords[2].minWidth(width_), m_coords[2].minWidth(height_));
        region = QRegion(m_coords[0].minWidth(width_)-r,
                         m_coords[1].minWidth(height_)-r, 2*r, 2*r,QRegion::Ellipse);
    }
    else if ((shape==Rect && m_coordsLen>=4) || (shape==Unknown && m_coordsLen == 4)) {
        int x0 = m_coords[0].minWidth(width_);
        int y0 = m_coords[1].minWidth(height_);
        int x1 = m_coords[2].minWidth(width_);
        int y1 = m_coords[3].minWidth(height_);
        // use qMin () and qAbs () to make sure that this works for any pair
        // of opposite corners (x0,y0) and (x1,y1)
        region = QRegion(qMin(x0,x1),qMin(y0,y1),qAbs(x1-x0),qAbs(y1-y0));
    }
    else if (shape==Default)
        region = QRegion(0,0,width_,height_);
    // else
       // return null region

    return region;
}
