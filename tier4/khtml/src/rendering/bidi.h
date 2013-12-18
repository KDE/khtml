/*
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003 Apple Computer, Inc.
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
#ifndef BIDI_H
#define BIDI_H

#include <QtCore/QString>
#include "rendering/render_object.h"

namespace khtml {
    class RenderArena;
        class RenderObject;
    class InlineBox;

    class BidiContext {
    public:
	BidiContext(unsigned char level, QChar::Direction embedding, BidiContext *parent = 0, bool override = false);
	~BidiContext();

	void ref() const;
	void deref() const;

	unsigned char level;
	bool override : 1;
	QChar::Direction dir : 5;
	QChar::Direction basicDir : 5;

	BidiContext *parent;


	// refcounting....
	mutable int count;
    };

    struct BidiRun {
	BidiRun(int _start, int _stop, RenderObject *_obj, BidiContext *context, QChar::Direction dir)
	    :  start( _start ), stop( _stop ), obj( _obj ), box(0), nextRun(0)
	{
	    if(dir == QChar::DirON) dir = context->dir;

	    level = context->level;

	    // add level of run (cases I1 & I2)
	    if( level % 2 ) {
		if(dir == QChar::DirL || dir == QChar::DirAN || dir == QChar::DirEN)
		    level++;
	    } else {
		if( dir == QChar::DirR )
		    level++;
		else if( dir == QChar::DirAN || dir == QChar::DirEN)
		    level += 2;
	    }
	}

        void detach(RenderArena* renderArena);

        // Overloaded new operator.
        void* operator new(size_t sz, RenderArena* renderArena) throw();

        // Overridden to prevent the normal delete from being called.
        void operator delete(void* ptr, size_t sz);

private:
        // The normal operator new is disallowed.
        void* operator new(size_t sz) throw();

public:
	int start;
	int stop;

        RenderObject *obj;
        InlineBox* box;

	// explicit + implicit levels here
	uchar level;

        bool compact : 1;

        BidiRun* nextRun;
    };

    struct BidiIterator;
    struct BidiState;

    struct BidiStatus {
        BidiStatus() : eor(QChar::DirON), lastStrong(QChar::DirON), last(QChar::DirON) {}

        QChar::Direction eor;
        QChar::Direction lastStrong;
        QChar::Direction last;
    };

    struct InlineMinMaxIterator
    {
    /* InlineMinMaxIterator is a class that will iterate over all render objects that contribute to
       inline min/max width calculations.  Note the following about the way it walks:
       (1) Positioned content is skipped (since it does not contribute to min/max width of a block)
       (2) We do not drill into the children of floats or replaced elements, since you can't break
           in the middle of such an element.
       (3) Inline flows (e.g., <a>, <span>, <i>) are walked twice, since each side can have
           distinct borders/margin/padding that contribute to the min/max width.
    */
        RenderObject* parent;
        RenderObject* current;
        bool endOfInline;
        bool skipPositioned;
        InlineMinMaxIterator(RenderObject* p, RenderObject* o, bool eOI=false, bool skipPos=true)
            :parent(p), current(o), endOfInline(eOI), skipPositioned(skipPos) {}
        inline RenderObject* next();
    };

    inline RenderObject* InlineMinMaxIterator::next()
    {
        RenderObject* result = 0;
        bool oldEndOfInline = endOfInline;
        endOfInline = false;
        while (current != 0 || (current == parent))
        {
            //qDebug() << "current = " << current;
            if (!oldEndOfInline &&
                (current == parent ||
                (!current->isFloating() && !current->isReplaced() && !current->isPositioned())))
                result = current->firstChild();
            if (!result) {
                // We hit the end of our inline. (It was empty, e.g., <span></span>.)
                if (!oldEndOfInline && current->isInlineFlow()) {
                    result = current;
                    endOfInline = true;
                    break;
                }
                while (current && current != parent) {
                    result = current->nextSibling();
                    if (result) break;
                    current = current->parent();
                    if (current && current != parent && current->isInlineFlow()) {
                        result = current;
                        endOfInline = true;
                        break;
                    }
                }
            }

            if (!result) break;

            if ((!skipPositioned || !result->isPositioned()) && (result->isText() || result->isBR() ||
                result->isFloatingOrPositioned() || result->isReplaced() || result->isGlyph() || result->isInlineFlow()))
                break;

            current = result;
            result = 0;
        }

        // Update our position.
        current = result;
        return current;
    }
}

#endif
