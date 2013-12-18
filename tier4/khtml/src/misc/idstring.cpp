/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2008, 2009 Maksim Orlovich (maksim@kde.org)
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

#include "idstring.h"
#include <assert.h>

namespace khtml {

CaseNormalizeMode IDTableBase::MappingKey::caseNormalizationMode;

bool IDTableBase::MappingKey::operator==(const MappingKey& other) const
{
    if (IDTableBase::MappingKey::caseNormalizationMode == IDS_CaseSensitive)
        return !strcmp(str, other.str);
    else
        return !strcasecmp(str, other.str);
}

static inline unsigned int qHash(const IDTableBase::MappingKey& key) {
    if (!key.str) {
        return 82610334; //same as empty
    } else if (key.caseNormalizationMode == IDS_CaseSensitive) {
        return key.str->hash();
    } else if (key.caseNormalizationMode == IDS_NormalizeLower) {
        return key.str->lowerHash();
    } else { // caseNormalizationMode == IDS_NormalizeUpper
        return key.str->upperHash();
    }
}

void IDTableBase::releaseId(unsigned id)
{
    IDTableBase::MappingKey::caseNormalizationMode = IDS_CaseSensitive;

    m_mappingLookup.remove(m_mappings[id].name);
    m_mappings[id].name->deref();
    m_idFreeList.append(id);
}

unsigned short IDTableBase::grabId(DOMStringImpl* origName, CaseNormalizeMode cnm)
{
    unsigned short newId;

    // Check for existing one, ignoring case if needed
    IDTableBase::MappingKey::caseNormalizationMode = cnm;
    QHash<MappingKey, unsigned short>::const_iterator i = m_mappingLookup.constFind(origName);
    if (i != m_mappingLookup.constEnd()) {
        newId = *i;
        refId(newId);
        return newId;
    }

    // Nope. Allocate new ID. If there is normalization going on, we may now have to 
    // update our case so the canonical mapping is of the expected case. We
    // may also have to deep-copy 
    DOMStringImpl* name = 0;
    switch (cnm) {
    case IDS_CaseSensitive:
        if (origName->m_shallowCopy) {
            // Create a new copy of the data since we may be extending its
            // lifetime indefinitely
            name = new DOMStringImpl(origName->s, origName->l);
            name->m_hash = origName->m_hash;
        } else {
            name = origName;
        }
        break;
    case IDS_NormalizeUpper:
        name = origName->upper();
        break;
    case IDS_NormalizeLower:
        name = origName->lower();
        break;
    }

    Q_ASSERT(name);
    name->ref();

    if (!m_idFreeList.isEmpty()) {
        // Grab from freelist..
        newId = m_idFreeList.last();
        m_idFreeList.removeLast();
        m_mappings[newId].name = name;
    } else {
        // Make a new one --- if we can (we keep one spot for "last resort" mapping)
        if (m_mappings.size() < 0xFFFE) {
            m_mappings.append(Mapping(name));
            newId = m_mappings.size() - 1;
        } else {
            // We ran out of resources. Did we add a fallback mapping yet?
            // We use the same one for everything; and don't even keep track
            // of what it may go to, as we will have no way of freeing
            // the aliases. In particular, this means we no longer need the name..
            name->deref();
            
            if (m_mappings.size() == 0xFFFE) {
                // Need a new mapping..
                name = new DOMStringImpl("_khtml_fallback");
                m_mappings.append(Mapping(name));
                m_mappings[0xFFFF].refCount = 1; // pin it.
                name->ref();
            } else {
                name = m_mappings[0xFFFF].name; // No need to ref the name
                                                // here as the entry is eternal anyway
            }
            newId = 0xFFFF;
        }
    }

    m_mappingLookup[name] = newId;

    refId(newId);
    return newId;
}

void IDTableBase::addStaticMapping(unsigned id, const DOMString& name)
{
    addHiddenMapping(id, name);
    IDTableBase::MappingKey::caseNormalizationMode = IDS_CaseSensitive;
    m_mappingLookup[name.implementation()] = id;
}

void IDTableBase::addHiddenMapping(unsigned id, const DOMString& name)
{
    DOMStringImpl* nameImpl = name.implementation();
    if (nameImpl) nameImpl->ref();

    if (id >= m_mappings.size())
        m_mappings.resize(id + 1);
    m_mappings[id] = Mapping(nameImpl);
    m_mappings[id].refCount = 1; // Pin it.
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
