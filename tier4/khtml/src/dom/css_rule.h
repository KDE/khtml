/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999 Lars Knoll (knoll@kde.org)
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
 * This file includes excerpts from the Document Object Model (DOM)
 * Level 2 Specification (Candidate Recommendation)
 * http://www.w3.org/TR/2000/CR-DOM-Level-2-20000510/
 * Copyright © 2000 W3C® (MIT, INRIA, Keio), All Rights Reserved.
 *
 */
#ifndef _CSS_css_rule_h_
#define _CSS_css_rule_h_

#include <dom/dom_string.h>
#include <dom/css_stylesheet.h>
#include <dom/css_value.h>

namespace DOM {

class CSSRuleImpl;

/**
 * The \c CSSRule interface is the abstract base interface
 * for any type of CSS <a
 * href="http://www.w3.org/TR/REC-CSS2/syndata.html#q5"> statement
 * </a> . This includes both <a
 * href="http://www.w3.org/TR/REC-CSS2/syndata.html#q8"> rule sets
 * </a> and <a
 * href="http://www.w3.org/TR/REC-CSS2/syndata.html#at-rules">
 * at-rules </a> . An implementation is expected to preserve all rules
 * specified in a CSS style sheet, even if it is not recognized.
 * Unrecognized rules are represented using the \c CSSUnknownRule
 * interface.
 *
 */
class KHTML_EXPORT CSSRule
{
public:
    CSSRule();
    CSSRule(const CSSRule &other);
    CSSRule(CSSRuleImpl *impl);
public:

    CSSRule & operator = (const CSSRule &other);

    ~CSSRule();
    /**
     * An integer indicating which type of rule this is.
     *
     */
    enum RuleType {
        UNKNOWN_RULE = 0,
        STYLE_RULE = 1,
        CHARSET_RULE = 2,
        IMPORT_RULE = 3,
        MEDIA_RULE = 4,
        FONT_FACE_RULE = 5,
        PAGE_RULE = 6,
        NAMESPACE_RULE = 10, ///< CSSOM, @since 4.6.0
	QUIRKS_RULE = 100 // KHTML CSS Extension
    };

    /**
     * The type of the rule, as defined above. The expectation is that
     * binding-specific casting methods can be used to cast down from
     * an instance of the \c CSSRule interface to the
     * specific derived interface implied by the \c type .
     *
     */
    unsigned short type() const;

    /**
     * The parsable textual representation of the rule. This reflects
     * the current state of the rule and not its initial value.
     *
     */
    DOM::DOMString cssText() const;

    /**
     * see cssText
     * @exception DOMException
     *
     *  HIERARCHY_REQUEST_ERR: Raised if the rule cannot be inserted
     * at this point in the style sheet.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this style sheet is
     * readonly.
     *
     * @exception CSSException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     * INVALID_MODIFICATION_ERR: Raised if the specified CSS string value
     * represents a different type of rule than the current one.
     */
    void setCssText( const DOM::DOMString & );

    /**
     * The style sheet that contains this rule.
     *
     */
    CSSStyleSheet parentStyleSheet() const;

    /**
     * If this rule is contained inside another rule (e.g. a style
     * rule inside an \@media block), this is the containing rule. If
     * this rule is not nested inside any other rules, this returns
     * \c null .
     *
     */
    CSSRule parentRule() const;

    /**
     * @internal
     * not part of the DOM
     */
    CSSRuleImpl *handle() const;
    bool isNull() const;

protected:
    CSSRuleImpl *impl;

    void assignOther( const CSSRule &other, RuleType thisType );
};

class CSSCharsetRuleImpl;

/**
 * The \c CSSCharsetRule interface a <a href=""> \@charset
 * rule </a> in a CSS style sheet. A \c \@charset rule can
 * be used to define the encoding of the style sheet.
 *
 */
class KHTML_EXPORT CSSCharsetRule : public CSSRule
{
public:
    CSSCharsetRule();
    CSSCharsetRule(const CSSCharsetRule &other);
    CSSCharsetRule(const CSSRule &other);
    CSSCharsetRule(CSSCharsetRuleImpl *impl);
public:

    CSSCharsetRule & operator = (const CSSCharsetRule &other);
    CSSCharsetRule & operator = (const CSSRule &other);

    ~CSSCharsetRule();

    /**
     * The encoding information used in this \c \@charset
     * rule.
     *
     */
    DOM::DOMString encoding() const;

    /**
     * see encoding
     * @exception CSSException
     * SYNTAX_ERR: Raised if the specified encoding value has a syntax
     * error and is unparsable.
     *
     * @exception DOMException
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this encoding rule is
     * readonly.
     *
     */
    void setEncoding( const DOM::DOMString & );
};


class CSSFontFaceRuleImpl;
/**
 * The \c CSSFontFaceRule interface represents a <a
 * href="http://www.w3.org/TR/REC-CSS2/fonts.html#font-descriptions">
 * \c \@font-face rule </a> in a CSS style sheet. The \c \@font-face
 * rule is used to hold a set of font descriptions.
 *
 */
class KHTML_EXPORT CSSFontFaceRule : public CSSRule
{
public:
    CSSFontFaceRule();
    CSSFontFaceRule(const CSSFontFaceRule &other);
    CSSFontFaceRule(const CSSRule &other);
    CSSFontFaceRule(CSSFontFaceRuleImpl *impl);
public:

    CSSFontFaceRule & operator = (const CSSFontFaceRule &other);
    CSSFontFaceRule & operator = (const CSSRule &other);

    ~CSSFontFaceRule();

    /**
     * The <a href="http://www.w3.org/TR/REC-CSS2/syndata.html#q8">
     * declaration-block </a> of this rule.
     *
     */
    CSSStyleDeclaration style() const;
};

class CSSImportRuleImpl;
/**
 * The \c CSSImportRule interface represents a <a
 * href="http://www.w3.org/TR/REC-CSS2/cascade.html#at-import">
 * \c \@import rule </a> within a CSS style sheet. The \c \@import
 * rule is used to import style rules from other style sheets.
 *
 */
class KHTML_EXPORT CSSImportRule : public CSSRule
{
public:
    CSSImportRule();
    CSSImportRule(const CSSImportRule &other);
    CSSImportRule(const CSSRule &other);
    CSSImportRule(CSSImportRuleImpl *impl);
public:

    CSSImportRule & operator = (const CSSImportRule &other);
    CSSImportRule & operator = (const CSSRule &other);

    ~CSSImportRule();

    /**
     * The location of the style sheet to be imported. The attribute
     * will not contain the \c "url(...)" specifier around
     * the URI.
     *
     */
    DOM::DOMString href() const;

    /**
     * A list of media types for which this style sheet may be used.
     *
     */
    MediaList media() const;

    /**
     * The style sheet referred to by this rule, if it has been
     * loaded. The value of this attribute is null if the style sheet
     * has not yet been loaded or if it will not be loaded (e.g. if
     * the style sheet is for a media type not supported by the user
     * agent).
     *
     */
    CSSStyleSheet styleSheet() const;
};

class CSSMediaRuleImpl;
/**
 * The \c CSSMediaRule interface represents a <a
 * href="http://www.w3.org/TR/REC-CSS2/media.html#at-media-rule">
 * \@media rule </a> in a CSS style sheet. A \c \@media rule
 * can be used to delimit style rules for specific media types.
 *
 */
class KHTML_EXPORT CSSMediaRule : public CSSRule
{
public:
    CSSMediaRule();
    CSSMediaRule(const CSSMediaRule &other);
    CSSMediaRule(const CSSRule &other);
    CSSMediaRule(CSSMediaRuleImpl *impl);
public:

    CSSMediaRule & operator = (const CSSMediaRule &other);
    CSSMediaRule & operator = (const CSSRule &other);

    ~CSSMediaRule();

    /**
     * A list of <a
     * href="http://www.w3.org/TR/REC-CSS2/media.html#media-types">
     * media types </a> for this rule.
     *
     */
    MediaList media() const;

    /**
     * A list of all CSS rules contained within the media block.
     *
     */
    CSSRuleList cssRules() const;

    /**
     * Used to insert a new rule into the media block.
     *
     * @param rule The parsable text representing the rule. For rule
     * sets this contains both the selector and the style declaration.
     * For at-rules, this specifies both the at-identifier and the
     * rule content.
     *
     * @param index The index within the media block's rule collection
     * of the rule before which to insert the specified rule. If the
     * specified index is equal to the length of the media blocks's
     * rule collection, the rule will be added to the end of the media
     * block.
     *
     * @return The index within the media block's rule collection of
     * the newly inserted rule.
     *
     * \exception DOMException
     * HIERARCHY_REQUEST_ERR: Raised if the rule cannot be inserted at
     * the specified index. e.g. if an \c \@import rule is
     * inserted after a standard rule set or other at-rule.
     *
     *  INDEX_SIZE_ERR: Raised if the specified index is not a valid
     * insertion point.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this media rule is
     * readonly.
     *
     * \exception CSSException
     *  SYNTAX_ERR: Raised if the specified rule has a syntax error
     * and is unparsable.
     *
     */
    unsigned long insertRule ( const DOM::DOMString &rule, unsigned long index );

    /**
     * Used to delete a rule from the media block.
     *
     * @param index The index within the media block's rule collection
     * of the rule to remove.
     *
     * @return
     *
     * \exception DOMException
     * INDEX_SIZE_ERR: Raised if the specified index does not
     * correspond to a rule in the media rule list.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this media rule is
     * readonly.
     *
     */
    void deleteRule ( unsigned long index );
};


class CSSPageRuleImpl;
/**
 * The \c CSSPageRule interface represents a <a
 * href="http://www.w3.org/TR/REC-CSS2/page.html#page-box"> page rule
 * </a> within a CSS style sheet. The \c @page rule is
 * used to specify the dimensions, orientation, margins, etc. of a
 * page box for paged media.
 *
 */
class KHTML_EXPORT CSSPageRule : public CSSRule
{
public:
    CSSPageRule();
    CSSPageRule(const CSSPageRule &other);
    CSSPageRule(const CSSRule &other);
    CSSPageRule(CSSPageRuleImpl *impl);
public:

    CSSPageRule & operator = (const CSSPageRule &other);
    CSSPageRule & operator = (const CSSRule &other);

    ~CSSPageRule();

    /**
     * The parsable textual representation of the page selector for
     * the rule.
     *
     */
    DOM::DOMString selectorText() const;

    /**
     * see selectorText
     * @exception CSSException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     * @exception DOMException
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this style sheet is
     * readonly.
     *
     */
    void setSelectorText( const DOM::DOMString & );

    /**
     * The <a href="http://www.w3.org/TR/REC-CSS2/syndata.html#q8">
     * declaration-block </a> of this rule.
     *
     */
    CSSStyleDeclaration style() const;
};

class CSSStyleRuleImpl;
/**
 * The \c CSSStyleRule interface represents a single <a
 * href="http://www.w3.org/TR/REC-CSS2/syndata.html#q8"> rule set </a>
 * in a CSS style sheet.
 *
 */
class KHTML_EXPORT CSSStyleRule : public CSSRule
{
public:
    CSSStyleRule();
    CSSStyleRule(const CSSStyleRule &other);
    CSSStyleRule(const CSSRule &other);
    CSSStyleRule(CSSStyleRuleImpl *impl);
public:

    CSSStyleRule & operator = (const CSSStyleRule &other);
    CSSStyleRule & operator = (const CSSRule &other);

    ~CSSStyleRule();

    /**
     * The textual representation of the <a
     * href="http://www.w3.org/TR/REC-CSS2/selector.html"> selector
     * </a> for the rule set. The implementation may have stripped out
     * insignificant whitespace while parsing the selector.
     *
     */
    DOM::DOMString selectorText() const;

    /**
     * see selectorText
     * @exception CSSException
     * SYNTAX_ERR: Raised if the specified CSS string value has a
     * syntax error and is unparsable.
     *
     * @exception DOMException
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this style sheet is
     * readonly.
     *
     */
    void setSelectorText( const DOM::DOMString & );

    /**
     * The <a href="http://www.w3.org/TR/REC-CSS2/syndata.html#q8">
     * declaration-block </a> of this rule set.
     *
     */
    CSSStyleDeclaration style() const;
};

class CSSNamespaceRuleImpl;
/**
 * The \c CSSNamespaceRule interface represents an @namespace rule
 * @since 4.6.0
 *
 */
class KHTML_EXPORT CSSNamespaceRule : public CSSRule
{
public:
    CSSNamespaceRule();
    CSSNamespaceRule(const CSSNamespaceRule &other);
    CSSNamespaceRule(const CSSRule &other);
    CSSNamespaceRule(CSSNamespaceRuleImpl *impl);
    
    DOMString namespaceURI() const;
    DOMString prefix() const;
public:

    CSSNamespaceRule & operator = (const CSSNamespaceRule &other);
    CSSNamespaceRule & operator = (const CSSRule &other);

    ~CSSNamespaceRule();
};




class CSSUnknownRuleImpl;
/**
 * The \c CSSUnkownRule interface represents an at-rule
 * not supported by this user agent.
 *
 */
class KHTML_EXPORT CSSUnknownRule : public CSSRule
{
public:
    CSSUnknownRule();
    CSSUnknownRule(const CSSUnknownRule &other);
    CSSUnknownRule(const CSSRule &other);
    CSSUnknownRule(CSSUnknownRuleImpl *impl);
public:

    CSSUnknownRule & operator = (const CSSUnknownRule &other);
    CSSUnknownRule & operator = (const CSSRule &other);

    ~CSSUnknownRule();
};


class CSSRuleListImpl;
class StyleListImpl;
/**
 * The \c CSSRuleList interface provides the abstraction
 * of an ordered collection of CSS rules.
 *
 */
class KHTML_EXPORT CSSRuleList
{
public:
    CSSRuleList();
    CSSRuleList(const CSSRuleList &other);
    CSSRuleList(CSSRuleListImpl *i);
    CSSRuleList(StyleListImpl *i);
public:

    CSSRuleList & operator = (const CSSRuleList &other);

    ~CSSRuleList();

    /**
     * The number of \c CSSRule s in the list. The range
     * of valid child rule indices is \c 0 to
     * \c length-1 inclusive.
     *
     */
    unsigned long length() const;

    /**
     * Used to retrieve a CSS rule by ordinal index. The order in this
     * collection represents the order of the rules in the CSS style
     * sheet.
     *
     * @param index Index into the collection
     *
     * @return The style rule at the \c index position in
     * the \c CSSRuleList , or \c null if that
     * is not a valid index.
     *
     */
    CSSRule item ( unsigned long index );

    /**
     * @internal
     * not part of the DOM
     */
    CSSRuleListImpl *handle() const;
    bool isNull() const;

protected:
    // we just need a pointer to an implementation here.
    CSSRuleListImpl *impl;
};


} // namespace

#endif
