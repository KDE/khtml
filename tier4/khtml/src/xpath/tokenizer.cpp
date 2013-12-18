/*
 * tokenizer.cc - Copyright 2005 Maksim Orlovich <maksim@kde.org>
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
#include "tokenizer.h"

#include "xml/dom_stringimpl.h"
#include "xml/dom3_xpathimpl.h"
#include "dom/dom3_xpath.h"

#include <cstdio>

using namespace std;

using namespace DOM;
using namespace DOM::XPath;
using namespace khtml;
using namespace khtml::XPath;

namespace khtml {
namespace XPath {

struct AxisNameMapping
{
	const char *name;
	Step::AxisType type;
};

static AxisNameMapping axisNames[] = {
	{ "ancestor", Step::AncestorAxis },
	{ "ancestor-or-self", Step::AncestorOrSelfAxis },
	{ "attribute", Step::AttributeAxis },
	{ "child", Step::ChildAxis },
	{ "descendant", Step::DescendantAxis },
	{ "descendant-or-self", Step::DescendantOrSelfAxis },
	{ "following", Step::FollowingAxis },
	{ "following-sibling", Step::FollowingSiblingAxis },
	{ "namespace", Step::NamespaceAxis },
	{ "parent", Step::ParentAxis },
	{ "preceding", Step::PrecedingAxis },
	{ "preceding-sibling", Step::PrecedingSiblingAxis },
	{ "self", Step::SelfAxis }
};
static unsigned int axisNamesCount = sizeof(axisNames) / sizeof(axisNames[0]);

static const char* const nodeTypeNames[] = {
	"comment",
	"text",
	"processing-instruction",
	"node",
	0
};

QHash<QString, Step::AxisType>* Tokenizer::s_axisNamesDict     = 0;
QSet<QString>* Tokenizer::s_nodeTypeNamesDict = 0;

Tokenizer &Tokenizer::self()
{
	static Tokenizer instance;
	return instance;
}

Tokenizer::XMLCat Tokenizer::charCat(QChar aChar)
{
	//### might need to add some special cases from the XML spec.

	if (aChar.unicode() == '_')
		return NameStart;

	if (aChar.unicode() == '.' || aChar.unicode() == '-')
		return NameCont;

	switch (aChar.category()) {
		case QChar::Letter_Lowercase: //Ll
		case QChar::Letter_Uppercase: //Lu
		case QChar::Letter_Other:     //Lo
		case QChar::Letter_Titlecase: //Lt
		case QChar::Number_Letter:    //Nl
			return NameStart;

		case QChar::Mark_SpacingCombining: //Mc
		case QChar::Mark_Enclosing:        //Me
		case QChar::Mark_NonSpacing:       //Mn
		case QChar::Letter_Modifier:       //Lm
		case QChar::Number_DecimalDigit:   //Nd
			return NameCont;

		default:
			return NotPartOfName;
	}
}

bool Tokenizer::isAxisName(QString name, Step::AxisType *type)
{
	if (!s_axisNamesDict) {
		s_axisNamesDict = new QHash<QString, Step::AxisType>;
		for (unsigned int p = 0; p < axisNamesCount; ++p)
			s_axisNamesDict->insert(QLatin1String(axisNames[p].name),
			                        axisNames[p].type);
	}

	QHash<QString, Step::AxisType>::ConstIterator it = s_axisNamesDict->constFind(name);
	if ( it != s_axisNamesDict->constEnd() ) {
		*type = *it;
	}
	return it != s_axisNamesDict->constEnd();
}

bool Tokenizer::isNodeTypeName(QString name)
{
	if (!s_nodeTypeNamesDict) {
		s_nodeTypeNamesDict = new QSet<QString>;
		for (int p = 0; nodeTypeNames[p]; ++p)
			s_nodeTypeNamesDict->insert(QLatin1String(nodeTypeNames[p]));
	}
	return s_nodeTypeNamesDict->contains(name);
}

/* Returns whether the last parsed token matches the [32] Operator rule
 * (check http://www.w3.org/TR/xpath#exprlex). Necessary to disambiguate
 * the tokens.
 */
bool Tokenizer::isOperatorContext()
{
	if ( m_nextPos == 0 ) {
		return false;
	}

	switch ( m_lastTokenType ) {
		case AND: case OR: case MULOP:
		case '/': case SLASHSLASH: case '|': case PLUS: case MINUS:
		case EQOP: case RELOP:
		case '@': case AXISNAME:   case '(': case '[':
			return false;
		default:
			return true;
	}
}

void Tokenizer::skipWS()
{
	while (m_nextPos < m_data.length() && m_data[m_nextPos].isSpace())
		++m_nextPos;
}

Token Tokenizer::makeTokenAndAdvance(int code, int advance)
{
	m_nextPos += advance;
	return Token(code);
}

Token Tokenizer::makeIntTokenAndAdvance(int code, int val, int advance)
{
	m_nextPos += advance;
	return Token(code, val);
}

//Returns next char if it's there and interesting, 0 otherwise
char Tokenizer::peekAheadHelper()
{
	if (m_nextPos + 1 >= m_data.length())
		return 0;
	QChar next = m_data[m_nextPos + 1];
	if (next.row() != 0)
		return 0;
	else
		return next.cell();
}

char Tokenizer::peekCurHelper()
{
	if (m_nextPos >= m_data.length())
		return 0;
	QChar next = m_data[m_nextPos];
	if (next.row() != 0)
		return 0;
	else
		return next.cell();
}

Token Tokenizer::lexString()
{
	QChar delimiter = m_data[m_nextPos];
	int   startPos  = m_nextPos + 1;

	for (m_nextPos = startPos; m_nextPos < m_data.length(); ++m_nextPos) {
		if (m_data[m_nextPos] == delimiter) {
			QString value = m_data.mid(startPos, m_nextPos - startPos);
			++m_nextPos; //Consume the char;
			return Token(LITERAL, value);
		}
	}

	//Ouch, went off the end -- report error
	return Token(ERROR);
}

Token Tokenizer::lexNumber()
{
	int startPos = m_nextPos;
	bool seenDot = false;

	//Go until end or a non-digits character
	for (; m_nextPos < m_data.length(); ++m_nextPos) {
		QChar aChar = m_data[m_nextPos];
		if (aChar.row() != 0) break;

		if (aChar.cell() < '0' || aChar.cell() > '9') {
			if (aChar.cell() == '.' && !seenDot)
				seenDot = true;
			else
				break;
		}
	}

	QString value = m_data.mid(startPos, m_nextPos - startPos);
	return Token(NUMBER, value);
}

Token Tokenizer::lexNCName()
{
	int startPos = m_nextPos;
	if (m_nextPos < m_data.length() && charCat(m_data[m_nextPos]) == NameStart)
	{
		//Keep going until we get a character that's not good for names.
		for (; m_nextPos < m_data.length(); ++m_nextPos) {
			if (charCat(m_data[m_nextPos]) == NotPartOfName)
				break;
		}

		QString value = m_data.mid(startPos, m_nextPos - startPos);
		return Token(value);
	}
	else
		return makeTokenAndAdvance(ERROR);
}

Token Tokenizer::lexQName()
{
	Token t1 = lexNCName();
	if (t1.type == ERROR) return t1;
	skipWS();
	//If the next character is :, what we just got it the prefix, if not,
	//it's the whole thing
	if (peekAheadHelper() != ':')
		return t1;

	Token t2 = lexNCName();
	if (t2.type == ERROR) return t2;

	return Token(t1.value + ":" + t2.value);
}

Token Tokenizer::nextTokenInternal()
{
	skipWS();

	if (m_nextPos >= m_data.length()) {
		return Token(0);
	}

	char code = peekCurHelper();
	switch (code) {
		case '(': case ')': case '[': case ']':
		case '@': case ',': case '|':
			return makeTokenAndAdvance(code);
		case '\'':
		case '\"':
			return lexString();
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return lexNumber();
		case '.': {
			char next = peekAheadHelper();
			if (next == '.')
				return makeTokenAndAdvance(DOTDOT, 2);
			else if (next >= '0' && next <= '9')
				return lexNumber();
			else
				return makeTokenAndAdvance('.');
		}
		case '/':
			if (peekAheadHelper() == '/')
				return makeTokenAndAdvance(SLASHSLASH, 2);
			else
				return makeTokenAndAdvance('/');
		case '+':
			return makeTokenAndAdvance(PLUS);
		case '-':
			return makeTokenAndAdvance(MINUS);
		case '=':
			return makeIntTokenAndAdvance(EQOP, RelationOp::OP_EQ);
		case '!':
			if (peekAheadHelper() == '=')
				return makeIntTokenAndAdvance(EQOP, RelationOp::OP_NE, 2);
			else {
				return Token(ERROR);
			}
		case '<':
			if (peekAheadHelper() == '=')
				return makeIntTokenAndAdvance(RELOP, RelationOp::OP_LE, 2);
			else
				return makeIntTokenAndAdvance(RELOP, RelationOp::OP_LT);
		case '>':
			if (peekAheadHelper() == '=')
				return makeIntTokenAndAdvance(RELOP, RelationOp::OP_GE, 2);
			else
				return makeIntTokenAndAdvance(RELOP, RelationOp::OP_GT);
		case '*':
			if (isOperatorContext())
				return makeIntTokenAndAdvance(MULOP, NumericOp::OP_Mul);
			else {
				++m_nextPos;
				return Token(NAMETEST, "*");
			}
		case '$': {//$ QName
			m_nextPos++;
			Token par = lexQName();
			if (par.type == ERROR)
				return par;
			else
				return Token(VARIABLEREFERENCE, par.value);
		}
	}

	Token t1 = lexNCName();
	if (t1.type == ERROR) return t1;

	skipWS();

	//If we're in an operator context, check for any operator names
	if (isOperatorContext()) {
		if (t1.value == QLatin1String("and")) //### hash?
			return Token(AND);
		if (t1.value == QLatin1String("or"))
			return Token(OR);
		if (t1.value == QLatin1String("mod"))
			return Token(MULOP, NumericOp::OP_Mod);
		if (t1.value == QLatin1String("div"))
			return Token(MULOP, NumericOp::OP_Div);
	}

	//See whether we are at a :
	if (peekCurHelper() == ':') {
		m_nextPos++;
		//Any chance it's an axis name?
		if (peekCurHelper() == ':') {
			m_nextPos++;

			//It might be an axis name.
			Step::AxisType axisType;
			if (isAxisName(t1.value, &axisType))
				return Token(AXISNAME, axisType);
			//Ugh, :: is only valid in axis names -> error
			return Token(ERROR);
		}

		//Seems like this is a fully qualified qname, or perhaps the * modified one from NameTest
		skipWS();
		if (peekCurHelper() == '*') {
			m_nextPos++;
			return Token(NAMETEST, t1.value + ":*");
		}

		//Make a full qname..
		Token t2 = lexNCName();
		if (t2.type == ERROR) return t2;

		t1.value = t1.value + ':' + t2.value;
	}

	skipWS();
	if (peekCurHelper() == '(') {
		//note: we don't swallow the ( here!

		//either node type of function name
		if (isNodeTypeName(t1.value)) {
			if (t1.value == "processing-instruction")
				return Token(PI, t1.value);
			else
				return Token(NODETYPE, t1.value);
		}
		//must be a function name.
		return Token(FUNCTIONNAME, t1.value);
	}

	//At this point, it must be NAMETEST
	return Token(NAMETEST, t1.value);
}

Token Tokenizer::nextToken()
{
	Token toRet = nextTokenInternal();
	m_lastTokenType = toRet.type;
	return toRet;
}

Tokenizer::Tokenizer()
{
	reset(QString());
}

Tokenizer::~Tokenizer()
{
	delete s_axisNamesDict;
	delete s_nodeTypeNamesDict;
}

void Tokenizer::reset(QString data)
{
	m_nextPos = 0;
	m_data = data;
	m_lastTokenType = 0;
}

int khtmlxpathyylex()
{
	Token tok = Tokenizer::self().nextToken();
	if (tok.hasString) {
		khtmlxpathyylval.str = new DOMString(tok.value);
	} else if (tok.intValue) {
		khtmlxpathyylval.num = tok.intValue;
	}
	return tok.type;
}

void initTokenizer(const DOM::DOMString& string)
{
	Tokenizer::self().reset(string.string());
}

} // namespace XPath
} // namespace khtml

// kate: indent-width 4; replace-tabs off; tab-width 4; indent-spaces: off;
