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

#ifndef EDITING_P_H
#define EDITING_P_H

#include "xml/dom_selection.h"

#include <limits.h>

namespace DOM {
  class Editor;
}

namespace khtml {

/**
 * Contextual information about the caret and the built-in editor
 * @author Leo Savernik
 */
struct EditorContext {

  enum { NoXPosForVerticalArrowNavigation = INT_MIN };

  DOM::Selection::ETextGranularity m_selectionGranularity;

  DOM::Selection m_selection;
  DOM::Selection m_dragCaret;
  int m_caretBlinkTimer;

  bool m_caretVisible:1;
  bool m_caretBlinks:1;
  bool m_caretPaint:1;

  bool m_beganSelectingText:1;

  void beginSelectingText(DOM::Selection::ETextGranularity granularity) {
    m_beganSelectingText   = true;
    m_selectionGranularity = granularity;
  }

  int m_xPosForVerticalArrowNavigation;
  DOM::Editor *m_editor;

  EditorContext()
  : m_caretBlinkTimer(-1),
    m_caretVisible(true),
    m_caretBlinks(true),
    m_caretPaint(true),
    m_beganSelectingText(false),
    m_editor(0)
  {
  }

  ~EditorContext();

  void reset();

};

}/*namespace khtml*/


#endif
