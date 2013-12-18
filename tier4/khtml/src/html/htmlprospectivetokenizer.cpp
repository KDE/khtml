/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *           (C) 2008 Germain Garand <germain@ebooksfrance.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "htmlprospectivetokenizer.h"

#include <QTime>
#include <QVarLengthArray>

#include "html_headimpl.h"
#include "html_documentimpl.h"
#include "htmlparser.h"
#include "dtd.h"

#include <misc/loader.h>
#include <khtmlview.h>
#include <khtml_part.h>
#include <xml/dom_docimpl.h>
#include <css/csshelper.h>
#include <ecma/kjs_proxy.h>
#include <ctype.h>
#include <assert.h>
#include <QtCore/QVariant>
#include <stdlib.h>

#ifdef __GNUC__
// The main tokenizer includes this too so we are getting two copies of the data. However, this way the code gets inlined.
#include "kentities.c"
#else
// Not inlined for non-GCC compilers
struct entity {
    const char* name;
    int code;
};
const struct entity *kde_findEntity (register const char *str, register unsigned int len);
#endif

#define PRELOAD_DEBUG 0

#define U16_TRAIL(sup) (ushort)(((sup)&0x3ff)|0xdc00)
#define U16_LEAD(sup) (ushort)(((sup)>>10)+0xd7c0)

using namespace khtml;
    
ProspectiveTokenizer::ProspectiveTokenizer(DOM::DocumentImpl * doc)
    : m_inProgress(false)
    , m_tagName(32)
    , m_attributeName(32)
    , m_attributeValue(255)
    , m_cssRule(16)
    , m_cssRuleValue(255)
    , m_timeUsed(0)
    , m_document(doc)
{
#if PRELOAD_DEBUG
    qDebug() << "CREATING PRELOAD SCANNER FOR" << m_document << m_document->URL().toDisplayString();
#endif
}
    
ProspectiveTokenizer::~ProspectiveTokenizer()
{
#if PRELOAD_DEBUG
    fprintf(stderr, "DELETING PRELOAD SCANNER FOR %p\n", m_document);
    fprintf(stderr, "TOTAL TIME USED %dms\n", m_timeUsed);
#endif
}
    
void ProspectiveTokenizer::begin() 
{ 
    assert(!m_inProgress); 
    reset(); 
    m_inProgress = true; 
}
    
void ProspectiveTokenizer::end() 
{ 
    assert(m_inProgress); 
    m_inProgress = false; 
}

void ProspectiveTokenizer::reset()
{
    m_source.clear();
    
    m_state = Data;
    m_escape = false;
    m_contentModel = PCDATA;
    m_commentPos = 0;

    m_closeTag = false;
    m_tagName.clear();
    m_attributeName.clear();
    m_attributeValue.clear();
    m_lastStartTag.clear();
    m_lastStartTagId = 0;
    
    m_urlToLoad = "";
    m_linkIsStyleSheet = false;
    m_lastCharacterIndex = 0;
    clearLastCharacters();
    
    m_cssState = CSSInitial;
    m_cssRule.clear();
    m_cssRuleValue.clear();
}
    
void ProspectiveTokenizer::write(const TokenizerString& source)
{
#if PRELOAD_DEBUG
    QTime t;
    t.start();
#endif

    tokenize(source);

#if PRELOAD_DEBUG
    m_timeUsed += t.elapsed();
#endif
}
    
static inline bool isWhitespace(const QChar& c)
{
    unsigned short u = c.unicode();
    if (u > 0x20)
        return false;
    return u == ' ' || u == '\n' || u == '\r' || u == '\t';
}
    
inline void ProspectiveTokenizer::clearLastCharacters()
{
    memset(m_lastCharacters, 0, lastCharactersBufferSize * sizeof(QChar));
}
    
inline void ProspectiveTokenizer::rememberCharacter(QChar c)
{
    m_lastCharacterIndex = (m_lastCharacterIndex + 1) % lastCharactersBufferSize;
    m_lastCharacters[m_lastCharacterIndex] = c;
}
    
inline bool ProspectiveTokenizer::lastCharactersMatch(const char* chars, unsigned count) const
{
    unsigned pos = m_lastCharacterIndex;
    while (count) {
        if (chars[count - 1] != m_lastCharacters[pos])
            return false;
        --count;
        if (!pos)
            pos = lastCharactersBufferSize;
        --pos;
    }
    return true;
}
    
static inline unsigned legalEntityFor(unsigned value)
{
    // FIXME There is a table for more exceptions in the HTML5 specification.
    if (value == 0 || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
        return 0xFFFD;
    return value;
}
    
unsigned ProspectiveTokenizer::consumeEntity(TokenizerString& source, bool& notEnoughCharacters)
{
    enum EntityState {
        Initial,
        NumberType,
        MaybeHex,
        Hex,
        Decimal,
        Named
    };
    EntityState entityState = Initial;
    unsigned result = 0;
    QVarLengthArray<QChar> seenChars;
    QVarLengthArray<char>  entityName;
    
    while (!source.isEmpty()) {
        seenChars.append(*source);
        ushort cc = source->unicode();
        switch (entityState) {
        case Initial:
            if (isWhitespace(cc) || cc == '<' || cc == '&')
                return 0;
            else if (cc == '#') 
                entityState = NumberType;
            else if ((cc >= 'a' && cc <= 'z') || (cc >= 'A' && cc <= 'Z')) {
                entityName.append(cc);
                entityState = Named;
            } else
                return 0;
            break;
        case NumberType:
            if (cc == 'x' || cc == 'X')
                entityState = MaybeHex;
            else if (cc >= '0' && cc <= '9') {
                entityState = Decimal;
                result = cc - '0';
            } else {
                source.push('#');
                return 0;
            }
            break;
        case MaybeHex:
            if (cc >= '0' && cc <= '9')
                result = cc - '0';
            else if (cc >= 'a' && cc <= 'f')
                result = 10 + cc - 'a';
            else if (cc >= 'A' && cc <= 'F')
                result = 10 + cc - 'A';
            else {
                source.push(seenChars[1]);
                source.push('#');
                return 0;
            }
            entityState = Hex;
            break;
        case Hex:
            if (cc >= '0' && cc <= '9')
                result = result * 16 + cc - '0';
            else if (cc >= 'a' && cc <= 'f')
                result = result * 16 + 10 + cc - 'a';
            else if (cc >= 'A' && cc <= 'F')
                result = result * 16 + 10 + cc - 'A';
            else if (cc == ';') {
                source.advance();
                return legalEntityFor(result);
            } else 
                return legalEntityFor(result);
            break;
        case Decimal:
            if (cc >= '0' && cc <= '9')
                result = result * 10 + cc - '0';
            else if (cc == ';') {
                source.advance();
                return legalEntityFor(result);
            } else
                return legalEntityFor(result);
            break;               
        case Named:
            // This is the attribute only version, generic version matches somewhat differently
            while (entityName.size() <= 8) {
                if (cc == ';') {
                    const entity* e = kde_findEntity(entityName.data(), entityName.size());
                    if (e) {
                        source.advance();
                        return e->code;
                    }
                    break;
                }
                if (!(cc >= 'a' && cc <= 'z') && !(cc >= 'A' && cc <= 'Z') && !(cc >= '0' && cc <= '9')) {
                    const entity* e = kde_findEntity(entityName.data(), entityName.size());
                    if (e)
                        return e->code;
                    break;
                }
                entityName.append(cc);
                source.advance();
                if (source.isEmpty())
                    goto outOfCharacters;
                cc = source->unicode();
                seenChars.append(cc);
            }
            if (seenChars.size() == 2)
                source.push(seenChars[0]);
            else if (seenChars.size() == 3) {
                source.push(seenChars[1]);
                source.push(seenChars[0]);
            } else
                source.prepend(TokenizerString(QString(seenChars.data(), seenChars.size() - 1)));
            return 0;
        }
        source.advance();
    }
outOfCharacters:
    notEnoughCharacters = true;
    source.prepend(TokenizerString(QString(seenChars.data(), seenChars.size())));
    return 0;
}

void ProspectiveTokenizer::tokenize(const TokenizerString& source)
{
    assert(m_inProgress);
    
    m_source.append(source);

    // This is a simplified HTML5 Tokenizer
    // http://www.whatwg.org/specs/web-apps/current-work/#tokenisation0
    while (!m_source.isEmpty()) {
        ushort cc = m_source->unicode();
        switch (m_state) {
        case Data:
            while (1) {
                rememberCharacter(cc);
                if (cc == '&') {
                    if (m_contentModel == PCDATA || m_contentModel == RCDATA) {
                        m_state = EntityData;
                        break;
                    }
                } else if (cc == '-') {
                    if ((m_contentModel == RCDATA || m_contentModel == CDATA) && !m_escape) {
                        if (lastCharactersMatch("<!--", 4))
                            m_escape = true;
                    }
                } else if (cc == '<') {
                    if (m_contentModel == PCDATA || ((m_contentModel == RCDATA || m_contentModel == CDATA) && !m_escape)) {
                        m_state = TagOpen;
                        break;
                    }
                } else if (cc == '>') {
                     if ((m_contentModel == RCDATA || m_contentModel == CDATA) && m_escape) {
                         if (lastCharactersMatch("-->", 3))
                             m_escape = false;
                     }
                }
                emitCharacter(cc);
                m_source.advance();
                if (m_source.isEmpty())
                     return;
                cc = m_source->unicode();
            }
            break;
        case EntityData:
            // should try to consume the entity but we only care about entities in attributes
            m_state = Data;
            break;
        case TagOpen:
            if (m_contentModel == RCDATA || m_contentModel == CDATA) {
                if (cc == '/')
                    m_state = CloseTagOpen;
                else {
                    m_state = Data;
                    continue;
                }
            } else if (m_contentModel == PCDATA) {
                if (cc == '!')
                    m_state = MarkupDeclarationOpen;
                else if (cc == '/')
                    m_state = CloseTagOpen;
                else if (cc >= 'A' && cc <= 'Z') {
                    m_tagName.clear();
                    m_tagName.append(cc + 0x20);
                    m_closeTag = false;
                    m_state = TagName;
                } else if (cc >= 'a' && cc <= 'z') {
                    m_tagName.clear();
                    m_tagName.append(cc);
                    m_closeTag = false;
                    m_state = TagName;
                } else if (cc == '>') {
                    m_state = Data;
                } else if (cc == '?') {
                    m_state = BogusComment;
                } else {
                    m_state = Data;
                    continue;
                }
            }
            break;
        case CloseTagOpen:
            if (m_contentModel == RCDATA || m_contentModel == CDATA) {
                if (!m_lastStartTag.size()) {
                    m_state = Data;
                    continue;
                }
                if ((unsigned)m_source.length() < m_lastStartTag.size() + 1)
                    return;
                QVector<QChar> tmpString;
                QChar tmpChar = 0;
                bool match = true;
                for (unsigned n = 0; n < m_lastStartTag.size() + 1; n++) {
                    tmpChar = m_source->toLower();
                    if (n < m_lastStartTag.size() && tmpChar != m_lastStartTag[n])
                        match = false;
                    tmpString.append(tmpChar);
                    m_source.advance();
                }
                m_source.prepend(TokenizerString(QString(tmpString.data(), tmpString.size())));
                if (!match || (!isWhitespace(tmpChar) && tmpChar != '>' && tmpChar != '/')) {
                    m_state = Data;
                    continue;
                }
            }
            if (cc >= 'A' && cc <= 'Z') {
                m_tagName.clear();
                m_tagName.append(cc + 0x20);
                m_closeTag = true;
                m_state = TagName;
            } else if (cc >= 'a' && cc <= 'z') {
                m_tagName.clear();
                m_tagName.append(cc);
                m_closeTag = true;
                m_state = TagName;
            } else if (cc == '>') {
                m_state = Data;
            } else
                m_state = BogusComment;
            break;
        case TagName:
            while (1) {
                if (isWhitespace(cc)) {
                    m_state = BeforeAttributeName;
                    break;
                }
                if (cc == '>') {
                    emitTag();
                    m_state = Data;
                    break;
                }
                if (cc == '/') {
                    m_state = BeforeAttributeName;
                    break;
                }
                if (cc >= 'A' && cc <= 'Z')
                    m_tagName.append(cc + 0x20);
                else
                    m_tagName.append(cc);
                m_source.advance();
                if (m_source.isEmpty())
                    return;
                cc = m_source->unicode();
            }
            break;
        case BeforeAttributeName:
            if (isWhitespace(cc))
                ;
            else if (cc == '>') {
                emitTag();
                m_state = Data;
            } else if (cc >= 'A' && cc <= 'Z') {
                m_attributeName.clear();
                m_attributeValue.clear();
                m_attributeName.append(cc + 0x20);
                m_state = AttributeName;
            } else if (cc == '/')
                ;
            else {
                m_attributeName.clear();
                m_attributeValue.clear();
                m_attributeName.append(cc);
                m_state = AttributeName;
            }
            break;
        case AttributeName:
            while (1) {
                if (isWhitespace(cc)) {
                    m_state = AfterAttributeName;
                    break;
                }
                if (cc == '=') {
                    m_state = BeforeAttributeValue;
                    break;
                }
                if (cc == '>') {
                    emitTag();
                    m_state = Data;
                    break;
                } 
                if (cc == '/') {
                    m_state = BeforeAttributeName;
                    break;
                }
                if (cc >= 'A' && cc <= 'Z')
                    m_attributeName.append(cc + 0x20);
                else
                    m_attributeName.append(cc);
                m_source.advance();
                if (m_source.isEmpty())
                    return;
                cc = m_source->unicode();
            }
            break;
        case AfterAttributeName:
            if (isWhitespace(cc))
                ;
            else if (cc == '=')
                m_state = BeforeAttributeValue; 
            else if (cc == '>') {
                emitTag();
                m_state = Data;
            } else if (cc >= 'A' && cc <= 'Z') {
                m_attributeName.clear();
                m_attributeValue.clear();
                m_attributeName.append(cc + 0x20);
                m_state = AttributeName;
            } else if (cc == '/')
                m_state = BeforeAttributeName;
            else {
                m_attributeName.clear();
                m_attributeValue.clear();
                m_attributeName.append(cc);
                m_state = AttributeName;
            }
            break;
        case BeforeAttributeValue:
            if (isWhitespace(cc))
                ;
            else if (cc == '"')
                m_state = AttributeValueDoubleQuoted;
            else if (cc == '&') {
                m_state = AttributeValueUnquoted;
                continue;
            } else if (cc == '\'')
                m_state = AttributeValueSingleQuoted;
            else if (cc == '>') {
                emitTag();
                m_state = Data;
            } else {
                m_attributeValue.append(cc);
                m_state = AttributeValueUnquoted;
            }
            break;
        case AttributeValueDoubleQuoted:
            while (1) {
                if (cc == '"') {
                    processAttribute();
                    m_state = BeforeAttributeName;
                    break;
                }
                if (cc == '&') {
                    m_stateBeforeEntityInAttributeValue = m_state;
                    m_state = EntityInAttributeValue;
                    break;
                } 
                m_attributeValue.append(cc);
                m_source.advance();
                if (m_source.isEmpty())
                    return;
                cc = m_source->unicode();
            }
            break;
        case AttributeValueSingleQuoted:
            while (1) {
                if (cc == '\'') {
                    processAttribute();
                    m_state = BeforeAttributeName;
                    break;
                }
                if (cc == '&') {
                    m_stateBeforeEntityInAttributeValue = m_state;
                    m_state = EntityInAttributeValue;
                    break;
                } 
                m_attributeValue.append(cc);
                m_source.advance();
                if (m_source.isEmpty())
                    return;
                cc = m_source->unicode();
            }
            break;
        case AttributeValueUnquoted:
            while (1) {
                if (isWhitespace(cc)) {
                    processAttribute();
                    m_state = BeforeAttributeName;
                    break;
                }
                if (cc == '&') {
                    m_stateBeforeEntityInAttributeValue = m_state;
                    m_state = EntityInAttributeValue;
                    break;
                }
                if (cc == '>') {
                    processAttribute();
                    emitTag();
                    m_state = Data;
                    break;
                }
                m_attributeValue.append(cc);
                m_source.advance();
                if (m_source.isEmpty())
                    return;
                cc = m_source->unicode();
            }
            break;
        case EntityInAttributeValue: 
            {
                bool notEnoughCharacters = false; 
                unsigned entity = consumeEntity(m_source, notEnoughCharacters);
                if (notEnoughCharacters)
                    return;
                if (entity > 0xFFFF) {
                    m_attributeValue.append(U16_LEAD(entity));
                    m_attributeValue.append(U16_TRAIL(entity));
                } else if (entity)
                    m_attributeValue.append(entity);
                else
                    m_attributeValue.append('&');
            }
            m_state = m_stateBeforeEntityInAttributeValue;
            continue;
        case BogusComment:
            while (1) {
                if (cc == '>') {
                    m_state = Data;
                    break;
                }
                m_source.advance();
                if (m_source.isEmpty())
                    return;
                cc = m_source->unicode();
            }
            break;
        case MarkupDeclarationOpen: {
            if (cc == '-') {
                if (m_source.length() < 2)
                    return;
                m_source.advance();
                cc = m_source->unicode();
                if (cc == '-')
                    m_state = CommentStart;
                else {
                    m_state = BogusComment;
                    continue;
                }
            // If we cared about the DOCTYPE we would test to enter those states here
            } else {
                m_state = BogusComment;
                continue;
            }
            break;
        }
        case CommentStart:
            if (cc == '-')
                m_state = CommentStartDash;
            else if (cc == '>')
                m_state = Data;
            else
                m_state = Comment;
            break;
        case CommentStartDash:
            if (cc == '-')
                m_state = CommentEnd;
            else if (cc == '>')
                m_state = Data;
            else
                m_state = Comment;
            break;
        case Comment:
            while (1) {
                if (cc == '-') {
                    m_state = CommentEndDash;
                    break;
                }
                m_source.advance();
                if (m_source.isEmpty())
                    return;
                cc = m_source->unicode();
            }
            break;
        case CommentEndDash:
            if (cc == '-')
                m_state = CommentEnd;
            else 
                m_state = Comment;
            break;
        case CommentEnd:
            if (cc == '>')
                m_state = Data;
            else if (cc == '-')
                ;
            else 
                m_state = Comment;
            break;
        }
        m_source.advance();
    }
}
    
void ProspectiveTokenizer::processAttribute()
{
    DOMStringImpl tagNameDS(DOMStringImpl::ShallowCopy, m_tagName.data(), m_tagName.size());
    LocalName tagLocal = LocalName::fromString(&tagNameDS, IDS_NormalizeLower);
    uint tag = tagLocal.id();

    switch (tag) {
    case ID_SCRIPT:
    case ID_IMAGE:
    case ID_IMG:
    {
        DOMStringImpl attrDS(DOMStringImpl::ShallowCopy, m_attributeName.data(), m_attributeName.size());
        LocalName attrLocal = LocalName::fromString(&attrDS, IDS_NormalizeLower);
        uint attribute = attrLocal.id();
        if (attribute == localNamePart(ATTR_SRC) && m_urlToLoad.isEmpty())
            m_urlToLoad = parseURL(DOMString(m_attributeValue.data(), m_attributeValue.size()));
        break;
    }
    case ID_LINK:
    {
        DOMStringImpl attrDS(DOMStringImpl::ShallowCopy, m_attributeName.data(), m_attributeName.size());
        LocalName attrLocal = LocalName::fromString(&attrDS, IDS_NormalizeLower);
        uint attribute = attrLocal.id();
        if (attribute == localNamePart(ATTR_HREF) && m_urlToLoad.isEmpty())
            m_urlToLoad = parseURL(DOMString(m_attributeValue.data(), m_attributeValue.size()));
        else if (attribute == localNamePart(ATTR_REL)) {
            DOMStringImpl* lowerAttribute = DOMStringImpl(DOMStringImpl::ShallowCopy, m_attributeValue.data(), m_attributeValue.size()).lower();
            QString val = lowerAttribute->string();
            delete lowerAttribute;
            m_linkIsStyleSheet = val.contains("stylesheet") && !val.contains("alternate") && !val.contains("icon");
        }
    }
    default:
        break;
    }
}
    
inline void ProspectiveTokenizer::emitCharacter(QChar c)
{
    if (m_contentModel == CDATA && m_lastStartTagId == ID_STYLE) 
        tokenizeCSS(c);
}
    
inline void ProspectiveTokenizer::tokenizeCSS(QChar c)
{    
    // We are just interested in @import rules, no need for real tokenization here
    // Searching for other types of resources is probably low payoff
    switch (m_cssState) {
    case CSSInitial:
        if (c == '@')
            m_cssState = CSSRuleStart;
        else if (c == '/')
            m_cssState = CSSMaybeComment;
        break;
    case CSSMaybeComment:
        if (c == '*')
            m_cssState = CSSComment;
        else
            m_cssState = CSSInitial;
        break;
    case CSSComment:
        if (c == '*')
            m_cssState = CSSMaybeCommentEnd;
        break;
    case CSSMaybeCommentEnd:
        if (c == '/')
            m_cssState = CSSInitial;
        else if (c == '*')
            ;
        else
            m_cssState = CSSComment;
        break;
    case CSSRuleStart:
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            m_cssRule.clear();
            m_cssRuleValue.clear();
            m_cssRule.append(c);
            m_cssState = CSSRule;
        } else
            m_cssState = CSSInitial;
        break;
    case CSSRule:
        if (isWhitespace(c))
            m_cssState = CSSAfterRule;
        else if (c == ';')
            m_cssState = CSSInitial;
        else
            m_cssRule.append(c);
        break;
    case CSSAfterRule:
        if (isWhitespace(c))
            ;
        else if (c == ';')
            m_cssState = CSSInitial;
        else {
            m_cssState = CSSRuleValue;
            m_cssRuleValue.append(c);
        }
        break;
    case CSSRuleValue:
        if (isWhitespace(c))
            m_cssState = CSSAferRuleValue;
        else if (c == ';') {
            emitCSSRule();
            m_cssState = CSSInitial;
        } else 
            m_cssRuleValue.append(c);
        break;
    case CSSAferRuleValue:
        if (isWhitespace(c))
            ;
        else if (c == ';') {
            emitCSSRule();
            m_cssState = CSSInitial;
        } else {
            // FIXME media rules
             m_cssState = CSSInitial;
        }
        break;
    }
}
    
void ProspectiveTokenizer::emitTag()
{
    if (m_closeTag) {
        m_contentModel = PCDATA;
        m_cssState = CSSInitial;
        clearLastCharacters();
        return;
    }
  
    DOMStringImpl tagNameDS(DOMStringImpl::ShallowCopy, m_tagName.data(), m_tagName.size());
    LocalName tagLocal = LocalName::fromString(&tagNameDS, IDS_NormalizeLower);
    uint tag = tagLocal.id();
    m_lastStartTagId = tag;
    m_lastStartTag = m_tagName;

    switch (tag) {
      case ID_TEXTAREA:
      case ID_TITLE:
        m_contentModel = RCDATA;
        break;
      case ID_STYLE:
      case ID_XMP:
      case ID_SCRIPT:
      case ID_IFRAME:
      case ID_NOEMBED:
      case ID_NOFRAMES:
        m_contentModel = CDATA;
        break;
      case ID_NOSCRIPT:
        // we wouldn't be here if scripts were disabled
        m_contentModel = CDATA;
        break;
      case ID_PLAINTEXT:
        m_contentModel = PLAINTEXT;
        break;
      default:
        m_contentModel = PCDATA;
    }

    if (m_urlToLoad.isEmpty()) {
        m_linkIsStyleSheet = false;
        return;
    }

    CachedObject* o = 0;
    if (tag == ID_SCRIPT)
         o = m_document->docLoader()->requestScript( m_urlToLoad, m_document->part()->encoding() );
    else if (tag == ID_IMAGE || tag == ID_IMG) 
         o = m_document->docLoader()->requestImage( m_urlToLoad );
    else if (tag == ID_LINK && m_linkIsStyleSheet) 
        o = m_document->docLoader()->requestStyleSheet( m_urlToLoad, m_document->part()->encoding() );

    if (o)
        m_document->docLoader()->registerPreload( o );

    m_urlToLoad = DOMString();
    m_linkIsStyleSheet = false;
}
    
void ProspectiveTokenizer::emitCSSRule()
{
    QString rule(m_cssRule.data(), m_cssRule.size());
    if (rule.toLower() == "import" && !m_cssRuleValue.isEmpty()) {
        DOMString value = DOMString(m_cssRuleValue.data(), m_cssRuleValue.size());
        DOMString url = parseURL(value);
        if (!url.isEmpty())
            m_document->docLoader()->registerPreload( m_document->docLoader()->requestStyleSheet( m_urlToLoad, m_document->part()->encoding() ) ); // #### charset
    }
    m_cssRule.clear();
    m_cssRuleValue.clear();
}
        
