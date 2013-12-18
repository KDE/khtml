/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright 1999 Waldo Bastian (bastian@kde.org)
 * Copyright 2001 Andreas Schlapbach (schlpbch@iam.unibe.ch)
 * Copyright 2001-2003 Dirk Mueller (mueller@kde.org)
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
 */

//#define CSS_DEBUG

#include "css_base.h"

#include <assert.h>
#include <QDebug>

#ifdef CSS_DEBUG
#include "cssproperties.h"
#endif

#include "css_stylesheetimpl.h"
#include <xml/dom_docimpl.h>
#include "css_valueimpl.h"
using namespace DOM;

void StyleBaseImpl::checkLoaded() const
{
    if(m_parent) m_parent->checkLoaded();
}

void StyleBaseImpl::checkPending() const
{
    if(m_parent) m_parent->checkPending();
} 

StyleSheetImpl* StyleBaseImpl::stylesheet()
{
    StyleBaseImpl* b = this;
    while(b && !b->isStyleSheet())
        b = b->m_parent;
    return static_cast<StyleSheetImpl *>(b);
}

QUrl StyleBaseImpl::baseURL()
{
    // try to find the style sheet. If found look for its url.
    // If it has none, look for the parentsheet, or the parentNode and
    // try to find out about their url

    StyleSheetImpl *sheet = stylesheet();

    if(!sheet) return QUrl();

    if(!sheet->href().isNull())
        return QUrl( sheet->href().string() );

    // find parent
    if(sheet->parent()) return sheet->parent()->baseURL();

    if(!sheet->ownerNode()) return QUrl();

    return sheet->ownerNode()->document()->baseURL();
}

void StyleBaseImpl::setParsedValue(int propId, const CSSValueImpl *parsedValue,
				   bool important, QList<CSSProperty*> *propList)
{
    QMutableListIterator<CSSProperty*> propIt(*propList);
    propIt.toBack(); // just remove the top one - not sure what should happen if we have multiple instances of the property
    CSSProperty* p;
    while (propIt.hasPrevious()) {
        p = propIt.previous();
        if (p->m_id == propId && p->m_important == important ) {
            delete propIt.value();
            propIt.remove();
            break;
        }
    }

    CSSProperty *prop = new CSSProperty();
    prop->m_id = propId;
    prop->setValue(const_cast<CSSValueImpl *>(parsedValue));
    prop->m_important = important;

    propList->append(prop);
#ifdef CSS_DEBUG
    qDebug() << "added property: " << getPropertyName(propId).string()
                    // non implemented yet << ", value: " << parsedValue->cssText().string()
                    << " important: " << prop->m_important;
#endif
}

// ------------------------------------------------------------------------------

StyleListImpl::~StyleListImpl()
{
    StyleBaseImpl *n;

    if(!m_lstChildren) return;

    QListIterator<StyleBaseImpl*> it( *m_lstChildren );
    while ( it.hasNext() )
    {
        n = it.next();
        n->setParent(0);
        if( !n->refCount() ) delete n;
    }
    delete m_lstChildren;
}

// --------------------------------------------------------------------------------

void CSSSelector::print(void)
{
    // qDebug() << "[Selector: tag = " <<       QString::number(makeId(tagNamespace.id(), tagLocalName.id()),16) << ", attr = \"" << makeId(attrNamespace.id(), attrLocalName.id()) << "\", match = \"" << match
		//    << "\" value = \"" << value.string().string().toLatin1().constData() << "\" relation = " << (int)relation
		//    << "]" << endl;
    if ( tagHistory )
        tagHistory->print();
    // qDebug() << "    specificity = " << specificity();
}

unsigned int CSSSelector::specificity() const
{

    int s = ((tagLocalName.id() == anyLocalName) ? 0 : 1);
    switch(match)
    {
    case Id:
	s += 0x10000;
	break;
    case Exact:
    case Set:
    case List:
    case Class:
    case Hyphen:
    case PseudoClass:
    case PseudoElement:
    case Contain:
    case Begin:
    case End:
        s += 0x100;
    case None:
        break;
    }
    if(tagHistory)
        s += tagHistory->specificity();
    // make sure it doesn't overflow
    return s & 0xffffff;
}

void CSSSelector::extractPseudoType() const
{
    if (match != PseudoClass && match != PseudoElement)
        return;
    _pseudoType = PseudoOther;
    bool element = false;
    bool compat = false;
    if (!value.isEmpty()) {
        value = value.string().lower();
        switch (value[0].unicode()) {
            case '-':
                if (value == "-khtml-replaced")
                    _pseudoType = PseudoReplaced;
                else
                if (value == "-khtml-marker")
                    _pseudoType = PseudoMarker;
                element = true;
                break;
            case 'a':
                if (value == "active")
                    _pseudoType = PseudoActive;
                else if (value == "after") {
                    _pseudoType = PseudoAfter;
                    element = compat = true;
                }
                break;
            case 'b':
                if (value == "before") {
                    _pseudoType = PseudoBefore;
                    element = compat = true;
                }
                break;
            case 'c':
                if (value == "checked")
                    _pseudoType = PseudoChecked;
                else if (value == "contains(")
                    _pseudoType = PseudoContains;
                break;
            case 'd':
                if (value == "disabled")
                    _pseudoType = PseudoDisabled;
                if (value == "default")
                    _pseudoType = PseudoDefault;
                break;
            case 'e':
                if (value == "empty")
                    _pseudoType = PseudoEmpty;
                else if (value == "enabled")
                    _pseudoType = PseudoEnabled;
                break;
            case 'f':
                if (value == "first-child")
                    _pseudoType = PseudoFirstChild;
                else if (value == "first-letter") {
                    _pseudoType = PseudoFirstLetter;
                    element = compat = true;
                }
                else if (value == "first-line") {
                    _pseudoType = PseudoFirstLine;
                    element = compat = true;
                }
                else if (value == "first-of-type")
                    _pseudoType = PseudoFirstOfType;
                else if (value == "focus")
                    _pseudoType = PseudoFocus;
                break;
            case 'h':
                if (value == "hover")
                    _pseudoType = PseudoHover;
                break;
            case 'i':
                if (value == "indeterminate")
                    _pseudoType = PseudoIndeterminate;
                break;
            case 'l':
                if (value == "link")
                    _pseudoType = PseudoLink;
                else if (value == "lang(")
                    _pseudoType = PseudoLang;
                else if (value == "last-child")
                    _pseudoType = PseudoLastChild;
                else if (value == "last-of-type")
                    _pseudoType = PseudoLastOfType;
                break;
            case 'n':
                if (value == "not(")
                    _pseudoType = PseudoNot;
                else if (value == "nth-child(")
                    _pseudoType = PseudoNthChild;
                else if (value == "nth-last-child(")
                    _pseudoType = PseudoNthLastChild;
                else if (value == "nth-of-type(")
                    _pseudoType = PseudoNthOfType;
                else if (value == "nth-last-of-type(")
                    _pseudoType = PseudoNthLastOfType;
                break;
            case 'o':
                if (value == "only-child")
                    _pseudoType = PseudoOnlyChild;
                else if (value == "only-of-type")
                    _pseudoType = PseudoOnlyOfType;
                break;
            case 'r':
                if (value == "root")
                    _pseudoType = PseudoRoot;
                else if (value == "read-only")
                    _pseudoType = PseudoReadOnly;
                else if (value == "read-write")
                    _pseudoType = PseudoReadWrite;
                break;
            case 's':
                if (value == "selection") {
                    _pseudoType = PseudoSelection;
                    element = true;
                }
                break;
            case 't':
                if (value == "target")
                    _pseudoType = PseudoTarget;
                break;
            case 'v':
                if (value == "visited")
                    _pseudoType = PseudoVisited;
                break;
        }
    }
    if (match == PseudoClass && element)
        if (!compat) _pseudoType = PseudoOther;
        else match = PseudoElement;
    else
    if (match == PseudoElement && !element)
        _pseudoType = PseudoOther;
}


bool CSSSelector::operator == ( const CSSSelector &other ) const
{
    const CSSSelector *sel1 = this;
    const CSSSelector *sel2 = &other;

    while ( sel1 && sel2 ) {
        //assert(sel1->_pseudoType != PseudoNotParsed);
        //assert(sel2->_pseudoType != PseudoNotParsed);
	if ( sel1->tagLocalName.id() != sel2->tagLocalName.id() || sel1->attrLocalName.id() != sel2->attrLocalName.id() ||
         sel1->tagNamespace.id() != sel2->tagNamespace.id() || sel1->attrNamespace.id() != sel2->attrNamespace.id() ||
	     sel1->relation != sel2->relation || sel1->match != sel2->match ||
	     sel1->value != sel2->value ||
             sel1->pseudoType() != sel2->pseudoType() ||
             sel1->string_arg != sel2->string_arg)
	    return false;
	sel1 = sel1->tagHistory;
	sel2 = sel2->tagHistory;
    }
    if ( sel1 || sel2 )
	return false;
    return true;
}

DOMString CSSSelector::selectorText() const
{
    // FIXME: Support namespaces when dumping the selector text.  This requires preserving
    // the original namespace prefix used. Ugh. -dwh
    DOMString str;
    const CSSSelector* cs = this;
    quint16 tag = cs->tagLocalName.id();
    if (tag == anyLocalName && cs->match == CSSSelector::None)
        str = "*";
    else if (tag != anyLocalName)
        str = LocalName::fromId(tag).toString();

    const CSSSelector* op = 0;
    while (true) {
        if ( makeId(cs->attrNamespace.id(), cs->attrLocalName.id()) == ATTR_ID && cs->match == CSSSelector::Id )
        {
            str += "#";
            str += cs->value;
        }
        else if ( cs->match == CSSSelector::Class )
        {
            str += ".";
            str += cs->value;
        }
        else if ( cs->match == CSSSelector::PseudoClass )
        {
            str += ":";
            str += cs->value;
            if (!cs->string_arg.isEmpty()) { // e.g :nth-child(...)
                str += cs->string_arg;
                str += ")";
            } else if (cs->simpleSelector && !op) { // :not(...)
                op = cs;
                cs = cs->simpleSelector;
                continue;
            }
        }
        else if ( cs->match == CSSSelector::PseudoElement )
        {
            str += "::";
            str += cs->value;
        }
        // optional attribute
        else if ( cs->attrLocalName.id() ) {
            DOMString attrName = LocalName::fromId(cs->attrLocalName.id()).toString();
            str += "[";
            str += attrName;
            switch (cs->match) {
            case CSSSelector::Exact:
                str += "=";
                break;
            case CSSSelector::Set:
                break;
            case CSSSelector::List:
                str += "~=";
                break;
            case CSSSelector::Hyphen:
                str += "|=";
                break;
            case CSSSelector::Begin:
                str += "^=";
                break;
            case CSSSelector::End:
                str += "$=";
                break;
            case CSSSelector::Contain:
                str += "*=";
                break;
            default:
                qWarning() << "Unhandled case in CSSStyleRuleImpl::selectorText : match=" << cs->match;
            }
            if (cs->match != CSSSelector::Set) {
                str += "\"";
                str += cs->value;
                str += "\"";
            }
            str += "]";
        }
        if (op && !cs->tagHistory) {
            cs=op;
            op=0;
            str += ")";
        }

        if ((cs->relation != CSSSelector::SubSelector && !op) || !cs->tagHistory)
            break;
        cs = cs->tagHistory;
    }

    if ( cs->tagHistory ) {
        DOMString tagHistoryText = cs->tagHistory->selectorText();
        if ( cs->relation == DirectAdjacent )
            str = tagHistoryText + DOMString(" + ") + str;
        else if ( cs->relation == IndirectAdjacent )
            str = tagHistoryText + DOMString(" ~ ") + str;
        else if ( cs->relation == Child )
            str = tagHistoryText + DOMString(" > ") + str;
        else // Descendant
            str = tagHistoryText + DOMString(" ") + str;
    }
    return str;
}

// ----------------------------------------------------------------------------
