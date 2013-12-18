/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll <knoll@kde.org>
 * Copyright (C) 2000 Gunnstein Lye <gunnstein@netcom.no>
 * Copyright (C) 2000 Frederik Holljen <frederik.holljen@hig.no>
 * Copyright (C) 2001 Peter Kelly <pmk@post.com>
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

#ifndef _DOM2_RangeImpl_h_
#define _DOM2_RangeImpl_h_

#include "dom/dom2_range.h"
#include "misc/shared.h"

namespace DOM {

class RangeImpl : public khtml::Shared<RangeImpl>
{
    friend class DocumentImpl;
public:
    RangeImpl(DocumentImpl *_ownerDocument);
    RangeImpl(DocumentImpl *_ownerDocument,
              NodeImpl *_startContainer, long _startOffset,
              NodeImpl *_endContainer, long _endOffset);

    ~RangeImpl();

    NodeImpl *startContainer(int &exceptioncode) const;
    long startOffset(int &exceptioncode) const;
    NodeImpl *endContainer(int &exceptioncode) const;
    long endOffset(int &exceptioncode) const;
    bool collapsed(int &exceptioncode) const;

    NodeImpl *commonAncestorContainer(int &exceptioncode);
    static NodeImpl *commonAncestorContainer(NodeImpl *containerA, NodeImpl *containerB);
    void setStart ( NodeImpl *refNode, long offset, int &exceptioncode );
    void setEnd ( NodeImpl *refNode, long offset, int &exceptioncode );
    void collapse ( bool toStart, int &exceptioncode );
    short compareBoundaryPoints ( Range::CompareHow how, RangeImpl *sourceRange, int &exceptioncode );
    static short compareBoundaryPoints ( NodeImpl *containerA, long offsetA, NodeImpl *containerB, long offsetB );
    bool boundaryPointsValid (  );
    void deleteContents ( int &exceptioncode );
    DocumentFragmentImpl *extractContents ( int &exceptioncode );
    DocumentFragmentImpl *cloneContents ( int &exceptioncode );
    void insertNode( NodeImpl *newNode, int &exceptioncode );
    DOMString toString ( int &exceptioncode );
    /** Converts the selection  to HTML.  The returned string will have matching
     *  tags, and all td, tr, etc tags will be inside a table tag.  CSS is not
     *  used at this stage - This needs to be fixed.
     *
     *  This is guaranteed to produce an xml valid snippet, no matter how crappy the input
     *  html page is.  It will have html and body tags.
     *  
     *  Any urls in images or links will be expanded to full urls <em>with passwords stripped</em>
     *  for security reasons.
     *
     *  Note: Originally this function didn't have the exceptioncode argument.  I added it
     *  since all the other functions do.  If this is correct, please remove this comment.
     *
     *  @param exceptioncode This will be set if m_detached is true.
     *  @return A string with html tags for this range.
     */
    DOMString toHTML ( int &exceptioncode );

    DocumentFragment createContextualFragment ( const DOMString &html, int &exceptioncode );

    void detach ( int &exceptioncode );
    bool isDetached() const;
    RangeImpl *cloneRange(int &exceptioncode);

    void setStartAfter( NodeImpl *refNode, int &exceptioncode );
    void setEndBefore( NodeImpl *refNode, int &exceptioncode );
    void setEndAfter( NodeImpl *refNode, int &exceptioncode );
    void selectNode( NodeImpl *refNode, int &exceptioncode );
    void selectNodeContents( NodeImpl *refNode, int &exceptioncode );
    void surroundContents( NodeImpl *newParent, int &exceptioncode );
    void setStartBefore( NodeImpl *refNode, int &exceptioncode );

    enum ActionType {
        DELETE_CONTENTS,
        EXTRACT_CONTENTS,
        CLONE_CONTENTS
    };
    DocumentFragmentImpl *processContents ( ActionType action, int &exceptioncode );

    bool readOnly() { return false; }

    DocumentImpl *ownerDocument() { return m_ownerDocument; }

protected:
    DocumentImpl *m_ownerDocument;
    NodeImpl *m_startContainer;
    unsigned long m_startOffset;
    NodeImpl *m_endContainer;
    unsigned long m_endOffset;
    bool m_detached;

private:
    void checkNodeWOffset( NodeImpl *n, int offset, int &exceptioncode) const;
    void checkNodeBA( NodeImpl *n, int &exceptioncode ) const;
    void setStartContainer(NodeImpl *_startContainer);
    void setEndContainer(NodeImpl *_endContainer);
    void checkDeleteExtract(int &exceptioncode);
    bool containedByReadOnly();
    unsigned long maxEndOffset() const;
    unsigned long maxStartOffset() const;
};

} // namespace

#endif

