/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2006 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2009 Vyacheslav Tokarev (tsjoker@gmail.com)
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

#ifndef _DOM_restyler_h_
#define _DOM_restyler_h_

#include <bitset>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include "xml/dom_elementimpl.h"
#include <QtCore/QVarLengthArray>

/*namespace DOM {
    class ElementImpl;
}*/

// Restyle dependency tracker for dynamic DOM

namespace khtml {

using DOM::ElementImpl;

// These types are different types of dependencies, and serves to identify which element should be
// restyled when a change of that kind triggers on the element
enum StructuralDependencyType {
        // Style relies on the children of the element (unaffected by append & close)
        StructuralDependency = 0,
        // Style relies on the last children of the element (affected by append & close)
        BackwardsStructuralDependency = 1,
        // Style relies on the element having hover
        HoverDependency = 2,
        // Style relies on the element being active
        ActiveDependency = 3,
        // Style relies on another state of element (focus, disabled, checked, etc.)
        // (focus is special cased though since elements always depend on their own focus)
        OtherStateDependency = 4,
        LastStructuralDependency
};

// Attribute dependencies are much coarser than structural, for memory reasons rather than performance
// This tracks global dependencies of various kinds.
// The groups are separated into where possible depending elements might be:
enum AttributeDependencyType {
        // Style of the changed element depend on this attribute
        PersonalDependency = 0,
        // Style of the elements children depend on this attribute
        AncestorDependency = 1,
        // Style of the elements later siblings or their children depend on this attribute
        PredecessorDependency = 2,
        LastAttributeDependency
};

// MultiMap implementation by mapping: ElementImpl* -> HashSet of ElementImpl*
// it includes an optimization which covers common mapping case: element -> element, element -> parent
// and set is created only if it contains more than one element otherwise direct mapping is stored as is
struct ElementMap
{
private:
    typedef WTF::HashSet<ElementImpl*> HashSet;
    struct Value {
        union {
            HashSet* set;
            ElementImpl* element;
        } m_value;
        bool isSet : 1;
        bool parentDependency : 1;
        bool selfDependency : 1;
    };
    typedef WTF::HashMap<ElementImpl*, Value> HashMap;
    typedef HashMap::iterator Iterator;
    HashMap map;

    void removeIfEmpty(const Iterator& it) {
        Value& value = it->second;
        if (value.isSet && value.m_value.set->isEmpty()) {
            delete value.m_value.set;
            value.isSet = false;
            value.m_value.element = 0;
        }
        if (!value.isSet && !value.m_value.element && !value.parentDependency && !value.selfDependency)
            map.remove(it);
    }

public:
    typedef QVarLengthArray<ElementImpl*> ElementsList;

    ~ElementMap() {
        Iterator end = map.end();
        for (Iterator it = map.begin(); it != end; ++it)
            if (it->second.isSet)
                delete it->second.m_value.set;
    }

    void add(ElementImpl* a, ElementImpl* b) {
        std::pair<Iterator, bool> it = map.add(a, Value());
        Value& value = it.first->second;
        if (it.second) {
            value.isSet = false;
            value.parentDependency = false;
            value.selfDependency = false;
            value.m_value.element = 0;
        }
        if (b == a) {
            value.selfDependency = true;
        } else if (b == a->parentNode()) {
            value.parentDependency = true;
        } else if (value.isSet) {
            value.m_value.set->add(b);
        } else if (!value.m_value.element || value.m_value.element == b) {
            value.m_value.element = b;
        } else {
            // convert into set
            HashSet* temp = new HashSet();
            temp->add(value.m_value.element);
            temp->add(b);
            value.m_value.set = temp;
            value.isSet = true;
        }
    }

    void remove(ElementImpl* a, ElementImpl* b) {
        Iterator it = map.find(a);
        if (it == map.end())
            return;
        Value& value = it->second;
        if (b == a) {
            value.selfDependency = false;
        } else if (b == a->parentNode()) {
            value.parentDependency = false;
        } else if (value.isSet) {
            // don't care if set contains 1 element after this operation
            // it could be converted back into non-set storage but it's a minor optimization only
            value.m_value.set->remove(b);
        } else if (value.m_value.element == b) {
            value.m_value.element = 0;
        }
        removeIfEmpty(it);
    }

    void remove(ElementImpl* a) {
        Iterator it = map.find(a);
        if (it != map.end()) {
            if (it->second.isSet)
                delete it->second.m_value.set;
            map.remove(it);
        }
    }

private:
    void addFromSet(HashSet* set, ElementsList& array) {
        HashSet::iterator end = set->end();
        for (HashSet::iterator it = set->begin(); it != end; ++it)
            array.append(*it);
    }

public:
    void getElements(ElementImpl* element, ElementsList& array) {
        Iterator it = map.find(element);
        if (it == map.end())
            return;
        Value& value = it->second;
        if (value.parentDependency)
            array.append(static_cast<ElementImpl*>(element->parentNode()));
        if (value.selfDependency)
            array.append(element);
        if (value.isSet)
            addFromSet(value.m_value.set, array);
        if (!value.isSet && value.m_value.element)
            array.append(value.m_value.element);
    }
};

/**
 * @internal
 */
class DynamicDomRestyler {
public:
    DynamicDomRestyler();

    // Structural dependencies are tracked from element to subject
    void addDependency(ElementImpl* subject, ElementImpl* dependency, StructuralDependencyType type);
    void resetDependencies(ElementImpl* subject);
    void restyleDependent(ElementImpl* dependency, StructuralDependencyType type);

    // Attribute dependencies are traced on attribute alone
    void addDependency(uint attrID, AttributeDependencyType type);
    bool checkDependency(uint attrID, AttributeDependencyType type);

    void dumpStats() const;
private:
     // Map of dependencies.
     ElementMap dependency_map[LastStructuralDependency];
     // Map of reverse dependencies. For fast reset
     ElementMap reverse_map;

     // Map of the various attribute dependencies
     std::bitset<512> attribute_map[LastAttributeDependency];
};

}

#endif

