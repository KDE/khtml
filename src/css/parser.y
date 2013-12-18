%{

/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2002-2003 Lars Knoll (knoll@kde.org)
 *  Copyright (c) 2003 Apple Computer
 *  Copyright (C) 2003 Dirk Mueller (mueller@kde.org)
 *  Copyright (C) 2008 Germain Garand (germain@ebooksfrance.org)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <string.h>
#include <stdlib.h>

#include <dom/dom_string.h>
#include <xml/dom_docimpl.h>
#include <css/cssstyleselector.h>
#include <css/css_ruleimpl.h>
#include <css/css_stylesheetimpl.h>
#include <css/css_valueimpl.h>
#include <css/css_mediaquery.h>
#include "cssparser.h"



#include <assert.h>
#include <QDebug>

//#define CSS_DEBUG

using namespace DOM;

//
// The following file defines the function
//     const struct props *findProp(const char *word, int len)
//
// with 'props->id' a CSS property in the range from CSS_PROP_MIN to
// (and including) CSS_PROP_TOTAL-1

// Turn off gnu90 inlining to avoid linker errors
#undef __GNUC_STDC_INLINE__
#undef __GNUC_GNU_INLINE__
#include "cssproperties.c"
#include "cssvalues.c"

static QHash<QString,int>* sCompatibleProperties = 0;

static const int sMinCompatPropLen = 21; // shortest key in the hash below

static void initCompatibleProperties() {
     QHash<QString,int>*& cp = sCompatibleProperties;
     // Hash of (Property name, Vendor Prefix length)
     cp = new QHash<QString, int>;
     cp->insert("-webkit-background-clip", 7);
     cp->insert("-webkit-background-origin", 7);
     cp->insert("-webkit-background-size", 7);
     cp->insert("-webkit-border-top-right-radius", 7);
     cp->insert("-webkit-border-bottom-right-radius", 7);
     cp->insert("-webkit-border-bottom-left-radius", 7);
     cp->insert("-webkit-border-top-left-radius", 7);
     cp->insert("-webkit-border-radius", 7);
}

int DOM::getPropertyID(const char *tagStr, int len)
{
    { // HTML CSS Properties
        const struct css_prop *prop = findProp(tagStr, len);
        if (!prop)
            return 0;

        return prop->id;
    }
}

int DOM::getValueID(const char *tagStr, int len)
{
    { // HTML CSS Values
        const struct css_value *val = findValue(tagStr, len);
        if (!val)
            return 0;

        return val->id;
    }
}

#define YYDEBUG 0
#define YYMAXDEPTH 10000
#define YYPARSE_PARAM parser
#define YYENABLE_NLS 0
#define YYLTYPE_IS_TRIVIAL 1
%}

%expect 39

%pure_parser

%union {
    CSSRuleImpl *rule;
    CSSSelector *selector;
    QList<CSSSelector*> *selectorList;
    bool ok;
    MediaListImpl *mediaList;
    CSSMediaRuleImpl *mediaRule;
    CSSRuleListImpl *ruleList;
    ParseString string;
    float val;
    int prop_id;
    unsigned int attribute;
    unsigned int element;
    CSSSelector::Relation relation;
    CSSSelector::Match match;
    bool b;
    char tok;
    Value value;
    ValueList *valueList;

    khtml::MediaQuery* mediaQuery;
    khtml::MediaQueryExp* mediaQueryExp;
    QList<khtml::MediaQueryExp*>* mediaQueryExpList;
    khtml::MediaQuery::Restrictor mediaQueryRestrictor;
}

%{

static inline int cssyyerror(const char *x )
{
#ifdef CSS_DEBUG
    qDebug( "%s", x );
#else
    Q_UNUSED( x );
#endif
    return 1;
}

static int cssyylex( YYSTYPE *yylval ) {
    return CSSParser::current()->lex( yylval );
}

#define null 1

%}

%destructor { delete $$; $$ = 0; } maybe_media_value expr;
%destructor { delete $$; $$ = 0; } maybe_media_list media_list;
%destructor { delete $$; $$ = 0; } maybe_and_media_query_exp_list media_query_exp_list;
%destructor { if ($$) qDeleteAll(*$$); delete $$; $$ = 0; } selector_list;
%destructor { delete $$; $$ = 0; } ruleset_list;
%destructor { delete $$; $$ = 0; } specifier specifier_list simple_selector simple_css3_selector selector class attrib pseudo;
%destructor { if ($$.function) delete $$.function->args; delete $$.function; $$.function = 0; } function;

%no-lines
%verbose

%left REDUCE

%token S SGML_CD

%token INCLUDES
%token DASHMATCH
%token BEGINSWITH
%token ENDSWITH
%token CONTAINS

%token <string> STRING

%right <string> IDENT

%token <string> NTH

%nonassoc <string> HASH
%nonassoc <string> HEXCOLOR	
%nonassoc ':'
%nonassoc '.'
%nonassoc '['
%nonassoc '*'
%nonassoc error
%left '|'

%left IMPORT_SYM
%token PAGE_SYM
%token MEDIA_SYM
%token FONT_FACE_SYM
%token CHARSET_SYM
%token NAMESPACE_SYM
%token KHTML_RULE_SYM
%token KHTML_DECLS_SYM
%token KHTML_VALUE_SYM
%token KHTML_MEDIAQUERY_SYM
%token KHTML_SELECTORS_SYM

%token IMPORTANT_SYM
%token MEDIA_ONLY
%token MEDIA_NOT
%token MEDIA_AND

%token <val> QEMS
%token <val> EMS
%token <val> EXS
%token <val> PXS
%token <val> CMS
%token <val> MMS
%token <val> INS
%token <val> PTS
%token <val> PCS
%token <val> DEGS
%token <val> RADS
%token <val> GRADS
%token <val> MSECS
%token <val> SECS
%token <val> HERZ
%token <val> KHERZ
%token <val> DPI
%token <val> DPCM
%token <string> DIMEN
%token <val> PERCENTAGE
%token <val> FLOAT
%token <val> INTEGER

%token <string> URI
%token <string> FUNCTION
%token <string> NOTFUNCTION

%token <string> UNICODERANGE

%type <relation> combinator

%type <rule> charset
%type <rule> ruleset
%type <rule> safe_ruleset
%type <rule> ruleset_or_import_or_namespace
%type <rule> media
%type <rule> import
%type <rule> page
%type <rule> font_face
%type <rule> invalid_rule
%type <rule> namespace_rule
%type <rule> invalid_at
%type <rule> rule

%type <string> namespace_selector

%type <string> string_or_uri
%type <string> ident_or_string
%type <string> medium
%type <string> hexcolor
%type <string> maybe_ns_prefix

%type <string> media_feature
%type <mediaList> media_list
%type <mediaList> maybe_media_list
%type <mediaQuery> media_query
%type <mediaQueryRestrictor> maybe_media_restrictor
%type <valueList> maybe_media_value
%type <mediaQueryExp> media_query_exp
%type <mediaQueryExpList> media_query_exp_list
%type <mediaQueryExpList> maybe_and_media_query_exp_list

%type <ruleList> ruleset_list

%type <prop_id> property

%type <selector> specifier
%type <selector> specifier_list
%type <selector> simple_selector
%type <selector> simple_css3_selector
%type <selector> selector
%type <selectorList> selector_list
%type <selector> class
%type <selector> attrib
%type <selector> pseudo

%type <ok> declaration_block
%type <ok> declaration_list
%type <ok> declaration

%type <b> prio

%type <match> match
%type <val> unary_operator
%type <tok> operator

%type <valueList> expr
%type <value> term
%type <value> unary_term
%type <value> function

%type <element> element_name

%type <attribute> attrib_id

%%

stylesheet:
    maybe_space maybe_charset maybe_sgml import_list namespace_list rule_list
  | khtml_rule maybe_space
  | khtml_decls maybe_space
  | khtml_value maybe_space
  | khtml_mediaquery maybe_space
  | khtml_selectors maybe_space
  ;

ruleset_or_import_or_namespace:
    ruleset |
    import  |
    namespace_rule
;

khtml_rule:
    KHTML_RULE_SYM '{' maybe_space ruleset_or_import_or_namespace maybe_space '}' {
        CSSParser *p = static_cast<CSSParser *>(parser);
	p->rule = $4;
    }
;

khtml_decls:
    KHTML_DECLS_SYM declaration_block {
	/* can be empty */
    }
;

khtml_value:
    KHTML_VALUE_SYM '{' maybe_space expr '}' {
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( $4 ) {
	    p->valueList = $4;
#ifdef CSS_DEBUG
	    qDebug() << "   got property for " << p->id <<
		(p->important?" important":"");
	    bool ok =
#endif
		p->parseValue( p->id, p->important );
#ifdef CSS_DEBUG
	    if ( !ok )
		qDebug() << "     couldn't parse value!";
#endif
	}
#ifdef CSS_DEBUG
	else
	    qDebug() << "     no value found!";
#endif
	delete p->valueList;
	p->valueList = 0;
    }
;

khtml_selectors:
	KHTML_SELECTORS_SYM '{' maybe_space selector_list '}' {
		CSSParser *p = static_cast<CSSParser *>(parser);
		if ($4) {
			p->selectors = *$4;
			delete $4;
			$4 = 0;
		} else
			p->selectors.clear(); // parse error
	}
;

khtml_mediaquery:
     KHTML_MEDIAQUERY_SYM S maybe_space media_query '}' {
         CSSParser *p = static_cast<CSSParser *>(parser);
         p->mediaQuery = $4;
     }
;

maybe_plus:
    /* empty */
  | '+'
  ;

maybe_space:
    /* empty */ %prec REDUCE
  | maybe_space S
  ;

maybe_sgml:
    /* empty */
  | maybe_sgml SGML_CD
  | maybe_sgml S
  ;

maybe_charset:
   /* empty */
  | charset
 ;
charset:
  CHARSET_SYM maybe_space STRING maybe_space ';' {
#ifdef CSS_DEBUG
     qDebug() << "charset rule: " << qString($3);
#endif
     CSSParser* p = static_cast<CSSParser*>(parser);
     if (p->styleElement && p->styleElement->isCSSStyleSheet()) {
         p->styleElement->append( new CSSCharsetRuleImpl(p->styleElement, domString($3)) );
     }
 }
  | CHARSET_SYM error invalid_block {
 }
  | CHARSET_SYM error ';' {
 }
 ;

import_list:
 /* empty */
 | import_list import maybe_sgml {
     CSSParser *p = static_cast<CSSParser *>(parser);
     if ( $2 && p->styleElement && p->styleElement->isCSSStyleSheet() ) {
	 p->styleElement->append( $2 );
     } else {
	 delete $2;
     }
 }
 ;

import:
    IMPORT_SYM maybe_space string_or_uri maybe_space maybe_media_list ';' {
#ifdef CSS_DEBUG
	qDebug() << "@import: " << qString($3);
#endif
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( $5 && p->styleElement && p->styleElement->isCSSStyleSheet() )
	    $$ = new CSSImportRuleImpl( p->styleElement, domString($3), $5 );
	else
	    $$ = 0;
    }
  | IMPORT_SYM error invalid_block {
        $$ = 0;
    }
  | IMPORT_SYM error ';' {
        $$ = 0;
    }
  ;

namespace_list:
    /* empty */ %prec REDUCE
  | namespace_list namespace maybe_sgml
;

namespace_rule:
NAMESPACE_SYM maybe_space maybe_ns_prefix string_or_uri {
#ifdef CSS_DEBUG
    qDebug() << "@namespace: " << qString($3) << qString($4);
#endif
    CSSParser *p = static_cast<CSSParser *>(parser);
    $$ = new CSSNamespaceRuleImpl(p->styleElement, domString($3), domString($4));
 }
    ;

namespace: 
namespace_rule maybe_space ';' {
    CSSParser *p = static_cast<CSSParser *>(parser);
    if (p->styleElement && p->styleElement->isCSSStyleSheet())
	static_cast<CSSStyleSheetImpl*>(p->styleElement)->appendNamespaceRule
                    (static_cast<CSSNamespaceRuleImpl*>($1)); 
                            // can't use ->append since it 
                            // wouldn't keep track of dirtiness
 }
| NAMESPACE_SYM error invalid_block
| NAMESPACE_SYM error ';'
    ;

maybe_ns_prefix:
/* empty */ { $$.string = 0; }
| IDENT S { $$ = $1; }
  ;

rule_list:
   /* empty */
 | rule_list rule maybe_sgml {
     CSSParser *p = static_cast<CSSParser *>(parser);
     if ( $2 && p->styleElement && p->styleElement->isCSSStyleSheet() ) {
	 p->styleElement->append( $2 );
     } else {
	 delete $2;
     }
 }
    ;

rule:
    ruleset
  | media
  | page
  | font_face
  | invalid_rule
  | invalid_at
  | import error { delete $1; $$ = 0; }
    ;

safe_ruleset:
    ruleset
  | invalid_rule
  | invalid_at
  | import error { delete $1; $$ = 0; }
    ;

string_or_uri:
    STRING
  | URI
    ;
 
media_feature:
    IDENT maybe_space {
        $$ = $1;
    }
    ;

maybe_media_value:
    /*empty*/ {
        $$ = 0;
    }
    | ':' maybe_space expr maybe_space {
        $$ = $3;
    }
    ;

media_query_exp:
    '(' maybe_space media_feature maybe_space maybe_media_value ')' maybe_space {
        $$ = new khtml::MediaQueryExp(domString($3).lower(), $5);
    }
    ;

media_query_exp_list:
    media_query_exp {
      $$ =  new QList<khtml::MediaQueryExp*>;
      $$->append($1);
    }
    | media_query_exp_list maybe_space MEDIA_AND maybe_space media_query_exp {
      $$ = $1;
      $$->append($5);
    }
    ;

maybe_and_media_query_exp_list:
    /*empty*/ {
        $$ = new QList<khtml::MediaQueryExp*>;
    }
    | MEDIA_AND maybe_space media_query_exp_list {
        $$ = $3;
    }
    ;

maybe_media_restrictor:
    /*empty*/ {
        $$ = khtml::MediaQuery::None;
    }
    | MEDIA_ONLY {
        $$ = khtml::MediaQuery::Only;
    }
    | MEDIA_NOT {
        $$ = khtml::MediaQuery::Not;
    }
    ;

media_query:
    media_query_exp_list {
        $$ = new khtml::MediaQuery(khtml::MediaQuery::None, "all", $1);
    }                    
    | maybe_media_restrictor maybe_space medium maybe_and_media_query_exp_list {
        $$ = new khtml::MediaQuery($1, domString($3).lower(), $4);
    }
    ;

maybe_media_list:
    /* empty */ {
	$$ = new MediaListImpl();
    }
    | media_list
;


media_list:
    media_query {
        $$ = new MediaListImpl();
        $$->appendMediaQuery($1);
    }
    | media_list ',' maybe_space media_query {
	$$ = $1;
	if ($$)
	    $$->appendMediaQuery( $4 );
    }
    | media_list error {
       delete $1;
       $$ = 0;
    }
    ;

media:
    MEDIA_SYM maybe_space media_list '{' maybe_space ruleset_list '}' {
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( $3 && $6 &&
	     p->styleElement && p->styleElement->isCSSStyleSheet() ) {
	    $$ = new CSSMediaRuleImpl( static_cast<CSSStyleSheetImpl*>(p->styleElement), $3, $6 );
	} else {
	    $$ = 0;
	    delete $3;
	    delete $6;
	}
    }
    | MEDIA_SYM maybe_space '{' maybe_space ruleset_list '}' {
        CSSParser *p = static_cast<CSSParser *>(parser);
        if ($5 && p->styleElement && p->styleElement->isCSSStyleSheet() ) {
            $$ = new CSSMediaRuleImpl( static_cast<CSSStyleSheetImpl*>(p->styleElement), 0, $5);
        } else {
            $$ = 0;
            delete $5;
        }
    }
  ;

ruleset_list:
    /* empty */ { $$ = 0; }
  | ruleset_list safe_ruleset maybe_space {
      $$ = $1;
      if ( $2 ) {
	  if ( !$$ ) $$ = new CSSRuleListImpl();
	  $$->append( $2 );
      }
  }
    ;

medium:
  IDENT maybe_space {
      $$ = $1;
  }
  ;

/*
page:
    PAGE_SYM maybe_space IDENT? pseudo_page? maybe_space
    '{' maybe_space declaration [ ';' maybe_space declaration ]* '}' maybe_space
  ;

pseudo_page
  : ':' IDENT
  ;

*/

page:
    PAGE_SYM error invalid_block {
      $$ = 0;
    }
  | PAGE_SYM error ';' {
      $$ = 0;
    }
    ;

font_face:
    FONT_FACE_SYM maybe_space declaration_block {
      CSSParser *p = static_cast<CSSParser *>(parser);
      CSSFontFaceRuleImpl *rule = new CSSFontFaceRuleImpl( p->styleElement );
      CSSStyleDeclarationImpl *decl = p->createFontFaceStyleDeclaration( rule );
      rule->setDeclaration(decl);
      $$ = rule;
    }
  | FONT_FACE_SYM error invalid_block {
      $$ = 0;
    }
  | FONT_FACE_SYM error ';' {
      $$ = 0;
    }
;

combinator:
    '+' maybe_space { $$ = CSSSelector::DirectAdjacent; }
  | '~' maybe_space { $$ = CSSSelector::IndirectAdjacent; }
  | '>' maybe_space { $$ = CSSSelector::Child; }
  ;

unary_operator:
    '-' { $$ = -1; }
  | '+' { $$ = 1; }
  ;

ruleset:
    selector_list declaration_block {
#ifdef CSS_DEBUG
	qDebug() << "got ruleset" << endl << "  selector:";
#endif
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( $1  ) {
	    CSSStyleRuleImpl *rule = new CSSStyleRuleImpl( p->styleElement );
	    CSSStyleDeclarationImpl *decl = p->createStyleDeclaration( rule );
	    rule->setSelector( $1 );
	    rule->setDeclaration(decl);
	    $$ = rule;
	} else {
	    $$ = 0;
	    if ($1) qDeleteAll(*$1);
	    delete $1;
	    $1 = 0;
	    p->clearProperties();
	}
    }
  ;

selector_list:
    selector %prec REDUCE {
	if ( $1 ) {
	    $$ = new QList<CSSSelector*>;
#ifdef CSS_DEBUG
	    qDebug() << "   got simple selector:";
	    $1->print();
#endif
	    $$->append( $1 );
	    khtml::CSSStyleSelector::precomputeAttributeDependencies(static_cast<CSSParser *>(parser)->document(), $1);
	} else {
	    $$ = 0;
	}
    }
    | selector_list ',' maybe_space selector %prec REDUCE {
	if ( $1 && $4 ) {
	    $$ = $1;
	    $$->append( $4 );
	    khtml::CSSStyleSelector::precomputeAttributeDependencies(static_cast<CSSParser *>(parser)->document(), $4);
#ifdef CSS_DEBUG
	    qDebug() << "   got simple selector:";
	    $4->print();
#endif
	} else {
            if ($1) qDeleteAll(*$1);
	    delete $1;
	    $1=0;

	    delete $4;
	    $$ = 0;
	}
    }
  | selector_list error {
        if ($1) qDeleteAll(*$1);
	delete $1;
	$1 = 0;
	$$ = 0;
    }
   ;


selector:
    simple_selector {
	$$ = $1;
    }
    | selector S {
        $$ = $1;
    }
    | selector S simple_selector {
        if ( !$1 || !$3 ) {
	    delete $1;
	    delete $3;
	    $$ = 0;
        } else {
            $$ = $3;
            CSSSelector *end = $$;
            while( end->tagHistory )
                end = end->tagHistory;
            end->relation = CSSSelector::Descendant;
            end->tagHistory = $1;
        }
    }
    | selector combinator simple_selector {
	if ( !$1 || !$3 ) {
	    delete $1;
	    delete $3;
	    $$ = 0;
	} else {
	    $$ = $3;
	    CSSSelector *end = $3;
	    while( end->tagHistory )
		end = end->tagHistory;
	    end->relation = $2;
	    end->tagHistory = $1;
	}
    }
    | selector error {
	delete $1;
	$$ = 0;
    }
    ;

namespace_selector:
    /* empty */ '|' { $$.string = 0; $$.length = 0; }
    | '*' '|' { static unsigned short star = '*'; $$.string = &star; $$.length = 1; }
    | IDENT '|' { $$ = $1; }
;

simple_selector:
    element_name {
	$$ = new CSSSelector();
        $$->tagLocalName = LocalName::fromId(localNamePart($1));
        $$->tagNamespace = NamespaceName::fromId(namespacePart($1));
    }
    | element_name specifier_list {
	$$ = $2;
        if ( $$ ) {
            $$->tagLocalName = LocalName::fromId(localNamePart($1));
            $$->tagNamespace = NamespaceName::fromId(namespacePart($1));
        }
    }
    | specifier_list {
	$$ = $1;
        if ( $$ ) {
            $$->tagLocalName = LocalName::fromId(anyLocalName);
            $$->tagNamespace = NamespaceName::fromId(static_cast<CSSParser*>(parser)->defaultNamespace());
        }
    }
    | namespace_selector element_name {
        $$ = new CSSSelector();
        $$->tagLocalName = LocalName::fromId(localNamePart($2));
        $$->tagNamespace = NamespaceName::fromId(namespacePart($2));
	CSSParser *p = static_cast<CSSParser *>(parser);
        if (p->styleElement && p->styleElement->isCSSStyleSheet())
            static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace($$->tagNamespace, domString($1));
    }
    | namespace_selector element_name specifier_list {
        $$ = $3;
        if ($$) {
            $$->tagLocalName = LocalName::fromId(localNamePart($2));
            $$->tagNamespace = NamespaceName::fromId(namespacePart($2));
            CSSParser *p = static_cast<CSSParser *>(parser);
            if (p->styleElement && p->styleElement->isCSSStyleSheet())
                static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace($$->tagNamespace, domString($1));
        }
    }
    | namespace_selector specifier_list {
        $$ = $2;
        if ($$) {
            $$->tagLocalName = LocalName::fromId(anyLocalName);
            $$->tagNamespace = NamespaceName::fromId(anyNamespace);
            CSSParser *p = static_cast<CSSParser *>(parser);
            if (p->styleElement && p->styleElement->isCSSStyleSheet())
                static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace($$->tagNamespace, domString($1));
        }
    }
  ;

simple_css3_selector:
    element_name {
	$$ = new CSSSelector();
        $$->tagLocalName = LocalName::fromId(localNamePart($1));
        $$->tagNamespace = NamespaceName::fromId(namespacePart($1));
    }
    | specifier {
	$$ = $1;
        if ( $$ ) {
            $$->tagLocalName = LocalName::fromId(anyLocalName);
            $$->tagNamespace = NamespaceName::fromId(static_cast<CSSParser*>(parser)->defaultNamespace());
        }
    }
    | namespace_selector element_name {
        $$ = new CSSSelector();
        $$->tagLocalName = LocalName::fromId(localNamePart($2));
        $$->tagNamespace = NamespaceName::fromId(namespacePart($2));
	CSSParser *p = static_cast<CSSParser *>(parser);
        if (p->styleElement && p->styleElement->isCSSStyleSheet())
            static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace($$->tagNamespace, domString($1));
    }
    | namespace_selector specifier {
        $$ = $2;
        if ($$) {
            $$->tagLocalName = LocalName::fromId(anyLocalName);
            $$->tagNamespace = NamespaceName::fromId(anyNamespace);
            CSSParser *p = static_cast<CSSParser *>(parser);
            if (p->styleElement && p->styleElement->isCSSStyleSheet())
                static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace($$->tagNamespace, domString($1));
        }
    }
  ;

element_name:
    IDENT {
      CSSParser *p = static_cast<CSSParser *>(parser);
      $$ = makeId(p->defaultNamespace(), p->getLocalNameId(domString($1)));
    }
    | '*' {
	$$ = makeId(static_cast<CSSParser*>(parser)->defaultNamespace(), anyLocalName);
    }
  ;

specifier_list:
    specifier {
	$$ = $1;
    }
    | specifier_list specifier {
	$$ = $1;
	if ( $$ ) {
            CSSSelector *end = $1;
            while( end->tagHistory )
                end = end->tagHistory;
            end->relation = CSSSelector::SubSelector;
            end->tagHistory = $2;
	}
    }
    | specifier_list error {
	delete $1;
	$$ = 0;
    }
;

specifier:
    HASH {
        CSSParser *p = static_cast<CSSParser *>(parser);

	$$ = new CSSSelector();
	$$->match = CSSSelector::Id;
        $$->attrLocalName = LocalName::fromId(localNamePart(ATTR_ID));
        $$->attrNamespace = NamespaceName::fromId(namespacePart(ATTR_ID));

        bool caseSensitive = p->document()->htmlMode() == DocumentImpl::XHtml || !p->document()->inCompatMode();
        if (caseSensitive)
            $$->value = domString($1);
        else
            $$->value = domString($1).lower();
    }
  | class
  | attrib
  | pseudo
    ;

class:
    '.' IDENT {
        CSSParser *p = static_cast<CSSParser *>(parser);

	$$ = new CSSSelector();
	$$->match = CSSSelector::Class;
        $$->attrLocalName = LocalName::fromId(localNamePart(ATTR_CLASS));
        $$->attrNamespace = NamespaceName::fromId(namespacePart(ATTR_CLASS));

        bool caseSensitive = p->document()->htmlMode() == DocumentImpl::XHtml || !p->document()->inCompatMode();
        if (caseSensitive)
            $$->value = domString($2);
        else
            $$->value = domString($2).lower();
    }
  ;

attrib_id:
    IDENT maybe_space {
      CSSParser *p = static_cast<CSSParser *>(parser);
      $$ = makeId(emptyNamespace, p->getLocalNameId(domString($1)));
    }
    ;

attrib:
    '[' maybe_space attrib_id ']' {
	$$ = new CSSSelector();
        $$->attrLocalName = LocalName::fromId(localNamePart($3));
        $$->attrNamespace = NamespaceName::fromId(namespacePart($3));
	$$->match = CSSSelector::Set;
    }
    | '[' maybe_space attrib_id match maybe_space ident_or_string maybe_space ']' {
	$$ = new CSSSelector();
        $$->attrLocalName = LocalName::fromId(localNamePart($3));
        $$->attrNamespace = NamespaceName::fromId(namespacePart($3));
	$$->match = $4;
	$$->value = domString($6);
    }
    | '[' maybe_space namespace_selector attrib_id ']' {
        $$ = new CSSSelector();
        $$->attrLocalName = LocalName::fromId(localNamePart($4));
        $$->attrNamespace = NamespaceName::fromId(namespacePart($4));
        $$->match = CSSSelector::Set;
        CSSParser *p = static_cast<CSSParser *>(parser);
        if (p->styleElement && p->styleElement->isCSSStyleSheet())
            static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace($$->attrNamespace, domString($3));
    }
    | '[' maybe_space namespace_selector attrib_id match maybe_space ident_or_string maybe_space ']' {
        $$ = new CSSSelector();
        $$->attrLocalName = LocalName::fromId(localNamePart($4));
        $$->attrNamespace = NamespaceName::fromId(namespacePart($4));
        $$->match = (CSSSelector::Match)$5;
        $$->value = domString($7);
        CSSParser *p = static_cast<CSSParser *>(parser);
        if (p->styleElement && p->styleElement->isCSSStyleSheet())
            static_cast<CSSStyleSheetImpl*>(p->styleElement)->determineNamespace($$->attrNamespace, domString($3));
   }
  ;

match:
    '=' {
	$$ = CSSSelector::Exact;
    }
    | INCLUDES {
	$$ = CSSSelector::List;
    }
    | DASHMATCH {
	$$ = CSSSelector::Hyphen;
    }
    | BEGINSWITH {
	$$ = CSSSelector::Begin;
    }
    | ENDSWITH {
	$$ = CSSSelector::End;
    }
    | CONTAINS {
	$$ = CSSSelector::Contain;
    }
    ;

ident_or_string:
    IDENT
  | STRING
    ;

pseudo:
    ':' IDENT {
	$$ = new CSSSelector();
	$$->match = CSSSelector::PseudoClass;
	$$->value = domString($2);
    }
    |
    ':' ':' IDENT {
	$$ = new CSSSelector();
	$$->match = CSSSelector::PseudoElement;
        $$->value = domString($3);
    }
    // used by :nth-*
    | ':' FUNCTION maybe_space NTH maybe_space ')' {
        $$ = new CSSSelector();
        $$->match = CSSSelector::PseudoClass;
        $$->string_arg = domString($4);
        $$->value = domString($2);
    }
    // used by :nth-*
    | ':' FUNCTION maybe_space maybe_plus INTEGER maybe_space ')' {
        $$ = new CSSSelector();
        $$->match = CSSSelector::PseudoClass;
        $$->string_arg = QString::number($5);
        $$->value = domString($2);
    }
    // used by :nth-* and :lang
    | ':' FUNCTION maybe_space IDENT maybe_space ')' {
        $$ = new CSSSelector();
        $$->match = CSSSelector::PseudoClass;
        $$->string_arg = domString($4);
        $$->value = domString($2);
    }
    // used by :contains and :lang
    | ':' FUNCTION maybe_space STRING maybe_space ')' {
        $$ = new CSSSelector();
        $$->match = CSSSelector::PseudoClass;
        $$->string_arg = domString($4);
        $$->value = domString($2);
    }
    // used only by :not
    | ':' NOTFUNCTION maybe_space simple_css3_selector maybe_space ')' {
        $$ = new CSSSelector();
        $$->match = CSSSelector::PseudoClass;
        $$->simpleSelector = $4;
        $$->value = domString($2);
    }
  ;

declaration_block:
    '{' maybe_space declaration '}' {
	$$ = $3;
    }
    | '{' maybe_space error '}' {
	$$ = false;
    }
    | '{' maybe_space declaration_list '}' {
	$$ = $3;
    }
    | '{' maybe_space declaration_list declaration '}' {
	$$ = $3;
	if ( $4 )
	    $$ = $4;
    }
    | '{' maybe_space declaration_list error '}' {
	$$ = $3;
    }
    ;

declaration_list:
    declaration ';' maybe_space {
	$$ = $1;
    }
    |
    error ';' maybe_space {
        $$ = false;
    }
    | declaration_list declaration ';' maybe_space {
	$$ = $1;
	if ( $2 )
	    $$ = $2;
    }
    | declaration_list error ';' maybe_space {
        $$ = $1;
    }
    ;

declaration:
    property ':' maybe_space expr prio {
	$$ = false;
	CSSParser *p = static_cast<CSSParser *>(parser);
	if ( $1 && $4 ) {
	    p->valueList = $4;
#ifdef CSS_DEBUG
	    qDebug() << "   got property: " << $1 <<
		($5?" important":"");
#endif
	        bool ok = p->parseValue( $1, $5 );
                if ( ok )
		    $$ = ok;
#ifdef CSS_DEBUG
	        else
		    qDebug() << "     couldn't parse value!";
#endif
	} else {
            delete $4;
        }
	delete p->valueList;
	p->valueList = 0;
    }
    | error invalid_block {
        $$ = false;
    }
  ;

property:
    IDENT maybe_space {
	QString str = qString($1);
	str = str.toLower();
	if (str.length() >= sMinCompatPropLen && str[0] == '-' && str[1] != 'k') {
	    // vendor extension. Lets try and convert a selected few
	    if (!sCompatibleProperties)
	        initCompatibleProperties();
            QHash<QString,int>::iterator it = sCompatibleProperties->find( str );
            if (it != sCompatibleProperties->end()) {
                str = "-khtml" + str.mid( it.value() );
              
                $$ = getPropertyID( str.toLatin1(), str.length() );
            } else {
                $$ = 0;
            }
	} else {
	    $$ = getPropertyID( str.toLatin1(), str.length() );
        }
    }
  ;

prio:
    IMPORTANT_SYM maybe_space { $$ = true; }
    | /* empty */ { $$ = false; }
  ;

expr:
    term {
	$$ = new ValueList;
	$$->addValue( $1 );
    }
    | expr operator term {
	$$ = $1;
	if ( $$ ) {
	    if ( $2 ) {
		Value v;
		v.id = 0;
		v.unit = Value::Operator;
		v.iValue = $2;
		$$->addValue( v );
	    }
	    $$->addValue( $3 );
	}
    }
  ;

operator:
    '/' maybe_space {
	$$ = '/';
    }
  | ',' maybe_space {
	$$ = ',';
    }
  | /* empty */ {
        $$ = 0;
  }
  ;

term:
  unary_term { $$ = $1; }
   | unary_operator unary_term { $$ = $2; $$.fValue *= $1; }
  /* DIMEN is an unary_term, but since we store the string we must not modify fValue */
  | DIMEN maybe_space { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_DIMENSION; }
  | STRING maybe_space { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_STRING; }
  | IDENT maybe_space {
      QString str = qString( $1 );
      $$.id = getValueID( str.toLower().toLatin1(), str.length() );
      $$.unit = CSSPrimitiveValue::CSS_IDENT;
      $$.string = $1;
  }
  | URI maybe_space { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_URI; }
  | UNICODERANGE maybe_space { $$.id = 0; $$.iValue = 0; $$.unit = CSSPrimitiveValue::CSS_UNKNOWN;/* ### */ }
  | hexcolor { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_RGBCOLOR; }
/* ### according to the specs a function can have a unary_operator in front. I know no case where this makes sense */
  | function {
      $$ = $1;
  }
  ;

unary_term:
  INTEGER maybe_space { $$.id = 0; $$.isInt = true; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_NUMBER; }
  | FLOAT maybe_space { $$.id = 0; $$.isInt = false; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_NUMBER; }
  | PERCENTAGE maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_PERCENTAGE; }
  | PXS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_PX; }
  | CMS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_CM; }
  | MMS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_MM; }
  | INS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_IN; }
  | PTS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_PT; }
  | PCS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_PC; }
  | DEGS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_DEG; }
  | RADS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_RAD; }
  | GRADS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_GRAD; }
  | MSECS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_MS; }
  | SECS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_S; }
  | HERZ maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_HZ; }
  | KHERZ maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_KHZ; }
  | EMS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_EMS; }
  | QEMS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = Value::Q_EMS; }
  | EXS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_EXS; }
  | DPI maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_DPI; }
  | DPCM maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_DPCM; }
    ;


function:
  FUNCTION maybe_space expr ')' maybe_space {
      Function *f = new Function;
      f->name = $1;
      f->args = $3;
      $$.id = 0;
      $$.unit = Value::Function;
      $$.function = f;
  }
  | FUNCTION maybe_space error {
      Function *f = new Function;
      f->name = $1;
      f->args = 0;
      $$.id = 0;
      $$.unit = Value::Function;
      $$.function = f;
  }

  ;
/*
 * There is a constraint on the color that it must
 * have either 3 or 6 hex-digits (i.e., [0-9a-fA-F])
 * after the "#"; e.g., "#000" is OK, but "#abcd" is not.
 */
hexcolor:
  HEXCOLOR maybe_space { $$ = $1; }
  ;


/* error handling rules */

invalid_at:
    '@' error invalid_block {
	$$ = 0;
#ifdef CSS_DEBUG
	qDebug() << "skipped invalid @-rule";
#endif
    }
  | '@' error ';' {
	$$ = 0;
#ifdef CSS_DEBUG
	qDebug() << "skipped invalid @-rule";
#endif
    }
    ;

invalid_rule:
    error invalid_block {
	$$ = 0;
#ifdef CSS_DEBUG
	qDebug() << "skipped invalid rule";
#endif
    }
/*
  Seems like the two rules below are trying too much and violating
  http://www.hixie.ch/tests/evil/mixed/csserrorhandling.html

  | error ';' {
	$$ = 0;
#ifdef CSS_DEBUG
	qDebug() << "skipped invalid rule";
#endif
    }
  | error '}' {
	$$ = 0;
#ifdef CSS_DEBUG
	qDebug() << "skipped invalid rule";
#endif
    }
*/
    ;

invalid_block:
    '{' error invalid_block_list error '}'
  | '{' error '}'
    ;

invalid_block_list:
    invalid_block
  | invalid_block_list error invalid_block
;

%%

