/*
 * This file is part of the CSS implementation for KDE.
 *
 * Copyright 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright 1999 Waldo Bastian (bastian@kde.org)
 * Copyright 2002 Apple Computer, Inc.
 * Copyright 2004 Allan Sandfeld Jensen (kde@carewolf.com)
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
#ifndef _CSS_BASE_H
#define _CSS_BASE_H

#include "misc/AtomicString.h"
#include "dom/dom_misc.h"
#include "xml/dom_nodeimpl.h"
#include "misc/shared.h"
#include "misc/enum.h"

#include <QtCore/QDate>

namespace DOM {

    class StyleSheetImpl;
    class MediaList;

    class CSSSelector;
    class CSSProperty;
    class CSSValueImpl;

// this class represents a selector for a StyleRule
    class CSSSelector
    {
    public:
	CSSSelector()
	    : tagHistory(0), simpleSelector(0), relation( Descendant ),
	      match( None ), pseudoId( 0 ), _pseudoType(PseudoNotParsed)
        {
            tagLocalName = LocalName::fromId(anyLocalName);
            tagNamespace = NamespaceName::fromId(anyNamespace);
            attrLocalName = LocalName::fromId(0);
            attrNamespace = NamespaceName::fromId(0);
        }

	~CSSSelector() {
	    delete tagHistory;
            delete simpleSelector;
	}

	/**
	 * Print debug output for this selector
	 */
	void print();

	/**
	 * Re-create selector text from selector's data
	 */
	DOMString selectorText() const;

	// checks if the 2 selectors (including sub selectors) agree.
	bool operator == ( const CSSSelector &other ) const;

	// tag == -1 means apply to all elements (Selector = *)

	unsigned int specificity() const;

	/* how the attribute value has to match.... Default is Exact */
	enum Match
	{
	    None = 0,
	    Id,
	    Exact,
	    Set,
	    Class,
	    List,
	    Hyphen,
	    PseudoClass,
	    PseudoElement,
	    Contain,   // css3: E[foo*="bar"]
	    Begin,     // css3: E[foo^="bar"]
	    End        // css3: E[foo$="bar"]
	};

	enum Relation
	{
	    Descendant = 0,
	    Child,
	    DirectAdjacent,
            IndirectAdjacent,
            SubSelector
	};

	enum PseudoType
	{
	    PseudoNotParsed = 0,
	    PseudoOther,
	    PseudoEmpty,
	    PseudoFirstChild,
            PseudoLastChild,
            PseudoNthChild,
            PseudoNthLastChild,
            PseudoOnlyChild,
            PseudoFirstOfType,
            PseudoLastOfType,
            PseudoNthOfType,
            PseudoNthLastOfType,
            PseudoOnlyOfType,
	    PseudoLink,
	    PseudoVisited,
	    PseudoHover,
	    PseudoFocus,
	    PseudoActive,
            PseudoTarget,
            PseudoLang,
            PseudoNot,
            PseudoContains,
            PseudoRoot,
            PseudoEnabled,
            PseudoDisabled,
            PseudoDefault,
            PseudoReadOnly,
            PseudoReadWrite,
            PseudoChecked,
            PseudoIndeterminate,
// pseudo-elements:
    // inherited:
            PseudoFirstLine,
            PseudoFirstLetter,
            PseudoSelection,
    // generated:
            PseudoBefore,
            PseudoAfter,
            PseudoMarker,
            PseudoReplaced
	};

	PseudoType pseudoType() const {
            if (_pseudoType == PseudoNotParsed)
                extractPseudoType();
            return KDE_CAST_BF_ENUM(PseudoType, _pseudoType);
        }

        mutable khtml::AtomicString value;
	CSSSelector *tagHistory;
        CSSSelector* simpleSelector; // Used by :not
        DOM::DOMString string_arg; // Used by :contains, :lang and :nth-*
        LocalName attrLocalName;
        NamespaceName attrNamespace;
        LocalName tagLocalName;
        NamespaceName tagNamespace;

	KDE_BF_ENUM(Relation) relation     : 3;
	mutable KDE_BF_ENUM(Match) 	 match         : 4;
	unsigned int pseudoId : 4;
	mutable KDE_BF_ENUM(PseudoType) _pseudoType : 6;

    private:
	void extractPseudoType() const;
    };

    // a style class which has a parent (almost all have)
    class StyleBaseImpl : public khtml::TreeShared<StyleBaseImpl>
    {
    public:
	StyleBaseImpl()  { m_parent = 0; hasInlinedDecl = false; strictParsing = true; multiLength = false; }
	StyleBaseImpl(StyleBaseImpl *p) {
	    m_parent = p; hasInlinedDecl = false;
	    strictParsing = (m_parent ? m_parent->useStrictParsing() : true);
	    multiLength = false;
	}

	virtual ~StyleBaseImpl() {}

	// returns the url of the style sheet this object belongs to
        // not const
	QUrl baseURL();

	virtual bool isStyleSheet() const { return false; }
	virtual bool isCSSStyleSheet() const { return false; }
	virtual bool isStyleSheetList() const { return false; }
	virtual bool isMediaList() const { return false; }
	virtual bool isRuleList() const { return false; }
	virtual bool isRule() const { return false; }
	virtual bool isStyleRule() const { return false; }
	virtual bool isCharsetRule() const { return false; }
	virtual bool isImportRule() const { return false; }
	virtual bool isMediaRule() const { return false; }
	virtual bool isFontFaceRule() const { return false; }
	virtual bool isPageRule() const { return false; }
	virtual bool isUnknownRule() const { return false; }
	virtual bool isStyleDeclaration() const { return false; }
	virtual bool isValue() const { return false; }
	virtual bool isPrimitiveValue() const { return false; }
	virtual bool isValueList() const { return false; }
	virtual bool isValueCustom() const { return false; }

	void setParent(StyleBaseImpl *parent) { m_parent = parent; }

	static void setParsedValue(int propId, const CSSValueImpl *parsedValue,
				   bool important, QList<CSSProperty*> *propList);

	virtual bool parseString(const DOMString &/*cssString*/, bool = false) { return false; }

        // verifies if the resource chain is fully loaded,
        // and in the affirmative, notifies the owner document
	virtual void checkLoaded() const;
	// makes sure the resource chain is considered 'Pending' by the owner document
        virtual void checkPending() const;

	void setStrictParsing( bool b ) { strictParsing = b; }
	bool useStrictParsing() const { return strictParsing; }

        // not const
	StyleSheetImpl* stylesheet();

    protected:
	bool hasInlinedDecl : 1;
	bool strictParsing : 1;
	bool multiLength : 1;
    };

    // a style class which has a list of children (StyleSheets for example)
    class StyleListImpl : public StyleBaseImpl
    {
    public:
	StyleListImpl() : StyleBaseImpl() { m_lstChildren = 0; }
	StyleListImpl(StyleBaseImpl *parent) : StyleBaseImpl(parent) { m_lstChildren = 0; }
	virtual ~StyleListImpl();

	unsigned long length() const { return m_lstChildren->count(); }
	StyleBaseImpl *item(unsigned long num) const { return num < length() ? m_lstChildren->at(num) : 0; }

	void append(StyleBaseImpl *item) { m_lstChildren->append(item); }

    protected:
	QList<StyleBaseImpl*> *m_lstChildren;
    };

    KHTML_NO_EXPORT int getPropertyID(const char *tagStr, int len);
    KHTML_NO_EXPORT int getValueID(const char *tagStr, int len);

    struct SelectorHash
    {
        static unsigned hash(CSSSelector* selector)
        {
            unsigned result = 0;
            while (selector) {
                result ^= (quintptr)selector->value.impl();
                result ^= (selector->attrLocalName.id() << 3);
                result ^= (selector->attrNamespace.id() << 7);
                result ^= (selector->tagLocalName.id() << 10);
                result ^= (selector->tagNamespace.id() << 13);
                result ^= (selector->relation << 17);
                result ^= (selector->match << 20);
                result ^= result << 5;
                selector = selector->tagHistory;
            }
            return result;
        }
        static bool equal(CSSSelector* a, CSSSelector* b) { return a == b || *a == *b; }
        static const bool safeToCompareToEmptyOrDeleted = false;
    };

}

namespace WTF
{
    template<typename T> struct DefaultHash;
    template<> struct DefaultHash<DOM::CSSSelector*> {
        typedef DOM::SelectorHash Hash;
    };
}

#endif
