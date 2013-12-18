/*
 * Copyright (C) 2007, 2008 Maksim Orlovich <maksim@kde.org>
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007, 2008 Fredrik Höglund <fredrik@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
 * Portions of this code are (c) by Apple Computer, Inc. and were licensed
 * under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef HTML_CANVASIMPL_H
#define HTML_CANVASIMPL_H

#include "dom/dom_string.h"
#include "html/html_elementimpl.h"
#include "misc/khtmllayout.h"
#include "rendering/render_object.h"

#include <QRegion>
#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QStack>
#include <QPixmap>
#include <QGradient>

namespace khtmlImLoad {
    class CanvasImage;
}

namespace DOM {

class CanvasContext2DImpl;

class HTMLCanvasElementImpl : public HTMLElementImpl
{
public:
    HTMLCanvasElementImpl(DocumentImpl *doc);
    ~HTMLCanvasElementImpl();

    virtual void parseAttribute(AttributeImpl*);
    virtual Id id() const;

    virtual void attach();

    int width () const { return w; }
    int height() const { return h; }

    CanvasContext2DImpl* getContext2D();

    // Note: we use QString here for efficiency reasons. We only need
    // one version since we only do the required image/png
    QString toDataURL(int& exceptionCode);

    // Returns the canvas image, but does not guarantee it's
    // up-to-date
    khtmlImLoad::CanvasImage* getCanvasImage();

    bool isUnsafe() const;
    void markUnsafe();
private:
    int w, h;
    SharedPtr<CanvasContext2DImpl> context;
    bool unsafe;
};

// Base class for representing styles for fill and stroke.
// Not part of the DOM
class CanvasStyleBaseImpl : public khtml::Shared<CanvasStyleBaseImpl>
{
public:
    enum Type {
        Color,
        Gradient,
        Pattern
    };

    virtual bool isUnsafe() const { return false; }
    virtual ~CanvasStyleBaseImpl() {}

    virtual Type   type() const = 0;
    virtual QBrush toBrush() const = 0;
};


// Not part of the DOM.
class CanvasColorImpl : public CanvasStyleBaseImpl
{
public:
    QColor color;

    CanvasColorImpl(const QColor& newColor) : color(newColor)
    {}

    virtual Type type() const { return Color; }

    virtual QBrush toBrush() const {
        return QBrush(color);
    }

    DOM::DOMString toString() const;

    // Note: returns 0 if it can not be parsed.
    static CanvasColorImpl* fromString(const DOM::DOMString &s);
};


class CanvasPatternImpl : public CanvasStyleBaseImpl
{
public:
    CanvasPatternImpl(const QImage& inImg, bool unsafe, bool rx, bool ry);

    virtual Type type() const { return Pattern; }
    virtual QBrush toBrush() const;

    // Returns the rect that a pattern fill or stroke should be clipped to, given
    // the repeat setting and the indicated brush origin and bounding rect.
    // The bounding rect (usually the canvas rect) limits the clip rect extent
    // in the repeating dimension(s).
    QRectF clipForRepeat(const QPointF &origin, const QRectF &bounds) const;

    virtual bool isUnsafe() const { return unsafe; }
private:
    QImage img;
    bool   repeatX, repeatY;
    bool unsafe;
};

class CanvasGradientImpl : public CanvasStyleBaseImpl
{
public:
    CanvasGradientImpl(QGradient* newGradient, float innerRadius = 0.0, bool inverse = false);
    ~CanvasGradientImpl();

    // Our internal interface..
    virtual Type type() const { return Gradient; }
    virtual QBrush toBrush() const;

    // DOM API
    void addColorStop(float offset, const DOM::DOMString& color, int& exceptionCode);
private:
    QGradient* gradient;
    float innerRadius; // In a radial gradient
    bool inverse; // For radial gradients only
};

class CanvasImageDataImpl : public khtml::Shared<CanvasImageDataImpl>
{
public:
    // Creates an uninitialized image..
    CanvasImageDataImpl(unsigned width, unsigned height);
    CanvasImageDataImpl* clone() const;
    

    unsigned width()  const;
    unsigned height() const;
    QColor pixel(unsigned pixelNum) const;
    void   setPixel(unsigned pixelNum, const QColor& val);
    void   setComponent(unsigned pixelNum, int component, int value);
    QImage data;
private:
    CanvasImageDataImpl(const QImage& _data);
};

class CanvasContext2DImpl : public khtml::Shared<CanvasContext2DImpl>
{
public:
    CanvasContext2DImpl(HTMLCanvasElementImpl* element, int width, int height);
    ~CanvasContext2DImpl();

    // Note: the native API does not attempt to validate
    // input for NaN and +/- infinity to raise exceptions;
    // but it does handle them for transformations

    // For renderer..
    void commit();

    // Public API, exported via the DOM.
    HTMLCanvasElementImpl* canvas() const;

    // State management
    void save();
    void restore();

    // Transformations
    void scale(float x, float y);
    void rotate(float angle);
    void translate(float x, float y);
    void transform(float m11, float m12, float m21, float m22, float dx, float dy);
    void setTransform(float m11, float m12, float m21, float m22, float dx, float dy);

    // Composition state setting
    float globalAlpha() const;
    void  setGlobalAlpha(float a);
    DOMString globalCompositeOperation() const;
    void  setGlobalCompositeOperation(const DOMString& op);

    // Colors and styles..
    void setStrokeStyle(CanvasStyleBaseImpl* strokeStyle);
    CanvasStyleBaseImpl* strokeStyle() const;
    void setFillStyle(CanvasStyleBaseImpl* fillStyle);
    CanvasStyleBaseImpl* fillStyle() const;
    CanvasGradientImpl* createLinearGradient(float x0, float y0, float x1, float y1) const;
    CanvasGradientImpl* createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1, int& exceptionCode) const;
    CanvasPatternImpl*  createPattern(ElementImpl* pat, const DOMString& rpt, int& exceptionCode) const;

    // Line attributes
    float lineWidth() const;
    void  setLineWidth(float newLW);
    DOMString lineCap() const;
    void  setLineCap(const DOMString& newCap);
    DOMString lineJoin() const;
    void  setLineJoin(const DOMString& newJoin);
    float miterLimit() const;
    void  setMiterLimit(float newML);

    // Shadow attributes
    float shadowOffsetX() const;
    void  setShadowOffsetX(float newOX);
    float shadowOffsetY() const;
    void  setShadowOffsetY(float newOY);
    float shadowBlur() const;
    void  setShadowBlur(float newBlur);
    DOMString shadowColor() const;
    void  setShadowColor(const DOMString& newColor);

    // Rectangle operations
    void clearRect (float x, float y, float w, float h, int& exceptionCode);
    void fillRect  (float x, float y, float w, float h, int& exceptionCode);
    void strokeRect(float x, float y, float w, float h, int& exceptionCode);

    // Path-based ops
    void beginPath();
    void closePath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void quadraticCurveTo(float cpx, float cpy, float x, float y);
    void bezierCurveTo   (float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
    void arcTo(float x1, float y1, float x2, float y2, float radius, int& exceptionCode);
    void rect(float x, float y, float w, float h, int& exceptionCode);
    void arc(float x, float y, float radius, float startAngle, float endAngle,
             bool ccw, int& exceptionCode);
    void fill();
    void stroke();
    void clip();
    bool isPointInPath(float x, float y) const;

    // Image ops
    void drawImage(ElementImpl* image, float dx, float dy, int& exceptionCode);
    void drawImage(ElementImpl* image, float dx, float dy, float dw, float dh, int& exceptionCode);
    void drawImage(ElementImpl* image, float sx, float sy, float sw, float sh,
                   float dx, float dy, float dw, float dh, int& exceptionCode);

    // Pixel ops
    CanvasImageDataImpl* getImageData(float sx, float sy, float sw, float sh, int& exceptionCode);
    void putImageData(CanvasImageDataImpl* data, float dx, float dy, int& exceptionCode);
    CanvasImageDataImpl* createImageData(float sw, float sh, int& exceptionCode);

private:
    friend class HTMLCanvasElementImpl;

    enum PathPaintOp { DrawFill, DrawStroke };
    enum PaintFlags { NoPaintFlags = 0, NotUsingCanvasPattern = 1 };

    // initialize canvas for new size
    void resetContext(int width, int height);

    bool isPathEmpty() const;

    bool needsShadow() const;

    // Returns the clip path in device coordinates that the fill or stroke should be
    // limited to, given the repeat setting of the pattern that will be used.
    QPainterPath clipForPatternRepeat(QPainter *painter, const PathPaintOp op) const;

    // Helper function that fills or strokes a path, honoring shadow and pattern
    // repeat settings.
    void drawPath(QPainter *painter, const QPainterPath &path, PathPaintOp op) const;

    // Draws a shadowed path using the painter.
    void drawPathWithShadow(QPainter *painter, const QPainterPath &path, PathPaintOp op,
                            PaintFlags flags = NoPaintFlags) const;

    void drawImage(QPainter *painter, const QRectF &dstRect, const QImage &image,
                   const QRectF &srcRect) const;

    // Cleared by canvas dtor..
    HTMLCanvasElementImpl* canvasElement;

    // Helper for methods that take images via elements; this will extract them,
    // and signal an exception if needed be. It will also return if the
    // image is unsafe
    QImage extractImage(ElementImpl* el, int& exceptionCode, bool& unsafeOut) const;

    // This method will prepare a painter for use, making sure it's active,
    // that all the dirty state got sync'd, etc, and will mark the
    // canvas as dirty so the renderer can flush everything, etc.
    QPainter* acquirePainter();

    // We use khtmlImLoad to manage our canvas image, so that
    // the Renderer can scale easily.
    khtmlImLoad::CanvasImage* canvasImage;

    // The path is global, and never saved/restored
    QPainterPath path;

    // We keep track of non-path state ourselves. There are two reasons:
    // 1) The painter may have to be end()ed so we can not rely on it to remember
    //    things
    // 2) The stroke and fill style can actually be DOM objects, so we have to
    //    be able to hand them back
    // "The canvas state" section of the spec describes what's included here
    struct PaintState {
        // Viewport state (not readable from outside)
        QTransform    transform;
        bool          infinityTransform; // Marks that the transform has become invalid,
                                         // and should be treated as identity.
        QPainterPath  clipPath; // This is in -physical- coordinates
        bool          clipping; // If clipping is enabled

        // Compositing state
        float globalAlpha;
        QPainter::CompositionMode globalCompositeOperation;

        // Stroke and fill styles.
        SharedPtr<CanvasStyleBaseImpl> strokeStyle;
        SharedPtr<CanvasStyleBaseImpl> fillStyle;

        // Line stuff
        float            lineWidth;
        Qt::PenCapStyle  lineCap;
        Qt::PenJoinStyle lineJoin;
        float            miterLimit;

        // Shadow stuff
        float shadowOffsetX;
        float shadowOffsetY;
        float shadowBlur;
        QColor shadowColor;
    };

    // The stack of states. The entry on the top is always the current state.
    QStack<PaintState> stateStack;

    const PaintState& activeState() const { return stateStack.top(); }
    PaintState& activeState() { return stateStack.top(); }

    QPointF mapToDevice(const QPointF &point) const {
        return point * stateStack.top().transform;
    }

    QPointF mapToDevice(float x, float y) const {
        return QPointF(x, y) * stateStack.top().transform;
    }

    QPointF mapToUser(const QPointF &point) const {
        return point * stateStack.top().transform.inverted();
    }

    enum DirtyFlags {
        DrtTransform = 0x01,
        DrtClip      = 0x02,
        DrtAlpha     = 0x04,
        DrtCompOp    = 0x08,
        DrtStroke    = 0x10,
        DrtFill      = 0x20,
        DrtAll       = 0xFF
    };

    // The painter for working on the QImage. Might not be active.
    // Should not be used directly, but rather via acquirePainter
    QPainter workPainter;

    // How out-of-date the painter is, if it is active.
    int dirty;

    // If we have not committed all the changes to the entire
    // scaler hierarchy
    bool needsCommit;

    // Tells the element and then the renderer that we changed, so need
    // repainting.
    void needRendererUpdate();

    // Flushes changes to the backbuffer.
    void syncBackBuffer();

    // False if no user moveTo-element was inserted in the path
    // see CanvasContext2DImpl::beginPath()
    bool emptyPath;
};


} //namespace

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
