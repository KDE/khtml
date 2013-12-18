/*
 * CSS Media Query
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
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

#ifndef css_mediaquery_h
#define css_mediaquery_h

#include "dom/dom_string.h"
#include "css/css_valueimpl.h"

class KHTMLPart;

namespace DOM {
class MediaListImpl;
class ValueList;
}

namespace khtml {

class MediaQueryExp;

class MediaQuery
{
public:
    enum Restrictor {
        Only, Not, None
    };

    MediaQuery(Restrictor r, const DOM::DOMString& mediaType, QList<MediaQueryExp*>* exprs);
    ~MediaQuery();

    Restrictor restrictor() const { return m_restrictor; }
    const QList<MediaQueryExp*>* expressions() const { return m_expressions; }
    DOM::DOMString mediaType() const { return m_mediaType; }
    bool operator==(const MediaQuery& other) const;
    void append(MediaQueryExp* newExp) { m_expressions->append(newExp); }
    DOM::DOMString cssText() const;

 private:
    Restrictor m_restrictor;
    DOM::DOMString m_mediaType;
    QList<MediaQueryExp*>* m_expressions;
};

class CSSStyleSelector;
class RenderStyle;
class MediaQueryExp;


/**
 * Class that evaluates css media queries as defined in
 * CSS3 Module "Media Queries" (http://www.w3.org/TR/css3-mediaqueries/)
 * Special constructors are needed, if simple media queries are to be
 * evaluated without knowledge of the medium features. This can happen
 * for example when parsing UA stylesheets, if evaluation is done
 * right after parsing.
 *
 * the boolean parameter is used to approximate results of evaluation, if
 * the device characteristics are not known. This can be used to prune the loading
 * of stylesheets to only those which are probable to match.
 */
class MediaQueryEvaluator
{
public:
    /** Creates evaluator which evaluates only simple media queries
     *  Evaluator returns true for "all", and returns value of \mediaFeatureResult
     *  for any media features
     */
    MediaQueryEvaluator(bool mediaFeatureResult = false);

    /** Creates evaluator which evaluates only simple media queries
     *  Evaluator  returns true for acceptedMediaType and returns value of \mediafeatureResult
     *  for any media features
     */
    MediaQueryEvaluator(const DOM::DOMString& acceptedMediaType, bool mediaFeatureResult = false);
    MediaQueryEvaluator(const char* acceptedMediaType, bool mediaFeatureResult = false);

    /** Creates evaluator which evaluates full media queries
     */
    MediaQueryEvaluator(const DOM::DOMString& acceptedMediaType, KHTMLPart*, RenderStyle*);

    ~MediaQueryEvaluator();

    bool mediaTypeMatch(const DOM::DOMString& mediaTypeToMatch) const;
    bool mediaTypeMatchSpecific(const char* mediaTypeToMatch) const;

    /** Evaluates a list of media queries */
    bool eval(const DOM::MediaListImpl*, CSSStyleSelector* = 0) const;

    /** Evaluates media query subexpression, ie "and (media-feature: value)" part */
    bool eval(const MediaQueryExp*) const;
    
    static void cleanup();

private:
    DOM::DOMString m_mediaType;
    KHTMLPart* m_part; // not owned
    RenderStyle* m_style; // not owned
    bool m_expResult;
};

class MediaQueryExp
{
public:
    MediaQueryExp(const DOM::DOMString& mediaFeature, DOM::ValueList* values);
    ~MediaQueryExp();

    DOM::DOMString mediaFeature() const { return m_mediaFeature; }

    DOM::CSSValueImpl* value() const { return m_value; }

    bool operator==(const MediaQueryExp& other) const  {
        return (other.m_mediaFeature == m_mediaFeature)
            && ((!other.m_value && !m_value)
                || (other.m_value && m_value && other.m_value->cssText() == m_value->cssText()));
    }

    bool isViewportDependent() const { return m_viewportDependent; }

private:
    bool m_viewportDependent;
    DOM::DOMString m_mediaFeature;
    DOM::CSSValueImpl* m_value;
};


} // namespace

#endif
