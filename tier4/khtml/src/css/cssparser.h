/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
 */
#ifndef _CSS_cssparser_h_
#define _CSS_cssparser_h_

#include <QtCore/QString>
#include <QColor>
#include <QtCore/QVector>
#include <dom/dom_string.h>
#include <misc/htmlnames.h>
#include <wtf/Vector.h>
#include <xml/dom_docimpl.h>

namespace khtml {
    class MediaQuery;
}

namespace DOM {
    class StyleListImpl;
    class CSSStyleSheetImpl;
    class CSSRuleImpl;
    class CSSStyleRuleImpl;
    class DocumentImpl;
    class CSSValueImpl;
    class CSSValueListImpl;
    class CSSPrimitiveValueImpl;
    class CSSStyleDeclarationImpl;
    class CSSFontFaceRuleImpl;
    class CSSProperty;
    class MediaListImpl;
    class CSSSelector;
    

    struct ParseString {
	unsigned short *string;
	int length;
    };

    struct Value;
    class ValueList;

    struct Function {
	ParseString name;
	ValueList *args;
    };

    struct Value {
	int id;
        bool isInt;
	union {
	    double fValue;
	    int iValue;
	    ParseString string;
	    struct Function *function;
	};
	enum {
	    Operator = 0x100000,
	    Function = 0x100001,
	    Q_EMS     = 0x100002
	};

	int unit;
    };

    static inline QString qString( const ParseString &ps ) {
	return QString( (QChar *)ps.string, ps.length );
    }
    static inline DOMString domString( const ParseString &ps ) {
	return DOMString( (QChar *)ps.string, ps.length );
    }

    class ValueList {
    public:
        ValueList() : m_current(0) { }
        ~ValueList();
        void addValue(const Value& v) { m_values.append(v); }
        unsigned int size() const { return m_values.size(); }
        Value* current() { return m_current < m_values.size() ? &m_values[m_current] : 0; }
        Value* next() { ++m_current; return current(); }
    private:
        QVector<Value> m_values;
        int m_current;
    };

    class CSSParser
    {
    public:
	CSSParser( bool strictParsing = true );
	~CSSParser();

	void parseSheet( DOM::CSSStyleSheetImpl *sheet, const DOM::DOMString &string );
	DOM::CSSRuleImpl *parseRule( DOM::CSSStyleSheetImpl *sheet, const DOM::DOMString &string );
	bool parseValue( DOM::CSSStyleDeclarationImpl *decls, int id, const DOM::DOMString &string,
			 bool _important );
	bool parseDeclaration( DOM::CSSStyleDeclarationImpl *decls, const DOM::DOMString &string );
	bool parseMediaQuery(DOM::MediaListImpl* queries, const DOM::DOMString& string);
	QList<DOM::CSSSelector*> parseSelectorList(DOM::DocumentImpl* doc, const DOM::DOMString& string);
	    // Returns an empty list on parse error.

	static CSSParser *current() { return currentParser; }

	unsigned int getLocalNameId(const DOMString& str) {
	    LocalName localname;
	    DOM::DocumentImpl *doc = document();
	    if (doc && doc->isHTMLDocument())
		localname = LocalName::fromString(str, khtml::IDS_NormalizeLower);
	    else
		localname = LocalName::fromString(str);
	    boundLocalNames.append(localname);
	    return localname.id();
	}

	DOM::DocumentImpl *document() const;

	unsigned int defaultNamespace();

	void addProperty( int propId, CSSValueImpl *value, bool important );
	bool hasProperties() const { return numParsedProperties > 0; }
	CSSStyleDeclarationImpl *createStyleDeclaration( CSSStyleRuleImpl *rule );
	CSSStyleDeclarationImpl *createFontFaceStyleDeclaration( CSSFontFaceRuleImpl *rule );
	void clearProperties();

	bool parseValue( int propId, bool important );
        bool parseSVGValue( int propId, bool important );
	bool parseShortHand( int propId, const int *properties, int numProperties, bool important );
	bool parse4Values( int propId, const int *properties, bool important );
	bool parseContent( int propId, bool important );

        CSSValueImpl* parseBackgroundColor();
        CSSValueImpl* parseBackgroundImage(bool& didParse);

        enum BackgroundPosKind {
            BgPos_X,
            BgPos_Y,
            BgPos_NonKW,
            BgPos_Center
        };
        
        CSSValueImpl* parseBackgroundPositionXY(BackgroundPosKind& kindOut);
        void parseBackgroundPosition(CSSValueImpl*& value1, CSSValueImpl*& value2);
        CSSValueImpl* parseBackgroundSize();

        bool parseBackgroundProperty(int propId, int& propId1, int& propId2, CSSValueImpl*& retValue1, CSSValueImpl*& retValue2);
        bool parseBackgroundShorthand(bool important);

        void addBackgroundValue(CSSValueImpl*& lval, CSSValueImpl* rval);

	bool parseShape( int propId, bool important );
	bool parseFont(bool important);
	bool parseFontFaceSrc();
        bool parseCounter(int propId, bool increment, bool important);

        bool parseColorParameters(Value*, int* colorValues, bool parseAlpha);
        bool parseHSLParameters(Value*, double* colorValues, bool parseAlpha);

        // returns the found property
        // 0 if nothing found (or ok == false)
        // @param forward if true, it parses the next in the list
	CSSValueListImpl *parseFontFamily();
        CSSPrimitiveValueImpl *parseColor();
        CSSPrimitiveValueImpl *parseColorFromValue(Value* val);
        CSSValueImpl* parseCounterContent(ValueList *args, bool counters);

        // CSS3 Parsing Routines (for properties specific to CSS3)
        bool parseShadow(int propId, bool important);

        // SVG parsing:
        CSSValueImpl* parseSVGStrokeDasharray();
        CSSValueImpl* parseSVGPaint();
        CSSValueImpl* parseSVGColor();
    private:
        // defines units allowed for a certain property, used in parseUnit
        enum Units {
            FUnknown   = 0x0000,
            FInteger   = 0x0001,
            FNumber    = 0x0002,  // Real Numbers
            FPercent   = 0x0004,
            FLength    = 0x0008,
            FAngle     = 0x0010,
            FTime      = 0x0020,
            FFrequency = 0x0040,
            FRelative  = 0x0100,
            FNonNeg    = 0x0200
        };

        bool parseBorderImage(int propId, bool important);
        bool parseBorderRadius(bool important);

        static bool validUnit( Value *value, int unitflags, bool strict );


    public:
	bool strict;
	bool important;
	unsigned int id;
	DOM::StyleListImpl* styleElement;
	mutable DOM::DocumentImpl*  styleDocument; // cached document for styleElement,
	                                           // or manually set one for special parses

	// Outputs for specialized parses. 
	DOM::CSSRuleImpl *rule;
	khtml::MediaQuery* mediaQuery;
	QList<DOM::CSSSelector*> selectors;
	
	ValueList *valueList;
	CSSProperty **parsedProperties;
	int numParsedProperties;
	int maxParsedProperties;

        int m_inParseShorthand;
        int m_currentShorthand;
        bool m_implicitShorthand;
        

	static CSSParser *currentParser;

	// tokenizer methods and data
    public:
	int lex( void *yylval );
	int token() { return yyTok; }
	unsigned short *text( int *length);
	int lex();
    private:
	int yyparse();
	void runParser();
        void setupParser(const char *prefix, const DOMString &string, const char *suffix);

        bool inShorthand() const { return m_inParseShorthand; }

	unsigned short *data;
	unsigned short *yytext;
	unsigned short *yy_c_buf_p;
	unsigned short yy_hold_char;
	int yy_last_accepting_state;
	unsigned short *yy_last_accepting_cpos;
        int block_nesting;
	int yyleng;
	int yyTok;
	int yy_start;
        WTF::Vector<LocalName> boundLocalNames;
    };

} // namespace
#endif
// kate: tab-width 8;

