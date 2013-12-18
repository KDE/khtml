/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#include "css_webfont.h"
#include "css/css_ruleimpl.h"
#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "xml/dom_docimpl.h"
#include "rendering/font.h"
#include "rendering/render_object.h"
#include "rendering/render_canvas.h"
#include <QDebug>
#include <QFontDatabase>
#include <QFont>

namespace DOM {

CSSFontFaceSource::CSSFontFaceSource(const DOMString& str, bool distant)
    : m_string(str)
    , m_font(0)
    , m_face(0)
    , m_refed(false)
    , m_distant(distant)
#if 0
    //ENABLE(SVG_FONTS)
    , m_svgFontFaceElement(0)
#endif
{
    m_id = -1;
}

CSSFontFaceSource::~CSSFontFaceSource()
{
    if (m_font) {
        if (m_refed)
            m_font->deref( this );
        if (m_id != -1) {
            WTF::Vector<DOMString> names = m_face->familyNames();
            unsigned size = names.size();
            for (unsigned i = 0; i < size; i++) {
                QFont::removeSubstitution( names[i].string() );
                khtml::Font::invalidateCachedFontFamily( names[i].string() );
            }
            QFontDatabase::removeApplicationFont( m_id );
        }
    }
}

bool CSSFontFaceSource::isLoaded() const
{
    if (m_distant)
        return m_font? m_font->isLoaded() : false;
    return true;
}

bool CSSFontFaceSource::isValid() const
{
    if (m_font) {
        return !m_font->hadError();
    }
    return true;
}

void CSSFontFaceSource::notifyFinished(khtml::CachedObject */*finishedObj*/)
{
    WTF::Vector<DOMString> names = m_face->familyNames();
    unsigned size = names.size();

    m_id = QFontDatabase::addApplicationFontFromData( m_font->font() );

    if (m_id == -1) {
        // qDebug() << "WARNING: downloaded web font" << (size?names[0].string():QString()) << "was rejected by the font subsystem.";
        return;
    }
    QString nativeName = QFontDatabase::applicationFontFamilies( m_id )[0];
    for (unsigned i = 0; i < size; i++) {
        if (names[i].string() != nativeName) {
            QFont::insertSubstitution( names[i].string(), nativeName );
        }
        khtml::Font::invalidateCachedFontFamily( names[i].string() );
    }
    if (m_face && m_refed) {
        m_face->fontLoaded(this);
    }
}

void CSSFontFaceSource::refLoader()
{
    if (!m_distant)
        return;
    if (!m_font) {
        assert(m_face);
        m_font = m_face->fontSelector()->docLoader()->requestFont(m_string);
    }
    if (m_font) {
        m_font->ref( this );
        m_refed = true;
    }
}

#if 0
SimpleFontData* CSSFontFaceSource::getFontData(const FontDef& fontDescription, bool syntheticBold, bool syntheticItalic, CSSFontSelector* fontSelector)
{
    // If the font hasn't loaded or an error occurred, then we've got nothing.
    if (!isValid())
        return 0;

#if 0
    // ENABLE(SVG_FONTS)
    if (!m_font && !m_svgFontFaceElement) {
#else
    if (!m_font) {
#endif
        FontPlatformData* data = fontCache()->getCachedFontPlatformData(fontDescription, m_string);
        SimpleFontData* fontData = fontCache()->getCachedFontData(data);

        // We're local. Just return a SimpleFontData from the normal cache.
        return fontData;
    }

    // See if we have a mapping in our FontData cache.
    unsigned hashKey = fontDescription.computedPixelSize() << 2 | (syntheticBold ? 2 : 0) | (syntheticItalic ? 1 : 0);
    if (SimpleFontData* cachedData = m_fontDataTable.get(hashKey))
        return cachedData;

    OwnPtr<SimpleFontData> fontData;

    // If we are still loading, then we let the system pick a font.
    if (isLoaded()) {
        if (m_font) {
#if 0
    //ENABLE(SVG_FONTS)
            if (m_font->isSVGFont()) {
                // For SVG fonts parse the external SVG document, and extract the <font> element.
                if (!m_font->ensureSVGFontData())
                    return 0;

                if (!m_externalSVGFontElement)
                    m_externalSVGFontElement = m_font->getSVGFontById(SVGURIReference::getTarget(m_string));

                if (!m_externalSVGFontElement)
                    return 0;

                SVGFontFaceElement* fontFaceElement = 0;

                // Select first <font-face> child
                for (Node* fontChild = m_externalSVGFontElement->firstChild(); fontChild; fontChild = fontChild->nextSibling()) {
                    if (fontChild->hasTagName(SVGNames::font_faceTag)) {
                        fontFaceElement = static_cast<SVGFontFaceElement*>(fontChild);
                        break;
                    }
                }

                if (fontFaceElement) {
                    if (!m_svgFontFaceElement) {
                        // We're created using a CSS @font-face rule, that means we're not associated with a SVGFontFaceElement.
                        // Use the imported <font-face> tag as referencing font-face element for these cases.
                        m_svgFontFaceElement = fontFaceElement;
                    }

                    SVGFontData* svgFontData = new SVGFontData(fontFaceElement);
                    fontData.set(new SimpleFontData(m_font->platformDataFromCustomData(fontDescription.computedPixelSize(), syntheticBold, syntheticItalic, fontDescription.renderingMode()), true, false, svgFontData));
                }
            } else
#endif
            {
                // Create new FontPlatformData from our CGFontRef, point size and ATSFontRef.
                if (!m_font->ensureCustomFontData())
                    return 0;

                fontData.set(new SimpleFontData(m_font->platformDataFromCustomData(fontDescription.computedPixelSize(), syntheticBold, syntheticItalic, fontDescription.renderingMode()), true, false));
            }
        } else {
#if 0
    //ENABLE(SVG_FONTS)
            // In-Document SVG Fonts
            if (m_svgFontFaceElement) {
                SVGFontData* svgFontData = new SVGFontData(m_svgFontFaceElement);
                fontData.set(new SimpleFontData(FontPlatformData(fontDescription.computedPixelSize(), syntheticBold, syntheticItalic), true, false, svgFontData));
            }
#endif
        }
    } else {
        // Kick off the load now.
        if (DocLoader* docLoader = fontSelector->docLoader())
            m_font->beginLoadIfNeeded(docLoader);
        // FIXME: m_string is a URL so it makes no sense to pass it as a family name.
        FontPlatformData* tempData = fontCache()->getCachedFontPlatformData(fontDescription, m_string);
        if (!tempData)
            tempData = fontCache()->getLastResortFallbackFont(fontDescription);
        fontData.set(new SimpleFontData(*tempData, true, true));
    }

    m_fontDataTable.set(hashKey, fontData.get());
    return fontData.release();
}
#endif

CSSFontFace::~CSSFontFace()
{
    deleteAllValues(m_sources);

}

bool CSSFontFace::isLoaded() const
{
    unsigned size = m_sources.size();
    for (unsigned i = 0; i < size; i++) {
        if (!m_sources[i]->isLoaded())
            return false;
    }
    return true;
}

bool CSSFontFace::isValid() const
{
    unsigned size = m_sources.size();
    if (!size)
        return false;
    for (unsigned i = 0; i < size; i++) {
        if (m_sources[i]->isValid())
            return true;
    }
    return false;
}

void CSSFontFace::refLoaders()
{
    if (m_refed)
        return;
    unsigned size = m_sources.size();
    if (!size)
        return;
    for (unsigned i = 0; i < size; i++) {
         m_sources[i]->refLoader();
    }
    m_refed = true;
}

void CSSFontFace::addedToSegmentedFontFace(CSSSegmentedFontFace* segmentedFontFace)
{
    (void) segmentedFontFace;
//    m_segmentedFontFaces.add(segmentedFontFace);
}

void CSSFontFace::removedFromSegmentedFontFace(CSSSegmentedFontFace* segmentedFontFace)
{
    (void) segmentedFontFace;
//    m_segmentedFontFaces.remove(segmentedFontFace);
}

void CSSFontFace::addSource(CSSFontFaceSource* source)
{
    m_sources.append(source);
    source->setFontFace(this);
}

void CSSFontFace::fontLoaded(CSSFontFaceSource*)
{
/* 
   // FIXME: Can we assert that m_segmentedFontFaces is not empty? That may
    // require stopping in-progress font loading when the last
    // CSSSegmentedFontFace is removed.
    if (m_segmentedFontFaces.isEmpty())
        return;

    HashSet<CSSSegmentedFontFace*>::iterator end = m_segmentedFontFaces.end();
    for (HashSet<CSSSegmentedFontFace*>::iterator it = m_segmentedFontFaces.begin(); it != end; ++it)
        (*it)->fontLoaded(this);

    // Use one of the CSSSegmentedFontFaces' font selector. They all have
    // the same font selector, so it's wasteful to store it in the CSSFontFace.
    CSSFontSelector* fontSelector = (*m_segmentedFontFaces.begin())->fontSelector();
*/
    m_fontSelector->fontLoaded();
}

#if 0
SimpleFontData* CSSFontFace::getFontData(const FontDef& fontDescription, bool syntheticBold, bool syntheticItalic)
{
    if (!isValid())
        return 0;
        
    ASSERT(!m_segmentedFontFaces.isEmpty());
    CSSFontSelector* fontSelector = (*m_segmentedFontFaces.begin())->fontSelector();

    SimpleFontData* result = 0;
    unsigned size = m_sources.size();
    for (unsigned i = 0; i < size && !result; i++)
        result = m_sources[i]->getFontData(fontDescription, syntheticBold, syntheticItalic, fontSelector);
    return result;
}
#endif


CSSFontSelector::CSSFontSelector(DocumentImpl* document)
    : m_document(document)
{
    assert(m_document);
//    fontCache()->addClient(this);
}

CSSFontSelector::~CSSFontSelector()
{
//    fontCache()->removeClient(this);
//    deleteAllValues(m_fontFaces);
//    deleteAllValues(m_locallyInstalledFontFaces);
//    deleteAllValues(m_fonts);
      QHash<DOMString, CSSFontFace*>::const_iterator cur = m_locallyInstalledFontFaces.constBegin();
      QHash<DOMString, CSSFontFace*>::const_iterator end = m_locallyInstalledFontFaces.constEnd();
      for (;cur != end; cur++)
           cur.value()->deref();
}

bool CSSFontSelector::isEmpty() const
{
    return false;
    //return m_fonts.isEmpty();
}

khtml::DocLoader* CSSFontSelector::docLoader() const
{
    return m_document ? m_document->docLoader() : 0;
}

void CSSFontSelector::addFontFaceRule(const CSSFontFaceRuleImpl* fontFaceRule)
{
     // Obtain the font-family property and the src property.  Both must be defined.
    const CSSStyleDeclarationImpl* style = fontFaceRule->style();
    CSSValueImpl* fontFamily = style->getPropertyCSSValue( CSS_PROP_FONT_FAMILY );
    CSSValueImpl* src = style->getPropertyCSSValue( CSS_PROP_SRC );
    CSSValueImpl* unicodeRange = style->getPropertyCSSValue(CSS_PROP_UNICODE_RANGE);

    if (!fontFamily || !src || !fontFamily->isValueList() || !src->isValueList() || (unicodeRange && !unicodeRange->isValueList()))
        return;

    CSSValueListImpl* familyList = static_cast<CSSValueListImpl*>(fontFamily);
    if (!familyList->length())
        return;

    CSSValueListImpl* srcList = static_cast<CSSValueListImpl*>(src);
    if (!srcList->length())
        return;

//    CSSValueListImpl* rangeList = static_cast<CSSValueListImpl*>(unicodeRange);

    unsigned traitsMask = 0;
/*
    if (CSSValueImpl* fontStyle = style->getPropertyCSSValue(CSS_PROP_FONT_STYLE)) {

        if (fontStyle->isPrimitiveValue()) {
            CSSValueListImpl* list = new CSSValueListImpl(CSSValueListImpl::Comma);
            list->append(fontStyle);
            fontStyle = list;
        } else if (!fontStyle->isValueList())
            return;

        CSSValueListImpl* styleList = static_cast<CSSValueListImpl*>(fontStyle);
        unsigned numStyles = styleList->length();
        if (!numStyles)
            return;

        for (unsigned i = 0; i < numStyles; ++i) {
            switch (static_cast<CSSPrimitiveValueImpl*>(styleList[i])->getIdent()) {
                case CSS_ALL:
                    traitsMask |= FontStyleMask;
                    break;
                case CSS_NORMAL:
                    traitsMask |= FontStyleNormalMask;
                    break;
                case CSS_ITALIC:
                case CSS_OBLIQUE:
                    traitsMask |= FontStyleItalicMask;
                    break;
                default:
                    break;
            }
        }
    } else
        traitsMask |= FontStyleMask;

    if (CSSValueImpl* fontWeight = style->getPropertyCSSValue(CSS_PROP_FONT_WEIGHT)) {
        if (fontWeight->isPrimitiveValue()) {
            CSSValueListImpl* list = new CSSValueListImpl(CSSValueListImpl::Comma);
            list->append(fontWeight);
            fontWeight = list;
        } else if (!fontWeight->isValueList())
            return;

        CSSValueListImpl* weightList = static_cast<CSSValueListImpl*>(fontWeight);
        unsigned numWeights = weightList->length();
        if (!numWeights)
            return;

        for (unsigned i = 0; i < numWeights; ++i) {
            switch (static_cast<CSSPrimitiveValueImpl*>(weightList[i])->getIdent()) {
                case CSS_VAL_ALL:
                    traitsMask |= FontWeightMask;
                    break;
                case CSS_VAL_BOLDER:
                case CSS_VAL_BOLD:
                case CSS_VAL_700:
                    traitsMask |= FontWeight700Mask;
                    break;
                case CSS_VAL_NORMAL:
                case CSS_VAL_400:
                    traitsMask |= FontWeight400Mask;
                    break;
                case CSS_VAL_900:
                    traitsMask |= FontWeight900Mask;
                    break;
                case CSS_VAL_800:
                    traitsMask |= FontWeight800Mask;
                    break;
                case CSS_VAL_600:
                    traitsMask |= FontWeight600Mask;
                    break;
                case CSS_VAL_500:
                    traitsMask |= FontWeight500Mask;
                    break;
                case CSS_VAL_300:
                    traitsMask |= FontWeight300Mask;
                    break;
                case CSS_VAL_LIGHTER:
                case CSS_VAL_200:
                    traitsMask |= FontWeight200Mask;
                    break;
                case CSS_VAL_100:
                    traitsMask |= FontWeight100Mask;
                    break;
                default:
                    break;
            }
        }
    } else
        traitsMask |= FontWeightMask;

    if (CSSValueImpl* fontVariant = style->getPropertyCSSValue(CSS_PROP_FONT_VARIANT)) {
        if (fontVariant->isPrimitiveValue()) {
            CSSValueListImpl* list = new CSSValueListImpl(CSSValueListImpl::Comma);
            list->append(fontVariant);
            fontVariant = list;
        } else if (!fontVariant->isValueList())
            return;

        CSSValueListImpl* variantList = static_cast<CSSValueListImpl*>(fontVariant);
        unsigned numVariants = variantList->length();
        if (!numVariants)
            return;

        for (unsigned i = 0; i < numVariants; ++i) {
            switch (static_cast<CSSPrimitiveValueImpl*>(variantList[i])->getIdent()) {
                case CSS_VAL_ALL:
                    traitsMask |= FontVariantMask;
                    break;
                case CSS_VAL_NORMAL:
                    traitsMask |= FontVariantNormalMask;
                    break;
                case CSS_VAL_SMALL_CAPS:
                    traitsMask |= FontVariantSmallCapsMask;
                    break;
                default:
                    break;
            }
        }
    } else
        traitsMask |= FontVariantNormalMask;
*/

    // Each item in the src property's list is a single CSSFontFaceSource. Put them all into a CSSFontFace.
    CSSFontFace* fontFace = 0;

    int srcLength = srcList->length();

#if 0
    // ENABLE(SVG_FONTS)
    bool foundSVGFont = false;
#endif
    for (int i = 0; i < srcLength; i++) {
        // An item in the list either specifies a string (local font name) or a URL (remote font to download).
        CSSFontFaceSrcValueImpl* item = static_cast<CSSFontFaceSrcValueImpl*>(srcList->item(i));
        CSSFontFaceSource* source = 0;

#if 0
    // ENABLE(SVG_FONTS)
        foundSVGFont = item->isSVGFontFaceSrc() || item->svgFontFaceElement();
#endif

        if (!item->isLocal()) {
            if (item->isSupportedFormat() && m_document) {
                source = new CSSFontFaceSource(item->resource(), true /*distant*/);
#if 0
    // ENABLE(SVG_FONTS)
                if (foundSVGFont)
                    cachedFont->setSVGFont(true);
#endif
            }
        } else {
            source = new CSSFontFaceSource(item->resource());
        }

        if (!fontFace)
            fontFace = new CSSFontFace( static_cast<FontTraitsMask>(traitsMask), this );

        if (source) {
#if 0
    // ENABLE(SVG_FONTS)
            source->setSVGFontFaceElement(item->svgFontFaceElement());
#endif
            fontFace->addSource(source);
        }
    }

    assert(fontFace);

    if (fontFace && !fontFace->isValid()) {
        delete fontFace;
        return;
    }

/*
    if (rangeList) {
        unsigned numRanges = rangeList->length();
        for (unsigned i = 0; i < numRanges; i++) {
            CSSUnicodeRangeValueImpl* range = static_cast<CSSUnicodeRangeValueImpl*>(rangeList->item(i));
            fontFace->addRange(range->from(), range->to());
        }
    }
*/

    // Hash under every single family name.
    int familyLength = familyList->length();
    for (int i = 0; i < familyLength; i++) {
        CSSPrimitiveValueImpl* item = static_cast<CSSPrimitiveValueImpl*>(familyList->item(i));
        DOMString familyName;
        if (item->primitiveType() == CSSPrimitiveValue::CSS_STRING) {
            familyName = DOMString(static_cast<FontFamilyValueImpl*>(item)->fontName());
        } else if (item->primitiveType() == CSSPrimitiveValue::CSS_IDENT) {
            // We need to use the raw text for all the generic family types, since @font-face is a way of actually
            // defining what font to use for those types.
            switch (item->getIdent()) {
                case CSS_VAL_SERIF:
                    familyName = "-khtml-serif";
                    break;
                case CSS_VAL_SANS_SERIF:
                    familyName = "-khtml-sans-serif";
                    break;
                case CSS_VAL_CURSIVE:
                    familyName = "-khtml-cursive";
                    break;
                case CSS_VAL_FANTASY:
                    familyName = "-khtml-fantasy";
                    break;
                case CSS_VAL_MONOSPACE:
                    familyName = "-khtml-monospace";
                    break;
                default:
                    break;
            }
        }

        if (familyName.isEmpty())
            continue;

        fontFace->addFamilyName( familyName );
        m_locallyInstalledFontFaces.insertMulti( familyName.lower(), fontFace );
        fontFace->ref();

#if 0
    // ENABLE(SVG_FONTS)
        // SVG allows several <font> elements with the same font-family, differing only
        // in ie. font-variant. Be sure to pick up the right one - in getFontData below.
        if (foundSVGFont && (traitsMask & FontVariantSmallCapsMask))
            familyName += "-webkit-svg-small-caps";
#endif

/*
        Vector<RefPtr<CSSFontFace> >* familyFontFaces = m_fontFaces.get(familyName);
        if (!familyFontFaces) {
            familyFontFaces = new Vector<RefPtr<CSSFontFace> >;
            m_fontFaces.set(familyName, familyFontFaces);

            ASSERT(!m_locallyInstalledFontFaces.contains(familyName));
            Vector<RefPtr<CSSFontFace> >* familyLocallyInstalledFaces;

            Vector<unsigned> locallyInstalledFontsTraitsMasks;
            fontCache()->getTraitsInFamily(familyName, locallyInstalledFontsTraitsMasks);
            unsigned numLocallyInstalledFaces = locallyInstalledFontsTraitsMasks.size();
            if (numLocallyInstalledFaces) {
                familyLocallyInstalledFaces = new Vector<RefPtr<CSSFontFace> >;
                m_locallyInstalledFontFaces.set(familyName, familyLocallyInstalledFaces);

                for (unsigned i = 0; i < numLocallyInstalledFaces; ++i) {
                    RefPtr<CSSFontFace> locallyInstalledFontFace = CSSFontFace::create(static_cast<FontTraitsMask>(locallyInstalledFontsTraitsMasks[i]));
                    locallyInstalledFontFace->addSource(new CSSFontFaceSource(familyName));
                    ASSERT(locallyInstalledFontFace->isValid());
                    familyLocallyInstalledFaces->append(locallyInstalledFontFace);
                }
            }
        }

        familyFontFaces->append(fontFace);
*/
    }
}

void CSSFontSelector::requestFamilyName( const DOMString& familyName )
{
    QHash<DOMString, CSSFontFace*>::const_iterator it = m_locallyInstalledFontFaces.constBegin();
    QHash<DOMString, CSSFontFace*>::const_iterator end = m_locallyInstalledFontFaces.constEnd();
    for ( ; it != end; ++it) {
        if (it.key() == familyName.lower()) {
            it.value()->refLoaders();
        }
     }
}

void CSSFontSelector::fontLoaded()
{
    if (!m_document || !m_document->renderer())
        return;
    static_cast<khtml::RenderCanvas*>(m_document->renderer())->updateInvalidatedFonts();
    khtml::Font::markAllCachedFontsAsValid();
}

void CSSFontSelector::fontCacheInvalidated()
{
    if (!m_document || !m_document->renderer())
        return;
    m_document->recalcStyle(DocumentImpl::Force);
   // m_document->renderer()->setNeedsLayoutAndMinMaxRecalc();
}

/*
static FontData* fontDataForGenericFamily(Document* document, const FontDescription& fontDescription, const AtomicString& familyName)
{
    if (!document || !document->frame())
        return 0;

    const Settings* settings = document->frame()->settings();
    if (!settings)
        return 0;
    
    AtomicString genericFamily;
    if (familyName == "-webkit-serif")
        genericFamily = settings->serifFontFamily();
    else if (familyName == "-webkit-sans-serif")
        genericFamily = settings->sansSerifFontFamily();
    else if (familyName == "-webkit-cursive")
        genericFamily = settings->cursiveFontFamily();
    else if (familyName == "-webkit-fantasy")
        genericFamily = settings->fantasyFontFamily();
    else if (familyName == "-webkit-monospace")
        genericFamily = settings->fixedFontFamily();
    else if (familyName == "-webkit-standard")
        genericFamily = settings->standardFontFamily();

    if (!genericFamily.isEmpty())
        return fontCache()->getCachedFontData(fontCache()->getCachedFontPlatformData(fontDescription, genericFamily));

    return 0;
}
*/
static FontTraitsMask desiredTraitsMaskForComparison;

static inline bool compareFontFaces(CSSFontFace* first, CSSFontFace* second)
{
    FontTraitsMask firstTraitsMask = first->traitsMask();
    FontTraitsMask secondTraitsMask = second->traitsMask();

    bool firstHasDesiredVariant = firstTraitsMask & desiredTraitsMaskForComparison & FontVariantMask;
    bool secondHasDesiredVariant = secondTraitsMask & desiredTraitsMaskForComparison & FontVariantMask;

    if (firstHasDesiredVariant != secondHasDesiredVariant)
        return firstHasDesiredVariant;

    bool firstHasDesiredStyle = firstTraitsMask & desiredTraitsMaskForComparison & FontStyleMask;
    bool secondHasDesiredStyle = secondTraitsMask & desiredTraitsMaskForComparison & FontStyleMask;

    if (firstHasDesiredStyle != secondHasDesiredStyle)
        return firstHasDesiredStyle;

    if (secondTraitsMask & desiredTraitsMaskForComparison & FontWeightMask)
        return false;
    if (firstTraitsMask & desiredTraitsMaskForComparison & FontWeightMask)
        return true;

    // http://www.w3.org/TR/2002/WD-css3-webfonts-20020802/#q46 says: "If there are fewer then 9 weights in the family, the default algorithm
    // for filling the "holes" is as follows. If '500' is unassigned, it will be assigned the same font as '400'. If any of the values '600',
    // '700', '800', or '900' remains unassigned, they are assigned to the same face as the next darker assigned keyword, if any, or the next
    // lighter one otherwise. If any of '300', '200', or '100' remains unassigned, it is assigned to the next lighter assigned keyword, if any,
    // or the next darker otherwise."
    // For '400', we made up our own rule (which then '500' follows).

    static const unsigned fallbackRuleSets = 9;
    static const unsigned rulesPerSet = 8;
    static const FontTraitsMask weightFallbackRuleSets[fallbackRuleSets][rulesPerSet] = {
        { FontWeight200Mask, FontWeight300Mask, FontWeight400Mask, FontWeight500Mask, FontWeight600Mask, FontWeight700Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight100Mask, FontWeight300Mask, FontWeight400Mask, FontWeight500Mask, FontWeight600Mask, FontWeight700Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight200Mask, FontWeight100Mask, FontWeight400Mask, FontWeight500Mask, FontWeight600Mask, FontWeight700Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight500Mask, FontWeight300Mask, FontWeight600Mask, FontWeight200Mask, FontWeight700Mask, FontWeight100Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight400Mask, FontWeight300Mask, FontWeight600Mask, FontWeight200Mask, FontWeight700Mask, FontWeight100Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight700Mask, FontWeight800Mask, FontWeight900Mask, FontWeight500Mask, FontWeight400Mask, FontWeight300Mask, FontWeight200Mask, FontWeight100Mask },
        { FontWeight800Mask, FontWeight900Mask, FontWeight600Mask, FontWeight500Mask, FontWeight400Mask, FontWeight300Mask, FontWeight200Mask, FontWeight100Mask },
        { FontWeight900Mask, FontWeight700Mask, FontWeight600Mask, FontWeight500Mask, FontWeight400Mask, FontWeight300Mask, FontWeight200Mask, FontWeight100Mask },
        { FontWeight800Mask, FontWeight700Mask, FontWeight600Mask, FontWeight500Mask, FontWeight400Mask, FontWeight300Mask, FontWeight200Mask, FontWeight100Mask }
    };

    unsigned ruleSetIndex = 0;
    unsigned w = FontWeight100Bit;
    while (!(desiredTraitsMaskForComparison & (1 << w))) {
        w++;
        ruleSetIndex++;
    }

    assert(ruleSetIndex < fallbackRuleSets);
    const FontTraitsMask* weightFallbackRule = weightFallbackRuleSets[ruleSetIndex];
    for (unsigned i = 0; i < rulesPerSet; ++i) {
        if (secondTraitsMask & weightFallbackRule[i])
            return false;
        if (firstTraitsMask & weightFallbackRule[i])
            return true;
    }

    return false;
}

/*
FontData* CSSFontSelector::getFontData(const FontDescription& fontDescription, const AtomicString& familyName)
{
    if (m_fontFaces.isEmpty()) {
        if (familyName.startsWith("-khtml-"))
            return fontDataForGenericFamily(m_document, fontDescription, familyName);
        return 0;
    }

    String family = familyName.string();

#if 0
    // ENABLE(SVG_FONTS)
    if (fontDescription.smallCaps())
        family += "-khtml-svg-small-caps";
#endif

    Vector<RefPtr<CSSFontFace> >* familyFontFaces = m_fontFaces.get(family);
    // If no face was found, then return 0 and let the OS come up with its best match for the name.
    if (!familyFontFaces || familyFontFaces->isEmpty()) {
        // If we were handed a generic family, but there was no match, go ahead and return the correct font based off our
        // settings.
        return fontDataForGenericFamily(m_document, fontDescription, familyName);
    }

    HashMap<unsigned, RefPtr<CSSSegmentedFontFace> >* segmentedFontFaceCache = m_fonts.get(family);
    if (!segmentedFontFaceCache) {
        segmentedFontFaceCache = new HashMap<unsigned, RefPtr<CSSSegmentedFontFace> >;
        m_fonts.set(family, segmentedFontFaceCache);
    }

    FontTraitsMask traitsMask = fontDescription.traitsMask();

    RefPtr<CSSSegmentedFontFace> face = segmentedFontFaceCache->get(traitsMask);

    if (!face) {
        face = CSSSegmentedFontFace::create(this);
        segmentedFontFaceCache->set(traitsMask, face);
        // Collect all matching faces and sort them in order of preference.
        Vector<CSSFontFace*, 32> candidateFontFaces;
        for (int i = familyFontFaces->size() - 1; i >= 0; --i) {
            CSSFontFace* candidate = familyFontFaces->at(i).get();
            unsigned candidateTraitsMask = candidate->traitsMask();
            if ((traitsMask & FontStyleNormalMask) && !(candidateTraitsMask & FontStyleNormalMask))
                continue;
            if ((traitsMask & FontVariantNormalMask) && !(candidateTraitsMask & FontVariantNormalMask))
                continue;
            candidateFontFaces.append(candidate);
        }

        if (Vector<RefPtr<CSSFontFace> >* familyLocallyInstalledFontFaces = m_locallyInstalledFontFaces.get(family)) {
            unsigned numLocallyInstalledFontFaces = familyLocallyInstalledFontFaces->size();
            for (unsigned i = 0; i < numLocallyInstalledFontFaces; ++i) {
                CSSFontFace* candidate = familyLocallyInstalledFontFaces->at(i).get();
                unsigned candidateTraitsMask = candidate->traitsMask();
                if ((traitsMask & FontStyleNormalMask) && !(candidateTraitsMask & FontStyleNormalMask))
                    continue;
                if ((traitsMask & FontVariantNormalMask) && !(candidateTraitsMask & FontVariantNormalMask))
                    continue;
                candidateFontFaces.append(candidate);
            }
        }

        desiredTraitsMaskForComparison = traitsMask;
        std::stable_sort(candidateFontFaces.begin(), candidateFontFaces.end(), compareFontFaces);
        unsigned numCandidates = candidateFontFaces.size();
        for (unsigned i = 0; i < numCandidates; ++i)
            face->appendFontFace(candidateFontFaces[i]);
    }

    // We have a face.  Ask it for a font data.  If it cannot produce one, it will fail, and the OS will take over.
    return face->getFontData(fontDescription);
}
*/


}
