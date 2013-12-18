// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003 Apple Computer, Inc.
 *  Copyright (C) 2009 Maksim Orlovich (maksim@kde.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "kjs_range.h"
#include "kjs_range.lut.h"
#include "khtml_part.h"
#include "dom/dom_exception.h"
#include "dom/dom2_range.h" 
#include <QDebug>


using DOM::DOMException;

namespace KJS {
// -------------------------------------------------------------------------

const ClassInfo DOMRange::info = { "Range", 0, &DOMRangeTable, 0 };
/*
@begin DOMRangeTable 7
  startContainer	DOMRange::StartContainer	DontDelete|ReadOnly
  startOffset		DOMRange::StartOffset		DontDelete|ReadOnly
  endContainer		DOMRange::EndContainer		DontDelete|ReadOnly
  endOffset		DOMRange::EndOffset		DontDelete|ReadOnly
  collapsed		DOMRange::Collapsed		DontDelete|ReadOnly
  commonAncestorContainer DOMRange::CommonAncestorContainer	DontDelete|ReadOnly
@end
@begin DOMRangeProtoTable 17
setStart		    DOMRange::SetStart			DontDelete|Function 2
  setEnd		    DOMRange::SetEnd			DontDelete|Function 2
  setStartBefore	    DOMRange::SetStartBefore		DontDelete|Function 1
  setStartAfter		    DOMRange::SetStartAfter		DontDelete|Function 1
  setEndBefore		    DOMRange::SetEndBefore		DontDelete|Function 1
  setEndAfter		    DOMRange::SetEndAfter		DontDelete|Function 1
  collapse		    DOMRange::Collapse			DontDelete|Function 1
  selectNode		    DOMRange::SelectNode		DontDelete|Function 1
  selectNodeContents	    DOMRange::SelectNodeContents	DontDelete|Function 1
  compareBoundaryPoints	    DOMRange::CompareBoundaryPoints	DontDelete|Function 2
  deleteContents	    DOMRange::DeleteContents		DontDelete|Function 0
  extractContents	    DOMRange::ExtractContents		DontDelete|Function 0
  cloneContents		    DOMRange::CloneContents		DontDelete|Function 0
  insertNode		    DOMRange::InsertNode		DontDelete|Function 1
  surroundContents	    DOMRange::SurroundContents		DontDelete|Function 1
  cloneRange		    DOMRange::CloneRange		DontDelete|Function 0
  toString		    DOMRange::ToString			DontDelete|Function 0
  detach		    DOMRange::Detach			DontDelete|Function 0
  createContextualFragment  DOMRange::CreateContextualFragment  DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE(DOMRangeProto)
KJS_IMPLEMENT_PROTOFUNC(DOMRangeProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("DOMRange",DOMRangeProto,DOMRangeProtoFunc,ObjectPrototype)

DOMRange::DOMRange(ExecState *exec, DOM::RangeImpl* r)
 : m_impl(r)
{
  assert(r);
  setPrototype(DOMRangeProto::self(exec));
}

DOMRange::~DOMRange()
{
  ScriptInterpreter::forgetDOMObject(m_impl.get());
}

bool DOMRange::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<DOMRange, DOMObject>(exec, &DOMRangeTable, this, propertyName, slot);
}

JSValue *DOMRange::getValueProperty(ExecState *exec, int token) const
{
  DOMExceptionTranslator exception(exec);
  DOM::RangeImpl &range = *m_impl;

  switch (token) {
  case StartContainer:
    return getDOMNode(exec,range.startContainer(exception));
  case StartOffset:
    return jsNumber(range.startOffset(exception));
  case EndContainer:
    return getDOMNode(exec,range.endContainer(exception));
  case EndOffset:
    return jsNumber(range.endOffset(exception));
  case Collapsed:
    return jsBoolean(range.collapsed(exception));
  case CommonAncestorContainer: {
    return getDOMNode(exec,range.commonAncestorContainer(exception));
  }
  default:
    // qDebug() << "WARNING: Unhandled token in DOMRange::getValueProperty : " << token;
    return jsNull();
  }
}

JSValue *DOMRangeProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::DOMRange, thisObj);
  DOMExceptionTranslator exception(exec);
  DOM::RangeImpl &range = *static_cast<DOMRange *>(thisObj)->impl();
  
  JSValue *result = jsUndefined();

  switch (id) {
    case DOMRange::SetStart:
      range.setStart(toNode(args[0]),args[1]->toInteger(exec), exception);
      break;
    case DOMRange::SetEnd:
      range.setEnd(toNode(args[0]),args[1]->toInteger(exec), exception);
      break;
    case DOMRange::SetStartBefore:
      range.setStartBefore(toNode(args[0]), exception);
      break;
    case DOMRange::SetStartAfter:
      range.setStartAfter(toNode(args[0]), exception);
      break;
    case DOMRange::SetEndBefore:
      range.setEndBefore(toNode(args[0]), exception);
      break;
    case DOMRange::SetEndAfter:
      range.setEndAfter(toNode(args[0]), exception);
      break;
    case DOMRange::Collapse:
      range.collapse(args[0]->toBoolean(exec), exception);
      break;
    case DOMRange::SelectNode:
      range.selectNode(toNode(args[0]), exception);
      break;
    case DOMRange::SelectNodeContents:
      range.selectNodeContents(toNode(args[0]), exception);
      break;
    case DOMRange::CompareBoundaryPoints:
      result = jsNumber(range.compareBoundaryPoints(static_cast<DOM::Range::CompareHow>(args[0]->toInt32(exec)),toRange(args[1]), exception));
      break;
    case DOMRange::DeleteContents:
      range.deleteContents(exception);
      break;
    case DOMRange::ExtractContents:
      result = getDOMNode(exec,range.extractContents(exception));
      break;
    case DOMRange::CloneContents:
      result = getDOMNode(exec,range.cloneContents(exception));
      break;
    case DOMRange::InsertNode:
      range.insertNode(toNode(args[0]), exception);
      break;
    case DOMRange::SurroundContents:
      range.surroundContents(toNode(args[0]), exception);
      break;
    case DOMRange::CloneRange:
      result = getDOMRange(exec,range.cloneRange(exception));
      break;
    case DOMRange::ToString:
      result = jsString(UString(range.toString(exception)));
      break;
    case DOMRange::Detach:
      range.detach(exception);
      break;
    case DOMRange::CreateContextualFragment:
      JSValue *value = args[0];
      DOM::DOMString str = value->type() == NullType ? DOM::DOMString() : value->toString(exec).domString();
      DOM::DocumentFragment frag = range.createContextualFragment(str, exception);
      result = getDOMNode(exec, frag.handle());
      break;
  };

  return result;
}

JSValue* getDOMRange(ExecState *exec, DOM::RangeImpl* r)
{
  return cacheDOMObject<DOM::RangeImpl, KJS::DOMRange>(exec, r);
}

// -------------------------------------------------------------------------

const ClassInfo RangeConstructor::info = { "RangeConstructor", 0, &RangeConstructorTable, 0 };
/*
@begin RangeConstructorTable 5
  START_TO_START	DOM::Range::START_TO_START	DontDelete|ReadOnly
  START_TO_END		DOM::Range::START_TO_END	DontDelete|ReadOnly
  END_TO_END		DOM::Range::END_TO_END		DontDelete|ReadOnly
  END_TO_START		DOM::Range::END_TO_START	DontDelete|ReadOnly
@end
*/

RangeConstructor::RangeConstructor(ExecState *exec)
  : DOMObject(exec->lexicalInterpreter()->builtinObjectPrototype())
{
  putDirect(exec->propertyNames().prototype, DOMRangeProto::self(exec), DontEnum|DontDelete|ReadOnly);
}

bool RangeConstructor::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<RangeConstructor,DOMObject>(exec, &RangeConstructorTable, this, propertyName, slot);
}

JSValue *RangeConstructor::getValueProperty(ExecState *, int token) const
{
  return jsNumber(token);
}

JSValue* getRangeConstructor(ExecState *exec)
{
  return cacheGlobalObject<RangeConstructor>(exec, "[[range.constructor]]");
}


DOM::RangeImpl* toRange(JSValue *val)
{
  JSObject *obj = val->getObject();
  if (!obj || !obj->inherits(&DOMRange::info))
    return 0;

  const DOMRange *dobj = static_cast<const DOMRange*>(obj);
  return dobj->impl();
}

/* Source for RangeExceptionProtoTable.
@begin RangeExceptionProtoTable 2
BAD_BOUNDARYPOINTS_ERR  DOM::RangeException::BAD_BOUNDARYPOINTS_ERR    DontDelete|ReadOnly
INVALID_NODE_TYPE_ERR   DOM::RangeException::INVALID_NODE_TYPE_ERR     DontDelete|ReadOnly
@end
*/

DEFINE_CONSTANT_TABLE(RangeExceptionProto)
IMPLEMENT_CONSTANT_TABLE(RangeExceptionProto, "RangeException")

IMPLEMENT_PSEUDO_CONSTRUCTOR_WITH_PARENT(RangeExceptionPseudoCtor, "RangeException",
                             RangeExceptionProto, RangeExceptionProto)

RangeException::RangeException(ExecState* exec)
  : DOMObject(RangeExceptionProto::self(exec))
{
}

const ClassInfo RangeException::info = { "RangeException", 0, 0, 0 };

// -------------------------------------------------------------------------

const ClassInfo DOMSelection::info = { "Selection", 0, &DOMSelectionTable, 0 };
/*
@begin DOMSelectionTable 7
  anchorNode            DOMSelection::AnchorNode        DontDelete|ReadOnly
  anchorOffset          DOMSelection::AnchorOffset      DontDelete|ReadOnly
  focusNode             DOMSelection::FocusNode         DontDelete|ReadOnly
  focusOffset           DOMSelection::FocusOffset       DontDelete|ReadOnly
  isCollapsed           DOMSelection::IsCollapsed       DontDelete|ReadOnly
  rangeCount            DOMSelection::RangeCount        DontDelete|ReadOnly
@end
@begin DOMSelectionProtoTable 13
 collapsed          DOMSelection::Collapsed               DontDelete|Function 2
 collapseToStart    DOMSelection::CollapseToStart         DontDelete|Function 0
 collapseToEnd      DOMSelection::CollapseToEnd           DontDelete|Function 0
 selectAllChildren  DOMSelection::SelectAllChildren       DontDelete|Function 1
 deleteFromDocument DOMSelection::DeleteFromDocument      DontDelete|Function 0
 getRangeAt         DOMSelection::GetRangeAt              DontDelete|Function 1
 addRange           DOMSelection::AddRange                DontDelete|Function 1
 removeRange        DOMSelection::RemoveRange             DontDelete|Function 1
 removeAllRanges    DOMSelection::RemoveAllRanges         DontDelete|Function 0
 toString           DOMSelection::ToString                DontDelete|Function 0
@end
*/
KJS_DEFINE_PROTOTYPE(DOMSelectionProto)
KJS_IMPLEMENT_PROTOFUNC(DOMSelectionProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("Selection",DOMSelectionProto,DOMSelectionProtoFunc,ObjectPrototype)

DOMSelection::DOMSelection(ExecState* exec, DOM::DocumentImpl* parentDocument):
        JSObject(DOMSelectionProto::self(exec)), m_document(parentDocument)
{}

bool DOMSelection::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // qDebug() << propertyName.ustring().qstring();
    return getStaticValueSlot<DOMSelection, JSObject>(exec, &DOMSelectionTable, this, propertyName, slot);
}

JSValue* DOMSelection::getValueProperty(ExecState* exec, int token) const
{
    // qDebug() << token;
    DOMExceptionTranslator exception(exec);
    DOM::Selection sel = currentSelection();
    // ### TODO: below doesn't really distinguish anchor and focus properly.
    switch (token) {
        case DOMSelection::AnchorNode:
            return sel.notEmpty() ? getDOMNode(exec, sel.base().node()) : jsNull();
        case DOMSelection::AnchorOffset:
            return jsNumber(sel.notEmpty() ? sel.base().offset() : 0L);
        case DOMSelection::FocusNode:
            return sel.notEmpty() ? getDOMNode(exec, sel.extent().node()) : jsNull();
        case DOMSelection::FocusOffset:
            return jsNumber(sel.notEmpty() ? sel.extent().offset() : 0L);
        case DOMSelection::IsCollapsed:
            return jsBoolean(sel.isCollapsed() || sel.isEmpty());
        case DOMSelection::RangeCount:
            return sel.notEmpty() ? jsNumber(1) : jsNumber(0);
    }

    assert (false);
    return jsUndefined();
}

JSValue* DOMSelectionProtoFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    KJS_CHECK_THIS(KJS::DOMSelection, thisObj);

    DOMSelection* self = static_cast<DOMSelection*>(thisObj);
    if (!self->attached())
        return jsUndefined();
    
    DOM::Selection sel = self->currentSelection();

    DOMExceptionTranslator exception(exec);
    switch (id) {
        case DOMSelection::Collapsed: {
            DOM::NodeImpl* node   = toNode(args[0]);
            int            offset = args[1]->toInt32(exec);
            if (node && node->document() == self->m_document)
                self->m_document->part()->setCaret(DOM::Selection(DOM::Position(node, offset)));
            else
                setDOMException(exec, DOMException::WRONG_DOCUMENT_ERR);
            break;
        }

        case DOMSelection::CollapseToStart:
            if (sel.notEmpty()) {
                sel.moveTo(sel.start());
                self->m_document->part()->setCaret(sel);
            } else {
                setDOMException(exec, DOMException::INVALID_STATE_ERR);
            }
            break;

        case DOMSelection::CollapseToEnd:
            if (sel.notEmpty()) {
                sel.moveTo(sel.end());
                self->m_document->part()->setCaret(sel);
            } else {
                setDOMException(exec, DOMException::INVALID_STATE_ERR);
            }
            break;

        case DOMSelection::SelectAllChildren: {
            DOM::NodeImpl* node = toNode(args[0]);
            if (node && node->document() == self->m_document) {
                DOM::RangeImpl* range = new DOM::RangeImpl(self->m_document);
                range->selectNodeContents(node, exception);
                self->m_document->part()->setCaret(DOM::Selection(DOM::Range(range)));
            } else {
                setDOMException(exec, DOMException::WRONG_DOCUMENT_ERR);
            }
            break;
        }

        case DOMSelection::DeleteFromDocument: {
            self->m_document->part()->setCaret(DOM::Selection());
            DOM::Range r = sel.toRange();
            DOM::RangeImpl* ri = r.handle();
            if (ri) {
                ri->deleteContents(exception);
            }
            break;
        }
            
        case DOMSelection::GetRangeAt: {
            int i = args[0]->toInt32(exec);
            if (sel.isEmpty() || i != 0) {
                setDOMException(exec, DOMException::INDEX_SIZE_ERR);
            } else {
                DOM::Range r = sel.toRange();
                return getDOMRange(exec, r.handle());
            }
            break;
        }

        case DOMSelection::AddRange: {
            // We only support a single range, so we merge the two.
            // This does violate HTML5, though, as it's actually supposed to report the
            // overlap twice. Perhaps this shouldn't be live?
            DOM::RangeImpl* range = toRange(args[0]);

            if (!range)
                return jsUndefined();
            if (range->ownerDocument() != self->m_document) {
                setDOMException(exec, DOMException::WRONG_DOCUMENT_ERR);
                return jsUndefined();
            }

            if (sel.isEmpty()) {
                self->m_document->part()->setCaret(DOM::Selection(range));
                return jsUndefined();
            }
            
            DOM::Range      ourRange = sel.toRange();
            DOM::RangeImpl* ourRangeImpl = ourRange.handle();

            bool startExisting = ourRangeImpl->compareBoundaryPoints(DOM::Range::START_TO_START, range, exception) == -1;
            bool endExisting   = ourRangeImpl->compareBoundaryPoints(DOM::Range::END_TO_END, range, exception) == -1;

            DOM::RangeImpl* rangeForStart = startExisting ? ourRangeImpl : range;
            DOM::RangeImpl* rangeForEnd   = endExisting ? ourRangeImpl : range;
            DOM::Position start = DOM::Position(rangeForStart->startContainer(exception), rangeForStart->startOffset(exception));
            DOM::Position end   = DOM::Position(rangeForEnd->endContainer(exception), rangeForEnd->endOffset(exception));

            self->m_document->part()->setCaret(DOM::Selection(start, end));
            break;
        }

        case DOMSelection::RemoveRange: {
            // This actually take a /Range/. How brittle.
            if (sel.isEmpty())
                return jsUndefined();

            DOM::RangeImpl* range    = toRange(args[0]);
            DOM::Range      ourRange = sel.toRange();
            DOM::RangeImpl* ourRangeImpl = ourRange.handle();
            if (range && range->startContainer(exception) == ourRangeImpl->startContainer(exception)
                      && range->startOffset(exception)    == ourRangeImpl->startOffset   (exception)
                      && range->endContainer(exception)   == ourRangeImpl->endContainer  (exception)
                      && range->endOffset(exception)      == ourRangeImpl->endOffset     (exception)) {
                self->m_document->part()->setCaret(DOM::Selection());
            }
            break;
        }

        case DOMSelection::RemoveAllRanges:
            self->m_document->part()->setCaret(DOM::Selection());
            break;

        case DOMSelection::ToString:
            if (sel.isEmpty() || sel.isCollapsed()) {
                return jsString(UString());
            } else {
                DOM::Range r = sel.toRange();
                DOM::RangeImpl* ri = r.handle();
                if (ri) {
                    return jsString(ri->toString(exception));
                }
            }
            break;
    }
    return jsUndefined();
}

DOM::Selection DOMSelection::currentSelection() const
{
    if (m_document && m_document->part())
        return m_document->part()->caret();
    else
        return DOM::Selection();
}

bool DOMSelection::attached() const
{
    return m_document && m_document->part();
}

}
