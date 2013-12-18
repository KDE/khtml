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

#include "dom_restyler.h"

namespace khtml {

DynamicDomRestyler::DynamicDomRestyler()
{
}

void DynamicDomRestyler::addDependency(ElementImpl* subject, ElementImpl* dependency, StructuralDependencyType type)
{
    assert(type < LastStructuralDependency);
    if (subject == dependency && type == HoverDependency) {
        subject->setHasHoverDependency(true);
        return;
    }

    dependency_map[type].add(dependency, subject);
    reverse_map.add(subject,dependency);
}

void DynamicDomRestyler::resetDependencies(ElementImpl* subject)
{
    subject->setHasHoverDependency(false);

    ElementMap::ElementsList list;
    reverse_map.getElements(subject, list);
    if (list.isEmpty())
        return;
    for (int i = 0; i < list.size(); ++i)
        for (int type = 0; type < LastStructuralDependency; ++type)
            dependency_map[type].remove(list[i], subject);
    reverse_map.remove(subject);
}

void DynamicDomRestyler::restyleDependent(ElementImpl* dependency, StructuralDependencyType type)
{
    assert(type < LastStructuralDependency);
    if (type == HoverDependency && dependency->hasHoverDependency())
        dependency->setChanged(true);

    ElementMap::ElementsList list;
    dependency_map[type].getElements(dependency, list);
    for (int i = 0; i < list.size(); ++i)
        list[i]->setChanged(true);
}

void DynamicDomRestyler::dumpStats() const
{
/*
    // qDebug() << "Keys in structural dependencies: " << dependency_map[StructuralDependency].size();
    // qDebug() << "Keys in attribute dependencies: " << dependency_map[AttributeDependency].size();

    // qDebug() << "Keys in reverse map: " << reverse_map.size();
    */
}

void DynamicDomRestyler::addDependency(uint attrID, AttributeDependencyType type)
{
    assert(type < LastAttributeDependency);

    unsigned int hash = (attrID * 257) % 512;
    attribute_map[type][hash] = true;
}

bool DynamicDomRestyler::checkDependency(uint attrID, AttributeDependencyType type)
{
    assert(type < LastAttributeDependency);

    unsigned int hash = (attrID * 257) % 512;
    // ### gives false positives, but that's okay.
    return attribute_map[type][hash];
}

} // namespace
