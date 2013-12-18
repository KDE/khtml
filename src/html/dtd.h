/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
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
#ifndef _DOM_dtd_h_
#define _DOM_dtd_h_

#include "dom/dom_string.h"
#include "misc/htmlnames.h"

#undef OPTIONAL

namespace DOM
{

void addForbidden(int tagId, ushort *forbiddenTags);
void removeForbidden(int tagId, ushort *forbiddenTags);

enum tagStatus { OPTIONAL, REQUIRED, FORBIDDEN };

bool checkIsScopeBoundary(ushort tagID);
bool checkChild(ushort tagID, ushort childID, bool strict = false);

extern const unsigned short KHTML_NO_EXPORT tagPriorityArray[];
extern const tagStatus endTagArray[];

inline unsigned short tagPriority(quint32 tagId)
{
    // Treat custom elements the same as <span>; also don't read past the end of the array.
    if (tagId > ID_LAST_TAG)
        return tagPriorityArray[ID_SPAN];
    return tagPriorityArray[tagId];
}

inline tagStatus endTagRequirement(quint32 tagId)
{
    // Treat custom elements the same as <span>; also don't read past the end of the array.
    if (tagId > ID_LAST_TAG)
        return endTagArray[ID_SPAN];
    return endTagArray[tagId];
}

} //namespace DOM
#endif
