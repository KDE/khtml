/* This file is part of the KDE project
 *
 * Copyright (C) 2003-2004 Leo Savernik <l.savernik@aon.at>
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


#include "khtml_caret_p.h"

#include "html/html_documentimpl.h"

namespace khtml {

/** Flags representing the type of advance that has been made.
 * @param LeftObject a render object was left and an ascent to its parent has
 *	taken place
 * @param AdvancedToSibling an actual advance to a sibling has taken place
 * @param EnteredObject a render object was entered by descending into it from
 *	its parent object.
 */
enum ObjectAdvanceState {
  LeftObject = 0x01, AdvancedToSibling = 0x02, EnteredObject = 0x04
};

/** All possible states that may occur during render object traversal.
 * @param OutsideDescending outside of the current object, ready to descend
 *	into children
 * @param InsideDescending inside the current object, descending into
 *	children
 * @param InsideAscending inside the current object, ascending to parents
 * @param OutsideAscending outsie the current object, ascending to parents
 */
enum ObjectTraversalState {
  OutsideDescending, InsideDescending, InsideAscending, OutsideAscending
};

/** Traverses the render object tree in a fine granularity.
 * @param obj render object
 * @param trav object traversal state
 * @param toBegin traverse towards beginning
 * @param base base render object which this method must not advance beyond
 *	(0 means document)
 * @param state object advance state (enum ObjectAdvanceState)
 * @return the render object according to the state. May be the same as \c obj
 */
static RenderObject* traverseRenderObjects(RenderObject *obj,
		ObjectTraversalState &trav, bool toBegin, RenderObject *base,
                int &state)
{
  RenderObject *r;
  switch (trav) {
    case OutsideDescending:
      trav = InsideDescending;
      break;
    case InsideDescending:
      r = toBegin ? obj->lastChild() : obj->firstChild();
      if (r) {
        trav = OutsideDescending;
        obj = r;
        state |= EnteredObject;
      } else {
        trav = InsideAscending;
      }
      break;
    case InsideAscending:
      trav = OutsideAscending;
      break;
    case OutsideAscending:
      r = toBegin ? obj->previousSibling() : obj->nextSibling();
      if (r) {
        trav = OutsideDescending;
        state |= AdvancedToSibling;
      } else {
        r = obj->parent();
        if (r == base) r = 0;
        trav = InsideAscending;
        state |= LeftObject;
      }
      obj = r;
      break;
  }/*end switch*/

  return obj;
}

/** Like RenderObject::objectBelow, but confined to stay within \c base.
 * @param obj render object to begin with
 * @param trav object traversal state, will be reset within this function
 * @param base base render object (0: no base)
 */
static inline RenderObject *renderObjectBelow(RenderObject *obj, ObjectTraversalState &trav, RenderObject *base)
{
  trav = InsideDescending;
  int state;		// we don't need the state, so we don't initialize it
  RenderObject *r = obj;
  while (r && trav != OutsideDescending) {
    r = traverseRenderObjects(r, trav, false, base, state);
#if DEBUG_CARETMODE > 3
    // qDebug() << "renderObjectBelow: r " << r << " trav " << trav;
#endif
  }
  trav = InsideDescending;
  return r;
}

/** Like RenderObject::objectAbove, but confined to stay within \c base.
 * @param obj render object to begin with
 * @param trav object traversal state, will be reset within this function
 * @param base base render object (0: no base)
 */
static inline RenderObject *renderObjectAbove(RenderObject *obj, ObjectTraversalState &trav, RenderObject *base)
{
  trav = OutsideAscending;
  int state;		// we don't need the state, so we don't initialize it
  RenderObject *r = obj;
  while (r && trav != InsideAscending) {
    r = traverseRenderObjects(r, trav, true, base, state);
#if DEBUG_CARETMODE > 3
    // qDebug() << "renderObjectAbove: r " << r << " trav " << trav;
#endif
  }
  trav = InsideAscending;
  return r;
}

/** Checks whether the given inline box matches the IndicatedFlows policy
 * @param box inline box to test
 * @return true on match
 */
static inline bool isIndicatedInlineBox(InlineBox *box)
{
  // text boxes are never indicated.
  if (box->isInlineTextBox()) return false;
  RenderStyle *s = box->object()->style();
  return s->borderLeftWidth() || s->borderRightWidth()
  	|| s->borderTopWidth() || s->borderBottomWidth()
	|| s->paddingLeft().value() || s->paddingRight().value()
	|| s->paddingTop().value() || s->paddingBottom().value()
	// ### Can inline elements have top/bottom margins? Couldn't find
	// it in the CSS 2 spec, but Mozilla ignores them, so we do, too.
	|| s->marginLeft().value() || s->marginRight().value();
}

/** Checks whether the given render object matches the IndicatedFlows policy
 * @param r render object to test
 * @return true on match
 */
static inline bool isIndicatedFlow(RenderObject *r)
{
  RenderStyle *s = r->style();
  return s->borderLeftStyle() != BNONE || s->borderRightStyle() != BNONE
  	|| s->borderTopStyle() != BNONE || s->borderBottomStyle() != BNONE
//	|| s->paddingLeft().value() || s->paddingRight().value()
//	|| s->paddingTop().value() || s->paddingBottom().value()
//	|| s->marginLeft().value() || s->marginRight().value()
	|| s->hasClip() || s->hidesOverflow()
	|| s->backgroundColor().isValid() || s->backgroundImage();
}

/** Advances to the next render object, taking into account the current
 * traversal state.
 *
 * @param r render object
 * @param trav object traversal state
 * @param toBegin @p true, advance towards beginning, @p false, advance toward end
 * @param base base render object which this method must not advance beyond
 *	(0 means document)
 * @param state object advance state (enum ObjectAdvanceState) (unchanged
 *	on LeafsOnly)
 * @return a pointer to the render object which we advanced to,
 *	or 0 if the last possible object has been reached.
 */
static RenderObject *advanceObject(RenderObject *r,
		ObjectTraversalState &trav, bool toBegin,
                RenderObject *base, int &state)
{

  ObjectTraversalState origtrav = trav;
  RenderObject *a = traverseRenderObjects(r, trav, toBegin, base, state);

  bool ignoreOutsideDesc = toBegin && origtrav == OutsideAscending;

  // render object and traversal state at which look ahead has been started
  RenderObject *la = 0;
  ObjectTraversalState latrav = trav;
  ObjectTraversalState lasttrav = origtrav;

  while (a) {
#if DEBUG_CARETMODE > 5
// qDebug() << "a " << a << " trav " << trav;
#endif
    if (a->element()) {
#if DEBUG_CARETMODE > 4
// qDebug() << "a " << a << " trav " << trav << " origtrav " << origtrav << " ignoreOD " << ignoreOutsideDesc;
#endif
      if (toBegin) {

        switch (origtrav) {
	  case OutsideDescending:
            if (trav == InsideAscending) return a;
	    if (trav == OutsideDescending) return a;
	    break;
          case InsideDescending:
	    if (trav == OutsideDescending) return a;
	    // fall through
          case InsideAscending:
            if (trav == OutsideAscending) return a;
	    break;
	  case OutsideAscending:
	    if (trav == OutsideAscending) return a;
            if (trav == InsideAscending && lasttrav == InsideDescending) return a;
	    if (trav == OutsideDescending && !ignoreOutsideDesc) return a;
	    // ignore this outside descending position, as it effectively
	    // demarkates the same position, but remember it in case we fall off
	    // the document.
	    la = a; latrav = trav;
	    ignoreOutsideDesc = false;
	    break;
        }/*end switch*/

      } else {

        switch (origtrav) {
	  case OutsideDescending:
            if (trav == InsideAscending) return a;
	    if (trav == OutsideDescending) return a;
	    break;
          case InsideDescending:
//	    if (trav == OutsideDescending) return a;
	    // fall through
          case InsideAscending:
//            if (trav == OutsideAscending) return a;
//	    break;
	  case OutsideAscending:
	    // ### what if origtrav == OA, and immediately afterwards trav
	    // becomes OD? In this case the effective position hasn't changed ->
	    // the caret gets stuck. Otherwise, it apparently cannot happen in
	    // real usage patterns.
	    if (trav == OutsideDescending) return a;
	    if (trav == OutsideAscending) {
	      if (la) return la;
              // starting lookahead here. Remember old object in case we fall off
	      // the document.
	      la = a; latrav = trav;
	    }
	    break;
	}/*end switch*/

      }/*end if*/
    }/*end if*/

    lasttrav = trav;
    a = traverseRenderObjects(a, trav, toBegin, base, state);
  }/*wend*/

  if (la) trav = latrav, a = la;
  return a;

}

/** Check whether the current render object is unsuitable in caret mode handling.
 *
 * Some render objects cannot be handled correctly in caret mode. These objects
 * are therefore considered to be unsuitable. The null object is suitable, as
 * it denotes reaching the end.
 * @param r current render object
 * @param trav current traversal state
 */
static inline bool isUnsuitable(RenderObject *r, ObjectTraversalState trav)
{
  if (!r) return false;
  return r->isTableCol() || r->isTableSection() || r->isTableRow()
  	|| (r->isText() && !static_cast<RenderText *>(r)->firstTextBox());
      ;
  Q_UNUSED(trav);
}

/** Advances to the next render object, taking into account the current
 * traversal state, but skipping render objects which are not suitable for
 * having placed the caret into them.
 * @param r render object
 * @param trav object traversal state (unchanged on LeafsOnly)
 * @param toBegin @p true, advance towards beginning, @p false, advance toward end
 * @param base base render object which this method must not advance beyond
 *	(0 means document)
 * @param state object advance state (enum ObjectAdvanceState) (unchanged
 *	on LeafsOnly)
 * @return a pointer to the advanced render object or 0 if the last possible
 *	object has been reached.
 */
static inline RenderObject *advanceSuitableObject(RenderObject *r,
		ObjectTraversalState &trav, bool toBegin,
		RenderObject *base, int &state)
{
  do {
    r = advanceObject(r, trav, toBegin, base, state);
#if DEBUG_CARETMODE > 2
    // qDebug() << "after advanceSWP: r " << r << " trav " << trav << " toBegin " << toBegin;
#endif
  } while (isUnsuitable(r, trav));
  return r;
}

/**
  * Returns the next leaf node.
  *
  * Using this function delivers leaf nodes as if the whole DOM tree
  * were a linear chain of its leaf nodes.
  * @param r dom node
  * @param baseElem base element not to search beyond
  * @return next leaf node or 0 if there are no more.
  */
static NodeImpl *nextLeafNode(NodeImpl *r, NodeImpl *baseElem)
{
  NodeImpl *n = r->firstChild();
  if (n) {
    while (n) { r = n; n = n->firstChild(); }
    return const_cast<NodeImpl *>(r);
  }/*end if*/
  n = r->nextSibling();
  if (n) {
    r = n;
    while (n) { r = n; n = n->firstChild(); }
    return const_cast<NodeImpl *>(r);
  }/*end if*/

  n = r->parentNode();
  if (n == baseElem) n = 0;
  while (n) {
    r = n;
    n = r->nextSibling();
    if (n) {
      r = n;
      n = r->firstChild();
      while (n) { r = n; n = n->firstChild(); }
      return const_cast<NodeImpl *>(r);
    }/*end if*/
    n = r->parentNode();
    if (n == baseElem) n = 0;
  }/*wend*/
  return 0;
}

#if 0		// currently not used
/** (Not part of the official DOM)
  * Returns the previous leaf node.
  *
  * Using this function delivers leaf nodes as if the whole DOM tree
  * were a linear chain of its leaf nodes.
  * @param r dom node
  * @param baseElem base element not to search beyond
  * @return previous leaf node or 0 if there are no more.
  */
static NodeImpl *prevLeafNode(NodeImpl *r, NodeImpl *baseElem)
{
  NodeImpl *n = r->firstChild();
  if (n) {
    while (n) { r = n; n = n->firstChild(); }
    return const_cast<NodeImpl *>(r);
  }/*end if*/
  n = r->previousSibling();
  if (n) {
    r = n;
    while (n) { r = n; n = n->firstChild(); }
    return const_cast<NodeImpl *>(r);
  }/*end if*/

  n = r->parentNode();
  if (n == baseElem) n = 0;
  while (n) {
    r = n;
    n = r->previousSibling();
    if (n) {
      r = n;
      n = r->lastChild();
      while (n) { r = n; n = n->lastChild(); }
      return const_cast<NodeImpl *>(r);
    }/*end if*/
    n = r->parentNode();
    if (n == baseElem) n = 0;
  }/*wend*/
  return 0;
}
#endif

/** Maps a DOM Range position to the corresponding caret position.
 *
 * The offset boundary is not checked for validity.
 * @param node DOM node
 * @param offset zero-based offset within node
 * @param r returns render object (may be 0 if DOM node has no render object)
 * @param r_ofs returns the appropriate offset for the found render object r
 * @param outside returns true when offset is applied to the outside of
 *	\c r, or false for the inside.
 * @param outsideEnd return true when the caret position is at the outside end.
 */
void /*KHTML_NO_EXPORT*/ mapDOMPosToRenderPos(NodeImpl *node, long offset,
		RenderObject *&r, long &r_ofs, bool &outside, bool &outsideEnd)
{
  if (node->nodeType() == Node::TEXT_NODE) {
    outside = false;
    outsideEnd = false;
    r = node->renderer();
    r_ofs = offset;
  } else if (node->nodeType() == Node::ELEMENT_NODE || node->nodeType() == Node::DOCUMENT_NODE) {

    // Though offset points between two children, attach it to the visually
    // most suitable one (and only there, because the mapping must stay bijective)
    if (node->firstChild()) {
      outside = true;
      NodeImpl *child = offset <= 0 ? node->firstChild()
      				// childNode is expensive
      				: node->childNode((unsigned long)offset);
      // index was child count or out of bounds
      bool atEnd = !child;
#if DEBUG_CARETMODE > 5
      // qDebug() << "mapDTR: child " << child << "@" << (child ? child->nodeName().string() : QString()) << " atEnd " << atEnd;
#endif
      if (atEnd) child = node->lastChild();

      r = child->renderer();
      r_ofs = 0;
      outsideEnd = atEnd;

      // Outside text nodes most likely stem from a continuation. Seek
      // the enclosing continued render object and use this one instead.
      if (r && child->nodeType() == Node::TEXT_NODE) {
        r = r->parent();
        RenderObject *o = node->renderer();
	while (o->continuation() && o->continuation() != r)
	  o = o->continuation();
	if (!r || o->continuation() != r) {
	  r = child->renderer();
	}
      }/*end if*/

      // BRs cause troubles. Returns the previous render object instead,
      // giving it the attributes outside, outside end.
      if (r && r->isBR()) {
        r = r->objectAbove();
        outsideEnd = true;
      }/*end if*/

    } else {
      // Element has no children, treat offset to be inside the node.
      outside = false;
      outsideEnd = false;
      r = node->renderer();
      r_ofs = 0;	// only offset 0 possible
    }

  } else {
    r = 0;
    qWarning() << "Mapping from nodes of type " << node->nodeType()
        << " not supported!" << endl;
  }
}

/** Maps a caret position to the corresponding DOM Range position.
 *
 * @param r render object
 * @param r_ofs offset within render object
 * @param outside true when offset is interpreted to be on the outside of
 *	\c r, or false if on the inside.
 * @param outsideEnd true when the caret position is at the outside end.
 * @param node returns DOM node
 * @param offset returns zero-based offset within node
 */
void /*KHTML_NO_EXPORT*/ mapRenderPosToDOMPos(RenderObject *r, long r_ofs,
		bool outside, bool outsideEnd, NodeImpl *&node, long &offset)
{
  node = r->element();
  Q_ASSERT(node);
#if DEBUG_CARETMODE > 5
      // qDebug() << "mapRTD: r " << r << '@' << (r ? r->renderName() : QString()) << (r && r->element() ? QString(".node ") + QString::number((unsigned)r->element(),16) + '@' + r->element()->nodeName().string() : QString()) << " outside " << outside << " outsideEnd " << outsideEnd;
#endif
  if (node->nodeType() == Node::ELEMENT_NODE || node->nodeType() == Node::TEXT_NODE) {

    if (outside) {
      NodeImpl *parent = node->parent();

      // If this is part of a continuation, use the actual node as the parent,
      // and the first render child as the node.
      if (r != node->renderer()) {
        RenderObject *o = node->renderer();
	while (o->continuation() && o->continuation() != r)
	  o = o->continuation();
	if (o->continuation() == r) {
	  parent = node;
	  // ### What if the first render child does not map to a child of
	  // the continued node?
	  node = r->firstChild() ? r->firstChild()->element() : node;
	}
      }/*end if*/

      if (!parent) goto inside;

      offset = (long)node->nodeIndex() + outsideEnd;
      node = parent;
#if DEBUG_CARETMODE > 5
   // qDebug() << node << "@" << (node ? node->nodeName().string() : QString()) << " offset " << offset;
#endif
    } else {	// !outside
inside:
      offset = r_ofs;
    }

  } else {
    offset = 0;
    qWarning() << "Mapping to nodes of type " << node->nodeType()
        << " not supported!" << endl;
  }
}

/** Make sure the given node is a leaf node. */
static inline void ensureLeafNode(NodeImpl *&node, NodeImpl *base)
{
  if (node && node->hasChildNodes()) node = nextLeafNode(node, base);
}

/** Converts a caret position to its respective object traversal state.
 * @param outside whether the caret is outside the object
 * @param atEnd whether the caret position is at the end
 * @param toBegin \c true when advancing towards the beginning
 * @param trav returns the corresponding traversal state
 */
static inline void mapRenderPosToTraversalState(bool outside, bool atEnd,
		bool toBegin, ObjectTraversalState &trav)
{
  if (!outside) atEnd = !toBegin;
  if (!atEnd ^ toBegin)
    trav = outside ? OutsideDescending : InsideDescending;
  else
    trav = outside ? OutsideAscending : InsideAscending;
}

/** Converts a traversal state to its respective caret position
 * @param trav object traversal state
 * @param toBegin \c true when advancing towards the beginning
 * @param outside whether the caret is outside the object
 * @param atEnd whether the caret position is at the end
 */
static inline void mapTraversalStateToRenderPos(ObjectTraversalState trav,
		bool toBegin, bool &outside, bool &atEnd)
{
  outside = false;
  switch (trav) {
    case OutsideDescending: outside = true; // fall through
    case InsideDescending: atEnd = toBegin; break;
    case OutsideAscending: outside = true; // fall through
    case InsideAscending: atEnd = !toBegin; break;
  }
}

/** Finds the next node that has a renderer.
 *
 * Note that if the initial @p node has a renderer, this will be returned,
 * regardless of the caret advance policy.
 * Otherwise, for the next nodes, only leaf nodes are considered.
 * @param node node to start with, will be updated accordingly
 * @param offset offset of caret within \c node
 * @param base base render object which this method must not advance beyond
 *	(0 means document)
 * @param r_ofs return the caret offset within the returned renderer
 * @param outside returns whether offset is to be interpreted to the outside
  *	(true) or the inside (false) of the render object.
 * @param outsideEnd returns whether the end of the outside position is meant
 * @return renderer or 0 if no following node has a renderer.
 */
static RenderObject* findRenderer(NodeImpl *&node, long offset,
			RenderObject *base, long &r_ofs,
                        bool &outside, bool &outsideEnd)
{
  if (!node) return 0;
  RenderObject *r;
  mapDOMPosToRenderPos(node, offset, r, r_ofs, outside, outsideEnd);
#if DEBUG_CARETMODE > 2
   // qDebug() << "findRenderer: node " << node << " " << (node ? node->nodeName().string() : QString()) << " offset " << offset << " r " << r << "[" << (r ? r->renderName() : QString()) << "] r_ofs " << r_ofs << " outside " << outside << " outsideEnd " << outsideEnd;
#endif
  if (r) return r;
  NodeImpl *baseElem = base ? base->element() : 0;
  while (!r) {
    node = nextLeafNode(node, baseElem);
    if (!node) break;
    r = node->renderer();
    if (r) r_ofs = offset;
  }
#if DEBUG_CARETMODE > 3
  // qDebug() << "1r " << r;
#endif
  ObjectTraversalState trav;
  int state;		// not used
  mapRenderPosToTraversalState(outside, outsideEnd, false, trav);
  if (r && isUnsuitable(r, trav)) {
    r = advanceSuitableObject(r, trav, false, base, state);
    mapTraversalStateToRenderPos(trav, false, outside, outsideEnd);
    if (r) r_ofs = r->minOffset();
  }
#if DEBUG_CARETMODE > 3
  // qDebug() << "2r " << r;
#endif
  return r;
}

/** returns a suitable base element
 * @param caretNode current node containing caret.
 */
static ElementImpl *determineBaseElement(NodeImpl *caretNode)
{
  // ### for now, only body is delivered for html documents,
  // and 0 for xml documents.

  DocumentImpl *doc = caretNode->getDocument();
  if (!doc) return 0;	// should not happen, but who knows.

  if (doc->isHTMLDocument())
    return static_cast<HTMLDocumentImpl *>(doc)->body();

  return 0;
}

// == class CaretBox implementation

#if DEBUG_CARETMODE > 0
void CaretBox::dump(QTextStream &ts, const QString &ind) const
{
  ts << ind << "b@" << _box;

  if (_box) {
    ts << "<" << _box->object() << ":" << _box->object()->renderName() << ">";
  }/*end if*/

  ts << " " << _x << "+" << _y << "+" << _w << "*" << _h;

  ts << " cb@" << cb;
  if (cb) ts << ":" << cb->renderName();

  ts << " " << (_outside ? (outside_end ? "oe" : "o-") : "i-");
//  ts << endl;
}
#endif

// == class CaretBoxLine implementation

#if DEBUG_CARETMODE > 0
#  define DEBUG_ACIB 1
#else
#  define DEBUG_ACIB DEBUG_CARETMODE
#endif
void CaretBoxLine::addConvertedInlineBox(InlineBox *box, SeekBoxParams &sbp) /*KHTML_NO_EXPORT*/
{
  // Generate only one outside caret box between two elements. If
  // coalesceOutsideBoxes is true, generating left outside boxes is inhibited.
  bool coalesceOutsideBoxes = false;
  CaretBoxIterator lastCoalescedBox;
  for (; box; box = box->nextOnLine()) {
#if DEBUG_ACIB
// qDebug() << "box " << box;
// qDebug() << "box->object " << box->object();
// qDebug() << "x " << box->m_x << " y " << box->m_y << " w " << box->m_width << " h " << box->m_height << " baseline " << box->m_baseline << " ifb " << box->isInlineFlowBox() << " itb " << box->isInlineTextBox() << " rlb " << box->isRootInlineBox();
#endif
    // ### Why the hell can object() ever be 0?!
    if (!box->object()) continue;

    RenderStyle *s = box->object()->style(box->m_firstLine);
    // parent style for outside caret boxes
    RenderStyle *ps = box->parent() && box->parent()->object()
    		? box->parent()->object()->style(box->parent()->m_firstLine)
		: s;

    if (box->isInlineFlowBox()) {
#if DEBUG_ACIB
// qDebug() << "isinlineflowbox " << box;
#endif
      InlineFlowBox *flowBox = static_cast<InlineFlowBox *>(box);
      bool rtl = ps->direction() == RTL;
      const QFontMetrics &pfm = ps->fontMetrics();

      if (flowBox->includeLeftEdge()) {
        // If this box is to be coalesced with the outside end box of its
        // predecessor, then check if it is the searched box. If it is, we
        // substitute the outside end box.
        if (coalesceOutsideBoxes) {
          if (sbp.equalsBox(flowBox, true, false)) {
            sbp.it = lastCoalescedBox;
            Q_ASSERT(!sbp.found);
            sbp.found = true;
          }
        } else {
          addCreatedFlowBoxEdge(flowBox, pfm, true, rtl);
          sbp.check(preEnd());
        }
      }/*end if*/

      if (flowBox->firstChild()) {
#if DEBUG_ACIB
// qDebug() << "this " << this << " flowBox " << flowBox << " firstChild " << flowBox->firstChild();
// qDebug() << "== recursive invocation";
#endif
        addConvertedInlineBox(flowBox->firstChild(), sbp);
#if DEBUG_ACIB
// qDebug() << "== recursive invocation end";
#endif
}
      else {
        addCreatedFlowBoxInside(flowBox, s->fontMetrics());
        sbp.check(preEnd());
      }

      if (flowBox->includeRightEdge()) {
        addCreatedFlowBoxEdge(flowBox, pfm, false, rtl);
        lastCoalescedBox = preEnd();
        sbp.check(lastCoalescedBox);
        coalesceOutsideBoxes = true;
      }

    } else if (box->isInlineTextBox()) {
#if DEBUG_ACIB
// qDebug() << "isinlinetextbox " << box << (box->object() ? QString(" contains \"%1\"").arg(QConstString(static_cast<RenderText *>(box->object())->str->s+box->minOffset(), qMin(box->maxOffset() - box->minOffset(), 15L)).string()) : QString());
#endif
      caret_boxes.append(new CaretBox(box, false, false));
      sbp.check(preEnd());
      // coalescing has been interrupted
      coalesceOutsideBoxes = false;

    } else {
#if DEBUG_ACIB
// qDebug() << "some replaced or what " << box;
#endif
      // must be an inline-block, inline-table, or any RenderReplaced
      bool rtl = ps->direction() == RTL;
      const QFontMetrics &pfm = ps->fontMetrics();

      if (coalesceOutsideBoxes) {
        if (sbp.equalsBox(box, true, false)) {
          sbp.it = lastCoalescedBox;
          Q_ASSERT(!sbp.found);
          sbp.found = true;
        }
      } else {
        addCreatedInlineBoxEdge(box, pfm, true, rtl);
        sbp.check(preEnd());
      }

      caret_boxes.append(new CaretBox(box, false, false));
      sbp.check(preEnd());

      addCreatedInlineBoxEdge(box, pfm, false, rtl);
      lastCoalescedBox = preEnd();
      sbp.check(lastCoalescedBox);
      coalesceOutsideBoxes = true;
    }/*end if*/
  }/*next box*/
}
#undef DEBUG_ACIB

void CaretBoxLine::addCreatedFlowBoxInside(InlineFlowBox *flowBox, const QFontMetrics &fm) /*KHTML_NO_EXPORT*/
{

  CaretBox *caretBox = new CaretBox(flowBox, false, false);
  caret_boxes.append(caretBox);

  // afaik an inner flow box can only have the width 0, therefore we don't
  // have to care for rtl or alignment
  // ### can empty inline elements have a width? css 2 spec isn't verbose about it

  caretBox->_y += flowBox->baseline() - fm.ascent();
  caretBox->_h = fm.height();
}

void CaretBoxLine::addCreatedFlowBoxEdge(InlineFlowBox *flowBox, const QFontMetrics &fm, bool left, bool rtl) /*KHTML_NO_EXPORT*/
{
  CaretBox *caretBox = new CaretBox(flowBox, true, !left);
  caret_boxes.append(caretBox);

  if (left ^ rtl) caretBox->_x -= flowBox->paddingLeft() + flowBox->borderLeft() + 1;
  else caretBox->_x += caretBox->_w + flowBox->paddingRight() + flowBox->borderRight();

  caretBox->_y += flowBox->baseline() - fm.ascent();
  caretBox->_h = fm.height();
  caretBox->_w = 1;
}

void CaretBoxLine::addCreatedInlineBoxEdge(InlineBox *box, const QFontMetrics &fm, bool left, bool rtl) /*KHTML_NO_EXPORT*/
{
  CaretBox *caretBox = new CaretBox(box, true, !left);
  caret_boxes.append(caretBox);

  if (left ^ rtl) caretBox->_x--;
  else caretBox->_x += caretBox->_w;

  caretBox->_y += box->baseline() - fm.ascent();
  caretBox->_h = fm.height();
  caretBox->_w = 1;
}

CaretBoxLine *CaretBoxLine::constructCaretBoxLine(CaretBoxLineDeleter *deleter,
	InlineFlowBox *basicFlowBox, InlineBox *seekBox, bool seekOutside,
        bool seekOutsideEnd, CaretBoxIterator &iter, RenderObject *seekObject)
// 	KHTML_NO_EXPORT
{
  // Iterate all inline boxes within this inline flow box.
  // Caret boxes will be created for each
  // - outside begin of an inline flow box (except for the basic inline flow box)
  // - outside end of an inline flow box (except for the basic inline flow box)
  // - inside of an empty inline flow box
  // - outside begin of an inline box resembling a replaced element
  // - outside end of an inline box resembling a replaced element
  // - inline text box
  // - inline replaced box

  CaretBoxLine *result = new CaretBoxLine(basicFlowBox);
  deleter->append(result);

  SeekBoxParams sbp(seekBox, seekOutside, seekOutsideEnd, seekObject, iter);

  // iterate recursively, I'm too lazy to do it iteratively
  result->addConvertedInlineBox(basicFlowBox, sbp);

  if (!sbp.found) sbp.it = result->end();

  return result;
}

CaretBoxLine *CaretBoxLine::constructCaretBoxLine(CaretBoxLineDeleter *deleter,
	RenderBox *cb, bool outside, bool outsideEnd, CaretBoxIterator &iter) /*KHTML_NO_EXPORT*/
{
  int _x = cb->xPos();
  int _y = cb->yPos();
  int height;
  int width = 1;		// no override is indicated in boxes

  if (outside) {

    RenderStyle *s = cb->element() && cb->element()->parent()
			&& cb->element()->parent()->renderer()
			? cb->element()->parent()->renderer()->style()
			: cb->style();
    bool rtl = s->direction() == RTL;

    const QFontMetrics &fm = s->fontMetrics();
    height = fm.height();

    if (!outsideEnd) {
        _x--;
    } else {
        _x += cb->width();
    }

    int hl = fm.leading() / 2;
    int baseline = cb->baselinePosition(false);
    if (!cb->isReplaced() || cb->style()->display() == BLOCK) {
        if (!outsideEnd ^ rtl)
            _y -= fm.leading() / 2;
        else
            _y += qMax(cb->height() - fm.ascent() - hl, 0);
    } else {
        _y += baseline - fm.ascent() - hl;
    }

  } else {		// !outside

    RenderStyle *s = cb->style();
    const QFontMetrics &fm = s->fontMetrics();
    height = fm.height();

    _x += cb->borderLeft() + cb->paddingLeft();
    _y += cb->borderTop() + cb->paddingTop();

    // ### regard direction
    switch (s->textAlign()) {
      case LEFT:
      case KHTML_LEFT:
      case TAAUTO:	// ### find out what this does
      case JUSTIFY:
        break;
      case CENTER:
      case KHTML_CENTER:
        _x += cb->contentWidth() / 2;
        break;
      case KHTML_RIGHT:
      case RIGHT:
        _x += cb->contentWidth();
        break;
    }/*end switch*/
  }/*end if*/

  CaretBoxLine *result = new CaretBoxLine;
  deleter->append(result);
  result->caret_boxes.append(new CaretBox(_x, _y, width, height, cb,
  			outside, outsideEnd));
  iter = result->begin();
  return result;
}

#if DEBUG_CARETMODE > 0
void CaretBoxLine::dump(QTextStream &ts, const QString &ind) const
{
  ts << ind << "cbl: baseFlowBox@" << basefb << endl;
  QString ind2 = ind + "  ";
  for (size_t i = 0; i < caret_boxes.size(); i++) {
    if (i > 0) ts << endl;
    caret_boxes[i]->dump(ts, ind2);
  }
}
#endif

// == caret mode related helper functions

/** seeks the root line box that is the parent of the given inline box.
 * @param b given inline box
 * @param base base render object which not to step over. If \c base's
 *	inline flow box is hit before the root line box, the flow box
 *	is returned instead.
 * @return the root line box or the base flow box.
 */
inline InlineFlowBox *seekBaseFlowBox(InlineBox *b, RenderObject *base = 0)
{
  // Seek root line box or base inline flow box, if \c base is interfering.
  while (b->parent() && b->object() != base) {
    b = b->parent();
  }/*wend*/
  Q_ASSERT(b->isInlineFlowBox());
  return static_cast<InlineFlowBox *>(b);
}

/** determines whether the given element is a block level replaced element.
 */
inline bool isBlockRenderReplaced(RenderObject *r)
{
  return r->isRenderReplaced() && r->style()->display() == BLOCK;
}

/** determines the caret line box that contains the given position.
 *
 * If the node does not map to a render object, the function will snap to
 * the next suitable render object following it.
 *
 * @param node node to begin with
 * @param offset zero-based offset within node.
 * @param cblDeleter deleter for caret box lines
 * @param base base render object which the caret must not be placed beyond.
 * @param r_ofs adjusted offset within render object
 * @param caretBoxIt returns an iterator to the caret box that contains the
 *	given position.
 * @return the determined caret box lineor 0 if either the node is 0 or
 *	there is no inline flow box containing this node. The containing block
 *	will still be set. If it is 0 too, @p node was invalid.
 */
static CaretBoxLine* findCaretBoxLine(DOM::NodeImpl *node, long offset,
		CaretBoxLineDeleter *cblDeleter, RenderObject *base,
                long &r_ofs, CaretBoxIterator &caretBoxIt)
{
  bool outside, outsideEnd;
  RenderObject *r = findRenderer(node, offset, base, r_ofs, outside, outsideEnd);
  if (!r) { return 0; }
#if DEBUG_CARETMODE > 0
  // qDebug() << "=================== findCaretBoxLine";
  // qDebug() << "node " << node << " offset: " << offset << " r " << r->renderName() << "[" << r << "].node " << r->element()->nodeName().string() << "[" << r->element() << "]" << " r_ofs " << r_ofs << " outside " << outside << " outsideEnd " << outsideEnd;
#endif

  // There are two strategies to find the correct line box. (The third is failsafe)
  // (A) First, if node's renderer is a RenderText, we only traverse its text
  // runs and return the root line box (saves much time for long blocks).
  // This should be the case 99% of the time.
  // (B) Second, we derive the inline flow box directly when the renderer is
  // a RenderBlock, RenderInline, or blocked RenderReplaced.
  // (C) Otherwise, we iterate linearly through all line boxes in order to find
  // the renderer.

  // (A)
  if (r->isText()) do {
    RenderText *t = static_cast<RenderText *>(r);
    int dummy;
    InlineBox *b = t->findInlineTextBox(offset, dummy, true);
    // Actually b should never be 0, but some render texts don't have text
    // boxes, so we insert the last run as an error correction.
    // If there is no last run, we resort to (B)
    if (!b) {
      if (!t->lastTextBox())
        break;
      b = t->lastTextBox();
    }/*end if*/
    Q_ASSERT(b);
    outside = false;	// text boxes cannot have outside positions
    InlineFlowBox *baseFlowBox = seekBaseFlowBox(b, base);
#if DEBUG_CARETMODE > 2
  // qDebug() << "text-box b: " << b << " baseFlowBox: " << baseFlowBox << (b && b->object() ? QString(" contains \"%1\"").arg(QConstString(static_cast<RenderText *>(b->object())->str->s+b->minOffset(), qMin(b->maxOffset() - b->minOffset(), 15L)).string()) : QString());
#endif
#if 0
    if (t->containingBlock()->isListItem()) dumpLineBoxes(static_cast<RenderFlow *>(t->containingBlock()));
#endif
#if DEBUG_CARETMODE > 0
  // qDebug() << "=================== end findCaretBoxLine (renderText)";
#endif
    return CaretBoxLine::constructCaretBoxLine(cblDeleter, baseFlowBox,
        b, outside, outsideEnd, caretBoxIt);
  } while(false);/*end if*/

  // (B)
  bool isrepl = isBlockRenderReplaced(r);
  if (r->isRenderBlock() || r->isRenderInline() || isrepl) {
    RenderFlow *flow = static_cast<RenderFlow *>(r);
    InlineFlowBox *firstLineBox = isrepl ? 0 : flow->firstLineBox();

    // On render blocks, if we are outside, or have a totally empty render
    // block, we simply construct a special caret box line.
    // The latter case happens only when the render block is a leaf object itself.
    if (isrepl || r->isRenderBlock() && (outside || !firstLineBox)
    		|| r->isRenderInline() && !firstLineBox) {
  #if DEBUG_CARETMODE > 0
    // qDebug() << "=================== end findCaretBoxLine (box " << (outside ? (outsideEnd ? "outside end" : "outside begin") : "inside") << ")";
  #endif
      Q_ASSERT(r->isBox());
      return CaretBoxLine::constructCaretBoxLine(cblDeleter,
      		static_cast<RenderBox *>(r), outside, outsideEnd, caretBoxIt);
    }/*end if*/

  // qDebug() << "firstlinebox " << firstLineBox;
    InlineFlowBox *baseFlowBox = seekBaseFlowBox(firstLineBox, base);
    return CaretBoxLine::constructCaretBoxLine(cblDeleter, baseFlowBox,
        firstLineBox, outside, outsideEnd, caretBoxIt);
  }/*end if*/

  RenderBlock *cb = r->containingBlock();
  //if ( !cb ) return 0L;
  Q_ASSERT(cb);

  // ### which element doesn't have a block as its containing block?
  // Is it still possible after the RenderBlock/RenderInline merge?
  if (!cb->isRenderBlock()) {
    qWarning() << "containing block is no render block!!! crash imminent";
  }/*end if*/

  InlineFlowBox *flowBox = cb->firstLineBox();
  // (C)
  // This case strikes when the element is replaced, but neither a
  // RenderBlock nor a RenderInline
  if (!flowBox) {	// ### utter emergency (why is this possible at all?)
//    flowBox = generateDummyFlowBox(arena, cb, r);
//    if (ibox) *ibox = flowBox->firstChild();
//    outside = outside_end = true;

//    qWarning() << "containing block contains no inline flow boxes!!! crash imminent";
#if DEBUG_CARETMODE > 0
   // qDebug() << "=================== end findCaretBoxLine (2)";
#endif
    return CaretBoxLine::constructCaretBoxLine(cblDeleter, cb,
			outside, outsideEnd, caretBoxIt);
  }/*end if*/

  // We iterate the inline flow boxes of the containing block until
  // we find the given node. This has one major flaw: it is linear, and therefore
  // painfully slow for really large blocks.
  for (; flowBox; flowBox = static_cast<InlineFlowBox *>(flowBox->nextLineBox())) {
#if DEBUG_CARETMODE > 0
    // qDebug() << "[scan line]";
#endif

    // construct a caret line box and stop when the element is contained within
    InlineFlowBox *baseFlowBox = seekBaseFlowBox(flowBox, base);
    CaretBoxLine *cbl = CaretBoxLine::constructCaretBoxLine(cblDeleter,
        baseFlowBox, 0, outside, outsideEnd, caretBoxIt, r);
#if DEBUG_CARETMODE > 5
    // qDebug() << cbl->information();
#endif
    if (caretBoxIt != cbl->end()) {
#if DEBUG_CARETMODE > 0
   // qDebug() << "=================== end findCaretBoxLine (3)";
#endif
      return cbl;
    }
  }/*next flowBox*/

  // no inline flow box found, approximate to nearest following node.
  // Danger: this is O(n^2). It's only called to recover from
  // errors, that means, theoretically, never. (Practically, far too often :-( )
  Q_ASSERT(!flowBox);
  CaretBoxLine *cbl = findCaretBoxLine(nextLeafNode(node, base ? base->element() : 0), 0, cblDeleter, base, r_ofs, caretBoxIt);
#if DEBUG_CARETMODE > 0
  // qDebug() << "=================== end findCaretBoxLine";
#endif
  return cbl;
}

/** finds the innermost table object @p r is contained within, but no
 * farther than @p cb.
 * @param r leaf element to begin with
 * @param cb bottom element where to stop search at least.
 * @return the table object or 0 if none found.
 */
static inline RenderTable *findTableUpTo(RenderObject *r, RenderFlow *cb)
{
  while (r && r != cb && !r->isTable()) r = r->parent();
  return r && r->isTable() ? static_cast<RenderTable *>(r) : 0;
}

/** checks whether @p r is a descendant of @p cb, or r == cb
 */
static inline bool isDescendant(RenderObject *r, RenderObject *cb)
{
  while (r && r != cb) r = r->parent();
  return r;
}

/** checks whether the given block contains at least one editable element.
 *
 * Warning: This function has linear complexity, and therefore is expensive.
 * Use it sparingly, and cache the result.
 * @param part part
 * @param cb block to be searched
 * @param table returns the nested table if there is one directly at the beginning
 *	or at the end.
 * @param fromEnd begin search from end (default: begin from beginning)
 */
static bool containsEditableElement(KHTMLPart *part, RenderBlock *cb,
	RenderTable *&table, bool fromEnd = false)
{
  RenderObject *r = cb;
  if (fromEnd)
    while (r->lastChild()) r = r->lastChild();
  else
    while (r->firstChild()) r = r->firstChild();

  RenderTable *tempTable = 0;
  table = 0;
  bool withinCb;
//  int state;		// not used
  ObjectTraversalState trav = InsideDescending;
  do {
    bool modWithinCb = withinCb = isDescendant(r, cb);

    // treat cb extra, it would not be considered otherwise
    if (!modWithinCb) {
      modWithinCb = true;
      r = cb;
    } else
      tempTable = findTableUpTo(r, cb);

#if DEBUG_CARETMODE > 1
    // qDebug() << "cee: r " << (r ? r->renderName() : QString()) << "@" << r << " cb " << cb << " withinCb " << withinCb << " modWithinCb " << modWithinCb << " tempTable " << tempTable;
#endif
    if (r && modWithinCb && r->element() && !isUnsuitable(r, trav)
    	&& (part->isCaretMode() || part->isEditable()
		|| r->style()->userInput() == UI_ENABLED)) {
      table = tempTable;
#if DEBUG_CARETMODE > 1
    // qDebug() << "cee: editable";
#endif
      return true;
    }/*end if*/

//    RenderObject *oldr = r;
//    while (r && r == oldr)
//      r = advanceSuitableObject(r, trav, fromEnd, cb->parent(), state);
    r = fromEnd ? r->objectAbove() : r->objectBelow();
  } while (r && withinCb);
  return false;
}

/** checks whether the given block contains at least one editable child
 * element, beginning with but excluding @p start.
 *
 * Warning: This function has linear complexity, and therefore is expensive.
 * Use it sparingly, and cache the result.
 * @param part part
 * @param cb block to be searched
 * @param table returns the nested table if there is one directly before/after
 *	the start object.
 * @param fromEnd begin search from end (default: begin from beginning)
 * @param start object after which to begin search.
 */
static bool containsEditableChildElement(KHTMLPart *part, RenderBlock *cb,
	RenderTable *&table, bool fromEnd, RenderObject *start)
{
  int state = 0;
  ObjectTraversalState trav = OutsideAscending;
// qDebug() << "start: " << start;
  RenderObject *r = start;
  do {
    r = traverseRenderObjects(r, trav, fromEnd, cb->parent(), state);
  } while(r && !(state & AdvancedToSibling));
// qDebug() << "r: " << r;
  //advanceObject(start, trav, fromEnd, cb->parent(), state);
//     RenderObject *oldr = r;
//     while (r && r == oldr)
  if (!r) return false;

  if (fromEnd)
    while (r->firstChild()) r = r->firstChild();
  else
    while (r->lastChild()) r = r->lastChild();
// qDebug() << "child r: " << r;
  if (!r) return false;

  RenderTable *tempTable = 0;
  table = 0;
  bool withinCb = false;
  do {

    bool modWithinCb = withinCb = isDescendant(r, cb);

    // treat cb extra, it would not be considered otherwise
    if (!modWithinCb) {
      modWithinCb = true;
      r = cb;
    } else
      tempTable = findTableUpTo(r, cb);

#if DEBUG_CARETMODE > 1
    // qDebug() << "cece: r " << (r ? r->renderName() : QString()) << "@" << r << " cb " << cb << " withinCb " << withinCb << " modWithinCb " << modWithinCb << " tempTable " << tempTable;
#endif
    if (r && withinCb && r->element() && !isUnsuitable(r, trav)
    	&& (part->isCaretMode() || part->isEditable()
		|| r->style()->userInput() == UI_ENABLED)) {
      table = tempTable;
#if DEBUG_CARETMODE > 1
    // qDebug() << "cece: editable";
#endif
      return true;
    }/*end if*/

    r = fromEnd ? r->objectAbove() : r->objectBelow();
  } while (withinCb);
  return false;
}

// == class LinearDocument implementation

LinearDocument::LinearDocument(KHTMLPart *part, NodeImpl *node, long offset,
  		CaretAdvancePolicy advancePolicy, ElementImpl *baseElem)
	: node(node), offset(offset), m_part(part),
	advPol(advancePolicy), base(0)
{
  if (node == 0) return;

  if (baseElem) {
    RenderObject *b = baseElem->renderer();
    if (b && (b->isRenderBlock() || b->isRenderInline()))
      base = b;
  }

  initPreBeginIterator();
  initEndIterator();
}

LinearDocument::~LinearDocument()
{
}

int LinearDocument::count() const
{
  // FIXME: not implemented
  return 1;
}

LinearDocument::Iterator LinearDocument::current()
{
  return LineIterator(this, node, offset);
}

LinearDocument::Iterator LinearDocument::begin()
{
  NodeImpl *n = base ? base->element() : 0;
  if (!base) n = node ? node->getDocument() : 0;
  if (!n) return end();

  n = n->firstChild();
  if (advPol == LeafsOnly)
    while (n->firstChild()) n = n->firstChild();

  if (!n) return end();		// must be empty document or empty base element
  return LineIterator(this, n, n->minOffset());
}

LinearDocument::Iterator LinearDocument::preEnd()
{
  NodeImpl *n = base ? base->element() : 0;
  if (!base) n = node ? node->getDocument() : 0;
  if (!n) return preBegin();

  n = n->lastChild();
  if (advPol == LeafsOnly)
    while (n->lastChild()) n = n->lastChild();

  if (!n) return preBegin();	// must be empty document or empty base element
  return LineIterator(this, n, n->maxOffset());
}

void LinearDocument::initPreBeginIterator()
{
  _preBegin = LineIterator(this, 0, 0);
}

void LinearDocument::initEndIterator()
{
  _end = LineIterator(this, 0, 1);
}

// == class LineIterator implementation

CaretBoxIterator LineIterator::currentBox /*KHTML_NO_EXPORT*/;
long LineIterator::currentOffset /*KHTML_NO_EXPORT*/;

LineIterator::LineIterator(LinearDocument *l, DOM::NodeImpl *node, long offset)
		: lines(l)
{
//  qDebug() << "LineIterator: node " << node << " offset " << offset;
  if (!node) { cbl = 0; return; }
  cbl = findCaretBoxLine(node, offset, &lines->cblDeleter,
  		l->baseObject(), currentOffset, currentBox);
  // can happen on partially loaded documents
#if DEBUG_CARETMODE > 0
  if (!cbl) // qDebug() << "no render object found!";
#endif
  if (!cbl) return;
#if DEBUG_CARETMODE > 1
  // qDebug() << "LineIterator: offset " << offset << " outside " << cbl->isOutside();
#endif
#if DEBUG_CARETMODE > 3
  // qDebug() << cbl->information();
#endif
  if (currentBox == cbl->end()) {
#if DEBUG_CARETMODE > 0
    // qDebug() << "LineIterator: findCaretBoxLine failed";
#endif
    cbl = 0;
  }/*end if*/
}

void LineIterator::nextBlock()
{
  RenderObject *base = lines->baseObject();

  bool cb_outside = cbl->isOutside();
  bool cb_outside_end = cbl->isOutsideEnd();

  {
    RenderObject *r = cbl->enclosingObject();

    ObjectTraversalState trav;
    int state;		// not used
    mapRenderPosToTraversalState(cb_outside, cb_outside_end, false, trav);
#if DEBUG_CARETMODE > 1
    // qDebug() << "nextBlock: before adv r" << r << ' ' << (r ? r->renderName() : QString()) << (r && r->isText() ? " contains \"" + QString(((RenderText *)r)->str->s, qMin(((RenderText *)r)->str->l,15)) + "\"" : QString()) << " trav " << trav << " cb_outside " << cb_outside << " cb_outside_end " << cb_outside_end;
#endif
    r = advanceSuitableObject(r, trav, false, base, state);
    if (!r) {
      cbl = 0;
      return;
    }/*end if*/

    mapTraversalStateToRenderPos(trav, false, cb_outside, cb_outside_end);
#if DEBUG_CARETMODE > 1
    // qDebug() << "nextBlock: after r" << r << " trav " << trav << " cb_outside " << cb_outside << " cb_outside_end " << cb_outside_end;
#endif
#if DEBUG_CARETMODE > 0
    // qDebug() << "++: r " << r << "[" << (r?r->renderName():QString()) << "]";
#endif

    RenderBlock *cb;

    // If we hit a block or replaced object, use this as its enclosing object
    bool isrepl = isBlockRenderReplaced(r);
    if (r->isRenderBlock() || isrepl) {
      RenderBox *cb = static_cast<RenderBox *>(r);

      cbl = CaretBoxLine::constructCaretBoxLine(&lines->cblDeleter, cb,
			cb_outside, cb_outside_end, currentBox);

#if DEBUG_CARETMODE > 0
      // qDebug() << "r->isFlow is cb. continuation @" << cb->continuation();
#endif
      return;
    } else {
      cb = r->containingBlock();
      Q_ASSERT(cb->isRenderBlock());
    }/*end if*/
    InlineFlowBox *flowBox = cb->firstLineBox();
#if DEBUG_CARETMODE > 0
    // qDebug() << "++: flowBox " << flowBox << " cb " << cb << '[' << (cb?cb->renderName()+QString(".node ")+QString::number((unsigned)cb->element(),16)+(cb->element()?'@'+cb->element()->nodeName().string():QString()):QString()) << ']';
#endif
    Q_ASSERT(flowBox);
    if (!flowBox) {	// ### utter emergency (why is this possible at all?)
      cb_outside = cb_outside_end = true;
      cbl = CaretBoxLine::constructCaretBoxLine(&lines->cblDeleter, cb,
			cb_outside, cb_outside_end, currentBox);
      return;
    }

    bool seekOutside = false, seekOutsideEnd = false; // silence gcc uninit warning
    CaretBoxIterator it;
    cbl = CaretBoxLine::constructCaretBoxLine(&lines->cblDeleter,
  	flowBox, flowBox->firstChild(), seekOutside, seekOutsideEnd, it);
  }
}

void LineIterator::prevBlock()
{
  RenderObject *base = lines->baseObject();

  bool cb_outside = cbl->isOutside();
  bool cb_outside_end = cbl->isOutsideEnd();

  {
    RenderObject *r = cbl->enclosingObject();
    if (r->isAnonymous() && !cb_outside)
      cb_outside = true, cb_outside_end = false;

    ObjectTraversalState trav;
    int state;		// not used
    mapRenderPosToTraversalState(cb_outside, cb_outside_end, true, trav);
#if DEBUG_CARETMODE > 1
    // qDebug() << "prevBlock: before adv r" << r << " " << (r ? r->renderName() : QString()) << (r && r->isText() ? " contains \"" + QString(((RenderText *)r)->str->s, qMin(((RenderText *)r)->str->l,15)) + "\"" : QString()) << " trav " << trav << " cb_outside " << cb_outside << " cb_outside_end " << cb_outside_end;
#endif
    r = advanceSuitableObject(r, trav, true, base, state);
    if (!r) {
      cbl = 0;
      return;
    }/*end if*/

    mapTraversalStateToRenderPos(trav, true, cb_outside, cb_outside_end);
#if DEBUG_CARETMODE > 1
    // qDebug() << "prevBlock: after r" << r << " trav " << trav << " cb_outside " << cb_outside << " cb_outside_end " << cb_outside_end;
#endif
#if DEBUG_CARETMODE > 0
    // qDebug() << "--: r " << r << "[" << (r?r->renderName():QString()) << "]";
#endif

    RenderBlock *cb;

    // If we hit a block, use this as its enclosing object
    bool isrepl = isBlockRenderReplaced(r);
//    qDebug() << "isrepl " << isrepl << " isblock " << r->isRenderBlock();
    if (r->isRenderBlock() || isrepl) {
      RenderBox *cb = static_cast<RenderBox *>(r);

      cbl = CaretBoxLine::constructCaretBoxLine(&lines->cblDeleter, cb,
			cb_outside, cb_outside_end, currentBox);

#if DEBUG_CARETMODE > 0
      // qDebug() << "r->isFlow is cb. continuation @" << cb->continuation();
#endif
      return;
    } else {
      cb = r->containingBlock();
      Q_ASSERT(cb->isRenderBlock());
    }/*end if*/
    InlineFlowBox *flowBox = cb->lastLineBox();
#if DEBUG_CARETMODE > 0
    // qDebug() << "--: flowBox " << flowBox << " cb " << cb << "[" << (cb?cb->renderName()+QString(".node ")+QString::number((unsigned)cb->element(),16)+(cb->element()?"@"+cb->element()->nodeName().string():QString()):QString()) << "]";
#endif
    Q_ASSERT(flowBox);
    if (!flowBox) {	// ### utter emergency (why is this possible at all?)
      cb_outside = true; cb_outside_end = false;
      cbl = CaretBoxLine::constructCaretBoxLine(&lines->cblDeleter, cb,
			cb_outside, cb_outside_end, currentBox);
      return;
    }

    bool seekOutside = false, seekOutsideEnd = false; // silence gcc uninit warning
    CaretBoxIterator it;
    cbl = CaretBoxLine::constructCaretBoxLine(&lines->cblDeleter,
  	flowBox, flowBox->firstChild(), seekOutside, seekOutsideEnd, it);
  }
}

void LineIterator::advance(bool toBegin)
{
  InlineFlowBox *flowBox = cbl->baseFlowBox();
  if (flowBox) {
    flowBox = static_cast<InlineFlowBox *>(toBegin ? flowBox->prevLineBox() : flowBox->nextLineBox());
    if (flowBox) {
      bool seekOutside = false, seekOutsideEnd = false; // silence gcc uninit warning
      CaretBoxIterator it;
      cbl = CaretBoxLine::constructCaretBoxLine(&lines->cblDeleter,
          flowBox, flowBox->firstChild(), seekOutside, seekOutsideEnd, it);
    }/*end if*/
  }/*end if*/

  // if there are no more lines in this block, move towards block to come
  if (!flowBox) { if (toBegin) prevBlock(); else nextBlock(); }

#if DEBUG_CARETMODE > 3
  if (cbl) // qDebug() << cbl->information();
#endif
}

// == class EditableCaretBoxIterator implementation

void EditableCaretBoxIterator::advance(bool toBegin)
{
#if DEBUG_CARETMODE > 3
      // qDebug() << "---------------" << "toBegin " << toBegin;
#endif
  const CaretBoxIterator preBegin = cbl->preBegin();
  const CaretBoxIterator end = cbl->end();

  CaretBoxIterator lastbox = *this, curbox;
  bool islastuseable = true;	// silence gcc
  bool iscuruseable;
  // Assume adjacency of caret boxes. Will be falsified later if applicable.
  adjacent = true;

#if DEBUG_CARETMODE > 4
//       qDebug() << "ebit::advance: before: " << (**this)->object() << "@" << (**this)->object()->renderName() << ".node " << (**this)->object()->element() << "[" << ((**this)->object()->element() ? (**this)->object()->element()->nodeName().string() : QString()) << "] inline " << (**this)->isInline() << " outside " << (**this)->isOutside() << " outsideEnd " << (**this)->isOutsideEnd();
#endif

  if (toBegin) CaretBoxIterator::operator --(); else CaretBoxIterator::operator ++();
  bool curAtEnd = *this == preBegin || *this == end;
  curbox = *this;
  bool atEnd = true;
  if (!curAtEnd) {
    iscuruseable = isEditable(curbox, toBegin);
    if (toBegin) CaretBoxIterator::operator --(); else CaretBoxIterator::operator ++();
    atEnd = *this == preBegin || *this == end;
  }
  while (!curAtEnd) {
    bool haslast = lastbox != end && lastbox != preBegin;
    bool hascoming = !atEnd;
    bool iscominguseable = true; // silence gcc

    if (!atEnd) iscominguseable = isEditable(*this, toBegin);
    if (iscuruseable) {
#if DEBUG_CARETMODE > 3
      // qDebug() << "ebit::advance: " << (*curbox)->object() << "@" << (*curbox)->object()->renderName() << ".node " << (*curbox)->object()->element() << "[" << ((*curbox)->object()->element() ? (*curbox)->object()->element()->nodeName().string() : QString()) << "] inline " << (*curbox)->isInline() << " outside " << (*curbox)->isOutside() << " outsideEnd " << (*curbox)->isOutsideEnd();
#endif

      CaretBox *box = *curbox;
      if (box->isOutside()) {
        // if this caret box represents no inline box, it is an outside box
	// which has to be considered unconditionally
        if (!box->isInline()) break;

        if (advpol == VisibleFlows) break;

	// IndicatedFlows and LeafsOnly are treated equally in caret box lines

	InlineBox *ibox = box->inlineBox();
	// get previous inline box
	InlineBox *prev = box->isOutsideEnd() ? ibox : ibox->prevOnLine();
	// get next inline box
	InlineBox *next = box->isOutsideEnd() ? ibox->nextOnLine() : ibox;

	const bool isprevindicated = !prev || isIndicatedInlineBox(prev);
	const bool isnextindicated = !next || isIndicatedInlineBox(next);
	const bool last = haslast && !islastuseable;
	const bool coming = hascoming && !iscominguseable;
	const bool left = !prev || prev->isInlineFlowBox() && isprevindicated
		|| (toBegin && coming || !toBegin && last);
	const bool right = !next || next->isInlineFlowBox() && isnextindicated
		|| (!toBegin && coming || toBegin && last);
        const bool text2indicated = toBegin && next && next->isInlineTextBox()
                && isprevindicated
            || !toBegin && prev && prev->isInlineTextBox() && isnextindicated;
        const bool indicated2text = !toBegin && next && next->isInlineTextBox()
                && prev && isprevindicated
            // ### this code is so broken.
            /*|| toBegin && prev && prev->isInlineTextBox() && isnextindicated*/;
#if DEBUG_CARETMODE > 5
      // qDebug() << "prev " << prev << " haslast " << haslast << " islastuseable " << islastuseable << " left " << left << " next " << next << " hascoming " << hascoming << " iscominguseable " << iscominguseable << " right " << right << " text2indicated " << text2indicated << " indicated2text " << indicated2text;
#endif

	if (left && right && !text2indicated || indicated2text) {
	  adjacent = false;
#if DEBUG_CARETMODE > 4
      // qDebug() << "left && right && !text2indicated || indicated2text";
#endif
	  break;
	}

      } else {
        // inside boxes are *always* valid
#if DEBUG_CARETMODE > 4
if (box->isInline()) {
        InlineBox *ibox = box->inlineBox();
      // qDebug() << "inside " << (!ibox->isInlineFlowBox() || static_cast<InlineFlowBox *>(ibox)->firstChild() ? "non-empty" : "empty") << (isIndicatedInlineBox(ibox) ? " indicated" : "") << " adjacent=" << adjacent;
    }
#if 0
  RenderStyle *s = ibox->object()->style();
  // qDebug()	<< "bordls " << s->borderLeftStyle()
  		<< " bordl " << (s->borderLeftStyle() != BNONE)
  		<< " bordr " << (s->borderRightStyle() != BNONE)
  		<< " bordt " << (s->borderTopStyle() != BNONE)
      		<< " bordb " << (s->borderBottomStyle() != BNONE)
		<< " padl " << s->paddingLeft().value()
                << " padr " << s->paddingRight().value()
      		<< " padt " << s->paddingTop().value()
                << " padb " << s->paddingBottom().value()
	// ### Can inline elements have top/bottom margins? Couldn't find
	// it in the CSS 2 spec, but Mozilla ignores them, so we do, too.
		<< " marl " << s->marginLeft().value()
                << " marr " << s->marginRight().value()
      		<< endl;
#endif
#endif
        break;
      }/*end if*/

    } else {

      if (!(*curbox)->isOutside()) {
        // cannot be adjacent anymore
	adjacent = false;
      }

    }/*end if*/
    lastbox = curbox;
    islastuseable = iscuruseable;
    curbox = *this;
    iscuruseable = iscominguseable;
    curAtEnd = atEnd;
    if (!atEnd) {
      if (toBegin) CaretBoxIterator::operator --(); else CaretBoxIterator::operator ++();
      atEnd = *this == preBegin || *this == end;
    }/*end if*/
  }/*wend*/

  *static_cast<CaretBoxIterator *>(this) = curbox;
#if DEBUG_CARETMODE > 4
//  qDebug() << "still valid? " << (*this != preBegin && *this != end);
#endif
#if DEBUG_CARETMODE > 3
      // qDebug() << "---------------" << "end ";
#endif
}

bool EditableCaretBoxIterator::isEditable(const CaretBoxIterator &boxit, bool fromEnd)
{
  Q_ASSERT(boxit != cbl->end() && boxit != cbl->preBegin());
  CaretBox *b = *boxit;
  RenderObject *r = b->object();
#if DEBUG_CARETMODE > 0
//  if (b->isInlineFlowBox()) qDebug() << "b is inline flow box" << (outside ? " (outside)" : "");
  // qDebug() << "isEditable r" << r << ": " << (r ? r->renderName() : QString()) << (r && r->isText() ? " contains \"" + QString(((RenderText *)r)->str->s, qMin(((RenderText *)r)->str->l,15)) + "\"" : QString());
#endif
  // Must check caret mode or design mode *after* r->element(), otherwise
  // lines without a backing DOM node get regarded, leading to a crash.
  // ### check should actually be in InlineBoxIterator
  NodeImpl *node = r->element();
  ObjectTraversalState trav;
  mapRenderPosToTraversalState(b->isOutside(), b->isOutsideEnd(), fromEnd, trav);
  if (isUnsuitable(r, trav) || !node) {
    return false;
  }

  // generally exclude replaced elements with no children from navigation
  if (!b->isOutside() && r->isRenderReplaced() && !r->firstChild())
    return false;

  RenderObject *eff_r = r;
  bool globallyNavigable = m_part->isCaretMode() || m_part->isEditable();

  // calculate the parent element's editability if this inline box is outside.
  if (b->isOutside() && !globallyNavigable) {
    NodeImpl *par = node->parent();
    // I wonder whether par can be 0. It shouldn't be possible if the
    // algorithm contained no bugs.
    Q_ASSERT(par);
    if (par) node = par;
    eff_r = node->renderer();
    Q_ASSERT(eff_r);	// this is a hard requirement
  }

  bool result = globallyNavigable || eff_r->style()->userInput() == UI_ENABLED;
#if DEBUG_CARETMODE > 0
  // qDebug() << result;
#endif
  return result;
}

// == class EditableLineIterator implementation

void EditableLineIterator::advance(bool toBegin)
{
  CaretAdvancePolicy advpol = lines->advancePolicy();
  LineIterator lasteditable, lastindicated;
  bool haslasteditable = false;
  bool haslastindicated = false;
  bool uselasteditable = false;

  LineIterator::advance(toBegin);
  while (cbl) {
    if (isEditable(*this)) {
#if DEBUG_CARETMODE > 3
      // qDebug() << "advance: " << cbl->enclosingObject() << "@" << cbl->enclosingObject()->renderName() << ".node " << cbl->enclosingObject()->element() << "[" << (cbl->enclosingObject()->element() ? cbl->enclosingObject()->element()->nodeName().string() : QString()) << "]";
#endif

      bool hasindicated = isIndicatedFlow(cbl->enclosingObject());
      if (hasindicated) {
        haslastindicated = true;
        lastindicated = *this;
      }

      switch (advpol) {
        case IndicatedFlows:
          if (hasindicated) goto wend;
          // fall through
        case LeafsOnly:
          if (cbl->isOutside()) break;
          // fall through
        case VisibleFlows: goto wend;
      }/*end switch*/

      // remember rejected editable element
      lasteditable = *this;
      haslasteditable = true;
#if DEBUG_CARETMODE > 4
      // qDebug() << "remembered lasteditable " << *lasteditable;
#endif
    } else {

      // If this element isn't editable, but the last one was, and it was only
      // rejected because it didn't match the caret advance policy, force it.
      // Otherwise certain combinations of editable and uneditable elements
      // could never be reached with some policies.
      if (haslasteditable) { uselasteditable = true; break; }

    }
    LineIterator::advance(toBegin);
  }/*wend*/
wend:

  if (uselasteditable) *this = haslastindicated ? lastindicated : lasteditable;
  if (!cbl && haslastindicated) *this = lastindicated;
}

// == class EditableCharacterIterator implementation

void EditableCharacterIterator::initFirstChar()
{
  CaretBox *box = *ebit;
  InlineBox *b = box->inlineBox();
  if (_offset == box->maxOffset())
    peekNext();
  else if (b && !box->isOutside() && b->isInlineTextBox())
    _char = static_cast<RenderText *>(b->object())->str->s[_offset].unicode();
  else
    _char = -1;
}

/** returns true when the given caret box is empty, i. e. should not
 * take place in caret movement.
 */
static inline bool isCaretBoxEmpty(CaretBox *box) {
  if (!box->isInline()) return false;
  InlineBox *ibox = box->inlineBox();
  return ibox->isInlineFlowBox()
  		&& !static_cast<InlineFlowBox *>(ibox)->firstChild()
      		&& !isIndicatedInlineBox(ibox);
}

EditableCharacterIterator &EditableCharacterIterator::operator ++()
{
  _offset++;

  CaretBox *box = *ebit;
  InlineBox *b = box->inlineBox();
  long maxofs = box->maxOffset();
#if DEBUG_CARETMODE > 0
  // qDebug() << "box->maxOffset() " << box->maxOffset() << " box->minOffset() " << box->minOffset();
#endif
  if (_offset == maxofs) {
#if DEBUG_CARETMODE > 2
// qDebug() << "_offset == maxofs: " << _offset << " == " << maxofs;
#endif
    peekNext();
  } else if (_offset > maxofs) {
#if DEBUG_CARETMODE > 2
// qDebug() << "_offset > maxofs: " << _offset << " > " << maxofs /*<< " _peekNext: " << _peekNext*/;
#endif
    if (/*!_peekNext*/true) {
      ++ebit;
      if (ebit == (*_it)->end()) {	// end of line reached, go to next line
        ++_it;
#if DEBUG_CARETMODE > 3
// qDebug() << "++_it";
#endif
        if (_it != _it.lines->end()) {
	  ebit = _it;
          box = *ebit;
          b = box->inlineBox();
#if DEBUG_CARETMODE > 3
// qDebug() << "box " << box << " b " << b << " isText " << box->isInlineTextBox();
#endif

#if DEBUG_CARETMODE > 3
	  RenderObject *_r = box->object();
// qDebug() << "_r " << _r << ":" << _r->element()->nodeName().string();
#endif
	  _offset = box->minOffset();
#if DEBUG_CARETMODE > 3
// qDebug() << "_offset " << _offset;
#endif
	} else {
          b = 0;
	  _end = true;
	}/*end if*/
        goto readchar;
      }/*end if*/
    }/*end if*/

    bool adjacent = ebit.isAdjacent();
#if 0
    // Jump over element if this one is not a text node.
    if (adjacent && !(*ebit)->isInlineTextBox()) {
      EditableCaretBoxIterator copy = ebit;
      ++ebit;
      if (ebit != (*_it)->end() && (*ebit)->isInlineTextBox()
          /*&& (!(*ebit)->isInlineFlowBox()
              || static_cast<InlineFlowBox *>(*ebit)->)*/)
        adjacent = false;
      else ebit = copy;
    }/*end if*/
#endif
    // Jump over empty elements.
    if (adjacent && !(*ebit)->isInlineTextBox()) {
      bool noemptybox = true;
      while (isCaretBoxEmpty(*ebit)) {
        noemptybox = false;
        EditableCaretBoxIterator copy = ebit;
        ++ebit;
        if (ebit == (*_it)->end()) { ebit = copy; break; }
      }
      if (noemptybox) adjacent = false;
    }/*end if*/
//     _r = (*ebit)->object();
    /*if (!_it.outside) */_offset = (*ebit)->minOffset() + adjacent;
    //_peekNext = 0;
    box = *ebit;
    b = box->inlineBox();
    goto readchar;
  } else {
readchar:
    // get character
    if (b && !box->isOutside() && b->isInlineTextBox() && _offset < b->maxOffset())
      _char = static_cast<RenderText *>(b->object())->str->s[_offset].unicode();
    else
      _char = -1;
  }/*end if*/
#if DEBUG_CARETMODE > 2
// qDebug() << "_offset: " << _offset /*<< " _peekNext: " << _peekNext*/ << " char '" << (char)_char << "'";
#endif

#if DEBUG_CARETMODE > 0
  if (!_end && ebit != (*_it)->end()) {
    CaretBox *box = *ebit;
    RenderObject *_r = box->object();
    // qDebug() << "echit++(1): box " << box << (box && box->isInlineTextBox() ? QString(" contains \"%1\"").arg(QConstString(static_cast<RenderText *>(box->object())->str->s+box->minOffset(), box->maxOffset() - box->minOffset()).string()) : QString()) << " _r " << (_r ? _r->element()->nodeName().string() : QString("<nil>"));
  }
#endif
  return *this;
}

EditableCharacterIterator &EditableCharacterIterator::operator --()
{
  _offset--;
  //qDebug() << "--: _offset=" << _offset;

  CaretBox *box = *ebit;
  CaretBox *_peekPrev = 0;
  CaretBox *_peekNext = 0;
  InlineBox *b = box->inlineBox();
  long minofs = box->minOffset();
#if DEBUG_CARETMODE > 0
  // qDebug() << "box->maxOffset() " << box->maxOffset() << " box->minOffset() " << box->minOffset();
#endif
  if (_offset == minofs) {
#if DEBUG_CARETMODE > 2
// qDebug() << "_offset == minofs: " << _offset << " == " << minofs;
#endif
//     _peekNext = b;
    // get character
    if (b && !box->isOutside() && b->isInlineTextBox())
      _char = static_cast<RenderText *>(b->object())->text()[_offset].unicode();
    else
      _char = -1;

    //peekPrev();
    bool do_prev = false;
    {
      EditableCaretBoxIterator copy;
      _peekPrev = 0;
      do {
        copy = ebit;
        --ebit;
        if (ebit == (*_it)->preBegin()) { ebit = copy; break; }
      } while (isCaretBoxEmpty(*ebit));
      // Jump to end of previous element if it's adjacent, and a text box
      if (ebit.isAdjacent() && ebit != (*_it)->preBegin() && (*ebit)->isInlineTextBox()) {
        _peekPrev = *ebit;
	do_prev = true;
      } else
        ebit = copy;
    }
    if (do_prev) goto prev;
  } else if (_offset < minofs) {
prev:
#if DEBUG_CARETMODE > 2
// qDebug() << "_offset < minofs: " << _offset << " < " << minofs /*<< " _peekNext: " << _peekNext*/;
#endif
    if (!_peekPrev) {
      _peekNext = *ebit;
      --ebit;
      if (ebit == (*_it)->preBegin()) { // end of line reached, go to previous line
        --_it;
#if DEBUG_CARETMODE > 3
// qDebug() << "--_it";
#endif
        if (_it != _it.lines->preBegin()) {
// 	  qDebug() << "begin from end!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
	  ebit = EditableCaretBoxIterator(_it, true);
          box = *ebit;
// 	  RenderObject *r = box->object();
#if DEBUG_CARETMODE > 3
// qDebug() << "box " << box << " b " << box->inlineBox() << " isText " << box->isInlineTextBox();
#endif
          _offset = box->maxOffset();
// 	  if (!_it.outside) _offset = r->isBR() ? (*ebit)->minOffset() : (*ebit)->maxOffset();
	  _char = -1;
#if DEBUG_CARETMODE > 0
          // qDebug() << "echit--(2): box " << box << " b " << box->inlineBox() << (box->isInlineTextBox() ? QString(" contains \"%1\"").arg(QConstString(static_cast<RenderText *>(box->object())->str->s+box->minOffset(), box->maxOffset() - box->minOffset()).string()) : QString());
#endif
	} else
	  _end = true;
	return *this;
      }/*end if*/
    }/*end if*/

#if DEBUG_CARETMODE > 0
    bool adjacent = ebit.isAdjacent();
    // qDebug() << "adjacent " << adjacent << " _peekNext " << _peekNext << " _peekNext->isInlineTextBox: " << (_peekNext ? _peekNext->isInlineTextBox() : false) << " !((*ebit)->isInlineTextBox): " << (*ebit ? !(*ebit)->isInlineTextBox() : true);
#endif
#if 0
    // Ignore this box if it isn't a text box, but the previous box was
    if (adjacent && _peekNext && _peekNext->isInlineTextBox()
    	&& !(*ebit)->isInlineTextBox()) {
      EditableCaretBoxIterator copy = ebit;
      --ebit;
      if (ebit == (*_it)->preBegin()) /*adjacent = false;
      else */ebit = copy;
    }/*end if*/
#endif
#if 0
    // Jump over empty elements.
    if (adjacent //&& _peekNext && _peekNext->isInlineTextBox()
        && !(*ebit)->isInlineTextBox()) {
      bool noemptybox = true;
      while (isCaretBoxEmpty(*ebit)) {
        noemptybox = false;
        EditableCaretBoxIterator copy = ebit;
        --ebit;
        if (ebit == (*_it)->preBegin()) { ebit = copy; break; }
        else _peekNext = *copy;
      }
      if (noemptybox) adjacent = false;
    }/*end if*/
#endif
#if DEBUG_CARETMODE > 0
    // qDebug() << "(*ebit)->obj " << (*ebit)->object()->renderName() << "[" << (*ebit)->object() << "]" << " minOffset: " << (*ebit)->minOffset() << " maxOffset: " << (*ebit)->maxOffset();
#endif
#if DEBUG_CARETMODE > 3
    RenderObject *_r = (*ebit)->object();
// qDebug() << "_r " << _r << ":" << _r->element()->nodeName().string();
#endif
    _offset = (*ebit)->maxOffset();
//     if (!_it.outside) _offset = (*ebit)->maxOffset()/* - adjacent*/;
#if DEBUG_CARETMODE > 3
// qDebug() << "_offset " << _offset;
#endif
    _peekPrev = 0;
  } else {
#if DEBUG_CARETMODE > 0
// qDebug() << "_offset: " << _offset << " _peekNext: " << _peekNext;
#endif
    // get character
    if (_peekNext && _offset >= box->maxOffset() && _peekNext->isInlineTextBox())
      _char = static_cast<RenderText *>(_peekNext->object())->text()[_peekNext->minOffset()].unicode();
    else if (b && _offset < b->maxOffset() && b->isInlineTextBox())
      _char = static_cast<RenderText *>(b->object())->text()[_offset].unicode();
    else
      _char = -1;
  }/*end if*/

#if DEBUG_CARETMODE > 0
  if (!_end && ebit != (*_it)->preBegin()) {
    CaretBox *box = *ebit;
    // qDebug() << "echit--(1): box " << box << " b " << box->inlineBox() << (box->isInlineTextBox() ? QString(" contains \"%1\"").arg(QConstString(static_cast<RenderText *>(box->object())->str->s+box->minOffset(), box->maxOffset() - box->minOffset()).string()) : QString());
  }
#endif
  return *this;
}

// == class TableRowIterator implementation

TableRowIterator::TableRowIterator(RenderTable *table, bool fromEnd,
  		RenderTableSection::RowStruct *row)
		: sec(table, fromEnd)
{
  // set index
  if (*sec) {
    if (fromEnd) index = (*sec)->grid.size() - 1;
    else index = 0;
  }/*end if*/

  // initialize with given row
  if (row && *sec) {
    while (operator *() != row)
      if (fromEnd) operator --(); else operator ++();
  }/*end if*/
}

TableRowIterator &TableRowIterator::operator ++()
{
  index++;

  if (index >= (int)(*sec)->grid.size()) {
    ++sec;

    if (*sec) index = 0;
  }/*end if*/
  return *this;
}

TableRowIterator &TableRowIterator::operator --()
{
  index--;

  if (index < 0) {
    --sec;

    if (*sec) index = (*sec)->grid.size() - 1;
  }/*end if*/
  return *this;
}

// == class ErgonomicEditableLineIterator implementation

// some decls
static RenderTableCell *findNearestTableCellInRow(KHTMLPart *part, int x,
		RenderTableSection::RowStruct *row, bool fromEnd);

/** finds the cell corresponding to absolute x-coordinate @p x in the given
 * table.
 *
 * If there is no direct cell, or the cell is not accessible, the function
 * will return the nearest suitable cell.
 * @param part part containing the document
 * @param x absolute x-coordinate
 * @param it table row iterator, will be adapted accordingly as more rows are
 *	investigated.
 * @param fromEnd @p true to begin search from end and work towards the
 *	beginning
 * @return the cell, or 0 if no editable cell was found.
 */
static inline RenderTableCell *findNearestTableCell(KHTMLPart *part, int x,
		TableRowIterator &it, bool fromEnd)
{
  RenderTableCell *result = 0;

  while (*it) {
    result = findNearestTableCellInRow(part, x, *it, fromEnd);
    if (result) break;

    if (fromEnd) --it; else ++it;
  }/*wend*/

  return result;
}

/** finds the nearest editable cell around the given absolute x-coordinate
 *
 * It will dive into nested tables as necessary to provide seamless navigation.
 *
 * If the cell at @p x is not editable, its left neighbor is tried, then its
 * right neighbor, then the left neighbor's left neighbor etc. If no
 * editable cell can be found, 0 is returned.
 * @param part khtml part
 * @param x absolute x-coordinate
 * @param row table row to be searched
 * @param fromEnd @p true, begin from end (applies only to nested tables)
 * @return the found cell or 0 if no editable cell was found
 */
static RenderTableCell *findNearestTableCellInRow(KHTMLPart *part, int x,
		RenderTableSection::RowStruct *row, bool fromEnd)
{
  // First pass. Find spatially nearest cell.
  int n = (int)row->row->size();
  int i;
  for (i = 0; i < n; i++) {
    RenderTableCell *cell = row->row->at(i);
    if (!cell || (long)cell == -1) continue;

    int absx, absy;
    cell->absolutePosition(absx, absy, false); // ### position: fixed?
#if DEBUG_CARETMODE > 1
    // qDebug() << "i/n " << i << "/" << n << " absx " << absx << " absy " << absy;
#endif

    // I rely on the assumption that all cells are in ascending visual order
    // ### maybe this assumption is wrong for bidi?
#if DEBUG_CARETMODE > 1
    // qDebug() << "x " << x << " < " << (absx + cell->width()) << "?";
#endif
    if (x < absx + cell->width()) break;
  }/*next i*/
  if (i >= n) i = n - 1;

  // Second pass. Find editable cell, beginning with the currently found,
  // extending to the left, and to the right, alternating.
  for (int cnt = 0; cnt < 2*n; cnt++) {
    int index = i - ((cnt >> 1) + 1)*(cnt & 1) + (cnt >> 1)*!(cnt & 1);
    if (index < 0 || index >= n) continue;

    RenderTableCell *cell = row->row->at(index);
    if (!cell || (long)cell == -1) continue;

#if DEBUG_CARETMODE > 1
    // qDebug() << "index " << index << " cell " << cell;
#endif
    RenderTable *nestedTable;
    if (containsEditableElement(part, cell, nestedTable, fromEnd)) {

      if (nestedTable) {
        TableRowIterator it(nestedTable, fromEnd);
	while (*it) {
// qDebug() << "=== recursive invocation";
          cell = findNearestTableCell(part, x, it, fromEnd);
	  if (cell) break;
	  if (fromEnd) --it; else ++it;
	}/*wend*/
      }/*end if*/

      return cell;
    }/*end if*/
  }/*next i*/
  return 0;
}

/** returns the nearest common ancestor of two objects that is a table cell,
 * a table section, or 0 if not inside a common table.
 *
 * If @p r1 and @p r2 belong to the same table, but different sections, @p r1's
 * section is returned.
 */
static RenderObject *commonAncestorTableSectionOrCell(RenderObject *r1,
		RenderObject *r2)
{
  if (!r1 || !r2) return 0;
  RenderTableSection *sec = 0;
  int start_depth=0, end_depth=0;
  // First we find the depths of the two objects in the tree (start_depth, end_depth)
  RenderObject *n = r1;
  while (n->parent()) {
    n = n->parent();
    start_depth++;
  }/*wend*/
  n = r2;
  while( n->parent()) {
    n = n->parent();
    end_depth++;
  }/*wend*/
  // here we climb up the tree with the deeper object, until both objects have equal depth
  while (end_depth > start_depth) {
    r2 = r2->parent();
    end_depth--;
  }/*wend*/
  while (start_depth > end_depth) {
    r1 = r1->parent();
//    if (r1->isTableSection()) sec = static_cast<RenderTableSection *>(r1);
    start_depth--;
  }/*wend*/
  // Climb the tree with both r1 and r2 until they are the same
  while (r1 != r2){
    r1 = r1->parent();
    if (r1->isTableSection()) sec = static_cast<RenderTableSection *>(r1);
    r2 = r2->parent();
  }/*wend*/

  // At this point, we found the most approximate common ancestor. Now climb
  // up until the condition of the function return value is satisfied.
  while (r1 && !r1->isTableCell() && !r1->isTableSection() && !r1->isTable())
    r1 = r1->parent();

  return r1 && r1->isTable() ? sec : r1;
}

/** Finds the row that contains the given cell, directly, or indirectly
 * @param section section to be searched
 * @param cell table cell
 * @param row returns the row
 * @param directCell returns the direct cell that contains @p cell
 * @return the index of the row.
 */
static int findRowInSection(RenderTableSection *section, RenderTableCell *cell,
		RenderTableSection::RowStruct *&row, RenderTableCell *&directCell)
{
  // Seek direct cell
  RenderObject *r = cell;
  while (r != section) {
    if (r->isTableCell()) directCell = static_cast<RenderTableCell *>(r);
    r = r->parent();
  }/*wend*/

  // So, and this is really nasty: As we have no indices, we have to do a
  // linear comparison. Oh, that sucks so much for long tables, you can't
  // imagine.
  int n = section->numRows();
  for (int i = 0; i < n; i++) {
    row = &section->grid[i];

    // check for cell
    int m = row->row->size();
    for (int j = 0; j < m; j++) {
      RenderTableCell *c = row->row->at(j);
      if (c == directCell) return i;
    }/*next j*/

  }/*next i*/
  Q_ASSERT(false);
  return -1;
}

/** finds the table that is the first direct or indirect descendant of @p block.
 * @param leaf object to begin search from.
 * @param block object to search to, or 0 to search up to top.
 * @return the table or 0 if there were none.
 */
static inline RenderTable *findFirstDescendantTable(RenderObject *leaf, RenderBlock *block)
{
  RenderTable *result = 0;
  while (leaf && leaf != block) {
    if (leaf->isTable()) result = static_cast<RenderTable *>(leaf);
    leaf = leaf->parent();
  }/*wend*/
  return result;
}

/** looks for the table cell the given object @p r is contained within.
 * @return the table cell or 0 if not contained in any table.
 */
static inline RenderTableCell *containingTableCell(RenderObject *r)
{
  while (r && !r->isTableCell()) r = r->parent();
  return static_cast<RenderTableCell *>(r);
}

inline void ErgonomicEditableLineIterator::calcAndStoreNewLine(
			RenderBlock *newBlock, bool toBegin)
{
  // take the first/last editable element in the found cell as the new
  // value for the iterator
  CaretBoxIterator it;
  cbl = CaretBoxLine::constructCaretBoxLine(&lines->cblDeleter,
  	newBlock, true, toBegin, it);
#if DEBUG_CARETMODE > 3
  // qDebug() << cbl->information();
#endif
//  if (toBegin) prevBlock(); else nextBlock();

  if (!cbl) {
    return;
  }/*end if*/

  EditableLineIterator::advance(toBegin);
}

void ErgonomicEditableLineIterator::determineTopologicalElement(
		RenderTableCell *oldCell, RenderObject *newObject, bool toBegin)
{
  // When we arrive here, a transition between cells has happened.
  // Now determine the type of the transition. This can be
  // (1) a transition from this cell into a table inside this cell.
  // (2) a transition from this cell into another cell of this table

  TableRowIterator it;

  RenderObject *commonAncestor = commonAncestorTableSectionOrCell(oldCell, newObject);
#if DEBUG_CARETMODE > 1
  // qDebug() << " ancestor " << commonAncestor;
#endif

  // The whole document is treated as a table cell.
  if (!commonAncestor || commonAncestor->isTableCell()) {	// (1)

    RenderTableCell *cell = static_cast<RenderTableCell *>(commonAncestor);
    RenderTable *table = findFirstDescendantTable(newObject, cell);

#if DEBUG_CARETMODE > 0
    // qDebug() << "table cell: " << cell;
#endif

    // if there is no table, we fell out of the previous table, and are now
    // in some table-less block. Therefore, done.
    if (!table) return;

    it = TableRowIterator(table, toBegin);

  } else if (commonAncestor->isTableSection()) {		// (2)

    RenderTableSection *section = static_cast<RenderTableSection *>(commonAncestor);
    RenderTableSection::RowStruct *row;
    int idx = findRowInSection(section, oldCell, row, oldCell);
#if DEBUG_CARETMODE > 1
    // qDebug() << "table section: row idx " << idx;
#endif

    it = TableRowIterator(section, idx);

    // advance rowspan rows
    int rowspan = oldCell->rowSpan();
    while (*it && rowspan--) {
      if (toBegin) --it; else ++it;
    }/*wend*/

  } else {
    kError(6201) << "Neither common cell nor section! " << commonAncestor->renderName() << endl;
    // will crash on uninitialized table row iterator
  }/*end if*/

  RenderTableCell *cell = findNearestTableCell(lines->m_part, xCoor, it, toBegin);
#if DEBUG_CARETMODE > 1
  // qDebug() << "findNearestTableCell result: " << cell;
#endif

  RenderBlock *newBlock = cell;
  if (!cell) {
    Q_ASSERT(commonAncestor->isTableSection());
    RenderTableSection *section = static_cast<RenderTableSection *>(commonAncestor);
    cell = containingTableCell(section);
#if DEBUG_CARETMODE > 1
    // qDebug() << "containing cell: " << cell;
#endif

    RenderTable *nestedTable;
    bool editableChild = cell && containsEditableChildElement(lines->m_part,
    		cell, nestedTable, toBegin, section->table());

    if (cell && !editableChild) {
#if DEBUG_CARETMODE > 1
      // qDebug() << "========= recursive invocation outer =========";
#endif
      determineTopologicalElement(cell, cell->section(), toBegin);
#if DEBUG_CARETMODE > 1
      // qDebug() << "========= end recursive invocation outer =========";
#endif
      return;

    } else if (cell && nestedTable) {
#if DEBUG_CARETMODE > 1
      // qDebug() << "========= recursive invocation inner =========";
#endif
      determineTopologicalElement(cell, nestedTable, toBegin);
#if DEBUG_CARETMODE > 1
      // qDebug() << "========= end recursive invocation inner =========";
#endif
      return;

    } else {
#if DEBUG_CARETMODE > 1
      // qDebug() << "newBlock is table: " << section->table();
#endif
      RenderObject *r = section->table();
      int state;		// not used
      ObjectTraversalState trav = OutsideAscending;
      r = advanceSuitableObject(r, trav, toBegin, lines->baseObject(), state);
      if (!r) { cbl = 0;  return; }
//      if (toBegin) prevBlock(); else nextBlock();
      newBlock = static_cast<RenderBlock *>(!r || r->isRenderBlock() ? r : r->containingBlock());
    }/*end if*/
#if 0
  } else {
    // adapt cell so that prevBlock/nextBlock works as expected
    newBlock = cell;
    // on forward advancing, we must start from the outside end of the
    // previous object
    if (!toBegin) {
      RenderObject *r = newBlock;
      int state;		// not used
      ObjectTraversalState trav = OutsideAscending;
      r = advanceSuitableObject(r, trav, true, lines->advancePolicy(), lines->baseObject(), state);
      newBlock = static_cast<RenderBlock *>(!r || r->isRenderBlock() ? r : r->containingBlock());
    }/*end if*/
#endif
  }/*end if*/

  calcAndStoreNewLine(newBlock, toBegin);
}

ErgonomicEditableLineIterator &ErgonomicEditableLineIterator::operator ++()
{
  RenderTableCell *oldCell = containingTableCell(cbl->enclosingObject());

  EditableLineIterator::operator ++();
  if (*this == lines->end() || *this == lines->preBegin()) return *this;

  RenderTableCell *newCell = containingTableCell(cbl->enclosingObject());

  if (!newCell || newCell == oldCell) return *this;

  determineTopologicalElement(oldCell, newCell, false);

  return *this;
}

ErgonomicEditableLineIterator &ErgonomicEditableLineIterator::operator --()
{
  RenderTableCell *oldCell = containingTableCell(cbl->enclosingObject());

  EditableLineIterator::operator --();
  if (*this == lines->end() || *this == lines->preBegin()) return *this;

  RenderTableCell *newCell = containingTableCell(cbl->enclosingObject());

  if (!newCell || newCell == oldCell) return *this;

  determineTopologicalElement(oldCell, newCell, true);

  return *this;
}

// == Navigational helper functions ==

/** seeks the caret box which contains or is the nearest to @p x
 * @param it line iterator pointing to line to be searched
 * @param cv caret view context
 * @param x returns the cv->origX approximation, relatively positioned to the
 *	containing block.
 * @param absx returns absolute x-coordinate of containing block
 * @param absy returns absolute y-coordinate of containing block
 * @return the most suitable caret box
 */
static CaretBox *nearestCaretBox(LineIterator &it, CaretViewContext *cv,
	int &x, int &absx, int &absy)
{
  // Find containing block
  RenderObject *cb = (*it)->containingBlock();
#if DEBUG_CARETMODE > 4
  // qDebug() << "nearestCB: cb " << cb << "@" << (cb ? cb->renderName() : "");
#endif

  if (cb) cb->absolutePosition(absx, absy);
  else absx = absy = 0;

  // Otherwise find out in which inline box the caret is to be placed.

  // this horizontal position is to be approximated
  x = cv->origX - absx;
  CaretBox *caretBox = 0; // Inline box containing the caret
//  NodeImpl *lastnode = 0;  // node of previously checked render object.
  int xPos;		   // x-coordinate of current inline box
  int oldXPos = -1;	   // x-coordinate of last inline box
  EditableCaretBoxIterator fbit = it;
#if DEBUG_CARETMODE > 0
/*  if (it.linearDocument()->advancePolicy() != LeafsOnly)
    qWarning() << "nearestInlineBox is only prepared to handle the LeafsOnly advance policy";*/
//   qDebug() << "*fbit = " << *fbit;
#endif
  // Iterate through all children
  for (CaretBox *b; fbit != (*it)->end(); ++fbit) {
    b = *fbit;

#if DEBUG_CARETMODE > 0
//    RenderObject *r = b->object();
//	if (b->isInlineFlowBox()) qDebug() << "b is inline flow box";
//	qDebug() << "approximate r" << r << ": " << (r ? r->renderName() : QString()) << (r && r->isText() ? " contains \"" + QString(((RenderText *)r)->str->s, ((RenderText *)r)->str->l) + "\"" : QString());
#endif
    xPos = b->xPos();

    // the caret is before this box
    if (x < xPos) {
      // snap to nearest box
      if (oldXPos < 0 || x - (oldXPos + caretBox->width()) > xPos - x) {
	caretBox = b;	// current box is nearer
      }/*end if*/
      break;		// Otherwise, preceding box is implicitly used
    }

    caretBox = b;

    // the caret is within this box
    if (x >= xPos && x < xPos + caretBox->width())
      break;
    oldXPos = xPos;

    // the caret can only be after the last box which is automatically
    // contained in caretBox when we fall out of the loop.
  }/*next fbit*/

  return caretBox;
}

/** moves the given iterator to the beginning of the next word.
 *
 * If the end is reached, the iterator will be positioned there.
 * @param it character iterator to be moved
 */
static void moveItToNextWord(EditableCharacterIterator &it)
{
#if DEBUG_CARETMODE > 0
  // qDebug() << "%%%%%%%%%%%%%%%%%%%%% moveItToNextWord";
#endif
  EditableCharacterIterator copy;
  while (!it.isEnd() && !(*it).isSpace() && !(*it).isPunct()) {
#if DEBUG_CARETMODE > 2
    // qDebug() << "reading1 '" << (*it).toLatin1().constData() << "'";
#endif
    copy = it;
    ++it;
  }

  if (it.isEnd()) {
    it = copy;
    return;
  }/*end if*/

  while (!it.isEnd() && ((*it).isSpace() || (*it).isPunct())) {
#if DEBUG_CARETMODE > 2
    // qDebug() << "reading2 '" << (*it).toLatin1().constData() << "'";
#endif
    copy = it;
    ++it;
  }

  if (it.isEnd()) it = copy;
}

/** moves the given iterator to the beginning of the previous word.
 *
 * If the beginning is reached, the iterator will be positioned there.
 * @param it character iterator to be moved
 */
static void moveItToPrevWord(EditableCharacterIterator &it)
{
  if (it.isEnd()) return;

#if DEBUG_CARETMODE > 0
  // qDebug() << "%%%%%%%%%%%%%%%%%%%%% moveItToPrevWord";
#endif
  EditableCharacterIterator copy;

  // Jump over all space and punctuation characters first
  do {
    copy = it;
    --it;
#if DEBUG_CARETMODE > 2
    if (!it.isEnd()) // qDebug() << "reading1 '" << (*it).toLatin1().constData() << "'";
#endif
  } while (!it.isEnd() && ((*it).isSpace() || (*it).isPunct()));

  if (it.isEnd()) {
    it = copy;
    return;
  }/*end if*/

  do {
    copy = it;
    --it;
#if DEBUG_CARETMODE > 0
    if (!it.isEnd()) // qDebug() << "reading2 '" << (*it).toLatin1().constData() << "' (" << (int)(*it).toLatin1().constData() << ") box " << it.caretBox();
#endif
  } while (!it.isEnd() && !(*it).isSpace() && !(*it).isPunct());

  it = copy;
#if DEBUG_CARETMODE > 1
    if (!it.isEnd()) // qDebug() << "effective '" << (*it).toLatin1().constData() << "' (" << (int)(*it).toLatin1().constData() << ") box " << it.caretBox();
#endif
}

/** moves the iterator by one page.
 * @param ld linear document
 * @param it line iterator, will be updated accordingly
 * @param mindist minimum distance in pixel the iterator should be moved
 *	(if possible)
 * @param next @p true, move downward, @p false move upward
 */
static void moveIteratorByPage(LinearDocument &ld,
		ErgonomicEditableLineIterator &it, int mindist, bool next)
{
  // ### This whole routine plainly sucks. Use an inverse strategie for pgup/pgdn.

  if (it == ld.end() || it == ld.preBegin()) return;

  ErgonomicEditableLineIterator copy = it;
#if DEBUG_CARETMODE > 0
  // qDebug() << " mindist: " << mindist;
#endif

  CaretBoxLine *cbl = *copy;
  int absx = 0, absy = 0;

  RenderBlock *lastcb = cbl->containingBlock();
  Q_ASSERT(lastcb->isRenderBlock());
  lastcb->absolutePosition(absx, absy, false);	// ### what about fixed?

  int lastfby = cbl->begin().data()->yPos();
  int lastheight = 0;
  int rescue = 1000;	// ### this is a hack to keep stuck carets from hanging the ua
  do {
    if (next) ++copy; else --copy;
    if (copy == ld.end() || copy == ld.preBegin()) break;

    cbl = *copy;
    RenderBlock *cb = cbl->containingBlock();

    int diff = 0;
    // ### actually flowBox->yPos() should suffice, but this is not ported
    // over yet from WebCore
    int fby = cbl->begin().data()->yPos();
    if (cb != lastcb) {
      if (next) {
        diff = absy + lastfby + lastheight;
        cb->absolutePosition(absx, absy, false);	// ### what about fixed?
        diff = absy - diff + fby;
        lastfby = 0;
      } else {
        diff = absy;
        cb->absolutePosition(absx, absy, false);	// ### what about fixed?
        diff -= absy + fby + lastheight;
	lastfby = fby - lastheight;
      }/*end if*/
#if DEBUG_CARETMODE > 2
      // qDebug() << "absdiff " << diff;
#endif
    } else {
      diff = qAbs(fby - lastfby);
    }/*end if*/
#if DEBUG_CARETMODE > 2
    // qDebug() << "cbl->begin().data()->yPos(): " << fby << " diff " << diff;
#endif

    mindist -= diff;

    lastheight = qAbs(fby - lastfby);
    lastfby = fby;
    lastcb = cb;
    it = copy;
#if DEBUG_CARETMODE > 0
    // qDebug() << " mindist: " << mindist;
#endif
    // trick: actually the distance is always one line short, but we cannot
    // calculate the height of the first line (### WebCore will make it better)
    // Therefore, we simply approximate that excess line by using the last
    // caluculated line height.
  } while (mindist - lastheight > 0 && --rescue);
}


}/*end namespace*/
