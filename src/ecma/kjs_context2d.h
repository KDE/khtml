/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2004 Apple Computer, Inc.
 *  Copyright (C) 2005 Zack Rusin <zack@kde.org>
 *  Copyright (C) 2007, 2008 Maksim Orlovich <maksim@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef KJS_CONTEXT2D_H
#define KJS_CONTEXT2D_H

#include "kjs_dom.h"
#include "kjs_binding.h"
#include "kjs_html.h"
#include <kjs/object.h>

#include "misc/loader_client.h"
#include "html/html_canvasimpl.h"

#include <QPainterPath>

namespace DOM
{
class HTMLElementImpl;
}

namespace KJS
{

////////////////////// Context2D Object ////////////////////////
DEFINE_PSEUDO_CONSTRUCTOR(Context2DPseudoCtor)

class Context2D : public DOMWrapperObject<DOM::CanvasContext2DImpl>
{
    friend class Context2DFunction;
public:
    Context2D(ExecState *exec, DOM::CanvasContext2DImpl *ctx);

    using KJS::JSObject::getOwnPropertySlot;
    bool getOwnPropertySlot(ExecState *, const Identifier &, PropertySlot &) override;
    JSValue *getValueProperty(ExecState *exec, int token) const;
    using KJS::JSObject::put;
    void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None) override;
    void putValueProperty(ExecState *exec, int token, JSValue *value, int /*attr*/);

    const ClassInfo *classInfo() const override
    {
        return &info;
    }
    static const ClassInfo info;

    enum {
        Canvas,
        Save, Restore, // state
        Scale, Rotate, Translate, Transform, SetTransform, // transformations
        GlobalAlpha, GlobalCompositeOperation,             // compositing
        StrokeStyle, FillStyle, CreateLinearGradient, CreateRadialGradient, CreatePattern, // colors and styles.
        LineWidth, LineCap, LineJoin, MiterLimit, // line properties
        ShadowOffsetX, ShadowOffsetY, ShadowBlur, ShadowColor, // shadow properties
        ClearRect, FillRect, StrokeRect,          // rectangle ops
        BeginPath, ClosePath, MoveTo, LineTo, QuadraticCurveTo, BezierCurveTo, ArcTo, Rect, Arc,
        Fill, Stroke, Clip, IsPointInPath,        // paths
        DrawImage,  // do we want backwards compat for drawImageFromRect?
        GetImageData, PutImageData, CreateImageData // pixel ops. ewww.
    };
};

class CanvasGradient : public DOMWrapperObject<DOM::CanvasGradientImpl>
{
public:
    CanvasGradient(ExecState *exec, DOM::CanvasGradientImpl *impl);

    const ClassInfo *classInfo() const override
    {
        return &info;
    }
    static const ClassInfo info;

    enum {
        AddColorStop
    };
};

class CanvasPattern : public DOMWrapperObject<DOM::CanvasPatternImpl>
{
public:
    CanvasPattern(ExecState *exec, DOM::CanvasPatternImpl *i);

    const ClassInfo *classInfo() const override
    {
        return &info;
    }
    static const ClassInfo info;
};

// Return 0 if conversion failed
DOM::CanvasImageDataImpl *toCanvasImageData(ExecState *exec, JSValue *val);

class CanvasImageDataArray;
class CanvasImageData : public DOMWrapperObject<DOM::CanvasImageDataImpl>
{
public:
    CanvasImageData(ExecState *exec, DOM::CanvasImageDataImpl *i);

    const ClassInfo *classInfo() const override
    {
        return &info;
    }
    static const ClassInfo info;

    JSObject *valueClone(Interpreter *targetCtx) const override;

    void mark() override;
private:
    CanvasImageDataArray *data;
};

class CanvasImageDataArray : public JSObject
{
public:
    CanvasImageDataArray(ExecState *exec, CanvasImageData *p);

    const ClassInfo *classInfo() const override
    {
        return &info;
    }
    static const ClassInfo info;

    void mark() override;

    // Performs conversion/claming/rounding of color components as specified in HTML5 spec.
    static unsigned char decodeComponent(ExecState *exec, JSValue *val);

    bool getOwnPropertySlot(ExecState *exec, const Identifier &propertyName, PropertySlot &slot) override;
    bool getOwnPropertySlot(ExecState *exec, unsigned index, PropertySlot &slot) override;
    void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr) override;
    void put(ExecState *exec, unsigned index, JSValue *value, int attr) override;

    JSValue *indexGetter(ExecState *exec, unsigned index);
private:
    unsigned size;
    CanvasImageData *parent;
};

DEFINE_PSEUDO_CONSTRUCTOR(SVGAnglePseudoCtor)

} // namespace

#endif
