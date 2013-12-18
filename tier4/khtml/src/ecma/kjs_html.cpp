// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001-2003 David Faure (faure@kde.org)
 *  Copyright (C) 2004 Apple Computer, Inc.
 *  Copyright (C) 2005 Maksim Orlovich (maksim@kde.org)
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

#include "kjs_html.h"
#include "kjs_html.lut.h"

#include <misc/loader.h>
#include <html/html_blockimpl.h>
#include <html/html_headimpl.h>
#include <html/html_imageimpl.h>
#include <html/html_inlineimpl.h>
#include <html/html_listimpl.h>
#include <html/html_tableimpl.h>
#include <html/html_objectimpl.h>
#include <html/html_canvasimpl.h>
#include <dom/dom_exception.h>

#include <css/csshelper.h> // for parseUrl

#include <html/html_baseimpl.h>
#include <html/html_documentimpl.h>
#include <html/html_formimpl.h>
#include <html/html_miscimpl.h>
#include <xml/dom2_eventsimpl.h>

#include <kparts/browserextension.h>

#include <khtml_part.h>
#include <khtmlview.h>

#include "kjs_css.h"
#include "kjs_events.h"
#include "kjs_window.h"
#include "kjs_context2d.h"
#include <kjs/PropertyNameArray.h>

#include <rendering/render_object.h>
#include <rendering/render_canvas.h>
#include <rendering/render_frames.h>
#include <rendering/render_layer.h>

#include <kmessagebox.h>
#include <kstringhandler.h>
#include <klocalizedstring.h>

#include <QDebug>
#include <QtCore/QList>
#include <QtCore/QHash>

using namespace DOM;

namespace KJS {

KJS_DEFINE_PROTOTYPE(HTMLDocumentProto)
KJS_IMPLEMENT_PROTOFUNC(HTMLDocFunction)
KJS_IMPLEMENT_PROTOTYPE("HTMLDocument", HTMLDocumentProto, HTMLDocFunction, DOMDocumentProto)

IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLDocumentPseudoCtor, "HTMLDocument", HTMLDocumentProto)

/* Source for HTMLDocumentProtoTable.
@begin HTMLDocumentProtoTable 11
  clear         HTMLDocument::Clear     DontDelete|Function 0
  open          HTMLDocument::Open      DontDelete|Function 0
  close         HTMLDocument::Close     DontDelete|Function 0
  write         HTMLDocument::Write     DontDelete|Function 1
  writeln       HTMLDocument::WriteLn       DontDelete|Function 1
  getElementsByName HTMLDocument::GetElementsByName DontDelete|Function 1
  getSelection  HTMLDocument::GetSelection  DontDelete|Function 1
  captureEvents     HTMLDocument::CaptureEvents DontDelete|Function 0
  releaseEvents     HTMLDocument::ReleaseEvents DontDelete|Function 0
@end
*/


JSValue* KJS::HTMLDocFunction::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( HTMLDocument, thisObj );

  DOM::HTMLDocumentImpl& doc = *static_cast<KJS::HTMLDocument *>(thisObj)->impl();

  switch (id) {
  case HTMLDocument::Clear: // even IE doesn't support that one...
    //doc.clear(); // TODO
    return jsUndefined();
  case HTMLDocument::Open:
    if (args.size() >= 3) // IE extension for document.open: it means window.open if it has 3 args or more
    {
      KHTMLPart* part = doc.part();
      if ( part ) {
        Window* win = Window::retrieveWindow(part);
        if( win ) {
          win->openWindow(exec, args);
        }
      }
    }

    doc.open();
    return jsUndefined();
  case HTMLDocument::Close:
    // see khtmltests/ecma/tokenizer-script-recursion.html
    doc.close();
    return jsUndefined();
  case HTMLDocument::Write:
  case HTMLDocument::WriteLn: {
    // DOM only specifies single string argument, but NS & IE allow multiple
    // or no arguments
    UString str = "";
    for (int i = 0; i < args.size(); i++)
      str += args[i]->toString(exec);
    if (id == HTMLDocument::WriteLn)
      str += "\n";
#ifdef KJS_VERBOSE
    qDebug() << "document.write: " << str.qstring();
#endif
    doc.write(str.qstring());
    return jsUndefined();
  }
  case HTMLDocument::GetElementsByName:
    return getDOMNodeList(exec,doc.getElementsByName(args[0]->toString(exec).domString()));
  case HTMLDocument::GetSelection: {
    // NS4 and Mozilla specific. IE uses document.selection.createRange()
    // http://docs.sun.com/source/816-6408-10/document.htm#1195981
    KHTMLPart *part = doc.part();
    if ( part )
       return jsString(part->selectedText());
    else
       return jsUndefined();
  }
  case HTMLDocument::CaptureEvents:
  case HTMLDocument::ReleaseEvents:
    // Do nothing for now. These are NS-specific legacy calls.
    break;
  }

  return jsUndefined();
}

const ClassInfo KJS::HTMLDocument::info =
  { "HTMLDocument", &DOMDocument::info, &HTMLDocumentTable, 0 };
/* Source for HTMLDocumentTable.
@begin HTMLDocumentTable 31
  referrer		HTMLDocument::Referrer		DontDelete|ReadOnly
  domain		HTMLDocument::Domain		DontDelete
  URL			HTMLDocument::URL		DontDelete|ReadOnly
  body			HTMLDocument::Body		DontDelete
  location		HTMLDocument::Location		DontDelete
  cookie		HTMLDocument::Cookie		DontDelete
  images		HTMLDocument::Images		DontDelete|ReadOnly
  applets		HTMLDocument::Applets		DontDelete|ReadOnly
  links			HTMLDocument::Links		DontDelete|ReadOnly
  forms			HTMLDocument::Forms		DontDelete|ReadOnly
  anchors		HTMLDocument::Anchors		DontDelete|ReadOnly
  scripts		HTMLDocument::Scripts		DontDelete|ReadOnly
  all			HTMLDocument::All		DontDelete|ReadOnly
  bgColor		HTMLDocument::BgColor		DontDelete
  fgColor		HTMLDocument::FgColor		DontDelete
  alinkColor		HTMLDocument::AlinkColor	DontDelete
  linkColor		HTMLDocument::LinkColor		DontDelete
  vlinkColor		HTMLDocument::VlinkColor	DontDelete
  lastModified		HTMLDocument::LastModified	DontDelete|ReadOnly
  height		HTMLDocument::Height		DontDelete|ReadOnly
  width			HTMLDocument::Width		DontDelete|ReadOnly
  dir			HTMLDocument::Dir		DontDelete
  compatMode		HTMLDocument::CompatMode	DontDelete|ReadOnly
  designMode            HTMLDocument::DesignMode        DontDelete
#IE extension
  frames		HTMLDocument::Frames		DontDelete|ReadOnly
#NS4 extension
  layers		HTMLDocument::Layers		DontDelete|ReadOnly
# HTML5
  activeElement         HTMLDocument::ActiveElement     DontDelete|ReadOnly
#potentially obsolete array properties
# plugins
# tags
#potentially obsolete properties
# embeds
# ids
@end
*/

KJS::HTMLDocument::HTMLDocument(ExecState *exec, DOM::HTMLDocumentImpl* d)
  : DOMDocument(HTMLDocumentProto::self(exec), d) { }

/* Should this property be checked after overrides? */
static bool isLateProperty(unsigned token)
{
  switch (token) {
    case HTMLDocument::BgColor:
    case HTMLDocument::FgColor:
    case HTMLDocument::AlinkColor:
    case HTMLDocument::LinkColor:
    case HTMLDocument::VlinkColor:
    case HTMLDocument::LastModified:
    case HTMLDocument::Height: // NS-only, not available in IE
    case HTMLDocument::Width: // NS-only, not available in IE
    case HTMLDocument::Dir:
    case HTMLDocument::Frames:
      return true;
    default:
      return false;
  }
}

bool KJS::HTMLDocument::getOwnPropertySlot(ExecState *exec, const Identifier &propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLDocument::getOwnPropertySlot " << propertyName.qstring();
#endif

  DOM::DocumentImpl* docImpl = impl();
  KHTMLPart *part = docImpl->part();

  Window* win = part ? Window::retrieveWindow(part) : 0L;
  if ( !win || !win->isSafeScript(exec) ) {
    slot.setUndefined(this);
    return true;
  }

  DOM::DOMString  propertyDOMString = propertyName.domString();

  //See whether to return named items under document.
  ElementMappingCache::ItemInfo* info = docImpl->underDocNamedCache().get(propertyDOMString);
  if (info) {
    //May be a false positive, but we can try to avoid doing it the hard way in
    //simpler cases. The trickiness here is that the cache is kept under both
    //name and id, but we sometimes ignore id for IE compat

    bool matched = false;
    if (info->nd && DOM::HTMLMappedNameCollectionImpl::matchesName(info->nd,
                              HTMLCollectionImpl::DOCUMENT_NAMED_ITEMS, propertyDOMString)) {
        matched = true;
    } else {
        //Can't tell it just like that, so better go through collection and count stuff. This is the slow path...
        DOM::HTMLMappedNameCollectionImpl coll(impl(), HTMLCollectionImpl::DOCUMENT_NAMED_ITEMS, propertyDOMString);
        matched = coll.firstItem() != 0;
    }

    if (matched) {
        slot.setCustom(this, nameGetter);
        return true;
    }
  }

  // Check for frames/iframes with name==propertyName
  if ( part ) {
    if (part->findFrame( propertyName.qstring() )) {
      slot.setCustom(this, frameNameGetter);
      return true;
    }
  }

  // Static properties
  const HashEntry* entry = Lookup::findEntry(&HTMLDocumentTable, propertyName);
  if (entry && !isLateProperty(entry->value)) {
    getSlotFromEntry<HTMLDocFunction, HTMLDocument>(entry, this, slot);
    return true;
  }

  // Look for overrides
  JSValue **val = getDirectLocation(propertyName);
  if (val) {
    fillDirectLocationSlot(slot, val);
    return true;
  }

  // The rest of static properties -- the late ones.
  if (entry) {
    getSlotFromEntry<HTMLDocFunction, HTMLDocument>(entry, this, slot);
    return true;
  }

  return DOMDocument::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue *HTMLDocument::nameGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
  HTMLDocument *thisObj = static_cast<HTMLDocument*>(slot.slotBase());
  DOM::DocumentImpl* docImpl = thisObj->impl();

  //Return named items under document (e.g. images, applets, etc.)
  DOMString name = propertyName.domString();
  ElementMappingCache::ItemInfo* info = docImpl->underDocNamedCache().get(name);
  if (info && info->nd)
    return getDOMNode(exec, info->nd);
  else {
    //No cached mapping, do it the hard way..
    DOM::HTMLMappedNameCollectionImpl* coll = new DOM::HTMLMappedNameCollectionImpl(docImpl,
                                        HTMLCollectionImpl::DOCUMENT_NAMED_ITEMS, name);

    if (info && coll->length() == 1) {
        info->nd = static_cast<DOM::ElementImpl*>(coll->firstItem());
        delete coll;
        return getDOMNode(exec, info->nd);
    }

    return getHTMLCollection(exec, coll);
  }

  assert(0);
  return jsUndefined();
}

JSValue *HTMLDocument::frameNameGetter(ExecState*, JSObject*, const Identifier& name, const PropertySlot& slot)
{
  HTMLDocument *thisObj = static_cast<HTMLDocument*>(slot.slotBase());
  // Check for frames/iframes with name==propertyName
  return Window::retrieve(thisObj->impl()->part()->findFrame( name.qstring() ));
}

JSValue *HTMLDocument::objectNameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot)
{
  HTMLDocument *thisObj = static_cast<HTMLDocument*>(slot.slotBase());
  DOM::HTMLCollectionImpl objectLike(thisObj->impl(), DOM::HTMLCollectionImpl::DOC_APPLETS);
  return getDOMNode(exec, objectLike.namedItem(name.domString()));
}

JSValue *HTMLDocument::layerNameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot)
{
  HTMLDocument *thisObj = static_cast<HTMLDocument*>(slot.slotBase());
  DOM::HTMLCollectionImpl layerLike(thisObj->impl(), DOM::HTMLCollectionImpl::DOC_LAYERS);
  return getDOMNode(exec, layerLike.namedItem(name.domString()));
}

JSValue* HTMLDocument::getValueProperty(ExecState *exec, int token)
{
  DOM::HTMLDocumentImpl& doc = *impl();
  KHTMLView* view = doc.view();
  KHTMLPart* part = doc.part();
  Window* win = part ? Window::retrieveWindow(part) : 0L;
  DOM::HTMLElementImpl* body = doc.body();

  switch (token) {
    case Referrer:
      return jsString(doc.referrer());
    case Domain:
      return jsString(doc.domain());
    case URL:
      return jsString(doc.URL().url());
    case Body:
      return getDOMNode(exec,doc.body());
    case Location:
      if (win)
        return win->location();
      else
        return jsUndefined();
    case Cookie:
      return jsString(doc.cookie());
    case Images:
      return getHTMLCollection(exec,doc.images());
    case Applets:
      return getHTMLCollection(exec,doc.applets());
    case Links:
      return getHTMLCollection(exec,doc.links());
    case Forms:
      return getHTMLCollection(exec,doc.forms());
    case Layers:
      // ### Should not be hidden when we emulate Netscape4
      return getHTMLCollection(exec,doc.layers(), true);
    case Anchors:
      return getHTMLCollection(exec,doc.anchors());
    case Scripts:
      return getHTMLCollection(exec,doc.scripts());
    case All:
      // Disable document.all when we try to be Netscape-compatible
      if ( exec->dynamicInterpreter()->compatMode() == Interpreter::NetscapeCompat )
        return jsUndefined();
      else
      if ( exec->dynamicInterpreter()->compatMode() == Interpreter::IECompat )
        return getHTMLCollection(exec,doc.all());
      else // enabled but hidden
        return getHTMLCollection(exec,doc.all(), true);
    case CompatMode:
      return jsString(doc.parseMode()
              == DocumentImpl::Compat ? "BackCompat" : "CSS1Compat");
    case DesignMode:
        return jsString((doc.designMode() ? "on":"off"));
    case ActiveElement:
	return getDOMNode(exec, doc.activeElement());
    case BgColor:
      return jsString(body ? body->getAttribute(ATTR_BGCOLOR) : DOMString());
    case FgColor:
      return jsString(body ? body->getAttribute(ATTR_TEXT) : DOMString());
    case AlinkColor:
      return jsString(body ? body->getAttribute(ATTR_ALINK) : DOMString());
    case LinkColor:
      return jsString(body ? body->getAttribute(ATTR_LINK) : DOMString());
    case VlinkColor:
      return jsString(body ? body->getAttribute(ATTR_VLINK) : DOMString());
    case LastModified:
      return jsString(doc.lastModified());
    case Height: // NS-only, not available in IE
      return jsNumber(view ? view->contentsHeight() : 0);
    case Width: // NS-only, not available in IE
      return jsNumber(view ? view->contentsWidth() : 0);
    case Dir:
      return body ? jsString(body->getAttribute(ATTR_DIR)) : jsUndefined();
    case Frames:
      if ( win )
        return win;
      else
        return jsUndefined();
  }
  assert(0);
  return 0;
}

void KJS::HTMLDocument::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr)
{
#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLDocument::out " << propertyName.qstring();
#endif
  KHTMLPart* part = impl()->part();
  Window* win = part ? Window::retrieveWindow(part) : 0L;
  if ( !win || !win->isSafeScript(exec) )
    return;

  lookupPut<HTMLDocument, DOMDocument>( exec, propertyName, value, attr, &HTMLDocumentTable, this );
}

void KJS::HTMLDocument::putValueProperty(ExecState *exec, int token, JSValue *value, int /*attr*/)
{
  DOM::HTMLDocumentImpl& doc = *impl();
  DOM::DOMString val = value->toString(exec).domString();
  DOMExceptionTranslator exception(exec);

  switch (token) {
    case Body: {
      DOM::NodeImpl* body = toNode(value);
      if (body && body->isHTMLElement())
        doc.setBody(static_cast<DOM::HTMLElementImpl*>(body), exception);
      return;
    }
    case Domain: { // not part of the DOM
      doc.setDomain(val);
      return;
    }
    case Cookie:
      doc.setCookie(val);
      return;
    case Location:
    {
      KHTMLPart *part = doc.part();
      if ( part )
        Window::retrieveWindow(part)->goURL(exec, value->toString(exec).qstring(), false /*don't lock history*/);
      return;
    }
    case DesignMode:
        doc.setDesignMode((value->toString(exec).qstring().toLower()=="on"));
        return;
  }

  /* The rest of the properties require a body. Note that Doc::body may be the
     frameset(!!) so we have to be a bit careful here. I am not sure this is
     100% right, but it should match previous behavior - M.O. */
  DOM::HTMLElementImpl* bodyCand = doc.body();
  if (!bodyCand || bodyCand->id() != ID_BODY)
    return; //Just ignore.

  DOM::HTMLBodyElementImpl& body = *static_cast<DOM::HTMLBodyElementImpl*>(bodyCand);

  switch (token) {
  case BgColor:
    if (body.bgColor() != val) body.setBgColor(val);
    break;
  case FgColor:
    if (body.text() != val) body.setText(val);
    break;
  case AlinkColor:
    if (body.aLink() != val) body.setALink(val);
    break;
  case LinkColor:
    if (body.link() != val) body.setLink(val);
    break;
  case VlinkColor:
    if (body.vLink() != val) body.setVLink(val);
    break;
  case Dir:
    body.setAttribute(ID_DIR, value->toString(exec).domString());
    break;
  default:
    // qDebug() << "WARNING: HTMLDocument::putValueProperty unhandled token " << token;
    break;
  }
}

// -------------------------------------------------------------------------

const ClassInfo KJS::HTMLElement::info = { "HTMLElement", &DOMElement::info, &HTMLElementTable, 0 };
const ClassInfo KJS::HTMLElement::html_info = { "HTMLHtmlElement", &KJS::HTMLElement::info, &HTMLHtmlElementTable, 0 };
const ClassInfo KJS::HTMLElement::head_info = { "HTMLHeadElement", &KJS::HTMLElement::info, &HTMLHeadElementTable, 0 };
const ClassInfo KJS::HTMLElement::link_info = { "HTMLLinkElement", &KJS::HTMLElement::info, &HTMLLinkElementTable, 0 };
const ClassInfo KJS::HTMLElement::title_info = { "HTMLTitleElement", &KJS::HTMLElement::info, &HTMLTitleElementTable, 0 };
const ClassInfo KJS::HTMLElement::meta_info = { "HTMLMetaElement", &KJS::HTMLElement::info, &HTMLMetaElementTable, 0 };
const ClassInfo KJS::HTMLElement::base_info = { "HTMLBaseElement", &KJS::HTMLElement::info, &HTMLBaseElementTable, 0 };
const ClassInfo KJS::HTMLElement::isIndex_info = { "HTMLIsIndexElement", &KJS::HTMLElement::info, &HTMLIsIndexElementTable, 0 };
const ClassInfo KJS::HTMLElement::style_info = { "HTMLStyleElement", &KJS::HTMLElement::info, &HTMLStyleElementTable, 0 };
const ClassInfo KJS::HTMLElement::body_info = { "HTMLBodyElement", &KJS::HTMLElement::info, &HTMLBodyElementTable, 0 };
const ClassInfo KJS::HTMLElement::form_info = { "HTMLFormElement", &KJS::HTMLElement::info, &HTMLFormElementTable, 0 };
const ClassInfo KJS::HTMLElement::select_info = { "HTMLSelectElement", &KJS::HTMLElement::info, &HTMLSelectElementTable, 0 };
const ClassInfo KJS::HTMLElement::optGroup_info = { "HTMLOptGroupElement", &KJS::HTMLElement::info, &HTMLOptGroupElementTable, 0 };
const ClassInfo KJS::HTMLElement::option_info = { "HTMLOptionElement", &KJS::HTMLElement::info, &HTMLOptionElementTable, 0 };
const ClassInfo KJS::HTMLElement::input_info = { "HTMLInputElement", &KJS::HTMLElement::info, &HTMLInputElementTable, 0 };
const ClassInfo KJS::HTMLElement::textArea_info = { "HTMLTextAreaElement", &KJS::HTMLElement::info, &HTMLTextAreaElementTable, 0 };
const ClassInfo KJS::HTMLElement::button_info = { "HTMLButtonElement", &KJS::HTMLElement::info, &HTMLButtonElementTable, 0 };
const ClassInfo KJS::HTMLElement::label_info = { "HTMLLabelElement", &KJS::HTMLElement::info, &HTMLLabelElementTable, 0 };
const ClassInfo KJS::HTMLElement::fieldSet_info = { "HTMLFieldSetElement", &KJS::HTMLElement::info, &HTMLFieldSetElementTable, 0 };
const ClassInfo KJS::HTMLElement::legend_info = { "HTMLLegendElement", &KJS::HTMLElement::info, &HTMLLegendElementTable, 0 };
const ClassInfo KJS::HTMLElement::ul_info = { "HTMLUListElement", &KJS::HTMLElement::info, &HTMLUListElementTable, 0 };
const ClassInfo KJS::HTMLElement::ol_info = { "HTMLOListElement", &KJS::HTMLElement::info, &HTMLOListElementTable, 0 };
const ClassInfo KJS::HTMLElement::dl_info = { "HTMLDListElement", &KJS::HTMLElement::info, &HTMLDListElementTable, 0 };
const ClassInfo KJS::HTMLElement::dir_info = { "HTMLDirectoryElement", &KJS::HTMLElement::info, &HTMLDirectoryElementTable, 0 };
const ClassInfo KJS::HTMLElement::menu_info = { "HTMLMenuElement", &KJS::HTMLElement::info, &HTMLMenuElementTable, 0 };
const ClassInfo KJS::HTMLElement::li_info = { "HTMLLIElement", &KJS::HTMLElement::info, &HTMLLIElementTable, 0 };
const ClassInfo KJS::HTMLElement::div_info = { "HTMLDivElement", &KJS::HTMLElement::info, &HTMLDivElementTable, 0 };
const ClassInfo KJS::HTMLElement::p_info = { "HTMLParagraphElement", &KJS::HTMLElement::info, &HTMLParagraphElementTable, 0 };
const ClassInfo KJS::HTMLElement::heading_info = { "HTMLHeadingElement", &KJS::HTMLElement::info, &HTMLHeadingElementTable, 0 };
const ClassInfo KJS::HTMLElement::blockQuote_info = { "HTMLBlockQuoteElement", &KJS::HTMLElement::info, &HTMLBlockQuoteElementTable, 0 };
const ClassInfo KJS::HTMLElement::q_info = { "HTMLQuoteElement", &KJS::HTMLElement::info, &HTMLQuoteElementTable, 0 };
const ClassInfo KJS::HTMLElement::pre_info = { "HTMLPreElement", &KJS::HTMLElement::info, &HTMLPreElementTable, 0 };
const ClassInfo KJS::HTMLElement::br_info = { "HTMLBRElement", &KJS::HTMLElement::info, &HTMLBRElementTable, 0 };
const ClassInfo KJS::HTMLElement::baseFont_info = { "HTMLBaseFontElement", &KJS::HTMLElement::info, &HTMLBaseFontElementTable, 0 };
const ClassInfo KJS::HTMLElement::font_info = { "HTMLFontElement", &KJS::HTMLElement::info, &HTMLFontElementTable, 0 };
const ClassInfo KJS::HTMLElement::hr_info = { "HTMLHRElement", &KJS::HTMLElement::info, &HTMLHRElementTable, 0 };
const ClassInfo KJS::HTMLElement::mod_info = { "HTMLModElement", &KJS::HTMLElement::info, &HTMLModElementTable, 0 };
const ClassInfo KJS::HTMLElement::a_info = { "HTMLAnchorElement", &KJS::HTMLElement::info, &HTMLAnchorElementTable, 0 };
const ClassInfo KJS::HTMLElement::canvas_info = { "HTMLCanvasElement", &KJS::HTMLElement::info, &HTMLCanvasElementTable, 0 };
const ClassInfo KJS::HTMLElement::img_info = { "HTMLImageElement", &KJS::HTMLElement::info, &HTMLImageElementTable, 0 };
const ClassInfo KJS::HTMLElement::object_info = { "HTMLObjectElement", &KJS::HTMLElement::info, &HTMLObjectElementTable, 0 };
const ClassInfo KJS::HTMLElement::param_info = { "HTMLParamElement", &KJS::HTMLElement::info, &HTMLParamElementTable, 0 };
const ClassInfo KJS::HTMLElement::applet_info = { "HTMLAppletElement", &KJS::HTMLElement::info, &HTMLAppletElementTable, 0 };
const ClassInfo KJS::HTMLElement::map_info = { "HTMLMapElement", &KJS::HTMLElement::info, &HTMLMapElementTable, 0 };
const ClassInfo KJS::HTMLElement::area_info = { "HTMLAreaElement", &KJS::HTMLElement::info, &HTMLAreaElementTable, 0 };
const ClassInfo KJS::HTMLElement::script_info = { "HTMLScriptElement", &KJS::HTMLElement::info, &HTMLScriptElementTable, 0 };
const ClassInfo KJS::HTMLElement::table_info = { "HTMLTableElement", &KJS::HTMLElement::info, &HTMLTableElementTable, 0 };
const ClassInfo KJS::HTMLElement::caption_info = { "HTMLTableCaptionElement", &KJS::HTMLElement::info, &HTMLTableCaptionElementTable, 0 };
const ClassInfo KJS::HTMLElement::col_info = { "HTMLTableColElement", &KJS::HTMLElement::info, &HTMLTableColElementTable, 0 };
const ClassInfo KJS::HTMLElement::tablesection_info = { "HTMLTableSectionElement", &KJS::HTMLElement::info, &HTMLTableSectionElementTable, 0 };
const ClassInfo KJS::HTMLElement::tr_info = { "HTMLTableRowElement", &KJS::HTMLElement::info, &HTMLTableRowElementTable, 0 };
const ClassInfo KJS::HTMLElement::tablecell_info = { "HTMLTableCellElement", &KJS::HTMLElement::info, &HTMLTableCellElementTable, 0 };
const ClassInfo KJS::HTMLElement::frameSet_info = { "HTMLFrameSetElement", &KJS::HTMLElement::info, &HTMLFrameSetElementTable, 0 };
const ClassInfo KJS::HTMLElement::frame_info = { "HTMLFrameElement", &KJS::HTMLElement::info, &HTMLFrameElementTable, 0 };
const ClassInfo KJS::HTMLElement::iFrame_info = { "HTMLIFrameElement", &KJS::HTMLElement::info, &HTMLIFrameElementTable, 0 };
const ClassInfo KJS::HTMLElement::marquee_info = { "HTMLMarqueeElement", &KJS::HTMLElement::info, 0, 0 };
const ClassInfo KJS::HTMLElement::layer_info = { "HTMLLayerElement", &KJS::HTMLElement::info, &HTMLLayerElementTable, 0 };

static JSObject* prototypeForID(ExecState* exec, DOM::NodeImpl::Id id);

KJS::HTMLElement::HTMLElement(ExecState *exec, DOM::HTMLElementImpl* e) :
        DOMElement(prototypeForID(exec, e->id()), e) { }

const ClassInfo* KJS::HTMLElement::classInfo() const
{
  DOM::HTMLElementImpl& element = *impl();
  switch (element.id()) {
  case ID_HTML:
    return &html_info;
  case ID_HEAD:
    return &head_info;
  case ID_LINK:
    return &link_info;
  case ID_TITLE:
    return &title_info;
  case ID_META:
    return &meta_info;
  case ID_BASE:
    return &base_info;
  case ID_ISINDEX:
    return &isIndex_info;
  case ID_STYLE:
    return &style_info;
  case ID_BODY:
    return &body_info;
  case ID_FORM:
    return &form_info;
  case ID_SELECT:
    return &select_info;
  case ID_OPTGROUP:
    return &optGroup_info;
  case ID_OPTION:
    return &option_info;
  case ID_INPUT:
    return &input_info;
  case ID_TEXTAREA:
    return &textArea_info;
  case ID_BUTTON:
    return &button_info;
  case ID_LABEL:
    return &label_info;
  case ID_FIELDSET:
    return &fieldSet_info;
  case ID_LEGEND:
    return &legend_info;
  case ID_UL:
    return &ul_info;
  case ID_OL:
    return &ol_info;
  case ID_DL:
    return &dl_info;
  case ID_DIR:
    return &dir_info;
  case ID_MENU:
    return &menu_info;
  case ID_LI:
    return &li_info;
  case ID_DIV:
    return &div_info;
  case ID_P:
    return &p_info;
  case ID_H1:
  case ID_H2:
  case ID_H3:
  case ID_H4:
  case ID_H5:
  case ID_H6:
    return &heading_info;
  case ID_BLOCKQUOTE:
    return &blockQuote_info;
  case ID_Q:
    return &q_info;
  case ID_PRE:
    return &pre_info;
  case ID_BR:
    return &br_info;
  case ID_BASEFONT:
    return &baseFont_info;
  case ID_FONT:
    return &font_info;
  case ID_HR:
    return &hr_info;
  case ID_INS:
  case ID_DEL:
    return &mod_info;
  case ID_A:
    return &a_info;
  case ID_IMG:
    return &img_info;
  case ID_CANVAS:
    return &canvas_info;
  case ID_OBJECT:
    return &object_info;
  case ID_PARAM:
    return &param_info;
  case ID_APPLET:
    return &applet_info;
  case ID_MAP:
    return &map_info;
  case ID_AREA:
    return &area_info;
  case ID_SCRIPT:
    return &script_info;
  case ID_TABLE:
    return &table_info;
  case ID_CAPTION:
    return &caption_info;
  case ID_COL:
  case ID_COLGROUP:
    return &col_info;
  case ID_THEAD:
  case ID_TBODY:
  case ID_TFOOT:
    return &tablesection_info;
  case ID_TR:
    return &tr_info;
  case ID_TH:
  case ID_TD:
    return &tablecell_info;
  case ID_FRAMESET:
    return &frameSet_info;
  case ID_FRAME:
    return &frame_info;
  case ID_IFRAME:
    return &iFrame_info;
  case ID_MARQUEE:
    return &marquee_info;
  case ID_LAYER:
    return &layer_info;
  default:
    return &info;
  }
}
/*
@begin HTMLElementTable 11
  id		KJS::HTMLElement::ElementId	DontDelete
  title		KJS::HTMLElement::ElementTitle	DontDelete
  lang		KJS::HTMLElement::ElementLang	DontDelete
  dir		KJS::HTMLElement::ElementDir	DontDelete
  className	KJS::HTMLElement::ElementClassName DontDelete
  innerHTML	KJS::HTMLElement::ElementInnerHTML DontDelete
  innerText	KJS::HTMLElement::ElementInnerText DontDelete
  document	KJS::HTMLElement::ElementDocument  DontDelete|ReadOnly
  tabIndex      KJS::HTMLElement::ElementTabIndex  DontDelete
# IE extension
  children	KJS::HTMLElement::ElementChildren  DontDelete|ReadOnly
  all           KJS::HTMLElement::ElementAll       DontDelete|ReadOnly
  contentEditable   KJS::HTMLElement::ElementContentEditable  DontDelete
  isContentEditable KJS::HTMLElement::ElementIsContentEditable  DontDelete|ReadOnly
@end
@begin HTMLElementProtoTable 1
  scrollIntoView KJS::HTMLElement::ElementScrollIntoView DontDelete|Function 0
@end
@begin HTMLHtmlElementTable 1
  version	KJS::HTMLElement::HtmlVersion	DontDelete
@end
@begin HTMLHeadElementTable 1
  profile	KJS::HTMLElement::HeadProfile	DontDelete
@end
@begin HTMLLinkElementTable 11
  disabled	KJS::HTMLElement::LinkDisabled	DontDelete
  charset	KJS::HTMLElement::LinkCharset	DontDelete
  href		KJS::HTMLElement::LinkHref	DontDelete
  hreflang	KJS::HTMLElement::LinkHrefLang	DontDelete
  media		KJS::HTMLElement::LinkMedia	DontDelete
  rel		KJS::HTMLElement::LinkRel      	DontDelete
  rev		KJS::HTMLElement::LinkRev	DontDelete
  target	KJS::HTMLElement::LinkTarget	DontDelete
  type		KJS::HTMLElement::LinkType	DontDelete
  sheet		KJS::HTMLElement::LinkSheet	DontDelete|ReadOnly
@end
@begin HTMLTitleElementTable 1
  text		KJS::HTMLElement::TitleText	DontDelete
@end
@begin HTMLMetaElementTable 4
  content	KJS::HTMLElement::MetaContent	DontDelete
  httpEquiv	KJS::HTMLElement::MetaHttpEquiv	DontDelete
  name		KJS::HTMLElement::MetaName	DontDelete
  scheme	KJS::HTMLElement::MetaScheme	DontDelete
@end
@begin HTMLBaseElementTable 2
  href		KJS::HTMLElement::BaseHref	DontDelete
  target	KJS::HTMLElement::BaseTarget	DontDelete
@end
@begin HTMLIsIndexElementTable 2
  form		KJS::HTMLElement::IsIndexForm	DontDelete|ReadOnly
  prompt	KJS::HTMLElement::IsIndexPrompt	DontDelete
@end
@begin HTMLStyleElementTable 4
  disabled	KJS::HTMLElement::StyleDisabled	DontDelete
  media		KJS::HTMLElement::StyleMedia	DontDelete
  type		KJS::HTMLElement::StyleType	DontDelete
  sheet		KJS::HTMLElement::StyleSheet	DontDelete|ReadOnly
@end
@begin HTMLBodyElementTable 12
  aLink		KJS::HTMLElement::BodyALink	DontDelete
  background	KJS::HTMLElement::BodyBackground	DontDelete
  bgColor	KJS::HTMLElement::BodyBgColor	DontDelete
  link		KJS::HTMLElement::BodyLink	DontDelete
  text		KJS::HTMLElement::BodyText	DontDelete
  vLink		KJS::HTMLElement::BodyVLink	DontDelete
# HTML5 specifies these as shadowing normal ones and forwarding to window
  onload        KJS::HTMLElement::BodyOnLoad        DontDelete
  onerror       KJS::HTMLElement::BodyOnError       DontDelete
  onfocus       KJS::HTMLElement::BodyOnFocus       DontDelete
  onblur        KJS::HTMLElement::BodyOnBlur        DontDelete
  onmessage     KJS::HTMLElement::BodyOnMessage     DontDelete
  onhashchange  KJS::HTMLElement::BodyOnHashChange  DontDelete
@end
@begin HTMLBodyElementProtoTable 2
# Even though we do blur/focus everywhere, we still handle body.focus()
# specially for now
  focus         KJS::HTMLElement::BodyFocus      DontDelete|Function 0
@end
@begin HTMLFormElementTable 11
# Also supported, by name/index
  elements	KJS::HTMLElement::FormElements	DontDelete|ReadOnly
  length	KJS::HTMLElement::FormLength	DontDelete|ReadOnly
  name		KJS::HTMLElement::FormName	DontDelete
  acceptCharset	KJS::HTMLElement::FormAcceptCharset	DontDelete
  action	KJS::HTMLElement::FormAction	DontDelete
  encoding	KJS::HTMLElement::FormEncType	DontDelete
  enctype	KJS::HTMLElement::FormEncType	DontDelete
  method	KJS::HTMLElement::FormMethod	DontDelete
  target	KJS::HTMLElement::FormTarget	DontDelete
@end
@begin HTMLFormElementProtoTable 2
  submit	KJS::HTMLElement::FormSubmit	DontDelete|Function 0
  reset		KJS::HTMLElement::FormReset	DontDelete|Function 0
@end
@begin HTMLSelectElementTable 11
# Also supported, by index
  type		KJS::HTMLElement::SelectType	DontDelete|ReadOnly
  selectedIndex	KJS::HTMLElement::SelectSelectedIndex	DontDelete
  value		KJS::HTMLElement::SelectValue	DontDelete
  length	KJS::HTMLElement::SelectLength	DontDelete
  form		KJS::HTMLElement::SelectForm	DontDelete|ReadOnly
  options	KJS::HTMLElement::SelectOptions	DontDelete|ReadOnly
  disabled	KJS::HTMLElement::SelectDisabled	DontDelete
  multiple	KJS::HTMLElement::SelectMultiple	DontDelete
  name		KJS::HTMLElement::SelectName	DontDelete
  size		KJS::HTMLElement::SelectSize	DontDelete
@end
@begin HTMLSelectElementProtoTable 4
  add		KJS::HTMLElement::SelectAdd	DontDelete|Function 2
  item		KJS::HTMLElement::SelectItem	DontDelete|Function 1
  remove	KJS::HTMLElement::SelectRemove	DontDelete|Function 1
@end
@begin HTMLOptGroupElementTable 2
  disabled	KJS::HTMLElement::OptGroupDisabled	DontDelete
  label		KJS::HTMLElement::OptGroupLabel		DontDelete
@end
@begin HTMLOptionElementTable 8
  form		KJS::HTMLElement::OptionForm		DontDelete|ReadOnly
  defaultSelected KJS::HTMLElement::OptionDefaultSelected	DontDelete
  text		KJS::HTMLElement::OptionText		DontDelete
  index		KJS::HTMLElement::OptionIndex		DontDelete|ReadOnly
  disabled	KJS::HTMLElement::OptionDisabled	DontDelete
  label		KJS::HTMLElement::OptionLabel		DontDelete
  selected	KJS::HTMLElement::OptionSelected	DontDelete
  value		KJS::HTMLElement::OptionValue		DontDelete
@end
@begin HTMLInputElementTable 25
  defaultValue	KJS::HTMLElement::InputDefaultValue	DontDelete
  defaultChecked KJS::HTMLElement::InputDefaultChecked	DontDelete
  form		KJS::HTMLElement::InputForm		DontDelete|ReadOnly
  accept	KJS::HTMLElement::InputAccept		DontDelete
  accessKey	KJS::HTMLElement::InputAccessKey	DontDelete
  align		KJS::HTMLElement::InputAlign		DontDelete
  alt		KJS::HTMLElement::InputAlt		DontDelete
  checked	KJS::HTMLElement::InputChecked		DontDelete
  indeterminate	KJS::HTMLElement::InputIndeterminate	DontDelete
  status	KJS::HTMLElement::InputChecked		DontDelete
  disabled	KJS::HTMLElement::InputDisabled		DontDelete
  maxLength	KJS::HTMLElement::InputMaxLength	DontDelete
  name		KJS::HTMLElement::InputName		DontDelete
  readOnly	KJS::HTMLElement::InputReadOnly		DontDelete
  size		KJS::HTMLElement::InputSize		DontDelete
  src		KJS::HTMLElement::InputSrc		DontDelete
  type		KJS::HTMLElement::InputType		DontDelete
  useMap	KJS::HTMLElement::InputUseMap		DontDelete
  value		KJS::HTMLElement::InputValue		DontDelete
  selectionStart KJS::HTMLElement::InputSelectionStart  DontDelete
  selectionEnd   KJS::HTMLElement::InputSelectionEnd    DontDelete
  placeholder   KJS::HTMLElement::InputPlaceholder      DontDelete
@end
@begin HTMLInputElementProtoTable 5
  select	KJS::HTMLElement::InputSelect		DontDelete|Function 0
  click		KJS::HTMLElement::InputClick		DontDelete|Function 0
  setSelectionRange KJS::HTMLElement::InputSetSelectionRange DontDelete|Function 2
@end
@begin HTMLTextAreaElementTable 13
  defaultValue	KJS::HTMLElement::TextAreaDefaultValue	DontDelete
  form		KJS::HTMLElement::TextAreaForm		DontDelete|ReadOnly
  accessKey	KJS::HTMLElement::TextAreaAccessKey	DontDelete
  cols		KJS::HTMLElement::TextAreaCols		DontDelete
  disabled	KJS::HTMLElement::TextAreaDisabled	DontDelete
  name		KJS::HTMLElement::TextAreaName		DontDelete
  readOnly	KJS::HTMLElement::TextAreaReadOnly	DontDelete
  rows		KJS::HTMLElement::TextAreaRows		DontDelete
  type		KJS::HTMLElement::TextAreaType		DontDelete|ReadOnly
  value		KJS::HTMLElement::TextAreaValue		DontDelete
  selectionStart KJS::HTMLElement::TextAreaSelectionStart DontDelete
  selectionEnd   KJS::HTMLElement::TextAreaSelectionEnd   DontDelete
  textLength     KJS::HTMLElement::TextAreaTextLength     DontDelete|ReadOnly
  placeholder   KJS::HTMLElement::TextAreaPlaceholder     DontDelete
@end
@begin HTMLTextAreaElementProtoTable 4
  select  KJS::HTMLElement::TextAreaSelect        DontDelete|Function 0
  setSelectionRange KJS::HTMLElement::TextAreaSetSelectionRange DontDelete|Function 2
@end
@begin HTMLButtonElementTable 9
  form		KJS::HTMLElement::ButtonForm		DontDelete|ReadOnly
  accessKey	KJS::HTMLElement::ButtonAccessKey	DontDelete
  disabled	KJS::HTMLElement::ButtonDisabled	DontDelete
  name		KJS::HTMLElement::ButtonName		DontDelete
  type		KJS::HTMLElement::ButtonType		DontDelete|ReadOnly
  value		KJS::HTMLElement::ButtonValue		DontDelete
@end
@begin HTMLButtonElementProtoTable 3
  click		KJS::HTMLElement::ButtonClick           DontDelete|Function 0
@end
@begin HTMLLabelElementTable 3
  form		KJS::HTMLElement::LabelForm		DontDelete|ReadOnly
  accessKey	KJS::HTMLElement::LabelAccessKey	DontDelete
  htmlFor	KJS::HTMLElement::LabelHtmlFor		DontDelete
@end
@begin HTMLFieldSetElementTable 1
  form		KJS::HTMLElement::FieldSetForm		DontDelete|ReadOnly
@end
@begin HTMLLegendElementTable 3
  form		KJS::HTMLElement::LegendForm		DontDelete|ReadOnly
  accessKey	KJS::HTMLElement::LegendAccessKey	DontDelete
  align		KJS::HTMLElement::LegendAlign		DontDelete
@end
@begin HTMLUListElementTable 2
  compact	KJS::HTMLElement::UListCompact		DontDelete
  type		KJS::HTMLElement::UListType		DontDelete
@end
@begin HTMLOListElementTable 3
  compact	KJS::HTMLElement::OListCompact		DontDelete
  start		KJS::HTMLElement::OListStart		DontDelete
  type		KJS::HTMLElement::OListType		DontDelete
@end
@begin HTMLDListElementTable 1
  compact	KJS::HTMLElement::DListCompact		DontDelete
@end
@begin HTMLDirectoryElementTable 1
  compact	KJS::HTMLElement::DirectoryCompact	DontDelete
@end
@begin HTMLMenuElementTable 1
  compact	KJS::HTMLElement::MenuCompact		DontDelete
@end
@begin HTMLLIElementTable 2
  type		KJS::HTMLElement::LIType		DontDelete
  value		KJS::HTMLElement::LIValue		DontDelete
@end
@begin HTMLDivElementTable 1
  align		KJS::HTMLElement::DivAlign		DontDelete
@end
@begin HTMLParagraphElementTable 1
  align		KJS::HTMLElement::ParagraphAlign	DontDelete
@end
@begin HTMLHeadingElementTable 1
  align		KJS::HTMLElement::HeadingAlign		DontDelete
@end
@begin HTMLBlockQuoteElementTable 1
  cite		KJS::HTMLElement::BlockQuoteCite	DontDelete
@end
@begin HTMLQuoteElementTable 1
  cite		KJS::HTMLElement::QuoteCite		DontDelete
@end
@begin HTMLPreElementTable 1
  width		KJS::HTMLElement::PreWidth		DontDelete
@end
@begin HTMLBRElementTable 1
  clear		KJS::HTMLElement::BRClear		DontDelete
@end
@begin HTMLBaseFontElementTable 3
  color		KJS::HTMLElement::BaseFontColor		DontDelete
  face		KJS::HTMLElement::BaseFontFace		DontDelete
  size		KJS::HTMLElement::BaseFontSize		DontDelete
@end
@begin HTMLFontElementTable 3
  color		KJS::HTMLElement::FontColor		DontDelete
  face		KJS::HTMLElement::FontFace		DontDelete
  size		KJS::HTMLElement::FontSize		DontDelete
@end
@begin HTMLHRElementTable 4
  align		KJS::HTMLElement::HRAlign		DontDelete
  noShade	KJS::HTMLElement::HRNoShade		DontDelete
  size		KJS::HTMLElement::HRSize		DontDelete
  width		KJS::HTMLElement::HRWidth		DontDelete
@end
@begin HTMLModElementTable 2
  cite		KJS::HTMLElement::ModCite		DontDelete
  dateTime	KJS::HTMLElement::ModDateTime		DontDelete
@end
@begin HTMLAnchorElementTable 23
  accessKey	KJS::HTMLElement::AnchorAccessKey	DontDelete
  charset	KJS::HTMLElement::AnchorCharset		DontDelete
  coords	KJS::HTMLElement::AnchorCoords		DontDelete
  href		KJS::HTMLElement::AnchorHref		DontDelete
  hreflang	KJS::HTMLElement::AnchorHrefLang	DontDelete
  hash		KJS::HTMLElement::AnchorHash		DontDelete|ReadOnly
  host		KJS::HTMLElement::AnchorHost		DontDelete|ReadOnly
  hostname	KJS::HTMLElement::AnchorHostname	DontDelete|ReadOnly
  name		KJS::HTMLElement::AnchorName		DontDelete
  pathname	KJS::HTMLElement::AnchorPathName	DontDelete|ReadOnly
  port		KJS::HTMLElement::AnchorPort		DontDelete|ReadOnly
  protocol	KJS::HTMLElement::AnchorProtocol	DontDelete|ReadOnly
  rel		KJS::HTMLElement::AnchorRel		DontDelete
  rev		KJS::HTMLElement::AnchorRev		DontDelete
  search	KJS::HTMLElement::AnchorSearch		DontDelete
  shape		KJS::HTMLElement::AnchorShape		DontDelete
  target	KJS::HTMLElement::AnchorTarget		DontDelete
  text		KJS::HTMLElement::AnchorText		DontDelete|ReadOnly
  type		KJS::HTMLElement::AnchorType		DontDelete
@end
@begin HTMLAnchorElementProtoTable 3
  click		KJS::HTMLElement::AnchorClick		DontDelete|Function 0
  toString      KJS::HTMLElement::AnchorToString        DontDelete|Function 0
@end
@begin HTMLImageElementTable 15
  name		KJS::HTMLElement::ImageName		DontDelete
  align		KJS::HTMLElement::ImageAlign		DontDelete
  alt		KJS::HTMLElement::ImageAlt		DontDelete
  border	KJS::HTMLElement::ImageBorder		DontDelete
  complete	KJS::HTMLElement::ImageComplete		DontDelete|ReadOnly
  height	KJS::HTMLElement::ImageHeight		DontDelete
  hspace	KJS::HTMLElement::ImageHspace		DontDelete
  isMap		KJS::HTMLElement::ImageIsMap		DontDelete
  longDesc	KJS::HTMLElement::ImageLongDesc		DontDelete
  src		KJS::HTMLElement::ImageSrc		DontDelete
  useMap	KJS::HTMLElement::ImageUseMap		DontDelete
  vspace	KJS::HTMLElement::ImageVspace		DontDelete
  width		KJS::HTMLElement::ImageWidth		DontDelete
  x     	KJS::HTMLElement::ImageX		DontDelete|ReadOnly
  y     	KJS::HTMLElement::ImageY		DontDelete|ReadOnly
@end
@begin HTMLObjectElementTable 23
  form		  KJS::HTMLElement::ObjectForm		  DontDelete|ReadOnly
  code		  KJS::HTMLElement::ObjectCode		  DontDelete
  align		  KJS::HTMLElement::ObjectAlign		  DontDelete
  archive	  KJS::HTMLElement::ObjectArchive	  DontDelete
  border	  KJS::HTMLElement::ObjectBorder	  DontDelete
  codeBase	  KJS::HTMLElement::ObjectCodeBase	  DontDelete
  codeType	  KJS::HTMLElement::ObjectCodeType	  DontDelete
  contentDocument KJS::HTMLElement::ObjectContentDocument DontDelete|ReadOnly
  data		  KJS::HTMLElement::ObjectData		  DontDelete
  declare	  KJS::HTMLElement::ObjectDeclare	  DontDelete
  height	  KJS::HTMLElement::ObjectHeight	  DontDelete
  hspace	  KJS::HTMLElement::ObjectHspace	  DontDelete
  name		  KJS::HTMLElement::ObjectName		  DontDelete
  standby	  KJS::HTMLElement::ObjectStandby	  DontDelete
  type		  KJS::HTMLElement::ObjectType		  DontDelete
  useMap	  KJS::HTMLElement::ObjectUseMap	  DontDelete
  vspace	  KJS::HTMLElement::ObjectVspace	  DontDelete
  width		  KJS::HTMLElement::ObjectWidth		  DontDelete
@end
@begin HTMLObjectElementProtoTable 1
# half deprecated - cf. http://lists.w3.org/Archives/Public/www-svg/2008Feb/0031.html
# only implemented in ecma, because of acid3 dependency
  getSVGDocument  KJS::HTMLElement::ObjectGetSVGDocument  DontDelete|Function 0
@end
@begin HTMLParamElementTable 4
  name		KJS::HTMLElement::ParamName		DontDelete
  type		KJS::HTMLElement::ParamType		DontDelete
  value		KJS::HTMLElement::ParamValue		DontDelete
  valueType	KJS::HTMLElement::ParamValueType	DontDelete
@end
@begin HTMLAppletElementTable 11
  align		KJS::HTMLElement::AppletAlign		DontDelete
  alt		KJS::HTMLElement::AppletAlt		DontDelete
  archive	KJS::HTMLElement::AppletArchive		DontDelete
  code		KJS::HTMLElement::AppletCode		DontDelete
  codeBase	KJS::HTMLElement::AppletCodeBase	DontDelete
  height	KJS::HTMLElement::AppletHeight		DontDelete
  hspace	KJS::HTMLElement::AppletHspace		DontDelete
  name		KJS::HTMLElement::AppletName		DontDelete
  object	KJS::HTMLElement::AppletObject		DontDelete
  vspace	KJS::HTMLElement::AppletVspace		DontDelete
  width		KJS::HTMLElement::AppletWidth		DontDelete
@end
@begin HTMLMapElementTable 2
  areas		KJS::HTMLElement::MapAreas		DontDelete|ReadOnly
  name		KJS::HTMLElement::MapName		DontDelete
@end
@begin HTMLAreaElementTable 15
  accessKey	KJS::HTMLElement::AreaAccessKey		DontDelete
  alt		KJS::HTMLElement::AreaAlt		DontDelete
  coords	KJS::HTMLElement::AreaCoords		DontDelete
  href		KJS::HTMLElement::AreaHref		DontDelete
  hash		KJS::HTMLElement::AreaHash		DontDelete|ReadOnly
  host		KJS::HTMLElement::AreaHost		DontDelete|ReadOnly
  hostname	KJS::HTMLElement::AreaHostName		DontDelete|ReadOnly
  pathname	KJS::HTMLElement::AreaPathName		DontDelete|ReadOnly
  port		KJS::HTMLElement::AreaPort		DontDelete|ReadOnly
  protocol	KJS::HTMLElement::AreaProtocol		DontDelete|ReadOnly
  search	KJS::HTMLElement::AreaSearch		DontDelete|ReadOnly
  noHref	KJS::HTMLElement::AreaNoHref		DontDelete
  shape		KJS::HTMLElement::AreaShape		DontDelete
  target	KJS::HTMLElement::AreaTarget		DontDelete
@end
@begin HTMLScriptElementTable 7
  text		KJS::HTMLElement::ScriptText		DontDelete
  htmlFor	KJS::HTMLElement::ScriptHtmlFor		DontDelete
  event		KJS::HTMLElement::ScriptEvent		DontDelete
  charset	KJS::HTMLElement::ScriptCharset		DontDelete
  defer		KJS::HTMLElement::ScriptDefer		DontDelete
  src		KJS::HTMLElement::ScriptSrc		DontDelete
  type		KJS::HTMLElement::ScriptType		DontDelete
@end
@begin HTMLTableElementTable 23
  caption	KJS::HTMLElement::TableCaption		DontDelete
  tHead		KJS::HTMLElement::TableTHead		DontDelete
  tFoot		KJS::HTMLElement::TableTFoot		DontDelete
  rows		KJS::HTMLElement::TableRows		DontDelete|ReadOnly
  tBodies	KJS::HTMLElement::TableTBodies		DontDelete|ReadOnly
  align		KJS::HTMLElement::TableAlign		DontDelete
  bgColor	KJS::HTMLElement::TableBgColor		DontDelete
  border	KJS::HTMLElement::TableBorder		DontDelete
  cellPadding	KJS::HTMLElement::TableCellPadding	DontDelete
  cellSpacing	KJS::HTMLElement::TableCellSpacing	DontDelete
  frame		KJS::HTMLElement::TableFrame		DontDelete
  rules		KJS::HTMLElement::TableRules		DontDelete
  summary	KJS::HTMLElement::TableSummary		DontDelete
  width		KJS::HTMLElement::TableWidth		DontDelete
@end
@begin HTMLTableElementProtoTable 8
  createTHead	KJS::HTMLElement::TableCreateTHead	DontDelete|Function 0
  deleteTHead	KJS::HTMLElement::TableDeleteTHead	DontDelete|Function 0
  createTFoot	KJS::HTMLElement::TableCreateTFoot	DontDelete|Function 0
  deleteTFoot	KJS::HTMLElement::TableDeleteTFoot	DontDelete|Function 0
  createCaption	KJS::HTMLElement::TableCreateCaption	DontDelete|Function 0
  deleteCaption	KJS::HTMLElement::TableDeleteCaption	DontDelete|Function 0
  insertRow	KJS::HTMLElement::TableInsertRow	DontDelete|Function 1
  deleteRow	KJS::HTMLElement::TableDeleteRow	DontDelete|Function 1
@end
@begin HTMLTableCaptionElementTable 1
  align		KJS::HTMLElement::TableCaptionAlign	DontDelete
@end
@begin HTMLTableColElementTable 7
  align		KJS::HTMLElement::TableColAlign		DontDelete
  ch		KJS::HTMLElement::TableColCh		DontDelete
  chOff		KJS::HTMLElement::TableColChOff		DontDelete
  span		KJS::HTMLElement::TableColSpan		DontDelete
  vAlign	KJS::HTMLElement::TableColVAlign	DontDelete
  width		KJS::HTMLElement::TableColWidth		DontDelete
@end
@begin HTMLTableSectionElementTable 7
  align		KJS::HTMLElement::TableSectionAlign		DontDelete
  ch		KJS::HTMLElement::TableSectionCh		DontDelete
  chOff		KJS::HTMLElement::TableSectionChOff		DontDelete
  vAlign	KJS::HTMLElement::TableSectionVAlign		DontDelete
  rows		KJS::HTMLElement::TableSectionRows		DontDelete|ReadOnly
@end
@begin HTMLTableSectionElementProtoTable 2
  insertRow	KJS::HTMLElement::TableSectionInsertRow		DontDelete|Function 1
  deleteRow	KJS::HTMLElement::TableSectionDeleteRow		DontDelete|Function 1
@end
@begin HTMLTableRowElementTable 11
  rowIndex	KJS::HTMLElement::TableRowRowIndex		DontDelete|ReadOnly
  sectionRowIndex KJS::HTMLElement::TableRowSectionRowIndex	DontDelete|ReadOnly
  cells		KJS::HTMLElement::TableRowCells			DontDelete|ReadOnly
  align		KJS::HTMLElement::TableRowAlign			DontDelete
  bgColor	KJS::HTMLElement::TableRowBgColor		DontDelete
  ch		KJS::HTMLElement::TableRowCh			DontDelete
  chOff		KJS::HTMLElement::TableRowChOff			DontDelete
  vAlign	KJS::HTMLElement::TableRowVAlign		DontDelete
@end
@begin HTMLTableRowElementProtoTable 2
  insertCell	KJS::HTMLElement::TableRowInsertCell		DontDelete|Function 1
  deleteCell	KJS::HTMLElement::TableRowDeleteCell		DontDelete|Function 1
@end
@begin HTMLTableCellElementTable 15
  cellIndex	KJS::HTMLElement::TableCellCellIndex		DontDelete|ReadOnly
  abbr		KJS::HTMLElement::TableCellAbbr			DontDelete
  align		KJS::HTMLElement::TableCellAlign		DontDelete
  axis		KJS::HTMLElement::TableCellAxis			DontDelete
  bgColor	KJS::HTMLElement::TableCellBgColor		DontDelete
  ch		KJS::HTMLElement::TableCellCh			DontDelete
  chOff		KJS::HTMLElement::TableCellChOff		DontDelete
  colSpan	KJS::HTMLElement::TableCellColSpan		DontDelete
  headers	KJS::HTMLElement::TableCellHeaders		DontDelete
  height	KJS::HTMLElement::TableCellHeight		DontDelete
  noWrap	KJS::HTMLElement::TableCellNoWrap		DontDelete
  rowSpan	KJS::HTMLElement::TableCellRowSpan		DontDelete
  scope		KJS::HTMLElement::TableCellScope		DontDelete
  vAlign	KJS::HTMLElement::TableCellVAlign		DontDelete
  width		KJS::HTMLElement::TableCellWidth		DontDelete
@end
@begin HTMLFrameSetElementTable 2
  cols		KJS::HTMLElement::FrameSetCols			DontDelete
  rows		KJS::HTMLElement::FrameSetRows			DontDelete
  onmessage     KJS::HTMLElement::FrameSetOnMessage             DontDelete
@end
@begin HTMLLayerElementTable 6
  top		  KJS::HTMLElement::LayerTop			DontDelete
  left		  KJS::HTMLElement::LayerLeft			DontDelete
  visibility	  KJS::HTMLElement::LayerVisibility		DontDelete
  bgColor	  KJS::HTMLElement::LayerBgColor		DontDelete
  document  	  KJS::HTMLElement::LayerDocument		DontDelete|ReadOnly
  clip	  	  KJS::HTMLElement::LayerClip			DontDelete|ReadOnly
  layers	  KJS::HTMLElement::LayerLayers			DontDelete|ReadOnly
@end
@begin HTMLFrameElementTable 13
  contentDocument KJS::HTMLElement::FrameContentDocument        DontDelete|ReadOnly
  contentWindow KJS::HTMLElement::FrameContentWindow        DontDelete|ReadOnly
  frameBorder     KJS::HTMLElement::FrameFrameBorder		DontDelete
  longDesc	  KJS::HTMLElement::FrameLongDesc		DontDelete
  marginHeight	  KJS::HTMLElement::FrameMarginHeight		DontDelete
  marginWidth	  KJS::HTMLElement::FrameMarginWidth		DontDelete
  name		  KJS::HTMLElement::FrameName			DontDelete
  noResize	  KJS::HTMLElement::FrameNoResize		DontDelete
  scrolling	  KJS::HTMLElement::FrameScrolling		DontDelete
  src		  KJS::HTMLElement::FrameSrc			DontDelete
  location	  KJS::HTMLElement::FrameLocation		DontDelete
# IE extension
  width		  KJS::HTMLElement::FrameWidth			DontDelete|ReadOnly
  height	  KJS::HTMLElement::FrameHeight			DontDelete|ReadOnly
@end
@begin HTMLIFrameElementTable 13
  align		  KJS::HTMLElement::IFrameAlign			DontDelete
  contentDocument KJS::HTMLElement::IFrameContentDocument       DontDelete|ReadOnly
  contentWindow KJS::HTMLElement::IFrameContentWindow        DontDelete|ReadOnly
  frameBorder	  KJS::HTMLElement::IFrameFrameBorder		DontDelete
  height	  KJS::HTMLElement::IFrameHeight		DontDelete
  longDesc	  KJS::HTMLElement::IFrameLongDesc		DontDelete
  marginHeight	  KJS::HTMLElement::IFrameMarginHeight		DontDelete
  marginWidth	  KJS::HTMLElement::IFrameMarginWidth		DontDelete
  name		  KJS::HTMLElement::IFrameName			DontDelete
  scrolling	  KJS::HTMLElement::IFrameScrolling		DontDelete
  src		  KJS::HTMLElement::IFrameSrc			DontDelete
  width		  KJS::HTMLElement::IFrameWidth			DontDelete
@end
@begin HTMLIFrameElementProtoTable 1
# half deprecated - cf. http://lists.w3.org/Archives/Public/www-svg/2008Feb/0031.html
# only implemented in ecma, because of acid3 dependency
  getSVGDocument  KJS::HTMLElement::IFrameGetSVGDocument        DontDelete|Function 0
@end

@begin HTMLMarqueeElementProtoTable 2
  start           KJS::HTMLElement::MarqueeStart		DontDelete|Function 0
  stop            KJS::HTMLElement::MarqueeStop                 DontDelete|Function 0
@end

@begin HTMLCanvasElementTable 2
  width           KJS::HTMLElement::CanvasWidth                 DontDelete
  height          KJS::HTMLElement::CanvasHeight                DontDelete
@end

@begin HTMLCanvasElementProtoTable 1
  getContext      KJS::HTMLElement::CanvasGetContext            DontDelete|Function 1
  toDataURL       KJS::HTMLElement::CanvasToDataURL             DontDelete|Function 0
@end
*/
KJS_IMPLEMENT_PROTOFUNC(HTMLElementFunction)

KParts::ScriptableExtension *HTMLElement::getScriptableExtension(const DOM::HTMLElementImpl &element)
{
  DOM::DocumentImpl* doc = element.document();
  if (doc->part())
    return doc->part()->scriptableExtension(&element);
  return 0L;
}

JSValue *HTMLElement::formNameGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
    HTMLElement* thisObj = static_cast<HTMLElement*>(slot.slotBase());
    HTMLFormElementImpl* form = static_cast<HTMLFormElementImpl*>(thisObj->impl());

    KJS::HTMLCollection coll(exec, form->elements());
    JSValue* result = coll.getNamedItems(exec, propertyName);
    
    // In case of simple result, remember in past names map
    // ### our HTMLFormElementsCollection is a bit too IE-compatey rather than HTML5ey
    if (DOM::NodeImpl* node = toNode(result)) {
	if (node->isGenericFormElement())
	    form->bindPastName(static_cast<HTMLGenericFormElementImpl*>(node));
    }
    return result;
}

//JSValue* KJS::HTMLElement::tryGet(ExecState *exec, const Identifier &propertyName) const
bool KJS::HTMLElement::getOwnPropertySlot(ExecState *exec, const Identifier &propertyName, PropertySlot& slot)
{
  DOM::HTMLElementImpl& element = *impl();
#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLElement::getOwnPropertySlot " << propertyName.qstring() << " thisTag=" << element.tagName().string();
#endif
  // First look at dynamic properties
  switch (element.id()) {
    case ID_FORM: {
      DOM::HTMLFormElementImpl& form = static_cast<DOM::HTMLFormElementImpl&>(element);
      // Check if we're retrieving an element (by index or by name)

      if (getIndexSlot(this, propertyName, slot))
        return true;

      KJS::HTMLCollection coll(exec, form.elements());
      JSValue *namedItems = coll.getNamedItems(exec, propertyName);
      if (namedItems->type() != UndefinedType) {
          slot.setCustom(this, formNameGetter);
          return true;
      }
      
      // Try with past names map
      if (HTMLGenericFormElementImpl* r = form.lookupByPastName(propertyName.domString())) 
	  return getImmediateValueSlot(this, getDOMNode(exec, r), slot);
      
      break;
    }
    case ID_SELECT:
      if (getIndexSlot(this, propertyName, slot))
        return true;
      break;
    case ID_APPLET:
    case ID_OBJECT:
    case ID_EMBED: {
      KParts::ScriptableExtension* se = getScriptableExtension(*impl());
      if (pluginRootGet(exec, se, propertyName, slot))
        return true;
      break;
    }
  }

  const HashTable* table = classInfo()->propHashTable; // get the right hashtable
  if (table && getStaticOwnPropertySlot<HTMLElementFunction, HTMLElement>(table, this, propertyName, slot))
    return true;

  // Base HTMLElement stuff or parent class forward, as usual
  return getStaticPropertySlot<KJS::HTMLElementFunction, KJS::HTMLElement, DOMElement>(
    exec, &KJS::HTMLElementTable, this, propertyName, slot);
}

JSValue* HTMLElement::indexGetter(ExecState *exec, unsigned index)
{
  switch (impl()->id())
  {
    case ID_FORM: {
      DOM::HTMLFormElementImpl* form = static_cast<DOM::HTMLFormElementImpl*>(impl());
      SharedPtr<DOM::HTMLCollectionImpl> elems = form->elements();
      return getDOMNode(exec, elems->item(index));
    }
  case ID_SELECT: {
      DOM::HTMLSelectElementImpl* select = static_cast<DOM::HTMLSelectElementImpl*>(impl());
      SharedPtr<DOM::HTMLCollectionImpl> opts = select->options();
      return getDOMNode(exec, opts->item(index)); // not specified by DOM(?) but supported in netscape/IE
    }
  default:
    assert(0);
    return jsUndefined();
  }
}

#if 0
  // First look at dynamic properties
  switch (element.id()) {
    case ID_FORM: {
      DOM::HTMLFormElementImpl& form = static_cast<DOM::HTMLFormElementImpl&>(element);
      // Check if we're retrieving an element (by index or by name)
      KJS::HTMLCollection coll(exec, form.elements());
      JSValue *namedItems = coll.getNamedItems(exec, propertyName);
      if (namedItems->type() != UndefinedType)
        return namedItems;
    }
  }

#endif

/**
 Table of how to connect JS tokens to attributes
*/
const KJS::HTMLElement::BoundPropInfo KJS::HTMLElement::bpTable[] = {
  {ID_HTML, HtmlVersion, T_String, ATTR_VERSION},
  {ID_HEAD, HeadProfile, T_String, ATTR_PROFILE},
  {ID_LINK, LinkDisabled, T_Bool, ATTR_DISABLED},
  {ID_LINK, LinkCharset,  T_String, ATTR_CHARSET},
  {ID_LINK, LinkHref,     T_URL,    ATTR_HREF},
  {ID_LINK, LinkHrefLang, T_String, ATTR_HREFLANG},
  {ID_LINK, LinkMedia,    T_String, ATTR_MEDIA},
  {ID_LINK, LinkRel,      T_String, ATTR_REL},
  {ID_LINK, LinkRev,      T_String, ATTR_REV},
  {ID_LINK, LinkTarget,   T_String, ATTR_TARGET},
  {ID_LINK, LinkType,     T_String, ATTR_TYPE},
  {ID_BASE, BaseHref,     T_URL,    ATTR_HREF},
  {ID_BASE, BaseTarget,   T_String, ATTR_TARGET},
  {ID_META, MetaContent,  T_String, ATTR_CONTENT},
  {ID_META, MetaHttpEquiv,T_String, ATTR_HTTP_EQUIV},
  {ID_META, MetaName,     T_String, ATTR_NAME},
  {ID_META, MetaScheme,   T_String, ATTR_SCHEME},
  {ID_STYLE, StyleDisabled, T_Bool, ATTR_DISABLED},
  {ID_STYLE, StyleMedia,  T_String, ATTR_MEDIA},
  {ID_STYLE, StyleType,   T_String, ATTR_TYPE},
  {ID_BODY,  BodyALink,   T_String, ATTR_ALINK},
  {ID_BODY,  BodyBackground, T_String, ATTR_BACKGROUND},
  {ID_BODY,  BodyBgColor,    T_String, ATTR_BGCOLOR},
  {ID_BODY,  BodyLink,    T_String, ATTR_LINK},
  {ID_BODY,  BodyText,       T_String, ATTR_TEXT},//### odd?
  {ID_BODY,  BodyVLink,      T_String, ATTR_VLINK},
  {ID_FORM,  FormName,       T_String, ATTR_NAME}, // NOT getString (IE gives empty string)
  {ID_FORM,  FormAcceptCharset, T_String, ATTR_ACCEPT_CHARSET},
  {ID_FORM,  FormAction,     T_String, ATTR_ACTION},
  {ID_FORM,  FormEncType,    T_String, ATTR_ENCTYPE},
  {ID_FORM,  FormMethod,     T_String, ATTR_METHOD},
  {ID_FORM,  FormTarget,     T_String, ATTR_TARGET},
  {ID_SELECT, SelectDisabled, T_Bool,  ATTR_DISABLED},
  {ID_SELECT, SelectMultiple, T_Bool,  ATTR_MULTIPLE},
  {ID_SELECT, SelectSize,     T_Int,   ATTR_SIZE}, //toInt on attr, then number
  {ID_OPTGROUP, OptGroupDisabled, T_Bool, ATTR_DISABLED},
  {ID_OPTGROUP, OptGroupLabel,    T_String, ATTR_LABEL},
  {ID_OPTION, OptionDefaultSelected, T_Bool, ATTR_SELECTED},
  {ID_OPTION, OptionDisabled, T_Bool,   ATTR_DISABLED},
  {ID_OPTION, OptionLabel,    T_String, ATTR_LABEL},
  {ID_INPUT,  InputDefaultValue,   T_String, ATTR_VALUE},
  {ID_INPUT,  InputDefaultChecked, T_Bool,   ATTR_CHECKED},
  {ID_INPUT,  InputAccept,    T_String, ATTR_ACCEPT},
  {ID_INPUT,  InputAccessKey, T_String, ATTR_ACCESSKEY},
  {ID_INPUT,  InputAlign,     T_String, ATTR_ALIGN},
  {ID_INPUT,  InputAlt,       T_String, ATTR_ALT},
  {ID_INPUT,  InputDisabled,  T_Bool,   ATTR_DISABLED},
  {ID_INPUT,  InputMaxLength, T_Int,    ATTR_MAXLENGTH},
  {ID_INPUT,  InputReadOnly,  T_Bool,   ATTR_READONLY},
  {ID_INPUT,  InputSize,      T_Int,    ATTR_SIZE},
  {ID_INPUT,  InputSrc,       T_URL,    ATTR_SRC},
  {ID_INPUT,  InputUseMap,    T_String, ATTR_USEMAP},
  {ID_TEXTAREA, TextAreaAccessKey, T_String, ATTR_ACCESSKEY},
  {ID_TEXTAREA, TextAreaCols, T_Int, ATTR_COLS},
  {ID_TEXTAREA, TextAreaDisabled, T_Bool, ATTR_DISABLED},
  {ID_TEXTAREA, TextAreaReadOnly, T_Bool, ATTR_READONLY},
  {ID_TEXTAREA, TextAreaRows, T_Int,  ATTR_ROWS},
  {ID_BUTTON,   ButtonAccessKey, T_String, ATTR_ACCESSKEY},
  {ID_BUTTON,   ButtonDisabled, T_Bool  , ATTR_DISABLED},
  {ID_BUTTON,   ButtonName,     T_String, ATTR_NAME},
  {ID_BUTTON,   ButtonValue,    T_String, ATTR_VALUE},
  {ID_LABEL,    LabelAccessKey, T_String, ATTR_ACCESSKEY},
  {ID_LABEL,    LabelHtmlFor,   T_String, ATTR_FOR},
  {ID_LEGEND,   LegendAccessKey, T_String, ATTR_ACCESSKEY},
  {ID_LEGEND,   LegendAlign,     T_String, ATTR_ALIGN},
  {ID_UL,       UListCompact,    T_Bool,   ATTR_COMPACT},
  {ID_UL,       UListType,       T_String, ATTR_TYPE},
  {ID_OL,       OListCompact,    T_Bool,   ATTR_COMPACT},
  {ID_OL,       OListStart,      T_Int,    ATTR_START},
  {ID_OL,       OListType,       T_String, ATTR_TYPE},
  {ID_DL,       DListCompact,    T_Bool,   ATTR_COMPACT},
  {ID_DIR,      DirectoryCompact, T_Bool,  ATTR_COMPACT},
  {ID_MENU,     MenuCompact,      T_Bool,  ATTR_COMPACT},
  {ID_LI,       LIType,           T_String, ATTR_TYPE},
  {ID_LI,       LIValue,          T_Int,    ATTR_VALUE},
  {ID_DIV,      DivAlign,         T_String, ATTR_ALIGN},
  {ID_P,        ParagraphAlign,   T_String, ATTR_ALIGN},
  {NotApplicable,HeadingAlign,     T_String, ATTR_ALIGN},
  {ID_BLOCKQUOTE, BlockQuoteCite, T_String, ATTR_CITE},
  {ID_Q,        QuoteCite,        T_String, ATTR_CITE},
  {ID_PRE,      PreWidth,         T_Int,    ATTR_WIDTH},
  {ID_BR,       BRClear,          T_String, ATTR_CLEAR},
  {ID_BASEFONT, BaseFontColor,    T_String, ATTR_COLOR},
  {ID_BASEFONT, BaseFontFace,     T_String, ATTR_FACE},
  {ID_BASEFONT, BaseFontSize,     T_Int,    ATTR_SIZE},
  {ID_FONT,     FontColor,        T_String, ATTR_COLOR},
  {ID_FONT,     FontFace,         T_String, ATTR_FACE},
  {ID_FONT,     FontSize,         T_String, ATTR_SIZE},
  {ID_HR,       HRAlign,          T_String, ATTR_ALIGN},
  {ID_HR,       HRNoShade,        T_Bool,   ATTR_NOSHADE},
  {ID_HR,       HRSize,           T_String, ATTR_SIZE},
  {ID_HR,       HRWidth,          T_String, ATTR_WIDTH},
  {NotApplicable, ModCite,          T_String, ATTR_CITE},
  {NotApplicable, ModDateTime,      T_String, ATTR_DATETIME},
  {ID_A,        AnchorAccessKey,  T_String, ATTR_ACCESSKEY},
  {ID_A,        AnchorCharset,    T_String, ATTR_CHARSET},
  {ID_A,        AnchorCoords,     T_String, ATTR_COORDS},
  {ID_A,        AnchorHref,       T_URL,    ATTR_HREF},
  {ID_A,        AnchorHrefLang,   T_String, ATTR_HREFLANG},
  {ID_A,        AnchorName,       T_String, ATTR_NAME},
  {ID_A,        AnchorRel,        T_String, ATTR_REL},
  {ID_A,        AnchorRev,        T_String, ATTR_REV},
  {ID_A,        AnchorShape,      T_String, ATTR_SHAPE},
  {ID_A,        AnchorTarget,     T_String, ATTR_TARGET},
  {ID_A,        AnchorType,       T_String, ATTR_TYPE},
  {ID_IMG,      ImageName,        T_String, ATTR_NAME},
  {ID_IMG,      ImageAlign,       T_String, ATTR_ALIGN},
  {ID_IMG,      ImageAlt,         T_String, ATTR_ALT},
  {ID_IMG,      ImageBorder,      T_String, ATTR_BORDER},
  {ID_IMG,      ImageHspace,      T_Int,    ATTR_HSPACE}, // ### return actual value
  {ID_IMG,      ImageIsMap,       T_Bool,   ATTR_ISMAP},
  {ID_IMG,      ImageLongDesc,    T_String, ATTR_LONGDESC},
  {ID_IMG,      ImageSrc,         T_URL,    ATTR_SRC},
  {ID_IMG,      ImageUseMap,      T_String, ATTR_USEMAP},
  {ID_IMG,      ImageVspace,      T_Int,    ATTR_VSPACE}, // ### return actual value
  {ID_OBJECT,   ObjectCode,       T_String, ATTR_CODE},
  {ID_OBJECT,   ObjectAlign,      T_String, ATTR_ALIGN},
  {ID_OBJECT,   ObjectArchive,    T_String, ATTR_ARCHIVE},
  {ID_OBJECT,   ObjectBorder,     T_String, ATTR_BORDER},
  {ID_OBJECT,   ObjectCodeBase,   T_String, ATTR_CODEBASE},
  {ID_OBJECT,   ObjectCodeType,   T_String, ATTR_CODETYPE},
  {ID_OBJECT,   ObjectData,       T_URL,    ATTR_DATA},
  {ID_OBJECT,   ObjectDeclare,    T_Bool,   ATTR_DECLARE},
  {ID_OBJECT,   ObjectHeight,     T_String, ATTR_HEIGHT},
  {ID_OBJECT,   ObjectHspace,     T_Int,    ATTR_HSPACE},
  {ID_OBJECT,   ObjectName,       T_String, ATTR_NAME},
  {ID_OBJECT,   ObjectStandby,    T_String, ATTR_STANDBY},
  {ID_OBJECT,   ObjectType,       T_String, ATTR_TYPE},
  {ID_OBJECT,   ObjectUseMap,     T_String, ATTR_USEMAP},
  {ID_OBJECT,   ObjectVspace,     T_Int,    ATTR_VSPACE},
  {ID_OBJECT,   ObjectWidth,      T_String, ATTR_WIDTH},
  {ID_PARAM,    ParamName,        T_String, ATTR_NAME},
  {ID_PARAM,    ParamType,        T_String, ATTR_TYPE},
  {ID_PARAM,    ParamValue,       T_String, ATTR_VALUE},
  {ID_PARAM,    ParamValueType,   T_String, ATTR_VALUETYPE},
  {ID_APPLET,   AppletAlign,      T_String, ATTR_ALIGN},
  {ID_APPLET,   AppletAlt,        T_String, ATTR_ALT},
  {ID_APPLET,   AppletArchive,    T_String, ATTR_ARCHIVE},
  {ID_APPLET,   AppletCode,       T_String, ATTR_CODE},
  {ID_APPLET,   AppletCodeBase,   T_String, ATTR_CODEBASE},
  {ID_APPLET,   AppletHeight,     T_String, ATTR_HEIGHT},
  {ID_APPLET,   AppletHspace,     T_Int,    ATTR_HSPACE},
  {ID_APPLET,   AppletName,       T_String, ATTR_NAME},
  {ID_APPLET,   AppletObject,     T_String, ATTR_OBJECT},
  {ID_APPLET,   AppletVspace,     T_Int,    ATTR_VSPACE},
  {ID_APPLET,   AppletWidth,      T_String, ATTR_WIDTH},
  {ID_MAP,      MapName,          T_String, ATTR_NAME},
  {ID_MAP,      MapAreas,         T_Coll,   HTMLCollectionImpl::MAP_AREAS},
  {ID_AREA,     AreaAccessKey,    T_String, ATTR_ACCESSKEY},
  {ID_AREA,     AreaAlt,          T_String, ATTR_ALT},
  {ID_AREA,     AreaCoords,       T_String, ATTR_COORDS},
  {ID_AREA,     AreaHref,         T_URL,    ATTR_HREF},
  {ID_AREA,     AreaNoHref,       T_Bool,   ATTR_NOHREF},
  {ID_AREA,     AreaShape,        T_String, ATTR_SHAPE},
  {ID_AREA,     AreaTarget,       T_String, ATTR_TARGET},
  {ID_SCRIPT,   ScriptHtmlFor,    T_Res,    NotApplicable},
  {ID_SCRIPT,   ScriptEvent,      T_Res,    NotApplicable},
  {ID_SCRIPT,   ScriptCharset,    T_String, ATTR_CHARSET},
  {ID_SCRIPT,   ScriptDefer,      T_Bool,   ATTR_DEFER},
  {ID_SCRIPT,   ScriptSrc,        T_URL,    ATTR_SRC},
  {ID_SCRIPT,   ScriptType,       T_String, ATTR_TYPE},
  {ID_TABLE,    TableAlign,       T_String, ATTR_ALIGN},
  {ID_TABLE,    TableBgColor,     T_String, ATTR_BGCOLOR},
  {ID_TABLE,    TableBorder,      T_String, ATTR_BORDER},
  {ID_TABLE,    TableCellPadding, T_String, ATTR_CELLPADDING},
  {ID_TABLE,    TableCellSpacing, T_String, ATTR_CELLSPACING},
  {ID_TABLE,    TableFrame,       T_String, ATTR_FRAME},
  {ID_TABLE,    TableRules,       T_String, ATTR_RULES},
  {ID_TABLE,    TableSummary,     T_String, ATTR_SUMMARY},
  {ID_TABLE,    TableWidth,       T_String, ATTR_WIDTH},
  {ID_TABLE,    TableRows,        T_Coll,   HTMLCollectionImpl::TABLE_ROWS},
  {ID_TABLE,    TableTBodies,     T_Coll,   HTMLCollectionImpl::TABLE_TBODIES},
  {ID_CAPTION,  TableCaptionAlign,T_String, ATTR_ALIGN},
  {NotApplicable,TableColAlign,    T_String, ATTR_ALIGN}, //Col/ColGroup
  {NotApplicable,TableColCh,       T_String, ATTR_CHAR},
  {NotApplicable,TableColChOff,    T_String, ATTR_CHAROFF},
  {NotApplicable,TableColSpan,     T_Int,    ATTR_SPAN},
  {NotApplicable,TableColVAlign,   T_String, ATTR_VALIGN},
  {NotApplicable,TableColWidth,    T_String, ATTR_WIDTH},
  {NotApplicable,TableSectionAlign,T_String, ATTR_ALIGN}, //THead/TBody/TFoot
  {NotApplicable,TableSectionCh,   T_String, ATTR_CHAR},
  {NotApplicable,TableSectionChOff,T_String, ATTR_CHAROFF},
  {NotApplicable,TableSectionVAlign,T_String, ATTR_VALIGN},
  {NotApplicable,TableSectionRows,  T_Coll,  HTMLCollectionImpl::TSECTION_ROWS},
  {ID_TR,       TableRowAlign,     T_String, ATTR_ALIGN},  //TR
  {ID_TR,       TableRowBgColor,   T_String, ATTR_BGCOLOR},
  {ID_TR,       TableRowCh,        T_String, ATTR_CHAR},
  {ID_TR,       TableRowChOff,     T_String, ATTR_CHAROFF},
  {ID_TR,       TableRowVAlign,    T_String, ATTR_VALIGN},
  {ID_TR,       TableRowCells,     T_Coll,   HTMLCollectionImpl::TR_CELLS},
  {NotApplicable,TableCellAbbr,     T_String, ATTR_ABBR}, //TD/TH
  {NotApplicable,TableCellAlign,    T_String, ATTR_ALIGN},
  {NotApplicable,TableCellAxis,     T_String, ATTR_AXIS},
  {NotApplicable,TableCellBgColor,  T_String, ATTR_BGCOLOR},
  {NotApplicable,TableCellCh,       T_String, ATTR_CHAR},
  {NotApplicable,TableCellChOff,    T_String, ATTR_CHAROFF},
  {NotApplicable,TableCellColSpan,  T_Int,    ATTR_COLSPAN},
  {NotApplicable,TableCellHeaders,  T_String, ATTR_HEADERS},
  {NotApplicable,TableCellHeight,   T_String, ATTR_HEIGHT},
  {NotApplicable,TableCellNoWrap,   T_Bool,   ATTR_NOWRAP},
  {NotApplicable,TableCellRowSpan,  T_Int,    ATTR_ROWSPAN},
  {NotApplicable,TableCellScope,    T_String, ATTR_SCOPE},
  {NotApplicable,TableCellVAlign,   T_String, ATTR_VALIGN},
  {NotApplicable,TableCellWidth,    T_String, ATTR_WIDTH},
  {ID_FRAMESET, FrameSetCols,      T_String, ATTR_COLS},
  {ID_FRAMESET, FrameSetRows,      T_String, ATTR_ROWS},
  {ID_LAYER,    LayerTop,          T_Int,    ATTR_TOP},
  {ID_LAYER,    LayerLeft,         T_Int,    ATTR_LEFT},
  {ID_LAYER,    LayerVisibility,   T_StrOrNl,ATTR_VISIBILITY},
  {ID_LAYER,    LayerBgColor,      T_StrOrNl,ATTR_BGCOLOR},
  {ID_LAYER,    LayerLayers,       T_Coll,   HTMLCollectionImpl::DOC_LAYERS},
  {ID_FRAME,    FrameFrameBorder,  T_String, ATTR_FRAMEBORDER},
  {ID_FRAME,    FrameLongDesc,     T_String, ATTR_LONGDESC},
  {ID_FRAME,    FrameMarginHeight, T_String, ATTR_MARGINHEIGHT},
  {ID_FRAME,    FrameMarginWidth,  T_String, ATTR_MARGINWIDTH},
  {ID_FRAME,    FrameName,         T_String, ATTR_NAME},
  {ID_FRAME,    FrameNoResize,     T_Bool,   ATTR_NORESIZE},
  {ID_FRAME,    FrameScrolling,    T_String, ATTR_SCROLLING},
  {ID_FRAME,    FrameSrc,          T_String, ATTR_SRC}, //### not URL?
  {ID_FRAME,    FrameLocation,     BoundPropType(T_String | T_ReadOnly), ATTR_SRC},
  {ID_IFRAME,   IFrameFrameBorder, T_String, ATTR_FRAMEBORDER},
  {ID_IFRAME,   IFrameLongDesc,    T_String, ATTR_LONGDESC},
  {ID_IFRAME,   IFrameMarginHeight,T_String, ATTR_MARGINHEIGHT},
  {ID_IFRAME,   IFrameMarginWidth, T_String, ATTR_MARGINWIDTH},
  {ID_IFRAME,   IFrameName,        T_String, ATTR_NAME},
  {ID_IFRAME,   IFrameScrolling,   T_String, ATTR_SCROLLING},
  {ID_IFRAME,   IFrameSrc,         T_URL,    ATTR_SRC},
  {ID_IFRAME,   IFrameAlign,       T_String, ATTR_ALIGN},
  {ID_IFRAME,   IFrameHeight,      T_String, ATTR_HEIGHT},
  {ID_IFRAME,   IFrameWidth,       T_String, ATTR_WIDTH},
  {NotApplicable,ElementId,         T_String, ATTR_ID},
  {NotApplicable,ElementTitle,      T_String, ATTR_TITLE},
  {NotApplicable,ElementLang,       T_String, ATTR_LANG},
  {NotApplicable,ElementDir,        T_String, ATTR_DIR},
  {NotApplicable,ElementClassName,  T_String, ATTR_CLASS},
  {NotApplicable,ElementChildren,   T_Coll,   HTMLCollectionImpl::NODE_CHILDREN},
  {0,           0,                 T_Res,    0},
};

QHash<int, const HTMLElement::BoundPropInfo*>* HTMLElement::s_boundPropInfo = 0;

QHash<int, const HTMLElement::BoundPropInfo*>* HTMLElement::boundPropInfo()
{
  if (!s_boundPropInfo) {
    s_boundPropInfo = new QHash<int, const BoundPropInfo*>();
    for (int c = 0; bpTable[c].elId; ++c) {
      s_boundPropInfo->insert(bpTable[c].token, &bpTable[c]);
    }
  }
  return s_boundPropInfo;
}

QString KJS::HTMLElement::getURLArg(unsigned id) const
{
  DOMString rel = khtml::parseURL(impl()->getAttribute(id));
  return !rel.isNull() ? impl()->document()->completeURL(rel.string()) : QString();
}

DOM::HTMLElementImpl *toHTMLElement(JSValue *val) {
  DOM::ElementImpl* e = toElement(val);
  if (e && e->isHTMLElement())
    return static_cast<HTMLElementImpl*>(e);
  return 0;
}

DOM::HTMLTableCaptionElementImpl *toHTMLTableCaptionElement(JSValue *val)
{
    DOM::ElementImpl *e = toElement(val);
    if (e && e->id() == ID_CAPTION)
        return static_cast<HTMLTableCaptionElementImpl *>(e);
    return 0;
}

HTMLTableSectionElementImpl *toHTMLTableSectionElement(JSValue *val)
{
    DOM::ElementImpl *e = toElement(val);
    if (e && (e->id() == ID_THEAD || e->id() == ID_TBODY || e->id() == ID_TFOOT))
        return static_cast<HTMLTableSectionElementImpl *>(e);
    return 0;
}

JSValue* KJS::HTMLElement::handleBoundRead(ExecState* exec, int token) const
{
  const BoundPropInfo* prop = boundPropInfo()->value(token);
  if (!prop) return 0;

  assert(prop->elId == NotApplicable || prop->elId == impl()->id());

  switch (prop->type & ~T_ReadOnly) {
    case T_String:
      return jsString(impl()->getAttribute(prop->attrId));
    case T_StrOrNl:
      return getStringOrNull(impl()->getAttribute(prop->attrId));
    case T_Bool:
      return jsBoolean(!impl()->getAttribute(prop->attrId).isNull());
    case T_Int:
      return jsNumber(impl()->getAttribute(prop->attrId).toInt());
    case T_URL:
      return jsString(getURLArg(prop->attrId));
    case T_Res:
      return jsString("");
    case T_Coll:
      return getHTMLCollection(exec, new HTMLCollectionImpl(impl(), prop->attrId));
  }
  assert(0);
  return 0;
}

KJS::Window* KJS::HTMLElement::ourWindow() const
{
    KHTMLPart* part = impl()->document()->part();
    if (part)
        return Window::retrieveWindow(part);
    else
        return 0;
}

JSValue* KJS::HTMLElement::getWindowListener(ExecState* exec, int ev) const
{
    if (KJS::Window* win = ourWindow()) {
        return win->getListener(exec, ev);
    } else {
        return jsNull();
    }
}

void KJS::HTMLElement::setWindowListener(ExecState* exec, int ev, JSValue* val) const
{
    if (KJS::Window* win = ourWindow()) {
        win->setListener(exec, ev, val);
    }
}

JSValue* KJS::HTMLElement::getValueProperty(ExecState *exec, int token) const
{
  JSValue* cand = handleBoundRead(exec, token);
  if (cand) return cand;

  DOM::HTMLElementImpl& element = *impl();
  switch (element.id()) {
  case ID_LINK: {
    DOM::HTMLLinkElementImpl& link = static_cast<DOM::HTMLLinkElementImpl&>(element);
    switch (token) {
    case LinkSheet:           return getDOMStyleSheet(exec,link.sheet());
    }
  }
  break;
  case ID_TITLE: {
    DOM::HTMLTitleElementImpl& title = static_cast<DOM::HTMLTitleElementImpl&>(element);
    switch (token) {
    case TitleText:                 return jsString(title.text());
    }
  }
  break;
  case ID_ISINDEX: {
    DOM::HTMLIsIndexElementImpl& isindex = static_cast<DOM::HTMLIsIndexElementImpl&>(element);
    switch (token) {
    case IsIndexForm:            return getDOMNode(exec,isindex.form()); // type HTMLFormElement
    case IsIndexPrompt:          return jsString(isindex.prompt());
    }
  }
  break;
  case ID_STYLE: {
    DOM::HTMLStyleElementImpl& style = static_cast<DOM::HTMLStyleElementImpl&>(element);
    switch (token) {
    case StyleSheet:           return getDOMStyleSheet(exec,style.sheet());
    }
  }
  break;
  case ID_BODY: {
    switch (token) {
    case BodyOnLoad:
        return getWindowListener(exec, DOM::EventImpl::LOAD_EVENT);
    case BodyOnError:
        return getWindowListener(exec, DOM::EventImpl::ERROR_EVENT);
    case BodyOnBlur:
        return getWindowListener(exec, DOM::EventImpl::BLUR_EVENT);
    case BodyOnFocus:
        return getWindowListener(exec, DOM::EventImpl::FOCUS_EVENT);
    case BodyOnMessage:
        return getWindowListener(exec, DOM::EventImpl::MESSAGE_EVENT);
    case BodyOnHashChange:
        return getWindowListener(exec, DOM::EventImpl::HASHCHANGE_EVENT);

    }
  }
  break;

  case ID_FRAMESET: {
    switch (token) {
    case FrameSetOnMessage:
        return getWindowListener(exec, DOM::EventImpl::MESSAGE_EVENT);
    }
  }
  break;  

  case ID_FORM: {
    DOM::HTMLFormElementImpl& form = static_cast<DOM::HTMLFormElementImpl&>(element);
    switch (token) {
    case FormElements:        return getHTMLCollection(exec,form.elements());
    case FormLength:          return jsNumber(form.length());
    }
  }
  break;

  case ID_SELECT: {
    DOM::HTMLSelectElementImpl& select = static_cast<DOM::HTMLSelectElementImpl&>(element);
    switch (token) {
    case SelectType:            return jsString(select.type());
    case SelectSelectedIndex:   return jsNumber(select.selectedIndex());
    case SelectValue:           return jsString(select.value());
    case SelectLength:          return jsNumber(select.length());
    case SelectForm:            return getDOMNode(exec,select.form()); // type HTMLFormElement
    case SelectOptions:         return getSelectHTMLCollection(exec, select.options(), &select); // type HTMLCollection
    case SelectName:            return jsString(select.name());
    }
  }
  break;
  case ID_OPTION: {
    DOM::HTMLOptionElementImpl& option = static_cast<DOM::HTMLOptionElementImpl&>(element);
    switch (token) {
    case OptionForm:            return getDOMNode(exec,option.form()); // type HTMLFormElement
    case OptionText:            return jsString(option.text());
    case OptionIndex:           return jsNumber(option.index());
    case OptionSelected:        return jsBoolean(option.selected());
    case OptionValue:           return jsString(option.value());
    }
  }
  break;

  case ID_INPUT: {
    DOM::HTMLInputElementImpl& input = static_cast<DOM::HTMLInputElementImpl&>(element);
    switch (token) {
    case InputForm:            return getDOMNode(exec,input.form()); // type HTMLFormElement
    case InputChecked:         return jsBoolean(input.checked());
    case InputIndeterminate:   return jsBoolean(input.indeterminate());
    case InputName:            return jsString(input.name()); // NOT getString (IE gives empty string)
    case InputType:            return jsString(input.type());
    case InputValue:           return jsString(input.value());
    case InputSelectionStart:  {
        long val = input.selectionStart();
        if (val != -1)
          return jsNumber(val);
        else
          return jsUndefined();
      }
    case InputSelectionEnd:  {
        long val = input.selectionEnd();
        if (val != -1)
          return jsNumber(val);
        else
          return jsUndefined();
      }
    case InputPlaceholder: {
         return jsString(input.placeholder());
      }
    }
  }
  break;
  case ID_TEXTAREA: {
    DOM::HTMLTextAreaElementImpl& textarea = static_cast<DOM::HTMLTextAreaElementImpl&>(element);
    switch (token) {
    case TextAreaDefaultValue:    return jsString(textarea.defaultValue());
    case TextAreaForm:            return getDOMNode(exec,textarea.form()); // type HTMLFormElement
    case TextAreaName:            return jsString(textarea.name());
    case TextAreaType:            return jsString(textarea.type());
    case TextAreaValue:           return jsString(textarea.value());
    case TextAreaSelectionStart:  return jsNumber(textarea.selectionStart());
    case TextAreaSelectionEnd:    return jsNumber(textarea.selectionEnd());
    case TextAreaTextLength:      return jsNumber(textarea.textLength());
    case TextAreaPlaceholder:     return jsString(textarea.placeholder());
    }
  }
  break;

  case ID_BUTTON: {
    DOM::HTMLButtonElementImpl& button = static_cast<DOM::HTMLButtonElementImpl&>(element);
    switch (token) {
    case ButtonForm:            return getDOMNode(exec,button.form()); // type HTMLFormElement
    case ButtonType:            return jsString(button.type());
    }
  }
  break;
  case ID_LABEL: {
    DOM::HTMLLabelElementImpl& label = static_cast<DOM::HTMLLabelElementImpl&>(element);
    switch (token) {
    case LabelForm:            return getDOMNode(exec,label.form()); // type HTMLFormElement
    }
  }
  break;
  case ID_FIELDSET: {
    DOM::HTMLFieldSetElementImpl& fieldSet = static_cast<DOM::HTMLFieldSetElementImpl&>(element);
    switch (token) {
    case FieldSetForm:            return getDOMNode(exec,fieldSet.form()); // type HTMLFormElement
    }
  }
  break;
  case ID_LEGEND: {
    DOM::HTMLLegendElementImpl& legend = static_cast<DOM::HTMLLegendElementImpl&>(element);
    switch (token) {
    case LegendForm:            return getDOMNode(exec,legend.form()); // type HTMLFormElement
    }
  }
  break;
  case ID_A: {
    DOM::HTMLAnchorElementImpl& anchor = static_cast<DOM::HTMLAnchorElementImpl&>(element);
    QString href = getURLArg(ATTR_HREF);
    switch (token) {
    case AnchorHash:            return jsString('#'+QUrl(href).fragment());
    case AnchorHost:            return jsString(QUrl(href).host());
    case AnchorHostname: {
      QUrl url(href);
      // qDebug() << "anchor::hostname uses:" <<url.toString();
      if (url.port()<=0)
        return jsString(url.host());
      else
        return jsString(url.host() + ":" + QString::number(url.port()));
    }
    case AnchorPathName:        return jsString(QUrl(href).path());
    case AnchorPort: {
        int port = QUrl(href).port();
        if (port > 0)
            return jsString(QString::number(port));
        return jsString("");
    }
    case AnchorProtocol:        return jsString(QUrl(href).scheme()+":");
    case AnchorSearch:          { QUrl u(href);
                                  QString q = u.query();
                                  if (q.length() == 1)
                                    return jsString("");
                                  return jsString(q); }
    // Not specified in http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/a.asp
    // Mozilla returns the inner text.
    case AnchorText:            return jsString(anchor.innerText());
    }
  }
  break;
  case ID_IMG: {
    DOM::HTMLImageElementImpl& image = static_cast<DOM::HTMLImageElementImpl&>(element);
    switch (token) {
    case ImageComplete:        return jsBoolean(image.complete());
    case ImageHeight:          return jsNumber(image.height());
    case ImageWidth:           return jsNumber(image.width());
    case ImageX:               return jsNumber(image.x());
    case ImageY:               return jsNumber(image.y());
    }
  }
  break;
  case ID_CANVAS: {
    DOM::HTMLCanvasElementImpl& canvas = static_cast<DOM::HTMLCanvasElementImpl&>(element);
    switch (token) {
    case CanvasHeight:          return jsNumber(canvas.height());
    case CanvasWidth:           return jsNumber(canvas.width());
    }
  }
  break;
  case ID_OBJECT: {
    DOM::HTMLObjectElementImpl& object = static_cast<DOM::HTMLObjectElementImpl&>(element);
    switch (token) {
    case ObjectForm:            return getDOMNode(exec,object.form()); // type HTMLFormElement
    case ObjectContentDocument: return checkNodeSecurity(exec,object.contentDocument()) ?
				       getDOMNode(exec, object.contentDocument()) : jsUndefined();
    }
  }
  break;
  case ID_AREA: {
    DOM::HTMLAreaElementImpl& area = static_cast<DOM::HTMLAreaElementImpl&>(element);
    // Everything here needs href
    DOM::Document doc = area.ownerDocument();
    DOM::DOMString href = getURLArg(ATTR_HREF);
    QUrl url;
    if ( !href.isNull() ) {
      url = QUrl(href.string());
      if ( href.isEmpty() ) {
        url = url.adjusted(QUrl::RemoveFilename); // href="" clears the filename (in IE)
      }
    }
    switch(token) {
      case AreaHref:
        return jsString(url.url());
      case AreaHash:            return jsString(url.isEmpty() ? "" : QString('#'+url.fragment()));
      case AreaHost:            return jsString(url.host());
      case AreaHostName: {
        if (url.port()<=0)
          return jsString(url.host());
        else
          return jsString(url.host() + ":" + QString::number(url.port()));
      }
      case AreaPathName:        {
        return jsString(url.path());
      }
      case AreaPort:            return jsString(QString::number(url.port()));
      case AreaProtocol:        return jsString(url.isEmpty() ? "" : QString(url.scheme()+":"));
      case AreaSearch:          return jsString(url.query());
    }
  }
  break;
  case ID_SCRIPT: {
    DOM::HTMLScriptElementImpl& script = static_cast<DOM::HTMLScriptElementImpl&>(element);
    switch (token) {
    case ScriptText:            return jsString(script.text());
    }
  }
  break;
  case ID_TABLE: {
    DOM::HTMLTableElementImpl& table = static_cast<DOM::HTMLTableElementImpl&>(element);
    switch (token) {
    case TableCaption:         return getDOMNode(exec,table.caption()); // type HTMLTableCaptionElement
    case TableTHead:           return getDOMNode(exec,table.tHead()); // type HTMLTableSectionElement
    case TableTFoot:           return getDOMNode(exec,table.tFoot()); // type HTMLTableSectionElement
    }
  }
  break;
  case ID_TR: {
   DOM::HTMLTableRowElementImpl& tableRow = static_cast<DOM::HTMLTableRowElementImpl&>(element);
   switch (token) {
   case TableRowRowIndex:        return jsNumber(tableRow.rowIndex());
   case TableRowSectionRowIndex: return jsNumber(tableRow.sectionRowIndex());
   }
  }
  break;
  case ID_TH:
  case ID_TD: {
    DOM::HTMLTableCellElementImpl& tableCell = static_cast<DOM::HTMLTableCellElementImpl&>(element);
    switch (token) {
    case TableCellCellIndex:       return jsNumber(tableCell.cellIndex());
    }
  }
  break;
  case ID_LAYER: {
    //DOM::HTMLLayerElementImpl& layerElement = static_cast<DOM::HTMLLayerElementImpl&>(element);
    switch (token) {
    /*case LayerClip:           return getLayerClip(exec, layerElement); */
    case LayerDocument:       return jsUndefined();
    }
  }
  break;
  case ID_FRAME: {
    DOM::HTMLFrameElementImpl& frameElement = static_cast<DOM::HTMLFrameElementImpl&>(element);
    switch (token) {
      case FrameContentDocument: return checkNodeSecurity(exec,frameElement.contentDocument()) ?
				      getDOMNode(exec, frameElement.contentDocument()) : jsUndefined();
    case FrameContentWindow:   {
        KHTMLPart* part = frameElement.contentPart();
        if (part) {
          Window *w = Window::retrieveWindow(part);
          if (w)
            return w;
        }
        return jsUndefined();
      }
    // IE only
    case FrameWidth:
    case FrameHeight:
      {
          frameElement.document()->updateLayout();
          khtml::RenderObject* r = frameElement.renderer();
          return jsNumber( r ? (token == FrameWidth ? r->width() : r->height()) : 0 );
      }
    }
  }
  break;
  case ID_IFRAME: {
    DOM::HTMLIFrameElementImpl& iFrame = static_cast<DOM::HTMLIFrameElementImpl&>(element);
    switch (token) {
    case IFrameContentDocument: return checkNodeSecurity(exec,iFrame.contentDocument()) ?
				       getDOMNode(exec, iFrame.contentDocument()) : jsUndefined();
    case IFrameContentWindow:       {
        KHTMLPart* part = iFrame.contentPart();
        if (part) {
          Window *w = Window::retrieveWindow(part);
          if (w)
            return w;
        }
        return jsUndefined();
    }
    }
    break;
  }
  } // xemacs (or arnt) could be a bit smarter when it comes to indenting switch()es ;)
  // it is not arnt to blame - it is the original Stroustrup style we like :) (Dirk)

  // generic properties
  switch (token) {
  case ElementInnerHTML:
    return jsString(element.innerHTML());
  case ElementInnerText:
    return jsString(element.innerText());
  case ElementDocument:
    return getDOMNode(exec,element.ownerDocument());
  case ElementAll:
    // Disable element.all when we try to be Netscape-compatible
    if ( exec->dynamicInterpreter()->compatMode() == Interpreter::NetscapeCompat )
      return jsUndefined();
    else
    if ( exec->dynamicInterpreter()->compatMode() == Interpreter::IECompat )
      return getHTMLCollection(exec,new HTMLCollectionImpl(&element, HTMLCollectionImpl::DOC_ALL));
    else // Enabled but hidden by default
      return getHTMLCollection(exec,new HTMLCollectionImpl(&element, HTMLCollectionImpl::DOC_ALL), true);
  case ElementTabIndex:
      return jsNumber(element.tabIndex());
  // ### what about style? or is this used instead for DOM2 stylesheets?
  case ElementContentEditable:
      return jsString(element.contentEditable());
  case ElementIsContentEditable:
      return jsBoolean(element.isContentEditable());
  }
  qCritical() << "HTMLElement::getValueProperty unhandled token " << token << endl;
  return jsUndefined();
}

UString KJS::HTMLElement::toString(ExecState *exec) const
{
  if (impl()->id() == ID_A)
    return UString(getURLArg(ATTR_HREF));
#if 0 // ### debug stuff?
  else if (impl()->id() == ID_APPLET) {
    KParts::LiveConnectExtension *lc = getLiveConnectExtension(*impl());
    QStringList qargs;
    QString retvalue;
    KParts::LiveConnectExtension::Type rettype;
    unsigned long retobjid;
    if (lc && lc->call(0, "hashCode", qargs, rettype, retobjid, retvalue)) {
      QString str("[object APPLET ref=");
      return UString(str + retvalue + QString("]"));
    }
  }
#endif
  else if (impl()->id() == ID_IMG) {
    DOMString alt = impl()->getAttribute(ATTR_ALT);
    if (!alt.isEmpty())
      return UString(alt) + " " + DOMElement::toString(exec);
  }
  return DOMElement::toString(exec);
}

static DOM::HTMLFormElementImpl* getForm(const DOM::HTMLElementImpl* element)
{
  switch (element->id()) {
    case ID_ISINDEX:
    case ID_SELECT:
    case ID_OPTION:
    case ID_INPUT:
    case ID_TEXTAREA:
    case ID_LABEL:
    case ID_FIELDSET:
    case ID_LEGEND: {
      const DOM::HTMLGenericFormElementImpl* fEl = static_cast<const HTMLGenericFormElementImpl*>(element);
      return fEl->form();
    }
    case ID_OBJECT: {
      const DOM::HTMLObjectElementImpl* oEl = static_cast<const HTMLObjectElementImpl*>(element);
      return oEl->form();
    }
    default:
      return 0;
  }
}

void KJS::HTMLElement::pushEventHandlerScope(ExecState *exec, ScopeChain &scope) const
{
  DOM::HTMLElementImpl& element = *impl();

  // The document is put on first, fall back to searching it only after the element and form.
  scope.push(static_cast<JSObject *>(getDOMNode(exec, element.document())));

  // The form is next, searched before the document, but after the element itself.
  DOM::HTMLFormElementImpl* formElt;

  // First try to obtain the form from the element itself.  We do this to deal with
  // the malformed case where <form>s aren't in our parent chain (e.g., when they were inside
  // <table> or <tbody>.
  formElt = getForm(impl());
  if (formElt)
    scope.push(static_cast<JSObject *>(getDOMNode(exec, formElt)));
  else {
    DOM::NodeImpl* form = element.parentNode();
    while (form && form->id() != ID_FORM)
        form = form->parentNode();

    if (form)
        scope.push(static_cast<JSObject *>(getDOMNode(exec, form)));
  }

  // The element is on top, searched first.
  scope.push(static_cast<JSObject *>(getDOMNode(exec, &element)));
}

JSValue* KJS::HTMLElementFunction::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( HTMLElement, thisObj );
  DOMExceptionTranslator exception(exec);

#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLElementFunction::callAsFunction ";
#endif
  DOM::HTMLElementImpl& element = *static_cast<KJS::HTMLElement *>(thisObj)->impl();

  switch (element.id()) {
    case ID_FORM: {
      DOM::HTMLFormElementImpl& form = static_cast<DOM::HTMLFormElementImpl&>(element);
      if (id == KJS::HTMLElement::FormSubmit) {

        KHTMLPart *part = element.document()->part();
        KHTMLSettings::KJSWindowOpenPolicy policy = KHTMLSettings::KJSWindowOpenAllow;
        if (part)
            policy = part->settings()->windowOpenPolicy(part->url().host());

        bool block = false;
        if ( policy != KHTMLSettings::KJSWindowOpenAllow ) {
          block = true;

          // if this is a form without a target, don't block
          if ( form.target().isEmpty() )
            block = false;

          QString caption;

          // if there is a frame with the target name, don't block
          if ( part )  {
            if (!part->url().host().isEmpty())
              caption = part->url().host() + " - ";

            if ( Window::targetIsExistingWindow(part, form.target().string()) )
              block = false;
          }

          if ( block && policy == KHTMLSettings::KJSWindowOpenAsk && part ) {
            if  (part )
              emit part->browserExtension()->requestFocus(part);
            caption += i18n( "Confirmation: JavaScript Popup" );
            if ( KMessageBox::questionYesNo(part->view(), form.action().isEmpty() ?
                   i18n( "This site is submitting a form which will open up a new browser "
                         "window via JavaScript.\n"
                         "Do you want to allow the form to be submitted?" ) :
                   i18n( "<qt>This site is submitting a form which will open <p>%1</p> in a new browser window via JavaScript.<br />"
                         "Do you want to allow the form to be submitted?</qt>", KStringHandler::csqueeze(form.action().string(),  100)),
                   caption, KGuiItem(i18n("Allow")), KGuiItem(i18n("Do Not Allow")) ) == KMessageBox::Yes )
              block = false;

          } else if ( block && policy == KHTMLSettings::KJSWindowOpenSmart ) {
            if( static_cast<KJS::ScriptInterpreter *>(exec->dynamicInterpreter())->isWindowOpenAllowed() ) {
              // This submission has been triggered by the user
              block = false;
            }
          }
        }

        if( !block )
          form.submit();

        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::FormReset) {
        form.reset();
        return jsUndefined();
      }
    }
    break;
    case ID_BODY: {
      if (id == KJS::HTMLElement::BodyFocus) {
        // Just blur everything. Not perfect, but good enough for now
        element.document()->setFocusNode(0);
      }
    }
    break;
    case ID_SELECT: {
      DOM::HTMLSelectElementImpl& select = static_cast<DOM::HTMLSelectElementImpl&>(element);
      if (id == KJS::HTMLElement::SelectAdd) {
        select.add(toHTMLElement(args[0]),toHTMLElement(args[1]),exception);
        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::SelectItem) {
        SharedPtr<DOM::HTMLCollectionImpl> opts = select.options();
        return getDOMNode(exec, opts->item(static_cast<unsigned long>(args[0]->toNumber(exec))));
      }
      else if (id == KJS::HTMLElement::SelectRemove) {
        // Apparently this takes both elements and indices (ebay.fr)
        DOM::NodeImpl* node = toNode(args[0]);
        if (node && node->id() == ID_OPTION)
          select.removeChild(node, exception);
        else
          select.remove(int(args[0]->toNumber(exec)));
        return jsUndefined();
      }
    }
    break;
    case ID_INPUT: {
      DOM::HTMLInputElementImpl& input = static_cast<DOM::HTMLInputElementImpl&>(element);
      if (id == KJS::HTMLElement::InputSelect) {
        input.select();
        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::InputClick) {
        input.click();
        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::InputSetSelectionRange) {
        input.setSelectionRange(args[0]->toNumber(exec), args[1]->toNumber(exec));
        return jsUndefined();
      }
    }
    break;
    case ID_BUTTON: {
      DOM::HTMLButtonElementImpl& button = static_cast<DOM::HTMLButtonElementImpl&>(element);
      if (id == KJS::HTMLElement::ButtonClick) {
        button.click();
        return jsUndefined();
      }
    }
    break;
    case ID_TEXTAREA: {
      DOM::HTMLTextAreaElementImpl& textarea = static_cast<DOM::HTMLTextAreaElementImpl&>(element);
      if (id == KJS::HTMLElement::TextAreaSelect) {
        textarea.select();
        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::TextAreaSetSelectionRange) {
        textarea.setSelectionRange(args[0]->toNumber(exec), args[1]->toNumber(exec));
        return jsUndefined();
      }

    }
    break;
    case ID_A: {
      DOM::HTMLAnchorElementImpl& anchor = static_cast<DOM::HTMLAnchorElementImpl&>(element);
      if (id == KJS::HTMLElement::AnchorClick) {
        anchor.click();
        return jsUndefined();
      } else if (id == KJS::HTMLElement::AnchorToString) {
        return jsString(static_cast<KJS::HTMLElement *>(thisObj)->toString(exec));
      }
    }
    break;
    case ID_TABLE: {
      DOM::HTMLTableElementImpl& table = static_cast<DOM::HTMLTableElementImpl&>(element);
      if (id == KJS::HTMLElement::TableCreateTHead)
        return getDOMNode(exec,table.createTHead());
      else if (id == KJS::HTMLElement::TableDeleteTHead) {
        table.deleteTHead();
        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::TableCreateTFoot)
        return getDOMNode(exec,table.createTFoot());
      else if (id == KJS::HTMLElement::TableDeleteTFoot) {
        table.deleteTFoot();
        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::TableCreateCaption)
        return getDOMNode(exec,table.createCaption());
      else if (id == KJS::HTMLElement::TableDeleteCaption) {
        table.deleteCaption();
        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::TableInsertRow)
        return getDOMNode(exec,table.insertRow(args[0]->toInteger(exec),exception));
      else if (id == KJS::HTMLElement::TableDeleteRow) {
        table.deleteRow(args[0]->toInteger(exec), exception);
        return jsUndefined();
      }
    }
    break;
    case ID_THEAD:
    case ID_TBODY:
    case ID_TFOOT: {
      DOM::HTMLTableSectionElementImpl& tableSection = static_cast<DOM::HTMLTableSectionElementImpl&>(element);
      if (id == KJS::HTMLElement::TableSectionInsertRow)
        return getDOMNode(exec,tableSection.insertRow(args[0]->toInteger(exec), exception));
      else if (id == KJS::HTMLElement::TableSectionDeleteRow) {
        tableSection.deleteRow(args[0]->toInteger(exec),exception);
        return jsUndefined();
      }
    }
    break;
    case ID_TR: {
      DOM::HTMLTableRowElementImpl& tableRow = static_cast<DOM::HTMLTableRowElementImpl&>(element);
      if (id == KJS::HTMLElement::TableRowInsertCell)
        return getDOMNode(exec,tableRow.insertCell(args[0]->toInteger(exec), exception));
      else if (id == KJS::HTMLElement::TableRowDeleteCell) {
        tableRow.deleteCell(args[0]->toInteger(exec), exception);
        return jsUndefined();
      }
      break;
    }
    case ID_MARQUEE: {
      if (id == KJS::HTMLElement::MarqueeStart && element.renderer() &&
        element.renderer()->layer() &&
        element.renderer()->layer()->marquee()) {
        element.renderer()->layer()->marquee()->start();
        return jsUndefined();
      }
      else if (id == KJS::HTMLElement::MarqueeStop && element.renderer() &&
              element.renderer()->layer() &&
              element.renderer()->layer()->marquee()) {
        element.renderer()->layer()->marquee()->stop();
        return jsUndefined();
      }
      break;
    }
  case ID_CANVAS: {
      DOM::HTMLCanvasElementImpl& canvasEl = static_cast<DOM::HTMLCanvasElementImpl&>(element);
      if (id == KJS::HTMLElement::CanvasGetContext) {
        if (args[0]->toString(exec) == "2d")
          return getWrapper<Context2D>(exec, canvasEl.getContext2D());
        return jsNull();
      } else if (id == KJS::HTMLElement::CanvasToDataURL) {
        return jsString(canvasEl.toDataURL(exception));
      }
      break;
    }
  case ID_IFRAME: {
      DOM::HTMLIFrameElementImpl& iFrame = static_cast<DOM::HTMLIFrameElementImpl&>(element);
      if (id == KJS::HTMLElement::IFrameGetSVGDocument) {
          if (!checkNodeSecurity(exec,iFrame.contentDocument()) || !iFrame.contentDocument() || !iFrame.contentDocument()->isSVGDocument())
              return jsUndefined();
          return getDOMNode(exec, iFrame.contentDocument());
      }
    }
  case ID_OBJECT: {
      DOM::HTMLObjectElementImpl& object = static_cast<DOM::HTMLObjectElementImpl&>(element);
      if (id == KJS::HTMLElement::ObjectGetSVGDocument) {
          if (!checkNodeSecurity(exec,object.contentDocument()) || !object.contentDocument() || !object.contentDocument()->isSVGDocument())
              return jsUndefined();
          return getDOMNode(exec, object.contentDocument());
      }
    }
  }

  if (id == HTMLElement::ElementScrollIntoView) {
    bool alignToTop = true;
    if (args.size() > 0)
      alignToTop = args[0]->toBoolean(exec);
    element.scrollIntoView(alignToTop);
    return jsUndefined();
  }

  return jsUndefined();
}

void KJS::HTMLElement::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr)
{
#ifdef KJS_VERBOSE
  DOM::DOMString str = value->type() == NullType ? DOM::DOMString() : value->toString(exec).domString();
#endif
  DOM::HTMLElementImpl& element = *impl();
#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLElement::tryPut " << propertyName.qstring()
                << " thisTag=" << element.tagName().string()
                << " str=" << str.string() << endl;
#endif
  //

  // First look at dynamic properties
  switch (element.id()) {
    case ID_SELECT: {
      DOM::HTMLSelectElementImpl& select = static_cast<DOM::HTMLSelectElementImpl&>(element);
      bool ok;
      /*uint u =*/ propertyName.qstring().toULong(&ok);
      if (ok) {
        JSObject *coll = getSelectHTMLCollection(exec, select.options(), &select)->getObject();
        if ( coll )
          coll->put(exec,propertyName,value);
        return;
      }
      break;
    }
    case ID_APPLET:
    case ID_OBJECT:
    case ID_EMBED: {
      KParts::ScriptableExtension* se = getScriptableExtension(element);
      if (pluginRootPut(exec, se, propertyName, value))
        return;

      break;
    }
    default:
      break;
  }

  const HashTable* table = classInfo()->propHashTable; // get the right hashtable
  const HashEntry* entry = table ? Lookup::findEntry(table, propertyName) : 0;
  if (entry) {
      if (entry->attr & Function) { // function: put as override property
          JSObject::put(exec, propertyName, value, attr);
          return;
      }
      else if (!(entry->attr & ReadOnly)) { // let lookupPut print the warning if read-only
          putValueProperty(exec, entry->value, value, attr);
          return;
      }
  }

  lookupPut<KJS::HTMLElement, DOMElement>(exec, propertyName, value, attr, &HTMLElementTable, this);
}


bool KJS::HTMLElement::handleBoundWrite(ExecState* exec, int token, JSValue* value)
{
  const BoundPropInfo* prop = boundPropInfo()->value(token);
  if (!prop) return false;
  if (prop->type & T_ReadOnly) return false;

  DOM::DOMString str = value->type() == NullType ? DOM::DOMString() : value->toString(exec).domString();

  assert(prop->elId == NotApplicable || prop->elId == impl()->id());


  switch (prop->type) {
    case T_String:
    case T_StrOrNl:
    case T_URL:
      impl()->setAttribute(prop->attrId, str);
      return true;
    case T_Int:
      impl()->setAttribute(prop->attrId, QString::number(value->toInteger(exec)));
      return true;
    case T_Bool:
      impl()->setAttribute(prop->attrId, value->toBoolean(exec) ? "" : 0);
      return true;
    case T_Res: //ignored
      return true;
    default:
      assert(0);
      return false;
  }
}


void KJS::HTMLElement::putValueProperty(ExecState *exec, int token, JSValue *value, int)
{
  if (handleBoundWrite(exec, token, value))
    return;

  DOMExceptionTranslator exception(exec);
  DOM::DOMString str = value->type() == NullType ? DOM::DOMString() : value->toString(exec).domString();
  DOM::HTMLElementImpl& element = *impl();
#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLElement::putValueProperty "
                << " thisTag=" << element.tagName().string()
                << " token=" << token << endl;
#endif

  switch (element.id()) {
    case ID_TITLE: {
      DOM::HTMLTitleElementImpl& title = static_cast<DOM::HTMLTitleElementImpl&>(element);
      switch (token) {
      case TitleText:                 { title.setText(str); return; }
      }
    }
    break;
    case ID_ISINDEX: {
      DOM::HTMLIsIndexElementImpl& isindex = static_cast<DOM::HTMLIsIndexElementImpl&>(element);
      switch (token) {
      // read-only: form
      case IsIndexPrompt:               { isindex.setPrompt(str); return; }
      }
    }
    break;
    case ID_BODY: {
      //DOM::HTMLBodyElementImpl& body = static_cast<DOM::HTMLBodyElementImpl&>(element);
      switch (token) {
        case BodyOnLoad:
            setWindowListener(exec, DOM::EventImpl::LOAD_EVENT, value);
            break;
        case BodyOnError:
            setWindowListener(exec, DOM::EventImpl::ERROR_EVENT, value);
            break;
        case BodyOnBlur:
            setWindowListener(exec, DOM::EventImpl::BLUR_EVENT, value);
            break;
        case BodyOnFocus:
            setWindowListener(exec, DOM::EventImpl::FOCUS_EVENT, value);
            break;
        case BodyOnMessage:
            setWindowListener(exec, DOM::EventImpl::MESSAGE_EVENT, value);
            break;
        case BodyOnHashChange:
            setWindowListener(exec, DOM::EventImpl::HASHCHANGE_EVENT, value);
            break;
      }
    }
    case ID_FRAMESET: {
        switch (token) {
        case FrameSetOnMessage:
            setWindowListener(exec, DOM::EventImpl::MESSAGE_EVENT, value);
            break;        
        }
    }
    break;
    case ID_SELECT: {
      DOM::HTMLSelectElementImpl& select = static_cast<DOM::HTMLSelectElementImpl&>(element);
      switch (token) {
      // read-only: type
      case SelectSelectedIndex:   { select.setSelectedIndex(value->toInteger(exec)); return; }
      case SelectValue:           { select.setValue(str.implementation()); return; }
      case SelectLength:          { // read-only according to the NS spec, but webpages need it writeable
                                         JSObject *coll = getSelectHTMLCollection(exec, select.options(), &select)->getObject();

                                         if ( coll )
                                             coll->put(exec, "length", value);
                                         return;
                                       }
      // read-only: form
      // read-only: options
      case SelectName:            { select.setName(str); return; }
      }
    }
    break;
    case ID_OPTION: {
      DOM::HTMLOptionElementImpl& option = static_cast<DOM::HTMLOptionElementImpl&>(element);
      switch (token) {
      // read-only: form
      // read-only: text  <--- According to the DOM, but JavaScript and JScript both allow changes.
      // So, we'll do it here and not add it to our DOM headers.
      case OptionText:            { SharedPtr<DOM::NodeListImpl> nl(option.childNodes());
                                    for (unsigned int i = 0; i < nl->length(); i++) {
                                        if (nl->item(i)->nodeType() == DOM::Node::TEXT_NODE) {
                                            static_cast<DOM::TextImpl*>(nl->item(i))->setData(str, exception);
                                            return;
                                        }
                                  }
                                  // No child text node found, creating one
                                  DOM::TextImpl* t = option.document()->createTextNode(str.implementation());
                                  int dummyexception;
                                  option.appendChild(t, dummyexception); // #### exec->setException ?
                                  return;
      }
      // read-only: index
      case OptionSelected:        { option.setSelected(value->toBoolean(exec)); return; }
      case OptionValue:           { option.setValue(str.implementation()); return; }
      }
    }
    break;
    case ID_INPUT: {
      DOM::HTMLInputElementImpl& input = static_cast<DOM::HTMLInputElementImpl&>(element);
      switch (token) {
      case InputChecked:         { input.setChecked(value->toBoolean(exec)); return; }
      case InputIndeterminate:   { input.setIndeterminate(value->toBoolean(exec)); return; }
      case InputName:            { input.setName(str); return; }
      case InputType:            { input.setType(str); return; }
      case InputValue:           { input.setValue(str); return; }
      case InputSelectionStart:  { input.setSelectionStart(value->toInteger(exec)); return; }
      case InputSelectionEnd:    { input.setSelectionEnd  (value->toInteger(exec)); return; }
      case InputPlaceholder:     { input.setPlaceholder(str); return; }
      }
    }
    break;
    case ID_TEXTAREA: {
      DOM::HTMLTextAreaElementImpl& textarea = static_cast<DOM::HTMLTextAreaElementImpl&>(element);
      switch (token) {
      case TextAreaDefaultValue:    { textarea.setDefaultValue(str); return; }
      case TextAreaName:            { textarea.setName(str); return; }
      case TextAreaValue:           { textarea.setValue(str); return; }
      case TextAreaSelectionStart:  { textarea.setSelectionStart(value->toInteger(exec)); return; }
      case TextAreaSelectionEnd:    { textarea.setSelectionEnd  (value->toInteger(exec)); return; }
      case TextAreaPlaceholder:     { textarea.setPlaceholder(str); return; }
      }
    }
    break;
    case ID_A: {
        DOM::HTMLAnchorElementImpl& anchor = static_cast<DOM::HTMLAnchorElementImpl&>(element);
        switch (token) {
        case AnchorSearch:         { QString href = getURLArg(ATTR_HREF);
                                     QUrl u(href);
                                     QString q = str.isEmpty() ? QString() : str.string();
                                     u.setQuery(q);
                                     anchor.setAttribute(ATTR_HREF, u.url());
                                     return; }
        }
    }
    break;
    case ID_IMG: {
        DOM::HTMLImageElementImpl& image = static_cast<DOM::HTMLImageElementImpl&>(element);
        switch (token) {
        case ImageHeight:          { image.setHeight(value->toInteger(exec)); return; }
        case ImageWidth:           { image.setWidth (value->toInteger(exec)); return; }
        }
    }
    break;
    case ID_CANVAS: {
        DOM::HTMLCanvasElementImpl& canvas = static_cast<DOM::HTMLCanvasElementImpl&>(element);
        switch (token) {
        // ### it may make sense to do something different here, to at least
        // emulate reflecting properties somewhat.
        case CanvasWidth:
            canvas.setAttribute(ATTR_WIDTH, value->toString(exec).domString());
            return;
        case CanvasHeight:
            canvas.setAttribute(ATTR_HEIGHT, value->toString(exec).domString());
            return;
        }
    }
    break;


//    case ID_FIELDSET: {
//      DOM::HTMLFieldSetElementImpl& fieldSet = static_cast<DOM::HTMLFieldSetElementImpl&>(element);
//      // read-only: form
//    }
//    break;
    case ID_SCRIPT: {
      DOM::HTMLScriptElementImpl& script = static_cast<DOM::HTMLScriptElementImpl&>(element);
      switch (token) {
      case ScriptText:            { script.setText(str); return; }
      }
    }
    break;
    case ID_TABLE: {
      DOM::HTMLTableElementImpl& table = static_cast<DOM::HTMLTableElementImpl&>(element);
      switch (token) {
      case TableCaption:         { table.setCaption(toHTMLTableCaptionElement(value)); return; } // type HTMLTableCaptionElement
      case TableTHead:           { table.setTHead(toHTMLTableSectionElement(value)); return; } // type HTMLTableSectionElement
      case TableTFoot:           { table.setTFoot(toHTMLTableSectionElement(value)); return; } // type HTMLTableSectionElement
      }
    }
    break;
    case ID_FRAME: {
      DOM::HTMLFrameElementImpl& frameElement = static_cast<DOM::HTMLFrameElementImpl&>(element);
      switch (token) {
       // read-only: FrameContentDocument:
      case FrameLocation:        {
                                   frameElement.setLocation(str);
                                   return;
                                 }
      }
    }
    break;
  }

  // generic properties
  switch (token) {
  case ElementInnerHTML:
    element.setInnerHTML(str, exception);
    return;
  case ElementInnerText:
    element.setInnerText(str, exception);
    return;
  case ElementContentEditable:
    element.setContentEditable(str);
    return;
  case ElementTabIndex:
    element.setTabIndex(value->toInteger(exec));
    return;
  default:
    // qDebug() << "WARNING: KJS::HTMLElement::putValueProperty unhandled token " << token << " thisTag=" << element.tagName().string() << " str=" << str.string();
    break;
  }
}


//Prototype mess for this...
KJS_DEFINE_PROTOTYPE(HTMLElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLElement", HTMLElementProto, HTMLElementFunction, DOMElementProto )
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLElementPseudoCtor, "HTMLElement", HTMLElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLHtmlElement", HTMLHtmlElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLHtmlElementPseudoCtor, "HTMLHtmlElement", HTMLHtmlElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLHeadElement", HTMLHeadElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLHeadElementPseudoCtor, "HTMLHeadElement", HTMLHeadElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLLinkElement", HTMLLinkElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLLinkElementPseudoCtor, "HTMLLinkElement", HTMLLinkElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLTitleElement", HTMLTitleElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLTitleElementPseudoCtor, "HTMLTitleElement", HTMLTitleElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLMetaElement", HTMLMetaElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLMetaElementPseudoCtor, "HTMLMetaElement", HTMLMetaElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLBaseElement", HTMLBaseElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLBaseElementPseudoCtor, "HTMLBaseElement", HTMLBaseElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLIsIndexElement", HTMLIsIndexElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLIsIndexElementPseudoCtor, "HTMLIsIndexElement", HTMLIsIndexElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLStyleElement", HTMLStyleElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLStyleElementPseudoCtor, "HTMLStyleElement", HTMLStyleElementProto)

KJS_DEFINE_PROTOTYPE(HTMLBodyElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLBodyElement", HTMLBodyElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLBodyElementPseudoCtor, "HTMLBodyElement", HTMLBodyElementProto)

KJS_DEFINE_PROTOTYPE(HTMLFormElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLFormElement", HTMLFormElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLFormElementPseudoCtor, "HTMLFormElement", HTMLFormElementProto)

KJS_DEFINE_PROTOTYPE(HTMLSelectElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLSelectElement", HTMLSelectElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLSelectElementPseudoCtor, "HTMLSelectElement", HTMLSelectElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLOptGroupElement", HTMLOptGroupElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLOptGroupElementPseudoCtor, "HTMLOptGroupElement", HTMLOptGroupElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLOptionElement", HTMLOptionElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLOptionElementPseudoCtor, "HTMLOptionElement", HTMLOptionElementProto)

KJS_DEFINE_PROTOTYPE(HTMLInputElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLInputElement", HTMLInputElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLInputElementPseudoCtor, "HTMLInputElement", HTMLInputElementProto)

KJS_DEFINE_PROTOTYPE(HTMLTextAreaElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLTextAreaElement", HTMLTextAreaElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLTextAreaElementPseudoCtor, "HTMLTextAreaElement", HTMLTextAreaElementProto)

KJS_DEFINE_PROTOTYPE(HTMLButtonElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLButtonElement", HTMLButtonElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLButtonElementPseudoCtor, "HTMLButtonElement", HTMLButtonElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLLabelElement", HTMLLabelElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLLabelElementPseudoCtor, "HTMLLabelElement", HTMLLabelElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLFieldSetElement", HTMLFieldSetElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLFieldSetElementPseudoCtor, "HTMLFieldSetElement", HTMLFieldSetElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLLegendElement", HTMLLegendElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLLegendElementPseudoCtor, "HTMLLegendElement", HTMLLegendElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLUListElement", HTMLUListElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLUListElementPseudoCtor, "HTMLUListElement", HTMLUListElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLOListElement", HTMLOListElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLOListElementPseudoCtor, "HTMLOListElement", HTMLOListElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLDListElement", HTMLDListElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLDListElementPseudoCtor, "HTMLDListElement", HTMLDListElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLDirectoryElement", HTMLDirectoryElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLDirectoryElementPseudoCtor, "HTMLDirectoryElement", HTMLDirectoryElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLMenuElement", HTMLMenuElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLMenuElementPseudoCtor, "HTMLMenuElement", HTMLMenuElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLLIElement", HTMLLIElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLLIElementPseudoCtor, "HTMLLIElement", HTMLLIElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLDivElement", HTMLDivElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLDivElementPseudoCtor, "HTMLDivElement", HTMLDivElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLParagraphElement", HTMLParagraphElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLParagraphElementPseudoCtor, "HTMLParagraphElement", HTMLParagraphElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLHeadingElement", HTMLHeadingElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLHeadingElementPseudoCtor, "HTMLHeadingElement", HTMLHeadingElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLBlockQuoteElement", HTMLBlockQuoteElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLBlockQuoteElementPseudoCtor, "HTMLBlockQuoteElement", HTMLBlockQuoteElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLQuoteElement", HTMLQuoteElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLQuoteElementPseudoCtor, "HTMLQuoteElement", HTMLQuoteElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLPreElement", HTMLPreElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLPreElementPseudoCtor, "HTMLPreElement", HTMLPreElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLBRElement", HTMLBRElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLBRElementPseudoCtor, "HTMLBRElement", HTMLBRElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLBaseFontElement", HTMLBaseFontElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLBaseFontElementPseudoCtor, "HTMLBaseFontElement", HTMLBaseFontElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLFontElement", HTMLFontElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLFontElementPseudoCtor, "HTMLFontElement", HTMLFontElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLHRElement", HTMLHRElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLHRElementPseudoCtor, "HTMLHRElement", HTMLHRElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLModElement", HTMLModElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLModElementPseudoCtor, "HTMLModElement", HTMLModElementProto)

KJS_DEFINE_PROTOTYPE(HTMLAnchorElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLAnchorElement", HTMLAnchorElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLAnchorElementPseudoCtor, "HTMLAnchorElement", HTMLAnchorElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLImageElement", HTMLImageElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLImageElementPseudoCtor, "HTMLImageElement", HTMLImageElementProto)

KJS_DEFINE_PROTOTYPE(HTMLObjectElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLObjectElement", HTMLObjectElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLObjectElementPseudoCtor, "HTMLObjectElement", HTMLObjectElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLParamElement", HTMLParamElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLParamElementPseudoCtor, "HTMLParamElement", HTMLParamElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLAppletElement", HTMLAppletElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLAppletElementPseudoCtor, "HTMLAppletElement", HTMLAppletElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLMapElement", HTMLMapElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLMapElementPseudoCtor, "HTMLMapElement", HTMLMapElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLAreaElement", HTMLAreaElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLAreaElementPseudoCtor, "HTMLAreaElement", HTMLAreaElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLScriptElement", HTMLScriptElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLScriptElementPseudoCtor, "HTMLScriptElement", HTMLScriptElementProto)

KJS_DEFINE_PROTOTYPE(HTMLTableElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLTableElement", HTMLTableElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLTableElementPseudoCtor, "HTMLTableElement", HTMLTableElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLTableCaptionElement", HTMLTableCaptionElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLTableCaptionElementPseudoCtor, "HTMLTableCaptionElement", HTMLTableCaptionElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLTableColElement", HTMLTableColElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLTableColElementPseudoCtor, "HTMLTableColElement", HTMLTableColElementProto)

KJS_DEFINE_PROTOTYPE(HTMLTableSectionElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLTableSectionElement", HTMLTableSectionElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLTableSectionElementPseudoCtor, "HTMLTableSectionElement", HTMLTableSectionElementProto)

KJS_DEFINE_PROTOTYPE(HTMLTableRowElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLTableRowElement", HTMLTableRowElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLTableRowElementPseudoCtor, "HTMLTableRowElement", HTMLTableRowElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLTableCellElement", HTMLTableCellElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLTableCellElementPseudoCtor, "HTMLTableCellElement", HTMLTableCellElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLFrameSetElement", HTMLFrameSetElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLFrameSetElementPseudoCtor, "HTMLFrameSetElement", HTMLFrameSetElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLLayerElement", HTMLLayerElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLLayerElementPseudoCtor, "HTMLLayerElement", HTMLLayerElementProto)

KJS_EMPTY_PROTOTYPE_WITH_PROTOTYPE("HTMLFrameElement", HTMLFrameElementProto, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLFrameElementPseudoCtor, "HTMLFrameElement", HTMLFrameElementProto)

KJS_DEFINE_PROTOTYPE(HTMLIFrameElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLIFrameElement", HTMLIFrameElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLIFrameElementPseudoCtor, "HTMLIFrameElement", HTMLIFrameElementProto)

KJS_DEFINE_PROTOTYPE(HTMLMarqueeElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLMarqueeElement", HTMLMarqueeElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLMarqueeElementPseudoCtor, "HTMLMarqueeElement", HTMLMarqueeElementProto)

KJS_DEFINE_PROTOTYPE(HTMLCanvasElementProto)
KJS_IMPLEMENT_PROTOTYPE("HTMLCanvasElement", HTMLCanvasElementProto, HTMLElementFunction, HTMLElementProto)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLCanvasElementPseudoCtor, "HTMLCanvasElement", HTMLCanvasElementProto)


static JSObject* prototypeForID(ExecState* exec, DOM::NodeImpl::Id id) {
  switch (id) {
  case ID_HTML:
    return HTMLHtmlElementProto::self(exec);
  case ID_HEAD:
    return HTMLHeadElementProto::self(exec);
  case ID_LINK:
    return HTMLLinkElementProto::self(exec);
  case ID_TITLE:
    return HTMLTitleElementProto::self(exec);
  case ID_META:
    return HTMLMetaElementProto::self(exec);
  case ID_BASE:
    return HTMLBaseElementProto::self(exec);
  case ID_ISINDEX:
    return HTMLIsIndexElementProto::self(exec);
  case ID_STYLE:
    return HTMLStyleElementProto::self(exec);
  case ID_BODY:
    return HTMLBodyElementProto::self(exec);
  case ID_FORM:
    return HTMLFormElementProto::self(exec);
  case ID_SELECT:
    return HTMLSelectElementProto::self(exec);
  case ID_OPTGROUP:
    return HTMLOptGroupElementProto::self(exec);
  case ID_OPTION:
    return HTMLOptionElementProto::self(exec);
  case ID_INPUT:
    return HTMLInputElementProto::self(exec);
  case ID_TEXTAREA:
    return HTMLTextAreaElementProto::self(exec);
  case ID_BUTTON:
    return HTMLButtonElementProto::self(exec);
  case ID_LABEL:
    return HTMLLabelElementProto::self(exec);
  case ID_FIELDSET:
    return HTMLFieldSetElementProto::self(exec);
  case ID_LEGEND:
    return HTMLLegendElementProto::self(exec);
  case ID_UL:
    return HTMLUListElementProto::self(exec);
  case ID_OL:
    return HTMLOListElementProto::self(exec);
  case ID_DL:
    return HTMLDListElementProto::self(exec);
  case ID_DIR:
    return HTMLDirectoryElementProto::self(exec);
  case ID_MENU:
    return HTMLMenuElementProto::self(exec);
  case ID_LI:
    return HTMLLIElementProto::self(exec);
  case ID_DIV:
    return HTMLDivElementProto::self(exec);
  case ID_P:
    return HTMLParagraphElementProto::self(exec);
  case ID_H1:
  case ID_H2:
  case ID_H3:
  case ID_H4:
  case ID_H5:
  case ID_H6:
    return HTMLHeadingElementProto::self(exec);
  case ID_BLOCKQUOTE:
    return HTMLBlockQuoteElementProto::self(exec);
  case ID_Q:
    return HTMLQuoteElementProto::self(exec);
  case ID_PRE:
    return HTMLPreElementProto::self(exec);
  case ID_BR:
    return HTMLBRElementProto::self(exec);
  case ID_BASEFONT:
    return HTMLBaseFontElementProto::self(exec);
  case ID_FONT:
    return HTMLFontElementProto::self(exec);
  case ID_HR:
    return HTMLHRElementProto::self(exec);
  case ID_INS:
  case ID_DEL:
    return HTMLModElementProto::self(exec);
  case ID_A:
    return HTMLAnchorElementProto::self(exec);
  case ID_IMG:
    return HTMLImageElementProto::self(exec);
  case ID_OBJECT:
    return HTMLObjectElementProto::self(exec);
  case ID_PARAM:
    return HTMLParamElementProto::self(exec);
  case ID_APPLET:
    return HTMLAppletElementProto::self(exec);
  case ID_MAP:
    return HTMLMapElementProto::self(exec);
  case ID_AREA:
    return HTMLAreaElementProto::self(exec);
  case ID_SCRIPT:
    return HTMLScriptElementProto::self(exec);
  case ID_TABLE:
    return HTMLTableElementProto::self(exec);
  case ID_CAPTION:
    return HTMLTableCaptionElementProto::self(exec);
  case ID_COL:
  case ID_COLGROUP:
    return HTMLTableColElementProto::self(exec);
  case ID_THEAD:
  case ID_TBODY:
  case ID_TFOOT:
    return HTMLTableSectionElementProto::self(exec);
  case ID_TR:
    return HTMLTableRowElementProto::self(exec);
  case ID_TD:
  case ID_TH:
    return HTMLTableCellElementProto::self(exec);
  case ID_FRAMESET:
    return HTMLFrameSetElementProto::self(exec);
  case ID_LAYER:
    return HTMLLayerElementProto::self(exec);
  case ID_FRAME:
    return HTMLFrameElementProto::self(exec);
  case ID_IFRAME:
    return HTMLIFrameElementProto::self(exec);
  case ID_MARQUEE:
    return HTMLMarqueeElementProto::self(exec);
  case ID_CANVAS:
    return HTMLCanvasElementProto::self(exec);
  default:
    return HTMLElementProto::self(exec);
  }
}


// -------------------------------------------------------------------------
/* Source for HTMLCollectionProtoTable.
@begin HTMLCollectionProtoTable 3
  item		HTMLCollection::Item		DontDelete|Function 1
  namedItem	HTMLCollection::NamedItem	DontDelete|Function 1
  tags		HTMLCollection::Tags		DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(HTMLCollectionProto)
KJS_IMPLEMENT_PROTOFUNC(HTMLCollectionProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("HTMLCollection", HTMLCollectionProto, HTMLCollectionProtoFunc, ObjectPrototype)
IMPLEMENT_PSEUDO_CONSTRUCTOR(HTMLCollectionPseudoCtor, "HTMLCollection", HTMLCollectionProto)

const ClassInfo KJS::HTMLCollection::info = { "HTMLCollection", 0, 0, 0 };

KJS::HTMLCollection::HTMLCollection(ExecState *exec, DOM::HTMLCollectionImpl* c)
  : DOMObject(HTMLCollectionProto::self(exec)), m_impl(c), hidden(false) {}

KJS::HTMLCollection::HTMLCollection(JSObject* proto, DOM::HTMLCollectionImpl* c)
  : DOMObject(proto), m_impl(c), hidden(false) {}

KJS::HTMLCollection::~HTMLCollection()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

bool KJS::HTMLCollection::masqueradeAsUndefined() const {
    return hidden;
}

bool KJS::HTMLCollection::toBoolean(ExecState *) const {
    return !hidden;
}

JSValue* HTMLCollection::indexGetter(ExecState *exec, unsigned index)
{
  return getDOMNode(exec, m_impl->item(index));
}

void KJS::HTMLCollection::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, PropertyMap::PropertyMode mode)
{
  for (unsigned i = 0; i < m_impl->length(); ++i)
      propertyNames.add(Identifier::from(i));

  propertyNames.add(exec->propertyNames().length);

  JSObject::getOwnPropertyNames(exec, propertyNames, mode);
}

JSValue *HTMLCollection::lengthGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot& slot)
{
  HTMLCollection *thisObj = static_cast<HTMLCollection *>(slot.slotBase());
  return jsNumber(thisObj->m_impl->length());
}


JSValue *HTMLCollection::nameGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
    HTMLCollection *thisObj = static_cast<HTMLCollection *>(slot.slotBase());
    return thisObj->getNamedItems(exec, propertyName);
}


bool KJS::HTMLCollection::getOwnPropertySlot(ExecState *exec, const Identifier &propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLCollection::getOwnPropertySlot " << propertyName.ascii();
#endif
  if (propertyName.isEmpty())
    return false;
  if (propertyName == exec->propertyNames().length)
  {
#ifdef KJS_VERBOSE
    qDebug() << "  collection length is " << m_impl->length();
#endif
    slot.setCustom(this, lengthGetter);
    return true;
  }

  // Look in the prototype (for functions) before assuming it's an item's name
  JSValue *proto = prototype();
  if (proto->isObject() && static_cast<JSObject *>(proto)->hasProperty(exec, propertyName))
    return false;

  // name or index ?
  if (getIndexSlot(this, *m_impl, propertyName, slot))
    return true;

  if (!getNamedItems(exec, propertyName)->isUndefined()) {
    slot.setCustom(this, nameGetter);
    return true;
  }

  return DOMObject::getOwnPropertySlot(exec, propertyName, slot);
}

// HTMLCollections are strange objects, they support both get and call,
// so that document.forms.item(0) and document.forms(0) both work.
JSValue* KJS::HTMLCollection::callAsFunction(ExecState *exec, JSObject *, const List &args)
{
  // Do not use thisObj here. It can be the HTMLDocument, in the document.forms(i) case.
  /*if( thisObj.imp() != this )
  {
    // qDebug() << "WARNING: thisObj.imp() != this in HTMLCollection::tryCall";
    KJS::printInfo(exec,"KJS::HTMLCollection::tryCall thisObj",thisObj,-1);
    KJS::printInfo(exec,"KJS::HTMLCollection::tryCall this",Value(this),-1);
  }*/
  // Also, do we need the TypeError test here ?

  HTMLCollectionImpl &collection = *m_impl;

  // Also, do we need the TypeError test here ?

  if (args.size() == 1) {
    // support for document.all(<index>) etc.
    bool ok;
    UString s = args[0]->toString(exec);
    unsigned int u = s.toArrayIndex(&ok);
    if (ok)
      return getDOMNode(exec, collection.item(u));
    // support for document.images('<name>') etc.
    return getNamedItems(exec, Identifier(s));
  }
  else if (args.size() >= 1) // the second arg, if set, is the index of the item we want
  {
    bool ok;
    UString s = args[0]->toString(exec);
    unsigned int u = args[1]->toString(exec).toArrayIndex(&ok);
    if (ok)
    {
      DOM::DOMString pstr = s.domString();
      DOM::NodeImpl* node = collection.namedItem(pstr);
      while (node) {
        if (!u)
          return getDOMNode(exec,node);
        node = collection.nextNamedItem(pstr);
        --u;
      }
    }
  }
  return jsUndefined();
}

JSValue* KJS::HTMLCollection::getNamedItems(ExecState *exec, const Identifier &propertyName) const
{
#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLCollection::getNamedItems " << propertyName.ascii();
#endif

  DOM::DOMString pstr = propertyName.domString();

  const QList<DOM::NodeImpl*> matches = m_impl->namedItems(pstr);

  if (!matches.isEmpty()) {
    if (matches.size() == 1) {
#ifdef KJS_VERBOSE
      qDebug() << "returning single node";
#endif
      return getDOMNode(exec,matches[0]);
    }
    else  {
      // multiple items, return a collection
      QList<SharedPtr<DOM::NodeImpl> > nodes;
      foreach (DOM::NodeImpl* node, matches)
        nodes.append(node);
#ifdef KJS_VERBOSE
      qDebug() << "returning list of " << matches.count() << " nodes";
#endif
      return new DOMNamedNodesCollection(exec, nodes);
    }
  }
#ifdef KJS_VERBOSE
  qDebug() << "not found";
#endif
  return jsUndefined();
}

JSValue* KJS::HTMLCollectionProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::HTMLCollection, thisObj );
  HTMLCollectionImpl &coll = *static_cast<HTMLCollection *>(thisObj)->impl();

  switch (id) {
  case KJS::HTMLCollection::Item:
  {
    // support for item(<index>) (DOM)
    UString s = args[0]->toString(exec);
    bool ok;
    unsigned int u = s.toArrayIndex(&ok);
    if (ok) {
      return getDOMNode(exec,coll.item(u));
    }

    // support for item('<name>') (IE only)
    qWarning() << "non-standard HTMLCollection.item('" << s.ascii() << "') called, use namedItem instead";
    return static_cast<HTMLCollection *>(thisObj)->getNamedItems(exec, Identifier(s));
  }
  case KJS::HTMLCollection::Tags:
  {
    DOM::DOMString tagName = args[0]->toString(exec).domString();
    DOM::NodeListImpl* list;
    // getElementsByTagName exists in Document and in Element, pick up the right one
    if ( coll.base()->nodeType() == DOM::Node::DOCUMENT_NODE )
    {
      DOM::DocumentImpl* doc = static_cast<DOM::DocumentImpl*>(coll.base());
      list = doc->getElementsByTagName(tagName);
#ifdef KJS_VERBOSE
      qDebug() << "KJS::HTMLCollectionProtoFunc::callAsFunction document.tags(" << tagName.string() << ") -> " << list->length() << " items in node list";
#endif
    } else
    {
      DOM::ElementImpl* e = static_cast<DOM::ElementImpl*>(coll.base());
      list = e->getElementsByTagName(tagName);
#ifdef KJS_VERBOSE
      qDebug() << "KJS::HTMLCollectionProtoFunc::tryCall element.tags(" << tagName.string() << ") -> " << list->length() << " items in node list";
#endif
    }
    return getDOMNodeList(exec, list);
  }
  case KJS::HTMLCollection::NamedItem:
  {
    JSValue *val = static_cast<HTMLCollection *>(thisObj)->getNamedItems(exec, Identifier(args[0]->toString(exec)));
    // Must return null when asking for a named item that isn't in the collection
    // (DOM2 testsuite, HTMLCollection12 test)
    if ( val->type() == KJS::UndefinedType )
      return jsNull();
    else
      return val;
  }
  default:
    return jsUndefined();
  }
}

// -------------------------------------------------------------------------
/* Source for HTMLSelectCollectionProtoTable.
@begin HTMLSelectCollectionProtoTable 1
  add		HTMLSelectCollection::Add		DontDelete|Function 2
  remove    HTMLSelectCollection::Remove        DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(HTMLSelectCollectionProto)
KJS_IMPLEMENT_PROTOFUNC(HTMLSelectCollectionProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("HTMLOptionsCollection", HTMLSelectCollectionProto, HTMLSelectCollectionProtoFunc, HTMLCollectionProto)

const ClassInfo KJS::HTMLSelectCollection::info = { "HTMLOptionsCollection", &HTMLCollection::info, 0, 0 };

KJS::HTMLSelectCollection::HTMLSelectCollection(ExecState *exec, DOM::HTMLCollectionImpl* c,
                                                DOM::HTMLSelectElementImpl* e)
      : HTMLCollection(HTMLSelectCollectionProto::self(exec), c), element(e) { }

JSValue *HTMLSelectCollection::selectedIndexGetter(ExecState*, JSObject*, const Identifier&, const PropertySlot& slot)
{
    HTMLSelectCollection *thisObj = static_cast<HTMLSelectCollection *>(slot.slotBase());
    return jsNumber(thisObj->element->selectedIndex());
}

JSValue *HTMLSelectCollection::selectedValueGetter(ExecState*, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
    Q_UNUSED(propertyName);
    HTMLSelectCollection *thisObj = static_cast<HTMLSelectCollection *>(slot.slotBase());
    return jsString(thisObj->element->value());
}

bool KJS::HTMLSelectCollection::getOwnPropertySlot(ExecState *exec, const Identifier &p, PropertySlot& slot)
{
  if (p == "selectedIndex") {
    slot.setCustom(this, selectedIndexGetter);
    return true;
  } else if (p == "value") {
    slot.setCustom(this, selectedValueGetter);
    return true;
  }

  return  HTMLCollection::getOwnPropertySlot(exec, p, slot);
}

void KJS::HTMLSelectCollection::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int)
{
  DOMExceptionTranslator exception(exec);
#ifdef KJS_VERBOSE
  qDebug() << "KJS::HTMLSelectCollection::put " << propertyName.qstring();
#endif
  if ( propertyName == "selectedIndex" ) {
    element->setSelectedIndex( value->toInteger( exec ) );
    return;
  }
  // resize ?
  else if (propertyName == exec->propertyNames().length) {
    uint32_t newLen;
    bool converted = value->getUInt32(newLen);

    if (!converted) {
      return;
    }

    // CVE-2009-2537 (vendors agreed on max 10000 elements)
    if (newLen > 10000) {
      setDOMException(exec, DOMException::INDEX_SIZE_ERR);
      return;
    }

    long diff = element->length() - newLen;

    if (diff < 0) { // add dummy elements
      do {
        ElementImpl *option = element->document()->createElement("option", exception);
        if (exception.triggered()) return;
        element->add(static_cast<HTMLElementImpl *>(option), 0, exception);
        if (exception.triggered()) return;
      } while (++diff);
    }
    else // remove elements
      while (diff-- > 0)
        element->remove(newLen + diff);

    return;
  }
  // an index ?
  bool ok;
  unsigned int u = propertyName.qstring().toULong(&ok);
  if (!ok)
    return;

  if (value->type() == NullType || value->type() == UndefinedType) {
    // null and undefined delete. others, too ?
    element->remove(u);
    return;
  }

  // is v an option element ?
  DOM::NodeImpl* node = KJS::toNode(value);
  if (!node || node->id() != ID_OPTION)
    return;

  DOM::HTMLOptionElementImpl* option = static_cast<DOM::HTMLOptionElementImpl*>(node);
  if ( option->document() != element->document() )
    option = static_cast<DOM::HTMLOptionElementImpl*>(element->ownerDocument()->importNode(option, true, exception));
  if (exception.triggered()) return;

  long diff = long(u) - element->length();
  DOM::HTMLElementImpl* before = 0;
  // out of array bounds ? first insert empty dummies
  if (diff > 0) {
    while (diff--) {
      element->add(
        static_cast<DOM::HTMLElementImpl*>(element->document()->createElement("OPTION")),
        before, exception);
      if (exception.triggered()) return;
    }
    // replace an existing entry ?
  } else if (diff < 0) {
    SharedPtr<DOM::HTMLCollectionImpl> options = element->options();
    before = static_cast<DOM::HTMLElementImpl*>(options->item(u+1));
    element->remove(u);
  }
  // finally add the new element
  element->add(option, before, exception);
}


JSValue* KJS::HTMLSelectCollectionProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::HTMLSelectCollection, thisObj );
  DOM::HTMLSelectElementImpl* element = static_cast<KJS::HTMLSelectCollection *>(thisObj)->toElement();

  switch (id) {
  case KJS::HTMLSelectCollection::Add:
  {
    //Non-standard select.options.add.
    //The first argument is the item, 2nd is offset.
    //IE and Mozilla are both quite picky here, too...
    DOM::NodeImpl* node = KJS::toNode(args[0]);
    if (!node || node->id() != ID_OPTION)
      return throwError(exec, GeneralError, "Invalid argument to HTMLOptionsCollection::add");

    DOM::HTMLOptionElementImpl* option = static_cast<DOM::HTMLOptionElementImpl*>(node);

    int  pos = 0;
    //By default append, if not specified or null..
    if (args[1]->isUndefined())
      pos = element->length();
    else
      pos = (int)args[1]->toNumber(exec);

    if (pos < 0)
      return throwError(exec, GeneralError, "Invalid index argument to HTMLOptionsCollection::add");

    DOMExceptionTranslator exception(exec);
    if (pos >= element->length()) {
      //Append
      element->add(option, 0, exception);
    } else {
      //Find what to prepend before..
      QVector<HTMLGenericFormElementImpl*> items = element->listItems();
      int dummy;
      element->insertBefore(option, items.at(pos), dummy);
    }
    return jsUndefined();
    break;
  }
  case KJS::HTMLSelectCollection::Remove:
  {
    double index;
    if (!args[0]->getNumber(index))
      index = 0;
    else {
      if (static_cast<long int>(index) >= element->length())
        index = 0;
    }
    element->remove(static_cast<long int>(index));
    return jsUndefined();
    break;
  }
  default:
    break;
  }
  return jsUndefined();
}


////////////////////// Option Object ////////////////////////

OptionConstructorImp::OptionConstructorImp(ExecState *exec, DOM::DocumentImpl* d)
    : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), doc(d)
{
  // ## isn't there some redundancy between JSObject::_proto and the "prototype" property ?
  //put(exec,"prototype", ...,DontEnum|DontDelete|ReadOnly);

  // no. of arguments for constructor
  // ## is 4 correct ? 0 to 4, it seems to be
  put(exec, exec->propertyNames().length, jsNumber(4), ReadOnly|DontDelete|DontEnum);
}

bool OptionConstructorImp::implementsConstruct() const
{
  return true;
}

JSObject *OptionConstructorImp::construct(ExecState *exec, const List &args)
{
  DOMExceptionTranslator exception(exec);
  DOM::ElementImpl* el = doc->createElement("OPTION");
  DOM::HTMLOptionElementImpl* opt = static_cast<DOM::HTMLOptionElementImpl*>(el);
  int sz = args.size();
  SharedPtr<DOM::TextImpl> t = doc->createTextNode("");

  int dummyexception = 0;// #### exec->setException ?
  opt->appendChild(t.get(), dummyexception);

  if (sz > 0)
    t->setData(args[0]->toString(exec).domString(), exception); // set the text
  if (sz > 1)
    opt->setValue(args[1]->toString(exec).domString().implementation());
  if (sz > 2)
    opt->setDefaultSelected(args[2]->toBoolean(exec));
  if (sz > 3)
    opt->setSelected(args[3]->toBoolean(exec));

  return getDOMNode(exec,opt)->getObject();
}

////////////////////// Image Object ////////////////////////

//Like in other browsers, we merely make a new HTMLImageElement
//not in tree for this.
ImageConstructorImp::ImageConstructorImp(ExecState* exec, DOM::DocumentImpl* d)
    : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), doc(d)
{
}

bool ImageConstructorImp::implementsConstruct() const
{
  return true;
}

JSObject *ImageConstructorImp::construct(ExecState *exec, const List &list)
{
  bool widthSet = false, heightSet = false;
  int width = 0, height = 0;
  if (list.size() > 0) {
    widthSet = true;
    JSValue *w = list.at(0);
    width = w->toInt32(exec);
  }
  if (list.size() > 1) {
    heightSet = true;
    JSValue *h = list.at(1);
    height = h->toInt32(exec);
  }

  HTMLImageElementImpl* image = static_cast<HTMLImageElementImpl*>(doc->createElement("img"));

  if (widthSet)
    image->setAttribute(ATTR_WIDTH, QString::number(width));

  if (heightSet)
    image->setAttribute(ATTR_HEIGHT, QString::number(height));

  return getDOMNode(exec,image)->getObject();
}


JSValue* getHTMLCollection(ExecState *exec, DOM::HTMLCollectionImpl* c, bool hide)
{
  assert(!c || c->getType() != HTMLCollectionImpl::SELECT_OPTIONS);
  JSValue *coll = cacheDOMObject<DOM::HTMLCollectionImpl, KJS::HTMLCollection>(exec, c);
  if (hide) {
    KJS::HTMLCollection *impl = static_cast<KJS::HTMLCollection*>(coll);
    impl->hide();
  }
  return coll;
}

JSValue* getSelectHTMLCollection(ExecState *exec, DOM::HTMLCollectionImpl* c, DOM::HTMLSelectElementImpl* e)
{
  assert(!c || c->getType() == HTMLCollectionImpl::SELECT_OPTIONS);
  DOMObject *ret;
  if (!c) return jsNull();
  ScriptInterpreter* interp = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter());
  if ((ret = interp->getDOMObject(c)))
    return ret;
  else {
    ret = new HTMLSelectCollection(exec, c, e);
    interp->putDOMObject(c,ret);
    return ret;
  }
}

} //namespace KJS
