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

#include "render_arena.h"

#include <string.h>
#include <assert.h>

using namespace khtml;

namespace khtml {

typedef struct {
    RenderArena *arena;
    size_t size;
} RenderArenaDebugHeader;

#ifdef VALGRIND_SUPPORT
Arena* findContainingArena(ArenaPool* pool, void* ptr) {
    uword ptrBits = reinterpret_cast<uword>(ptr);
    for (Arena* a = &pool->first; a; a = a->next)
        if (ptrBits >= a->base && ptrBits < a->limit)
            return a;
    return 0; //Should not happen
}
#endif

RenderArena::RenderArena(unsigned int arenaSize)
{
    // Initialize the arena pool
    INIT_ARENA_POOL(&m_pool, "RenderArena", arenaSize);

    // Zero out the recyclers array
    memset(m_recyclers, 0, sizeof(m_recyclers));
}

RenderArena::~RenderArena()
{
    // Free the arena in the pool and finish using it
    FreeArenaPool(&m_pool);
}

void* RenderArena::allocate(size_t size)
{
#ifndef KHTML_USE_ARENA_ALLOCATOR
    // Use standard malloc so that memory debugging tools work.
    void *block = ::malloc(sizeof(RenderArenaDebugHeader) + size);
    RenderArenaDebugHeader *header = (RenderArenaDebugHeader *)block;
    header->arena = this;
    header->size = size;
    return header + 1;
#else
    void* result = 0;

    // Ensure we have correct alignment for pointers.  Important for Tru64
    size = KHTML_ROUNDUP(size, sizeof(void*));

    // Check recyclers first
    if (size < KHTML_MAX_RECYCLED_SIZE) {
        const int   index = size >> 2;

        result = m_recyclers[index];
        if (result) {
#ifdef VALGRIND_SUPPORT
            VALGRIND_MEMPOOL_ALLOC(findContainingArena(&m_pool, result)->base, result, size);
#endif
            // Need to move to the next object
            void* next = *((void**)result);
            m_recyclers[index] = next;
        }
    }

    if (!result) {
        // Allocate a new chunk from the arena
        ARENA_ALLOCATE(result, &m_pool, size);
    }

    return result;
#endif
}

void RenderArena::free(size_t size, void* ptr)
{
#ifndef KHTML_USE_ARENA_ALLOCATOR
    // Use standard free so that memory debugging tools work.
    assert(this);
    RenderArenaDebugHeader *header = (RenderArenaDebugHeader *)ptr - 1;
    //assert(header->size == size);
    //assert(header->arena == this);
    ::free(header);
#else

#ifdef VALGRIND_SUPPORT
    VALGRIND_MEMPOOL_FREE(findContainingArena(&m_pool, ptr)->base, ptr);
#endif

    // Ensure we have correct alignment for pointers.  Important for Tru64
    size = KHTML_ROUNDUP(size, sizeof(void*));

    // See if it's a size that we recycle
    if (size < KHTML_MAX_RECYCLED_SIZE) {
        const int   index = size >> 2;
        void*       currentTop = m_recyclers[index];
        m_recyclers[index] = ptr;
        *((void**)ptr) = currentTop;
    }
#endif
}

}
