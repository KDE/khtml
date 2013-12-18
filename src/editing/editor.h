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

#ifndef __EDITOR_H
#define __EDITOR_H

#include "editor_command.h"

#include <khtml_export.h>

#include <QObject>

#include "wtf/PassRefPtr.h"

class QKeyEvent;

class KHTMLPart;
class KHTMLView;
class KHTMLEditorPart;

namespace khtml {
  class EditCommandImpl;
  struct EditorContext;
}

namespace DOM {

class Range;
class NodeImpl;
class ElementImpl;
class DOMString;
class CSSStyleDeclarationImpl;
class EditorPrivate;

/**
 * This class resembles the editing API when the associated khtml document
 * is editable (in design mode), or contains editable elements.
 *
 * FIXME: document this thoroughly
 *
 * @short API to Wysiwyg Markup-Editor.
 * @author Leo Savernik
 */
class KHTML_EXPORT Editor : public QObject {
  Q_OBJECT

  Editor(KHTMLPart *);
  virtual ~Editor();
public:

  /**
   * Tri-state boolean.
   */
  enum TriState { FalseTriState, TrueTriState, MixedTriState };

  // == interface to editor commands

  /**
   * Executes the given editor command.
   * @param command name of command
   * @param userInterface whether a user interface should be used to input data. This is command dependent.
   * @param value value for command. Its semantic depends on the command.
   */
  bool execCommand(const DOMString &command, bool userInterface, const DOMString &value);
  /** Checks whether the given command is enabled. */
  bool queryCommandEnabled(const DOMString &command);
  /** Checks whether the given command's style is indeterminate */
  bool queryCommandIndeterm(const DOMString &command);
  /** Checks whether the given command's style is state */
  bool queryCommandState(const DOMString &command);
  /** Checks whether the given command is supported in the current context */
  bool queryCommandSupported(const DOMString &command);
  /** Returns the given command's value */
  DOMString queryCommandValue(const DOMString &command);

  /**
   * Executes the given built-in editor command.
   * @param EditorCommand index of command
   * @param userInterface whether a user interface should be used to input data. This is command dependent.
   * @param value value for command. Its semantic depends on the command.
   */
  bool execCommand(EditorCommand, bool userInterface, const DOMString &value);
  /** Checks whether the given command is enabled. */
  bool queryCommandEnabled(EditorCommand);
  /** Checks whether the given command's style is indeterminate */
  bool queryCommandIndeterm(EditorCommand);
  /** Checks whether the given command's style is state */
  bool queryCommandState(EditorCommand);
  /** Checks whether the given command is supported in the current context */
  bool queryCommandSupported(EditorCommand);
  /** Returns the given command's value */
  DOMString queryCommandValue(EditorCommand);

  // == direct interface to some built-in commands

  /** copy selection to clipboard */
  void copy();
  /** cut selection and insert into clipboard */
  void cut();
  /** paste into current selection from clipboard */
  void paste();
  /** returns whether clipboard contains data to be pasted */
  bool canPaste() const;
  /** redo last undone action */
  void redo();
  /** undo last action */
  void undo();
  /** returns whether any actions can be redone */
  bool canRedo() const;
  /** returns whether any actions can be undone */
  bool canUndo() const;
  /** applies the given style to the current selection */
  void applyStyle(DOM::CSSStyleDeclarationImpl *);
  /** returns whether the selection has got applied the given style */
  TriState selectionHasStyle(DOM::CSSStyleDeclarationImpl *) const;
  /** returns whether the selection has got applied the given style */
  bool selectionStartHasStyle(DOM::CSSStyleDeclarationImpl *) const;
  /** ? */
  DOM::DOMString selectionStartStylePropertyValue(int stylePropertyID) const;
  /** prints the current document */
  void print();
  /** computed style of current selection */
  DOM::CSSStyleDeclarationImpl *selectionComputedStyle(DOM::NodeImpl *&nodeToRemove) const;


  // == ### more stuff I'm not sure about whether it should be public

  /**
   * Returns the most recent edit command applied.
   */
  WTF::PassRefPtr<khtml::EditCommandImpl> lastEditCommand() const;

  /**
   * Called when editing has been applied.
   */
  void appliedEditing(khtml::EditCommandImpl *);

  /**
   * Called when editing has been unapplied.
   */
  void unappliedEditing(khtml::EditCommandImpl *);

  /**
   * Called when editing has been reapplied.
   */
  void reappliedEditing(khtml::EditCommandImpl *);

  /**
   * Returns the typing style for the document.
   */
  DOM::CSSStyleDeclarationImpl *typingStyle() const;

  /**
   * Sets the typing style for the document.
   */
  void setTypingStyle(DOM::CSSStyleDeclarationImpl *);

  /**
   * Clears the typing style for the document.
   */
  void clearTypingStyle();

  void closeTyping();

  /**
   * indent/outdent current selection
   */
  void indent();
  void outdent();

private:
  /** Handles key events. Returns true if event has been handled. */
  bool handleKeyEvent(QKeyEvent *);

private:
  EditorPrivate *const d;

  DOM::CSSStyleDeclarationImpl *m_typingStyle;

  KHTMLPart *m_part;

  friend class ::KHTMLPart;
  friend class ::KHTMLView;
  friend class ::KHTMLEditorPart;
  friend struct khtml::EditorContext;
  friend class DOM::ElementImpl;
};

}/*namespace DOM*/

#endif // __EDITOR_H
