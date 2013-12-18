/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 *           (C) 2007, 2008 Maks Orlovich (maksim@kde.org)
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
#include "html/html_objectimpl.h"

#include "khtml_part.h"
#include "dom/dom_string.h"
#include "imload/imagemanager.h"
#include "khtmlview.h"
#include <QtCore/QCharRef>
#include <QtCore/QVariant>
#include <QtCore/QMap>
#include <QtCore/QTimer>
#include <QImageReader>

#include <QDebug>
#include <kmessagebox.h>
#include <qmimedatabase.h>


#include "xml/dom_docimpl.h"
#include "css/cssstyleselector.h"
#include "css/csshelper.h"
#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "rendering/render_frames.h"
#include "rendering/render_image.h"
#include "xml/dom2_eventsimpl.h"

using namespace DOM;
using namespace khtml;

HTMLPartContainerElementImpl::HTMLPartContainerElementImpl(DocumentImpl *doc)
    : HTMLElementImpl(doc)
{
    m_needToComputeContent = true;
    m_childWidget          = 0;
}

HTMLPartContainerElementImpl::~HTMLPartContainerElementImpl()
{
    // Kill the renderer here, since we are asking for a widget to be deleted
    if (m_render)
        detach();

    if (m_childWidget)
        m_childWidget->deleteLater();
}

void HTMLPartContainerElementImpl::recalcStyle(StyleChange ch)
{
    computeContentIfNeeded();

    HTMLElementImpl::recalcStyle( ch );
}

void HTMLPartContainerElementImpl::close()
{
    HTMLElementImpl::close(); // Do it first, to make sure closed() is set.
    computeContentIfNeeded();
}

void HTMLPartContainerElementImpl::computeContentIfNeeded()
{
    if (!m_needToComputeContent)
        return;

    m_needToComputeContent = false;
    computeContent();
}

void HTMLPartContainerElementImpl::setNeedComputeContent()
{
    m_needToComputeContent = true;
    if (closed())
        setChanged(); //React quickly when not in the middle of parsing..
}

void HTMLPartContainerElementImpl::setWidget(QWidget* widget)
{
    if (widget == m_childWidget)
        return; // The same part got navigated. Don't do anything

    QWidget* oldWidget = m_childWidget;
    m_childWidget = widget;
    if (m_childWidget)
        m_childWidget->hide();

    setWidgetNotify(m_childWidget);
    if (oldWidget) {
        oldWidget->hide();
        oldWidget->deleteLater();
    }
}

void HTMLPartContainerElementImpl::partLoadingErrorNotify()
{
    clearChildWidget();
}

void HTMLPartContainerElementImpl::clearChildWidget()
{
    setWidget(0);
}

bool HTMLPartContainerElementImpl::mimetypeHandledInternally(const QString&)
{
    return false;
}

void HTMLPartContainerElementImpl::slotEmitLoadEvent()
{
    dispatchHTMLEvent(EventImpl::LOAD_EVENT, false, false);
}

void HTMLPartContainerElementImpl::postResizeEvent()
{
   QApplication::postEvent( this, new QEvent(static_cast<QEvent::Type>(DOMCFResizeEvent)) );
}

void HTMLPartContainerElementImpl::sendPostedResizeEvents()
{
    QApplication::sendPostedEvents(0, DOMCFResizeEvent);
}

bool HTMLPartContainerElementImpl::event(QEvent *e)
{
    if (e->type() == static_cast<QEvent::Type>(DOMCFResizeEvent)) {
        dispatchWindowEvent(EventImpl::RESIZE_EVENT, false, false);
        e->accept();
        return true;
    }
    return QObject::event(e);
}

// -------------------------------------------------------------------------
HTMLObjectBaseElementImpl::HTMLObjectBaseElementImpl(DocumentImpl *doc)
    : HTMLPartContainerElementImpl(doc)
{
    m_renderAlternative    = false;
    m_imageLike            = false;
    m_rerender             = false;
}

void HTMLObjectBaseElementImpl::setServiceType(const QString & val) {
    serviceType = val.toLower();
    int pos = serviceType.indexOf( ";" );
    if ( pos!=-1 )
        serviceType.truncate( pos );
}

void HTMLObjectBaseElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch ( attr->id() )
    {
        case ATTR_TYPE:
        case ATTR_CODETYPE:
            if (attr->val()) {
                setServiceType(attr->val()->string());
                setNeedComputeContent();
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
        case ATTR_NAME:
            if (inDocument() && m_name != attr->value()) {
                document()->underDocNamedCache().remove(m_name,        this);
                document()->underDocNamedCache().add   (attr->value(), this);
            }
            m_name = attr->value();
            //fallthrough
        default:
            HTMLElementImpl::parseAttribute( attr );
    }
}


void HTMLObjectBaseElementImpl::defaultEventHandler(EventImpl *e)
{
    // ### duplicated in HTMLIFrameElementImpl
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
    HTMLElementImpl::defaultEventHandler(e);
}


void HTMLObjectBaseElementImpl::removedFromDocument()
{
    document()->underDocNamedCache().remove(m_name, this);

    // When removed from document, we destroy the widget/plugin.
    // We have to do it here and not just call setNeedComputeContent(),
    // since khtml will not try to restyle changed() things not in document.
    clearChildWidget();
    
    HTMLPartContainerElementImpl::removedFromDocument();
}

void HTMLObjectBaseElementImpl::insertedIntoDocument()
{
    document()->underDocNamedCache().add(m_name, this);
    setNeedComputeContent();
    HTMLPartContainerElementImpl::insertedIntoDocument();
}

void HTMLObjectBaseElementImpl::removeId(const DOMString& id)
{
    document()->underDocNamedCache().remove(id, this);
    HTMLElementImpl::removeId(id);
}

void HTMLObjectBaseElementImpl::addId   (const DOMString& id)
{
    document()->underDocNamedCache().add(id, this);
    HTMLElementImpl::addId(id);
}

void HTMLObjectBaseElementImpl::requestRerender()
{
    if (m_rerender) return;
    m_rerender = true;
    QTimer::singleShot( 0, this, SLOT(slotRerender()) );
}

void HTMLObjectBaseElementImpl::slotRerender()
{
    // the singleshot timer might have fired after we're removed
    // from the document, but not yet deleted due to references
    if ( !inDocument() || !m_rerender ) return;

    // ### there can be a m_render if this is called from our attach indirectly
    if ( attached() || m_render) {
        detach();
        attach();
    }

    m_rerender = false;
}

void HTMLObjectBaseElementImpl::attach() {
    assert(!attached());
    assert(!m_render);

    computeContentIfNeeded();
    m_rerender = false;

    if (m_renderAlternative && !m_imageLike) {
        // render alternative content
        ElementImpl::attach();
        return;
    }

    if (!parentNode()->renderer()) {
        NodeBaseImpl::attach();
        return;
    }

    RenderStyle* _style = document()->styleSelector()->styleForElement(this);
    _style->ref();

    if (parentNode()->renderer() && parentNode()->renderer()->childAllowed() &&
        _style->display() != NONE)
    {
        if (m_imageLike) {
            m_render = new (document()->renderArena()) RenderImage(this);
        } else {
            m_render = new (document()->renderArena()) RenderPartObject(this);
            // If we already have a widget, set it.
            if (childWidget())
                static_cast<RenderFrame*>(m_render)->setWidget(childWidget());
        }

        m_render->setStyle(_style);
        parentNode()->renderer()->addChild(m_render, nextRenderer());
        if (m_imageLike)
            m_render->updateFromElement();
    }

    _style->deref();
    NodeBaseImpl::attach();
}

HTMLEmbedElementImpl* HTMLObjectBaseElementImpl::relevantEmbed()
{
    for (NodeImpl *child = firstChild(); child; child = child->nextSibling()) {
        if ( child->id() == ID_EMBED ) {
            return static_cast<HTMLEmbedElementImpl *>( child );
        }
    }

    return 0;
}

bool HTMLObjectBaseElementImpl::mimetypeHandledInternally(const QString& mime)
{
    QStringList supportedImageTypes = khtmlImLoad::ImageManager::loaderDatabase()->supportedMimeTypes();

    bool newImageLike = supportedImageTypes.contains(mime);

    if (newImageLike != m_imageLike) {
        m_imageLike = newImageLike;
        requestRerender();
    }

    return newImageLike; // No need for kpart for that.
}

void HTMLObjectBaseElementImpl::computeContent()
{
    QStringList params;
    QString     effectiveURL = url; // May be overwritten by some of the <params>
                                    // if the URL isn't there
    QString     effectiveServiceType = serviceType;

    // We need to wait until everything has parsed, since we need the <param>s,
    // and the embedded <embed>
    if (!closed()) {
        setNeedComputeContent();
        return;
    }

    // Not in document => no plugin.
    if (!inDocument()) {
        clearChildWidget();
        return;
    }

    // Collect information from <param> children for ...
    // It also sometimes supplements or replaces some of the element's attributes
    for (NodeImpl* child = firstChild(); child; child = child->nextSibling()) {
        if (child->id() == ID_PARAM) {
            HTMLParamElementImpl *p = static_cast<HTMLParamElementImpl *>( child );

            QString aStr = p->name();
            aStr += QLatin1String("=\"");
            aStr += p->value();
            aStr += QLatin1String("\"");
            QString name_lower = p->name().toLower();
            if (name_lower == QLatin1String("type") && id() != ID_APPLET) {
                setServiceType(p->value());
                effectiveServiceType = serviceType;
            } else if (effectiveURL.isEmpty() &&
                        (name_lower == QLatin1String("src") ||
                        name_lower == QLatin1String("movie") ||
                        name_lower == QLatin1String("code"))) {
                    effectiveURL = p->value();
            }
            params.append(aStr);
        }
    }

    // For <applet>(?) and <embed> we also make each attribute a part parameter
    if (id() != ID_OBJECT) {
          NamedAttrMapImpl* a = attributes();
          if (a) {
            for (unsigned i = 0; i < a->length(); ++i) {
                NodeImpl::Id id = a->idAt(i);
                DOMString value = a->valueAt(i);
                params.append(LocalName::fromId(localNamePart(id)).toString().string() + "=\"" + value.string() + "\"");
            }
        }
    }

    params.append( QLatin1String("__KHTML__PLUGINEMBED=\"YES\"") );
    params.append( QString::fromLatin1("__KHTML__PLUGINBASEURL=\"%1\"").arg(document()->baseURL().url()));
    params.append( QString::fromLatin1("__KHTML__PLUGINPAGEURL=\"%1\"").arg(document()->URL().url()));

    // Non-embed elements parse a bunch of attributes and inherit things off <embed>, if any
    HTMLEmbedElementImpl* embed = relevantEmbed();
    if (id() != ID_EMBED) {
        params.append( QString::fromLatin1("__KHTML__CLASSID=\"%1\"").arg( classId ) );
        params.append( QString::fromLatin1("__KHTML__CODEBASE=\"%1\"").arg( getAttribute(ATTR_CODEBASE).string() ) );

        if (embed && !embed->getAttribute(ATTR_WIDTH).isEmpty())
            setAttribute(ATTR_WIDTH, embed->getAttribute(ATTR_WIDTH));

        if (embed && !embed->getAttribute(ATTR_HEIGHT).isEmpty())
            setAttribute(ATTR_HEIGHT, embed->getAttribute(ATTR_HEIGHT));

        if (!getAttribute(ATTR_WIDTH).isEmpty())
            params.append( QString::fromLatin1("WIDTH=\"%1\"").arg( getAttribute(ATTR_WIDTH).string() ) );

        if (!getAttribute(ATTR_HEIGHT).isEmpty())
            params.append( QString::fromLatin1("HEIGHT=\"%1\"").arg( getAttribute(ATTR_HEIGHT).string() ) );

        // Fix up the serviceType from embed, or applet info..
        if (embed) {
            effectiveURL = embed->url;
            if (!embed->serviceType.isEmpty())
                effectiveServiceType = embed->serviceType;
        } else if (effectiveURL.isEmpty() &&
                   classId.startsWith(QLatin1String("java:"))) {
            effectiveServiceType = "application/x-java-applet";
            effectiveURL         = classId.mid(5);
        }

        // Translate ActiveX gibberish into mimetypes
        if ((effectiveServiceType.isEmpty() || serviceType == "application/x-oleobject") && !classId.isEmpty()) {
            if(classId.indexOf(QString::fromLatin1("D27CDB6E-AE6D-11cf-96B8-444553540000")) >= 0)
                effectiveServiceType = "application/x-shockwave-flash";
            else if(classId.indexOf(QLatin1String("CFCDAA03-8BE4-11cf-B84B-0020AFBBCCFA")) >= 0)
                effectiveServiceType = "audio/x-pn-realaudio-plugin";
            else if(classId.indexOf(QLatin1String("8AD9C840-044E-11D1-B3E9-00805F499D93")) >= 0 ||
                    classId.indexOf(QLatin1String("CAFEEFAC-0014-0000-0000-ABCDEFFEDCBA")) >= 0)
                effectiveServiceType = "application/x-java-applet";
            // http://www.apple.com/quicktime/tools_tips/tutorials/activex.html
            else if(classId.indexOf(QLatin1String("02BF25D5-8C17-4B23-BC80-D3488ABDDC6B")) >= 0)
                effectiveServiceType = "video/quicktime";
            // http://msdn.microsoft.com/library/en-us/dnwmt/html/adding_windows_media_to_web_pages__etse.asp?frame=true
            else if(classId.indexOf(QString::fromLatin1("6BF52A52-394A-11d3-B153-00C04F79FAA6")) >= 0 ||
                    classId.indexOf(QString::fromLatin1("22D6f312-B0F6-11D0-94AB-0080C74C7E95")) >= 0)
                effectiveServiceType = "video/x-msvideo";
            else {
                // qDebug() << "ActiveX classId " << classId;
            }
              // TODO: add more plugins here
        }
    }

    if (effectiveServiceType.isEmpty() &&
        effectiveURL.startsWith(QLatin1String("data:"))) {
        // Extract the MIME type from the data URL.
        int index = effectiveURL.indexOf(';');
        if (index == -1)
            index = effectiveURL.indexOf(',');
        if (index != -1) {
            int len = index - 5;
            if (len > 0)
                effectiveServiceType = effectiveURL.mid(5, len);
            else
                effectiveServiceType = "text/plain"; // Data URLs with no MIME type are considered text/plain.
        }
    }

    // Figure out if may be we're image-like. In this case, we don't need to load anything,
    // but may need to do a detach/attach
    QStringList supportedImageTypes = khtmlImLoad::ImageManager::loaderDatabase()->supportedMimeTypes();

    bool newImageLike = effectiveServiceType.startsWith(QLatin1String("image/")) && supportedImageTypes.contains(effectiveServiceType);

    if (newImageLike != m_imageLike) {
        m_imageLike = newImageLike;
        requestRerender();
    }

    if (m_imageLike)
        return;

    // Now see if we have to render alternate content.
    bool newRenderAlternative = false;

    // If we aren't permitted to load this by security policy, render alternative content instead.
    if (!document()->isURLAllowed(effectiveURL))
        newRenderAlternative = true;

    // If Java is off, render alternative as well...
    if (effectiveServiceType == "application/x-java-applet") {
        KHTMLPart* p = document()->part();
        if (!p || !p->javaEnabled())
            newRenderAlternative = true;
    }

    // If there is no <embed> (here or as a child), and we don't have a type + url to go on,
    // we need to render alternative as well
    if (!embed && effectiveURL.isEmpty() && effectiveServiceType.isEmpty())
        newRenderAlternative = true;

    if (newRenderAlternative != m_renderAlternative) {
        m_renderAlternative = newRenderAlternative;
        requestRerender();
    }

    if (m_renderAlternative)
        return;

    KHTMLPart* part = document()->part();
    clearChildWidget();

    // qDebug() << effectiveURL << effectiveServiceType << params;

    if (!part->loadObjectElement( this, effectiveURL, effectiveServiceType, params)) {
        // Looks like we are gonna need alternative content after all...
        m_renderAlternative = true;
    }

    // Either way, we need to re-attach, either for alternative content, or because
    // we got the part..
    requestRerender();
}

void HTMLObjectBaseElementImpl::setWidgetNotify(QWidget *widget)
{
    // Ick.
    if(m_render && strcmp( m_render->renderName(),  "RenderPartObject" ) == 0 )
        static_cast<RenderPartObject*>(m_render)->setWidget(widget);
}

void HTMLObjectBaseElementImpl::renderAlternative()
{
    if (m_renderAlternative)
        return;

    m_renderAlternative = true;
    requestRerender();
}

void HTMLObjectBaseElementImpl::partLoadingErrorNotify()
{
    // Defer ourselves from the current event loop (to prevent crashes due to the message box staying up)
    QTimer::singleShot(0, this, SLOT(slotPartLoadingErrorNotify()));

    // Either way, we don't have stuff to display, so have to render alternative content.
    if (!m_renderAlternative) {
        m_renderAlternative = true;
        requestRerender();
    }

    clearChildWidget();
}

void HTMLObjectBaseElementImpl::slotPartLoadingErrorNotify()
{
    // If we have an embed, we may be able to tell the user where to
    // download the plugin.

    HTMLEmbedElementImpl *embed = relevantEmbed();
    QString serviceType; // shadows ours, but we don't care.

    if (!embed)
        return;

    serviceType = embed->serviceType;

    KHTMLPart* part = document()->part();
    KParts::BrowserExtension *ext = part->browserExtension();

    if(!embed->pluginPage.isEmpty() && ext) {
        // Prepare the mimetype to show in the question (comment if available, name as fallback)
        QString mimeName = serviceType;
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForName(serviceType);
        if (mime.isValid())
            mimeName = mime.comment();

        // Check if we already asked the user, for this page
        if (!mimeName.isEmpty() && !part->pluginPageQuestionAsked(serviceType))
        {
            part->setPluginPageQuestionAsked(serviceType);

            // Prepare the URL to show in the question (host only if http, to make it short)
            QUrl pluginPageURL(embed->pluginPage);
            QString shortURL = pluginPageURL.scheme() == "http" ? pluginPageURL.host() : pluginPageURL.toDisplayString();
            int res = KMessageBox::questionYesNo( part->view(),
                                                  i18n("No plugin found for '%1'.\nDo you want to download one from %2?", mimeName, shortURL),
                                                  i18n("Missing Plugin"), KGuiItem(i18n("Download")), KGuiItem(i18n("Do Not Download")), QString("plugin-")+serviceType);
            if (res == KMessageBox::Yes)
            {
                // Display vendor download page
                ext->createNewWindow(pluginPageURL);
                return;
            }
        }
    }
}


// -------------------------------------------------------------------------

HTMLAppletElementImpl::HTMLAppletElementImpl(DocumentImpl *doc)
  : HTMLObjectBaseElementImpl(doc)
{
    serviceType = "application/x-java-applet";
}

HTMLAppletElementImpl::~HTMLAppletElementImpl()
{
}

NodeImpl::Id HTMLAppletElementImpl::id() const
{
    return ID_APPLET;
}

void HTMLAppletElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch( attr->id() )
    {
    case ATTR_CODEBASE:
    case ATTR_ARCHIVE:
    case ATTR_CODE:
    case ATTR_OBJECT:
    case ATTR_ALT:
        break;
    case ATTR_ALIGN:
	addHTMLAlignment( attr->value() );
	break;
    case ATTR_VSPACE:
        addCSSLength(CSS_PROP_MARGIN_TOP, attr->value());
        addCSSLength(CSS_PROP_MARGIN_BOTTOM, attr->value());
        break;
    case ATTR_HSPACE:
        addCSSLength(CSS_PROP_MARGIN_LEFT, attr->value());
        addCSSLength(CSS_PROP_MARGIN_RIGHT, attr->value());
        break;
    case ATTR_VALIGN:
        addCSSProperty(CSS_PROP_VERTICAL_ALIGN, attr->value().lower() );
        break;
    default:
        HTMLObjectBaseElementImpl::parseAttribute(attr);
    }
}

void HTMLAppletElementImpl::computeContent()
{
    DOMString codeBase = getAttribute( ATTR_CODEBASE );
    DOMString code = getAttribute( ATTR_CODE );
    if ( !codeBase.isEmpty() )
        url = codeBase.string();
    if ( !code.isEmpty() )
        url = code.string();
    HTMLObjectBaseElementImpl::computeContent();
}

// -------------------------------------------------------------------------

HTMLEmbedElementImpl::HTMLEmbedElementImpl(DocumentImpl *doc)
    : HTMLObjectBaseElementImpl(doc)
{
}

HTMLEmbedElementImpl::~HTMLEmbedElementImpl()
{
}

NodeImpl::Id HTMLEmbedElementImpl::id() const
{
    return ID_EMBED;
}

HTMLEmbedElementImpl* HTMLEmbedElementImpl::relevantEmbed()
{
    return this;
}

void HTMLEmbedElementImpl::parseAttribute(AttributeImpl *attr)
{
  switch ( attr->id() )
  {
     case ATTR_CODE:
     case ATTR_SRC:
         url = khtml::parseURL(attr->val()).string();
         setNeedComputeContent();
         break;
     case ATTR_BORDER:
        addCSSLength(CSS_PROP_BORDER_WIDTH, attr->value());
        addCSSProperty( CSS_PROP_BORDER_TOP_STYLE, CSS_VAL_SOLID );
        addCSSProperty( CSS_PROP_BORDER_RIGHT_STYLE, CSS_VAL_SOLID );
        addCSSProperty( CSS_PROP_BORDER_BOTTOM_STYLE, CSS_VAL_SOLID );
        addCSSProperty( CSS_PROP_BORDER_LEFT_STYLE, CSS_VAL_SOLID );
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
        addCSSProperty(CSS_PROP_VERTICAL_ALIGN, attr->value().lower() );
        break;
     case ATTR_PLUGINPAGE:
     case ATTR_PLUGINSPAGE: {
        pluginPage = attr->value().string();
        break;
      }
     case ATTR_HIDDEN:
        if (strcasecmp( attr->value(), "yes" ) == 0 || strcasecmp( attr->value() , "true") == 0 )
           hidden = true;
        else
           hidden = false;
        break;
     default:
        HTMLObjectBaseElementImpl::parseAttribute( attr );
  }
}

void HTMLEmbedElementImpl::attach()
{
    if (parentNode()->id() == ID_OBJECT)
        NodeBaseImpl::attach();
    else
        HTMLObjectBaseElementImpl::attach();
}

void HTMLEmbedElementImpl::computeContent()
{
    if (parentNode()->id() != ID_OBJECT)
        HTMLObjectBaseElementImpl::computeContent();
}

// -------------------------------------------------------------------------

HTMLObjectElementImpl::HTMLObjectElementImpl(DocumentImpl *doc)
    : HTMLObjectBaseElementImpl(doc)
{
}

HTMLObjectElementImpl::~HTMLObjectElementImpl()
{
}

NodeImpl::Id HTMLObjectElementImpl::id() const
{
    return ID_OBJECT;
}

HTMLFormElementImpl *HTMLObjectElementImpl::form() const
{
  return 0;
}

void HTMLObjectElementImpl::parseAttribute(AttributeImpl *attr)
{
  switch ( attr->id() )
  {
    case ATTR_DATA:
      url = khtml::parseURL( attr->val() ).string();
      setNeedComputeContent();
      break;
    case ATTR_CLASSID:
      classId = attr->value().string();
      setNeedComputeContent();
      break;
    case ATTR_ONLOAD: // ### support load/unload on object elements
        setHTMLEventListener(EventImpl::LOAD_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onload", this));
        break;
    case ATTR_ONUNLOAD:
        setHTMLEventListener(EventImpl::UNLOAD_EVENT,
	    document()->createHTMLEventListener(attr->value().string(), "onunload", this));
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
        addCSSProperty(CSS_PROP_VERTICAL_ALIGN, attr->value().lower() );
        break;
    default:
      HTMLObjectBaseElementImpl::parseAttribute( attr );
  }
}

DocumentImpl* HTMLObjectElementImpl::contentDocument() const
{
    QWidget* widget = childWidget();
    if( widget && qobject_cast<KHTMLView*>( widget ) )
        return static_cast<KHTMLView*>( widget )->part()->xmlDocImpl();
    return 0;
}

void HTMLObjectElementImpl::attach()
{
    HTMLObjectBaseElementImpl::attach();
}

// -------------------------------------------------------------------------

NodeImpl::Id HTMLParamElementImpl::id() const
{
    return ID_PARAM;
}

void HTMLParamElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch( attr->id() )
    {
    case ATTR_VALUE:
        m_value = attr->value().string();
        break;
    case ATTR_ID:
        if (document()->htmlMode() != DocumentImpl::XHtml) break;
        // fall through
    case ATTR_NAME:
        m_name = attr->value().string();
        // fall through
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
