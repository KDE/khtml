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

#include "editing_p.h"

#include "editor.h"

using DOM::Selection;
using namespace khtml;

EditorContext::~EditorContext() {
  delete m_editor;
}

void EditorContext::reset() {
  m_selection = Selection();
  m_dragCaret = Selection();
  
  m_caretBlinkTimer = -1;
  m_caretVisible = true;
  m_caretBlinks = true;
  m_caretPaint = true;
  m_beganSelectingText = false;

  delete m_editor;
  m_editor = 0;

}
