/*
 * parser_tester.cc - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#include "parsedstatement.h"

#include <QtXml/QDomDocument>
#include <QtDebug>

QString indentedTree( const QString &markup )
{
	QDomDocument doc;
	doc.setContent( markup );
	return doc.toString( 2 );
}

void check( const QString &statement, const QString &expected )
{
	QString result = ParsedStatement( statement ).dump();
	if ( indentedTree( result ) != indentedTree( expected ) ) {
		qDebug() << "ERROR! Failed to parse '" << statement << "' as expected!";
		qDebug() << "Expected:";
		qDebug() << indentedTree( expected );
		qDebug() << "Got:";
		qDebug() << indentedTree( result );
		exit( 1 );
	}
}

extern int xpathyydebug;
int main()
{
//	xpathyydebug=1;
	check( "/book",
	       "<locationpath absolute=\"true\"><step axis=\"child\" nodetest=\"book\"></step></locationpath>" );
	check( "book/self::chapter/@id",
	       "<locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"book\"></step><step axis=\"self\" nodetest=\"chapter\"></step><step axis=\"attribute\" nodetest=\"id\"></step></locationpath>" );
	check( "child[\"foo\"=\"bar\" ]",
	       "<locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"child\"><predicate><relationEQ><operand><string>foo</string></operand><operand><string>bar</string></operand></relationEQ></predicate></step></locationpath>" );
	check( "//parent[position() < last()-1]",
	       "<locationpath absolute=\"true\"><step axis=\"descendant-or-self\" nodetest=\"node()\"></step><step axis=\"child\" nodetest=\"parent\"><predicate><relationLT><operand><function name=\"position\"/></operand><operand><subtraction><operand><function name=\"last\"/></operand><operand><number>1</number></operand></subtraction></predicate></step></locationpath>" );
	check( "/foo[true()][substring-after(\"First name=Joe\", \"=\") != \"Mary\"]",
	       "<locationpath absolute=\"true\"><step axis=\"child\" nodetest=\"foo\"><predicate><function name=\"true\"/></predicate><predicate><relationNE><operand><function name=\"substring-after\"><operand><string>First name=Joe</string></operand><operand><string>=</string></operand></function></operand><operand><string>Mary</string></operand></relationNE></predicate></step></locationpath>" );
	check( "/foo[true()][substring-after(\"First name=Joe\", \"=\") != \"Mary\"]/child[false()][substring-before(\"First name=Joe\", '=') != 'Mary']",
	       "<locationpath absolute=\"true\"><step axis=\"child\" nodetest=\"foo\"><predicate><function name=\"true\"/></predicate><predicate><relationNE><operand><function name=\"substring-after\"><operand><string>First name=Joe</string></operand><operand><string>=</string></operand></function></operand><operand><string>Mary</string></operand></relationNE></predicate></step> <step axis=\"child\" nodetest=\"child\"><predicate><function name=\"false\"/></predicate><predicate><relationNE><operand><function name=\"substring-before\"><operand><string>First name=Joe</string></operand><operand><string>=</string></operand></function></operand><operand><string>Mary</string></operand></relationNE></predicate></step></locationpath>" );
	check( "following::this[@that]",
	       "<locationpath absolute=\"false\"><step axis=\"following\" nodetest=\"this\"><predicate><locationpath absolute=\"false\"><step axis=\"attribute\" nodetest=\"that\"/></locationpath></predicate></step></locationpath>" );
	check( "descendant::para[@type != $type]",
	       "<locationpath absolute=\"false\"><step axis=\"descendant\" nodetest=\"para\"><predicate><relationNE><operand><locationpath absolute=\"false\"><step axis=\"attribute\" nodetest=\"type\"></step></locationpath></operand><operand><variablereference name=\"type\"/></operand></relationNE></predicate></step></locationpath>" );
	check( "child::processing-instruction()",
	       "<locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"processing-instruction\"/></locationpath>" );
	check( "child::processing-instruction(\"someParameter\")",
	       "<locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"processing-instruction someParameter\"/></locationpath>" );
	check( "*",
	       "<locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"*\"/></locationpath>" );
	check( "comment()",
	       "<locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"comment()\"/></locationpath>" );
	check( "comment() | text() | processing-instruction() | node()",
	       "<union><operand><union><operand><union><operand><locationpath absolute=\"false\"><step nodetest=\"comment()\" axis=\"child\"/></locationpath></operand><operand><locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"text()\"/></locationpath></operand></union></operand><operand><locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"processing-instruction\"/></locationpath></operand></union></operand><operand><locationpath absolute=\"false\"><step axis=\"child\" nodetest=\"node()\"/></locationpath></operand></union>" );
	qDebug( "All OK!" );
}

