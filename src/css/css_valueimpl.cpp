/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2008 Apple Computer, Inc.
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2009 Germain Garand (germain@ebooksfrance.org)
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

#include "css_valueimpl.h"
#include "css_ruleimpl.h"
#include "css_stylesheetimpl.h"
#include "css/csshelper.h"
#include "cssparser.h"
#include "cssproperties.h"
#include "cssvalues.h"

#include <dom/css_value.h>
#include <dom/dom_exception.h>
#include <dom/dom_string.h>

#include <xml/dom_stringimpl.h>
#include <xml/dom_docimpl.h>

#include <misc/loader.h>

#include <rendering/font.h>
#include <rendering/render_style.h>

#include <wtf/ASCIICType.h>

#include <QDebug>
#include <QtCore/QRegExp>
#include <QPaintDevice>

// Hack for debugging purposes
extern DOM::DOMString getPropertyName(unsigned short id);

using khtml::FontDef;

using namespace DOM;
using namespace WTF;

static int propertyID(const DOMString &s)
{
    char buffer[maxCSSPropertyNameLength];

    unsigned len = s.length();
    if (len > maxCSSPropertyNameLength)
        return 0;

    for (unsigned i = 0; i != len; ++i) {
        unsigned short c = s[i].unicode();
        if (c == 0 || c >= 0x7F)
            return 0; // illegal character
        buffer[i] = s[i].toLower().unicode();
    }

    return getPropertyID(buffer, len);
}

// "ident" from the CSS tokenizer, minus backslash-escape sequences
static bool isCSSTokenizerIdentifier(const DOMString& string)
{
    const QChar* p = string.unicode();
    const QChar* end = p + string.length();

    // -?
    if (p != end && p[0] == '-')
        ++p;

    // {nmstart}
    if (p == end || !(p[0] == '_' || p[0] >= 128 || isASCIIAlpha(p->unicode())))
        return false;
    ++p;

    // {nmchar}*
    for (; p != end; ++p) {
        if (!(p[0] == '_' || p[0] == '-' || p[0] >= 128 || isASCIIAlphanumeric(p->unicode())))
            return false;
    }

    return true;
}

static DOMString quoteString(const DOMString &string)
{
    // FIXME: Also need to transform control characters into \ sequences.
    QString s = string.string();
    s.replace('\\', "\\\\");
    s.replace('\'', "\\'");
    return QString('\'' + s + '\'');
}

// Quotes the string if it needs quoting.
static DOMString quoteStringIfNeeded(const DOMString &string)
{
    return isCSSTokenizerIdentifier(string) ? string : quoteString(string);
}

CSSStyleDeclarationImpl::CSSStyleDeclarationImpl(CSSRuleImpl *parent)
    : StyleBaseImpl(parent)
{
    m_lstValues = 0;
    m_node = 0;
}

CSSStyleDeclarationImpl::CSSStyleDeclarationImpl(CSSRuleImpl *parent, QList<CSSProperty*> *lstValues)
    : StyleBaseImpl(parent)
{
    m_lstValues = lstValues;
    m_node = 0;
}

CSSStyleDeclarationImpl&  CSSStyleDeclarationImpl::operator= (const CSSStyleDeclarationImpl& o)
{
    if (this == &o) return *this;

    // don't attach it to the same node, just leave the current m_node value
    if (m_lstValues)
        qDeleteAll(*m_lstValues);
    delete m_lstValues;
    m_lstValues = 0;
    if (o.m_lstValues) {
        m_lstValues = new QList<CSSProperty*>;
        QListIterator<CSSProperty*> lstValuesIt(*o.m_lstValues);
        while ( lstValuesIt.hasNext() )
            m_lstValues->append(new CSSProperty(*lstValuesIt.next()));
    }

    return *this;
}

CSSStyleDeclarationImpl::~CSSStyleDeclarationImpl()
{
    if (m_lstValues)
        qDeleteAll( *m_lstValues );
    delete m_lstValues;
    // we don't use refcounting for m_node, to avoid cyclic references (see ElementImpl)
}

CSSValueImpl *CSSStyleDeclarationImpl::getPropertyCSSValue(const DOMString &propertyName) const
{
    int propID = propertyID(propertyName);
    if (!propID)
        return 0;
    return getPropertyCSSValue(propID);
}

DOMString CSSStyleDeclarationImpl::getPropertyValue(const DOMString &propertyName) const
{
    int propID = propertyID(propertyName);
    if (!propID)
        return DOMString();
    return getPropertyValue(propID);
}

DOMString CSSStyleDeclarationImpl::getPropertyPriority(const DOMString &propertyName) const
{
    int propID = propertyID(propertyName);
    if (!propID)
        return DOMString();
    return getPropertyPriority(propID) ? "important" : "";
}

void CSSStyleDeclarationImpl::setProperty(const DOMString &propertyName, const DOMString &value, const DOMString &priority)
{
    int propID = propertyID(propertyName);
    if (!propID) // set exception?
        return;
    bool important = priority.string().indexOf("important", 0, Qt::CaseInsensitive) != -1;
    setProperty(propID, value, important);
}

DOMString CSSStyleDeclarationImpl::removeProperty(const DOMString &propertyName)
{
    int propID = propertyID(propertyName);
    if (!propID)
        return DOMString();
    DOMString old;
    removeProperty(propID, &old);
    return old;
}

DOMString CSSStyleDeclarationImpl::getPropertyValue( int propertyID ) const
{
    if(!m_lstValues) return DOMString();
    CSSValueImpl* value = getPropertyCSSValue( propertyID );
    if ( value )
        return value->cssText();

    // Shorthand and 4-values properties
    switch ( propertyID ) {
    case CSS_PROP_BACKGROUND_POSITION:
    {
        // ## Is this correct? The code in cssparser.cpp is confusing
        const int properties[2] = { CSS_PROP_BACKGROUND_POSITION_X,
                                    CSS_PROP_BACKGROUND_POSITION_Y };
        return getLayeredShortHandValue( properties, 2 );
    }
    case CSS_PROP_BACKGROUND:
    {
        const int properties[6] = { CSS_PROP_BACKGROUND_IMAGE, CSS_PROP_BACKGROUND_REPEAT,
                                    CSS_PROP_BACKGROUND_ATTACHMENT,CSS_PROP_BACKGROUND_POSITION_X,
                                    CSS_PROP_BACKGROUND_POSITION_Y, CSS_PROP_BACKGROUND_COLOR };
        return getLayeredShortHandValue( properties, 6 );
    }
    case CSS_PROP_BORDER:
    {
            const int properties[3][4] = {{ CSS_PROP_BORDER_TOP_WIDTH,
                                            CSS_PROP_BORDER_RIGHT_WIDTH,
                                            CSS_PROP_BORDER_BOTTOM_WIDTH,
                                            CSS_PROP_BORDER_LEFT_WIDTH },
                                          { CSS_PROP_BORDER_TOP_STYLE,
                                            CSS_PROP_BORDER_RIGHT_STYLE,
                                            CSS_PROP_BORDER_BOTTOM_STYLE,
                                            CSS_PROP_BORDER_LEFT_STYLE },
                                          { CSS_PROP_BORDER_TOP_COLOR,
                                            CSS_PROP_BORDER_RIGHT_COLOR,
                                            CSS_PROP_BORDER_LEFT_COLOR,
                                            CSS_PROP_BORDER_BOTTOM_COLOR }};
            DOMString res;
            const int nrprops = sizeof(properties) / sizeof(properties[0]);
            for (int i = 0; i < nrprops; ++i) {
                DOMString value = getCommonValue(properties[i], 4);
                if (!value.isNull()) {
                    if (!res.isNull())
                        res += " ";
                    res += value;
                }
            }
            return res;

    }
    case CSS_PROP_BORDER_TOP:
    {
        const int properties[3] = { CSS_PROP_BORDER_TOP_WIDTH, CSS_PROP_BORDER_TOP_STYLE,
                                    CSS_PROP_BORDER_TOP_COLOR};
        return getShortHandValue( properties, 3 );
    }
    case CSS_PROP_BORDER_RIGHT:
    {
        const int properties[3] = { CSS_PROP_BORDER_RIGHT_WIDTH, CSS_PROP_BORDER_RIGHT_STYLE,
                                    CSS_PROP_BORDER_RIGHT_COLOR};
        return getShortHandValue( properties, 3 );
    }
    case CSS_PROP_BORDER_BOTTOM:
    {
        const int properties[3] = { CSS_PROP_BORDER_BOTTOM_WIDTH, CSS_PROP_BORDER_BOTTOM_STYLE,
                                    CSS_PROP_BORDER_BOTTOM_COLOR};
        return getShortHandValue( properties, 3 );
    }
    case CSS_PROP_BORDER_LEFT:
    {
        const int properties[3] = { CSS_PROP_BORDER_LEFT_WIDTH, CSS_PROP_BORDER_LEFT_STYLE,
                                    CSS_PROP_BORDER_LEFT_COLOR};
        return getShortHandValue( properties, 3 );
    }
    case CSS_PROP_OUTLINE:
    {
        const int properties[3] = { CSS_PROP_OUTLINE_WIDTH, CSS_PROP_OUTLINE_STYLE,
                                    CSS_PROP_OUTLINE_COLOR };
        return getShortHandValue( properties, 3 );
    }
    case CSS_PROP_BORDER_COLOR:
    {
        const int properties[4] = { CSS_PROP_BORDER_TOP_COLOR, CSS_PROP_BORDER_RIGHT_COLOR,
                                    CSS_PROP_BORDER_BOTTOM_COLOR, CSS_PROP_BORDER_LEFT_COLOR };
        return get4Values( properties );
    }
    case CSS_PROP_BORDER_WIDTH:
    {
        const int properties[4] = { CSS_PROP_BORDER_TOP_WIDTH, CSS_PROP_BORDER_RIGHT_WIDTH,
                                    CSS_PROP_BORDER_BOTTOM_WIDTH, CSS_PROP_BORDER_LEFT_WIDTH };
        return get4Values( properties );
    }
    case CSS_PROP_BORDER_STYLE:
    {
        const int properties[4] = { CSS_PROP_BORDER_TOP_STYLE, CSS_PROP_BORDER_RIGHT_STYLE,
                                    CSS_PROP_BORDER_BOTTOM_STYLE, CSS_PROP_BORDER_LEFT_STYLE };
        return get4Values( properties );
    }
    case CSS_PROP_MARGIN:
    {
        const int properties[4] = { CSS_PROP_MARGIN_TOP, CSS_PROP_MARGIN_RIGHT,
                                    CSS_PROP_MARGIN_BOTTOM, CSS_PROP_MARGIN_LEFT };
        return get4Values( properties );
    }
    case CSS_PROP_PADDING:
    {
        const int properties[4] = { CSS_PROP_PADDING_TOP, CSS_PROP_PADDING_RIGHT,
                                    CSS_PROP_PADDING_BOTTOM, CSS_PROP_PADDING_LEFT };
        return get4Values( properties );
    }
    case CSS_PROP_LIST_STYLE:
    {
        const int properties[3] = { CSS_PROP_LIST_STYLE_TYPE, CSS_PROP_LIST_STYLE_POSITION,
                                    CSS_PROP_LIST_STYLE_IMAGE };
        return getShortHandValue( properties, 3 );
    }
    }
    //qDebug() << "property not found:" << propertyID;
    return DOMString();
}
 
// only returns a non-null value if all properties have the same, non-null value
DOMString CSSStyleDeclarationImpl::getCommonValue(const int* properties, int number) const
{
    DOMString res;
    for (int i = 0; i < number; ++i) {
        CSSValueImpl* value = getPropertyCSSValue(properties[i]);
        if (!value)
            return DOMString();
        DOMString text = value->cssText();
        if (text.isNull())
            return DOMString();
        if (res.isNull())
            res = text;
        else if (res != text)
            return DOMString();
    }
    return res;
}

DOMString CSSStyleDeclarationImpl::get4Values( const int* properties ) const
{
    DOMString res;
    for ( int i = 0 ; i < 4 ; ++i ) {
        if (!isPropertyImplicit(properties[i])) {
            CSSValueImpl* value = getPropertyCSSValue( properties[i] );
            if ( !value ) { // apparently all 4 properties must be specified.
                return DOMString();
            }
            if ( i > 0 )
                res += " ";
            res += value->cssText();
        }
    }
    return res;
}

DOMString CSSStyleDeclarationImpl::getLayeredShortHandValue(const int* properties, unsigned number) const
{
    DOMString res;
    unsigned i;
    unsigned j;

    // Begin by collecting the properties into an array.
    QVector<CSSValueImpl*> values(number);
    unsigned numLayers = 0;

    for (i = 0; i < number; ++i) {
        values[i] = getPropertyCSSValue(properties[i]);
        if (values[i]) {
            if (values[i]->isValueList()) {
                CSSValueListImpl* valueList = static_cast<CSSValueListImpl*>(values[i]);
                numLayers = qMax(valueList->length(), (unsigned long)numLayers);
            } else
                numLayers = qMax(1U, numLayers);
        }
    }

    // Now stitch the properties together.  Implicit initial values are flagged as such and
    // can safely be omitted.
    for (i = 0; i < numLayers; i++) {
        DOMString layerRes;
        for (j = 0; j < number; j++) {
            CSSValueImpl* value = 0;
            if (values[j]) {
                if (values[j]->isValueList())
                    value = static_cast<CSSValueListImpl*>(values[j])->item(i);
                else {
                    value = values[j];

                    // Color only belongs in the last layer.
                    if (properties[j] == CSS_PROP_BACKGROUND_COLOR) {
                        if (i != numLayers - 1)
                            value = 0;
                    } else if (i != 0) // Other singletons only belong in the first layer.
                        value = 0;
                }
            }

            if (value && !value->isImplicitInitialValue()) {
                if (!layerRes.isNull())
                    layerRes += " ";
                layerRes += value->cssText();
            }
        }

        if (!layerRes.isNull()) {
            if (!res.isNull())
                res += ", ";
            res += layerRes;
        }
    }

    return res;
}

DOMString CSSStyleDeclarationImpl::getShortHandValue( const int* properties, int number ) const
{
    DOMString res;
    for ( int i = 0 ; i < number ; ++i ) {
        CSSValueImpl* value = getPropertyCSSValue( properties[i] );
        if ( value ) { // TODO provide default value if !value
            if ( !res.isNull() )
                res += " ";
            res += value->cssText();
        }
    }
    return res;
}

 CSSValueImpl *CSSStyleDeclarationImpl::getPropertyCSSValue( int propertyID ) const
{
    if(!m_lstValues) return 0;

    QListIterator<CSSProperty*> lstValuesIt(*m_lstValues);
    CSSProperty *current;
    while ( lstValuesIt.hasNext() ) {
        current = lstValuesIt.next();
        if (current->m_id == propertyID)
            return current->value();
    }
    return 0;
}

bool CSSStyleDeclarationImpl::isPropertyImplicit(int propertyID) const
{
    QListIterator<CSSProperty*> lstValuesIt(*m_lstValues);
    CSSProperty const *current;
    while ( lstValuesIt.hasNext() ) {
        current = lstValuesIt.next();
        if (current->m_id == propertyID)
            return current->isImplicit();
    }
    return false;
}

// --------------- Shorthands mapping ----------------

// In order top be able to remove a shorthand property,
// we need a reverse mapping from the shorthands to their composing properties.

// ### Warning: keep in sync when introducing new shorthands.

struct PropertyLonghand {
    PropertyLonghand()
        : m_properties(0)
        , m_length(0)
    {
    }

    PropertyLonghand(const int* firstProperty, unsigned numProperties)
        : m_properties(firstProperty)
        , m_length(numProperties)
    {
    }

    const int* properties() const { return m_properties; }
    unsigned length() const { return m_length; }

private:
    const int* m_properties;
    unsigned m_length;
};

static void initShorthandMap(QHash<int, PropertyLonghand>& shorthandMap)
{
    #define SET_SHORTHAND_MAP_ENTRY(map, propID, array) \
        map.insert(propID, PropertyLonghand(array, sizeof(array) / sizeof(array[0])))

    // FIXME: The 'font' property has "shorthand nature" but is not parsed as a shorthand.

    // Do not change the order of the following four shorthands, and keep them together.
    static const int borderProperties[4][3] = {
        { CSS_PROP_BORDER_TOP_COLOR, CSS_PROP_BORDER_TOP_STYLE, CSS_PROP_BORDER_TOP_WIDTH },
        { CSS_PROP_BORDER_RIGHT_COLOR, CSS_PROP_BORDER_RIGHT_STYLE, CSS_PROP_BORDER_RIGHT_WIDTH },
        { CSS_PROP_BORDER_BOTTOM_COLOR, CSS_PROP_BORDER_BOTTOM_STYLE, CSS_PROP_BORDER_BOTTOM_WIDTH },
        { CSS_PROP_BORDER_LEFT_COLOR, CSS_PROP_BORDER_LEFT_STYLE, CSS_PROP_BORDER_LEFT_WIDTH }
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_TOP, borderProperties[0]);
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_RIGHT, borderProperties[1]);
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_BOTTOM, borderProperties[2]);
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_LEFT, borderProperties[3]);

    shorthandMap.insert(CSS_PROP_BORDER, PropertyLonghand(borderProperties[0], sizeof(borderProperties) / sizeof(borderProperties[0][0])));

    static const int borderColorProperties[] = {
        CSS_PROP_BORDER_TOP_COLOR,
        CSS_PROP_BORDER_RIGHT_COLOR,
        CSS_PROP_BORDER_BOTTOM_COLOR,
        CSS_PROP_BORDER_LEFT_COLOR
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_COLOR, borderColorProperties);

    static const int borderStyleProperties[] = {
        CSS_PROP_BORDER_TOP_STYLE,
        CSS_PROP_BORDER_RIGHT_STYLE,
        CSS_PROP_BORDER_BOTTOM_STYLE,
        CSS_PROP_BORDER_LEFT_STYLE
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_STYLE, borderStyleProperties);

    static const int borderWidthProperties[] = {
        CSS_PROP_BORDER_TOP_WIDTH,
        CSS_PROP_BORDER_RIGHT_WIDTH,
        CSS_PROP_BORDER_BOTTOM_WIDTH,
        CSS_PROP_BORDER_LEFT_WIDTH
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_WIDTH, borderWidthProperties);

    static const int backgroundPositionProperties[] = { CSS_PROP_BACKGROUND_POSITION_X, CSS_PROP_BACKGROUND_POSITION_Y };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BACKGROUND_POSITION, backgroundPositionProperties);

    static const int borderSpacingProperties[] = { CSS_PROP__KHTML_BORDER_HORIZONTAL_SPACING, CSS_PROP__KHTML_BORDER_VERTICAL_SPACING };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_SPACING, borderSpacingProperties);

    static const int listStyleProperties[] = {
        CSS_PROP_LIST_STYLE_IMAGE,
        CSS_PROP_LIST_STYLE_POSITION,
        CSS_PROP_LIST_STYLE_TYPE
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_LIST_STYLE, listStyleProperties);

    static const int marginProperties[] = {
        CSS_PROP_MARGIN_TOP,
        CSS_PROP_MARGIN_RIGHT,
        CSS_PROP_MARGIN_BOTTOM,
        CSS_PROP_MARGIN_LEFT
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_MARGIN, marginProperties);

#ifdef APPLE_CHANGES
    static const int marginCollapseProperties[] = { CSS_PROP__KHTML_MARGIN_TOP_COLLAPSE, CSS_PROP__KHTML_MARGIN_BOTTOM_COLLAPSE };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP__KHTML_MARGIN_COLLAPSE, marginCollapseProperties);
#endif

    static const int marqueeProperties[] = {
        CSS_PROP__KHTML_MARQUEE_DIRECTION,
        CSS_PROP__KHTML_MARQUEE_INCREMENT,
        CSS_PROP__KHTML_MARQUEE_REPETITION,
        CSS_PROP__KHTML_MARQUEE_STYLE,
        CSS_PROP__KHTML_MARQUEE_SPEED
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP__KHTML_MARQUEE, marqueeProperties);

    static const int outlineProperties[] = {
        CSS_PROP_OUTLINE_COLOR,
        CSS_PROP_OUTLINE_OFFSET,
        CSS_PROP_OUTLINE_STYLE,
        CSS_PROP_OUTLINE_WIDTH
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_OUTLINE, outlineProperties);

    static const int paddingProperties[] = {
        CSS_PROP_PADDING_TOP,
        CSS_PROP_PADDING_RIGHT,
        CSS_PROP_PADDING_BOTTOM,
        CSS_PROP_PADDING_LEFT
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_PADDING, paddingProperties);

#ifdef APPLE_CHANGES
    static const int textStrokeProperties[] = { CSS_PROP__KHTML_TEXT_STROKE_COLOR, CSS_PROP__KHTML_TEXT_STROKE_WIDTH };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP__KHTML_TEXT_STROKE, textStrokeProperties);
#endif

    static const int backgroundProperties[] = {
        CSS_PROP_BACKGROUND_ATTACHMENT,
        CSS_PROP_BACKGROUND_COLOR,
        CSS_PROP_BACKGROUND_IMAGE,
        CSS_PROP_BACKGROUND_POSITION_X,
        CSS_PROP_BACKGROUND_POSITION_Y,
        CSS_PROP_BACKGROUND_REPEAT,
        CSS_PROP__KHTML_BACKGROUND_SIZE,
        CSS_PROP_BACKGROUND_SIZE
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BACKGROUND, backgroundProperties);

#ifdef APPLE_CHANGES
    static const int columnsProperties[] = { CSS_PROP__KHTML_COLUMN_WIDTH, CSS_PROP__KHTML_COLUMN_COUNT };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP__KHTML_COLUMNS, columnsProperties);

    static const int columnRuleProperties[] = {
        CSS_PROP__KHTML_COLUMN_RULE_COLOR,
        CSS_PROP__KHTML_COLUMN_RULE_STYLE,
        CSS_PROP__KHTML_COLUMN_RULE_WIDTH
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP__KHTML_COLUMN_RULE, columnRuleProperties);
#endif

    static const int overflowProperties[] = { CSS_PROP_OVERFLOW_X, CSS_PROP_OVERFLOW_Y };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_OVERFLOW, overflowProperties);

    static const int borderRadiusProperties[] = {
        CSS_PROP_BORDER_TOP_RIGHT_RADIUS,
        CSS_PROP_BORDER_TOP_LEFT_RADIUS,
        CSS_PROP_BORDER_BOTTOM_LEFT_RADIUS,
        CSS_PROP_BORDER_BOTTOM_RIGHT_RADIUS
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_BORDER_RADIUS, borderRadiusProperties);
    static const int prefixedBorderRadiusProperties[] = {
        CSS_PROP__KHTML_BORDER_TOP_RIGHT_RADIUS,
        CSS_PROP__KHTML_BORDER_TOP_LEFT_RADIUS,
        CSS_PROP__KHTML_BORDER_BOTTOM_LEFT_RADIUS,
        CSS_PROP__KHTML_BORDER_BOTTOM_RIGHT_RADIUS
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP__KHTML_BORDER_RADIUS, prefixedBorderRadiusProperties);
                                                
    static const int markerProperties[] = {
        CSS_PROP_MARKER_START, 
        CSS_PROP_MARKER_MID,
        CSS_PROP_MARKER_END
    };
    SET_SHORTHAND_MAP_ENTRY(shorthandMap, CSS_PROP_MARKER, markerProperties);

    #undef SET_SHORTHAND_MAP_ENTRY
}

// -------------------------------------------

void CSSStyleDeclarationImpl::removeProperty(int propertyID,
                                             DOM::DOMString* old)
{
    if(!m_lstValues)
        return;

    static QHash<int, PropertyLonghand> shorthandMap;
    if (shorthandMap.isEmpty())
        initShorthandMap(shorthandMap);

    PropertyLonghand longhand = shorthandMap.value(propertyID);
    if (longhand.length()) {
        removePropertiesInSet(longhand.properties(), longhand.length());
        // FIXME: Return an equivalent shorthand when possible.
        return;
    }

    QMutableListIterator<CSSProperty*> lstValuesIt(*m_lstValues);
    CSSProperty *current;
    lstValuesIt.toBack();
    while ( lstValuesIt.hasPrevious() ) {
        current = lstValuesIt.previous();
        if (current->m_id == propertyID) {
            if (old)
                *old = current->value()->cssText();
            delete lstValuesIt.value();
            lstValuesIt.remove();
            setChanged();
            break;
        }
     }
}

void CSSStyleDeclarationImpl::removePropertiesInSet(const int* set, unsigned length)
{
    bool changed = false;
    for (unsigned i = 0; i < length; i++) {
        QMutableListIterator<CSSProperty*> lstValuesIt(*m_lstValues);
        CSSProperty *current;
        lstValuesIt.toBack();
        while ( lstValuesIt.hasPrevious() ) {
            current = lstValuesIt.previous();
            if (current->m_id == set[i]) {
                delete lstValuesIt.value();
                lstValuesIt.remove();
                changed = true;
                break;
            }
        }
    }
    if (changed)
        setChanged();
}

void CSSStyleDeclarationImpl::setChanged()
{
    if (m_node) {
        m_node->setChanged();
        return;
    }

    // ### quick&dirty hack for KDE 3.0... make this MUCH better! (Dirk)
    for (StyleBaseImpl* stylesheet = this; stylesheet; stylesheet = stylesheet->parent())
        if (stylesheet->isCSSStyleSheet()) {
            static_cast<CSSStyleSheetImpl*>(stylesheet)->doc()->updateStyleSelector();
            break;
        }
}

void CSSStyleDeclarationImpl::clear()
{
    if (!m_lstValues)
        return;

    QMutableListIterator<CSSProperty*> it(*m_lstValues);
    while (it.hasNext()) {
            delete it.next();
            it.remove();
    }
}

bool CSSStyleDeclarationImpl::getPropertyPriority( int propertyID ) const
{
    if ( m_lstValues) {
	QListIterator<CSSProperty*> lstValuesIt(*m_lstValues);
	CSSProperty *current;
	while (lstValuesIt.hasNext()) {
            current = lstValuesIt.next();
	    if( propertyID == current->m_id )
		return current->m_important;
	}
    }
    return false;
}

bool CSSStyleDeclarationImpl::setProperty(int id, const DOMString &value, bool important, int &ec)
{
    ec = 0;

    // Setting the value to an empty string just removes the property in both IE and Gecko.
    // Setting it to null seems to produce less consistent results, but we treat it just the same.
    if (value.isEmpty()) {
        removeProperty(id);
        return true;
    }

    bool success = setProperty(id, value, important);
#if 0
    if (!success) {
        // CSS DOM requires raising SYNTAX_ERR here, but this is too dangerous for compatibility,
        // see <http://bugs.webkit.org/show_bug.cgi?id=7296>.
    }
#endif
    return success;
}

bool CSSStyleDeclarationImpl::setProperty(int id, const DOMString &value, bool important)
{
    if(!m_lstValues) {
	m_lstValues = new QList<CSSProperty*>;
    }

    CSSParser parser( strictParsing );
    bool success = parser.parseValue( this, id, value, important );
    if(!success) {
	// qDebug() << "CSSStyleDeclarationImpl::setProperty invalid property: [" << getPropertyName(id).string()
			//<< "] value: [" << value.string() << "]"<< endl;
    } else
        setChanged();
    return success;
}

void CSSStyleDeclarationImpl::setProperty(int id, int value, bool important )
{
    if(!m_lstValues) {
	m_lstValues = new QList<CSSProperty*>;
    }
    removeProperty(id);

    CSSValueImpl * cssValue = new CSSPrimitiveValueImpl(value);
    setParsedValue(id, cssValue, important, m_lstValues);
    setChanged();
}

void CSSStyleDeclarationImpl::setLengthProperty(int id, const DOM::DOMString &value, bool important, bool _multiLength )
{
    bool parseMode = strictParsing;
    strictParsing = false;
    multiLength = _multiLength;
    setProperty( id, value, important );
    strictParsing = parseMode;
    multiLength = false;
}

void CSSStyleDeclarationImpl::setProperty ( const DOMString &propertyString)
{
    if(!m_lstValues) {
	m_lstValues = new QList<CSSProperty*>;
    }

    CSSParser parser( strictParsing );
    parser.parseDeclaration( this, propertyString );
    setChanged();
}

unsigned long CSSStyleDeclarationImpl::length() const
{
    return m_lstValues ? m_lstValues->count() : 0;
}

DOMString CSSStyleDeclarationImpl::item( unsigned long index ) const
{
    if(m_lstValues && index < (unsigned)m_lstValues->count() && m_lstValues->at(index))
	return getPropertyName(m_lstValues->at(index)->m_id);
    return DOMString();
}

CSSRuleImpl *CSSStyleDeclarationImpl::parentRule() const
{
    return (m_parent && m_parent->isRule() ) ?
	static_cast<CSSRuleImpl *>(m_parent) : 0;
}

DOM::DOMString CSSStyleDeclarationImpl::cssText() const
{
    DOMString result;

    const CSSProperty* positionXProp = 0;
    const CSSProperty* positionYProp = 0;

    if ( m_lstValues) {
	QListIterator<CSSProperty*> lstValuesIt(*m_lstValues);
	while (lstValuesIt.hasNext()) {
	    const CSSProperty* cur = lstValuesIt.next();
            if (cur->id() == CSS_PROP_BACKGROUND_POSITION_X)
                positionXProp = cur;
            else if (cur->id() == CSS_PROP_BACKGROUND_POSITION_Y)
                positionYProp = cur;
            else
                result += cur->cssText();
	}
    }

    // FIXME: This is a not-so-nice way to turn x/y positions into single background-position in output.
    // It is required because background-position-x/y are non-standard properties and generated output
    // would not work in Firefox
    // It would be a better solution if background-position was CSS_PAIR.
    if (positionXProp && positionYProp && positionXProp->isImportant() == positionYProp->isImportant()) {
        DOMString positionValue;
        const int properties[2] = { CSS_PROP_BACKGROUND_POSITION_X, CSS_PROP_BACKGROUND_POSITION_Y };
        if (positionXProp->value()->isValueList() || positionYProp->value()->isValueList())
            positionValue = getLayeredShortHandValue(properties, 2);
        else
            positionValue = positionXProp->value()->cssText() + DOMString(" ") + positionYProp->value()->cssText();
        result += DOMString("background-position: ") + positionValue
                                                     + DOMString((positionXProp->isImportant() ? " !important" : ""))
                                                     + DOMString("; ");
    } else {
        if (positionXProp)
            result += positionXProp->cssText();
        if (positionYProp)
            result += positionYProp->cssText();
    }
    return result;
}

void CSSStyleDeclarationImpl::setCssText(const DOM::DOMString& text)
{
    if (m_lstValues) {
        qDeleteAll(*m_lstValues);
	m_lstValues->clear();
    } else {
	m_lstValues = new QList<CSSProperty*>;
    }

    CSSParser parser( strictParsing );
    parser.parseDeclaration( this, text );
    setChanged();
}

bool CSSStyleDeclarationImpl::parseString( const DOMString &/*string*/, bool )
{
    // qDebug() << "WARNING: CSSStyleDeclarationImpl::parseString, unimplemented, was called";
    return false;
    // ###
}

// --------------------------------------------------------------------------------------

void CSSInlineStyleDeclarationImpl::setChanged()
{
    if (m_node)
        m_node->setNeedsStyleAttributeUpdate();
    CSSStyleDeclarationImpl::setChanged();
}

void CSSInlineStyleDeclarationImpl::updateFromAttribute(const DOMString &value)
{
    if(!m_lstValues) {
	m_lstValues = new QList<CSSProperty*>;
    } else {
        clear();
    }
    CSSParser parser( strictParsing );
    parser.parseDeclaration( this, value );
    CSSStyleDeclarationImpl::setChanged();
}

// --------------------------------------------------------------------------------------

unsigned short CSSInheritedValueImpl::cssValueType() const
{
    return CSSValue::CSS_INHERIT;
}

DOM::DOMString CSSInheritedValueImpl::cssText() const
{
    return DOMString("inherit");
}

unsigned short CSSInitialValueImpl::cssValueType() const
{
    return CSSValue::CSS_INITIAL;
}

DOM::DOMString CSSInitialValueImpl::cssText() const
{
    return DOMString("initial");
}

// ----------------------------------------------------------------------------------------

CSSValueListImpl::~CSSValueListImpl()
{
    for (QListIterator<CSSValueImpl*> iterator(m_values); iterator.hasNext();)
        iterator.next()->deref();
}

unsigned short CSSValueListImpl::cssValueType() const
{
    return CSSValue::CSS_VALUE_LIST;
}

void CSSValueListImpl::append(CSSValueImpl *val)
{
    m_values.append(val);
    val->ref();
}

DOM::DOMString CSSValueListImpl::cssText() const
{
    DOMString result = "";

    for (QListIterator<CSSValueImpl*> iterator(m_values); iterator.hasNext();) {
        if (!result.isEmpty()) {
            if (m_separator == Comma)
                result += ", ";
            else if (m_separator == Space)
                result += " ";
        }
        result += iterator.next()->cssText();
    }

    return result;
}

// -------------------------------------------------------------------------------------

CSSPrimitiveValueImpl::CSSPrimitiveValueImpl()
    : CSSValueImpl()
{
    m_type = 0;
}

CSSPrimitiveValueImpl::CSSPrimitiveValueImpl(int ident)
    : CSSValueImpl()
{
    m_value.ident = ident;
    m_type = CSSPrimitiveValue::CSS_IDENT;
}

CSSPrimitiveValueImpl::CSSPrimitiveValueImpl(double num, CSSPrimitiveValue::UnitTypes type)
{
    m_value.num = num;
    m_type = type;
}

CSSPrimitiveValueImpl::CSSPrimitiveValueImpl(const DOMString &str, CSSPrimitiveValue::UnitTypes type)
{
    m_value.string = str.implementation();
    if(m_value.string) m_value.string->ref();
    m_type = type;
}

CSSPrimitiveValueImpl::CSSPrimitiveValueImpl(CounterImpl *c)
{
    m_value.counter = c;
    if (m_value.counter)
	m_value.counter->ref();
    m_type = CSSPrimitiveValue::CSS_COUNTER;
}

CSSPrimitiveValueImpl::CSSPrimitiveValueImpl( RectImpl *r)
{
    m_value.rect = r;
    if (m_value.rect)
	m_value.rect->ref();
    m_type = CSSPrimitiveValue::CSS_RECT;
}

CSSPrimitiveValueImpl::CSSPrimitiveValueImpl(QRgb color)
{
    m_value.rgbcolor = color;
    m_type = CSSPrimitiveValue::CSS_RGBCOLOR;
}

CSSPrimitiveValueImpl::CSSPrimitiveValueImpl(PairImpl *p)
{
    m_value.pair = p;
    if (m_value.pair)
	m_value.pair->ref();
    m_type = CSSPrimitiveValue::CSS_PAIR;
}


CSSPrimitiveValueImpl::~CSSPrimitiveValueImpl()
{
    cleanup();
}

void CSSPrimitiveValueImpl::cleanup()
{
    switch(m_type) {
    case CSSPrimitiveValue::CSS_STRING:
    case CSSPrimitiveValue::CSS_URI:
    case CSSPrimitiveValue::CSS_ATTR:
	if(m_value.string) m_value.string->deref();
        break;
    case CSSPrimitiveValue::CSS_COUNTER:
	m_value.counter->deref();
        break;
    case CSSPrimitiveValue::CSS_RECT:
	m_value.rect->deref();
        break;
    case CSSPrimitiveValue::CSS_PAIR:
        m_value.pair->deref();
        break;
    default:
        break;
    }

    m_type = 0;
}

int CSSPrimitiveValueImpl::computeLength( khtml::RenderStyle *style, int logicalDpiY)
{
    return snapValue( computeLengthFloat( style, logicalDpiY ) );
}

double CSSPrimitiveValueImpl::computeLengthFloat( khtml::RenderStyle *style, int logicalDpiY)
{
    unsigned short type = primitiveType();

    double dpiY = 72.; // fallback
    if ( logicalDpiY )
        dpiY = logicalDpiY;
    if ( !khtml::printpainter && dpiY < 96 )
        dpiY = 96.;

    double factor = 1.;
    switch(type)
    {
    case CSSPrimitiveValue::CSS_EMS:
        factor = style->font().pixelSize();
        break;
    case CSSPrimitiveValue::CSS_EXS:
	{
        QFontMetrics fm = style->fontMetrics();
        factor = fm.xHeight();
        break;
	}
    case CSSPrimitiveValue::CSS_PX:
        break;
    case CSSPrimitiveValue::CSS_CM:
	factor = dpiY/2.54; //72dpi/(2.54 cm/in)
        break;
    case CSSPrimitiveValue::CSS_MM:
	factor = dpiY/25.4;
        break;
    case CSSPrimitiveValue::CSS_IN:
        factor = dpiY;
        break;
    case CSSPrimitiveValue::CSS_PT:
        factor = dpiY/72.;
        break;
    case CSSPrimitiveValue::CSS_PC:
        // 1 pc == 12 pt
        factor = dpiY*12./72.;
        break;
    default:
        return -1;
    }

    return floatValue(type)*factor;
}

int CSSPrimitiveValueImpl::getDPIResolution() const
{
    unsigned short type = primitiveType();
    double factor = 1.;
    switch(type)
    {
    case CSSPrimitiveValue::CSS_DPI:
        break;
    case CSSPrimitiveValue::CSS_DPCM:
        factor = 2.54;
        break;
    default:
        return -1;
    }

    return (int)(0.01+floatValue(type)*factor);
}

void CSSPrimitiveValueImpl::setFloatValue( unsigned short unitType, double floatValue, int &exceptioncode )
{
    exceptioncode = 0;
    cleanup();
    // ### check if property supports this type
    if(m_type > CSSPrimitiveValue::CSS_DIMENSION) {
	exceptioncode = CSSException::SYNTAX_ERR + CSSException::_EXCEPTION_OFFSET;
	return;
    }
    //if(m_type > CSSPrimitiveValue::CSS_DIMENSION) throw DOMException(DOMException::INVALID_ACCESS_ERR);
    m_value.num = floatValue;
    m_type = unitType;
}

void CSSPrimitiveValueImpl::setStringValue( unsigned short stringType, const DOMString &stringValue, int &exceptioncode )
{
    exceptioncode = 0;
    cleanup();
    //if(m_type < CSSPrimitiveValue::CSS_STRING) throw DOMException(DOMException::INVALID_ACCESS_ERR);
    //if(m_type > CSSPrimitiveValue::CSS_ATTR) throw DOMException(DOMException::INVALID_ACCESS_ERR);
    if(m_type < CSSPrimitiveValue::CSS_STRING || m_type > CSSPrimitiveValue::CSS_ATTR) {
	exceptioncode = CSSException::SYNTAX_ERR + CSSException::_EXCEPTION_OFFSET;
	return;
    }
    if(stringType != CSSPrimitiveValue::CSS_IDENT)
    {
	m_value.string = stringValue.implementation();
	m_value.string->ref();
	m_type = stringType;
    }
    // ### parse ident
}

unsigned short CSSPrimitiveValueImpl::cssValueType() const
{
    return CSSValue::CSS_PRIMITIVE_VALUE;
}

bool CSSPrimitiveValueImpl::parseString( const DOMString &/*string*/, bool )
{
    // ###
    // qDebug() << "WARNING: CSSPrimitiveValueImpl::parseString, unimplemented, was called";
    return false;
}

int CSSPrimitiveValueImpl::getIdent()
{
    if(m_type != CSSPrimitiveValue::CSS_IDENT) return 0;
    return m_value.ident;
}

DOM::DOMString CSSPrimitiveValueImpl::cssText() const
{
    // ### return the original value instead of a generated one (e.g. color
    // name if it was specified) - check what spec says about this
    DOMString text;
    switch ( m_type ) {
	case CSSPrimitiveValue::CSS_UNKNOWN:
	    // ###
	    break;
	case CSSPrimitiveValue::CSS_NUMBER:
	    // We want to output integral values w/o a period, but others as-is
	    if ( m_value.num == (int)m_value.num )
		text = DOMString(QString::number( (int)m_value.num ));
	    else
		text = DOMString(QString::number( m_value.num ));
	    break;
	case CSSPrimitiveValue::CSS_PERCENTAGE:
	    text = DOMString(QString::number( m_value.num ) + "%");
	    break;
	case CSSPrimitiveValue::CSS_EMS:
	    text = DOMString(QString::number( m_value.num ) + "em");
	    break;
	case CSSPrimitiveValue::CSS_EXS:
	    text = DOMString(QString::number( m_value.num ) + "ex");
	    break;
	case CSSPrimitiveValue::CSS_PX:
	    text = DOMString(QString::number( m_value.num ) + "px");
	    break;
	case CSSPrimitiveValue::CSS_CM:
	    text = DOMString(QString::number( m_value.num ) + "cm");
	    break;
	case CSSPrimitiveValue::CSS_MM:
	    text = DOMString(QString::number( m_value.num ) + "mm");
	    break;
	case CSSPrimitiveValue::CSS_IN:
	    text = DOMString(QString::number( m_value.num ) + "in");
	    break;
	case CSSPrimitiveValue::CSS_PT:
	    text = DOMString(QString::number( m_value.num ) + "pt");
	    break;
	case CSSPrimitiveValue::CSS_PC:
	    text = DOMString(QString::number( m_value.num ) + "pc");
	    break;
	case CSSPrimitiveValue::CSS_DEG:
	    text = DOMString(QString::number( m_value.num ) + "deg");
	    break;
	case CSSPrimitiveValue::CSS_RAD:
	    text = DOMString(QString::number( m_value.num ) + "rad");
	    break;
	case CSSPrimitiveValue::CSS_GRAD:
	    text = DOMString(QString::number( m_value.num ) + "grad");
	    break;
	case CSSPrimitiveValue::CSS_MS:
	    text = DOMString(QString::number( m_value.num ) + "ms");
	    break;
	case CSSPrimitiveValue::CSS_S:
	    text = DOMString(QString::number( m_value.num ) + "s");
	    break;
	case CSSPrimitiveValue::CSS_HZ:
	    text = DOMString(QString::number( m_value.num ) + "hz");
	    break;
	case CSSPrimitiveValue::CSS_KHZ:
	    text = DOMString(QString::number( m_value.num ) + "khz");
	    break;
	case CSSPrimitiveValue::CSS_DIMENSION:
	    // ###
	    break;
	case CSSPrimitiveValue::CSS_STRING:
	    text = quoteStringIfNeeded(m_value.string);
	    break;
	case CSSPrimitiveValue::CSS_URI:
            text  = "url(";
	    text += DOMString( m_value.string );
            text += ")";
	    break;
	case CSSPrimitiveValue::CSS_IDENT:
	    text = getValueName(m_value.ident);
	    break;
	case CSSPrimitiveValue::CSS_ATTR:
            text = "attr(";
            text += DOMString( m_value.string );
            text += ")";
	    break;
	case CSSPrimitiveValue::CSS_COUNTER:
            text = "counter(";
            text += m_value.counter->m_identifier;
            text += ")";
            // ### add list-style and separator
	    break;
	case CSSPrimitiveValue::CSS_RECT:
        {
            RectImpl* rectVal = getRectValue();
            text = "rect(";
            text += rectVal->top()->cssText() + DOMString(" ");
            text += rectVal->right()->cssText() + DOMString(" ");
            text += rectVal->bottom()->cssText() + DOMString(" ");
            text += rectVal->left()->cssText() + DOMString(")");
            break;
        }
	case CSSPrimitiveValue::CSS_RGBCOLOR:
	    if (qAlpha(m_value.rgbcolor) != 0xFF) {
		if (m_value.rgbcolor == khtml::transparentColor)
		    text = "transparent";
		else
                    text = QString("rgba(" + QString::number(qRed  (m_value.rgbcolor)) + ", "
				   + QString::number(qGreen(m_value.rgbcolor)) + ", "
				   + QString::number(qBlue (m_value.rgbcolor)) + ", "
                                   + QString::number(qAlpha(m_value.rgbcolor)/255.0) + ")");
	    } else {
                text = QString("rgb(" + QString::number(qRed  (m_value.rgbcolor)) + ", "
                              + QString::number(qGreen(m_value.rgbcolor)) + ", "
                              + QString::number(qBlue (m_value.rgbcolor)) + ")");
	    }
	    break;
        case CSSPrimitiveValue::CSS_PAIR:
            text = m_value.pair->first()->cssText();
            text += " ";
            text += m_value.pair->second()->cssText();
            break;
	default:
	    break;
    }
    return text;
}

// -----------------------------------------------------------------

RectImpl::RectImpl()
{
    m_top = 0;
    m_right = 0;
    m_bottom = 0;
    m_left = 0;
}

RectImpl::~RectImpl()
{
    if (m_top) m_top->deref();
    if (m_right) m_right->deref();
    if (m_bottom) m_bottom->deref();
    if (m_left) m_left->deref();
}

void RectImpl::setTop( CSSPrimitiveValueImpl *top )
{
    if( top ) top->ref();
    if ( m_top ) m_top->deref();
    m_top = top;
}

void RectImpl::setRight( CSSPrimitiveValueImpl *right )
{
    if( right ) right->ref();
    if ( m_right ) m_right->deref();
    m_right = right;
}

void RectImpl::setBottom( CSSPrimitiveValueImpl *bottom )
{
    if( bottom ) bottom->ref();
    if ( m_bottom ) m_bottom->deref();
    m_bottom = bottom;
}

void RectImpl::setLeft( CSSPrimitiveValueImpl *left )
{
    if( left ) left->ref();
    if ( m_left ) m_left->deref();
    m_left = left;
}

// -----------------------------------------------------------------

PairImpl::~PairImpl()
{
    if (m_first) m_first->deref(); if (m_second) m_second->deref();
}

void PairImpl::setFirst(CSSPrimitiveValueImpl* first)
{
    if (first == m_first) return;
    if (m_first) m_first->deref();
    m_first = first;
    if (m_first) m_first->ref();
}

void PairImpl::setSecond(CSSPrimitiveValueImpl* second)
{
    if (second == m_second) return;
    if (m_second) m_second->deref();
    m_second = second;
    if (m_second) m_second->ref();
}

// -----------------------------------------------------------------

CSSImageValueImpl::CSSImageValueImpl(const DOMString &url, StyleBaseImpl* style)
    : CSSPrimitiveValueImpl(url, CSSPrimitiveValue::CSS_URI)
{
    khtml::DocLoader *docLoader = 0;
    const StyleBaseImpl *root = style;
    while (root->parent())
	root = root->parent();
    if (root->isCSSStyleSheet())
	docLoader = static_cast<const CSSStyleSheetImpl*>(root)->docLoader();
    if (docLoader) {
    QUrl fullURL( style->baseURL().resolved(QUrl(khtml::parseURL(url).string())) );
    m_image = docLoader->requestImage( fullURL.url() );
    if(m_image) m_image->ref(this);
}
}

CSSImageValueImpl::CSSImageValueImpl()
    : CSSPrimitiveValueImpl(CSS_VAL_NONE)
{
    m_image = 0;
}

CSSImageValueImpl::~CSSImageValueImpl()
{
    if(m_image) m_image->deref(this);
}

// ------------------------------------------------------------------------

FontFamilyValueImpl::FontFamilyValueImpl( const QString &string)
: CSSPrimitiveValueImpl( DOMString(string), CSSPrimitiveValue::CSS_STRING)
{
    static const QRegExp parenReg(" \\(.*\\)$");
//  static const QRegExp braceReg(" \\[.*\\]$");

    parsedFontName = string;

    // a language tag is often added in braces at the end. Remove it.
    parsedFontName.replace(parenReg, QString());

#if 0
    // cannot use such early checks against the font database anymore,
    // as the font subsystem might not contain the requested font yet
    // (case of downloadable font faces)

    // remove [Xft] qualifiers
    parsedFontName.replace(braceReg, QString());

    const QString &available = KHTMLSettings::availableFamilies();

    parsedFontName = parsedFontName.toLower();
    // qDebug() << "searching for face '" << parsedFontName << "'";

    int pos = available.indexOf( ',' + parsedFontName + ',', 0, Qt::CaseInsensitive );
    if ( pos == -1 ) {
        // many pages add extra MSs to make sure it's windows only ;(
        if ( parsedFontName.startsWith( "ms " ) )
            parsedFontName = parsedFontName.mid( 3 );
        if ( parsedFontName.endsWith( " ms" ) )
            parsedFontName.truncate( parsedFontName.length() - 3 );
        pos = available.indexOf( ",ms " + parsedFontName + ',', 0, Qt::CaseInsensitive );
        if ( pos == -1 )
            pos = available.indexOf( ',' + parsedFontName + " ms,", 0, Qt::CaseInsensitive );
    }

    if ( pos != -1 ) {
       ++pos;
       int p = available.indexOf(',', pos);
       assert( p != -1 ); // available is supposed to start and end with ,
       parsedFontName = available.mid( pos, p - pos);
       // qDebug() << "going for '" << parsedFontName << "'";
    }

#endif // !APPLE_CHANGES
}

FontValueImpl::FontValueImpl()
    : style(0), variant(0), weight(0), size(0), lineHeight(0), family(0)
{
}

FontValueImpl::~FontValueImpl()
{
    delete style;
    delete variant;
    delete weight;
    delete size;
    delete lineHeight;
    delete family;
}

DOMString FontValueImpl::cssText() const
{
    // font variant weight size / line-height family

    DOMString result("");

    if (style) {
	result += style->cssText();
    }
    if (variant) {
	if (result.length() > 0) {
	    result += " ";
	}
	result += variant->cssText();
    }
    if (weight) {
	if (result.length() > 0) {
	    result += " ";
	}
	result += weight->cssText();
    }
    if (size) {
	if (result.length() > 0) {
	    result += " ";
	}
	result += size->cssText();
    }
    if (lineHeight) {
	if (!size) {
	    result += " ";
	}
	result += "/";
	result += lineHeight->cssText();
    }
    if (family) {
	if (result.length() > 0) {
	    result += " ";
	}
	result += family->cssText();
    }

    return result;
}

QuotesValueImpl::QuotesValueImpl()
    : levels(0)
{
}

DOMString QuotesValueImpl::cssText() const
{
    return QString("\"" + data.join("\" \"") + "\"");
}

void QuotesValueImpl::addLevel(const QString& open, const QString& close)
{
    data.append(open);
    data.append(close);
    levels++;
}

QString QuotesValueImpl::openQuote(int level) const
{
    if (levels == 0) return "";
    level--; // increments are calculated before openQuote is called
//     qDebug() << "Open quote level:" << level;
    if (level < 0) level = 0;
    else
    if (level >= (int) levels) level = (int) (levels-1);
    return data[level*2];
}

QString QuotesValueImpl::closeQuote(int level) const
{
    if (levels == 0) return "";
//     qDebug() << "Close quote level:" << level;
    if (level < 0) level = 0;
    else
    if (level >= (int) levels) level = (int) (levels-1);
    return data[level*2+1];
}

// Used for text-shadow and box-shadow
ShadowValueImpl::ShadowValueImpl(CSSPrimitiveValueImpl* _x, CSSPrimitiveValueImpl* _y,
                                 CSSPrimitiveValueImpl* _blur, CSSPrimitiveValueImpl* _color)
    :x(_x), y(_y), blur(_blur), color(_color)
{}

ShadowValueImpl::~ShadowValueImpl()
{
    delete x;
    delete y;
    delete blur;
    delete color;
}

DOMString ShadowValueImpl::cssText() const
{
    DOMString text("");
    if (color) {
        text += color->cssText();
    }
    if (x) {
        if (text.length() > 0) {
            text += " ";
        }
        text += x->cssText();
    }
    if (y) {
        if (text.length() > 0) {
            text += " ";
        }
        text += y->cssText();
    }
    if (blur) {
        if (text.length() > 0) {
            text += " ";
        }
        text += blur->cssText();
    }

    return text;
}

DOMString CounterActImpl::cssText() const
{
    DOMString text(m_counter);
    text += DOMString(QString::number(m_value));

    return text;
}

DOMString CSSProperty::cssText() const
{
    return getPropertyName(m_id) + DOMString(": ") + m_value->cssText() + (m_important ? DOMString(" !important") : DOMString()) + DOMString("; ");
}

// -------------------------------------------------------------------------

#if 0
    // ENABLE(SVG_FONTS)
bool CSSFontFaceSrcValueImpl::isSVGFontFaceSrc() const
{
    return !strcasecmp(m_format, "svg");
}
#endif

bool CSSFontFaceSrcValueImpl::isSupportedFormat() const
{
    // Normally we would just check the format, but in order to avoid conflicts with the old WinIE style of font-face,
    // we will also check to see if the URL ends with .eot.  If so, we'll go ahead and assume that we shouldn't load it.
    if (m_format.isEmpty()) {
        // Check for .eot.
        if (m_resource.endsWith(".eot") || m_resource.endsWith(".EOT"))
            return false;
        return true;
    }

    return !strcasecmp(m_format, "truetype") || !strcasecmp(m_format, "opentype") || !strcasecmp(m_format, "woff")
#if 0
    //ENABLE(SVG_FONTS)
           || isSVGFontFaceSrc()
#endif
           ;
}

DOMString CSSFontFaceSrcValueImpl::cssText() const
{
    DOMString result;
    if (isLocal())
        result += "local(";
    else
        result += "url(";
    result += m_resource;
    result += ")";
    if (!m_format.isEmpty()) {
        result += " format(";
        result += m_format;
        result += ")";
    }
    return result;
}

