/*
 * tokenizer_tester.cc - Copyright 2005 Frerich Raabe <raabe@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "expression.h"
#include "step.h"         //
#include "path.h"         //
#include "predicate.h"    // Order dependency.
#include "parser.h"       //

#include <QStringList>
#include <QtDebug>

extern int xpathyylex();
extern void initTokenizer( QString s );

QString tokenAsString( yytokentype token )
{
	switch ( token ) {
		case MULOP: return QString( "MULOP[%1]" ).arg( xpathyylval.num );
		case MINUS: return "MINUS";
		case PLUS: return "PLUS";
		case RELOP: return QString( "RELOP[%1]" ).arg( xpathyylval.num );
		case EQOP: return QString( "EQOP[%1]" ).arg( xpathyylval.num );
		case AND: return "AND";
		case OR: return "OR";
		case AXISNAME: return QString( "AXISNAME['%1']" ).arg( Step::axisAsString( xpathyylval.axisType ) );
		case NODETYPE: return QString( "NODETYPE['%1']" ).arg( *xpathyylval.str );
		case PI: return "PI";
		case FUNCTIONNAME: return QString( "FUNCTIONNAME['%1']" ).arg( *xpathyylval.str );
		case LITERAL: return QString( "LITERAL['%1']" ).arg( *xpathyylval.str );
		case VARIABLEREFERENCE: return QString( "VARIABLEREFERENCE['%1']" ).arg( *xpathyylval.str );
		case NAMETEST: return QString( "NAMETEST['%1']" ).arg( *xpathyylval.str );
		case NUMBER: return QString( "NUMBER['%1']" ).arg( *xpathyylval.str );
		case DOTDOT: return "DOTDOT";
		case SLASHSLASH: return "SLASHSLASH";
		case ERROR:  return "ERROR";
	}

	return QString( "'%1'" ).arg( char( token ) );
}

QString getTokenString( const QString &s )
{
	initTokenizer( s );

	QStringList tokenStrings;
	int token = xpathyylex();
	while ( token != 0 ) {
		tokenStrings << tokenAsString( ( yytokentype )token );
		token = xpathyylex();
	};

	return tokenStrings.join( " " );
}

void check( const QString &statement, const QString &expected )
{
	QString result = getTokenString( statement );
	if ( result != expected ) {
		qDebug() << "ERROR: failed to tokenizer " << statement << " as expected";
		qDebug() << "Expected: " << expected;
		qDebug() << "Got     : " << result;
		exit( 1 );
	}
}

int main()
{
	check( "child 		::book",
	       "AXISNAME['child'] NAMETEST['book']" );
	check( "/this/and/that",
	       "'/' NAMETEST['this'] '/' NAMETEST['and'] '/' NAMETEST['that']" );
	check( "self::self/book[@and=\"and\" and true( \t\t)   ]",
	       "AXISNAME['self'] NAMETEST['self'] '/' NAMETEST['book'] '[' '@' NAMETEST['and'] EQOP[1] LITERAL['and'] AND FUNCTIONNAME['true'] '(' ')' ']'" );
	check( "123 . .43 31. 3131.22 2.2.2",
	       "NUMBER['123'] '.' NUMBER['.43'] NUMBER['31.'] NUMBER['3131.22'] NUMBER['2.2'] NUMBER['.2']" );
	check( "/or/and[\"and\" and 'or\"' ]",
	       "'/' NAMETEST['or'] '/' NAMETEST['and'] '[' LITERAL['and'] AND LITERAL['or\"'] ']'" );
	check( "self	::node	(   )[true	(   )]",
	       "AXISNAME['self'] NODETYPE['node'] '(' ')' '[' FUNCTIONNAME['true'] '(' ')' ']'" );

	const QChar alpha( 0x03B1 );
	check( QString( "self::" ) + alpha + alpha + alpha,
	       QString( "AXISNAME['self'] NAMETEST['" ) + alpha + alpha + alpha + "']" );

	check( "this[substring-after(\"Name=Joe\", \"=\" )=\"Joe\"]",
	       "NAMETEST['this'] '[' FUNCTIONNAME['substring-after'] '(' LITERAL['Name=Joe'] ',' LITERAL['='] ')' EQOP[1] LITERAL['Joe'] ']'" );
	check( "child::processing-instruction()",
	       "AXISNAME['child'] PI '(' ')'" );
	check( "child::processing-instruction(\"yadda\")",
	       "AXISNAME['child'] PI '(' LITERAL['yadda'] ')'" );
	check( "*",
	       "NAMETEST['*']" );
	check( "comment() | text() | processing-instruction() | node()",
	       "NODETYPE['comment'] '(' ')' '|' NODETYPE['text'] '(' ')' '|' PI '(' ')' '|' NODETYPE['node'] '(' ')'" );
	qDebug( "All OK!" );
}

