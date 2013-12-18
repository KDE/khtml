/*
 * interpreter_tester.cpp - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#include "parsedstatement.h"

#include "XPathExceptionImpl.h"

#include "DocumentImpl.h"
#include "DOMStringImpl.h"
#include "KDOMParser.h"
#include "KDOMParserFactory.h"
#include "NamedAttrMapImpl.h"

#include "kdom.h"

#include <QApplication>
#include <QBuffer>
#include <QtDebug>

using namespace KDOM;

void check( DocumentImpl *doc, const QString &statement, const QString &expected )
{
	ParsedStatement s( statement );
	Value result = s.evaluate( doc );
	if ( !result.isString() ) {
		qDebug( "ERROR: Query '%s' did not return a string!", statement.latin1() );
		exit( 1 );
	}

	QString string = result.toString();
	if ( string != expected ) {
		qDebug( "ERROR: Failed to interprete '%s' correctly!", statement.latin1() );
		qDebug( "Expected to get: %s", expected.latin1() );
		qDebug( "Got            : %s", string.latin1() );
		exit( 1 );
	}
}


void check( DocumentImpl *doc, const QString &statement, double expected )
{
	ParsedStatement s( statement );
	Value result = s.evaluate( doc );
	if ( !result.isNumber() ) {
		qDebug( "ERROR: Query '%s' did not return a number!", statement.latin1() );
		exit( 1 );
	}

	double number = result.toNumber();
	if ( number != expected ) {
		qDebug( "ERROR: Failed to interprete '%s' correctly!", statement.latin1() );
		qDebug( "Expected to get: %f", expected );
		qDebug( "Got            : %f", number );
		exit( 1 );
	}
}

void check( DocumentImpl *doc, const QString &statement,
            const QStringList &idsOfExpectedMatches )
{
	ParsedStatement s( statement );
	Value result = s.evaluate( doc );
	if ( !result.isNodeset() ) {
		qDebug( "ERROR: Query '%s' did not return a nodeset!", statement.latin1() );
		exit( 1 );
	}

	QStringList idsOfResultMatches;

	DomNodeList nodes = result.toNodeset();
	foreach( NodeImpl *node, nodes ) {
		if ( node->nodeType() != ELEMENT_NODE ) {
			continue;
		}
		NodeImpl *idNode = 0;
		NamedAttrMapImpl *attrs = node->attributes( true /*read-only*/ );
		for ( unsigned long i = 0; i < attrs->length(); ++i ) {
			idNode = attrs->item( i );
			if ( idNode->nodeName()->string() == "id" ) {
				break;
			}
		}
		if ( !idNode ) {
			qDebug( "ERROR: Found match without id attribute!" );
			exit( 1 );
		}
		idsOfResultMatches.append( idNode->nodeValue()->string() );
	}

	bool failure = false;

	foreach( QString id, idsOfExpectedMatches ) {
		if ( !idsOfResultMatches.contains( id ) ) {
			failure = true;
			break;
		}
	}

	if ( !failure ) {
		foreach( QString id, idsOfResultMatches ) {
			if ( !idsOfExpectedMatches.contains( id ) ) {
				failure = true;
				break;
			}
		}
	}

	if ( failure ) {
		qDebug() << "ERROR: Failed to interprete '" << statement << "' correctly!";
		qDebug() << "Expected to match: " << idsOfExpectedMatches.join( "," );
		qDebug() << "Got matches      : " << idsOfResultMatches.join( "," );
		exit( 1 );
	}
}

int main( int argc, char **argv )
{
	QString bookMarkup =
	       "<book id=\"1\">"
	       "<abstract id=\"2\">"
	       "</abstract>"
	       "<chapter id=\"3\" title=\"Introduction\">"
	       "<para id=\"4\">Blah blah blah</para>"
	       "<para id=\"5\">Foo bar yoyodyne</para>"
	       "</chapter>"
	       "<chapter id=\"6\" title=\"TEST\" type=\"x\">"
	       "<para id=\"7\">My sister got bitten by a yak.</para>"
	       "</chapter>"
	       "<chapter id=\"8\" title=\"note\" type=\"y\">"
	       "<para id=\"9\">141,000</para>"
	       "</chapter>"
	       "<chapter id=\"10\" title=\"note\">"
	       "<para id=\"11\">Hell yeah, yaks are nice.</para>"
	       "</chapter>"
	       "</book>";

    QApplication::setApplicationName("interpreter_tester");

	QApplication app(argc, argv);

	// Parser::syncParse called blow deletes the given buffer, so we
	// cannot use QBuffer objects which live on the stack.
	QBuffer *buf = new QBuffer;
	buf->open( QBuffer::ReadWrite );
	buf->write( bookMarkup.toUtf8() );

	// I can't believe that I have to go through all these hoops to get
	// a DocumentImpl* out of a string with markup.
	Parser *parser = ParserFactory::self()->request( KURL(), 0, "qxml" );
	DocumentImpl *doc = parser->syncParse( buf );

	try {
	check( doc, "/book", QStringList() << "1" );
	check( doc, "/book/chapter", QStringList() << "3" << "6" << "8" );
	check( doc, "/book/chapter[@title=\"TEST\"]", QStringList() << "6" );
	check( doc, "/book/chapter[last()-1]", QStringList() << "8" );
	check( doc, "/book/chapter[contains(string(@title), \"tro\")]", QStringList() << "3" );
	check( doc, "//para", QStringList() << "4" << "5" << "7" << "9" );
	check( doc, "/book/chapter[position()=2]/following::*", QStringList() << "8" << "9" );
	check( doc, "//chapter[@title and @type]", QStringList() << "6" << "8" );
	check( doc, "/book/chapter[attribute::title = \"note\"][position() = 2]", QStringList() << "10" );
	check( doc, "count(//para)", 5.0 );
	check( doc, "string(/book/chapter/para[text() = \"Foo bar yoyodyne\" ])", QLatin1String( "Foo bar yoyodyne" ) );
	check( doc, "substring-before(/book/chapter/para[@id=\"9\" ], ',' )", QLatin1String( "141" ) );
	} catch ( XPath::XPathExceptionImpl *e ) {
		qDebug( "Caught XPath exception '%s'.", e->codeAsString().latin1() );
		delete e;
		return 1;
	}

	qDebug( "All OK!" );
}

