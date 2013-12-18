/*
 * util.h - Copyright 2005 Frerich Raabe <raabe@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef UTIL_H
#define UTIL_H

#include <QList>
#include <misc/shared.h>
#include <dom/dom_string.h>

namespace DOM {
	class NodeImpl;
	class StaticNodeListImpl;
}

namespace khtml {
namespace XPath {

// ### removeme
typedef SharedPtr<DOM::StaticNodeListImpl> DomNodeList;


/* @return whether the given node is the root node.
 */
bool isRootDomNode( DOM::NodeImpl *node );

/* @return the 'string-value' of the given node as specified by
   http://www.w3.org/TR/xpath
 */
DOM::DOMString stringValue( DOM::NodeImpl *node );

/* @return append all descendant nodes of the given node, in document order,
   to the given set
 */
void collectChildrenRecursively( SharedPtr<DOM::StaticNodeListImpl> out,
                                 DOM::NodeImpl *root );

/* this one is in reverse order */
void collectChildrenReverse( SharedPtr<DOM::StaticNodeListImpl> out,
                             DOM::NodeImpl *root );

/* @return whether the given node is a valid context node
 */
bool isValidContextNode( DOM::NodeImpl *node );

/* @returns the parent node of the given node under the XPath model
  (which has some additional links that DOM doesn't
*/
DOM::NodeImpl *xpathParentNode( DOM::NodeImpl *node );

/* @returns the first/last kid of the given node under the XPath model,
   which doesn't have text nodes or the likes under attributes
*/
DOM::NodeImpl *xpathFirstChild( DOM::NodeImpl *node );
DOM::NodeImpl *xpathLastChild( DOM::NodeImpl *node );

/* @returns a slightly generalized notion of a sibling needed to implement
   the following axis. Essentially, for that axis, and only that axis,
   if we have something like this:
     <node attr1 attr2><kid></node>
   the <kid> is considered to be the next thing following attr1
*/
DOM::NodeImpl *nextSiblingForFollowing( DOM::NodeImpl *node );

// Enable for some low debug output.
// #define XPATH_VERBOSE


} // namespace XPath

} // namespace khtml


#endif // UTIL_H

