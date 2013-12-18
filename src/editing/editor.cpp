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

#include "editor.h"

#include "jsediting.h"
#include "htmlediting_impl.h"

#include "css/css_renderstyledeclarationimpl.h"
#include "css/css_valueimpl.h"
#include "xml/dom_selection.h"
#include "xml/dom_docimpl.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_textimpl.h"
#include "xml/dom2_rangeimpl.h"
#include "khtml_part.h"
#include "khtml_ext.h"
#include "khtmlpart_p.h"

#include <QStack>

#ifndef APPLE_CHANGES
#  ifdef assert
#    undef assert
#  endif
#  define assert(x) Q_ASSERT(x)
#endif

#define PREPARE_JSEDITOR_CALL(command, retval) \
	JSEditor *js = m_part->xmlDocImpl() ? m_part->xmlDocImpl()->jsEditor() : 0; \
        if (!js) return retval; \
        const CommandImp *imp = js->commandImp(command)

#define DEBUG_COMMANDS

using namespace WTF;

using namespace DOM;

using khtml::RenderStyleDeclarationImpl;
using khtml::EditCommandImpl;
using khtml::ApplyStyleCommandImpl;
using khtml::TypingCommandImpl;
using khtml::EditorContext;
using khtml::IndentOutdentCommandImpl;

// --------------------------------------------------------------------------

namespace DOM {

static const int sMaxUndoSteps = 1000;

class EditorPrivate {
  public:
    void registerUndo(EditCommandImpl *cmd, bool clearRedoStack = true) {
        if (m_undo.count()>= sMaxUndoSteps)
            m_undo.pop_front();
        if (clearRedoStack)
            m_redo.clear();
        m_undo.push(cmd);
    }
    void registerRedo(EditCommandImpl *cmd) {
        if (m_redo.count()>= sMaxUndoSteps)
            m_redo.pop_front();
        m_redo.push(cmd);
    }
    RefPtr<EditCommandImpl> m_lastEditCommand;
    QStack<RefPtr<EditCommandImpl> > m_undo;
    QStack<RefPtr<EditCommandImpl> > m_redo;
};

}

// ==========================================================================

Editor::Editor(KHTMLPart *part)
  : d(new EditorPrivate), m_typingStyle(0), m_part(part) {
}

Editor::~Editor() {
  if (m_typingStyle)
    m_typingStyle->deref();
  delete d;
}

bool Editor::execCommand(const DOMString &command, bool userInterface, const DOMString &value)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->execCommand(imp, userInterface, value);
}

bool Editor::queryCommandEnabled(const DOMString &command)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->queryCommandEnabled(imp);
}

bool Editor::queryCommandIndeterm(const DOMString &command)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->queryCommandIndeterm(imp);
}

bool Editor::queryCommandState(const DOMString &command)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->queryCommandState(imp);
}

bool Editor::queryCommandSupported(const DOMString &command)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->queryCommandSupported(imp);
}

DOMString Editor::queryCommandValue(const DOMString &command)
{
  PREPARE_JSEDITOR_CALL(command, DOMString());
  return js->queryCommandValue(imp);
}

bool Editor::execCommand(EditorCommand command, bool userInterface, const DOMString &value)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->execCommand(imp, userInterface, value);
}

bool Editor::queryCommandEnabled(EditorCommand command)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->queryCommandEnabled(imp);
}

bool Editor::queryCommandIndeterm(EditorCommand command)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->queryCommandIndeterm(imp);
}

bool Editor::queryCommandState(EditorCommand command)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->queryCommandState(imp);
}

bool Editor::queryCommandSupported(EditorCommand command)
{
  PREPARE_JSEDITOR_CALL(command, false);
  return js->queryCommandSupported(imp);
}

DOMString Editor::queryCommandValue(EditorCommand command)
{
  PREPARE_JSEDITOR_CALL(command, DOMString());
  return js->queryCommandValue(imp);
}

void Editor::copy()
{
   static_cast<KHTMLPartBrowserExtension*>(m_part->browserExtension())->copy();
}

void Editor::cut()
{
   // ###
   static_cast<KHTMLPartBrowserExtension*>(m_part->browserExtension())->cut();
}

void Editor::paste()
{
  // ###
  // security?
  // static_cast<KHTMLPartBrowserExtension*>(m_part->browserExtension())->paste();
}

void Editor::print()
{
    static_cast<KHTMLPartBrowserExtension*>(m_part->browserExtension())->print();
}

bool Editor::canPaste() const
{
  // ###
  return false;
}

void Editor::redo()
{
    if (d->m_redo.isEmpty())
        return;
    RefPtr<EditCommandImpl> e = d->m_redo.pop();
    e->reapply();
}

void Editor::undo()
{
    if (d->m_undo.isEmpty())
        return;
    RefPtr<EditCommandImpl> e = d->m_undo.pop();
    e->unapply();
}

bool Editor::canRedo() const
{
    return !d->m_redo.isEmpty();
}

bool Editor::canUndo() const
{
    return !d->m_undo.isEmpty();
}

void Editor::applyStyle(CSSStyleDeclarationImpl *style)
{
  switch (m_part->caret().state()) {
    case Selection::NONE:
            // do nothing
      break;
    case Selection::CARET:
            // FIXME: This blows away all the other properties of the typing style.
      setTypingStyle(style);
      break;
    case Selection::RANGE:
      if (m_part->xmlDocImpl() && style) {
#ifdef DEBUG_COMMANDS
        // qDebug() << "[create ApplyStyleCommand]" << endl;
#endif
        // FIXME
        (new ApplyStyleCommandImpl(m_part->xmlDocImpl(), style))->apply();
      }
      break;
  }
}

static void updateState(CSSStyleDeclarationImpl *desiredStyle, CSSStyleDeclarationImpl *computedStyle, bool &atStart, Editor::TriState &state)
{
  QListIterator<CSSProperty*> it(*desiredStyle->values());
  while (it.hasNext()) {
    int propertyID = it.next()->id();
    DOMString desiredProperty = desiredStyle->getPropertyValue(propertyID);
    DOMString computedProperty = computedStyle->getPropertyValue(propertyID);
    Editor::TriState propertyState = strcasecmp(desiredProperty, computedProperty) == 0
        ? Editor::TrueTriState : Editor::FalseTriState;
    if (atStart) {
      state = propertyState;
      atStart = false;
    } else if (state != propertyState) {
      state = Editor::MixedTriState;
      break;
    }
  }
}

Editor::TriState Editor::selectionHasStyle(CSSStyleDeclarationImpl *style) const
{
  bool atStart = true;
  TriState state = FalseTriState;

  EditorContext *ctx = m_part->editorContext();
  if (ctx->m_selection.state() != Selection::RANGE) {
    NodeImpl *nodeToRemove;
    CSSStyleDeclarationImpl *selectionStyle = selectionComputedStyle(nodeToRemove);
    if (!selectionStyle)
      return FalseTriState;
    selectionStyle->ref();
    updateState(style, selectionStyle, atStart, state);
    selectionStyle->deref();
    if (nodeToRemove) {
      int exceptionCode = 0;
      nodeToRemove->remove(exceptionCode);
      assert(exceptionCode == 0);
    }
  } else {
    for (NodeImpl *node = ctx->m_selection.start().node(); node; node = node->traverseNextNode()) {
      if (node->isHTMLElement()) {
        CSSStyleDeclarationImpl *computedStyle = new RenderStyleDeclarationImpl(node);
        computedStyle->ref();
        updateState(style, computedStyle, atStart, state);
        computedStyle->deref();
        if (state == MixedTriState)
          break;
      }
      if (node == ctx->m_selection.end().node())
        break;
    }
  }

  return state;
}

bool Editor::selectionStartHasStyle(CSSStyleDeclarationImpl *style) const
{
  NodeImpl *nodeToRemove;
  CSSStyleDeclarationImpl *selectionStyle = selectionComputedStyle(nodeToRemove);
  if (!selectionStyle)
    return false;

  selectionStyle->ref();

  bool match = true;

  QListIterator<CSSProperty*> it(*style->values());
  while (it.hasNext()) {
    int propertyID = it.next()->id();
    DOMString desiredProperty = style->getPropertyValue(propertyID);
    DOMString selectionProperty = selectionStyle->getPropertyValue(propertyID);
    if (strcasecmp(selectionProperty, desiredProperty) != 0) {
      match = false;
      break;
    }
  }

  selectionStyle->deref();

  if (nodeToRemove) {
    int exceptionCode = 0;
    nodeToRemove->remove(exceptionCode);
    assert(exceptionCode == 0);
  }

  return match;
}

DOMString Editor::selectionStartStylePropertyValue(int stylePropertyID) const
{
  NodeImpl *nodeToRemove;
  CSSStyleDeclarationImpl *selectionStyle = selectionComputedStyle(nodeToRemove);
  if (!selectionStyle)
    return DOMString();

  selectionStyle->ref();
  DOMString value = selectionStyle->getPropertyValue(stylePropertyID);
  selectionStyle->deref();

  if (nodeToRemove) {
    int exceptionCode = 0;
    nodeToRemove->remove(exceptionCode);
    assert(exceptionCode == 0);
  }

  return value;
}

CSSStyleDeclarationImpl *Editor::selectionComputedStyle(NodeImpl *&nodeToRemove) const
{
  nodeToRemove = 0;

  if (!m_part->xmlDocImpl())
    return 0;

  EditorContext *ctx = m_part->editorContext();
  if (ctx->m_selection.state() == Selection::NONE)
    return 0;

  Range range(ctx->m_selection.toRange());
  Position pos(range.startContainer().handle(), range.startOffset());
  assert(pos.notEmpty());
  ElementImpl *elem = pos.element();
  ElementImpl *styleElement = elem;
  int exceptionCode = 0;

  if (m_typingStyle) {
    styleElement = m_part->xmlDocImpl()->createHTMLElement("SPAN");
//     assert(exceptionCode == 0);

    styleElement->setAttribute(ATTR_STYLE, m_typingStyle->cssText().implementation());
//     assert(exceptionCode == 0);

    TextImpl *text = m_part->xmlDocImpl()->createEditingTextNode("");
    styleElement->appendChild(text, exceptionCode);
    assert(exceptionCode == 0);

    elem->appendChild(styleElement, exceptionCode);
    assert(exceptionCode == 0);

    nodeToRemove = styleElement;
  }

  return new RenderStyleDeclarationImpl(styleElement);
}

PassRefPtr<EditCommandImpl> Editor::lastEditCommand() const
{
  return d->m_lastEditCommand;
}

void Editor::appliedEditing(EditCommandImpl *cmd)
{
#ifdef DEBUG_COMMANDS
    // qDebug() << "[Applied editing]" << endl;
#endif
  // make sure we have all the changes in rendering tree applied with relayout if needed before setting caret
  // in particular that could be required for inline boxes recomputation when inserting text
  m_part->xmlDocImpl()->updateLayout();

  m_part->setCaret(cmd->endingSelection(), false);
    // Command will be equal to last edit command only in the case of typing
  if (d->m_lastEditCommand == cmd) {
    assert(cmd->isTypingCommand());
  } else {
        // Only register a new undo command if the command passed in is
        // different from the last command
        d->registerUndo(cmd);
        d->m_lastEditCommand = cmd;
  }
    m_part->editorContext()->m_selection.setNeedsLayout(true);
    m_part->selectionLayoutChanged();
  // ### only emit if caret pos changed
    m_part->emitCaretPositionChanged(cmd->endingSelection().caretPos());
}

void Editor::unappliedEditing(EditCommandImpl *cmd)
{
  // see comment in appliedEditing()
  m_part->xmlDocImpl()->updateLayout();

  m_part->setCaret(cmd->startingSelection());
  d->registerRedo(cmd);
#ifdef APPLE_CHANGES
  KWQ(this)->respondToChangedContents();
#else
  m_part->editorContext()->m_selection.setNeedsLayout(true);
  m_part->selectionLayoutChanged();
  // ### only emit if caret pos changed
  m_part->emitCaretPositionChanged(cmd->startingSelection().caretPos());
#endif
  d->m_lastEditCommand = 0;
}

void Editor::reappliedEditing(EditCommandImpl *cmd)
{
  // see comment in appliedEditing()
  m_part->xmlDocImpl()->updateLayout();

  m_part->setCaret(cmd->endingSelection());
  d->registerUndo(cmd, false /*clearRedoStack*/);
#ifdef APPLE_CHANGES
  KWQ(this)->respondToChangedContents();
#else
  m_part->selectionLayoutChanged();
  // ### only emit if caret pos changed
  m_part->emitCaretPositionChanged(cmd->endingSelection().caretPos());
#endif
  d->m_lastEditCommand = 0;
}

CSSStyleDeclarationImpl *Editor::typingStyle() const
{
  return m_typingStyle;
}

void Editor::setTypingStyle(CSSStyleDeclarationImpl *style)
{
  CSSStyleDeclarationImpl *old = m_typingStyle;
  m_typingStyle = style;
  if (m_typingStyle)
    m_typingStyle->ref();
  if (old)
    old->deref();
}

void Editor::clearTypingStyle()
{
  setTypingStyle(0);
}

void Editor::closeTyping()
{
    EditCommandImpl *lastCommand = lastEditCommand().get();
    if (lastCommand && lastCommand->isTypingCommand())
        static_cast<TypingCommandImpl*>(lastCommand)->closeTyping();
}

void Editor::indent()
{
    RefPtr<IndentOutdentCommandImpl> command = new IndentOutdentCommandImpl(m_part->xmlDocImpl(), 
            IndentOutdentCommandImpl::Indent);
    command->apply();
}

void Editor::outdent()
{
    RefPtr<IndentOutdentCommandImpl> command = new IndentOutdentCommandImpl(m_part->xmlDocImpl(), 
            IndentOutdentCommandImpl::Outdent);
    command->apply();
}

bool Editor::handleKeyEvent(QKeyEvent *_ke)
{
  bool handled = false;

  bool ctrl  = _ke->modifiers() & Qt::ControlModifier;
  bool alt   = _ke->modifiers() & Qt::AltModifier;
  //bool shift = _ke->modifiers() & Qt::ShiftModifier;
  bool meta  = _ke->modifiers() & Qt::MetaModifier;

  if (ctrl || alt || meta) {
      return false;
  }

  switch(_ke->key()) {

    case Qt::Key_Delete: {
      Selection selectionToDelete = m_part->caret();
#ifdef DEBUG_COMMANDS
      // qDebug() << "========== KEY_DELETE ==========" << endl;
#endif
      if (selectionToDelete.state() == Selection::CARET) {
          Position pos(selectionToDelete.start());
#ifdef DEBUG_COMMANDS
          // qDebug() << "pos.inLastEditableInRootEditableElement " << pos.inLastEditableInRootEditableElement() << " pos.offset " << pos.offset() << " pos.max " << pos.node()->caretMaxRenderedOffset() << endl;
#endif
          if (pos.nextCharacterPosition() == pos) {
              // we're at the end of a root editable block...do nothing
#ifdef DEBUG_COMMANDS
              // qDebug() << "no delete!!!!!!!!!!" << endl;
#endif
              break;
          }
          m_part->d->editor_context.m_selection
                               = Selection(pos, pos.nextCharacterPosition());
      }
      // fall through
    }
    case Qt::Key_Backspace:
      TypingCommandImpl::deleteKeyPressed0(m_part->xmlDocImpl());
      handled = true;
      break;

    case Qt::Key_Return:
    case Qt::Key_Enter:
//       if (shift)
        TypingCommandImpl::insertNewline0(m_part->xmlDocImpl());
//       else
//         TypingCommand::insertParagraph(m_part->xmlDocImpl());
      handled = true;
      break;

    case Qt::Key_Escape:
    case Qt::Key_Insert:
      // FIXME implement me
      handled = true;
      break;

    default:
// handle_input:
      if (m_part->caret().state() != Selection::CARET) {
        // We didn't get a chance to grab the caret, likely because
        // a script messed with contentEditable in the middle of events
        // acquire it now if there isn't a selection
        // qDebug() << "Editable node w/o caret!";
        DOM::NodeImpl* focus = m_part->xmlDocImpl()->focusNode();
        if (m_part->caret().state() == Selection::NONE) {
            if (focus)
                m_part->setCaret(Position(focus, focus->caretMinOffset()));
            else
                break;
        }
      }

      if (!_ke->text().isEmpty()) {
        TypingCommandImpl::insertText0(m_part->xmlDocImpl(), _ke->text());
        handled = true;
      }

  }

  //if (handled) {
    // ### check when to emit it
//     m_part->emitSelectionChanged();
  //}

  return handled;

}



