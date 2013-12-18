// -*- c-basic-offset: 4 -*-
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

#include "kjs_context2d.h"

#include "kjs_html.h"

#include <misc/loader.h>
#include <dom/dom_exception.h>
#include <xml/dom2_eventsimpl.h>
#include <xml/dom_textimpl.h>
#include <html/html_baseimpl.h>
#include <html/html_blockimpl.h>
#include <html/html_canvasimpl.h>
#include <html/html_documentimpl.h>
#include <html/html_formimpl.h>
#include <html/html_headimpl.h>
#include <html/html_imageimpl.h>
#include <html/html_inlineimpl.h>
#include <html/html_listimpl.h>
#include <html/html_objectimpl.h>
#include <html/html_tableimpl.h>

#include <imload/imagemanager.h>

#include <khtml_part.h>
#include <khtmlview.h>

#include "kjs_css.h"
#include "kjs_window.h"
#include "kjs_events.h"
#include "kjs_proxy.h"
#include <kjs/operations.h>

#include <rendering/render_canvasimage.h>
#include <rendering/render_object.h>
#include <rendering/render_layer.h>

#include <QDebug>

#include <css/cssparser.h>
#include <css/css_stylesheetimpl.h>
#include <css/css_ruleimpl.h>


using namespace DOM;

#include "kjs_context2d.lut.h"

namespace KJS {

////////////////////// Context2D Object ////////////////////////

KJS_DEFINE_PROTOTYPE(Context2DProto)
KJS_IMPLEMENT_PROTOFUNC(Context2DFunction)
KJS_IMPLEMENT_PROTOTYPE("CanvasRenderingContext2DProto", Context2DProto, Context2DFunction, ObjectPrototype)

/*
   @begin Context2DProtoTable 48
   #
   # state ops
   save                     Context2D::Save                        DontDelete|Function 0
   restore                  Context2D::Restore                     DontDelete|Function 0
   #
   # transformations
   scale                    Context2D::Scale                       DontDelete|Function 2
   rotate                   Context2D::Rotate                      DontDelete|Function 2
   translate                Context2D::Translate                   DontDelete|Function 1
   transform                Context2D::Transform                   DontDelete|Function 6
   setTransform             Context2D::SetTransform                DontDelete|Function 6
   #
   # colors and styles
   createLinearGradient     Context2D::CreateLinearGradient        DontDelete|Function 4
   createRadialGradient     Context2D::CreateRadialGradient        DontDelete|Function 6
   createPattern            Context2D::CreatePattern               DontDelete|Function 2
   #
   # rectangle ops
   clearRect                Context2D::ClearRect                   DontDelete|Function 4
   fillRect                 Context2D::FillRect                    DontDelete|Function 4
   strokeRect               Context2D::StrokeRect                  DontDelete|Function 4
   #
   # paths
   beginPath                Context2D::BeginPath                   DontDelete|Function 0
   closePath                Context2D::ClosePath                   DontDelete|Function 0
   moveTo                   Context2D::MoveTo                      DontDelete|Function 2
   lineTo                   Context2D::LineTo                      DontDelete|Function 2
   quadraticCurveTo         Context2D::QuadraticCurveTo            DontDelete|Function 4
   bezierCurveTo            Context2D::BezierCurveTo               DontDelete|Function 6
   arcTo                    Context2D::ArcTo                       DontDelete|Function 5
   rect                     Context2D::Rect                        DontDelete|Function 4
   arc                      Context2D::Arc                         DontDelete|Function 6
   fill                     Context2D::Fill                        DontDelete|Function 0
   isPointInPath            Context2D::IsPointInPath               DontDelete|Function 2
   stroke                   Context2D::Stroke                      DontDelete|Function 0
   clip                     Context2D::Clip                        DontDelete|Function 0
   #
   # images. Lots of overloading here!
   drawImage                Context2D::DrawImage                   DontDelete|Function 3
   #
   # pixel ops.
   getImageData             Context2D::GetImageData                DontDelete|Function 4
   putImageData             Context2D::PutImageData                DontDelete|Function 3
   createImageData          Context2D::CreateImageData             DontDelete|Function 2
   @end
*/

IMPLEMENT_PSEUDO_CONSTRUCTOR(Context2DPseudoCtor, "CanvasRenderingContext2D", Context2DProto)

Context2D::Context2D(ExecState* exec, DOM::CanvasContext2DImpl *ctx):
    WrapperBase(Context2DProto::self(exec), ctx)
{}

// Checks count and sets an exception if needed
static bool enoughArguments(ExecState* exec, const List& args, int limit)
{
    if (args.size() < limit) {
        setDOMException(exec, DOMException::NOT_SUPPORTED_ERR);
        return false;
    }
    return true;
}

// Verifies that a float value is not NaN or a plus/minus infinity (unless infOK)
static bool valFloatOK(ExecState* exec, const JSValue* v, bool infOK)
{
    float val = v->toFloat(exec);
    if (KJS::isNaN(val) || (!infOK && KJS::isInf(val))) {
        setDOMException(exec, DOMException::NOT_SUPPORTED_ERR);
        return false;
    }
    return true;
}

// Verifies that float arguments are not NaN or a plus/minus infinity (unless infOK)
static bool argFloatsOK(ExecState* exec, const List& args, int minArg, int maxArg, bool infOK)
{
    for (int c = minArg; c <= maxArg; ++c) {
        if (!valFloatOK(exec, args[c], infOK))
            return false;
    }
    return true;
}

// Checks if the JSValue is Inf or NaN
static bool argFloatIsInforNaN(ExecState* exec, const JSValue* v)
{
    float val = v->toFloat(exec);
    if (KJS::isNaN(val) || KJS::isInf(val))
        return true;
    return false;
}


// Checks if one the arguments if Inf or NaN
static bool argumentsContainInforNaN(ExecState* exec, const List& args, int minArg, int maxArg)
{
    for (int c = minArg; c <= maxArg; ++c) {
        if (argFloatIsInforNaN(exec, args[c]))
            return true;
    }
    return false;
}

// HTML5-style checking
#define KJS_REQUIRE_ARGS(n) do { if (!enoughArguments(exec, args,n)) return jsUndefined(); } while(0);
#define KJS_CHECK_FLOAT_ARGS(min,max) do { if (!argFloatsOK(exec, args, min, max, false )) return jsUndefined(); } while(0);
#define KJS_CHECK_FLOAT_OR_INF_ARGS(min,max) do { if (!argFloatsOK(exec, args, min, max, true)) return jsUndefined(); } while(0);
#define KJS_CHECK_FLOAT_VAL(v) if (!valFloatOK(exec, v, false)) return;

// Unlike the above checks, ignore the invalid(Inf/NaN) values,
// without throwing an exception 
#define KJS_CHECK_FLOAT_IGNORE_INVALID(v) do { if (argFloatIsInforNaN(exec, v)) return; } while(0)
#define KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(min,max) do { if (argumentsContainInforNaN(exec, args, min, max)) return jsUndefined(); } while(0)


JSValue *KJS::Context2DFunction::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    KJS_CHECK_THIS(Context2D, thisObj);

#ifdef KJS_VERBOSE
    qDebug() << "KJS::Context2DFunction::callAsFunction " << functionName().qstring();
#endif

    Context2D *jsContextObject = static_cast<KJS::Context2D *>(thisObj);
    CanvasContext2DImpl* ctx   = jsContextObject->impl();
    DOMExceptionTranslator exception(exec);

    switch (id) {
    // State ops
    /////////////
    case Context2D::Save: {
        ctx->save();
        break;
    }

    case Context2D::Restore: {
        ctx->restore();
        break;
    }

    // Transform ops. These have NaN inf handled specially in the impl
    case Context2D::Scale: {
        KJS_REQUIRE_ARGS(2);
        KJS_CHECK_FLOAT_OR_INF_ARGS(0, 1);

        ctx->scale(args[0]->toFloat(exec), args[1]->toFloat(exec));
        break;
    }

    case Context2D::Rotate: {
        KJS_REQUIRE_ARGS(1);
        // Rotate actually rejects NaN/infinity as well
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 0);


        ctx->rotate(args[0]->toFloat(exec));
        break;
    }

    case Context2D::Translate: {
        KJS_REQUIRE_ARGS(2);
        KJS_CHECK_FLOAT_OR_INF_ARGS(0, 1);

        ctx->translate(args[0]->toFloat(exec), args[1]->toFloat(exec));
        break;
    }

    case Context2D::Transform: {
        KJS_REQUIRE_ARGS(6);
        KJS_CHECK_FLOAT_OR_INF_ARGS(0, 5);

        ctx->transform(args[0]->toFloat(exec), args[1]->toFloat(exec),
                       args[2]->toFloat(exec), args[3]->toFloat(exec),
                       args[4]->toFloat(exec), args[5]->toFloat(exec));

        break;
    }

    case Context2D::SetTransform: {
        KJS_REQUIRE_ARGS(6);
        KJS_CHECK_FLOAT_OR_INF_ARGS(0, 5);

        ctx->setTransform(args[0]->toFloat(exec), args[1]->toFloat(exec),
                          args[2]->toFloat(exec), args[3]->toFloat(exec),
                          args[4]->toFloat(exec), args[5]->toFloat(exec));
        break;
    }

    // Composition state is properties --- not in prototype

    // Color and style info..
    case Context2D::CreateLinearGradient: {
        KJS_REQUIRE_ARGS(4);
        KJS_CHECK_FLOAT_ARGS(0, 3);

        CanvasGradientImpl* grad = ctx->createLinearGradient(
              args[0]->toFloat(exec), args[1]->toFloat(exec),
              args[2]->toFloat(exec), args[3]->toFloat(exec));
        return getWrapper<CanvasGradient>(exec, grad);
    }

    case Context2D::CreateRadialGradient: {
        KJS_REQUIRE_ARGS(6);
        KJS_CHECK_FLOAT_ARGS(0, 5);

        CanvasGradientImpl* grad = ctx->createRadialGradient(
              args[0]->toFloat(exec), args[1]->toFloat(exec),
              args[2]->toFloat(exec), args[3]->toFloat(exec),
              args[4]->toFloat(exec), args[5]->toFloat(exec),
              exception);

        return getWrapper<CanvasGradient>(exec, grad);
    }

    case Context2D::CreatePattern: {
        KJS_REQUIRE_ARGS(2);

        ElementImpl* el = toElement(args[0]);
        if (!el) {
          setDOMException(exec, DOMException::TYPE_MISMATCH_ERR);
          return jsUndefined();
        }

        CanvasPatternImpl* pat = ctx->createPattern(el, valueToStringWithNullCheck(exec, args[1]),
                                                    exception);

        return getWrapper<CanvasPattern>(exec, pat);
    }

    // Line properties are all... properties!

    // Rectangle ops
    case Context2D::ClearRect: {
        KJS_REQUIRE_ARGS(4);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 3);

        ctx->clearRect(args[0]->toFloat(exec), args[1]->toFloat(exec),
                       args[2]->toFloat(exec), args[3]->toFloat(exec),
                       exception);

        break;
    }

    case Context2D::FillRect: {
        KJS_REQUIRE_ARGS(4);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 3);

        ctx->fillRect(args[0]->toFloat(exec), args[1]->toFloat(exec),
                      args[2]->toFloat(exec), args[3]->toFloat(exec),
                      exception);

        break;
    }

    case Context2D::StrokeRect: {
        KJS_REQUIRE_ARGS(4);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 3);

        ctx->strokeRect(args[0]->toFloat(exec), args[1]->toFloat(exec),
                        args[2]->toFloat(exec), args[3]->toFloat(exec),
                        exception);

        break;
    }

    // Path ops
    case Context2D::BeginPath: {
        ctx->beginPath();
        break;
    }

    case Context2D::ClosePath: {
        ctx->closePath();
        break;
    }

    case Context2D::MoveTo: {
        KJS_REQUIRE_ARGS(2);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 1);

        ctx->moveTo(args[0]->toFloat(exec), args[1]->toFloat(exec));
        break;
    }

    case Context2D::LineTo: {
        KJS_REQUIRE_ARGS(2);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 1);

        ctx->lineTo(args[0]->toFloat(exec), args[1]->toFloat(exec));
        break;
    }

    case Context2D::QuadraticCurveTo: {
        KJS_REQUIRE_ARGS(4);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 3);

        ctx->quadraticCurveTo(args[0]->toFloat(exec), args[1]->toFloat(exec),
                              args[2]->toFloat(exec), args[3]->toFloat(exec));
        break;
    }

    case Context2D::BezierCurveTo: {
        KJS_REQUIRE_ARGS(6);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 5);

        ctx->bezierCurveTo(args[0]->toFloat(exec), args[1]->toFloat(exec),
                           args[2]->toFloat(exec), args[3]->toFloat(exec),
                           args[4]->toFloat(exec), args[5]->toFloat(exec));
        break;
    }

    case Context2D::ArcTo: {
        KJS_REQUIRE_ARGS(5);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 4);

        ctx->arcTo(args[0]->toFloat(exec), args[1]->toFloat(exec),
                   args[2]->toFloat(exec), args[3]->toFloat(exec),
                   args[4]->toFloat(exec), exception);
        break;
    }

    case Context2D::Rect: {
        KJS_REQUIRE_ARGS(4);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 3);

        ctx->rect(args[0]->toFloat(exec), args[1]->toFloat(exec),
                  args[2]->toFloat(exec), args[3]->toFloat(exec),
                  exception);
        break;
    }

    case Context2D::Arc: {
        KJS_REQUIRE_ARGS(6);
        KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(0, 5);

        ctx->arc(args[0]->toFloat(exec), args[1]->toFloat(exec),
                 args[2]->toFloat(exec), args[3]->toFloat(exec),
                 args[4]->toFloat(exec), args[5]->toBoolean(exec),
                 exception);
        break;
    }

    case Context2D::Fill: {
        ctx->fill();
        break;
    }

    case Context2D::Stroke: {
        ctx->stroke();
        break;
    }

    case Context2D::Clip: {
        ctx->clip();
        break;
    }

    case Context2D::IsPointInPath: {
        KJS_REQUIRE_ARGS(2);
        if (argumentsContainInforNaN(exec, args, 0, 1)) {
            return jsBoolean(false);
        }
        return jsBoolean(ctx->isPointInPath(args[0]->toFloat(exec),
                                            args[1]->toFloat(exec)));
    }

    case Context2D::DrawImage: {
        ElementImpl* el = toElement(args[0]);
        if (!el) {
            setDOMException(exec, DOMException::TYPE_MISMATCH_ERR);
            break;
        }

        if (args.size() < 3) {
            setDOMException(exec, DOMException::NOT_SUPPORTED_ERR);
            break;
        }

        if (args.size() < 5) { // 3 or 4 arguments
            KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(1, 2);
            ctx->drawImage(el,
                           args[1]->toFloat(exec),
                           args[2]->toFloat(exec),
                           exception);
        } else if (args.size() < 9) { // 5 through 9 arguments
            KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(1, 4);
            ctx->drawImage(el,
                           args[1]->toFloat(exec),
                           args[2]->toFloat(exec),
                           args[3]->toFloat(exec),
                           args[4]->toFloat(exec),
                           exception);
        } else  { // 9 or more arguments
            KJS_CHECK_FLOAT_ARGUMENTS_IGNORE_INVALID(1, 8);
            ctx->drawImage(el,
                           args[1]->toFloat(exec),
                           args[2]->toFloat(exec),
                           args[3]->toFloat(exec),
                           args[4]->toFloat(exec),
                           args[5]->toFloat(exec),
                           args[6]->toFloat(exec),
                           args[7]->toFloat(exec),
                           args[8]->toFloat(exec),
                           exception);
        }
        break;
    }
    case Context2D::GetImageData: {
        KJS_REQUIRE_ARGS(4);
        KJS_CHECK_FLOAT_ARGS(0, 3);
        CanvasImageDataImpl* id = ctx->getImageData(args[0]->toFloat(exec), args[1]->toFloat(exec),
                                                    args[2]->toFloat(exec), args[3]->toFloat(exec),
                                                    exception);
        return getWrapper<CanvasImageData>(exec, id);
        break;
    }
    case Context2D::PutImageData: {
        KJS_REQUIRE_ARGS(3);
        KJS_CHECK_FLOAT_ARGS(1, 2);
        SharedPtr<CanvasImageDataImpl> id = toCanvasImageData(exec, args[0]); // may need to delete..
        ctx->putImageData(id.get(), args[1]->toFloat(exec), args[2]->toFloat(exec), exception);
        break;
    }
    case Context2D::CreateImageData: {
        KJS_REQUIRE_ARGS(2);
        KJS_CHECK_FLOAT_ARGS(0, 1);
        CanvasImageDataImpl* id = ctx->createImageData(args[0]->toFloat(exec),
                                                       args[1]->toFloat(exec),
                                                       exception);
        return getWrapper<CanvasImageData>(exec, id);
    }

    }

    return jsUndefined();
}

const ClassInfo Context2D::info = { "CanvasRenderingContext2D", 0, &Context2DTable, 0 };

/*
   @begin Context2DTable 11
   canvas                   Context2D::Canvas                      DontDelete|ReadOnly
   #
   # compositing
   globalAlpha              Context2D::GlobalAlpha                 DontDelete
   globalCompositeOperation Context2D::GlobalCompositeOperation    DontDelete
   #
   # colors and styles
   strokeStyle              Context2D::StrokeStyle                 DontDelete
   fillStyle                Context2D::FillStyle                   DontDelete
   #
   # line drawing properties
   lineWidth                Context2D::LineWidth                   DontDelete
   lineCap                  Context2D::LineCap                     DontDelete
   lineJoin                 Context2D::LineJoin                    DontDelete
   miterLimit               Context2D::MiterLimit                  DontDelete
   # shadow properties
   shadowOffsetX            Context2D::ShadowOffsetX               DontDelete
   shadowOffsetY            Context2D::ShadowOffsetY               DontDelete
   shadowBlur               Context2D::ShadowBlur                  DontDelete
   shadowColor              Context2D::ShadowColor                 DontDelete
   @end
*/

bool Context2D::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<Context2D, DOMObject>(exec, &Context2DTable, this, propertyName, slot);
}

static JSValue* encodeStyle(ExecState* exec, CanvasStyleBaseImpl* style)
{
    switch (style->type()) {
    case CanvasStyleBaseImpl::Color:
        return jsString(UString(static_cast<CanvasColorImpl*>(style)->toString()));
    case CanvasStyleBaseImpl::Gradient:
        return getWrapper<CanvasGradient>(exec, static_cast<CanvasGradientImpl*>(style));
    case CanvasStyleBaseImpl::Pattern:
        return getWrapper<CanvasPattern>(exec, static_cast<CanvasPatternImpl*>(style));
    }

    return jsNull();
}


// ### TODO: test how non-string things are handled in other browsers.
static CanvasStyleBaseImpl* decodeStyle(ExecState* exec, JSValue* v)
{
    if (v->isObject() && static_cast<JSObject*>(v)->inherits(&CanvasGradient::info))
        return static_cast<CanvasGradient*>(v)->impl();
    else if (v->isObject() && static_cast<JSObject*>(v)->inherits(&CanvasPattern::info))
        return static_cast<CanvasPattern*>(v)->impl();
    else
        return CanvasColorImpl::fromString(v->toString(exec).domString());
}

JSValue* Context2D::getValueProperty(ExecState* exec, int token) const
{
    const CanvasContext2DImpl* ctx = impl();
    switch(token) {
    case Canvas:
        return getDOMNode(exec, ctx->canvas());

    case GlobalAlpha:
        return jsNumber(ctx->globalAlpha());

    case GlobalCompositeOperation:
        return jsString(ctx->globalCompositeOperation());

    case StrokeStyle:
        return encodeStyle(exec, ctx->strokeStyle());

    case FillStyle:
        return encodeStyle(exec, ctx->fillStyle());

    case LineWidth:
        return jsNumber(ctx->lineWidth());

    case LineCap:
        return jsString(ctx->lineCap());

    case LineJoin:
        return jsString(ctx->lineJoin());

    case MiterLimit:
        return jsNumber(ctx->miterLimit());

    case ShadowOffsetX:
        return jsNumber(ctx->shadowOffsetX());

    case ShadowOffsetY:
        return jsNumber(ctx->shadowOffsetY());

    case ShadowBlur:
        return jsNumber(ctx->shadowBlur());

    case ShadowColor:
        return jsString(ctx->shadowColor());

    default:
        assert(0);
        return jsUndefined();
    }
}

void Context2D::put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr)
{
    lookupPut<Context2D,DOMObject>(exec, propertyName, value, attr, &Context2DTable, this );
}

void Context2D::putValueProperty(ExecState *exec, int token, JSValue *value, int /*attr*/)
{
    CanvasContext2DImpl* ctx = impl();
    switch(token) {
    case GlobalAlpha:
        KJS_CHECK_FLOAT_IGNORE_INVALID(value);
        ctx->setGlobalAlpha(value->toFloat(exec));
        break;
    case GlobalCompositeOperation:
        ctx->setGlobalCompositeOperation(value->toString(exec).domString());
        break;
    case StrokeStyle:
        ctx->setStrokeStyle(decodeStyle(exec, value));
        break;
    case FillStyle:
        ctx->setFillStyle(decodeStyle(exec, value));
        break;
    case LineWidth:
        KJS_CHECK_FLOAT_IGNORE_INVALID(value);
        ctx->setLineWidth(value->toFloat(exec));
        break;
    case LineCap:
        ctx->setLineCap(value->toString(exec).domString());
        break;
    case LineJoin:
        ctx->setLineJoin(value->toString(exec).domString());
        break;
    case MiterLimit:
        KJS_CHECK_FLOAT_IGNORE_INVALID(value);
        ctx->setMiterLimit(value->toFloat(exec));
        break;
    case ShadowOffsetX:
        KJS_CHECK_FLOAT_IGNORE_INVALID(value);
        ctx->setShadowOffsetX(value->toFloat(exec));
        break;
    case ShadowOffsetY:
        KJS_CHECK_FLOAT_IGNORE_INVALID(value);
        ctx->setShadowOffsetY(value->toFloat(exec));
        break;
    case ShadowBlur:
        KJS_CHECK_FLOAT_IGNORE_INVALID(value);
        ctx->setShadowBlur(value->toFloat(exec));
        break;
    case ShadowColor:
        ctx->setShadowColor(value->toString(exec).domString());
        break;
    default:
        {} // huh?
    }
}



////////////////////// CanvasGradient Object ////////////////////////
const ClassInfo KJS::CanvasGradient::info = { "CanvasGradient", 0, 0, 0 };

KJS_DEFINE_PROTOTYPE(CanvasGradientProto)
KJS_IMPLEMENT_PROTOFUNC(CanvasGradientFunction)
KJS_IMPLEMENT_PROTOTYPE("CanvasGradientProto", CanvasGradientProto, CanvasGradientFunction, ObjectPrototype)

/*
   @begin CanvasGradientProtoTable 1
   addColorStop             CanvasGradient::AddColorStop                DontDelete|Function 2
   @end
*/

JSValue *CanvasGradientFunction::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    KJS_CHECK_THIS(CanvasGradient, thisObj);

    CanvasGradientImpl* impl = static_cast<KJS::CanvasGradient*>(thisObj)->impl();

    DOMExceptionTranslator exception(exec);
    switch (id) {
    case CanvasGradient::AddColorStop:
        KJS_REQUIRE_ARGS(2);
        impl->addColorStop(args[0]->toFloat(exec), args[1]->toString(exec).domString(), exception);
        break;
    default:
        assert(0);
    }

    return jsUndefined();
}

CanvasGradient::CanvasGradient(ExecState* exec, DOM::CanvasGradientImpl* impl) :
    WrapperBase(CanvasGradientProto::self(exec), impl)
{}

////////////////////// CanvasPattern Object ////////////////////////

const ClassInfo CanvasPattern::info = { "CanvasPattern", 0, 0, 0 };

// Provide an empty prototype in case people want to hack it
KJS_DEFINE_PROTOTYPE(CanvasPatternProto)
KJS_IMPLEMENT_PROTOFUNC(CanvasPatternFunction)
KJS_IMPLEMENT_PROTOTYPE("CanvasPatternProto", CanvasPatternProto, CanvasPatternFunction, ObjectPrototype)

/*
   @begin CanvasPatternProtoTable 0
   @end
*/

JSValue *CanvasPatternFunction::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    Q_UNUSED(exec);
    Q_UNUSED(thisObj);
    Q_UNUSED(args);
    assert(0);
    return NULL;
}

CanvasPattern::CanvasPattern(ExecState* exec, DOM::CanvasPatternImpl* impl) :
    WrapperBase(CanvasPatternProto::self(exec), impl)
{}

////////////////////// CanvasImageData[Array] Object /////////////////

const ClassInfo CanvasImageData::info = { "ImageData", 0, 0, 0 };

CanvasImageData::CanvasImageData(ExecState* exec, DOM::CanvasImageDataImpl* impl) :
    WrapperBase(exec->lexicalInterpreter()->builtinObjectPrototype(), impl)
{
    data = new CanvasImageDataArray(exec, this);
    // Set out properties from the image info..
    putDirect("width",  jsNumber(impl->width()),  DontDelete | ReadOnly);
    putDirect("height", jsNumber(impl->height()), DontDelete | ReadOnly);
    putDirect("data",   data, DontDelete | ReadOnly);
}

void CanvasImageData::mark()
{
    JSObject::mark();
    if (!data->marked())
        data->mark();
}

JSObject* CanvasImageData::valueClone(Interpreter* targetCtx) const
{
    return static_cast<JSObject*>(getWrapper<CanvasImageData>(targetCtx->globalExec(), impl()->clone()));
}

const ClassInfo CanvasImageDataArray::info = { "ImageDataArray", 0, 0, 0 };

CanvasImageDataArray::CanvasImageDataArray(ExecState* exec, CanvasImageData* p) :
        JSObject(exec->lexicalInterpreter()->builtinArrayPrototype()), parent(p)
{
    size = p->impl()->width() * p->impl()->height() * 4;
    putDirect(exec->propertyNames().length, jsNumber(size), DontDelete | ReadOnly);
}

void CanvasImageDataArray::mark()
{
    JSObject::mark();
    if (!parent->marked())
        parent->mark();
}

JSValue* CanvasImageDataArray::indexGetter(ExecState* exec, unsigned index)
{
    Q_UNUSED(exec);
    if (index >= size) // paranoia..
        return jsNull();

    unsigned pixel = index / 4;
    unsigned comp  = index % 4;
    QColor color = parent->impl()->pixel(pixel);
    //"... with each pixel's red, green, blue, and alpha components being given in that order"
    switch (comp) {
    case 0: return jsNumber(color.red());
    case 1: return jsNumber(color.green());
    case 2: return jsNumber(color.blue());
    default:     // aka case 3, for quietness purposes
            return jsNumber(color.alpha());
    }
}

bool CanvasImageDataArray::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // ### this doesn't behave exactly like array does --- should we care?
    bool ok;
    unsigned index = propertyName.toArrayIndex(&ok);
    if (ok && index < size) {
        slot.setCustomIndex(this, index, indexGetterAdapter<CanvasImageDataArray>);
        return true;
    }

    return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

bool CanvasImageDataArray::getOwnPropertySlot(ExecState* exec, unsigned index, PropertySlot& slot)
{
    if (index < size) {
        slot.setCustomIndex(this, index, indexGetterAdapter<CanvasImageDataArray>);
        return true;
    }

    return JSObject::getOwnPropertySlot(exec, index, slot);
}

void CanvasImageDataArray::put(ExecState* exec, const Identifier& propertyName, JSValue* value, int attr)
{
  bool ok;
  unsigned index = propertyName.toArrayIndex(&ok);
  if (ok) {
    put(exec, index, value, attr);
    return;
  }

  JSObject::put(exec, propertyName, value, attr);
}

unsigned char CanvasImageDataArray::decodeComponent(ExecState* exec, JSValue* jsVal)
{
    double val = jsVal->toNumber(exec);

    if (jsVal->isUndefined())
        val = 0.0;
    else if (val < 0.0)
        val = 0.0;
    else if (val > 255.0)
        val = 255.0;

    // ### fixme: round to even
    return (unsigned char)qRound(val);
}

void CanvasImageDataArray::put(ExecState* exec, unsigned index, JSValue* value, int attr)
{
    if (index < size) {
        unsigned char componentValue = decodeComponent(exec, value);
        unsigned int  pixel = index / 4;
        unsigned int  comp  = index % 4;
        parent->impl()->setComponent(pixel, comp, componentValue);
        return;
    }

    // Must use the string version here since numberic one will fall back to
    // us again.
    JSObject::put(exec, Identifier::from(index), value, attr);
}

DOM::CanvasImageDataImpl* toCanvasImageData(ExecState* exec, JSValue* val)
{
    JSObject* obj = val->getObject();
    if (!obj) return 0;

    if (obj->inherits(&CanvasImageData::info))
        return static_cast<CanvasImageData*>(val)->impl();

    // Uff. May be a fake one.
    bool ok = true;
    uint32_t width  = obj->get(exec, "width")->toUInt32(exec, ok);
    if (!ok || !width || exec->hadException())
        return 0;
    uint32_t height = obj->get(exec, "height")->toUInt32(exec, ok);
    if (!ok || !height || exec->hadException())
        return 0;

    // Perform safety check on the size.
    if (!khtmlImLoad::ImageManager::isAcceptableSize(width, height))
        return 0;

    JSObject* data = obj->get(exec, "data")->getObject();
    if (!data)
        return 0;

    uint32_t length = data->get(exec, "length")->toUInt32(exec, ok);
    if (!ok || !length || exec->hadException())
        return 0;

    if (length != 4 * width * height)
        return 0;

    // Uff. Well, it sounds sane enough for us to decode..
    CanvasImageDataImpl* id = new CanvasImageDataImpl(width, height);
    for (unsigned pixel = 0; pixel < width * height; ++pixel) {
        unsigned char r = CanvasImageDataArray::decodeComponent(exec, data->get(exec, pixel*4));
        unsigned char g = CanvasImageDataArray::decodeComponent(exec, data->get(exec, pixel*4 + 1));
        unsigned char b = CanvasImageDataArray::decodeComponent(exec, data->get(exec, pixel*4 + 2));
        unsigned char a = CanvasImageDataArray::decodeComponent(exec, data->get(exec, pixel*4 + 3));
        id->setPixel(pixel, QColor(r, g, b, a));
    }
    return id;
}

// This is completely fake!
IMPLEMENT_PSEUDO_CONSTRUCTOR(SVGAnglePseudoCtor, "SVGAngle", Context2DProto)


} // namespace

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
