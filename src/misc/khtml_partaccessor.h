/* This file is part of the KDE project
 *
 * Copyright (C) 2004 Leo Savernik <l.savernik@aon.at>
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef KHTML_PARTACCESSOR_H
#define KHTML_PARTACCESSOR_H

#include "khtml_part.h"

namespace khtml {

/**
 * This class is merely a namespace for static accessor methods to private
 * methods of KHTMLPart.
 *
 * As this class is a friend of KHTMLPart, and every method of this class is
 * public, it can be used to invoke methods of KHTMLPart that otherwise would
 * be private to functions that cannot be declared friends to KHTMLPart
 * (i. e. functions that belong to anonymous namespaces)
 * @author Leo Savernik
 * @internal
 */
class KHTMLPartAccessor {
  KHTMLPartAccessor();
  ~KHTMLPartAccessor();
public:

  inline static DOM::DocumentImpl *xmlDocImpl(const KHTMLPart *part) { return part->xmlDocImpl(); }
  inline static const DOM::Selection &caret(const KHTMLPart *part) { return part->caret(); }
  inline static void clearSelection(KHTMLPart *part) { part->clearSelection(); }

  inline static KHTMLPartPrivate *d(const KHTMLPart *part) { return part->d; }
};

}/*namespace khtml*/


#endif // KHTML_PARTACCESSOR_H
