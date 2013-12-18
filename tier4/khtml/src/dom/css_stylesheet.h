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
#ifndef _CSS_css_stylesheet_h_
#define _CSS_css_stylesheet_h_

#include <khtml_export.h>

#include <dom/dom_string.h>
#include <dom/dom_node.h>
#include <dom/dom_misc.h>

namespace DOM {

class StyleSheetImpl;
class MediaList;
class NodeImpl;
class DocumentImpl;

/**
 * The \c StyleSheet interface is the abstract base
 * interface for any type of style sheet. It represents a single style
 * sheet associated with a structured document. In HTML, the
 * StyleSheet interface represents either an external style sheet,
 * included via the HTML <a
 * href="http://www.w3.org/TR/REC-html40/struct/links.html#h-12.3">
 * LINK </a> element, or an inline <a
 * href="http://www.w3.org/TR/REC-html40/present/styles.html#h-14.2.3">
 * STYLE </a> element. In XML, this interface represents an external
 * style sheet, included via a <a
 * href="http://www.w3.org/TR/xml-stylesheet"> style sheet processing
 * instruction </a> .
 *
 */
class KHTML_EXPORT StyleSheet
{
public:
    StyleSheet();
    StyleSheet(const StyleSheet &other);
    StyleSheet(StyleSheetImpl *impl);
public:

    StyleSheet & operator = (const StyleSheet &other);

    ~StyleSheet();

    /**
     * This specifies the style sheet language for this style sheet.
     * The style sheet language is specified as a content type (e.g.
     * "text/css"). The content type is often specified in the
     * \c ownerNode . A list of registered content types can be
     * found at <a
     * href="ftp://ftp.isi.edu/in-notes/iana/assignments/media-types/">
     * ftp://ftp.isi.edu/in-notes/iana/assignments/media-types/ </a> .
     * Also see the <a
     * href="http://www.w3.org/TR/REC-html40/struct/links.html#adef-type-A">
     * type attribute definition </a> for the \c LINK
     * element in HTML 4.0, and the type pseudo-attribute for the XML
     * <a href="http://www.w3.org/TR/xml-stylesheet"> style sheet
     * processing instruction </a> .
     *
     */
    DOM::DOMString type() const;

    /**
     * \c false if the style sheet is applied to the
     * document. \c true if it is not. Modifying this
     * attribute may cause a reresolution of style for the document.
     *
     */
    bool disabled() const;

    /**
     * see disabled
     */
    void setDisabled( bool );

    /**
     * The node that associates this style sheet with the document.
     * For HTML, this may be the corresponding \c LINK or
     * \c STYLE element. For XML, it may be the linking
     * processing instruction. For style sheets that are included by
     * other style sheets, this attribute has a value of null.
     *
     */
    DOM::Node ownerNode() const;

    /**
     * For style sheet languages that support the concept of style
     * sheet inclusion, this attribute represents the including style
     * sheet, if one exists. If the style sheet is a top-level style
     * sheet, or the style sheet language does not support inclusion,
     * the value of the attribute is null.
     *
     */
    StyleSheet parentStyleSheet() const;

    /**
     * If the style sheet is a linked style sheet, the value of its
     * attribute is its location. For inline style sheets, the value
     * of this attribute is null. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/links.html#adef-href">
     * href attribute definition </a> for the \c LINK
     * element in HTML 4.0, and the href pseudo-attribute for the XML
     * <a href="http://www.w3.org/TR/xml-stylesheet"> style sheet
     * processing instruction </a> .
     *
     */
    DOM::DOMString href() const;

    /**
     * The advisory title. The title is often specified in the
     * \c ownerNode . See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/global.html#adef-title">
     * title attribute definition </a> for the \c LINK
     * element in HTML 4.0, and the title pseudo-attribute for the XML
     * <a href="http://www.w3.org/TR/xml-stylesheet"> style sheet
     * processing instruction </a> .
     *
     */
    DOM::DOMString title() const;

    /**
     * The intended destination media for style information. The media
     * is often specified in the \c ownerNode . See the <a
     * href="http://www.w3.org/TR/REC-html40/present/styles.html#adef-media">
     * media attribute definition </a> for the \c LINK
     * element in HTML 4.0, and the media pseudo-attribute for the XML
     * <a href="http://www.w3.org/TR/WD-xml-stylesheet"> style sheet
     * processing instruction </a> .
     *
     */
    MediaList media() const;

    /**
     * @internal
     */
    QUrl baseUrl();
    bool isCSSStyleSheet() const;
    StyleSheetImpl *handle() const { return impl; }
    bool isNull() const { return !impl; }
protected:
    StyleSheetImpl *impl;
};


/**
 * This exception is raised when a specific CSS operation is impossible
 * to perform.
 */
class KHTML_EXPORT CSSException
{
public:
    CSSException(unsigned short _code) { code = _code; }
    CSSException(const CSSException &other) { code = other.code; }

    CSSException & operator = (const CSSException &other)
        { code = other.code; return *this; }

    virtual ~CSSException() {}
    /**
     * An integer indicating the type of error generated.
     *
     */
    unsigned short   code;

    enum ExceptionCode
    {
        SYNTAX_ERR                     = 0,
        INVALID_MODIFICATION_ERR       = 1,
        _EXCEPTION_OFFSET              = 1000,
        _EXCEPTION_MAX                 = 1999
    };

    /// Returns the name of this error
    DOMString codeAsString() const;

    /// Returns the name of given error code
    static DOMString codeAsString(int cssCode);

    /** @internal - checks to see whether internal code is a CSS one */
    static bool isCSSExceptionCode(int exceptioncode);
};

class CSSStyleSheetImpl;
class CSSRule;
class CSSRuleList;

/**
 * The \c CSSStyleSheet interface is a concrete interface
 * used to represent a CSS style sheet i.e. a style sheet whose
 * content type is "text/css".
 *
 */
class KHTML_EXPORT CSSStyleSheet : public StyleSheet
{
public:
    CSSStyleSheet();
    CSSStyleSheet(const CSSStyleSheet &other);
    CSSStyleSheet(const StyleSheet &other);
    CSSStyleSheet(CSSStyleSheetImpl *impl);
public:

    CSSStyleSheet & operator = (const CSSStyleSheet &other);
    CSSStyleSheet & operator = (const StyleSheet &other);

    ~CSSStyleSheet();

    /**
     * If this style sheet comes from an \c \@import rule,
     * the \c ownerRule attribute will contain the
     * \c CSSImportRule . In that case, the \c ownerNode
     * attribute in the \c StyleSheet interface
     * will be \c null . If the style sheet comes from an
     * element or a processing instruction, the \c ownerRule
     * attribute will be \c null and the
     * \c ownerNode attribute will contain the \c Node .
     *
     */
    CSSRule ownerRule() const;

    /**
     * The list of all CSS rules contained within the style sheet.
     * This includes both <a
     * href="http://www.w3.org/TR/REC-CSS2/syndata.html#q8"> rule sets
     * </a> and <a
     * href="http://www.w3.org/TR/REC-CSS2/syndata.html#at-rules">
     * at-rules </a> .
     *
     */
    CSSRuleList cssRules() const;

    /**
     * Used to insert a new rule into the style sheet. The new rule
     * now becomes part of the cascade.
     *
     * @param rule The parsable text representing the rule. For rule
     * sets this contains both the selector and the style declaration.
     * For at-rules, this specifies both the at-identifier and the
     * rule content.
     *
     * @param index The index within the style sheet's rule list of
     * the rule before which to insert the specified rule. If the
     * specified index is equal to the length of the style sheet's
     * rule collection, the rule will be added to the end of the style
     * sheet.
     *
     * @return The index within the style sheet's rule collection of
     * the newly inserted rule.
     *
     * @exception DOMException
     * HIERARCHY_REQUEST_ERR: Raised if the rule cannot be inserted at
     * the specified index e.g. if an \c \@import rule is
     * inserted after a standard rule set or other at-rule.
     *
     *  INDEX_SIZE_ERR: Raised if the specified index is not a valid
     * insertion point.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this style sheet is
     * readonly.
     *
     * @exception CSSException
     *  SYNTAX_ERR: Raised if the specified rule has a syntax error
     * and is unparsable.
     *
     */
    unsigned long insertRule ( const DOM::DOMString &rule, unsigned long index );

    /**
     * Used to delete a rule from the style sheet.
     *
     * @param index The index within the style sheet's rule list of
     * the rule to remove.
     *
     * @return
     *
     * @exception DOMException
     * INDEX_SIZE_ERR: Raised if the specified index does not
     * correspond to a rule in the style sheet's rule list.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this style sheet is
     * readonly.
     *
     */
    void deleteRule ( unsigned long index );

    /** @internal */
    DOM::DOMString charset() const;
};


class StyleSheetListImpl;
class StyleSheet;

/**
 * The \c StyleSheetList interface provides the
 * abstraction of an ordered collection of style sheets.
 *
 */
class KHTML_EXPORT StyleSheetList
{
public:
    StyleSheetList();
    StyleSheetList(const StyleSheetList &other);
    StyleSheetList(StyleSheetListImpl *impl);
public:

    StyleSheetList & operator = (const StyleSheetList &other);

    ~StyleSheetList();

    /**
     * The number of \c StyleSheet in the list. The range
     * of valid child stylesheet indices is \c 0 to
     * \c length-1 inclusive.
     *
     */
    unsigned long length() const;

    /**
     * Used to retrieve a style sheet by ordinal index.
     *
     * @param index Index into the collection
     *
     * @return The style sheet at the \c index position in
     * the \c StyleSheetList , or \c null if
     * that is not a valid index.
     *
     */
    StyleSheet item ( unsigned long index );

    /**
     * @internal
     */
    StyleSheetListImpl *handle() const;
    bool isNull() const;

protected:
    StyleSheetListImpl *impl;
};


class MediaListImpl;
class CSSRule;
class CSSStyleSheet;

/**
 * The \c MediaList interface provides the abstraction of
 * an ordered collection of media, without defining or constraining
 * how this collection is implemented. All media are lowercase
 * strings.
 *
 */
class KHTML_EXPORT MediaList
{
public:
    MediaList();
    MediaList(const MediaList &other);
    MediaList(MediaListImpl *impl);
public:

    MediaList & operator = (const MediaList &other);

    ~MediaList();

    /**
     * The parsable textual representation of the media list. This is a
     * comma-separated list of media.
     *
     * @exception DOMException
     * SYNTAX_ERR: Raised if the specified string value has a syntax error and
     * is unparsable.
     *
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this media list is readonly.
     */
    DOM::DOMString mediaText() const;

    /**
     * see mediaText
     */
    void setMediaText(const DOM::DOMString &value);

    /**
     * The number of media in the list. The range of valid media is 0 to length-1 inclusive.
     */
    unsigned long length() const;


    /**
     * Returns the indexth in the list. If index is greater than or equal to
     * the number of media in the list, this returns null.
     *
     * @param index Index into the collection.
     *
     * @return The medium at the indexth position in the MediaList, or null if
     * that is not a valid index.
     */
    DOM::DOMString item(unsigned long index) const;

    /**
     * Deletes the medium indicated by oldMedium from the list.
     *
     * @param oldMedium The medium to delete in the media list.
     *
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this list is readonly.
     *
     * NOT_FOUND_ERR: Raised if oldMedium is not in the list.
     */
    void deleteMedium(const DOM::DOMString &oldMedium);

    /**
     * Adds the medium newMedium to the end of the list. If the newMedium is
     * already used, it is first removed.
     *
     * @param newMedium The new medium to add.
     *
     * @exception DOMException
     * INVALID_CHARACTER_ERR: If the medium contains characters that are
     * invalid in the underlying style language.
     *
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this list is readonly.
     */
    void appendMedium(const DOM::DOMString &newMedium);

    /**
     * @internal
     */
    MediaListImpl *handle() const;
    bool isNull() const;

protected:
    MediaListImpl *impl;
};

class LinkStyleImpl;

class KHTML_EXPORT LinkStyle
{
public:
    LinkStyle();
    LinkStyle(const LinkStyle &other);

    LinkStyle & operator = (const LinkStyle &other);
    LinkStyle & operator = (const Node &other);

    ~LinkStyle();

    StyleSheet sheet();

    bool isNull() const;

protected:
    DOM::NodeImpl *node;
    LinkStyleImpl *impl;
};

class DocumentStyleImpl;

class KHTML_EXPORT DocumentStyle
{
public:
    DocumentStyle();
    DocumentStyle(const DocumentStyle &other);

    DocumentStyle & operator = (const DocumentStyle &other);
    DocumentStyle & operator = (const Document &other);

    ~DocumentStyle();

    StyleSheetList styleSheets() const ;

    DOMString preferredStylesheetSet() const;
    DOMString selectedStylesheetSet() const;
    void setSelectedStylesheetSet( const DOMString& aString );

    bool isNull() const { return !impl; }

protected:
    DOM::DocumentImpl *doc;
    DocumentStyleImpl *impl;
};

} // namespace

#endif
