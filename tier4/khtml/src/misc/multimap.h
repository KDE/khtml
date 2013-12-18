/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2006 Allan Sandfeld Jensen (kde@carewolf.com)
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

#ifndef _MultiMap_h_
#define _MultiMap_h_


#include <assert.h>
#include <stdlib.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QSet>

template<class T> class MultiMapPtrList;

template<class K, class T>
class KMultiMap {
public:
    typedef QSet<T*> Set;
    typedef QHash<K*, Set*> Map;
    typedef MultiMapPtrList<T*> List;

    KMultiMap() {}
    ~KMultiMap() {
        qDeleteAll(map);
        map.clear();
    }

    void insert(K* key, T* element)
    {
        Set *set = map.value(key);
        if (!set){
             set = new Set();
             map.insert(key, set);
        }
        if (!set->contains(element))
            set->insert(element);
    }
    void remove(K* key, T* element) 
    {
        Set *set = map.value(key);
        if (set) {
            set->remove(element);
            if (set->isEmpty()) {
                map.remove(key);
                delete set;
            }
        }
    }
    void remove(K* key) {
        Set *set = map.value(key);
        if (set) {
            map.remove(key);
            delete set;
        }
    }
    Set* find(K* key) {
        return map.value(key);
    }
private:
    Map map;
};

static inline unsigned int stupidHash(void* ptr)
{
    unsigned long val = (unsigned long)ptr;
    // remove alignment and multiply by a prime unlikely to be a factor of size
    val = (val >> 4) * 1237;
    return val;
}

#define START_PTRLIST_SIZE 4
#define MAX_PTRLIST_SIZE 27

class PtrListEntry {
public:
    PtrListEntry(unsigned int log_size) : count(0), log_size(log_size), search(log_size), next(0) {
//         entry = new T* [size];
        assert(log_size < MAX_PTRLIST_SIZE);
        entry = (void**)calloc ((1<<log_size), sizeof(void*));
    }
    ~PtrListEntry() {
//         delete[] entry;
        free(entry);
    }
    bool insert(void* e) {
        unsigned int t_size = size();
        if (count == t_size) return false;
        unsigned int hash = stupidHash(e);
        void** firstFree = 0;
        // Only let elements be placed 'search' spots from their hash
        for(unsigned int j=0; j<search; j++) {
            unsigned int i = (hash + j) & (t_size-1); // modulo size
            // We need check to all hashes in 'search' to garuantee uniqueness
            if (entry[i] == 0) {
                if (!firstFree)
                    firstFree = entry + i;
            } else
            if (entry[i] == e)
                return true;
        }
        if (firstFree) {
            *firstFree = e;
            count++;
            return true;
        }
        // We had more than 'search' collisions
        if (count < (t_size/3)*2) {
            // only 2/3 full => increase search
            unsigned int s = search *2;
            if (s >= t_size) s = t_size;
            search = s;
            return insert(e);
        }
        return false;
    }
    // Insert another smaller set into this one
    // Is only garuantied to succede when this PtrList is new
    void insert(PtrListEntry* listEntry) {
        assert(size() >= listEntry->count * 2);
        unsigned int old_size = 1U << listEntry->log_size;
        for(unsigned int i = 0; i < old_size; i++) {
            bool s = true;
            void *e = listEntry->entry[i];
            if (e) s = insert(e);
            assert(s);
        }
    }
    bool remove(void* e) {
        if (count == 0) return false;
        unsigned int size = (1U<<log_size);
        unsigned int hash = stupidHash(e);
        // Elements are at most placed 'search' spots from their hash
        for(unsigned int j=0; j<search; j++) {
            unsigned int i = (hash + j) & (size-1); // modulo size
            if (entry[i] == e) {
                entry[i] = 0;
                count--;
                return true;
            }
        }
        return false;
    }
    bool contains(void* e) {
        if (count == 0) return false;
        unsigned int t_size = size();
        unsigned int hash = stupidHash(e);
        // Elements are at most placed 'search' spots from their hash
        for(unsigned int j=0; j<search; j++) {
            unsigned int i = (hash + j) & (t_size-1); // modulo size
            if (entry[i] == e) return true;
        }
        return false;
    }
    void* at(unsigned int i) const {
        assert (i < (1U<<log_size));
        return entry[i];
    }
    bool isEmpty() const {
        return count == 0;
    }
    bool isFull() const {
        return count == size();
    }
    unsigned int size() const {
        return (1U << log_size);
    }

    unsigned int count;
    const unsigned short log_size;
    unsigned short search;
    PtrListEntry *next;
    void** entry;
};

// An unsorted and unique PtrList that is implement as a linked list of hash-sets
// Optimized for fast insert and fast lookup
template<class T>
class MultiMapPtrList {
public:
    MultiMapPtrList(unsigned int init_size= 16) : m_first(0), m_current(0), m_pos(0) {
        assert(init_size > 0);
        unsigned int s = init_size - 1;
        unsigned int log_size = 0;
        while (s > 0) {
            log_size++;
            s = s >> 1;
        }
        m_first = new PtrListEntry(log_size);
    }
    MultiMapPtrList(const MultiMapPtrList& ptrList) : m_first(0), m_current(0), m_pos(0) {
        unsigned int t_count = ptrList.count();
        unsigned int log_size = 0;
        while (t_count > 0) {
            log_size++;
            t_count = t_count >> 1;
        }
        // At least as large as the largest ptrListEntry in the original
        if (t_count < ptrList.m_first->log_size) log_size = ptrList.m_first->log_size;
        m_first = new PtrListEntry(log_size);

        PtrListEntry *t_current = ptrList.m_first;
        while (t_current) {
            unsigned int t_size = t_current->size();
            for(unsigned int i=0; i < t_size; i++) {
                void* e = t_current->at(i);
                if (e != 0) {
                    bool t = m_first->insert(e);
                    if (!t) {
                        // Make a new, but keep the size
                        PtrListEntry *t_new = new PtrListEntry(log_size);
                        t_new->insert(e);
                        t_new->next = m_first;
                        m_first = t_new;
                    }
                }
            }
            t_current = t_current->next;
        }
    }
    ~MultiMapPtrList() {
        PtrListEntry *t_next, *t_current = m_first;
        while (t_current) {
            t_next = t_current->next;
            delete t_current;
            t_current = t_next;
        }
    }
    void append(T* e) {
        PtrListEntry *t_last = 0, *t_current = m_first;
        int count = 0;
        while (t_current) {
            if (t_current->insert(e)) return;
            t_last = t_current;
            t_current = t_current->next;
            count++;
        }
        // Create new hash-set
        unsigned int newsize = m_first->log_size+1;
        if (newsize > MAX_PTRLIST_SIZE) newsize = MAX_PTRLIST_SIZE;
        t_current = new PtrListEntry(newsize);
        bool t = t_current->insert(e);
        assert(t);
        // Prepend it to the list, for insert effeciency
        t_current->next = m_first;
        m_first = t_current;
        // ### rehash some of the smaller sets
        /*
        if (count > 4) {
            // rehash the last in the new
            t_current->insert(t_last);
        }*/
    }
    void remove(T* e) {
        PtrListEntry *t_next, *t_last = 0, *t_current = m_first;
        // Remove has to check every PtrEntry.
        while (t_current) {
            t_next = t_current->next;
            if (t_current->remove(e) && t_current->isEmpty()) {
                if (t_last) {
                    t_last->next = t_current->next;
                }
                else {
                    assert (m_first == t_current);
                    m_first = t_current->next;
                }
                delete t_current;
            } else {
                t_last = t_current;
            }
            t_current = t_next;
        }
    }
    bool contains(T* e) {
        PtrListEntry *t_current = m_first;
        while (t_current) {
            if (t_current->contains(e)) return true;
            t_current = t_current->next;
        }
        return false;
    }
    bool isEmpty() {
        if (!m_first) return true;
        PtrListEntry *t_current = m_first;
        while (t_current) {
            if (!t_current->isEmpty()) return false;
            t_current = t_current->next;
        }
        return true;
    }
    unsigned int count() const {
        unsigned int t_count = 0;
        PtrListEntry *t_current = m_first;
        while (t_current) {
            t_count += t_current->count;
            t_current = t_current->next;
        }
        return t_count;
    }
// Iterator functions:
    T* first() {
        m_current = m_first;
        m_pos = 0;
        // skip holes
        if (m_current && !m_current->at(m_pos))
            return next();
        else
            return current();
    }
    T* current() {
        if (!m_current)
            return (T*)0;
        else
            return (T*)m_current->at(m_pos);
    }
    T* next() {
        if (!m_current) return (T*)0;
        m_pos++;
        if (m_pos >= m_current->size()) {
            m_current = m_current->next;
            m_pos = 0;
        }
        // skip holes
        if (m_current && !m_current->at(m_pos))
            return next();
        else
            return current();
    }
private:
    PtrListEntry *m_first;
// iteration:
    PtrListEntry *m_current;
    unsigned int m_pos;
};

#undef START_PTRLIST_SIZE
#undef MAX_PTRLIST_SIZE
#endif
