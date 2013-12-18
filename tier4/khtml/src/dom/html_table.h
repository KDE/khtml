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
 * Level 1 Specification (Recommendation)
 * http://www.w3.org/TR/REC-DOM-Level-1/
 * Copyright © World Wide Web Consortium , (Massachusetts Institute of
 * Technology , Institut National de Recherche en Informatique et en
 * Automatique , Keio University ). All Rights Reserved.
 *
 */
#ifndef HTML_TABLE_H
#define HTML_TABLE_H

// --------------------------------------------------------------------------
#include <khtml_export.h>
#include <dom/html_element.h>

namespace DOM {

class HTMLTableCaptionElementImpl;
class DOMString;

/**
 * Table caption See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/tables.html#edef-CAPTION">
 * CAPTION element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLTableCaptionElement : public HTMLElement
{
    friend class HTMLTableElement;

public:
    HTMLTableCaptionElement();
    HTMLTableCaptionElement(const HTMLTableCaptionElement &other);
    HTMLTableCaptionElement(const Node &other) : HTMLElement()
         {(*this)=other;}
protected:
    HTMLTableCaptionElement(HTMLTableCaptionElementImpl *impl);
public:

    HTMLTableCaptionElement & operator = (const HTMLTableCaptionElement &other);
    HTMLTableCaptionElement & operator = (const Node &other);

    ~HTMLTableCaptionElement();

    /**
     * Caption alignment with respect to the table. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-align-CAPTION">
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

class HTMLTableCellElementImpl;

/**
 * The object used to represent the \c TH and \c TD
 * elements. See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/tables.html#edef-TD">
 * TD element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLTableCellElement : public HTMLElement
{
    friend class HTMLTableElement;

public:
    HTMLTableCellElement();
    HTMLTableCellElement(const HTMLTableCellElement &other);
    HTMLTableCellElement(const Node &other) : HTMLElement()
         {(*this)=other;}
protected:
    HTMLTableCellElement(HTMLTableCellElementImpl *impl);
public:

    HTMLTableCellElement & operator = (const HTMLTableCellElement &other);
    HTMLTableCellElement & operator = (const Node &other);

    ~HTMLTableCellElement();

    /**
     * The index of this cell in the row.
     *
     */
    long cellIndex() const;

    /**
     * see cellIndex
     *
     * This function is obsolete - the cellIndex property is actually supposed to be read-only
     * (http://www.w3.org/DOM/updates/REC-DOM-Level-1-19981001-errata.html)
     */
    void setCellIndex( long  );

    /**
     * Abbreviation for header cells. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-abbr">
     * abbr attribute definition </a> in HTML 4.0.
     *
     */
    DOMString abbr() const;

    /**
     * see abbr
     */
    void setAbbr( const DOMString & );

    /**
     * Horizontal alignment of data in cell. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-align-TD">
     * align attribute definition </a> in HTML 4.0.
     *
     */
    DOMString align() const;

    /**
     * see align
     */
    void setAlign( const DOMString & );

    /**
     * Names group of related headers. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-axis">
     * axis attribute definition </a> in HTML 4.0.
     *
     */
    DOMString axis() const;

    /**
     * see axis
     */
    void setAxis( const DOMString & );

    /**
     * Cell background color. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-bgcolor">
     * bgcolor attribute definition </a> in HTML 4.0. This attribute
     * is deprecated in HTML 4.0.
     *
     */
    DOMString bgColor() const;

    /**
     * see bgColor
     */
    void setBgColor( const DOMString & );

    /**
     * Alignment character for cells in a column. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-char">
     * char attribute definition </a> in HTML 4.0.
     *
     */
    DOMString ch() const;

    /**
     * see ch
     */
    void setCh( const DOMString & );

    /**
     * Offset of alignment character. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-charoff">
     * charoff attribute definition </a> in HTML 4.0.
     *
     */
    DOMString chOff() const;

    /**
     * see chOff
     */
    void setChOff( const DOMString & );

    /**
     * Number of columns spanned by cell. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-colspan">
     * colspan attribute definition </a> in HTML 4.0.
     *
     */
    long colSpan() const;

    /**
     * see colSpan
     */
    void setColSpan( long  );

    /**
     * List of \c id attribute values for header cells.
     * See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-headers">
     * headers attribute definition </a> in HTML 4.0.
     *
     */
    DOMString headers() const;

    /**
     * see headers
     */
    void setHeaders( const DOMString & );

    /**
     * Cell height. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-height-TH">
     * height attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    DOMString height() const;

    /**
     * see height
     */
    void setHeight( const DOMString & );

    /**
     * Suppress word wrapping. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-nowrap">
     * nowrap attribute definition </a> in HTML 4.0. This attribute is
     * deprecated in HTML 4.0.
     *
     */
    bool noWrap() const;

    /**
     * see noWrap
     */
    void setNoWrap( bool );

    /**
     * Number of rows spanned by cell. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-rowspan">
     * rowspan attribute definition </a> in HTML 4.0.
     *
     */
    long rowSpan() const;

    /**
     * see rowSpan
     */
    void setRowSpan( long );

    /**
     * Scope covered by header cells. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-scope">
     * scope attribute definition </a> in HTML 4.0.
     *
     */
    DOMString scope() const;

    /**
     * see scope
     */
    void setScope( const DOMString & );

    /**
     * Vertical alignment of data in cell. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-valign">
     * valign attribute definition </a> in HTML 4.0.
     *
     */
    DOMString vAlign() const;

    /**
     * see vAlign
     */
    void setVAlign( const DOMString & );

    /**
     * Cell width. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-width-TH">
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

class HTMLTableColElementImpl;

/**
 * Regroups the \c COL and \c COLGROUP
 * elements. See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/tables.html#edef-COL">
 * COL element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLTableColElement : public HTMLElement
{
    friend class HTMLTableElement;

public:
    HTMLTableColElement();
    HTMLTableColElement(const HTMLTableColElement &other);
    HTMLTableColElement(const Node &other) : HTMLElement()
         {(*this)=other;}
protected:
    HTMLTableColElement(HTMLTableColElementImpl *impl);
public:

    HTMLTableColElement & operator = (const HTMLTableColElement &other);
    HTMLTableColElement & operator = (const Node &other);

    ~HTMLTableColElement();

    /**
     * Horizontal alignment of cell data in column. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-align-TD">
     * align attribute definition </a> in HTML 4.0.
     *
     */
    DOMString align() const;

    /**
     * see align
     */
    void setAlign( const DOMString & );

    /**
     * Alignment character for cells in a column. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-char">
     * char attribute definition </a> in HTML 4.0.
     *
     */
    DOMString ch() const;

    /**
     * see ch
     */
    void setCh( const DOMString & );

    /**
     * Offset of alignment character. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-charoff">
     * charoff attribute definition </a> in HTML 4.0.
     *
     */
    DOMString chOff() const;

    /**
     * see chOff
     */
    void setChOff( const DOMString & );

    /**
     * Indicates the number of columns in a group or affected by a
     * grouping. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-span-COL">
     * span attribute definition </a> in HTML 4.0.
     *
     */
    long span() const;

    /**
     * see span
     */
    void setSpan( long  );

    /**
     * Vertical alignment of cell data in column. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-valign">
     * valign attribute definition </a> in HTML 4.0.
     *
     */
    DOMString vAlign() const;

    /**
     * see vAlign
     */
    void setVAlign( const DOMString & );

    /**
     * Default column width. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-width-COL">
     * width attribute definition </a> in HTML 4.0.
     *
     */
    DOMString width() const;

    /**
     * see width
     */
    void setWidth( const DOMString & );
};

// --------------------------------------------------------------------------

class HTMLTableElementImpl;
class HTMLCollection;
class HTMLTableSectionElement;
class HTMLTableCaptionElement;
class HTMLElement;
class DOMString;

/**
 * The create* and delete* methods on the table allow authors to
 * construct and modify tables. HTML 4.0 specifies that only one of
 * each of the \c CAPTION , \c THEAD , and
 * \c TFOOT elements may exist in a table. Therefore, if
 * one exists, and the createTHead() or createTFoot() method is
 * called, the method returns the existing THead or TFoot element. See
 * the <a
 * href="http://www.w3.org/TR/REC-html40/struct/tables.html#edef-TABLE">
 * TABLE element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLTableElement : public HTMLElement
{
public:
    HTMLTableElement();
    HTMLTableElement(const HTMLTableElement &other);
    HTMLTableElement(const Node &other) : HTMLElement()
         {(*this)=other;}

protected:
    HTMLTableElement(HTMLTableElementImpl *impl);
public:

    HTMLTableElement & operator = (const HTMLTableElement &other);
    HTMLTableElement & operator = (const Node &other);

    ~HTMLTableElement();

    /**
     * Returns the table's \c CAPTION , or void if none
     * exists.
     *
     */
    HTMLTableCaptionElement caption() const;

    /**
     * see caption
     */
    void setCaption( const HTMLTableCaptionElement & );

    /**
     * Returns the table's \c THEAD , or \c null
     * if none exists.
     *
     */
    HTMLTableSectionElement tHead() const;

    /**
     * see tHead
     */
    void setTHead( const HTMLTableSectionElement & );

    /**
     * Returns the table's \c TFOOT , or \c null
     * if none exists.
     *
     */
    HTMLTableSectionElement tFoot() const;

    /**
     * see tFoot
     */
    void setTFoot( const HTMLTableSectionElement & );

    /**
     * Returns a collection of all the rows in the table, including
     * all in \c THEAD , \c TFOOT , all
     * \c TBODY elements.
     *
     */
    HTMLCollection rows() const;

    /**
     * Returns a collection of the table bodies (including implicit ones).
     *
     */
    HTMLCollection tBodies() const;

    /**
     * Specifies the table's position with respect to the rest of the
     * document. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-align-TABLE">
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
     * Cell background color. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-bgcolor">
     * bgcolor attribute definition </a> in HTML 4.0. This attribute
     * is deprecated in HTML 4.0.
     *
     */
    DOMString bgColor() const;

    /**
     * see bgColor
     */
    void setBgColor( const DOMString & );

    /**
     * The width of the border around the table. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-border-TABLE">
     * border attribute definition </a> in HTML 4.0.
     *
     */
    DOMString border() const;

    /**
     * see border
     */
    void setBorder( const DOMString & );

    /**
     * Specifies the horizontal and vertical space between cell
     * content and cell borders. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-cellpadding">
     * cellpadding attribute definition </a> in HTML 4.0.
     *
     */
    DOMString cellPadding() const;

    /**
     * see cellPadding
     */
    void setCellPadding( const DOMString & );

    /**
     * Specifies the horizontal and vertical separation between cells.
     * See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-cellspacing">
     * cellspacing attribute definition </a> in HTML 4.0.
     *
     */
    DOMString cellSpacing() const;

    /**
     * see cellSpacing
     */
    void setCellSpacing( const DOMString & );

    /**
     * Specifies which external table borders to render. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-frame">
     * frame attribute definition </a> in HTML 4.0.
     *
     */
    DOMString frame() const;

    /**
     * see frame
     */
    void setFrame( const DOMString & );

    /**
     * Specifies which internal table borders to render. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-rules">
     * rules attribute definition </a> in HTML 4.0.
     *
     */
    DOMString rules() const;

    /**
     * see rules
     */
    void setRules( const DOMString & );

    /**
     * Supplementary description about the purpose or structure of a
     * table. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-summary">
     * summary attribute definition </a> in HTML 4.0.
     *
     */
    DOMString summary() const;

    /**
     * see summary
     */
    void setSummary( const DOMString & );

    /**
     * Specifies the desired table width. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-width-TABLE">
     * width attribute definition </a> in HTML 4.0.
     *
     */
    DOMString width() const;

    /**
     * see width
     */
    void setWidth( const DOMString & );

    /**
     * Create a table header row or return an existing one.
     *
     * @return A new table header element ( \c THEAD ).
     *
     */
    HTMLElement createTHead (  );

    /**
     * Delete the header from the table, if one exists.
     *
     * @return
     *
     */
    void deleteTHead (  );

    /**
     * Create a table footer row or return an existing one.
     *
     * @return A footer element ( \c TFOOT ).
     *
     */
    HTMLElement createTFoot (  );

    /**
     * Delete the footer from the table, if one exists.
     *
     * @return
     *
     */
    void deleteTFoot (  );

    /**
     * Create a new table caption object or return an existing one.
     *
     * @return A \c CAPTION element.
     *
     */
    HTMLElement createCaption (  );

    /**
     * Delete the table caption, if one exists.
     *
     * @return
     *
     */
    void deleteCaption (  );

    /**
     * Insert a new empty row in the table.
     * The new row is inserted immediately before and in the same section
     * as the current indexth row in the table. If index is -1 or equal
     * to the number of rows, the new row is appended. In addition, when
     * the table is empty the row is inserted into a TBODY which is created
     * and inserted into the table.
     *  Note. A table row cannot
     *  be empty according to HTML 4.0 Recommendation.
     *
     * @param index The row number where to insert a new row.
     * The index starts from 0 and is relative to the logical order
     * (not document order) of all the rows contained inside the table.
     *
     * @return The newly created row.
     *
     */
    HTMLElement insertRow ( long index );

    /**
     * Delete a table row.
     *
     * @param index The index of the row to be deleted.
     * This index starts from 0 and is relative to the logical order
     * (not document order) of all the rows contained inside the table.
     * If the index is -1 the last row in the table is deleted.
     *
     * @return
     *
     */
    void deleteRow ( long index );
};

// --------------------------------------------------------------------------

class HTMLTableRowElementImpl;
class HTMLCollection;
class HTMLElement;
class DOMString;

/**
 * A row in a table. See the <a
 * href="http://www.w3.org/TR/REC-html40/struct/tables.html#edef-TR">
 * TR element definition </a> in HTML 4.0.
 *
 */
class KHTML_EXPORT HTMLTableRowElement : public HTMLElement
{
    friend class HTMLTableElement;

public:
    HTMLTableRowElement();
    HTMLTableRowElement(const HTMLTableRowElement &other);
    HTMLTableRowElement(const Node &other) : HTMLElement()
         {(*this)=other;}

protected:
    HTMLTableRowElement(HTMLTableRowElementImpl *impl);
public:

    HTMLTableRowElement & operator = (const HTMLTableRowElement &other);
    HTMLTableRowElement & operator = (const Node &other);

    ~HTMLTableRowElement();

    /**
     * The index of this row, relative to the entire table.
     * This is in logical order and not in document order.
     * The rowIndex does take into account sections
     * (THEAD, TFOOT or TBODY) within the table,
     * placing THEAD rows first in the index, followed by
     * TBODY rows, followed by TFOOT rows.
     */
    long rowIndex() const;

    /**
     * see rowIndex
     *
     * This function is obsolete - the rowIndex property is actually supposed to be read-only
     * (http://www.w3.org/DOM/updates/REC-DOM-Level-1-19981001-errata.html)
     */
    void setRowIndex( long  );

    /**
     * The index of this row, relative to the current section (
     * \c THEAD , \c TFOOT , or \c TBODY
     * ).
     *
     */
    long sectionRowIndex() const;

    /**
     * see sectionRowIndex
     *
     * This function is obsolete - the sectionRowIndex property is actually supposed to be read-only
     * (http://www.w3.org/DOM/updates/REC-DOM-Level-1-19981001-errata.html)
     */
    void setSectionRowIndex( long  );

    /**
     * The collection of cells in this row.
     *
     */
    HTMLCollection cells() const;

    /**
     * see cells
     *
     * This function is obsolete - the cells property is actually supposed to be read-only
     * (http://www.w3.org/DOM/updates/REC-DOM-Level-1-19981001-errata.html)
     */
    void setCells( const HTMLCollection & );

    /**
     * Horizontal alignment of data within cells of this row. See the
     * <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-align-TD">
     * align attribute definition </a> in HTML 4.0.
     *
     */
    DOMString align() const;

    /**
     * see align
     */
    void setAlign( const DOMString & );

    /**
     * Background color for rows. See the <a
     * href="http://www.w3.org/TR/REC-html40/present/graphics.html#adef-bgcolor">
     * bgcolor attribute definition </a> in HTML 4.0. This attribute
     * is deprecated in HTML 4.0.
     *
     */
    DOMString bgColor() const;

    /**
     * see bgColor
     */
    void setBgColor( const DOMString & );

    /**
     * Alignment character for cells in a column. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-char">
     * char attribute definition </a> in HTML 4.0.
     *
     */
    DOMString ch() const;

    /**
     * see ch
     */
    void setCh( const DOMString & );

    /**
     * Offset of alignment character. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-charoff">
     * charoff attribute definition </a> in HTML 4.0.
     *
     */
    DOMString chOff() const;

    /**
     * see chOff
     */
    void setChOff( const DOMString & );

    /**
     * Vertical alignment of data within cells of this row. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-valign">
     * valign attribute definition </a> in HTML 4.0.
     *
     */
    DOMString vAlign() const;

    /**
     * see vAlign
     */
    void setVAlign( const DOMString & );

    /**
     * Insert an empty \c TD cell into this row.
     * If index is -1 or equal to the number of cells, the new
     * cell is appended.
     *
     * @param index The place to insert the cell.
     *
     * @return The newly created cell.
     *
     */
    HTMLElement insertCell ( long index );

    /**
     * Delete a cell from the current row.
     *
     * @param index The index of the cell to delete, starting from 0.
     * If the index is -1 the last cell in the row is deleted.
     *
     * @return
     *
     */
    void deleteCell ( long index );
};

// --------------------------------------------------------------------------

class HTMLTableSectionElementImpl;
class HTMLCollection;
class HTMLElement;
class DOMString;

/**
 * The \c THEAD , \c TFOOT , and \c TBODY
 * elements.
 *
 */
class KHTML_EXPORT HTMLTableSectionElement : public HTMLElement
{
    friend class HTMLTableElement;

public:
    HTMLTableSectionElement();
    HTMLTableSectionElement(const HTMLTableSectionElement &other);
    HTMLTableSectionElement(const Node &other) : HTMLElement()
         {(*this)=other;}


protected:
    HTMLTableSectionElement(HTMLTableSectionElementImpl *impl);
public:

    HTMLTableSectionElement & operator = (const HTMLTableSectionElement &other);
    HTMLTableSectionElement & operator = (const Node &other);

    ~HTMLTableSectionElement();

    /**
     * Horizontal alignment of data in cells. See the \c align
     * attribute for HTMLTheadElement for details.
     *
     */
    DOMString align() const;

    /**
     * see align
     */
    void setAlign( const DOMString & );

    /**
     * Alignment character for cells in a column. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-char">
     * char attribute definition </a> in HTML 4.0.
     *
     */
    DOMString ch() const;

    /**
     * see ch
     */
    void setCh( const DOMString & );

    /**
     * Offset of alignment character. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/tables.html#adef-charoff">
     * charoff attribute definition </a> in HTML 4.0.
     *
     */
    DOMString chOff() const;

    /**
     * see chOff
     */
    void setChOff( const DOMString & );

    /**
     * Vertical alignment of data in cells. See the \c valign
     * attribute for HTMLTheadElement for details.
     *
     */
    DOMString vAlign() const;

    /**
     * see vAlign
     */
    void setVAlign( const DOMString & );

    /**
     * The collection of rows in this table section.
     *
     */
    HTMLCollection rows() const;

    /**
     * Insert a row into this section.
     * The new row is inserted immediately before the current indexth
     * row in this section. If index is -1 or equal to the number of rows
     * in this sectino, the new row is appended.
     *
     * @param index The row number where to insert a new row.
     *
     * @return The newly created row.
     *
     */
    HTMLElement insertRow ( long index );

    /**
     * Delete a row from this section.
     *
     * @param index The index of the row to be deleted,
     * or -1 to delete the last row. This index starts from 0 and is relative only
     * to the rows contained inside this section, not all the rows in the table.
     *
     */
    void deleteRow ( long index );
};

} //namespace

#endif
