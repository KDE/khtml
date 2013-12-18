/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2009 Germain Garand <germain@ebooksfrance.org>
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

#ifndef css_webfont_h
#define css_webfont_h

#include "dom/dom_string.h"
#include "misc/shared.h"
#include "misc/loader.h"
#include "css/css_valueimpl.h"

#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

#include <QHash>

namespace DOM {

class CSSFontFace;
class CSSFontFaceRuleImpl;
class CSSFontSelector;
class CSSSegmentedFontFace;
class DocumentImpl;
class DocLoader;
class FontDef;

    enum {
        FontStyleNormalBit = 0,
        FontStyleItalicBit,
        FontVariantNormalBit,
        FontVariantSmallCapsBit,
        FontWeight100Bit,
        FontWeight200Bit,
        FontWeight300Bit,
        FontWeight400Bit,
        FontWeight500Bit,
        FontWeight600Bit,
        FontWeight700Bit,
        FontWeight800Bit,
        FontWeight900Bit,
        FontTraitsMaskWidth
    };

    enum FontTraitsMask {
        FontStyleNormalMask = 1 << FontStyleNormalBit,
        FontStyleItalicMask = 1 << FontStyleItalicBit,
        FontStyleMask = FontStyleNormalMask | FontStyleItalicMask,

        FontVariantNormalMask = 1 << FontVariantNormalBit,
        FontVariantSmallCapsMask = 1 << FontVariantSmallCapsBit,
        FontVariantMask = FontVariantNormalMask | FontVariantSmallCapsMask,

        FontWeight100Mask = 1 << FontWeight100Bit,
        FontWeight200Mask = 1 << FontWeight200Bit,
        FontWeight300Mask = 1 << FontWeight300Bit,
        FontWeight400Mask = 1 << FontWeight400Bit,
        FontWeight500Mask = 1 << FontWeight500Bit,
        FontWeight600Mask = 1 << FontWeight600Bit,
        FontWeight700Mask = 1 << FontWeight700Bit,
        FontWeight800Mask = 1 << FontWeight800Bit,
        FontWeight900Mask = 1 << FontWeight900Bit,
        FontWeightMask = FontWeight100Mask | FontWeight200Mask | FontWeight300Mask | FontWeight400Mask | FontWeight500Mask | FontWeight600Mask | FontWeight700Mask | FontWeight800Mask | FontWeight900Mask
    };

class CSSSegmentedFontFace :  public khtml::Shared<CSSSegmentedFontFace>{
public:
    CSSSegmentedFontFace(CSSFontSelector*);
    ~CSSSegmentedFontFace();

    bool isLoaded() const;
    bool isValid() const;
    CSSFontSelector* fontSelector() const { return m_fontSelector; }

    void fontLoaded(CSSFontFace*);

    void appendFontFace(CSSFontFace);

//    FontData* getFontData(const FontDescription&);

private:

//    void pruneTable();

    CSSFontSelector* m_fontSelector;
//    HashMap<unsigned, SegmentedFontData*> m_fontDataTable;
//    WTF::Vector<WTF::RefPtr<CSSFontFace>, 1> m_fontFaces;
};

class CSSFontFaceSource : public khtml::CachedObjectClient {
public:
    CSSFontFaceSource(const DOMString&, bool distant = false);
    virtual ~CSSFontFaceSource();

    bool isLoaded() const;
    bool isValid() const;

    DOMString string() const { return m_string; }

    void setFontFace(CSSFontFace* face) { m_face = face; }

    virtual void notifyFinished(khtml::CachedObject * /*finishedObj*/);
    void refLoader();

//    SimpleFontData* getFontData(const FontDef&, bool syntheticBold, bool syntheticItalic, CSSFontSelector*);
//    void pruneTable();

#if 0
    // ENABLE(SVG_FONTS)
    SVGFontFaceElement* svgFontFaceElement() const { return m_svgFontFaceElement; }
    void setSVGFontFaceElement(SVGFontFaceElement* element) { m_svgFontFaceElement = element; }
#endif

private:
    DOMString m_string; // URI for remote, built-in font name for local.
    khtml::CachedFont* m_font; // For remote fonts, a pointer to our cached resource.
    CSSFontFace* m_face; // Our owning font face.
    int m_id; // Qt identifier for the Application font.
    bool m_refed;
    bool m_distant;
//    HashMap<unsigned, SimpleFontData*> m_fontDataTable; // The hash key is composed of size synthetic styles.

#if 0
    // ENABLE(SVG_FONTS)
    SVGFontFaceElement* m_svgFontFaceElement;
    RefPtr<SVGFontElement> m_externalSVGFontElement;
#endif
};

class CSSFontFace : public khtml::Shared<CSSFontFace> {
public:
    CSSFontFace(FontTraitsMask traitsMask, CSSFontSelector* fs)
        : m_traitsMask(traitsMask), m_fontSelector(fs), m_refed(false)
    {
    }
    ~CSSFontFace();

    FontTraitsMask traitsMask() const { return m_traitsMask; }

/*
    struct UnicodeRange;

    void addRange(UChar32 from, UChar32 to) { m_ranges.append(UnicodeRange(from, to)); }
    const Vector<UnicodeRange>& ranges() const { return m_ranges; }
*/
    void addedToSegmentedFontFace(CSSSegmentedFontFace*);
    void removedFromSegmentedFontFace(CSSSegmentedFontFace*);

    bool isLoaded() const;
    bool isValid() const;

    void addSource(CSSFontFaceSource*);

    void fontLoaded(CSSFontFaceSource*);
    void addFamilyName(const DOMString& name) { m_names.append( name ); }
    WTF::Vector<DOMString> familyNames() const { return m_names; }
    CSSFontSelector* fontSelector() const { return m_fontSelector; }
    void refLoaders(); // start loading all sources

//    SimpleFontData* getFontData(const FontDef&, bool syntheticBold, bool syntheticItalic);

/*
    struct UnicodeRange {
        UnicodeRange(UChar32 from, UChar32 to)
            : m_from(from)
            , m_to(to)
        {
        }

        UChar32 from() const { return m_from; }
        UChar32 to() const { return m_to; }

    private:
        UChar32 m_from;
        UChar32 m_to;
    };
*/

private:
    FontTraitsMask m_traitsMask;
//    Vector<UnicodeRange> m_ranges;
//    HashSet<CSSSegmentedFontFace*> m_segmentedFontFaces;
    WTF::Vector<DOMString> m_names;
    WTF::Vector<CSSFontFaceSource*> m_sources;
    CSSFontSelector* m_fontSelector;
    bool m_refed;
};

class CSSFontSelector : public khtml::Shared<CSSFontSelector> {
public:
    CSSFontSelector(DocumentImpl*);
    virtual ~CSSFontSelector();

//    virtual FontData* getFontData(const FontDef& fontDescription, const DOMString& familyName);
    void requestFamilyName( const DOMString& familyName );
    void clearDocument() { m_document = 0; }
    void addFontFaceRule(const CSSFontFaceRuleImpl*);
    void fontLoaded();
    virtual void fontCacheInvalidated();
    bool isEmpty() const;
    khtml::DocLoader* docLoader() const;

private:

    DocumentImpl* m_document;
//    HashMap<DOMString, Vector<RefPtr<CSSFontFace> >*, CaseFoldingHash> m_fontFaces;
//    WTF::HashMap<DOMString, WTF::Vector<WTF::RefPtr<CSSFontFace> >*, CaseFoldingHash> m_locallyInstalledFontFaces;
      QHash<DOMString, CSSFontFace*> m_locallyInstalledFontFaces;
//    HashMap<DOMString, HashMap<unsigned, RefPtr<CSSSegmentedFontFace> >*, CaseFoldingHash> m_fonts;
};

} // namespace DOM

#endif // css_webfont_h
