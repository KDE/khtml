// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003 Apple Computer, Inc.
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

#ifndef _KJS_HTML_H_
#define _KJS_HTML_H_

#include "html/html_documentimpl.h"
#include "html/html_baseimpl.h"
#include "html/html_miscimpl.h"
#include "html/html_formimpl.h"
#include "misc/loader_client.h"

#include "ecma/kjs_binding.h"
#include "ecma/kjs_dom.h"
#include "ecma/kjs_scriptable.h"
#include "xml/dom_nodeimpl.h"  // for NodeImpl::Id

namespace KJS {
  class HTMLElement;
  class Window;

  class HTMLDocument : public DOMDocument {
  public:
    HTMLDocument(ExecState *exec, DOM::HTMLDocumentImpl* d);
    JSValue* getValueProperty(ExecState *exec, int token);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue* value, int /*attr*/);

    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Referrer, Domain, URL, Body, Location, Cookie,
           Images, Applets, Links, Forms, Layers, Anchors, Scripts, All, Clear, Open, Close,
           Write, WriteLn, GetElementsByName, GetSelection, CaptureEvents, ReleaseEvents,
           BgColor, FgColor, AlinkColor, LinkColor, VlinkColor, LastModified,
           Height, Width, Dir, Frames, CompatMode, DesignMode, ActiveElement };
    DOM::HTMLDocumentImpl* impl() const { return static_cast<DOM::HTMLDocumentImpl*>( m_impl.get() ); }
  private:
    static JSValue *nameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot);
    static JSValue *frameNameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot);
    static JSValue *objectNameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot);
    static JSValue *layerNameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot);
  };

  DEFINE_PSEUDO_CONSTRUCTOR(HTMLDocumentPseudoCtor)

  class HTMLElement : public DOMElement {
  public:
    HTMLElement(ExecState *exec, DOM::HTMLElementImpl* e);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    JSValue* getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);
    void putValueProperty(ExecState *exec, int token, JSValue* value, int);
    virtual UString toString(ExecState *exec) const;
    virtual void pushEventHandlerScope(ExecState *exec, ScopeChain &scope) const;
    virtual const ClassInfo* classInfo() const;
    static const ClassInfo info;

    static const ClassInfo html_info, head_info, link_info, title_info,
      meta_info, base_info, isIndex_info, style_info, body_info, form_info,
      select_info, optGroup_info, option_info, input_info, textArea_info,
      button_info, label_info, fieldSet_info, legend_info, ul_info, ol_info,
      dl_info, dir_info, menu_info, li_info, div_info, p_info, heading_info,
      blockQuote_info, q_info, pre_info, br_info, baseFont_info, font_info,
      hr_info, mod_info, a_info, canvas_info, img_info, object_info, param_info,
      applet_info, map_info, area_info, script_info, table_info,
      caption_info, col_info, tablesection_info, tr_info,
      tablecell_info, frameSet_info, frame_info, iFrame_info, marquee_info, layer_info;

    enum { HtmlVersion, HeadProfile, LinkHref, LinkRel, LinkMedia,
           LinkCharset, LinkDisabled, LinkHrefLang, LinkRev, LinkTarget, LinkType,
           LinkSheet, TitleText, MetaName, MetaHttpEquiv, MetaContent, MetaScheme,
           BaseHref, BaseTarget, IsIndexForm, IsIndexPrompt, StyleDisabled,
           StyleSheet, StyleType, StyleMedia, BodyBackground, BodyVLink, BodyText,
           BodyLink, BodyALink, BodyBgColor, BodyOnLoad, BodyOnBlur, BodyOnFocus,
           BodyOnError, BodyOnMessage, BodyFocus, BodyOnHashChange,
           FormAction, FormEncType, FormElements, FormLength, FormAcceptCharset,
           FormReset, FormTarget, FormName, FormMethod, FormSubmit, SelectAdd,
           SelectValue, SelectSelectedIndex, SelectLength,
           SelectRemove, SelectForm, SelectType, SelectOptions,
           SelectDisabled, SelectMultiple, SelectName, SelectSize, SelectItem,  
           OptGroupDisabled, OptGroupLabel, OptionIndex, OptionSelected,
           OptionForm, OptionText, OptionDefaultSelected, OptionDisabled,
           OptionLabel, OptionValue, InputReadOnly, InputAccept,
           InputSize, InputDefaultValue, InputValue, InputType,
           InputMaxLength, InputDefaultChecked, InputDisabled,
           InputChecked, InputIndeterminate, InputForm, InputAccessKey, InputAlign, InputAlt,
           InputName, InputSrc, InputUseMap, InputSelect, InputClick,
           InputSelectionStart, InputSelectionEnd, InputSetSelectionRange, InputPlaceholder,
           TextAreaAccessKey, TextAreaName, TextAreaDefaultValue, TextAreaSelect,
           TextAreaCols, TextAreaDisabled, TextAreaForm, TextAreaType,
           TextAreaReadOnly, TextAreaRows, TextAreaValue,
           TextAreaSelectionStart, TextAreaSelectionEnd, TextAreaSetSelectionRange,
           TextAreaTextLength,  TextAreaPlaceholder,
           ButtonClick, ButtonForm, ButtonName,
           ButtonDisabled, ButtonAccessKey, ButtonType, ButtonValue, LabelHtmlFor,
           LabelForm, LabelAccessKey, FieldSetForm, LegendForm, LegendAccessKey,
           LegendAlign, UListType, UListCompact, OListStart, OListCompact,
           OListType, DListCompact, DirectoryCompact, MenuCompact, LIType,
           LIValue, DivAlign, ParagraphAlign, HeadingAlign, BlockQuoteCite,
           QuoteCite, PreWidth, BRClear, BaseFontColor, BaseFontSize,
           BaseFontFace, FontColor, FontSize, FontFace, HRWidth, HRNoShade,
           HRAlign, HRSize, ModCite, ModDateTime, AnchorShape, AnchorRel,
           AnchorAccessKey, AnchorCoords, AnchorHref, AnchorProtocol, AnchorHost,
           AnchorCharset, AnchorHrefLang, AnchorHostname, AnchorType,
           AnchorPort, AnchorPathName, AnchorHash, AnchorSearch, AnchorName,
           AnchorRev, AnchorTarget, AnchorText, AnchorClick, AnchorToString,
           ImageName, ImageAlign, ImageHspace, ImageVspace, ImageUseMap, ImageAlt,
           ImageLowSrc, ImageWidth, ImageIsMap, ImageBorder, ImageHeight,
           ImageLongDesc, ImageSrc, ImageX, ImageY, ImageComplete, ObjectHspace, ObjectHeight, ObjectAlign,
           ObjectBorder, ObjectCode, ObjectType, ObjectVspace, ObjectArchive,
           ObjectDeclare, ObjectForm, ObjectCodeBase, ObjectCodeType, ObjectData, ObjectGetSVGDocument,
           ObjectName, ObjectStandby, ObjectUseMap, ObjectWidth, ObjectContentDocument,
           ParamName, ParamType, ParamValueType, ParamValue, AppletArchive,
           AppletAlt, AppletCode, AppletWidth, AppletAlign, AppletCodeBase,
           AppletName, AppletHeight, AppletHspace, AppletObject, AppletVspace,
           MapAreas, MapName, AreaHash, AreaHref, AreaTarget, AreaPort, AreaShape,
           AreaCoords, AreaAlt, AreaAccessKey, AreaNoHref, AreaHost, AreaProtocol,
           AreaHostName, AreaPathName, AreaSearch, ScriptEvent,
           ScriptType, ScriptHtmlFor, ScriptText, ScriptSrc, ScriptCharset,
           ScriptDefer, TableSummary, TableTBodies, TableTHead, TableCellPadding,
           TableDeleteCaption, TableCreateCaption, TableCaption, TableWidth,
           TableCreateTFoot, TableAlign, TableTFoot, TableDeleteRow,
           TableCellSpacing, TableRows, TableBgColor, TableBorder, TableFrame,
           TableRules, TableCreateTHead, TableDeleteTHead, TableDeleteTFoot,
           TableInsertRow, TableCaptionAlign, TableColCh, TableColChOff,
           TableColAlign, TableColSpan, TableColVAlign, TableColWidth,
           TableSectionCh, TableSectionDeleteRow, TableSectionChOff,
           TableSectionRows, TableSectionAlign, TableSectionVAlign,
           TableSectionInsertRow, TableRowSectionRowIndex, TableRowRowIndex,
           TableRowChOff, TableRowCells, TableRowVAlign, TableRowCh,
           TableRowAlign, TableRowBgColor, TableRowDeleteCell, TableRowInsertCell,
           TableCellColSpan, TableCellNoWrap, TableCellAbbr, TableCellHeight,
           TableCellWidth, TableCellCellIndex, TableCellChOff, TableCellBgColor,
           TableCellCh, TableCellVAlign, TableCellRowSpan, TableCellHeaders,
           TableCellAlign, TableCellAxis, TableCellScope, FrameSetCols,
           FrameSetRows, FrameSetOnMessage, FrameSrc, FrameLocation, FrameFrameBorder, FrameScrolling,
           FrameMarginWidth, FrameLongDesc, FrameMarginHeight, FrameName,
           FrameContentDocument, FrameContentWindow,
           FrameNoResize, FrameWidth, FrameHeight, IFrameLongDesc, IFrameAlign,
           IFrameFrameBorder, IFrameSrc, IFrameName, IFrameHeight,
           IFrameMarginHeight, IFrameMarginWidth, IFrameScrolling, IFrameWidth,
           IFrameContentDocument, IFrameContentWindow, IFrameGetSVGDocument,
           MarqueeStart, MarqueeStop,
           CanvasGetContext, CanvasWidth, CanvasHeight, CanvasToDataURL,
           LayerTop, LayerLeft, LayerVisibility, LayerBgColor, LayerClip, LayerDocument, LayerLayers,
           ElementInnerHTML, ElementTitle, ElementId, ElementDir, ElementLang,
           ElementClassName, ElementInnerText, ElementDocument,
           ElementChildren, ElementContentEditable, ElementIsContentEditable,
           ElementAll, ElementScrollIntoView, ElementTabIndex };

    DOM::HTMLElementImpl* impl() const { return static_cast<DOM::HTMLElementImpl*>(m_impl.get()); }
    JSValue* indexGetter(ExecState *exec, unsigned index);
  private:
    static JSValue *formNameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot);

    QString getURLArg(unsigned id) const;

    static KParts::ScriptableExtension *getScriptableExtension(const DOM::HTMLElementImpl &element);

    /* Many of properties in the DOM bindings can be implemented by merely returning
      an attribute as the right type, and setting it in similar manner; or perhaps
      returning a collection of appropriate type*/
    enum BoundPropType {
      T_String, //String, to be return by String()
      T_StrOrNl, //String, to be return by getStringOrNull()
      T_Bool,   //Boolean, return true if property is not null
      T_Int,
      T_URL,
      T_Res,      //Reserved, ignore sets, return empty string
      T_Coll,     //Collection, type is in attrID
      T_ReadOnly = 0x80 //Property should be handled only on read.
    };

    enum {
        NotApplicable = 0xFFFFFFFu
    };

    struct BoundPropInfo {
      unsigned elId;  //Applicable element type
      int      token; //Token
      BoundPropType type;
      unsigned attrId; //Attribute to get
    };

    JSValue* handleBoundRead (ExecState* exec, int token) const;
    bool      handleBoundWrite(ExecState* exec, int token, JSValue* value);

    static const BoundPropInfo bpTable[];

    static QHash<int, const BoundPropInfo*>* s_boundPropInfo;
    static QHash<int, const BoundPropInfo*>* boundPropInfo();

    // returns our window object --- may return 0.
    KJS::Window* ourWindow() const;
    JSValue*     getWindowListener(ExecState *exec, int eventId) const;
    void         setWindowListener(ExecState *exec, int eventId, JSValue* val) const;
  };

  class HTMLCollection : public DOMObject {
  public:
    HTMLCollection(ExecState *exec,  DOM::HTMLCollectionImpl* c);
    HTMLCollection(KJS::JSObject *proto, DOM::HTMLCollectionImpl* c);
    ~HTMLCollection();
    JSValue* getValueProperty(ExecState *exec, int token);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);

    virtual JSValue* callAsFunction(ExecState *exec, JSObject* thisObj, const List& args);
    virtual bool implementsCall() const { return true; }
    virtual bool isFunctionType() const { return false; }
    virtual bool masqueradeAsUndefined() const;
    virtual bool toBoolean(ExecState *) const;
    virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, PropertyMap::PropertyMode mode);
    enum { Item, NamedItem, Tags };
    JSValue* getNamedItems(ExecState *exec, const Identifier &propertyName) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    DOM::HTMLCollectionImpl* impl() const { return m_impl.get(); }
    virtual void hide() { hidden = true; }
    JSValue* indexGetter(ExecState *exec, unsigned index);
  protected:
    SharedPtr<DOM::HTMLCollectionImpl> m_impl;
    bool hidden;
  private:
    static JSValue *lengthGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot);
    static JSValue *nameGetter(ExecState *exec, JSObject*, const Identifier& name, const PropertySlot& slot);
  };

  class HTMLSelectCollection : public HTMLCollection {
  public:
    enum { Add, Remove };
    HTMLSelectCollection(ExecState *exec, DOM::HTMLCollectionImpl* c, DOM::HTMLSelectElementImpl* e);
    using KJS::JSObject::getOwnPropertySlot;
    virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
    using KJS::JSObject::put;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue* value, int attr = None);

    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    DOM::HTMLSelectElementImpl* toElement() const { return element.get(); }
  private:
    SharedPtr<DOM::HTMLSelectElementImpl> element;
    static JSValue *selectedIndexGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot);
    static JSValue *selectedValueGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot);
  };

  ////////////////////// Option Object ////////////////////////

  class OptionConstructorImp : public JSObject {
  public:
    OptionConstructorImp(ExecState *exec, DOM::DocumentImpl* d);
    virtual bool implementsConstruct() const;
    using KJS::JSObject::construct;
    virtual JSObject* construct(ExecState *exec, const List &args);
  private:
    SharedPtr<DOM::DocumentImpl> doc;
  };

  ////////////////////// Image Object ////////////////////////

  class ImageConstructorImp : public JSObject {
  public:
    ImageConstructorImp(ExecState *exec, DOM::DocumentImpl* d);
    virtual bool implementsConstruct() const;
    using KJS::JSObject::construct;
    virtual JSObject* construct(ExecState *exec, const List &args);
  private:
    SharedPtr<DOM::DocumentImpl> doc;
  };


  JSValue* getHTMLCollection(ExecState *exec, DOM::HTMLCollectionImpl* c, bool hide=false);
  JSValue* getSelectHTMLCollection(ExecState *exec, DOM::HTMLCollectionImpl* c, DOM::HTMLSelectElementImpl* e);

  //All the pseudo constructors..
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLHtmlElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLHeadElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLLinkElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLTitleElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLMetaElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLBaseElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLIsIndexElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLStyleElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLBodyElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLFormElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLSelectElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLOptGroupElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLOptionElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLInputElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLTextAreaElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLButtonElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLLabelElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLFieldSetElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLLegendElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLUListElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLOListElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLDListElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLDirectoryElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLMenuElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLLIElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLDivElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLParagraphElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLHeadingElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLBlockQuoteElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLQuoteElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLPreElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLBRElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLBaseFontElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLFontElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLHRElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLModElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLAnchorElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLImageElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLObjectElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLParamElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLAppletElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLMapElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLAreaElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLScriptElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLTableElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLTableCaptionElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLTableColElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLTableSectionElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLTableRowElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLTableCellElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLFrameSetElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLLayerElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLFrameElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLIFrameElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLCollectionPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLMarqueeElementPseudoCtor)
  DEFINE_PSEUDO_CONSTRUCTOR(HTMLCanvasElementPseudoCtor)
} // namespace

#endif
