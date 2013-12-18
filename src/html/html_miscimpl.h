/*
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
 *
 */
#ifndef HTML_MISCIMPL_H
#define HTML_MISCIMPL_H

#include "html_elementimpl.h"
#include "xml/dom_nodelistimpl.h"
#include "misc/shared.h"

namespace DOM {

class Node;
class DOMString;
class HTMLCollection;

class HTMLBaseFontElementImpl : public HTMLElementImpl
{
public:
    HTMLBaseFontElementImpl(DocumentImpl *doc);

    ~HTMLBaseFontElementImpl();

    virtual Id id() const;
};

// -------------------------------------------------------------------------

class HTMLCollectionImpl : public DynamicNodeListImpl
{
    friend class DOM::HTMLCollection;
public:
    enum Type {
        // from HTMLDocument
        DOC_IMAGES = LAST_NODE_LIST + 1, // all IMG elements in the document
        DOC_APPLETS,   // all OBJECT and APPLET elements
        DOC_FORMS,     // all FORMS
        DOC_LAYERS,    // all LAYERS
        DOC_LINKS,     // all A _and_ AREA elements with a value for href
        DOC_ANCHORS,      // all A elements with a value for name
        DOC_SCRIPTS,   // all SCRIPT elements
        // from HTMLTable, HTMLTableSection, HTMLTableRow
        TABLE_ROWS,    // all rows in this table
        TABLE_TBODIES, // all TBODY elements in this table
        TSECTION_ROWS, // all rows elements in this table section
        TR_CELLS,      // all CELLS in this row
        // from SELECT
        SELECT_OPTIONS,
        // from HTMLMap
        MAP_AREAS,
        FORMLESS_INPUT, // input elements that do not have form associated w/them
        DOC_ALL,        // "all" elements (IE)
        NODE_CHILDREN,   // first-level children (IE)
        FORM_ELEMENTS,   // input elements in a form
        WINDOW_NAMED_ITEMS,
        DOCUMENT_NAMED_ITEMS,
        LAST_TYPE
    };

    HTMLCollectionImpl(NodeImpl *_base, int _tagId);

    virtual NodeImpl *item ( unsigned long index ) const;

    // obsolete and not domtree changes save
    virtual NodeImpl *firstItem() const;
    virtual NodeImpl *nextItem() const;

    virtual NodeImpl *namedItem ( const DOMString &name ) const;
    // In case of multiple items named the same way
    virtual NodeImpl *nextNamedItem( const DOMString &name ) const;

    QList<NodeImpl*> namedItems( const DOMString &name ) const;

    int getType() const {
        return type;
    }

    NodeImpl* base() {
      return m_refNode;
    }
protected:
    virtual unsigned long calcLength(NodeImpl *start) const;

    // The collection list the following elements
    int type:8;

    // Reimplemented from DynamicNodeListImpl
    virtual bool nodeMatches( NodeImpl *testNode, bool& doRecurse ) const;

    // Helper for name iteration: checks whether ID matches,
    // and inserts any name-matching things into namedItemsWithName
    bool checkForNameMatch(NodeImpl *node, const DOMString &name) const;
};

// this whole class is just a big hack to find form elements even in
// malformed HTML elements
// the famous <table><tr><form><td> problem
class HTMLFormCollectionImpl : public HTMLCollectionImpl
{
public:
    // base must inherit HTMLGenericFormElementImpl or this won't work
    HTMLFormCollectionImpl(NodeImpl* _base);
    ~HTMLFormCollectionImpl() { }

    virtual NodeImpl *item ( unsigned long index ) const;

    virtual NodeImpl *namedItem ( const DOMString &name ) const;
    // In case of multiple items named the same way
    virtual NodeImpl *nextNamedItem( const DOMString &name ) const;
protected:
    virtual unsigned long calcLength( NodeImpl *start ) const;

private:
    mutable unsigned currentNamePos;
    mutable unsigned currentNameImgPos;
    mutable bool foundInput;
};

/*
 Special collection for items of given name/id under document. or window.; but using
 iteration interface
*/
class HTMLMappedNameCollectionImpl : public HTMLCollectionImpl
{
public:
    HTMLMappedNameCollectionImpl(NodeImpl* _base, int type, const DOMString& name);
    virtual bool nodeMatches( NodeImpl *testNode, bool& doRecurse ) const;

    static bool matchesName( ElementImpl* el, int type, const DOMString& name);
private:
    DOMString name;
};

} //namespace

#endif
