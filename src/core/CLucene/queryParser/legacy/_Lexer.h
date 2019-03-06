/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Updated by https://github.com/farfella/.
 Updated by https://github.com/farfella/.
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_queryParser_legacy_Lexer_
#define _lucene_queryParser_legacy_Lexer_


CL_CLASS_DEF(util, FastCharStream)
CL_CLASS_DEF(util,BufferedReader)
CL_CLASS_DEF(util,StringBuffer)

//#include "TokenList.h"
class QueryToken;
class TokenList;
class QueryParserBase;

CL_NS_DEF2(queryParser,legacy)

	// A simple Lexer that is used by QueryParser.
	class Lexer:LUCENE_BASE
	{
	private:
		CL_NS(util)::FastCharStream* reader;
		QueryParserBase* queryparser; //holds the queryparser so that we can do callbacks
		bool delSR;  //Indicates if the reader must be deleted or not

    public:
		// Initializes a new instance of the Lexer class with the specified
		// query to lex.
		Lexer(QueryParserBase* queryparser, const wchar_t* query);

		// Initializes a new instance of the Lexer class with the specified
		// TextReader to lex.
		Lexer(QueryParserBase* queryparser, CL_NS(util)::BufferedReader* source);

		//Breaks the input stream onto the tokens list tokens
		void Lex(TokenList *tokens);
		
		~Lexer();

	private:
		bool GetNextToken(QueryToken* token);

		// Reads an integer number. buf should quite large, probably as large as a field should ever be
		void ReadIntegerNumber(const wchar_t ch, wchar_t* buf, int buflen);

		// Reads an inclusive range like [some words]
		bool ReadInclusiveRange(const wchar_t prev, QueryToken* token);

		// Reads an exclusive range like {some words}
		bool ReadExclusiveRange(const wchar_t prev, QueryToken* token);

		// Reads quoted string like "something else"
		bool ReadQuoted(const wchar_t prev, QueryToken* token);

		bool ReadTerm(const wchar_t prev, QueryToken* token);

        //reads an escaped character into the buf. Buf requires at least 3 characters
		bool ReadEscape(const wchar_t prev, wchar_t* buf);
	};
CL_NS_END2
#endif
