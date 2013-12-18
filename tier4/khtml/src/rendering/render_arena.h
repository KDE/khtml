/*
 * Copyright (C) 2002 Apple Computer, Inc.
 * Copyright (C) 2003 Dirk Mueller (mueller@kde.org)
 *
 * Portions are Copyright (C) 1998 Netscape Communications Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#ifndef RENDERARENA_H
#define RENDERARENA_H

#define KHTML_USE_ARENA_ALLOCATOR

#include "misc/arena.h"
#include "misc/shared.h"

#include <stdlib.h>

namespace khtml {

#define KHTML_MAX_RECYCLED_SIZE 400
#define KHTML_ROUNDUP(x,y) ((((x)+((y)-1))/(y))*(y))

class RenderArena: public Shared<RenderArena> {
public:
   RenderArena(unsigned int arenaSize = 4096);
    ~RenderArena();

  // Memory management functions
  void* allocate(size_t size);
  void  free(size_t size, void* ptr);

private:
  // Underlying arena pool
  ArenaPool m_pool;

  // The recycler array is sparse with the indices being multiples of 4,
  // i.e., 0, 4, 8, 12, 16, 20, ...
  void* m_recyclers[KHTML_MAX_RECYCLED_SIZE >> 2];
};


} // namespace


#endif

