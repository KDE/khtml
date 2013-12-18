/**
 * This file is part of the CSS implementation for KDE.
 *
 * Copyright 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright 2003-2004 Apple Computer, Inc.
 * Copyright 2004-2010 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright 2004-2008 Germain Garand (germain@ebooksfrance.org)
 * Copyright 2008 Vyacheslav Tokarev (tsjoker@gmail.com)
 *           (C) 2005, 2006, 2008 Apple Computer, Inc.
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

#include "css/cssstyleselector.h"
#include "rendering/render_style.h"
#include "css/css_stylesheetimpl.h"
#include "css/css_ruleimpl.h"
#include "css/css_valueimpl.h"
#include "css/csshelper.h"
#include "css/css_webfont.h"
#include "rendering/render_object.h"
#include "html/html_documentimpl.h"
#include "html/html_elementimpl.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_restyler.h"
#include "dom/css_rule.h"
#include "dom/css_value.h"
#include "khtml_global.h"
#include "khtmlpart_p.h"
using namespace khtml;
using namespace DOM;

#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "css/css_mediaquery.h"

#include "misc/khtmllayout.h"
#include "khtml_settings.h"
#include "misc/helper.h"
#include "misc/loader.h"

#include "rendering/font.h"

#include "khtmlview.h"
#include "khtml_part.h"


#include <kconfig.h>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QFileInfo>
#include <QUrl>
#include <QFontDatabase>
#include <qstandardpaths.h>

#include <QDebug>
#include <assert.h>
#include <stdlib.h>

// keep in sync with html4.css'
#define KHTML_STYLE_VERSION 1

#undef PRELATIVE
#undef PABSOLUTE

// handle value "inherit" on a default inherited property
#define HANDLE_INHERIT_ON_INHERITED_PROPERTY(prop, Prop) \
if (isInherit) \
{\
    style->set##Prop(parentStyle->prop());\
    return;\
}

// handle value "inherit" on a default non-inherited property
#define HANDLE_INHERIT_ON_NONINHERITED_PROPERTY(prop, Prop) \
if (isInherit) \
{\
    style->setInheritedNoninherited(true);\
    style->set##Prop(parentStyle->prop());\
    return;\
}

#define HANDLE_INITIAL(prop, Prop) \
if (isInitial) \
{\
    style->set##Prop(RenderStyle::initial##Prop());\
    return;\
}

#define HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(prop, Prop) \
HANDLE_INITIAL(prop, Prop) \
else \
HANDLE_INHERIT_ON_NONINHERITED_PROPERTY(prop, Prop)

#define HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(prop, Prop) \
HANDLE_INITIAL(prop, Prop) \
else \
HANDLE_INHERIT_ON_INHERITED_PROPERTY(prop, Prop)

// all non-inherited properties
#define HANDLE_INHERIT_AND_INITIAL_WITH_VALUE(prop, Prop, Value) \
HANDLE_INHERIT_ON_NONINHERITED_PROPERTY(prop, Prop) \
else if (isInitial) \
{\
    style->set##Prop(RenderStyle::initial##Value());\
    return;\
}

#define HANDLE_BACKGROUND_INHERIT_AND_INITIAL(prop, Prop) \
if (isInherit) { \
    BackgroundLayer* currChild = style->accessBackgroundLayers(); \
    BackgroundLayer* prevChild = 0; \
    const BackgroundLayer* currParent = parentStyle->backgroundLayers(); \
    while (currParent && currParent->is##Prop##Set()) { \
        if (!currChild) { \
            /* Need to make a new layer.*/ \
            currChild = new BackgroundLayer(); \
            prevChild->setNext(currChild); \
        } \
        currChild->set##Prop(currParent->prop()); \
        prevChild = currChild; \
        currChild = prevChild->next(); \
        currParent = currParent->next(); \
    } \
    \
    while (currChild) { \
        /* Reset any remaining layers to not have the property set. */ \
        currChild->clear##Prop(); \
        currChild = currChild->next(); \
    } \
} else if (isInitial) { \
    BackgroundLayer* currChild = style->accessBackgroundLayers(); \
    currChild->set##Prop(RenderStyle::initial##Prop()); \
    for (currChild = currChild->next(); currChild; currChild = currChild->next()) \
        currChild->clear##Prop(); \
}

#define HANDLE_BACKGROUND_VALUE(prop, Prop, value) { \
HANDLE_BACKGROUND_INHERIT_AND_INITIAL(prop, Prop) \
else { \
    if (!value->isPrimitiveValue() && !value->isValueList()) \
        return; \
    BackgroundLayer* currChild = style->accessBackgroundLayers(); \
    BackgroundLayer* prevChild = 0; \
    if (value->isPrimitiveValue()) { \
        map##Prop(currChild, value); \
        currChild = currChild->next(); \
    } \
    else { \
        /* Walk each value and put it into a layer, creating new layers as needed. */ \
        CSSValueListImpl* valueList = static_cast<CSSValueListImpl*>(value); \
        for (unsigned int i = 0; i < valueList->length(); i++) { \
            if (!currChild) { \
                /* Need to make a new layer to hold this value */ \
                currChild = new BackgroundLayer(); \
                prevChild->setNext(currChild); \
            } \
            map##Prop(currChild, valueList->item(i)); \
            prevChild = currChild; \
            currChild = currChild->next(); \
        } \
    } \
    while (currChild) { \
        /* Reset all remaining layers to not have the property set. */ \
        currChild->clear##Prop(); \
        currChild = currChild->next(); \
    } \
} }

#define HANDLE_INHERIT_COND(propID, prop, Prop) \
if (id == propID) \
{\
    style->set##Prop(parentStyle->prop());\
    return;\
}

#define HANDLE_INHERIT_COND_WITH_BACKUP(propID, prop, propAlt, Prop) \
if (id == propID) { \
    if (parentStyle->prop().isValid()) \
        style->set##Prop(parentStyle->prop()); \
    else \
        style->set##Prop(parentStyle->propAlt()); \
    return; \
}

#define HANDLE_INITIAL_COND(propID, Prop) \
if (id == propID) \
{\
    style->set##Prop(RenderStyle::initial##Prop());\
    return;\
}

#define HANDLE_INITIAL_COND_WITH_VALUE(propID, Prop, Value) \
if (id == propID) \
{\
    style->set##Prop(RenderStyle::initial##Value());\
    return;\
}

namespace khtml {

CSSStyleSelectorList *CSSStyleSelector::s_defaultStyle;
CSSStyleSelectorList *CSSStyleSelector::s_defaultQuirksStyle;
CSSStyleSelectorList *CSSStyleSelector::s_defaultNonCSSHintsStyle;
CSSStyleSelectorList *CSSStyleSelector::s_defaultPrintStyle;
CSSStyleSheetImpl *CSSStyleSelector::s_defaultSheet;
CSSStyleSheetImpl *CSSStyleSelector::s_defaultNonCSSHintsSheet;
RenderStyle* CSSStyleSelector::styleNotYetAvailable;
CSSStyleSheetImpl *CSSStyleSelector::s_quirksSheet;

enum PseudoState { PseudoUnknown, PseudoNone, PseudoLink, PseudoVisited};
static PseudoState pseudoState;


CSSStyleSelector::CSSStyleSelector( DocumentImpl* doc, QString userStyleSheet, StyleSheetListImpl *styleSheets,
                                    const QUrl &url, bool _strictParsing )
{
    KHTMLView* view = doc->view();
    KHTMLPart* part = doc->part();

    m_fontSelector = new CSSFontSelector( doc );

    init(part ? part->settings() : 0, doc);

    strictParsing = _strictParsing;

    selectors = 0;
    selectorCache = 0;
    propertiesBuffer = 0;
    nextPropertyIndexes = 0;
    userStyle = 0;
    userSheet = 0;
    logicalDpiY = doc->logicalDpiY();

    if(logicalDpiY) // this may be null, not everyone uses khtmlview (Niko)
        computeFontSizes(logicalDpiY, part ? part->fontScaleFactor() : 100);

    // build a limited default style suitable to evaluation of media queries
    // containing relative constraints, like "screen and (max-width: 10em)"
    setupDefaultRootStyle(doc);

    if (view) 
        m_medium = new MediaQueryEvaluator(view->mediaType(), view->part(), m_rootDefaultStyle);
    else
        m_medium = new MediaQueryEvaluator("all", 0, m_rootDefaultStyle);

    if ( !userStyleSheet.isEmpty() ) {
        userSheet = new DOM::CSSStyleSheetImpl(doc);
        userSheet->parseString( DOMString( userStyleSheet ) );

        userStyle = new CSSStyleSelectorList();
        userStyle->append( userSheet, m_medium, this );
    }

    // add stylesheets from document
    authorStyle = 0;
    implicitStyle = 0;

    foreach (StyleSheetImpl* sh, styleSheets->styleSheets) {
        if ( sh->isCSSStyleSheet() ) {
            if ( static_cast<CSSStyleSheetImpl*>(sh)->implicit() ) {
                if (!implicitStyle)
                    implicitStyle = new CSSStyleSelectorList();
                implicitStyle->append( static_cast<CSSStyleSheetImpl*>( sh ), m_medium, this );
            } else if ( sh->isCSSStyleSheet() && !sh->disabled()) {
                if (!authorStyle)
                    authorStyle = new CSSStyleSelectorList();
                authorStyle->append( static_cast<CSSStyleSheetImpl*>( sh ), m_medium, this );
            }
        }
    }

    buildLists();

    //qDebug() << "number of style sheets in document " << authorStyleSheets.count();
    //qDebug() << "CSSStyleSelector: author style has " << authorStyle->count() << " elements";

    QUrl u = url;

    u.setQuery( QString() );
    u.setFragment( QString() );
    encodedurl.file = u.url();
    int pos = encodedurl.file.lastIndexOf('/');
    encodedurl.path = encodedurl.file;
    if ( pos > 0 ) {
	encodedurl.path.truncate( pos );
	encodedurl.path += '/';
    }
    u.setPath( QString() );
    encodedurl.host = u.url();

    //qDebug() << "CSSStyleSelector::CSSStyleSelector encoded url " << encodedurl.path;
}

CSSStyleSelector::CSSStyleSelector( CSSStyleSheetImpl *sheet )
{
    m_fontSelector = new CSSFontSelector( sheet->doc() );

    init(0L, 0L);

    KHTMLView *view = sheet->doc()->view();

    setupDefaultRootStyle(sheet->doc());

    if (view)
        m_medium = new MediaQueryEvaluator(view->mediaType(), view->part(), m_rootDefaultStyle);
    else
        m_medium = new MediaQueryEvaluator("screen", 0, m_rootDefaultStyle);

    if (sheet->implicit()) {
        implicitStyle = new CSSStyleSelectorList();
        implicitStyle->append( sheet, m_medium, this );
    } else {
        authorStyle = new CSSStyleSelectorList();
        authorStyle->append( sheet, m_medium, this );
    }
}

void CSSStyleSelector::init(const KHTMLSettings* _settings, DocumentImpl* doc)
{
    element = 0;
    settings = _settings;
    logicalDpiY = 0;
    if(!s_defaultStyle) loadDefaultStyle(settings, doc);

    defaultStyle = s_defaultStyle;
    defaultPrintStyle = s_defaultPrintStyle;
    defaultQuirksStyle = s_defaultQuirksStyle;
    defaultNonCSSHintsStyle = s_defaultNonCSSHintsStyle;
    m_rootDefaultStyle = 0;
    m_medium = 0;
}

CSSStyleSelector::~CSSStyleSelector()
{
    clearLists();
    delete authorStyle;
    delete implicitStyle;
    delete userStyle;
    delete userSheet;
    delete m_rootDefaultStyle;
    delete m_medium;
    delete m_fontSelector;
}

void CSSStyleSelector::addSheet( CSSStyleSheetImpl *sheet )
{
    KHTMLView *view = sheet->doc()->view();

    setupDefaultRootStyle(sheet->doc());

    delete m_medium; m_medium = 0;
    delete authorStyle; authorStyle = 0;
    delete implicitStyle; implicitStyle = 0;

    if (view)
        m_medium = new MediaQueryEvaluator(view->mediaType(), view->part(), m_rootDefaultStyle);
    else
        m_medium = new MediaQueryEvaluator("screen", 0, m_rootDefaultStyle);

    if (sheet->implicit()) {
        if (!implicitStyle)
            implicitStyle = new CSSStyleSelectorList();
        implicitStyle->append( sheet, m_medium, this );
    } else {
        if (!authorStyle)
            authorStyle = new CSSStyleSelectorList();
        authorStyle->append( sheet, m_medium, this );
    }
}

void CSSStyleSelector::loadDefaultStyle(const KHTMLSettings *s, DocumentImpl *doc)
{
    if(s_defaultStyle) return;

    MediaQueryEvaluator screenEval("screen");
    MediaQueryEvaluator printEval("print");

    {
	QFile f(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "khtml/css/html4.css" ) );
	f.open(QIODevice::ReadOnly);

	QByteArray file( f.size()+1, 0 );
	int readbytes = f.read( file.data(), f.size() );
	f.close();
	if ( readbytes >= 0 )
	    file[readbytes] = '\0';

	QString style = QLatin1String( file.data() );
	
	QRegExp checkVersion( "KHTML_STYLE_VERSION:\\s*(\\d+)" );
	checkVersion.setMinimal( true );
	if (checkVersion.indexIn( style ) == -1 || checkVersion.cap(1).toInt() != KHTML_STYLE_VERSION) {
	    qFatal( "!!!!!!! ERROR !!!!!!! - KHTML default stylesheet version mismatch. Aborting. Check your installation. File used was: %s. Expected STYLE_VERSION %d\n", 
	        QFileInfo( f ).absoluteFilePath().toLatin1().data(), KHTML_STYLE_VERSION );
        }
	
	if(s)
	    style += s->settingsToCSS();
	DOMString str(style);

	s_defaultSheet = new DOM::CSSStyleSheetImpl(doc);
	s_defaultSheet->parseString( str );
	
	// Collect only strict-mode rules.
	s_defaultStyle = new CSSStyleSelectorList();
	s_defaultStyle->append( s_defaultSheet, &screenEval, doc->styleSelector() );  

	s_defaultPrintStyle = new CSSStyleSelectorList();
	s_defaultPrintStyle->append( s_defaultSheet, &printEval, doc->styleSelector() );
    }
    {
	QFile f(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "khtml/css/quirks.css" ) );
	f.open(QIODevice::ReadOnly);

	QByteArray file( f.size()+1, 0 );
	int readbytes = f.read( file.data(), f.size() );
	f.close();
	if ( readbytes >= 0 )
	    file[readbytes] = '\0';

	QString style = QLatin1String( file.data() );
	DOMString str(style);

	s_quirksSheet = new DOM::CSSStyleSheetImpl(doc);
	s_quirksSheet->parseString( str );

	// Collect only quirks-mode rules.
	s_defaultQuirksStyle = new CSSStyleSelectorList();
	s_defaultQuirksStyle->append( s_quirksSheet, &screenEval, doc->styleSelector() );
    }
    {
	QFile f(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "khtml/css/presentational.css" ) );
	f.open(QIODevice::ReadOnly);

	QByteArray file( f.size()+1, 0 );
	int readbytes = f.read( file.data(), f.size() );
	f.close();
	if ( readbytes >= 0 )
	    file[readbytes] = '\0';

	QString style = QLatin1String( file.data() );
	DOMString str(style);

	s_defaultNonCSSHintsSheet = new DOM::CSSStyleSheetImpl(doc);
	s_defaultNonCSSHintsSheet->parseString( str );

        s_defaultNonCSSHintsStyle = new CSSStyleSelectorList();
	s_defaultNonCSSHintsStyle->append( s_defaultNonCSSHintsSheet, &screenEval, doc->styleSelector() );
    }
    //qDebug() << "CSSStyleSelector: default style has " << defaultStyle->count() << " elements";
}

void CSSStyleSelector::clear()
{
    delete s_defaultStyle;
    delete s_defaultQuirksStyle;
    delete s_defaultPrintStyle;
    delete s_defaultNonCSSHintsStyle;
    delete s_defaultSheet;
    delete s_defaultNonCSSHintsSheet;
    delete styleNotYetAvailable;
    s_defaultStyle = 0;
    s_defaultQuirksStyle = 0;
    s_defaultPrintStyle = 0;
    s_defaultNonCSSHintsStyle = 0;
    s_defaultSheet = 0;
    s_defaultNonCSSHintsSheet = 0;
    styleNotYetAvailable = 0;
}

void CSSStyleSelector::reparseConfiguration()
{
    // nice leak, but best we can do right now. hopefully it is only rare.
    s_defaultStyle = 0;
    s_defaultQuirksStyle = 0;
    s_defaultPrintStyle = 0;
    s_defaultNonCSSHintsStyle = 0;
    s_defaultSheet = 0;
}

#define MAXFONTSIZES 8

void CSSStyleSelector::computeFontSizes(int logicalDpiY,  int zoomFactor)
{
    computeFontSizesFor(logicalDpiY, zoomFactor, m_fontSizes, false);
    computeFontSizesFor(logicalDpiY, zoomFactor, m_fixedFontSizes, true);
}

void CSSStyleSelector::computeFontSizesFor(int logicalDpiY, int zoomFactor, QVector<int>& fontSizes, bool isFixed)
{
#ifdef APPLE_CHANGES
    // We don't want to scale the settings by the dpi.
    const float toPix = 1.0;
#else
    Q_UNUSED( isFixed );

    // ### get rid of float / double
    float toPix = logicalDpiY/72.0f;
    if (toPix  < 96.0f/72.0f)
         toPix = 96.0f/72.0f;
#endif // ######### fix isFixed code again.

    fontSizes.resize( MAXFONTSIZES );
    float scale = 1.0;
    static const float fontFactors[] =      {3.0f/5.0f, 3.0f/4.0f, 8.0f/9.0f, 1.0f, 6.0f/5.0f, 3.0f/2.0f, 2.0f, 3.0f};
    static const float smallFontFactors[] = {3.0f/4.0f, 5.0f/6.0f, 8.0f/9.0f, 1.0f, 6.0f/5.0f, 3.0f/2.0f, 2.0f, 3.0f};
    float mediumFontSize, minFontSize, factor;
    if (!khtml::printpainter) {
        scale *= zoomFactor / 100.0;
#ifdef APPLE_CHANGES
	if (isFixed)
	    mediumFontSize = settings->mediumFixedFontSize() * toPix;
	else
#endif
	    mediumFontSize = settings->mediumFontSize() * toPix;
        minFontSize = settings->minFontSize() * toPix;
    }
    else {
        // ### depending on something / configurable ?
        mediumFontSize = 12;
        minFontSize = 6;
    }
    const float* factors = scale*mediumFontSize >= 12.5 ? fontFactors : smallFontFactors;
    for ( int i = 0; i < MAXFONTSIZES; i++ ) {
        factor = scale*factors[i];
        fontSizes[i] = int(qMax( mediumFontSize*factor +.5f, minFontSize));
        //qDebug() << "index: " << i << " factor: " << factors[i] << " font pix size: " << int(qMax( mediumFontSize*factor +.5f, minFontSize));
    }
}

#undef MAXFONTSIZES

RenderStyle* CSSStyleSelector::locateSimilarStyle()
{
    ElementImpl *s=0, *t=0, *c=0;
    if (!element) return 0;
    // Check previous siblings.
    unsigned count = 0;
    NodeImpl* n = element;
    do {
        for (n = n->previousSibling(); n && !n->isElementNode(); n = n->previousSibling());
        if (!n) break;
        ElementImpl *e = static_cast<ElementImpl*>(n);
        if (++count > 10) break;
        if (!s) s = e; // sibling match
        if (e->id() != element->id()) continue;
        if (!t) t = e; // tag match
        if (element->hasClass()) {
            if (!e->hasClass()) continue;
            const DOMString& class1 = element->getAttribute(ATTR_CLASS);
            const DOMString& class2 = e->getAttribute(ATTR_CLASS);
            if  (class1 != class2) continue;
        }
        if (!c) c = e; // class match
        break;
    } while(true);

    // if possible return sibling that matches tag and class
    if (c && c->renderer() && c->renderer()->style()) return c->renderer()->style();
    // second best: return sibling that matches tag
    if (t && t->renderer() && t->renderer()->style()) return t->renderer()->style();
    // third best: return sibling element
    if (s && s->renderer() && s->renderer()->style()) return s->renderer()->style();
    // last attempt: return parent element
    NodeImpl* p = element->parentNode();
    if (p && p->renderer()) return p->renderer()->style();

    return 0;
}

static inline void bubbleSort( CSSOrderedProperty **b, CSSOrderedProperty **e )
{
    while( b < e ) {
	bool swapped = false;
        CSSOrderedProperty **y = e+1;
	CSSOrderedProperty **x = e;
        CSSOrderedProperty **swappedPos = 0;
	do {
	    if ( !((**(--x)) < (**(--y))) ) {
		swapped = true;
                swappedPos = x;
                CSSOrderedProperty *tmp = *y;
                *y = *x;
                *x = tmp;
	    }
	} while( x != b );
	if ( !swapped ) break;
        b = swappedPos + 1;
    }
}

RenderStyle *CSSStyleSelector::styleForElement(ElementImpl *e, RenderStyle* fallbackParentStyle)
{
    if (!e->document()->haveStylesheetsLoaded() || !e->document()->view()) {
        if (!styleNotYetAvailable) {
            styleNotYetAvailable = new RenderStyle();
            styleNotYetAvailable->setDisplay(NONE);
            styleNotYetAvailable->ref();
        }
        return styleNotYetAvailable;
    }

    // set some variables we will need
    pseudoState = PseudoUnknown;

    element = e;
    parentNode = e->parentNode();
    parentStyle = ( parentNode && parentNode->renderer()) ?
                            parentNode->renderer()->style() : fallbackParentStyle;
    view = element->document()->view();
    part = view->part();
    settings = part->settings();
    logicalDpiY = element->document()->logicalDpiY();

    // reset dynamic DOM dependencies
    e->document()->dynamicDomRestyler().resetDependencies(e);

    style = new RenderStyle();
    if( parentStyle )
        style->inheritFrom( parentStyle );
    else
	parentStyle = style;

    // try to sort out most style rules as early as possible.
    quint16 cssTagId = localNamePart(element->id());
    int smatch = 0;
    int schecked = 0;

    // do aggressive selection of selectors to check
    // instead of going over whole constructed list,
    // skip selectors that won't match for sure (e.g. with different id or class)
    QVarLengthArray<int> selectorsForCheck;
    // add unknown selectors to always be checked
    for (unsigned int i = otherSelector; i < selectors_size; i = nextSimilarSelector[i])
        selectorsForCheck.append(i);
    // check if we got class attribute on element: add selectors with it to the list
    if (e->hasClass()) {
        const ClassNames& classNames = element->classNames();
        for (unsigned int i = 0; i < classNames.size(); ++i) {
            WTF::HashMap<quintptr, int>::iterator it = classSelector.find((quintptr)classNames[i].impl());
            if (it != classSelector.end())
                for (unsigned int j = it->second; j < selectors_size; j = nextSimilarSelector[j])
                    selectorsForCheck.append(j);
        }
    }
    // check if we got id attribute on element: add selectors with it to the list
    DOMStringImpl* idValue = element->getAttributeImplById(ATTR_ID);
    if (idValue && idValue->length()) {
        bool caseSensitive = (e->document()->htmlMode() == DocumentImpl::XHtml) || strictParsing;
        AtomicString elementId = caseSensitive ? idValue : idValue->lower();
        WTF::HashMap<quintptr, int>::iterator it = idSelector.find((quintptr)elementId.impl());
        if (it != idSelector.end())
            for (unsigned int j = it->second; j < selectors_size; j = nextSimilarSelector[j])
                selectorsForCheck.append(j);
    }
    // add all selectors with given local tag
    WTF::HashMap<unsigned, int>::iterator it = tagSelector.find(cssTagId);
    if (it != tagSelector.end())
        for (unsigned int j = it->second; j < selectors_size; j = nextSimilarSelector[j])
            selectorsForCheck.append(j);

    // build per-element cache summaries.
    prepareToMatchElement(element, true);

    propsToApply.clear();
    pseudoProps.clear();
    // now go over selectors that we prepared for check
    // selectors yet in random order, so we store only matched selector indexes to sort after
    unsigned amountOfMatchedSelectors = 0;
    for (int k = 0; k < selectorsForCheck.size(); ++k) {
        unsigned i = selectorsForCheck[k];
        quint16 tag = selectors[i]->tagLocalName.id();
        if (cssTagId == tag || tag == anyLocalName) {
            ++schecked;
            checkSelector(i, e);
            if (selectorCache[i].state == Applies || selectorCache[i].state == AppliesPseudo) {
                selectorsForCheck[amountOfMatchedSelectors++] = i;
            }
        } else
            selectorCache[i].state = Invalid;
    }

    // sort only matched selectors and then collect properties
    qSort(selectorsForCheck.data(), selectorsForCheck.data() + amountOfMatchedSelectors);
    for (unsigned k = 0; k < amountOfMatchedSelectors; ++k) {
        unsigned i = selectorsForCheck[k];
        if (selectorCache[i].state == Applies) {
            ++smatch;
            for (unsigned p = selectorCache[i].firstPropertyIndex; p < properties_size; p = nextPropertyIndexes[p])
                propsToApply.append(propertiesBuffer + p);
        } else if (selectorCache[i].state == AppliesPseudo) {
            for (unsigned p = selectorCache[i].firstPropertyIndex; p < properties_size; p = nextPropertyIndexes[p]) {
                pseudoProps.append(propertiesBuffer + p);
                propertiesBuffer[p].pseudoId = (RenderStyle::PseudoId) selectors[i]->pseudoId;
            }
        }
    }
    // clear selectorsForCheck, it shouldn't be used after
    selectorsForCheck.clear();

    // Inline style declarations, after all others.
    // Non-css hints from presentational attributes will also be collected here
    // receiving the proper priority so has to cascade from before author rules (cf.CSS 2.1-6.4.4).
    addInlineDeclarations(e);

//     qDebug( "styleForElement( %s )", e->tagName().string().toLatin1().constData() );
//     qDebug( "%d selectors, %d checked,  %d match,  %d properties ( of %d )",
// 	    selectors_size, schecked, smatch, numPropsToApply, properties_size );

    if (propsToApply.size())
        bubbleSort(propsToApply.data(), propsToApply.data() + propsToApply.size() - 1);
    if (pseudoProps.size())
        bubbleSort(pseudoProps.data(), pseudoProps.data() + pseudoProps.size() - 1);

    // we can't apply style rules without a view() and a part. This
    // tends to happen on delayed destruction of widget Renderobjects
    if ( part ) {
        fontDirty = false;

        if (propsToApply.size()) {
            CSSStyleSelector::style = style;
            for (unsigned int i = 0; i < propsToApply.size(); ++i) {
		if ( fontDirty && propsToApply[i]->priority >= (1 << 30) ) {
		    // we are past the font properties, time to update to the
		    // correct font
#ifdef APPLE_CHANGES
		    checkForGenericFamilyChange(style, parentStyle);
#endif
		    CSSStyleSelector::style->htmlFont().update( logicalDpiY );
		    fontDirty = false;
		}
		DOM::CSSProperty *prop = propsToApply[i]->prop;
//		if (prop->m_id == CSS_PROP__KONQ_USER_INPUT) qDebug() << "El: "<<e->nodeName().string() << " user-input: "<<((CSSPrimitiveValueImpl *)prop->value())->getIdent();
//		if (prop->m_id == CSS_PROP_TEXT_TRANSFORM) qDebug() << "El: "<<e->nodeName().string();
                applyRule( prop->m_id, prop->value() );
	    }
	    if ( fontDirty ) {
#ifdef APPLE_CHANGES
	        checkForGenericFamilyChange(style, parentStyle);
#endif
		CSSStyleSelector::style->htmlFont().update( logicalDpiY );
            }
        }

        // Clean up our style object's display and text decorations (among other fixups).
        adjustRenderStyle(style, e);

        if (pseudoProps.size()) {
	    fontDirty = false;
            //qDebug("%d applying %d pseudo props", e->cssTagId(), pseudoProps->count() );
            for (unsigned int i = 0; i < pseudoProps.size(); ++i) {
		if ( fontDirty && pseudoProps[i]->priority >= (1 << 30) ) {
		    // we are past the font properties, time to update to the
		    // correct font
		    //We have to do this for all pseudo styles
		    RenderStyle *pseudoStyle = style->pseudoStyle;
		    while ( pseudoStyle ) {
			pseudoStyle->htmlFont().update( logicalDpiY );
			pseudoStyle = pseudoStyle->pseudoStyle;
		    }
		    fontDirty = false;
		}

                RenderStyle *pseudoStyle;
                pseudoStyle = style->getPseudoStyle(pseudoProps[i]->pseudoId);
                if (!pseudoStyle)
                {
                    pseudoStyle = style->addPseudoStyle(pseudoProps[i]->pseudoId);
                    if (pseudoStyle)
                        pseudoStyle->inheritFrom( style );
                }

                RenderStyle* oldStyle = style;
                RenderStyle* oldParentStyle = parentStyle;
                parentStyle = style;
		style = pseudoStyle;
                if ( pseudoStyle ) {
		    DOM::CSSProperty *prop = pseudoProps[i]->prop;
		    applyRule( prop->m_id, prop->value() );
		}
                style = oldStyle;
                parentStyle = oldParentStyle;
            }

	    if ( fontDirty ) {
		RenderStyle *pseudoStyle = style->pseudoStyle;
		while ( pseudoStyle ) {
		    pseudoStyle->htmlFont().update( logicalDpiY );
		    pseudoStyle = pseudoStyle->pseudoStyle;
		}
	    }
        }
    }

    // Now adjust all our pseudo-styles.
    RenderStyle *pseudoStyle = style->pseudoStyle;
    while (pseudoStyle) {
        adjustRenderStyle(pseudoStyle, 0);
        pseudoStyle = pseudoStyle->pseudoStyle;
    }

    // Try and share or partially share the style with our siblings
    RenderStyle *commonStyle = locateSimilarStyle();
    if (commonStyle)
        style->compactWith(commonStyle);

    // Now return the style.
    return style;
}

void CSSStyleSelector::prepareToMatchElement(DOM::ElementImpl* e, bool withDeps)
{
    rememberDependencies = withDeps;
    element              = e;

    // build caches for element so it could be used in heuristic for descendant selectors
    // go up the tree and cache possible tags, classes and ids
    tagCache.clear();
    idCache.clear();
    classCache.clear();
    ElementImpl* current = element;
    while (true) {
        NodeImpl* parent = current->parentNode();
        if (!parent || !parent->isElementNode())
            break;
        current = static_cast<ElementImpl*>(parent);

        if (current->hasClass()) {
            const ClassNames& classNames = current->classNames();
            for (unsigned i = 0; i < classNames.size(); ++i)
                classCache.add((quintptr)classNames[i].impl());
        }

        DOMStringImpl* idValue = current->getAttributeImplById(ATTR_ID);
        if (idValue && idValue->length()) {
            bool caseSensitive = (current->document()->htmlMode() == DocumentImpl::XHtml) || strictParsing;
            AtomicString currentId = caseSensitive ? idValue : idValue->lower();
            // though currentId is local and could be deleted from AtomicStringImpl cache right away
            // don't care about that, cause selector values are stable and only they will be checked later
            idCache.add((quintptr)currentId.impl());
        }

        tagCache.add(localNamePart(current->id()));
    }
}

void CSSStyleSelector::adjustRenderStyle(RenderStyle* style, DOM::ElementImpl *e)
{
     // Cache our original display.
     style->setOriginalDisplay(style->display());

    if (style->display() != NONE) {
        // If we have a <td> that specifies a float property, in quirks mode we just drop the float
        // property.
        // Sites also commonly use display:inline/block on <td>s and <table>s.  In quirks mode we force
        // these tags to retain their display types.
        if (!strictParsing && e) {
            if (e->id() == ID_TD) {
                style->setDisplay(TABLE_CELL);
                style->setFloating(FNONE);
            }
            else if (e->id() == ID_TABLE)
                style->setDisplay(style->isDisplayInlineType() ? INLINE_TABLE : TABLE);
        }

        // Table headers with a text-align of auto will change the text-align to center.
        if (e && e->id() == ID_TH && style->textAlign() == TAAUTO)
            style->setTextAlign(CENTER);

        // Mutate the display to BLOCK or TABLE for certain cases, e.g., if someone attempts to
        // position or float an inline, compact, or run-in.  Cache the original display, since it
        // may be needed for positioned elements that have to compute their static normal flow
        // positions.  We also force inline-level roots to be block-level.
        if (style->display() != BLOCK && style->display() != TABLE /*&& style->display() != BOX*/ &&
            (style->position() == PABSOLUTE || style->position() == PFIXED || style->floating() != FNONE ||
             (e && e->document()->documentElement() == e))) {
             if (style->display() == INLINE_TABLE)
                 style->setDisplay(TABLE);
//             else if (style->display() == INLINE_BOX)
//                 style->setDisplay(BOX);
            else if (style->display() == LIST_ITEM) {
                // It is a WinIE bug that floated list items lose their bullets, so we'll emulate the quirk,
                // but only in quirks mode.
                if (!strictParsing && style->floating() != FNONE)
                    style->setDisplay(BLOCK);
            }
            else
                style->setDisplay(BLOCK);
        } else if (e && e->id() == ID_BUTTON && style->isOriginalDisplayInlineType()) {
            // <button>s are supposed to be replaced elements; but we don't handle 
            // them as such (as they are rendered as CSS contexts, not natives 
            // with intrinsic sizes), so we must be careful not to make them fully 
            // inline, as that will display stuff like width:; so mutate inline-like
            // display types into inline-block
            style->setDisplay(INLINE_BLOCK);
        }

        // After performing the display mutation, check our position.  We do not honor position:relative on
        // table rows and some other table displays. This is undefined behaviour in CSS2.1 (cf. 9.3.1)
        if (style->position() == PRELATIVE) {
            switch (style->display()) {
              case TABLE_ROW_GROUP:
              case TABLE_HEADER_GROUP:
              case TABLE_FOOTER_GROUP:
              case TABLE_ROW:
                style->setPosition(PSTATIC);
              default:
                break;
            }
        }
    }

    // Frames and framesets never honor position:relative or position:absolute.  This is necessary to
    // fix a crash where a site tries to position these objects.
    if ( e ) {
        // ignore display: none for <frame>
        if ( e->id() == ID_FRAME ) {
            style->setPosition( PSTATIC );
            style->setDisplay( BLOCK );
        }
        else if ( e->id() == ID_FRAMESET ) {
            style->setPosition( PSTATIC );
        }
    }

    // Finally update our text decorations in effect, but don't allow text-decoration to percolate through
    // tables, inline blocks, inline tables, or run-ins.
    if (style->display() == TABLE || style->display() == INLINE_TABLE || style->display() == RUN_IN
        || style->display() == INLINE_BLOCK /*|| style->display() == INLINE_BOX*/)
        style->setTextDecorationsInEffect(style->textDecoration());
    else
        style->addToTextDecorationsInEffect(style->textDecoration());

    // If either overflow value is not visible, change to auto.
    if (style->overflowX() == OMARQUEE && style->overflowY() != OMARQUEE)
        style->setOverflowY(OMARQUEE);
    else if (style->overflowY() == OMARQUEE && style->overflowX() != OMARQUEE)
        style->setOverflowX(OMARQUEE);
    else if (style->overflowX() == OVISIBLE && style->overflowY() != OVISIBLE)
        style->setOverflowX(OAUTO);
    else if (style->overflowY() == OVISIBLE && style->overflowX() != OVISIBLE)
        style->setOverflowY(OAUTO);

    // Table rows, sections and the table itself will support overflow:hidden and will ignore scroll/auto.
    // FIXME: Eventually table sections will support auto and scroll.
    if (style->display() == TABLE || style->display() == INLINE_TABLE ||
        style->display() == TABLE_ROW_GROUP || style->display() == TABLE_ROW) {
        if (style->overflowX() != OVISIBLE && style->overflowX() != OHIDDEN)
            style->setOverflowX(OVISIBLE);
        if (style->overflowY() != OVISIBLE && style->overflowY() != OHIDDEN)
            style->setOverflowY(OVISIBLE);

        // do comparable resets as Mozilla's nsStyleContext::ApplyStyleFixups
        // except they decided to do it only for center and right, for whatever strange reason. cf.#193093
        if (style->display() == TABLE && (style->textAlign() == KHTML_LEFT ||
                                          style->textAlign() == KHTML_RIGHT ||
                                          style->textAlign() == KHTML_CENTER)) {
            style->setTextAlign( TAAUTO );
        }

    }

    // Cull out any useless layers and also repeat patterns into additional layers.
    style->adjustBackgroundLayers();
}

void CSSStyleSelector::addInlineDeclarations(DOM::ElementImpl* e)
{
    CSSStyleDeclarationImpl* inlineDecls = e->inlineStyleDecls();
    CSSStyleDeclarationImpl* nonCSSDecls = e->nonCSSStyleDecls();
    if (!inlineDecls && !nonCSSDecls)
        return;

    QList<CSSProperty*>* values = inlineDecls ? inlineDecls->values() : 0;
    QList<CSSProperty*>* nonCSSValues = nonCSSDecls ? nonCSSDecls->values() : 0;
    if (!values && !nonCSSValues)
        return;

    int firstLen = values ? values->count() : 0;
    int secondLen = nonCSSValues ? nonCSSValues->count() : 0;
    uint totalLen = firstLen + secondLen;

    if ((unsigned)inlineProps.size() < totalLen)
	{
        inlineProps.resize(totalLen + 1);
	}
    propsToApply.reserveCapacity(propsToApply.size() + totalLen);

    bool inNonCSSDecls = false;
    CSSOrderedProperty *array = (CSSOrderedProperty *)inlineProps.data();
    for(int i = 0; i < (int)totalLen; i++)
    {
        if (i == firstLen) {
            values = nonCSSValues;
            inNonCSSDecls = true;
        }

        CSSProperty *prop = values->at(i >= firstLen ? i - firstLen : i);
	Source source = Inline;

        if( prop->m_important ) source = InlineImportant;
	if( inNonCSSDecls ) source = NonCSSHint;

	bool first;
        // give special priority to font-xxx, color properties
        switch(prop->m_id)
        {
        case CSS_PROP_FONT_STYLE:
	case CSS_PROP_FONT_SIZE:
	case CSS_PROP_FONT_WEIGHT:
        case CSS_PROP_FONT_FAMILY:
        case CSS_PROP_FONT_VARIANT:
        case CSS_PROP_FONT:
        case CSS_PROP_COLOR:
        case CSS_PROP_DIRECTION:
        case CSS_PROP_DISPLAY:
            // these have to be applied first, because other properties use the computed
            // values of these properties.
	    first = true;
            break;
        default:
            first = false;
            break;
        }

	array->prop = prop;
	array->pseudoId = RenderStyle::NOPSEUDO;
	array->selector = 0;
	array->position = i;
	array->priority = (!first << 30) | (source << 24);
        propsToApply.append(array++);
    }
}

// modified version of the one in kurl.cpp
static void cleanpath(QString &path)
{
    int pos;
    while ((pos = path.indexOf(QLatin1String("/../"))) != -1) {
        int prev = 0;
        if (pos > 0)
            prev = path.lastIndexOf(QLatin1Char('/'), pos - 1);
        // don't remove the host, i.e. http://foo.org/../foo.html
        if (prev < 0 || (prev > 3 && path.midRef(prev - 2, 3) == QLatin1String("://")))
            path.remove(pos, 3);
        else
            // matching directory found ?
            path.remove(prev, pos - prev + 3);
    }
    pos = 0;

    // Don't remove "//" from an anchor identifier. -rjw
    // Set refPos to -2 to mean "I haven't looked for the anchor yet".
    // We don't want to waste a function call on the search for the anchor
    // in the vast majority of cases where there is no "//" in the path.
    int refPos = -2;
    while ((pos = path.indexOf(QLatin1String("//"), pos)) != -1) {
        if (refPos == -2)
            refPos = path.indexOf(QLatin1Char('#'), 0);
        if (refPos > 0 && pos >= refPos)
            break;

        if (pos == 0 || path[pos-1] != QLatin1Char(':'))
            path.remove(pos, 1);
        else
            pos += 2;
    }
    while ((pos = path.indexOf(QLatin1String("/./"))) != -1)
        path.remove(pos, 2);
    //qDebug() << "checkPseudoState " << path;
}

static PseudoState checkPseudoState(const CSSStyleSelector::Encodedurl& encodedurl, DOM::ElementImpl *e)
{
    if (e->id() != ID_A) {
        return PseudoNone;
    }
    DOMString attr = e->getAttribute(ATTR_HREF);
    if (attr.isNull()) {
        return PseudoNone;
    }
    QString url = QString::fromRawData(attr.unicode(), attr.length());
    if (!url.contains(QLatin1String("://"))) {
        if (url[0] == QLatin1Char('/'))
            url = encodedurl.host + url;
        else if (url[0] == QLatin1Char('#'))
            url = encodedurl.file + url;
        else
            url = encodedurl.path + url;
        cleanpath(url);
    }
    //completeURL( attr.string() );
    bool contains = KHTMLGlobal::vLinks()->contains(url);
    if (!contains && url.count(QLatin1Char('/')) == 2)
        contains = KHTMLGlobal::vLinks()->contains(url + QLatin1Char('/'));
    return contains ? PseudoVisited : PseudoLink;
}

// a helper function for parsing nth-arguments
static bool matchNth(int count, const QString& nth)
{
    if (nth.isEmpty()) return false;
    int a = 0;
    int b = 0;
    bool ok = true;
    if (nth == "odd") {
        a = 2;
        b = 1;
    }
    else if (nth == "even") {
        a = 2;
        b = 0;
    }
    else {
        int n = nth.indexOf('n'), l = nth.length();
        if (n != -1) {
            int i = 0, j, sgn = 0;
            // skip trailing spaces
            while (i < n && nth[i].isSpace()) ++i;

            // check sign
            if (nth[i] == '-') { sgn = -1; ++i; }
            else if (nth[i] == '+') { sgn = 1; ++i; }

            // skip spaces between '-'/'+' and digits
            while (i < n && nth[i].isSpace()) ++i;

            // find the number before 'n'
            for (j = i; j < n && nth[j].isDigit(); ++j) {}

            // do we have number or it's assumed to be 1
            a = (i < j) ? nth.mid(i, j - i).toInt(&ok) : 1;
            if (!ok) return false;
            if (sgn == -1) a = -a;

            // should have nothing between a and n except spaces
            for(; j < n; ++j) if (!nth[j].isSpace()) return false;

            // skip spaces
            for (i = n + 1; i < l && nth[i].isSpace(); ++i) {}

            // parse b
            if (i < l) {
                // must have sign
                if (nth[i] == '-') { sgn = -1; ++i; }
                else if (nth[i] == '+') { sgn = 1; ++i; }
                else return false;

                // skip spaces
                while (i < l && nth[i].isSpace()) ++i;

                // find digits
                for (j = i; j < l && nth[j].isDigit(); ++j) {}

                // must have digits
                if (j == i) return false;

                b = sgn * nth.mid(i, j - i).toInt(&ok);
                if (!ok) return false;

                // should be nothing except spaces in the end
                for (; j < l; ++j) if (!nth[j].isSpace()) return false;
            }
        }
        else {
            b = nth.toInt(&ok);
            if (!ok) return false;
        }
    }
    if (a == 0)
        return b != 0 && count == b;
    if (a > 0)
        return (count < b) ? false : ((count - b) % a == 0);
    // a < 0
    return (count > b) ? false : ((b - count) % (-a) == 0);
}


// Recursively work the combinator to compute static attribute dependency, similar to
//structure of checkSubSelectors
static void precomputeAttributeDependenciesAux(DOM::DocumentImpl* doc, DOM::CSSSelector* sel, bool isAncestor, bool isSubject)
{
    if(sel->attrLocalName.id())
    {
        uint selAttr = makeId(sel->attrNamespace.id(), sel->attrLocalName.id());
        // Sets up global dependencies of attributes
        if (isSubject)
            doc->dynamicDomRestyler().addDependency(selAttr, PersonalDependency);
        else if (isAncestor)
            doc->dynamicDomRestyler().addDependency(selAttr, AncestorDependency);
        else
            doc->dynamicDomRestyler().addDependency(selAttr, PredecessorDependency);
    }
    if(sel->match == CSSSelector::PseudoClass)
    {
	switch (sel->pseudoType()) {
            case CSSSelector::PseudoNot:
                precomputeAttributeDependenciesAux(doc, sel->simpleSelector, isAncestor, true);
                break;
            default:
                break;
        }
    }
    CSSSelector::Relation relation = KDE_CAST_BF_ENUM(CSSSelector::Relation, sel->relation);
    sel = sel->tagHistory;
    if (!sel) return;

    switch(relation)
    {
    case CSSSelector::Descendant:
    case CSSSelector::Child:
        precomputeAttributeDependenciesAux(doc, sel, true, false);
        break;
    case CSSSelector::IndirectAdjacent:
    case CSSSelector::DirectAdjacent:
        precomputeAttributeDependenciesAux(doc, sel, false, false);
        break;
    case CSSSelector::SubSelector:
        precomputeAttributeDependenciesAux(doc, sel, isAncestor, isSubject);
        break;
    }
}

void CSSStyleSelector::precomputeAttributeDependencies(DOM::DocumentImpl* doc, DOM::CSSSelector* sel)
{
    precomputeAttributeDependenciesAux(doc, sel, false, true);
}

// Recursive check of selectors and combinators
// It can return 3 different values:
// * SelectorMatches - the selector is match for the node e
// * SelectorFailsLocal - the selector fails for the node e
// * SelectorFails - the selector fails for e and any sibling or ancestor of e
CSSStyleSelector::SelectorMatch CSSStyleSelector::checkSelector(DOM::CSSSelector *sel, DOM::ElementImpl *e, bool isAncestor, bool isSubSelector)
{
    // The simple selector has to match
    if(!checkSimpleSelector(sel, e, isAncestor, isSubSelector)) return SelectorFailsLocal;

    // The rest of the selectors has to match
    CSSSelector::Relation relation = KDE_CAST_BF_ENUM(CSSSelector::Relation, sel->relation);

    // Prepare next sel
    sel = sel->tagHistory;
    if (!sel) return SelectorMatches;

    switch(relation) {
        case CSSSelector::Descendant:
        {
            // if ancestor of original element we may want to check prepared caches first
            // whether given selector could possibly have a match
            // if no we return SelectorFails result right away and avoid going up the tree
            if (isAncestor) {
                int id = sel->tagLocalName.id();
                if (id != anyLocalName && !tagCache.contains(id))
                    return SelectorFails;
                if (sel->match == CSSSelector::Class && !classCache.contains((quintptr)sel->value.impl()))
                    return SelectorFails;
                if (sel->match == CSSSelector::Id && !idCache.contains((quintptr)sel->value.impl()))
                    return SelectorFails;
            }

            while(true)
            {
                DOM::NodeImpl* n = e->parentNode();
                if(!n || !n->isElementNode()) return SelectorFails;
                e = static_cast<ElementImpl *>(n);
                SelectorMatch match = checkSelector(sel, e, true);
                if (match != SelectorFailsLocal)
                    return match;
            }
            break;
        }
        case CSSSelector::Child:
        {
            DOM::NodeImpl* n = e->parentNode();
            if (!strictParsing)
                while (n && n->implicitNode()) n = n->parentNode();
            if(!n || !n->isElementNode()) return SelectorFails;
            e = static_cast<ElementImpl *>(n);
            return checkSelector(sel, e, true);
        }
        case CSSSelector::IndirectAdjacent:
        {
            // Sibling selectors always generate structural dependencies
            // because newly inserted element might fullfill them.
            if (e->parentNode() && e->parentNode()->isElementNode())
                addDependency(StructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
            while(true)
            {
                DOM::NodeImpl* n = e->previousSibling();
                while( n && !n->isElementNode() )
                    n = n->previousSibling();
                if( !n ) return SelectorFailsLocal;
                e = static_cast<ElementImpl *>(n);
                SelectorMatch match = checkSelector(sel, e, false);
                if (match != SelectorFailsLocal)
                    return match;
            };
            break;
        }
        case CSSSelector::DirectAdjacent:
        {
            if (e->parentNode() && e->parentNode()->isElementNode())
                addDependency(StructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
            DOM::NodeImpl* n = e->previousSibling();
            while( n && !n->isElementNode() )
                n = n->previousSibling();
            if( !n ) return SelectorFailsLocal;
            e = static_cast<ElementImpl *>(n);
            return checkSelector(sel, e, false);
        }
        case CSSSelector::SubSelector:
            return checkSelector(sel, e, isAncestor, true);
    }
    assert(false); // never reached
    return SelectorFails;
}

void CSSStyleSelector::checkSelector(int selIndex, DOM::ElementImpl * e)
{
    assert(e == element); // yes, actually

    dynamicPseudo = RenderStyle::NOPSEUDO;

    selectorCache[ selIndex ].state = Invalid;
    CSSSelector *sel = selectors[ selIndex ];

    // Check the selector
    SelectorMatch match = checkSelector(sel, e, true);
    if(match != SelectorMatches) return;

    if ( dynamicPseudo != RenderStyle::NOPSEUDO ) {
	selectorCache[selIndex].state = AppliesPseudo;
	selectors[ selIndex ]->pseudoId = dynamicPseudo;
    } else
	selectorCache[ selIndex ].state = Applies;
    //qDebug( "selector %d applies", selIndex );
    //selectors[ selIndex ]->print();
    return;
}

bool CSSStyleSelector::isMatchedByAnySelector(DOM::ElementImpl* e, const QList<DOM::CSSSelector*>& sels)
{
    bool inited = false;

    quint16 elementTagId = localNamePart(e->id());

    // ### this may introduce extraneous restyling dependencies
    for (int i = 0; i < sels.size(); ++i) {
        DOM::CSSSelector* sel = sels[i];
        quint16 tag = sel->tagLocalName.id();
        if (elementTagId == tag || tag == anyLocalName) {
            if (!inited) {
                prepareToMatchElement(e, false);
                inited = true;
            }
    
            dynamicPseudo = RenderStyle::NOPSEUDO;
            SelectorMatch match = checkSelector(sel, e, true);

            if (match == SelectorMatches && dynamicPseudo == RenderStyle::NOPSEUDO)
                return true;
        }
    }

    return false;
}

void CSSStyleSelector::addDependency(int dependencyType, ElementImpl* dependency)
{
    if (!rememberDependencies) return;
    element->document()->dynamicDomRestyler().addDependency(element, dependency, (StructuralDependencyType)dependencyType);
}

bool CSSStyleSelector::checkSimpleSelector(DOM::CSSSelector *sel, DOM::ElementImpl *e, bool isAncestor, bool isSubSelector)
{
    uint selTag = makeId(sel->tagNamespace.id(), sel->tagLocalName.id());
    if (selTag != anyQName) {
        int eltID = e->id();
        quint16 localName = localNamePart(eltID);
        quint16 ns = namespacePart(eltID);
        quint16 selLocalName = localNamePart(selTag);
        quint16 selNS = namespacePart(selTag);

        // match on local
        if (selLocalName != anyLocalName && localName != selLocalName) return false;
        // match on namespace
        if (selNS != anyNamespace && ns != selNS) return false;
    }

    uint selAttr = makeId(sel->attrNamespace.id(), sel->attrLocalName.id());
    if(selAttr)
    {
        // "class" is special attribute which is pre-parsed for fast look-ups
        // avoid ElementImpl::getAttributeImpl here, as we don't need it
        if (sel->match == CSSSelector::Class)
            return e->hasClass() && e->classNames().contains(sel->value);

        DOMStringImpl* value = e->getAttributeImplById(selAttr);
        if(!value) return false; // attribute is not set

        // attributes are always case-sensitive in XHTML
        // attributes are sometimes case-sensitive in HTML
        bool caseSensitive = (e->document()->htmlMode() == DocumentImpl::XHtml) || caseSensitiveAttr(selAttr);

        switch(sel->match)
        {
        case CSSSelector::Set:
            // True if we make it this far
            break;
        case CSSSelector::Id:
            // treat id selectors as case-sensitive in HTML strict
            // for compatibility reasons
            caseSensitive = (e->document()->htmlMode() == DocumentImpl::XHtml) || strictParsing;
            // no break
        case CSSSelector::Exact:
            return caseSensitive ?
                !strcmp(sel->value.impl(), value) :
                !strcasecmp(sel->value.impl(), value);
            break;
        case CSSSelector::List:
        {
            int sel_len = sel->value.length();
            int val_len = value->length();
            // Be smart compare on length first
            if ((!sel_len && !val_len) || sel_len > val_len) return false;
            // Selector string may not contain spaces
            if ((selAttr != ATTR_CLASS || e->hasClass()) && sel->value.string().find(' ') != -1) return false;
            if (sel_len == val_len)
                return caseSensitive ?
                    !strcmp(sel->value.impl(), value) :
                    !strcasecmp(sel->value.impl(), value);
            // else the value is longer and can be a list
 
            QChar* sel_uc = sel->value.string().unicode();
            QChar* val_uc = value->unicode();

            QString sel_str = QString::fromRawData(sel_uc, sel_len);
            QString val_str = QString::fromRawData(val_uc, val_len);

            int pos = 0;
            for ( ;; ) {
                pos = val_str.indexOf(sel_str, pos, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                if ( pos == -1 ) return false;
                if ( pos == 0 || val_uc[pos-1].isSpace() ) {
                    int endpos = pos + sel_len;
                    if ( endpos >= val_len || val_uc[endpos].isSpace() )
                        break; // We have a match.
                }
                ++pos;
            }
            break;
        }
        case CSSSelector::Contain:
        {
            //qDebug() << "checking for contains match";
            QString val_str = QString::fromRawData(value->unicode(), value->length());
            QString sel_str = QString::fromRawData(sel->value.string().unicode(), sel->value.length());
            return val_str.contains(sel_str, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) && !sel_str.isEmpty();
        }
        case CSSSelector::Begin:
        {
            //qDebug() << "checking for beginswith match";
            DOMStringImpl* selValue = sel->value.impl();
            return selValue && selValue->length() && value->startsWith(selValue, caseSensitive ? DOM::CaseSensitive : DOM::CaseInsensitive);
        }
        case CSSSelector::End:
        {
            //qDebug() << "checking for endswith match";
            DOMStringImpl* selValue = sel->value.impl();
            return selValue && selValue->length() && value->endsWith(selValue, caseSensitive ? DOM::CaseSensitive : DOM::CaseInsensitive);
        }
        case CSSSelector::Hyphen:
        {
            //qDebug() << "checking for hyphen match";
            DOMStringImpl* selValue = sel->value.impl();
            if (value->length() < selValue->length())
                return false;
            // Check if value begins with selStr:
            if (!value->startsWith(selValue, caseSensitive ? DOM::CaseSensitive : DOM::CaseInsensitive))
                return false;
            // It does. Check for exact match or following '-':
            return value->length() == selValue->length() || (*value)[selValue->length()].unicode() == '-';
        }
        case CSSSelector::PseudoClass:
        case CSSSelector::PseudoElement:
        case CSSSelector::None:
            break;
        }
    }

    if(sel->match == CSSSelector::PseudoClass || sel->match == CSSSelector::PseudoElement)
    {
	switch (sel->pseudoType()) {
        // Pseudo classes:
	case CSSSelector::PseudoEmpty: {
	    addDependency(BackwardsStructuralDependency, e);
            // If e is not closed yet we don't know the number of children
            if (!e->closed())
                return false;
            NodeImpl *t = e->firstChild();

            // check for empty text nodes and comments
            while (t && (t->nodeType() == Node::COMMENT_NODE ||
                   (t->isTextNode() && static_cast<TextImpl*>(t)->length() == 0)))
                t = t->nextSibling();

            return !t;
            break;
        }
	case CSSSelector::PseudoFirstChild: {
	    // first-child matches the first child that is an element!
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                // Handle dynamic DOM changes
                addDependency(StructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                DOM::NodeImpl* n = e->previousSibling();
                while ( n && !n->isElementNode() )
                    n = n->previousSibling();
                if ( !n )
                    return true;
            }
            break;
        }
        case CSSSelector::PseudoLastChild: {
            // last-child matches the last child that is an element!
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                // Handle unfinished parsing and dynamic DOM changes
                addDependency(BackwardsStructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                if (!e->parentNode()->closed()) {
//                     qDebug() << e->nodeName().string() << "::last-child: Parent unclosed";
                    return false;
                }
                DOM::NodeImpl* n = e->nextSibling();
                while ( n && !n->isElementNode() )
                    n = n->nextSibling();
                if ( !n )
                    return true;
            }
            break;
        }
        case CSSSelector::PseudoOnlyChild: {
            // If both first-child and last-child apply, then only-child applies.
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                addDependency(BackwardsStructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                if (!e->parentNode()->closed()) {
                    return false;
                }
                DOM::NodeImpl* n = e->previousSibling();
                while ( n && !n->isElementNode() )
                    n = n->previousSibling();
                if ( !n ) {
                    n = e->nextSibling();
                    while ( n && !n->isElementNode() )
                        n = n->nextSibling();
                    if ( !n )
                        return true;
                }
            }
            break;
        }
        case CSSSelector::PseudoNthChild: {
	    // nth-child matches every (a*n+b)th element!
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                addDependency(StructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                int count = 1;
                DOM::NodeImpl* n = e->previousSibling();
                while ( n ) {
                    if (n->isElementNode()) count++;
                    n = n->previousSibling();
                }
//                 qDebug() << "NthChild " << count << "=" << sel->string_arg;
                if (matchNth(count,sel->string_arg.string()))
                    return true;
            }
            break;
        }
        case CSSSelector::PseudoNthLastChild: {
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                addDependency(BackwardsStructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                if (!e->parentNode()->closed()) {
                    return false;
                }
                int count = 1;
                DOM::NodeImpl* n = e->nextSibling();
                while ( n ) {
                    if (n->isElementNode()) count++;
                    n = n->nextSibling();
                }
//                qDebug() << "NthLastChild " << count << "=" << sel->string_arg;
                if (matchNth(count,sel->string_arg.string()))
                    return true;
            }
            break;
        }
	case CSSSelector::PseudoFirstOfType: {
	    // first-of-type matches the first element of its type!
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                addDependency(StructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                const DOMString& type = e->tagName();
                DOM::NodeImpl* n = e->previousSibling();
                while ( n ) {
                    if (n->isElementNode())
                        if (static_cast<ElementImpl*>(n)->tagName() == type) break;
                    n = n->previousSibling();
                }
                if ( !n )
                    return true;
            }
            break;
        }
        case CSSSelector::PseudoLastOfType: {
            // last-child matches the last child that is an element!
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                addDependency(BackwardsStructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                if (!e->parentNode()->closed()) {
                    return false;
                }
                const DOMString& type = e->tagName();
                DOM::NodeImpl* n = e->nextSibling();
                while ( n ) {
                    if (n->isElementNode())
                        if (static_cast<ElementImpl*>(n)->tagName() == type) break;
                    n = n->nextSibling();
                }
                if ( !n )
                    return true;
            }
            break;
        }
        case CSSSelector::PseudoOnlyOfType: {
            // If both first-of-type and last-of-type apply, then only-of-type applies.
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                addDependency(BackwardsStructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                if (!e->parentNode()->closed()) {
                    return false;
                }
                const DOMString& type = e->tagName();
                DOM::NodeImpl* n = e->previousSibling();
                while ( n && !(n->isElementNode() && static_cast<ElementImpl*>(n)->tagName() == type))
                    n = n->previousSibling();
                if ( !n ) {
                    n = e->nextSibling();
                    while ( n && !(n->isElementNode() && static_cast<ElementImpl*>(n)->tagName() == type))
                        n = n->nextSibling();
                    if ( !n )
                        return true;
	        }
            }
	    break;
        }
        case CSSSelector::PseudoNthOfType: {
	    // nth-of-type matches every (a*n+b)th element of this type!
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                addDependency(StructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                int count = 1;
                const DOMString& type = e->tagName();
                DOM::NodeImpl* n = e->previousSibling();
                while ( n ) {
                    if (n->isElementNode() && static_cast<ElementImpl*>(n)->tagName() == type) count++;
                    n = n->previousSibling();
                }
//                qDebug() << "NthOfType " << count << "=" << sel->string_arg;
                if (matchNth(count,sel->string_arg.string()))
                    return true;
            }
            break;
        }
        case CSSSelector::PseudoNthLastOfType: {
            if (e->parentNode() && e->parentNode()->isElementNode()) {
                addDependency(BackwardsStructuralDependency, static_cast<ElementImpl*>(e->parentNode()));
                if (!e->parentNode()->closed()) {
                    return false;
                }
                int count = 1;
                const DOMString& type = e->tagName();
                DOM::NodeImpl* n = e->nextSibling();
                while ( n ) {
                    if (n->isElementNode() && static_cast<ElementImpl*>(n)->tagName() == type) count++;
                    n = n->nextSibling();
                }
//                qDebug() << "NthLastOfType " << count << "=" << sel->string_arg;
                if (matchNth(count,sel->string_arg.string()))
                    return true;
            }
            break;
        }
        case CSSSelector::PseudoTarget:
            if (e == e->document()->getCSSTarget())
                return true;
            break;
        case CSSSelector::PseudoRoot:
            if (e == e->document()->documentElement())
                return true;
            break;
	case CSSSelector::PseudoLink:
	    if (e == element) {
	       // cache pseudoState
	       if ( pseudoState == PseudoUnknown )
                    pseudoState = checkPseudoState( encodedurl, e );
	       if ( pseudoState == PseudoLink )
                    return true;
            } else
                return checkPseudoState( encodedurl, e ) == PseudoLink;
	    break;
	case CSSSelector::PseudoVisited:
	    if (e == element) {
                // cache pseudoState
                if ( pseudoState == PseudoUnknown )
                    pseudoState = checkPseudoState( encodedurl, e );
                if ( pseudoState == PseudoVisited )
                    return true;
            } else
                return checkPseudoState( encodedurl, e ) == PseudoVisited;
	    break;
        case CSSSelector::PseudoHover: {
	    // If we're in quirks mode, then *:hover should only match focusable elements.
	    if (strictParsing || (selTag != anyQName) || isSubSelector || e->isFocusable() ) {
                addDependency(HoverDependency, e);

                if (e->hovered())
			return true;
		}
	    break;
            }
	case CSSSelector::PseudoActive:
	    // If we're in quirks mode, then *:active should only match focusable elements
	    if (strictParsing || (selTag != anyQName) || isSubSelector || e->isFocusable()) {
                addDependency(ActiveDependency, e);

		if (e->active())
		    return true;
	    }
	    break;
	case CSSSelector::PseudoFocus:
	    if (e != element && e->isFocusable()) {
                // *:focus is a default style, no need to track it.
                addDependency(OtherStateDependency, e);
            }
            if (e->focused()) return true;
            break;
        case CSSSelector::PseudoLang: {
            // Set dynamic attribute dependency
            if (e == element) {
                e->document()->dynamicDomRestyler().addDependency(ATTR_LANG, PersonalDependency);
                e->document()->dynamicDomRestyler().addDependency(ATTR_LANG, AncestorDependency);
            } else if (isAncestor)
                e->document()->dynamicDomRestyler().addDependency(ATTR_LANG, AncestorDependency);
            else
                e->document()->dynamicDomRestyler().addDependency(ATTR_LANG, PredecessorDependency);
            // ### check xml:lang attribute in XML and XHTML documents
            DOMStringImpl* value = e->getAttributeImplById(ATTR_LANG);
            // The LANG attribute is inherited like a property
            NodeImpl *n = e->parent();
            while (n && !(value && value->length())) {
                if (n->isElementNode()) {
                    value = static_cast<ElementImpl*>(n)->getAttributeImplById(ATTR_LANG);
                } else if (n->isDocumentNode()) {
                    value = static_cast<DocumentImpl*>(n)->contentLanguage().implementation();
                }
                n = n->parent();
            }
            if (!(value && value->length()))
                return false;

            DOMStringImpl* langValue = sel->string_arg.implementation();
            if (value->length() < langValue->length())
                return false;
            if (!value->startsWith(langValue, DOM::CaseInsensitive))
                return false;
            if (value->length() != langValue->length() && (*value)[langValue->length()].unicode() != '-')
                return false;
            return true;
        }
        case CSSSelector::PseudoNot: {
            // check the simple selector
            for (CSSSelector* subSel = sel->simpleSelector; subSel;
                 subSel = subSel->tagHistory) {
                // :not cannot nest.  I don't really know why this is a restriction in CSS3,
                // but it is, so let's honor it.
                if (subSel->simpleSelector)
                    break;
                if (!checkSimpleSelector(subSel, e, isAncestor, true))
                    return true;
            }
            break;
        }
        case CSSSelector::PseudoEnabled: {
            if (e->isGenericFormElement()) {
                addDependency(OtherStateDependency, e);
                HTMLGenericFormElementImpl *form;
                form = static_cast<HTMLGenericFormElementImpl*>(e);
                return !form->disabled() && !form->isHiddenInput();
            }
            break;
        }
        case CSSSelector::PseudoDisabled: {
            if (e->isGenericFormElement()) {
                addDependency(OtherStateDependency, e);
                HTMLGenericFormElementImpl *form;
                form = static_cast<HTMLGenericFormElementImpl*>(e);
                return form->disabled() && !form->isHiddenInput();
            }
            break;
        }
        case CSSSelector::PseudoContains: {
            if (e->isHTMLElement()) {
                addDependency(BackwardsStructuralDependency, e);
                if (!e->closed()) {
                    return false;
                }
                HTMLElementImpl *elem;
                elem = static_cast<HTMLElementImpl*>(e);
                DOMString s = elem->innerText();
                QString selStr = sel->string_arg.string();
//                qDebug() << ":contains(\"" << selStr << "\")" << " on \"" << s << "\"";
                return s.string().contains(selStr);
            }
            break;
        }
        case CSSSelector::PseudoDefault: {
            if (e->isGenericFormElement()) {
                return static_cast<HTMLGenericFormElementImpl*>(e)->isDefault();
            }
            break;
        }
         case CSSSelector::PseudoReadWrite:
             if (e->isGenericFormElement()) {
                 HTMLGenericFormElementImpl* fe = static_cast<HTMLGenericFormElementImpl*>(e);
                 // no need for dependency tracking - readonly attr change always triggers a recalc
                 return fe->isEditable() && !fe->readOnly();
             } else {
                 // ### contentEditable should either not be accessible from CSS, or should not trigger selection
                 //     by this pseudo class. See with CSS WG.
                 return e->isContentEditable();
             }
             break;
        case CSSSelector::PseudoReadOnly:
             if (e->isGenericFormElement()) {
                 HTMLGenericFormElementImpl* fe = static_cast<HTMLGenericFormElementImpl*>(e);
                 // no need for dependency tracking - readonly attr change always triggers a recalc
                 return !fe->isEditable() || fe->readOnly();
             } else {
                 // ### contentEditable should either not be accessible from CSS, or should not trigger selection
                 //     by this pseudo class. See with CSS WG.
                 return !e->isContentEditable();
             }
             break;
	case CSSSelector::PseudoChecked: {
           if (e->isHTMLElement() && e->id() == ID_INPUT) {
               HTMLInputElementImpl* ie = static_cast<HTMLInputElementImpl*>(e);
               addDependency(OtherStateDependency, e);
               return (ie->checked() &&
                       (ie->inputType() == HTMLInputElementImpl::RADIO || ie->inputType() == HTMLInputElementImpl::CHECKBOX));
           }
           return false;
        }
 	case CSSSelector::PseudoIndeterminate: {
#if 0
           if (e->isHTMLElement() && e->id() == ID_INPUT) {
               return (static_cast<HTMLInputElementImpl*>(e)->indeterminate() &&
                      !static_cast<HTMLInputElementImpl*>(e)->checked());
           }
           return false;
#endif
        }
	case CSSSelector::PseudoOther:
	    break;

	// Pseudo-elements:
	case CSSSelector::PseudoFirstLine:
	case CSSSelector::PseudoFirstLetter:
	case CSSSelector::PseudoSelection:
	case CSSSelector::PseudoBefore:
	case CSSSelector::PseudoAfter:
	case CSSSelector::PseudoMarker:
	case CSSSelector::PseudoReplaced:
	    // Pseudo-elements can only apply to subject
	    if ( e == element ) {
                // Pseudo-elements has to be the last sub-selector on subject
                if (sel->tagHistory && sel->relation == CSSSelector::SubSelector) return false;

                assert(dynamicPseudo == RenderStyle::NOPSEUDO);

                switch (sel->pseudoType()) {
                case CSSSelector::PseudoFirstLine:
                    dynamicPseudo = RenderStyle::FIRST_LINE;
	    break;
	case CSSSelector::PseudoFirstLetter:
                    dynamicPseudo = RenderStyle::FIRST_LETTER;
	    break;
	case CSSSelector::PseudoSelection:
	    dynamicPseudo = RenderStyle::SELECTION;
                    break;
	case CSSSelector::PseudoBefore:
	    dynamicPseudo = RenderStyle::BEFORE;
                    break;
	case CSSSelector::PseudoAfter:
	    dynamicPseudo = RenderStyle::AFTER;
                    break;
                case CSSSelector::PseudoMarker:
                    dynamicPseudo = RenderStyle::MARKER;
                    break;
                case CSSSelector::PseudoReplaced:
                    dynamicPseudo = RenderStyle::REPLACED;
                    break;
                default:
                    assert(false);
                }
	    return true;
	    }
	    break;
	case CSSSelector::PseudoNotParsed:
	    assert(false);
	    break;
	}
	return false;
    }
    // ### add the rest of the checks...
    return true;
}

void CSSStyleSelector::clearLists()
{
    delete[] selectors;
    if (selectorCache) {
        delete[] selectorCache;
        delete[] nextSimilarSelector;
    }
    if (propertiesBuffer) {
        delete[] propertiesBuffer;
        delete[] nextPropertyIndexes;
    }
    selectors = 0;
    propertiesBuffer = 0;
    selectorCache = 0;
    nextPropertyIndexes = 0;

    classSelector.clear();
    idSelector.clear();
    tagSelector.clear();
}

void CSSStyleSelector::setupDefaultRootStyle(DOM::DocumentImpl *d)
{
    assert(m_fontSizes.size());

    // We only care about setting up a default font
    // that the media queries module can use early for
    // computing its font-relative units (e.g.em/ex)
    if (d) {
        // setup some variables needed by applyRule
        logicalDpiY = d->logicalDpiY();
        if (d->view())
            view = d->view();
        if (view)
            part = view->part();
        if (part)
           settings = part->settings();
    }
    parentNode = 0;
    delete m_rootDefaultStyle;
    m_rootDefaultStyle = style = new RenderStyle();
    CSSInitialValueImpl i(true);
    applyRule( CSS_PROP_FONT_SIZE, &i );
    style->htmlFont().update( logicalDpiY );
}

void CSSStyleSelector::buildLists()
{
    clearLists();
    // collect all selectors and Properties in lists. Then transfer them to the array for faster lookup.

    QList<CSSSelector*> selectorList;
    CSSOrderedPropertyList propertyList;
    WTF::HashMap<CSSSelector*, int> selectorsCache;

    if(defaultPrintStyle && m_medium->mediaTypeMatchSpecific("print"))
      defaultPrintStyle->collect(&selectorsCache, &selectorList, &propertyList, Default,
        Default );
    else if(defaultStyle) defaultStyle->collect(&selectorsCache, &selectorList, &propertyList,
      Default, Default );

    if (!strictParsing && defaultQuirksStyle)
        defaultQuirksStyle->collect(&selectorsCache, &selectorList, &propertyList, Default, Default );

    if(userStyle) userStyle->collect(&selectorsCache, &selectorList, &propertyList, User, UserImportant );

    if (defaultNonCSSHintsStyle)
        defaultNonCSSHintsStyle->collect(&selectorsCache, &selectorList, &propertyList, NonCSSHint, NonCSSHint);

    // Implicit styles are gathered from hidden, dynamically generated, implicit stylesheets.
    // They have the same priority as presentational attributes.
    if (implicitStyle) implicitStyle->collect(&selectorsCache, &selectorList, &propertyList, NonCSSHint, NonCSSHint);

    if (authorStyle) authorStyle->collect(&selectorsCache, &selectorList, &propertyList, Author, AuthorImportant );

    selectors_size = selectorList.count();
    selectors = new CSSSelector *[selectors_size];
    CSSSelector **sel = selectors;
    for (QListIterator<CSSSelector*> sit(selectorList); sit.hasNext(); ++sel)
	*sel = sit.next();

    selectorCache = new SelectorCache[selectors_size];
    for (unsigned int i = 0; i < selectors_size; i++) {
        selectorCache[i].state = Unknown;
        selectorCache[i].firstPropertyIndex = propertyList.size();
    }

    // do some pre-compution to make styleForElement faster:
    // 1. class selectors: put in hash by selector value
    // 2. the same goes for id selectors
    // 3. put tag selectors in hash by id
    // 4. other selectors (shouldn't be much) goes to plain list
    nextSimilarSelector = new unsigned[selectors_size];
    otherSelector = selectors_size;
    for (unsigned int i = 0; i < selectors_size; ++i)
        nextSimilarSelector[i] = selectors_size;
    for (int i = selectors_size - 1; i >= 0; --i) {
        if (selectors[i]->match == CSSSelector::Class) {
            pair<WTF::HashMap<quintptr, int>::iterator, bool> it = classSelector.add((quintptr)selectors[i]->value.impl(), i);
            if (!it.second) {
                nextSimilarSelector[i] = it.first->second;
                it.first->second = i;
            }
        } else if (selectors[i]->match == CSSSelector::Id) {
            pair<WTF::HashMap<quintptr, int>::iterator, bool> it = idSelector.add((quintptr)selectors[i]->value.impl(), i);
            if (!it.second) {
                nextSimilarSelector[i] = it.first->second;
                it.first->second = i;
            }
        } else if (selectors[i]->tagLocalName.id() && selectors[i]->tagLocalName.id() != anyLocalName) {
            pair<WTF::HashMap<unsigned, int>::iterator, bool> it = tagSelector.add(selectors[i]->tagLocalName.id(), i);
            if (!it.second) {
                nextSimilarSelector[i] = it.first->second;
                it.first->second = i;
            }
        } else {
            nextSimilarSelector[i] = otherSelector;
            otherSelector = i;
        }
    }

    // presort properties. Should make the sort() calls in styleForElement faster.
    qSort(propertyList.begin(), propertyList.end());
    properties_size = propertyList.count();
    propertiesBuffer = new CSSOrderedProperty[properties_size];
    for (int i = 0; i < propertyList.size(); ++i)
        propertiesBuffer[i] = propertyList[i];

    // properties for one selector are not necessarily adjacent at this point
    // prepare sublists with same selector
    // create for every property next property index with same selector
    nextPropertyIndexes = new unsigned[properties_size];
    for (int i = properties_size - 1; i >= 0; --i) {
        unsigned selector = propertiesBuffer[i].selector;
        nextPropertyIndexes[i] = selectorCache[selector].firstPropertyIndex;
        selectorCache[selector].firstPropertyIndex = i;
    }
}


// ----------------------------------------------------------------------


CSSOrderedRule::CSSOrderedRule(DOM::CSSStyleRuleImpl *r, DOM::CSSSelector *s, int _index)
{
    rule = r;
    if(rule) r->ref();
    index = _index;
    selector = s;
}

CSSOrderedRule::~CSSOrderedRule()
{
    if(rule) rule->deref();
}

// -----------------------------------------------------------------

CSSStyleSelectorList::CSSStyleSelectorList()
    : QList<CSSOrderedRule*>()
{
}
CSSStyleSelectorList::~CSSStyleSelectorList()
{
    qDeleteAll(*this);
    clear();
}

void CSSStyleSelectorList::append( CSSStyleSheetImpl *sheet,
                                   MediaQueryEvaluator* medium, CSSStyleSelector* styleSelector )
{
    if(!sheet || !sheet->isCSSStyleSheet()) return;

    // No media implies "all", but if a medialist exists it must
    // contain our current medium
    if( sheet->media() && !medium->eval(sheet->media(), styleSelector) )
        return; // style sheet not applicable for this medium

    int len = sheet->length();
    for(int i = 0; i< len; i++)
    {
        StyleBaseImpl *item = sheet->item(i);
        if(item->isStyleRule())
        {
            CSSStyleRuleImpl *r = static_cast<CSSStyleRuleImpl *>(item);
            QList<CSSSelector*> *s = r->selector();
            for(int j = 0; j < (int)s->count(); j++)
            {
                CSSOrderedRule *rule = new CSSOrderedRule(r, s->at(j), count());
		QList<CSSOrderedRule*>::append(rule);
                //qDebug() << "appending StyleRule!";
            }
        }
        else if(item->isImportRule())
        {
            CSSImportRuleImpl *import = static_cast<CSSImportRuleImpl *>(item);

            //qDebug() << "@import: Media: "
            //                << import->media()->mediaText().string() << endl;

            if( !import->media() || medium->eval(import->media(), styleSelector) )
            {
                CSSStyleSheetImpl *importedSheet = import->styleSheet();
                append( importedSheet, medium, styleSelector );
            }
        }
        else if( item->isMediaRule() )
        {
            CSSMediaRuleImpl *r = static_cast<CSSMediaRuleImpl *>( item );
            CSSRuleListImpl *rules = r->cssRules();

            //qDebug() << "@media: Media: "
            //                << r->media()->mediaText().string() << endl;

            if( ( !r->media() || medium->eval(r->media(), styleSelector)) && rules)
            {
                // Traverse child elements of the @import rule. Since
                // many elements are not allowed as child we do not use
                // a recursive call to append() here
                for( unsigned j = 0; j < rules->length(); j++ )
                {
                    //qDebug() << "*** Rule #" << j;

                    CSSRuleImpl *childItem = rules->item( j );
                    if( childItem->isStyleRule() )
                    {
                        // It is a StyleRule, so append it to our list
                        CSSStyleRuleImpl *styleRule =
                                static_cast<CSSStyleRuleImpl *>( childItem );

                        QList<CSSSelector*> *s = styleRule->selector();
                        for( int j = 0; j < ( int ) s->count(); j++ )
                        {
                            CSSOrderedRule *orderedRule = new CSSOrderedRule(
                                            styleRule, s->at( j ), count() );
                	    QList<CSSOrderedRule*>::append( orderedRule );
                        }
                    }
                    else if ( childItem->isFontFaceRule() && styleSelector ) {
                        const CSSFontFaceRuleImpl* fontFaceRule = static_cast<CSSFontFaceRuleImpl*>(childItem);
                        styleSelector->fontSelector()->addFontFaceRule(fontFaceRule);
                    }
                    else
                    {
                        //qDebug() << "Ignoring child rule of "
                        //    "ImportRule: rule is not a StyleRule!" << endl;
                    }
                }   // for rules
            }   // if rules
            else
            {
                //qDebug() << "CSSMediaRule not rendered: "
                //                << "rule empty or wrong medium!" << endl;
            }
        }
        else if ( item->isFontFaceRule() && styleSelector )
        {
            const CSSFontFaceRuleImpl* fontFaceRule = static_cast<CSSFontFaceRuleImpl*>(item);
            styleSelector->fontSelector()->addFontFaceRule(fontFaceRule);
        }
        // ### include other rules
    }
}

void CSSStyleSelectorList::collect(WTF::HashMap<CSSSelector*, int>* selectorsCache, QList<CSSSelector*> *selectorList,
        CSSOrderedPropertyList *propList, Source regular, Source important)
{
    CSSOrderedRule *r;
    QListIterator<CSSOrderedRule*> tIt(*this);

    propList->reserve(propList->size() + selectorList->size());
    while( tIt.hasNext() ) {
        r = tIt.next();
        int selectorNum = selectorsCache->size();
        pair<WTF::HashMap<CSSSelector*, int>::iterator, bool> cacheIterator = selectorsCache->add(r->selector, selectorNum);
        if (cacheIterator.second)
            selectorList->append(r->selector);
        selectorNum = cacheIterator.first->second;
        propList->append(r->rule->declaration(), selectorNum, r->selector->specificity(), regular, important);
    }
}

// -------------------------------------------------------------------------

void CSSOrderedPropertyList::append(DOM::CSSStyleDeclarationImpl *decl, uint selector, uint specificity,
                                    Source regular, Source important)
{
    QList<CSSProperty*> *values = decl->values();
    if(!values) return;
    int len = values->count();
    for(int i = 0; i < len; i++)
    {
        CSSProperty *prop = values->at(i);
	Source source = regular;

	if( prop->m_important ) source = important;

	bool first = false;
        // give special priority to font-xxx, color properties
        switch(prop->m_id)
        {
        case CSS_PROP_FONT_STYLE:
	case CSS_PROP_FONT_SIZE:
	case CSS_PROP_FONT_WEIGHT:
        case CSS_PROP_FONT_FAMILY:
        case CSS_PROP_FONT_VARIANT:
        case CSS_PROP_FONT:
        case CSS_PROP_COLOR:
        case CSS_PROP_DIRECTION:
        case CSS_PROP_DISPLAY:
            // these have to be applied first, because other properties use the computed
            // values of these porperties.
	    first = true;
            break;
        default:
            break;
        }

        QVector<CSSOrderedProperty>::append(CSSOrderedProperty(prop, selector, first, source, specificity, count()));
    }
}

// -------------------------------------------------------------------------------------
// this is mostly boring stuff on how to apply a certain rule to the renderstyle...

static Length convertToLength( CSSPrimitiveValueImpl *primitiveValue, RenderStyle *style, int logicalDpiY, bool *ok = 0 )
{
    Length l;
    if ( !primitiveValue ) {
	if ( ok )
            *ok = false;
    } else {
	int type = primitiveValue->primitiveType();
	if(type > CSSPrimitiveValue::CSS_PERCENTAGE && type < CSSPrimitiveValue::CSS_DEG)
	    l = Length(primitiveValue->computeLength(style, logicalDpiY), Fixed);
	else if(type == CSSPrimitiveValue::CSS_PERCENTAGE)
	    l = Length(primitiveValue->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
	else if(type == CSSPrimitiveValue::CSS_NUMBER)
	    l = Length(primitiveValue->floatValue(CSSPrimitiveValue::CSS_NUMBER)*100.0, Percent);
	else if (type == CSSPrimitiveValue::CSS_HTML_RELATIVE)
	    l = Length(int(primitiveValue->floatValue(CSSPrimitiveValue::CSS_HTML_RELATIVE)), Relative);
	else if ( ok )
	    *ok = false;
    }
    return l;
}

// Extracts out border radii lengths from a pair
static BorderRadii convertToBorderRadii(CSSPrimitiveValueImpl *value, RenderStyle *style, int logicalDpiY)
{
    BorderRadii ret;
    if (!value)
        return ret;

    PairImpl* p = value->getPairValue();
    if (!p)
        return ret;

    assert(p->first()->isPrimitiveValue() && p->second()->isPrimitiveValue());
    ret.horizontal = static_cast<CSSPrimitiveValueImpl*>(p->first())->computeLength(style, logicalDpiY);
    ret.vertical   = static_cast<CSSPrimitiveValueImpl*>(p->second())->computeLength(style, logicalDpiY);
    return ret;
}

static inline int nextFontSize(const QVector<int>& a, int v, bool smaller)
{
    // return the nearest bigger/smaller value in scale a, when v is in range.
    // otherwise increase/decrease value using a 1.2 fixed ratio
    int m, l = 0, r = a.count()-1;
    while (l <= r) {
        m = (l+r)/2;
        if (a[m] == v)
            return smaller ? ( m ? a[m-1] : (v*5)/6 ) :
                             ( m+1<int(a.count()) ? a[m+1] : (v*6)/5 );
        else if (v < a[m])
            r = m-1;
        else
            l = m+1;
    }
    if (!l)
        return smaller ? (v*5)/6 : qMin((v*6)/5, a[0]);
    if (l == int(a.count()))
        return smaller ? qMax((v*5)/6, a[r]) : (v*6)/5;

    return smaller ? a[r] : a[l];
}

// If we're explicitly inheriting an initial border-color, its computed value is based
// on the parents' computed value of color, not ours.
static QColor inheritedBorderColor( RenderStyle* parentStyle, const QColor& value )
{
    if ( value.isValid() )
       return value;
    else
       return parentStyle->color();
}

void CSSStyleSelector::applyRule( int id, DOM::CSSValueImpl *value )
{
//     qDebug() << "applying property " << getPropertyName(id);

    CSSPrimitiveValueImpl *primitiveValue = 0;
    if(value->isPrimitiveValue()) primitiveValue = static_cast<CSSPrimitiveValueImpl *>(value);

    Length l;
    bool apply = false;

    bool isInherit = (parentNode && value->cssValueType() == CSSValue::CSS_INHERIT);
    bool isInitial = (value->cssValueType() == CSSValue::CSS_INITIAL) ||
                     (!parentNode && value->cssValueType() == CSSValue::CSS_INHERIT);

    // These properties are used to set the correct margins/padding on RTL lists.
    if (id == CSS_PROP__KHTML_MARGIN_START)
        id = style->direction() == LTR ? CSS_PROP_MARGIN_LEFT : CSS_PROP_MARGIN_RIGHT;
    else if (id == CSS_PROP__KHTML_PADDING_START)
        id = style->direction() == LTR ? CSS_PROP_PADDING_LEFT : CSS_PROP_PADDING_RIGHT;

    // What follows is a list that maps the CSS properties into their corresponding front-end
    // RenderStyle values.  Shorthands (e.g. border, background) occur in this list as well and
    // are only hit when mapping "inherit" or "initial" into front-end values.
    switch(id)
    {
// ident only properties
    case CSS_PROP_BACKGROUND_ATTACHMENT:
        HANDLE_BACKGROUND_VALUE(backgroundAttachment, BackgroundAttachment, value)
        break;
    case CSS_PROP__KHTML_BACKGROUND_CLIP:
    case CSS_PROP_BACKGROUND_CLIP:
        HANDLE_BACKGROUND_VALUE(backgroundClip, BackgroundClip, value)
        break;
    case CSS_PROP__KHTML_BACKGROUND_ORIGIN:
    case CSS_PROP_BACKGROUND_ORIGIN:
        HANDLE_BACKGROUND_VALUE(backgroundOrigin, BackgroundOrigin, value)
        break;
    case CSS_PROP_BACKGROUND_REPEAT:
        HANDLE_BACKGROUND_VALUE(backgroundRepeat, BackgroundRepeat, value)
        break;
    case CSS_PROP__KHTML_BACKGROUND_SIZE:
    case CSS_PROP_BACKGROUND_SIZE:
        HANDLE_BACKGROUND_VALUE(backgroundSize, BackgroundSize, value)
        break;
    case CSS_PROP_BORDER_COLLAPSE:
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(borderCollapse, BorderCollapse)
        if(!primitiveValue) break;
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL_COLLAPSE:
            style->setBorderCollapse(true);
            break;
        case CSS_VAL_SEPARATE:
            style->setBorderCollapse(false);
            break;
        default:
            return;
        }
        break;

    case CSS_PROP_BORDER_TOP_STYLE:
        HANDLE_INHERIT_AND_INITIAL_WITH_VALUE(borderTopStyle, BorderTopStyle, BorderStyle)
        if (!primitiveValue) return;
        style->setBorderTopStyle((EBorderStyle)(primitiveValue->getIdent() - CSS_VAL__KHTML_NATIVE));
        break;
    case CSS_PROP_BORDER_RIGHT_STYLE:
        HANDLE_INHERIT_AND_INITIAL_WITH_VALUE(borderRightStyle, BorderRightStyle, BorderStyle)
        if (!primitiveValue) return;
        style->setBorderRightStyle((EBorderStyle)(primitiveValue->getIdent() - CSS_VAL__KHTML_NATIVE));
        break;
    case CSS_PROP_BORDER_BOTTOM_STYLE:
        HANDLE_INHERIT_AND_INITIAL_WITH_VALUE(borderBottomStyle, BorderBottomStyle, BorderStyle)
        if (!primitiveValue) return;
        style->setBorderBottomStyle((EBorderStyle)(primitiveValue->getIdent() - CSS_VAL__KHTML_NATIVE));
        break;
    case CSS_PROP_BORDER_LEFT_STYLE:
        HANDLE_INHERIT_AND_INITIAL_WITH_VALUE(borderLeftStyle, BorderLeftStyle, BorderStyle)
        if (!primitiveValue) return;
        style->setBorderLeftStyle((EBorderStyle)(primitiveValue->getIdent() - CSS_VAL__KHTML_NATIVE));
        break;
    case CSS_PROP_OUTLINE_STYLE:
        HANDLE_INHERIT_AND_INITIAL_WITH_VALUE(outlineStyle, OutlineStyle, BorderStyle)
        if (!primitiveValue) return;
        style->setOutlineStyle((EBorderStyle)(primitiveValue->getIdent() - CSS_VAL__KHTML_NATIVE));
        break;
    case CSS_PROP_CAPTION_SIDE:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(captionSide, CaptionSide)

        if(!primitiveValue) break;
        ECaptionSide c = RenderStyle::initialCaptionSide();
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL_LEFT:
            c = CAPLEFT; break;
        case CSS_VAL_RIGHT:
            c = CAPRIGHT; break;
        case CSS_VAL_TOP:
            c = CAPTOP; break;
        case CSS_VAL_BOTTOM:
            c = CAPBOTTOM; break;
        default:
            return;
        }
        style->setCaptionSide(c);
        return;
    }
    case CSS_PROP_CLEAR:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(clear, Clear)
        if(!primitiveValue) break;
        EClear c = CNONE;
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL_LEFT:
            c = CLEFT; break;
        case CSS_VAL_RIGHT:
            c = CRIGHT; break;
        case CSS_VAL_BOTH:
            c = CBOTH; break;
        case CSS_VAL_NONE:
            c = CNONE; break;
        default:
            return;
        }
        style->setClear(c);
        return;
    }
    case CSS_PROP_DIRECTION:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(direction, Direction)
        if(!primitiveValue) break;
        style->setDirection( (EDirection) (primitiveValue->getIdent() - CSS_VAL_LTR) );
        return;
    }
    case CSS_PROP_DISPLAY:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(display, Display)
        if(!primitiveValue) break;
	int id = primitiveValue->getIdent();
        style->setDisplay( id == CSS_VAL_NONE ? NONE : EDisplay(id - CSS_VAL_INLINE) );
        break;
    }

    case CSS_PROP_EMPTY_CELLS:
    {
        HANDLE_INHERIT_ON_INHERITED_PROPERTY(emptyCells, EmptyCells);
        if (!primitiveValue) break;
        int id = primitiveValue->getIdent();
        if (id == CSS_VAL_SHOW)
            style->setEmptyCells(SHOW);
        else if (id == CSS_VAL_HIDE)
            style->setEmptyCells(HIDE);
        break;
    }
    case CSS_PROP_FLOAT:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(floating, Floating)
        if(!primitiveValue) return;
        EFloat f;
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL__KHTML_LEFT:
            f = FLEFT_ALIGN; break;
        case CSS_VAL_LEFT:
            f = FLEFT; break;
        case CSS_VAL__KHTML_RIGHT:
            f = FRIGHT_ALIGN; break;
        case CSS_VAL_RIGHT:
            f = FRIGHT; break;
        case CSS_VAL_NONE:
        case CSS_VAL_CENTER:  //Non standart CSS-Value
            f = FNONE; break;
        default:
            return;
        }
        if (f!=FNONE && style->display()==LIST_ITEM)
            style->setDisplay(BLOCK);

        style->setFloating(f);
        break;
    }

    case CSS_PROP_FONT_STYLE:
    {
        FontDef fontDef = style->htmlFont().fontDef;
        if (isInherit)
            fontDef.italic = parentStyle->htmlFont().fontDef.italic;
	else if (isInitial)
            fontDef.italic = false;
        else {
	    if(!primitiveValue) return;
	    switch(primitiveValue->getIdent()) {
		case CSS_VAL_OBLIQUE:
		// ### oblique is the same as italic for the moment...
		case CSS_VAL_ITALIC:
		    fontDef.italic = true;
		    break;
		case CSS_VAL_NORMAL:
		    fontDef.italic = false;
		    break;
		default:
		    return;
	    }
	}
        fontDirty |= style->setFontDef( fontDef );
        break;
    }


    case CSS_PROP_FONT_VARIANT:
    {
        FontDef fontDef = style->htmlFont().fontDef;
        if (isInherit)
            fontDef.smallCaps = parentStyle->htmlFont().fontDef.smallCaps;
        else if (isInitial)
            fontDef.smallCaps = false;
        else {
	    if(!primitiveValue) return;
	    int id = primitiveValue->getIdent();
	    if ( id == CSS_VAL_NORMAL )
		fontDef.smallCaps = false;
	    else if ( id == CSS_VAL_SMALL_CAPS )
		fontDef.smallCaps = true;
	    else
		return;
	}
	fontDirty |= style->setFontDef( fontDef );
	break;
    }

    case CSS_PROP_FONT_WEIGHT:
    {
        FontDef fontDef = style->htmlFont().fontDef;
        if (isInherit)
            fontDef.weight = parentStyle->htmlFont().fontDef.weight;
        else if (isInitial)
            fontDef.weight = QFont::Normal;
        else {
	    if(!primitiveValue) return;
	    if(primitiveValue->getIdent())
	    {
		switch(primitiveValue->getIdent()) {
		    // ### we just support normal and bold fonts at the moment...
		    // setWeight can actually accept values between 0 and 99...
		case CSS_VAL_BOLD:
		case CSS_VAL_BOLDER:
		case CSS_VAL_600:
		case CSS_VAL_700:
		case CSS_VAL_800:
		case CSS_VAL_900:
		    fontDef.weight = QFont::Bold;
		    break;
		case CSS_VAL_NORMAL:
		case CSS_VAL_LIGHTER:
		case CSS_VAL_100:
		case CSS_VAL_200:
		case CSS_VAL_300:
		case CSS_VAL_400:
		case CSS_VAL_500:
		    fontDef.weight = QFont::Normal;
		    break;
		default:
		    return;
		}
	    }
	    else
	    {
		// ### fix parsing of 100-900 values in parser, apply them here
	    }
	}
        fontDirty |= style->setFontDef( fontDef );
        break;
    }

    case CSS_PROP_LIST_STYLE_POSITION:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(listStylePosition, ListStylePosition)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent())
            style->setListStylePosition( (EListStylePosition) (primitiveValue->getIdent() - CSS_VAL_OUTSIDE) );
        return;
    }

    case CSS_PROP_LIST_STYLE_TYPE:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(listStyleType, ListStyleType)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent())
        {
            EListStyleType t;
	    int id = primitiveValue->getIdent();
	    if ( id == CSS_VAL_NONE) { // important!!
	      t = LNONE;
	    } else {
	      t = EListStyleType(id - CSS_VAL_DISC);
	    }
            style->setListStyleType(t);
        }
        return;
    }

    case CSS_PROP_OVERFLOW:
    {
        if (isInherit) {
            style->setOverflowX(parentStyle->overflowX());
            style->setOverflowY(parentStyle->overflowY());
            style->setInheritedNoninherited(true);
            return;
        }

        if (isInitial) {
            style->setOverflowX(RenderStyle::initialOverflowX());
            style->setOverflowY(RenderStyle::initialOverflowY());
            return;
        }

        if (!primitiveValue) return;
        EOverflow o;
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL_VISIBLE:
            o = OVISIBLE; break;
        case CSS_VAL_HIDDEN:
            o = OHIDDEN; break;
        case CSS_VAL_SCROLL:
	    o = OSCROLL; break;
        case CSS_VAL_AUTO:
	    o = OAUTO; break;
        case CSS_VAL_MARQUEE:
            o = OMARQUEE; break;
        default:
            return;
        }
        style->setOverflowX(o);
        style->setOverflowY(o);
        return;
    }
    case CSS_PROP_OVERFLOW_X:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(overflowX, OverflowX)
        if (!primitiveValue) return;
        EOverflow o;
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL_VISIBLE:
            o = OVISIBLE; break;
        case CSS_VAL_HIDDEN:
            o = OHIDDEN; break;
        case CSS_VAL_SCROLL:
	    o = OSCROLL; break;
        case CSS_VAL_AUTO:
	    o = OAUTO; break;
        default:
            return;
        }
        style->setOverflowX(o);
        return;
    }
    case CSS_PROP_OVERFLOW_Y:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(overflowY, OverflowY)
        if (!primitiveValue) return;
        EOverflow o;
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL_VISIBLE:
            o = OVISIBLE; break;
        case CSS_VAL_HIDDEN:
            o = OHIDDEN; break;
        case CSS_VAL_SCROLL:
            o = OSCROLL; break;
        case CSS_VAL_AUTO:
            o = OAUTO; break;
        default:
            return;
        }
        style->setOverflowY(o);
        return;
    }
    case CSS_PROP_PAGE_BREAK_BEFORE:
    {
        HANDLE_INHERIT_AND_INITIAL_WITH_VALUE(pageBreakBefore, PageBreakBefore, PageBreak)
        if (!primitiveValue) return;
        switch (primitiveValue->getIdent()) {
            case CSS_VAL_AUTO:
                style->setPageBreakBefore(PBAUTO);
                break;
            case CSS_VAL_LEFT:
            case CSS_VAL_RIGHT:
                // CSS2.1: "Conforming user agents may map left/right to always."
            case CSS_VAL_ALWAYS:
                style->setPageBreakBefore(PBALWAYS);
                break;
            case CSS_VAL_AVOID:
                style->setPageBreakBefore(PBAVOID);
                break;
        }
        break;
    }

    case CSS_PROP_PAGE_BREAK_AFTER:
    {
        HANDLE_INHERIT_AND_INITIAL_WITH_VALUE(pageBreakAfter, PageBreakAfter, PageBreak)
        if (!primitiveValue) return;
        switch (primitiveValue->getIdent()) {
            case CSS_VAL_AUTO:
                style->setPageBreakAfter(PBAUTO);
                break;
            case CSS_VAL_LEFT:
            case CSS_VAL_RIGHT:
                // CSS2.1: "Conforming user agents may map left/right to always."
            case CSS_VAL_ALWAYS:
                style->setPageBreakAfter(PBALWAYS);
                break;
            case CSS_VAL_AVOID:
                style->setPageBreakAfter(PBAVOID);
                break;
        }
        break;
    }

    case CSS_PROP_PAGE_BREAK_INSIDE: {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(pageBreakInside, PageBreakInside)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent() == CSS_VAL_AUTO)
            style->setPageBreakInside(true);
        else if (primitiveValue->getIdent() == CSS_VAL_AVOID)
            style->setPageBreakInside(false);
        return;
    }
//    case CSS_PROP_PAUSE_AFTER:
//    case CSS_PROP_PAUSE_BEFORE:
        break;

    case CSS_PROP_POSITION:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(position, Position)
        if (!primitiveValue) return;
        EPosition p;
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL_STATIC:
            p = PSTATIC; break;
        case CSS_VAL_RELATIVE:
            p = PRELATIVE; break;
        case CSS_VAL_ABSOLUTE:
            p = PABSOLUTE; break;
        case CSS_VAL_FIXED:
            p = PFIXED; break;
        default:
            return;
        }
        style->setPosition(p);
        return;
    }

    case CSS_PROP_TABLE_LAYOUT: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(tableLayout, TableLayout)

	if ( !primitiveValue )
	    return;

	ETableLayout l = RenderStyle::initialTableLayout();
	switch( primitiveValue->getIdent() ) {
	case CSS_VAL_FIXED:
	    l = TFIXED;
	    // fall through
	case CSS_VAL_AUTO:
	    style->setTableLayout( l );
	default:
	    break;
	}
	break;
    }

    case CSS_PROP_UNICODE_BIDI: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(unicodeBidi, UnicodeBidi)
        if(!primitiveValue) break;
        switch (primitiveValue->getIdent()) {
            case CSS_VAL_NORMAL:
                style->setUnicodeBidi(UBNormal);
                break;
            case CSS_VAL_EMBED:
                style->setUnicodeBidi(Embed);
                break;
            case CSS_VAL_BIDI_OVERRIDE:
                style->setUnicodeBidi(Override);
                break;
            default:
                return;
        }
	break;
    }
    case CSS_PROP_TEXT_TRANSFORM: {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(textTransform, TextTransform)

        if(!primitiveValue) break;
        if(!primitiveValue->getIdent()) return;

        ETextTransform tt;
        switch(primitiveValue->getIdent()) {
        case CSS_VAL_CAPITALIZE:  tt = CAPITALIZE;  break;
        case CSS_VAL_UPPERCASE:   tt = UPPERCASE;   break;
        case CSS_VAL_LOWERCASE:   tt = LOWERCASE;   break;
        case CSS_VAL_NONE:
        default:                  tt = TTNONE;      break;
        }
        style->setTextTransform(tt);
        break;
        }

    case CSS_PROP_VISIBILITY:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(visibility, Visibility)

        if(!primitiveValue) break;
        switch( primitiveValue->getIdent() ) {
        case CSS_VAL_HIDDEN:
            style->setVisibility( HIDDEN );
            break;
        case CSS_VAL_VISIBLE:
            style->setVisibility( VISIBLE );
            break;
        case CSS_VAL_COLLAPSE:
            style->setVisibility( COLLAPSE );
        default:
            break;
        }
        break;
    }
    case CSS_PROP_WHITE_SPACE:
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(whiteSpace, WhiteSpace)

        if(!primitiveValue) break;
        if(!primitiveValue->getIdent()) return;

        EWhiteSpace s;
        switch(primitiveValue->getIdent()) {
        case CSS_VAL__KHTML_NOWRAP:
            s = KHTML_NOWRAP;
            break;
        case CSS_VAL_NOWRAP:
            s = NOWRAP;
            break;
        case CSS_VAL_PRE:
            s = PRE;
            break;
        case CSS_VAL_PRE_WRAP:
            s = PRE_WRAP;
            break;
        case CSS_VAL_PRE_LINE:
            s = PRE_LINE;
            break;
        case CSS_VAL_NORMAL:
        default:
            s = NORMAL;
            break;
        }
        style->setWhiteSpace(s);
        break;

    case CSS_PROP_BACKGROUND_POSITION:
        HANDLE_BACKGROUND_INHERIT_AND_INITIAL(backgroundXPosition, BackgroundXPosition);
        HANDLE_BACKGROUND_INHERIT_AND_INITIAL(backgroundYPosition, BackgroundYPosition);
        break;
    case CSS_PROP_BACKGROUND_POSITION_X: {
        HANDLE_BACKGROUND_VALUE(backgroundXPosition, BackgroundXPosition, value)
        break;
    }
    case CSS_PROP_BACKGROUND_POSITION_Y: {
        HANDLE_BACKGROUND_VALUE(backgroundYPosition, BackgroundYPosition, value)
        break;
    }
    case CSS_PROP_BORDER_SPACING: {
        if(value->cssValueType() != CSSValue::CSS_INHERIT || !parentNode) return;
        style->setBorderHorizontalSpacing(parentStyle->borderHorizontalSpacing());
        style->setBorderVerticalSpacing(parentStyle->borderVerticalSpacing());
        break;
    }
    case CSS_PROP__KHTML_BORDER_HORIZONTAL_SPACING: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(borderHorizontalSpacing, BorderHorizontalSpacing)
        if (!primitiveValue) break;
        short spacing =  primitiveValue->computeLength(style, logicalDpiY);
        style->setBorderHorizontalSpacing(spacing);
        break;
    }
    case CSS_PROP__KHTML_BORDER_VERTICAL_SPACING: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(borderVerticalSpacing, BorderVerticalSpacing)
        if (!primitiveValue) break;
        short spacing =  primitiveValue->computeLength(style, logicalDpiY);
        style->setBorderVerticalSpacing(spacing);
        break;
    }

    // ### should these handle initial & inherit?
    case CSS_PROP__KHTML_BORDER_TOP_RIGHT_RADIUS:
    case CSS_PROP_BORDER_TOP_RIGHT_RADIUS:
        style->setBorderTopRightRadius(convertToBorderRadii(primitiveValue, style, logicalDpiY));
        break;
    case CSS_PROP__KHTML_BORDER_TOP_LEFT_RADIUS:
    case CSS_PROP_BORDER_TOP_LEFT_RADIUS:
        style->setBorderTopLeftRadius(convertToBorderRadii(primitiveValue, style, logicalDpiY));
        break;
    case CSS_PROP__KHTML_BORDER_BOTTOM_RIGHT_RADIUS:
    case CSS_PROP_BORDER_BOTTOM_RIGHT_RADIUS:
        style->setBorderBottomRightRadius(convertToBorderRadii(primitiveValue, style, logicalDpiY));
        break;
    case CSS_PROP__KHTML_BORDER_BOTTOM_LEFT_RADIUS:
    case CSS_PROP_BORDER_BOTTOM_LEFT_RADIUS:
        style->setBorderBottomLeftRadius(convertToBorderRadii(primitiveValue, style, logicalDpiY));
        break;

    case CSS_PROP_CURSOR:
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(cursor, Cursor)
        if (!primitiveValue) break;
	ECursor cursor;
        switch(primitiveValue->getIdent()) {
        case CSS_VAL_NONE:
	    cursor = CURSOR_NONE;
            break;
	default:
	    cursor = (ECursor) (primitiveValue->getIdent() - CSS_VAL_AUTO);
	}
	style->setCursor(cursor);
        break;
// colors || inherit
    case CSS_PROP_COLOR:
        // If the 'currentColor' keyword is set on the 'color' property itself,
        // it is treated as 'color:inherit' at parse time
        if (primitiveValue && primitiveValue->getIdent() == CSS_VAL_CURRENTCOLOR)
            isInherit = true;      
    case CSS_PROP_BACKGROUND_COLOR:
    case CSS_PROP_BORDER_TOP_COLOR:
    case CSS_PROP_BORDER_RIGHT_COLOR:
    case CSS_PROP_BORDER_BOTTOM_COLOR:
    case CSS_PROP_BORDER_LEFT_COLOR:
    case CSS_PROP_OUTLINE_COLOR:
        // this property is an extension used to get HTML4 <font> right.
    case CSS_PROP_SCROLLBAR_BASE_COLOR:
    case CSS_PROP_SCROLLBAR_FACE_COLOR:
    case CSS_PROP_SCROLLBAR_SHADOW_COLOR:
    case CSS_PROP_SCROLLBAR_HIGHLIGHT_COLOR:
    case CSS_PROP_SCROLLBAR_3DLIGHT_COLOR:
    case CSS_PROP_SCROLLBAR_DARKSHADOW_COLOR:
    case CSS_PROP_SCROLLBAR_TRACK_COLOR:
    case CSS_PROP_SCROLLBAR_ARROW_COLOR:
    {
        QColor col;
        if (isInherit) {
            if (id != CSS_PROP_COLOR)
                style->setInheritedNoninherited(true);
            HANDLE_INHERIT_COND(CSS_PROP_BACKGROUND_COLOR, backgroundColor, BackgroundColor)
            HANDLE_INHERIT_COND_WITH_BACKUP(CSS_PROP_BORDER_TOP_COLOR, borderTopColor, color, BorderTopColor)
            HANDLE_INHERIT_COND_WITH_BACKUP(CSS_PROP_BORDER_BOTTOM_COLOR, borderBottomColor, color, BorderBottomColor)
            HANDLE_INHERIT_COND_WITH_BACKUP(CSS_PROP_BORDER_RIGHT_COLOR, borderRightColor, color, BorderRightColor)
            HANDLE_INHERIT_COND_WITH_BACKUP(CSS_PROP_BORDER_LEFT_COLOR, borderLeftColor, color, BorderLeftColor)
            HANDLE_INHERIT_COND(CSS_PROP_COLOR, color, Color)
            HANDLE_INHERIT_COND_WITH_BACKUP(CSS_PROP_OUTLINE_COLOR, outlineColor, color, OutlineColor)
            return;
        } else if (isInitial) {
            // The border/outline colors will just map to the invalid color |col| above.  This will have the
            // effect of forcing the use of the currentColor when it comes time to draw the borders (and of
            // not painting the background since the color won't be valid).
            if (id == CSS_PROP_COLOR)
                col = RenderStyle::initialColor();
        } else {
	    if(!primitiveValue )
		return;
	    int ident = primitiveValue->getIdent();
	    if ( ident ) {
		if ( ident == CSS_VAL__KHTML_TEXT )
		    col = element->document()->textColor();
                else if (ident == CSS_VAL_CURRENTCOLOR)
                    col = style->color();
		else
		    col = colorForCSSValue( ident );
	    } else if ( primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_RGBCOLOR ) {
		    col.setRgba(primitiveValue->getRGBColorValue());
	    } else {
		return;
	    }
	}
        //qDebug() << "applying color " << col.isValid();
        switch(id)
        {
        case CSS_PROP_BACKGROUND_COLOR:
	    style->setBackgroundColor(col); break;
        case CSS_PROP_BORDER_TOP_COLOR:
            style->setBorderTopColor(col); break;
        case CSS_PROP_BORDER_RIGHT_COLOR:
            style->setBorderRightColor(col); break;
        case CSS_PROP_BORDER_BOTTOM_COLOR:
            style->setBorderBottomColor(col); break;
        case CSS_PROP_BORDER_LEFT_COLOR:
            style->setBorderLeftColor(col); break;
        case CSS_PROP_COLOR:
            style->setColor(col); break;
        case CSS_PROP_OUTLINE_COLOR:
            style->setOutlineColor(col); break;
#ifndef APPLE_CHANGES
        case CSS_PROP_SCROLLBAR_FACE_COLOR:
            style->setPaletteColor(QPalette::Active, QPalette::Button, col);
            style->setPaletteColor(QPalette::Inactive, QPalette::Button, col);
            break;
        case CSS_PROP_SCROLLBAR_SHADOW_COLOR:
            style->setPaletteColor(QPalette::Active, QPalette::Shadow, col);
            style->setPaletteColor(QPalette::Inactive, QPalette::Shadow, col);
            break;
        case CSS_PROP_SCROLLBAR_HIGHLIGHT_COLOR:
            style->setPaletteColor(QPalette::Active, QPalette::Light, col);
            style->setPaletteColor(QPalette::Inactive, QPalette::Light, col);
            break;
        case CSS_PROP_SCROLLBAR_3DLIGHT_COLOR:
            break;
        case CSS_PROP_SCROLLBAR_DARKSHADOW_COLOR:
            style->setPaletteColor(QPalette::Active, QPalette::Dark, col);
            style->setPaletteColor(QPalette::Inactive, QPalette::Dark, col);
            break;
        case CSS_PROP_SCROLLBAR_TRACK_COLOR:
            style->setPaletteColor(QPalette::Active, QPalette::Mid, col);
            style->setPaletteColor(QPalette::Inactive, QPalette::Mid, col);
            style->setPaletteColor(QPalette::Active, QPalette::Window, col);
            style->setPaletteColor(QPalette::Inactive, QPalette::Window, col);
            // fall through
        case CSS_PROP_SCROLLBAR_BASE_COLOR:
            style->setPaletteColor(QPalette::Active, QPalette::Base, col);
            style->setPaletteColor(QPalette::Inactive, QPalette::Base, col);
            break;
        case CSS_PROP_SCROLLBAR_ARROW_COLOR:
            style->setPaletteColor(QPalette::Active, QPalette::ButtonText, col);
            style->setPaletteColor(QPalette::Inactive, QPalette::ButtonText, col);
            break;
#endif
        default:
            return;
        }
        return;
    }
    break;
// uri || inherit
    case CSS_PROP_BACKGROUND_IMAGE:
        HANDLE_BACKGROUND_VALUE(backgroundImage, BackgroundImage, value)
        break;
    case CSS_PROP_LIST_STYLE_IMAGE:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(listStyleImage, ListStyleImage)
        if (!primitiveValue) return;
	style->setListStyleImage(static_cast<CSSImageValueImpl *>(primitiveValue)->image());
        //qDebug() << "setting image in list to " << image->image();
        break;
    }

// length
    case CSS_PROP_BORDER_TOP_WIDTH:
    case CSS_PROP_BORDER_RIGHT_WIDTH:
    case CSS_PROP_BORDER_BOTTOM_WIDTH:
    case CSS_PROP_BORDER_LEFT_WIDTH:
    case CSS_PROP_OUTLINE_WIDTH:
    {
	if (isInherit) {
	    style->setInheritedNoninherited(true);
            HANDLE_INHERIT_COND(CSS_PROP_BORDER_TOP_WIDTH, borderTopWidth, BorderTopWidth)
            HANDLE_INHERIT_COND(CSS_PROP_BORDER_RIGHT_WIDTH, borderRightWidth, BorderRightWidth)
            HANDLE_INHERIT_COND(CSS_PROP_BORDER_BOTTOM_WIDTH, borderBottomWidth, BorderBottomWidth)
            HANDLE_INHERIT_COND(CSS_PROP_BORDER_LEFT_WIDTH, borderLeftWidth, BorderLeftWidth)
            HANDLE_INHERIT_COND(CSS_PROP_OUTLINE_WIDTH, outlineWidth, OutlineWidth)
            return;
        }
        else if (isInitial) {
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_BORDER_TOP_WIDTH, BorderTopWidth, BorderWidth)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_BORDER_RIGHT_WIDTH, BorderRightWidth, BorderWidth)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_BORDER_BOTTOM_WIDTH, BorderBottomWidth, BorderWidth)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_BORDER_LEFT_WIDTH, BorderLeftWidth, BorderWidth)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_OUTLINE_WIDTH, OutlineWidth, BorderWidth)
            return;
        }

        if(!primitiveValue) break;
        short width = 3;
        switch(primitiveValue->getIdent())
        {
        case CSS_VAL_THIN:
            width = 1;
            break;
        case CSS_VAL_MEDIUM:
            width = 3;
            break;
        case CSS_VAL_THICK:
            width = 5;
            break;
        case CSS_VAL_INVALID:
        {
            double widthd = primitiveValue->computeLengthFloat(style, logicalDpiY);
            width = CSSPrimitiveValueImpl::snapValue(widthd);
            // somewhat resemble Mozilla's granularity
            // this makes border-width: 0.5pt borders visible
            if (width == 0 && widthd >= 0.025) width++;
            break;
        }
        default:
            return;
        }

        if(width < 0) return;
        switch(id)
        {
        case CSS_PROP_BORDER_TOP_WIDTH:
            style->setBorderTopWidth(width);
            break;
        case CSS_PROP_BORDER_RIGHT_WIDTH:
            style->setBorderRightWidth(width);
            break;
        case CSS_PROP_BORDER_BOTTOM_WIDTH:
            style->setBorderBottomWidth(width);
            break;
        case CSS_PROP_BORDER_LEFT_WIDTH:
            style->setBorderLeftWidth(width);
            break;
        case CSS_PROP_OUTLINE_WIDTH:
            style->setOutlineWidth(width);
            break;
        default:
            return;
        }
        return;
    }

    case CSS_PROP_LETTER_SPACING:
    case CSS_PROP_WORD_SPACING:
    {
        if (isInherit) {
            HANDLE_INHERIT_COND(CSS_PROP_LETTER_SPACING, letterSpacing, LetterSpacing)
            HANDLE_INHERIT_COND(CSS_PROP_WORD_SPACING, wordSpacing, WordSpacing)
            return;
        } else if (isInitial) {
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_LETTER_SPACING, LetterSpacing, LetterWordSpacing)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_WORD_SPACING, WordSpacing, LetterWordSpacing)
            return;
        }
        if(!primitiveValue) return;

        int width = 0;
        if (primitiveValue->getIdent() != CSS_VAL_NORMAL)
	    width = primitiveValue->computeLength(style, logicalDpiY);

        switch(id)
        {
        case CSS_PROP_LETTER_SPACING:
            style->setLetterSpacing(width);
            break;
        case CSS_PROP_WORD_SPACING:
            style->setWordSpacing(width);
            break;
            // ### needs the definitions in renderstyle
        default: break;
        }
        return;
    }

        // length, percent
    case CSS_PROP_MAX_WIDTH:
        // +none +inherit
        if(primitiveValue && primitiveValue->getIdent() == CSS_VAL_NONE) {
            apply = true;
            l = Length(UNDEFINED, Fixed);
        }
    case CSS_PROP_TOP:
    case CSS_PROP_LEFT:
    case CSS_PROP_RIGHT:
    case CSS_PROP_BOTTOM:
    case CSS_PROP_WIDTH:
    case CSS_PROP_MIN_WIDTH:
    case CSS_PROP_MARGIN_TOP:
    case CSS_PROP_MARGIN_RIGHT:
    case CSS_PROP_MARGIN_BOTTOM:
    case CSS_PROP_MARGIN_LEFT:
        // +inherit +auto
        if(id != CSS_PROP_MAX_WIDTH && primitiveValue &&
           primitiveValue->getIdent() == CSS_VAL_AUTO)
        {
            //qDebug() << "found value=auto";
            apply = true;
        }
    case CSS_PROP_PADDING_TOP:
    case CSS_PROP_PADDING_RIGHT:
    case CSS_PROP_PADDING_BOTTOM:
    case CSS_PROP_PADDING_LEFT:
    case CSS_PROP_TEXT_INDENT:
        // +inherit
    {
        if (isInherit) {
            if (id != CSS_PROP_TEXT_INDENT)
                style->setInheritedNoninherited(true);
            HANDLE_INHERIT_COND(CSS_PROP_MAX_WIDTH, maxWidth, MaxWidth)
            HANDLE_INHERIT_COND(CSS_PROP_BOTTOM, bottom, Bottom)
            HANDLE_INHERIT_COND(CSS_PROP_TOP, top, Top)
            HANDLE_INHERIT_COND(CSS_PROP_LEFT, left, Left)
            HANDLE_INHERIT_COND(CSS_PROP_RIGHT, right, Right)
            HANDLE_INHERIT_COND(CSS_PROP_WIDTH, width, Width)
            HANDLE_INHERIT_COND(CSS_PROP_MIN_WIDTH, minWidth, MinWidth)
            HANDLE_INHERIT_COND(CSS_PROP_PADDING_TOP, paddingTop, PaddingTop)
            HANDLE_INHERIT_COND(CSS_PROP_PADDING_RIGHT, paddingRight, PaddingRight)
            HANDLE_INHERIT_COND(CSS_PROP_PADDING_BOTTOM, paddingBottom, PaddingBottom)
            HANDLE_INHERIT_COND(CSS_PROP_PADDING_LEFT, paddingLeft, PaddingLeft)
            HANDLE_INHERIT_COND(CSS_PROP_MARGIN_TOP, marginTop, MarginTop)
            HANDLE_INHERIT_COND(CSS_PROP_MARGIN_RIGHT, marginRight, MarginRight)
            HANDLE_INHERIT_COND(CSS_PROP_MARGIN_BOTTOM, marginBottom, MarginBottom)
            HANDLE_INHERIT_COND(CSS_PROP_MARGIN_LEFT, marginLeft, MarginLeft)
            HANDLE_INHERIT_COND(CSS_PROP_TEXT_INDENT, textIndent, TextIndent)
            return;
        } else if (isInitial) {
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_MAX_WIDTH, MaxWidth, MaxSize)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_BOTTOM, Bottom, Offset)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_TOP, Top, Offset)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_LEFT, Left, Offset)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_RIGHT, Right, Offset)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_WIDTH, Width, Size)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_MIN_WIDTH, MinWidth, MinSize)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_PADDING_TOP, PaddingTop, Padding)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_PADDING_RIGHT, PaddingRight, Padding)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_PADDING_BOTTOM, PaddingBottom, Padding)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_PADDING_LEFT, PaddingLeft, Padding)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_MARGIN_TOP, MarginTop, Margin)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_MARGIN_RIGHT, MarginRight, Margin)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_MARGIN_BOTTOM, MarginBottom, Margin)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_MARGIN_LEFT, MarginLeft, Margin)
            HANDLE_INITIAL_COND(CSS_PROP_TEXT_INDENT, TextIndent)
            return;
        }

        if (primitiveValue && !apply) {
            int type = primitiveValue->primitiveType();
            if(type > CSSPrimitiveValue::CSS_PERCENTAGE && type < CSSPrimitiveValue::CSS_DEG)
                // Handle our quirky margin units if we have them.
                l = Length(primitiveValue->computeLength(style, logicalDpiY), Fixed,
                           primitiveValue->isQuirkValue());
            else if(type == CSSPrimitiveValue::CSS_PERCENTAGE)
                l = Length(primitiveValue->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
	    else if (type == CSSPrimitiveValue::CSS_HTML_RELATIVE)
		l = Length(int(primitiveValue->floatValue(CSSPrimitiveValue::CSS_HTML_RELATIVE)), Relative);
            else
                return;
            apply = true;
        }
        if(!apply) return;
        switch(id)
            {
            case CSS_PROP_MAX_WIDTH:
                style->setMaxWidth(l); break;
            case CSS_PROP_BOTTOM:
                style->setBottom(l); break;
            case CSS_PROP_TOP:
                style->setTop(l); break;
            case CSS_PROP_LEFT:
                style->setLeft(l); break;
            case CSS_PROP_RIGHT:
                style->setRight(l); break;
            case CSS_PROP_WIDTH:
                style->setWidth(l); break;
            case CSS_PROP_MIN_WIDTH:
                style->setMinWidth(l); break;
            case CSS_PROP_PADDING_TOP:
                style->setPaddingTop(l); break;
            case CSS_PROP_PADDING_RIGHT:
                style->setPaddingRight(l); break;
            case CSS_PROP_PADDING_BOTTOM:
                style->setPaddingBottom(l); break;
            case CSS_PROP_PADDING_LEFT:
                style->setPaddingLeft(l); break;
            case CSS_PROP_MARGIN_TOP:
                style->setMarginTop(l); break;
            case CSS_PROP_MARGIN_RIGHT:
                style->setMarginRight(l); break;
            case CSS_PROP_MARGIN_BOTTOM:
                style->setMarginBottom(l); break;
            case CSS_PROP_MARGIN_LEFT:
                style->setMarginLeft(l); break;
            case CSS_PROP_TEXT_INDENT:
                style->setTextIndent(l); break;
            default: break;
            }
        return;
    }

    case CSS_PROP_MAX_HEIGHT:
        if(primitiveValue && primitiveValue->getIdent() == CSS_VAL_NONE) {
            apply = true;
            l = Length(UNDEFINED, Fixed);
        }
    case CSS_PROP_HEIGHT:
    case CSS_PROP_MIN_HEIGHT:
        if(id != CSS_PROP_MAX_HEIGHT && primitiveValue &&
           primitiveValue->getIdent() == CSS_VAL_AUTO)
            apply = true;
        if (isInherit) {
	    style->setInheritedNoninherited(true);
            HANDLE_INHERIT_COND(CSS_PROP_MAX_HEIGHT, maxHeight, MaxHeight)
            HANDLE_INHERIT_COND(CSS_PROP_HEIGHT, height, Height)
            HANDLE_INHERIT_COND(CSS_PROP_MIN_HEIGHT, minHeight, MinHeight)
            return;
        }
        else if (isInitial) {
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_MAX_HEIGHT, MaxHeight, MaxSize)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_HEIGHT, Height, Size)
            HANDLE_INITIAL_COND_WITH_VALUE(CSS_PROP_MIN_HEIGHT, MinHeight, MinSize)
            return;
        }

        if (primitiveValue && !apply)
        {
            int type = primitiveValue->primitiveType();
            if(type > CSSPrimitiveValue::CSS_PERCENTAGE && type < CSSPrimitiveValue::CSS_DEG)
                l = Length(primitiveValue->computeLength(style, logicalDpiY), Fixed);
            else if(type == CSSPrimitiveValue::CSS_PERCENTAGE)
                l = Length(primitiveValue->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
            else
                return;
            apply = true;
        }
        if(!apply) return;
        switch(id)
        {
        case CSS_PROP_MAX_HEIGHT:
            style->setMaxHeight(l); break;
        case CSS_PROP_HEIGHT:
            style->setHeight(l); break;
        case CSS_PROP_MIN_HEIGHT:
            style->setMinHeight(l); break;
        default:
            return;
        }
        return;

        break;

    case CSS_PROP_VERTICAL_ALIGN:
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(verticalAlign, VerticalAlign)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent()) {
	  khtml::EVerticalAlign align;

	  switch(primitiveValue->getIdent())
	    {
		case CSS_VAL_TOP:
		    align = TOP; break;
		case CSS_VAL_BOTTOM:
		    align = BOTTOM; break;
		case CSS_VAL_MIDDLE:
		    align = MIDDLE; break;
		case CSS_VAL_BASELINE:
		    align = BASELINE; break;
		case CSS_VAL_TEXT_BOTTOM:
		    align = TEXT_BOTTOM; break;
		case CSS_VAL_TEXT_TOP:
		    align = TEXT_TOP; break;
		case CSS_VAL_SUB:
		    align = SUB; break;
		case CSS_VAL_SUPER:
		    align = SUPER; break;
		case CSS_VAL__KHTML_BASELINE_MIDDLE:
		    align = BASELINE_MIDDLE; break;
		default:
		    return;
	    }
	  style->setVerticalAlign(align);
	  return;
        } else {
	  int type = primitiveValue->primitiveType();
	  Length l;
	  if(type > CSSPrimitiveValue::CSS_PERCENTAGE && type < CSSPrimitiveValue::CSS_DEG)
	    l = Length(primitiveValue->computeLength(style, logicalDpiY), Fixed );
	  else if(type == CSSPrimitiveValue::CSS_PERCENTAGE)
	    l = Length( primitiveValue->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent );

	  style->setVerticalAlign( LENGTH );
	  style->setVerticalAlignLength( l );
	}
        break;

    case CSS_PROP_FONT_SIZE:
    {
        FontDef fontDef = style->htmlFont().fontDef;
        int oldSize;
        float size = 0;

        float toPix = logicalDpiY/72.0f;
        if (toPix  < 96.0f/72.0f)
            toPix = 96.0f/72.0f;

        float minFontSize = settings->minFontSize() * toPix;

        if(parentNode) {
            oldSize = parentStyle->font().pixelSize();
        } else
            oldSize = m_fontSizes[3];

        if (isInherit )
            size = oldSize;
        else if (isInitial)
            size = m_fontSizes[3];
        else if(primitiveValue->getIdent()) {
	    // keywords are being used.  Pick the correct default
	    // based off the font family.
#ifdef APPLE_CHANGES
	    const QVector<int>& fontSizes = (fontDef.genericFamily == FontDef::eMonospace) ?
					 m_fixedFontSizes : m_fontSizes;
#else
	    const QVector<int>& fontSizes = m_fontSizes;
#endif
            switch(primitiveValue->getIdent())
            {
            case CSS_VAL_XX_SMALL: size = fontSizes[0]; break;
            case CSS_VAL_X_SMALL:  size = fontSizes[1]; break;
            case CSS_VAL_SMALL:    size = fontSizes[2]; break;
            case CSS_VAL_MEDIUM:   size = fontSizes[3]; break;
            case CSS_VAL_LARGE:    size = fontSizes[4]; break;
            case CSS_VAL_X_LARGE:  size = fontSizes[5]; break;
            case CSS_VAL_XX_LARGE: size = fontSizes[6]; break;
            case CSS_VAL__KHTML_XXX_LARGE: size = fontSizes[7]; break;
            case CSS_VAL_LARGER:
                size = nextFontSize(fontSizes, oldSize, false);
                break;
            case CSS_VAL_SMALLER:
                size = nextFontSize(fontSizes, oldSize, true);
                break;
            default:
                return;
            }

        } else {
            int type = primitiveValue->primitiveType();
            if(type > CSSPrimitiveValue::CSS_PERCENTAGE && type < CSSPrimitiveValue::CSS_DEG) {
                if ( !khtml::printpainter && type != CSSPrimitiveValue::CSS_EMS && type != CSSPrimitiveValue::CSS_EXS &&
                     view && view->part())
                    size = primitiveValue->computeLengthFloat(parentStyle, logicalDpiY) *
                                view->part()->fontScaleFactor() / 100.0;
		else
                    size = primitiveValue->computeLengthFloat(parentStyle, logicalDpiY);
            }
            else if(type == CSSPrimitiveValue::CSS_PERCENTAGE)
                size = primitiveValue->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE)
                       * parentStyle->font().pixelSize() / 100.0;
            else
                return;
        }

        if (size < 0) return;

        // we never want to get smaller than the minimum font size to keep fonts readable
        // do not however maximize zero as that is commonly used for fancy layouting purposes
        if (size && size < minFontSize) size = minFontSize;

        //qDebug() << "computed raw font size: " << size;

	fontDef.size = qRound(size);
        fontDirty |= style->setFontDef( fontDef );
        return;
    }

    case CSS_PROP_Z_INDEX:
    {
        HANDLE_INHERIT_ON_NONINHERITED_PROPERTY(zIndex, ZIndex)
        else if (isInitial) {
            style->setHasAutoZIndex();
            return;
        }

        if (!primitiveValue)
            return;

        if (primitiveValue->getIdent() == CSS_VAL_AUTO) {
            style->setHasAutoZIndex();
            return;
        }

        if (primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_NUMBER)
            return; // Error case.

        style->setZIndex((int)primitiveValue->floatValue(CSSPrimitiveValue::CSS_NUMBER));
        return;
    }

    case CSS_PROP_WIDOWS:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(widows, Widows)
        if (!primitiveValue || primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_NUMBER)
            return;
        style->setWidows((int)primitiveValue->floatValue(CSSPrimitiveValue::CSS_NUMBER));
        break;
     }

    case CSS_PROP_ORPHANS:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(orphans, Orphans)
        if (!primitiveValue || primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_NUMBER)
            return;
        style->setOrphans((int)primitiveValue->floatValue(CSSPrimitiveValue::CSS_NUMBER));
        break;
    }

// length, percent, number
    case CSS_PROP_LINE_HEIGHT:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(lineHeight, LineHeight)
        if(!primitiveValue) return;
        Length lineHeight;
        int type = primitiveValue->primitiveType();
        if (primitiveValue->getIdent() == CSS_VAL_NORMAL)
            lineHeight = Length( -100.0, Percent );
        else if (type > CSSPrimitiveValue::CSS_PERCENTAGE && type < CSSPrimitiveValue::CSS_DEG) {
            // Scale for the font zoom factor only for types other than "em" and "ex", since those are
            // already based on the font size.
		if ( !khtml::printpainter && type != CSSPrimitiveValue::CSS_EMS && type != CSSPrimitiveValue::CSS_EXS &&
                    view && view->part())
                    lineHeight = Length(primitiveValue->computeLength(style, logicalDpiY) *
                                        view->part()->fontScaleFactor()/100, Fixed );
                else
                    lineHeight = Length(primitiveValue->computeLength(style, logicalDpiY), Fixed );
        } else if (type == CSSPrimitiveValue::CSS_PERCENTAGE)
            lineHeight = Length( ( style->font().pixelSize() * int(primitiveValue->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE)) ) / 100, Fixed );
        else if (type == CSSPrimitiveValue::CSS_NUMBER)
            lineHeight = Length(primitiveValue->floatValue(CSSPrimitiveValue::CSS_NUMBER)*100.0, Percent);
        else
            return;
        style->setLineHeight(lineHeight);
        return;
    }

// string
    case CSS_PROP_TEXT_ALIGN:
    {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(textAlign, TextAlign)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent())
            style->setTextAlign( (ETextAlign) (primitiveValue->getIdent() - CSS_VAL__KHTML_AUTO) );
	return;
    }

// rect
    case CSS_PROP_CLIP:
    {
        Length top = Length();
        Length right = Length();
        Length bottom = Length();
        Length left = Length();

        bool hasClip = false;

        if (isInherit && parentStyle->hasClip()) {
            hasClip = true;
	    top = parentStyle->clipTop();
	    right = parentStyle->clipRight();
	    bottom = parentStyle->clipBottom();
	    left = parentStyle->clipLeft();
        } else if (primitiveValue && primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_RECT) {
            RectImpl *rect = primitiveValue->getRectValue();
            if (rect) {
                hasClip = true;
                // As a convention, we pass in auto as Length(Auto). See RenderBox::clipRect
                top    = rect->top()->getIdent()    == CSS_VAL_AUTO ? Length(Auto)
                            : convertToLength( rect->top(), style, logicalDpiY );

                right  = rect->right()->getIdent()  == CSS_VAL_AUTO ? Length(Auto)
                            : convertToLength( rect->right(), style, logicalDpiY );

                bottom = rect->bottom()->getIdent() == CSS_VAL_AUTO ? Length(Auto)
                            : convertToLength( rect->bottom(), style, logicalDpiY );

                left   = rect->left()->getIdent()   == CSS_VAL_AUTO ? Length(Auto)
                            : convertToLength( rect->left(), style, logicalDpiY );
            }
        }

        style->setClip(top, right, bottom, left);
        style->setHasClip(hasClip);

        // rect, ident
        break;
    }

// lists
    case CSS_PROP_CONTENT:
        // list of string, uri, counter, attr, i
    {
        // FIXME: In CSS3, it will be possible to inherit content.  In CSS2 it is not.  This
        // note is a reminder that eventually "inherit" needs to be supported.

        // not allowed on non-generated pseudo-elements:
        if ( style->styleType()==RenderStyle::FIRST_LETTER ||
             style->styleType()==RenderStyle::FIRST_LINE ||
             style->styleType()==RenderStyle::SELECTION )
            break;

        if (isInitial) {
            style->setContentNormal();
            return;
        }

        if (primitiveValue && primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_IDENT) {
            // normal | none
            if (primitiveValue->getIdent() == CSS_VAL_NORMAL)
                style->setContentNormal();
            else
            if (primitiveValue->getIdent() == CSS_VAL_NONE)
                style->setContentNone();
            else
                assert(false);
            return;
        }

        if(!value->isValueList()) return;
        CSSValueListImpl *list = static_cast<CSSValueListImpl *>(value);
        int len = list->length();

        style->setContentNormal(); // clear the content

        for(int i = 0; i < len; i++) {
            CSSValueImpl *item = list->item(i);
            if(!item->isPrimitiveValue()) continue;
            CSSPrimitiveValueImpl *val = static_cast<CSSPrimitiveValueImpl *>(item);
            if(val->primitiveType()==CSSPrimitiveValue::CSS_STRING)
            {
                style->addContent(val->getStringValue());
            }
            else if (val->primitiveType()==CSSPrimitiveValue::CSS_ATTR)
            {
                // TODO: setup dynamic attribute dependencies
                LocalName attrName;
                if (element->htmlCompat())
                    attrName = LocalName::fromString(DOMString(val->getStringValue()).lower());
                else
                    attrName = LocalName::fromString(val->getStringValue());
                if (attrName.id())
                    style->addContent(element->getAttribute(makeId(emptyNamespace, attrName.id())).implementation());
                else {
                    // qDebug() << "Attribute \"" << val->getStringValue() << "\" not found";
                }
            }
            else if (val->primitiveType()==CSSPrimitiveValue::CSS_URI)
            {
                CSSImageValueImpl *image = static_cast<CSSImageValueImpl *>(val);
                style->addContent(image->image());
            }
            else if (val->primitiveType()==CSSPrimitiveValue::CSS_COUNTER)
            {
                style->addContent(val->getCounterValue());
            }
            else if (val->primitiveType()==CSSPrimitiveValue::CSS_IDENT)
            {
                EQuoteContent quote;
                switch (val->getIdent()) {
                    case CSS_VAL_OPEN_QUOTE:
                        quote = OPEN_QUOTE;
                        break;
                    case CSS_VAL_NO_OPEN_QUOTE:
                        quote = NO_OPEN_QUOTE;
                        break;
                    case CSS_VAL_CLOSE_QUOTE:
                        quote = CLOSE_QUOTE;
                        break;
                    case CSS_VAL_NO_CLOSE_QUOTE:
                        quote = NO_CLOSE_QUOTE;
                        break;
                    default:
                        assert(false);
                        quote = NO_QUOTE;
                }
                style->addContent(quote);
            } else {
                // qDebug() << "Unrecognized CSS content";
            }
        }
        break;
    }

    case CSS_PROP_COUNTER_INCREMENT: {
        if(!value->isValueList()) return;

        CSSValueListImpl *list = static_cast<CSSValueListImpl *>(value);
        style->setCounterIncrement(list);
        break;
    }
    case CSS_PROP_COUNTER_RESET: {
        if(!value->isValueList()) return;

        CSSValueListImpl *list = static_cast<CSSValueListImpl *>(value);
        style->setCounterReset(list);
        break;
    }
    case CSS_PROP_FONT_FAMILY:
        // list of strings and ids
    {
        if (isInherit) {
            FontDef parentFontDef = parentStyle->htmlFont().fontDef;
            FontDef fontDef = style->htmlFont().fontDef;
            fontDef.family = parentFontDef.family;
            if (style->setFontDef(fontDef))
                fontDirty = true;
            return;
        }
        else if (isInitial) {
            FontDef fontDef = style->htmlFont().fontDef;
            FontDef initialDef = FontDef();
#ifdef APPLE_CHANGES
            fontDef.family = initialDef.firstFamily();
#else
            fontDef.family.clear();
#endif
            if (style->setFontDef(fontDef))
                fontDirty = true;
            return;
        }
        if(!value->isValueList()) return;
	FontDef fontDef = style->htmlFont().fontDef;
        CSSValueListImpl *list = static_cast<CSSValueListImpl *>(value);
        int len = list->length();
        bool hasFamilyName = false;
        for(int i = 0; i < len; i++) {
            CSSValueImpl *item = list->item(i);
            if(!item->isPrimitiveValue()) continue;
            CSSPrimitiveValueImpl *val = static_cast<CSSPrimitiveValueImpl *>(item);
	    QString face;
            if( val->primitiveType() == CSSPrimitiveValue::CSS_STRING ) {
		face = static_cast<FontFamilyValueImpl *>(val)->fontName();
		m_fontSelector->requestFamilyName( DOMString(face) );
	    } else if ( val->primitiveType() == CSSPrimitiveValue::CSS_IDENT ) {
		switch( val->getIdent() ) {
		case CSS_VAL_SERIF:
		    face = settings->serifFontName();
		    break;
		case CSS_VAL_SANS_SERIF:
		    face = settings->sansSerifFontName();
		    break;
		case CSS_VAL_CURSIVE:
		    face = settings->cursiveFontName();
		    break;
		case CSS_VAL_FANTASY:
		    face = settings->fantasyFontName();
		    break;
		case CSS_VAL_MONOSPACE:
		    face = settings->fixedFontName();
		    break;
		default:
		    return;
		}
	    } else {
		return;
	    }
	    if ( !face.isEmpty() ) {
	        if (!hasFamilyName) {
	            fontDef.family = face;
	            hasFamilyName = true;
                } else {
	            fontDef.family += ",";
		    fontDef.family += face;
                }
		fontDirty |= style->setFontDef( fontDef );
	    }
	}
        break;
    }
    case CSS_PROP_QUOTES:
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(quotes, Quotes)
        if(primitiveValue && primitiveValue->getIdent() == CSS_VAL_NONE) {
            // set a set of empty quotes
            QuotesValueImpl* quotes = new QuotesValueImpl();
            style->setQuotes(quotes);
        } else {
            QuotesValueImpl* quotes = static_cast<QuotesValueImpl *>(value);
            style->setQuotes(quotes);
        }
        break;
    case CSS_PROP_SIZE:
        // ### look up
      break;
    case CSS_PROP_TEXT_DECORATION: {
        // list of ident
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(textDecoration, TextDecoration)
        int t = RenderStyle::initialTextDecoration();
        if(primitiveValue && primitiveValue->getIdent() == CSS_VAL_NONE) {
	    // do nothing
	} else {
	    if(!value->isValueList()) return;
	    CSSValueListImpl *list = static_cast<CSSValueListImpl *>(value);
	    int len = list->length();
	    for(int i = 0; i < len; i++)
	    {
		CSSValueImpl *item = list->item(i);
		if(!item->isPrimitiveValue()) continue;
		primitiveValue = static_cast<CSSPrimitiveValueImpl *>(item);
		switch(primitiveValue->getIdent())
		{
		    case CSS_VAL_NONE:
			t = TDNONE; break;
		    case CSS_VAL_UNDERLINE:
			t |= UNDERLINE; break;
		    case CSS_VAL_OVERLINE:
			t |= OVERLINE; break;
		    case CSS_VAL_LINE_THROUGH:
			t |= LINE_THROUGH; break;
		    case CSS_VAL_BLINK:
			t |= BLINK; break;
		    default:
			return;
		}
	    }
        }
	style->setTextDecoration(t);
        break;
    }
    case CSS_PROP__KHTML_FLOW_MODE:
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(flowAroundFloats, FlowAroundFloats)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent()) {
            style->setFlowAroundFloats( primitiveValue->getIdent() == CSS_VAL__KHTML_AROUND_FLOATS );
            return;
        }
        break;
    case CSS_PROP__KHTML_USER_INPUT: {
        if(value->cssValueType() == CSSValue::CSS_INHERIT)
        {
            if(!parentNode) return;
            style->setUserInput(parentStyle->userInput());
//	    qDebug() << "UI erm";
            return;
        }
        if(!primitiveValue) return;
        int id = primitiveValue->getIdent();
	if (id == CSS_VAL_NONE)
	    style->setUserInput(UI_NONE);
	else
	    style->setUserInput(EUserInput(id - CSS_VAL_ENABLED));
//	qDebug() << "userInput: " << style->userEdit();
	return;
    }

// shorthand properties
    case CSS_PROP_BACKGROUND:
        if (isInitial) {
            style->setBackgroundColor(QColor());
            return;
        }
        else if (isInherit) {
            style->inheritBackgroundLayers(*parentStyle->backgroundLayers());
            style->setBackgroundColor(parentStyle->backgroundColor());
        }
        break;
    case CSS_PROP_BORDER:
    case CSS_PROP_BORDER_STYLE:
    case CSS_PROP_BORDER_WIDTH:
    case CSS_PROP_BORDER_COLOR:
        if (isInherit)
            style->setInheritedNoninherited(true);
        if(id == CSS_PROP_BORDER || id == CSS_PROP_BORDER_COLOR)
        {
            if (isInherit) {
                style->setBorderTopColor(inheritedBorderColor(parentStyle, parentStyle->borderTopColor()));
                style->setBorderBottomColor(inheritedBorderColor(parentStyle, parentStyle->borderBottomColor()));
                style->setBorderLeftColor(inheritedBorderColor(parentStyle, parentStyle->borderLeftColor()));
                style->setBorderRightColor(inheritedBorderColor(parentStyle, parentStyle->borderRightColor()));
            }
            else if (isInitial) {
                style->setBorderTopColor(QColor()); // Reset to invalid color so currentColor is used instead.
                style->setBorderBottomColor(QColor());
                style->setBorderLeftColor(QColor());
                style->setBorderRightColor(QColor());
            }
        }
        if (id == CSS_PROP_BORDER || id == CSS_PROP_BORDER_STYLE)
        {
            if (isInherit) {
                style->setBorderTopStyle(parentStyle->borderTopStyle());
                style->setBorderBottomStyle(parentStyle->borderBottomStyle());
                style->setBorderLeftStyle(parentStyle->borderLeftStyle());
                style->setBorderRightStyle(parentStyle->borderRightStyle());
            }
            else if (isInitial) {
                style->setBorderTopStyle(RenderStyle::initialBorderStyle());
                style->setBorderBottomStyle(RenderStyle::initialBorderStyle());
                style->setBorderLeftStyle(RenderStyle::initialBorderStyle());
                style->setBorderRightStyle(RenderStyle::initialBorderStyle());
            }
        }
        if (id == CSS_PROP_BORDER || id == CSS_PROP_BORDER_WIDTH)
        {
            if (isInherit) {
                style->setBorderTopWidth(parentStyle->borderTopWidth());
                style->setBorderBottomWidth(parentStyle->borderBottomWidth());
                style->setBorderLeftWidth(parentStyle->borderLeftWidth());
                style->setBorderRightWidth(parentStyle->borderRightWidth());
            }
            else if (isInitial) {
                style->setBorderTopWidth(RenderStyle::initialBorderWidth());
                style->setBorderBottomWidth(RenderStyle::initialBorderWidth());
                style->setBorderLeftWidth(RenderStyle::initialBorderWidth());
                style->setBorderRightWidth(RenderStyle::initialBorderWidth());
            }
        }
        return;
    case CSS_PROP_BORDER_TOP:
        if ( isInherit ) {
            style->setInheritedNoninherited(true);
            style->setBorderTopColor(inheritedBorderColor(parentStyle, parentStyle->borderTopColor()));
            style->setBorderTopStyle(parentStyle->borderTopStyle());
            style->setBorderTopWidth(parentStyle->borderTopWidth());
        } else if (isInitial)
            style->resetBorderTop();
        return;
    case CSS_PROP_BORDER_RIGHT:
        if (isInherit) {
            style->setInheritedNoninherited(true);
            style->setBorderRightColor(inheritedBorderColor(parentStyle,parentStyle->borderRightColor()));
            style->setBorderRightStyle(parentStyle->borderRightStyle());
            style->setBorderRightWidth(parentStyle->borderRightWidth());
        }
        else if (isInitial)
            style->resetBorderRight();
        return;
    case CSS_PROP_BORDER_BOTTOM:
        if (isInherit) {
            style->setInheritedNoninherited(true);
            style->setBorderBottomColor(inheritedBorderColor(parentStyle, parentStyle->borderBottomColor()));
            style->setBorderBottomStyle(parentStyle->borderBottomStyle());
            style->setBorderBottomWidth(parentStyle->borderBottomWidth());
        }
        else if (isInitial)
            style->resetBorderBottom();
        return;
    case CSS_PROP_BORDER_LEFT:
        if (isInherit) {
            style->setInheritedNoninherited(true);
            style->setBorderLeftColor(inheritedBorderColor(parentStyle, parentStyle->borderLeftColor()));
            style->setBorderLeftStyle(parentStyle->borderLeftStyle());
            style->setBorderLeftWidth(parentStyle->borderLeftWidth());
        }
        else if (isInitial)
            style->resetBorderLeft();
        return;
    case CSS_PROP_MARGIN:
        if (isInherit) {
            style->setInheritedNoninherited(true);
            style->setMarginTop(parentStyle->marginTop());
            style->setMarginBottom(parentStyle->marginBottom());
            style->setMarginLeft(parentStyle->marginLeft());
            style->setMarginRight(parentStyle->marginRight());
        }
        else if (isInitial)
            style->resetMargin();
        return;
    case CSS_PROP_PADDING:
        if (isInherit) {
            style->setInheritedNoninherited(true);
            style->setPaddingTop(parentStyle->paddingTop());
            style->setPaddingBottom(parentStyle->paddingBottom());
            style->setPaddingLeft(parentStyle->paddingLeft());
            style->setPaddingRight(parentStyle->paddingRight());
        }
        else if (isInitial)
            style->resetPadding();
        return;
    case CSS_PROP_FONT:
        if ( isInherit ) {
            FontDef fontDef = parentStyle->htmlFont().fontDef;
	    style->setLineHeight( parentStyle->lineHeight() );
	    fontDirty |= style->setFontDef( fontDef );
        } else if (isInitial) {
            FontDef fontDef;
            style->setLineHeight(RenderStyle::initialLineHeight());
            if (style->setFontDef( fontDef ))
                fontDirty = true;
	} else if ( value->isFontValue() ) {
	    FontValueImpl *font = static_cast<FontValueImpl *>(value);
	    if ( !font->style || !font->variant || !font->weight ||
		 !font->size || !font->lineHeight || !font->family )
		return;
	    applyRule( CSS_PROP_FONT_STYLE, font->style );
	    applyRule( CSS_PROP_FONT_VARIANT, font->variant );
	    applyRule( CSS_PROP_FONT_WEIGHT, font->weight );
	    applyRule( CSS_PROP_FONT_SIZE, font->size );

            // Line-height can depend on font().pixelSize(), so we have to update the font
            // before we evaluate line-height, e.g., font: 1em/1em.  FIXME: Still not
            // good enough: style="font:1em/1em; font-size:36px" should have a line-height of 36px.
            if (fontDirty)
                CSSStyleSelector::style->htmlFont().update( logicalDpiY );

	    applyRule( CSS_PROP_LINE_HEIGHT, font->lineHeight );
	    applyRule( CSS_PROP_FONT_FAMILY, font->family );
	} else if (primitiveValue) {
            // Handle system fonts. We extract out properties from a QFont
            // into the RenderStyle. 
            QFont f;
            switch (primitiveValue->getIdent()) {
            case CSS_VAL_ICON:
                f = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
                break;

            case CSS_VAL_MENU:
                f = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
                break;

            case CSS_VAL_SMALL_CAPTION:
                f = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
                break;

            case CSS_VAL_CAPTION:
            case CSS_VAL_MESSAGE_BOX:
            case CSS_VAL_STATUS_BAR:
                f = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
                break;
            default:
                return;
            }

            FontDef fontDef = style->htmlFont().fontDef;
            fontDef.family    = f.family();
            fontDef.italic    = f.italic();
            fontDef.smallCaps = (f.capitalization() == QFont::SmallCaps);
            fontDef.weight    = f.weight();
            fontDirty |= style->setFontDef( fontDef );

            // Use applyRule to apply height, so it can convert
            // point sizes, and cap pixel sizes appropriately.
            if (f.pixelSize() != -1) {
                CSSPrimitiveValueImpl size( f.pixelSize(), CSSPrimitiveValue::CSS_PX );
                applyRule( CSS_PROP_FONT_SIZE, &size );
            } else {
                CSSPrimitiveValueImpl size( f.pointSize(), CSSPrimitiveValue::CSS_PT );
                applyRule( CSS_PROP_FONT_SIZE, &size );
            }

            // line-height just gets default.
            style->setLineHeight(RenderStyle::initialLineHeight());            
	}
	return;

    case CSS_PROP_LIST_STYLE:
        if (isInherit) {
            style->setListStyleType(parentStyle->listStyleType());
            style->setListStyleImage(parentStyle->listStyleImage());
            style->setListStylePosition(parentStyle->listStylePosition());
        }
        else if (isInitial) {
            style->setListStyleType(RenderStyle::initialListStyleType());
            style->setListStyleImage(RenderStyle::initialListStyleImage());
            style->setListStylePosition(RenderStyle::initialListStylePosition());
        }
        break;
    case CSS_PROP_OUTLINE:
        if (isInherit) {
            style->setOutlineWidth(parentStyle->outlineWidth());
            style->setOutlineColor(parentStyle->outlineColor());
            style->setOutlineStyle(parentStyle->outlineStyle());
        }
        else if (isInitial)
            style->resetOutline();
        break;
    /* CSS3 properties */
    case CSS_PROP_BOX_SIZING:
        HANDLE_INHERIT_ON_NONINHERITED_PROPERTY(boxSizing, BoxSizing)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent() == CSS_VAL_CONTENT_BOX)
            style->setBoxSizing(CONTENT_BOX);
        else
        if (primitiveValue->getIdent() == CSS_VAL_BORDER_BOX)
            style->setBoxSizing(BORDER_BOX);
        break;
    case CSS_PROP_OUTLINE_OFFSET: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(outlineOffset, OutlineOffset)

        int offset = primitiveValue->computeLength(style, logicalDpiY);
        if (offset < 0) return;

        style->setOutlineOffset(offset);
        break;
    }
    case CSS_PROP_TEXT_SHADOW: {
        if (isInherit) {
            style->setTextShadow(parentStyle->textShadow() ? new ShadowData(*parentStyle->textShadow()) : 0);
            return;
        }
        else if (isInitial) {
            style->setTextShadow(0);
            return;
        }

        if (primitiveValue) { // none
            style->setTextShadow(0);
            return;
        }

        if (!value->isValueList()) return;
        CSSValueListImpl *list = static_cast<CSSValueListImpl *>(value);
        int len = list->length();
        for (int i = 0; i < len; i++) {
            ShadowValueImpl *item = static_cast<ShadowValueImpl*>(list->item(i));

            int x = item->x->computeLength(style, logicalDpiY);
            int y = item->y->computeLength(style, logicalDpiY);
            int blur = item->blur ? item->blur->computeLength(style, logicalDpiY) : 0;
            QColor col = khtml::transparentColor;
            if (item->color) {
                int ident = item->color->getIdent();
                if (ident)
                    col = colorForCSSValue( ident );
                else if (item->color->primitiveType() == CSSPrimitiveValue::CSS_RGBCOLOR)
                    col.setRgb(item->color->getRGBColorValue());
            }
            ShadowData* shadowData = new ShadowData(x, y, blur, col);
            style->setTextShadow(shadowData, i != 0);
        }

        break;
    }
    case CSS_PROP_OPACITY:
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(opacity, Opacity)
        if (!primitiveValue || primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_NUMBER)
            return; // Error case.

        // Clamp opacity to the range 0-1
        style->setOpacity(qMin(1.0f, qMax(0.0f, (float)primitiveValue->floatValue(CSSPrimitiveValue::CSS_NUMBER))));
        break;
    case CSS_PROP__KHTML_MARQUEE:
        if (value->cssValueType() != CSSValue::CSS_INHERIT || !parentNode) return;
        style->setMarqueeDirection(parentStyle->marqueeDirection());
        style->setMarqueeIncrement(parentStyle->marqueeIncrement());
        style->setMarqueeSpeed(parentStyle->marqueeSpeed());
        style->setMarqueeLoopCount(parentStyle->marqueeLoopCount());
        style->setMarqueeBehavior(parentStyle->marqueeBehavior());
        break;
    case CSS_PROP__KHTML_MARQUEE_REPETITION: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(marqueeLoopCount, MarqueeLoopCount)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent() == CSS_VAL_INFINITE)
            style->setMarqueeLoopCount(-1); // -1 means repeat forever.
        else if (primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_NUMBER)
            style->setMarqueeLoopCount((int)(primitiveValue->floatValue(CSSPrimitiveValue::CSS_NUMBER)));
        break;
    }
    case CSS_PROP__KHTML_MARQUEE_SPEED: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(marqueeSpeed, MarqueeSpeed)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent()) {
            switch (primitiveValue->getIdent())
            {
                case CSS_VAL_SLOW:
                    style->setMarqueeSpeed(500); // 500 msec.
                    break;
                case CSS_VAL_NORMAL:
                    style->setMarqueeSpeed(85); // 85msec. The WinIE default.
                    break;
                case CSS_VAL_FAST:
                    style->setMarqueeSpeed(10); // 10msec. Super fast.
                    break;
            }
        }
        else if (primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_S)
            style->setMarqueeSpeed(int(1000*primitiveValue->floatValue(CSSPrimitiveValue::CSS_S)));
        else if (primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_MS)
            style->setMarqueeSpeed(int(primitiveValue->floatValue(CSSPrimitiveValue::CSS_MS)));
        else if (primitiveValue->primitiveType() == CSSPrimitiveValue::CSS_NUMBER) // For scrollamount support.
            style->setMarqueeSpeed(int(primitiveValue->floatValue(CSSPrimitiveValue::CSS_NUMBER)));
        break;
    }
    case CSS_PROP__KHTML_MARQUEE_INCREMENT: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(marqueeIncrement, MarqueeIncrement)
        if (!primitiveValue) return;
        if (primitiveValue->getIdent()) {
            switch (primitiveValue->getIdent())
            {
                case CSS_VAL_SMALL:
                    style->setMarqueeIncrement(Length(1, Fixed)); // 1px.
                    break;
                case CSS_VAL_NORMAL:
                    style->setMarqueeIncrement(Length(6, Fixed)); // 6px. The WinIE default.
                    break;
                case CSS_VAL_LARGE:
                    style->setMarqueeIncrement(Length(36, Fixed)); // 36px.
                    break;
            }
        }
        else {
            bool ok = true;
            Length l = convertToLength(primitiveValue, style, logicalDpiY, &ok);
            if (ok)
                style->setMarqueeIncrement(l);
        }
        break;
    }
    case CSS_PROP__KHTML_MARQUEE_STYLE: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(marqueeBehavior, MarqueeBehavior)
        if (!primitiveValue || !primitiveValue->getIdent()) return;
        switch (primitiveValue->getIdent())
        {
            case CSS_VAL_NONE:
                style->setMarqueeBehavior(MNONE);
                break;
            case CSS_VAL_SCROLL:
                style->setMarqueeBehavior(MSCROLL);
                break;
            case CSS_VAL_SLIDE:
                style->setMarqueeBehavior(MSLIDE);
                break;
            case CSS_VAL_ALTERNATE:
                style->setMarqueeBehavior(MALTERNATE);
                break;
            case CSS_VAL_UNFURL:
                style->setMarqueeBehavior(MUNFURL);
                break;
        }
        break;
    }
    case CSS_PROP__KHTML_MARQUEE_DIRECTION: {
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(marqueeDirection, MarqueeDirection)
        if (!primitiveValue || !primitiveValue->getIdent()) return;
        switch (primitiveValue->getIdent())
        {
            case CSS_VAL_FORWARDS:
                style->setMarqueeDirection(MFORWARD);
                break;
            case CSS_VAL_BACKWARDS:
                style->setMarqueeDirection(MBACKWARD);
                break;
            case CSS_VAL_AUTO:
                style->setMarqueeDirection(MAUTO);
                break;
            case CSS_VAL_AHEAD:
            case CSS_VAL_UP: // We don't support vertical languages, so AHEAD just maps to UP.
                style->setMarqueeDirection(MUP);
                break;
            case CSS_VAL_REVERSE:
            case CSS_VAL_DOWN: // REVERSE just maps to DOWN, since we don't do vertical text.
                style->setMarqueeDirection(MDOWN);
                break;
            case CSS_VAL_LEFT:
                style->setMarqueeDirection(MLEFT);
                break;
            case CSS_VAL_RIGHT:
                style->setMarqueeDirection(MRIGHT);
                break;
        }
        break;
    case CSS_PROP_TEXT_OVERFLOW: {
        // This property is supported by WinIE, and so we leave off the "-khtml-" in order to
        // work with WinIE-specific pages that use the property.
        HANDLE_INITIAL_AND_INHERIT_ON_NONINHERITED_PROPERTY(textOverflow, TextOverflow)
        if (!primitiveValue || !primitiveValue->getIdent())
            return;
        style->setTextOverflow(primitiveValue->getIdent() == CSS_VAL_ELLIPSIS);
        break;
    }
    }
    case CSS_PROP_WORD_WRAP: {
        HANDLE_INITIAL_AND_INHERIT_ON_INHERITED_PROPERTY(wordWrap, WordWrap)
        if (!primitiveValue) return;
        style->setWordWrap(primitiveValue->getIdent() == CSS_VAL_NORMAL ? WWNORMAL : WWBREAKWORD);
        break;
    }
    default:
        applySVGRule(id, value);
        return;
    }
}

void CSSStyleSelector::mapBackgroundAttachment(BackgroundLayer* layer, DOM::CSSValueImpl* value)
{
    if (value->cssValueType() == CSSValue::CSS_INITIAL) {
        layer->setBackgroundAttachment(RenderStyle::initialBackgroundAttachment());
        return;
    }

    if (!value->isPrimitiveValue()) return;
    CSSPrimitiveValueImpl* primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);
    switch (primitiveValue->getIdent()) {
        case CSS_VAL_SCROLL:
            layer->setBackgroundAttachment(BGASCROLL);
            break;
        case CSS_VAL_FIXED:
            layer->setBackgroundAttachment(BGAFIXED);
            break;
        case CSS_VAL_LOCAL:
            layer->setBackgroundAttachment(BGALOCAL);
            break;
        default:
            return;
    }
}

void CSSStyleSelector::mapBackgroundClip(BackgroundLayer* layer, CSSValueImpl* value)
{
    if (value->cssValueType() == CSSValue::CSS_INITIAL) {
        layer->setBackgroundClip(RenderStyle::initialBackgroundClip());
        return;
    }

    if (!value->isPrimitiveValue()) return;
    CSSPrimitiveValueImpl* primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);
    switch (primitiveValue->getIdent()) {
        case CSS_VAL_BORDER:
        case CSS_VAL_BORDER_BOX:
            layer->setBackgroundClip(BGBORDER);
            break;
        case CSS_VAL_PADDING:
        case CSS_VAL_PADDING_BOX:
            layer->setBackgroundClip(BGPADDING);
            break;
        default: // CSS_VAL_CONTENT
            layer->setBackgroundClip(BGCONTENT);
            break;
    }
}

void CSSStyleSelector::mapBackgroundOrigin(BackgroundLayer* layer, CSSValueImpl* value)
{
    if (value->cssValueType() == CSSValue::CSS_INITIAL) {
        layer->setBackgroundOrigin(RenderStyle::initialBackgroundOrigin());
        return;
    }

    if (!value->isPrimitiveValue()) return;
    CSSPrimitiveValueImpl* primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);
    switch (primitiveValue->getIdent()) {
        case CSS_VAL_BORDER:
        case CSS_VAL_BORDER_BOX:
            layer->setBackgroundOrigin(BGBORDER);
            break;
        case CSS_VAL_PADDING:
        case CSS_VAL_PADDING_BOX:
            layer->setBackgroundOrigin(BGPADDING);
            break;
        default: // CSS_VAL_CONTENT
            layer->setBackgroundOrigin(BGCONTENT);
            break;
    }
}

void CSSStyleSelector::mapBackgroundImage(BackgroundLayer* layer, DOM::CSSValueImpl* value)
{
    if (value->cssValueType() == CSSValue::CSS_INITIAL) {
        layer->setBackgroundImage(RenderStyle::initialBackgroundImage());
        return;
    }

    if (!value->isPrimitiveValue()) return;
    CSSPrimitiveValueImpl* primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);
    layer->setBackgroundImage(static_cast<CSSImageValueImpl *>(primitiveValue)->image());
}

void CSSStyleSelector::mapBackgroundRepeat(BackgroundLayer* layer, DOM::CSSValueImpl* value)
{
    if (value->cssValueType() == CSSValue::CSS_INITIAL) {
        layer->setBackgroundRepeat(RenderStyle::initialBackgroundRepeat());
        return;
    }

    if (!value->isPrimitiveValue()) return;
    CSSPrimitiveValueImpl* primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);
    switch(primitiveValue->getIdent()) {
	case CSS_VAL_REPEAT:
	    layer->setBackgroundRepeat(REPEAT);
	    break;
	case CSS_VAL_REPEAT_X:
	    layer->setBackgroundRepeat(REPEAT_X);
	    break;
	case CSS_VAL_REPEAT_Y:
	    layer->setBackgroundRepeat(REPEAT_Y);
	    break;
	case CSS_VAL_NO_REPEAT:
	    layer->setBackgroundRepeat(NO_REPEAT);
	    break;
	default:
	    return;
    }
}


void CSSStyleSelector::mapBackgroundSize(BackgroundLayer* layer, CSSValueImpl* value)
{
    LengthSize b = RenderStyle::initialBackgroundSize();

    if (value->cssValueType() == CSSValue::CSS_INITIAL) {
        layer->setBackgroundSize(b);
        return;
    }

    if (!value->isPrimitiveValue())
        return;

    CSSPrimitiveValueImpl* primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);
    int id = primitiveValue->getIdent();
    if (id == CSS_VAL_CONTAIN || id == CSS_VAL_COVER) {
        layer->setBackgroundSizeType( (id ==  CSS_VAL_CONTAIN) ? BGSCONTAIN : BGSCOVER ); 
        return;
    }

    PairImpl* pair = primitiveValue->getPairValue();
    if (!pair)
        return;

    CSSPrimitiveValueImpl* first = static_cast<CSSPrimitiveValueImpl*>(pair->first());
    CSSPrimitiveValueImpl* second = static_cast<CSSPrimitiveValueImpl*>(pair->second());

    if (!first || !second)
        return;

    Length firstLength, secondLength;
    int firstType = first->primitiveType();
    int secondType = second->primitiveType();

    if (firstType == CSSPrimitiveValue::CSS_UNKNOWN)
        firstLength = Length(Auto);
    else if (firstType > CSSPrimitiveValue::CSS_PERCENTAGE && firstType < CSSPrimitiveValue::CSS_DEG)
        firstLength = Length(first->computeLength(style, logicalDpiY), Fixed);
    else if (firstType == CSSPrimitiveValue::CSS_PERCENTAGE)
        firstLength = Length(first->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else
        return;

    if (secondType == CSSPrimitiveValue::CSS_UNKNOWN)
        secondLength = Length(Auto);
    else if (secondType > CSSPrimitiveValue::CSS_PERCENTAGE && secondType < CSSPrimitiveValue::CSS_DEG)
        secondLength = Length(second->computeLength(style, logicalDpiY), Fixed);
    else if (secondType == CSSPrimitiveValue::CSS_PERCENTAGE)
        secondLength = Length(second->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else
        return;

    b.width = firstLength;
    b.height = secondLength;
    layer->setBackgroundSize(b);
}

void CSSStyleSelector::mapBackgroundXPosition(BackgroundLayer* layer, DOM::CSSValueImpl* value)
{
    if (value->cssValueType() == CSSValue::CSS_INITIAL) {
        layer->setBackgroundXPosition(RenderStyle::initialBackgroundXPosition());
        return;
    }

    if (!value->isPrimitiveValue()) return;
    CSSPrimitiveValueImpl* primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);
    Length l;
    int type = primitiveValue->primitiveType();
    if(type > CSSPrimitiveValue::CSS_PERCENTAGE && type < CSSPrimitiveValue::CSS_DEG)
        l = Length(primitiveValue->computeLength(style, logicalDpiY), Fixed);
    else if(type == CSSPrimitiveValue::CSS_PERCENTAGE)
        l = Length(primitiveValue->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else
        return;
    layer->setBackgroundXPosition(l);
}

void CSSStyleSelector::mapBackgroundYPosition(BackgroundLayer* layer, DOM::CSSValueImpl* value)
{
    if (value->cssValueType() == CSSValue::CSS_INITIAL) {
        layer->setBackgroundYPosition(RenderStyle::initialBackgroundYPosition());
        return;
    }

    if (!value->isPrimitiveValue()) return;
    CSSPrimitiveValueImpl* primitiveValue = static_cast<CSSPrimitiveValueImpl*>(value);
    Length l;
    int type = primitiveValue->primitiveType();
    if(type > CSSPrimitiveValue::CSS_PERCENTAGE && type < CSSPrimitiveValue::CSS_DEG)
        l = Length(primitiveValue->computeLength(style, logicalDpiY), Fixed);
    else if(type == CSSPrimitiveValue::CSS_PERCENTAGE)
        l = Length(primitiveValue->floatValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else
        return;
    layer->setBackgroundYPosition(l);
}

#ifdef APPLE_CHANGES
void CSSStyleSelector::checkForGenericFamilyChange(RenderStyle* aStyle, RenderStyle* aParentStyle)
{
  const FontDef& childFont = aStyle->htmlFont().fontDef;

  if (childFont.sizeSpecified || !aParentStyle)
    return;

  const FontDef& parentFont = aParentStyle->htmlFont().fontDef;

  if (childFont.genericFamily == parentFont.genericFamily)
    return;

  // For now, lump all families but monospace together.
  if (childFont.genericFamily != FontDef::eMonospace &&
      parentFont.genericFamily != FontDef::eMonospace)
    return;

  // We know the parent is monospace or the child is monospace, and that font
  // size was unspecified.  We want to alter our font size to use the correct
  // "medium" font for our family.
  float size = 0;
  int minFontSize = settings->minFontSize();
  size = (childFont.genericFamily == FontDef::eMonospace) ? m_fixedFontSizes[3] : m_fontSizes[3];
  int isize = (int)size;
  if (isize < minFontSize)
    isize = minFontSize;

  FontDef newFontDef(childFont);
  newFontDef.size = isize;
  aStyle->setFontDef(newFontDef);
}
#endif

// #### FIXME!! this is ugly and isn't even properly updated or destroyed.

void CSSStyleSelector::addViewportDependentMediaQueryResult(const MediaQueryExp* expr, bool result)
{
    m_viewportDependentMediaQueryResults.append( new MediaQueryResult(*expr, result) );
}

bool CSSStyleSelector::affectedByViewportChange() const
{
    unsigned s = m_viewportDependentMediaQueryResults.size();
    for (unsigned i = 0; i < s; i++) {
        if (m_medium->eval(&m_viewportDependentMediaQueryResults[i]->m_expression) != m_viewportDependentMediaQueryResults[i]->m_result)
            return true;
    }
    return false;
}

} // namespace khtml

// kate: space-indent on; indent-width 4; tab-width 8;
