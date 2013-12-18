/*
 * This file is part of the CSS implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2005, 2006 Apple Computer, Inc.
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
#ifndef _CSS_cssstyleselector_h_
#define _CSS_cssstyleselector_h_

#include "rendering/render_style.h"
#include "dom/dom_string.h"
#include "css/css_mediaquery.h"
#include <QtCore/QVarLengthArray>
#include <QtCore/QList>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/HashSet.h>

class KHTMLSettings;
class KHTMLView;
class KHTMLPart;
class QUrl;

namespace DOM {
    class DocumentImpl;
    class NodeImpl;
    class ElementImpl;
    class StyleSheetImpl;
    class CSSStyleRuleImpl;
    class CSSStyleSheetImpl;
    class CSSSelector;
    class CSSStyleDeclarationImpl;
    class CSSProperty;
    class StyleSheetListImpl;
    class CSSValueImpl;
    class CSSFontSelector;
}

namespace khtml
{
    class CSSStyleSelectorList;
    class CSSOrderedRule;
    class CSSOrderedPropertyList;
    class RenderStyle;
    class MediaQueryEvaluator;

    /*
     * to remember the source where a rule came from. Differentiates between
     * important and not important rules. This is ordered in the order they have to be applied
     * to the RenderStyle.
     */
    enum Source {
	Default = 0,
	User = 1,
	NonCSSHint = 2,
	Author = 3,
	Inline = 4,
	AuthorImportant = 5,
	InlineImportant = 6,
	UserImportant =7
    };

    /*
     * List of properties that get applied to the Element. We need to collect them first
     * and then apply them one by one, because we have to change the apply order.
     * Some properties depend on other one already being applied (for example all properties specifying
     * some length need to have already the correct font size. Same applies to color
     *
     * While sorting them, we have to take care not to mix up the original order.
     */
    class CSSOrderedProperty
    {
    public:
	CSSOrderedProperty(DOM::CSSProperty *_prop, uint _selector,
			   bool first, Source source, unsigned int specificity,
			   unsigned int _position )
	    : prop ( _prop ), pseudoId( RenderStyle::NOPSEUDO ), selector( _selector ),
	      position( _position )
	{
	    priority = (!first << 30) | (source << 24) | specificity;
	}
	CSSOrderedProperty(): prop( 0 ), pseudoId( RenderStyle::NOPSEUDO ), selector(0),
	      position( 0 )
        {
        
        }

	bool operator < ( const CSSOrderedProperty &other ) const {
             if (priority < other.priority) return true;
             if (priority > other.priority) return false;
             if (position < other.position) return true;
             return false;
	}

	DOM::CSSProperty *prop;
	RenderStyle::PseudoId pseudoId;
	unsigned int selector;
	unsigned int position;

	quint32 priority;
    };

    class MediaQueryResult
    {
    public:
        MediaQueryResult(const MediaQueryExp& expr, bool result)
        : m_expression(expr)
        , m_result(result)
        {}

        MediaQueryExp m_expression;
        bool m_result;
    };

    /**
     * this class selects a RenderStyle for a given Element based on the
     * collection of stylesheets it contains. This is just a virtual base class
     * for specific implementations of the Selector. At the moment only CSSStyleSelector
     * exists, but someone may wish to implement XSL...
     */
    class StyleSelector
    {
    public:
	StyleSelector() {}

	/* as nobody has implemented a second style selector up to now comment out
	   the virtual methods until then, so the class has no vptr.
	*/
// 	virtual ~StyleSelector() {}
// 	virtual RenderStyle *styleForElement(DOM::ElementImpl *e) = 0;

	enum State {
	    None = 0x00,
	    Hover = 0x01,
	    Focus = 0x02,
	    Active = 0x04
	};
    };


    /**
     * the StyleSelector implementation for CSS.
     */
    class CSSStyleSelector : public StyleSelector
    {
    public:
	/**
	 * creates a new StyleSelector for a Document.
	 * goes through all StyleSheets defined in the document and
	 * creates a list of rules it needs to apply to objects
	 *
	 * Also takes into account special cases for HTML documents,
	 * including the defaultStyle (which is html only)
	 */
	CSSStyleSelector( DOM::DocumentImpl* doc, QString userStyleSheet, DOM::StyleSheetListImpl *styleSheets, const QUrl &url,
                          bool _strictParsing );
	/**
	 * same as above but for a single stylesheet.
	 */
	CSSStyleSelector( DOM::CSSStyleSheetImpl *sheet );

	~CSSStyleSelector();

	void addSheet( DOM::CSSStyleSheetImpl *sheet );
        KHTML_EXPORT static void clear();
        static void reparseConfiguration();

	static void loadDefaultStyle(const KHTMLSettings *s, DOM::DocumentImpl *doc);

	// fallbackParentStyle will be inheritted from if the parent doesn't have style info
	RenderStyle *styleForElement(DOM::ElementImpl *e, RenderStyle* fallbackParentStyle = 0);

	bool isMatchedByAnySelector(DOM::ElementImpl* e, const QList<DOM::CSSSelector*>& sels);

        QVector<int> fontSizes() const { return m_fontSizes; }
	QVector<int> fixedFontSizes() const { return m_fixedFontSizes; }

	bool strictParsing;
	struct Encodedurl {
	    QString host; //also contains protocol
	    QString path;
	    QString file;
	} encodedurl;

        // called from KHTMLView::print()
	void computeFontSizes(int logicalDpiY, int zoomFactor);
	void computeFontSizesFor(int logicalDpiY, int zoomFactor, QVector<int>& fontSizes, bool isFixed);

	static void precomputeAttributeDependencies(DOM::DocumentImpl* doc, DOM::CSSSelector* sel);
        void addViewportDependentMediaQueryResult(const MediaQueryExp*, bool result);
        bool affectedByViewportChange() const;
	DOM::CSSFontSelector* fontSelector() const { return m_fontSelector; }
    protected:
	/* checks if the complete selector (which can be build up from a few CSSSelector's
	    with given relationships matches the given Element */
	void checkSelector(int selector, DOM::ElementImpl *e);
	/* checks if the selector matches the given Element */
	bool checkSimpleSelector(DOM::CSSSelector *selector, DOM::ElementImpl *e, bool isAncestor, bool isSubSelector = false);

        enum SelectorMatch {SelectorMatches = 0, SelectorFailsLocal, SelectorFails};
        SelectorMatch checkSelector(DOM::CSSSelector *sel, DOM::ElementImpl *e, bool isAncestor, bool isSubSelector = false);

        // computes various summaries of the documents' properties, which are used in
        // the various checkSelector methods for optimizations
        void prepareToMatchElement(DOM::ElementImpl* e, bool withDeps);

        void addDependency(int dependencyType, DOM::ElementImpl* dependency);
        void setupDefaultRootStyle(DOM::DocumentImpl *d=0);
#ifdef APPLE_CHANGES
	/* This function fixes up the default font size if it detects that the
	   current generic font family has changed. -dwh */
	void checkForGenericFamilyChange(RenderStyle* aStyle, RenderStyle* aParentStyle);
#endif
        // locates potentially similar style
        RenderStyle* locateSimilarStyle();

	/* builds up the selectors and properties lists from the CSSStyleSelectorList's */
	void buildLists();
	void clearLists();

        void adjustRenderStyle(RenderStyle* style, DOM::ElementImpl *e);

        void addInlineDeclarations(DOM::ElementImpl* e);

	static DOM::CSSStyleSheetImpl *s_defaultSheet;
	static DOM::CSSStyleSheetImpl *s_defaultNonCSSHintsSheet;
	static DOM::CSSStyleSheetImpl *s_quirksSheet;
	static CSSStyleSelectorList *s_defaultStyle;
	static CSSStyleSelectorList *s_defaultQuirksStyle;
	static CSSStyleSelectorList *s_defaultPrintStyle;
	static CSSStyleSelectorList *s_defaultNonCSSHintsStyle;
        static RenderStyle* styleNotYetAvailable;

	CSSStyleSelectorList *defaultStyle;
	CSSStyleSelectorList *defaultQuirksStyle;
	CSSStyleSelectorList *defaultNonCSSHintsStyle;
	CSSStyleSelectorList *defaultPrintStyle;

	CSSStyleSelectorList *authorStyle;
	CSSStyleSelectorList *implicitStyle;
        CSSStyleSelectorList *userStyle;
        DOM::CSSStyleSheetImpl *userSheet;

public:

    private:
        void init(const KHTMLSettings* settings, DOM::DocumentImpl* doc);

        void mapBackgroundAttachment(BackgroundLayer* layer, DOM::CSSValueImpl* value);
        void mapBackgroundClip(BackgroundLayer* layer, DOM::CSSValueImpl* value);
        void mapBackgroundOrigin(BackgroundLayer* layer, DOM::CSSValueImpl* value);
        void mapBackgroundImage(BackgroundLayer* layer, DOM::CSSValueImpl* value);
        void mapBackgroundRepeat(BackgroundLayer* layer, DOM::CSSValueImpl* value);
        void mapBackgroundSize(BackgroundLayer* layer, DOM::CSSValueImpl* value);
        void mapBackgroundXPosition(BackgroundLayer* layer, DOM::CSSValueImpl* value);
        void mapBackgroundYPosition(BackgroundLayer* layer, DOM::CSSValueImpl* value);

    public: // we need to make the enum public for SelectorCache
	enum SelectorState {
	    Unknown = 0,
	    Applies,
	    AppliesPseudo,
	    Invalid
	};

        enum SelectorMedia {
            MediaAural = 1,
            MediaBraille,
            MediaEmboss,
            MediaHandheld,
            MediaPrint,
            MediaProjection,
            MediaScreen,
            MediaTTY,
            MediaTV
        };
    protected:

        struct SelectorCache {
            SelectorState state;
            unsigned firstPropertyIndex;
        };

	unsigned int selectors_size;
	DOM::CSSSelector **selectors;
	SelectorCache *selectorCache;
        unsigned *nextPropertyIndexes;
	unsigned int properties_size;
        unsigned *nextSimilarSelector;
        CSSOrderedProperty *propertiesBuffer;
	QVarLengthArray<CSSOrderedProperty> inlineProps;
        MediaQueryEvaluator* m_medium;
        WTF::Vector<CSSOrderedProperty*> propsToApply;
        WTF::Vector<CSSOrderedProperty*> pseudoProps;

        // hashes for faster styleForElement
        WTF::HashMap<quintptr, int> classSelector, idSelector;
        WTF::HashMap<unsigned, int> tagSelector;
        int otherSelector;

        WTF::HashSet<unsigned> tagCache;
        WTF::HashSet<quintptr> classCache, idCache;

	RenderStyle::PseudoId dynamicPseudo;

	RenderStyle *style;
	RenderStyle *parentStyle;
	DOM::ElementImpl *element;
	DOM::NodeImpl *parentNode;
	KHTMLView *view;
	KHTMLPart *part;
	const KHTMLSettings *settings;
	bool rememberDependencies;
	int logicalDpiY;
  	RenderStyle*     m_rootDefaultStyle;
        QVector<int>     m_fontSizes;
	QVector<int>     m_fixedFontSizes;

	bool fontDirty;
	DOM::CSSFontSelector* m_fontSelector;
        QList<MediaQueryResult*> m_viewportDependentMediaQueryResults;

	void applyRule(int id, DOM::CSSValueImpl *value);
        void applySVGRule(int id, DOM::CSSValueImpl *value);
    };

    /*
     * This is the list we will collect all properties we need to apply in.
     * It will get sorted once before applying.
     */
    class CSSOrderedPropertyList : public QVector<CSSOrderedProperty>
    {
    public:
        void append(DOM::CSSStyleDeclarationImpl *decl, uint selector, uint specificity,
                    Source regular, Source important);
    };

    class CSSOrderedRule
    {
    public:
	CSSOrderedRule(DOM::CSSStyleRuleImpl *r, DOM::CSSSelector *s, int _index);
	~CSSOrderedRule();

	DOM::CSSSelector *selector;
	DOM::CSSStyleRuleImpl *rule;
	int index;
    };

    class CSSStyleSelectorList : public QList<CSSOrderedRule*>
    {
    public:
	CSSStyleSelectorList();
	virtual ~CSSStyleSelectorList();

	void append( DOM::CSSStyleSheetImpl *sheet,
		     MediaQueryEvaluator *medium, CSSStyleSelector* styleSelector );

        void collect(WTF::HashMap<DOM::CSSSelector*, int>* selectorsCache, QList<DOM::CSSSelector*> *selectorList,
                CSSOrderedPropertyList *propList, Source regular, Source important);
    };

}
#endif
