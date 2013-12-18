/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2006 Maksim Orlovich (maksim@kde.org)
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
#ifndef HTML_TABLEIMPL_H
#define HTML_TABLEIMPL_H

#include "html/html_elementimpl.h"

namespace DOM {

class HTMLTableElementImpl;
class HTMLTableSectionElementImpl;
class HTMLTableSectionElement;
class HTMLTableRowElementImpl;
class HTMLTableRowElement;
class HTMLTableCellElementImpl;
class HTMLTableCellElement;
class HTMLTableColElementImpl;
class HTMLTableColElement;
class HTMLTableCaptionElementImpl;
class HTMLTableCaptionElement;
class HTMLElement;

// -------------------------------------------------------------------------

class HTMLTablePartElementImpl : public HTMLElementImpl

{
public:
    HTMLTablePartElementImpl(DocumentImpl *doc)
        : HTMLElementImpl(doc)
        { }

    virtual void parseAttribute(AttributeImpl *attr);
};

// -------------------------------------------------------------------------

class HTMLTableSectionElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableSectionElementImpl(DocumentImpl *doc, ushort tagid, bool implicit);

    ~HTMLTableSectionElementImpl();

    virtual Id id() const;

    HTMLElementImpl *insertRow ( long index, int& exceptioncode );
    void deleteRow ( long index, int& exceptioncode );

    int numRows() const;

protected:
    ushort _id;
};

// -------------------------------------------------------------------------

class HTMLTableRowElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableRowElementImpl(DocumentImpl *doc)
        : HTMLTablePartElementImpl(doc) {}

    virtual Id id() const;

    long rowIndex() const;
    long sectionRowIndex() const;

    HTMLElementImpl *insertCell ( long index, int &exceptioncode );
    void deleteCell ( long index, int &exceptioncode );

protected:
    int ncols;
};

// -------------------------------------------------------------------------

class HTMLTableCellElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableCellElementImpl(DocumentImpl *doc, int tagId);
    ~HTMLTableCellElementImpl();

    long cellIndex() const;

    int col() const { return _col; }
    void setCol(int col) { _col = col; }
    int row() const { return _row; }
    void setRow(int r) { _row = r; }

    int colSpan() const { return cSpan; }
    int rowSpan() const { return rSpan; }

    virtual Id id() const { return _id; }
    virtual void parseAttribute(AttributeImpl *attr);
    virtual void attach();

protected:
    int _row;
    int _col;
    int rSpan;
    int cSpan;
    int _id;
    int rowHeight;
    bool m_solid        : 1;
    bool m_nowrap : 1;
};

// -------------------------------------------------------------------------

class HTMLTableColElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableColElementImpl(DocumentImpl *doc, ushort i);

    virtual Id id() const;

    void setTable(HTMLTableElementImpl *t) { table = t; }

    // overrides
    virtual void parseAttribute(AttributeImpl *attr);

    int span() const { return _span; }

protected:
    // could be ID_COL or ID_COLGROUP ... The DOM is not quite clear on
    // this, but since both elements work quite similar, we use one
    // DOMElement for them...
    ushort _id;
    int _span;
    HTMLTableElementImpl *table;
};

// -------------------------------------------------------------------------

class HTMLTableCaptionElementImpl : public HTMLTablePartElementImpl
{
public:
    HTMLTableCaptionElementImpl(DocumentImpl *doc)
        : HTMLTablePartElementImpl(doc) {}

    virtual Id id() const;
    virtual void parseAttribute(AttributeImpl *attr);
};

// -------------------------------------------------------------------------

/*
This class helps memorize pointers to child objects that may be
yanked around via the DOM. It always picks the first pointer of the
given type.

The pointer it stores can have 3 meanings:
0      -- no child
parent -- no idea about the state
other  -- pointer to the child
*/
template<typename ChildType, int ChildId> class ChildHolder
{
public:
    ChildHolder():ptr(0) {}

    ChildType* get(const ElementImpl* parent) const {
        if (static_cast<const NodeImpl *>(ptr) == parent) {
            //Do lookup.
            ptr = 0;
            for (NodeImpl* child = parent->firstChild(); child; child = child->nextSibling())
                if (child->id() == ChildId) {
                    ptr = static_cast<ElementImpl*>(child);
                    break;
                }
        }
        return static_cast<ChildType*>(ptr);
    }

    void childAdded(ElementImpl* parent, NodeImpl* child) {
        if (ptr)
            ptr = parent; //No clue now..
        else
            ptr = child;
    }

    void childAppended(NodeImpl* child) {
        if (!ptr)
            ptr = child;
    }

    void childRemoved(ElementImpl* parent, NodeImpl* child) {
        if (child == ptr)
            ptr = parent; //We removed what was pointing - no clue now..
        //else things are unchanged.
    }

    void clear() {
        ptr = 0;
    }

    void operator =(ChildType* child) {
        ptr = child;
    }
private:
    mutable NodeImpl* ptr;
};

// -------------------------------------------------------------------------
class HTMLTableElementImpl : public HTMLElementImpl
{
public:
    enum Rules {
        None    = 0x00,
        RGroups = 0x01,
        CGroups = 0x02,
        Groups  = 0x03,
        Rows    = 0x05,
        Cols    = 0x0a,
        All     = 0x0f
    };
    enum Frame {
        Void   = 0x00,
        Above  = 0x01,
        Below  = 0x02,
        Lhs    = 0x04,
        Rhs    = 0x08,
        Hsides = 0x03,
        Vsides = 0x0c,
        Box    = 0x0f
    };

    HTMLTableElementImpl(DocumentImpl *doc);
    ~HTMLTableElementImpl();

    virtual Id id() const;

    HTMLTableCaptionElementImpl *caption() const { return tCaption.get(this); }
    NodeImpl *setCaption( HTMLTableCaptionElementImpl * );

    HTMLTableSectionElementImpl *tHead() const { return head.get(this); }
    NodeImpl *setTHead( HTMLTableSectionElementImpl * );

    HTMLTableSectionElementImpl *tFoot() const { return foot.get(this); }
    NodeImpl *setTFoot( HTMLTableSectionElementImpl * );

    NodeImpl *setTBody( HTMLTableSectionElementImpl * );

    HTMLElementImpl *createTHead (  );
    void deleteTHead (  );
    HTMLElementImpl *createTFoot (  );
    void deleteTFoot (  );
    HTMLElementImpl *createCaption (  );
    void deleteCaption (  );
    HTMLElementImpl *insertRow ( long index, int &exceptioncode );
    void deleteRow ( long index, int &exceptioncode );

    // overrides
    virtual NodeImpl *addChild(NodeImpl *child);
    virtual NodeImpl *insertBefore ( NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode );
    virtual void      replaceChild ( NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode );
    virtual void      removeChild ( NodeImpl *oldChild, int &exceptioncode );
    virtual void      removeChildren();
    virtual NodeImpl *appendChild ( NodeImpl *newChild, int &exceptioncode );
    
    virtual void parseAttribute(AttributeImpl *attr);
    virtual void attach();
    virtual void close();

    /* Tries to find the section containing row number outIndex.
       Returns whether it succeeded or not. negative outIndex values
       are interpreted as being infinite.

       On success, outSection, outIndex points to section, and index in that
       section.

       On failure, outSection points to the last section of the table, and
       index is the offset the row would have if there was an additional section.
    */
    bool findRowSection(long inIndex,
                        HTMLTableSectionElementImpl*& outSection,
                        long&                         outIndex) const;
protected:
    //Actual implementations of keeping things in place.
    void handleChildAdd   ( NodeImpl *newChild );
    void handleChildAppend( NodeImpl *newChild );
    void handleChildRemove( NodeImpl *oldChild );

    void updateFrame();

    ChildHolder<HTMLTableSectionElementImpl, ID_THEAD> head;
    ChildHolder<HTMLTableSectionElementImpl, ID_TFOOT> foot;
    ChildHolder<HTMLTableSectionElementImpl, ID_TBODY> firstBody;
    ChildHolder<HTMLTableCaptionElementImpl, ID_CAPTION> tCaption;

    HTMLTableSectionElementImpl *tFirstBody() const { return firstBody.get(this); }

    KDE_BF_ENUM(Frame) frame : 4;
    KDE_BF_ENUM(Rules) rules : 4;

    bool m_solid        : 1;
    uint unused		: 7;
    ushort padding	: 16;
    friend class HTMLTableCellElementImpl;
};


} //namespace

#endif

