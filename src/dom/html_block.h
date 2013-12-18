/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999 Lars Knoll (knoll@kde.org)
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
 * This file includes excerpts from the Document Object Model (DOM)
 * Level 1 Specification (Recommendation)
 * http://www.w3.org/TR/REC-DOM-Level-1/
 * Copyright © World Wide Web Consortium , (Massachusetts Institute of
 * Technology , Institut National de Recherche en Informatique et en
 * Automatique , Keio University ). All Rights Reserved.
 *
 */
// --------------------------------------------------------------------------

#ifndef HTML_BLOCK_H
#define HTML_BLOCK_H

#include <khtml_export.h>
#include <dom/html_element.h>

namespace DOM {

class HTMLElementImpl;
class DOMString;

/**
 * ??? See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/text.html#edef-BLOCKQUOTE">
 * BLOCKQUOTE element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLBlockquoteElement : public HTMLElement
{
public:
    HTMLBlockquoteElement();
    HTMLBlockquoteElement(const HTMLBlockquoteElement &other);
    HTMLBlockquoteElement(const Node &other) : HTMLElement()
        {(*this)=other;}
protected:
    HTMLBlockquoteElement(HTMLElementImpl *impl);
public:

    HTMLBlockquoteElement & operator = (const HTMLBlockquoteElement &other);
    HTMLBlockquoteElement & operator = (const Node &other);

    ~HTMLBlockquoteElement();

    /**
     * A URI designating a document that describes the reason for the
     * change. See the <a href="http://www.w3.org/TR/REC-html40/">
     * cite attribute definition </a> in HTML 4.0.
     *
     */
    DOMString cite() const;

    /**
     * see cite
     */
    void setCite( const DOMString & );
};

// --------------------------------------------------------------------------

class HTMLDivElementImpl;
class DOMString;

/**
 * Generic block container. See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/global.html#edef-DIV">
 * DIV element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLDivElement : public HTMLElement
{
public:
    HTMLDivElement();
    HTMLDivElement(const HTMLDivElement &other);
    HTMLDivElement(const Node &other) : HTMLElement()
        {(*this)=other;}
protected:
    HTMLDivElement(HTMLDivElementImpl *impl);
public:

    HTMLDivElement & operator = (const HTMLDivElement &other);
    HTMLDivElement & operator = (const Node &other);

    ~HTMLDivElement();

    /**
     * Horizontal text alignment. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-align">
     * align attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    DOMString align() const;

    /**
     * see align
     */
    void setAlign( const DOMString & );
};

// --------------------------------------------------------------------------

class HTMLHRElementImpl;
class DOMString;

/**
 * Create a horizontal rule. See the <a
 * href="http://www.w3.org/TR/REC-html40/present/graphics.html#edef-HR">
 * HR element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLHRElement : public HTMLElement
{
public:
    HTMLHRElement();
    HTMLHRElement(const HTMLHRElement &other);
    HTMLHRElement(const Node &other) : HTMLElement()
        {(*this)=other;}
protected:
    HTMLHRElement(HTMLHRElementImpl *impl);
public:

    HTMLHRElement & operator = (const HTMLHRElement &other);
    HTMLHRElement & operator = (const Node &other);

    ~HTMLHRElement();

    /**
     * Align the rule on the page. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-align-HR">
     * align attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    DOMString align() const;

    /**
     * see align
     */
    void setAlign( const DOMString & );

    /**
     * Indicates to the user agent that there should be no shading in
     * the rendering of this element. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-noshade">
     * noshade attribute definition </a> in HTML 4.0. This attribute
     * is deprecated in HTML 4.0.
     *
     */
    bool noShade() const;

    /**
     * see noShade
     */
    void setNoShade( bool );

    /**
     * The height of the rule. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-size-HR">
     * size attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    DOMString size() const;

    /**
     * see size
     */
    void setSize( const DOMString & );

    /**
     * The width of the rule. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-width-HR">
     * width attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    DOMString width() const;

    /**
     * see width
     */
    void setWidth( const DOMString & );
};

// --------------------------------------------------------------------------

class DOMString;

/**
 * For the \c H1 to \c H6 elements. See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/global.html#edef-H1">
 * H1 element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLHeadingElement : public HTMLElement
{
public:
    HTMLHeadingElement();
    HTMLHeadingElement(const HTMLHeadingElement &other);
    HTMLHeadingElement(const Node &other) : HTMLElement()
         {(*this)=other;}
protected:
    HTMLHeadingElement(HTMLElementImpl *impl);
public:

    HTMLHeadingElement & operator = (const HTMLHeadingElement &other);
    HTMLHeadingElement & operator = (const Node &other);

    ~HTMLHeadingElement();

    /**
     * Horizontal text alignment. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-align">
     * align attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    DOMString align() const;

    /**
     * see align
     */
    void setAlign( const DOMString & );
};

// --------------------------------------------------------------------------

class DOMString;

/**
 * Paragraphs. See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/text.html#edef-P"> P
 * element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLParagraphElement : public HTMLElement
{
public:
    HTMLParagraphElement();
    HTMLParagraphElement(const HTMLParagraphElement &other);
    HTMLParagraphElement(const Node &other) : HTMLElement()
         {(*this)=other;}
protected:
    HTMLParagraphElement(HTMLElementImpl *impl);
public:

    HTMLParagraphElement & operator = (const HTMLParagraphElement &other);
    HTMLParagraphElement & operator = (const Node &other);

    ~HTMLParagraphElement();

    /**
     * Horizontal text alignment. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-align">
     * align attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    DOMString align() const;

    /**
     * see align
     */
    void setAlign( const DOMString & );
};

// --------------------------------------------------------------------------

class HTMLPreElementImpl;

/**
 * Preformatted text. See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/text.html#edef-PRE">
 * PRE element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLPreElement : public HTMLElement
{
public:
    HTMLPreElement();
    HTMLPreElement(const HTMLPreElement &other);
    HTMLPreElement(const Node &other) : HTMLElement()
         {(*this)=other;}
protected:
    HTMLPreElement(HTMLPreElementImpl *impl);
public:

    HTMLPreElement & operator = (const HTMLPreElement &other);
    HTMLPreElement & operator = (const Node &other);

    ~HTMLPreElement();

    /**
     * Fixed width for content. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/text.html#adef-width-PRE">
     * width attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    long width() const;

    /**
     * see width
     */
    void setWidth( long );
};

class HTMLLayerElementImpl;

/**
 * Layer container for Netscape 4.x compatibility.
 * Behaves mostly like absolute positioned DIV-blocks.
 *
 */
class KHTML_EXPORT HTMLLayerElement : public HTMLElement
{
public:
    HTMLLayerElement();
    HTMLLayerElement(const HTMLLayerElement &other);
    HTMLLayerElement(const Node &other) : HTMLElement()
         {(*this)=other;}
protected:
    HTMLLayerElement(HTMLLayerElementImpl *impl);
public:

    HTMLLayerElement & operator = (const HTMLLayerElement &other);
    HTMLLayerElement & operator = (const Node &other);

    ~HTMLLayerElement();

    /**
     * The absolute position of the layer from the top.
     *
     */
    long top() const;

    /**
     * see top
     */
    void setTop( long );

    /**
     * The absolute position of the layer from the left.
     *
     */
    long left() const;

    /**
     * see left
     */
    void setLeft( long );

    /**
     * The visibility of layers is either "show" or "hide"
     *
     */
    DOMString visibility() const;

    /**
     * see visibility
     */
    void setVisibility( const DOMString & );

    /**
     * The background color of the layer.
     *
     */
    DOMString bgColor() const;

    /**
     * see bgColor
     */
    void setBgColor( const DOMString & );

    /**
     * The collection of sub-layers
     *
     */
    HTMLCollection layers() const;
};

} //namespace

#endif
